/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * robi_test.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains tests for proactive copy error handling test cases.
 *
 * HISTORY
 *   03/12/2012:  Created. Lili Chen
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
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
#include "sep_utils.h"
#include "sep_hook.h"
#include "fbe_database.h"
#include "fbe_spare.h"
#include "fbe_test_configurations.h"
#include "fbe_job_service.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_test_common_utils.h"
#include "pp_utils.h"
#include "sep_rebuild_utils.h"                      //  SEP rebuild utils
#include "sep_test_background_ops.h"


/*************************
 *   DEFINITIONS
 *************************/
char * robi_short_desc = "This scenario will verify proactive copy error handling when media error is inserted.";
char * robi_long_desc = 
    "\n"
    "\n"
    "This test verifies error handling when a copy read gets media error. \n"  
    "\n"
    "Dependencies:\n\n"
    "\t - Drive Sparing service must provide the policy to select the Proactive Spare.\n"
    "\t - Proactive Copy operation must be serialized across the SPs\n."
    "\t - VD object gets Private Data Library interface to update the metadata (checkpoint) information.\n"
    "\t - Drive Spare service must provide the handling to update the required configuration database based on requested operation.\n"
    "\n"
    "Starting Config:\n"
    "\n"
    "\t [PP] armada board\n"
    "\t [PP] SAS PMC port\n"
    "\t [PP] viper enclosure\n"
    "\t [PP] 5 SAS drives\n"
    "\t [PP] 5 logical drives\n"
    "\t [SEP] 5 provisioned drives\n"
    "\t [SEP] 5 virtual drives\n"
    "\t [SEP] raid group (raid 0), (raid 1), (raid 5), (raid 6), (raid 1/0) (Note: Different set of drives will be used for different RAID group)\n"
    "\t [SEP] 3 luns\n"
    "\n"
    "Test Sequence:\n\n"
    "\tSTEP 1: Bring up the initial topology.\n"
    "\t\t - Create and verify the initial physical package config.\n"
    "\t\t - RAID group attributes:  width: 5 drives  capacity: 0x6000 blocks.\n"
    "\t\t - Create and attach 3 LUNs to the raid 5 object.\n"
    "\t\t - Each LUN will have capacity of 0x2000 blocks and element size of 128.\n"
    "\n"
    "\tSTEP 2: Start the Proactive Copy operation using fbe api.\n"
    "\t\t - Issue the command through fbe api/fbe_cli to start the Proactive Copy operation.\n"
    "\t\t - Verify that Drive Spare Service has received request for the Proactive Spare swap-in.\n"
    "\n"
    "\tSTEP 3: Verify the progress of the actual Proactive Copy operation.\n"
    "\t\t - Verify that VD object starts actual Proactive Copy based on LBA order.\n"
    "\t\t - Verify that VD object updates checkpoint metadata using PDL (Private Data Library) regularly on Proactive Spare drive.\n"
    "\t\t - Verify that VD object sends notification to Drive Swap Service when Proactive Copy operation completes.\n"
    "\n"
    "\tSTEP 4: Create a error record and start error injection to the proactive candidate.\n"
    "\t\t - Verify that the error record is set in PVD.\n"
    "\n"
    "\tSTEP 5: Verify error verify starts in the parent raid group.\n"
    "\t\t - Verify that error verify starts in raid group.\n"
    "\t\t - Verigy that error verify completes successfully.\n"
    "\n"
    "\tSTEP 6: Verify Proactive Copy operation completes.\n"
    "\t\t - Verify that VD object completes proactive copy operation.\n"
    "\t\t - Verify that VD object proactive candidate swaps out.\n"
    "\t\t - Verify that all the data is written to spare correctly.\n"
    "\n"
    "\n";



/*********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define ROBI_TEST_CHUNKS_PER_LUN                      3

/*********************************************************************
 * @brief group.
 *
 *********************************************************************/
#define ROBI_TEST_LUNS_PER_RAID_GROUP                 3 

/*********************************************************************
 * @brief Element size of our LUNs.
 *
 *********************************************************************/
#define ROBI_TEST_ELEMENT_SIZE                       128

/*!*******************************************************************
 * @var robi_user_space_lba_to_inject_to
 *********************************************************************
 * @brief This is the default user space lba to start the error injection
 *        to.
 *
 *********************************************************************/
static fbe_lba_t robi_user_space_lba_to_inject_to          =   0x0ULL; /* First block of LUN. Must be aligned to 4K block size.*/

/*!*******************************************************************
 * @var robi_user_space_lba_to_inject_to
 *********************************************************************
 * @brief This is the default user space lba to start the error injection
 *        to.
 *
 *********************************************************************/
static fbe_block_count_t robi_user_space_blocks_to_inject  = 0x20ULL; /* Must be aligned to 4K block size.*/

