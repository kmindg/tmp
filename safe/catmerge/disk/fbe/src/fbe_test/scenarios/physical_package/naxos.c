#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"


/*************************
 *   GLOBAL VARIABLES
 *************************/


/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @enum naxos_tests_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * naxos_test.
 *********************************************************************/
/* Naxos configuration has one port with 3 enclosures. 
 * We can only have a maximum of 2 Viking enclosures per bus. So we don't add the Viking test here. 
 */
typedef enum naxos_e
{
    NAXOS_TEST_VIPER,
    NAXOS_TEST_DERRINGER,
    NAXOS_TEST_VOYAGER,
    NAXOS_TEST_BUNKER,
    NAXOS_TEST_CITADEL,
    NAXOS_TEST_FALLBACK,
    NAXOS_TEST_BOXWOOD,
    NAXOS_TEST_KNOT,
    NAXOS_TEST_ANCHO,
    NAXOS_TEST_CALYPSO,
    NAXOS_TEST_RHEA,
    NAXOS_TEST_MIRANDA,
    NAXOS_TEST_TABASCO,
} naxos_tests_t;


/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/

fbe_test_params_t naxos_table[] = 
{  
    {NAXOS_TEST_VIPER,
     "VIPER",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_VIPER,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_VIPER,
     0,
     NAXOS_MAX_ENCLS,
     VIPER_MED_DRVCNT_MASK,
    },
    {NAXOS_TEST_DERRINGER,
     "DERRINGER",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_DERRINGER,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_DERRINGER,
     0,
     NAXOS_MAX_ENCLS,
     DERRINGER_MED_DRVCNT_MASK,
    },
    {NAXOS_TEST_VOYAGER,
     "VOYAGER", 
     FBE_BOARD_TYPE_ARMADA, 
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_VOYAGER,
     0,
     NAXOS_MAX_OB_AND_VOY,
     VOYAGER_MED_DRVCNT_MASK,
    },
    {NAXOS_TEST_BUNKER,
     "BUNKER",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_BUNKER,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_BUNKER,
     0,
     NAXOS_MAX_ENCLS,
     BUNKER_MED_DRVCNT_MASK,
    },
    {NAXOS_TEST_CITADEL,
     "CITADEL",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_CITADEL,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_CITADEL,
     0,
     NAXOS_MAX_ENCLS,
     CITADEL_MED_DRVCNT_MASK,
    },
    {NAXOS_TEST_FALLBACK,
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
     0,
     NAXOS_MAX_ENCLS,
     FALLBACK_MED_DRVCNT_MASK,
    },
    {NAXOS_TEST_BOXWOOD,
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
     0,
     NAXOS_MAX_ENCLS,
     BOXWOOD_MED_DRVCNT_MASK,
    },
    {NAXOS_TEST_KNOT,
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
     0,
     NAXOS_MAX_ENCLS,
     KNOT_MED_DRVCNT_MASK,

    },    
    {NAXOS_TEST_ANCHO,
     "ANCHO",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_ANCHO,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_ANCHO,
     0,
     NAXOS_MAX_ENCLS,
     ANCHO_MED_DRVCNT_MASK,
    },
// Moons
    {NAXOS_TEST_CALYPSO,
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
     0,
     NAXOS_MAX_ENCLS,
     CALYPSO_MED_DRVCNT_MASK,
    },
    {NAXOS_TEST_RHEA,
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
     0,
     NAXOS_MAX_ENCLS,
     RHEA_MED_DRVCNT_MASK,
    },
    {NAXOS_TEST_MIRANDA,
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
     MAX_DRIVE_COUNT_KNOT,
     0,
     NAXOS_MAX_ENCLS,
     MIRANDA_MED_DRVCNT_MASK,
    },
    {NAXOS_TEST_TABASCO,
     "TABASCO",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_TABASCO,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_TABASCO,
     0,
     NAXOS_MAX_ENCLS,
     TABASCO_MED_DRVCNT_MASK,
    },
};


/****************************************************************
 * naxos_verify()
 ****************************************************************
 * Description:
 *  Function to get and verify the state of the enclosure 
 *  and its drives for naxos configuration.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  08/15/2008 - Created. Arunkumar Selvapillai
 *  09/09/2008 - Moved to a common file to remove redudancy. AS
 *
 ****************************************************************/

