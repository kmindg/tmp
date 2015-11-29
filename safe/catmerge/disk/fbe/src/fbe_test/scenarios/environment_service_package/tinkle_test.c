/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file tinkle_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify the ESP object notification.
 * 
 * @version
 *   25-Nov-2010 - Created. Vaibhav Gaonkar
 *   18-Feb-2011 - Vaibhav Gaonkar - Updated to verifiy physical location of
 *                                   notification info.
 *
 ***************************************************************************/

 /*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "physical_package_tests.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_sps_info.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe_test_esp_utils.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"
#include "pp_utils.h"

char * tinkle_short_desc = "Verify the ESP Object's notification for data change";
char * tinkle_long_desc ="\
\n\
Starting Config:\n\
        [PP] chautauqua_config\n\
        [PP] armada board\n\
        [PP] 2 SAS PMC ports\n\
        [PP] 8 viper enclosure/bus\n\
        [PP] 15 SAS drives/enclosure\n\
        [PP] 15 logical drives/enclosure\n\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
        - Create and verify the initial ESP config. \n\
STEP 2: Insert and remove enclosure and verify ESP encl_mgmt data change notification.\n\
        Insert the drive and verify ESP drive_mgmt data change notification.\n\
        Verify ESP SPS_mgmt data change notification. \n\
        Verify ESP module_mgmt data change notification. \n\
        Verfiy ESP ps_mgmt data change notification \n\
";

/*******************************
 *   local function prototypes
 *******************************/
#define TINKLE_BUS_0        0
#define TINKLE_BUS_1        1
#define TINKLE_ENCL_7       7
#define TINKLE_SLOT_5       5
#define TINKLE_IOMOD_SLOT_1 1

static fbe_object_id_t tinkle_expected_objectId = FBE_OBJECT_ID_INVALID;
static fbe_u64_t tinkle_expected_device = FBE_DEVICE_TYPE_INVALID;
static fbe_device_physical_location_t tinkle_expected_phys_location = {0};
fbe_semaphore_t tinkle_sem;

static void tinkle_encl_mgmt_data_change_notification_test(void);
static void tinkle_drive_mgmt_data_change_notification_test(void);
static void tinkle_sps_mgmt_data_change_notification_test(void);
static void tinkle_module_mgmt_data_change_notification_test(void);
static void tinkle_ps_mgmt_data_change_notification_test(void);
static void tinkle_cooling_mgmt_data_change_notification_test(void);
static void 
tinkle_esp_object_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                void *context);

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
 /*!*************************************************************
 * tinkle_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test ESP data change notification
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  25-Nov-2010 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/ 
void tinkle_test(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;  
    fbe_notification_registration_id_t reg_id;

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    mut_printf(MUT_LOG_LOW, "=== Wait max 30 seconds for resume prom read to complete ===");

    /* Wait util there is no resume prom read in progress. */
    status = fbe_test_esp_wait_for_no_resume_prom_read_in_progress(30000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for resume prom read to complete failed!");

    mut_printf(MUT_LOG_LOW, "Tinkle test started...\n");

    fbe_semaphore_init(&tinkle_sem, 0, 1);

        /* Register to get event notification from ESP */
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  tinkle_esp_object_data_change_callback_function,
                                                                  &tinkle_sem,
                                                                  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    tinkle_encl_mgmt_data_change_notification_test();
    tinkle_drive_mgmt_data_change_notification_test();
    tinkle_sps_mgmt_data_change_notification_test();
 //   tinkle_module_mgmt_data_change_notification_test(); // need to add support for starting sep in chataqua config
    tinkle_ps_mgmt_data_change_notification_test();
 //   tinkle_cooling_mgmt_data_change_notification_test();
    
    /* Unregistered the notification*/
    status = fbe_api_notification_interface_unregister_notification(tinkle_esp_object_data_change_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&tinkle_sem);
    
    mut_printf(MUT_LOG_LOW, "Tinkle test passed...\n");
    return;
}
/**********************
 * end of tinkle_test()
 ***********************/
 /*!*************************************************************
 *  tinkle_setup() 
 ****************************************************************
 * @brief
 *  Initiate the setup required to run the test
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  25-Nov-2010 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/ 
void tinkle_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                    FBE_ESP_TEST_CHAUTAUQUA_VIPER_CONFIG,
                                    SPID_PROMETHEUS_M1_HW_TYPE,
                                    NULL,
                                    NULL);
}
/***********************
 *  end of tinkle_setup
 ***********************/
 /*!*************************************************************
 *  tinkle_destory() 
 ****************************************************************
 * @brief
 *  Unload all packages.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  25-Nov-2010 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/  
void tinkle_destroy(void)
{
    fbe_test_sep_util_destroy_neit_sep_physical();
    fbe_test_esp_delete_registry_file();
    return;
}
/***************************
 *  end of tinkle_destory()
 **************************/

 /*!*************************************************************
 *  tinkle_encl_mgmt_data_change_notification_test() 
 ****************************************************************
 * @brief
 *  This function is entry point for Encl_mgmt data change 
 *  notification test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  25-Nov-2010 - Created. Vaibhav Gaonkar
 *  18-Feb-2011 - Vaibhav Gaonkar - Updated to verifiy physical location of
 *                                  notification info.
 *
 ****************************************************************/  
