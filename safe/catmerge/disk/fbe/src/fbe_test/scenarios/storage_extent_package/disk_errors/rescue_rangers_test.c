
/***************************************************************************
 * Copyright (C) EMC Corporation 2011 -2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

 /*!**************************************************************************
 * @file rescue_rangers_test.c
 ***************************************************************************
 *
 * @brief
 *   Tests that monitor does not get stuck due to stripe locks 
 *   when transitioning to ACTIVATE.
 *
 * @version
 *  4/22/2013  Deanna Heng  - Created
 *
 ***************************************************************************/

/*-----------------------------------------------------------------------------
 *   INCLUDES OF REQUIRED HEADER FILES:
 */
#include "mut.h"
#include "sep_tests.h"
#include "sep_test_io.h"
#include "neit_utils.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "sep_event.h"
#include "sep_hook.h"
#include "sep_utils.h"
#include "fbe_database.h"
#include "fbe_spare.h"
#include "fbe_test_configurations.h"
#include "fbe_job_service.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_test_common_utils.h"
#include "pp_utils.h"
#include "sep_rebuild_utils.h"                     
#include "sep_test_background_ops.h"
#include "fbe/fbe_random.h"
#include "fbe_test.h"


/*-----------------------------------------------------------------------------
 *    TEST DESCRIPTION:
 */
 char * rescue_rangers_short_desc = 
     "Tests that monitor does not get stuck due to stripe locks when transitioning to ACTIVATE.";
 char * rescue_rangers_long_desc =
     "\n"
     "\n"
     "The Rescue Rangers verifies that a monitor operation does not get stuck behind a user io\n"
     "that held a stripe lock in the user area when transitioning to ACTIVATE.\n"
     "\n"  
     "1. Insert a rebuild hook on the active sp at checkpoint 0.\n"
     "2. Remove 2 drives from the Raid 6 raid group.\n"
     "3. Reinsert one of the removed drives and wait for the rebuild hook.\n"
     "   Note: this will cause write verify and rebuild to occur simultaneously.\n"
     "4. Setup an lei record to delay IO on the passive SP.\n"
     "5. Start rdgen io on the passive SP. The ios will be delayed\n"
     "6. Start rdgen io on the active SP. The ios will be in contention for the\n"
     "   same paged area as in the passive SP\n"
     "7. Remove the rebuild hook after a couple seconds. This will cause the stripe\n"
     "   lock for the rebuild IO to be queued behind the user IO\n"
     "8. Put the raid group into ACTIVATE on the passive SP. \n"
     "   Note: The active SP will go through pending activate and all stripe locks \n"
     "   and monitor operations should get aborted appropriately to allow the monitor\n"
     "   to run.\n"
     "9. Wait for the active and passive SP to transition into ACTIVATE.\n"
     "10. Wait for the rebuild to complete.\n"
     "\n";

/*-----------------------------------------------------------------------------
 *    DEFINITIONS:
 */

/*!*******************************************************************
 * @def RESCUE_RANGERS_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define RESCUE_RANGERS_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def RESCUE_RANGERS_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define RESCUE_RANGERS_CHUNKS_PER_LUN 10

/*!*******************************************************************
 * @def RESCUE_RANGERS_NUM_RDGEN_CONTEXTS
 *********************************************************************
 * @brief number of raid groups to test with
 *
 *********************************************************************/
#define RESCUE_RANGERS_NUM_RDGEN_CONTEXTS                     2

/*!*******************************************************************
 * @def RESCUE_RANGERS_WAIT_MSEC
 *********************************************************************
 * @brief max number of millseconds we will wait.
 *        The general idea is that waits should be brief.
 *        We put in an overall wait time to make sure the test does not get stuck.
 *
 *********************************************************************/
#define RESCUE_RANGERS_WAIT_MSEC 60000

 /* General LEI Record to create to delay the IO going up the stack
  */
fbe_api_logical_error_injection_record_t rescue_rangers_record_up =        
              { 0x1,  /* pos_bitmap */
                0x10, /* width */
                0x0,  /* lba */
                FBE_LBA_INVALID,  /* blocks */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_UP,      /* error type */
                FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0,   /* error count */
                0x8000, /* error limit is msecs to delay */
                0x0,  /* skip count */
                0x15, /* skip limit */
                0x1,  /* error adjcency */
                0x0,  /* start bit */
                0x0,  /* number of bits */
                0x0,  /* erroneous bits */
                0x0, /* crc detectable */
                0};

