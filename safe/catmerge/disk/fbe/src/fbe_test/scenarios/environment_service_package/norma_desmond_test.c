/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file norma_desmond_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify the event notifications to SPS Management Object of ESP.
 * 
 * @version
 *   03/24/2010 - Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_sps_info.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"

#include "fbe_test_configurations.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * norma_desmond_short_desc = "Verify event notification of the ESP's SPS Management Object";
char * norma_desmond_long_desc ="\
Norma Desmond\n\
        -lead character in the movie SUNSET BOULEVARD\n\
        -famous quotes 'Im ready for my closeup Mr DeMille.'\n\
                       'Its the pictures that got small.'\n\
\n\
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
        - 3. SPS Management\n\
        - 4. Flexports\n\
        - 5. EIR\n\
STEP 2: Validate event notification for SPS Status changes\n\
        - Verify that \n\
";

static fbe_notification_registration_id_t  reg_id;


static void norma_desmond_commmand_response_callback_function (update_object_msg_t * update_object_msg, 
                                                               void *context)
{
    fbe_semaphore_t     *sem = (fbe_semaphore_t *)context;
    fbe_device_physical_location_t      location = {0};
    fbe_esp_board_mgmt_board_info_t     boardInfo = {0};
    fbe_status_t                        status = FBE_STATUS_OK;

    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    location = update_object_msg->notification_info.notification_data.data_change_info.phys_location;

//    mut_printf(MUT_LOG_LOW, "%s, obj 0x%x, device_mask %d, SPS %d\n",
//               __FUNCTION__,
//               update_object_msg->object_id,
//               update_object_msg->notification_info.notification_data.data_change_info.device_mask, FBE_DEVICE_TYPE_SPS);
    if (update_object_msg->notification_info.notification_data.data_change_info.device_mask == FBE_DEVICE_TYPE_SPS) 
    {
        if (((boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE) && 
            (location.bus == FBE_XPE_PSEUDO_BUS_NUM) && 
            (location.enclosure == FBE_XPE_PSEUDO_ENCL_NUM)) ||
            ((boardInfo.platform_info.enclosureType != XPE_ENCLOSURE_TYPE) && 
            (location.bus == 0) && 
            (location.enclosure == 0)))
        {
            mut_printf(MUT_LOG_LOW, "NORMA: SPS %d_%d_%d status change notified\n", location.bus, location.enclosure, location.slot);
            fbe_semaphore_release(sem, 0, 1 ,FALSE);
        }
    }

}

void norma_desmond_insertRemoveBbu(SP_ID sp, fbe_u32_t bbuIndex, fbe_bool_t bbuInsert)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;

    mut_printf(MUT_LOG_LOW, "%s, update BBU, sp %d, bbuIndex %d, insert %d\n,",
               __FUNCTION__, sp, bbuIndex, bbuInsert);
    sfi_mask_data.structNumber = SPECL_SFI_BATTERY_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.batteryStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.batteryStatus.batterySummary[sp].batteryStatus[bbuIndex].inserted = bbuInsert;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
}

