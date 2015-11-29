/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file smelly_cat_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains write log remap test for degraded raid objects.
 *
 * @version
 *   11/5/2013 - Created. Wenxuan Yin
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "sep_rebuild_utils.h"
#include "sep_hook.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_system_bg_service_interface.h"
#include "fbe/fbe_api_panic_sp_interface.h"
#include "fbe/fbe_api_dest_injection_interface.h"
#include "fbe/fbe_dest_service.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe/fbe_scsi_interface.h"
#include "fbe_test.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
char * smelly_cat_short_desc = "Test write log remap in degraded raid group.";
char * smelly_cat_long_desc ="\
The smelly_cat senario tests write log remap in degraded raid group with media error on journal space..\n\
\n\
Dependencies:\n\
        - Dual SP \n\
        - Debug Hooks for Write Log Remap.\n\
\n\
After bring up the initial topology for parity Raid Groups SPs (SPA == Active)\n\
The test follows steps:\n\
Step 1: Remove some disk(s) to create degraded RG.\n\
        -R3 or R5: remove the 1st non-parity drive\n\
        -R6: remove 1st 2 non-parity drives\n\
Step 2: On SPB, setup DEST to inject HARD Media error on READ to the 1st PBA of the journal space.\n\
        -The 1st PBA is at the last position of associated drives\n\
Step 3: Insert write log remap debug hook into raid group condition handler.\n\
Step 4: On SPB, run IO to exercise write log.\n\
Step 5: Panic SPA.\n\
Step 6: Wait for write log remap debug hook triggered.\n\
Step 7: On SPB, run IO on the degraded RG and wait for a while.\n\
Step 8: Delete the write log remap debug hook and check whether the IO is complete.\n\
Step 9: Verify dest error and Stop error injection.\n\
Step 10: Verify journal remap.\n\
Step 11: Restart SPA.\n\
\n\
Description Last Updated: 11/14/2013\n\
\n";

/*!*******************************************************************
 * @def SMELLY_CAT_MS_THREAD_DELAY
 *********************************************************************
 * @brief Number of milliseconds we should delay the thread.
 *
 *********************************************************************/
#define SMELLY_CAT_MS_THREAD_DELAY      200

/*!*******************************************************************
 * @def SMELLY_CAT_MS_RDGEN_WAIT
 *********************************************************************
 * @brief Number of milliseconds we should wait for rdgen IO completion
 *
 *********************************************************************/
#define SMELLY_CAT_MS_RDGEN_WAIT        120000

/*!*******************************************************************
 * @def SMELLY_CAT_MIN_IO_BLOCK_COUNT
 *********************************************************************
 * @brief IO block count for only drive, or spilled over onto 2nd drive.
 *
 *********************************************************************/
#define SMELLY_CAT_MIN_IO_BLOCK_COUNT 0x1

/*!*******************************************************************
 * @def SMELLY_CAT_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate raid config type groups we will setup.
 *
 *********************************************************************/
#define SMELLY_CAT_RAID_TYPE_COUNT 3

/*!*******************************************************************
 * @def SMELLY_CAT_RAID_CONFIG_PER_TYPE_COUNT
 *********************************************************************
 * @brief Max number of separate configs per type we will setup.
 *
 *********************************************************************/
#define SMELLY_CAT_RAID_CONFIG_PER_TYPE_COUNT 1

/*!*******************************************************************
 * @def SMELLY_CAT_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define SMELLY_CAT_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def SMELLY_CAT_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define SMELLY_CAT_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def FBE_API_POLLING_INTERVAL
 *********************************************************************
 * @brief Number of milliseconds we should wait to poll the SP
 *
 *********************************************************************/
#define FBE_API_POLLING_INTERVAL 100 /*ms*/

/*!*******************************************************************
 * @def SMELLY_CAT_THREAD_COUNT
 *********************************************************************
 * @brief The number of threads to run io in during the test
 *
 *********************************************************************/
#define SMELLY_CAT_THREAD_COUNT 32

/*!*******************************************************************
 * @def SMELLY_CAT_ACTIVE_SP
 *********************************************************************
 * @brief The active SP 
 *
 *********************************************************************/
#define SMELLY_CAT_ACTIVE_SP FBE_SIM_SP_A

/*!*******************************************************************
 * @def SMELLY_CAT_PASSIVE_SP
 *********************************************************************
 * @brief The passive SP 
 *
 *********************************************************************/
#define SMELLY_CAT_PASSIVE_SP FBE_SIM_SP_B

