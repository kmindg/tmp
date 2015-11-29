/***************************************************************************
* Copyright (C) EMC Corporation 2010
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_test_config_loader.c
***************************************************************************
*
* DESCRIPTION
* Functions: init terminator, load packages, setup configurations and unload
*            packages.
* 
* HISTORY
*   09/16/2010:  Created. hud1
*
***************************************************************************/


#include "fbe_cli_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_common_utils.h"


/**************************************************************************
*  fbe_cli_common_setup()
**************************************************************************
*
*  DESCRIPTION:
*    A common setup function to init terminator,setup configuration,
*    init simimuation server and load packages
*
*  PARAMETERS:
*    config_function: configuration function defined for tests.
*
*  RETURN VALUES/ERRORS:
*
*  NOTES:
*
*  HISTORY:
*    09/16/2010:  Created. hud1
**************************************************************************/
fbe_status_t fbe_cli_common_setup(fbe_cli_config_function_t config_function)
{
    fbe_status_t        status;


    /* Before initializing the physical package, initialize the terminator. */
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: terminator initialized.", __FUNCTION__);

    /* Load the test config */
    status = config_function();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/* Initialize the fbe_api on server. */ 
    mut_printf(MUT_LOG_LOW, "%s: starting fbe_api...", __FUNCTION__);
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the physical package. */
    mut_printf(MUT_LOG_LOW, "%s: starting physical package...", __FUNCTION__);
    fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    
    fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}


/**************************************************************************
*  fbe_cli_common_teardown()
**************************************************************************
*
*  DESCRIPTION:
*    A common teardown function to unload packages
*
*  PARAMETERS:
*
*  RETURN VALUES/ERRORS:
*
*  NOTES:
*
*  HISTORY:
*    09/16/2010:  Created. hud1
**************************************************************************/

fbe_status_t fbe_cli_common_teardown(void)
{

    fbe_status_t status;
    fbe_test_package_list_t package_list;
    fbe_zero_memory(&package_list, sizeof(package_list));
	
    mut_printf(MUT_LOG_LOW, "=== %s starts ===", __FUNCTION__);

    package_list.number_of_packages = 1;
    package_list.package_list[0] = FBE_PACKAGE_ID_PHYSICAL;

    fbe_test_common_util_package_destroy(&package_list);
    status = fbe_api_terminator_destroy();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "destroy terminator api failed");

    fbe_test_common_util_verify_package_trace_error(&package_list);

    mut_printf(MUT_LOG_LOW, "=== %s completes ===", __FUNCTION__);


    return status;
}


