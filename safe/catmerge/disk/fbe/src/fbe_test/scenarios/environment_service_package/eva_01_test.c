/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file eva_01_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify B side configuration page for Voyager and Derringer.
 * 
 * @version
 *   10/25/2011 - Created. dongz
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
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_sps_info.h"
#include "pp_utils.h"

#include "fbe_test_configurations.h"
#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   GLOBALS
 *************************/

char * eva_01_short_desc = "Verify B side configuration page for Voyager and Derringer.";
char * eva_01_long_desc ="\
EVA 01\n\
        -The machine in Neon Genesis Evangelion, drive by IKARI SHINJI. \n\
\n\
Using Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n\
   1) initializes the terminator\n\
   2) loads the test configuration\n\
            a) inserts a board\n\
            b) inserts a port\n\
            c) inserts an enclosure to port 0\n\
            d) inserts 12 sas drives to port 0 encl 0\n\
\n\
   3) Initializes the physical package\n\
   4) initializes fbe_api\n\
   5) wait for the expected number of objects to be created\n\
   6) checks for the fleet board inserted\n\
   7) validates the ports added\n\
        - Loops through all the enclosures and their drives to check their state\n\
            a) Validates that the enclosure is in READY/ACTIVATE state\n\
            b) Validate that the physical drives are in READY/ACTIVATE state\n\
            c) Validate that the logical drives are in READY/ACTIVATE state\n\
        - if the number of enclosures is > 1 (i.e. other than 0th enclosure),\n\
          Get the handle of the port and validate enclosure exists or not\n\
\n\
   8) verifies the number of objects created.\n";

typedef enum eva_01_test_number_e
{
    EVA_01_TEST_DERRINGER,
    EVA_01_TEST_VOYAGER,
}eva_01_test_number_t;

/*!*******************************************************************
 * @var eva_01_test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has
 * to be executed.
 *********************************************************************/
fbe_test_params_t eva_01_test_table[] =
{
    {EVA_01_TEST_DERRINGER,
     "DERRINGER",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_DERRINGER,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_DERRINGER,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {EVA_01_TEST_VOYAGER,
     "VOYAGER",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_VOYAGER,
     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
};

#define EVA_01_TEST_MAX     sizeof(eva_01_test_table)/sizeof(eva_01_test_table[0])

void eva_01_test_setup(eva_01_test_number_t testNum);

/*!**************************************************************
 * eva_01_test_table_driven()
 ****************************************************************
 * @brief
 *  Load and verify B side configuration page by test case, and unload when finished.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   10/25/2011 - Created. dongz
 *
 ****************************************************************/

void eva_01_test_table_driven(fbe_test_params_t *test)
{
    fbe_status_t status;
    fbe_terminator_device_count_t   dev_counts;

    mut_printf(MUT_LOG_LOW, "=== eva_01_test Loading Physical Config on SPA ===");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    status = fbe_test_calc_object_count(&dev_counts);    /* Count number of objects in Terminator */
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "object number on SPA: %d", dev_counts.total_devices);

    status = fbe_test_startPhyPkg(TRUE, dev_counts.total_devices);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();
    mut_printf(MUT_LOG_LOW, "=== sep and neit are started ===\n");

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== ESP has started ===\n");


    fbe_test_wait_for_all_esp_objects_ready();

    /*
     * we do the setup on SPB side
     */
    mut_printf(MUT_LOG_LOW, "=== eva_01_test Loading Physical Config on SPB ===");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    status = fbe_test_calc_object_count(&dev_counts);    /* Count number of objects in Terminator */
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "object number on SPB: %d", dev_counts.total_devices);

    status = fbe_test_startPhyPkg(TRUE, dev_counts.total_devices);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();
    mut_printf(MUT_LOG_LOW, "=== sep and neit are started ===\n");

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== ESP has started ===\n");

    

    fbe_test_wait_for_all_esp_objects_ready();
}
/******************************************
 * end eva_01_test_table_driven()
 ******************************************/

/*!**************************************************************
 * eva_01_derringer_test()
 ****************************************************************
 * @brief
 *  Main entry point to the Derringer test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   02/22/2012 - Created.  Joe Perry
 *
 ****************************************************************/

void eva_01_derringer_test(void)
{
    eva_01_test_table_driven(&eva_01_test_table[EVA_01_TEST_DERRINGER]);
    return;
}
/******************************************
 * end eva_01_derringer_test()
 ******************************************/

/*!**************************************************************
 * eva_01_voyager_test()
 ****************************************************************
 * @brief
 *  Main entry point to the Voyager test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   02/22/2012 - Created.  Joe Perry
 *
 ****************************************************************/

void eva_01_voyager_test(void)
{
    eva_01_test_table_driven(&eva_01_test_table[EVA_01_TEST_VOYAGER]);
    return;
}
/******************************************
 * end eva_01_voyager_test()
 ******************************************/

/*!**************************************************************
 * eva_01_derringer_test_setup()
 ****************************************************************
 * @brief
 *  Setup function for Derringer test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   02/22/2012 - Created.  Joe Perry
 *
 ****************************************************************/

void eva_01_derringer_test_setup(void)
{
    eva_01_test_setup(EVA_01_TEST_DERRINGER);
}
/******************************************
 * end eva_01_derringer_test_setup()
 ******************************************/

/*!**************************************************************
 * eva_01_voyager_test_setup()
 ****************************************************************
 * @brief
 *  Setup function for Voyager test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   02/22/2012 - Created.  Joe Perry
 *
 ****************************************************************/

void eva_01_voyager_test_setup(void)
{
    eva_01_test_setup(EVA_01_TEST_VOYAGER);
}
/******************************************
 * end eva_01_voyager_test_setup()
 ******************************************/

/*!**************************************************************
 * eva_01_test_setup()
 ****************************************************************
 * @brief
 *  Load and verify B side configuration page by test case, and unload when finished.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   02/22/2012 - Created.  Joe Perry
 *
 ****************************************************************/

void eva_01_test_setup(eva_01_test_number_t testNum)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");

    /*
     * we do the setup on SPA side
     */
    mut_printf(MUT_LOG_LOW, "=== eva_01_test Loading Physical Config on SPA ===");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_load_maui_config(SPID_DEFIANT_M1_HW_TYPE, &eva_01_test_table[testNum]);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*
     * we do the setup on SPB side
     */
    mut_printf(MUT_LOG_LOW, "=== eva_01_test Loading Physical Config on SPB ===");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_load_maui_config(SPID_DEFIANT_M1_HW_TYPE, &eva_01_test_table[testNum]);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}
/******************************************
 * end eva_01_test_setup()
 ******************************************/

/*!**************************************************************
 * eva_01_test_destroy()
 ****************************************************************
 * @brief
 *  Load and verify B side configuration page by test case, and unload when finished.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   10/25/2011 - Created. dongz
 *
 ****************************************************************/

void eva_01_test_destroy(void)
{
    fbe_test_esp_common_destroy_all_dual_sp();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
}
/******************************************
 * end eva_01_test_destroy()
 ******************************************/

/*************************
 * end file eva_01_test.c
 *************************/