/*!*******************************************************************
 * @var smelly_cat_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t smelly_cat_rdgen_contexts[2];

fbe_u32_t   smelly_cat_number_physical_objects = 0;


#ifndef __SAFE__
fbe_test_rg_configuration_array_t smelly_cat_raid_group_config_qual[SMELLY_CAT_RAID_TYPE_COUNT + 1] = 
#else
fbe_test_rg_configuration_array_t smelly_cat_raid_group_config_qual[] = 
#endif /* __SAFE__ SAFEMESS - shrink table/executable size */
{
    /* Raid 3 configurations */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.   bandwidth.*/
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            0,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    
    /* Raid 5 configurations */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    
    /* Raid 6 configurations */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.   bandwidth.*/
        {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            0,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    
    /* Terminator. */
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var smelly_cat_temp_array
 *********************************************************************
 * @brief This is a temp array of configurations we use to reboot peer
 *        without overwriting the original array.
 *
 *********************************************************************/

/* Had to allocate this here instead of on the restart_peer call stack (as other tests do) because
 *  this is an array of configs, and was too big for the stack.
 */
fbe_test_rg_configuration_array_t smelly_cat_temp_array[SMELLY_CAT_RAID_TYPE_COUNT + 1];

/*-----------------------------------------------------------------------------
 FORWARD DECLARATIONS:
 */
static void smelly_cat_start_io(   fbe_test_rg_configuration_t*    in_rg_config_p,
                                            fbe_u32_t                       in_rdgen_context_index,
                                            fbe_lba_t                       in_lba,
                                            fbe_block_count_t               in_num_blocks,
                                            fbe_rdgen_operation_t           in_rdgen_op); 
static void smelly_cat_verify_io_results(fbe_u32_t in_rdgen_context_index);
static void smelly_cat_cleanup_io(fbe_u32_t in_rdgen_context_index);
static void smelly_cat_drive_mgmt_dieh_clear_stats(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
static void smelly_cat_fill_dest_error_record(fbe_dest_error_record_t * record_p, 
                                                    fbe_object_id_t pdo_obj_id,
                                                    fbe_object_id_t rg_obj_id);
static void smelly_cat_inject_dest_error_on_journal_space(fbe_test_rg_configuration_t *rg_config_p,
                                                                  fbe_object_id_t rg_obj_id,
                                                                  fbe_dest_error_record_t  *dest_record_p,
                                                                  fbe_dest_record_handle_t *record_handle_p);
static void smelly_cat_inject_dest_error_cleanup(void);
static void smelly_cat_inject_dest_error_verify(fbe_dest_error_record_t *dest_record_p,
                                                   fbe_dest_record_handle_t record_handle);
static fbe_status_t smelly_cat_set_degraded_position(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_u32_t *num_to_remove);
static void smelly_cat_degrade_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                                    fbe_u32_t * num_to_remove);
static void smelly_cat_duplicate_config_array(fbe_test_rg_configuration_array_t *src_rg_config_array_p,
                                          fbe_test_rg_configuration_array_t *dest_rg_config_array_p);
static void smelly_cat_io_wait_for_slots_allocation(fbe_object_id_t rg_object_id);
static void smelly_cat_panic_sp(fbe_sim_transport_connection_target_t sp);
static void smelly_cat_restore_sp(fbe_sim_transport_connection_target_t sp,
                              fbe_test_rg_configuration_t *rg_config_p,
                              fbe_u32_t num_to_remove);
static void smelly_cat_write_log_remap_test(fbe_test_rg_configuration_t *rg_config_p, void *test_case_in);
static void smelly_cat_wait_rg_degraded(fbe_test_rg_configuration_t * rg_config_p, fbe_bool_t is_degraded);
static void smelly_cat_reinsert_drive(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_u32_t num_to_remove);

/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/

/*!**************************************************************************
 * smelly_cat_start_io()
 ***************************************************************************
 * @brief
 *  This function issues I/O to proper object ID in the RAID group using rdgen.
 * 
 * @param in_rg_config_p            - raid group config information 
 * @param in_rdgen_context_index    - index in context array to issue I/O
 * @param in_lba                    - LBA to start I/O from
 * @param in_num_blocks             - number of blocks to issue I/O
 * @param in_rdgen_op               - rdgen operation (read,write,etc.)
 *
 * @return void
 *
 * @author
 *  11/05/2013 - Created. Wenxuan Yin
 *
 ***************************************************************************/
void smelly_cat_start_io(   fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_rdgen_context_index,
                                    fbe_lba_t                       in_lba,
                                    fbe_block_count_t               in_num_blocks,
                                    fbe_rdgen_operation_t           in_rdgen_op)
{
    fbe_status_t                status;
    fbe_api_rdgen_context_t     *context_p = &smelly_cat_rdgen_contexts[in_rdgen_context_index];
    fbe_object_id_t             rdgen_object_id = FBE_OBJECT_ID_INVALID;
    fbe_class_id_t              rdgen_class_id = FBE_CLASS_ID_INVALID;

    switch(in_rdgen_context_index)
    {
        case 0: /* First IO */
        {   
            mut_printf(MUT_LOG_TEST_STATUS, "Starting rdgen IO with index %d..\n", in_rdgen_context_index);
            status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rdgen_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            rdgen_class_id = FBE_CLASS_ID_PARITY;
            break;
        }
        case 1: /* Second IO */
        {
            mut_printf(MUT_LOG_TEST_STATUS, "Starting rdgen IO with index %d\n", in_rdgen_context_index);
            status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rdgen_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            rdgen_class_id = FBE_CLASS_ID_PARITY;
            break;
        }
        default:
            MUT_FAIL_MSG("Illegal rdgen_context_index value!!\n");
    }

    /*  Send a single I/O to proper object provided as input */
    status = fbe_api_rdgen_send_one_io_asynch(context_p,
                                              rdgen_object_id,
                                              rdgen_class_id,
                                              FBE_PACKAGE_ID_SEP_0,
                                              in_rdgen_op,
                                              FBE_RDGEN_LBA_SPEC_FIXED,
                                              in_lba,
                                              in_num_blocks,
                                              FBE_RDGEN_OPTIONS_INVALID);
    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end smelly_cat_start_io()
 ******************************************/

/*!**************************************************************************
 * smelly_cat_verify_io_results
 ***************************************************************************
 * @brief
 *  This function verifies the results of issuing I/O to the pvd in a RG
 *  using rdgen.
 * 
 * @param in_rdgen_context_index - index in context array to issue I/O
 *
 * @return void
 *
 * @author
 *  11/05/2013 - Created. Wenxuan Yin
 *
 ***************************************************************************/
void smelly_cat_verify_io_results(fbe_u32_t in_rdgen_context_index)
{
    fbe_api_rdgen_context_t*                context_p = &smelly_cat_rdgen_contexts[in_rdgen_context_index];
    fbe_status_t                            status;

    mut_printf(MUT_LOG_TEST_STATUS, "Verifying rdgen IO with index %d..\n", in_rdgen_context_index);

    //  Wait for the IO's to complete before checking the status
    status = fbe_semaphore_wait_ms(&context_p->semaphore, SMELLY_CAT_MS_RDGEN_WAIT);
    if (status != FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                  "%s semaphore wait failed with status of 0x%x\n",
                  __FUNCTION__, status);
        
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed with IO timing out!!");
    }

    MUT_ASSERT_INT_EQUAL_MSG(context_p->start_io.statistics.error_count, 0,
                                "Failed with IO errors!!");
    MUT_ASSERT_INT_NOT_EQUAL_MSG(context_p->start_io.statistics.io_count, 0,
                                "Failed: IO count is 0!!");
    
    //  Succeed
    if (context_p->object_id != FBE_OBJECT_ID_INVALID)
    {
        mut_printf(MUT_LOG_TEST_STATUS,
                   "rdgen test for object id: 0x%x complete. passes: 0x%x io_count: 0x%x error_count: 0x%x \n",
                   context_p->object_id,
                   context_p->start_io.statistics.pass_count,
                   context_p->start_io.statistics.io_count,
                   context_p->start_io.statistics.error_count);
    }
    else
    {
        mut_printf(MUT_LOG_TEST_STATUS,
                   "rdgen test for class id: 0x%x complete. passes: 0x%x io_count: 0x%x error_count: 0x%x \n",
                   context_p->start_io.filter.class_id,
                   context_p->start_io.statistics.pass_count,
                   context_p->start_io.statistics.io_count,
                   context_p->start_io.statistics.error_count);
    }

    return;
}
/******************************************
 * end smelly_cat_verify_io_results()
 ******************************************/

/*!**************************************************************************
 * smelly_cat_cleanup_io()
 ***************************************************************************
 * @brief
 *  This function cleansup I/O to the LUNs in the RAID group using rdgen.
 * 
 * @param in_rdgen_context_index - index in context array to issue I/O
 *
 * @return void
 *
 ***************************************************************************/
void smelly_cat_cleanup_io(fbe_u32_t in_rdgen_context_index)
{
    fbe_status_t                            status;
    fbe_api_rdgen_context_t                *context_p = &smelly_cat_rdgen_contexts[in_rdgen_context_index];

    mut_printf(MUT_LOG_TEST_STATUS, "Cleaning up rdgen IO with index %d..\n", in_rdgen_context_index);

    // Destroy all the contexts (even if we got an error).
    status = fbe_api_rdgen_test_context_destroy(context_p);
    // Make sure the I/O was cleaned up successfully.
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed in cleaning up IO!");

    mut_printf(MUT_LOG_TEST_STATUS, "Succeed!\n");
    return;
}
/******************************************
 * end smelly_cat_cleanup_io()
 ******************************************/


/*!**************************************************************
 * smelly_cat_drive_mgmt_dieh_clear_stats()
 ****************************************************************
 * @brief
 *  This function clears the DIEH (Drive Improved Error Handling) 
 *  error category stats. 
 *
 * @param bus, enclosure, slot -  drive location
 *
 * @return none.
 *
 * @author
 *  11/05/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_drive_mgmt_dieh_clear_stats(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)  
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     object_id = FBE_OBJECT_ID_INVALID;

    mut_printf(MUT_LOG_TEST_STATUS, "Clearing drive %d_%d_%d DIEH stats...", bus, encl, slot);

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &object_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Error getting pdo object ID");

    status = fbe_api_physical_drive_clear_error_counts(object_id, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Error clearing error tags");

    mut_printf(MUT_LOG_TEST_STATUS, "Succeed!\n");

    return;
}
/******************************************
 * end smelly_cat_drive_mgmt_dieh_clear_stats()
 ******************************************/

/*!**************************************************************
 * smelly_cat_fill_dest_error_record()
 ****************************************************************
 * @brief
 *  Fill up the dest error record. 
 *
 * @param fbe_dest_error_record_t * -  dest error record
 *        fbe_object_id_t - PDO object ID
 *        fbe_object_id_t - Raid Group object ID
 *
 * @return none.
 *
 * @author
 *  11/05/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_fill_dest_error_record(fbe_dest_error_record_t * record_p, 
                                                    fbe_object_id_t pdo_obj_id,
                                                    fbe_object_id_t rg_obj_id)
{    
    fbe_api_raid_group_get_info_t   raid_group_info;
    fbe_lba_t                       error_start_lba;
    fbe_status_t                    status;

	mut_printf(MUT_LOG_TEST_STATUS, "%s: Entry ...\n", __FUNCTION__);
    
    
    fbe_zero_memory(record_p, sizeof(fbe_dest_error_record_t));
    status = fbe_api_raid_group_get_info(rg_obj_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Error getting rg info");

    /* Calculate the journal space start point */
    error_start_lba = raid_group_info.write_log_start_pba + raid_group_info.physical_offset;
    
    /* Put PDO object ID into the error record
    */
    record_p->object_id = pdo_obj_id;
    
    /* Fill in the fields in the error record that are common to all
     * the error cases.
     */
    record_p->dest_error_type = FBE_DEST_ERROR_TYPE_SCSI;
    record_p->dest_error.dest_scsi_error.scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION;
    record_p->dest_error.dest_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    record_p->lba_start = error_start_lba;
    record_p->lba_end = error_start_lba + 1; 
    
    /* Inject the READ error with sense key 03/11/00 (hard media error)
    FBE_SCSI_READ               = 0x08,
    FBE_SCSI_READ_6             = 0x08,
    FBE_SCSI_READ_10            = 0x28,
    FBE_SCSI_READ_12            = 0xA8,
    FBE_SCSI_READ_16            = 0x88,
    */
    record_p->dest_error.dest_scsi_error.scsi_command[0] = FBE_SCSI_READ;
    record_p->dest_error.dest_scsi_error.scsi_command[1] = FBE_SCSI_READ_6;
    record_p->dest_error.dest_scsi_error.scsi_command[2] = FBE_SCSI_READ_10;
    record_p->dest_error.dest_scsi_error.scsi_command[3] = FBE_SCSI_READ_12;
    record_p->dest_error.dest_scsi_error.scsi_command[4] = FBE_SCSI_READ_16;
    record_p->dest_error.dest_scsi_error.scsi_command_count = 5;
    record_p->dest_error.dest_scsi_error.scsi_sense_key = 0x03 /* FBE_SCSI_SENSE_KEY_MEDIUM_ERROR */;
    record_p->dest_error.dest_scsi_error.scsi_additional_sense_code = 0x11/* (FBE_SCSI_ASC_READ_DATA_ERROR )*/;
    record_p->dest_error.dest_scsi_error.scsi_additional_sense_code_qualifier = 0x00; 
    record_p->num_of_times_to_insert = 2;
    record_p->frequency = 1;
    record_p->times_inserting = 0;
    record_p->times_inserted = 0;
    record_p->valid_lba = 1;
    record_p->is_active_record = FALSE;


	mut_printf(MUT_LOG_TEST_STATUS, "%s: LBA 0x%llx PDO : 0x%X\n", __FUNCTION__, error_start_lba, pdo_obj_id);


    mut_printf(MUT_LOG_TEST_STATUS, "Succeed in filling dest error record!\n");
}
/******************************************
 * end smelly_cat_fill_dest_error_record()
 ******************************************/

/*!**************************************************************
 * smelly_cat_inject_dest_error_on_journal_space()
 ****************************************************************
 * @brief
 *  Inject hard media error on the 1st PBA of drive journal space
 *
 * @param rg_config_p - the raid group configuration to test
 *        fbe_object_id_t rg_obj_id,
 *        fbe_dest_error_record_t  *dest_record_p,
 *        fbe_dest_record_handle_t *record_handle_p
 *
 * @return None.
 *
 * @author
 *  11/06/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_inject_dest_error_on_journal_space(fbe_test_rg_configuration_t *rg_config_p,
                                                                  fbe_object_id_t rg_obj_id,
                                                                  fbe_dest_error_record_t  *dest_record_p,
                                                                  fbe_dest_record_handle_t *record_handle_p)
{
    fbe_u32_t                   bus;
    fbe_u32_t                   enclosure;
    fbe_u32_t                   slot;
    fbe_u32_t                   object_handle;
    fbe_object_id_t             pdo_obj_id;
    fbe_status_t                status;

    mut_printf(MUT_LOG_TEST_STATUS, "%s Entry.\n", __FUNCTION__);
    
    /* LBA 0 starts at the last drive (width - 1) */
    bus = rg_config_p->rg_disk_set[rg_config_p->width - 1].bus;
    enclosure = rg_config_p->rg_disk_set[rg_config_p->width - 1].enclosure;
    slot = rg_config_p->rg_disk_set[rg_config_p->width - 1].slot;

    /* 1. Clear DIEH statistics first */
    smelly_cat_drive_mgmt_dieh_clear_stats(bus, enclosure, slot);

    /* 2. Get the physical drive object id.
    */
    status = fbe_api_get_physical_drive_object_id_by_location(bus, enclosure, slot, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);
    pdo_obj_id = (fbe_object_id_t)object_handle; 
    //mut_printf(MUT_LOG_TEST_STATUS, "PDO object ID is 0x%x...\n", pdo_obj_id);
        
    /* 3. Fill the error record
    */
    smelly_cat_fill_dest_error_record(dest_record_p, pdo_obj_id, rg_obj_id);    
    
    /* 4. Add the error record
    */
    mut_printf(MUT_LOG_TEST_STATUS, "Adding dest error record...\n");
    status = fbe_api_dest_injection_add_record(dest_record_p, record_handle_p);  
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
    
    /* 5. Start the error injection 
    */
    mut_printf(MUT_LOG_TEST_STATUS, "Starting error injection...\n");
    status = fbe_api_dest_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Succeed!!\n", __FUNCTION__);  
}
/******************************************
 * end smelly_cat_inject_dest_error_on_journal_space()
 ******************************************/

