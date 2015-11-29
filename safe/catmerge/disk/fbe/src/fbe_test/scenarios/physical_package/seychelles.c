/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * seychelles.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This test simulates LCC Power Cycle commands.
 *  This scenario is for a 1 port, 1 enclosures configuration, aka the maui.xml
 *  configuration.
 * 
 *  The test run in this case is:
 * 1. Load and verify the 1-Port, 1-Enclosure configuration. (use Maui configuration)
 * 2. Issue the GET_ENCLOSURE_INFO command to get EDAL blob from Enclosure object.
 * 3. Use API to send Expander an LCC Power Cycle command.
 * 4. Issue GET_ENCLOSURE_INFO commands to verify that the actions occurred (check
 *    Reset Ride Through count).
 *
 * HISTORY
 *   03/09/2009:  Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_enclosure_data_access_public.h"

/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @enum seychelles_tests_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * seychelles_test.
 *********************************************************************/

typedef enum seychelles_e
{
    SEYCHELLES_TEST_NAGA,
    SEYCHELLES_TEST_VIPER,
    SEYCHELLES_TEST_DERRINGER,
    SEYCHELLES_TEST_VOYAGER,
    SEYCHELLES_TEST_BUNKER,
    SEYCHELLES_TEST_CITADEL,
    SEYCHELLES_TEST_FALLBACK,
    SEYCHELLES_TEST_BOXWOOD,
    SEYCHELLES_TEST_KNOT,
    SEYCHELLES_TEST_ANCHO,
    SEYCHELLES_TEST_CALYPSO,
    SEYCHELLES_TEST_RHEA,
    SEYCHELLES_TEST_MIRANDA,
    SEYCHELLES_TEST_TABASCO,
} seychelles_tests_t;



/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/

static fbe_test_params_t test_table[] = 
{ 
    {SEYCHELLES_TEST_NAGA,
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
    {SEYCHELLES_TEST_VIPER,
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
    {SEYCHELLES_TEST_DERRINGER,
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
    {SEYCHELLES_TEST_VOYAGER,
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

    {SEYCHELLES_TEST_BUNKER,
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

    {SEYCHELLES_TEST_CITADEL,
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
    {SEYCHELLES_TEST_FALLBACK,
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
    {SEYCHELLES_TEST_BOXWOOD,
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
    {SEYCHELLES_TEST_KNOT,
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
    {SEYCHELLES_TEST_ANCHO,
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
    {SEYCHELLES_TEST_CALYPSO,
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
    {SEYCHELLES_TEST_RHEA,
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
    {SEYCHELLES_TEST_MIRANDA,
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
    {SEYCHELLES_TEST_TABASCO,
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

#define MAX_REQUEST_STATUS_COUNT    10
#define POWER_CYCLE_DURATION        0

void seychelles_run(fbe_test_params_t *test)
{
    fbe_u32_t                                       object_handle_p;
    fbe_status_t                                    status;
    fbe_edal_status_t                               edalStatus;
    fbe_u32_t                                       originalStateChangeCount;
    fbe_u32_t                                       newStateChangeCount;
    fbe_base_object_mgmt_set_enclosure_control_t    enclosureControlInfoBuffer;
    fbe_edal_block_handle_t                         enclosureControlInfo = &enclosureControlInfoBuffer;
    fbe_u8_t                                        requestCount = 0;
    fbe_u8_t                                        busyCount = 0;
    fbe_u8_t                                        testPassCount = 0;
    fbe_u8_t                                        enclNumber = 0;
    fbe_bool_t                                      statusChangeFound = FALSE;
    fbe_base_object_mgmt_exp_ctrl_t                 expanderInfo;
    fbe_u32_t                                       originalResetRiceThruCount = 0;
    fbe_u32_t                                       resetRiceThruCount = 0;

    /*
     * Expander Power Cycle Testing
     */
    mut_printf(MUT_LOG_LOW, "*************************************\n");
    mut_printf(MUT_LOG_LOW, "**** seychelles: Test Expander Power Cycling  for %s ****\n",test->title);
    mut_printf(MUT_LOG_LOW, "*************************************\n\n");
    

//    for (testPassCount = 0; testPassCount < DRIVE_POWER_TABLE_COUNT; testPassCount++)
    {
        /*
         * Bypass the appropriate Drive from table
         */
        mut_printf(MUT_LOG_LOW, "**** Pass %d, ENCL %d for %s****\n",
            testPassCount, test->encl_number, test->title);

        // Get handle to enclosure object
        status = fbe_api_get_enclosure_object_id_by_location(test->backend_number, test->encl_number, &object_handle_p);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

        // Request current enclosure data
        fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_get_enclosure_info_t));
        status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        // Save State change count & Reset Ride Thru count
        edalStatus = fbe_edal_getOverallStateChangeCount(enclosureControlInfo,
                                                     &originalStateChangeCount);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "**** Encl %d , originalStateChangeCount %d, for %s ****\n",
            enclNumber, originalStateChangeCount, test->title);
        edalStatus = fbe_edal_getU32(enclosureControlInfo,
                                     FBE_ENCL_RESET_RIDE_THRU_COUNT,
                                     FBE_ENCL_ENCLOSURE,
                                     0,
                                     &originalResetRiceThruCount);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);

        // Send Cold Reset command which will power cycle Expander
        fbe_zero_memory(&expanderInfo, sizeof(fbe_base_object_mgmt_exp_ctrl_t));
        expanderInfo.expanderAction = FBE_EXPANDER_ACTION_COLD_RESET;
        expanderInfo.powerCycleDuration = POWER_CYCLE_DURATION;
        expanderInfo.powerCycleDelay = 0;

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
            busyCount = 0;
            do
            {
                status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
                MUT_ASSERT_TRUE((status == FBE_STATUS_OK) || (status == FBE_STATUS_BUSY));
                if (status == FBE_STATUS_OK)
                {
                    break;
                }
                else if (status == FBE_STATUS_BUSY)
                {
                    busyCount++;
                    fbe_api_sleep (1000);
                }
            }
            while (busyCount < 10);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            // Check for State Change
            edalStatus = fbe_edal_getOverallStateChangeCount(enclosureControlInfo,
                                                         &newStateChangeCount);
            MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
            mut_printf(MUT_LOG_LOW, "**** Encl %d , requestCount %d, newStateChangeCount %d for %s ****\n",
                enclNumber, requestCount, newStateChangeCount, test->title);
            if (newStateChangeCount > originalStateChangeCount)
            {

                //State Change detected, so time to check if Drive is powered correctly
                edalStatus = fbe_edal_getU32(enclosureControlInfo,
                                             FBE_ENCL_RESET_RIDE_THRU_COUNT,
                                             FBE_ENCL_ENCLOSURE,
                                             0,
                                             &resetRiceThruCount);
                MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
                if ((originalResetRiceThruCount + 1) == resetRiceThruCount)
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

    }   // end of for loop

}   // end of seychelles_run

/*!***************************************************************
 * seychelles()
 ****************************************************************
 * @brief
 *  Function to run the seychelles scenario.
 *
 * @author
 *  08/10/2011 - Added to support table driven approach. 
 *
 ****************************************************************/
void seychelles(void)
{
    fbe_u32_t test_num;
    for(test_num = 0; test_num < STATIC_TEST_MAX; test_num++)
    {
        /* Load the configuration for test
         */
        maui_load_and_verify_table_driven(&test_table[test_num]);
        seychelles_run(&test_table[test_num]);
       
       /* Unload the configuration
        */
        fbe_test_physical_package_tests_config_unload();
    }
}
