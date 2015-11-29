/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file chanakya_test.c
 ***************************************************************************
 *
 * @brief
 *  This file ensures that environment limit service is working properly.
 *
 * @version
 *   21/09/2010 - Created. Vishnu Sharma
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "mut.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_environment_limit_interface.h"
#include "esp_tests.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

#define MAXBEBUS 2
#define MAXFRUS 250

char * chanakya_short_desc = "Setting and Getting Environment Limits";
char * chanakya_long_desc ="\
This test validates that the values for environment limit service are set and get properly from fbe api. \n\
\n\
\n\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
 STEP 2: Set the Environment Limit Service values from fbe api\n\
 STEP 3: Get Environment Limit Service values from fbe api\n\
        - Verify that these values are the same values we have set in the second step.";



/*!**************************************************************
 * chanakya_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test ESP framework
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  21/09/2010 - Created. Vishnu Sharma
 *
 ****************************************************************/
void chanakya_test(void)
{
    fbe_u32_t status = FBE_STATUS_OK;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_ESP;
    fbe_environment_limits_t env_limits;
    
    env_limits.platform_fru_count = MAXFRUS;
    env_limits.platform_max_be_count = MAXBEBUS;
    
    mut_printf(MUT_LOG_LOW, "Setting platform fru count = :%d  \n",env_limits.platform_fru_count);
    mut_printf(MUT_LOG_LOW, "Setting platform max be count = :%d  \n",env_limits.platform_max_be_count);

    /* Set Environment limit values*/
    status = fbe_api_set_environment_limits(&env_limits,package_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Environment Limit values changed properly");
    
    env_limits.platform_fru_count = 0;
    env_limits.platform_max_be_count = 0;
    
    /* Get Environment limit values*/
    status = fbe_api_get_environment_limits(&env_limits,package_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(MAXFRUS,env_limits.platform_fru_count,"Fru count is not set properly\n");
    MUT_ASSERT_INT_EQUAL_MSG(MAXBEBUS,env_limits.platform_max_be_count,"Platfor max be count is not set properly\n");

    mut_printf(MUT_LOG_LOW, " Platform fru count = :%d  \n",env_limits.platform_fru_count);
    mut_printf(MUT_LOG_LOW, " Platform max be count = :%d  \n",env_limits.platform_max_be_count);

    mut_printf(MUT_LOG_LOW, "chanakya test passed...\n");
    return;
}
/******************************************
 * end chanakya_test()
 ******************************************/
void chanakya_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_FP_CONIG,
                                        SPID_DEFIANT_M1_HW_TYPE,
                                        NULL,
                                        fbe_test_reg_set_persist_ports_true);
}

void chanakya_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 * end file chanakya_test.c
 *************************/