/*!*******************************************************************
 * @var robi_rdgen_context
 *********************************************************************
 * @brief rdgen context.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t robi_rdgen_context;

/*!*******************************************************************
 * @var robi_copy_pause_params
 *********************************************************************
 * @brief This variable contains any values that need to be passed to
 *        the reboot methods.
 *
 *********************************************************************/
static fbe_sep_package_load_params_t robi_copy_pause_params = { 0 };

/*!*******************************************************************
 * @var robi_pause_params
 *********************************************************************
 * @brief This variable contains any values that need to be passed to
 *        the reboot methods.
 *
 *********************************************************************/
static fbe_sep_package_load_params_t robi_pause_params = { 0 };

/*!*******************************************************************
 * @def ROBI_TEST_CONFIGURATION_TYPES
 *********************************************************************
 * @brief this is the number of test configuration types
 *
 *********************************************************************/
#define ROBI_TEST_CONFIGURATION_TYPES     5

/*!*******************************************************************
 * @var robi_raid_group_config_extended
 *********************************************************************
 * @brief Test config for raid test level 1 and greater.
 *
 * @note  Currently we limit the number of different raid group
 *        configurations of each type due to memory constraints.
 *********************************************************************/
#ifndef __SAFE__
fbe_test_rg_configuration_t robi_raid_group_config_extended[FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT] = 
#else
fbe_test_rg_configuration_t robi_raid_group_config_extended[] = 
#endif /* __SAFE__ SAFEMESS - shrink table/executable size */
{
    /* width,                       capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {3,                             0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            1,          0},
    {4,                             0xE000,     FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,   520,            2,          1},
    {5,                             0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            3,          1},
    {6,                             0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            4,          0},
    {5,                             0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            5,          1},
    /* RAID-0 configurations (not supported !).   */
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var robi_raid_group_config_qual
 *********************************************************************
 * @brief Test config for raid test level 0 (default test level).
 *
 * @note  Currently we limit the number of different raid group
 *        configurations of each type due to memory constraints.
 *********************************************************************/
#ifndef __SAFE__
fbe_test_rg_configuration_t robi_raid_group_config_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT] = 
#else
fbe_test_rg_configuration_t robi_raid_group_config_qual[] = 
#endif /* __SAFE__ SAFEMESS - shrink table/executable size */
{
    /* width,                       capacity    raid type,                                  class,                  block size      RAID-id.    bandwidth.*/
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000,     FBE_TEST_RG_CONFIG_RANDOM_TYPE_REDUNDANT,  FBE_TEST_RG_CONFIG_RANDOM_TAG,    520,            0,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var robi_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_array_t robi_raid_group_config[] = 
{

    /* RAID-1 configuraiton
     */
    {
        /* width,   capacity    raid type,                  class,                  block size, raid group id,  bandwidth */
        {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,     520,            0,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* RAID-10 configuraiton
     */
    {
        /* width,   capacity    raid type,                  class,                  block size, raid group id,  bandwidth */
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,    520,            0,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* RAID-3 configuration
     */
    {
        /* width,   capacity    raid type,                  class,                  block size, RAID-id.    bandwidth. */
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,     520,            0,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, },
    },
    /* RAID-5 configuration
     */
    {
        /* width,   capacity    raid type,                  class,                  block size, RAID-id.    bandwidth. */
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,     520,            0,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, },
    },
    /* RAID-6 configuration
     */
    {
        /* width,   capacity    raid type,                  class,                  block size, RAID-id.    bandwidth. */
        {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,     520,            0,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, },
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*************************
 *   FUNCTIONS
 *************************/
extern void fbe_test_sep_util_set_object_trace_flags(fbe_trace_type_t trace_type, 
                                              fbe_object_id_t object_id, 
                                              fbe_package_id_t package_id,
                                              fbe_trace_level_t level);
extern void diabolicdiscdemon_get_source_destination_edge_index(fbe_object_id_t  vd_object_id,
                                                                fbe_edge_index_t * source_edge_index_p,
                                                                fbe_edge_index_t * dest_edge_index_p);
extern void diabolicdiscdemon_wait_for_proactive_spare_swap_in(fbe_object_id_t  vd_object_id,
                                                               fbe_edge_index_t spare_edge_index,
                                                               fbe_test_raid_group_disk_set_t * spare_location_p);
extern void diabolicdiscdemon_wait_for_proactive_copy_completion(fbe_object_id_t vd_object_id);
extern void diabolicdiscdemon_check_event_log_msg(fbe_u32_t message_code,
                                           fbe_bool_t b_is_spare_drive_valid,
                                           fbe_test_raid_group_disk_set_t *spare_drive_location,
                                           fbe_bool_t b_is_original_drive_valid,
                                           fbe_test_raid_group_disk_set_t *original_drive_location);
extern void diabolicdiscdemon_wait_for_source_drive_swap_out(fbe_object_id_t vd_object_id,
                                                                    fbe_edge_index_t source_edge_index);
extern void diabolicdiscdemon_verify_background_pattern(fbe_api_rdgen_context_t *  in_test_context_p,
                                                        fbe_u32_t in_element_size);

/*!****************************************************************************
 * robi_inject_scsi_error()
 ******************************************************************************
 * @brief
 *  This function starts inserting errors to a PDO.
 *
 * @param   rg_config_p - Pointer to array of array groups under test
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_inject - Position in all raid groups to inject to.
 * @param   user_lba_to_object_to - User space lba to inject to
 * @param   number_of_blocks_to_inject_for - Number of bloocks to inject for.
 *
 * @return None.
 *
 * @author
 *  3/12/2011 - Created. Lili Chen
 ******************************************************************************/
static void robi_inject_scsi_error(fbe_test_rg_configuration_t *const rg_config_p,
                                   fbe_u32_t raid_group_count,
                                   fbe_u32_t position_to_inject,
                                   fbe_lba_t user_lba_to_object_to,
                                   fbe_block_count_t number_of_blocks_to_inject_for)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t                    *current_rg_config_p = rg_config_p;
    fbe_u32_t                                       rg_index;
    fbe_protocol_error_injection_error_record_t     record[FBE_TEST_MAX_RAID_GROUP_COUNT];
    fbe_protocol_error_injection_record_handle_t    record_handle_p[FBE_TEST_MAX_RAID_GROUP_COUNT];
    fbe_test_raid_group_disk_set_t                  disk_location;

    /* Validate that we haven't exceeded the maximum number of raid groups under test.
     */
    MUT_ASSERT_TRUE(raid_group_count <= FBE_TEST_MAX_RAID_GROUP_COUNT);

    /* For all the raid groups under test add a media error injection record.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        /* If this raid group is not enabled goto the next.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
    
        /* Invoke the method to initialize and populate the (1) error record
         */
        disk_location = current_rg_config_p->rg_disk_set[position_to_inject];
        status = fbe_test_neit_utils_populate_protocol_error_injection_record(disk_location.bus,
                                                                              disk_location.enclosure,
                                                                              disk_location.slot,
                                                                              &record[rg_index],
                                                                              &record_handle_p[rg_index],
                                                                              user_lba_to_object_to,
                                                                              number_of_blocks_to_inject_for,
                                                                              FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI,
                                                                              FBE_SCSI_READ_16,
                                                                              FBE_SCSI_SENSE_KEY_MEDIUM_ERROR,
                                                                              FBE_SCSI_ASC_READ_DATA_ERROR,
                                                                              FBE_SCSI_ASCQ_GENERAL_QUALIFIER,
                                                                              FBE_PORT_REQUEST_STATUS_SUCCESS, 
                                                                              1);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Add the error injection record to the service, which also returns the record handle for later use */
        status = fbe_api_protocol_error_injection_add_record(&record[rg_index], &record_handle_p[rg_index]);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);  

        /* Goto the next raid group.
         */
        current_rg_config_p++;

    } /* end for all raid groups*/

    /* Start the error injection */
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

    return;
}
/******************************
 * end robi_inject_scsi_error()
 ******************************/

/*!**************************************************************
 * robi_get_rebuild_hook_lba()
 ****************************************************************
 * @brief
 *  This function determines the lba for the rebuild hook based
 *  on the user space lba passed.
 *
 * @param   pvd_object_id - object id of pvd associated with hook
 * @param   user_space_hook_lba - user space lba to generate for
 *
 * @return  lba - The correct lba for the rebuild hook
 *
 * @author
 *  05/17/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_lba_t robi_get_rebuild_hook_lba(fbe_object_id_t pvd_object_id, fbe_lba_t user_space_hook_lba)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_lba_t                       debug_hook_lba = FBE_LBA_INVALID;

    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    debug_hook_lba = user_space_hook_lba + provision_drive_info.default_offset;

    return debug_hook_lba;
}
/******************************************
 * end robi_get_rebuild_hook_lba()
 ******************************************/

/*!**************************************************************
 * robi_wait_for_rebuild_hook()
 ****************************************************************
 * @brief
 *  This function waits for rebuild debug hook to hit.
 *
 * @param   object_id - pvd object id to check hook for
 *          lba_checkpoint - the checkpoint to wait for   
 *          wait_time - wait time in ms      
 *
 * @return  None
 *
 * @author
 *  3/12/2011 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t robi_wait_for_rebuild_hook(fbe_object_id_t object_id, fbe_lba_t lba_checkpoint, fbe_u32_t wait_time) 
{
    fbe_scheduler_debug_hook_t          hook;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t                           current_time = 0;

    hook.object_id = object_id;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.check_type = SCHEDULER_CHECK_VALS_LT;
    hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD;
    hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY;
    hook.val1 = lba_checkpoint;
    hook.val2 = NULL;
    hook.counter = 0;

    while (current_time < wait_time)
    {
        status = fbe_api_scheduler_get_debug_hook(&hook);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        if (hook.counter > 0)
        {
            return FBE_STATUS_OK;
        }

        current_time = current_time + SEP_REBUILD_UTILS_RETRY_TIME;
        fbe_api_sleep (SEP_REBUILD_UTILS_RETRY_TIME);
    }

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end robi_wait_for_rebuild_hook()
 ******************************************/

/*!****************************************************************************
 * robi_wait_for_raid_group_verify()
 ******************************************************************************
 *
 * @brief   This function waits error verify to start or complete of the raid 
 *          group specified.
 *
 * @param   rg_object_id - pointer to a raid group.
 * @param   wait_time    - time in ms to wait.
 * @param   b_wait_for_start - FBE_TRUE - wait for error verify to start
 *                             FBE_FALSE - wait for error verify to complete
 *
 * @return fbe_status_t.
 *
 * @author
 *  3/12/2011 - Created. Lili Chen
 ******************************************************************************/
static fbe_status_t robi_wait_for_raid_group_verify(fbe_object_id_t rg_object_id, 
                                                    fbe_u32_t wait_time,
                                                    fbe_bool_t b_wait_for_start)
{
    fbe_api_raid_group_get_info_t   raid_group_info;
    fbe_u32_t                       current_time = 0;
    fbe_status_t                    status = FBE_STATUS_OK;

    while (current_time < wait_time)
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* If we are waiting for the start the check for not equal end marker.
         * Otherwise we are waiting for the completion.  In this case wait
         * for the error verify checkpoint to be the end marker.
         */
        if ((b_wait_for_start == FBE_TRUE)                               &&
            (raid_group_info.error_verify_checkpoint != FBE_LBA_INVALID)    )
        {
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "== %s wait for start on rg obj: 0x%x type: %d error chkpt: 0x%llx done in: %d seconds ==", 
                       __FUNCTION__, rg_object_id, raid_group_info.raid_type, 
                       (unsigned long long)raid_group_info.error_verify_checkpoint, (current_time / 1000)); 
            return FBE_STATUS_OK;
        }
        else if ((b_wait_for_start == FBE_FALSE)                              &&
                 (raid_group_info.error_verify_checkpoint == FBE_LBA_INVALID) &&
                 (raid_group_info.b_is_event_q_empty == FBE_TRUE)               )
        {
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "== %s wait for complete on rg obj: 0x%x type: %d done in: %d seconds ==", 
                       __FUNCTION__, rg_object_id, raid_group_info.raid_type,
                       (current_time / 1000)); 
            return FBE_STATUS_OK;
        }
        current_time = current_time + 200;
        fbe_api_sleep (200);
    }
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s wait for %s on rg obj: 0x%x type: %d error chkpt: 0x%llx not done in: %d seconds ==", 
               __FUNCTION__, (b_wait_for_start) ? "start" : "complete",
               rg_object_id, raid_group_info.raid_type, (unsigned long long)raid_group_info.error_verify_checkpoint, (wait_time / 1000)); 
    return FBE_STATUS_TIMEOUT;
}
/******************************************
 * end robi_wait_for_raid_group_verify()
 ******************************************/

