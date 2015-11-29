/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!**************************************************************************
 * @file ravana_ext_peer_boot_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify Extended Peer Boot Logging functionality of board_mgmt object.
 * 
 * @version
 *   06-Sep-2011 Chengkai Hu - Created.
 *
 ***************************************************************************/
 /*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "mut.h"
#include "fbe_test_configurations.h"
#include "esp_tests.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_event_log_utils.h"
#include "spid_types.h"
#include "fbe/fbe_devices.h"
#include "physical_package_tests.h"
#include "fbe/fbe_api_resume_prom_interface.h"
#include "fbe/fbe_peer_boot_utils.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe_test_esp_utils.h"
#include "generic_utils_lib.h"
#include "fbe_base_environment_debug.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

#define RAVANA_PEER_SP_ID           SP_B
#define RAVANA_SLAVE_PORT_ID        0
#define FBE_API_POLLING_INTERVAL    100 /*ms*/

/*************************
 *   TYPE DEFINITIONS
 *************************/
typedef enum {
    RAVANA_PLATFORM_UNKNOWN,
    RAVANA_PLATFORM_MEGATRON,
    RAVANA_PLATFORM_JETFIRE,
}ravana_platform_type_t;

typedef struct ravana_ext_peer_boot_test_s {
    fbe_u8_t                       generalStatus;
    fbe_u8_t                       componentStatus;
    fbe_u8_t                       componentExtStatus;
}ravana_ext_peer_boot_test_t;

/*************************
 *   GLOBAL DEFINITIONS
 *************************/
char * ravana_ext_peer_boot_megatron_short_desc = "Test for Extended Peer Boot Log functionality on Megatron platform";
char * ravana_ext_peer_boot_long_desc ="\
This test validates the Extended Peer Boot Logging by explicitly modifying the slave port status in SPECL \n\
\n\
\n\
STEP 1: Bring up the initial topology for dual SPs.\n\
STEP 2: Validate the Peer Boot Logging ";

fbe_object_id_t             ext_peer_boot_expected_objectId;
fbe_u64_t           ext_peer_boot_device_mask = FBE_DEVICE_TYPE_INVALID;

static ravana_platform_type_t   platformType = RAVANA_PLATFORM_UNKNOWN;
static fbe_semaphore_t          ext_peer_boot_sem;
static fbe_bool_t               isFmea = FBE_FALSE;
static SPECL_SFI_MASK_DATA      orig_sfi_mask_data = {0,};

/*************************
 *   FUNCTION PROTOTYPES
 *************************/
static void ravana_ext_peer_boot_setup(void);
static void ravana_ext_peer_boot_test_start(void);

static void ravana_ext_peer_boot_log_test(ravana_ext_peer_boot_test_t   * extPeerBootTest);
static void ravana_set_slave_port(ravana_ext_peer_boot_test_t   * extPeerBootTest,
                                  SPECL_SLAVE_PORT_STATUS       * slavePortStatus);
static fbe_bool_t ravana_ext_peer_boot_check_log(ravana_ext_peer_boot_test_t   * extPeerBootTest);
static fbe_status_t ravana_ext_peer_boot_check_event(ravana_ext_peer_boot_test_t   * extPeerBootTest,
                                                     fbe_u32_t timeoutMs);
static void ravana_ext_peer_boot_add_test(ravana_ext_peer_boot_test_t * extPeerBootTest,
                                          fbe_u8_t                      generalStatus,
                                          fbe_u8_t                      componentStatus,
                                          fbe_u8_t                      componentExtStatus);
static fbe_status_t ravana_get_specl_slave_port(PSPECL_SFI_MASK_DATA  sfi_mask_data);
static fbe_status_t ravana_set_specl_slave_port(PSPECL_SFI_MASK_DATA sfi_mask_data);
static void ravana_esp_object_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                            void *context);


/*************************
 *   FUNCTION DEFINITIONS
 ************************/
/*!**************************************************************
 *  ravana_ext_peer_boot_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the ravana extended peer boot test
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  06-Sep-2012 Chengkai Hu - Created
 *
 ****************************************************************/
