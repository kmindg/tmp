/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file biff_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a firmware download test.
 *
 * @version
 *   9/10/2009 - Created from kermit.
 *               Relocated fw download test code here. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_database.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "neit_utils.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_esp_drive_mgmt.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "EspMessages.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe/fbe_drive_configuration_download.h"
#include "fbe/fbe_api_database_interface.h"
#include "PhysicalPackageMessages.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe_testability.h"
#include "sep_hook.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * biff_short_desc = "Firmware Download test.";
char * biff_long_desc ="\
Downloads firmware to multiple drives..\n\
Optional Switches:\n\
   -run_test_case    : Runs a single Test Case. \n\
\n\
   Test Cases:\n\
    1 - Test Non EQ Drive with DCS Policy\n\
    2 - Test Non EQ Drive with DMO Policy\n\
    3 - Test Downloads with RG Config\n\
";


/*!*******************************************************************
 * @def BIFF_LUNS_PER_RAID_GROUP_QUAL
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define BIFF_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def BIFF_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define BIFF_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def BIFF_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define BIFF_SMALL_IO_SIZE_MAX_BLOCKS 1024


/*!*******************************************************************
 * @var BIFF_LARGE_IO_COUNT
 *********************************************************************
 * @brief Number of large I/Os we will perform before finishing the test.
 *
 *********************************************************************/
fbe_u32_t BIFF_LARGE_IO_COUNT = 5;

/*!*******************************************************************
 * @var BIFF_THREAD_COUNT
 *********************************************************************
 * @brief Number of threads we will run for I/O.
 *
 *********************************************************************/
fbe_u32_t BIFF_THREAD_COUNT = 5;

static fbe_api_rdgen_context_t biff_test_contexts[BIFF_LUNS_PER_RAID_GROUP];

/*!*******************************************************************
 * @def BIFF_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define BIFF_RAID_TYPE_COUNT 6

/*!*******************************************************************
 * @def BIFF_DOWNLOAD_SLEEP_INTERVAL
 *********************************************************************
 * @brief Number of miliseconds we sleep to check firmware download callback.
 *
 *********************************************************************/
#define BIFF_DOWNLOAD_SLEEP_INTERVAL 1000

/*!*******************************************************************
 * @def BIFF_EVENT_LOG_CHECK_SLEEP_INTERVAL
 *********************************************************************
 * @brief Number of miliseconds we sleep to check firmware download
 *        event log update.
 *
 *********************************************************************/
#define BIFF_EVENT_LOG_CHECK_SLEEP_INTERVAL 2000

/*!*******************************************************************
 * @def BIFF_EVENT_LOG_CHECK_COUNT
 *********************************************************************
 * @brief Number of times we will try and query event log to check for 
 *        firmware download related logs.
 *
 *********************************************************************/
#define BIFF_EVENT_LOG_CHECK_COUNT 7

/*!*******************************************************************
 * @def BIFF_DOWNLOAD_SECONDS_PER_DRIVE
 *********************************************************************
 * @brief
 *  We will wait at least 30 seconds in between each drive download
 *  in a raid group.  We double this to cover any variations in
 *  timing caused by running multiple threads in parallel.
 *
 *********************************************************************/
#define BIFF_DOWNLOAD_SECONDS_PER_DRIVE 60

/*!*******************************************************************
 * @var biff_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t biff_raid_group_config_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t biff_raid_group_config_qual[] =
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
    /* Raid 1 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            0,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 5 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 3 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            0,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 6 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            0,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 0 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {3,       0x9000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            1,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 10 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,    520,           0,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/**************************************
 * end biff_raid_group_config_qual()
 **************************************/


/*!*******************************************************************
 * @var biff_vault_raid_group_config
 *********************************************************************
 * @brief This is the raid group created on vault drives.
 *
 *********************************************************************/
static fbe_test_rg_configuration_t biff_vault_raid_group_config = 
    /* width,   capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {4,         0x32000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            0,          0};

/*!*******************************************************************
 * @var biff_vault_disk_set_520
 *********************************************************************
 * @brief This is the disk set for raid group created on vault drives.
 *
 *********************************************************************/
static fbe_test_raid_group_disk_set_t biff_vault_disk_set_520[] = {
    /* width = 4 */
    {0,0,0}, {0,0,1}, {0,0,2}, {0,0,3}, 
    /* terminator */
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}
};



extern fbe_status_t sleeping_beauty_wait_for_state(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_lifecycle_state_t state, fbe_u32_t timeout_ms);
extern void         sleeping_beauty_verify_event_log_drive_online(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
extern void         sleeping_beauty_verify_event_log_drive_offline(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_physical_drive_information_t *pdo_drive_info_p, dmo_event_log_drive_offline_reason_t offline_reason);
extern void sir_topham_hatt_create_fw(fbe_sas_drive_type_t drive_type, fbe_drive_configuration_control_fw_info_t *fw_info);
extern void sir_topham_hatt_destroy_fw(fbe_drive_configuration_control_fw_info_t *fw_info_p);
extern void sir_topham_hatt_fw_set_tla(fbe_drive_configuration_control_fw_info_t *fw_info, const fbe_u8_t *tla, fbe_u32_t tla_size);
extern fbe_status_t sir_topham_hatt_wait_download_process_state(fbe_drive_configuration_download_get_process_info_t * dl_status,
                                                         fbe_drive_configuration_download_process_state_t dl_state,
                                                         fbe_u32_t seconds);
extern void sir_topham_hatt_check_drive_download_status(fbe_drive_selected_element_t *drive,
                                                 fbe_drive_configuration_download_drive_state_t state,
                                                 fbe_drive_configuration_download_drive_fail_t fail_reason);
extern fbe_status_t sir_topham_hatt_check_fw_download_event_log(fbe_drive_configuration_control_fw_info_t *fw_info, 
                                                         fbe_u32_t msg_id, fbe_drive_configuration_download_process_fail_t fail_code,
                                                         fbe_bool_t *msg_present);

extern void big_bird_insert_drives(fbe_test_rg_configuration_t *rg_config_p,
                                   fbe_u32_t raid_group_count,
                                   fbe_bool_t b_transition_pvd);
extern void big_bird_remove_drives(fbe_test_rg_configuration_t *rg_config_p, 
                                   fbe_u32_t raid_group_count,
                                   fbe_bool_t b_transition_pvd_fail,
                                   fbe_bool_t b_pull_drive,
                                   fbe_test_drive_removal_mode_t removal_mode);


static void     biff_list_test_cases(void);
static void     biff_run_test_case(fbe_u32_t test_case);
static void     biff_run_download_with_all_rg_configs(void);
static void     biff_run_all_test_cases(void);
static void     biff_trial_run_test(fbe_test_rg_configuration_t *rg_config_p);
void            biff_send_trial_run_verify_expected(fbe_u8_t *tla, fbe_u32_t tla_size, fbe_drive_selected_element_t *expect_qualified, fbe_u32_t expect_qualified_count);
void            biff_verify_fw_rev(fbe_test_rg_configuration_t *rg_config_p, const fbe_u8_t * expected_rev, fbe_sim_transport_connection_target_t sp);
fbe_status_t    biff_test_wait_for_pvds_powerup(const fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t wait_time_ms);
void            biff_verify_default_non_eq_policy(void);
void            biff_test_non_eq_drive_upgrade_with_dcs_policy(void);
void            biff_test_non_eq_drive_with_dmo_policy(void);
void            biff_verify_drive_is_logically_online(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
void            biff_wait_for_pdo_clients(fbe_object_id_t pdo, fbe_u32_t wait_time_ms);
fbe_status_t    biff_get_pvd_connected_to_pdo(fbe_object_id_t pdo, fbe_object_id_t *pvd, fbe_u32_t timeout_ms);
void            biff_insert_new_drive(fbe_sas_drive_type_t drive_type, fbe_terminator_sas_drive_type_default_info_t *new_page_info_p, fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
void            biff_insert_new_drive_for_sp(fbe_sas_drive_type_t drive_type, fbe_terminator_sas_drive_type_default_info_t *new_page_info_p, fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_sas_address_t * sas_addr, fbe_sim_transport_connection_target_t sp);
void            biff_remove_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_bool_t wait_for_destroy);
void            biff_remove_drive_for_sp(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_bool_t wait_for_destroy, fbe_sim_transport_connection_target_t sp);


/*!**************************************************************
 * biff_start_heavy_io()
 ****************************************************************
 * @brief
 *  Run multi-thread IO to all the raid groups.
 *   
 *
 * @author
 *  4/4/2011 - Created. Ruomu Gao
 *
 ****************************************************************/
void biff_start_heavy_io(void)
{
    fbe_status_t status;
    fbe_api_rdgen_context_t *context_p = biff_test_contexts;

    /* Set up for issuing write/read/check forever
     */
    status = fbe_api_rdgen_test_context_init(   context_p, 
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_CLASS_ID_LUN,
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                                FBE_RDGEN_PATTERN_LBA_PASS,
                                                0,      /* run forever*/ 
                                                0, /* io count not used */
                                                0, /* time not used*/
                                                BIFF_THREAD_COUNT, /* threads per lun */
                                                FBE_RDGEN_LBA_SPEC_RANDOM,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                FBE_LBA_INVALID,    /* use capacity */
                                                FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                                1,    /* Min blocks */
                                                BIFF_SMALL_IO_SIZE_MAX_BLOCKS /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Run our I/O test
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}

/* This variable is used to track the number of drives remaining to download
 * Do we need a lock around the variable. We'll see...
 */
fbe_u32_t biff_num_drives_remaining;

/*!**************************************************************
 * biff_download_async_callback()
 ****************************************************************
 * @brief
 *  This function handles callback for download.
 *  
 * @param callback_context - the download context
 * 
 *
 * @author
 *  4/4/2011 - Created. Ruomu Gao
 *
 ****************************************************************/
void biff_download_async_callback(void *callback_context)
{
    fbe_physical_drive_fw_download_asynch_t *dl_context_p =
        (fbe_physical_drive_fw_download_asynch_t *)callback_context;

    /* Check the status of the download and decrement the unfinished count
     * There are limited number of return status, we can potentially check them.
     */
    if (dl_context_p->completion_status ==  FBE_STATUS_OK)
    {
        biff_num_drives_remaining--;
    }

    MUT_ASSERT_INT_EQUAL(dl_context_p->download_info.download_error_code, 0);

    fbe_api_free_memory(dl_context_p);

}


/*!**************************************************************
 * biff_trial_run_test()
 ****************************************************************
 * @brief
 *  Run drive firmware trial run test.
 *  
 * @param rg_config_p - Raid group table.
 * @param num_raid_groups - Number of raid groups.
 * 
 *
 * @author
 *  12/24/2012  Wayne Garrett - Created.
 *
 ****************************************************************/

static void biff_trial_run_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                    status;
    fbe_drive_configuration_control_fw_info_t fw_info;
    fbe_u32_t i, j;
    fbe_drive_selected_element_t drive;
    fbe_drive_configuration_download_get_process_info_t dl_status;
    fbe_bool_t is_rg_degraded = FBE_FALSE;

    mut_printf(MUT_LOG_TEST_STATUS, "%s start - rg_id=%d type=%d width=%d ===", 
               __FUNCTION__, rg_config_p->raid_group_id, rg_config_p->raid_type, rg_config_p->width);

    sir_topham_hatt_create_fw(FBE_SAS_DRIVE_SIM_520, &fw_info);
    fw_info.header_image_p->cflags2 |= FBE_FDF_CFLAGS2_TRIAL_RUN;

    /* Send download to DCS */
    status = fbe_api_drive_configuration_interface_download_firmware(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check trial run completed succesfully.  It should always be success even if drives are marked non-qual. */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);

    sir_topham_hatt_destroy_fw(&fw_info);

    /* if atleast one drive has been removed, then consider RG as degraded, which means we expect
       trial run to return non_qual */
    if (FBE_U32_MAX != rg_config_p->specific_drives_to_remove[0])
    {
        is_rg_degraded = FBE_TRUE;
    }

    /* Verify each drive status */
    for (i = 0; i < rg_config_p->width; i++)
    {
        fbe_bool_t is_drive_removed = FBE_FALSE;

        drive.bus =       (fbe_u8_t)rg_config_p->rg_disk_set[i].bus;
        drive.enclosure = (fbe_u8_t)rg_config_p->rg_disk_set[i].enclosure;
        drive.slot =      (fbe_u8_t)rg_config_p->rg_disk_set[i].slot;

        /* check if this drive was removed by test */
        j = 0;
        while (j < FBE_RAID_MAX_DISK_ARRAY_WIDTH &&
               FBE_U32_MAX != rg_config_p->specific_drives_to_remove[j] )
        {        
            if (i == rg_config_p->specific_drives_to_remove[j])
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s verify removed drive %d_%d_%d", 
                           __FUNCTION__, rg_config_p->rg_disk_set[i].bus, rg_config_p->rg_disk_set[i].enclosure, rg_config_p->rg_disk_set[i].slot);

                is_drive_removed = FBE_TRUE;
            }
            j++;
        }


        if (is_drive_removed)
        {
            /* Drive does not exist, no status to get.
               TODO: code should be fixed to return a specific fail status when drive doesn't exist. */
            continue;
        }

        if (is_rg_degraded)
        {
            sir_topham_hatt_check_drive_download_status(&drive,
                                                        FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL,
                                                        FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_NONQUALIFIED);
            continue;
        }

        sir_topham_hatt_check_drive_download_status(&drive,
                                                    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_COMPLETE_TRIAL_RUN,
                                                    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_NO_FAILURE);
    }    
}

