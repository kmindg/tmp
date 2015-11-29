/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * trichy.c (Enclosure Fault scenario)
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the Trichirapalli (Trichy) scenario and simulates
 *  
 *  - Power Supply Fault (AC Failed): This test will exercise element status
 *      page and power supply related code. 
 * 
 *  - Fan Fault (Singe fan Fault): This test will exercise element status
 *      page and cooling element code section. 
 *
 *  - Overtemp: This test will exercise element status page and 
 *      temperature section of the code. 
 *
 *  This scenario is for a 1 port, 1 enclosure configuration, aka the maui.xml
 *  configuration.
 * 
 *  The test for this case are:
 *  1) Load and verify the configuration. (1-Port 1-Enclosure)
 *  2) Cause the Power Supply fault.
 *  3) Retrive the enclosure status blob.
 *  4) Check the power supply status by using the enclosure provided parsing function.
 *  5) Clear the Power Supply fault.
 *  6) Poll the enclosure and update the internal data.
 *  7) Retrieve and verify the Power Supply status.
 *  8) Repeat steps 3 to 7 with fan and overtemp fault. 
 *  9) Shutdown the Terminator/Physical package.
 *
 * HISTORY
 *   08/26/2008:  Created. ArunKumar Selvapillai
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_eses.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_topology_interface.h"

/*************************
 *   MACRO DEFINITIONS
 *************************/
typedef enum 
{
    PS_FAULT = 0,
    FAN_FAULT,
    OT_FAULT,
}fault_component;


/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @enum trichirapally_test_number_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * trichirapally_test.
 *********************************************************************/
typedef enum trichirapally_test_number_e
{
    TRICHIRAPALLY_TEST_VIPER,
    TRICHIRAPALLY_TEST_DERRINGER,
    TRICHIRAPALLY_TEST_VOYAGER,
    TRICHIRAPALLY_TEST_ANCHO,
    TRICHIRAPALLY_TEST_CAYENNE,
    TRICHIRAPALLY_TEST_VIKING,
    TRICHIRAPALLY_TEST_NAGA,
    TRICHIRAPALLY_TEST_TABASCO,
} trichirapally_test_number_t;

