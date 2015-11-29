/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file travis_bickle_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify the SPS Management Object is created by ESP Framework.
 * 
 * @version
 *   03/09/2010 - Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_sps_info.h"
#include "esp_tests.h"
#include "fbe_test_configurations.h"
#include "fbe_test_esp_utils.h"
#include "EspMessages.h"
#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * travis_bickle_short_desc = "Verify the SPS Management Object is created by ESP Framework";
char * travis_bickle_long_desc ="\
Travis Bickle\n\
        -lead character in the movie TAXI DRIVER\n\
        -famous quote 'You talkin' to me?'\n\
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
STEP 2: Validate SPS Status from SPS Mgmt\n\
        - Verify that the SPS Status reported by SPS Mgmt is good\n\
";


/*!**************************************************************
 * travis_bickle_generic_test() 
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
 *                Updated from Sentry to Oberon
 *
 ****************************************************************/

void travis_bickle_generic_test(void)
{
    fbe_status_t                    status;
    fbe_u32_t                       bobCount;
    fbe_esp_sps_mgmt_bobStatus_t    bobStatus;
    fbe_event_log_msg_count_t       eventLogMsgCount;

    // verify the initial SPS Status (could be OK or TESTING)
//    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_ESP);       // clear ESP event logs
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_esp_sps_mgmt_getBobCount(&bobCount);
    mut_printf(MUT_LOG_LOW, "%s, bobCount %d, status %d\n",
               __FUNCTION__, bobCount, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_zero_memory(&bobStatus, sizeof(fbe_esp_sps_mgmt_bobStatus_t));
    status = fbe_api_esp_sps_mgmt_getBobStatus(&bobStatus);
    mut_printf(MUT_LOG_LOW, "Bbu Inserted %d, BatteryState %d status %d\n", 
        bobStatus.bobInfo.batteryInserted, bobStatus.bobInfo.batteryState, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(bobStatus.bobInfo.batteryInserted);
    MUT_ASSERT_TRUE(bobStatus.bobInfo.batteryState == FBE_BATTERY_STATUS_BATTERY_READY);
    eventLogMsgCount.msg_id = ESP_INFO_SPS_INSERTED;

    eventLogMsgCount.msg_count = 0;
    status = fbe_api_get_event_log_msg_count(&eventLogMsgCount, FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "eventLogMsgCount.msg_count %d\n", 
        eventLogMsgCount.msg_count);
    MUT_ASSERT_TRUE(eventLogMsgCount.msg_count == bobCount);

    return;
}
/******************************************
 * end travis_bickle_generic_test()
 ******************************************/

void travis_bickle_oberon_test(void)
{
    travis_bickle_generic_test();
}

void travis_bickle_hyperion_test(void)
{
    travis_bickle_generic_test();
}

void travis_bickle_triton_test(void)
{
    travis_bickle_generic_test();
}

/*!**************************************************************
 * travis_bickle_test_load_test_env()
 ****************************************************************
 * @brief
 *  This function loads all the test environment.
 *
 * @param platform_type -               
 *
 * @return fbe_status_t -   
 *
 * @author
 *   24-Jan-2011: PHE Created. 
 *
 ****************************************************************/
static void travis_bickle_test_load_test_env(SPID_HW_TYPE platform_type)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_FP_CONIG,
                                        platform_type,
                                        NULL,
                                        fbe_test_reg_set_persist_ports_true);
}   // end of travis_bickle_test_load_test_env

/*!**************************************************************
 * travis_bickle_setup()
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
void travis_bickle_oberon_setup(void)
{
    travis_bickle_test_load_test_env(SPID_OBERON_1_HW_TYPE);
    return;
}

void travis_bickle_hyperion_setup(void)
{
    travis_bickle_test_load_test_env(SPID_HYPERION_1_HW_TYPE);
    return;
}

void travis_bickle_triton_setup(void)
{
    travis_bickle_test_load_test_env(SPID_TRITON_1_HW_TYPE);
    return;
}

/*!**************************************************************
 * travis_bickle_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the travis_bickle test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/

void travis_bickle_destroy(void)
{
    fbe_api_sleep(20000);
    fbe_test_esp_delete_registry_file();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/******************************************
 * end travis_bickle_cleanup()
 ******************************************/
/*************************
 * end file travis_bickle_test.c
 *************************/




