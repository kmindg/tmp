/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
#include "esp_tests.h"
#include "physical_package_tests.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_registry.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_eses.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_test_esp_utils.h"
#include "fbe_base_environment_debug.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_power_supply_interface.h"
#include "fbe/fbe_api_cooling_interface.h"

 /*************************
 *   MACRO DEFINITIONS
 *************************/
typedef enum 
{
    PS_REMOVAL = 0,
    FAN_REMOVAL,
}fault_component;
 
 /*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @enum grylls_test_number_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * grylls_test.
 *********************************************************************/
typedef enum grylls_test_number_e
{
    GRYLLS_TEST_VIKING,
    GRYLLS_TEST_NAGA,
} grylls_test_number_t;

/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/
static fbe_test_params_t test_table[] = 
{
    {GRYLLS_TEST_VIKING,
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
    {GRYLLS_TEST_NAGA,
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
     DRIVE_MASK_IS_NOT_USED,
    }
} ;

/*!*******************************************************************
 * @def GRYLLS_TEST_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define GRYLLS_TEST_MAX     sizeof(test_table)/sizeof(test_table[0])

/****************************************************************
 * grylls_remove_fan()
 ****************************************************************
 * Description:
 *  Function to simulate the Fan removal
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  12/04/2013 - zhoue1
 *
 ****************************************************************/
static fbe_status_t grylls_remove_fan(fbe_u32_t port, fbe_api_terminator_device_handle_t device_handle, fbe_u32_t object_id)
{
    fbe_status_t                        status;
    fbe_device_physical_location_t      location = {0};
    ses_stat_elem_cooling_struct        cooling_struct;
    fbe_u32_t                           fan_count;
    fbe_u8_t                            index;

    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;

    status = fbe_api_cooling_get_fan_count(object_id, &fan_count);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    for (index=0; index<fan_count; index++)
    {
        /* Get the Fan Status before introducing the fault */
        status = fbe_api_terminator_get_cooling_eses_status(device_handle, index, &cooling_struct);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        
        /* Remove the Fan.
         */
        cooling_struct.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
        status = fbe_api_terminator_set_cooling_eses_status(device_handle, 
                                                       index, 
                                                       cooling_struct);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        location.slot = index;
        status = fbe_test_pp_wait_for_cooling_status(object_id, &location, FALSE /* Removed */, FBE_MGMT_STATUS_FALSE, 30000);
        MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for FAN removal failed!");
        
        mut_printf(MUT_LOG_LOW, "FAN(0_0_%d) is removed.", location.slot);
        
        /* Insert the Fan.
         */
        cooling_struct.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
        status = fbe_api_terminator_set_cooling_eses_status(device_handle, 
                                                       index, 
                                                       cooling_struct);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        
        status = fbe_test_pp_wait_for_cooling_status(object_id, &location, TRUE /* Inserted */, FBE_MGMT_STATUS_FALSE, 30000);
        MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for FAN insertion failed!");
        
        mut_printf(MUT_LOG_LOW, "FAN(0_0_%d) is inserted.", location.slot);
    }

    return FBE_STATUS_OK;
}

/****************************************************************
 * grylls_remove_ps()
 ****************************************************************
 * Description:
 *  Function to simulate the Power Supply removal
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  12/04/2013 - zhoue1
 *
 ****************************************************************/
static fbe_status_t grylls_remove_ps(fbe_u32_t port, fbe_api_terminator_device_handle_t device_handle, fbe_u32_t object_id)
{
    fbe_status_t    status;
    fbe_device_physical_location_t       location = {0};
    ses_stat_elem_ps_struct     ps_Struct;

    location.bus = 0;
    location.enclosure = 0;

    /* PS A1
     */
    location.slot = 0;
    /* Get the present eses status before inserting the fault.
     */    
    status = fbe_api_terminator_get_ps_eses_status(device_handle, 
                                                   PS_0, 
                                                   &ps_Struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Remove the power supply.
     */
    ps_Struct.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    status = fbe_api_terminator_set_ps_eses_status(device_handle, 
                                                   PS_0, 
                                                   ps_Struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_pp_wait_for_ps_status(object_id, &location, FALSE /* Removed */, FBE_MGMT_STATUS_FALSE, 30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for PS removal failed!");

    mut_printf(MUT_LOG_LOW, "PS(0_0_%d) is removed.", location.slot);

    /* Insert the power supply.
     */
    ps_Struct.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    status = fbe_api_terminator_set_ps_eses_status(device_handle, 
                                                   PS_0, 
                                                   ps_Struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_pp_wait_for_ps_status(object_id, &location, TRUE /* Inserted */, FBE_MGMT_STATUS_FALSE, 30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for PS insertion failed!");

    mut_printf(MUT_LOG_LOW, "PS(0_0_%d) is inserted.", location.slot);

    /* PS B1
     */
    location.slot = 1;
    /* Get the present eses status before inserting the fault.
     */    
    status = fbe_api_terminator_get_ps_eses_status(device_handle, 
                                                   PS_1, 
                                                   &ps_Struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Remove the power supply.
     */
    ps_Struct.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    status = fbe_api_terminator_set_ps_eses_status(device_handle, 
                                                   PS_1, 
                                                   ps_Struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_pp_wait_for_ps_status(object_id, &location, FALSE /* Removed */, FBE_MGMT_STATUS_FALSE, 30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for PS removal failed!");

    mut_printf(MUT_LOG_LOW, "PS(0_0_%d) is removed.", location.slot);

    /* Insert the power supply.
     */
    ps_Struct.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    status = fbe_api_terminator_set_ps_eses_status(device_handle, 
                                                   PS_1, 
                                                   ps_Struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_pp_wait_for_ps_status(object_id, &location, TRUE /* Inserted */, FBE_MGMT_STATUS_FALSE, 30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for PS insertion failed!");

    mut_printf(MUT_LOG_LOW, "PS(0_0_%d) is inserted.", location.slot);

    /* PS A0
     */
    location.slot = 2;
    /* Get the present eses status before inserting the fault.
     */    
    status = fbe_api_terminator_get_ps_eses_status(device_handle, 
                                                   PS_2, 
                                                   &ps_Struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Remove the power supply.
     */
    ps_Struct.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    status = fbe_api_terminator_set_ps_eses_status(device_handle, 
                                                   PS_2, 
                                                   ps_Struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_pp_wait_for_ps_status(object_id, &location, FALSE /* Removed */, FBE_MGMT_STATUS_FALSE, 30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for PS removal failed!");

    mut_printf(MUT_LOG_LOW, "PS(0_0_%d) is removed.", location.slot);

    /* Insert the power supply.
     */
    ps_Struct.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    status = fbe_api_terminator_set_ps_eses_status(device_handle, 
                                                   PS_2, 
                                                   ps_Struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_pp_wait_for_ps_status(object_id, &location, TRUE /* Inserted */, FBE_MGMT_STATUS_FALSE, 30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for PS insertion failed!");

    mut_printf(MUT_LOG_LOW, "PS(0_0_%d) is inserted.", location.slot);


    /* PS B0
     */
    location.slot = 3;
    /* Get the present eses status before inserting the fault.
     */    
    status = fbe_api_terminator_get_ps_eses_status(device_handle, 
                                                   PS_3, 
                                                   &ps_Struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Remove the power supply.
     */
    ps_Struct.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    status = fbe_api_terminator_set_ps_eses_status(device_handle, 
                                                   PS_3, 
                                                   ps_Struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_pp_wait_for_ps_status(object_id, &location, FALSE /* Removed */, FBE_MGMT_STATUS_FALSE, 30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for PS removal failed!");

    mut_printf(MUT_LOG_LOW, "PS(0_0_%d) is removed.", location.slot);

    /* Insert the power supply.
     */
    ps_Struct.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    status = fbe_api_terminator_set_ps_eses_status(device_handle, 
                                                   PS_3, 
                                                   ps_Struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_pp_wait_for_ps_status(object_id, &location, TRUE /* Inserted */, FBE_MGMT_STATUS_FALSE, 30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for PS insertion failed!");

    mut_printf(MUT_LOG_LOW, "PS(0_0_%d) is inserted.", location.slot);

    return FBE_STATUS_OK;
}


/****************************************************************
 * grylls_remove_component()
 ****************************************************************
 * Description:
 *  Function to simulate the enclosure removal insertion for 
 *  - Power Supply 
 *  - Fan
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  12/04/2013 - zhoue1
 *
 ****************************************************************/

static fbe_status_t grylls_remove_component(fbe_u32_t component,
                                                    fbe_test_params_t *test)
{
    fbe_status_t    fault_status;
    fbe_u32_t       object_id;

    fbe_api_terminator_device_handle_t    encl_device_handle;

    /* Get the object handle */
    fault_status = fbe_api_get_enclosure_object_id_by_location(
                                                     test->backend_number,
                                                     test->encl_number,
                                                     &object_id);
    MUT_ASSERT_TRUE(fault_status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);

    /* Get the device id of the enclosure object */
    fault_status = fbe_api_terminator_get_enclosure_handle(test->backend_number,
                                                           test->encl_number,
                                                           &encl_device_handle);
    MUT_ASSERT_TRUE(fault_status == FBE_STATUS_OK);

    switch (component)
    {
        /* Power Supply */
        case PS_REMOVAL:
        {
            mut_printf(MUT_LOG_LOW, "Grylls Scenario: Power Supply Removal - Start for %s", test->title);

            /* Cause the Power Supply fault */
            fault_status = grylls_remove_ps(0, encl_device_handle, object_id);
            MUT_ASSERT_TRUE(fault_status == FBE_STATUS_OK);

            mut_printf(MUT_LOG_LOW, "Grylls Scenario: Power Supply Removal - End for %s", test->title);
        }
        break;

        /* Fan */
        case FAN_REMOVAL:
        {
            mut_printf(MUT_LOG_LOW, "Grylls Scenario: Fan Removal - Start for %s", test->title);

            /* Cause the Fan fault */
            fault_status = grylls_remove_fan(0, encl_device_handle, object_id);
            MUT_ASSERT_TRUE(fault_status == FBE_STATUS_OK);

            mut_printf(MUT_LOG_LOW, "Grylls Scenario: Fan Removal - End for %s", test->title);
        }
        break;

        default:
        {
            mut_printf(MUT_LOG_LOW, "Grylls Scenario: End %s for %s", __FUNCTION__,  test->title);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        break;
    }

    return fault_status;
}

/****************************************************************
 * grylls_run()
 ****************************************************************
 * Description:
 *  Function to simulate the enclosure removal insertion for 
 *  - Power Supply 
 *  - Fan
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  12/03/2013 - zhoue1
 *
 ****************************************************************/
static fbe_status_t grylls_run(fbe_test_params_t *test)
{
    fbe_u32_t       component;
    fbe_status_t    status;

    mut_printf(MUT_LOG_LOW, "Grylls Scenario: Start for %s", test->title);

    for (component = PS_REMOVAL; component <= FAN_REMOVAL; component++ )
    {
        status = grylls_remove_component(component, test);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }

    mut_printf(MUT_LOG_LOW, "Grylls Scenario: End for %s", test->title);

    return FBE_STATUS_OK;
}

/****************************************************************
 * grylls()
 ****************************************************************
 * Description:
 *  Function to load, unload the config and run the grylls scenario.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  12/03/2013 - zhoue1.
 *
 ****************************************************************/

void grylls(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;

    for(test_num = 0; test_num < GRYLLS_TEST_MAX; test_num++)
    {
        /* Load the configuration for test
         */
        maui_load_and_verify_table_driven(&test_table[test_num]);
        run_status = grylls_run(&test_table[test_num]);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
         */
        fbe_test_physical_package_tests_config_unload();
    }
}
