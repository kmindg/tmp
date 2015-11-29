/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file bamboo_bmc_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify the event notifications for SP Environment changes (BMC status).
 * 
 * @version
 *   09/19/2012 - Created. Eric Zhou
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_common_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe_test_esp_utils.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * bamboo_bmc_short_desc = "Verify event notification for BMC changes";
char * bamboo_bmc_long_desc ="\
Bamboo_bmc\n\
\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 1 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
STEP 1: Bring up the initial topology.\n\
STEP 2: Validate event notification for BMC Status changes\n\
";

static fbe_class_id_t                       expected_class_id = FBE_CLASS_ID_INVALID;
static SP_ID                                currentSp;

void bamboo_bmc_startupSp(fbe_sim_transport_connection_target_t targetSp,
                        SPID_HW_TYPE platform_type);


static void bamboo_bmc_commmand_response_callback_function (update_object_msg_t * update_object_msg, 
                                                               void *context)
{
    fbe_semaphore_t *callbackSem = (fbe_semaphore_t *)context;

    mut_printf(MUT_LOG_LOW, "=== callback, objId 0x%x, devMsk 0x%llx, classId 0x%x===",
               update_object_msg->object_id,
               update_object_msg->notification_info.notification_data.data_change_info.device_mask,
               update_object_msg->notification_info.class_id);

    if ((update_object_msg->notification_info.notification_data.data_change_info.device_mask == FBE_DEVICE_TYPE_BMC) &&
        (update_object_msg->notification_info.class_id == expected_class_id))
    {
        mut_printf(MUT_LOG_LOW, "%s, FBE_DEVICE_TYPE_SP_ENVIRON_STATE detected, release semaphore\n", __FUNCTION__);
        fbe_semaphore_release(callbackSem, 0, 1 ,FALSE);
    }
}

void bamboo_bmc_updateBmcFault(SP_ID spid, fbe_u32_t bmcIndex, fbe_bool_t bmcFault)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;

    mut_printf(MUT_LOG_LOW, "  %s, sp %d, bmc %d, force set bmc shutdownWarning to %d\n",
               __FUNCTION__, spid, bmcIndex, bmcFault);

    sfi_mask_data.structNumber = SPECL_SFI_BMC_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.bmcStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    mut_printf(MUT_LOG_LOW, "  %s, sp %d, bmc %d, shutdownWarning %d\n",
                __FUNCTION__, spid, bmcIndex, sfi_mask_data.sfiSummaryUnions.bmcStatus.bmcSummary[spid].bmcStatus[bmcIndex].shutdownWarning);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.bmcStatus.bmcSummary[spid].bmcStatus[bmcIndex].shutdownWarning = bmcFault;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of bamboo_bmc_updateBmcFault