/*!**************************************************************
 * smelly_cat_inject_dest_error_cleanup()
 ****************************************************************
 * @brief
 *  This function stops the protocol error injection.
 *
 * @param None.
 *
 * @return FBE Status.
 *
 * @author
 *  11/06/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_inject_dest_error_cleanup()
{
    fbe_status_t   status = FBE_STATUS_OK;

    /*set the error using NEIT package. */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Stop error injection..", __FUNCTION__);

    status = fbe_api_dest_injection_stop(); 
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Error stopping dest injection");

    status = fbe_api_dest_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Error removing all dest records");
    return;
}
/******************************************
 * end smelly_cat_inject_dest_error_cleanup()
 ******************************************/

/*!**************************************************************
 * smelly_cat_inject_dest_error_verify()
 ****************************************************************
 * @brief
 *  This function stops the protocol error injection.
 *
 * @param None.
 *
 * @return FBE Status.
 *
 * @author
 *  11/13/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_inject_dest_error_verify(fbe_dest_error_record_t *dest_record_p,
                                                   fbe_dest_record_handle_t record_handle)
{
    fbe_status_t   status = FBE_STATUS_OK;

    /* 6. Verify the error injection is OK
    */
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying error injection...\n");
    status = fbe_api_dest_injection_get_record(dest_record_p, record_handle);  
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(0, dest_record_p->times_inserted);
}
/******************************************
 * end smelly_cat_inject_dest_error_verify()
 ******************************************/


