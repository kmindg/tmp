/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file sir_topham_hatt_test.c
 ***************************************************************************
 *
 * @brief
 *  This file tests firmware download with interaction with SEP.
 *
 * @version
 *   03/14/2011 - Created.  Wayne Garrett
 *   05/12/2014 - Moved to SEP and cleaned up.  Wayne Garrett.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_file.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "mut.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe/fbe_drive_configuration_download.h"
#include "esp_tests.h"  
#include "pp_utils.h"  
#include "sep_tests.h"
#include "sep_utils.h"  
#include "fbe/fbe_api_event_log_interface.h"
#include "EspMessages.h"
#include "fbe_test_common_utils.h"
#include "sir_topham_hatt_test.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_event_log_api.h"
#include "PhysicalPackageMessages.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"

/*!*******************************************************************
 * @def SIR_TOPHAM_HATT_EVENT_LOG_CHECK_SLEEP_INTERVAL
 *********************************************************************
 * @brief Number of miliseconds we sleep to check firmware download
 *        event log update.
 *
 *********************************************************************/
#define SIR_TOPHAM_HATT_EVENT_LOG_CHECK_SLEEP_INTERVAL 2000

/*!*******************************************************************
 * @def SIR_TOPHAM_HATT_EVENT_LOG_CHECK_COUNT
 *********************************************************************
 * @brief Number of times we will try and query event log to check for 
 *        firmware download related logs.
 *
 *********************************************************************/
#define SIR_TOPHAM_HATT_EVENT_LOG_CHECK_COUNT 7


/*************************
 *   GLOBALS
 *************************/

char * sir_topham_hatt_short_desc = "Drive Firmware Download";
char * sir_topham_hatt_long_desc ="\
Tests the online firwmare download (ODFU) with SEP interaction\n\
\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] 1 SAS PMC port\n\
        [PP] 1 SAS VIPER enclosure\n\
        [PP] 15 SAS drives/drive\n\
        [PP] 15 logical drives/drive\n\
STEP 1: Bring up the initial topology.\n\
STEP 2: Test firmware download on unbound drives.\n\
STEP 3: Test firmware download on bound drives.\n\
STEP 4: Test firmware download with degraded RG.\n\
STEP 5: Test firmware download abort.\n\
STEP 6: fail drive during download.\n";


/*************************
 *   GLOBAL DATA
 *************************/

/*!*******************************************************************
 * @def SIR_TOPHAM_HATT_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define SIR_TOPHAM_HATT_RAID_TYPE_COUNT 1

#define BYTE_SWAP_16(p) ((((p) >> 8) & 0x00ff) | (((p) << 8) & 0xff00))

#define BYTE_SWAP_32(p) \
    ((((p) & 0xff000000) >> 24) |   \
     (((p) & 0x00ff0000) >>  8) |   \
     (((p) & 0x0000ff00) <<  8) |   \
     (((p) & 0x000000ff) << 24))

#define SIR_TOPHAM_HATT_BUS 0
#define SIR_TOPHAM_HATT_ENCL 0
#define SIR_TOPHAM_HATT_SLOT 5
#define SIR_TOPHAM_NUMBER_OF_DRIVES 3


static fbe_test_rg_configuration_t sir_topham_hatt_raid_group_config = 
    /* width,   capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {3,         0x32000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            0,          0};


/* PROTOTYPES */
static void sir_topham_hatt_test_get_all(void);
static void sir_topham_hatt_abort_download_after_delay_ms(fbe_drive_configuration_control_fw_info_t * fw_info_p, sir_topham_hatt_select_drives_t * drives_p, fbe_u32_t delay_msec);

static void sir_topham_hatt_add_debug_hook(fbe_object_id_t object_id, 
                                           fbe_u32_t state, 
                                           fbe_u32_t substate,
                                           fbe_u64_t val1,
                                           fbe_u64_t val2,
                                           fbe_u32_t check_type,
                                           fbe_u32_t action);


static void sir_topham_hatt_delete_debug_hook(fbe_object_id_t object_id, 
                                              fbe_u32_t state, 
                                              fbe_u32_t substate,
                                              fbe_u64_t val1,
                                              fbe_u64_t val2,
                                              fbe_u32_t check_type,
                                              fbe_u32_t action);

static fbe_status_t sir_topham_hatt_wait_for_target_SP_hook(fbe_object_id_t object_id, 
                                                            fbe_u32_t state, 
                                                            fbe_u32_t substate);


/*!**************************************************************
 * sir_topham_hatt_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test ESP framework
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  03/16/2011 - Created.  Wayne Garrett
 ****************************************************************/
