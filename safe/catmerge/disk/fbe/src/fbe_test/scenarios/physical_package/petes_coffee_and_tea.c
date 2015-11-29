/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file petes_coffee_and_tea.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the pete's coffee and tea scenario.
 * 
 *  This scenario is for a 1 port, 1 enclosure configuration, aka the maui.xml
 *  configuration.
 * 
 *  The test run in this case is:
 *  1) create and validate a configuration.
 *  2) Cause the port to receive a logout for the drive in slot-0, but leave the
 *  insert bit as is.
 *  3) Validate that the physical and logical drives are in the ACTIVATE state.
 *  4) Cause port to receive login for this drive.
 *  5) Validate that the physical and logical drives are in the READY state.
 *
 * HISTORY
 *   7/28/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"


char * petes_coffee_and_tea_short_desc = "logout while enclosure insert bit set";
char * petes_coffee_and_tea_long_desc = "\n\tSetup: load and verify maui\n"
                                        "\tStep 1) create and validate a configuration. \n"
                                        "\tStep 2) Cause the port to receive a logout for the drive in slot-0, but leave the insert bit as is.\n"
                                        "\tStep 3) Validate that the physical and logical drives are in the ACTIVATE state. \n"
                                        "\tStep 4) Cause port to receive login for this drive. \n"
                                        "\tStep 5) Validate that the physical and logical drives are in the READY state. \n"
                                        "\tTeardown: destroy physical package, fbe api, and terminator\n";


/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @enum petes_coffee_and_tea_test_number_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * petes_coffee_and_tea_test.
 *********************************************************************/
typedef enum petes_coffee_and_tea_test_number_e
{
    PETES_COFFEE_AND_TEA_TEST_NAGA,
    PETES_COFFEE_AND_TEA_TEST_VIPER,
    PETES_COFFEE_AND_TEA_TEST_DERRINGER,
    PETES_COFFEE_AND_TEA_TEST_BUNKER,
    PETES_COFFEE_AND_TEA_TEST_CITADEL,
    PETES_COFFEE_AND_TEA_TEST_FALLBACK,
    PETES_COFFEE_AND_TEA_TEST_BOXWOOD,
    PETES_COFFEE_AND_TEA_TEST_KNOT,
    PETES_COFFEE_AND_TEA_TEST_ANCHO,
    PETES_COFFEE_AND_TEA_TEST_CALYPSO,
    PETES_COFFEE_AND_TEA_TEST_RHEA,
    PETES_COFFEE_AND_TEA_TEST_MIRANDA,
    PETES_COFFEE_AND_TEA_TEST_TABASCO,
} petes_coffee_and_tea_test_number_t;

