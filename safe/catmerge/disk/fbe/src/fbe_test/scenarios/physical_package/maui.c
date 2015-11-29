#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_package.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"

#define MAUI_MAX_PORT 1
#define MAUI_MAX_ENCL 1
#define MAUI_MAX_DRIVE 12

char * maui_short_desc = "Terminator initialization and configuration loading, unloading and verification.";
char * maui_long_desc = "\
This file contains the Maui scenario and performs the following tasks:\n\
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



/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @enum maui_test_number_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * trichirapally_test.
 *********************************************************************/
typedef enum maui_test_number_e
{
    MAUI_TEST_NAGA,
    MAUI_TEST_VIPER,
    MAUI_TEST_DERRINGER,
    MAUI_TEST_VOYAGER,
    MAUI_TEST_BUNKER,
    MAUI_TEST_CITADEL,
    MAUI_TEST_FALLBACK,
    MAUI_TEST_BOXWOOD,
    MAUI_TEST_KNOT,
    MAUI_TEST_PINECONE, 
/*
    MAUI_TEST_STEELJAW,
*/    
    MAUI_TEST_RAMHORN,
    MAUI_TEST_RHEA,
    MAUI_TEST_MIRANDA,
    MAUI_TEST_CALYPSO,
    MAUI_TEST_TABASCO,
    MAUI_TEST_ANCHO,
    MAUI_TEST_CAYENNE,
}maui_test_number_t;

/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/
fbe_test_params_t test_table[] = 
{
    {MAUI_TEST_NAGA,
     "NAGA",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_NAGA,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {MAUI_TEST_VIPER,
     "VIPER",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_VIPER,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_VIPER,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },    
    {MAUI_TEST_DERRINGER,
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
    {MAUI_TEST_VOYAGER,
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
    {MAUI_TEST_BUNKER,
     "BUNKER",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_BUNKER,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_BUNKER,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {MAUI_TEST_CITADEL,
     "CITADEL",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_CITADEL,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_CITADEL,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {MAUI_TEST_FALLBACK,
     "FALLBACK",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_FALLBACK,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_FALLBACK,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {MAUI_TEST_BOXWOOD,
     "BOXWOOD",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_BOXWOOD,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_BOXWOOD,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {MAUI_TEST_KNOT,
     "KNOT",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_KNOT,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_KNOT,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {MAUI_TEST_PINECONE,
     "PINECONE",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_PINECONE,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_PINECONE,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
/*
    {MAUI_TEST_STEELJAW,
     "STEELJAW",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_STEELJAW,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_STEELJAW,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
*/
    {MAUI_TEST_RAMHORN,
     "RAMHORN",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_RAMHORN,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_RAMHORN,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },    
// Moons
    {MAUI_TEST_RHEA,
     "RHEA",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_RHEA,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_RHEA,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {MAUI_TEST_MIRANDA,
     "MIRANDA",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_MIRANDA,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_MIRANDA,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {MAUI_TEST_CALYPSO,
     "CALYPSO",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_CALYPSO,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_CALYPSO,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {MAUI_TEST_TABASCO,
     "TABASCO",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_TABASCO,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_TABASCO,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {MAUI_TEST_ANCHO,
     "ANCHO",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_ANCHO,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_ANCHO,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {MAUI_TEST_CAYENNE,
     "CAYENNE", 
     FBE_BOARD_TYPE_FLEET, 
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_CAYENNE,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
};

/*!*******************************************************************
 * @def MAUI_TEST_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define MAUI_TEST_MAX     sizeof(test_table)/sizeof(test_table[0])

/****************************************************************
 * maui_verify()
 ****************************************************************
 * Description:
 *  Function to get and verify the state of the enclosure 
 *  and its drives for maui configuration.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  09/09/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

fbe_status_t maui_verify(fbe_u32_t port, fbe_u32_t encl, 
                            fbe_lifecycle_state_t state,
                            fbe_u32_t timeout)
{
    fbe_status_t    status;
    fbe_u32_t       no_of_encls;
    fbe_u32_t       object_handle = 0;
    fbe_u32_t       slot_num;
    
    /* Loop through all the enclosures and its drives to check its state */
    for ( no_of_encls = encl; no_of_encls < FBE_TEST_ENCLOSURES_PER_BUS; no_of_encls++ )
    {
        if ( no_of_encls < 1 )
        {
            /* Validate that the enclosure is in READY/ACTIVATE state */
            status = fbe_zrt_wait_enclosure_status(port, encl, state, timeout);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            for( slot_num = 0; slot_num < 12; slot_num ++ )
            {
                /* Validate that the physical drives are in READY/ACTIVATE state */
                status = fbe_test_pp_util_verify_pdo_state(port, encl, slot_num, state, timeout);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
            }
        }
        else
        {
            /* Get the handle of the port and validate enclosure exists or not */
            status = fbe_api_get_enclosure_object_id_by_location(port, no_of_encls, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }
    return FBE_STATUS_OK;
}

fbe_status_t maui_load_and_verify_table_driven(fbe_test_params_t *test)
{
    fbe_class_id_t      class_id;
    fbe_u32_t           object_handle = 0;
    fbe_u32_t           port_idx;
    fbe_port_number_t   port_number;
    fbe_status_t        status;

    mut_printf(MUT_LOG_TEST_STATUS, "\t\t\t %s TEST!\n", test->title);
    /* Before initializing the physical package, initialize the terminator. */
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: terminator initialized.", __FUNCTION__);

    /* Load the test config */
    status = fbe_test_load_maui_config(SPID_DEFIANT_M1_HW_TYPE, test);
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

    /* wait for the expected number of objects */
    status = fbe_test_wait_for_term_pp_sync(20000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "%s: starting configuration verification...", __FUNCTION__);

    /*we inserted a fleet board so we check for it*/
    /* board is always object id 0 ??? Maybe we need an interface to get the board object id*/
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* We should have exactly one port. */
    for (port_idx = 0; port_idx < FBE_TEST_ENCLOSURES_PER_BUS; port_idx++) 
    {
        if (port_idx < 1) 
        {
            /*get the handle of the port and validate port exists*/
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

            /* Verify the existence of the enclosure and its drives */
            status = maui_verify(port_idx, test->encl_number, FBE_LIFECYCLE_STATE_READY, 
                                                READY_STATE_WAIT_TIME);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        } 
        else 
        {
            /* Get the handle of the port and validate port exists or not*/
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }

    /* Let's see how many objects we have...
    1 board
    1 port
    1 enclosure
    12 physical drives
    12 logical drives
    27 objects
    */  
    status = fbe_test_verify_term_pp_sync();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    mut_printf(MUT_LOG_LOW, "%s: configuration verification SUCCEEDED!", __FUNCTION__);

    return FBE_STATUS_OK;
}

void maui_load_single_configuration(void)
{
    fbe_status_t run_status;
    /* Load the configuration of only viper
     * for tests those are not specific to enclosure object.
     * hence no need to execute tests for all enclosures
     */
    run_status = maui_load_and_verify_table_driven(&test_table[0]);
    MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

}

void maui(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;

    for(test_num = 0; test_num < MAUI_TEST_MAX; test_num++)
    {
        /* Load the configuration for test
         */
        run_status = maui_load_and_verify_table_driven(&test_table[test_num]);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
        */
        fbe_test_physical_package_tests_config_unload();
    }
}
