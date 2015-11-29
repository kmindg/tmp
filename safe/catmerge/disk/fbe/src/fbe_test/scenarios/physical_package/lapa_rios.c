#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"

#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_package.h"
#include "pp_utils.h"

#define LAPA_RIOS_MAX_ENCLS          3
#define LAPA_RIOS_MAX_PORTS          2

/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @enum lapa_rios_test_number_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * lapa_rios_test.
 *********************************************************************/
typedef enum lapa_rios_test_number_e
{
    LAPA_RIOS_TEST_NAGA,
    LAPA_RIOS_TEST_VIPER,
    LAPA_RIOS_TEST_DERRINGER,
    LAPA_RIOS_TEST_BUNKER,
    LAPA_RIOS_TEST_CITADEL,
    LAPA_RIOS_TEST_FALLBACK,
    LAPA_RIOS_TEST_BOXWOOD,
    LAPA_RIOS_TEST_KNOT,
    LAPA_RIOS_TEST_ANCHO,
    LAPA_RIOS_TEST_CALYPSO,
    LAPA_RIOS_TEST_RHEA,
    LAPA_RIOS_TEST_MIRANDA,
    LAPA_RIOS_TEST_TABASCO,
} lapa_rios_test_number_t;

/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/
static fbe_test_params_t test_table[] = 
{
    {LAPA_RIOS_TEST_NAGA,
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
     LAPA_RIOS_MAX_PORTS,
     LAPA_RIOS_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED 
    },   
    {LAPA_RIOS_TEST_VIPER,
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
     LAPA_RIOS_MAX_PORTS,
     LAPA_RIOS_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED 
    },
    {LAPA_RIOS_TEST_DERRINGER,
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
     LAPA_RIOS_MAX_PORTS,
     LAPA_RIOS_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED 
    },
    {LAPA_RIOS_TEST_BUNKER,
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
     LAPA_RIOS_MAX_PORTS,
     LAPA_RIOS_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED 
    },
    {LAPA_RIOS_TEST_CITADEL,
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
     LAPA_RIOS_MAX_PORTS,
     LAPA_RIOS_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED 
    },
    {LAPA_RIOS_TEST_FALLBACK,
     "FALLBACK",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_FALLBACK,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_FALLBACK,
     LAPA_RIOS_MAX_PORTS,
     LAPA_RIOS_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED 
    },
    {LAPA_RIOS_TEST_BOXWOOD,
     "BOXWOOD",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_BOXWOOD,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_BOXWOOD,
     LAPA_RIOS_MAX_PORTS,
     LAPA_RIOS_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED 
    },
    {LAPA_RIOS_TEST_KNOT,
     "KNOT",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_KNOT,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_KNOT,
     LAPA_RIOS_MAX_PORTS,
     LAPA_RIOS_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED 
    },    
    {LAPA_RIOS_TEST_ANCHO,
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
     LAPA_RIOS_MAX_PORTS,
     LAPA_RIOS_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED 
    },
// Moons
    {LAPA_RIOS_TEST_CALYPSO,
     "CALYPSO",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_CALYPSO,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_CALYPSO,
     LAPA_RIOS_MAX_PORTS,
     LAPA_RIOS_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED 
    },
    {LAPA_RIOS_TEST_RHEA,
     "RHEA",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_RHEA,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_RHEA,
     LAPA_RIOS_MAX_PORTS,
     LAPA_RIOS_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED 
    },
    {LAPA_RIOS_TEST_MIRANDA,
     "MIRANDA",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_MIRANDA,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_MIRANDA,
     LAPA_RIOS_MAX_PORTS,
     LAPA_RIOS_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED 
    },
    {LAPA_RIOS_TEST_TABASCO,
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
     LAPA_RIOS_MAX_PORTS,
     LAPA_RIOS_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED 
    },
} ;

