/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file red_riding_hood.c
 ***************************************************************************
 *
 * @brief
 *   This file test Non-paged metadata error handling for System and non-system
 *   objects .  
 *
 * @author
 *  04/20/2012  Y. Zhang    
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:


#include "mut.h"   
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_utils.h"
#include "sep_utils.h"
#include "sep_hook.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_metadata_interface.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "sep_tests.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"                
#include "fbe_private_space_layout.h"               
#include "sep_rebuild_utils.h"    
#include "sep_test_background_ops.h"                  
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_lun.h"
#include "fbe_test.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "neit_utils.h"

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* red_riding_hood_short_desc = "Non-Paged Metadata Error Handling";
char* red_riding_hood_long_desc =
    "\n"
    "\n"
    "The test is for MD raw mirror error handling and rebuilding\n"
    "red_riding_hood_rebuild_test does:.\n"
    "\n"
    "STEP 1: remove system dirve 0-0-2,0-0-1\n"
    "\n"
    "STEP 2: check the removed time stamp of 0-0-2's pvd and 0-0-1's pvd.\n"
    "\n"
    "STEP 3: write and persist the pvd removed time stamp of 0-0-0.\n"
    "\n"
    "STEP 4: reinsert 0-0-2 .\n" 
    "\n"
    "STEP 5: wait for pvd(0-0-0)finishing its nonpaged MD write verify.\n" 
    "\n"
    "STEP 6: load non paged MD from disk 0-0-2(only 0-0-2).\n" 
    "\n"
    "STEP 7: read the removed time stamp of pvd(0-0-0),check whether it is correct.\n" 
    "\n"
    "STEP 8: reinsert 0-0-1 .\n" 
    "\n"
    "STEP 9: wait for system pvds finishing their nonpaged MD write verify.\n" 
    "\n"
    "STEP 10: load non paged MD from disk 0-0-1(only 0-0-1).\n" 
    "\n"
    "STEP 11: read the removed time stamp of pvd(0-0-0),check whether it is correct.\n" 
    "\n"
    
"\n"
"Desription last updated: 3/26/2012.\n"    ;

//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:

                                                        

/*!*******************************************************************
 * @def ELMO_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
 #define RED_HIDING_HOOD_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66  
/*!*******************************************************************
 * @def DIEGO_DRIVES_PER_ENCL
 *********************************************************************
 * @brief Allows us to change how many drives per enclosure we have.
 *
 *********************************************************************/
 #define DIEGO_DRIVES_PER_ENCL 15
 
#define RED_RIDING_HOOD_TEST_WAIT_MSEC 30000
#define RED_RIDING_HOOD_POLLING_MSEC 200
#define RED_RIDING_HOOD_TEST_HS_COUNT 1
#define RED_RIDING_HOOD_PVD_READY_WAIT_TIME           20  // max. time (in seconds) to wait for pvd to become ready
#define RED_RIDING_HOOD_LUNS_PER_RAID_GROUP 1
#define RED_RIDING_HOOD_CHUNKS_PER_LUN 3
#define RED_RIDING_HOOD_FAILED_DRIVERS_BITMASK 7

#define RED_RIDING_HOOD_POLLING_INTERVAL 100 /*ms*/

//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:
 
 typedef  struct  red_riding_hood_test_spare_context_s
 {
	 fbe_semaphore_t	 sem;
	 fbe_object_id_t	object_id;
	 fbe_job_service_info_t    job_srv_info;
 }red_riding_hood_test_spare_context_t;
 fbe_test_hs_configuration_t red_riding_hood_hs_config ={{0, 0, 7}, 520, FBE_OBJECT_ID_INVALID};

/*!*******************************************************************
 * @def RED_RIDING_HOOD_DATABASE_READY_WAIT_MS
 *********************************************************************
 * @brief  How long to wait (in milliseconds) for the database to be
 *         ready.
 *
 *********************************************************************/
#define RED_RIDING_HOOD_DATABASE_READY_WAIT_MS  30000

/*!*******************************************************************
 * @def RED_RIDING_HOOD_MAX_IO_CONTEXT
 *********************************************************************
 * @brief  Five I/O threads. lg and sm.
 *
 *********************************************************************/
#define RED_RIDING_HOOD_MAX_IO_CONTEXT 5

/*!*******************************************************************
 * @def RED_RIDING_HOOD_TEST_ELEMENT_SIZE
 *********************************************************************
 * @brief Element size of our raid groups.
 *
 *********************************************************************/
#define RED_RIDING_HOOD_ELEMENT_SIZE 128 

/*!*******************************************************************
 * @var red_riding_hood_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t red_riding_hood_test_contexts[RED_RIDING_HOOD_LUNS_PER_RAID_GROUP * RED_RIDING_HOOD_MAX_IO_CONTEXT];

/*!*******************************************************************
 * @var red_riding_hood_pause_params
 *********************************************************************
 * @brief This variable contains any values that need to be passed to
 *        the reboot methods.
 *
 *********************************************************************/
static fbe_sep_package_load_params_t red_riding_hood_pause_params = { 0 };

/*!*******************************************************************
 * @var red_riding_hood_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t red_riding_hood_raid_group_config_qual[] = 
{
  /* width,   capacity    raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {2,       0x32000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            0,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var red_riding_hood_system_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of sytem configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t red_riding_hood_system_raid_group_config_qual[] = 
{
  /* width,   capacity    raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {3,       0x32000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            1,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

extern fbe_u32_t   sep_rebuild_utils_number_physical_objects_g;

static void red_riding_hood_NP_magic_number_test(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot);
static void red_riding_hood_write_error_injection_test(void);
static void red_riding_hood_read_error_injection_test(void);
static void red_riding_hood_test_register_spare_notification(fbe_object_id_t object_id, 
   									  red_riding_hood_test_spare_context_t* context, 
   									  fbe_notification_registration_id_t* registration_id);
static void  red_riding_hood_spare_callback_function(update_object_msg_t* update_obj_msg,
   												  void* context);                                                
static void  red_riding_hood_test_wait_spare_notification(red_riding_hood_test_spare_context_t* spare_context);

static void  red_riding_hood_test_unregister_spare_notification(fbe_object_id_t object_id, 
   									   fbe_notification_registration_id_t registration_id);

static void  red_riding_hood_test_unregister_spare_notification(fbe_object_id_t object_id, 
   										   fbe_notification_registration_id_t registration_id);
 
static fbe_status_t red_riding_hood_inject_read_errors_to_system_drive(fbe_u32_t fail_drives,
                                                                       fbe_u32_t start_offset,
                                                                       fbe_u32_t block_count,
                                                                       fbe_private_space_layout_region_id_t region_id,
                                                                       fbe_lun_number_t lun_number);

static void  red_riding_hood_test_get_spare_in_pvd_id(fbe_object_id_t vd_id, fbe_object_id_t* pvd_id);
static fbe_status_t red_riding_hood_inject_errors_to_system_drive(fbe_u32_t fail_drives);
static void red_riding_hood_dualsp_test_bind_after_active_fails(fbe_test_rg_configuration_t *rg_config_p);
static void red_riding_hood_dualsp_test_user_lun_preserved_when_peer_lost(fbe_test_rg_configuration_t *rg_config_p);
static void red_riding_hood_dualsp_test_user_lun_preserved_when_peer_lost_debug(fbe_test_rg_configuration_t *rg_config_p);
static void red_riding_hood_dualsp_test_system_lun_preserved_when_peer_lost(fbe_test_rg_configuration_t *rg_config_p);
static void red_riding_hood_dualsp_test_new_created_lun_initialize_when_peer_lost(fbe_test_rg_configuration_t *rg_config_p);
static void red_riding_hood_dualsp_validate_persist_pending_rg(fbe_test_rg_configuration_t * rg_config_p);
static void red_riding_hood_dualsp_validate_persist_pending_pvd(fbe_test_rg_configuration_t * rg_config_p);
static void red_riding_hood_populate_delay_io_error_record(fbe_object_id_t object_id, 
                                                           fbe_api_logical_error_injection_record_t *error_record);
static void red_riding_hood_perform_error_injection(fbe_object_id_t object_id,
                                                    fbe_api_logical_error_injection_record_t *in_error_case_p);
static void red_riding_hood_shutdown_active_sp(void);
static void red_riding_hood_add_non_paged_persist_hook(fbe_object_id_t object_id);
static void red_riding_hood_del_non_paged_persist_hook(fbe_object_id_t object_id);
static void red_riding_hood_wait_for_non_paged_persist_hook(fbe_object_id_t object_id);
static void red_riding_hood_add_lun_incomplete_write_verify_hook(fbe_object_id_t object_id);
static void red_riding_hood_del_lun_incomplete_write_verify_hook(fbe_object_id_t object_id);
static void red_riding_hood_wait_for_lun_incomplete_write_verify_hook(fbe_object_id_t object_id);
static fbe_status_t red_riding_hood_wait_for_lei_error(fbe_u32_t error_count);
static fbe_status_t red_riding_hood_persit_pending_setup(fbe_object_id_t object_id);
static fbe_status_t red_riding_hood_persit_pending_post_processing(fbe_object_id_t object_id,
                                                                   fbe_test_rg_configuration_t *rg_config_p);

static void red_riding_hood_rebuild_test(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot);
static void red_riding_hood_NP_state_test_phase_two(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot);
static void red_riding_hood_np_load_error_test(void);
static fbe_status_t red_riding_hood_wait_for_metadata_state(fbe_object_id_t object_id, 
                                                            fbe_base_config_nonpaged_metadata_state_t metadata_state);
static fbe_status_t red_riding_hood_create_lun_without_notify(fbe_api_job_service_lun_create_t* lu_create_job);
static void red_riding_hood_read_zero_background_pattern(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_object_id_t lun_object_id);


//-----------------------------------------------------------------------------
//  FUNCTIONS:


/*!****************************************************************************
 *  red_riding_hood_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for red_riding_hood test.  The test does the 
 *   following:
 *
 *   - remove a system dirve 0-0-1, this will trigger the nonpaged metadata write to the system PVD 
 *   - reinsert the system dive
 *   - reboot the system. this will trigger the system load system nonpaged metadata from raw mirror
 *   - remove 0-0-0,0-0-2 
 *   - read the system pvd's nonpaged metadata.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void red_riding_hood_test(void)
{    
	red_riding_hood_rebuild_test(0,0,1);
	
	red_riding_hood_NP_state_test_phase_two(0,0,1);
	red_riding_hood_NP_magic_number_test(0,0,1);
	red_riding_hood_read_error_injection_test();
	red_riding_hood_write_error_injection_test();
    red_riding_hood_np_load_error_test();
    return ;
}

void red_riding_hood_run_dualsp_tests(fbe_test_rg_configuration_t * rg_config_p, void * context_p)
{
#ifndef __SAFE__
    fbe_test_rg_configuration_t *system_rg_config_p = &red_riding_hood_system_raid_group_config_qual[0];
#endif

    /*! @note The order that the below test execute in matters.
     */

    /* Validate that new raid groups/luns can be bound when the active SP fails */
    red_riding_hood_dualsp_test_bind_after_active_fails(rg_config_p);

#ifndef __SAFE__
    /* Validate that the system lun non-paged metadata is preserved when peer lost */
    red_riding_hood_dualsp_test_system_lun_preserved_when_peer_lost(system_rg_config_p);
#endif

    /* Validate that the user lun non-paged metadata is preserved when peer lost */
    red_riding_hood_dualsp_test_user_lun_preserved_when_peer_lost(rg_config_p);

    /* Validate that a newly created lun never reads nonpaged from disk when peer 
     * lost before syncing the nonpaged MD*/
    red_riding_hood_dualsp_test_new_created_lun_initialize_when_peer_lost(rg_config_p);

    /* Validate that PVD can handle peer SP failure during persist */
    red_riding_hood_dualsp_validate_persist_pending_pvd(rg_config_p);

    /* Validate that RG can handle peer SP failure during persist */
    red_riding_hood_dualsp_validate_persist_pending_rg(rg_config_p);
}

static void red_riding_hood_dualsp_validate_persist_pending_pvd(fbe_test_rg_configuration_t * rg_config_p)
{
    fbe_object_id_t pvd_id;
    fbe_status_t status;
    fbe_api_provision_drive_info_t  provision_drive_info;

    mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s Started for PVD ****\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");

   // get id of a PVD object associated with this RAID group
    status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_p->rg_disk_set[0].bus,
                                                             rg_config_p->rg_disk_set[0].enclosure,
                                                             rg_config_p->rg_disk_set[0].slot,
                                                             &pvd_id );
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,"Unable to get the PVD object id\n");

    status = red_riding_hood_persit_pending_setup(pvd_id);

    status = fbe_api_provision_drive_get_info(pvd_id, 
                                              &provision_drive_info);

    status = fbe_api_provision_drive_set_zero_checkpoint(pvd_id, provision_drive_info.default_offset);
    status = fbe_test_sep_util_provision_drive_enable_zeroing(pvd_id);
    status = red_riding_hood_persit_pending_post_processing(pvd_id, rg_config_p);
}

/*!****************************************************************************
 *  red_riding_hood_dualsp_validate_persist_pending_rg
 ******************************************************************************
 *
 * @brief
 *    This tests validate the RG handling of peer SP death and Non paged write
 * outstanding
 *
 * @param rg_config_ptr - The RG configuration for this test   
 *
 * @return  void
 *
 * @notes: 
 *     This test validates that when NON-Paged Write is outstanding and the 
 *  SP that is performing the writes goes down:
 *     1. The surviving SP writes out the Non-paged information
 *     2. Since the LUN will be dirty, validate that LUN issues a incomplete write
 *        verify to the RAID object
 * 
 * @author
 *    04/26/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static void red_riding_hood_dualsp_validate_persist_pending_rg(fbe_test_rg_configuration_t * rg_config_p)
{
    fbe_object_id_t rg_object_id;
    fbe_object_id_t lun_object_id;
    fbe_status_t status;
    fbe_api_terminator_device_handle_t drive_handle;
    fbe_u32_t total_objects;
    
    /* Get the RG object id.
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s Started for RG %d ****\n", __FUNCTION__, rg_config_p->raid_group_id);
    mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");

    status = red_riding_hood_persit_pending_setup(rg_object_id);

    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[0].lun_number, 
                                                   &lun_object_id);

    fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);

    /* Remove a drive such a way that RG goes degrded and rebuid checkpoint is updated which
     * will cause a Non-paged Write
     */
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s Removing drive ****\n", __FUNCTION__);
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p,
                                              0,
                                              total_objects - 1,
                                              &drive_handle);
    sep_rebuild_utils_verify_rb_logging_by_object_id(rg_object_id, 0);

    status = red_riding_hood_persit_pending_post_processing(rg_object_id, rg_config_p);
    return;
}
/**********************************************************
 * end red_riding_hood_dualsp_validate_persist_pending_rg()
 ***********************************************************/