/*!**************************************************************
 * biff_send_trial_run_verify_expected()
 ****************************************************************
 * @brief
 *  Send trial run, then verify it was succesful.
 *  
 * @param tla - TLA string to send trial run for
 * @param tla_size - tla size
 * @param expect_qualified - an array of drives that are expected
 *           to be qualifed.
 * @param expect_qualified_count - number of drives to be qualifed.
 * 
 *
 * @author
 *  12/24/2012  Wayne Garrett - Created.
 *
 ****************************************************************/
void biff_send_trial_run_verify_expected(fbe_u8_t *tla, fbe_u32_t tla_size, fbe_drive_selected_element_t *expect_qualified, fbe_u32_t expect_qualified_count)
{
    fbe_drive_configuration_control_fw_info_t fw_info;
    fbe_drive_configuration_download_get_process_info_t dl_status;
    fbe_drive_configuration_download_get_drive_info_t *	get_all;
    fbe_u32_t 			expected_count = FBE_MAX_DOWNLOAD_DRIVES;
	fbe_u32_t			returned_count;
    fbe_u32_t           i,d;
    fbe_u32_t           num_qualified = 0;
    fbe_status_t status;

    sir_topham_hatt_create_fw(FBE_SAS_DRIVE_SIM_520, &fw_info);
    fw_info.header_image_p->cflags2 = (FBE_FDF_CFLAGS2_CHECK_TLA | FBE_FDF_CFLAGS2_TRIAL_RUN);

    sir_topham_hatt_fw_set_tla(&fw_info, tla, tla_size);

    /* Send download to DCS */
    status = fbe_api_drive_configuration_interface_download_firmware(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check trial run completed succesfully.  It should always be success even if drives are marked non-qual. */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);


    /* verify results 
    */
    get_all = (fbe_drive_configuration_download_get_drive_info_t *)fbe_api_allocate_memory(sizeof(fbe_drive_configuration_download_get_drive_info_t) * expected_count);


    status = fbe_api_drive_configuration_interface_get_all_download_drives(get_all, expected_count, &returned_count);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);    

    /* verify that only the expected_qualifed drives return the state "TRIAL_RUN" */
    for (d = 0; d < returned_count; d++)
    {
        if (get_all[d].state == FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_COMPLETE_TRIAL_RUN)
        {
            /* find matching drive */
            for (i = 0; i < expect_qualified_count; i++)
            {
                if (get_all[d].bus       == expect_qualified[i].bus &&
                    get_all[d].enclosure == expect_qualified[i].enclosure &&
                    get_all[d].slot      == expect_qualified[i].slot)
                {
                    num_qualified++;
                    break;
                }
                MUT_FAIL_MSG("Drive was qualifid by trial run, but not in the expected list");
            }
        }                    
    }

    MUT_ASSERT_INT_EQUAL_MSG(num_qualified, expect_qualified_count, "mismatch for expected qualified drives");


    fbe_api_free_memory(get_all);

    sir_topham_hatt_destroy_fw(&fw_info);
}


/*!**************************************************************
 * biff_dfu_test()
 ****************************************************************
 * @brief
 *  Run drive firmware download dfu test.
 *  DFU - rapid download to all drives at once. IO can not proceed during DFU.
 *  
 * @param rg_config_p - Raid group table.
 * @param num_raid_groups - Number of raid groups.
 * 
 *
 * @author
 *  4/4/2011 - Created. Ruomu Gao
 *
 ****************************************************************/

void biff_dfu_test(fbe_test_rg_configuration_t *rg_config_p,
                     fbe_api_rdgen_context_t *context_p)
{
    fbe_status_t                    status;
    fbe_u32_t                       i;
    fbe_u32_t                       retry_count;
    fbe_bool_t                      msg_present;
    fbe_test_raid_group_disk_set_t *drive_to_download_p;
    fbe_drive_configuration_control_fw_info_t fw_info;
    fbe_drive_configuration_download_get_process_info_t dl_status;
    fbe_drive_selected_element_t *drive;
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test Begin ===", __FUNCTION__);        

    /* Run heavy IO to all the raid groups. 
     */
    biff_start_heavy_io();


    sir_topham_hatt_create_fw(FBE_SAS_DRIVE_SIM_520, &fw_info);
    /* Select drives */
    fw_info.header_image_p->num_drives_selected = rg_config_p->width;
    fw_info.header_image_p->cflags  |= FBE_FDF_CFLAGS_SELECT_DRIVE;
    fw_info.header_image_p->cflags2 |= FBE_FDF_CFLAGS2_FAST_DOWNLOAD;
    drive = &fw_info.header_image_p->first_drive;
    for (i = 0; i < rg_config_p->width; i++)
    {
        drive_to_download_p = &rg_config_p->rg_disk_set[i];
        drive->bus = drive_to_download_p->bus;
        drive->enclosure = drive_to_download_p->enclosure;
        drive->slot = drive_to_download_p->slot;
        mut_printf(MUT_LOG_TEST_STATUS, "%s sending fast download to %d_%d_%d", __FUNCTION__, 
                   drive->bus, drive->enclosure, drive->slot);
        drive++;
    }

    /* Send download to DCS */
    status = fbe_api_drive_configuration_interface_download_firmware(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check download process started */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_RUNNING, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Checking event log for download started message.", __FUNCTION__);
    for (retry_count = 0; retry_count < BIFF_EVENT_LOG_CHECK_COUNT; retry_count++){
        msg_present = FBE_FALSE;
        status = sir_topham_hatt_check_fw_download_event_log(&fw_info, PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_STARTED, 
                                                            0,&msg_present);
        if ((status == FBE_STATUS_OK) && msg_present)
        {
            break;
        }
        fbe_api_sleep(BIFF_EVENT_LOG_CHECK_SLEEP_INTERVAL);
    }

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(msg_present);

    /* Check download process finished.   This if fast download so it should finish quickly.  
       2 seconds for drive + 5 secs for pdo activate rotary + some margin because this is a simulation.  */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, 
                                                         FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL, 
                                                         BIFF_DOWNLOAD_SECONDS_PER_DRIVE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE, dl_status.fail_code);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Checking event log for download comlpeted message.", __FUNCTION__);
    for (retry_count = 0; retry_count < BIFF_EVENT_LOG_CHECK_COUNT; retry_count++){
        msg_present = FBE_FALSE;
        status = sir_topham_hatt_check_fw_download_event_log(&fw_info, PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_COMPLETED, 
                                                            0,&msg_present);
        if ((status == FBE_STATUS_OK) && msg_present)
        {
            break;
        }
        fbe_api_sleep(BIFF_EVENT_LOG_CHECK_SLEEP_INTERVAL);
    }

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(msg_present);

    /* Let IO continue for 3 second, and observe that there are no IO errors.
     */
    fbe_api_sleep(BIFF_DOWNLOAD_SLEEP_INTERVAL*3);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, 
                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

    status = biff_test_wait_for_pvds_powerup(rg_config_p, 10000 /*msec*/);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Clean up */
    sir_topham_hatt_destroy_fw(&fw_info);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test End ===\n", __FUNCTION__);
    return;
}