void sir_topham_hatt_test(void)
{
    fbe_status_t status;
    fbe_drive_configuration_control_fw_info_t fw_info;
    fbe_u32_t i;
    fbe_drive_configuration_download_get_process_info_t dl_status;
    sir_topham_hatt_select_drives_t *dl_drives;
    sir_topham_hatt_select_drives_t set1, set2;
    fbe_test_discovered_drive_locations_t   drive_locations;
    fbe_test_raid_group_disk_set_t          disk_set;    
    fbe_test_raid_group_disk_set_t          rg_520_set[10];

    fbe_object_id_t pdo_object_id = 0;
    fbe_object_id_t pvd_object_id = 0;
    fbe_u32_t retry_count;
    fbe_bool_t msg_present = FBE_FALSE;    


    /* populate sets of drives for download tests */

    fbe_test_sep_util_discover_all_drive_information(&drive_locations);


    set1.num_of_drives = 3;
    for (i=0;  i < set1.num_of_drives; i++)
    {
        status = fbe_test_get_520_drive_location(&drive_locations, &disk_set);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Not enough available drives for test");

        mut_printf(MUT_LOG_TEST_STATUS, "%s set1 add %d_%d_%d", __FUNCTION__, disk_set.bus, disk_set.enclosure, disk_set.slot);
        set1.selected_drives[i].bus = disk_set.bus;
        set1.selected_drives[i].enclosure = disk_set.enclosure;
        set1.selected_drives[i].slot = disk_set.slot;
    }

    set2.num_of_drives = sir_topham_hatt_raid_group_config.width;
    for (i=0;  i < set2.num_of_drives; i++)
    {
        status = fbe_test_get_520_drive_location(&drive_locations, &disk_set);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Not enough available drives for test");

        mut_printf(MUT_LOG_TEST_STATUS, "%s set2 add %d_%d_%d", __FUNCTION__, disk_set.bus, disk_set.enclosure, disk_set.slot);
        set2.selected_drives[i].bus = disk_set.bus;
        set2.selected_drives[i].enclosure = disk_set.enclosure;
        set2.selected_drives[i].slot = disk_set.slot;

        rg_520_set[i].bus = disk_set.bus;
        rg_520_set[i].enclosure = disk_set.enclosure;
        rg_520_set[i].slot = disk_set.slot;
    }
    /* add terminator*/
    rg_520_set[i].bus = FBE_U32_MAX;
    rg_520_set[i].enclosure = FBE_U32_MAX;
    rg_520_set[i].slot = FBE_U32_MAX;    

    /* create RG */
    fbe_test_sep_util_fill_raid_group_disk_set(&sir_topham_hatt_raid_group_config, 1, rg_520_set, NULL);
    fbe_test_sep_util_fill_raid_group_lun_configuration(&sir_topham_hatt_raid_group_config,
                                                        1,   
                                                        0,         /* first lun number for this RAID group. */
                                                        1,
                                                        0x6000);
    fbe_test_sep_util_create_raid_group_configuration(&sir_topham_hatt_raid_group_config, 1);


 /****************************************************************
 * STEP 2: download fw to unbound drives
 ****************************************************************/
    mut_printf(MUT_LOG_TEST_STATUS,"Sir Topham Hatt TEST: downloading to unbound drives.\n");
    dl_drives = &set1;

    /* Create fw image */
    sir_topham_hatt_create_fw(FBE_SAS_DRIVE_SIM_520, &fw_info);
    sir_topham_hatt_select_drives(fw_info.header_image_p, dl_drives);

    /* Send download to DCS */
    status = fbe_api_drive_configuration_interface_download_firmware(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check download process started */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_RUNNING, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);

    /* Wait download process till done */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL, 20);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);

    mut_printf(MUT_LOG_TEST_STATUS, "%s sir_topham_hatt_wait_download_process_state, status = %d, fail_code = %d", __FUNCTION__, status, dl_status.fail_code);

    /* Check all drives done download */
    for (i = 0; i < dl_drives->num_of_drives; i++)
    {
        sir_topham_hatt_check_drive_download_status(&dl_drives->selected_drives[i],
                                                    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_COMPLETE,
                                                    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_NO_FAILURE);
    }

    for (i = 0; i < dl_drives->num_of_drives; i++)
    {
        status = fbe_test_pp_util_verify_pdo_state(dl_drives->selected_drives[i].bus,
                                                   dl_drives->selected_drives[i].enclosure,
                                                   dl_drives->selected_drives[i].slot,
                                                   FBE_LIFECYCLE_STATE_READY,
                                                   5000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

 /****************************************************************
 * STEP 3: download fw to bound drives
 ****************************************************************/
    mut_printf(MUT_LOG_TEST_STATUS,"Sir Topham Hatt TEST: downloading to bound drives.\n");
    dl_drives = &set2;
    sir_topham_hatt_select_drives(fw_info.header_image_p, dl_drives);
    status = fbe_api_drive_configuration_interface_download_firmware(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check download process started */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_RUNNING, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);
	
    /* Wait download process till done.  RAID will wait 30 seconds between each drive for a given RG, +30 more for margin */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL, 30*(dl_drives->num_of_drives+1));

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);

    /* Check all drives done download */
    for (i = 0; i < dl_drives->num_of_drives; i++)
    {
        sir_topham_hatt_check_drive_download_status(&dl_drives->selected_drives[i],
                                                    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_COMPLETE,
                                                    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_NO_FAILURE);
    }

    for (i = 0; i < dl_drives->num_of_drives; i++)
    {
        status = fbe_test_pp_util_verify_pdo_state(dl_drives->selected_drives[i].bus,
                                                   dl_drives->selected_drives[i].enclosure,
                                                   dl_drives->selected_drives[i].slot,
                                                   FBE_LIFECYCLE_STATE_READY,
                                                   5000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

 /****************************************************************
 * STEP 4: download fw to drives with degrade RG
 ****************************************************************/
    mut_printf(MUT_LOG_TEST_STATUS,"Sir Topham Hatt TEST: downloading drives with degraded RG.\n");

    /* Degrade the Raid Group */
    status = fbe_api_get_physical_drive_object_id_by_location(rg_520_set[0].bus,
                                                              rg_520_set[0].enclosure,
                                                              rg_520_set[0].slot,
                                                              &pdo_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(pdo_object_id, FBE_OBJECT_ID_INVALID);
    mut_printf(MUT_LOG_TEST_STATUS, "%s RG degraded pdo 0x%x",
               __FUNCTION__, pdo_object_id);

    status = fbe_api_set_object_to_state(pdo_object_id, FBE_LIFECYCLE_STATE_FAIL, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_wait_for_object_lifecycle_state(pdo_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                     60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_sleep(3000);

    /* Send download to ESP */
    dl_drives = &set2;
    sir_topham_hatt_select_drives(fw_info.header_image_p, dl_drives);
    status = fbe_api_drive_configuration_interface_download_firmware(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check download process started */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_RUNNING, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);
	
    /* Wait download process till done */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL, 20);
    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);

    /* Check all drives still in ACTIVE state */
    for (i = 0; i < dl_drives->num_of_drives; i++)
    {
        if (dl_drives->selected_drives[i].bus       == rg_520_set[0].bus &&
            dl_drives->selected_drives[i].enclosure == rg_520_set[0].enclosure &&
            dl_drives->selected_drives[i].slot      == rg_520_set[0].slot)
        {
            sir_topham_hatt_check_drive_download_status(&dl_drives->selected_drives[i],
                                                        FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL,
                                                        FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_NONQUALIFIED);
        }
        else
        {
            sir_topham_hatt_check_drive_download_status(&dl_drives->selected_drives[i],
                                                        FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_ACTIVE,
                                                        FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_NO_FAILURE);
        }
    }

 /****************************************************************
 * STEP 5: abort download, for drives downloading from STEP 4.
 ****************************************************************/
    mut_printf(MUT_LOG_TEST_STATUS,"Sir Topham Hatt TEST: aborting download.\n");
    status = fbe_api_drive_configuration_interface_abort_download();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Wait download process till done */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_ABORTED, 20);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_ABORTED);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Checking event log for download failed message.\n", __FUNCTION__);
    for (retry_count = 0; retry_count < SIR_TOPHAM_HATT_EVENT_LOG_CHECK_COUNT; retry_count++){
        msg_present = FBE_FALSE;
        status = sir_topham_hatt_check_fw_download_event_log(&fw_info, PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_ABORTED,
                                                             0 , &msg_present);
        if ((status == FBE_STATUS_OK) && msg_present)
        {
            break;
        }
        fbe_api_sleep(SIR_TOPHAM_HATT_EVENT_LOG_CHECK_SLEEP_INTERVAL);
    }

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(msg_present);

    /* Check all drives started download */
    for (i = 0; i < dl_drives->num_of_drives; i++)
    {
        if (dl_drives->selected_drives[i].bus       == rg_520_set[0].bus &&
            dl_drives->selected_drives[i].enclosure == rg_520_set[0].enclosure &&
            dl_drives->selected_drives[i].slot      == rg_520_set[0].slot)
        {
            /* this was the drive we shot at an earlier step.*/
            sir_topham_hatt_check_drive_download_status(&dl_drives->selected_drives[i],
                                                        FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL,
                                                        FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_NONQUALIFIED);
        }
        else
        {
            /* these were aborted */
            sir_topham_hatt_check_drive_download_status(&dl_drives->selected_drives[i],
                                                        FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL,
                                                        FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_ABORTED);
        }
    }

    for (i = 0; i < dl_drives->num_of_drives; i++)
    {
        if (dl_drives->selected_drives[i].bus       == rg_520_set[0].bus &&
            dl_drives->selected_drives[i].enclosure == rg_520_set[0].enclosure &&
            dl_drives->selected_drives[i].slot      == rg_520_set[0].slot)
        {
            /* this was the drive we shot at an earlier step.*/
            status = fbe_test_pp_util_verify_pdo_state(dl_drives->selected_drives[i].bus,
                                                       dl_drives->selected_drives[i].enclosure,
                                                       dl_drives->selected_drives[i].slot,
                                                       FBE_LIFECYCLE_STATE_FAIL,
                                                       5000);
        }
        else
        {
            /* make sure no other drives failed. */
            status = fbe_test_pp_util_verify_pdo_state(dl_drives->selected_drives[i].bus,
                                                       dl_drives->selected_drives[i].enclosure,
                                                       dl_drives->selected_drives[i].slot,
                                                       FBE_LIFECYCLE_STATE_READY,
                                                       5000);
        }
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /****************************************************************
    * STEP 6: abort download after delay
    ****************************************************************/

    /* restore failed drive*/
    status = fbe_api_set_object_to_state(pdo_object_id, FBE_LIFECYCLE_STATE_ACTIVATE, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_wait_for_object_lifecycle_state(pdo_object_id, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_520_set[0].bus,  rg_520_set[0].enclosure, rg_520_set[0].slot, &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    sir_topham_hatt_abort_download_after_delay_ms(&fw_info, &set2, 0);
    sir_topham_hatt_abort_download_after_delay_ms(&fw_info, &set2, 500);
    sir_topham_hatt_abort_download_after_delay_ms(&fw_info, &set2, 1000);
    sir_topham_hatt_abort_download_after_delay_ms(&fw_info, &set2, 1500);
    sir_topham_hatt_abort_download_after_delay_ms(&fw_info, &set2, 2000);

	/*test the interface that gets all drives at one*/
	sir_topham_hatt_test_get_all();

    /****************************************************************
    * STEP 7: fail drive during download
    ****************************************************************/
    mut_printf(MUT_LOG_TEST_STATUS,"Sir Topham Hatt TEST: fail drive during download.\n");
    dl_drives = &set1;

    /* Create fw image */
    sir_topham_hatt_create_fw(FBE_SAS_DRIVE_SIM_520, &fw_info);
    sir_topham_hatt_select_drives(fw_info.header_image_p, dl_drives);

    /* add hook FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_MODE_FW_DOWNLOAD_DELAY to block the firmware download usurper packet */
    for (i = 0; i < dl_drives->num_of_drives; i++)
    {
        status = fbe_api_get_physical_drive_object_id_by_location(dl_drives->selected_drives[i].bus,
                                                                  dl_drives->selected_drives[i].enclosure,
                                                                  dl_drives->selected_drives[i].slot,
                                                                  &pdo_object_id);
        
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(pdo_object_id, FBE_OBJECT_ID_INVALID);

        mut_printf(MUT_LOG_TEST_STATUS, "%s add hook FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_MODE_FW_DOWNLOAD_DELAY to pdo 0x%x", __FUNCTION__, pdo_object_id);

        sir_topham_hatt_add_debug_hook(pdo_object_id, 
                                       SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE, 
                                       FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_MODE_FW_DOWNLOAD_DELAY,
                                       NULL,
                                       NULL,
                                       SCHEDULER_CHECK_STATES,
                                       SCHEDULER_DEBUG_ACTION_PAUSE);
    }
    

    /* Send download to DCS */
    status = fbe_api_drive_configuration_interface_download_firmware(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check download process started */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_RUNNING, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);

    /* wait till hitting the hook */
    for (i = 0; i < dl_drives->num_of_drives; i++)
    {
        status = fbe_api_get_physical_drive_object_id_by_location(dl_drives->selected_drives[i].bus,
                                                                  dl_drives->selected_drives[i].enclosure,
                                                                  dl_drives->selected_drives[i].slot,
                                                                  &pdo_object_id);
        
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(pdo_object_id, FBE_OBJECT_ID_INVALID);

        sir_topham_hatt_wait_for_target_SP_hook(pdo_object_id, 
                                                SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE, 
                                                FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_MODE_FW_DOWNLOAD_DELAY);

        /* fail the drive */
        status = fbe_api_set_object_to_state(pdo_object_id, FBE_LIFECYCLE_STATE_FAIL, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_wait_for_object_lifecycle_state(pdo_object_id, FBE_LIFECYCLE_STATE_FAIL, 60000, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    

    /* Wait download process till done */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED, 20);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_NO_DRIVES_TO_UPGRADE);

    /* Check all drives done download */
    for (i = 0; i < dl_drives->num_of_drives; i++)
    {
        sir_topham_hatt_check_drive_download_status(&dl_drives->selected_drives[i],
                                                    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL,
                                                    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_NONQUALIFIED);
    }

    /* make sure PVD got to fail state */
    for (i = 0; i < dl_drives->num_of_drives; i++)
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(dl_drives->selected_drives[i].bus,
                                                                dl_drives->selected_drives[i].enclosure,
                                                                dl_drives->selected_drives[i].slot,
                                                                &pvd_object_id);
    
        
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(pvd_object_id, FBE_OBJECT_ID_INVALID);

        status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL, 10000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }


    /***************************************************************/

    /* Clean up */
    sir_topham_hatt_destroy_fw(&fw_info);

    fbe_api_sleep(3000);

    return;
}
/******************************************
 * end sir_topham_hatt_test()
 ******************************************/

/*!**************************************************************
 * sir_topham_hatt_setup() 
 ****************************************************************
 * @brief
 *  This function sets up the test environment.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  03/16/2011 - Created.  Wayne Garrett
 ****************************************************************/
void sir_topham_hatt_setup(void)
{
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_pp_util_create_physical_config_for_disk_counts(8 /*520*/,0 /*4k*/, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);
        elmo_load_sep_and_neit();
    }    
}
/******************************************
 * end sir_topham_hatt_setup()
 ******************************************/

/*!**************************************************************
 * sir_topham_hatt_destroy() 
 ****************************************************************
 * @brief
 *  This function destroys the setup.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  03/16/2011 - Created.  Wayne Garrett
 ****************************************************************/
void sir_topham_hatt_destroy(void)
{
    fbe_status_t status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_package_list_t package_list;

    fbe_zero_memory(&package_list, sizeof(package_list));
	
    package_list.number_of_packages = 3;
    package_list.package_list[0] = FBE_PACKAGE_ID_NEIT;
    package_list.package_list[1] = FBE_PACKAGE_ID_SEP_0;
	//package_list.package_list[2] = FBE_PACKAGE_ID_ESP;
    package_list.package_list[2] = FBE_PACKAGE_ID_PHYSICAL;
    fbe_test_common_util_package_destroy(&package_list);

    status = fbe_api_terminator_destroy();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "destroy terminator api failed");

    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);

    mut_printf(MUT_LOG_LOW, "=== %s completes ===\n", __FUNCTION__);
    return;
}
/******************************************
 * end sir_topham_hatt_destroy()
 ******************************************/