/* protocol error injection handle 
 */
fbe_protocol_error_injection_record_handle_t protocol_error_handles;

/* RDGEN test context. 
 */
fbe_api_rdgen_context_t rescue_rangers_rdgen_context[RESCUE_RANGERS_NUM_RDGEN_CONTEXTS];

     
/*!*******************************************************************
 * @var rescue_rangers_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t rescue_rangers_raid_group_config[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {10,        0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            0,          0},
    {6,         0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            1,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

fbe_test_rg_configuration_t tmp_rescue_rangers_raid_group_config[3];

/*-----------------------------------------------------------------------------
 *  FORWARD DECLARATIONS:
 */
void rescue_rangers_setup_tracing(fbe_sim_transport_connection_target_t active_sp,
                                  fbe_test_rg_configuration_t *rg_config_p);
void rescue_rangers_set_trace_level(fbe_trace_type_t trace_type, fbe_u32_t id, fbe_trace_level_t level);
void rescue_rangers_start_rdgen_io(fbe_sim_transport_connection_target_t target_sp,
                                   fbe_api_rdgen_context_t *rdgen_context_p,
                                   fbe_object_id_t rg_object_id, 
                                   fbe_u32_t lba);
void rescue_rangers_calculate_drive_positions(fbe_object_id_t rg_object_id,
                                              fbe_u32_t	* drive_pos_to_remove,
                                              fbe_u32_t	* drive_pos_to_remove_2,
                                              fbe_u16_t * delay_io_pos_bitmap);
void rescue_rangers_setup_delay_io(fbe_sim_transport_connection_target_t target_sp,
                                   fbe_test_rg_configuration_t *rg_config_p);
void rescue_rangers_setup_io_error(fbe_sim_transport_connection_target_t target_sp,
                                   fbe_test_rg_configuration_t *rg_config_p);
void rescue_rangers_abort_sl_activate_test(fbe_test_rg_configuration_t *rg_config_p);
void rescue_rangers_abort_sl_reboot_test(fbe_test_rg_configuration_t *rg_config_p);
void rescue_rangers_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p);

/*!**************************************************************
 * rescue_rangers_setup_tracing()
 ****************************************************************
 * @brief
 *  setup extra tracing
 *
 * @param active_sp sp to setup tracing
 * @param rg_config_p object for extra tracing
 *
 * @return None.
 *
 * @author
 *  4/22/2013 - Created. Deanna Heng
 *
 ****************************************************************/
void rescue_rangers_setup_tracing(fbe_sim_transport_connection_target_t active_sp,
                                  fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                            status;
    fbe_object_id_t                         rg_object_id;
   
    fbe_api_sim_transport_set_target_server(active_sp);

    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                 &rg_object_id);

    fbe_test_sep_util_set_rg_trace_level_both_sps(rg_config_p, FBE_TRACE_LEVEL_DEBUG_HIGH);
    rescue_rangers_set_trace_level(FBE_TRACE_TYPE_LIB, FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_DEBUG_HIGH);
    /* set library debug flags 
     */
    status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, 
                                                        (FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING | 
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING |
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING |
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_TRACING));
    /* set raid group debug flags 
     */
    fbe_test_sep_util_set_rg_debug_flags_both_sps(rg_config_p, 
                                                  FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING |
                                                  FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING |
                                                  FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING |
                                                  FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING |
                                                  FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING);
    fbe_test_sep_util_set_service_trace_flags(FBE_TRACE_TYPE_SERVICE, FBE_SERVICE_ID_METADATA, FBE_PACKAGE_ID_SEP_0, FBE_TRACE_LEVEL_DEBUG_HIGH);
}    
/**************************************
 * end rescue_rangers_setup_tracing()
 **************************************/

/*!**************************************************************
 * rescue_rangers_set_trace_level()
 ****************************************************************
 * @brief
 *  set trace level
 *
 * @param trace_type type of tracing
 * @param id object id
 * @param level level of tracing
 *
 * @return None.
 *
 * @author
 *  4/22/2013 - Created. Deanna Heng
 *
 ****************************************************************/