/*!**************************************************************
 * biff_odfu_test()
 ****************************************************************
 * @brief
 *  Run drive firmware download odfu test.
 *  ODFU - traditional download type. Download one drive at a time.
 *  IO can proceed during ODFU.
 *  
 * @param rg_config_p - Raid group table.
 * @param num_raid_groups - Number of raid groups.
 * 
 *
 * @author
 *  4/4/2011 - Created. Ruomu Gao
 *  4/15/2013 - Modified to handle FBE_FDF_CFLAGS_FAIL_NONQUALIFIED
 *              and to check each drive's download status. Darren Insko
 *
 ****************************************************************/

void biff_odfu_test(fbe_test_rg_configuration_t *rg_config_p,
                      fbe_api_rdgen_context_t *context_p)
{
    fbe_u32_t    pass_num, total_num_passes=1;
    fbe_status_t status;
    fbe_u32_t    i;
    fbe_bool_t   msg_present;
    fbe_u32_t    retry_count;
    fbe_test_raid_group_disk_set_t *drive_to_download_p;
    fbe_drive_configuration_control_fw_info_t fw_info;
    fbe_drive_configuration_download_get_process_info_t dl_status;
    fbe_drive_selected_element_t *drive;    


    if (FBE_RAID_GROUP_TYPE_RAID0 == rg_config_p->raid_type)
    {
        total_num_passes = 2;
    }

    for (pass_num = 1; pass_num <= total_num_passes; pass_num++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s start, pass %d of %d passes\n", __FUNCTION__, pass_num, total_num_passes); 

        /* Take the first raid group, and send drive firmware download command one 
         * drive at a time.
         * TODO: need to add quiesce or drive pull test.
         */
        biff_num_drives_remaining = rg_config_p->width;

        /* Run heavy IO to all the raid groups. 
         */
        biff_start_heavy_io();

        sir_topham_hatt_create_fw(FBE_SAS_DRIVE_SIM_520, &fw_info);

        /* Select drives */
        fw_info.header_image_p->num_drives_selected = rg_config_p->width;
        fw_info.header_image_p->cflags |= FBE_FDF_CFLAGS_SELECT_DRIVE;
        drive = &fw_info.header_image_p->first_drive;
        for (i = 0; i < rg_config_p->width; i++)
        {
            drive_to_download_p = &rg_config_p->rg_disk_set[i];
            drive->bus = drive_to_download_p->bus;
            drive->enclosure = drive_to_download_p->enclosure;
            drive->slot = drive_to_download_p->slot;
            mut_printf(MUT_LOG_TEST_STATUS, "%s, pass %d.  Sending download to %d_%d_%d\n", __FUNCTION__, pass_num,
                       drive->bus, drive->enclosure, drive->slot);
            drive++;
        }

        /* Add more CFLAGS if warranted, depending upon the RAID type being tested
         * and what pass of this test is being executed.
         */
        if ((FBE_RAID_GROUP_TYPE_RAID0 == rg_config_p->raid_type) &&
            (2 == pass_num))
        {
            fw_info.header_image_p->cflags |= FBE_FDF_CFLAGS_FAIL_NONQUALIFIED;
            mut_printf(MUT_LOG_TEST_STATUS, "%s, pass %d.  Adding FBE_FDF_CFLAGS_FAIL_NONQUALIFIED\n", __FUNCTION__, pass_num); 
        }

        /* Send download to DCS */
        status = fbe_api_drive_configuration_interface_download_firmware(&fw_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        {
            /* Check download process started */
            status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_RUNNING, 10);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);
    
            mut_printf(MUT_LOG_TEST_STATUS, "%s, pass %d.  Checking event log for download started message.\n", __FUNCTION__, pass_num);
            for (retry_count = 0; retry_count < BIFF_EVENT_LOG_CHECK_COUNT; retry_count++){
                msg_present = FBE_FALSE;
                status = sir_topham_hatt_check_fw_download_event_log(&fw_info, PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_STARTED,
                                                                     0,&msg_present);
                if ((status == FBE_STATUS_OK) && msg_present)
                {
                    break;
                }
                fbe_api_sleep(BIFF_EVENT_LOG_CHECK_SLEEP_INTERVAL);
            }
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(msg_present);
    
            /* Check download process finished.   Estimate 30 seconds for each drive. */
            status = sir_topham_hatt_wait_download_process_state(&dl_status, 
                                                                 FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL, 
                                                                 BIFF_DOWNLOAD_SECONDS_PER_DRIVE * fw_info.header_image_p->num_drives_selected);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_EQUAL(FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE, dl_status.fail_code);

            mut_printf(MUT_LOG_TEST_STATUS, "%s, pass %d.  Checking event log for download completed message.\n", __FUNCTION__, pass_num);
            for (retry_count = 0; retry_count < BIFF_EVENT_LOG_CHECK_COUNT; retry_count++){
                msg_present = FBE_FALSE;
                status = sir_topham_hatt_check_fw_download_event_log(&fw_info, PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_COMPLETED,
                                                                     0,&msg_present);
                if ((status == FBE_STATUS_OK) && msg_present)
                {
                    break;
                }
                fbe_api_sleep(BIFF_EVENT_LOG_CHECK_SLEEP_INTERVAL);
            }

            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(msg_present);
        }

        /* Let IO continue for 3 second, and observe that there are no IO errors.
         */
        fbe_api_sleep(BIFF_DOWNLOAD_SLEEP_INTERVAL*3);
        status = fbe_api_rdgen_stop_tests(context_p, 1);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, 
                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

        status = biff_test_wait_for_pvds_powerup(rg_config_p, 10000 /*msec*/);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);               

        /* Clean up */
        sir_topham_hatt_destroy_fw(&fw_info);

    }

    return;
}

/*!**************************************************************
 * biff_verify_fw_rev()
 ****************************************************************
 * @brief
 *  Verify the fw rev for the drives in RG match
 *  
 * @param rg_config_p - Raid group table.
 * @param expected_rev - expected fw rev
 * @param sp - SP to verify rev on.
 * 
 *
 * @author
 *  12/24/2013 : Wayne Garrett - created.
 *
 ****************************************************************/