/*!**************************************************************
 * smelly_cat_set_degraded_position()
 ****************************************************************
 * @brief
 *  Degrade raid groups:
 *  For R3 or R5, remove 1st non-parity drive;
 *  For R6, remove row parity and 1st non-parity drives
 *
 * @param rg_config_p - the raid group configuration to test
 *
 * @return None.   
 *
 * @author
 *  11/05/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
fbe_status_t smelly_cat_set_degraded_position(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_u32_t *num_to_remove)
{
    /* Note: parity is drive 0 or 0 and 1, and lba 0 starts at the last drive (width - 1)
     *  and that the delay drive is always drive 2 for all raid groups
     */
    switch (rg_config_p->raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID3:
        case FBE_RAID_GROUP_TYPE_RAID5:
        {
            /* kill drive 1 and do a small io on the last drive (lba 0)
            */
            *num_to_remove = 1;
            rg_config_p->specific_drives_to_remove[0] = 1;
            break;
        }
        case FBE_RAID_GROUP_TYPE_RAID6:
        {
            /* kill the 1st 2 non-parity drives
            * do a small io on the last drive (lba 0)
            */
            *num_to_remove = 2;
            rg_config_p->specific_drives_to_remove[0] = 2;
            rg_config_p->specific_drives_to_remove[1] = 3;
            break;
        }
        
        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * End smelly_cat_set_test_params()
 ******************************************/

/*!**************************************************************
 * smelly_cat_degrade_raid_groups()
 ****************************************************************
 * @brief
 *  Degrade raid groups:
 *  For R3 or R5, remove first non-parity drive;
 *  For R6, remove row parity and first non-parity drives
 *
 * @param rg_config_p - the raid group configuration to test
 *
 * @return None.   
 *
 * @author
 *  11/05/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_degrade_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                                    fbe_u32_t * num_to_remove)
{
    fbe_u32_t                           degraded_position;
    fbe_u32_t                           max_remove_nums;
    fbe_status_t                        status;
    fbe_u32_t                           i;

    mut_printf(MUT_LOG_TEST_STATUS, "%s Entry.\n", __FUNCTION__);
    
    status = fbe_api_get_total_objects(&smelly_cat_number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Error getting object number");
    
    status = smelly_cat_set_degraded_position(rg_config_p, num_to_remove);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Error setting degraded position");

    max_remove_nums = *num_to_remove;
    for (i = 0; i < max_remove_nums; i++)
    {
        smelly_cat_number_physical_objects -= 1; /*Pdo*/
        degraded_position = rg_config_p->specific_drives_to_remove[i];
        /*  Remove drive(s) in the RG. Check the rb logging is marked. */
        sep_rebuild_utils_remove_drive_and_verify(rg_config_p, 
                                                  degraded_position, 
                                                  smelly_cat_number_physical_objects,
                                                  &rg_config_p->drive_handle[i]);
        sep_rebuild_utils_verify_rb_logging(rg_config_p, rg_config_p->specific_drives_to_remove[i]);
        sep_rebuild_utils_check_for_reb_restart(rg_config_p, rg_config_p->specific_drives_to_remove[i]);
        mut_printf(MUT_LOG_TEST_STATUS, "Drive removed: %d_%d_%d\n", 
                   rg_config_p->rg_disk_set[degraded_position].bus,
                   rg_config_p->rg_disk_set[degraded_position].enclosure,
                   rg_config_p->rg_disk_set[degraded_position].slot);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "%s Succeed!!\n", __FUNCTION__);
    return;
}
/******************************************
 * End smelly_cat_degrade_raid_groups()
 ******************************************/

