/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * zhiqis_coffee_and_tea.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the zhiqis_coffee_and_tea scenario.
 * 
 *  This scenario is for a 1 port, 1 enclosure configuration, aka the maui.xml
 *  configuration.
 * 
 *  The test run in this case is:
 *  1) Create and validate the configuration. (1-Port 1-Enclosure)
 *  2) One Physical Drive Object sends a RESET command along the protocol edge to its Port Object.
 *     Port Object sends it down to miniport and then to the terminator test.
 *  3) Causes the port to receive a logout for the drive in slot-0, but ESES remains on.
 *  4) Validate that the physical drive is in the ACTIVATE state.
 *  5) Cause the port to receive a login for the SAS address of the drive in slot-0, and 
 *     the physical drive INQUIRY data is unchanged. 
 *  6) Validate that both the physical drive and logical drive are in the READY state.
 *  7) Shutdown the physical package.
 *
 * HISTORY
 *   08/01/2008:  Created. CLC
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"

#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"

/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @enum zhiqis_coffee_and_tea_test_number_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * zhiqis_coffee_and_tea_test.
 *********************************************************************/
typedef enum zhiqis_coffee_and_tea_test_number_e
{
    ZHIQIS_COFFEE_AND_TEA_TEST_NAGA,
    ZHIQIS_COFFEE_AND_TEA_TEST_VIPER,
    ZHIQIS_COFFEE_AND_TEA_DERRINGER,
    ZHIQIS_COFFEE_AND_TEA_TABASCO,
    ZHIQIS_COFFEE_AND_TEA_TEST_BUNKER,
    ZHIQIS_COFFEE_AND_TEA_CITADEL,
    ZHIQIS_COFFEE_AND_TEA_TEST_FALLBACK,
    ZHIQIS_COFFEE_AND_TEA_TEST_BOXWOOD,
    ZHIQIS_COFFEE_AND_TEA_TEST_KNOT,
    ZHIQIS_COFFEE_AND_TEA_TEST_ANCHO,
    ZHIQIS_COFFEE_AND_TEA_TEST_CALYPSO,
    ZHIQIS_COFFEE_AND_TEA_TEST_RHEA,
    ZHIQIS_COFFEE_AND_TEA_TEST_MIRANDA,
} zhiqis_coffee_and_tea_test_number_t;

