/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp1_ikkyusan_test.c
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
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_module_info.h"
#include "esp_tests.h"
#include "fbe_test_configurations.h"
#include "fp_test.h"
#include "fbe_test_esp_utils.h"
#include "sep_tests.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * fp1_ikkyusan_short_desc = "Verify the Module Management Object is created by ESP Framework";
char * fp1_ikkyusan_long_desc ="\
Ikkyu San\n\
        -The historical Zen Buddhist monk Ikkyu \n\
        -the story follows his mischievous adventures as a child during his stay at Ankoku Temple.\n\
        -In each episode, Ikkyu relies on his intelligence and wit to solve all types of problems, \n\
        -from distraught farmers to greedy merchants.\n\
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
STEP 2: Validate Module State from Module Mgmt\n\
        - Verify that the Module State reported by Module Mgmt is enabled\n\
";

/*!**************************************************************
 * fp1_ikkyusan_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   04/20/2010 - Created. Brion Philbin
 *
 ****************************************************************/

void fp1_ikkyusan_test(void)
{
    fbe_status_t                    status;
    fbe_esp_module_mgmt_module_status_t       module_status_info;

    //specify to get information about IO Module 0 on SP A
    module_status_info.header.sp = 0;
    module_status_info.header.type = FBE_DEVICE_TYPE_BACK_END_MODULE;
    module_status_info.header.slot = 0;


    status = fbe_api_esp_module_mgmt_get_module_status(&module_status_info);
    mut_printf(MUT_LOG_LOW, "Module State 0x%x, status 0x%x\n", 
        module_status_info.state, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(module_status_info.state, MODULE_STATE_ENABLED);
    MUT_ASSERT_INT_EQUAL(module_status_info.substate, MODULE_SUBSTATE_GOOD);

    return;
}
/******************************************
 * end fp1_ikkyusan_test()
 ******************************************/
/*!**************************************************************
 * fp1_ikkyusan_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   04/20/2010 - Created. Brion Philbin
 *
 ****************************************************************/
void fp1_ikkyusan_setup(void)
{
    fbe_status_t status;
    fbe_test_load_fp_config(SPID_DEFIANT_M1_HW_TYPE);

    fp_test_start_physical_package();

	/* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();
}
/**************************************
 * end fp1_ikkyusan_setup()
 **************************************/

/*!**************************************************************
 * fp1_ikkyusan_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the fp1_ikkyusan test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   04/20/2010 - Created. Brion Philbin
 *
 ****************************************************************/

void fp1_ikkyusan_destroy(void)
{
    fbe_test_esp_common_destroy_all();
    return;
}
/******************************************
 * end fp1_ikkyusan_cleanup()
 ******************************************/
/*************************
 * end file fp1_ikkyusan_test.c
 *************************/