/*!**************************************************************
 * smelly_cat_reinsert_drive()
 ****************************************************************
 * @brief
 *  Degrade raid groups:
 *  For R3 or R5, remove first non-parity drive;
 *  For R6, remove row parity and first non-parity drives
 *
 * @param rg_config_p - the raid group configuration to test
 *
 * @return None.   
 *
 * @author
 *  11/05/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_reinsert_drive(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_u32_t num_to_remove)
{
    fbe_u32_t i;
    
    for (i = 0; i < num_to_remove; i++)
    {
        smelly_cat_number_physical_objects += 1; /*  PDO */
        sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, 
                                                    rg_config_p->specific_drives_to_remove[i], 
                                                    smelly_cat_number_physical_objects, 
                                                    &rg_config_p->drive_handle[i]);
    }
    smelly_cat_wait_rg_degraded(rg_config_p, FBE_FALSE);
}
/******************************************
 * End smelly_cat_reinsert_drive()
 ******************************************/

/*!**************************************************************
 * smelly_cat_wait_rg_degraded()
 ****************************************************************
 * @brief
 *  This function checks pvd slf state from one SP.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * @param is_degraded - whether the raid is degraded.
 * 
 * @return None.   
 *
 * @author
 *  11/05/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_wait_rg_degraded(fbe_test_rg_configuration_t * rg_config_p, fbe_bool_t is_degraded)
{
    fbe_u32_t                           current_time = 0;
    fbe_bool_t                          b_degraded;

    while (current_time < 60000)
    {
        /* Get raid degrade info */
        b_degraded = fbe_test_rg_is_degraded(rg_config_p);
        if (b_degraded == is_degraded)
        {
            break;
        }
        current_time += 500;
        fbe_api_sleep(500);
    }

    MUT_ASSERT_INT_NOT_EQUAL(current_time, 60000);

}
/******************************************
 * end smelly_cat_wait_rg_degraded()
 ******************************************/