void biff_verify_fw_rev(fbe_test_rg_configuration_t *rg_config_p, const fbe_u8_t * expected_rev, fbe_sim_transport_connection_target_t sp)
{
    fbe_test_raid_group_disk_set_t *drive_p;
    fbe_physical_drive_information_t drive_info = {0};
    fbe_object_id_t pdo_id;
    fbe_u32_t i;
    fbe_status_t status;
    fbe_sim_transport_connection_target_t orig_sp; /* so we can restore */

    orig_sp = fbe_api_sim_transport_get_target_server();  
    fbe_api_sim_transport_set_target_server(sp);

    
    for (i = 0; i < rg_config_p->width; i++)
    {
        drive_p = &rg_config_p->rg_disk_set[i];
        
        status = fbe_api_get_physical_drive_object_id_by_location(drive_p->bus, 
                                                                  drive_p->enclosure, 
                                                                  drive_p->slot, 
                                                                  &pdo_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK)
                                    
        status = fbe_api_physical_drive_get_drive_information(pdo_id, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        MUT_ASSERT_STRING_EQUAL_MSG(drive_info.product_rev, expected_rev, "fw rev doesn't match");
    }

    fbe_api_sim_transport_set_target_server(orig_sp); /* restore */
}


/*!**************************************************************
 * biff_test_wait_for_pvds_powerup()
 ****************************************************************
 * @brief
 *  Wait for PVDs to complete powerup step in download state
 *  machine.
 *  
 * @param rg_config_p - Raid group table.
 * @param wait_time_ms - time to wait.
 * 
 *
 * @author
 *  12/24/2013 - Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_status_t biff_test_wait_for_pvds_powerup(const fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t wait_time_ms)
{
    fbe_status_t status;
    fbe_u32_t i,time;
    fbe_object_id_t pvd_object_id;
    fbe_provision_drive_get_download_info_t pvd_dl_info = {0};
    const fbe_u32_t sleep_interval_ms = 100; 

    mut_printf(MUT_LOG_TEST_STATUS, "%s", __FUNCTION__);

    /* Make sure all PVDs complete powerup. 
    */
    for (i = 0; i < rg_config_p->width; i++)
    {
        status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(rg_config_p->rg_disk_set[i].bus, 
                                                                              rg_config_p->rg_disk_set[i].enclosure, 
                                                                              rg_config_p->rg_disk_set[i].slot, 
                                                                              &pvd_object_id);
        MUT_ASSERT_INTEGER_EQUAL(status, FBE_STATUS_OK);

        for (time=0; time < wait_time_ms; time += sleep_interval_ms)
        {            
            status = fbe_api_provision_drive_get_download_info(pvd_object_id, &pvd_dl_info);
            MUT_ASSERT_INTEGER_EQUAL(status, FBE_STATUS_OK);

            if (pvd_dl_info.local_state == 0){
                break;  /* we have powered up. */
            }
            fbe_api_sleep(sleep_interval_ms);
        }

        if (pvd_dl_info.local_state != 0 )
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }        
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * biff_verify_default_non_eq_policy()
 ****************************************************************
 * @brief
 *  This verifies that SP has correct default policy for handling
 *  drives that don't support Enhanced Queuing.
 *  
 * @param void
 *
 * @author
 *  12/24/2013 - Wayne Garrett - Created.
 *
 ****************************************************************/
void biff_verify_default_non_eq_policy(void)
{
    fbe_dcs_tunable_params_t params = {0};
    fbe_drive_mgmt_policy_t policy;
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s", __FUNCTION__);

    /* Verify DMO policy is Disabled and DCS Policy is Enabled.
    */        
    /*DMO*/
    policy.id = FBE_DRIVE_MGMT_POLICY_VERIFY_ENHANCED_QUEUING;
    status = fbe_api_esp_drive_mgmt_get_policy(&policy);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(policy.value, FBE_FALSE);
    /*DCS*/
    status = fbe_api_drive_configuration_interface_get_params(&params);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL((params.control_flags & FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES), FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES);

}

/*!**************************************************************
 * biff_test_non_eq_drive_upgrade_with_dcs_policy()
 ****************************************************************
 * @brief
 *  Test upgrading drives that are logically offline because
 *  they do not support EQ.  The test does the following...
 *    1. Verify SSD that don't support EQ come online.  Not a requirement for SSDs.
 *    2. Test HDD drives that don't support EQ are logically offline
 *    3. Verify fw upgrade with fw that supports EQ will bring drives back online.
 *  
 * @param void
 *
 * @author
 *  12/24/2013 - Wayne Garrett - Created.
 *
 ****************************************************************/
void biff_test_non_eq_drive_upgrade_with_dcs_policy(void)
{  
    fbe_terminator_sas_drive_type_default_info_t drive_page_info = {0};    
    fbe_sas_drive_type_t drive_type = FBE_SAS_DRIVE_INVALID;
    fbe_physical_drive_attributes_t attributes = {0};
    fbe_object_id_t pdo,pvd;
    fbe_status_t status;  
    fbe_drive_selected_element_t expect_qualified[1];
    const fbe_u32_t expect_qualified_count = 1;
    fbe_scheduler_debug_hook_t  powercycle_hook;
    fbe_u8_t tla[FBE_SCSI_INQUIRY_TLA_SIZE] = "TLATLATLATLA";
    fbe_u8_t initial_rev[FBE_SCSI_INQUIRY_REVISION_SIZE] = {"INIT"};
    fbe_u8_t new_rev[FBE_SCSI_INQUIRY_REVISION_SIZE] = {"NEW-"};    
    const fbe_u32_t bus = 0;
    const fbe_u32_t encl = 0;
    const fbe_u32_t slot = 5;
    fbe_drive_configuration_control_fw_info_t fw_info={0};
    fbe_drive_selected_element_t *drive = NULL;
    fbe_drive_configuration_download_get_process_info_t dl_status;
    fbe_u32_t retry_count;
    fbe_bool_t msg_present;
    fbe_terminator_sas_drive_fw_image_t* image_ptr = NULL;
    fbe_bool_t                    prev_dcs_policy_state;
    fbe_drive_mgmt_policy_value_t prev_dmo_policy_state;
    fbe_drive_mgmt_policy_t policy;
    fbe_dcs_tunable_params_t params = {0};
  
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test Begin ===", __FUNCTION__);
      
    /************************************************** 
       0. Initialize test
    */  
    /* initialize hook */
    powercycle_hook.monitor_state = SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE;
    powercycle_hook.monitor_substate = FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_POWERCYCLE;
    powercycle_hook.check_type = SCHEDULER_CHECK_STATES;
    powercycle_hook.action = SCHEDULER_DEBUG_ACTION_CONTINUE;
    powercycle_hook.val1 = 0;
    powercycle_hook.val2 = 0;

    /* Enable DCS to enforce policy, and enable policy in DCS
    */        
    fbe_api_drive_configuration_interface_get_params(&params);
    prev_dcs_policy_state = (params.control_flags & FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES)?FBE_TRUE:FBE_FALSE;
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES, FBE_TRUE /*enable*/);
    
    policy.id = FBE_DRIVE_MGMT_POLICY_VERIFY_ENHANCED_QUEUING;

    fbe_api_esp_drive_mgmt_get_policy(&policy);
    prev_dmo_policy_state = policy.value;
    policy.value =  FBE_DRIVE_MGMT_POLICY_OFF;
    fbe_api_esp_drive_mgmt_change_policy(&policy);


    /************************************************** 
       1. Verify flash drives come ready with EQ disabled.
    */  
    drive_type =  FBE_SAS_DRIVE_SIM_520_FLASH_HE;
    fbe_api_terminator_sas_drive_get_default_page_info(drive_type, &drive_page_info);    
    drive_page_info.vpd_inquiry_f3[7] = 0x0;   /* disable EQ */

    biff_insert_new_drive(drive_type, &drive_page_info, bus, encl, slot);

    biff_verify_drive_is_logically_online(bus, encl, slot);
    
    biff_remove_drive(bus, encl, slot, FBE_TRUE);


    /************************************************** 
       2. Verify that SAS HDD drives with EQ disabled are brought up Logically Offline
    */                  
    drive_type =  FBE_SAS_NL_DRIVE_SIM_520;
    fbe_api_clear_event_log(FBE_PACKAGE_NOTIFICATION_ID_ESP);

    fbe_api_terminator_sas_drive_get_default_page_info(drive_type, &drive_page_info);    
    drive_page_info.vpd_inquiry_f3[7] = 0x0;   /* disable EQ */
    fbe_copy_memory(&drive_page_info.inquiry[FBE_SCSI_INQUIRY_TLA_OFFSET], tla, sizeof(tla));  /* change tla */
    fbe_copy_memory(&drive_page_info.inquiry[FBE_SCSI_INQUIRY_REVISION_OFFSET], initial_rev, sizeof(initial_rev));  /* change rev */
    
    biff_insert_new_drive(drive_type, &drive_page_info, bus, encl, slot);

    status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 10000 /* ms */);                                                
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_physical_drive_get_attributes(pdo, &attributes);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(attributes.maintenance_mode_bitmap, FBE_PDO_MAINTENANCE_FLAG_EQ_NOT_SUPPORTED, "PDO not in maintenance mode");

    /* verify pvd not connected */        
    status = fbe_api_provision_drive_get_obj_id_by_location(bus, encl, slot, &pvd);        
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_wait_for_block_edge_path_state(pvd, 0, FBE_PATH_STATE_INVALID /*not connected*/, FBE_PACKAGE_ID_SEP_0, 10000/*ms*/);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* verify logical offline event log */
    sleeping_beauty_verify_event_log_drive_offline(bus, encl, slot, NULL, DMO_DRIVE_OFFLINE_REASON_NON_EQ);

    /************************************************** 
       3.1  Send trial run and verify Logical Offine drive is qualified
    */
    fbe_api_clear_event_log(FBE_PACKAGE_NOTIFICATION_ID_ESP);

    expect_qualified[0].bus = bus;
    expect_qualified[0].enclosure = encl;
    expect_qualified[0].slot = slot;

    biff_send_trial_run_verify_expected(tla, sizeof(tla), expect_qualified, expect_qualified_count);        


    /************************************************** 
       3.2  Upgrade failed drive and verify it's online
    */        
    fbe_api_clear_event_log(FBE_PACKAGE_NOTIFICATION_ID_ESP);

    /* setup debug hook */
    powercycle_hook.object_id = pdo;  
    status = fbe_api_scheduler_add_debug_hook_pp(powercycle_hook.object_id,
                                                 powercycle_hook.monitor_state,
                                                 powercycle_hook.monitor_substate,
                                                 powercycle_hook.val1, 
                                                 powercycle_hook.val2, 
                                                 powercycle_hook.check_type, 
                                                 powercycle_hook.action);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    

    sir_topham_hatt_create_fw(drive_type, &fw_info);

    /* modify tla & rev in image*/
    image_ptr = (fbe_terminator_sas_drive_fw_image_t*)fw_info.data_image_p;                
    fbe_copy_memory(&image_ptr->inquiry[FBE_SCSI_INQUIRY_TLA_OFFSET], tla, sizeof(tla));        
    fbe_copy_memory(&image_ptr->inquiry[FBE_SCSI_INQUIRY_REVISION_OFFSET], new_rev, sizeof(new_rev));

    /* Select drives */
    fw_info.header_image_p->num_drives_selected = 1;
    fw_info.header_image_p->cflags |= FBE_FDF_CFLAGS_SELECT_DRIVE;
    drive = &fw_info.header_image_p->first_drive;
    drive->bus = bus;
    drive->enclosure = encl;
    drive->slot = slot;
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Sending download to %d_%d_%d\n", __FUNCTION__, drive->bus, drive->enclosure, drive->slot);

    /* Send download to DCS */
    status = fbe_api_drive_configuration_interface_download_firmware(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check download process started */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_RUNNING, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Checking event log for download started message.\n", __FUNCTION__);
    for (retry_count = 0; retry_count < BIFF_EVENT_LOG_CHECK_COUNT; retry_count++){
        msg_present = FBE_FALSE;
        status = sir_topham_hatt_check_fw_download_event_log(&fw_info, PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_STARTED, 
                                                            0,&msg_present);
        if ((status == FBE_STATUS_OK) && msg_present)
        {
            break;
        }
        fbe_api_sleep(BIFF_EVENT_LOG_CHECK_SLEEP_INTERVAL);
    }

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(msg_present);

    /* Check download process finished.   */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, 
                                                         FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL, 
                                                         30+60); /* 30 for dl, 60 for powercycle */
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE, dl_status.fail_code);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Checking event log for download completed message.\n", __FUNCTION__);
    for (retry_count = 0; retry_count < BIFF_EVENT_LOG_CHECK_COUNT; retry_count++){
        msg_present = FBE_FALSE;
        status = sir_topham_hatt_check_fw_download_event_log(&fw_info, PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_COMPLETED, 
                                                             0,&msg_present);
        if ((status == FBE_STATUS_OK) && msg_present)
        {
            break;
        }
        fbe_api_sleep(BIFF_EVENT_LOG_CHECK_SLEEP_INTERVAL);
    }

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(msg_present);    
                
    sir_topham_hatt_destroy_fw(&fw_info);                

    /* verify online */
    biff_verify_drive_is_logically_online(bus, encl, slot);

    /* verify powercycle occured */      
    status = fbe_api_scheduler_get_debug_hook_pp(&powercycle_hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    MUT_ASSERT_INTEGER_NOT_EQUAL(powercycle_hook.counter, 0);
    

    /************************************************** 
       4.  Cleanup 
    */
    status = fbe_api_scheduler_del_debug_hook_pp(pdo,
                                                 SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE,
                                                 FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_POWERCYCLE,
                                                 0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    biff_remove_drive(bus, encl, slot, FBE_TRUE);   

    /* restore policies */         
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES, prev_dcs_policy_state);

    policy.id = FBE_DRIVE_MGMT_POLICY_VERIFY_ENHANCED_QUEUING; 
    policy.value = prev_dmo_policy_state;
    fbe_api_esp_drive_mgmt_change_policy(&policy);


    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test End ===\n", __FUNCTION__);
}