void bamboo_bmc_testDpeBmcFaults(void)
{
    fbe_status_t                            status;
    DWORD                                   dwWaitResult;
    fbe_semaphore_t                         sem;
    fbe_notification_registration_id_t      regId;
    fbe_device_physical_location_t          bmcLocation;
    fbe_esp_board_mgmt_bmcInfo_t            bmcInfo;

    fbe_semaphore_init(&sem, 0, 1);

    expected_class_id = FBE_CLASS_ID_BASE_BOARD;
    mut_printf(MUT_LOG_LOW, "%s, expected_class_id 0x%x\n", 
               __FUNCTION__, expected_class_id);

    // Initialize to get event notification
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                FBE_TOPOLOGY_OBJECT_TYPE_BOARD,
                                                                bamboo_bmc_commmand_response_callback_function,
                                                                &sem,
                                                                &regId);

    // fault bmc SPA slot 0
    bamboo_bmc_updateBmcFault(SP_A, 0, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for bmc change event (set shutdown warning) ===", __FUNCTION__);
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (5000);

    bmcLocation.bus = 0;
    bmcLocation.enclosure = 0;
    bmcLocation.sp = SP_A;
    bmcLocation.slot = 0;
    bmcInfo.location = bmcLocation;

    status = fbe_api_esp_board_mgmt_getBmcInfo(&bmcInfo);

    MUT_ASSERT_TRUE(bmcInfo.bmcInfo.shutdownWarning == TRUE);
    mut_printf(MUT_LOG_LOW, "=== %s, SP %d bmc %d shutdownWarning: %d ===", 
            __FUNCTION__, bmcLocation.sp, bmcLocation.slot, bmcInfo.bmcInfo.shutdownWarning);

    // fault bmc SPB slot 0
    bamboo_bmc_updateBmcFault(SP_B, 0, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for bmc change event (set shutdown warning) ===", __FUNCTION__);
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (5000);

    bmcLocation.bus = 0;
    bmcLocation.enclosure = 0;
    bmcLocation.sp = SP_B;
    bmcLocation.slot = 0;
    bmcInfo.location = bmcLocation;

    status = fbe_api_esp_board_mgmt_getBmcInfo(&bmcInfo);

    MUT_ASSERT_TRUE(bmcInfo.bmcInfo.shutdownWarning == TRUE);
    mut_printf(MUT_LOG_LOW, "=== %s, SP %d bmc %d shutdownWarning: %d ===", 
            __FUNCTION__, bmcLocation.sp, bmcLocation.slot, bmcInfo.bmcInfo.shutdownWarning);

    // clear fault bmc SPA slot 0
    bamboo_bmc_updateBmcFault(SP_A, 0, FALSE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for bmc change event (set shutdown warning) ===", __FUNCTION__);
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (5000);

    bmcLocation.bus = 0;
    bmcLocation.enclosure = 0;
    bmcLocation.sp = SP_A;
    bmcLocation.slot = 0;
    bmcInfo.location = bmcLocation;

    status = fbe_api_esp_board_mgmt_getBmcInfo(&bmcInfo);

    MUT_ASSERT_TRUE(bmcInfo.bmcInfo.shutdownWarning == FALSE);
    mut_printf(MUT_LOG_LOW, "=== %s, SP %d bmc %d shutdownWarning: %d ===", 
            __FUNCTION__, bmcLocation.sp, bmcLocation.slot, bmcInfo.bmcInfo.shutdownWarning);

    // clear fault bmc SPB slot 0
    bamboo_bmc_updateBmcFault(SP_B, 0, FALSE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for bmc change event (set shutdown warning) ===", __FUNCTION__);
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (5000);

    bmcLocation.bus = 0;
    bmcLocation.enclosure = 0;
    bmcLocation.sp = SP_B;
    bmcLocation.slot = 0;
    bmcInfo.location = bmcLocation;

    status = fbe_api_esp_board_mgmt_getBmcInfo(&bmcInfo);

    MUT_ASSERT_TRUE(bmcInfo.bmcInfo.shutdownWarning == FALSE);
    mut_printf(MUT_LOG_LOW, "=== %s, SP %d bmc %d shutdownWarning: %d ===", 
            __FUNCTION__, bmcLocation.sp, bmcLocation.slot, bmcInfo.bmcInfo.shutdownWarning);

    status = fbe_api_notification_interface_unregister_notification(bamboo_bmc_commmand_response_callback_function,
                                                                    regId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);
}   // end of bamboo_bmc_testDpeBmcFaults

void bamboo_bmc_testXpeBmcFaults(void)
{
    fbe_status_t                            status;
    DWORD                                   dwWaitResult;
    fbe_semaphore_t                         sem;
    fbe_notification_registration_id_t      regId;
    fbe_device_physical_location_t          bmcLocation;
    fbe_esp_board_mgmt_bmcInfo_t            bmcInfo;

    fbe_semaphore_init(&sem, 0, 1);

    expected_class_id = FBE_CLASS_ID_BASE_BOARD;
    mut_printf(MUT_LOG_LOW, "%s, expected_class_id 0x%x\n", 
               __FUNCTION__, expected_class_id);

    // Initialize to get event notification
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                FBE_TOPOLOGY_OBJECT_TYPE_BOARD,
                                                                bamboo_bmc_commmand_response_callback_function,
                                                                &sem,
                                                                &regId);

    // fault bmc SPA slot 0
    bamboo_bmc_updateBmcFault(SP_A, 0, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for bmc change event (set shutdown warning) ===", __FUNCTION__);
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (5000);

    bmcLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
    bmcLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    bmcLocation.sp = SP_A;
    bmcLocation.slot = 0;
    bmcInfo.location = bmcLocation;

    status = fbe_api_esp_board_mgmt_getBmcInfo(&bmcInfo);

    MUT_ASSERT_TRUE(bmcInfo.bmcInfo.shutdownWarning == TRUE);
    mut_printf(MUT_LOG_LOW, "=== %s, SP %d bmc %d shutdownWarning: %d ===", 
            __FUNCTION__, bmcLocation.sp, bmcLocation.slot, bmcInfo.bmcInfo.shutdownWarning);

    // fault bmc SPB slot 0
    bamboo_bmc_updateBmcFault(SP_B, 0, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for bmc change event (set shutdown warning) ===", __FUNCTION__);
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (5000);

    bmcLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
    bmcLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    bmcLocation.sp = SP_B;
    bmcLocation.slot = 0;
    bmcInfo.location = bmcLocation;

    status = fbe_api_esp_board_mgmt_getBmcInfo(&bmcInfo);

    MUT_ASSERT_TRUE(bmcInfo.bmcInfo.shutdownWarning == TRUE);
    mut_printf(MUT_LOG_LOW, "=== %s, SP %d bmc %d shutdownWarning: %d ===", 
            __FUNCTION__, bmcLocation.sp, bmcLocation.slot, bmcInfo.bmcInfo.shutdownWarning);

    // clear fault bmc SPA slot 0
    bamboo_bmc_updateBmcFault(SP_A, 0, FALSE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for bmc change event (set shutdown warning) ===", __FUNCTION__);
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (5000);

    bmcLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
    bmcLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    bmcLocation.sp = SP_A;
    bmcLocation.slot = 0;
    bmcInfo.location = bmcLocation;

    status = fbe_api_esp_board_mgmt_getBmcInfo(&bmcInfo);

    MUT_ASSERT_TRUE(bmcInfo.bmcInfo.shutdownWarning == FALSE);
    mut_printf(MUT_LOG_LOW, "=== %s, SP %d bmc %d shutdownWarning: %d ===", 
            __FUNCTION__, bmcLocation.sp, bmcLocation.slot, bmcInfo.bmcInfo.shutdownWarning);

    // clear fault bmc SPB slot 0
    bamboo_bmc_updateBmcFault(SP_B, 0, FALSE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for bmc change event (set shutdown warning) ===", __FUNCTION__);
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (5000);

    bmcLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
    bmcLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    bmcLocation.sp = SP_B;
    bmcLocation.slot = 0;
    bmcInfo.location = bmcLocation;

    status = fbe_api_esp_board_mgmt_getBmcInfo(&bmcInfo);

    MUT_ASSERT_TRUE(bmcInfo.bmcInfo.shutdownWarning == FALSE);
    mut_printf(MUT_LOG_LOW, "=== %s, SP %d bmc %d shutdownWarning: %d ===", 
            __FUNCTION__, bmcLocation.sp, bmcLocation.slot, bmcInfo.bmcInfo.shutdownWarning);

    status = fbe_api_notification_interface_unregister_notification(bamboo_bmc_commmand_response_callback_function,
                                                                    regId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);
}   // end of bamboo_bmc_testXpeBmcFaults

/*!**************************************************************
 * bamboo_bmc_jetfire_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test BMC Status on Jetfire array.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   09/19/2012 - Created. Eric Zhou
 *
 ****************************************************************/
void bamboo_bmc_jetfire_test(void)
{
    /*
     * Start up other SP
     */
    mut_printf(MUT_LOG_LOW, "  bamboo_bmc(JETFIRE): starting up peer SP\n");
    currentSp = SP_A;
    fbe_test_startEspAndSep(FBE_SIM_SP_A, SPID_DEFIANT_M1_HW_TYPE);

    /*
     * Verify DPE BMC faulted condition detected
     */
    mut_printf(MUT_LOG_LOW, "bamboo_bmc(JETFIRE): SP %d detect BMC faults\n", currentSp);
    bamboo_bmc_testDpeBmcFaults();
    mut_printf(MUT_LOG_LOW, "bamboo_bmc(JETFIRE): SP %d detect BMC faults - SUCCESSFUL\n", currentSp);

    fbe_api_sleep (5000);

    return;
}
/******************************************
 * end bamboo_bmc_jetfire_test()
 ******************************************/

/*!**************************************************************
 * bamboo_bmc_megatron() 
 ****************************************************************
 * @brief
 *  Main entry point to the test BMC Status on Megatron array.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   09/19/2012 - Created. Eric Zhou
 *
 ****************************************************************/

void bamboo_bmc_megatron_test(void)
{

    /*
     * Start up other SP
     */
    mut_printf(MUT_LOG_LOW, "  bamboo_bmc(MEGATRON): starting up peer SP\n");
    currentSp = SP_A;
    fbe_test_startEspAndSep(FBE_SIM_SP_A, SPID_PROMETHEUS_M1_HW_TYPE);

    /*
     * Verify xPE BMC faulted condition detected
     */
    mut_printf(MUT_LOG_LOW, "bamboo_bmc(MEGATRON): SP %d detect BMC faults\n", currentSp);
    bamboo_bmc_testXpeBmcFaults();
    mut_printf(MUT_LOG_LOW, "bamboo_bmc(MEGATRON): SP %d detect BMC faults - SUCCESSFUL\n", currentSp);

    fbe_api_sleep (5000);
    return;
}
/******************************************
 * end bamboo_bmc_megatron()
 ******************************************/

void bamboo_bmc_startupSp(fbe_sim_transport_connection_target_t targetSp,
                        SPID_HW_TYPE platform_type)
{
    fbe_status_t status;

    /*
     * we do the setup on SPA side
     */
    mut_printf(MUT_LOG_LOW, "bamboo_bmc: %s %s, %s\n",
               __FUNCTION__, 
               (targetSp == FBE_SIM_SP_A ? "SPA" : "SPB"),
               (platform_type == SPID_DEFIANT_M1_HW_TYPE ? "Jetfire" : "Megatron"));
    fbe_api_sim_transport_set_target_server(targetSp);

    status = fbe_test_load_fp_config(platform_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fp_test_start_physical_package();

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Load sep and neit packages */
    sep_config_load_esp_sep_and_neit();
    fbe_test_wait_for_all_esp_objects_ready();
}

/*!**************************************************************
 * bamboo_bmc_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/19/2012 - Created. Eric Zhou
 *
 ****************************************************************/
void bamboo_bmc_setup(SPID_HW_TYPE platform_type)
{

    currentSp = SP_B;
    fbe_test_startEspAndSep(FBE_SIM_SP_B, platform_type);
}
/**************************************
 * end bamboo_bmc_setup()
 **************************************/

void bamboo_bmc_jetfire_setup()
{
    bamboo_bmc_setup(SPID_DEFIANT_M1_HW_TYPE);
}

void bamboo_bmc_megatron_setup()
{
    bamboo_bmc_setup(SPID_PROMETHEUS_M1_HW_TYPE);
}

/*!**************************************************************
 * bamboo_bmc_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the bamboo_bmc test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   09/19/2012 - Created. Eric Zhou
 *
 ****************************************************************/
void bamboo_bmc_destroy(void)
{
    fbe_test_esp_common_destroy_all_dual_sp();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    return;
}
/******************************************
 * end bamboo_bmc_destroy()
 ******************************************/

/*************************
 * end file bamboo_bmc_test.c
 *************************/