/*!**************************************************************
 * sir_topham_hatt_create_fw() 
 ****************************************************************
 * @brief
 *  This function creates a fw header and an image.
 *
 * @param fw_info - Pointer to fw information.               
 *
 * @return None.
 *
 * @author
 *  03/16/2011 - Created.  Wayne Garrett
 ****************************************************************/
void sir_topham_hatt_create_fw(fbe_sas_drive_type_t drive_type, fbe_drive_configuration_control_fw_info_t *fw_info)
{    
    fbe_terminator_sas_drive_fw_image_t *image_ptr;
    const fbe_u32_t image_size = sizeof(fbe_terminator_sas_drive_fw_image_t);
    fbe_fdf_header_block_t *header_ptr;
    fbe_fdf_serviceability_block_t *service_block;
    fbe_terminator_sas_drive_type_default_info_t default_info;

    header_ptr = (fbe_fdf_header_block_t *)fbe_api_allocate_memory(FBE_FDF_INSTALL_HEADER_SIZE); 
    MUT_ASSERT_NOT_NULL(header_ptr);

    /* Initialize header */
    fbe_zero_memory(header_ptr, FBE_FDF_INSTALL_HEADER_SIZE);
    header_ptr->header_marker = FBE_FDF_HEADER_MARKER;
    header_ptr->image_size = image_size;
    fbe_set_memory(header_ptr->vendor_id, 0xFF, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE);
    fbe_set_memory(header_ptr->product_id, 0xFF, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE);
    fbe_copy_memory(header_ptr->fw_revision, "E501", 4);
    header_ptr->install_header_size = FBE_FDF_INSTALL_HEADER_SIZE;
    header_ptr->trailer_offset = (FBE_FDF_INSTALL_HEADER_SIZE - sizeof(fbe_fdf_serviceability_block_t));
    header_ptr->cflags = FBE_FDF_CFLAGS_ONLINE | FBE_FDF_CFLAGS_TRAILER_PRESENT;
    header_ptr->cflags2 = FBE_FDF_CFLAGS2_CHECK_TLA;
    fbe_copy_memory(header_ptr->ofd_filename, "test.fdf", 8);
    header_ptr->num_drives_selected = 0;

    /* Initialize serviceability block */
    service_block = (fbe_fdf_serviceability_block_t *)((fbe_u8_t *)header_ptr + header_ptr->trailer_offset);
    service_block->trailer_marker = FBE_FDF_TRAILER_MARKER;
    fbe_set_memory(service_block->tla_number, 0xFF, FBE_SCSI_INQUIRY_TLA_SIZE);
    fbe_copy_memory(service_block->fw_revision, "FBE_", 4);

    /* create the image. */
    image_ptr = fbe_api_allocate_memory(image_size); 
    MUT_ASSERT_NOT_NULL(image_ptr);   
    fbe_api_terminator_sas_drive_get_default_page_info(drive_type, &default_info);
    fbe_copy_memory(image_ptr->inquiry, default_info.inquiry, sizeof(image_ptr->inquiry));
    fbe_copy_memory(image_ptr->vpd_inquiry_f3, default_info.vpd_inquiry_f3, sizeof(image_ptr->vpd_inquiry_f3));
    /* modify fw rev in image*/
    fbe_copy_memory(image_ptr->inquiry+FBE_SCSI_INQUIRY_REVISION_OFFSET, "FBE_", FBE_SCSI_INQUIRY_REVISION_SIZE);

    fw_info->header_image_p = header_ptr;
    fw_info->header_size = FBE_FDF_INSTALL_HEADER_SIZE;
    fw_info->data_image_p = (fbe_u8_t*)image_ptr;
    fw_info->data_size = image_size;

    /* DEBUG ONLY.  DON'T CHECK IN WRITING OF FILE*/
    //sir_topham_hatt_write_fdf(fw_info);
    return;
}
/******************************************
 * end sir_topham_hatt_create_fw()
 ******************************************/

