/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * cliffs_of_moher.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This test simulates Drives being removed & inserted by cru_on_off commands.
 *  This scenario is for a 1 port, 1 enclosures configuration, aka the maui.xml
 *  configuration.
 * 
 *  The test run in this case is:
 * 1. Load and verify the 1-Port, 1-Enclosure configuration. (use Maui configuration)
 * 2. Issue the GET_ENCLOSURE_INFO command to get EDAL blob from Enclosure object.
 * 3. Use EDAL set functions to:
 *      a. Drive Debug actions (CruOff & CruOn).
 *      b. Drive Power actions (PowerDown & PowerUp).
 *      c. Expander Reset
 * 4. Send SET_ENCLOSURE_CONTROL command to Enclosure object.
 * 5. Issue GET_ENCLOSURE_INFO commands to verify that the actions occurred.
 *
 * HISTORY
 *   12/11/2008:  Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure_data_access_public.h"


/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @enum cliffs_of_moher_test_number_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * cliffs_of_moher_test.
 *********************************************************************/
typedef enum cliffs_of_moher_test_number_e
{
    CLIFFS_OF_MOHER_TEST_VIPER,
    CLIFFS_OF_MOHER_TEST_DERRINGER,
    CLIFFS_OF_MOHER_TEST_BUNKER,
    CLIFFS_OF_MOHER_TEST_CITADEL,
    CLIFFS_OF_MOHER_TEST_FALLBACK,
    CLIFFS_OF_MOHER_TEST_BOXWOOD,
    CLIFFS_OF_MOHER_TEST_KNOT,
    CLIFFS_OF_MOHER_TEST_ANCHO,
    CLIFFS_OF_MOHER_TEST_CALYPSO,
    CLIFFS_OF_MOHER_TEST_RHEA,
    CLIFFS_OF_MOHER_TEST_MIRANDA,
    CLIFFS_OF_MOHER_TEST_TABASCO,
} cliffs_of_moher_test_number_t;

