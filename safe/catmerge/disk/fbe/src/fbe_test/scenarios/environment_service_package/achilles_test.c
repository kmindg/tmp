/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file achilles_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify I/O Module & I/O Port LED behavior.
 * 
 * @version
 *   03/04/2011 - Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_esp_cooling_mgmt_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_ps_info.h"
#include "esp_tests.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_esp_drive_mgmt.h"
#include "pp_utils.h"  
#include "fbe/fbe_api_event_log_interface.h"
#include "EspMessages.h"

#include "fbe_test_esp_utils.h"
#include "sep_tests.h"
#include "fp_test.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * achilles_short_desc = "Verify I/O Module & I/O Port LED behavior";
char * achilles_long_desc ="\
\n\
Achilles\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
        - Start the foll. service in user space:\n\
        - 1. Memory Service\n\
        - 2. Scheduler Service\n\
        - 3. Eventlog service\n\
        - 4. FBE API \n\
        - Verify that the foll. classes are loaded and initialized\n\
        - 1. ERP\n\
        - 2. Enclosure Firmware Download\n\
        - 3. PS Management\n\
        - 4. Flexports\n\
        - 5. EIR\n\
STEP 2: Verify ps_mgmt control Enclosure Fault LED correctly\n\
        - fault a DAE PS (Enclosure Fault LED On)\n\
        - remove DAE PS fault (Enclosure Fault LED Off)\n\
        - remove DAE PS (Enclosure Fault LED On)\n\
        - insert DAE PS (Enclosure Fault LED Off)\n\
STEP 3: Verify drive_mgmt control Enclosure Fault LED correctly\n\
        - fault a DAE drive (Enclosure Fault LED On)\n\
        - remove DAE drive fault (Enclosure Fault LED Off)\n\
";

#define achilles_LIFECYCLE_NOTIFICATION_TIMEOUT 15000  // 15 sec


static fbe_object_id_t          expected_object_id = FBE_OBJECT_ID_INVALID;
static fbe_class_id_t           expected_class_id = FBE_CLASS_ID_INVALID;
static fbe_u64_t        expected_device_type = FBE_DEVICE_TYPE_INVALID;
static fbe_device_physical_location_t expected_phys_location = {0};
static fbe_notification_registration_id_t   reg_id;
static fbe_semaphore_t 	                    sem;


static void achilles_commmand_response_callback_function (update_object_msg_t * update_object_msg, 
                                                          void *context)
{
	fbe_semaphore_t 	*semPtr = (fbe_semaphore_t *)context;

	if (update_object_msg->object_id | expected_object_id) 
	{
	    if (update_object_msg->notification_info.notification_data.data_change_info.device_mask | 
            expected_device_type) 
	    {
            if (expected_phys_location.slot == 
                update_object_msg->notification_info.notification_data.data_change_info.phys_location.slot)
            {
                mut_printf(MUT_LOG_LOW, "=== callback, objId 0x%x, exp 0x%x, device 0x%llx, exp 0x%llx ===\n",
                    update_object_msg->object_id, 
                    expected_object_id,
                    update_object_msg->notification_info.notification_data.data_change_info.device_mask, 
                    expected_device_type);
                fbe_semaphore_release(semPtr, 0, 1 ,FALSE);
            }
        }
	}
}