/*!**************************************************************
 * biff_test_non_eq_drive_with_dmo_policy()
 ****************************************************************
 * @brief
 *  Test upgrading drives that are logically offline because
 *  they do not support EQ.  The test does the following...
 *    1. Verify SSD that don't support EQ come online.  Not a requirement for SSDs.
 *    2. Test HDD drives that don't support EQ are failed by DMO
 *  
 * @param void
 *
 * @author
 *  12/24/2013 - Wayne Garrett - Created.
 *
 ****************************************************************/
void biff_test_non_eq_drive_with_dmo_policy(void)
{  
    fbe_terminator_sas_drive_type_default_info_t drive_page_info = {0};    
    fbe_sas_drive_type_t drive_type = FBE_SAS_DRIVE_INVALID;
    fbe_physical_drive_attributes_t attributes = {0};
    fbe_object_id_t pdo;
    fbe_status_t status;      
    fbe_u8_t tla[FBE_SCSI_INQUIRY_TLA_SIZE] = "TLATLATLATLA";
    fbe_u8_t initial_rev[FBE_SCSI_INQUIRY_REVISION_SIZE] = {"INIT"};    
    const fbe_u32_t bus = 0;
    const fbe_u32_t encl = 0;
    const fbe_u32_t slot = 5;           
    fbe_dcs_tunable_params_t params = {0};
    fbe_bool_t                    prev_dcs_policy_state;
    fbe_drive_mgmt_policy_value_t prev_dmo_policy_state;
    fbe_drive_mgmt_policy_t policy;
  
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test Begin ===", __FUNCTION__);
      
    /* Disable DCS from enforcing policy, and enable policy in DMO
    */        
    fbe_api_drive_configuration_interface_get_params(&params);
    prev_dcs_policy_state = (params.control_flags & FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES)?FBE_TRUE:FBE_FALSE;
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES, FBE_FALSE /*disable*/);
    
    policy.id = FBE_DRIVE_MGMT_POLICY_VERIFY_ENHANCED_QUEUING;
    fbe_api_esp_drive_mgmt_get_policy(&policy);
    prev_dmo_policy_state = policy.value;
    policy.value =  FBE_DRIVE_MGMT_POLICY_ON;
    fbe_api_esp_drive_mgmt_change_policy(&policy);
          

    /************************************************** 
       1. Verify flash drives come ready with EQ disabled.
    */  
    drive_type =  FBE_SAS_DRIVE_SIM_520_FLASH_HE;
    fbe_api_terminator_sas_drive_get_default_page_info(drive_type, &drive_page_info);    
    drive_page_info.vpd_inquiry_f3[7] = 0x0;   /* disable EQ */

    biff_insert_new_drive(drive_type, &drive_page_info, bus, encl, slot);

    biff_verify_drive_is_logically_online(bus, encl, slot);
    
    biff_remove_drive(bus, encl, slot, FBE_TRUE);


    /************************************************** 
       2. Verify that SAS HDD drives with EQ disabled are brought up Logically Offline
    */                  
    drive_type =  FBE_SAS_NL_DRIVE_SIM_520;
    fbe_api_clear_event_log(FBE_PACKAGE_NOTIFICATION_ID_ESP);

    fbe_api_terminator_sas_drive_get_default_page_info(drive_type, &drive_page_info);    
    drive_page_info.vpd_inquiry_f3[7] = 0x0;   /* disable EQ */
    fbe_copy_memory(&drive_page_info.inquiry[FBE_SCSI_INQUIRY_TLA_OFFSET], tla, sizeof(tla));  /* change tla */
    fbe_copy_memory(&drive_page_info.inquiry[FBE_SCSI_INQUIRY_REVISION_OFFSET], initial_rev, sizeof(initial_rev));  /* change rev */
    
    biff_insert_new_drive(drive_type, &drive_page_info, bus, encl, slot);

    status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_FAIL, 10000 /* ms */);                                                
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* verify PDO not in maintenance mode since PDO policy is disabled */
    status = fbe_api_physical_drive_get_attributes(pdo, &attributes);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(attributes.maintenance_mode_bitmap, 0, "PDO in maintenance mode");

    /* verify logical offline event log */
    sleeping_beauty_verify_event_log_drive_offline(bus, encl, slot, NULL, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENHANCED_QUEUING_NOT_SUPPORTED); 

    /************************************************** 
       4.  Cleanup 
    */
    biff_remove_drive(bus, encl, slot, FBE_TRUE);  
    
    /* restore policies */         
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES, prev_dcs_policy_state);

    policy.id = FBE_DRIVE_MGMT_POLICY_VERIFY_ENHANCED_QUEUING;    
    policy.value = prev_dmo_policy_state;

    fbe_api_esp_drive_mgmt_change_policy(&policy);


    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test End ===\n", __FUNCTION__);
}


/*!**************************************************************
 * biff_verify_drive_is_logically_online()
 ****************************************************************
 * @brief
 *  Verify that drive is logically Online, i.e. PVD has connected
 *  to it.
 *  
 * @param bus, encl, slot - drive location.
 *
 * @author
 *  12/24/2013 - Wayne Garrett - Created.
 *
 ****************************************************************/
void biff_verify_drive_is_logically_online(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_status_t status;
    fbe_object_id_t pdo, pvd;
    fbe_physical_drive_attributes_t attributes;    

    status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 10000 /* ms */);                                                
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    biff_wait_for_pdo_clients(pdo, 10000 /*ms*/);

    status = fbe_api_physical_drive_get_attributes(pdo, &attributes);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(attributes.maintenance_mode_bitmap, 0, "PDO in maintenance mode");

    /* verify pvd connected */ 
    status = biff_get_pvd_connected_to_pdo(pdo, &pvd, 10000 /*ms*/);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "pvd not found");

    status = fbe_api_wait_for_object_lifecycle_state(pvd, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_wait_for_block_edge_path_state(pvd, 0, FBE_PATH_STATE_ENABLED, FBE_PACKAGE_ID_SEP_0, 10000/*ms*/);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* verify event log */
    sleeping_beauty_verify_event_log_drive_online(bus, encl, slot);    
}


/*!**************************************************************
 * biff_wait_for_pdo_clients()
 ****************************************************************
 * @brief
 *  Wait for a client to connect to PDO.  Only client is PVD and
 *  there is only one connection.
 *  
 * @param bus, encl, slot - drive location.
 * @param wait_time_ms - msec to wait for connection.
 *
 * @author
 *  12/24/2013 - Wayne Garrett - Created.
 *
 ****************************************************************/
void biff_wait_for_pdo_clients(fbe_object_id_t pdo, fbe_u32_t wait_time_ms)
{
    fbe_status_t status;
    fbe_physical_drive_attributes_t attributes;
    const fbe_u32_t sleep_interval_ms = 200;
    fbe_u32_t time;

    for (time = 0;  time < wait_time_ms; time += sleep_interval_ms)
    {    
        status = fbe_api_physical_drive_get_attributes(pdo, &attributes);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if (attributes.server_info.number_of_clients != 0)
        {
            break;
        }
       
        fbe_api_sleep(sleep_interval_ms);       
    }

    MUT_ASSERT_TRUE( time < wait_time_ms );
}

/*!**************************************************************
 * biff_get_pvd_connected_to_pdo()
 ****************************************************************
 * @brief
 *  Find and return the pvd that is connected to the given PDO.
 *  It will continue to search for the PVD until timeout_ms
 *  expires.
 *  
 * @param pdo - PDO object ID
 * @param pvd - Provision Drive object ID that's returned.  FBE_OBJECT_ID_INVALID
 *              is returned if no match is found.
 * @param timeout_ms - msec to search for PVD.
 *
 * @author
 *  12/23/2013 : Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_status_t biff_get_pvd_connected_to_pdo(fbe_object_id_t pdo, fbe_object_id_t *pvd, fbe_u32_t timeout_ms)
{
    fbe_status_t status;
    fbe_object_id_t *pvd_list = NULL;
    fbe_u32_t pvd_count = 0;
    fbe_u32_t i, time;
    fbe_api_get_block_edge_info_t block_edge_info;
    const fbe_u32_t sleep_interval = 500; /*ms*/

    *pvd = FBE_OBJECT_ID_INVALID;

    for (time = 0; time <= timeout_ms; time += sleep_interval)
    {        
        status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE, FBE_PACKAGE_ID_SEP_0, &pvd_list, &pvd_count);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        for (i = 0; i < pvd_count; i++) {
            
            status = fbe_api_get_block_edge_info(pvd_list[i], 0, &block_edge_info, FBE_PACKAGE_ID_SEP_0);
            if (status != FBE_STATUS_OK)
            {
                continue;
            }
    
            if (block_edge_info.server_id == pdo)
            {
                *pvd = pvd_list[i];
                break;
            }
    
        }    
        fbe_api_free_memory(pvd_list);
    
        if (*pvd != FBE_OBJECT_ID_INVALID)
        {
            break;
        }

        if (time < timeout_ms)
        {
            fbe_api_sleep(sleep_interval);
        }
    }

    if (*pvd == FBE_OBJECT_ID_INVALID)
    {
        return FBE_STATUS_NO_OBJECT;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * biff_insert_new_drive()
 ****************************************************************
 * @brief
 *  Insert a new drive (i.e. a drive that has never been inserted),
 *  This function will generated a unique SAS Address and SN.  
 *  
 * @param drive_type - Type of drive to insert
 * @param new_page_info_p - innialize drive with these pages 
 * @param bus, encl, slot - drive location
 *
 * @author
 *  12/23/2013 : Wayne Garrett - Created.
 *
 ****************************************************************/
void biff_insert_new_drive(fbe_sas_drive_type_t drive_type, fbe_terminator_sas_drive_type_default_info_t *new_page_info_p, fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_sim_transport_connection_target_t this_sp, other_sp;
    fbe_sas_address_t sas_addr = FBE_U64_MAX;   /* generate new unique addr */

    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 
                                                      
    biff_insert_new_drive_for_sp(drive_type, new_page_info_p, bus,encl,slot, &sas_addr, this_sp);
                               
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        biff_insert_new_drive_for_sp(drive_type, new_page_info_p, bus,encl,slot, &sas_addr, other_sp);
    }    
}