/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/
static fbe_test_params_t test_table[] = 
{   
    {CLIFFS_OF_MOHER_TEST_VIPER,
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
    {CLIFFS_OF_MOHER_TEST_DERRINGER,
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
    {CLIFFS_OF_MOHER_TEST_BUNKER,
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
    {CLIFFS_OF_MOHER_TEST_CITADEL,
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
    {CLIFFS_OF_MOHER_TEST_FALLBACK,
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

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {CLIFFS_OF_MOHER_TEST_BOXWOOD,
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

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {CLIFFS_OF_MOHER_TEST_KNOT,
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

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },    
    {CLIFFS_OF_MOHER_TEST_ANCHO,
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
// Moons
    {CLIFFS_OF_MOHER_TEST_CALYPSO,
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

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {CLIFFS_OF_MOHER_TEST_RHEA,
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

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {CLIFFS_OF_MOHER_TEST_MIRANDA,
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

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {CLIFFS_OF_MOHER_TEST_TABASCO,
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
 * @def CLIFFS_OF_MOHER_TEST_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define CLIFFS_OF_MOHER_TEST_MAX     sizeof(test_table)/sizeof(test_table[0])


/*
 * Structure/table of info on what Drives to test
 */
// Cru On/Off testing
typedef struct
{
    fbe_u8_t                                enclNumber;
    fbe_u8_t                                driveNumber;
    fbe_base_object_mgmt_drv_dbg_action_t   action;
} drive_cruOnOff_info_t;

#define DRIVE_CRUONOFF_TABLE_COUNT    4
drive_cruOnOff_info_t cliffsOfMoherDriveCruOnOffTable[DRIVE_CRUONOFF_TABLE_COUNT] =
{   
    {   0,      1,      FBE_DRIVE_ACTION_REMOVE },
    {   0,      1,      FBE_DRIVE_ACTION_INSERT },
    {   0,      5,      FBE_DRIVE_ACTION_REMOVE },
    {   0,      5,      FBE_DRIVE_ACTION_INSERT }
};
// Power Down/Up/Cycle testing
typedef struct
{
    fbe_u8_t                                enclNumber;
    fbe_u8_t                                driveNumber;
    fbe_base_object_mgmt_drv_power_action_t action;
    // drivePowerCycleDuration is only valid for drive power cycle command. 
    // Reserved for other power control commands. 
    fbe_u8_t                                drivePowerCycleDuration; 
} drive_power_info_t;

#define DRIVE_POWER_TABLE_COUNT    4
drive_power_info_t cliffsOfMoherDrivePowerTable[DRIVE_POWER_TABLE_COUNT] =
{   
    {   0,      2,      FBE_DRIVE_POWER_ACTION_POWER_DOWN, 0 },
    {   0,      2,      FBE_DRIVE_POWER_ACTION_POWER_UP,   0 },
    {   0,      8,      FBE_DRIVE_POWER_ACTION_POWER_DOWN, 0 },
    {   0,      8,      FBE_DRIVE_POWER_ACTION_POWER_UP,   0 }
};
#define DRIVE_POWER_CYCLE_TABLE_COUNT    2
drive_power_info_t cliffsOfMoherDrivePowerCycleTable[DRIVE_POWER_CYCLE_TABLE_COUNT] =
{   
    {   0,      3,      FBE_DRIVE_POWER_ACTION_POWER_CYCLE, 20 },  /* the power off state will last 10 seconds. */
    {   0,      10,     FBE_DRIVE_POWER_ACTION_POWER_CYCLE, 20 }   /* the power off state will last 10 seconds. */
};
// Expander Reset testing
typedef struct
{
    fbe_u8_t                                enclNumber;
    fbe_base_object_mgmt_exp_ctrl_action_t  action;
} expander_info_t;

#define EXPANDER_TABLE_COUNT    1
drive_power_info_t cliffsOfMoherExpanderTable[EXPANDER_TABLE_COUNT] =
{   
    {   0,      FBE_EXPANDER_ACTION_COLD_RESET }
};

#define MAX_REQUEST_STATUS_COUNT    10

fbe_status_t cliffs_of_moher_run(fbe_test_params_t *test)
{
    fbe_u32_t                                       object_handle_p;
    fbe_status_t                                    status;
    fbe_edal_status_t                               edalStatus;
    fbe_u32_t                                       originalStateChangeCount;
    fbe_u32_t                                       newStateChangeCount;
    fbe_bool_t                                      driveInserted;
    fbe_bool_t                                      phyDisabled;	
    fbe_base_object_mgmt_set_enclosure_control_t    enclosureControlInfoBuffer;
    fbe_edal_block_handle_t                         enclosureControlInfo = &enclosureControlInfoBuffer;
    fbe_u8_t                                        requestCount = 0;
    fbe_u8_t                                        testPassCount;
    fbe_u8_t                                        driveNumber, enclNumber;
    fbe_bool_t                                      statusChangeFound = FALSE;
    fbe_base_object_mgmt_drv_dbg_ctrl_t             enclosureDriveDebugInfo;
    fbe_base_object_mgmt_drv_dbg_action_t           cruOnOffAction;
    fbe_bool_t                                      drivePoweredOff;
    fbe_base_object_mgmt_drv_power_ctrl_t           enclosureDrivePowerInfo;
    fbe_base_object_mgmt_drv_power_action_t         powerAction;
// jap - enable when Terminator support is available
#if FALSE
    fbe_base_object_mgmt_exp_ctrl_t                 expanderInfo;
    fbe_base_object_mgmt_exp_ctrl_action_t          expanderAction;
#endif

    /*
     * Drive Cru On/Off Testing
     */
    mut_printf(MUT_LOG_LOW, "***********************************\n");
    mut_printf(MUT_LOG_LOW, "**** Test Cru On/Off of Drives for %s ****\n", test->title);
    mut_printf(MUT_LOG_LOW, "***********************************\n\n");

    for (testPassCount = 0; testPassCount < DRIVE_CRUONOFF_TABLE_COUNT; testPassCount++)
    {
        /*
         * Bypass the appropriate Drive from table
         */
        enclNumber = cliffsOfMoherDriveCruOnOffTable[testPassCount].enclNumber;
        driveNumber = cliffsOfMoherDriveCruOnOffTable[testPassCount].driveNumber;
        cruOnOffAction = cliffsOfMoherDriveCruOnOffTable[testPassCount].action;
        mut_printf(MUT_LOG_LOW, "**** Pass %d, CRU_ON_OFF, ENCL %d, DRIVE %d, ACTION %s ****\n",
            testPassCount, enclNumber, driveNumber, 
            (cruOnOffAction == FBE_DRIVE_ACTION_REMOVE ? "CruOff" : "CruOn"));

        // Get handle to enclosure object
        status = fbe_api_get_enclosure_object_id_by_location(0, enclNumber, &object_handle_p);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

        // Request current enclosure data
        fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
        status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)&enclosureControlInfoBuffer);

        // Save State change count
        edalStatus = fbe_edal_getOverallStateChangeCount(enclosureControlInfo,
                                                     &originalStateChangeCount);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "**** Encl %d , originalStateChangeCount %d****\n",
            enclNumber, originalStateChangeCount);

        // Send Drive Debug command
        fbe_zero_memory(&enclosureDriveDebugInfo, sizeof(fbe_base_object_mgmt_drv_dbg_ctrl_t));
//    enclosureDriveDebugInfo.driveCount = FBE_SAS_VIPER_ENCLOSURE_NUMBER_OF_SLOTS;
        enclosureDriveDebugInfo.driveCount = test->max_drives;
        enclosureDriveDebugInfo.driveDebugAction[driveNumber] = cruOnOffAction;

        // send Control to enclosure
        status = fbe_api_enclosure_send_drive_debug_control(object_handle_p, &enclosureDriveDebugInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        // Verify the appropriate Drive/PHY actions
        statusChangeFound = FALSE;
        requestCount = 0;
        while (requestCount < MAX_REQUEST_STATUS_COUNT)
        {
            // Request current enclosure data
            requestCount++;
            fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
            status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

           fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)&enclosureControlInfoBuffer);

            // Check for State Change
            edalStatus = fbe_edal_getOverallStateChangeCount(enclosureControlInfo,
                                                         &newStateChangeCount);
            MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
            mut_printf(MUT_LOG_LOW, "**** Encl %d , requestCount %d, newStateChangeCount %d****\n",
                enclNumber, requestCount, newStateChangeCount);
            if (newStateChangeCount > originalStateChangeCount)
            {
                //State Change detected, so time to check if Drive is bypassed
                edalStatus = fbe_edal_getBool(enclosureControlInfo,
                                              FBE_ENCL_COMP_INSERTED,
                                              FBE_ENCL_DRIVE_SLOT,
                                              driveNumber,
                                              &driveInserted);
                MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
                edalStatus = fbe_edal_getDrivePhyBool(enclosureControlInfo,
                                                      FBE_ENCL_EXP_PHY_DISABLED,
                                                      driveNumber,
                                                      &phyDisabled);
                MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
                if (((cruOnOffAction == FBE_DRIVE_ACTION_REMOVE) && (!driveInserted) && (phyDisabled)) ||
                    ((cruOnOffAction == FBE_DRIVE_ACTION_INSERT) && (driveInserted) && (!phyDisabled)))
                {
                    statusChangeFound = TRUE;
                    break;
                }
                else
                {
                    originalStateChangeCount = newStateChangeCount;
                }
            }

            // Delay before requesting Enclosure status again
            fbe_api_sleep (3000);

        }   // end of while loop
        if (!statusChangeFound)
        {
            fbe_edal_printAllComponentData(enclosureControlInfo, fbe_edal_trace_func);
        }
        MUT_ASSERT_TRUE(statusChangeFound == TRUE);

    }

    /*
     * Drive Power Down/Up Testing
     */
    mut_printf(MUT_LOG_LOW, "**********************************\n");
    mut_printf(MUT_LOG_LOW, "**** Test Drive Power Down/Up for %s ****\n", test->title);
    mut_printf(MUT_LOG_LOW, "**********************************\n\n");

    for (testPassCount = 0; testPassCount < DRIVE_POWER_TABLE_COUNT; testPassCount++)
    {
        /*
         * Bypass the appropriate Drive from table
         */
        enclNumber = cliffsOfMoherDrivePowerTable[testPassCount].enclNumber;
        driveNumber = cliffsOfMoherDrivePowerTable[testPassCount].driveNumber;
        powerAction = cliffsOfMoherDrivePowerTable[testPassCount].action;
        mut_printf(MUT_LOG_LOW, "**** Pass %d, ENCL %d, DRIVE %d, ACTION %s ****\n",
            testPassCount, enclNumber, driveNumber, 
            (powerAction == FBE_DRIVE_POWER_ACTION_POWER_DOWN ? "PowerDown" : "PowerUp"));

        // Get handle to enclosure object
        status = fbe_api_get_enclosure_object_id_by_location(0, enclNumber, &object_handle_p);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

        // Request current enclosure data
        fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
        status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)&enclosureControlInfoBuffer);
        // Save State change count
        edalStatus = fbe_edal_getOverallStateChangeCount(enclosureControlInfo,
                                                     &originalStateChangeCount);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "**** Encl %d , originalStateChangeCount %d****\n",
            enclNumber, originalStateChangeCount);

        // Send Drive Debug command
        fbe_zero_memory(&enclosureDrivePowerInfo, sizeof(fbe_base_object_mgmt_drv_dbg_ctrl_t));
//    enclosureDrivePowerInfo.driveCount = FBE_SAS_VIPER_ENCLOSURE_NUMBER_OF_SLOTS;
        enclosureDrivePowerInfo.driveCount = test->max_drives;
        enclosureDrivePowerInfo.drivePowerAction[driveNumber] = powerAction;

        // send Control to enclosure
        status = fbe_api_enclosure_send_drive_power_control(object_handle_p, &enclosureDrivePowerInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        // Verify the appropriate Drive power status change
        statusChangeFound = FALSE;
        requestCount = 0;
        while (requestCount < MAX_REQUEST_STATUS_COUNT)
        {
            // Request current enclosure data
            requestCount++;
            fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_get_enclosure_info_t));
            status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

           fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)&enclosureControlInfoBuffer);
            // Check for State Change
            edalStatus = fbe_edal_getOverallStateChangeCount(enclosureControlInfo,
                                                         &newStateChangeCount);
            MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
            mut_printf(MUT_LOG_LOW, "**** Encl %d , requestCount %d, newStateChangeCount %d****\n",
                enclNumber, requestCount, newStateChangeCount);
            if (newStateChangeCount > originalStateChangeCount)
            {
                //State Change detected, so time to check if Drive is powered correctly
                edalStatus = fbe_edal_getBool(enclosureControlInfo,
                                              FBE_ENCL_COMP_POWERED_OFF,
                                              FBE_ENCL_DRIVE_SLOT,
                                              driveNumber,
                                              &drivePoweredOff);
                MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
                if (((powerAction == FBE_DRIVE_POWER_ACTION_POWER_DOWN) && (drivePoweredOff)) ||
                    ((powerAction == FBE_DRIVE_POWER_ACTION_POWER_UP) && (!drivePoweredOff)))
                {
                    statusChangeFound = TRUE;
                    break;
                }
                else
                {
                    originalStateChangeCount = newStateChangeCount;
                }
            }

            // Delay before requesting Enclosure status again
            fbe_api_sleep (3000);

        }   // end of while loop
        if (!statusChangeFound)
        {
            fbe_edal_printAllComponentData(enclosureControlInfo, fbe_edal_trace_func);
        }
        MUT_ASSERT_TRUE(statusChangeFound == TRUE);


    }

    mut_printf(MUT_LOG_LOW, "********************************\n");
    mut_printf(MUT_LOG_LOW, "**** Test Drive Power Cycle for %s ****\n", test->title);
    mut_printf(MUT_LOG_LOW, "********************************\n\n");

    for (testPassCount = 0; testPassCount < DRIVE_POWER_CYCLE_TABLE_COUNT; testPassCount++)
    {
        /*
         * Bypass the appropriate Drive from table
         */
        enclNumber = cliffsOfMoherDrivePowerCycleTable[testPassCount].enclNumber;
        driveNumber = cliffsOfMoherDrivePowerCycleTable[testPassCount].driveNumber;
        mut_printf(MUT_LOG_LOW, "**** Pass %d, ENCL %d, DRIVE %d ****\n",
            testPassCount, enclNumber, driveNumber);

        // Get handle to enclosure object
        status = fbe_api_get_enclosure_object_id_by_location(0, enclNumber, &object_handle_p);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

        // Request current enclosure data
        fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
        status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)&enclosureControlInfoBuffer);

        // Save State change count
        edalStatus = fbe_edal_getOverallStateChangeCount(enclosureControlInfo,
                                                     &originalStateChangeCount);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "**** Encl %d , originalStateChangeCount %d****\n",
            enclNumber, originalStateChangeCount);

        // Send Drive Debug command
        fbe_zero_memory(&enclosureDrivePowerInfo, sizeof(fbe_base_object_mgmt_drv_dbg_ctrl_t));
//    enclosureDrivePowerInfo.driveCount = FBE_SAS_VIPER_ENCLOSURE_NUMBER_OF_SLOTS;
        enclosureDrivePowerInfo.driveCount = test->max_drives;
        enclosureDrivePowerInfo.drivePowerAction[driveNumber] = FBE_DRIVE_POWER_ACTION_POWER_CYCLE;
        enclosureDrivePowerInfo.drivePowerCycleDuration[driveNumber] = 
            cliffsOfMoherDrivePowerCycleTable[testPassCount].drivePowerCycleDuration;

        // send Control to enclosure
        status = fbe_api_enclosure_send_drive_power_control(object_handle_p, &enclosureDrivePowerInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        // Verify drive powered down
        statusChangeFound = FALSE;
        requestCount = 0;
        while (requestCount < MAX_REQUEST_STATUS_COUNT)
        {
            // Request current enclosure data
            requestCount++;
            fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_get_enclosure_info_t));
            status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)&enclosureControlInfoBuffer);
            // Check for State Change
            edalStatus = fbe_edal_getOverallStateChangeCount(enclosureControlInfo,
                                                         &newStateChangeCount);
            MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
            mut_printf(MUT_LOG_LOW, "**** Encl %d , requestCount %d, newStateChangeCount %d****\n",
                enclNumber, requestCount, newStateChangeCount);
            if (newStateChangeCount > originalStateChangeCount)
            {
                //State Change detected, so time to check if Drive is powered correctly
                edalStatus = fbe_edal_getBool(enclosureControlInfo,
                                              FBE_ENCL_COMP_POWERED_OFF,
                                              FBE_ENCL_DRIVE_SLOT,
                                              driveNumber,
                                              &drivePoweredOff);
                MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
                if (drivePoweredOff)
                {
                    statusChangeFound = TRUE;
                    break;
                }
                else
                {
                    originalStateChangeCount = newStateChangeCount;
                }
            }

            // Delay before requesting Enclosure status again
            fbe_api_sleep (3000);

        }   // end of while loop

        if (!statusChangeFound)
        {
            fbe_edal_printAllComponentData(enclosureControlInfo, fbe_edal_trace_func);
        }
        MUT_ASSERT_TRUE(statusChangeFound == TRUE);

        // Delay for specified Power Cycle time interval
        mut_printf(MUT_LOG_LOW, "**** Delay for 6 seconds ****\n");
        fbe_api_sleep (6000);

        // Verify drive powered up
        statusChangeFound = FALSE;
        requestCount = 0;
        while (requestCount < MAX_REQUEST_STATUS_COUNT)
        {
            // Request current enclosure data
            requestCount++;
            fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_get_enclosure_info_t));
            status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
           fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)&enclosureControlInfoBuffer);

            // Check for State Change
            edalStatus = fbe_edal_getOverallStateChangeCount(enclosureControlInfo,
                                                         &newStateChangeCount);
            MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
            mut_printf(MUT_LOG_LOW, "**** Encl %d , requestCount %d, newStateChangeCount %d****\n",
                enclNumber, requestCount, newStateChangeCount);
            if (newStateChangeCount > originalStateChangeCount)
            {
                //State Change detected, so time to check if Drive is powered correctly
                edalStatus = fbe_edal_getBool(enclosureControlInfo,
                                              FBE_ENCL_COMP_POWERED_OFF,
                                              FBE_ENCL_DRIVE_SLOT,
                                              driveNumber,
                                              &drivePoweredOff);
                MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
                if (!drivePoweredOff)
                {
                    statusChangeFound = TRUE;
                    break;
                }
                else
                {
                    originalStateChangeCount = newStateChangeCount;
                }
            }

            // Delay before requesting Enclosure status again
            fbe_api_sleep (3000);

        }   // end of while loop

        if (!statusChangeFound)
        {
            fbe_edal_printAllComponentData(enclosureControlInfo, fbe_edal_trace_func);
        }
        MUT_ASSERT_TRUE(statusChangeFound == TRUE);

    }