/*!**************************************************************
 * sir_topham_hatt_destroy_fw() 
 ****************************************************************
 * @brief
 *  This function destroys resources allocated by creating 
 *  drive fw.
 *
 * @param fw_info - Pointer to fw information.               
 *
 * @return None.
 *
 * @author
 *  12/24/2013 - Created.  Wayne Garrett
 ****************************************************************/
void sir_topham_hatt_destroy_fw(fbe_drive_configuration_control_fw_info_t *fw_info_p)
{
    fbe_api_free_memory(fw_info_p->data_image_p);
    fbe_api_free_memory(fw_info_p->header_image_p);
}

/*!**************************************************************
 * sir_topham_hatt_fw_set_tla() 
 ****************************************************************
 * @brief
 *  This function changes the TLA in the fw header and image.
 *  drive fw.
 *
 * @param fw_info - Pointer to fw information.               
 *
 * @return None.
 *
 * @author
*  12/24/2013 - Created.  Wayne Garrett
 ****************************************************************/
void sir_topham_hatt_fw_set_tla(fbe_drive_configuration_control_fw_info_t *fw_info, const fbe_u8_t *tla, fbe_u32_t tla_size)
{            
    fbe_fdf_serviceability_block_t *service_block;    
    
    MUT_ASSERT_INTEGER_EQUAL_MSG(tla_size, sizeof(service_block->tla_number), "invalid TLA length");

    service_block = (fbe_fdf_serviceability_block_t *)((fbe_u8_t *)fw_info->header_image_p + fw_info->header_image_p->trailer_offset);
    fbe_copy_memory(service_block->tla_number, tla, sizeof(service_block->tla_number));    
}