void achilles_testIomFaultLed(fbe_object_id_t objectId,
                              SP_ID sp,
                              fbe_u64_t deviceType,
                              fbe_u32_t ioModNum)
{
    fbe_esp_module_io_module_info_t             ioModuleInfo = {0};
    fbe_status_t                                fbeStatus = FBE_STATUS_OK;
    SPECL_SFI_MASK_DATA                         sfi_mask_data;
    fbe_status_t   					        		dwWaitResult;
    fbe_u32_t                                   sfiIoModNum;
    fbe_u32_t                                   index;

    expected_class_id = FBE_CLASS_ID_MODULE_MGMT;
    expected_device_type = deviceType;
    expected_phys_location.slot = ioModNum;
    sfiIoModNum = ioModNum;
//    expected_phys_location.slot = sfiIoModNum;

    /*
     * Verify that IO Port is good
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s, verify initial IOM %d LedStatus", 
               __FUNCTION__, ioModNum);
    ioModuleInfo.header.sp = sp;
    ioModuleInfo.header.type = deviceType;
    ioModuleInfo.header.slot = ioModNum;
    fbeStatus = fbe_api_esp_module_mgmt_getIOModuleInfo(&ioModuleInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(ioModuleInfo.io_module_phy_info.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
    MUT_ASSERT_TRUE(ioModuleInfo.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);
    MUT_ASSERT_TRUE(ioModuleInfo.io_module_phy_info.faultLEDStatus == LED_BLINK_OFF);
    MUT_ASSERT_TRUE(ioModuleInfo.io_module_phy_info.powerStatus == FBE_POWER_STATUS_POWER_ON);
    mut_printf(MUT_LOG_TEST_STATUS, "  IOM %d, inserted %d, faultLEDStatus %d, powerStatus %d\n", 
               ioModNum,
               ioModuleInfo.io_module_phy_info.inserted,
               ioModuleInfo.io_module_phy_info.faultLEDStatus,
               ioModuleInfo.io_module_phy_info.powerStatus);
    mut_printf(MUT_LOG_TEST_STATUS, "Success\n");

    /*
     * Fault IOM
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s, fault IOM %d & verify Fault\n", 
               __FUNCTION__, ioModNum);
    fbe_zero_memory(&sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));
    sfi_mask_data.structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[sfiIoModNum].type1.powerUpFailure = TRUE;
    sfi_mask_data.sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[sfiIoModNum].type1.powerGood = FALSE;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    mut_printf(MUT_LOG_TEST_STATUS, "  IOM %d, powerUpFailure %d, powerGood %d\n", 
               ioModNum,
               sfi_mask_data.sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[sfiIoModNum].type1.powerUpFailure,
               sfi_mask_data.sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[sfiIoModNum].type1.powerGood);
    mut_printf(MUT_LOG_TEST_STATUS, "Success\n");

    // Verify that ESP module_mgmt detects IOM Info change
    for (index = 0; index < 7; index++)
    {
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 1000);
        if (dwWaitResult == FBE_STATUS_TIMEOUT)
        {
            break;
        }
    }

    fbe_api_sleep (10000);

    /*
     * Verify Fault LED
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s, fault IOM %d & verify Fault LED turned ON\n", 
               __FUNCTION__, ioModNum);
    ioModuleInfo.header.sp = sp;
    ioModuleInfo.header.type = deviceType;
    ioModuleInfo.header.slot = ioModNum;
    fbeStatus = fbe_api_esp_module_mgmt_getIOModuleInfo(&ioModuleInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(ioModuleInfo.io_module_phy_info.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
    MUT_ASSERT_TRUE(ioModuleInfo.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);     
    MUT_ASSERT_TRUE(ioModuleInfo.io_module_phy_info.faultLEDStatus == LED_BLINK_ON);
//    MUT_ASSERT_TRUE(ioModuleInfo.io_module_phy_info.powerStatus == FBE_POWER_STATUS_POWERUP_FAILED);   
    mut_printf(MUT_LOG_TEST_STATUS, "  IOM %d, inserted %d, faultLEDStatus %d, envInterfaceStatus %d\n", 
               ioModNum,
               ioModuleInfo.io_module_phy_info.inserted,
               ioModuleInfo.io_module_phy_info.faultLEDStatus,
               ioModuleInfo.io_module_phy_info.envInterfaceStatus);
    mut_printf(MUT_LOG_TEST_STATUS, "Success\n");

    /*
     * Clear IOM Fault
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s, fault IOM %d & verify Fault LED cleared\n", 
               __FUNCTION__, ioModNum);
    sfi_mask_data.structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[sfiIoModNum].type1.powerUpFailure = FALSE;
    sfi_mask_data.sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[sfiIoModNum].type1.powerGood = TRUE;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    mut_printf(MUT_LOG_TEST_STATUS, "  IOM %d, powerUpFailure %d, powerGood %d\n", 
               ioModNum,
               sfi_mask_data.sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[sfiIoModNum].type1.powerUpFailure,
               sfi_mask_data.sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[sfiIoModNum].type1.powerGood);
    mut_printf(MUT_LOG_TEST_STATUS, "Success\n");

    // Verify that ESP module_mgmt detects IOM Info change
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 10000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    fbe_api_sleep (10000);

    expected_class_id = FBE_CLASS_ID_INVALID;
    expected_device_type = FBE_DEVICE_TYPE_INVALID;

}   // end of achilles_testIomFaultLed

void achilles_testIoPortFault (fbe_object_id_t objectId,
                               SP_ID sp,
                               fbe_u32_t ioModNum,
                               fbe_u32_t ioPortNum)
{
    fbe_board_mgmt_io_port_info_t               ioPortInfo;
    fbe_status_t                                fbeStatus = FBE_STATUS_OK;
    fbe_status_t                                status = FBE_STATUS_OK;
//    SPECL_SFI_MASK_DATA                         sfi_mask_data;
    fbe_status_t 				        		dwWaitResult;
    fbe_object_id_t                             portObjectId;
    fbe_cpd_shim_sfp_media_interface_info_t     sfp_info;
    fbe_port_sfp_info_t                         portSfpInfo;
//    fbe_api_terminator_device_handle_t          port_handle;
    fbe_api_terminator_device_handle_t          port0_handle;

    /*
     * Verify that IO Port is good
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s, verify I/O Port %d_%d\n", 
               __FUNCTION__, ioModNum, ioPortNum);
    ioPortInfo.associatedSp = sp;
    ioPortInfo.slotNumOnBlade = ioModNum;
    ioPortInfo.portNumOnModule = ioPortNum;
    ioPortInfo.deviceType = FBE_DEVICE_TYPE_IOMODULE;

    fbeStatus = fbe_api_board_get_io_port_info(objectId, &ioPortInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(ioPortInfo.present == TRUE);
    MUT_ASSERT_TRUE(ioPortInfo.powerStatus == FBE_POWER_STATUS_POWER_ON);
//    MUT_ASSERT_TRUE(ioPortInfo.ioPortLED == TRUE);
//    MUT_ASSERT_TRUE(ioPortInfo.ioPortLEDColor == TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "  I/O Port %d_%d, present %d, powerStatus %d\n",
               ioModNum, ioPortNum, ioPortInfo.present, ioPortInfo.powerStatus);
    mut_printf(MUT_LOG_TEST_STATUS, "  I/O Port %d_%d, ioPortLED %d, ioPortLEDColor %d\n",
               ioModNum, ioPortNum, ioPortInfo.ioPortLED, ioPortInfo.ioPortLEDColor);

    fbeStatus = fbe_api_get_port_object_id_by_location(ioPortNum, &portObjectId);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(portObjectId != FBE_OBJECT_ID_INVALID);

    fbeStatus = fbe_api_get_port_sfp_information(portObjectId, &portSfpInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "  SFP, condition_type %d, sub_condition_type %d\n",
               portSfpInfo.condition_type, portSfpInfo.sub_condition_type);
    MUT_ASSERT_TRUE(portSfpInfo.condition_type == FBE_PORT_SFP_CONDITION_GOOD);
    MUT_ASSERT_TRUE(portSfpInfo.sub_condition_type == FBE_PORT_SFP_SUBCONDITION_NONE);

    /*
     * Fault IO Port
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s, fault I/O Port %d_%d (fault SFP)\n", 
               __FUNCTION__, ioModNum, ioPortNum);
#if TRUE
    status = fbe_api_terminator_get_port_handle(0, &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

//    mut_printf(MUT_LOG_TEST_STATUS, "Setting SFP status to FAULTED...");
    fbe_zero_memory(&sfp_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
    sfp_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_FAULT;
    sfp_info.condition_additional_info = 8196; // unqual part
    fbeStatus = fbe_api_terminator_set_sfp_media_interface_info(port0_handle,&sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "  I/O Port fault - Success\n");
#else
    sfi_mask_data.structNumber = SPECL_SFI_SFP_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.sfpStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    if (sfi_mask_data.sfiSummaryUnions.sfpStatus.sfpSummary[sp].sfpStatus[sfiIoModNum].sfpStatus[ioPortNum].type == 1)
    {
        sfi_mask_data.sfiSummaryUnions.sfpStatus.sfpSummary[sp].sfpStatus[sfiIoModNum].sfpStatus[ioPortNum].type1.rxLossOfSignal = TRUE;
        sfi_mask_data.sfiSummaryUnions.sfpStatus.sfpSummary[sp].sfpStatus[sfiIoModNum].sfpStatus[ioPortNum].type1.txDisable = TRUE;
    }
//    sfi_mask_data.sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[sfiIoModNum].ioControllerInfo[0].ioPortInfo[ioPortNum] = FALSE;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    mut_printf(MUT_LOG_TEST_STATUS, "Success\n");
#endif

    // Verify that ESP module_mgmt detects IOM Info change
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 10000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    fbe_api_sleep (5000);

    /*
     * Verify that IO Port is good
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s, verify I/O Port %d_%d LED\n", 
               __FUNCTION__, ioModNum, ioPortNum);

    fbeStatus = fbe_api_board_get_io_port_info(objectId, &ioPortInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "  I/O Port %d_%d, present %d, powerStatus %d\n",
               ioModNum, ioPortNum, ioPortInfo.present, ioPortInfo.powerStatus);
    MUT_ASSERT_TRUE(ioPortInfo.present == TRUE);
    MUT_ASSERT_TRUE(ioPortInfo.powerStatus == FBE_POWER_STATUS_POWER_ON);
//    MUT_ASSERT_TRUE(ioPortInfo.ioPortLED == TRUE);
//    MUT_ASSERT_TRUE(ioPortInfo.ioPortLEDColor == TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "Success\n");
    mut_printf(MUT_LOG_TEST_STATUS, "  I/O Port %d_%d, ioPortLED %d, ioPortLEDColor %d\n",
               ioModNum, ioPortNum, ioPortInfo.ioPortLED, ioPortInfo.ioPortLEDColor);

    fbeStatus = fbe_api_get_port_sfp_information(portObjectId, &portSfpInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "  SFP, condition_type %d, sub_condition_type %d\n",
               portSfpInfo.condition_type, portSfpInfo.sub_condition_type);
    MUT_ASSERT_TRUE(portSfpInfo.condition_type == FBE_PORT_SFP_CONDITION_FAULT);
    MUT_ASSERT_TRUE(portSfpInfo.sub_condition_type == FBE_PORT_SFP_SUBCONDITION_NONE);

}   // end of achilles_testIoPortFault

void achilles_testIoPortMarking(fbe_object_id_t objectId,
                                SP_ID sp,
                                fbe_u32_t ioModNum,
                                fbe_u32_t ioPortNum)
{
    fbe_status_t                            fbeStatus = FBE_STATUS_OK;
    fbe_esp_module_mgmt_mark_port_t         portMarkCmd;

    fbe_zero_memory(&portMarkCmd, sizeof(fbe_esp_module_mgmt_mark_port_t));

    /*
     * Mark the specified I/O Port
     */
//    portMarkCmd.iomHeader.type = FBE_DEVICE_TYPE_MEZZANINE;
    portMarkCmd.iomHeader.type = FBE_DEVICE_TYPE_IOMODULE;
    portMarkCmd.iomHeader.slot = ioModNum;
    portMarkCmd.iomHeader.port = ioPortNum;
    portMarkCmd.markPortOn = TRUE;
    fbeStatus = fbe_api_esp_module_mgmt_markIoPort(&portMarkCmd);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /*
     * Unmark the specified I/O Port
     */
//    portMarkCmd.iomClass = FBE_MODULE_MGMT_CLASS_MEZZANINE;
//    portMarkCmd.ioModuleNum = ioModNum;
//    portMarkCmd.ioPortNum = ioPortNum;
    portMarkCmd.markPortOn = FALSE;
    fbeStatus = fbe_api_esp_module_mgmt_markIoPort(&portMarkCmd);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

}   // end of achilles_testIoPortMarking