void rescue_rangers_set_trace_level(fbe_trace_type_t trace_type, fbe_u32_t id, fbe_trace_level_t level)
{
    fbe_api_trace_level_control_t level_control;
    fbe_status_t status;

    level_control.trace_type = trace_type;
    level_control.fbe_id = id;
    level_control.trace_level = level;
    status = fbe_api_trace_set_level(&level_control, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/**************************************
 * end rescue_rangers_set_trace_level()
 **************************************/

/*!**************************************************************
 * rescue_rangers_start_io()
 ****************************************************************
 * @brief
 *  Start io to the target raid group
 *
 * @param target_sp - sp to run io from
 * @param rdgen_context_p - rdgen context
 * @param rg_object_id - target raidgroup
 * @param lba - target lba to run io
 *
 * @return None.
 *
 * @author
 *  4/22/2013 - Created. Deanna Heng
 *
 ****************************************************************/
void rescue_rangers_start_rdgen_io(fbe_sim_transport_connection_target_t target_sp,
                                   fbe_api_rdgen_context_t *rdgen_context_p,
                                   fbe_object_id_t rg_object_id, 
                                   fbe_u32_t lba) 
{
    fbe_status_t                            status;

    /* ACTIVE: Write a background pattern to the LUNs.
     */
    fbe_api_sim_transport_set_target_server(target_sp);
    status = fbe_api_rdgen_test_context_init(rdgen_context_p, 
                                             rg_object_id, 
                                             FBE_CLASS_ID_INVALID,
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_WRITE_ONLY,
                                             FBE_RDGEN_PATTERN_LBA_PASS,    /* Standard rdgen data pattern */
                                             0,                             /* run forever */
                                             0,                             /* io count not used */
                                             0,
                                             1,                             /* num threads */
                                             FBE_RDGEN_LBA_SPEC_FIXED,     /* Standard I/O mode is random */
                                             0,                             /* Start lba*/
                                             lba,                             /* Min lba */
                                             lba+1,                      /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,   /* Standard I/O transfer size is random */
                                             1,                             /* Min blocks per request */
                                             1                 /* Max blocks per request */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Start IO to LUN on SP %d ==", __FUNCTION__, target_sp);
    status = fbe_api_rdgen_start_tests(rdgen_context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/**************************************
 * end rescue_rangers_start_io()
 **************************************/

/*!**************************************************************
 * rescue_rangers_calculate_drive_positions()
 ****************************************************************
 * @brief
 *  Determine the target drive to delay io,
 *
 * @param rg_object_id - target raid group
 * @param drive_pos_to_remove - drive to run rebuild on
 * @param drive_pos_to_remove - drive to keep in rl
 * @param delay_io_pos_bitmap - bitmap of position to delay io on
 *
 * @return None.
 *
 * @author
 *  4/22/2013 - Created. Deanna Heng
 *
 ****************************************************************/
void rescue_rangers_calculate_drive_positions(fbe_object_id_t rg_object_id,
                                              fbe_u32_t	* drive_pos_to_remove,
                                              fbe_u32_t	* drive_pos_to_remove_2,
                                              fbe_u16_t * delay_io_pos_bitmap) 
{
    fbe_status_t                        status;
    fbe_u32_t	                        drive_pos_to_delay_io;
    fbe_api_raid_group_get_info_t       rg_info;
    fbe_raid_group_map_info_t           rg_map_info;
    fbe_u32_t                           i = 0;

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                         FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* a smarter way to do this would be to calculate the metadata lba
     * based on the target lba which should be passed in
     */
    rg_map_info.lba = rg_info.paged_metadata_start_lba;
    status = fbe_api_raid_group_map_lba(rg_object_id, &rg_map_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    drive_pos_to_delay_io = rg_map_info.data_pos;

    /* calculate which positions need to be removed
     */
    if (drive_pos_to_delay_io < rg_info.width - 1) 
    {
        *drive_pos_to_remove = drive_pos_to_delay_io + 1;
    } else {
        *drive_pos_to_remove = drive_pos_to_delay_io - 1;
    }
    for (i=0; i< rg_info.width; i++) 
    {
        if (drive_pos_to_delay_io != i && *drive_pos_to_remove != i) 
        {
            *drive_pos_to_remove_2 = i;
            break;
        }
    }

    /* position bitmap = 1*2^drive_pos_to_delay_io
     */
    *delay_io_pos_bitmap = 1 << drive_pos_to_delay_io;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Pos to delay: %d bitmap: %d drive toremove: %d 2nd drive: %d metalba: 0x%llx==", 
               __FUNCTION__, drive_pos_to_delay_io, *delay_io_pos_bitmap, 
               *drive_pos_to_remove, *drive_pos_to_remove_2, rg_map_info.lba);

}
/**************************************
 * end rescue_rangers_calculate_drive_positions()
 **************************************/

/*!**************************************************************
 * rescue_rangers_setup_delay_io()
 ****************************************************************
 * @brief
 *  Setup the lei record to delay io on the raid group
 *
 * @param target_sp - sp to delay io on
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  4/22/2013 - Created. Deanna Heng
 *
 ****************************************************************/
void rescue_rangers_setup_delay_io(fbe_sim_transport_connection_target_t target_sp,
                                   fbe_test_rg_configuration_t *rg_config_p) 
{
    fbe_status_t                        status;

    fbe_api_sim_transport_set_target_server(target_sp);
    status = fbe_api_logical_error_injection_disable_records(0, 128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_create_record(&rescue_rangers_record_up);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable injection on all raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_enable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/**************************************
 * end rescue_rangers_setup_delay_io()
 **************************************/

/*!**************************************************************
 * rescue_rangers_setup_io_error()
 ****************************************************************
 * @brief
 *  Setup the lei record to delay io on the raid group
 *
 * @param target_sp - sp to delay io on
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *
 ****************************************************************/
void rescue_rangers_setup_io_error(fbe_sim_transport_connection_target_t target_sp,
                                   fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_protocol_error_injection_error_record_t     record;
    fbe_protocol_error_injection_record_handle_t    record_handle_p;


    fbe_api_sim_transport_set_target_server(target_sp);

    /* Invoke the method to initialize and populate the (1) error record to position 1, which is the disk for the metadata access.
     */
    status = fbe_test_neit_utils_populate_protocol_error_injection_record(rg_config_p->rg_disk_set[1].bus,
                                                                          rg_config_p->rg_disk_set[1].enclosure,
                                                                          rg_config_p->rg_disk_set[1].slot,
                                                                          &record,
                                                                          &record_handle_p,
                                                                          0x0,
                                                                          0x800000,
                                                                          FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI,
                                                                          FBE_SCSI_READ_16,
                                                                          FBE_SCSI_SENSE_KEY_UNIT_ATTENTION,
                                                                          FBE_SCSI_ASC_MODE_PARAM_CHANGED,
                                                                          FBE_SCSI_ASCQ_GENERAL_QUALIFIER,
                                                                          FBE_PORT_REQUEST_STATUS_SUCCESS, 
                                                                          1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add the error injection record to the service, which also returns the record handle for later use */
    status = fbe_api_protocol_error_injection_add_record(&record, &record_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);  

    /* Start the error injection */
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

    return;
}
/**************************************
 * end rescue_rangers_setup_delay_io()
 **************************************/

/*!**************************************************************
 * rescue_rangers_abort_sl_activate_test()
 ****************************************************************
 * @brief
 *  Run rescue rangers tests for transitioning to ACTIVATE
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  4/22/2013 - Created. Deanna Heng
 *
 ****************************************************************/
void rescue_rangers_abort_sl_activate_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_api_rdgen_context_t *context_passive_p = &rescue_rangers_rdgen_context[0];
    fbe_api_rdgen_context_t *context_active_p = &rescue_rangers_rdgen_context[1];
    fbe_status_t                            status;
    fbe_object_id_t                         rg_object_id;
    fbe_u32_t           				    number_physical_objects = 0;
	fbe_u32_t							    drive_pos_to_remove = 1;
    fbe_u32_t							    drive_pos_to_remove_2 = 2;
    fbe_api_terminator_device_handle_t      drive_info;
	fbe_api_terminator_device_handle_t      drive_info_2;
    fbe_sim_transport_connection_target_t   active_sp = FBE_SIM_SP_A;               
    fbe_sim_transport_connection_target_t   passive_sp = FBE_SIM_SP_B;   
     fbe_test_common_util_lifecycle_state_ns_context_t ns_context;

    /* This test only runs for raid 6 
     */
    if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID6) {
        return;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting abort stripe locks during PENDING ACTIVATE test ==", __FUNCTION__);

    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                 &rg_object_id);

    /* get the positions in which we want to remove and delay io 
     */
    rescue_rangers_calculate_drive_positions(rg_object_id, 
                                             &drive_pos_to_remove, // allow rebuild to run on this position
                                             &drive_pos_to_remove_2, // keep this position in rl
                                             &rescue_rangers_record_up.pos_bitmap); // bitmap of position to delay io

    /* determine the active and passive sps fo the rg object 
     */
    fbe_test_sep_util_get_active_passive_sp(rg_object_id, &active_sp, &passive_sp);
    rescue_rangers_setup_tracing(active_sp, rg_config_p);

    /* 1. insert a debug hook to pause the rebuild 
     */
    fbe_api_sim_transport_set_target_server(active_sp);
    fbe_test_sep_util_update_permanent_spare_trigger_timer(10000000000000000); /* hotspare timeout */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Add the rebuild hook on Active SP ==", __FUNCTION__);
    status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                              0,
                                              NULL,
                                              SCHEDULER_CHECK_VALS_LT,
                                              SCHEDULER_DEBUG_ACTION_PAUSE); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* 2. Remove 2 drives in the target raid group 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove a drive %d ==", __FUNCTION__, drive_pos_to_remove_2);
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_remove_2, number_physical_objects, &drive_info_2);        

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove a drive %d ==", __FUNCTION__, drive_pos_to_remove);
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_remove, number_physical_objects, &drive_info);        

    /* write a background pattern to the luns so that there is something to rebuild 
     */
    big_bird_write_background_pattern();

    fbe_api_sim_transport_set_target_server(active_sp);
    /* 3. Reinsert the first drive to fail and verify drive state changes 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reinsert the removed drive %d ==", __FUNCTION__, drive_pos_to_remove);
    number_physical_objects += 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_pos_to_remove, number_physical_objects, &drive_info);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for the rebuild hook on Active SP ==", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(active_sp);
    status = fbe_test_wait_for_debug_hook(rg_object_id, 
                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                         FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                         SCHEDULER_CHECK_VALS_LT, 
                                         SCHEDULER_DEBUG_ACTION_PAUSE, 
                                         0,
                                         NULL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* 4. Add lei record to delay io on the passive sp 
     */
    rescue_rangers_setup_delay_io(passive_sp, rg_config_p);

    /* 5. PASSIVE: Write a background pattern to the LUNs. 
     */
    rescue_rangers_start_rdgen_io(passive_sp, context_passive_p, rg_object_id, 0);
    fbe_api_sleep(2000);

    /* 6. ACTIVE: Write a background pattern to the LUNs. 
     */
    rescue_rangers_start_rdgen_io(active_sp, context_active_p, rg_object_id, 1);
    fbe_api_sleep(2000);

    /* 7. Delete the rebuild hook 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove the rebuild hook on Active SP ==", __FUNCTION__);
    status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                              0, 
                                              NULL, 
                                              SCHEDULER_CHECK_VALS_LT, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_sleep(2000);

    /* set up the notification for activate on the passive sp */
    fbe_test_common_util_register_lifecycle_state_notification(&ns_context, 
                                                               FBE_PACKAGE_NOTIFICATION_ID_SEP_0,
                                                               FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP,
                                                               rg_object_id,
                                                               FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE);

    /* 8. Set the rg lifecycle state into activate on the passive sp 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set the Raid group into Activate on Passive SP ==", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(passive_sp);
    status = fbe_api_set_object_to_state(rg_object_id, FBE_LIFECYCLE_STATE_ACTIVATE, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for the raid group to get to the expected state.  The notification was
     * initialized above. 
     */
    fbe_test_common_util_wait_lifecycle_state_notification(&ns_context, RESCUE_RANGERS_WAIT_MSEC);
    fbe_test_common_util_unregister_lifecycle_state_notification(&ns_context);

    /* Stop logical error injection. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* 9. Set the target server to the active sp and wait for the lifecycle to be ready 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for the object to become ready on active sp ==", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(active_sp);
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, RESCUE_RANGERS_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for object READY failed on active sp");

    /* wait for the lifecycle state of the passive SP 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for the object to become ready on passive sp ==", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(passive_sp);
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, RESCUE_RANGERS_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for object READY failed on passive sp");

    /* 10. Wait for the rebuilds to complete 
     */
    fbe_api_sim_transport_set_target_server(active_sp);
    /* Wait for rebuild to start and complete
     */
    sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, drive_pos_to_remove);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos_to_remove);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reinsert the first removed drive ==", __FUNCTION__);
    number_physical_objects += 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_pos_to_remove_2, number_physical_objects, &drive_info_2);
    /* wait until raid completes rebuild to the 2nd drive. 
     */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos_to_remove_2 );

    fbe_api_sim_transport_set_target_server(passive_sp);
    status = fbe_api_rdgen_stop_tests(context_passive_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    fbe_api_sim_transport_set_target_server(active_sp);
    status = fbe_api_rdgen_stop_tests(context_active_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    return;
    
}
/**************************************
 * end rescue_rangers_abort_sl_activate_test()
 **************************************/

/*!**************************************************************
 * rescue_rangers_abort_sl_reboot_test()
 ****************************************************************
 * @brief
 *  heavy IO to degraded raid group and peer lost
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *
 ****************************************************************/
void rescue_rangers_abort_sl_reboot_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_api_rdgen_context_t *context_passive_p = &rescue_rangers_rdgen_context[0];
    fbe_api_rdgen_context_t *context_active_p = &rescue_rangers_rdgen_context[1];
    fbe_status_t                            status;
    fbe_object_id_t                         rg_object_id;
    fbe_u32_t           				    number_physical_objects = 0;
	fbe_u32_t							    drive_pos_to_remove = 1;
    fbe_u32_t							    drive_pos_to_remove_2 = 2;
    fbe_api_terminator_device_handle_t      drive_info;
	fbe_api_terminator_device_handle_t      drive_info_2;
    fbe_sim_transport_connection_target_t   active_sp = FBE_SIM_SP_A;               
    fbe_sim_transport_connection_target_t   passive_sp = FBE_SIM_SP_B;   
    fbe_u32_t                               raid_group_count = 0;
    fbe_u32_t num_raid_groups = 0;


    fbe_test_sep_duplicate_config(&rescue_rangers_raid_group_config[0], &tmp_rescue_rangers_raid_group_config[0]);
    fbe_test_sep_util_init_rg_configuration_array(tmp_rescue_rangers_raid_group_config);
    num_raid_groups = fbe_test_get_rg_array_length(tmp_rescue_rangers_raid_group_config);    

    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* This test only runs for raid 6 
     */
    if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID6) {
        return;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting abort stripe locks during PENDING ACTIVATE test ==", __FUNCTION__);

    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                 &rg_object_id);

    /* get the positions in which we want to remove and delay io 
     */
    rescue_rangers_calculate_drive_positions(rg_object_id, 
                                             &drive_pos_to_remove, // allow rebuild to run on this position
                                             &drive_pos_to_remove_2, // keep this position in rl
                                             &rescue_rangers_record_up.pos_bitmap); // bitmap of position to delay io

    /* determine the active and passive sps fo the rg object 
     */
    fbe_test_sep_util_get_active_passive_sp(rg_object_id, &active_sp, &passive_sp);
    rescue_rangers_setup_tracing(active_sp, rg_config_p);

    /* 1. insert a debug hook to pause the rebuild 
     */
    fbe_api_sim_transport_set_target_server(active_sp);
    fbe_test_sep_util_update_permanent_spare_trigger_timer(10000000000000000); /* hotspare timeout */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Add the rebuild hook on Active SP ==", __FUNCTION__);
    status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                              0,
                                              NULL,
                                              SCHEDULER_CHECK_VALS_LT,
                                              SCHEDULER_DEBUG_ACTION_PAUSE); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* 2. Remove 2 drives in the target raid group 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove a drive %d ==", __FUNCTION__, drive_pos_to_remove_2);
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_remove_2, number_physical_objects, &drive_info_2);        

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove a drive %d ==", __FUNCTION__, drive_pos_to_remove);
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_remove, number_physical_objects, &drive_info);        

    /* write a background pattern to the luns so that there is something to rebuild 
     */
    big_bird_write_background_pattern();

    fbe_api_sim_transport_set_target_server(active_sp);
    /* 3. Reinsert the first drive to fail and verify drive state changes 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reinsert the removed drive %d ==", __FUNCTION__, drive_pos_to_remove);
    number_physical_objects += 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_pos_to_remove, number_physical_objects, &drive_info);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for the rebuild hook on Active SP ==", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(active_sp);
    status = fbe_test_wait_for_debug_hook(rg_object_id, 
                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                         FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                         SCHEDULER_CHECK_VALS_LT, 
                                         SCHEDULER_DEBUG_ACTION_PAUSE, 
                                         0,
                                         NULL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* 4. Add lei record to delay io on the passive sp 
     */
    rescue_rangers_setup_delay_io(passive_sp, rg_config_p);
    rescue_rangers_setup_io_error(passive_sp, rg_config_p);

    /* 5. PASSIVE: Write a background pattern to the LUNs. 
     */
    rescue_rangers_start_rdgen_io(passive_sp, context_passive_p, rg_object_id, 0);
    fbe_api_sleep(2000);

    /* 6. ACTIVE: Write a background pattern to the LUNs. 
     */
    rescue_rangers_start_rdgen_io(active_sp, context_active_p, rg_object_id, 1);
    fbe_api_sleep(2000);

    /* 7. Delete the rebuild hook 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove the rebuild hook on Active SP ==", __FUNCTION__);
    status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                              0, 
                                              NULL, 
                                              SCHEDULER_CHECK_VALS_LT, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_sleep(2000);
    
    /* now that passive sp had an outstanding IO holding metadata stripe lock
     * let's shutdown active sp, and passive side should abort this metadata operation
     * and retrid it after journal flush completes
     */

    /* 8. shutdown active sp 
     */
    fbe_api_sim_transport_set_target_server(active_sp);

    mut_printf(MUT_LOG_LOW, " == SHUTDOWN active sp %s == ", active_sp == FBE_SIM_SP_A ? "SP A"
                                                                      : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA
                                                      : FBE_TEST_SPB);

    /* we can't destroy fbe_api, because it's shared between two SPs */
    fbe_test_base_suite_startup_single_sp(active_sp);

    /* Stop logical error injection. 
     */
    fbe_api_sim_transport_set_target_server(passive_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sim_transport_set_target_server(passive_sp);
    status = fbe_api_rdgen_stop_tests(context_passive_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful on passive ==", __FUNCTION__);

    /* stop protocol error injection
     */
    fbe_api_sim_transport_set_target_server(passive_sp);
    status = fbe_api_protocol_error_injection_stop();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sim_transport_set_target_server(passive_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reinsert the first removed drive ==", __FUNCTION__);
    number_physical_objects += 1;
    fbe_test_pp_util_reinsert_drive(rg_config_p->rg_disk_set[drive_pos_to_remove_2].bus,
                                    rg_config_p->rg_disk_set[drive_pos_to_remove_2].enclosure,
                                    rg_config_p->rg_disk_set[drive_pos_to_remove_2].slot,
                                    drive_info_2);
    fbe_test_pp_util_verify_pdo_state(rg_config_p->rg_disk_set[drive_pos_to_remove_2].bus,
                                    rg_config_p->rg_disk_set[drive_pos_to_remove_2].enclosure,
                                    rg_config_p->rg_disk_set[drive_pos_to_remove_2].slot,
                                    FBE_LIFECYCLE_STATE_READY, SEP_REBUILD_UTILS_WAIT_MSEC);

    //  Verify that the PVD and VD objects are in the READY state
    fbe_test_sep_drive_verify_pvd_state(rg_config_p->rg_disk_set[drive_pos_to_remove_2].bus,
                                        rg_config_p->rg_disk_set[drive_pos_to_remove_2].enclosure,
                                        rg_config_p->rg_disk_set[drive_pos_to_remove_2].slot, 
                                        FBE_LIFECYCLE_STATE_READY, SEP_REBUILD_UTILS_WAIT_MSEC);


    /* start active sp 
     */
    fbe_api_sim_transport_set_target_server(active_sp);
    elmo_create_physical_config_for_rg(tmp_rescue_rangers_raid_group_config, num_raid_groups);
    sep_config_load_sep_and_neit();

    /* 9. Set the target server to the active sp and wait for the lifecycle to be ready 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for the object to become ready on active sp ==", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(active_sp);
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, RESCUE_RANGERS_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for object READY failed on active sp");

    /* wait for the lifecycle state of the passive SP 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for the object to become ready on passive sp ==", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(passive_sp);
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, RESCUE_RANGERS_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for object READY failed on passive sp");

    /* 10. Wait for the rebuilds to complete 
     */
    fbe_api_sim_transport_set_target_server(active_sp);
    /* Wait for rebuild to start and complete
     */
    sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, drive_pos_to_remove);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos_to_remove);

    /* wait until raid completes rebuild to the 2nd drive. 
     */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos_to_remove_2 );


    fbe_api_rdgen_test_context_destroy(context_active_p);

    return;
    
}
/**************************************
 * end rescue_rangers_abort_sl_reboot_test()
 **************************************/