// jap - enable when Terminator support is available
#if FALSE
    /*
     * Enclosure Expander Reset Testing
     */
    mut_printf(MUT_LOG_LOW, "*****************************\n");
    mut_printf(MUT_LOG_LOW, "**** Test Expander Reset for %s ****\n", test->title);
    mut_printf(MUT_LOG_LOW, "*****************************\n\n");

    for (testPassCount = 0; testPassCount < DRIVE_POWER_TABLE_COUNT; testPassCount++)
    {
        /*
         * Bypass the appropriate Drive from table
         */
        enclNumber = cliffsOfMoherExpanderTable[testPassCount].enclNumber;
        expanderAction = cliffsOfMoherExpanderTable[testPassCount].action;
        mut_printf(MUT_LOG_LOW, "**** Pass %d, ENCL %d, ACTION %s ****\n",
            testPassCount, enclNumber, 
            (expanderAction == FBE_EXPANDER_ACTION_COLD_RESET ? "ColdReset" : "SilentMode"));

        // Get handle to enclosure object
        status = fbe_api_get_enclosure_object_id_by_location(0, enclNumber, &object_handle_p);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

        // Request current enclosure data
        fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_get_enclosure_info_t));
        status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)&enclosureControlInfoBuffer);

        // Save State change count
        edalStatus = fbe_edal_getOverallStateChangeCount(enclosureControlInfo,
                                                     &originalStateChangeCount);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "**** Encl %d , originalStateChangeCount %d****\n",
            enclNumber, originalStateChangeCount);

        // Send Drive Debug command
        fbe_zero_memory(&expanderInfo, sizeof(fbe_base_object_mgmt_exp_ctrl_t));
        expanderInfo.expanderAction = expanderAction;