void achilles_testIoModuleMarking(fbe_object_id_t objectId,
                                SP_ID sp,
                                fbe_u32_t ioModNum)
{
    fbe_status_t                            fbeStatus = FBE_STATUS_OK;
    fbe_esp_module_mgmt_mark_module_t       IOMMarkCmd;

    fbe_zero_memory(&IOMMarkCmd, sizeof(fbe_esp_module_mgmt_mark_module_t));

    /*
     * Mark the specified I/O Module
     */
//    portMarkCmd.iomHeader.type = FBE_DEVICE_TYPE_MEZZANINE;
    IOMMarkCmd.iomHeader.type = FBE_DEVICE_TYPE_IOMODULE;
    IOMMarkCmd.iomHeader.slot = ioModNum;
    IOMMarkCmd.markPortOn = TRUE;
    fbeStatus = fbe_api_esp_module_mgmt_markIoModule(&IOMMarkCmd);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /*
     * Unmark the specified I/O Port
     */
//    portMarkCmd.iomClass = FBE_MODULE_MGMT_CLASS_MEZZANINE;
//    portMarkCmd.ioModuleNum = ioModNum;
//    portMarkCmd.ioPortNum = ioPortNum;
    IOMMarkCmd.markPortOn = FALSE;
    fbeStatus = fbe_api_esp_module_mgmt_markIoModule(&IOMMarkCmd);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
}   // end of achilles_testIoPortMarking