static void 
tinkle_encl_mgmt_data_change_notification_test(void)
{
    DWORD           dwWaitResult;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;  
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    ses_stat_elem_encl_struct       lcc_stat;
    fbe_esp_encl_mgmt_encl_info_t   encl_info;
    fbe_device_physical_location_t  location;
    fbe_terminator_sas_encl_info_t  sas_encl;    
    fbe_api_terminator_device_handle_t  port_handle;
    fbe_api_terminator_device_handle_t  enclosure_handle;
    fbe_api_terminator_device_handle_t  drive_handle;

    mut_printf(MUT_LOG_LOW, "Tinkle test: Started Encl_mgmt notification test...\n");

    /* Get encl_mgmt object ID*/
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    tinkle_expected_objectId = object_id;
    tinkle_expected_device = FBE_DEVICE_TYPE_ENCLOSURE;
    tinkle_expected_phys_location.bus = TINKLE_BUS_0;
    tinkle_expected_phys_location.enclosure = TINKLE_ENCL_7;

    /* VERIFY NOTIFICATION FOR ENCL STATE CHANGE */
    /* Remove an enclosure */
    status = fbe_api_terminator_get_enclosure_handle(TINKLE_BUS_0, TINKLE_ENCL_7, &enclosure_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_terminator_remove_sas_enclosure(enclosure_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Waiting for enclosure to be removed...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tinkle_sem, 30000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /*Verify the enclosure get removed*/
    location.bus = TINKLE_BUS_0;
    location.enclosure = TINKLE_ENCL_7;
    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, encl_info.enclPresent, "enclPresent value not expected\n");
    mut_printf(MUT_LOG_LOW, "Tinkle test: Verified enclosure state change notification successfully...\n");

    /* Re-insert the enclosure */
    status = fbe_api_terminator_get_port_handle(TINKLE_BUS_0, &port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    sas_encl.backend_number = TINKLE_BUS_0;
    sas_encl.encl_number = TINKLE_ENCL_7;
    sas_encl.uid[0] = 15; // some unique ID.
    sas_encl.sas_address = 0x5000097A79000000 ;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    status = fbe_api_terminator_insert_sas_enclosure(port_handle, &sas_encl, &enclosure_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        // insert some drives
    //for (slot = 0; slot < (TINKLE_SLOT_5+1); slot ++) {
    status  = fbe_test_pp_util_insert_sas_drive(0,
                                                TINKLE_ENCL_7,
                                                TINKLE_SLOT_5,
                                                520,
                                                TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);        /*insert a sas_drive to port 0 encl 0 slot */
    //status  = fbe_api_terminator_insert_sas_drive (encl0_0_handle, TINKLE_SLOT_5, &sas_drive, &drive_handle);
    //}


    mut_printf(MUT_LOG_LOW, "Waiting for enclosure to get add...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tinkle_sem, 30000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* VERIFY NOTIFICATION FOR ENCLOSURE DATA CHANGE */
    status = fbe_api_terminator_get_lcc_eses_status(enclosure_handle, &lcc_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    /* Change the data */
    lcc_stat.warning_indicaton = 0x01;
    status = fbe_api_terminator_set_lcc_eses_status(enclosure_handle, lcc_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Waiting for notification of lcc data change...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tinkle_sem, 30000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    mut_printf(MUT_LOG_LOW, "Tinkle test: Verified enclosure data change notification successfully...\n");

    tinkle_expected_objectId = FBE_OBJECT_ID_INVALID;
    tinkle_expected_device = FBE_DEVICE_TYPE_INVALID;
    
    mut_printf(MUT_LOG_LOW, "Tinkle test: Notification test for encl_mgmt passed successfully ...\n");
    return;
   
}
/*********************************************************
 *  end of tinkle_encl_mgmt_data_change_notification_test
 ********************************************************/ 
 /*!*************************************************************
 *  tinkle_drive_mgmt_data_change_notification_test() 
 ****************************************************************
 * @brief
 *  This function is entry point for drive_mgmt data change 
 *  notification test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  25-Nov-2010 - Created. Vaibhav Gaonkar
 *  18-Feb-2011 - Vaibhav Gaonkar - Updated to verifiy physical location of
 *                                  notification info.
 *
 ****************************************************************/
static void 
tinkle_drive_mgmt_data_change_notification_test(void)
{
    DWORD        dwWaitResult;
    fbe_u32_t    object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t status;
    fbe_terminator_sas_drive_info_t drive_info;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_api_terminator_device_handle_t enclosure_handle;
    fbe_api_terminator_device_handle_t drive_handle;    

    mut_printf(MUT_LOG_LOW, "Tinkle test: Started Drive_mgmt notification test...\n");
    
   /* Get drive_mgmt object ID*/
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    tinkle_expected_objectId = object_id;
    tinkle_expected_device = FBE_DEVICE_TYPE_DRIVE;
    tinkle_expected_phys_location.bus = 0;
    tinkle_expected_phys_location.enclosure = TINKLE_ENCL_7;
    tinkle_expected_phys_location.slot = TINKLE_SLOT_5;

    /* Verify FBE_LIFECYCLE_STATE_DESTROY notificaton from physical package to ESP */
    /* Remove the drive */
    status = fbe_api_terminator_get_drive_handle(0, 
                                                 TINKLE_ENCL_7, 
                                                 TINKLE_SLOT_5,
                                                 &drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    /*Get drive info */
    status = fbe_api_terminator_get_sas_drive_info(drive_handle, 
                                                   &drive_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);    
    status = fbe_api_terminator_remove_sas_drive(drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Waiting for drive to get removed...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tinkle_sem, 30000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Verify FBE_LIFECYCLE_STATE_READY notificaton from physical package to ESP*/
    /* Insert the drive */
    status = fbe_api_terminator_get_enclosure_handle(0,
                                                     TINKLE_ENCL_7, 
                                                     &enclosure_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Re-insert the drive */
    sas_drive.drive_type = drive_info.drive_type;
    sas_drive.sas_address = drive_info.sas_address;
    sas_drive.backend_number = 0;
    sas_drive.encl_number = TINKLE_ENCL_7;
    sas_drive.block_size = drive_info.block_size;
    sas_drive.capacity = drive_info.capacity;
    status = fbe_api_terminator_insert_sas_drive(enclosure_handle, 
                                                 TINKLE_SLOT_5,
                                                 &sas_drive, 
                                                 &drive_handle);
    mut_printf(MUT_LOG_LOW, "Waiting for drive to get inserted...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tinkle_sem, 30000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    tinkle_expected_objectId = FBE_OBJECT_ID_INVALID;
    tinkle_expected_device = FBE_DEVICE_TYPE_INVALID;
    mut_printf(MUT_LOG_LOW, "Tinkle test: Notification test for drive_mgmt passed successfully ...\n");
    return;
       
}
/************************************************************
 *  end of tinkle_drive_mgmt_data_change_notification_test()
 ************************************************************/

 /*!*************************************************************
 *  tinkle_sps_mgmt_data_change_notification_test() 
 ****************************************************************
 * @brief
 *  This function is entry point for SPS_mgmt data change 
 *  notification test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  25-Nov-2010 - Created. Vaibhav Gaonkar
 *  18-Feb-2011 - Vaibhav Gaonkar - Updated to verifiy physical location of
 *                                  notification info.
 *
 ****************************************************************/
static void 
tinkle_sps_mgmt_data_change_notification_test(void)
{
    DWORD dwWaitResult;
    fbe_u32_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t status;
    SPECL_SFI_MASK_DATA                     sfi_mask_data;

    mut_printf(MUT_LOG_LOW, "Tinkle test: Started sps_mgmt notification test...\n");

    mut_printf(MUT_LOG_LOW, "%s, disable simulateSps", __FUNCTION__);
    status = fbe_api_esp_sps_mgmt_setSimulateSps(FALSE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Waiting for Initial SPS Testing to complete\n");
    
//    status = fbe_test_esp_wait_for_sps_test_to_complete(FALSE, FBE_LOCAL_SPS, 150000);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Get sps_mgmt object ID*/
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    tinkle_expected_objectId = object_id;
    tinkle_expected_device = FBE_DEVICE_TYPE_SPS;
    /* This test scenario uses MUSTANG platform. 
     * So for the SPS location, bus should be FBE_XPE_PSEUDO_BUS_NUM and 
     * encl should be FBE_XPE_PSEUDO_ENCL_NUM 
     */
    tinkle_expected_phys_location.bus = FBE_XPE_PSEUDO_BUS_NUM;
    tinkle_expected_phys_location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    tinkle_expected_phys_location.slot = SP_A;

    /* Get Board Object ID */
    status = fbe_api_get_board_object_id(&object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    
    /* Change SPS Status in Board Object */
    sfi_mask_data.structNumber = SPECL_SFI_SPS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.spsStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.spsStatus.state = SPS_STATE_ON_BATTERY;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    /* Verify that ESP sps_mgmt detects SPS Status change */
    mut_printf(MUT_LOG_LOW, "Waiting for SPS status changed to SPS_STATE_ON_BATTERY...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tinkle_sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    tinkle_expected_objectId = FBE_OBJECT_ID_INVALID;
    tinkle_expected_device = FBE_DEVICE_TYPE_INVALID;
    
    mut_printf(MUT_LOG_LOW, "%s, enable simulateSps", __FUNCTION__);
    status = fbe_api_esp_sps_mgmt_setSimulateSps(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        
    mut_printf(MUT_LOG_LOW, "Tinkle test: Notification test for sps_mgmt passed successfully ...\n");
    return;
    
}
/************************************************************
 *  end of tinkle_sps_mgmt_data_change_notification_test()
 ************************************************************/
 /*!*************************************************************
 *  tinkle_module_mgmt_data_change_notification_test() 
 ****************************************************************
 * @brief
 *  This function is entry point for module_mgmt data change 
 *  notification test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  25-Nov-2010 - Created. Vaibhav Gaonkar
 *  18-Feb-2011 - Vaibhav Gaonkar - Updated to verifiy physical location of
 *                                  notification info.
 *
 ****************************************************************/
static void 
tinkle_module_mgmt_data_change_notification_test(void)
{
    DWORD           dwWaitResult;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_esp_module_io_module_info_t espIoModuleInfo = {0};
    
    mut_printf(MUT_LOG_LOW, "Tinkle test: Started module_mgmt notification test...\n");

    /* Get module_mgmt object ID*/
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    tinkle_expected_objectId= object_id;
    tinkle_expected_device = FBE_DEVICE_TYPE_IOMODULE;
    tinkle_expected_phys_location.slot = TINKLE_IOMOD_SLOT_1;
    tinkle_expected_phys_location.sp = SP_B;

    /* Get handle to the board object */
    status  = fbe_api_get_board_object_id(&object_id);
    MUT_ASSERT_TRUE(status  == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    
    /* Change the io module led blink rate. */
    status  = fbe_api_board_set_IOModuleFaultLED(object_id, 
                                                 SP_B, 
                                                 TINKLE_IOMOD_SLOT_1,
                                                 FBE_DEVICE_TYPE_IOMODULE,
                                                 LED_BLINK_ON);
    MUT_ASSERT_TRUE(status  == FBE_STATUS_OK);

    /* Wait 1 minute for power supply data change notification. */
    mut_printf(MUT_LOG_LOW, "Waiting for IO module fault led changed...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tinkle_sem, 30000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Verify the status from ESP */
    espIoModuleInfo.header.sp = SP_B;
    espIoModuleInfo.header.slot = TINKLE_IOMOD_SLOT_1;
    espIoModuleInfo.header.type = FBE_DEVICE_TYPE_IOMODULE;
    status  = fbe_api_esp_module_mgmt_getIOModuleInfo(&espIoModuleInfo);
    MUT_ASSERT_TRUE(status  == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(espIoModuleInfo.io_module_phy_info.associatedSp == SP_B);
    MUT_ASSERT_TRUE(espIoModuleInfo.io_module_phy_info.slotNumOnBlade == TINKLE_IOMOD_SLOT_1);

    tinkle_expected_objectId = FBE_OBJECT_ID_INVALID;
    tinkle_expected_device = FBE_DEVICE_TYPE_INVALID;

    mut_printf(MUT_LOG_LOW, "Tinkle test: Notification test for module_mgmt passed successfully ...\n");
    
    return;
}
/************************************************************
 *  end of tinkle_module_mgmt_data_change_notification_test()
 ************************************************************/
 /*!*************************************************************
 *  tinkle_ps_mgmt_data_change_notification_test() 
 ****************************************************************
 * @brief
 *  This function is entry point for ps_mgmt data change 
 *  notification test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  25-Nov-2010 - Created. Vaibhav Gaonkar
 *  18-Feb-2011 - Vaibhav Gaonkar - Updated to verifiy physical location of
 *                                  notification info.
 *
 ****************************************************************/
static void 
tinkle_ps_mgmt_data_change_notification_test(void)
{
    DWORD           dwWaitResult;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    ses_stat_elem_ps_struct   ps_stat;
    fbe_terminator_sas_encl_info_t  sas_encl; 
    fbe_api_terminator_device_handle_t  enclosure_handle;
    fbe_api_terminator_device_handle_t  port_handle;

    mut_printf(MUT_LOG_LOW, "Tinkle test: Started ps_mgmt notification test...\n");

    /* Get ps_mgmt object ID*/
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_PS_MGMT, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    tinkle_expected_objectId = object_id;
    tinkle_expected_device = FBE_DEVICE_TYPE_PS;
    /* Set expected physical location */
    tinkle_expected_phys_location.bus = TINKLE_BUS_0;
    tinkle_expected_phys_location.enclosure = TINKLE_ENCL_7;
    tinkle_expected_phys_location.slot = PS_1;

    /* VERIFY NOTIFICATION FOR PS_MGMT DATA CHANGE */
    status = fbe_api_terminator_get_enclosure_handle(TINKLE_BUS_0,
                                                     TINKLE_ENCL_7, 
                                                     &enclosure_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_terminator_get_ps_eses_status(enclosure_handle, 
                                                   PS_1, 
                                                   &ps_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(ps_stat.ac_fail == 0);
    /* Set the ac fault */
    ps_stat.ac_fail = 1;
    status = fbe_api_terminator_set_ps_eses_status(enclosure_handle, 
                                                   PS_1, 
                                                   ps_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
  /* Verify that ESP ps_mgmt detects SPS Data change */
    mut_printf(MUT_LOG_LOW, "Waiting for PS AC FAIL...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tinkle_sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != EMCPAL_STATUS_TIMEOUT);

    /* Set back ac_fail */
    ps_stat.ac_fail = 0;
    status = fbe_api_terminator_set_ps_eses_status(enclosure_handle, 
                                                   PS_1, 
                                                   ps_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    /* Verify that ESP ps_mgmt detects SPS Data change */
    mut_printf(MUT_LOG_LOW, "Waiting for PS AC restore...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tinkle_sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != EMCPAL_STATUS_TIMEOUT);
    mut_printf(MUT_LOG_LOW, "Tinkle test: Verified ps_mgmt data change notification successfully...\n");

    /*  VERIFY NOTIFICATION FOR PS_MGMT STATE CHANGE */
    /* Remove the enclosure so PS state change occoured */
    tinkle_expected_phys_location.slot = PS_0;
    status = fbe_api_terminator_remove_sas_enclosure(enclosure_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Verify that ESP ps_mgmt detects SPS Status change */
    dwWaitResult = fbe_semaphore_wait_ms(&tinkle_sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != EMCPAL_STATUS_TIMEOUT);
    mut_printf(MUT_LOG_LOW, "Tinkle test: Verified ps_mgmt state change notification successfully...\n");

    /* Re-insert the enclosure */
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    tinkle_expected_objectId = object_id;
    tinkle_expected_device = FBE_DEVICE_TYPE_ENCLOSURE;
    tinkle_expected_phys_location.bus = TINKLE_BUS_0;
    tinkle_expected_phys_location.enclosure = TINKLE_ENCL_7;

    status = fbe_api_terminator_get_port_handle(TINKLE_BUS_0, &port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    sas_encl.backend_number = TINKLE_BUS_0;
    sas_encl.encl_number = TINKLE_ENCL_7;
    sas_encl.uid[0] = 15; // some unique ID.
    sas_encl.sas_address = 0x5000097A79000000 ;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    status = fbe_api_terminator_insert_sas_enclosure(port_handle, &sas_encl, &enclosure_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Waiting for Encl inserted...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tinkle_sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != EMCPAL_STATUS_TIMEOUT);
    
    tinkle_expected_objectId = FBE_OBJECT_ID_INVALID;
    tinkle_expected_device = FBE_DEVICE_TYPE_INVALID;
    mut_printf(MUT_LOG_LOW, "Tinkle test: Notification test for ps_mgmt passed successfully ...\n");
    return;
}
/************************************************************
 *  end of tinkle_ps_mgmt_data_change_notification_test()
 ************************************************************/
 /*!*************************************************************
 *  tinkle_cooling_mgmt_data_change_notification_test() 
 ****************************************************************
 * @brief
 *  This function is entry point for cooling_mgmt data and state change 
 *  notification test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  25-Nov-2010 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/
static void
tinkle_cooling_mgmt_data_change_notification_test(void)
{
    DWORD           dwWaitResult;
    fbe_u32_t       object_handle;
    fbe_status_t    status;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    ses_stat_elem_cooling_struct        cooling_status;
    fbe_terminator_sas_encl_info_t      sas_encl;    
    fbe_api_terminator_device_handle_t  encl_device_id;
    fbe_api_terminator_device_handle_t  port_handle;

    /* Get cooling_mgmt object ID*/
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_COOLING_MGMT, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    tinkle_expected_objectId = object_id;
    tinkle_expected_device = FBE_DEVICE_TYPE_FAN;

    mut_printf(MUT_LOG_LOW, "Tinkle test: Started cooling_mgmt notification test...\n");

    /* VERIFY NOTIFICATION FOR COOLING_MGMT DATA CHANGE */
    status = fbe_api_get_enclosure_object_id_by_location(TINKLE_BUS_0, TINKLE_ENCL_7, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);
    
    status = fbe_api_terminator_get_enclosure_handle(TINKLE_BUS_0, TINKLE_ENCL_7, &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Get the Fan Status before introducing the fault */
    status = fbe_api_terminator_get_cooling_eses_status(encl_device_id, COOLING_0, &cooling_status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    tinkle_expected_phys_location.bus = TINKLE_BUS_0;
    tinkle_expected_phys_location.enclosure = TINKLE_ENCL_7;
    tinkle_expected_phys_location.slot = COOLING_0;
    /* Cause the Fan fault on enclosure 0 */
    cooling_status.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    status = fbe_api_terminator_set_cooling_eses_status(encl_device_id, COOLING_0, cooling_status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
  /* Verify that ESP cooling_mgmt detects Data change */
    mut_printf(MUT_LOG_LOW, "Waiting for Cooling Module Faulted...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tinkle_sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != EMCPAL_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_LOW, "Tinkle test: Verified cooling_mgmt data change notification successfully...\n");
    /*  VERIFY NOTIFICATION FOR COOLING_MGMT STATE CHANGE */
    status = fbe_api_terminator_remove_sas_enclosure(encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    tinkle_expected_phys_location.slot = 0;
    /* Verify that ESP cooling_mgmt detects fan state change */
    mut_printf(MUT_LOG_LOW, "Waiting for Cooling Module 0 state change...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tinkle_sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != EMCPAL_STATUS_TIMEOUT);

    tinkle_expected_phys_location.slot = 1;
    /* Verify that ESP ps_mgmt detects fan state change */
    mut_printf(MUT_LOG_LOW, "Waiting for Cooling Module 1 state change...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tinkle_sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != EMCPAL_STATUS_TIMEOUT);

    /* Re-insert the enclosure */
    status = fbe_api_terminator_get_port_handle(TINKLE_BUS_0, &port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    sas_encl.backend_number = TINKLE_BUS_0;
    sas_encl.encl_number = TINKLE_ENCL_7;
    sas_encl.uid[0] = 15; // some unique ID.
    sas_encl.sas_address = 0x5000097A79000000 ;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    status = fbe_api_terminator_insert_sas_enclosure(port_handle, &sas_encl, &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Tinkle test: Verified cooling_mgmt state change notification successfully...\n");

    tinkle_expected_objectId = FBE_OBJECT_ID_INVALID;
    tinkle_expected_device = FBE_DEVICE_TYPE_INVALID;
    mut_printf(MUT_LOG_LOW, "Tinkle test: Notification test for cooling_mgmt passed successfully ...\n");
    return;
}
/************************************************************
 *  end of tinkle_cooling_mgmt_data_change_notification_test()
 ************************************************************/
 /*!**************************************************************
 *  tinkle_esp_object_data_change_callback_function() 
 ****************************************************************
 * @brief
 *  Notification callback function for data change
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  25-Nov-2010 Created Vaibhav Gaonkar
 *  18-Feb-2011 - Vaibhav Gaonkar - Updated to verifiy physical location of
 *                                  notification info.
 *
 ****************************************************************/
static void 
tinkle_esp_object_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                void *context)
{
    fbe_bool_t  get_notificaton = FALSE;
    fbe_semaphore_t *sem = (fbe_semaphore_t *)context;
    fbe_u64_t device_mask;

    device_mask = update_object_msg->notification_info.notification_data.data_change_info.device_mask;

    if (update_object_msg->object_id == tinkle_expected_objectId &&
        (device_mask & tinkle_expected_device) == tinkle_expected_device) 
    {
        switch(tinkle_expected_device)
        {
            case FBE_DEVICE_TYPE_ENCLOSURE:
                if(tinkle_expected_phys_location.bus == update_object_msg->notification_info.notification_data.data_change_info.phys_location.bus &&
                   tinkle_expected_phys_location.enclosure == update_object_msg->notification_info.notification_data.data_change_info.phys_location.enclosure)
                {
                    mut_printf(MUT_LOG_LOW, "Tinkle test: Get the notification from encl_mgmt object...\n");
                    get_notificaton= TRUE;
                }
            break;

            case FBE_DEVICE_TYPE_DRIVE:
                if(tinkle_expected_phys_location.bus == update_object_msg->notification_info.notification_data.data_change_info.phys_location.bus &&
                   tinkle_expected_phys_location.enclosure == update_object_msg->notification_info.notification_data.data_change_info.phys_location.enclosure &&
                   tinkle_expected_phys_location.slot == update_object_msg->notification_info.notification_data.data_change_info.phys_location.slot)
                {
                    mut_printf(MUT_LOG_LOW, "Tinkle test: Get the notification from drive_mgmt object...\n");
                    get_notificaton= TRUE;
                }
            break;

            case FBE_DEVICE_TYPE_SPS:
                if(tinkle_expected_phys_location.bus == update_object_msg->notification_info.notification_data.data_change_info.phys_location.bus &&
                   tinkle_expected_phys_location.enclosure == update_object_msg->notification_info.notification_data.data_change_info.phys_location.enclosure &&
                   tinkle_expected_phys_location.slot == update_object_msg->notification_info.notification_data.data_change_info.phys_location.slot)
                {
                    mut_printf(MUT_LOG_LOW, "Tinkle test: Get the notification from sps_mgmt object...\n");
                    get_notificaton= TRUE;
                }
            break;

            case FBE_DEVICE_TYPE_IOMODULE:
                if(tinkle_expected_phys_location.sp == update_object_msg->notification_info.notification_data.data_change_info.phys_location.sp &&
                   tinkle_expected_phys_location.slot == update_object_msg->notification_info.notification_data.data_change_info.phys_location.slot)
                {
                    mut_printf(MUT_LOG_LOW, "Tinkle test: Get the notification from module_mgmt object...\n");                
                    get_notificaton= TRUE;
                }
            break;

            case FBE_DEVICE_TYPE_PS:
                if(tinkle_expected_phys_location.bus == update_object_msg->notification_info.notification_data.data_change_info.phys_location.bus &&
                   tinkle_expected_phys_location.enclosure == update_object_msg->notification_info.notification_data.data_change_info.phys_location.enclosure &&
                   tinkle_expected_phys_location.slot == update_object_msg->notification_info.notification_data.data_change_info.phys_location.slot)
                {
                    mut_printf(MUT_LOG_LOW, "Tinkle test: Get the notification from ps_mgmt object...\n");                
                    get_notificaton= TRUE;
                }
            break;

            case FBE_DEVICE_TYPE_FAN:
                if(tinkle_expected_phys_location.bus == update_object_msg->notification_info.notification_data.data_change_info.phys_location.bus &&
                   tinkle_expected_phys_location.enclosure == update_object_msg->notification_info.notification_data.data_change_info.phys_location.enclosure &&
                   tinkle_expected_phys_location.slot == update_object_msg->notification_info.notification_data.data_change_info.phys_location.slot)
                {
                    mut_printf(MUT_LOG_LOW, "Tinkle test: Get the notification from cooling_mgmt object...\n");                
                    get_notificaton= TRUE;
                }
            break;
        }
    }
    if(get_notificaton)
    {
        fbe_semaphore_release(sem, 0, 1 ,FALSE);
    }
    return;
}
/*********************************************************
 *  end of tinkle_esp_object_data_change_callback_function
 ********************************************************/
 