/*!**************************************************************
 * smelly_cat_duplicate_config_array()
 ****************************************************************
 * @brief
 *  Make a copy of a config array to recreate 
 *
 * @param src_rg_config_array_p - raid group config array to recreate    
 *        dest_rg_config_array_p - duplicate raid group config array 
 *
 * @return
 *
 * @author
 *  11/08/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_duplicate_config_array(fbe_test_rg_configuration_array_t *src_rg_config_array_p,
                                          fbe_test_rg_configuration_array_t *dest_rg_config_array_p)
{
    fbe_u32_t type_index = 0;
    fbe_u32_t group_index = 0;
    fbe_u32_t group_count = 0;
    fbe_test_rg_configuration_t * current_group_p = NULL;

    /* Types to copy must add one to copy the terminator */
    for (type_index = 0; type_index < SMELLY_CAT_RAID_TYPE_COUNT + 1; type_index++)
    {
        current_group_p = &src_rg_config_array_p[type_index][0];

        /* Count groups to copy and add one to copy the terminator */
        group_count = fbe_test_get_rg_array_length(current_group_p) + 1;

        for (group_index = 0; group_index < group_count; group_index++)
        {
            dest_rg_config_array_p[type_index][group_index] = src_rg_config_array_p[type_index][group_index];
        }  /* for all groups */
    } /* for all types */
}
/******************************************
 * end smelly_cat_duplicate_config_array()
 ******************************************/

/*!**************************************************************
 * smelly_cat_io_wait_for_slots_allocation()
 ****************************************************************
 * @brief
 *  This function is used after the io is sent. It can ensure that
 *  The RG io path has walked through the func 
 *  fbe_parity_write_log_allocate_slot(). That means make sure IO find
 *  no available slots and the siots is put onto the write log waiting queue
 *
 * @param fbe_object_id_t - rg_object_id    
 *
 * @return None.
 *
 * @author
 *  11/13/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_io_wait_for_slots_allocation(fbe_object_id_t rg_object_id)
{
    fbe_status_t                        status;
    fbe_bool_t                          is_queue_empty;
    fbe_parity_get_write_log_info_t     write_log_info;
    fbe_u32_t                           max_delay_ms = 0;

    status = fbe_api_raid_group_get_write_log_info(rg_object_id,
                                                   &write_log_info, 
                                                   FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Failed to get write log info!");
    
    /* Check for any pending siots on waiting queue */
    is_queue_empty = write_log_info.is_request_queue_empty;
    
    while(is_queue_empty)
    {
        max_delay_ms += SMELLY_CAT_MS_THREAD_DELAY;
        if(max_delay_ms <= 10000)
        {
            fbe_api_sleep(SMELLY_CAT_MS_THREAD_DELAY);
            status = fbe_api_raid_group_get_write_log_info(rg_object_id,
                                                   &write_log_info, 
                                                   FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Failed to get write log info!");
            is_queue_empty = write_log_info.is_request_queue_empty;
        }
        else
            MUT_FAIL_MSG("IO wait for slots allocation timed out!");
    }

    return;
}
/******************************************
 * end smelly_cat_io_wait_for_slots_allocation()
 ******************************************/