/*!**************************************************************
 * sir_topham_hatt_write_fdf() 
 ****************************************************************
 * @brief
 *  This function creates fw FDF file, to be used with simulated
 *  drives.
 *
 * @param fw_info - Pointer to fw information.               
 *
 * @return None.
 *
 * @author
 *  12/17/2012 - Created.  Wayne Garrett
 ****************************************************************/
void sir_topham_hatt_write_fdf(fbe_drive_configuration_control_fw_info_t *fw_info)
{
    fbe_u32_t nbytes = 0;
    fbe_u8_t file_path[] = {"test.fdf"};
    fbe_file_handle_t file;
    fbe_fdf_header_block_t  * header_copy_p = NULL;

    file = fbe_file_open(file_path, (FBE_FILE_WRONLY|FBE_FILE_APPEND), 0, NULL);
    if(file == FBE_FILE_INVALID_HANDLE)
    {
        mut_printf(MUT_LOG_LOW, "%s: Could not open file, creating new file = %s \n", __FUNCTION__, file_path);

        /* The open failed, the file must not be there, so lets create a new file. */
        file = fbe_file_open(file_path, (FBE_FILE_WRONLY|FBE_FILE_APPEND|FBE_FILE_CREAT|FBE_FILE_TRUNC), 0, NULL);

        if(file == FBE_FILE_INVALID_HANDLE)
        {
            mut_printf(MUT_LOG_LOW, "%s: Could not create file = %s \n", __FUNCTION__, file_path);
        }
        else
        {
            fbe_file_close(file);
            /* Re-open the newly created file for writing. */
            file = fbe_file_open(file_path, (FBE_FILE_WRONLY|FBE_FILE_APPEND), 0, NULL);
            if(file == FBE_FILE_INVALID_HANDLE)
            {
                mut_printf(MUT_LOG_LOW, "%s: Could not open file after creation = %s \n", __FUNCTION__, file_path);
            }
        }        
    }


    /* format of file is big endian.   We need to convert integer fields 
    */
    header_copy_p = malloc(fw_info->header_size);
    MUT_ASSERT_NOT_NULL(header_copy_p);
    fbe_copy_memory(header_copy_p, fw_info->header_image_p, fw_info->header_size);

    header_copy_p->header_marker = BYTE_SWAP_32(fw_info->header_image_p->header_marker);
    header_copy_p->image_size = BYTE_SWAP_32(fw_info->header_image_p->image_size);
    header_copy_p->trailer_offset = BYTE_SWAP_16(fw_info->header_image_p->trailer_offset);

    nbytes = fbe_file_write(file, header_copy_p, fw_info->header_size, NULL);

    /* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
    if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != fw_info->header_size))
    {
        MUT_FAIL_MSG("sir_topham_hatt_write_fdf: Could not write header");
    }

    nbytes = fbe_file_write(file, fw_info->data_image_p, fw_info->data_size, NULL);

    /* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
    if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != fw_info->data_size))
    {
        MUT_FAIL_MSG("sir_topham_hatt_write_fdf: Could not write data");
    }

    fbe_file_close(file);

}


/*!**************************************************************
 * sir_topham_hatt_select_drives() 
 ****************************************************************
 * @brief
 *  This function sets up the header for drive selected.
 *
 * @param header_ptr - Pointer to header structure.               
 * @param selected_drives               
 *
 * @return None.
 *
 * @author
 *  03/16/2011 - Created.  Wayne Garrett
 ****************************************************************/
void sir_topham_hatt_select_drives(fbe_fdf_header_block_t *header_ptr, sir_topham_hatt_select_drives_t *selected_drives)
{
    fbe_u32_t i;
    fbe_drive_selected_element_t *drive;

    header_ptr->num_drives_selected = selected_drives->num_of_drives;
    header_ptr->cflags |= FBE_FDF_CFLAGS_SELECT_DRIVE;
    drive = &header_ptr->first_drive;
    for (i = 0; i < selected_drives->num_of_drives; i++)
    {
        drive->bus = (selected_drives->selected_drives)[i].bus;
        drive->enclosure = (selected_drives->selected_drives)[i].enclosure;
        drive->slot = (selected_drives->selected_drives)[i].slot;
        drive++;
    }

    return;
}
/******************************************
 * end sir_topham_hatt_select_drives()
 ******************************************/


/*!**************************************************************
 * sir_topham_hatt_check_drive_download_status() 
 ****************************************************************
 * @brief
 *  This function checks drive download status.
 *
 * @param drive - Pointer to drive.               
 * @param state - Expected drive state.               
 * @param fail_reason - Expected download fail reason.               
 *
 * @return None.
 *
 * @author
 *  03/16/2011 - Created.  Wayne Garrett
 ****************************************************************/
void sir_topham_hatt_check_drive_download_status(fbe_drive_selected_element_t *drive,
												 fbe_drive_configuration_download_drive_state_t state,
												 fbe_drive_configuration_download_drive_fail_t fail_reason)
{
    fbe_status_t    status;
    fbe_drive_configuration_download_get_drive_info_t   drive_request = {0};

    /* Get drive download status */
    drive_request.bus = drive->bus; 
    drive_request.enclosure = drive->enclosure; 
    drive_request.slot = drive->slot;
    status = fbe_api_drive_configuration_interface_get_download_drive(&drive_request);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Verify drive download status */
    MUT_ASSERT_INT_EQUAL(drive_request.state, state);
    MUT_ASSERT_INT_EQUAL(drive_request.fail_code, fail_reason);
}
/******************************************
 * end sir_topham_hatt_check_drive_download_status()
 ******************************************/

/*!**************************************************************
 * sir_topham_hatt_wait_download_process_state() 
 ****************************************************************
 * @brief
 *  This function waits some seconds for download process done.
 *
 * @param dl_status - Pointer to download status.               
 * @param seconds - Seconds to wait before return failure.               
 *
 * @return fbe_status_t.
 *
 * @author
 *  03/16/2011 - Created.  Wayne Garrett
 ****************************************************************/
fbe_status_t sir_topham_hatt_wait_download_process_state(fbe_drive_configuration_download_get_process_info_t * dl_status,
                                                         fbe_drive_configuration_download_process_state_t dl_state,
                                                         fbe_u32_t seconds)
{
    fbe_u32_t i;
    fbe_status_t status;

    for (i = 0; i < 2*seconds; i++)
    {
        status = fbe_api_drive_configuration_interface_get_download_process(dl_status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        if ((dl_status->state == dl_state))
        {
            return FBE_STATUS_OK;
        }

        if (i%20 == 0)
        {
            mut_printf(MUT_LOG_TEST_STATUS,"Sir Topham Hatt: dl_state = %d \n", dl_status->state);
        }
        fbe_api_sleep(500);
    }

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end sir_topham_hatt_wait_download_process_state()
 ******************************************/

/*!**************************************************************
 * sir_topham_hatt_check_fw_download_event_log() 
 ****************************************************************
 * @brief
 *  This function creates a fw header and an image.
 *
 * @param fw_info - Pointer to fw information.               
 *
 * @return fbe_status_t.
 *
 * @author
 *  05/05/2012 - Created.  
 ****************************************************************/
fbe_status_t sir_topham_hatt_check_fw_download_event_log(fbe_drive_configuration_control_fw_info_t *fw_info, fbe_u32_t msg_id,
                                                         fbe_drive_configuration_download_process_fail_t fail_code, fbe_bool_t *msg_present)
{
    fbe_status_t  status;
    fbe_bool_t is_msg_present = FBE_FALSE;
    fbe_fdf_header_block_t *header = fw_info->header_image_p;
    fbe_fdf_serviceability_block_t *service_block;
    fbe_u8_t  tla_number[FBE_FDF_TLA_SIZE + 1];

    service_block = (fbe_fdf_serviceability_block_t *)((fbe_u8_t *)header + header->trailer_offset);

    /* copy tla number from image header */
    fbe_copy_memory(tla_number, service_block->tla_number, FBE_FDF_TLA_SIZE);
    tla_number[FBE_FDF_TLA_SIZE] = '\0';


    switch(msg_id){
        case PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_STARTED:
            status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_PHYSICAL, 
                                                 &is_msg_present, 
                                                 PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_STARTED,
                                                 tla_number, service_block->fw_revision);    

            break;

        case PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_COMPLETED:
            status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_PHYSICAL, 
                                                 &is_msg_present, 
                                                 PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_COMPLETED,
                                                 tla_number, service_block->fw_revision);    

            break;
        case PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_ABORTED:
            status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_PHYSICAL, 
                                                 &is_msg_present, 
                                                 PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_ABORTED);    

            break;
        case PHYSICAL_PACKAGE_ERROR_DRIVE_FW_DOWNLOAD_FAILED:
            status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_PHYSICAL, 
                                                 &is_msg_present, 
                                                 PHYSICAL_PACKAGE_ERROR_DRIVE_FW_DOWNLOAD_FAILED,
                                                 tla_number, service_block->fw_revision, fail_code);    

            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    *msg_present = is_msg_present;
    return status;
}
/******************************************
 * end sir_topham_hatt_check_fw_download_event_log()
 ******************************************/