/*!*******************************************************************
 * @def LAPA_RIOS_TEST_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define LAPA_RIOS_TEST_MAX     sizeof(test_table)/sizeof(test_table[0])

fbe_status_t lapa_rios_load_and_verify(fbe_test_params_t *test)
{
    fbe_class_id_t      class_id;
    fbe_u32_t           object_handle = 0;
    fbe_u32_t           port_idx;
    fbe_u32_t           enclosure_idx;
    fbe_u32_t           drive_idx;
    fbe_port_number_t   port_number;
    fbe_status_t        status;
    /* Before initializing the physical package, initialize the terminator. */
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: terminator initialized for %s.", __FUNCTION__, test->title);

    /* Load the test config*/
    status = fbe_test_load_lapa_rios_config();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: Lapa_Rios config loaded  for %s.", __FUNCTION__, test->title);

	/* Initialize the fbe_api on server. */ 
    mut_printf(MUT_LOG_LOW, "%s: starting fbe_api for %s....", __FUNCTION__, test->title);
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the physical package. */
    mut_printf(MUT_LOG_LOW, "%s: starting physical package for %s....", __FUNCTION__, test->title);
    fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    
    fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(LAPA_RIOS_NUMBER_OF_OBJ, 60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "%s: starting configuration verification for %s...", __FUNCTION__, test->title);

    /*we inserted a fleet board so we check for it*/
    /* board is always object id 0 ??? Maybe we need an interface to get the board object id*/
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type for %s....Passed", test->title);

    /* We should have exactly TWO ports. */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) {
        if (port_idx < 2) {
            /*get the handle of the port and validate port exists*/
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
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

        } else {
            /*get the handle of the port and validate port exists or not*/
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }
    mut_printf(MUT_LOG_LOW, "Verifying port existance, state and number for %s ...Passed", test->title);

    /* We should have exactly 3 enclosure on each port. */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) {
        for (enclosure_idx = 0; enclosure_idx < FBE_TEST_ENCLOSURES_PER_BUS; enclosure_idx++) {
            if ((port_idx < 2) && (enclosure_idx < 3)) {
                /*get the handle of the port and validate enclosure exists*/
                status = fbe_zrt_wait_enclosure_status (port_idx, enclosure_idx, FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
             } else {
                /*get the handle of the port and validate enclosure exists or not*/
                status = fbe_api_get_enclosure_object_id_by_location(port_idx, enclosure_idx, &object_handle);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
            }
        }
    }
    mut_printf(MUT_LOG_LOW, "Verifying enclosure existance and state for %s....Passed", test->title);

    /* Check for the physical drives on the enclosure. */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) {
        for (enclosure_idx = 0; enclosure_idx < FBE_TEST_ENCLOSURES_PER_BUS; enclosure_idx++) {
            for (drive_idx = 0; drive_idx < FBE_TEST_ENCLOSURE_SLOTS; drive_idx++) {
                if (((port_idx < 2) && (enclosure_idx < 2) && (drive_idx < 15))||
                    ((port_idx < 2) && (enclosure_idx == 2) && (drive_idx == 4))||
                    ((port_idx < 2) && (enclosure_idx == 2) && (drive_idx == 6))||
                    ((port_idx < 2) && (enclosure_idx == 2) && (drive_idx == 10)))
                {
                    /*get the handle of the port and validate drive exists*/
                    status = fbe_test_pp_util_verify_pdo_state(port_idx, 
                                                                enclosure_idx, 
                                                                drive_idx, 
                                                                FBE_LIFECYCLE_STATE_READY, 
                                                                READY_STATE_WAIT_TIME);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                } else {
                    /*get the handle of the drive and validate drive exists*/
                    status = fbe_api_get_physical_drive_object_id_by_location(port_idx, enclosure_idx, drive_idx, &object_handle);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
                }
            }
        }
    }
    mut_printf(MUT_LOG_LOW, "Verifying physical drive existance and state for %s....Passed", test->title);

    /* Let's see how many objects we have...
    1 board
    2 port
    2* 3 enclosure
    2* (15*2 + 3) physical drives
    2* (15*2 + 3) logical drives
    141 objects
    */  
    status = fbe_test_verify_term_pp_sync();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "%s: configuration verification SUCCEEDED for %s!", __FUNCTION__, test->title);

    return FBE_STATUS_OK;
}


void lapa_rios(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;

    for(test_num = 0; test_num < LAPA_RIOS_TEST_MAX; test_num++)
    {
        /* Load the configuration for test
         */
        run_status = lapa_rios_load_and_verify(&test_table[test_num]);

        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
         */
        fbe_test_physical_package_tests_config_unload();
    }
}