fbe_bool_t norma_desmond_verifyBbuInsertRemove(SP_ID sp, fbe_u32_t bbuIndex, fbe_u32_t bbuCount, fbe_bool_t bbuInsert)
{

    fbe_status_t                            status;
    fbe_esp_sps_mgmt_bobStatus_t            bobStatus;

    fbe_zero_memory(&bobStatus, sizeof(fbe_esp_sps_mgmt_bobStatus_t));
    bobStatus.bobIndex = bbuIndex;
/*
    if (sp == SP_A)
    {
        bobStatus.bobIndex = bbuIndex;
    }
    else
    {
        bobStatus.bobIndex = bbuIndex - (bbuCount / 2); 
    }
*/
    status = fbe_api_esp_sps_mgmt_getBobStatus(&bobStatus);
    mut_printf(MUT_LOG_LOW, "%s, BBU %d, Inserted %d, BatteryState %d status %d\n", 
        __FUNCTION__, bobStatus.bobIndex, bobStatus.bobInfo.batteryInserted, bobStatus.bobInfo.batteryState, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    if (bobStatus.bobInfo.batteryInserted == bbuInsert)
    {
        return TRUE;
    }

    return FALSE;
}

/*!**************************************************************
 * norma_desmond_general_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/

void norma_desmond_general_test(void)
{
    fbe_status_t                            status;
    fbe_object_id_t                         objectId;
    fbe_u32_t                               bbuCount, bbuIndex, spBbuIndex, bbuCountPerSp;
    fbe_u32_t                               firstBbuIndex, lastBbuIndex;
	fbe_semaphore_t 						sem;
	DWORD 									dwWaitResult;
    fbe_u32_t                               retryIndex;
    fbe_bool_t                              changeDetected = FALSE;

    fbe_semaphore_init(&sem, 0, 1);

    // Get Board Object ID
    status = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);

    // Initialize to get event notification
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_ESP,
																  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
																  norma_desmond_commmand_response_callback_function,
																  &sem,
																  &reg_id);

    // turn off SPS simulation
    status = fbe_api_esp_sps_mgmt_setSimulateSps(FALSE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_api_sleep (5000);

    // get BBU count for this platform
    status = fbe_api_esp_sps_mgmt_getBobCount(&bbuCount);
    bbuCountPerSp = bbuCount / 2;
    mut_printf(MUT_LOG_LOW, "%s, bbuCount %d, status %d, bbuCountPerSp %d\n",
               __FUNCTION__, bbuCount, status, bbuCountPerSp);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*
     * Test SPA BBU's
     */
    firstBbuIndex = 0;
    lastBbuIndex = bbuCountPerSp - 1;
    mut_printf(MUT_LOG_LOW, "NORMA:Test SPA BBU's (%d to %d)\n",
               firstBbuIndex, lastBbuIndex);
    spBbuIndex = 0;
    for (bbuIndex = firstBbuIndex; bbuIndex <= lastBbuIndex; bbuIndex++)
    {
        mut_printf(MUT_LOG_LOW, "  NORMA:Test SPA BBU%d\n", bbuIndex);
        // Verify first BBU on SPA is present
        mut_printf(MUT_LOG_LOW, "  %s, Verify BBUA Inserted\n", __FUNCTION__);
        MUT_ASSERT_TRUE(norma_desmond_verifyBbuInsertRemove(SP_A, bbuIndex, bbuCount, TRUE));

        // Remove first BBU on SPA
        mut_printf(MUT_LOG_LOW, "  %s, Remove BBUA\n", __FUNCTION__);
        norma_desmond_insertRemoveBbu(SP_A, spBbuIndex, FALSE);
        // Verify first BBU on SPA removed
        changeDetected = FALSE;
        for (retryIndex = 0; retryIndex < 3; retryIndex++)
        {
            // Verify that ESP sps_mgmt detects SPS Status change
            dwWaitResult = fbe_semaphore_wait_ms(&sem, 10000);
        
            mut_printf(MUT_LOG_LOW, "    %s, Check if BBUA Removed, retry %d\n", __FUNCTION__, retryIndex);
            if (norma_desmond_verifyBbuInsertRemove(SP_A, bbuIndex, bbuCount, FALSE))
            {
                changeDetected = TRUE;
                break;
            }
        }
        MUT_ASSERT_TRUE(changeDetected);
        mut_printf(MUT_LOG_LOW, "  NORMA: SPA BBU remove detected\n");

        // Reinsert first BBU on SPA
        mut_printf(MUT_LOG_LOW, "  %s, Insert BBUA\n", __FUNCTION__);
        norma_desmond_insertRemoveBbu(SP_A, spBbuIndex, TRUE);
        // Verify first BBU on SPA inserted
        changeDetected = FALSE;
        for (retryIndex = 0; retryIndex < 3; retryIndex++)
        {
            // Verify that ESP sps_mgmt detects SPS Status change
            dwWaitResult = fbe_semaphore_wait_ms(&sem, 10000);
        
            mut_printf(MUT_LOG_LOW, "    %s, Check if BBUA Inserted, retry %d\n", __FUNCTION__, retryIndex);
            if (norma_desmond_verifyBbuInsertRemove(SP_A, bbuIndex, bbuCount, TRUE))
            {
                changeDetected = TRUE;
                break;
            }
        }
        MUT_ASSERT_TRUE(changeDetected);
        mut_printf(MUT_LOG_LOW, "  NORMA: SPA BBU insert detected\n");

        spBbuIndex++;
    }

    /*
     * Test SPB BBU's
     */
    firstBbuIndex = bbuCountPerSp;
    lastBbuIndex = bbuCount - 1;
    mut_printf(MUT_LOG_LOW, "NORMA:Test SPB BBU's (%d to %d)\n",
               firstBbuIndex, lastBbuIndex);
    spBbuIndex = 0;
    for (bbuIndex = firstBbuIndex; bbuIndex <= lastBbuIndex; bbuIndex++)
    {

        mut_printf(MUT_LOG_LOW, "  NORMA:Test SPB BBU%d\n", bbuIndex);
        // Verify first BBU on SPA is present
        mut_printf(MUT_LOG_LOW, "  %s, Verify BBUB Inserted\n", __FUNCTION__);
        MUT_ASSERT_TRUE(norma_desmond_verifyBbuInsertRemove(SP_B, bbuIndex, bbuCount, TRUE));

        // Remove first BBU on SPA
        mut_printf(MUT_LOG_LOW, "  %s, Remove BBUB\n", __FUNCTION__);
        norma_desmond_insertRemoveBbu(SP_B, spBbuIndex, FALSE);
        // Verify first BBU on SPA removed
        changeDetected = FALSE;
        for (retryIndex = 0; retryIndex < 3; retryIndex++)
        {
            // Verify that ESP sps_mgmt detects SPS Status change
            dwWaitResult = fbe_semaphore_wait_ms(&sem, 10000);
        
            mut_printf(MUT_LOG_LOW, "    %s, Check if BBUB Removed, retry %d\n", __FUNCTION__, retryIndex);
            if (norma_desmond_verifyBbuInsertRemove(SP_B, bbuIndex, bbuCount, FALSE))
            {
                changeDetected = TRUE;
                break;
            }
        }
        MUT_ASSERT_TRUE(changeDetected);
        mut_printf(MUT_LOG_LOW, "  NORMA: SPB BBU remove detected\n");

        // Reinsert first BBU on SPA
        mut_printf(MUT_LOG_LOW, "  %s, Insert BBUB\n", __FUNCTION__);
        norma_desmond_insertRemoveBbu(SP_B, spBbuIndex, TRUE);
        // Verify first BBU on SPA inserted
        changeDetected = FALSE;
        for (retryIndex = 0; retryIndex < 3; retryIndex++)
        {
            // Verify that ESP sps_mgmt detects SPS Status change
            dwWaitResult = fbe_semaphore_wait_ms(&sem, 10000);
        
            mut_printf(MUT_LOG_LOW, "    %s, Check if BBUB Inserted, retry %d\n", __FUNCTION__, retryIndex);
            if (norma_desmond_verifyBbuInsertRemove(SP_B, bbuIndex, bbuCount, TRUE))
            {
                changeDetected = TRUE;
                break;
            }
        }
        MUT_ASSERT_TRUE(changeDetected);
        mut_printf(MUT_LOG_LOW, "  NORMA: SPB BBU insert detected\n");

        spBbuIndex++;
    }


    // cleanup
    status = fbe_api_notification_interface_unregister_notification(norma_desmond_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

    return;
}
/******************************************
 * end norma_desmond_general_test()
 ******************************************/

void norma_desmond_oberon_test(void)
{
    norma_desmond_general_test();
}

void norma_desmond_hyperion_test(void)
{
    norma_desmond_general_test();
}

void norma_desmond_triton_test(void)
{
    norma_desmond_general_test();
}

/*!**************************************************************
 * norma_desmond_oberon_setup()
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
void norma_desmond_oberon_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_FP_CONIG,
                                        SPID_OBERON_1_HW_TYPE,
                                        NULL,
                                        fbe_test_reg_set_persist_ports_true);
}
/**************************************
 * end norma_desmond_oberon_setup()
 **************************************/

/*!**************************************************************
 * norma_desmond_hyperion_setup()
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
void norma_desmond_hyperion_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_FP_CONIG,
                                        SPID_HYPERION_1_HW_TYPE,
                                        NULL,
                                        fbe_test_reg_set_persist_ports_true);
}
/**************************************
 * end norma_desmond_hyperion_setup()
 **************************************/

/*!**************************************************************
 * norma_desmond_triton_setup()
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
void norma_desmond_triton_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_FP_CONIG,
                                        SPID_TRITON_1_HW_TYPE,
                                        NULL,
                                        fbe_test_reg_set_persist_ports_true);
}
/**************************************
 * end norma_desmond_triton_setup()
 **************************************/

/*!**************************************************************
 * norma_desmond_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the norma_desmond test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/

void norma_desmond_cleanup(void)
{
    return;
}
/******************************************
 * end norma_desmond_cleanup()
 ******************************************/
/*************************
 * end file norma_desmond_test.c
 *************************/