//        expanderInfo.drivePowerAction[driveNumber] = powerAction;

        // send Control to enclosure
        status = fbe_api_enclosure_send_expander_control(object_handle_p, &expanderInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        // Verify the appropriate Drive power status change
        statusChangeFound = FALSE;
        requestCount = 0;
        while (requestCount < MAX_REQUEST_STATUS_COUNT)
        {
            // Request current enclosure data
            requestCount++;
            fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
            status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)&enclosureControlInfoBuffer);

            // Check for State Change
            edalStatus = fbe_edal_getOverallStateChangeCount(enclosureControlInfo,
                                                         &newStateChangeCount);
            MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
            mut_printf(MUT_LOG_LOW, "**** Encl %d , requestCount %d, newStateChangeCount %d****\n",
                enclNumber, requestCount, newStateChangeCount);
            if (newStateChangeCount > originalStateChangeCount)
            {
/*
                //State Change detected, so time to check if Drive is powered correctly
                edalStatus = fbe_edal_getBool(enclosureControlInfo,
                                              FBE_ENCL_COMP_POWERED_OFF,
                                              FBE_ENCL_DRIVE_SLOT,
                                              driveNumber,
                                              &drivePoweredOff);
                MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
                if (((powerAction == FBE_DRIVE_POWER_ACTION_POWER_DOWN) && (drivePoweredOff)) ||
                    ((powerAction == FBE_DRIVE_POWER_ACTION_POWER_UP) && (!drivePoweredOff)))
                {
                    statusChangeFound = TRUE;
                    break;
                }
                else
                {
                    originalStateChangeCount = newStateChangeCount;
                }
*/
            }

            // Delay before requesting Enclosure status again
            fbe_api_sleep (3000);

        }   // end of while loop


        if (!statusChangeFound)
        {
            fbe_edal_printAllComponentData(enclosureControlInfo, fbe_edal_trace_func);
        }
        MUT_ASSERT_TRUE(statusChangeFound == TRUE);

    }
#endif
    return FBE_STATUS_OK;

}   // end of cliffs_of_moher

/*!***************************************************************
 * cliffs_of_moher()
 ****************************************************************
 * @brief
 *  Function to run the cliffs_of_moher scenario.
 *
 * @author
 *  07/28/2010 - Created. Trupti Ghate (to support table driven approach)
 *
 ****************************************************************/

void cliffs_of_moher(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;

    for(test_num = 0; test_num < CLIFFS_OF_MOHER_TEST_MAX; test_num++)
    {
        /* Load the configuration for test
         */
        maui_load_and_verify_table_driven(&test_table[test_num]);
        run_status = cliffs_of_moher_run(&test_table[test_num]);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
         */
        fbe_test_physical_package_tests_config_unload();
    }
}