/*!**************************************************************
 * smelly_cat_panic_sp()
 ****************************************************************
 * @brief
 *  Panic SP by killing the simulation client and process.
 *
 * @param fbe_sim_transport_connection_target_t              
 *
 * @return None.
 *
 * @author
 *  11/05/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_panic_sp(fbe_sim_transport_connection_target_t sp)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* go to sp which need panic */
    status = fbe_api_sim_transport_set_target_server(sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, " <<< PANIC! %s >>>\n", sp == FBE_SIM_SP_A ? 
                                                            "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(sp);
    fbe_test_sp_sim_stop_single_sp(sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* we can't destroy fbe_api, because it's shared between two SPs */
    fbe_test_base_suite_startup_single_sp(sp);

    return;
}
/******************************************
 * End smelly_cat_panic_sp()
 ******************************************/

/*!**************************************************************
 * smelly_cat_restore_sp()
 ****************************************************************
 * @brief
 *  Startup the sp after a hard reboot and recreate the config
 *
 * @param none      
 *
 * @return 
 *
 * @author
 *  11/05/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_restore_sp(fbe_sim_transport_connection_target_t sp,
                              fbe_test_rg_configuration_t *rg_config_p,
                              fbe_u32_t num_to_remove)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p;
    fbe_u32_t i = 0;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t index = 0;

    mut_printf(MUT_LOG_TEST_STATUS, " %s entry\n", __FUNCTION__);

    /* go to sp which need restore */
    status = fbe_api_sim_transport_set_target_server(sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, " <<< Restore %s from panic >>>\n", 
                            sp == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* Start the SP with a new session
     * NOTE: Must copy the rg_config array, because sep_config_load_sep_and_neit below will scramble the one used
     *        to load the physical config, and we need the rg_config array for the other SP and next run
     */
    smelly_cat_duplicate_config_array(&smelly_cat_raid_group_config_qual[0], &smelly_cat_temp_array[0]);

    fbe_test_sep_util_rg_config_array_load_physical_config(&smelly_cat_temp_array[0]);

    /* Re-remove the failed drives
     */
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            for (i = 0; i < num_to_remove; i++)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "Re-remove a drive for rgid: %d\n", current_rg_config_p->raid_group_id);

                status = fbe_test_pp_util_pull_drive(rg_config_p[index].rg_disk_set[current_rg_config_p->specific_drives_to_remove[i]].bus,
                                                     rg_config_p[index].rg_disk_set[current_rg_config_p->specific_drives_to_remove[i]].enclosure,
                                                     rg_config_p[index].rg_disk_set[current_rg_config_p->specific_drives_to_remove[i]].slot, 
                                                     &current_rg_config_p->drive_handle[i]);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                mut_printf(MUT_LOG_TEST_STATUS, "Drive removed: %d_%d_%d\n", 
                           rg_config_p[index].rg_disk_set[current_rg_config_p->specific_drives_to_remove[i]].bus,
                           rg_config_p[index].rg_disk_set[current_rg_config_p->specific_drives_to_remove[i]].enclosure,
                           rg_config_p[index].rg_disk_set[current_rg_config_p->specific_drives_to_remove[i]].slot);
            }
        }
        current_rg_config_p++;
    }

    sep_config_load_sep_and_neit();
}
/******************************************
 * end smelly_cat_restore_sp()
 ******************************************/