/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/
static fbe_test_params_t test_table[] = 
{   
    {PETES_COFFEE_AND_TEA_TEST_NAGA,
     "NAGA",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     0,
     MAX_DRIVE_COUNT_NAGA,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {PETES_COFFEE_AND_TEA_TEST_VIPER,
     "VIPER",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_VIPER,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     0,
     MAX_DRIVE_COUNT_VIPER,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
     {PETES_COFFEE_AND_TEA_TEST_DERRINGER,
     "DERRINGER",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_DERRINGER,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     0,
     MAX_DRIVE_COUNT_DERRINGER,
     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
     {PETES_COFFEE_AND_TEA_TEST_BUNKER,
     "BUNKER",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_BUNKER,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     0,
     MAX_DRIVE_COUNT_BUNKER,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
     {PETES_COFFEE_AND_TEA_TEST_CITADEL,
     "CITADEL",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_CITADEL,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     0,
     MAX_DRIVE_COUNT_CITADEL,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {PETES_COFFEE_AND_TEA_TEST_FALLBACK,
     "FALLBACK",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_FALLBACK,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     0,
     MAX_DRIVE_COUNT_FALLBACK,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {PETES_COFFEE_AND_TEA_TEST_BOXWOOD,
     "BOXWOOD",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_BOXWOOD,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     0,
     MAX_DRIVE_COUNT_BOXWOOD,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {PETES_COFFEE_AND_TEA_TEST_KNOT,
     "KNOT",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_KNOT,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     0,
     MAX_DRIVE_COUNT_KNOT,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },    
    {PETES_COFFEE_AND_TEA_TEST_ANCHO,
     "ANCHO",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_ANCHO,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     0,
     MAX_DRIVE_COUNT_ANCHO,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
// Moons
    {PETES_COFFEE_AND_TEA_TEST_CALYPSO,
     "CALYPSO",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_CALYPSO,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     0,
     MAX_DRIVE_COUNT_CALYPSO,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {PETES_COFFEE_AND_TEA_TEST_RHEA,
     "RHEA",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_RHEA,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     0,
     MAX_DRIVE_COUNT_RHEA,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {PETES_COFFEE_AND_TEA_TEST_MIRANDA,
     "MIRANDA",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_MIRANDA,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     0,
     MAX_DRIVE_COUNT_MIRANDA,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {PETES_COFFEE_AND_TEA_TEST_TABASCO,
     "TABASCO",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_TABASCO,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     0,
     MAX_DRIVE_COUNT_TABASCO,
     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
} ;

/*!*******************************************************************
 * @def PETES_COFFEE_AND_TEA_TEST_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define PETES_COFFEE_AND_TEA_TEST_MAX     sizeof(test_table)/sizeof(test_table[0])



/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * @fn validate_physical_drive(fbe_u32_t port,
 *                            fbe_u32_t enclosure,
 *                            fbe_u32_t drive,
 *                            fbe_lifecycle_state_t expected_lifecycle_state)
 ****************************************************************
 * @brief
 *  Make sure the physical drive exists and is in the appropriate state.
 *
 * @param port - The port number of the drive to check.   
 * @param enclosure - The enclosure number of the drive to check.
 * @param drive - The drive number of the drive to check.
 * @param expected_lifecycle_state - This is the state we expect
 *                                   the object to be in.
 *
 * @return - FBE_STATUS_OK if the drive is in the state expected.
 *
 * HISTORY:
 *  7/29/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_status_t validate_physical_drive(fbe_u32_t port,
                                            fbe_u32_t enclosure,
                                            fbe_u32_t drive,
                                            fbe_lifecycle_state_t expected_lifecycle_state)
{
    fbe_status_t            status;
	fbe_u32_t				object_handle;
	fbe_port_number_t		port_number;

    /* Verify if the drive exists */
	status = fbe_api_get_physical_drive_object_id_by_location (port, enclosure, drive, &object_handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    /* Check the state of the physical_drive */
	/* Use the handle to get lifecycle state*/
	status = fbe_api_wait_for_object_lifecycle_state (object_handle, expected_lifecycle_state, 20000, FBE_PACKAGE_ID_PHYSICAL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/* Use the handle to get port number*/
	status = fbe_api_get_object_port_number (object_handle, &port_number);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(port_number == port);

    return FBE_STATUS_OK;
}
/******************************************
 * end validate_physical_drive()
 ******************************************/

/*!**************************************************************
 * @fn validate_logical_drive(fbe_u32_t port,
 *                            fbe_u32_t enclosure,
 *                            fbe_u32_t drive,
 *                            fbe_lifecycle_state_t expected_lifecycle_state)
 ****************************************************************
 * @brief
 *  Make sure the logical drive exists and is in the appropriate state.
 *
 * @param port - The port number of the drive to check.   
 * @param enclosure - The enclosure number of the drive to check.
 * @param drive - The drive number of the drive to check.
 * @param expected_lifecycle_state - This is the state we expect
 *                                   the object to be in.
 *
 * @return - FBE_STATUS_OK if the drive is in the state expected.
 *
 * HISTORY:
 *  7/29/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_status_t validate_logical_drive(fbe_u32_t port,
                                           fbe_u32_t enclosure,
                                           fbe_u32_t drive,
                                           fbe_lifecycle_state_t expected_lifecycle_state)
{
    fbe_status_t            status;
	fbe_u32_t				object_handle;
	fbe_port_number_t		port_number;

    /* Verify if the drive exists */
	status = fbe_api_get_physical_drive_object_id_by_location (port, enclosure, drive, &object_handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    /* Check the state of the logical_drive */
	/* Use the handle to get lifecycle state*/
	status = fbe_api_wait_for_object_lifecycle_state (object_handle, expected_lifecycle_state, 10000, FBE_PACKAGE_ID_PHYSICAL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/* Use the handle to get port number*/
	status = fbe_api_get_object_port_number (object_handle, &port_number);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(port_number == port);

    return FBE_STATUS_OK;
}
/******************************************
 * end validate_logical_drive()
 ******************************************/

/*!**************************************************************
 * @fn petes_coffee_and_tea_validate_discovery_edge_states(
 *                                       fbe_bool_t b_logged_out)
 ****************************************************************
 * @brief
 *  This function validates the discovery edge states.
 *  We expect that for this test, when a drive is not logged out
 *  then all the edge states will be enabled.
 * 
 *  If the drive is logged out, then we will expect
 *  the edge state between physical and logical drive to be disabled.
 *
 * @param b_logged_out - True if we are at the point of the test
 *                       where a drive is logged out.
 *                       False otherwise.
 *
 * @return None.
 *
 * HISTORY:
 *  8/18/2008 - Created. RPF
 *
 ****************************************************************/

static void petes_coffee_and_tea_validate_discovery_edge_states(fbe_test_params_t *test,fbe_bool_t b_logged_out)
{
    fbe_u32_t port_handle_p;
    fbe_u32_t enclosure_handle_p;
    fbe_u32_t physical_drive_handle_p;
    fbe_status_t status;
    fbe_api_get_discovery_edge_info_t edge_info;

    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_validate_discovery_edge_states for %s Starting ===\n", test->title);

    /* First get the handle to the port.
     */
    status = fbe_api_get_port_object_id_by_location(test->backend_number, &port_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(port_handle_p != FBE_OBJECT_ID_INVALID);

    /* Use this handle to fetch the discovery info. 
     * Make sure the path state is enabled.
     */
	status = fbe_api_get_discovery_edge_info (port_handle_p, &edge_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(edge_info.path_state == FBE_PATH_STATE_ENABLED);

    /* Get the enclosure's handle. We are only interested in port=0,
     * enclosure=0, since this is where the drive we are testing resides.
     */
    status = fbe_api_get_enclosure_object_id_by_location(test->backend_number,test->encl_number,
                                                                                     &enclosure_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(enclosure_handle_p, FBE_OBJECT_ID_INVALID);

    /* Use this handle to fetch the discovery info. 
     * Make sure the path state is enabled.
     */
	status = fbe_api_get_discovery_edge_info (enclosure_handle_p, &edge_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(edge_info.path_state == FBE_PATH_STATE_ENABLED);

    /* Get physical drive's handle.  We are only interested in port=0,
     * enclosure=0, slot=0, since this is the drive we are testing.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(test->backend_number,test->encl_number,
                                                                                             test->drive_number, &physical_drive_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(physical_drive_handle_p, FBE_OBJECT_ID_INVALID);

    /* Use this handle to fetch the discovery info. 
     * Make sure the path state is enabled. We do not expect this state to
     * change in this test. 
     */
	status = fbe_api_get_discovery_edge_info (physical_drive_handle_p, &edge_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(edge_info.path_state == FBE_PATH_STATE_ENABLED);

    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_validate_discovery_edge_states for %s Successful ===\n", test->title);
    return;
}
/******************************************
 * end petes_coffee_and_tea_validate_discovery_edge_states()
 ******************************************/

/*!**************************************************************
 * @fn petes_coffee_and_tea_validate_ssp_edge_states()
 ****************************************************************
 * @brief
 *  This function validates the discovery edge states.
 * 
 *  For this test we always expect the SSP edge states to be enabled.
 *
 * @param  - None.
 *
 * @return  - None.
 *
 * HISTORY:
 *  8/18/2008 - Created. RPF
 *
 ****************************************************************/
static void petes_coffee_and_tea_validate_ssp_edge_states(fbe_test_params_t *test)
{
    fbe_u32_t physical_drive_handle_p;
    fbe_u32_t enclosure_handle_p;
    fbe_status_t status;
    fbe_api_get_ssp_edge_info_t ssp_transport_edge_info;
    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_validate_ssp_edge_states for %s Starting ===\n", test->title);

    /* Get enclosure handle.  We are interested in port=0 enclosure=0 since this
     * is where the drive we are testing resides. 
     */
    status = fbe_api_get_enclosure_object_id_by_location(test->backend_number,
                                                                                     test->encl_number,
                                                                                     &enclosure_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(enclosure_handle_p, FBE_OBJECT_ID_INVALID);

    /* Use handle to get edge info. 
     */
    status = fbe_api_get_ssp_edge_info(enclosure_handle_p, &ssp_transport_edge_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Make sure the path state is enabled. We do not expect this state to
     * change in this test. 
     */
    MUT_ASSERT_TRUE(ssp_transport_edge_info.path_state == FBE_PATH_STATE_ENABLED);

    /* Get drive handle.  We are only interested in port=0 enclosure=0 slot=0
     * since this is the only drive we are testing. 
     */
    status = fbe_api_get_physical_drive_object_id_by_location(test->backend_number,
                                                                                            test->encl_number,
                                                                                            test->drive_number,
                                                                                            &physical_drive_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(physical_drive_handle_p, FBE_OBJECT_ID_INVALID);

    /* Use handle to get edge info. 
     */
    status = fbe_api_get_ssp_edge_info(physical_drive_handle_p, &ssp_transport_edge_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Make sure the path state is enabled. We do not expect this state to
     * change in this test. 
     */
    MUT_ASSERT_TRUE(ssp_transport_edge_info.path_state == FBE_PATH_STATE_ENABLED);

    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_validate_ssp_edge_states for %s Successful ===\n", test->title);
    return;
}
/******************************************
 * end petes_coffee_and_tea_validate_ssp_edge_states()
 ******************************************/

/*!**************************************************************
 * @fn petes_coffee_and_tea_validate_block_edge_states(fbe_bool_t b_logged_out)
 ****************************************************************
 * @brief
 *  This function validates the discovery edge states.
 *  We expect that for this test, when a drive is not logged out
 *  then all the edge states will be enabled.
 * 
 *  If the drive is logged out, then we will expect
 *  the edge state between physical and logical drive to be disabled.
 *
 * @param b_logged_out - True if we are at the point of the test
 *                       where a drive is logged out.
 *                       False otherwise.
 *
 * @return None.
 *
 * HISTORY:
 *  8/18/2008 - Created. RPF
 *
 ****************************************************************/
static void petes_coffee_and_tea_validate_block_edge_states(fbe_test_params_t *test,fbe_bool_t b_logged_out)
{
#if 0
    fbe_u32_t logical_drive_handle_p;
    fbe_status_t status;
    fbe_api_get_block_edge_info_t block_edge_info;

    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_validate_block_edge_states Starting ===\n");

    /* Fetch handle.
     */
    status = fbe_api_get_logical_drive_object_id_by_location(test->backend_number,
                                                                   test->encl_number,
                                                                   test->drive_number,
                                                                   &logical_drive_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(logical_drive_handle_p, FBE_OBJECT_ID_INVALID);

    /* Use handle to fetch the block path state.
     */
    status = fbe_api_get_block_edge_info(logical_drive_handle_p, 0, &block_edge_info, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Make sure the path state is disabled when drive is logged out, and 
     * enabled when drive is alive. 
     */ 
    if (b_logged_out)
    {
        MUT_ASSERT_TRUE(block_edge_info.path_state == FBE_PATH_STATE_DISABLED);
    }
    else
    {
        MUT_ASSERT_TRUE(block_edge_info.path_state == FBE_PATH_STATE_ENABLED);
    }
#endif
    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_validate_block_edge_states for %s skipped ===\n", test->title);
    return;
}
/******************************************
 * end petes_coffee_and_tea_validate_block_edge_states()
 ******************************************/

/*!**************************************************************
 * @fn petes_coffee_and_tea_run()
 ****************************************************************
 * @brief
 *  Run the pete's coffee and tea test.
 *  This is a 1 port, 1 enclosure test.
 * 
 *  - The test logs out a drive without changing the insert bit.
 *  - Then we check that physical and logical drive are in
 *    the activate state.
 *  - Then we log the drive back in.
 *  - Then we check that physical and logical drive are in the ready state.
 *
 * @param  - none          
 *
 * @return FBE_STATUS_OK if no errors. 
 *
 * HISTORY:
 *  7/29/2008 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t petes_coffee_and_tea_run(fbe_test_params_t *test)
{
    fbe_status_t status;
    fbe_api_terminator_device_handle_t drive_device_id;
    fbe_api_terminator_device_handle_t encl_device_id;

    /* Validate the path states.
     * All should be enabled except the logical drive -> physical drive edges. 
     */
    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_run Validate edge states or %s Starting ===\n", test->title);
    petes_coffee_and_tea_validate_discovery_edge_states(test, FBE_FALSE /* No, not logged out. */);
    petes_coffee_and_tea_validate_ssp_edge_states(test);
    petes_coffee_and_tea_validate_block_edge_states(test,FBE_FALSE /* No, not logged out. */ );
    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_run Validate edge states or %s Successful ===\n", test->title);

    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_run checking object states or %s Starting  ===\n", test->title);

    /* Make sure that both logical and physical drives are in READY.
     *  
     * Physical drive would have transitioned to READY when it saw the edge 
     * state change and the closed bit clear on the protocol edge. 
     *  
     * Logical drive transitioned to READY when it received the path state 
     * change caused by the physical drive changing state. 
     */
    status = validate_physical_drive(test->backend_number, 
                                                    test->encl_number, 
                                                    test->drive_number,
                                                    FBE_LIFECYCLE_STATE_READY);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = validate_logical_drive(test->backend_number, 
                                                  test->encl_number, 
                                                  test->drive_number,
                                                  FBE_LIFECYCLE_STATE_READY);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_run checking object states for %s Successful  ===\n", test->title);

    /* First get the device id of the enclosure.
     */
    status = fbe_api_terminator_get_enclosure_handle(test->backend_number, 
                                                     test->encl_number,
                                                     &encl_device_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Then use this enclosure id to get the drive's id.
     * We need this in order to do the logout below. 
     */
    status = fbe_api_terminator_get_drive_handle(test->backend_number, 
                                                    test->encl_number, 
                                                    test->drive_number, 
                                                    &drive_device_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_run logging out drive %d_%d_%d for %s ===\n", 
                                                                                                         test->backend_number, 
                                                                                                         test->encl_number, 
                                                                                                         test->drive_number,
                                                                                                         test->title);

    /* Force logout of a drive without changing the insert bit.
     */
    status = fbe_api_terminator_force_logout_device(drive_device_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_run checking object states for %s Starting  ===\n", test->title);

    /* Make sure that both logical and physical drives are in ACTIVATE.
     *  
     * Physical drive would have transitioned to ACTIVATE when it saw the edge 
     * state change and the closed bit set on the protocol edge. 
     *  
     * Logical drive transitioned to ACTIVATE when it received the path state 
     * change caused by the physical drive changing state. 
     */
    status = validate_physical_drive(test->backend_number,
                                     test->encl_number,
                                     test->drive_number,
                                     FBE_LIFECYCLE_STATE_ACTIVATE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = validate_logical_drive(test->backend_number,
                                     test->encl_number,
                                     test->drive_number,
                                     FBE_LIFECYCLE_STATE_ACTIVATE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_run checking object states for %s Successful  ===\n", test->title);

    /* Validate the path states.
     * All should be enabled except the logical drive -> physical drive edges. 
     */
    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_run Validate edge states for %s Starting ===\n", test->title);
    petes_coffee_and_tea_validate_discovery_edge_states(test, FBE_TRUE /* Yes we are logged out. */);
    petes_coffee_and_tea_validate_ssp_edge_states(test);
    petes_coffee_and_tea_validate_block_edge_states(test, FBE_TRUE /* Yes we are logged out. */ );

    /* Force the login to occur. 
     */
    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_run logging in drive %d_%d_%d for %s  ===\n", 
                                                                                                         test->backend_number, 
                                                                                                         test->encl_number, 
                                                                                                         test->drive_number,
                                                                                                         test->title);
    status = fbe_api_terminator_force_login_device(drive_device_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_run checking object states Starting  ===\n");

    /* Make sure that both logical and physical drives are in READY.
     *  
     * Physical drive would have transitioned to READY when it saw the edge 
     * state change and the closed bit clear on the protocol edge. 
     *  
     * Logical drive transitioned to READY when it received the path state 
     * change caused by the physical drive changing state. 
     */
    status = validate_physical_drive(test->backend_number, test->encl_number, 
                                                    test->drive_number, FBE_LIFECYCLE_STATE_READY);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = validate_logical_drive(test->backend_number, test->encl_number, 
                                                 test->drive_number, FBE_LIFECYCLE_STATE_READY);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_run checking object states Successful  ===\n");

    /* Validate the path states.  All should be enabled.
     */
    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_run Validate edge states Starting ===\n");
    petes_coffee_and_tea_validate_discovery_edge_states(test, FBE_FALSE /* not logged out */);
    petes_coffee_and_tea_validate_ssp_edge_states(test);
    petes_coffee_and_tea_validate_block_edge_states(test, FBE_FALSE/* not logged out */);
    mut_printf(MUT_LOG_LOW, "=== petes_coffee_and_tea_run Validate edge states Successful ===\n");

    return FBE_STATUS_OK;
}
/******************************************
 * end petes_coffee_and_tea_run()
 ******************************************/

/*!**************************************************************
 * @fn petes_coffee_and_tea()
 ****************************************************************
 * @brief
 *  This function executes the pete's coffee and tea scenario.
 *
 * @param  - None.               
 *
 * @return  - None.
 *
 * HISTORY:
 *  08/18/2008  Created.  RPF. 
 ****************************************************************/

void petes_coffee_and_tea(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;
    for(test_num = 0; test_num < PETES_COFFEE_AND_TEA_TEST_MAX; test_num++)
    {
    /* This scenario uses the "maui" configuration, which is a 1 port, 1 
     * enclosure configuration. 
     *  
     * Use the configuration specific load and validate routine. 
     * We always expect it to load and validate successfully. 
     */

	/* Now, run the scenario.  We always expect this to work.
     */
        maui_load_and_verify_table_driven(&test_table[test_num]);
        run_status = petes_coffee_and_tea_run(&test_table[test_num]);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);
       fbe_test_physical_package_tests_config_unload();
    }
    return;
}
/******************************************
 * end petes_coffee_and_tea()
 ******************************************/

/*************************
 * end file petes_coffee_and_tea.c
 *************************/
