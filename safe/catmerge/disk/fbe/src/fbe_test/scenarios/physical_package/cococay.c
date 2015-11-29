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


/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @enum cococay_test_number_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * cococay_test.
 *********************************************************************/
typedef enum cococay_test_number_e
{
    COCOCAY_TEST_VIPER,
    COCOCAY_TEST_DERRINGER,
    COCOCAY_TEST_BUNKER,
    COCOCAY_TEST_CITADEL,   
    COCOCAY_TEST_FALLBACK,
    COCOCAY_TEST_BOXWOOD,
    COCOCAY_TEST_KNOT,
    COCOCAY_TEST_ANCHO,
    COCOCAY_TEST_CALYPSO,
    COCOCAY_TEST_RHEA,
    COCOCAY_TEST_MIRANDA,
    COCOCAY_TEST_TABASCO,
} cococay_test_number_t;

/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/
static fbe_test_params_t test_table[] = 
{   
    {COCOCAY_TEST_VIPER,
     "VIPER",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_VIPER,
     0,
     FBE_SATA_DRIVE_HITACHI_HUA,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_VIPER,
     COCOCAY_MAX_PORTS,
     COCOCAY_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED,
    },
    {COCOCAY_TEST_DERRINGER,
     "DERRINGER",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_DERRINGER,
     0,
     FBE_SATA_DRIVE_HITACHI_HUA,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_DERRINGER,
     COCOCAY_MAX_PORTS,
     COCOCAY_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED,
    },
    {COCOCAY_TEST_BUNKER,
     "BUNKER",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_BUNKER,
     0,
     FBE_SATA_DRIVE_HITACHI_HUA,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_BUNKER,
     COCOCAY_MAX_PORTS,
     COCOCAY_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED,
    },
    {COCOCAY_TEST_CITADEL,
     "CITADEL",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_CITADEL,
     0,
     FBE_SATA_DRIVE_HITACHI_HUA,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_CITADEL,
     COCOCAY_MAX_PORTS,
     COCOCAY_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED,
    },
    {COCOCAY_TEST_FALLBACK,
     "FALLBACK",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_FALLBACK,
     0,
     FBE_SATA_DRIVE_HITACHI_HUA,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_FALLBACK,
     COCOCAY_MAX_PORTS,
     COCOCAY_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED,
    },
    {COCOCAY_TEST_BOXWOOD,
     "BOXWOOD",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_BOXWOOD,
     0,
     FBE_SATA_DRIVE_HITACHI_HUA,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_BOXWOOD,
     COCOCAY_MAX_PORTS,
     COCOCAY_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED,
    },
    {COCOCAY_TEST_KNOT,
     "KNOT",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_KNOT,
     0,
     FBE_SATA_DRIVE_HITACHI_HUA,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_KNOT,
     COCOCAY_MAX_PORTS,
     COCOCAY_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED,
    },      
    {COCOCAY_TEST_ANCHO,
     "ANCHO",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_ANCHO,
     0,
     FBE_SATA_DRIVE_HITACHI_HUA,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_ANCHO,
     COCOCAY_MAX_PORTS,
     COCOCAY_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED,
    },
// Moons
    {COCOCAY_TEST_CALYPSO,
     "CALYPSO",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_CALYPSO,
     0,
     FBE_SATA_DRIVE_HITACHI_HUA,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_CALYPSO,
     COCOCAY_MAX_PORTS,
     COCOCAY_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED,
    },
    {COCOCAY_TEST_RHEA,
     "RHEA",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_RHEA,
     0,
     FBE_SATA_DRIVE_HITACHI_HUA,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_RHEA,
     COCOCAY_MAX_PORTS,
     COCOCAY_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED,
    },
    {COCOCAY_TEST_MIRANDA,
     "MIRANDA",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_MIRANDA,
     0,
     FBE_SATA_DRIVE_HITACHI_HUA,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_MIRANDA,
     COCOCAY_MAX_PORTS,
     COCOCAY_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED,
    },
    {COCOCAY_TEST_TABASCO,
     "TABASCO",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_TABASCO,
     0,
     FBE_SATA_DRIVE_HITACHI_HUA,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_TABASCO,
     COCOCAY_MAX_PORTS,
     COCOCAY_MAX_ENCLS,
     DRIVE_MASK_IS_NOT_USED,
    },
} ;