fbe_status_t naxos_verify(fbe_u32_t port, fbe_u32_t encl, 
                            fbe_lifecycle_state_t state,
                            fbe_u32_t timeout,fbe_u32_t max_drives, 
                            fbe_u64_t drive_mask, fbe_u32_t max_enclosures)
{
    fbe_status_t    status;
    fbe_u32_t       no_of_encls;
    fbe_u32_t       object_handle = 0;
    fbe_u32_t       slot_num;

    /* Loop through all the enclosures and its drives to check its state */
    for ( no_of_encls = encl; no_of_encls < FBE_TEST_ENCLOSURES_PER_BUS; no_of_encls++ )
    {
        if ( no_of_encls < max_enclosures)
        {
            /* Validate that the enclosure is in READY/ACTIVATE state */
            status = fbe_zrt_wait_enclosure_status(port, no_of_encls, state, timeout);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            if ( no_of_encls != 2 )
            {
                for( slot_num = 0; slot_num < max_drives; slot_num ++ )
                {
                    /* Validate that the Physical drives are in READY/ACTIVATE state */
                    status = fbe_test_pp_util_verify_pdo_state(port, no_of_encls, slot_num, state, timeout);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
                }
            }
            else
            {
                /* For depth-2 we only have 12 drives starting from slot 2 */
                for( slot_num = 2; slot_num < max_drives-1; slot_num ++ )
                {
                    /* Validate that the Physical drives are in READY/ACTIVATE state */
                    status = fbe_test_pp_util_verify_pdo_state(port, no_of_encls, slot_num, state, timeout);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

                }
            }
        }
        else
        {
            /* Get the handle of the port and validate enclosure exists or not */
            status = fbe_api_get_enclosure_object_id_by_location (port, no_of_encls, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }

    return FBE_STATUS_OK;
}

fbe_status_t naxos_load_and_verify_table_driven(fbe_test_params_t *test)
{
    fbe_class_id_t      class_id;
    fbe_u32_t           object_handle = 0;
    fbe_u32_t           port_idx;
    fbe_port_number_t   port_number;
	fbe_status_t        status;

    /* Before initializing the physical package, initialize the terminator. */
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: terminator initialized . for %s", __FUNCTION__,test->title);

    /* Load the test config */
    status = fbe_test_load_naxos_config(test);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: Naxos config loaded.. for %s", __FUNCTION__,test->title);

	/* Initialize the fbe_api on server. */ 
    mut_printf(MUT_LOG_LOW, "%s: starting fbe_api... . for %s", __FUNCTION__,test->title);
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the physical package. */
    mut_printf(MUT_LOG_LOW, "%s: starting physical package.... for %s", __FUNCTION__,test->title);
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

    /* Check for the enclosures, physical drives and logical drives */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) 
    {
        if ( port_idx < 1 )
        {
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

            /* Verify the existence of the enclosure and its drives */
            status = naxos_verify(port_idx, 0, 
                            FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME,
                            test->max_drives, test->drive_mask, test->max_enclosures);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        }
        else
        {
            /*get the handle of the port and validate port exists or not*/
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }

    /* Let's see how many objects we have...
    1 board
    1 port
    3 enclosure
    15*2 + 12 physical drives
    15*2 + 12 logical drives
    89 objects
    */  
    status = fbe_test_verify_term_pp_sync();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed for %s", test->title);

    mut_printf(MUT_LOG_LOW, "%s: configuration verification SUCCEEDED for %s!", __FUNCTION__, test->title);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_test_get_naxos_test_table()
 ****************************************************************
 * @brief
 * This function takes index num and returns a pointer to the table entry.
 *
 * @return pointer to fbe_test_params_t structure .
 * 
 * @author
 *  8/2/2011 - Created.
 ****************************************************************/

fbe_test_params_t  *fbe_test_get_naxos_test_table(fbe_u32_t index_num)
{
     return (&naxos_table[ index_num]);
}

/*!***************************************************************
 * fbe_test_get_naxos_num_of_tests()
 ****************************************************************
 * @brief
 * This function returns number of table entries.
 *
 * @ returns number of table entries.
 * 
 * @author
 *   8/2/2011 - Created. 
 ****************************************************************/

fbe_u32_t fbe_test_get_naxos_num_of_tests(void)
{
     return  (sizeof(naxos_table))/(sizeof(naxos_table[0]));
}

void naxos (void)
{
     fbe_status_t run_status;
     fbe_u32_t test_num;
     fbe_u32_t max_entries ;
     fbe_test_params_t *naxos_test;

     max_entries = fbe_test_get_naxos_num_of_tests() ;
     for(test_num = 0; test_num < max_entries; test_num++)
     {
         /* Load the configuration for test
         */
         naxos_test =  fbe_test_get_naxos_test_table(test_num); 
         run_status = naxos_load_and_verify_table_driven(naxos_test);
         MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);
         /* Unload the configuration
         */
         fbe_test_physical_package_tests_config_unload();
    }
    /* do nothing */
}