static void sir_topham_hatt_test_get_all(void)
{
	fbe_status_t		status;
	fbe_u32_t 			expected_count = FBE_MAX_DOWNLOAD_DRIVES;
	fbe_u32_t			d;
	fbe_u32_t			returned_count;

    fbe_drive_configuration_download_get_drive_info_t *	get_all = 
		(fbe_drive_configuration_download_get_drive_info_t *)fbe_api_allocate_memory(sizeof(fbe_drive_configuration_download_get_drive_info_t) * expected_count);

	mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

	status = fbe_api_drive_configuration_interface_get_all_download_drives(get_all, expected_count, &returned_count);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(returned_count == SIR_TOPHAM_NUMBER_OF_DRIVES);

	/*search for one of the drive we did the download to*/
	for (d = 0; d < FBE_MAX_DOWNLOAD_DRIVES; d++) {
		if (get_all[d].bus == SIR_TOPHAM_HATT_BUS && get_all[d].enclosure == SIR_TOPHAM_HATT_ENCL && get_all[d].slot == SIR_TOPHAM_HATT_SLOT) {
			/*look if the drive info makes sense*/
			MUT_ASSERT_TRUE(get_all[d].state == FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_IDLE);
			MUT_ASSERT_TRUE(get_all[d].fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_NO_FAILURE);
			MUT_ASSERT_TRUE(get_all[d].prior_product_rev[0] != 0);
			break;

		}
	}
    
    fbe_api_free_memory(get_all);

	mut_printf(MUT_LOG_LOW, "=== %s completed ===\n", __FUNCTION__);

}