void biff_insert_new_drive_for_sp(fbe_sas_drive_type_t drive_type, fbe_terminator_sas_drive_type_default_info_t *new_page_info_p, fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_sas_address_t * sas_addr, fbe_sim_transport_connection_target_t sp)
{
    fbe_status_t status;
    fbe_api_terminator_device_handle_t drive_handle;
    fbe_terminator_sas_drive_type_default_info_t prev_page_info = {0};           
    fbe_sim_transport_connection_target_t orig_sp;

    mut_printf(MUT_LOG_LOW, "%s %d_%d_%d SP%s", __FUNCTION__, bus, encl, slot, (sp==FBE_SIM_SP_A)?"A":"B");

    orig_sp = fbe_api_sim_transport_get_target_server();
    fbe_api_sim_transport_set_target_server(sp);

    fbe_api_terminator_sas_drive_get_default_page_info(drive_type, &prev_page_info);

    /* modify default page info, so new drive picks it up.  Only if not NULL */
    if (new_page_info_p != NULL)
    {
        fbe_api_terminator_sas_drive_set_default_page_info(drive_type, new_page_info_p);
    }

    /* insert drive */

    if (*sas_addr == FBE_U64_MAX)
    {
        *sas_addr = fbe_test_pp_util_get_unique_sas_address();
    }

    status = fbe_test_pp_util_insert_sas_drive_extend(bus, encl, slot, 
                                                      520, 
                                                      TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY,
                                                      *sas_addr, 
                                                      drive_type, 
                                                      &drive_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* restore default info */
    fbe_api_terminator_sas_drive_set_default_page_info(drive_type, &prev_page_info);

    fbe_api_sim_transport_set_target_server(orig_sp);
}

/*!**************************************************************
 * biff_remove_drive()
 ****************************************************************
 * @brief
 *  Removes a drive and destroys terminator device.   The same
 *  drive cannot be re-inserted again.  Must insert a new one
 *  with a unique SN.   Use biff_insert_new_drive().
 *  
 * @param bus, encl, slot - drive location
 *
 * @author
 *  12/23/2013 : Wayne Garrett - Created.
 *
 ****************************************************************/
void biff_remove_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_bool_t wait_for_destroy)
{    
    fbe_sim_transport_connection_target_t this_sp, other_sp;

    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    biff_remove_drive_for_sp(bus, encl, slot, wait_for_destroy, this_sp);

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        biff_remove_drive_for_sp(bus, encl, slot, wait_for_destroy, other_sp);
    }
}

void biff_remove_drive_for_sp(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_bool_t wait_for_destroy, fbe_sim_transport_connection_target_t sp)
{
    fbe_status_t status;
    fbe_sim_transport_connection_target_t orig_sp;

    mut_printf(MUT_LOG_LOW, "%s %d_%d_%d SP%s", __FUNCTION__, bus, encl, slot, (sp==FBE_SIM_SP_A)?"A":"B");

    orig_sp = fbe_api_sim_transport_get_target_server();
    fbe_api_sim_transport_set_target_server(sp);

    status = fbe_test_pp_util_remove_sas_drive(bus, encl, slot);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (wait_for_destroy)
    {
        /* verify objects are destroyed */
        status = sleeping_beauty_wait_for_state(bus, encl, slot, FBE_LIFECYCLE_STATE_DESTROY, 10000 /*ms*/);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    fbe_api_sim_transport_set_target_server(orig_sp);
}

/*!**************************************************************
 * biff_pull_drive()
 ****************************************************************
 * @brief
 *  pull a drive and destroys terminator device.
 *
 * @param bus, encl, slot - drive location
 *
 * @author
 *  08/25/2014 : Kang Jamin - Created.
 *
 ****************************************************************/
void biff_pull_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_api_terminator_device_handle_t *drive_handle_p)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "%s %d_%d_%d", __FUNCTION__, bus, encl, slot);
    status = fbe_test_pp_util_pull_drive(bus, encl, slot, drive_handle_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* verify objects are destroyed */
    status = sleeping_beauty_wait_for_state(bus, encl, slot, FBE_LIFECYCLE_STATE_DESTROY, 10000 /*ms*/);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * biff_reinsert_drive()
 ****************************************************************
 * @brief
 *  reinsert a drive and wait for pvd to be ready
 *
 * @param bus, encl, slot - drive location
 *
 * @author
 *  08/25/2014 : Kang Jamin - Created.
 *
 ****************************************************************/
void biff_reinsert_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_api_terminator_device_handle_t drive_handle)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "%s %d_%d_%d", __FUNCTION__, bus, encl, slot);
    status = fbe_test_pp_util_reinsert_drive(bus, encl, slot, drive_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = sleeping_beauty_wait_for_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 30000 /*ms*/);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * biff_download_test()
 ****************************************************************
 * @brief
 *  Run drive firmware download test. We support two download scenarios:
 *  DFU - rapid download to all drives at once.
 *  ODFU - the traditional type that download one at a time / per raid group.
 *  The emphasis of the test is PVD and raid group interaction. The test
 *  won't perform simulated firmware download itself. That test was done
 *  in thomas_the_tank_engine_test and sir_topham_hatt_test
 *  
 * @param rg_config_p - Raid group table.
 * 
 *
 * @author
 *  4/4/2011 - Created. Ruomu Gao
 *
 ****************************************************************/

void biff_download_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_api_rdgen_context_t                 *context_p = biff_test_contexts;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Drive download test starting. rg_id=%d type=%d width=%d ===", rg_config_p->raid_group_id, rg_config_p->raid_type, rg_config_p->width);

    biff_trial_run_test(rg_config_p);
    biff_dfu_test(rg_config_p, context_p);
    biff_odfu_test(rg_config_p, context_p);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Drive download test ended. rg_id=%d type=%d width=%d ===",  rg_config_p->raid_group_id, rg_config_p->raid_type, rg_config_p->width);

    return;
}

/*!**************************************************************
 * biff_test_rg_config()
 ****************************************************************
 * @brief
 *  Run fw download test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  5/31/2011 - Created. Rob Foley
 *
 ****************************************************************/

void biff_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    /*  Disable the recovery queue so that a spare cannot swap-in */
    fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);

    big_bird_write_background_pattern();

    /* Run drive firmware download related tests */
    biff_download_test(rg_config_p);

    /*  Enable the recovery queue at the end of the test */
    fbe_test_sep_util_enable_recovery_queue(FBE_OBJECT_ID_INVALID);

    /* Make sure we did not get any trace errors.  We do this in between each
     * raid group test since it stops the test sooner. 
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end biff_test_rg_config()
 ******************************************/

/*!**************************************************************
 * biff_test_verify_vault_status()
 ****************************************************************
 * @brief
 * Verify the download status on vault drives. Move common code from
 * 'biff_test_on_vault_drive' to this function.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  08/22/2014 - Created. Kang Jamin
 ****************************************************************/