/*!**************************************************************
 * achilles_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   02/01/2011 - Created. Joe Perry
 *
 ****************************************************************/
void achilles_test(void)
{
    fbe_status_t                                fbeStatus = FBE_STATUS_OK;
    fbe_object_id_t                             objectId;

    /*
     * Initialize Semaphore for event notification
     */
    fbe_semaphore_init(&sem, 0, 1);
	fbeStatus = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                     FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                     FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                     achilles_commmand_response_callback_function,
                                                                     &sem,
                                                                     &reg_id);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    fbe_api_sleep (5000);

    fbeStatus = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);

    fbeStatus = fbe_api_esp_common_get_object_id_by_device_type(FBE_DEVICE_TYPE_IOMODULE, &expected_object_id);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(expected_object_id != FBE_OBJECT_ID_INVALID);
    mut_printf(MUT_LOG_TEST_STATUS, "IoModule, objId %d\n", expected_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "ACHILLES:Verify I/O Module Fault LED\n");
    achilles_testIomFaultLed(objectId, SP_A, FBE_DEVICE_TYPE_IOMODULE, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "ACHILLES:Verify I/O Module Fault LED - Success\n");

    mut_printf(MUT_LOG_TEST_STATUS, "ACHILLES:Verify I/O Port Fault LED\n");
    achilles_testIoPortFault(objectId, SP_A, 1, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "ACHILLES:Verify I/O Port Fault LED - Success\n");

    mut_printf(MUT_LOG_TEST_STATUS, "ACHILLES:Verify I/O Port Marking\n");
    achilles_testIoPortMarking(objectId, SP_A, 1, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "ACHILLES:Verify I/O Port Marking - Success\n");

    mut_printf(MUT_LOG_TEST_STATUS, "ACHILLES:Verify I/O Module Marking\n");
    achilles_testIoModuleMarking(objectId, SP_A, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "ACHILLES:Verify I/O Module Marking - Success\n");