/*!**************************************************************
 * smelly_cat_write_log_remap_test()
 ****************************************************************
 * @brief
 *  Entry of the smelly cat dual SP test
 *
 * @param rg_config_p - the raid group configuration to test
 *
 * @return None.   
 *
 * @author
 *  11/05/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_write_log_remap_test(fbe_test_rg_configuration_t *rg_config_p, void *test_case_in)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id;
    fbe_u32_t                           num_to_remove;
    fbe_dest_error_record_t             dest_record;
    fbe_dest_record_handle_t            record_handle = NULL;
    
    mut_printf(MUT_LOG_TEST_STATUS, "===== %s Entry =====\n", __FUNCTION__);

    /* Lookup RG object ID and save them locally */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Start from the active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1: Remove some disk(s) to create degraded RG.
    */
    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 1: Degrade Raid Group. ~~~~~\n");
    smelly_cat_degrade_raid_groups(rg_config_p, &num_to_remove);

    /* Step 2: On SPB, setup DEST to inject HARD Media error on READ to the 1st PBA of the 
    *          journal space, this is on the last drive.
    */
    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 2: Inject media error on journal space. ~~~~~\n");
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    smelly_cat_inject_dest_error_on_journal_space(rg_config_p, rg_object_id, &dest_record, &record_handle);

    /* Step 3: Insert write log remap debug hook into raid group condition handler.
    */
    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 3: Add write log remap debug hook. ~~~~~\n");
    status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REMAP,
                                              FBE_RAID_GROUP_SUBSTATE_JOURNAL_REMAP_SLOTS_ALLOCATED,
                                              0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAILED adding write log remap debug hook!");

    /* Step 4: Run single IO on RG to trigger the write log.
    */
    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 4: Run IO to trigger write log. ~~~~~\n");
    /* IO Start, Verify and Clean */
    smelly_cat_start_io(rg_config_p, 0/*index*/, 0/*lba*/, 1/*blocks*/, FBE_RDGEN_OPERATION_WRITE_ONLY);
    smelly_cat_verify_io_results(0/*index*/);
    smelly_cat_cleanup_io(0/*index*/);

    /* Step 5: Panic SPA.
    */
    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 5: Panic SPA. ~~~~~\n");
    smelly_cat_panic_sp(FBE_SIM_SP_A);
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Set single SP mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    /* Step 6: Wait for write log remap debug hook triggered.
    */
    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 6: Wait for write log remap debug hook triggered. ~~~~~\n");
    status = fbe_test_wait_for_debug_hook(rg_object_id, 
                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REMAP,
                                         FBE_RAID_GROUP_SUBSTATE_JOURNAL_REMAP_SLOTS_ALLOCATED,
                                         SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 7: If write log remap debug hook is triggered, we can send a single IO
               on the degraded RG to recreate the issue of AR 598706
    */
    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 7: Run IO to recreate issue. ~~~~~\n");
    smelly_cat_start_io(rg_config_p, 1/*index*/, 0/*lba*/, 1/*blocks*/, FBE_RDGEN_OPERATION_WRITE_ONLY);

    /* Use the wait function to make sure that the IO has walked into
      the fbe_parity_write_log_allocate_slot function and some siots is 
      pending on the write log waiting queue */
    smelly_cat_io_wait_for_slots_allocation(rg_object_id);
    
    /* fbe_api_sleep(SMELLY_CAT_MS_THREAD_DELAY); */

    /* Step 8: Delete write log remap debug hook and check whether the IO can be 
              completed Successfully.
    */
    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 8: Delete debug hook and verify IO completion. ~~~~~\n");
    /* Add the remap done hook first */
    status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REMAP,
                                              FBE_RAID_GROUP_SUBSTATE_JOURNAL_REMAP_DONE,
                                              0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAILED adding write log remap debug hook!");
    
    status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REMAP,
                                              FBE_RAID_GROUP_SUBSTATE_JOURNAL_REMAP_SLOTS_ALLOCATED,
                                              0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAILED deleting write log remap debug hook!");

    /* Verify and Cleanup IO */
    smelly_cat_verify_io_results(1/*index*/);
    smelly_cat_cleanup_io(1/*index*/);

    /* Step 9: Verify dest error and Stop error injection
    */
    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 9: Verify dest error and Stop error injection. ~~~~~\n");
    smelly_cat_inject_dest_error_verify(&dest_record, record_handle);
    smelly_cat_inject_dest_error_cleanup();

    /* Step 10: Verify journal remap.
    */
    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 10: Verify remap is done. ~~~~~\n");
    /* Wait for the remap condition is done first */
    status = fbe_test_wait_for_debug_hook(rg_object_id, 
                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REMAP,
                                         FBE_RAID_GROUP_SUBSTATE_JOURNAL_REMAP_DONE,
                                         SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    
    /* Delete hook */
    status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REMAP,
                                              FBE_RAID_GROUP_SUBSTATE_JOURNAL_REMAP_DONE,
                                              0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAILED deleting write log remap debug hook!");
    
    /* Step 11: Restart SPA.
    */
    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 11: Restart SPA.. ~~~~~\n");
    
    smelly_cat_reinsert_drive(rg_config_p, num_to_remove);
    
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    smelly_cat_restore_sp(FBE_SIM_SP_A, rg_config_p, 0);
    
    return;
}
/******************************************
 * End smelly_cat_write_log_remap_test()
 ******************************************/

/*!**************************************************************
 * smelly_cat_dualsp_test()
 ****************************************************************
 * @brief
 *  Run dual SP write log remap test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  11/05/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_dualsp_test(void)
{
    fbe_u32_t                   raid_type_index;
    fbe_u32_t                   raid_group_count = 0;
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Disable the recovery queue so that a spare cannot swap-in */
    fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);

    /* Run the test cases for all raid types supported
     */
    for (raid_type_index = 0; raid_type_index < SMELLY_CAT_RAID_TYPE_COUNT; raid_type_index++)
    {
        rg_config_p = &smelly_cat_raid_group_config_qual[raid_type_index][0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }
        raid_group_count = fbe_test_get_rg_count(rg_config_p);

        if (raid_group_count == 0)
        {
            continue;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "===== Start to run test on raid type %s (%d)\n =====\n",
                   raid_type_string_p, rg_config_p->raid_type);

        if (raid_type_index + 1 >= SMELLY_CAT_RAID_TYPE_COUNT) {
               /* Now create the raid groups and run the test 
                */
               fbe_test_run_test_on_rg_config(rg_config_p, 
                                              NULL,
                                              smelly_cat_write_log_remap_test,
                                              SMELLY_CAT_LUNS_PER_RAID_GROUP,
                                              SMELLY_CAT_CHUNKS_PER_LUN);
        }
        else {
            fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, 
                                                          NULL,
                                                          smelly_cat_write_log_remap_test,
                                                          SMELLY_CAT_LUNS_PER_RAID_GROUP,
                                                          SMELLY_CAT_CHUNKS_PER_LUN,
                                                          FBE_FALSE);
        }
        
    }    /* for all raid types. */

    /* Enable the recovery queue */
    fbe_test_sep_util_enable_recovery_queue(FBE_OBJECT_ID_INVALID);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end smelly_cat_dualsp_test()
 ******************************************/

/*!**************************************************************
 * smelly_cat_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a dual SP write log remap test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  11/05/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_dualsp_setup(void)
{
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_array_t *array_p = NULL;
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        const fbe_char_t            *raid_type_string_p = NULL;
        fbe_u32_t raid_group_count = 0;
        fbe_u32_t raid_type_index;

        mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for smelly cat test ===\n");

        array_p = &smelly_cat_raid_group_config_qual[0];

        for (raid_type_index = 0; raid_type_index < SMELLY_CAT_RAID_TYPE_COUNT; raid_type_index++)
        {
            rg_config_p = &array_p[raid_type_index][0];

            fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
            if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled",
                           raid_type_string_p,
                           rg_config_p->raid_type);
                continue;
            }
            raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
            if (raid_group_count == 0)
            {
                continue;
            }

            /* Initialize the raid group configuration 
             */
            fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        } /* for all raid types. */

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);

        sep_config_load_sep_and_neit_load_balance_both_sps();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/******************************************
 * End smelly_cat_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * smelly_cat_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the smelly_cat dual SP test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/05/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void smelly_cat_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for smelly_cat_dualsp_test ===\n");
    if (fbe_test_util_is_simulation())
    {
        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        /* clear the error counter */
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
        fbe_test_sep_util_destroy_neit_sep_physical();

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        /* clear the error counter */
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * End smelly_cat_dualsp_cleanup()
 ******************************************/

/*******************************
 * End file smelly_cat_test.c
 *******************************/