static void biff_test_verify_vault_status(void)
{
    fbe_u32_t num_of_vaults = 4;
    fbe_u32_t i;
    fbe_drive_configuration_download_get_process_info_t dl_status;
    fbe_status_t status;
    fbe_drive_selected_element_t selected_drive;
    fbe_provision_drive_get_download_info_t pvd_dl_info;
    fbe_object_id_t pvd_object_id;
    fbe_u32_t num_in_dl;
    /* Wait download process till done */
    mut_printf(MUT_LOG_TEST_STATUS,"%s: Wait download status to successful\n", __FUNCTION__);
    for (i = 0; i < BIFF_DOWNLOAD_SECONDS_PER_DRIVE * num_of_vaults; i++) {
        fbe_api_sleep(1000);
        status = fbe_api_drive_configuration_interface_get_download_process(&dl_status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        if (dl_status.state == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL) {
            break;
        }
        /* Check only one drive could be in download at a time */
        num_in_dl = 0;
        for (i = 0; i < num_of_vaults; i++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(0, 0, i, &pvd_object_id);
            status = fbe_api_provision_drive_get_download_info(pvd_object_id, &pvd_dl_info);
            if (status == FBE_STATUS_OK && pvd_dl_info.local_state != 0)
            {
                num_in_dl++;
            }
        }
        MUT_ASSERT_TRUE(num_in_dl <= 1);
    }
    MUT_ASSERT_TRUE(dl_status.state == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);

    /* Check all drives done download */
    mut_printf(MUT_LOG_TEST_STATUS,"%s: Check all PDOs are complete\n", __FUNCTION__);
    for (i = 0; i < num_of_vaults; i++) {
        mut_printf(MUT_LOG_TEST_STATUS,"    Check PDO %u is complete\n", i);
        selected_drive.bus = 0;
        selected_drive.enclosure = 0;
        selected_drive.slot = i;
        sir_topham_hatt_check_drive_download_status(&selected_drive,
                                                    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_COMPLETE,
                                                    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_NO_FAILURE);
    }

    /* Check all PDOs in ready state */
    mut_printf(MUT_LOG_TEST_STATUS,"%s: Check all PDOs are ready\n", __FUNCTION__);
    for (i = 0; i < num_of_vaults; i++) {
        status = fbe_test_pp_util_verify_pdo_state(0, 0, i,
                                                   FBE_LIFECYCLE_STATE_READY,
                                                   5000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Check all PDOs in ready state in SPB */
    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        mut_printf(MUT_LOG_TEST_STATUS,"%s: Check all PDOs on SPB are ready\n", __FUNCTION__);
        for (i = 0; i < num_of_vaults; i++)
        {
            status = fbe_test_pp_util_verify_pdo_state(0, 0, i,
                                                       FBE_LIFECYCLE_STATE_READY,
                                                       5000);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
}

/*!**************************************************************
 * biff_test_misorder_vault_drives()
 ****************************************************************
 * @brief
 *  pull and reinsert vault drives to let 0_0_0 has bigger pdo object
 *  id than 0_0_3
 *  
 * @param None.
 * 
 * @return None.   
 *
 * @author
 *  08/25/2014 - Created. Kang Jamin
 *
 ****************************************************************/
static void biff_test_misorder_vault_drives(void)
{
    fbe_api_terminator_device_handle_t drive_handle_0_0_0;
    fbe_api_terminator_device_handle_t drive_handle_0_0_3;
    fbe_object_id_t pdo_object_id_0_0_0;
    fbe_object_id_t pdo_object_id_0_0_3;
    fbe_status_t status;
    fbe_object_id_t pvd_object_id_0_0_0;
    fbe_object_id_t pvd_object_id_0_0_3;


    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(0, 0, 0, &pvd_object_id_0_0_0);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(0, 0, 3, &pvd_object_id_0_0_3);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    biff_pull_drive(0, 0, 0, &drive_handle_0_0_0);
    biff_pull_drive(0, 0, 3, &drive_handle_0_0_3);
    biff_reinsert_drive(0, 0, 3, drive_handle_0_0_3);
    biff_reinsert_drive(0, 0, 0, drive_handle_0_0_0);
    fbe_api_get_physical_drive_object_id_by_location(0, 0, 0, &pdo_object_id_0_0_0);
    fbe_api_get_physical_drive_object_id_by_location(0, 0, 3, &pdo_object_id_0_0_3);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: object id: 0_0_0: %#x, 0_0_3: %#x\n",
               __FUNCTION__, pdo_object_id_0_0_0, pdo_object_id_0_0_3);
    MUT_ASSERT_TRUE(pdo_object_id_0_0_0 > pdo_object_id_0_0_3);
    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id_0_0_0, FBE_LIFECYCLE_STATE_READY,
                                                     30000, FBE_PACKAGE_ID_SEP_0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id_0_0_3, FBE_LIFECYCLE_STATE_READY,
                                                     30000, FBE_PACKAGE_ID_SEP_0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * biff_test_on_vault_drives()
 ****************************************************************
 * @brief
 *  Run drive firmware download test on vautl drives.
 *  
 * @param None.
 * 
 * @return None.   
 *
 * @author
 *  12/05/2011 - Created. chenl6
 *
 ****************************************************************/

void biff_test_on_vault_drives(void)
{
    fbe_drive_selected_element_t *drive;
    fbe_status_t status;
    fbe_drive_configuration_control_fw_info_t fw_info;
    fbe_drive_configuration_download_get_process_info_t dl_status;
    fbe_u32_t i, num_of_vaults = 4;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t pvd_object_id;


    biff_test_misorder_vault_drives();


    /* Kick off the creation of raid group with Logical unit configuration. */
    fbe_test_sep_util_fill_raid_group_disk_set(&biff_vault_raid_group_config, 1, biff_vault_disk_set_520, NULL);
    fbe_test_sep_util_fill_raid_group_lun_configuration(&biff_vault_raid_group_config,
                                                        1,   
                                                        0,         /* first lun number for this RAID group. */
                                                        1,
                                                        0x6000);
    fbe_test_sep_util_create_raid_group_configuration(&biff_vault_raid_group_config, 1);
    /* Wait RAID object to be ready on SPA */
    fbe_api_database_lookup_raid_group_by_number(biff_vault_raid_group_config.raid_group_id, &rg_object_id);
    fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_SEP_0);

    /* Wait RAID object to be ready on SPB */
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_api_database_lookup_raid_group_by_number(biff_vault_raid_group_config.raid_group_id, &rg_object_id);
        fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_SEP_0);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    mut_printf(MUT_LOG_TEST_STATUS,"biff_test: downloading to vault drives\n");
    sir_topham_hatt_create_fw(FBE_SAS_DRIVE_SIM_520, &fw_info);
    /* Select drives */
    fw_info.header_image_p->num_drives_selected = num_of_vaults;
    fw_info.header_image_p->cflags |= FBE_FDF_CFLAGS_SELECT_DRIVE;
    drive = &fw_info.header_image_p->first_drive;
    for (i = 0; i < num_of_vaults; i++)
    {
        drive->bus = 0;
        drive->enclosure = 0;
        drive->slot = i;
        drive++;
    }

    /* 0_0_0 is the last drive for download */
    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(0, 0, 0, &pvd_object_id);
    status = fbe_api_scheduler_add_debug_hook(pvd_object_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_DOWNLOAD,
                                              FBE_PROVISION_DRIVE_SUBSTATE_DOWNLOAD_DONE,
                                              0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Test case 1:
     *     AR663334
     *     Add debug hook to let last PVD (0_0_0) stuck in download done state,
     *     and then start another download.
     *
     * Test case 2:
     *     AR662278
     *     A race condition when sending download to multiple pdos at the same time.
     */
    mut_printf(MUT_LOG_TEST_STATUS,"biff_test: Start first download\n");

    /* Send download to DCS */
    status = fbe_api_drive_configuration_interface_download_firmware(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check download process started */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_RUNNING, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);
    
    {
        fbe_u32_t timeout;

        timeout = fbe_test_get_timeout_ms();
        fbe_test_set_timeout_ms(BIFF_DOWNLOAD_SECONDS_PER_DRIVE * 1000 * num_of_vaults);
        mut_printf(MUT_LOG_TEST_STATUS,"    Wait for download hook for %u, vaults: %u\n",
                   pvd_object_id, num_of_vaults);
        status = fbe_test_wait_for_debug_hook(pvd_object_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_DOWNLOAD,
                                              FBE_PROVISION_DRIVE_SUBSTATE_DOWNLOAD_DONE,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE,
                                              0, 0);
        fbe_test_set_timeout_ms(timeout);
    }

    /* Wait download process till done */
    biff_test_verify_vault_status();

    mut_printf(MUT_LOG_TEST_STATUS,"biff_test: Start second download\n");
    /* Send download to DCS */
    status = fbe_api_drive_configuration_interface_download_firmware(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check download process started */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_RUNNING, 10);

    fbe_api_sleep(5); /* Wait for path attribute change event */
    fbe_api_scheduler_del_debug_hook(pvd_object_id,
                                     SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_DOWNLOAD,
                                     FBE_PROVISION_DRIVE_SUBSTATE_DOWNLOAD_DONE,
                                     0, 0,
                                     SCHEDULER_CHECK_STATES,
                                     SCHEDULER_DEBUG_ACTION_PAUSE);

    /* Wait download process till done */
    biff_test_verify_vault_status();

    /* Clean up */
    sir_topham_hatt_destroy_fw(&fw_info);

    fbe_api_sleep(3000);
}
/******************************************
 * end biff_test_on_vault_drives()
 ******************************************/


/*!**************************************************************
 * biff_test_on_degraded_rg()
 ****************************************************************
 * @brief
 *  Run drive firmware download test on degraded RAID group.
 *  The download should resume after rebuild.
 *  
 * @param None.
 * 
 * @return None.   
 *
 * @author
 *  12/06/2011 - Created. chenl6
 *
 ****************************************************************/

void biff_test_on_degraded_rg(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_u32_t drives_to_remove[] = {0, FBE_U32_MAX};
    fbe_drive_configuration_control_fw_info_t fw_info;
    fbe_drive_configuration_download_get_process_info_t dl_status;
    fbe_u32_t i;
    fbe_drive_selected_element_t *drive, selected_drive;
    fbe_test_raid_group_disk_set_t *drive_to_download_p;
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS,"biff_test: downloading to degraded RG\n");
    /*  Disable the recovery queue so that a spare cannot swap-in */
    fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);

    big_bird_write_background_pattern();

    fbe_test_set_specific_drives_to_remove(rg_config_p, drives_to_remove);
    big_bird_remove_drives(rg_config_p, 
                           1, 
                           FBE_FALSE,    /* don't fail pvd */
                           FBE_TRUE,
                           FBE_DRIVE_REMOVAL_MODE_SPECIFIC);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s removing drive Completed. ==", __FUNCTION__);

    fbe_api_sleep(1000);

    /* first verify trial run returns non_qual for removed drives 
    */
    biff_trial_run_test(rg_config_p);

    /* now test download 
    */
    sir_topham_hatt_create_fw(FBE_SAS_DRIVE_SIM_520, &fw_info);
    /* Select drives */
    fw_info.header_image_p->num_drives_selected = rg_config_p->width - 1;
    fw_info.header_image_p->cflags |= FBE_FDF_CFLAGS_SELECT_DRIVE;
    drive = &fw_info.header_image_p->first_drive;
    for (i = 1; i < rg_config_p->width; i++)
    {
        drive_to_download_p = &rg_config_p->rg_disk_set[i];
        drive->bus = drive_to_download_p->bus;
        drive->enclosure = drive_to_download_p->enclosure;
        drive->slot = drive_to_download_p->slot;
        drive++;
    }

    /* Send download to DCS */
    status = fbe_api_drive_configuration_interface_download_firmware(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check download process started */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_RUNNING, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);

    /* Sleep 2 seconds before insert the drive */
    fbe_api_sleep(2000);

    /* Put the drives back in */
    big_bird_insert_drives(rg_config_p, 
                           1, 
                           FBE_FALSE);    /* don't fail pvd */

    mut_printf(MUT_LOG_TEST_STATUS, "== %s inserting drive Complete. ==", __FUNCTION__);

    /* Make sure all objects are ready. */
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* Wait for the rebuilds to finish */
    big_bird_wait_for_rebuilds(rg_config_p, 1, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);

    /* Check download process to finish */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, 
                                                         FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL, 
                                                         BIFF_DOWNLOAD_SECONDS_PER_DRIVE * fw_info.header_image_p->num_drives_selected);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);

    /* Check all drives done download */
    for (i = 1; i < rg_config_p->width; i++)
    {
        drive_to_download_p = &rg_config_p->rg_disk_set[i];
        selected_drive.bus = drive_to_download_p->bus;
        selected_drive.enclosure = drive_to_download_p->enclosure;
        selected_drive.slot = drive_to_download_p->slot;
        sir_topham_hatt_check_drive_download_status(&selected_drive,
                                                    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_COMPLETE,
                                                    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_NO_FAILURE);
    }

    /*  Enable the recovery queue at the end of the test */
    fbe_test_sep_util_enable_recovery_queue(FBE_OBJECT_ID_INVALID);

    return;
}
/******************************************
 * end biff_test_on_degraded_rg()
 ******************************************/

/*!**************************************************************
 * biff_odfu_test_with_special_condition()
 ****************************************************************
 * @brief
 *  Run drive firmware download test on degraded RAID group.
 *  And set up hook in pvd state machine, verify that object 
 *. drive object destroys properly.
 *  
 * @param None.
 * 
 * @return None.   
 *
 * @author
 *  11/11/2013 - Created. DP
 *
 ****************************************************************/