/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/
static fbe_test_params_t test_table[] = 
{
    {ZHIQIS_COFFEE_AND_TEA_TEST_NAGA,
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
    {ZHIQIS_COFFEE_AND_TEA_TEST_VIPER,
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
    {ZHIQIS_COFFEE_AND_TEA_DERRINGER,
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
    {ZHIQIS_COFFEE_AND_TEA_TABASCO,
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
    {ZHIQIS_COFFEE_AND_TEA_TEST_BUNKER,
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
    {ZHIQIS_COFFEE_AND_TEA_CITADEL,
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
    {ZHIQIS_COFFEE_AND_TEA_TEST_FALLBACK,
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
    {ZHIQIS_COFFEE_AND_TEA_TEST_BOXWOOD,
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
    {ZHIQIS_COFFEE_AND_TEA_TEST_KNOT,
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
    {ZHIQIS_COFFEE_AND_TEA_TEST_ANCHO,
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
    {ZHIQIS_COFFEE_AND_TEA_TEST_CALYPSO,
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
    {ZHIQIS_COFFEE_AND_TEA_TEST_RHEA,
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
    {ZHIQIS_COFFEE_AND_TEA_TEST_MIRANDA,
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
} ;

/*!*******************************************************************
 * @def ZHIQIS_COFFEE_AND_TEA_TEST_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define ZHIQIS_COFFEE_AND_TEA_TEST_MAX     sizeof(test_table)/sizeof(test_table[0])


static fbe_status_t validate_physical_drive(fbe_test_params_t *test,
                                            fbe_lifecycle_state_t expected_lifecycle_state)
{
    fbe_lifecycle_state_t lifecycle_state;
    fbe_status_t status;
    fbe_u32_t object_handle_p;

    /* Check for the physical drives on the enclosure.
     * If status was bad, then the drive does not exist.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(test->backend_number,
                                                                    test->encl_number,
                                                                    test->drive_number,
                                                                    &object_handle_p);
         
    /* If status was bad, then return error.
     */
    if (status !=  FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_LOW,
                   "%s: ERROR can't get physical drive [%u] in slot [%u], enclosure [%u] on port [%u] lifecycle state, status: 0x%X for %s.",
                   __FUNCTION__, object_handle_p, test->drive_number, test->encl_number, test->backend_number, status, test->title);
        return status;
    }

    /* Get the lifecycle state of the drive.
     */
    status = fbe_api_get_object_lifecycle_state(object_handle_p, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
      
    /* If status was bad, then return error.
     */
    if (status !=  FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_LOW,
                   "%s: ERROR can't get lifecycle state [%u] in slot [%u], enclosure [%u] on port [%u] lifecycle state, status: 0x%X.for %s",
                   __FUNCTION__, object_handle_p, test->drive_number, test->encl_number, test->backend_number, status, test->title);
        return status;
    }

    /* If the lifecycle state does not match, then display and return error.
     */
    if (lifecycle_state != expected_lifecycle_state)
    {
        mut_printf(MUT_LOG_LOW,
                   "%s: ERROR - Lifecycle state does not match!, state: 0x%X for %s",
                   __FUNCTION__, lifecycle_state, test->title);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return FBE_STATUS_OK;
}

static fbe_status_t zhiqis_coffee_and_tea_run(fbe_test_params_t *test)
{
    fbe_status_t status;
    fbe_api_terminator_device_handle_t device_id;
    //fbe_u32_t object_handle_p;
  
    mut_printf(MUT_LOG_LOW, "=== zhiqi_coffee_and_tea for %s Starting ===\n", test->title);
    
    /* Send a RESET command along the protocol edge to its Port Object.
     * Port Object sends it down to miniport and then to the terminator test. 
     */
    //status = fbe_api_get_physical_drive_object_handle(0, 0, 0, &object_handle_p);
    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //status = fbe_api_drive_reset(object_handle_p); 
    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* Wait some reasonable time for states to stabilize.
     */
    //Sleep(10000);    
    
    /* Get the device hanlde of the drive.
     */
    status = fbe_api_terminator_get_drive_handle(test->backend_number,
                                                 test->encl_number,
                                                 test->drive_number,
                                                 &device_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
                                                     
    /* Force logout of a drive without changing the insert bit.
     */
    status = fbe_api_terminator_force_logout_device(device_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait some reasonable time for states to stabilize.
     */
    EmcutilSleep(5000);

    /* Make sure that physical drives are in ACTIVATE.
     */
    status = validate_physical_drive(test, FBE_LIFECYCLE_STATE_ACTIVATE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* Force the login to occur.
     */
    status = fbe_api_terminator_force_login_device(device_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait some reasonable time for states to stabilize.
     */
    EmcutilSleep(5000);

    /* Make sure that physical drives are in READY.
     */
    status = validate_physical_drive(test, FBE_LIFECYCLE_STATE_READY);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return FBE_STATUS_OK;
}

void zhiqis_coffee_and_tea(void)
{
   fbe_status_t run_status;
    fbe_u32_t test_num;

    for(test_num = 0; test_num < ZHIQIS_COFFEE_AND_TEA_TEST_MAX; test_num++)
    {
        /* Load the configuration for test
         */
        maui_load_and_verify_table_driven(&test_table[test_num]);
       run_status = zhiqis_coffee_and_tea_run(&test_table[test_num]);

        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
         */
        fbe_test_physical_package_tests_config_unload();
    }
}

