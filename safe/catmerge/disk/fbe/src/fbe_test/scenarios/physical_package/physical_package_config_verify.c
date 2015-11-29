/**************************************************************************
 * Copyright (C) EMC Corporation 2008 - 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
***************************************************************************/

#include "physical_package_tests.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "fbe/fbe_types.h"

char * static_config_short_desc = "static configuration test";
char * static_config_long_desc = "\n\tSetup: create a board, a port, an enclosure and 2 drives in Terminator, start physical Package and fbe_api\n"
                                 "\tStep 1: verify from physical package \n"
                                 "\tTeardown: destroy physical package, fbe api, and terminator\n";

/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/
/*!*******************************************************************
 * @enum static_test_t
 *********************************************************************
 * @brief This is a enum for the test senarios.
 *********************************************************************/


typedef enum static_test_e
{
    STATIC_TEST_VIPER,
    STATIC_TEST_DERRINGER,
    STATIC_TEST_VOYAGER,
    STATIC_TEST_BUNKER,
    STATIC_TEST_CITADEL,
    STATIC_TEST_FALLBACK,
    STATIC_TEST_BOXWOOD,
    STATIC_TEST_KNOT,
    STATIC_TEST_ANCHO,
    STATIC_TEST_VIKING,
    STATIC_TEST_CAYENNE,
    STATIC_TEST_NAGA,
    STATIC_TEST_CALYPSO,
    STATIC_TEST_RHEA,
    STATIC_TEST_MIRANDA,
    STATIC_TEST_TABASCO,
}static_test_t;