/*!****************************************************************************
 * robi_check_error_verify_start()
 ******************************************************************************
 * @brief
 *  This function waits error verify to start on a raid group.
 *
 * @param   rg_config_p - Pointer to array of array groups under test
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_inject - Position in all raid groups to inject to.
 * @param   wait_time    - time in ms to wait.
 *
 * @return fbe_status_t.
 *
 * @author
 *  3/12/2011 - Created. Lili Chen
 ******************************************************************************/
static fbe_status_t robi_check_error_verify_start(fbe_test_rg_configuration_t *const rg_config_p,      
                                                  fbe_u32_t raid_group_count,                          
                                                  fbe_u32_t position_to_inject,
                                                  fbe_u32_t wait_time)                        

{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t                   rg_index;
    fbe_object_id_t             rg_object_id;

    /* Validate that we haven't exceeded the maximum number of raid groups under test.
     */
    MUT_ASSERT_TRUE(raid_group_count <= FBE_TEST_MAX_RAID_GROUP_COUNT);

    /* For all the raid groups under test add a media error injection record.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        /* If this raid group is not enabled goto the next.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        /* Get the raid group object id.
         */
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* For RAID-10 we will wait for the verify on all the downstream mirrors.
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_u32_t index;
            fbe_api_base_config_downstream_object_list_t downstream_object_list;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

            /* Wait for the verify on position requested
             */
            index = position_to_inject / 2;
            status = robi_wait_for_raid_group_verify(downstream_object_list.downstream_object_list[index], 
                                                 wait_time,
                                                 FBE_TRUE /* Wait for verify to start */);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        } else {
            /* Else just wait for the raid group to start the verify.
            */
            status = robi_wait_for_raid_group_verify(rg_object_id, 
                                                 wait_time,
                                                 FBE_TRUE /* Wait for verify to start */);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Goto next riad group*/
        current_rg_config_p++;
    }

    return status;
}
/******************************************
 * end robi_check_error_verify_start()
 ******************************************/