/*
    mut_printf(MUT_LOG_TEST_STATUS, "ACHILLES:Verify I/O Module Marking\n");
*/
            
    /*
     * Cleanup Semaphore
     */
    fbeStatus = fbe_api_notification_interface_unregister_notification(achilles_commmand_response_callback_function,
                                                                       reg_id);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

    return;
}
/******************************************
 * end achilles_test()
 ******************************************/

fbe_status_t fbe_test_achilles_load_specl_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       slot = 0, sp = 0;
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return status;
    }

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    for(sp=0;sp<2;sp++)
    {
        sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioModuleCount = TOTAL_IO_MOD_PER_BLADE;
        for(slot = 0; slot < TOTAL_IO_MOD_PER_BLADE; slot++)
        {
            if(slot == 0)
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = GLACIER_REV_C;
            }
            else if(slot == 1)
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = MOONLITE;
            }
            else
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = NO_MODULE;
            }
        }
    }
    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return status;
}

/*!**************************************************************
 * achilles_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/
void achilles_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_FP_CONIG,
                                        SPID_DEFIANT_M1_HW_TYPE,
                                        NULL,
                                        NULL);
}
/**************************************
 * end achilles_setup()
 **************************************/

/*!**************************************************************
 * achilles_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the achilles test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/

void achilles_destroy(void)
{
    //fbe_test_esp_common_destroy();
    fbe_test_esp_common_destroy_all();
    return;
}
/******************************************
 * end achilles_cleanup()
 ******************************************/
/*************************
 * end file achilles_test.c
 *************************/