/*!*******************************************************************
 * @def COCOCAY_TEST_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define COCOCAY_TEST_MAX     sizeof(test_table)/sizeof(test_table[0])

fbe_status_t load_cococay_config(fbe_test_params_t *test)
{
    fbe_status_t status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t board_info;
    fbe_terminator_sas_port_info_t	sas_port;
    fbe_terminator_sas_encl_info_t	sas_encl;

    fbe_api_terminator_device_handle_t	port_handle;
    fbe_api_terminator_device_handle_t	encl_handle;
    fbe_api_terminator_device_handle_t	drive_handle;
    fbe_u32_t no_of_ports = 0;
    fbe_u32_t no_of_encls = 0;
    fbe_u32_t slot = 0;
    fbe_u32_t num_handles;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DEFIANT_M1_HW_TYPE;
    board_info.board_type = test->board_type;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (no_of_ports = 0; no_of_ports <  test->max_ports; no_of_ports ++)
    {
        /*insert a port*/
        sas_port.backend_number = no_of_ports;
        sas_port.io_port_number = test->io_port_number+ no_of_ports;
        sas_port.portal_number = test->portal_number + no_of_ports;
        sas_port.sas_address = FBE_BUILD_PORT_SAS_ADDRESS(test->backend_number + no_of_ports);
        sas_port.port_type = test->port_type;
        status  = fbe_api_terminator_insert_sas_port (&sas_port, &port_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        for ( no_of_encls = 0; no_of_encls < test->max_enclosures; no_of_encls++ )
        {
            /*insert an enclosure to port 0*/
            sas_encl.backend_number = no_of_ports;
            sas_encl.encl_number = no_of_encls;
            sas_encl.uid[0] = no_of_encls; // some unique ID.
            sas_encl.sas_address = FBE_BUILD_ENCL_SAS_ADDRESS(test->backend_number, no_of_encls);
            sas_encl.encl_type = test->encl_type;
            sas_encl.connector_id = 0;
            status  = fbe_test_insert_sas_enclosure (port_handle, &sas_encl, &encl_handle, &num_handles);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
            for (slot = 0; slot < test->max_drives; slot ++)
            {
                if(no_of_encls <  test->max_enclosures - 1 || (no_of_encls == (test->max_enclosures - 1) && slot >=2 && slot <= (test->max_drives - 2)))
                {
                    status  = fbe_test_pp_util_insert_sata_drive(no_of_ports,
                                                                 no_of_encls,
                                                                 slot,
                                                                 520,
                                                                 TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                                 &drive_handle);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                }
            }
        }
    }
    return status;
}
/****************************************************************
 * cococay_verify()
 ****************************************************************
 * Description:
 *  Function to get and verify the state of the enclosure 
 *  and its drives for cococay configuration.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  12/30/2008 - Ported from naxos.c
 *  11/20/2009:  Migrated from zeus_ready_test. Bo Gao
 *
 ****************************************************************/

fbe_status_t cococay_verify(fbe_u32_t port, fbe_u32_t encl, 
                            fbe_lifecycle_state_t state,
                            fbe_u32_t timeout,
                            fbe_u32_t max_enclosures,
                            fbe_u32_t max_drives)
{
    fbe_status_t    status;
    fbe_u32_t       no_of_encls;
    fbe_u32_t       object_handle = 0;
    fbe_u32_t       slot_num;

    /* Loop through all the enclosures and its drives to check its state */
    for ( no_of_encls = encl; no_of_encls < max_enclosures; no_of_encls++ )
    {
        if ( no_of_encls < max_enclosures )
        {
            /* Validate that the enclosure is in READY/ACTIVATE state */
            status = fbe_zrt_wait_enclosure_status(port, no_of_encls, state, timeout);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            if ( no_of_encls != max_enclosures - 1 )
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
                for( slot_num = 2; slot_num < max_drives - 1; slot_num ++ )
                {

                    /* For depth-2 we only have 12 drives starting from slot 2 */
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

fbe_status_t cococay_load_and_verify(fbe_test_params_t *test)
{
    fbe_class_id_t      class_id;
    fbe_u32_t           object_handle = 0;
    fbe_u32_t           port_idx;
    fbe_port_number_t   port_number;
	fbe_status_t        status;

    /* Option to change trace level
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_DEBUG_HIGH);
    */

    /* Before initializing the physical package, initialize the terminator. */
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: terminator initialized for %s.", __FUNCTION__, test->title);
    /* Enable the IO mode.  This will cause the terminator to create
     * disk files as drive objects are created.
     */ 
    status = fbe_api_terminator_set_io_mode(FBE_TERMINATOR_IO_MODE_ENABLED);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Load the test config file. */
    status = fbe_test_load_cococay_config(test);

	/* Initialize the fbe_api on server. */ 
    mut_printf(MUT_LOG_LOW, "%s: starting fbe_api for %s.", __FUNCTION__, test->title);
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the physical package. */
    mut_printf(MUT_LOG_LOW, "%s: starting physical package for %s.", __FUNCTION__, test->title);
    fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    
    fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    status = fbe_test_wait_for_term_pp_sync(20000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "%s: starting configuration verification for %s.", __FUNCTION__, test->title);

    /*we inserted a fleet board so we check for it*/
    /* board is always object id 0 ??? Maybe we need an interface to get the board object id*/
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type for %s ....Passed", test->title);

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
            status = cococay_verify(port_idx, 0, 
                            FBE_LIFECYCLE_STATE_READY, 
                            READY_STATE_WAIT_TIME,
                            test->max_enclosures,
                            test->max_drives);
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

    mut_printf(MUT_LOG_LOW, "Verifying object count for %s ...Passed", test->title);

    mut_printf(MUT_LOG_LOW, "%s: configuration verification for %s SUCCEEDED!", __FUNCTION__, test->title);


    return FBE_STATUS_OK;
}


void cococay(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;

    for(test_num = 0; test_num < COCOCAY_TEST_MAX; test_num++)
    {
        /* Load the configuration for test
         */
        run_status = cococay_load_and_verify(&test_table[test_num]);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
         */
        fbe_test_physical_package_tests_config_unload();
    }
}