/*!****************************************************************************
 * robi_check_error_verify_complete()
 ******************************************************************************
 * @brief
 *  This function waits error verify to complete on a raid group.
 *
 * @param   rg_config_p - Pointer to array of array groups under test
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_copy - Position in all raid groups to inject to.
 * @param   wait_time - How long to wait for each verify to complete
 *
 * @return fbe_status_t.
 *
 * @author
 *  3/12/2011 - Created. Lili Chen
 ******************************************************************************/
static fbe_status_t robi_check_error_verify_complete(fbe_test_rg_configuration_t *const rg_config_p,  
                                                     fbe_u32_t raid_group_count,                      
                                                     fbe_u32_t position_to_copy,
                                                     fbe_u32_t wait_time)                
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t                   rg_index;
    fbe_object_id_t             rg_object_id;

    /* Validate that we haven't exceeded the maximum number of raid groups under test.
     */
    MUT_ASSERT_TRUE(raid_group_count <= FBE_TEST_MAX_RAID_GROUP_COUNT);

    /* For all the raid groups under test add a media error injection record.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        /* If this raid group is not enabled goto the next.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        /* Get the raid group object id.
         */
        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* For RAID-10 we will wait for the verify on all the downstream mirrors.
         */
        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_u32_t index;
            fbe_api_base_config_downstream_object_list_t downstream_object_list;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

            /* Wait for the verify on first (i.e. pos: 0) mirror
             */
            index = position_to_copy / 2;
            status = robi_wait_for_raid_group_verify(downstream_object_list.downstream_object_list[index], 
                                                 wait_time,
                                                 FBE_FALSE /* Wait for verify to complete */);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        } else {
            /* Else just wait for the raid group to start the verify.
             */
            status = robi_wait_for_raid_group_verify(rg_object_id, 
                                                 wait_time,
                                                 FBE_FALSE /* Wait for verify to complete */);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }

    return status;
}
/******************************************
 * end robi_check_error_verify_complete()
 ******************************************/