static fbe_test_params_t test_table[] = 
{
    {STATIC_TEST_VIPER,              //Test Number
      "VIPER",                               //Title
      FBE_BOARD_TYPE_FLEET,       //Board type
      FBE_PORT_TYPE_SAS_PMC,    //Port type
      1,                                      // io port number
      2,                                      //portal number
      0,                                      //backend number
      FBE_SAS_ENCLOSURE_TYPE_VIPER,     //enclosure type
      0,                                                 //enclosure number
      FBE_SAS_DRIVE_CHEETA_15K,          //Drive type 
      INVALID_DRIVE_NUMBER,                 //Drive number
      MAX_DRIVE_COUNT_VIPER,              //Max drives

      MAX_PORTS_IS_NOT_USED,             //Max ports
      MAX_ENCLS_IS_NOT_USED,             //max enclosures
      VIPER_HIGH_DRVCNT_MASK,            //drive mask
    },
    {STATIC_TEST_DERRINGER,
      "DERRINGER", 
      FBE_BOARD_TYPE_FLEET, 
      FBE_PORT_TYPE_SAS_PMC,
      1,
      2,
      0,
      FBE_SAS_ENCLOSURE_TYPE_DERRINGER,
      0,
      FBE_SAS_DRIVE_CHEETA_15K,
      INVALID_DRIVE_NUMBER,
      MAX_DRIVE_COUNT_DERRINGER,

      MAX_PORTS_IS_NOT_USED,
      MAX_ENCLS_IS_NOT_USED,
      DERRINGER_HIGH_DRVCNT_MASK,
    },
    {STATIC_TEST_BUNKER,
      "BUNKER", 
      FBE_BOARD_TYPE_FLEET, 
      FBE_PORT_TYPE_SAS_PMC,
      1,
      2,
      0,
      FBE_SAS_ENCLOSURE_TYPE_BUNKER,
      0,
      FBE_SAS_DRIVE_CHEETA_15K,
      INVALID_DRIVE_NUMBER,
      MAX_DRIVE_COUNT_BUNKER,
      MAX_PORTS_IS_NOT_USED,
      MAX_ENCLS_IS_NOT_USED,
     BUNKER_HIGH_DRVCNT_MASK,
      
    },
    {STATIC_TEST_CITADEL,
      "CITADEL", 
      FBE_BOARD_TYPE_FLEET, 
      FBE_PORT_TYPE_SAS_PMC,
      1,
      2,
      0,
      FBE_SAS_ENCLOSURE_TYPE_CITADEL,
      0,
      FBE_SAS_DRIVE_CHEETA_15K,
      INVALID_DRIVE_NUMBER,
      MAX_DRIVE_COUNT_CITADEL,
      MAX_PORTS_IS_NOT_USED,
      MAX_ENCLS_IS_NOT_USED,
      CITADEL_HIGH_DRVCNT_MASK,      
    },
    {STATIC_TEST_VOYAGER,
      "VOYAGER",
      FBE_BOARD_TYPE_FLEET, 
      FBE_PORT_TYPE_SAS_PMC,
      1,
      2,
      0,
      FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM,
      0,
      FBE_SAS_DRIVE_CHEETA_15K,
      INVALID_DRIVE_NUMBER,
      MAX_DRIVE_COUNT_VOYAGER,

      MAX_PORTS_IS_NOT_USED,
      MAX_ENCLS_IS_NOT_USED,
      VOYAGER_HIGH_DRVCNT_MASK,
    },
    {STATIC_TEST_FALLBACK,
      "FALLBACK", 
      FBE_BOARD_TYPE_FLEET, 
      FBE_PORT_TYPE_SAS_PMC,
      1,
      2,
      0,
      FBE_SAS_ENCLOSURE_TYPE_FALLBACK,
      0,
      FBE_SAS_DRIVE_CHEETA_15K,
      INVALID_DRIVE_NUMBER,
      MAX_DRIVE_COUNT_FALLBACK,
      MAX_PORTS_IS_NOT_USED,
      MAX_ENCLS_IS_NOT_USED,
      FALLBACK_HIGH_DRVCNT_MASK,
    },
    {STATIC_TEST_BOXWOOD,
      "BOXWOOD", 
      FBE_BOARD_TYPE_FLEET, 
      FBE_PORT_TYPE_SAS_PMC,
      1,
      2,
      0,
      FBE_SAS_ENCLOSURE_TYPE_BOXWOOD,
      0,
      FBE_SAS_DRIVE_CHEETA_15K,
      INVALID_DRIVE_NUMBER,
      MAX_DRIVE_COUNT_BOXWOOD,
      MAX_PORTS_IS_NOT_USED,
      MAX_ENCLS_IS_NOT_USED,
      BOXWOOD_HIGH_DRVCNT_MASK,
    },
    {STATIC_TEST_KNOT,
      "KNOT", 
      FBE_BOARD_TYPE_FLEET, 
      FBE_PORT_TYPE_SAS_PMC,
      1,
      2,
      0,
      FBE_SAS_ENCLOSURE_TYPE_KNOT,
      0,
      FBE_SAS_DRIVE_CHEETA_15K,
      INVALID_DRIVE_NUMBER,
      MAX_DRIVE_COUNT_KNOT,
      MAX_PORTS_IS_NOT_USED,
      MAX_ENCLS_IS_NOT_USED,
      KNOT_HIGH_DRVCNT_MASK,
    },    
    {STATIC_TEST_ANCHO,              //Test Number
      "ANCHO",                               //Title
      FBE_BOARD_TYPE_FLEET,       //Board type
      FBE_PORT_TYPE_SAS_PMC,    //Port type
      1,                                      // io port number
      2,                                      //portal number
      0,                                      //backend number
      FBE_SAS_ENCLOSURE_TYPE_ANCHO,     //enclosure type
      0,                                                 //enclosure number
      FBE_SAS_DRIVE_CHEETA_15K,          //Drive type 
      INVALID_DRIVE_NUMBER,                 //Drive number
      MAX_DRIVE_COUNT_ANCHO,              //Max drives

      MAX_PORTS_IS_NOT_USED,             //Max ports
      MAX_ENCLS_IS_NOT_USED,             //max enclosures
      ANCHO_HIGH_DRVCNT_MASK,            //drive mask
    },
    {STATIC_TEST_VIKING,
      "VIKING",
      FBE_BOARD_TYPE_FLEET, 
      FBE_PORT_TYPE_SAS_PMC,
      1,
      2,
      0,
      FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP,
      0,
      FBE_SAS_DRIVE_CHEETA_15K,
      INVALID_DRIVE_NUMBER,
      MAX_DRIVE_COUNT_VIKING,

      MAX_PORTS_IS_NOT_USED,
      MAX_ENCLS_IS_NOT_USED,
      VIKING_HIGH_DRVCNT_MASK,
    },
    {STATIC_TEST_CAYENNE,
      "CAYENNE",
      FBE_BOARD_TYPE_FLEET, 
      FBE_PORT_TYPE_SAS_PMC,
      1,
      2,
      0,
      FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP,
      0,
      FBE_SAS_DRIVE_CHEETA_15K,
      INVALID_DRIVE_NUMBER,
      MAX_DRIVE_COUNT_CAYENNE,

      MAX_PORTS_IS_NOT_USED,
      MAX_ENCLS_IS_NOT_USED,
      CAYENNE_HIGH_DRVCNT_MASK,
    },
    {STATIC_TEST_NAGA,
      "NAGA",
      FBE_BOARD_TYPE_FLEET, 
      FBE_PORT_TYPE_SAS_PMC,
      1,
      2,
      0,
      FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP,
      0,
      FBE_SAS_DRIVE_CHEETA_15K,
      INVALID_DRIVE_NUMBER,
      MAX_DRIVE_COUNT_NAGA,

      MAX_PORTS_IS_NOT_USED,
      MAX_ENCLS_IS_NOT_USED,
      VIKING_HIGH_DRVCNT_MASK,
    },
// Moons
    {STATIC_TEST_CALYPSO,
      "CALYPSO", 
      FBE_BOARD_TYPE_FLEET, 
      FBE_PORT_TYPE_SAS_PMC,
      1,
      2,
      0,
      FBE_SAS_ENCLOSURE_TYPE_CALYPSO,
      0,
      FBE_SAS_DRIVE_CHEETA_15K,
      INVALID_DRIVE_NUMBER,
      MAX_DRIVE_COUNT_CALYPSO,
      MAX_PORTS_IS_NOT_USED,
      MAX_ENCLS_IS_NOT_USED,
      CALYPSO_HIGH_DRVCNT_MASK,
    },
    {STATIC_TEST_RHEA,
      "RHEA", 
      FBE_BOARD_TYPE_FLEET, 
      FBE_PORT_TYPE_SAS_PMC,
      1,
      2,
      0,
      FBE_SAS_ENCLOSURE_TYPE_RHEA,
      0,
      FBE_SAS_DRIVE_CHEETA_15K,
      INVALID_DRIVE_NUMBER,
      MAX_DRIVE_COUNT_RHEA,
      MAX_PORTS_IS_NOT_USED,
      MAX_ENCLS_IS_NOT_USED,
      RHEA_HIGH_DRVCNT_MASK,
    },
    {STATIC_TEST_MIRANDA,
      "MIRANDA", 
      FBE_BOARD_TYPE_FLEET, 
      FBE_PORT_TYPE_SAS_PMC,
      1,
      2,
      0,
      FBE_SAS_ENCLOSURE_TYPE_MIRANDA,
      0,
      FBE_SAS_DRIVE_CHEETA_15K,
      INVALID_DRIVE_NUMBER,
      MAX_DRIVE_COUNT_MIRANDA,
      MAX_PORTS_IS_NOT_USED,
      MAX_ENCLS_IS_NOT_USED,
      MIRANDA_HIGH_DRVCNT_MASK,
    },
    {STATIC_TEST_TABASCO,
      "TABASCO", 
      FBE_BOARD_TYPE_FLEET, 
      FBE_PORT_TYPE_SAS_PMC,
      1,
      2,
      0,
      FBE_SAS_ENCLOSURE_TYPE_TABASCO,
      0,
      FBE_SAS_DRIVE_CHEETA_15K,
      INVALID_DRIVE_NUMBER,
      MAX_DRIVE_COUNT_TABASCO,

      MAX_PORTS_IS_NOT_USED,
      MAX_ENCLS_IS_NOT_USED,
      TABASCO_HIGH_DRVCNT_MASK,
    },
};