/*!**************************************************************
 * sir_topham_hatt_abort_download_after_delay_ms() 
 ****************************************************************
 * @brief
 *  This function will issue a download then wait some number
 *  of msec before issuing abort.
 *
 * @param delay_msec - delay before aborting. In msec            
 *
 * @return fbe_status_t.
 *
 * @author
 *  04/10/2013  Wayne Garrett - Created.  
 ****************************************************************/
static void sir_topham_hatt_abort_download_after_delay_ms(fbe_drive_configuration_control_fw_info_t * fw_info_p, sir_topham_hatt_select_drives_t * drives_p, fbe_u32_t delay_msec)
{
    fbe_u32_t retry_count, i;
    fbe_status_t status;
    fbe_bool_t msg_present;
    fbe_drive_configuration_download_get_process_info_t dl_status;
    fbe_bool_t is_drive_active = FBE_TRUE;
    fbe_u32_t elapsed_time = 0;
    fbe_u32_t sleep_interval = 500; //ms
    fbe_u32_t wait_timeout = 20000; //ms

    mut_printf(MUT_LOG_TEST_STATUS,"Sir Topham Hatt TEST: aborting download after %dms delay.\n", delay_msec);

    /* send download */
    sir_topham_hatt_select_drives(fw_info_p->header_image_p, drives_p);
    status = fbe_api_drive_configuration_interface_download_firmware(fw_info_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_api_sleep(delay_msec);

    /* issue abort */
    status = fbe_api_drive_configuration_interface_abort_download();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Wait download process till done */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_ABORTED, 20);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_ABORTED);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Checking event log for download failed message.\n", __FUNCTION__);
    for (retry_count = 0; retry_count < SIR_TOPHAM_HATT_EVENT_LOG_CHECK_COUNT; retry_count++){
        msg_present = FBE_FALSE;
        status = sir_topham_hatt_check_fw_download_event_log(fw_info_p, PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_ABORTED,
                                                             0 , &msg_present);
        if ((status == FBE_STATUS_OK) && msg_present)
        {
            break;
        }
        fbe_api_sleep(SIR_TOPHAM_HATT_EVENT_LOG_CHECK_SLEEP_INTERVAL);
    }

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(msg_present);


    /* Wait until all drives finish the abort.   Note, that the code currently sets the process state to Aborted before
       waiting for all drives. */
    do
    {
        for (i = 0; i < drives_p->num_of_drives; i++)
        {
            fbe_drive_configuration_download_get_drive_info_t   drive_request = {0};
            is_drive_active = FBE_FALSE;
    
            /* Get drive download status */
            drive_request.bus = drives_p->selected_drives[i].bus;
            drive_request.enclosure = drives_p->selected_drives[i].enclosure; 
            drive_request.slot = drives_p->selected_drives[i].slot;
            status = fbe_api_drive_configuration_interface_get_download_drive(&drive_request);
    
            if (drive_request.state == FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_ACTIVE)
            {
                is_drive_active = FBE_TRUE;
                fbe_api_sleep(sleep_interval);
                elapsed_time += sleep_interval;
                break;
            }
        }
    } while (is_drive_active && elapsed_time < wait_timeout);


    /* Verify drives are in expected states */
    for (i = 0; i < drives_p->num_of_drives; i++)
    {
        fbe_drive_configuration_download_get_drive_info_t   drive_request = {0};
    
        /* Get drive download status */
        drive_request.bus = drives_p->selected_drives[i].bus;
        drive_request.enclosure = drives_p->selected_drives[i].enclosure; 
        drive_request.slot = drives_p->selected_drives[i].slot;
        status = fbe_api_drive_configuration_interface_get_download_drive(&drive_request);

        /* check that all drives are either fail/aborted or completed. */
        if (!(drive_request.state == FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL &&
              drive_request.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_ABORTED) &&
            drive_request.state != FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_COMPLETE)
        {       
            mut_printf(MUT_LOG_TEST_STATUS, "%d_%d_%d not failed/aborted or completed. state=%d failcode=%d\n", 
                       drive_request.bus, drive_request.enclosure, drive_request.slot,
                       drive_request.state, drive_request.fail_code);     
            MUT_FAIL_MSG("Drive not in expected state\n");
        }
    }

    for (i = 0; i < drives_p->num_of_drives; i++)
    {
        status = fbe_test_pp_util_verify_pdo_state(drives_p->selected_drives[i].bus,
                                                   drives_p->selected_drives[i].enclosure,
                                                   drives_p->selected_drives[i].slot,
                                                   FBE_LIFECYCLE_STATE_READY,
                                                   5000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

}

/*!**************************************************************
 * sir_topham_hatt_add_debug_hook()
 ****************************************************************
 * @brief
 *  Add the debug hook for the given state and substate.
 *
 * @param object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  1/19/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
static void sir_topham_hatt_add_debug_hook(fbe_object_id_t object_id, 
                                           fbe_u32_t state, 
                                           fbe_u32_t substate,
                                           fbe_u64_t val1,
                                           fbe_u64_t val2,
                                           fbe_u32_t check_type,
                                           fbe_u32_t action)

{
    fbe_status_t    status;

    mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook: obj id 0x%X state %d substate %d", object_id, state, substate);

    status = fbe_api_scheduler_add_debug_hook_pp(object_id, 
                                              state, 
                                              substate, 
                                              val1, 
                                              val2, 
                                              check_type, 
                                              action);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}

/*!**************************************************************
 * sir_topham_hatt_delete_debug_hook()
 ****************************************************************
 * @brief
 *  Delete the debug hook for the given state and substate.
 *
 * @param object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return None 
 *
 * @author
 *  3/26/2014 - Created. Geng.Han
 *
 ****************************************************************/
static void sir_topham_hatt_delete_debug_hook(fbe_object_id_t object_id, 
                                              fbe_u32_t state, 
                                              fbe_u32_t substate,
                                              fbe_u64_t val1,
                                              fbe_u64_t val2,
                                              fbe_u32_t check_type,
                                              fbe_u32_t action)
{
    fbe_status_t                        status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook: obj id: 0x%X state: %d, substate: %d", object_id, state, substate);

    status = fbe_api_scheduler_del_debug_hook_pp(object_id, 
                                              state, 
                                              substate, 
                                              val1, 
                                              val2, 
                                              check_type, 
                                              action);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return;
}


/*!**************************************************************
 * sir_topham_hatt_wait_for_target_SP_hook()
 ****************************************************************
 * @brief
 *  Wait for the hook to be hit
 *
 * @param object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  3/26/2014 - Created. Geng.Han
 *
 ****************************************************************/
fbe_status_t sir_topham_hatt_wait_for_target_SP_hook(fbe_object_id_t object_id, 
                                                     fbe_u32_t state, 
                                                     fbe_u32_t substate) 
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = 50000;
    
    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.object_id = object_id;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.val1 = NULL;
    hook.val2 = NULL;
    hook.counter = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the hook. obj id: 0x%X state: %d substate: %d.", object_id, state, substate);

    while (current_time < timeout_ms){

        status = fbe_api_scheduler_get_debug_hook_pp(&hook);

        if (hook.counter != 0) {
            mut_printf(MUT_LOG_TEST_STATUS, "We have hit the debug hook %d times", (int)hook.counter);

            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);      
            return status;
        }
        current_time = current_time + 200;
        fbe_api_sleep (200);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: object %d did not hit state: %d substate: %d in %d ms!\n", __FUNCTION__, object_id, state, substate, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}


/*********************************
 * end file sir_topham_hatt_test.c
 *********************************/


