/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * ring_of_kerry.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Ring of Kerry scenario.
 * 
 *  This scenario is for a 1 port, 1 enclosure configuration, aka the maui
 *  configuration.
 * 
 *  The test run in this case is:
 *      - Initialize the 1-Port, 1-Enclosure configuration and 
 *          verify that the topology is correct.
 *      - Set display number to 123.
 *      - Verify the displayed number through EDAL.
 *      - Set display number to “---“.
 *      - Verify the displayed number through EDAL.
 *      - Shutdown the physical package. 
 *
 * HISTORY
 *   03/17/2009:  Created. Naizhong Chiu
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "physical_package_tests.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_enclosure_data_access_public.h"

/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @enum ring_of_kerry_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * ring_of_kerry.
 *********************************************************************/

typedef enum ring_of_kerry_e
{
    RING_OF_KERRY_TEST_NAGA,
    RING_OF_KERRY_TEST_VIPER,
    RING_OF_KERRY_TEST_DERRINGER,
    RING_OF_KERRY_TEST_VOYAGER,
    RING_OF_KERRY_TEST_ANCHO,
    RING_OF_KERRY_TEST_TABASCO,
} ring_of_kerry_t;


/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/

static fbe_test_params_t test_table[] = 
{
    {RING_OF_KERRY_TEST_NAGA,
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
    {RING_OF_KERRY_TEST_VIPER,
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
     {RING_OF_KERRY_TEST_DERRINGER,
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
    {RING_OF_KERRY_TEST_VOYAGER,
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
    {RING_OF_KERRY_TEST_ANCHO,
     "VIPER",
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
     {RING_OF_KERRY_TEST_TABASCO,
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
} ;

/*!*******************************************************************
 * @def STATIC_TEST_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define STATIC_TEST_MAX     sizeof(test_table)/sizeof(test_table[0])

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
fbe_status_t ring_of_kerry_run(fbe_test_params_t *test);

/*!***************************************************************
 * @fn  ring_of_kerry_run
 ****************************************************************
 * @brief:
 *  Function to sets display and verify the result.
 *
 * @return 
 *  FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *   03/17/2009:  Created. Naizhong Chiu
 *
 ****************************************************************/
fbe_status_t ring_of_kerry_run(fbe_test_params_t *test)
{
    fbe_u32_t           object_handle;
    fbe_u8_t            display_char;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_base_enclosure_led_status_control_t ledCommandInfo;

    mut_printf(MUT_LOG_LOW, "****Ring of Kerry for %s ****\n",test->title);	
    /* Get handle to enclosure object */
    status = fbe_api_get_enclosure_object_id_by_location(test->backend_number, test->encl_number, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    mut_printf(MUT_LOG_LOW, "Change Bus number to 01 and Encl number to 2");

    /* set up the command */
    fbe_zero_memory(&ledCommandInfo, sizeof(fbe_base_enclosure_led_status_control_t));
    ledCommandInfo.ledAction = FBE_ENCLOSURE_LED_CONTROL;
    ledCommandInfo.ledInfo.busNumber[1] = 0x31;
    ledCommandInfo.ledInfo.busNumber[0] = 0x30;
    ledCommandInfo.ledInfo.busDisplay = FBE_ENCLOSURE_LED_BEHAVIOR_DISPLAY;
    ledCommandInfo.ledInfo.enclNumber = 0x32;
    ledCommandInfo.ledInfo.enclDisplay = FBE_ENCLOSURE_LED_BEHAVIOR_DISPLAY;

    /* Issue the LED command to the enclosure */
    status = fbe_api_enclosure_send_set_led(object_handle, &ledCommandInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check the display chars in EDAL */
    display_char = 0x30;
    status = fbe_api_wait_for_encl_attr(object_handle, FBE_DISPLAY_CHARACTER_STATUS, 
                                            display_char, FBE_ENCL_DISPLAY, 0, 10000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Charactor 0 of Bus number is updated in EDAL");
    display_char = 0x31;
    status = fbe_api_wait_for_encl_attr(object_handle, FBE_DISPLAY_CHARACTER_STATUS, 
                                            display_char, FBE_ENCL_DISPLAY, 1, 10000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Charactor 1 of Bus number is updated in EDAL");
    display_char = 0x32;
    status = fbe_api_wait_for_encl_attr(object_handle, FBE_DISPLAY_CHARACTER_STATUS, 
                                            display_char, FBE_ENCL_DISPLAY, 2, 10000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Encl number is updated in EDAL");

	mut_printf(MUT_LOG_LOW, "Change Bus number to -- and Encl number to -");

    /* set up the command */
    fbe_zero_memory(&ledCommandInfo, sizeof(fbe_base_enclosure_led_status_control_t));
    ledCommandInfo.ledAction = FBE_ENCLOSURE_LED_CONTROL;
    ledCommandInfo.ledInfo.busNumber[1] = '-';
    ledCommandInfo.ledInfo.busNumber[0] = '-';
    ledCommandInfo.ledInfo.busDisplay = FBE_ENCLOSURE_LED_BEHAVIOR_DISPLAY;
    ledCommandInfo.ledInfo.enclNumber = '-';
    ledCommandInfo.ledInfo.enclDisplay = FBE_ENCLOSURE_LED_BEHAVIOR_DISPLAY;

    /* Issue the LED command to the enclosure */
    status = fbe_api_enclosure_send_set_led(object_handle, &ledCommandInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check the display chars in EDAL */
    display_char = '-';
    status = fbe_api_wait_for_encl_attr(object_handle, FBE_DISPLAY_CHARACTER_STATUS, 
                                            display_char, FBE_ENCL_DISPLAY, 0, 10000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Charactor 0 of Bus number is updated in EDAL");
    display_char = '-';
    status = fbe_api_wait_for_encl_attr(object_handle, FBE_DISPLAY_CHARACTER_STATUS, 
                                            display_char, FBE_ENCL_DISPLAY, 1, 10000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Charactor 1 of Bus number is updated in EDAL");
    display_char = '-';
    status = fbe_api_wait_for_encl_attr(object_handle, FBE_DISPLAY_CHARACTER_STATUS, 
                                            display_char, FBE_ENCL_DISPLAY, 2, 10000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Encl number is updated in EDAL");

    return status;
}


/*!***************************************************************
 * @fn ring_of_kerry()
 ****************************************************************
 * @brief:
 *  Function to load the config and run the ring of kerry scenario.
 *
 * @return 
 *  FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *   03/17/2009:  Created. Naizhong Chiu
 *
 ****************************************************************/

void ring_of_kerry(void)
{
    fbe_status_t run_status = FBE_STATUS_OK;
    fbe_u32_t test_num;

    for(test_num = 0; test_num < STATIC_TEST_MAX; test_num++)
    {
        /* Load the configuration for test
         */
        maui_load_and_verify_table_driven(&test_table[test_num]);
        run_status = ring_of_kerry_run(&test_table[test_num]);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
        */
        fbe_test_physical_package_tests_config_unload();
    }
}