/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/
static fbe_test_params_t test_table[] =
{
     {TRICHIRAPALLY_TEST_VIPER,
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
    {TRICHIRAPALLY_TEST_DERRINGER,
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
    {TRICHIRAPALLY_TEST_VOYAGER,
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
     DRIVE_MASK_IS_NOT_USED,
    },    
    {TRICHIRAPALLY_TEST_ANCHO,
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
    {TRICHIRAPALLY_TEST_CAYENNE,
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
     DRIVE_MASK_IS_NOT_USED,
    },
    {TRICHIRAPALLY_TEST_VIKING,
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
     DRIVE_MASK_IS_NOT_USED,
    },
    {TRICHIRAPALLY_TEST_NAGA,
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
     MAX_DRIVE_COUNT_VIKING,
    
     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {TRICHIRAPALLY_TEST_TABASCO,
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
    }
};

/*!*******************************************************************
 * @def TRICHIRAPALLY_TEST_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define TRICHIRAPALLY_TEST_MAX     sizeof(test_table)/sizeof(test_table[0])

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/****************************************************************
 * trichy_fault_overtemp()
 ****************************************************************
 * Description:
 *  Function to simulate the Overtemp fault
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  09/12/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

static fbe_status_t trichy_fault_overtemp(fbe_u32_t port, fbe_api_terminator_device_handle_t encl_id, fbe_u32_t object_handle)
{
    fbe_status_t    ot_status;

    ses_stat_elem_temp_sensor_struct    temp_sensor_stat;

    /* Get the Temperature Status before introducing the fault */
    ot_status = fbe_api_terminator_get_temp_sensor_eses_status(encl_id, TEMP_SENSOR_0, &temp_sensor_stat);
    MUT_ASSERT_TRUE(ot_status == FBE_STATUS_OK);

    /* Set the Overtemp fault */
    temp_sensor_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    ot_status = fbe_api_terminator_set_temp_sensor_eses_status(encl_id, TEMP_SENSOR_0, temp_sensor_stat);
    MUT_ASSERT_TRUE(ot_status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Temperature Sensor is faulted");

    /* Verify the attribute is Set */
    ot_status = fbe_api_wait_for_fault_attr(object_handle, FBE_ENCL_COMP_INSERTED, FALSE, FBE_ENCL_TEMP_SENSOR, FBE_EDAL_LCCA_TEMP_SENSOR_0, 8000);
    MUT_ASSERT_TRUE(ot_status == FBE_STATUS_OK);

    /* Get the Temperature Status before clearing the fault */
    ot_status = fbe_api_terminator_get_temp_sensor_eses_status(encl_id, TEMP_SENSOR_0, &temp_sensor_stat);
    MUT_ASSERT_TRUE(ot_status == FBE_STATUS_OK);

    /* Clear the Overtemp fault */
    temp_sensor_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    ot_status = fbe_api_terminator_set_temp_sensor_eses_status(encl_id, TEMP_SENSOR_0, temp_sensor_stat);
    MUT_ASSERT_TRUE(ot_status == FBE_STATUS_OK);

    /* Verify the attribute is cleared */
    ot_status = fbe_api_wait_for_fault_attr(object_handle, FBE_ENCL_COMP_INSERTED, TRUE, FBE_ENCL_TEMP_SENSOR, FBE_EDAL_LCCA_TEMP_SENSOR_0, 8000);
    MUT_ASSERT_TRUE(ot_status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Temperature Sensor fault is now cleared");

    return FBE_STATUS_OK;
}

/****************************************************************
 * trichy_fault_fan()
 ****************************************************************
 * Description:
 *  Function to simulate the Fan fault
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  09/12/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

static fbe_status_t trichy_fault_fan(fbe_u32_t port, fbe_api_terminator_device_handle_t encl_id, fbe_u32_t object_handle)
{
    fbe_status_t    fan_status;

    ses_stat_elem_cooling_struct    cooling_status;

    /* Get the Fan Status before introducing the fault */
    fan_status = fbe_api_terminator_get_cooling_eses_status(encl_id, COOLING_0, &cooling_status);
    MUT_ASSERT_TRUE(fan_status == FBE_STATUS_OK);

    /* Cause the Fan fault on enclosure 0 */
    cooling_status.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    fan_status = fbe_api_terminator_set_cooling_eses_status(encl_id, COOLING_0, cooling_status);
    MUT_ASSERT_TRUE(fan_status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Fan is faulted");
    /* Verify the attribute is Set */
    fan_status = fbe_api_wait_for_fault_attr(object_handle, FBE_ENCL_COMP_FAULTED, TRUE, FBE_ENCL_COOLING_COMPONENT, FBE_EDAL_PSA_COOLING_0, 8000);
    MUT_ASSERT_TRUE(fan_status == FBE_STATUS_OK);

    /* Get the Fan Status before clearing the fault */
    fan_status = fbe_api_terminator_get_cooling_eses_status(encl_id, COOLING_0, &cooling_status);
    MUT_ASSERT_TRUE(fan_status == FBE_STATUS_OK);

    /* Clear the Fan fault */
    cooling_status.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    fan_status = fbe_api_terminator_set_cooling_eses_status(encl_id, COOLING_0, cooling_status);
    MUT_ASSERT_TRUE(fan_status == FBE_STATUS_OK);

    /* Verify the attribute is cleared */
    fan_status = fbe_api_wait_for_fault_attr(object_handle, FBE_ENCL_COMP_FAULTED, FALSE, FBE_ENCL_COOLING_COMPONENT, FBE_EDAL_PSA_COOLING_0, 8000);
    MUT_ASSERT_TRUE(fan_status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Fan fault is now cleared");

    return FBE_STATUS_OK;
}

/****************************************************************
 * trichy_fault_ps()
 ****************************************************************
 * Description:
 *  Function to simulate the Power Supply AC_FAIL fault
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  09/12/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

static fbe_status_t trichy_fault_ps(fbe_u32_t port, fbe_api_terminator_device_handle_t encl_id, fbe_u32_t object_handle)
{
    fbe_status_t    ps_status;

    ses_stat_elem_ps_struct     ps_struct;

    /* Cause the power supply AC_FAIL on enclosure 0 */
    ps_status = fbe_api_terminator_get_ps_eses_status(encl_id, PS_0, &ps_struct);
    MUT_ASSERT_TRUE(ps_status == FBE_STATUS_OK);

    /* Set the AC_FAIL to TRUE */
    ps_struct.ac_fail = TRUE;
    ps_status = fbe_api_terminator_set_ps_eses_status(encl_id, PS_0, ps_struct);
    MUT_ASSERT_TRUE(ps_status == FBE_STATUS_OK);

    /* Verify the attribute is Set */
    ps_status = fbe_api_wait_for_fault_attr(object_handle, FBE_ENCL_PS_AC_FAIL, TRUE, FBE_ENCL_POWER_SUPPLY, 0, 8000);
    MUT_ASSERT_TRUE(ps_status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Power Supply is Faulted");

    /* Clear the Power Supply fault */
    ps_status = fbe_api_terminator_get_ps_eses_status(encl_id, PS_0, &ps_struct);
    MUT_ASSERT_TRUE(ps_status == FBE_STATUS_OK);

    /* Set the AC_FAIL to FALSE */
    ps_struct.ac_fail = FALSE;
    ps_status = fbe_api_terminator_set_ps_eses_status(encl_id, PS_0, ps_struct);
    MUT_ASSERT_TRUE(ps_status == FBE_STATUS_OK);

    /* Verify the attribute is cleared */
    ps_status = fbe_api_wait_for_fault_attr(object_handle, FBE_ENCL_PS_AC_FAIL, FALSE, FBE_ENCL_POWER_SUPPLY, 0, 8000);
    MUT_ASSERT_TRUE(ps_status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Power Supply fault is now cleared");

    return FBE_STATUS_OK;
}

/****************************************************************
 * trichy_fault_component()
 ****************************************************************
 * Description:
 *  Function to simulate the enclosure faults for 
 *  - Power Supply 
 *  - Fan
 *  - Overtemp
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  09/15/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

static fbe_status_t trichy_fault_component(fbe_u32_t component,
                                                                        fbe_test_params_t *test)
{
    fbe_status_t    fault_status;
    fbe_u32_t       object_handle;

    fbe_api_terminator_device_handle_t    encl_device_id;

    /* Get the object handle */
    fault_status = fbe_api_get_enclosure_object_id_by_location(
                                                     test->backend_number,
                                                     test->encl_number,
                                                     &object_handle);
    MUT_ASSERT_TRUE(fault_status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    /* Get the device id of the enclosure object */
    fault_status = fbe_api_terminator_get_enclosure_handle(test->backend_number,
                                                           test->encl_number,
                                                           &encl_device_id);
    MUT_ASSERT_TRUE(fault_status == FBE_STATUS_OK);

    switch (component)
    {
        /* Power Supply */
        case PS_FAULT:
        {
            mut_printf(MUT_LOG_LOW, "Trichirapalli Scenario: Power Supply Fault - Start for %s", test->title);

            /* Cause the Power Supply fault */
            fault_status = trichy_fault_ps(0, encl_device_id, object_handle);
            MUT_ASSERT_TRUE(fault_status == FBE_STATUS_OK);

            mut_printf(MUT_LOG_LOW, "Trichirapalli Scenario: Power Supply Fault - End for %s", test->title);
        }
        break;

        /* Fan */
        case FAN_FAULT:
        {
            mut_printf(MUT_LOG_LOW, "Trichirapalli Scenario: Fan Fault - Start for %s", test->title);

            /* Cause the Fan fault */
            fault_status = trichy_fault_fan(0, encl_device_id, object_handle);
            MUT_ASSERT_TRUE(fault_status == FBE_STATUS_OK);

            mut_printf(MUT_LOG_LOW, "Trichirapalli Scenario: Fan Fault - End for %s", test->title);
        }
        break;

        /* OverTemp */
        case OT_FAULT:
        {
            mut_printf(MUT_LOG_LOW, "Trichirapalli Scenario: OverTemp Fault - Start for %s", test->title);

            /* Cause the Overtemp fault */
            fault_status = trichy_fault_overtemp(test->backend_number, encl_device_id, object_handle);
            MUT_ASSERT_TRUE(fault_status == FBE_STATUS_OK);

            mut_printf(MUT_LOG_LOW, "Trichirapalli Scenario: OverTemp Fault - End for %s", test->title);
        }
        break;

        default:
        {
            mut_printf(MUT_LOG_LOW, "Trichirapalli Scenario: End %s for %s", __FUNCTION__,  test->title);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        break;
    }

    return fault_status;
}

/****************************************************************
 * trichy_run()
 ****************************************************************
 * Description:
 *  Function to simulate the enclosure faults for 
 *  - Power Supply 
 *  - Fan
 *  - Overtemp
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  08/26/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

static fbe_status_t trichy_run(fbe_test_params_t *test)
{
    fbe_u32_t       component;
    fbe_status_t    trichy_status;

    mut_printf(MUT_LOG_LOW, "Trichirapalli Scenario: Start for %s", test->title);

    for (component = PS_FAULT; component <= OT_FAULT; component++ )
    {
        trichy_status = trichy_fault_component(component, test);
        MUT_ASSERT_TRUE(trichy_status == FBE_STATUS_OK);
    }

    mut_printf(MUT_LOG_LOW, "Trichirapalli Scenario: End for %s", test->title);

    return FBE_STATUS_OK;
}

/****************************************************************
 * trichy()
 ****************************************************************
 * Description:
 *  Function to run the trichy scenario.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  08/26/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

void trichirapalli(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;

    for(test_num = 0; test_num < TRICHIRAPALLY_TEST_MAX; test_num++)
    {
        /* Load the configuration for test
         */
        maui_load_and_verify_table_driven(&test_table[test_num]);
        run_status = trichy_run(&test_table[test_num]);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
         */
        fbe_test_physical_package_tests_config_unload();
    }
}