/*!***************************************************************************
 *          red_riding_hood_write_background_pattern()
 *****************************************************************************
 *
 * @brief   Seed the specified lun with background pattern
 *
 * @param   lun_object_id   - Object id of LUN under test           
 *
 * @return  None.
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from vincent_van_ghoul_write_background_pattern
 *
 *****************************************************************************/
static void red_riding_hood_write_background_pattern(fbe_object_id_t lun_object_id, fbe_lba_t max_lba)
{
    fbe_status_t                status;
    fbe_api_rdgen_context_t    *context_p = &red_riding_hood_test_contexts[0];
    
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Writing background pattern for lun obj: 0x%x==", 
               __FUNCTION__, lun_object_id);

    /* Write a background pattern to the LUNs.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                                  lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                  max_lba, /* limit range accessed */
                                                  RED_RIDING_HOOD_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                                  lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                                  max_lba, /* limit range accessed */
                                                  RED_RIDING_HOOD_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/************************************************
 * end red_riding_hood_write_background_pattern()
 ************************************************/

/*!***************************************************************************
 *          red_riding_hood_wait_for_all_luns_ready()
 *****************************************************************************
 *
 * @brief   Wait for all luns to be ready before starting I/O.
 *
 * @param   rg_config_p - Pointer to array of raid groups with the luns to run
 *              run I/O against.  This method validates that all the luns are
 *              in the ready state.
 *
 * @return  status - Determine if test passed or not
 *
 * @note    In dualsp mode the `other' SP maybe shutdown.  Therefore for either
 *          single SP mode or dualsp mode we only wait for all the LUNs to be
 *          be ready on `this' SP.
 *
 * @author
 *  03/24/2011  Ron Proulx  - Created 
 *
 *****************************************************************************/
static fbe_status_t red_riding_hood_wait_for_all_luns_ready(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_time_t                  start_time_for_sp;
    fbe_time_t                  elapsed_time_for_sp;
    fbe_transport_connection_target_t target_invoked_on = FBE_TRANSPORT_SP_A;
    fbe_transport_connection_target_t peer_target = FBE_TRANSPORT_SP_B;

    /* First get the sp id this was invoked on.
     */
    target_invoked_on = fbe_api_transport_get_target_server();

    /* Determine the peer sp (for debug)
     */
    peer_target = (target_invoked_on == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A; 

    /* Get the number of raid groups to run I/O against
     */
    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    /* First iterate over the raid groups on `this' sp. Then if neccessary iterate
     * over the raid groups on the other sp.
     */
    current_rg_config_p = rg_config_p;
    start_time_for_sp = fbe_get_time();
    for (rg_index = 0; 
         (rg_index < rg_count) && (status == FBE_STATUS_OK); 
         rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            /* Wait for all lun objects of this raid group to be ready
             */
            status = fbe_test_sep_util_wait_for_all_lun_objects_ready(current_rg_config_p,
                                                                      target_invoked_on);
            if (status != FBE_STATUS_OK)
            {
                /* Failure, break out and return the failure.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "wait for all luns ready: raid_group_id: 0x%x failed to come ready", 
                           current_rg_config_p->raid_group_id);
                break;
            }
        }

        /* Goto next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups configured */
    elapsed_time_for_sp = fbe_get_time() - start_time_for_sp;
    mut_printf(MUT_LOG_TEST_STATUS, 
               "wait for all luns ready: for local sp: %d took: %lld ms with status: 0x%x", 
               target_invoked_on, elapsed_time_for_sp, status);

    return status;
}
/***********************************************
 * end red_riding_hood_wait_for_all_luns_ready()
 ***********************************************/

/*!***************************************************************************
 *          red_riding_hood_read_background_pattern()
 *****************************************************************************
 *
 * @brief   Read and validate all the LUN under test.
 *
 * @param   rg_config_p - Pointer to array of raid groups under test.
 * @param   lun_object_id - The object id of the lun to validate 
 * @param   b_validate_originating_sp - FBE_TRUE - The data was written from
 *              the `other' SP (specified in `originating_sp').
 * @param   originating_sp - The SP identifier that wrote the data.
 *
 * @return None.
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from vincent_van_ghoul_read_background_pattern
 *
 *****************************************************************************/
static void red_riding_hood_read_background_pattern(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_object_id_t lun_object_id,
                                             fbe_bool_t b_validate_originating_sp,
                                             fbe_sim_transport_connection_target_t originating_sp,
                                             fbe_lba_t max_lba)

{
    fbe_status_t                status;
    fbe_api_rdgen_context_t    *context_p = &red_riding_hood_test_contexts[0];
    fbe_rdgen_sp_id_t           rdgen_originating_sp = FBE_DATA_PATTERN_SP_ID_INVALID;
    //fbe_lba_t                   max_lba = (RED_RIDING_HOOD_CHUNKS_PER_LUN * FBE_RAID_DEFAULT_CHUNK_SIZE);

    /* Since this is a reboot test we must wait for all LUs ready.
     */
    status = red_riding_hood_wait_for_all_luns_ready(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Validating background pattern lun obj: 0x%x ==", 
               __FUNCTION__, lun_object_id);
    
    /* Read back the pattern and check it was written OK.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                                  lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                                  max_lba, /* limit rage accessed */
                                                  RED_RIDING_HOOD_ELEMENT_SIZE);

    /* If the data was written from the `other' SP set the necessary rdgen
     * options.
     */
    if (b_validate_originating_sp == FBE_TRUE)
    {
        /* Set the rdgen option that indicates which SP wrote the data and
         * set the expected originating SP.
         */
        MUT_ASSERT_TRUE((originating_sp == FBE_SIM_SP_A) ||
                        (originating_sp == FBE_SIM_SP_B)    );
        rdgen_originating_sp = (originating_sp == FBE_SIM_SP_A) ? FBE_DATA_PATTERN_SP_ID_A : FBE_DATA_PATTERN_SP_ID_B;
        status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, FBE_RDGEN_OPTIONS_CHECK_ORIGINATING_SP);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_rdgen_io_specification_set_originating_sp(&context_p->start_io.specification, rdgen_originating_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Run the I/O.
     */
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Validating background pattern - success ==", 
               __FUNCTION__);
    return;
}
/***********************************************
 * end red_riding_hood_read_background_pattern()
 ***********************************************/

/*!*********************************************************************************
 *          red_riding_hood_read_zero_background_pattern()
 ***********************************************************************************
 *
 * @brief   Read and validate all the LUN under test that the data pattern is zero
 *
 * @param   rg_config_p - Pointer to array of raid groups under test.
 * @param   lun_object_id - The object id of the lun to validate
 *
 * @return None.
 *
 * @author
 *  01/18/2013  Zhipeng Hu Created
 *
 ***********************************************************************************/
static void red_riding_hood_read_zero_background_pattern(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_object_id_t lun_object_id)
{
    fbe_status_t                status;
    fbe_api_rdgen_context_t    *context_p = &red_riding_hood_test_contexts[0];
    fbe_lba_t                   max_lba = (RED_RIDING_HOOD_CHUNKS_PER_LUN * FBE_RAID_DEFAULT_CHUNK_SIZE);

    /* Since this is a reboot test we must wait for all LUs ready.
     */
    status = red_riding_hood_wait_for_all_luns_ready(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Validating background pattern lun obj: 0x%x ==", 
               __FUNCTION__, lun_object_id);


    status = fbe_api_rdgen_test_context_init(context_p, 
                                         lun_object_id,
                                         FBE_CLASS_ID_INVALID, 
                                         FBE_PACKAGE_ID_SEP_0,
                                         FBE_RDGEN_OPERATION_READ_CHECK,
                                         FBE_RDGEN_PATTERN_ZEROS,
                                         1,    /* We do one full sequential pass. */
                                         0,    /* num ios not used */
                                         0,    /* time not used*/
                                         1,    /* threads */
                                         FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                         0,    /* Start lba*/
                                         0,    /* Min lba */
                                         max_lba,    /* max lba */
                                         FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                         16,    /* Number of stripes to write. */
                                         RED_RIDING_HOOD_ELEMENT_SIZE    /* Max blocks */ );
    
    /* Run the I/O.
     */
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Validating background pattern - success ==", 
               __FUNCTION__);
    return;
}
/***********************************************
 * end red_riding_hood_read_background_pattern()
 ***********************************************/

/*!***************************************************************************
 *          red_riding_hood_reboot()
 *****************************************************************************
 *
 * @brief   Depending if the test in running in single SP or dualsp mode, this
 *          method will either reboot `this' SP (single SP mode) or will reboot
 *          the `other' SP.
 *
 * @param   sep_params_p - Pointer to `sep params' for the reboot of either this
 *              SP or the other SP.
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  06/25/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t red_riding_hood_reboot(fbe_sep_package_load_params_t *sep_params_p)
{   
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_bool_t                              b_is_dualsp_mode = FBE_FALSE;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_bool_t                              b_preserve_data = FBE_TRUE;
    
    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* Determine if we are in dualsp mode or not.
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /* If we are in dualsp mode (the normal mode on the array), shutdown
     * the current SP to transfer control to the other SP.
     */
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        /* If both SPs are enabled shutdown this SP and validate that the other 
         * SP takes over.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Dualsp: %d shutdown SP: %d make current SP: %d ==", 
                   __FUNCTION__, b_is_dualsp_mode, this_sp, other_sp);

        /* Only use the `quick' destroy if `preserve data' is not set.
         */
        if (b_preserve_data == FBE_TRUE)
        {
            fbe_test_sep_util_destroy_neit_sep();
        }
        else
        {
            fbe_api_sim_transport_destroy_client(this_sp);
            fbe_test_sp_sim_stop_single_sp(this_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);
        }

        /* Set the transport server to the correct SP.
         */
        status = fbe_api_sim_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        this_sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(this_sp, other_sp);
    }
    else
    {
        /* Else simply reboot this SP. (currently no specific reboot params for NEIT).
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot current SP: %d ==", 
                   __FUNCTION__, this_sp);
        status = fbe_test_common_reboot_this_sp(sep_params_p, NULL /* No params for NEIT*/); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }  

    return status;
}
/******************************************
 * end red_riding_hood_reboot()
 ******************************************/