#define STATIC_TEST_MAX     sizeof(test_table)/sizeof(test_table[0])


void static_config_test(void)
{
    fbe_u32_t  test_count;

    mut_printf(MUT_LOG_TEST_STATUS, "static_config_test ..... started!\n");
    for(test_count = 0; test_count < STATIC_TEST_MAX; test_count++)
    {
        static_config_test_table_driven(&test_table[test_count]);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "static_config_test: Completed succesfully");
}
void static_config_test_table_driven(fbe_test_params_t *test)

{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t                           object_handle = 0;
    fbe_u32_t                           port_idx = 0, drive_idx = 0;
    fbe_port_number_t                   port_number;
    fbe_class_id_t                      class_id;
    fbe_u32_t                           device_count = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "\t\t\t %s TEST!\n", test->title);
    status = fbe_test_load_simple_config_table_data(test);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting system discovery ===\n");
    /* wait for the expected number of objects */
    status = fbe_test_wait_for_term_pp_sync(20000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===\n");

    /*we inserted a fleet board so we check for it*/
    /* board is always object id 0 ??? Maybe we need an interface to get the board object id*/
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* We should have exactly one port. */
    for (port_idx = 0; port_idx < 8; port_idx++)
    {
        if (port_idx < 1)
        {
            /* Get the handle of the port and validate port exists*/
            status = fbe_api_get_port_object_id_by_location(port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

            /* Use the handle to get lifecycle state*/
            status = fbe_api_wait_for_object_lifecycle_state (object_handle,
                FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            /* Use the handle to get port number*/
            status = fbe_api_get_object_port_number (object_handle, &port_number);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(port_number == port_idx);

            /* Validate that the enclosure is in READY state */
            status = fbe_zrt_wait_enclosure_status(port_idx, 0,
                                FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            for (drive_idx = 0; drive_idx < 2; drive_idx++)
            {
                /* Validate that the physical drives are in READY state */
                status = fbe_test_pp_util_verify_pdo_state(port_idx, 0, drive_idx,
                                    FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
            }
        }
        else
        {
            /* Get the handle of the port and validate port exists or not*/
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }

    /*let's see how many objects we have, at this stage we should have 7 objects:
    1 board
    1 port
    1 enclosure
    2 physical drives
    2 logical drives
    */
    status = fbe_test_verify_term_pp_sync();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    status = fbe_api_terminator_get_devices_count_by_type_name("Board", &device_count);
    MUT_ASSERT_TRUE(device_count == 1);
    mut_printf(MUT_LOG_LOW, "Verifying board count...Passed");
    /*TODO, check here for the existanse of other objects, their SAS address and do on, using the FBE API*/
    
    mut_printf(MUT_LOG_LOW, "=== Configuration verification successful !!!!! ===\n");
    fbe_test_physical_package_tests_config_unload();
    return;
}