void biff_odfu_test_with_special_condition(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_drive_configuration_control_fw_info_t fw_info = {0};
    fbe_drive_configuration_download_get_process_info_t dl_status = {0};
    fbe_drive_selected_element_t *drive = NULL;
    fbe_drive_selected_element_t selected_drive = {0};
    fbe_test_raid_group_disk_set_t *drive_to_download_p = NULL;
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_object_id_t pvd_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t i = 0;

    mut_printf(MUT_LOG_TEST_STATUS,"biff_test: odfu_test_with_special_condition\n");
    /*  Disable the recovery queue so that a spare cannot swap-in */
    fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);

    big_bird_write_background_pattern();

    /* first verify trial run returns non_qual for removed drives 
    */
    biff_trial_run_test(rg_config_p);

    /* now test download 
    */
    sir_topham_hatt_create_fw(FBE_SAS_DRIVE_SIM_520, &fw_info);
    
    /* Select drives */
    fw_info.header_image_p->num_drives_selected = rg_config_p->width - 1;
    fw_info.header_image_p->cflags |= FBE_FDF_CFLAGS_SELECT_DRIVE;
    drive = &fw_info.header_image_p->first_drive;
    for (i = 1; i < rg_config_p->width; i++)
    {
        drive_to_download_p = &rg_config_p->rg_disk_set[i];
        drive->bus = drive_to_download_p->bus;
        drive->enclosure = drive_to_download_p->enclosure;
        drive->slot = drive_to_download_p->slot;
        drive++;
    }

    /*R5 consist of 0_1_2, 0_1_3 and 0_1_4. Testing with removal of 
      drive 0_1_2. First get PVD object ID so we can add hook.*/
    status = fbe_api_provision_drive_get_obj_id_by_location(0,
                                                            1,
                                                            2,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*Add a hook into for this PVD. This hook will hold pvd while getting
      permission from PDO */
    status = fbe_api_scheduler_add_debug_hook(pvd_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_DOWNLOAD_PERMISSION,
                                              FBE_PROVISION_DRIVE_SUBSTATE_DOWNLOAD_PERMISSION,
                                              0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    


    /* Send download to DCS */
    status = fbe_api_drive_configuration_interface_download_firmware(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check download process started */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_RUNNING, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);

    /*Wait for 35 sec for PVD to hold state machine*/
    fbe_api_sleep(35000);

    /* Remove drive */
    status = fbe_test_pp_util_pull_drive(0, 
                                         1, 
                                         2,
                                         &rg_config_p->drive_handle[0]);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sleep(1000);

        /* Remove the hook so that we continue
     */
    status = fbe_api_scheduler_del_debug_hook(pvd_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_DOWNLOAD_PERMISSION,
                                              FBE_PROVISION_DRIVE_SUBSTATE_DOWNLOAD_PERMISSION,
                                              0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*Wait for 15 sec so PDO destroy proprely and download for this 
      drive failed */
    fbe_api_sleep(15000);

    status = fbe_test_pp_util_reinsert_drive(0, 
                                             1, 
                                             2,
                                             rg_config_p->drive_handle[0]);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Check download process to finish  Wait for 20 sec */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, 
                                                         FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL, 
                                                         20);

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);

    for (i = 1; i < rg_config_p->width; i++)
    {
        drive_to_download_p = &rg_config_p->rg_disk_set[i];
        selected_drive.bus = drive_to_download_p->bus;
        selected_drive.enclosure = drive_to_download_p->enclosure;
        selected_drive.slot = drive_to_download_p->slot;

        if((selected_drive.bus == 0)&& 
           (selected_drive.enclosure== 1) && 
           (selected_drive.slot == 2))
        {
            sir_topham_hatt_check_drive_download_status(&selected_drive,
                                                    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL,
                                                    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_NONQUALIFIED);
        }
        else
        {
            sir_topham_hatt_check_drive_download_status(&selected_drive,
                                                        FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_COMPLETE,
                                                        FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_NO_FAILURE);
        }
    }

    /*  Enable the recovery queue at the end of the test */
    fbe_test_sep_util_enable_recovery_queue(FBE_OBJECT_ID_INVALID);

    return;
}
/**********************************************
 * end biff_odfu_test_with_special_condition()
 **********************************************/
 

static void biff_run_test_case(fbe_u32_t test_case)
{
    switch(test_case)
    {
        case 1:
            biff_test_non_eq_drive_upgrade_with_dcs_policy();
            break;

        case 2:
            biff_test_non_eq_drive_with_dmo_policy();
            break;

        case 3:
            biff_run_download_with_all_rg_configs();
            break;

        default:
            mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid test case %d", __FUNCTION__, test_case);
            break;
    }
}


static void biff_run_download_with_all_rg_configs(void)
{
    fbe_u32_t                   raid_type_index;
    fbe_u32_t                   raid_group_count = 0;
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    for (raid_type_index = 0; raid_type_index < BIFF_RAID_TYPE_COUNT; raid_type_index++)
    {
        rg_config_p = &biff_raid_group_config_qual[raid_type_index][0];
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

        if (raid_type_index + 1 >= BIFF_RAID_TYPE_COUNT) {
           /* Now create the raid groups and run the tests
            */
           fbe_test_run_test_on_rg_config(rg_config_p, NULL,
                                          biff_test_rg_config,
                                          BIFF_LUNS_PER_RAID_GROUP,
                                          BIFF_CHUNKS_PER_LUN);
        }
        else {
           /* Now create the raid groups and run the tests
            */
           fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, NULL,
                                          biff_test_rg_config,
                                          BIFF_LUNS_PER_RAID_GROUP,
                                          BIFF_CHUNKS_PER_LUN,FBE_FALSE);
        }

    }    /* for all raid types. */
}

static void biff_run_all_test_cases(void)
{
    /* Test upgrading Logically Offline drive. Currently only supported in Single SP mode
       since terminator doesn't coordinate with peer when fw or mode pages are changed.  
    */
    if (!fbe_test_sep_util_get_dualsp_test_mode())
    {
        biff_verify_default_non_eq_policy(); /*Verify that we are running with expected default policies. */
        biff_test_non_eq_drive_upgrade_with_dcs_policy();  /* tests logical offline */
        biff_test_non_eq_drive_with_dmo_policy();
    }

    /* Run the error cases for all raid types supported
     */
    biff_run_download_with_all_rg_configs();

    return;
}



/*!**************************************************************
 * biff_test()
 ****************************************************************
 * @brief
 *  Run a fw download test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  5/31/2011 - Created. Rob Foley
 *
 ****************************************************************/

void biff_test(void)
{
    fbe_u32_t test_case = 0;

    if (fbe_test_sep_util_get_cmd_option_int("-run_test_case", &test_case))
    {
        biff_run_test_case(test_case);
    }
    else
    {
        biff_run_all_test_cases();
    }
}
/******************************************
 * end biff_test()
 ******************************************/

/*!**************************************************************
 * biff_setup()
 ****************************************************************
 * @brief
 *  Setup for a fw dl test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  5/31/2011 - Created. Rob Foley
 *
 ****************************************************************/
void biff_setup(void)
{
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_array_t *array_p = NULL;
        fbe_u32_t test_index;

        array_p = &biff_raid_group_config_qual[0];
        for (test_index = 0; test_index < BIFF_RAID_TYPE_COUNT; test_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&array_p[test_index][0]);
        }
        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);
        elmo_load_esp_sep_and_neit();
    }
    return;
}
/******************************************
 * end biff_setup()
 ******************************************/

/*!**************************************************************
 * biff_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the biff test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/31/2011 - Created. Rob Foley
 *
 ****************************************************************/

void biff_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_esp_neit_sep_physical();
    }
    return;
}
/******************************************
 * end biff_cleanup()
 ******************************************/


/*!**************************************************************
 * biff_dualsp_test()
 ****************************************************************
 * @brief
 *  Run a dual SP fw download test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  12/02/2011 - Created. chenl6
 *
 ****************************************************************/

void biff_dualsp_test(void)
{
    fbe_status_t status;

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Run download tests for all raid types supported
     */
    biff_run_all_test_cases();  /* single SP tests run in dual SP mode */

    status = fbe_test_sep_util_destroy_all_user_luns();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_sep_util_destroy_all_user_raid_groups();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* Run download test for degraded raid group
     */
    fbe_test_run_test_on_rg_config(&biff_raid_group_config_qual[1][0], 
                                   NULL,
                                   biff_test_on_degraded_rg,
                                   BIFF_LUNS_PER_RAID_GROUP,
                                   BIFF_CHUNKS_PER_LUN);

    status = fbe_test_sep_util_destroy_all_user_luns();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_sep_util_destroy_all_user_raid_groups();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

     /*Run the download test with condition like holding pvd state 
       machine through debug hook */
     fbe_test_run_test_on_rg_config(&biff_raid_group_config_qual[1][0], 
                                   NULL,
                                   biff_odfu_test_with_special_condition,
                                   BIFF_LUNS_PER_RAID_GROUP,
                                   BIFF_CHUNKS_PER_LUN);

    status = fbe_test_sep_util_destroy_all_user_luns();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_sep_util_destroy_all_user_raid_groups();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Run download test for vault drives
     */
    biff_test_on_vault_drives();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end biff_dualsp_test()
 ******************************************/

/*!**************************************************************
 * biff_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a dual SP fw dl test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  12/02/2011 - Created. chenl6
 *
 ****************************************************************/
void biff_dualsp_setup(void)
{
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_array_t *array_p = NULL;
        fbe_u32_t test_index;

        array_p = &biff_raid_group_config_qual[0];
        for (test_index = 0; test_index < BIFF_RAID_TYPE_COUNT; test_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&array_p[test_index][0]);
        }

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_esp_sep_and_neit_both_sps();
    }
    return;
}
/******************************************
 * end biff_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * biff_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the biff dual SP test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  12/02/2011 - Created. chenl6
 *
 ****************************************************************/

void biff_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_esp_neit_sep_physical();

        /* First execute teardown on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_esp_neit_sep_physical();
    }
    return;
}
/******************************************
 * end biff_dualsp_cleanup()
 ******************************************/

/*************************
 * end file biff_test.c
 *************************/