void ravana_ext_peer_boot_test(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;  
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_notification_registration_id_t reg_id;

    mut_printf(MUT_LOG_LOW, "Ravana Extended Peer Boot Log test: Started...\n");

    /* Update fema flag */
    if (fbe_test_esp_util_get_cmd_option("-fmea")) 
    {
        isFmea = FBE_TRUE;
    }
    else
    {
        isFmea = FBE_FALSE;
    }
  
	/* Init the Semaphore */
    fbe_semaphore_init(&ext_peer_boot_sem, 0, 1);

    /* Get the original Slave Port status from SPECL */
    status = ravana_get_specl_slave_port(&orig_sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    /* Register to get event notification from ESP */
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  ravana_esp_object_data_change_callback_function,
                                                                  &ext_peer_boot_sem,
                                                                  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    ext_peer_boot_expected_objectId = object_id;

    mut_printf(MUT_LOG_LOW, "Ravana test: Started Extended Peer Boot Log test...\n");
    ext_peer_boot_device_mask = FBE_DEVICE_TYPE_SLAVE_PORT;
    /* Test the peer Boot logging */
    ravana_ext_peer_boot_test_start();
    ext_peer_boot_device_mask = FBE_DEVICE_TYPE_INVALID;
    mut_printf(MUT_LOG_LOW, "Ravana test: Extended Peer Boot Log test passed successfully ...\n");

    /* Unregistered the notification*/
    status = fbe_api_notification_interface_unregister_notification(ravana_esp_object_data_change_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&ext_peer_boot_sem);
}
/**********************************
 *  end of ravana_ext_peer_boot_test()
 *********************************/ 

/*!**************************************************************
*  ravana_ext_peer_boot_test_start() 
****************************************************************
* @brief
*  This function start to validate the Extended Peer Boot test.
*
* @param None.
*
* @return None.
*
* @author
*  06-Sep-2012 Chengkai Hu - Created
*
****************************************************************/
static void 
ravana_ext_peer_boot_test_start(void)
{
    ravana_ext_peer_boot_test_t extPeerBootTest = {0,};
    
    memset(&extPeerBootTest, 0, sizeof(ravana_ext_peer_boot_test_t));
    ravana_ext_peer_boot_add_test(&extPeerBootTest, 0, SW_COMPONENT_K10DGSSP, SW_FLARE_GetSpid_FAIL);
    ravana_ext_peer_boot_log_test(&extPeerBootTest);
    fbe_api_sleep(5000);

    memset(&extPeerBootTest, 0, sizeof(ravana_ext_peer_boot_test_t));
    ravana_ext_peer_boot_add_test(&extPeerBootTest, 0, SW_COMPONENT_K10DGSSP, SW_APPLICATION_STARTING);
    ravana_ext_peer_boot_log_test(&extPeerBootTest);
    fbe_api_sleep(5000);
}
/***************************************
 *  end of ravana_ext_peer_boot_test_start()
 ***************************************/
/*!**************************************************************
 *  ravana_ext_peer_boot_log_test() 
 ****************************************************************
 * @brief
 *  This function update Slave Port data in SPECL sfi, and 
 *  validate log.
 *
 * @param extPeerBootTest - handle to test data.
 *
 * @return None.
 *
 * @author
 *  06-Sep-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static void
ravana_ext_peer_boot_log_test(ravana_ext_peer_boot_test_t * extPeerBootTest)
{
    DWORD                           dwWaitResult;
    fbe_status_t                    status;
    SPECL_SFI_MASK_DATA             sfi_mask_data;
    SPECL_SLAVE_PORT_STATUS       * slavePortStatus = NULL;

    /* Get the original Slave Port status */
    memcpy(&sfi_mask_data, &orig_sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    /* Update Slave Portwith test data */
    slavePortStatus = &sfi_mask_data.sfiSummaryUnions.slavePortStatus.slavePortSummary[RAVANA_PEER_SP_ID].slavePortStatus[RAVANA_SLAVE_PORT_ID];
    ravana_set_slave_port(extPeerBootTest, slavePortStatus);

    /* Set the Slave Port status to SPECL */
    status = ravana_set_specl_slave_port(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Wait for notification from ESP, 
     * that is get updated in board_mgmt object
     */
    mut_printf(MUT_LOG_LOW, "Ravana test: Waiting for Slave Port status update in board_mgmt object...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&ext_peer_boot_sem, 600000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_LOW, "Ravana test: Waiting for check Slave Port event...\n");
    status = ravana_ext_peer_boot_check_event(extPeerBootTest, 30000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Restore SPECL satus with original info */
    status = ravana_set_specl_slave_port(&orig_sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Ravana test: Verified successfully for general status:0x%x, component:0x%x, component status:0x%x.",
                extPeerBootTest->generalStatus,
                extPeerBootTest->componentStatus,
                extPeerBootTest->componentExtStatus);
}
/***************************************
 *  end of ravana_ext_peer_boot_log_test()
 ***************************************/
/*!**************************************************************
 *  ravana_set_slave_port() 
 ****************************************************************
 * @brief
 * @param extPeerBootTest - handle to test data.
 * @param slavePortStatus - handle to slave port status.
 *
 *
 * @return None.
 *
 * @author
 *  06-Sep-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static void
ravana_set_slave_port(ravana_ext_peer_boot_test_t   * extPeerBootTest,
                      SPECL_SLAVE_PORT_STATUS       * slavePortStatus)
{

    slavePortStatus->generalStatus = extPeerBootTest->generalStatus;
    slavePortStatus->componentStatus = extPeerBootTest->componentStatus;
    slavePortStatus->componentExtStatus = extPeerBootTest->componentExtStatus;
    mut_printf(MUT_LOG_LOW, "Ravana test: Verifying general status 0x%x(%s), component:0x%x(%s), component status:0x%x(%s).\n",
                            slavePortStatus->generalStatus, decodeAmiPort80Status(slavePortStatus->generalStatus),
                            slavePortStatus->componentStatus, decodeSoftwareComponentStatus(extPeerBootTest->componentStatus),
                            slavePortStatus->componentExtStatus, decodeSoftwareComponentExtStatus(extPeerBootTest->componentExtStatus));
}
/*************************************************
 *  end of ravana_set_slave_port()
 ************************************************/

/*!**************************************************************
 *  ravana_ext_peer_boot_check_log() 
 ****************************************************************
 * @brief
 *  This function return whether particular message present in 
 *  event_log or not 
 *
 * @param extPeerBootTest  - handle to fault test info
 *
 * @return None.
 *
 * @author
 *  06-Sep-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static fbe_bool_t
ravana_ext_peer_boot_check_log(ravana_ext_peer_boot_test_t   * extPeerBootTest)
{
    fbe_bool_t                  isMsgPresent = FBE_FALSE;
    fbe_status_t                status;
    fbe_u16_t                   extStatusCode       = 0;

    if (extPeerBootTest->generalStatus == 0x41 || extPeerBootTest->generalStatus == 0x61)
    {
        extStatusCode =  extPeerBootTest->componentStatus;
        extStatusCode |= (extPeerBootTest->componentExtStatus << 8);
    
        status  = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP,
                                              &isMsgPresent,
                                              ESP_INFO_EXT_PEER_BOOT_STATUS_CHANGE,
                                              decodeSpId(RAVANA_PEER_SP_ID),
                                              decodeSoftwareComponentStatus(SW_COMPONENT_POST),
                                              decodePOSTExtStatus(extStatusCode),
                                              extStatusCode); 
    }
    else
    {
        status  = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP,
                                              &isMsgPresent,
                                              ESP_INFO_EXT_PEER_BOOT_STATUS_CHANGE,
                                              decodeSpId(RAVANA_PEER_SP_ID),
                                              decodeSoftwareComponentStatus(extPeerBootTest->componentStatus),
                                              decodeSoftwareComponentExtStatus(extPeerBootTest->componentExtStatus),
                                              extPeerBootTest->componentExtStatus);
    }
    
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Ravana ext peer boot test: Verifying : ....%s",
                isMsgPresent?"OK":".");
        
    return isMsgPresent;
}
/*************************************************
 *  end of ravana_ext_peer_boot_check_log()
 ************************************************/

 /*!**************************************************************
 *  ravana_ext_peer_boot_check_event() 
 ****************************************************************
 * @brief
 *  Check event function for Ravana extended peer boot Test.
 *
 * @param extPeerBootTest  - handle to fault test info
 * @param timeoutMs 
 *
 * @return None.
 *
 * @author
 *  08-Aug-2012 Chengkai Hu - Created 
 *
 ****************************************************************/
static fbe_status_t
ravana_ext_peer_boot_check_event(ravana_ext_peer_boot_test_t   * extPeerBootTest,
                                 fbe_u32_t timeoutMs)
{
    fbe_u32_t   currentTime = 0;
    fbe_bool_t  isMsgPresent = FBE_FALSE;

    while(currentTime < timeoutMs)
    {
       isMsgPresent = ravana_ext_peer_boot_check_log(extPeerBootTest);

       if (isMsgPresent == FBE_TRUE){
            return FBE_STATUS_OK;
        }
        currentTime = currentTime + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
     }
    return FBE_STATUS_GENERIC_FAILURE;
}
/*************************************************
 *  end of ravana_ext_peer_boot_check_event()
 ************************************************/
 /*!**************************************************************
*  ravana_ext_peer_boot_add_test() 
****************************************************************
* @brief
*
* @return None.
*
* @author
*  06-Sep-2012 Chengkai Hu - Created
*
****************************************************************/
static void
ravana_ext_peer_boot_add_test(ravana_ext_peer_boot_test_t * extPeerBootTest,
                              fbe_u8_t                      generalStatus,
                              fbe_u8_t                      componentStatus,
                              fbe_u8_t                      componentExtStatus)
{
    extPeerBootTest->generalStatus = generalStatus;
    extPeerBootTest->componentStatus = componentStatus;
    extPeerBootTest->componentExtStatus = componentExtStatus;
}
/**********************************
 *  end of ravana_ext_peer_boot_add_test()
 *********************************/ 
 /*!**************************************************************
*  ravana_get_specl_slave_port() 
****************************************************************
* @brief
*  This function get the Slave Port info from SPECL.
*
* @param sfi_mask_data - Handle to SPECL_SFI_MASK_DATA
*
* @return fbe_status_t
*
* @author
*  06-Sep-2012 Chengkai Hu - Created
*
****************************************************************/
static fbe_status_t
ravana_get_specl_slave_port(PSPECL_SFI_MASK_DATA  sfi_mask_data)
{
    fbe_status_t status;
    sfi_mask_data->structNumber = SPECL_SFI_SLAVE_PORT_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.sfiMaskStatus = TRUE;

    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    return status;
}
/**************************************
 *  end of ravana_get_specl_slave_port()
 *************************************/
/*!**************************************************************
 *  ravana_set_specl_slave_port() 
 ****************************************************************
 * @brief
 *  This function set the Slave Port info to SPECL.
 *
 * @param sfi_mask_data - Handle to SPECL_SFI_MASK_DATA
  *
 * @return fbe_status_t
 *
 * @author
 *  06-Sep-2012 Chengkai Hu - Created
 *
 ****************************************************************/
fbe_status_t
ravana_set_specl_slave_port(PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    fbe_status_t status;
    
    sfi_mask_data->structNumber = SPECL_SFI_SLAVE_PORT_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.sfiMaskStatus = TRUE;

    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    return status;
}
/**************************************
 *  end of ravana_set_specl_slave_port()
 *************************************/
 /*!**************************************************************
 *  ravana_esp_object_data_change_callback_function() 
 ****************************************************************
 * @brief
 *  Notification callback function for data change
 *
 * @param update_object_msg: Object message
 * @param context: Callback context.               
 *
 * @return None.
 *
 * @author
 *  06-Sep-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static void 
ravana_esp_object_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                void *context)
{
    fbe_semaphore_t *sem = (fbe_semaphore_t *)context;
    fbe_u64_t device_mask;

    device_mask = update_object_msg->notification_info.notification_data.data_change_info.device_mask;
    if (update_object_msg->object_id == ext_peer_boot_expected_objectId) 
    {
        if((device_mask & ext_peer_boot_device_mask) == ext_peer_boot_device_mask) 
        {
            mut_printf(MUT_LOG_LOW, "Ravana test extended peer reboot: Get the notification...\n");
            fbe_semaphore_release(sem, 0, 1 ,FALSE);
        }
    }
    return;
}
/*************************************************************
 *  end of ravana_esp_object_data_change_callback_function()
 ************************************************************/

 /*!**************************************************************
 *  ravana_ext_peer_boot_setup() 
 ****************************************************************
 * @brief
 *  Set up function for Ravana extended peer boot Test.
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  06-Sep-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static void ravana_ext_peer_boot_setup()
{
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");

    if(RAVANA_PLATFORM_MEGATRON == platformType)
    {
        status = fbe_test_load_fp_config(SPID_PROMETHEUS_M1_HW_TYPE);
    }
    else if(RAVANA_PLATFORM_JETFIRE == platformType)
    {
        status = fbe_test_load_fp_config(SPID_DEFIANT_M1_HW_TYPE);
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "Unkown type platform:%d\n", platformType);
    }
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // debug only
    mut_printf(MUT_LOG_LOW, "=== simple config loading is done ===\n");

    fp_test_start_physical_package();

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Initialize registry parameter to persist ports */
    fp_test_set_persist_ports(TRUE);

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();
    mut_printf(MUT_LOG_LOW, "=== sep and neit are started ===\n");

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== ESP has started ===\n");
    fbe_test_wait_for_all_esp_objects_ready();
}
/*************************
 *  end of ravana_ext_peer_boot_setup()
 *************************/

 /*!**************************************************************
 *  ravana_ext_peer_boot_destroy() 
 ****************************************************************
 * @brief
 *  Destroy function for Ravana extended peer boot Test.
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  06-Sep-2012 Chengkai Hu - Created
 *
 ****************************************************************/
void ravana_ext_peer_boot_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 *  end of ravana_ext_peer_boot_destroy()
 *************************/
/*!**************************************************************
 *  ravana_ext_peer_boot_megatron_setup() 
 ****************************************************************
 * @brief
 *  Set up function for Ravana ext peer boot Test for Megatron platform.
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  06-Sep-2012 Chengkai Hu - Created
 *
 ****************************************************************/
void ravana_ext_peer_boot_megatron_setup()
{
    platformType = RAVANA_PLATFORM_MEGATRON;
    ravana_ext_peer_boot_setup();
}
/*******************************************
 *  end of ravana_ext_peer_boot_megatron_setup()
 ********************************************/

