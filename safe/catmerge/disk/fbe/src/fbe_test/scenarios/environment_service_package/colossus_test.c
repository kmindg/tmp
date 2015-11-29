/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file colossus_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify that ESP and SEP packages can run independently of each other,
 *  so that ESP can load in service mode by itself.
 * 
 * @version
 *   05/05/2014 - Created. bphilbin
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_pe_types.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_module_info.h"
#include "esp_tests.h"
#include "fbe_test_configurations.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_file.h"
#include "fp_test.h"
#include "fbe_test_esp_utils.h"
#include "sep_tests.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_esp_common_interface.h"


char * colossus_short_desc = "Verify ESP and SEP can load independently of each other";
char * colossus_long_desc ="\
Colossus\n\
         - is Character from the marvel universe of comic books. \n\
\n\
";

/*!**************************************************************
 * colossus_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   05/05/2014 - Created. bphilbin
 *
 ****************************************************************/
void colossus_test(void)
{
    fbe_status_t status;

    //Loading SEP without ESP, make sure the db service is ready
    status = fbe_test_sep_util_wait_for_database_service(20000);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

}

void colossus_test2(void)
{
    fbe_bool_t is_service_mode = FALSE;
    //Verify ESP thinks it is in service mode
    fbe_api_esp_common_get_is_service_mode(FBE_CLASS_ID_ENCL_MGMT, &is_service_mode);

    MUT_ASSERT_INT_EQUAL(TRUE, is_service_mode);

}

/*!**************************************************************
 * colossus_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   05/05/2014 - Created. bphilbin
 *
 ****************************************************************/
void colossus_setup(void)
{
    fbe_status_t status;


    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE); 
    status = fbe_test_load_fp_config(SPID_PROMETHEUS_M1_HW_TYPE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // debug only
    mut_printf(MUT_LOG_LOW, "=== simple config loading is done ===\n");

    fp_test_start_physical_package();

	/* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();
    mut_printf(MUT_LOG_LOW, "=== sep and neit are started ===\n");

}


/*!****************************************************************************
 * colossus_cleanup()
 ******************************************************************************
 * @brief
 *  Cleanup function.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  5/05/2014 - Created. bphilbin
 *******************************************************************************/
void colossus_cleanup(void)
{
    fbe_status_t status;
    fbe_test_package_list_t package_list;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===", __FUNCTION__);

    fbe_test_sep_util_disable_trace_limits();

    fbe_zero_memory(&package_list, sizeof(fbe_test_package_list_t));

    package_list.number_of_packages = 2;
    package_list.package_list[0] = FBE_PACKAGE_ID_ESP;
    package_list.package_list[1] = FBE_PACKAGE_ID_PHYSICAL;

    fbe_test_common_util_package_destroy(&package_list);
    status = fbe_api_terminator_destroy();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "destroy terminator api failed");

    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);

    mut_printf(MUT_LOG_LOW, "=== %s completes ===", __FUNCTION__);
}

void colossus_unload_sep(void)
{
    fbe_status_t status;
    fbe_test_package_list_t package_list;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===", __FUNCTION__);

    fbe_test_sep_util_disable_trace_limits();

    fbe_zero_memory(&package_list, sizeof(fbe_test_package_list_t));

    package_list.number_of_packages = 3;
    package_list.package_list[0] = FBE_PACKAGE_ID_NEIT;
    package_list.package_list[1] = FBE_PACKAGE_ID_SEP_0;
    package_list.package_list[2] = FBE_PACKAGE_ID_PHYSICAL;

    fbe_test_common_util_package_destroy(&package_list);
    status = fbe_api_terminator_destroy();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "destroy terminator api failed");

    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);

    mut_printf(MUT_LOG_LOW, "=== %s completes ===", __FUNCTION__);
}

void colossus_load_esp(void)
{
    fbe_status_t status;

    status = fbe_test_load_fp_config(SPID_DEFIANT_M1_HW_TYPE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // debug only
    mut_printf(MUT_LOG_LOW, "=== fp config loading is done ===\n");


    status = fbe_test_startPhyPkg(TRUE, FP_MAX_OBJECTS);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //
    mut_printf(MUT_LOG_LOW, "=== phy pkg is started ===\n");

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready

    mut_printf(MUT_LOG_LOW, "=== env mgmt pkg started ===\n");
}