/*!***************************************************************************
 *          red_riding_hood_reboot_cleanup()
 *****************************************************************************
 *
 * @brief   Depending if the test in running in single SP or dualsp mode, this
 *          method will either reboot `this' SP (single SP mode) or will reboot
 *          the `other' SP.
 *
 * @param   sep_params_p - Pointer to `sep params' for the reboot of either this
 *              SP or the other SP.
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  06/25/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t red_riding_hood_reboot_cleanup(fbe_sep_package_load_params_t *sep_params_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_bool_t                              b_is_dualsp_mode = FBE_FALSE;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_u32_t                               test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t            *rg_config_p = NULL;
    fbe_bool_t                              b_preserve_data = FBE_TRUE;

    /* Get the array of array test configs.
     */
    if (test_level > 0)
    {
        /* Extended.
         */
        rg_config_p = &red_riding_hood_raid_group_config_qual[0];
    }
    else
    {
        /* Qual.
         */
        rg_config_p = &red_riding_hood_raid_group_config_qual[0];
    }

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* Determine if we are in dualsp mode or not.
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /* If we are single SP mode reboot this SP.
     */
    if (b_is_dualsp_mode == FBE_FALSE)
    {
        /* If we are in single SP mode there currently is no cleanup neccessary.
         */
    }
    else
    {
        /* Else we are in dualsp mode. Boot the `other' SP and use/set the same
         * debug hooks that we have for this sp.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Boot other SP: %d ==", 
                   __FUNCTION__, other_sp);

        /* Only use the `quick' create if `preserve data' is not set.
         */
        if (b_preserve_data == FBE_TRUE)
        {
            status = fbe_test_common_boot_sp(other_sp, 
                                             sep_params_p,
                                             NULL   /* No neit params*/);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        else
        {
            status = fbe_api_sim_transport_set_target_server(other_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            fbe_test_boot_sp(rg_config_p, other_sp);
            status = fbe_api_sim_transport_set_target_server(this_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }  

    return status;
}
/******************************************
 * end vincent_van_ghoul_reboot_cleanup()
 ******************************************/

/*!**************************************************************
 * red_riding_hood_create_rg_config()
 ****************************************************************
 * @brief
 *  This function creates the configs, runs the test and destroys
 *  the configs.
 * 
 *  We will run this using whichever drives we find in the system.
 *  If there are not enough drives to create all the configs, then
 *  we will create a set of configs, run the test, destroy the configs
 *  and then run another test.
 *
 * @param rg_config_p - Array of raid group configs.
 * @param luns_per_rg - Number of LUs to bind per rg.
 * @param chunks_per_lun - Number of chunks in each lun.
 *
 * @return  None.
 *
 * @author
 *  12/03/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static void red_riding_hood_create_rg_config(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_u32_t luns_per_rg,
                                             fbe_u32_t chunks_per_lun)
{
    fbe_test_discovered_drive_locations_t   drive_locations;
    fbe_u32_t                               raid_group_count_this_pass = 0;
    fbe_u32_t                               raid_group_count = 0;
    fbe_bool_t                              b_is_dualsp_test = FBE_TRUE;

    /* Since (1) SP is probably in the failed state we need to clear the dualsp
     * flag so that the methods below do not fail.
     */
    b_is_dualsp_test = fbe_test_sep_util_get_dualsp_test_mode();
    if (b_is_dualsp_test == FBE_TRUE)
    {
        /* Disable dualsp test since there is an SP down.
         */
        fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    }


    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Must initialize the array of rg configurations.
     */
    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

    fbe_test_sep_util_discover_all_drive_information(&drive_locations);

    /* Get the raid group config for the first pass.
     */
    fbe_test_sep_util_setup_rg_config(rg_config_p, &drive_locations);
    raid_group_count_this_pass = fbe_test_get_rg_count(rg_config_p);
    MUT_ASSERT_INT_EQUAL(1, raid_group_count_this_pass);

    /* Continue to loop until we have no more raid groups.
     */
    while (raid_group_count_this_pass > 0)
    {
        fbe_u32_t debug_flags;
        fbe_u32_t trace_level = FBE_TRACE_LEVEL_INFO;
        mut_printf(MUT_LOG_LOW, "testing %d raid groups \n", raid_group_count_this_pass);
        /* Create the raid groups.
         */
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        /* Setup the lun capacity with the given number of chunks for each lun.
         */
        fbe_test_sep_util_fill_lun_configurations_rounded(rg_config_p, raid_group_count,
                                                          chunks_per_lun, 
                                                          luns_per_rg);

        /* Kick of the creation of all the raid group with Logical unit configuration.
         */
        fbe_test_sep_util_create_raid_group_configuration(rg_config_p, raid_group_count);
        
        if (fbe_test_sep_util_get_cmd_option_int("-raid_group_trace_level", &trace_level))
        {
            fbe_test_sep_util_set_rg_trace_level(rg_config_p, (fbe_trace_level_t) trace_level);
        }
        if (fbe_test_sep_util_get_cmd_option_int("-raid_group_debug_flags", &debug_flags))
        {
            fbe_test_sep_util_set_rg_debug_flags(rg_config_p, (fbe_raid_group_debug_flags_t)debug_flags);
        }
        if (fbe_test_sep_util_get_cmd_option_int("-raid_library_debug_flags", &debug_flags))
        {
            fbe_test_sep_util_set_rg_library_debug_flags(rg_config_p, (fbe_raid_library_debug_flags_t)debug_flags);
        }

        /* Decrement the number of raid groups to create.
         */
        raid_group_count_this_pass--;
    }

    /* If this was a dualsp test, re-enable dualsp mode.
     */
    if (b_is_dualsp_test == FBE_TRUE)
    {
        /* Enalbe dualsp test since there is an SP down.
         */
        fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    }
    return;
}
/******************************************
 * end red_riding_hood_create_rg_config()
 ******************************************/

/*!***************************************************************************
 *          red_riding_hood_dualsp_test_bind_after_active_fails()
 *****************************************************************************
 *
 * @brief   This test validates that if the active SP goes away before the lun
 *          or raid group is created, that the original SP is shutdown.  The
 *          newly active SP should handle a raid group and lun creation.
 *
 * @param   rg_config_p - The RG configuration for this test   
 *
 * @return  void
 *
 * @author
 *  12/03/2012  Ron Proulx  - Created
 *
 *****************************************************************************/
static void red_riding_hood_dualsp_test_bind_after_active_fails(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               raid_group_count;
    fbe_object_id_t                         rg_object_id;
    fbe_object_id_t                         lun_object_id;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_sim_transport_connection_target_t   passive_sp;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    
    /* Get the raid group and lun object ids
     */
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[0].lun_number, 
                                                   &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the raid group and lun object ids
     */
    status = fbe_test_sep_util_get_active_passive_sp(lun_object_id, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "** %s for lun obj: 0x%x active SP: %d **\n", 
               __FUNCTION__, lun_object_id, active_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");

    /* Set the target SP to the active 
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    if (this_sp != active_sp)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Change current SP from: %d to %d ==", 
                   __FUNCTION__, this_sp, active_sp);
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        this_sp = active_sp;
    }
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* Step 1:  Write a known background pattern.
     */
    red_riding_hood_write_background_pattern(lun_object_id, RED_RIDING_HOOD_CHUNKS_PER_LUN * FBE_RAID_DEFAULT_CHUNK_SIZE);

    /* Step 2: Remove the luns and raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove luns and raid groups ==", __FUNCTION__);
    status = fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, raid_group_count);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3:  Make the other SP active
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown this SP ==", __FUNCTION__);
    status = red_riding_hood_reboot(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown this SP - complete ==", __FUNCTION__);

    /* Step 4: Create raid group and luns on the now active SP.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Create raid group/lun on new active SP ==", __FUNCTION__);
    red_riding_hood_create_rg_config(rg_config_p, 
                                     RED_RIDING_HOOD_LUNS_PER_RAID_GROUP,
                                     RED_RIDING_HOOD_CHUNKS_PER_LUN);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Create raid group/lun on new active SP - complete ==", __FUNCTION__);

    /* Step 5: Get the raid group and lun object ids
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[0].lun_number, 
                                                   &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 6: Write the background pattern.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Write background pattern on new active SP ==", __FUNCTION__);
    red_riding_hood_write_background_pattern(lun_object_id, RED_RIDING_HOOD_CHUNKS_PER_LUN * FBE_RAID_DEFAULT_CHUNK_SIZE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Write background pattern on new active SP - complete ==", __FUNCTION__);

    /* Step 7:  Boot the orignal SP with the previously configured debug hooks.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot this SP ==", __FUNCTION__);
    status = red_riding_hood_reboot_cleanup(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot this SP - complete ==", __FUNCTION__);

    /* Step 8:  Immediately shutdown the other SP
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown other SP ==", __FUNCTION__);
    status = red_riding_hood_reboot(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown other SP - complete ==", __FUNCTION__);

    /* Step 9:  Validate the data.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data written on other SP==", __FUNCTION__);
    red_riding_hood_read_background_pattern(rg_config_p, lun_object_id,
                                            FBE_TRUE,   /* Validate which SP wrote the data */
                                            other_sp,     /* This is the SP that wrote the data */
                                            RED_RIDING_HOOD_CHUNKS_PER_LUN * FBE_RAID_DEFAULT_CHUNK_SIZE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data - success==", __FUNCTION__);

    /* Step 10: Cleanup (boot other SP).
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup ==", __FUNCTION__);
    status = red_riding_hood_reboot_cleanup(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup - complete ==", __FUNCTION__);

    /* Step 11:  Validate the data (again)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data ==", __FUNCTION__);
    red_riding_hood_read_background_pattern(rg_config_p, lun_object_id,
                                            FBE_TRUE,   /* Validate which SP wrote the data */
                                            other_sp,     /* This is the SP that wrote the data */
                                            RED_RIDING_HOOD_CHUNKS_PER_LUN * FBE_RAID_DEFAULT_CHUNK_SIZE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data - success==", __FUNCTION__);

    return;
}
/****************************************************************
 * end red_riding_hood_dualsp_test_bind_after_active_fails()
 ****************************************************************/

/*!***************************************************************************
 *          red_riding_hood_dualsp_test_user_lun_preserved_when_peer_lost()
 *****************************************************************************
 *
 * @brief   This test validates that if the peer goes away before the lun
 *          non-paged metadata is populated, that the non-paged is read from
 *          disk.  It write a pattern to the luns on the raid group specified
 *          sets a debug hook on SPA then shutdown SPA.  Then it brings up
 *          SPA and immediately shutdowns SPB.
 *
 * @param   rg_config_p - The RG configuration for this test   
 *
 * @return  void
 *
 * @author
 *  11/29/2012  Ron Proulx  - Created
 *
 *****************************************************************************/
static void red_riding_hood_dualsp_test_user_lun_preserved_when_peer_lost(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         rg_object_id;
    fbe_object_id_t                         lun_object_id;
    fbe_u32_t                               hook_index = 0;
    fbe_bool_t                              b_is_debug_hook_reached = FBE_FALSE;
    fbe_u32_t                               current_time = 0;
    fbe_u32_t                               timeout_ms = FBE_TEST_HOOK_WAIT_MSEC;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_sim_transport_connection_target_t   passive_sp;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
  
    /* Get the raid group and lun object ids
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[0].lun_number, 
                                                   &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the active and passive for the lun object.
     */
    status = fbe_test_sep_util_get_active_passive_sp(lun_object_id, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "** %s for lun obj: 0x%x active: %d **\n", 
               __FUNCTION__, lun_object_id, active_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");
      
    /* Set the target SP to the active 
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    if (this_sp != active_sp)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Change current SP from: %d to %d ==", 
                   __FUNCTION__, this_sp, active_sp);
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        this_sp = active_sp;
    }
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* Step 1:  Write a known background pattern.
     */
    red_riding_hood_write_background_pattern(lun_object_id, RED_RIDING_HOOD_CHUNKS_PER_LUN * FBE_RAID_DEFAULT_CHUNK_SIZE);

    /* Step 2: Set a debug hook when we read the non-paged metadata for the
     *         lun object.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set non-paged metadata load hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_set_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[hook_index],
                                                          hook_index,
                                                          lun_object_id,
                                                          SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_UPDATE,
                                                          FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_CHECK,
                                                          0, 0,
                                                          SCHEDULER_CHECK_STATES,
                                                          SCHEDULER_DEBUG_ACTION_PAUSE,
                                                          FBE_TRUE /* This is a reboot test */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3:  Make the other SP active
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown this SP ==", __FUNCTION__);
    status = red_riding_hood_reboot(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown this SP - complete ==", __FUNCTION__);

    /* Step 4:  Boot the orignal SP with the previously configured debug hooks.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot this SP ==", __FUNCTION__);
    status = red_riding_hood_reboot_cleanup(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot this SP - complete ==", __FUNCTION__);

    /* Step 5:  Immediately shutdown the other SP
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown other SP ==", __FUNCTION__);
    status = red_riding_hood_reboot(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown other SP - complete ==", __FUNCTION__);

    /* Step 6:  Validate that the `init non-paged' hook was hit
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for init non-paged hook ==", __FUNCTION__);
    while ((b_is_debug_hook_reached == FBE_FALSE) &&
           (current_time < timeout_ms)               )
    {
        status = fbe_test_sep_background_pause_check_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[hook_index],
                                                                &b_is_debug_hook_reached);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (b_is_debug_hook_reached == FBE_FALSE)
        {
            fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
            current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        }
    }
    MUT_ASSERT_TRUE(b_is_debug_hook_reached == FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for init non-paged hook - success ==", __FUNCTION__);

    /* Step 7:  Remove the debug hook and allow the non-paged to be initialized.
     */    
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove init non-paged hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_remove_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[hook_index],
                                                             hook_index,
                                                             FBE_FALSE /* The peer is down */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove init non-paged hook - complete ==", __FUNCTION__);

    /* Step 8:  Validate the data.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data written on this SP==", __FUNCTION__);
    red_riding_hood_read_background_pattern(rg_config_p, lun_object_id,
                                            FBE_TRUE,   /* Validate which SP wrote the data */
                                            this_sp,     /* This is the SP that wrote the data */
                                            RED_RIDING_HOOD_CHUNKS_PER_LUN * FBE_RAID_DEFAULT_CHUNK_SIZE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data - success==", __FUNCTION__);

    /* Step 9: Cleanup (boot other SP).
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup ==", __FUNCTION__);
    status = red_riding_hood_reboot_cleanup(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup - complete ==", __FUNCTION__);

    /* Step 10:  Validate the data (again)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data ==", __FUNCTION__);
    red_riding_hood_read_background_pattern(rg_config_p, lun_object_id,
                                            FBE_TRUE,   /* Validate which SP wrote the data */
                                            this_sp,     /* This is the SP that wrote the data */
                                            RED_RIDING_HOOD_CHUNKS_PER_LUN * FBE_RAID_DEFAULT_CHUNK_SIZE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data - success==", __FUNCTION__);

    /* Step 11: Remove the luns and raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove luns and raid groups ==", __FUNCTION__);
    status = fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 12: Create raid group and luns on the now active SP.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Create raid groups and luns ==", __FUNCTION__);
    red_riding_hood_create_rg_config(rg_config_p, 
                                     RED_RIDING_HOOD_LUNS_PER_RAID_GROUP,
                                     RED_RIDING_HOOD_CHUNKS_PER_LUN);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Create raid groups and luns - complete ==", __FUNCTION__);

    return;
}
/************************************************************************************
 * end red_riding_hood_dualsp_test_user_lun_preserved_when_peer_lost()
 *********************************************************************/

/*!**********************************************************************************
 *          red_riding_hood_dualsp_test_new_created_lun_initialize_when_peer_lost()
 ************************************************************************************
 *
 * @brief   This test validates that if the peer goes away before a newly created lun
 *          non-paged metadata is populated, that the non-paged will not be read from
 *          disk and it will just be reinitialized.
 *
 * @param   rg_config_p - The RG configuration for this test   
 *
 * @return  void
 *
 * @author
 *  01/17/2013  Zhipeng Hu (huz3) - Created
 *
 ***********************************************************************************/
static void red_riding_hood_dualsp_test_new_created_lun_initialize_when_peer_lost(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         lun_object_id;
    fbe_u32_t                               HOOK_INDEX_PASSIVE_SP_NONPAGED_CHECK = 0;
    fbe_u32_t                               HOOK_INDEX_PASSIVE_SP_NONPAGED_LOAD = 1;
    fbe_u32_t                               HOOK_INDEX_ACTIVE_SP_NONPAGED_CHECK = 2;
    fbe_bool_t                              b_is_debug_hook_reached = FBE_FALSE;
    fbe_u32_t                               current_time = 0;
    fbe_u32_t                               timeout_ms = FBE_TEST_HOOK_WAIT_MSEC;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_sim_transport_connection_target_t   passive_sp;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_api_lun_destroy_t                   lun_destroy_req;
    fbe_api_job_service_lun_create_t        lun_create_req;
    fbe_job_service_error_type_t            job_error_type;

    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[0].lun_number, 
                                                   &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the active and passive for the lun object.
     */
    status = fbe_test_sep_util_get_active_passive_sp(lun_object_id, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "** %s for lun obj: 0x%x active: %d **\n", 
               __FUNCTION__, lun_object_id, active_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");
      
    /* Set the target SP to the active 
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    if (this_sp != active_sp)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Change current SP from: %d to %d ==", 
                   __FUNCTION__, this_sp, active_sp);
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        this_sp = active_sp;
    }
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /*Step 0: Write some pattern onto the LUN*/
    red_riding_hood_write_background_pattern(lun_object_id, RED_RIDING_HOOD_CHUNKS_PER_LUN * FBE_RAID_DEFAULT_CHUNK_SIZE);

    /*Step 1: Destroy the LUN in the RG*/
    lun_destroy_req.lun_number = rg_config_p->logical_unit_configuration_list[0].lun_number;
    status = fbe_api_destroy_lun(&lun_destroy_req, FBE_TRUE, timeout_ms, &job_error_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_NO_ERROR, job_error_type);

    /*Step 2: Now recreate the LUN without NDB (LUN will do reinitialize after creation)*/
    fbe_zero_memory(&lun_create_req, sizeof(lun_create_req));    
    lun_create_req.raid_type = rg_config_p->raid_type;
    lun_create_req.raid_group_id = rg_config_p->raid_group_id;
    lun_create_req.lun_number = rg_config_p->logical_unit_configuration_list[0].lun_number;
    lun_create_req.capacity = rg_config_p->logical_unit_configuration_list[0].capacity; 
    lun_create_req.addroffset = FBE_LBA_INVALID;
    lun_create_req.ndb_b = FBE_FALSE;
    lun_create_req.noinitialverify_b = FBE_FALSE; 
    lun_create_req.user_private = FBE_FALSE;
    lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    lun_create_req.wait_ready = FBE_FALSE;
    lun_create_req.ready_timeout_msec = timeout_ms;

    /*before create the LUN, set debug hook for database service to wait before committing objects*/
    status = fbe_api_database_add_hook(FBE_DATABASE_HOOK_TYPE_WAIT_IN_UPDATE_TRANSACTION);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    status = red_riding_hood_create_lun_without_notify(&lun_create_req); /*do not wait for the notify as there is a hook in db there*/
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*step 3: object is created but not commit, add hook here:
     * (1) NON_PAGED_CHECK on passive SP. So we could let the object pause when it enters nonpaged metadata check logic
       (2) NON_PAGED_CHECK on active SP. So we could let the object pause when it enters nonpaged metadata check logic. Then
           we could shutdown active SP so that the peer object never get synced nonpaged MD.
       (3) NON_PAGED_LOAD_FROM_DISK on passive SP. So we can check this hook is never encoutered because a newly created 
           object never loads nonpaged MD from disk
     */
     
    /*job need some time to create the LUN, let us wait until the LUN is there*/
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_SPECIALIZE, timeout_ms, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set non-paged metadata load hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_set_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[HOOK_INDEX_ACTIVE_SP_NONPAGED_CHECK],
                                                          HOOK_INDEX_ACTIVE_SP_NONPAGED_CHECK,
                                                          lun_object_id,
                                                          SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_UPDATE,
                                                          FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_CHECK,
                                                          0, 0,
                                                          SCHEDULER_CHECK_STATES,
                                                          SCHEDULER_DEBUG_ACTION_PAUSE,
                                                          FBE_FALSE /* This is not a reboot test */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    status = fbe_api_sim_transport_set_target_server(other_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_background_pause_set_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[HOOK_INDEX_PASSIVE_SP_NONPAGED_CHECK],
                                                          HOOK_INDEX_PASSIVE_SP_NONPAGED_CHECK,
                                                          lun_object_id,
                                                          SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_UPDATE,
                                                          FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_CHECK,
                                                          0, 0,
                                                          SCHEDULER_CHECK_STATES,
                                                          SCHEDULER_DEBUG_ACTION_PAUSE,
                                                          FBE_FALSE /* This is not a  reboot test */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_background_pause_set_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[HOOK_INDEX_PASSIVE_SP_NONPAGED_LOAD],
                                                          HOOK_INDEX_PASSIVE_SP_NONPAGED_LOAD,
                                                          lun_object_id,
                                                          SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_UPDATE,
                                                          FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_LOAD_FROM_DISK,
                                                          0, 0,
                                                          SCHEDULER_CHECK_STATES,
                                                          SCHEDULER_DEBUG_ACTION_PAUSE,
                                                          FBE_FALSE /* This is not a  reboot test */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_sim_transport_set_target_server(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*remove database hook so that object would be commit*/
    status = fbe_api_database_remove_hook(FBE_DATABASE_HOOK_TYPE_WAIT_IN_UPDATE_TRANSACTION);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*step 4:  wait for the ACTIVE SP object hook that object has entered nonpaged request check logic*/
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for entering nonpaged request check hook ==", __FUNCTION__);
    b_is_debug_hook_reached = FBE_FALSE;
    current_time = 0;
    while ((b_is_debug_hook_reached == FBE_FALSE) &&
           (current_time < timeout_ms)               )
    {
        status = fbe_test_sep_background_pause_check_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[HOOK_INDEX_ACTIVE_SP_NONPAGED_CHECK],
                                                                &b_is_debug_hook_reached);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (b_is_debug_hook_reached == FBE_FALSE)
        {
            fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
            current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        }
    }
    MUT_ASSERT_TRUE(b_is_debug_hook_reached == FBE_TRUE);

    status = fbe_api_sim_transport_set_target_server(other_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    b_is_debug_hook_reached = FBE_FALSE;
    current_time = 0;
    while ((b_is_debug_hook_reached == FBE_FALSE) &&
           (current_time < timeout_ms)               )
    {
        status = fbe_test_sep_background_pause_check_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[HOOK_INDEX_PASSIVE_SP_NONPAGED_CHECK],
                                                                &b_is_debug_hook_reached);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (b_is_debug_hook_reached == FBE_FALSE)
        {
            fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
            current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        }
    }
    MUT_ASSERT_TRUE(b_is_debug_hook_reached == FBE_TRUE);
    status = fbe_api_sim_transport_set_target_server(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for init non-paged hook - success ==", __FUNCTION__);

    /*step 5: now shutdown this SP (active SP)*/
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown ACTIVE SP ==", __FUNCTION__);
    status = red_riding_hood_reboot(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown ACTIVE SP - complete ==", __FUNCTION__);

    /*step 6: passive SP becomes ACTIVE SP now. Remove its nonpaged request check hook so that object continues to run*/
    status = fbe_test_sep_util_wait_for_database_service(timeout_ms); /*let database finishes its peer lost process routine*/
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_sep_background_pause_remove_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[HOOK_INDEX_PASSIVE_SP_NONPAGED_CHECK],
                                                             HOOK_INDEX_PASSIVE_SP_NONPAGED_CHECK,
                                                             FBE_FALSE /* This is not a  reboot test */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*step 7: wait for this LUN becoming READY*/
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for object on the surviving SP READY ==", __FUNCTION__);
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY, timeout_ms, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*step 8: check that we never encounter the load nonpaged MD from disk hook*/
    b_is_debug_hook_reached = FBE_FALSE;

    status = fbe_test_sep_background_pause_check_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[HOOK_INDEX_PASSIVE_SP_NONPAGED_LOAD],
                                                                &b_is_debug_hook_reached);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(b_is_debug_hook_reached == FBE_FALSE);
    status = fbe_test_sep_background_pause_remove_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[HOOK_INDEX_PASSIVE_SP_NONPAGED_LOAD],
                                                             HOOK_INDEX_PASSIVE_SP_NONPAGED_LOAD,
                                                             FBE_FALSE /* This is not a  reboot test */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    

    /*step 9: boot the dead SP now*/
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot dead SP ==", __FUNCTION__);
    status = red_riding_hood_reboot_cleanup(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot dead SP - complete ==", __FUNCTION__);

    /*remove the debug hook on the just booted SP*/
    status = fbe_api_sim_transport_set_target_server(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*step 10: wait for the object on just booted SP READY*/
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for object on the rebooted SP READY ==", __FUNCTION__);
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY, timeout_ms, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*As the LUN is newly created, so we should expect zero data this time*/
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data - start==", __FUNCTION__);        
    red_riding_hood_read_zero_background_pattern(rg_config_p, lun_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data - success==", __FUNCTION__);


    /* Step 11: Remove the luns and raid groups.     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove luns and raid groups ==", __FUNCTION__);
    status = fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 12: Create raid group and luns on the now active SP.   */  
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Create raid groups and luns ==", __FUNCTION__);
    red_riding_hood_create_rg_config(rg_config_p, 
                                     RED_RIDING_HOOD_LUNS_PER_RAID_GROUP,
                                     RED_RIDING_HOOD_CHUNKS_PER_LUN);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Create raid groups and luns - complete ==", __FUNCTION__);

    return;
}
/*********************************************************************
 * end red_riding_hood_dualsp_test_user_lun_preserved_when_peer_lost()
 *********************************************************************/


/*!***************************************************************************
 *          red_riding_hood_dualsp_test_user_lun_preserved_when_peer_lost_debug()
 *****************************************************************************
 *
 * @brief   This test validates that if the peer goes away before the lun
 *          non-paged metadata is populated, that the non-paged is read from
 *          disk.  It write a pattern to the luns on the raid group specified
 *          sets a debug hook on SPA then shutdown SPA.  Then it brings up
 *          SPA and immediately shutdowns SPB.
 *
 * @param   rg_config_p - The RG configuration for this test   
 *
 * @return  void
 *
 * @author
 *  11/29/2012  Ron Proulx  - Created
 *
 *****************************************************************************/
static void red_riding_hood_dualsp_test_user_lun_preserved_when_peer_lost_debug(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         rg_object_id;
    fbe_object_id_t                         lun_object_id;
/*
    fbe_u32_t                               hook_index = 0;
    fbe_bool_t                              b_is_debug_hook_reached = FBE_FALSE;
    fbe_u32_t                               current_time = 0;
    fbe_u32_t                               timeout_ms = FBE_TEST_HOOK_WAIT_MSEC;
*/
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    
    /* Set the target SP to the CMI active.
     */
    fbe_test_sep_get_active_target_id(&active_sp);
    this_sp = active_sp;
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* Wait for the database ready on both SPs.
     */
    //status = fbe_test_sep_util_wait_for_database_service(RED_RIDING_HOOD_DATABASE_READY_WAIT_MS)
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the raid group and lun object ids
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[0].lun_number, 
                                                   &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s Started for rg obj: 0x%x****\n", __FUNCTION__, rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");

    /* Step 1:  Write a known background pattern.
     */
    red_riding_hood_write_background_pattern(lun_object_id, RED_RIDING_HOOD_CHUNKS_PER_LUN * FBE_RAID_DEFAULT_CHUNK_SIZE);

    /* Step 2: Set a debug hook when we read the non-paged metadata for the
     *         lun object.
     */
    /* 
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set non-paged metadata load hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_set_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[hook_index],
                                                          hook_index,
                                                          lun_object_id,
                                                          SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_UPDATE,
                                                          FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_INIT,
                                                          0, 0,
                                                          SCHEDULER_CHECK_STATES,
                                                          SCHEDULER_DEBUG_ACTION_PAUSE,
    */
    //                                                      FBE_TRUE /* This is a reboot test */);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3:  Make the other SP active
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown this SP ==", __FUNCTION__);
    status = red_riding_hood_reboot(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown this SP - complete ==", __FUNCTION__);

    /* Step 5:  Immediately shutdown the other SP
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown other SP ==", __FUNCTION__);
    status = red_riding_hood_reboot(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown other SP - complete ==", __FUNCTION__);

    /* Step 4:  Boot the orignal SP with the previously configured debug hooks.
     */
    fbe_api_sim_transport_set_target_server(other_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot this SP ==", __FUNCTION__);
    status = red_riding_hood_reboot_cleanup(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot this SP - complete ==", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(this_sp);

    /* Step 6:  Validate that the `init non-paged' hook was hit
     */
    /*
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for init non-paged hook ==", __FUNCTION__);
    while ((b_is_debug_hook_reached == FBE_FALSE) &&
           (current_time < timeout_ms)               )
    {
        status = fbe_test_sep_background_pause_check_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[hook_index],
                                                                &b_is_debug_hook_reached);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (b_is_debug_hook_reached == FBE_FALSE)
        {
            fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
            current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        }
    }
    MUT_ASSERT_TRUE(b_is_debug_hook_reached == FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for init non-paged hook - success ==", __FUNCTION__);
    */

    /* Step 7:  Remove the debug hook and allow the non-paged to be initialized.
     */    
    //mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove init non-paged hook ==", __FUNCTION__);
    //status = fbe_test_sep_background_pause_remove_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[hook_index],
    //                                                         hook_index,
    //                                                         FBE_TRUE /* This is a reboot test */);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    //mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove init non-paged hook - complete ==", __FUNCTION__);

    /* Step 8:  Validate the data.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data written on this SP==", __FUNCTION__);
    red_riding_hood_read_background_pattern(rg_config_p, lun_object_id,
                                            FBE_TRUE,   /* Validate which SP wrote the data */
                                            this_sp,     /* This is the SP that wrote the data */
                                            RED_RIDING_HOOD_CHUNKS_PER_LUN * FBE_RAID_DEFAULT_CHUNK_SIZE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data - success==", __FUNCTION__);

    /* Step 9: Cleanup (boot other SP).
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup ==", __FUNCTION__);
    status = red_riding_hood_reboot_cleanup(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup - complete ==", __FUNCTION__);

    /* Step 10:  Validate the data (again)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data ==", __FUNCTION__);
    red_riding_hood_read_background_pattern(rg_config_p, lun_object_id,
                                            FBE_TRUE,   /* Validate which SP wrote the data */
                                            this_sp,     /* This is the SP that wrote the data */
                                            RED_RIDING_HOOD_CHUNKS_PER_LUN * FBE_RAID_DEFAULT_CHUNK_SIZE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data - success==", __FUNCTION__);

    return;
}
/***************************************************************************
 * end red_riding_hood_dualsp_test_user_lun_preserved_when_peer_lost_debug()
 ***************************************************************************/

/*!***************************************************************************
 *          red_riding_hood_dualsp_test_system_lun_preserved_when_peer_lost()
 *****************************************************************************
 *
 * @brief   This test validates that if the peer goes away before the lun
 *          non-paged metadata is populated, that the non-paged is read from
 *          disk.  It write a pattern to the luns on the raid group specified
 *          sets a debug hook on SPA then shutdown SPA.  Then it brings up
 *          SPA and immediately shutdowns SPB.
 *
 * @param   rg_config_p - The RG configuration for this test   
 *
 * @return  void
 *
 * @author
 *  01/04/2012  Ron Proulx  - Created
 *
 *****************************************************************************/
static void red_riding_hood_dualsp_test_system_lun_preserved_when_peer_lost(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         rg_object_id;
    fbe_object_id_t                         lun_object_id;
    fbe_u32_t                               hook_index = 0;
    fbe_bool_t                              b_is_debug_hook_reached = FBE_FALSE;
    fbe_u32_t                               current_time = 0;
    fbe_u32_t                               timeout_ms = FBE_TEST_HOOK_WAIT_MSEC;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_sim_transport_connection_target_t   passive_sp;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    
    /* Populate the raid group test configuration with the system 
     * raid group/lun information.
     */
    rg_config_p->raid_group_id = FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR;
    status = fbe_test_populate_system_rg_config(rg_config_p,
                                                RED_RIDING_HOOD_LUNS_PER_RAID_GROUP,
                                                RED_RIDING_HOOD_CHUNKS_PER_LUN);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the active and passive for the lun object.
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[0].lun_number, 
                                                   &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_util_get_active_passive_sp(lun_object_id, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "** %s for lun obj: 0x%x active: %d **\n", 
               __FUNCTION__, lun_object_id, active_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");
      
    /* Set the target SP to the active 
     */    
    this_sp = fbe_api_sim_transport_get_target_server();
    if (this_sp != active_sp)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Change current SP from: %d to %d ==", 
                   __FUNCTION__, this_sp, active_sp);
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        this_sp = active_sp;
    }
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s lun: 0x%x max lba: 0x%llx ==", 
                   __FUNCTION__, lun_object_id, rg_config_p->logical_unit_configuration_list[0].imported_capacity);
    /* Step 1:  Write a known background pattern.
     */
    red_riding_hood_write_background_pattern(lun_object_id, rg_config_p->logical_unit_configuration_list[0].capacity);

    /* Step 2: Set a debug hook when we read the non-paged metadata for the
     *         lun object.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set non-paged metadata load hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_set_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[hook_index],
                                                          hook_index,
                                                          lun_object_id,
                                                          SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_UPDATE,
                                                          FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_CHECK,
                                                          0, 0,
                                                          SCHEDULER_CHECK_STATES,
                                                          SCHEDULER_DEBUG_ACTION_PAUSE,
                                                          FBE_TRUE /* This is a reboot test */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3:  Make the other SP active
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown this SP ==", __FUNCTION__);
    status = red_riding_hood_reboot(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown this SP - complete ==", __FUNCTION__);

    /* Step 4:  Boot the orignal SP with the previously configured debug hooks.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot this SP ==", __FUNCTION__);
    status = red_riding_hood_reboot_cleanup(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot this SP - complete ==", __FUNCTION__);

    /* Step 5:  Immediately shutdown the other SP
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown other SP ==", __FUNCTION__);
    status = red_riding_hood_reboot(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown other SP - complete ==", __FUNCTION__);

    /* Step 6:  Validate that the `init non-paged' hook was hit
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for init non-paged hook ==", __FUNCTION__);
    while ((b_is_debug_hook_reached == FBE_FALSE) &&
           (current_time < timeout_ms)               )
    {
        status = fbe_test_sep_background_pause_check_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[hook_index],
                                                                &b_is_debug_hook_reached);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (b_is_debug_hook_reached == FBE_FALSE)
        {
            fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
            current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        }
    }
    MUT_ASSERT_TRUE(b_is_debug_hook_reached == FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for init non-paged hook - success ==", __FUNCTION__);

    /* Step 7:  Remove the debug hook and allow the non-paged to be initialized.
     */    
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove init non-paged hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_remove_debug_hook(&red_riding_hood_pause_params.scheduler_hooks[hook_index],
                                                             hook_index,
                                                             FBE_FALSE /* Peer is down  */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove init non-paged hook - complete ==", __FUNCTION__);

    /* Step 8:  Validate the data.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data written on this SP==", __FUNCTION__);
    red_riding_hood_read_background_pattern(rg_config_p, lun_object_id,
                                            FBE_TRUE,   /* Validate which SP wrote the data */
                                            this_sp,     /* This is the SP that wrote the data */
                                            rg_config_p->logical_unit_configuration_list[0].capacity);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data - success==", __FUNCTION__);

    /* Step 9: Cleanup (boot other SP).
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup ==", __FUNCTION__);
    status = red_riding_hood_reboot_cleanup(&red_riding_hood_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup - complete ==", __FUNCTION__);

    /* Step 10:  Validate the data (again)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data ==", __FUNCTION__);
    red_riding_hood_read_background_pattern(rg_config_p, lun_object_id,
                                            FBE_TRUE,   /* Validate which SP wrote the data */
                                            this_sp,     /* This is the SP that wrote the data */
                                            rg_config_p->logical_unit_configuration_list[0].capacity);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate data - success==", __FUNCTION__);

    return;
}
/***********************************************************************
 * end red_riding_hood_dualsp_test_system_lun_preserved_when_peer_lost()
 ***********************************************************************/

/*!****************************************************************************
 *  red_riding_hood_persit_pending_setup
 ******************************************************************************
 *
 * @brief
 *    This function performs the necessary setup that is required to run the 
 * persist pending test
 *
 * @param object_id - Object ID we want to setup
 *
 * @return  fbe_status_t
 *
 * 
 * @author
 *    04/26/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static fbe_status_t red_riding_hood_persit_pending_setup(fbe_object_id_t object_id)
{
    fbe_status_t status;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;
    fbe_api_logical_error_injection_record_t record_down;

    status = fbe_test_sep_util_get_active_passive_sp(object_id, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* We need to perform operation on the active SP */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Create a error record to delay the Non-paged IO for the object */
    red_riding_hood_populate_delay_io_error_record(object_id, &record_down);

    /* Enable Error injection on the Metadata LUN  */
    red_riding_hood_perform_error_injection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA, 
                                            &record_down);
    return status;
}

/*!****************************************************************************
 *  red_riding_hood_persit_pending_post_processing
 ******************************************************************************
 *
 * @brief
 *    This function performs the necessary post processing that is required to 
 * validate the persist pending test
 *
 * @param object_id - Object ID we want to process
 * @param rg_config_ptr - The RG configuration for this test   
 *
 * @return  fbe_status_t
 *
 * @author
 *    04/26/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static fbe_status_t red_riding_hood_persit_pending_post_processing(fbe_object_id_t object_id,
                                                                   fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;
    fbe_object_id_t lun_object_id;

    status = fbe_test_sep_util_get_active_passive_sp(object_id, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

     /* Wait for the LEI to see the error to be injected */
    status = red_riding_hood_wait_for_lei_error(1);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "LEI Error count unexpected\n");
    
    /* We need to setup the hooks on the passive SP and so set the target as passive SP*/
    status = fbe_api_sim_transport_set_target_server(passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "**** %s Adding debug hooks ****\n", __FUNCTION__);
    red_riding_hood_add_non_paged_persist_hook(object_id);
    red_riding_hood_add_lun_incomplete_write_verify_hook(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA);

    /* Set the target back to the active SP */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Reboot the active SP */
    red_riding_hood_shutdown_active_sp();

    /* validate that hook is hit */
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s Wait for Non-paged debug hooks ****\n", __FUNCTION__);
    red_riding_hood_wait_for_non_paged_persist_hook(object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "**** %s Wait for Incomplete write debug hooks ****\n", __FUNCTION__);
    red_riding_hood_wait_for_lun_incomplete_write_verify_hook(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA);

    red_riding_hood_del_non_paged_persist_hook(object_id);
    red_riding_hood_del_lun_incomplete_write_verify_hook(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA);

    /* Bring up the peer SP */
    darkwing_duck_startup_peer_sp(rg_config_p, NULL, NULL);

    /*wait for the READY of LUN before next step*/
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[0].lun_number, 
                                                   &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                          RED_RIDING_HOOD_TEST_WAIT_MSEC,
                                                                          FBE_PACKAGE_ID_SEP_0);
    return status;
}

/*!**************************************************************
 * red_riding_hood_shutdown_active_sp()
 ****************************************************************
 * @brief
 *  Do a hard reboot on the active sp
 *
 * @param  
 *
 * @return 
 *
 * @author
 *  04/24/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static void red_riding_hood_shutdown_active_sp(void) 
{
    fbe_sim_transport_connection_target_t   target_sp;
	fbe_sim_transport_connection_target_t   peer_sp;
    fbe_status_t status = FBE_STATUS_OK;

    /* Get the local SP ID */
    target_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, target_sp);

    mut_printf(MUT_LOG_LOW, " == SHUTDOWN ACTIVE SP (%s) == ", target_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(target_sp);
    fbe_test_sp_sim_stop_single_sp(target_sp == FBE_SIM_SP_A? FBE_TEST_SPA: FBE_TEST_SPB);
    /* we can't destroy fbe_api, because it's shared between two SPs */
    fbe_test_base_suite_startup_single_sp(target_sp);

    /* set the passive SP */
    peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end red_riding_hood_shutdown_active_sp()
 ******************************************/

/*!**************************************************************
 * red_riding_hood_populate_delay_io_error_record()
 ****************************************************************
 * @brief
 *  Populate the Error record structure with the error we want to 
 * inject for the persist pending test case
 *
 * @param  object_id - Object ID
 * @param error_record - Buffer to store the error record
 *
 * @return None
 *
 * @author
 *  04/24/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static void red_riding_hood_populate_delay_io_error_record(fbe_object_id_t object_id, 
                                                           fbe_api_logical_error_injection_record_t *error_record)
{
    error_record->pos_bitmap =  0x1;
    error_record->width =       0x10, /* width */
    error_record->opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED;
    error_record->lba = object_id; /* In case of NP record, Object ID is the LBA*/
    error_record->blocks = 0x1;
    error_record->err_type = FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN;
    error_record->err_mode = FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS;
    error_record->err_count = 0;
    error_record->err_limit = FBE_LOGICAL_ERROR_INJECTION_MAX_DELAY_MS; /* error limit is msec to delay*/
    error_record->skip_count = 0;
    error_record->skip_limit = 0;
    error_record->err_adj = 0;
    error_record->start_bit = 0;
    error_record->num_bits = 0;
    error_record->bit_adj = 0;
    error_record->crc_det = 0;
}

/*!**************************************************************
 * red_riding_hood_wait_for_lei_error()
 ****************************************************************
 * @brief
 *  This function waits for the specified number of errors to be hit
 * by the LEI
 *
 * @param  error_count - Number of errors to be hit
 *
 * @return FBE_STATUS_OK - If error number is expected
 *         FBE_STATUS_GENERIC_FAILURE - Otherwise
 *
 * @author
 *  04/24/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t red_riding_hood_wait_for_lei_error(fbe_u32_t error_count)
{
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = RED_RIDING_HOOD_TEST_WAIT_MSEC;

    while (current_time < timeout_ms)
    {
        fbe_api_logical_error_injection_get_stats(&stats);
        if(stats.num_errors_injected == error_count)
        {
            return FBE_STATUS_OK;
        }

        current_time = current_time + RED_RIDING_HOOD_POLLING_MSEC;
        fbe_api_sleep(RED_RIDING_HOOD_POLLING_MSEC);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: Did not hit the expected error count :%d, actual:%d !\n", 
                  __FUNCTION__, error_count, (int)stats.num_errors_injected);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!**************************************************************
 * red_riding_hood_perform_error_injection()
 ****************************************************************
 * @brief
 *  This function performs the error injection setup for the given object
 *
 * @param object_id - Object id to perform error injection 
 * 
 * @return fbe_status_t 
 *
 * @author
 *  03/17/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static void red_riding_hood_perform_error_injection(fbe_object_id_t object_id,
                                                    fbe_api_logical_error_injection_record_t *in_error_case_p)
{
    fbe_status_t status;
    fbe_api_logical_error_injection_get_stats_t stats;

    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // disable any previous error injection records
    status = fbe_api_logical_error_injection_disable_records( 0, 255 );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
    // add new error injection record for selected error case
    status = fbe_api_logical_error_injection_create_record(in_error_case_p );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
    // enable error injection on this provision drive
    status = fbe_api_logical_error_injection_enable_object( object_id, FBE_PACKAGE_ID_SEP_0 );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
    // enable error injection service
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
    // check status of enabling error injection service
    status = fbe_api_logical_error_injection_get_stats( &stats );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
    // check state of logical error injection service, should be enabled 
    MUT_ASSERT_INT_EQUAL( stats.b_enabled, FBE_TRUE );
    
    // check number of error injection records and objects; both should be one
    MUT_ASSERT_INT_EQUAL( stats.num_records, 1 );
    MUT_ASSERT_INT_EQUAL( stats.num_objects_enabled, 1 );
}

/*!**************************************************************
 *  red_riding_hood_dualsp_test()
 ****************************************************************
 * @brief
 *  Run a series of I/O tests to an object's paged metadata.
 *  The test runs dual-SP here.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 12/2012   Created.   Susan Rundbaken
 *
 ****************************************************************/
void red_riding_hood_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    rg_config_p = &red_riding_hood_raid_group_config_qual[0];
    fbe_test_run_test_on_rg_config(rg_config_p, 
                                   NULL, 
                                   red_riding_hood_run_dualsp_tests,
                                   RED_RIDING_HOOD_LUNS_PER_RAID_GROUP,
                                   RED_RIDING_HOOD_CHUNKS_PER_LUN);

    return;
}
/******************************************
 * end red_riding_hood_dualsp_test()
 ******************************************/

/*!**************************************************************
 *  red_riding_hood_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup the Hermione test for dual-SP testing.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  12/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/
void red_riding_hood_dualsp_setup(void)
{    
    fbe_sim_transport_connection_target_t sp;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        rg_config_p = &red_riding_hood_raid_group_config_qual[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        //elmo_load_sep_and_neit();

         /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    
        /* Load the physical config on the target server */    
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        //elmo_load_sep_and_neit();

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */

        sep_config_load_sep_and_neit_both_sps();
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    /* The following utility function does some setup for hardware. 
     */
    fbe_test_common_util_test_setup_init();
}
/**************************************
 * end red_riding_hood_dualsp_setup()
 **************************************/

/*!**************************************************************
 *  red_riding_hood_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the Hermione test for dual-SP testing.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 12/2011  Created. Susan Rundbaken 
 *
 ****************************************************************/
void red_riding_hood_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }
    return;
}
/******************************************
 * end red_riding_hood_dualsp_cleanup()
 ******************************************/

/*!**************************************************************
 * arwen_create_physical_config()
 ****************************************************************
 * @brief
 *  Configure test's physical configuration. Note that
 *  only 4160 block SAS and 520 block SAS drives are supported.
 *  We make all the disks(0_0_x) have the same capacity 
 *
 * @param b_use_4160 - TRUE if we will use both 4160 and 520.              
 *
 * @return None.
 *
 * @author
 *  10/19/2011 - Created. Zhangy
 *
 ****************************************************************/

void red_riding_hood_create_physical_config(fbe_bool_t b_use_4160)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;

    fbe_api_terminator_device_handle_t  port0_handle;
    fbe_api_terminator_device_handle_t  encl0_0_handle;
    fbe_api_terminator_device_handle_t  encl0_1_handle;
    fbe_api_terminator_device_handle_t  encl0_2_handle;
    fbe_api_terminator_device_handle_t  encl0_3_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t slot;
    fbe_u32_t secondary_block_size;

    if (b_use_4160)
    {
        secondary_block_size = 4160;
    }
    else
    {
        secondary_block_size = 520;
    }

    mut_printf(MUT_LOG_LOW, "=== %s configuring terminator ===", __FUNCTION__);
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                 2, /* portal */
                                 0, /* backend number */ 
                                 &port0_handle);

    /* insert an enclosures to port 0
     */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 1, port0_handle, &encl0_1_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 2, port0_handle, &encl0_2_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 3, port0_handle, &encl0_3_handle);

    /* Insert drives for enclosures.
     */
    for ( slot = 0; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
       
       //disks 0_0_x should have the same capacity 
       fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    for ( slot = 0; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    /*! @note Native SATA drives are no longer supported.
     */
    for ( slot = 0; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 2, slot, secondary_block_size, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    /*! @note Native SATA drives are no longer supported.
     */
    for ( slot = 0; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 3, slot, secondary_block_size, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    mut_printf(MUT_LOG_LOW, "=== starting physical package===");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(RED_HIDING_HOOD_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===");

    /* We inserted a armada board so we check for it.
     * board is always assumed to be object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == RED_HIDING_HOOD_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");
    fbe_test_pp_util_enable_trace_limits();
    return;
}
/******************************************
 * end arwen_create_physical_config()
 *******************************************


/*!****************************************************************************
 *  red_riding_hood_setup
 ******************************************************************************
 *
 * @brief
 *   This is the common setup function for the red_riding_hood test.  It creates the  
 *   physical configuration and loads the packages.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void red_riding_hood_setup(void)
{  

    /* Initialize any required fields and perform cleanup if required
     */
    if (fbe_test_util_is_simulation())
    {
        /*  Load the physical package and create the physical configuration. */
        red_riding_hood_create_physical_config(FBE_FALSE /* Do not use 4160 block drives */);

        /* Load the logical packages */
        sep_config_load_sep_and_neit();
    }

    fbe_test_common_util_test_setup_init();

} 

void red_riding_hood_cleanup(void)
{
    if (fbe_test_util_is_simulation())
         fbe_test_sep_util_destroy_neit_sep_physical();
    return;

}
/*!**************************************************************
 * red_riding_hood_rebuild_test()
 ****************************************************************
 * @brief
 *  step 1. pull out 0-0-1,0-0-2
 *  step 2. set(with persist) pvd(0-0-0)'s removed time stamp.
 *  step 3. reinserted 0-0-2, wait pvd(0-0-0) finish its rebuild.
 *  step 4. read nonpaged MD from disk 0-0-2, check pvd(0-0-0)'s removed timestamp
 *  step 5. reinserted 0-0-1, wait pvd(0-0-0) finish its rebuild.
 *  step 6. read nonpaged MD from disk 0-0-1, check pvd(0-0-0)'s removed timestamp
 * @param None.               
 *
 * @return None.
 *
 * @author  zhangy26
 *
 ****************************************************************/

static void red_riding_hood_rebuild_test(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot)
{

		fbe_status_t status;
        fbe_object_id_t             pvd_object_id;                        // pvd's object id    
        fbe_object_id_t             pvd_object_id_1;                        // pvd's object id    
        fbe_object_id_t             pvd_object_id_2;                        // pvd's object id    

		fbe_api_terminator_device_handle_t  drive_handle_p;
		
		fbe_api_terminator_device_handle_t  drive_handle_p_2;
		fbe_system_time_t    set_timestamp = {0};
		fbe_system_time_t    get_timestamp = {0};
        fbe_u16_t  first_day = 55;

		
		/* Get the PVD object id before we remove the drive */
        status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                                enclosure,
                                                                2,
                                                                &pvd_object_id_2);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			
        mut_printf(MUT_LOG_TEST_STATUS, "removing drive: %d - %d - %d\n", bus, enclosure, 2);

        status = fbe_test_pp_util_pull_drive(bus, enclosure, 2, &drive_handle_p_2);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);                 
    
        /*    Print message as to where the test is at */
        mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d, PVD: 0x%X\n",
                   bus, enclosure, 2, pvd_object_id_2);
    
        /*    Verify that the PVD  are in the FAIL state */
        sep_rebuild_utils_verify_sep_object_state(pvd_object_id_2, FBE_LIFECYCLE_STATE_FAIL);

        /* Get the PVD object id before we remove the drive */
        status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                                enclosure,
                                                                slot,
                                                                &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		
        mut_printf(MUT_LOG_TEST_STATUS, "removing drive: %d - %d - %d\n", bus, enclosure, slot);

        status = fbe_test_pp_util_pull_drive(bus, enclosure, slot, &drive_handle_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);                 
    
       /*    Print message as to where the test is at */
        mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d, PVD: 0x%X\n", 
                   bus, enclosure, slot, pvd_object_id);
    
        /*    Verify that the PVD  are in the FAIL state */
        sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);

		/*check 0-0-2 and 0-0-1 removed time stamp*/
		status = fbe_api_provision_drive_get_removed_timestamp(pvd_object_id_2, &get_timestamp);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "object id %x :get time stamp hour: %d ,minute %d, second %d\n",pvd_object_id_2, get_timestamp.hour, get_timestamp.minute,get_timestamp.second);
		fbe_zero_memory(&get_timestamp,sizeof(fbe_system_time_t));

		status = fbe_api_provision_drive_get_removed_timestamp(pvd_object_id, &get_timestamp);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "object id %x :get time stamp hour: %d ,minute %d, second %d\n",pvd_object_id, get_timestamp.hour, get_timestamp.minute,get_timestamp.second);
		fbe_zero_memory(&get_timestamp,sizeof(fbe_system_time_t));

		/* Get the PVD object id  */
        status = fbe_api_provision_drive_get_obj_id_by_location(bus, enclosure, 0, &pvd_object_id_1);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		    
		
		mut_printf(MUT_LOG_LOW, "Non Paged Metadata System Rebuild Test Started ...\n");
        status = fbe_api_provision_drive_get_removed_timestamp(pvd_object_id_1, &set_timestamp);
        mut_printf(MUT_LOG_TEST_STATUS, "object id %x :original time stamp hour: %d ,minute %d, second  %d\n",pvd_object_id_1, set_timestamp.hour, set_timestamp.minute,set_timestamp.second);

		set_timestamp.day = first_day;
		/*Set 0-0-0 removed time stamp to a new value*/
		
		mut_printf(MUT_LOG_LOW, "Set removed time stamp\n");
		status = fbe_api_provision_drive_set_removed_timestamp(pvd_object_id_1 , set_timestamp,FBE_TRUE);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		
		fbe_api_sleep(5000);
		status = fbe_api_provision_drive_get_removed_timestamp(pvd_object_id_1, &get_timestamp);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "get time stamp day: %d ,set time stamp day %d\n", get_timestamp.day, set_timestamp.day);
		MUT_ASSERT_TRUE(get_timestamp.day == first_day);

         /*	Print message as to where the test is at */
        mut_printf(MUT_LOG_TEST_STATUS, "Reinsert Drive: %d_%d_%d, PVD: 0x%X\n", 
                   bus, enclosure, 2, pvd_object_id_2);

		status = fbe_test_pp_util_reinsert_drive(bus,  enclosure, 2, drive_handle_p_2);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);				   
		
		 /*	Verify that the PVD  are in the READY state */
		sep_rebuild_utils_verify_sep_object_state(pvd_object_id_2, FBE_LIFECYCLE_STATE_READY);

        /* Wait for the drive we just inserted to rebuild.
         * We now expect that only drive 1 is failed. 
         */
        status = fbe_test_sep_util_wait_for_degraded_bitmask(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAW_MIRROR_RG, 
                                                             0x2, FBE_TEST_WAIT_TIMEOUT_MS);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_sep_util_wait_for_degraded_bitmask(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, 
                                                             0x2, FBE_TEST_WAIT_TIMEOUT_MS);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

		mut_printf(MUT_LOG_LOW, "nonpaged system load from 0-0-2\n");
		/* Load from 0-0-2 */
		fbe_api_metadata_nonpaged_system_load_with_disk_bitmask((fbe_u16_t)4);
		/* Check that we have the first value */
		status = fbe_api_provision_drive_get_removed_timestamp(pvd_object_id_1, &get_timestamp);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		mut_printf(MUT_LOG_LOW, "Check that we have the first value\n");
		MUT_ASSERT_TRUE(get_timestamp.day == first_day);


		 /*    Print message as to where the test is at */
        mut_printf(MUT_LOG_TEST_STATUS, "Reinsert Drive: %d_%d_%d, PVD: 0x%X\n", 
                   bus, enclosure, slot, pvd_object_id);
    
        status = fbe_test_pp_util_reinsert_drive(bus, enclosure, slot, drive_handle_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);                 
		
       
        /*    Verify that the PVD  are in the READY state */
        sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY);
        status = fbe_test_sep_util_wait_for_degraded_bitmask(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAW_MIRROR_RG, 
                                                             0x0, /* not degraded */ FBE_TEST_WAIT_TIMEOUT_MS);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_sep_util_wait_for_degraded_bitmask(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, 
                                                             0x0, FBE_TEST_WAIT_TIMEOUT_MS);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		mut_printf(MUT_LOG_LOW, "nonpaged system load from 0-0-1\n");

		/* Load from 0-0-1 */
		fbe_api_metadata_nonpaged_system_load_with_disk_bitmask((fbe_u16_t)2);
		/* Check that we have the first value */
		status = fbe_api_provision_drive_get_removed_timestamp(pvd_object_id_1, &get_timestamp);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		mut_printf(MUT_LOG_LOW, "Check that we have the first value\n");
		MUT_ASSERT_TRUE(get_timestamp.day == first_day);
		fbe_zero_memory(&get_timestamp,sizeof(fbe_system_time_t));
		
		/*clear 0-0-0 pvd's removed time stamp*/
		fbe_zero_memory(&set_timestamp,sizeof(fbe_system_time_t));
		mut_printf(MUT_LOG_LOW, "Clear 0-0-0 removed time stamp\n");
		status = fbe_api_provision_drive_set_removed_timestamp(pvd_object_id_1 , set_timestamp,FBE_TRUE);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        return;
    
}