/*!**************************************************************
 * rescue_rangers_run_tests()
 ****************************************************************
 * @brief
 *  Run rescue rangers tests for transitioning to ACTIVATE
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 * @param context_p - Not used.
 *
 * @return None.
 *
 * @author
 *  4/22/2013 - Created. Deanna Heng
 *
 ****************************************************************/

void rescue_rangers_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_u32_t                   raid_group_count = 0;
    fbe_u32_t                   index = 0;
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    for(index = 0; index < raid_group_count; index++)
    {
        /* Test that monitor does not get stuck due to stripe locks 
         */
        rescue_rangers_abort_sl_activate_test(rg_config_p);
        /* heavy IO to degraded raid group and peer lost
         */
        rescue_rangers_abort_sl_reboot_test(rg_config_p);
        rg_config_p++;
    }

    return;
}
/**************************************
 * end rescue_rangers_run_tests()
 **************************************/

 /*!**************************************************************
 * rescue_rangers_test()
 ****************************************************************
 * @brief
 *  Create the config and run tests for transitioning to ACTIVATE
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  4/22/2013 - Created. Deanna Heng
 *
 ****************************************************************/

void rescue_rangers_dualsp_test(void)
{
    
    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_test_run_test_on_rg_config(&rescue_rangers_raid_group_config[0], 
                                   NULL, 
                                   rescue_rangers_run_tests,
                                   RESCUE_RANGERS_LUNS_PER_RAID_GROUP,
                                   RESCUE_RANGERS_CHUNKS_PER_LUN);

    /* Always clear dualsp mode 
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end rescue_rangers_dualsp_test()
 ******************************************/

/*!**************************************************************
 * rescue_rangers_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for rescue rangers test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  4/22/2013 - Created. Deanna Heng
 *
 ****************************************************************/
void rescue_rangers_dualsp_setup(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t                   raid_group_count = 0;

    if (fbe_test_util_is_simulation()) 
    {
        rg_config_p = &rescue_rangers_raid_group_config[0];
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        /* Initialize the raid group configuration 
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        /* Setup the physical config for the raid groups on both SPs
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, raid_group_count);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, raid_group_count);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_load_balance_both_sps();
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
    return;
}
/******************************************
 * end rescue_rangers_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * rescue_rangers_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the rescue rangers test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  4/22/2013 - Created. Deanna Heng
 *
 ****************************************************************/

void rescue_rangers_dualsp_cleanup(void) 
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    if (fbe_test_util_is_simulation()) 
    {
        /* First execute teardown on SP B then on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();

        /* First execute teardown on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end rescue_rangers_dualsp_cleanup()
 ******************************************/

 /*************************
 * end file rescue_rangers_test.c
 *************************/