/*!****************************************************************************
 * robi_test_verify_for_copy_error()
 ******************************************************************************
 * @brief
 *  This function confirms that when the copy operation encounters a media error
 *  that the following process occurs:
 *      1. The associated chunk on the destination drive is invalidated.
 *      2. An event is sent to the upstream (a.k.a. parent) raid group that
 *         indicates that the parent raid group must mark for error verify on the
 *         associated chunks(s).
 *      3. The parent raid group will (eventually) issue a verify.
 *      4. The invalidated data will be reconstructed from parity.
 *
 * @param   rg_config_p - Pointer to array of array groups under test
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_copy - Position in all raid groups to copy.
 *
 * @return  status
 *
 * @note    This test now runs against multiple raids group in parallel using the 
 *          background operations.
 *
 * @author
 *  3/12/2011 - Created. Lili Chen
 ******************************************************************************/
static fbe_status_t robi_test_verify_for_copy_error(fbe_test_rg_configuration_t *const rg_config_p,      
                                                    fbe_u32_t raid_group_count,                          
                                                    fbe_u32_t position_to_copy)                       
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t    *context_p = &robi_rdgen_context;
    fbe_object_id_t             rg_object_id;
    fbe_object_id_t             vd_object_id;
    fbe_spare_swap_command_t    copy_type = FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND;
    fbe_test_sep_background_copy_state_t current_copy_state;
    fbe_test_sep_background_copy_state_t copy_state_to_pause_at;
    fbe_test_sep_background_copy_event_t validation_event;
    fbe_u32_t                   percentage_rebuilt_before_pause = 0;
    fbe_u32_t                   percent_copied_before_pause = 5;
    fbe_lba_t                   checkpoint_to_pause_at = FBE_LBA_INVALID;

    /* Step 1: Write background pattern to all the luns before we start copy 
     *         operation. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Write background pattern ==", __FUNCTION__);
    diabolicdiscdemon_write_background_pattern(context_p);

    /* Step 2: Set a debug hook when 5% of the virtual drive is copied.
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_sep_background_pause_get_rebuild_checkpoint_to_pause_at(vd_object_id,
                                                                              percent_copied_before_pause,
                                                                              &checkpoint_to_pause_at);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_background_pause_set_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               &robi_copy_pause_params,
                                                               SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                               FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                               checkpoint_to_pause_at,
                                                               0,
                                                               SCHEDULER_CHECK_VALS_GT,
                                                               SCHEDULER_DEBUG_ACTION_PAUSE,
                                                               FBE_FALSE /* This is not a reboot test */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3: Start a user proactive copy request and pause at 0% complete.
     */
    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED;
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &robi_pause_params,
                                                               percentage_rebuilt_before_pause,  /* Stop when 0% of consumed rebuilt*/
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set pause when %d percent rebuilt - successfully. ==", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 4: Wait for the copy start hook but do not clear it.
     */
    status = fbe_test_sep_background_pause_wait_for_hook(rg_config_p, raid_group_count, position_to_copy,
                                                         &robi_pause_params,
                                                         FBE_FALSE /* This is not a reboot test*/);

    /* Step 5: Enable error injection when the first chunk is copied.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Enable hard media injection on copy source ==", 
               __FUNCTION__);
    robi_inject_scsi_error(rg_config_p, raid_group_count, position_to_copy,
                           robi_user_space_lba_to_inject_to, /* User lba on each raid group to inject to */
                           robi_user_space_blocks_to_inject  /* Number of blocks to inject for */);

    /* Step 6: Allow the copy to proceed.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_METADATA_REBUILD_START;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &robi_pause_params,
                                                               percentage_rebuilt_before_pause,
                                                               FBE_FALSE,   /* Only remove the hook on the active*/
                                                               NULL         /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate metadata rebuild start on new active - successful. ==", __FUNCTION__); 

    /* Step 7: Wait for the rebuild hook to be hit.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for rebuild lba: 0x%llx ==", 
               __FUNCTION__, (unsigned long long)checkpoint_to_pause_at);
    status = fbe_test_sep_background_pause_wait_for_hook(rg_config_p, raid_group_count, position_to_copy,
                                                         &robi_copy_pause_params,
                                                         FBE_FALSE /* This is not a reboot test*/);

    /* Step 8: Check error verify starts.*/
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait %d seconds for error verify start on each raid group ==", 
               __FUNCTION__, (30000 / 1000));
    status = robi_check_error_verify_start(rg_config_p, raid_group_count, position_to_copy,
                                           30000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Error verify started. ==", __FUNCTION__);

    /* Step 9: Disable error injection.
     */
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 10: Remove the copy debug hook.
     */
    status = fbe_test_sep_background_pause_remove_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                  &robi_copy_pause_params,
                                                                  FBE_FALSE /* This is not a reboot test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 11: Wait for the copy operation to complete.
     */
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_COPY_OPERATION_COMPLETE;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &robi_pause_params,
                                                               0,
                                                               FBE_FALSE, /* This is not a reboot test. */
                                                               NULL         /* No destination required*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Copy Cleanup - successful. ==", __FUNCTION__); 

    /* Step 12: Wait for all error verifies to complete.
     */
    /*! @note Since the copy operation priority (even though it's downstream) 
     *        is higher that the error verify priority, the error verify
     *        will not run until the proactive copy is complete.
     *          o FBE_MEDIC_ACTION_ERROR_VERIFY   ==> 6
     *                  :
     *          o FBE_MEDIC_ACTION_PROACTIVE_COPY ==> 9
     *  
     *        Since we must wait for the entire copy operation to complete
     *        before the error verify runs the timeout (200 seconds)
     *        is very large.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait %d seconds for error verify complete on all raid groups==", 
               __FUNCTION__, (200000 / 1000));
    status = robi_check_error_verify_complete(rg_config_p, raid_group_count, position_to_copy,
                                              200000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Error verify completed. ==", __FUNCTION__);

    /* Step 13: Validate data pattern.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reading background pattern ==", __FUNCTION__);
    diabolicdiscdemon_verify_background_pattern(context_p, ROBI_TEST_ELEMENT_SIZE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Read background pattern - success ==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "===%s complete. ===", __FUNCTION__);

    return status;
}
/*************************************** 
 * end robi_test_verify_for_copy_error()
 ***************************************/

/*!****************************************************************************
 * robi_test_permission_for_copy_denied()
 ******************************************************************************
 * @brief
 *  This function validates that if copy is disabled (after a copy operation was
 *  started) that it cannot run.
 *
 * @param   rg_config_p - Pointer to array of array groups under test
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_copy - Position in all raid groups to copy.
 *
 * @return  status
 *
 * @note    This test only runs against a since raid group.
 *
 * @author
 *  3/12/2011 - Created. Lili Chen
 ******************************************************************************/
static fbe_status_t robi_test_permission_for_copy_denied(fbe_test_rg_configuration_t *const rg_config_p,    
                                                 fbe_u32_t raid_group_count,                        
                                                 fbe_u32_t position_to_copy)                        
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t        *context_p = &robi_rdgen_context;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_spare_swap_command_t        copy_type = FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND;
    fbe_test_sep_background_copy_state_t current_copy_state;
    fbe_test_sep_background_copy_state_t copy_state_to_pause_at;
    fbe_test_sep_background_copy_event_t validation_event;
    fbe_u32_t                       percent_copied_before_pause = 5;
    fbe_lba_t                       checkpoint_to_pause_at;
    fbe_u32_t                       percentage_rebuilt_before_pause = 10;

    /* Validate a single raid group.
     */
    MUT_ASSERT_TRUE(raid_group_count == 1);
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Step 1: Write background pattern to all the luns before we start copy 
     *         operation. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Write background pattern ==", __FUNCTION__);
    diabolicdiscdemon_write_background_pattern(context_p);

    /* Step 2: Set a debug hook to pause when permission is denied.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Add Rebuild Permission Denied Debug Hook vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_PERMISSION,
                                              0,
                                              NULL,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

    /* Step 3: Set a debug hook when 5% of the virtual drive is copied.
     */
    status = fbe_test_sep_background_pause_get_rebuild_checkpoint_to_pause_at(vd_object_id,
                                                                              percent_copied_before_pause,
                                                                              &checkpoint_to_pause_at);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_background_pause_set_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               &robi_copy_pause_params,
                                                               SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                               FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                               checkpoint_to_pause_at,
                                                               0,
                                                               SCHEDULER_CHECK_VALS_GT,
                                                               SCHEDULER_DEBUG_ACTION_PAUSE,
                                                               FBE_FALSE /* This is not a reboot test */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 4: Start a user proactive copy request and pause at 10% complete.
     */
    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED;
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &robi_pause_params,
                                                               percentage_rebuilt_before_pause,  /* Stop when 10% of consumed rebuilt*/
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set pause when %d percent rebuilt - successfully. ==", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 5: Wait for the rebuild hook to be hit.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for rebuild lba: 0x%llx ==", 
               __FUNCTION__, (unsigned long long)checkpoint_to_pause_at);
    status = fbe_test_sep_background_pause_wait_for_hook(rg_config_p, raid_group_count, position_to_copy,
                                                         &robi_copy_pause_params,
                                                         FBE_FALSE /* This is not a reboot test*/);

    /* Step 6: Now set the `permission denied' flag for this virtual drive.
     */
    status = fbe_api_base_config_set_deny_operation_permission(vd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

    /* Step 7: Now remove the debug hook and let the copy start
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove Rebuild Debug Hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, (unsigned long long)checkpoint_to_pause_at);
    status = fbe_test_sep_background_pause_remove_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                  &robi_copy_pause_params,
                                                                  FBE_FALSE /* This is not a reboot test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

    /* Step 8: Validate that we observe permission denied.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for `No Permission' Debug Hook vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                         FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_PERMISSION,
                                         SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for `No Permission' Debug Hook vd obj: 0x%x - complete==", 
               __FUNCTION__, vd_object_id);

    /* Step 9: Now clear the `permission denied' flag for this virtual drive.
     */
    status = fbe_api_base_config_clear_deny_operation_permission(vd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 10: Now delete the `deny' background operation debug hook.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove Deny Debug Hook vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_PERMISSION,
                                              0,
                                              NULL,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

    /* Step 11: Validate virtual drive hits 10% copied hook.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_DESIRED_PERCENTAGE_REBUILT;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &robi_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate %d percent rebuilt complete - successful. ==", __FUNCTION__, percentage_rebuilt_before_pause);
    
    /* Step 12: Wait for the copy operation to complete.
     */
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_COPY_OPERATION_COMPLETE;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &robi_pause_params,
                                                               0,
                                                               FBE_FALSE, /* This is not a reboot test. */
                                                               NULL         /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Copy Cleanup - successful. ==", __FUNCTION__); 

    /* Step 13: Validate data pattern.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reading background pattern ==", __FUNCTION__);
    diabolicdiscdemon_verify_background_pattern(context_p, ROBI_TEST_ELEMENT_SIZE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Read background pattern - success ==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "===%s complete. ===", __FUNCTION__);

    return status;
}
/********************************************* 
 * end robi_test_permission_for_copy_denied()
 *********************************************/

/*!****************************************************************************
 * robi_run_tests_on_rg_config()
 ******************************************************************************
 * @brief
 *  This function runs sparing tests on a raid group.
 *
 * @param 
 *   rg_config_ptr - pointer to config.
 *   context_p     - pointer to context.
 *
 * @return  None.
 *
 * @note    This test now runs against multiple raids group in parallel using the 
 *          background operations.
 *
 * @author
 *  3/12/2011 - Created. Lili Chen
 ******************************************************************************/
static void robi_run_tests_on_rg_config(fbe_test_rg_configuration_t *rg_config_p ,void *unused_context_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_sector_trace_error_flags_t sector_trace_debug_flags;
    fbe_u32_t                       position_to_copy;

    /* Wait for all verifies to complete.
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    /* Set the trace level as information. */
    fbe_test_sep_util_set_object_trace_flags(FBE_TRACE_TYPE_DEFAULT, FBE_OBJECT_ID_INVALID, FBE_PACKAGE_ID_SEP_0, FBE_TRACE_LEVEL_INFO);

    /*! @note Set the the sector trace debug flags (when set can `stop on error' 
     *        is set any unexpected errors will generate an error trace).
     *        Due to the fact that we invalidate the row parity and not the
     *        diagonal parity we must allow POC COH errors.
     */
    fbe_test_sep_util_enable_trace_limits(FBE_TRUE);
    sector_trace_debug_flags = (FBE_SECTOR_TRACE_DEFAULT_ERROR_FLAGS & ~FBE_SECTOR_TRACE_ERROR_FLAG_POC);
    fbe_test_sep_util_set_sector_trace_debug_flags(sector_trace_debug_flags);

    /* Except for injecting media errors we don't expect ANY other
     * errors (for instance we don't expect any coherency errors).
     */
    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_TRUE);

    /* Determine the position to copy (must be the same for all raid groups).
     */
    position_to_copy = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Speed up VD hot spare */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Test 1:  Start a copy operation and injection a media error into the
     *          read of the source drive.  Confirm parent raid group corrects
     *          invalidated data.
     */
    status = robi_test_verify_for_copy_error(rg_config_p, raid_group_count, position_to_copy);                         
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Test 2:  Validate the `permission denied' if the copy process is disabled.
     */
    /*! @note Currently only runs against a single raid group.*/
    status = robi_test_permission_for_copy_denied(rg_config_p, 1, position_to_copy);       
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Restore the sector trace limits.*/
    fbe_test_sep_util_disable_trace_limits();

    /* Disable stop on error
     */
    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_FALSE);

    /* Restore the default spare trigger time
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    return;
}
/********************************* 
 * end robi_run_tests_on_rg_config()
 *********************************/

/*!****************************************************************************
 * robi_test()
 ******************************************************************************
 * @brief
 *  Run drive proactive copy error handling test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  3/12/2011 - Created. Lili Chen
 ******************************************************************************/
void robi_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    /* We now test all raid groups at the same time.
     */
    if (test_level > 0)
    {
        /* Extended.
         */
        rg_config_p = &robi_raid_group_config_extended[0];
    }
    else
    {
        /* Qual.
         */
        rg_config_p = &robi_raid_group_config_qual[0];
    }

    /* Since this doesn't test degraded disk not additional disk are 
     * required.  All the copy disk will come from the automatic spare pool.
     */
    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p,NULL,robi_run_tests_on_rg_config,
                                                  ROBI_TEST_LUNS_PER_RAID_GROUP,
                                                  ROBI_TEST_CHUNKS_PER_LUN,FBE_FALSE);

    return;    

}


/*!****************************************************************************
 * robi_setup()
 ******************************************************************************
 * @brief
 *  Run drive sparing error handling test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  3/12/2011 - Created. Lili Chen
 ******************************************************************************/
void robi_setup(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        /* Randomize which raid type to use.
         */
        if (test_level > 0)
        {
            /* Extended.
             */
            rg_config_p = &robi_raid_group_config_extended[0];
        }
        else
        {
            /* Qual.
             */
            rg_config_p = &robi_raid_group_config_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* Initialize the number of extra drives required by RG. 
         * Used when create physical configuration for RG in simulation. 
         */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        /* Determine and load the physical config and populate all the 
         * raid groups.
         */
        fbe_test_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        sep_config_load_sep_and_neit();
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            sep_config_load_kms(NULL);
            fbe_test_sep_util_enable_kms_encryption();
        }
    }
    
    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    return;
}

/*!****************************************************************************
 * robi_cleanup()
 ******************************************************************************
 * @brief
 *  clean up function for robi test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  3/12/2011 - Created. Lili Chen
 ******************************************************************************/
void robi_cleanup(void)
{
    if (fbe_test_util_is_simulation())
    {
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            fbe_test_sep_util_destroy_kms();
        }
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    fbe_test_sep_util_set_encryption_test_mode(FBE_FALSE);
    return;
}