static void red_riding_hood_NP_magic_number_test(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot)
{

		fbe_status_t status;
        fbe_object_id_t             pvd_object_id;                        // pvd's object id    
		fbe_api_terminator_device_handle_t  drive_handle_p;
        fbe_u32_t  failed_drivers_bitmask = 0;
		/* Get the PVD object id before we remove the drive */
        status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                                enclosure,
                                                                slot,
                                                                &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		
        mut_printf(MUT_LOG_TEST_STATUS, "removing drive: %d - %d - %d\n", bus, enclosure, slot);
        status = fbe_test_pp_util_pull_drive(bus, enclosure, slot, &drive_handle_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);                 
    
       /*    Print message as to where the test is at */
        mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d, PVD: 0x%X\n", 
                   bus, enclosure, slot, pvd_object_id);
    
        /*    Verify that the PVD  are in the FAIL state */
        sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
		
		/*reboot sep*/
		fbe_test_sep_util_destroy_neit_sep();
		sep_config_load_sep_and_neit();
		status = fbe_test_sep_util_wait_for_database_service(30000);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
		/*inject read errors to physical drive 0_0_0 and test if it can
		   handle it approapriately */
		 mut_printf(MUT_LOG_TEST_STATUS, "Starting nonpaged metadata state test");
		 status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS); 
		 MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		   
		/*	Enable the error injection service*/
		 status = fbe_api_logical_error_injection_enable();
		 MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		 failed_drivers_bitmask = RED_RIDING_HOOD_FAILED_DRIVERS_BITMASK & ~(0x1 << slot);
		 mut_printf(MUT_LOG_TEST_STATUS, "inject errors to all raw mirror disks and load from raw mirror");
		 red_riding_hood_inject_read_errors_to_system_drive(failed_drivers_bitmask,pvd_object_id,1,
                                                            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_SEP_RAW_MIRROR_RG,
                                                            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_NP);

		 /* Load from 0-0-0 ,error injection will change read mode to single block mode read
		    and stuck reinserted pvd in SPECIALIZD state*/
		// fbe_api_metadata_nonpaged_system_load_with_disk_bitmask((fbe_u16_t)1);
		 fbe_api_metadata_nonpaged_system_load();

		 status = fbe_test_pp_util_reinsert_drive(bus,
												 enclosure,
												 slot,
												 drive_handle_p);
		 MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
		 fbe_api_sleep(30000);
		 /*    Verify that the PVD	are stuck in specialized state */
		 sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_SPECIALIZE);

		/*set the dafault NP for pvd object*/
		status = fbe_api_base_config_metadata_set_default_nonpaged_metadata(pvd_object_id);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		/*check the object is in READY state*/
		fbe_api_sleep(3000);
	    sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY);


		/*stop error injection*/
		status = fbe_api_protocol_error_injection_stop();
		MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
		status = fbe_api_logical_error_injection_disable();
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		
        status = fbe_test_sep_util_wait_for_degraded_bitmask(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAW_MIRROR_RG, 
                                                             0x0, /* not degraded */ FBE_TEST_WAIT_TIMEOUT_MS);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_sep_util_wait_for_degraded_bitmask(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, 
                                                             0x0, FBE_TEST_WAIT_TIMEOUT_MS);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
       return;
    
}
static void red_riding_hood_NP_state_test_phase_two(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot)
{

		fbe_status_t status;
        fbe_object_id_t             pvd_object_id;                        // pvd's object id    
		fbe_api_terminator_device_handle_t  drive_handle_p;
        fbe_u32_t  failed_drivers_bitmask = 0;
		fbe_metadata_nonpaged_data_t nonpaged_data;
		/* Get the PVD object id before we remove the drive */
        status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                                enclosure,
                                                                slot,
                                                                &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		
        mut_printf(MUT_LOG_TEST_STATUS, "removing drive: %d - %d - %d\n", bus, enclosure, slot);
                
        status = fbe_test_pp_util_pull_drive(bus, enclosure, slot, &drive_handle_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);                 
    
        /*    Print message as to where the test is at */
        mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d, PVD: 0x%X\n", 
                   bus, enclosure, slot, pvd_object_id);
    
        /*    Verify that the PVD  are in the FAIL state */
        sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
		
		status = fbe_api_base_config_metadata_get_data(pvd_object_id, &nonpaged_data);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		/*reboot sep*/
		fbe_test_sep_util_destroy_neit_sep();
		sep_config_load_sep_and_neit();
		
		
		/*inject read errors to physical drive 0_0_0 and test if it can
		   handle it approapriately */
		 mut_printf(MUT_LOG_TEST_STATUS, "Starting nonpaged metadata state phase two test");
		 status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS); 
		 MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		   
		/*	Enable the error injection service*/
		 status = fbe_api_logical_error_injection_enable();
		 MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		 failed_drivers_bitmask = RED_RIDING_HOOD_FAILED_DRIVERS_BITMASK & ~(0x1 << slot);
		 mut_printf(MUT_LOG_TEST_STATUS, "inject errors to all raw mirror disks and load from raw mirror");
		 red_riding_hood_inject_read_errors_to_system_drive(failed_drivers_bitmask,pvd_object_id,1,
                                                            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_SEP_RAW_MIRROR_RG,
                                                            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_NP);

		 /* Load from 0-0-0 ,error injection will change read mode to single block mode read
		    and stuck reinserted pvd in SPECIALIZD state*/
		// fbe_api_metadata_nonpaged_system_load_with_disk_bitmask((fbe_u16_t)1);
		 fbe_api_metadata_nonpaged_system_load();

		 status = fbe_test_pp_util_reinsert_drive(bus,
												 enclosure,
												 slot,
												 drive_handle_p);
		 MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
		 fbe_api_sleep(30000);
		 /*    Verify that the PVD	are stuck in specialized state */
		 sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_SPECIALIZE);

		/*stop error injection*/
		status = fbe_api_protocol_error_injection_stop();
		MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
		status = fbe_api_logical_error_injection_disable();
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

		/*set the dafault NP for pvd object*/
		status = fbe_api_base_config_metadata_set_nonpaged_metadata(pvd_object_id,nonpaged_data.data,sizeof(fbe_provision_drive_nonpaged_metadata_t));
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		/*check the object is in READY state*/
		fbe_api_sleep(3000);
	    sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY);

        status = fbe_test_sep_util_wait_for_degraded_bitmask(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAW_MIRROR_RG, 
                                                             0x0, /* not degraded */ FBE_TEST_WAIT_TIMEOUT_MS);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		
        status = fbe_test_sep_util_wait_for_degraded_bitmask(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, 
                                                             0x0, FBE_TEST_WAIT_TIMEOUT_MS);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
       return;
    
}

/*!**************************************************************
 * red_riding_hood_test_register_spare_notification()
 ****************************************************************
 * @brief
 *  Register spare notification.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *
 ****************************************************************/
static void red_riding_hood_test_register_spare_notification(fbe_object_id_t object_id, 
                                         red_riding_hood_test_spare_context_t* context, 
                                         fbe_notification_registration_id_t* registration_id)
{
    fbe_status_t    status;

    MUT_ASSERT_TRUE(NULL != context);
    MUT_ASSERT_TRUE(NULL != registration_id);

    context->object_id = object_id;
    fbe_semaphore_init(&(context->sem), 0, 1);

    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED, 
		FBE_PACKAGE_NOTIFICATION_ID_SEP_0, FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE, 
		red_riding_hood_spare_callback_function, context, registration_id);
	
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
static void  red_riding_hood_spare_callback_function(update_object_msg_t* update_obj_msg,
                                                     void* context)
{
    red_riding_hood_test_spare_context_t*  spare_context = (red_riding_hood_test_spare_context_t*)context;

    MUT_ASSERT_TRUE(NULL != update_obj_msg);
    MUT_ASSERT_TRUE(NULL != context);

    if(update_obj_msg->object_id == spare_context->object_id)
    {
        if(FBE_JOB_TYPE_DRIVE_SWAP != update_obj_msg->notification_info.notification_data.job_service_error_info.job_type)
            return;
        spare_context->job_srv_info = update_obj_msg->notification_info.notification_data.job_service_error_info;
        fbe_semaphore_release(&spare_context->sem, 0, 1, FALSE);
    }
}
static void  red_riding_hood_test_wait_spare_notification(red_riding_hood_test_spare_context_t* spare_context)
{
    fbe_status_t  status;
	
    MUT_ASSERT_TRUE(NULL != spare_context);
	
    status = fbe_semaphore_wait_ms(&(spare_context->sem), RED_RIDING_HOOD_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

static void  red_riding_hood_test_unregister_spare_notification(fbe_object_id_t object_id, 
                                          fbe_notification_registration_id_t registration_id)
{
    fbe_status_t  status;
    status = fbe_api_notification_interface_unregister_notification(red_riding_hood_spare_callback_function ,
		 registration_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
static void  red_riding_hood_test_get_spare_in_pvd_id(fbe_object_id_t vd_id, fbe_object_id_t* pvd_id)
{
    fbe_status_t status;
    fbe_virtual_drive_configuration_mode_t vd_cfg_mode;
    fbe_edge_index_t  edge_index;
    fbe_api_get_block_edge_info_t  edge_info;

    MUT_ASSERT_TRUE(pvd_id != NULL);

    fbe_test_sep_util_get_virtual_drive_configuration_mode(vd_id, &vd_cfg_mode);
    MUT_ASSERT_INT_NOT_EQUAL(vd_cfg_mode, FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN);
    if(FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE == vd_cfg_mode)
        edge_index = 0;
    else
        edge_index = 1;

    status = fbe_api_wait_for_block_edge_path_state(vd_id, edge_index, FBE_PATH_STATE_ENABLED,
		    FBE_PACKAGE_ID_SEP_0, RED_RIDING_HOOD_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_block_edge_info(vd_id, edge_index, &edge_info, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    *pvd_id = edge_info.server_id;
}
static fbe_status_t red_riding_hood_inject_errors_to_system_drive(fbe_u32_t fail_drives)
{
    fbe_object_id_t                                     object_id[4];
    fbe_status_t                                        status;
    fbe_u32_t                                           i;
    fbe_protocol_error_injection_error_record_t         record;
    fbe_protocol_error_injection_record_handle_t        out_record_handle_p[4];
    fbe_private_space_layout_region_t region_info;
    fbe_private_space_layout_lun_info_t lun_info;
	fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_SEP_RAW_MIRROR_RG, &region_info);
    fbe_private_space_layout_get_lun_by_lun_number(FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_NP, &lun_info);

    
    fbe_test_neit_utils_init_error_injection_record(&record);
    record.lba_start                = region_info.starting_block_address + lun_info.raid_group_address_offset;
    record.lba_end                  = region_info.starting_block_address + lun_info.raid_group_address_offset + lun_info.external_capacity - 1;
    record.num_of_times_to_insert   = FBE_U32_MAX;
           
    record.protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = FBE_SCSI_WRITE;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[1] = FBE_SCSI_WRITE_6;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[2] = FBE_SCSI_WRITE_10;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[3] = FBE_SCSI_WRITE_12;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[4] = FBE_SCSI_WRITE_16;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = FBE_SCSI_SENSE_KEY_MEDIUM_ERROR;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code =   FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_GENERAL_QUALIFIER;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    for (i = 0; i < PROTOCOL_ERROR_INJECTION_MAX_ERRORS ; i++) {
        if (i <= (record.lba_end - record.lba_start)) {
            record.b_active[i] = FBE_TRUE;
        }
    } 
    record.frequency = 0;
    record.b_test_for_media_error = FBE_FALSE;
    record.reassign_count = 0;

    for (i = 0; i < 3; i++) {
        if (fail_drives & (0x1 << i)) {
            status = fbe_api_get_physical_drive_object_id_by_location(0, 0, i, &object_id[i]);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            record.object_id = object_id[i];
            status = fbe_api_protocol_error_injection_add_record(&record, &out_record_handle_p[i]);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        }
    }

    status = fbe_api_protocol_error_injection_start();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return FBE_STATUS_OK;
}
static void red_riding_hood_write_error_injection_test(void)
{
    fbe_status_t status = FBE_STATUS_OK;
	fbe_object_id_t  pvd_object_id;
	fbe_system_time_t	 set_timestamp = {0};
	
	/* Get the PVD object id before we issue write */
	status = fbe_api_provision_drive_get_obj_id_by_location(0,
															0,
															0,
															&pvd_object_id);
	set_timestamp.day = 44;

   /*inject write errors to physical drive 0_0_0 and test if it can
      handle it approapriately */
    mut_printf(MUT_LOG_TEST_STATUS, "Starting write error injection test");
    status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	  
   /*  Enable the error injection service*/
    status = fbe_api_logical_error_injection_enable();
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	  
	mut_printf(MUT_LOG_TEST_STATUS, "inject errors to disk 0_0_0 and do raw-mirror IO");
	red_riding_hood_inject_errors_to_system_drive(0x1);
    /*issue write to metadata raw mirror*/
	
	status = fbe_api_provision_drive_set_removed_timestamp(pvd_object_id , set_timestamp,FBE_TRUE);

    /* Note that since we have a raw mirror object, this write succeeds even though the drive fails.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	/*stop error injection*/
    status = fbe_api_protocol_error_injection_stop();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

}

static void red_riding_hood_add_non_paged_persist_hook(fbe_object_id_t object_id)
{
    fbe_status_t status;
    status = fbe_api_scheduler_add_debug_hook(object_id, SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_PERSIST,
                                              FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_PERSIST, 
                                              NULL,
                                              NULL, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
static void red_riding_hood_del_non_paged_persist_hook(fbe_object_id_t object_id)
{
    fbe_status_t status;
    status = fbe_api_scheduler_del_debug_hook(object_id, SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_PERSIST,
                                              FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_PERSIST, 
                                              NULL,
                                              NULL, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

}
static void red_riding_hood_wait_for_non_paged_persist_hook(fbe_object_id_t object_id)
{
    fbe_status_t status;

    status = fbe_test_wait_for_debug_hook(object_id,
                                         SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_PERSIST,
                                         FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_PERSIST,
                                         SCHEDULER_CHECK_STATES,
                                         SCHEDULER_DEBUG_ACTION_PAUSE,
                                         NULL, NULL);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Wait for Non Paged Perist hook failed\n");
    return;
}

static void red_riding_hood_add_lun_incomplete_write_verify_hook(fbe_object_id_t object_id)
{
    fbe_status_t status;
    status = fbe_api_scheduler_add_debug_hook(object_id, SCHEDULER_MONITOR_STATE_LUN_INCOMPLETE_WRITE_VERIFY,
                                              FBE_LUN_SUBSTATE_INCOMPLETE_WRITE_VERIFY_REQUESTED, 
                                              NULL,
                                              NULL, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
static void red_riding_hood_del_lun_incomplete_write_verify_hook(fbe_object_id_t object_id)
{
    fbe_status_t status;
    status = fbe_api_scheduler_del_debug_hook(object_id, SCHEDULER_MONITOR_STATE_LUN_INCOMPLETE_WRITE_VERIFY,
                                              FBE_LUN_SUBSTATE_INCOMPLETE_WRITE_VERIFY_REQUESTED, 
                                              NULL,
                                              NULL, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

}
static void red_riding_hood_wait_for_lun_incomplete_write_verify_hook(fbe_object_id_t object_id)
{
    fbe_status_t status;

    status = fbe_test_wait_for_debug_hook(object_id,
                                         SCHEDULER_MONITOR_STATE_LUN_INCOMPLETE_WRITE_VERIFY,
                                         FBE_LUN_SUBSTATE_INCOMPLETE_WRITE_VERIFY_REQUESTED,
                                         SCHEDULER_CHECK_STATES,
                                         SCHEDULER_DEBUG_ACTION_CONTINUE,
                                         NULL, NULL);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Wait for incomplete write verify hook failed\n");
    return;
}
static fbe_status_t red_riding_hood_inject_read_errors_to_system_drive(fbe_u32_t fail_drives,
                                                                       fbe_u32_t start_offset,
                                                                       fbe_u32_t block_count,
                                                                       fbe_private_space_layout_region_id_t region_id,
                                                                       fbe_lun_number_t lun_number)
{
    fbe_object_id_t                                     object_id[4];
    fbe_status_t                                        status;
    fbe_u32_t                                           i;
    fbe_protocol_error_injection_error_record_t         record = {0};
    fbe_protocol_error_injection_record_handle_t        out_record_handle_p[4];
    fbe_lba_t start_lba;
    fbe_private_space_layout_region_t region_info;
    fbe_private_space_layout_lun_info_t lun_info;
	fbe_private_space_layout_get_region_by_region_id(region_id, &region_info);
    fbe_private_space_layout_get_lun_by_lun_number(lun_number, &lun_info);
    start_lba = region_info.starting_block_address + lun_info.raid_group_address_offset;

    fbe_test_neit_utils_init_error_injection_record(&record);
    record.lba_start                = start_lba + start_offset; 
    record.lba_end                  = start_lba + start_offset + block_count - 1;
    record.num_of_times_to_insert   = FBE_U32_MAX;

    record.protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = FBE_SCSI_READ;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[1] = FBE_SCSI_READ_6;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[2] = FBE_SCSI_READ_10;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[3] = FBE_SCSI_READ_12;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[4] = FBE_SCSI_READ_16;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = FBE_SCSI_SENSE_KEY_MEDIUM_ERROR;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code =   FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_NO_ADDITIONAL_SENSE_INFO;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    for (i = 0; i < PROTOCOL_ERROR_INJECTION_MAX_ERRORS ; i++) {
        if (i <= (record.lba_end - record.lba_start)) {
            record.b_active[i] = FBE_TRUE;
        }
    } 
    record.frequency = 0;
    record.b_test_for_media_error = FBE_FALSE;
    record.reassign_count = 0;

    for (i = 0; i < 3; i++) {
        if (fail_drives & (0x1 << i)) {
            status = fbe_api_get_physical_drive_object_id_by_location(0, 0, i, &object_id[i]);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            record.object_id = object_id[i];
            status = fbe_api_protocol_error_injection_add_record(&record, &out_record_handle_p[i]);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        }
    }

    status = fbe_api_protocol_error_injection_start();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return FBE_STATUS_OK;
}
static void red_riding_hood_read_error_injection_test(void)
{
    fbe_status_t status = FBE_STATUS_OK;
	fbe_object_id_t  pvd_object_id;
	fbe_object_id_t  pvd_object_id_1;
	fbe_object_id_t  pvd_object_id_2;
	fbe_system_time_t	 set_timestamp = {0};
	fbe_system_time_t    get_timestamp = {0};
	fbe_u16_t            day_in_memory = 60;
	fbe_u16_t            day_in_disk = 66;
    fbe_sim_transport_connection_target_t   active_sp = FBE_SIM_SP_A;               
	
	/* Get the PVD object id before we issue write */
	status = fbe_api_provision_drive_get_obj_id_by_location(0,
															0,
															0,
															&pvd_object_id);
	
	/* Get the PVD object id before we issue write */
	status = fbe_api_provision_drive_get_obj_id_by_location(0,
															0,
															1,
															&pvd_object_id_1);
	/* Get the PVD object id before we issue write */
	status = fbe_api_provision_drive_get_obj_id_by_location(0,
															0,
															2,
															&pvd_object_id_2);
	set_timestamp.day = day_in_disk;
	
	mut_printf(MUT_LOG_LOW, "Set and persist removed time stamp for object %X\n",pvd_object_id);
	status = fbe_api_provision_drive_set_removed_timestamp(pvd_object_id , set_timestamp,FBE_TRUE);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	
	mut_printf(MUT_LOG_LOW, "Set and persist removed time stamp for object %X\n",pvd_object_id_1);
	status = fbe_api_provision_drive_set_removed_timestamp(pvd_object_id_1 , set_timestamp,FBE_TRUE);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	
	mut_printf(MUT_LOG_LOW, "Set and persist removed time stamp for object %X\n",pvd_object_id_2);
	status = fbe_api_provision_drive_set_removed_timestamp(pvd_object_id_2 , set_timestamp,FBE_TRUE);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	
	set_timestamp.day = day_in_memory;
	mut_printf(MUT_LOG_LOW, "Set removed time stamp for object %X\n",pvd_object_id);
	status = fbe_api_provision_drive_set_removed_timestamp(pvd_object_id , set_timestamp,FBE_FALSE);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	
	mut_printf(MUT_LOG_LOW, "Set removed time stamp for object %X\n",pvd_object_id_1);
	status = fbe_api_provision_drive_set_removed_timestamp(pvd_object_id_1 , set_timestamp,FBE_FALSE);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	
	mut_printf(MUT_LOG_LOW, "Set removed time stamp for object %X\n",pvd_object_id_2);
	status = fbe_api_provision_drive_set_removed_timestamp(pvd_object_id_2 , set_timestamp,FBE_FALSE);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /*make sure we have changed the value in memory*/
	status = fbe_api_provision_drive_get_removed_timestamp(pvd_object_id, &get_timestamp);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_timestamp.day == day_in_memory);
	get_timestamp.day = 0;
	status = fbe_api_provision_drive_get_removed_timestamp(pvd_object_id_1, &get_timestamp);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_timestamp.day == day_in_memory);
	get_timestamp.day = 0;
	status = fbe_api_provision_drive_get_removed_timestamp(pvd_object_id_2, &get_timestamp);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_timestamp.day == day_in_memory);
	mut_printf(MUT_LOG_LOW, "we have changed the value in memory\n");

   /*inject write errors to physical drive 0_0_0 and test if it can
      handle it approapriately */
    mut_printf(MUT_LOG_TEST_STATUS, "Starting write error injection test");
    status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	  
   /*  Enable the error injection service*/
    status = fbe_api_logical_error_injection_enable();
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	  
	mut_printf(MUT_LOG_TEST_STATUS, "inject errors to all raw mirror disks and load from raw mirror");
	red_riding_hood_inject_read_errors_to_system_drive(RED_RIDING_HOOD_FAILED_DRIVERS_BITMASK,pvd_object_id_1,1,
                                                       FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_SEP_RAW_MIRROR_RG,
                                                       FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_NP);
	/* Load from 0-0-0 ,error injection will change read mode to single block mode read*/
	//fbe_api_metadata_nonpaged_system_load_with_disk_bitmask((fbe_u16_t)1);
	fbe_api_metadata_nonpaged_system_load();
	status = fbe_api_provision_drive_get_removed_timestamp(pvd_object_id, &get_timestamp);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "object: %X,get time stamp day : %d ,set time stamp day %d\n",pvd_object_id, get_timestamp.day, day_in_disk);
	MUT_ASSERT_TRUE(get_timestamp.day == day_in_disk);
	get_timestamp.day = 0;
	status = fbe_api_provision_drive_get_removed_timestamp(pvd_object_id_2, &get_timestamp);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "object: %X,get time stamp day : %d ,set time stamp day %d\n",pvd_object_id_2, get_timestamp.day, day_in_disk);
	MUT_ASSERT_TRUE(get_timestamp.day == day_in_disk);
	/*make sure pvd(0-0-1) cann't load its NP from raw mirror*/
	get_timestamp.day = 0;
	status = fbe_api_provision_drive_get_removed_timestamp(pvd_object_id_1, &get_timestamp);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "object: %X,get time stamp day : %d ,time stamp in memory %d\n",pvd_object_id_1, get_timestamp.day, day_in_memory);
	MUT_ASSERT_TRUE(get_timestamp.day == day_in_memory);

	/*stop error injection*/
    status = fbe_api_protocol_error_injection_stop();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* by doing the error injection earlier, all object non-paged metadata state will be changed to invalid
     * there's no better way to clean up this, because we may hit assert during normal destroy,
     * we need to shutdown this SP, and reload it.
     */
    fbe_api_sim_transport_set_target_server(active_sp);

    mut_printf(MUT_LOG_LOW, " == SHUTDOWN active sp %s == ", active_sp == FBE_SIM_SP_A ? "SP A"
                                                                      : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA
                                                      : FBE_TEST_SPB);

    fbe_test_base_suite_startup_single_sp(active_sp);

    red_riding_hood_setup();
}

/*!****************************************************************************
 *  red_riding_hood_np_load_error_test
 ******************************************************************************
 *
 * @brief
 *    This tests validate the Non paged metadata read errors for Non-System objects
 *
 * @param - None
 *
 * @return  void
 *
 * @notes: 
 *     This test validates when there are errors reading the Non-paged Data
 *     1. The object is stuck in specialize state
 *     2. Only one object is stuck in that state
 *     3. The rest of the objects are good(This will validate our single block
 *        read algorithm)
 * 
 * @author
 *    06/26/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static void red_riding_hood_np_load_error_test(void)
{
    fbe_object_id_t pvd_object_id_1, pvd_object_id_2, pvd_object_id_3;
    fbe_status_t status;
    fbe_api_terminator_device_handle_t  drive_handle_p_1, drive_handle_p_2, drive_handle_p_3;
    fbe_u32_t bus,encl,slot;
    fbe_metadata_nonpaged_data_t nonpaged_data;

    bus = 0;
    encl = 1;
    slot = 0;

    mut_printf(MUT_LOG_TEST_STATUS, 
               "Starting NP Read Error test for Non-System Objects\n");

    /* The Non-paged data is read only when the object loads for the first time
     * i.e in specialize state. In order to force that scenario, we insert the
     * drive after injecting errors:
     * 1. Remove the drive
     * 2. Reboot the SP
     * 3. Inject Errors
     * 4. Force read of NP Metadata
     * 5. Insert the object
     */
    /* Get the PVD object id before we issue write */
	status = fbe_api_provision_drive_get_obj_id_by_location(bus, encl, slot, &pvd_object_id_1);

    status = fbe_api_base_config_metadata_get_data(pvd_object_id_1, &nonpaged_data);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, 
               "removing drive: %d_%d_%d. PVD ID:0x%x\n", 
               bus, encl, slot, pvd_object_id_1);

    status = fbe_test_pp_util_pull_drive(bus, encl, slot, &drive_handle_p_1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);                 

    /* Remove two other drives also. The idea being since our algorithm narrows it down to 
     * single block, the other object should be okay. So we will remove it and re-insert 
     * further down to see if they are okay */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus, encl, slot+1,
															&pvd_object_id_2);

    mut_printf(MUT_LOG_TEST_STATUS, 
               "removing drive: %d_%d_%d. PVD ID:0x%x\n", 
               bus, encl, slot+1, pvd_object_id_2);

    status = fbe_test_pp_util_pull_drive(bus, encl, slot+1,
                                         &drive_handle_p_2);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);     
    status = fbe_api_provision_drive_get_obj_id_by_location(bus, encl, slot+2,
															&pvd_object_id_3);

    mut_printf(MUT_LOG_TEST_STATUS, 
               "removing drive: %d_%d_%d. PVD ID:0x%x\n", 
               bus, encl, slot+2, pvd_object_id_3);

    status = fbe_test_pp_util_pull_drive(bus, encl, slot+2,
                                         &drive_handle_p_3);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);     

    /* Verify that the PVD  are in the FAIL state */
    sep_rebuild_utils_verify_sep_object_state(pvd_object_id_1, FBE_LIFECYCLE_STATE_FAIL);


    /*reboot sep*/
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    
    /* We dont want verifies to kick off on Media Error since that will invalidate 64 blocks
     * which we dont want. So disable verifies
     */
    fbe_api_base_config_disable_background_operation(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                                     FBE_RAID_GROUP_BACKGROUND_OP_ALL);

    /* Inject Errors in the Non-paged Metadata region for the LBA(which is the object ID) */
    red_riding_hood_inject_read_errors_to_system_drive(0x7, pvd_object_id_1, 1,
                                                       FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_TRIPLE_MIRROR_RG,
                                                       FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_RAID_METADATA);
    fbe_api_metadata_nonpaged_load();

    mut_printf(MUT_LOG_TEST_STATUS, 
               "Inserting drive...: %d_%d_%d\n", 
               bus, encl, slot);
    status = fbe_test_pp_util_reinsert_drive( bus,
                                               encl,
                                               slot,
                                              drive_handle_p_1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 

    mut_printf(MUT_LOG_TEST_STATUS, 
               "Inserting drive...: %d_%d_%d\n", 
               bus, encl, slot+1);
    status = fbe_test_pp_util_reinsert_drive( bus,
                                               encl,
                                               slot+1,
                                              drive_handle_p_2);
    mut_printf(MUT_LOG_TEST_STATUS, 
               "Inserting drive...: %d_%d_%d\n", 
               bus, encl, slot+2);
    status = fbe_test_pp_util_reinsert_drive( bus,
                                               encl,
                                               slot+2,
                                              drive_handle_p_3);
    /* We might catch the object in a transitioning state and so wait for a while and then check the state
     * This will solve the case where the object incorrectly goes to ready but would pass the specialize 
     * state and will pass the check below. So first sleep to make sure things are settled
     */
    fbe_api_sleep(30000);
    sep_rebuild_utils_verify_sep_object_state(pvd_object_id_1, FBE_LIFECYCLE_STATE_SPECIALIZE);
    
    /* On Error, the NP recovery algorithm narrows down to the block that takes the error and keeps the object
     * in specialize state which we have validate above. But we need to make sure the Object ID just before and 
     * after this is good. So validate that we really did narrow down to single block by checking the 
     */
    sep_rebuild_utils_verify_sep_object_state(pvd_object_id_2, FBE_LIFECYCLE_STATE_READY);
    sep_rebuild_utils_verify_sep_object_state(pvd_object_id_3, FBE_LIFECYCLE_STATE_READY);

    /* Try to recover the PVD Object */
    /*set the dafault NP for pvd object*/
    status = fbe_api_base_config_metadata_set_nonpaged_metadata(pvd_object_id_1, nonpaged_data.data,sizeof(fbe_provision_drive_nonpaged_metadata_t));
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Validate that the object is back to normal */
    sep_rebuild_utils_verify_sep_object_state(pvd_object_id_1, FBE_LIFECYCLE_STATE_READY);

    /*stop error injection*/
    status = fbe_api_protocol_error_injection_stop();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
}

/*!****************************************************************************
 *  red_riding_hood_create_lun_without_notify
 ******************************************************************************
 *
 * @brief
 *    Send the create lun job out, do not wait for the job complete
 *
 * @param - None
 *
 * @return  FBE_STATUS_OK if no error
 * 
 * @author
 *    01/18/2013 - Created. Zhipeng Hu (huz3)
 *
 *****************************************************************************/
static fbe_status_t red_riding_hood_create_lun_without_notify(fbe_api_job_service_lun_create_t* lu_create_job)
{
    fbe_status_t                         status;
    fbe_block_transport_control_get_max_unused_extent_size_t get_extent;
    fbe_object_id_t 					 rg_object_id;

    /*fill in all the information we need*/
    lu_create_job->bind_time = fbe_get_time();

    /*now we need to get the RG object id from the database*/
    status = fbe_api_database_lookup_raid_group_by_number(lu_create_job->raid_group_id, &rg_object_id);
    if (status != FBE_STATUS_OK) {
        mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: could not find the specified RG.", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(lu_create_job->capacity == 0)
    {
        status = fbe_api_raid_group_get_max_unused_extent_size(rg_object_id, &get_extent);
        if (status != FBE_STATUS_OK) {
            mut_printf(MUT_LOG_TEST_STATUS, 
                "%s: failed to get unused extent size.", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (get_extent.extent_size != 0) {
            fbe_api_lun_calculate_max_lun_size(lu_create_job->raid_group_id, get_extent.extent_size, 1, &lu_create_job->capacity);
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, 
                "%s: no enough extent.", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;/*if the user already sent us 0 and we gave him everything we had and he keeps doing that, too bad*/
        }
    }

    /*start the job*/
    status = fbe_api_job_service_lun_create(lu_create_job);
    if (status != FBE_STATUS_OK) {
        mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: failed to send the job.", __FUNCTION__);
    }

    return status;
}

