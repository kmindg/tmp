/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * Kamphaengphet.c
 ***************************************************************************
 *
 * DESCRIPTION
 * This test simulates a PHY being disabled, which is reported as Drive status 
 *  unavailable.
 *  This scenario is for a 1 port, 1 enclosures configuration, aka the maui.xml
 *  configuration.
 * 
 *  The test run in this case is:
 * 1. Load and verify the 1-Port, 1-Enclosure configuration. (use Maui configuration)
 * 2. Issue the GET_ENCLOSURE_INFO command to get EDAL blob from Enclosure object.
 * 3. Use EDAL set functions to request Disable of PHY for Drive X.
 * 4. Send SET_ENCLOSURE_CONTROL command to Enclosure object.
 * 5. Issue GET_ENCLOSURE_INFO commands to verify that Drive X has been bypassed.
 * 6. Repeat this sequence to Enable the PHY.
 *
 * HISTORY
 *   11/25/2008:  Created. Joe Perry
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
 * @enum kamphaengphet_tests_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * kamphaengphet.
 *********************************************************************/

typedef enum kamphaengpheti_e
{
    KAMPHAENGPHETI_TEST_VIPER,
    KAMPHAENGPHETI_TEST_DERRINGER,
    KAMPHAENGPHETI_TEST_VOYAGER,
    KAMPHAENGPHETI_TEST_VIKING,
    KAMPHAENGPHETI_TEST_CAYENNE,
    KAMPHAENGPHETI_TEST_NAGA,
    KAMPHAENGPHETI_TEST_BUNKER,
    KAMPHAENGPHETI_TEST_CITADEL,
    KAMPHAENGPHETI_TEST_FALLBACK,
    KAMPHAENGPHETI_TEST_BOXWOOD,
    KAMPHAENGPHETI_TEST_KNOT,
    KAMPHAENGPHETI_TEST_ANCHO,
    KAMPHAENGPHETI_TEST_CALYPSO,
    KAMPHAENGPHETI_TEST_RHEA,
    KAMPHAENGPHETI_TEST_MIRANDA,
    KAMPHAENGPHETI_TEST_TABASCO,
    KAMPHAENGPHETI_TEST_ENUM_LIMIT
} kamphaengphet_tests_t;

/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/

static fbe_test_params_t test_table[] = 
{   
    {KAMPHAENGPHETI_TEST_VIPER,
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
    {KAMPHAENGPHETI_TEST_DERRINGER,
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
    {KAMPHAENGPHETI_TEST_VOYAGER,
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
    {KAMPHAENGPHETI_TEST_VIKING,
     "VIKING",
     FBE_BOARD_TYPE_FLEET, 
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
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
    {KAMPHAENGPHETI_TEST_CAYENNE,
     "Cayenne",
     FBE_BOARD_TYPE_FLEET, 
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
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
    {KAMPHAENGPHETI_TEST_NAGA,
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
     MAX_DRIVE_COUNT_VIKING,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {KAMPHAENGPHETI_TEST_BUNKER,
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
    {KAMPHAENGPHETI_TEST_CITADEL,
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
    {KAMPHAENGPHETI_TEST_FALLBACK,
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
    {KAMPHAENGPHETI_TEST_BOXWOOD,
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
    {KAMPHAENGPHETI_TEST_KNOT,
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
    {KAMPHAENGPHETI_TEST_ANCHO,
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
    {KAMPHAENGPHETI_TEST_CALYPSO,
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
    {KAMPHAENGPHETI_TEST_RHEA,
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
    {KAMPHAENGPHETI_TEST_MIRANDA,
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
    {KAMPHAENGPHETI_TEST_TABASCO,
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


/*
 * Structure/table of info on what Drives to test
 */
typedef struct
{
    fbe_u8_t        enclNumber;
    fbe_u8_t        driveNumber;
} drive_bypass_info_t;

#define DRIVE_BYPASS_TABLE_COUNT    3
drive_bypass_info_t kamphaengphetDriveBypassTable[DRIVE_BYPASS_TABLE_COUNT] =
{   
    {   0,      1   },
    {   0,      5   },
    {   0,      11  }  // Changed from drive 14 to drive 11. Because the disable phy (write flag) will be cleared if the drive 
                       // is not inserted (i. e. we can not bypass an empty slot). 
                       // For maui.xml config file, there is only 12 drives inserted on enclosure 0.
};

#define MAX_REQUEST_STATUS_COUNT    10

fbe_status_t kamphaengphet_get_enclosure_object_id(fbe_u8_t bus,   // Input
                                                   fbe_u8_t encl,   // Input
                                                   fbe_u8_t *pDrive, //Input and Output
                                                   fbe_object_id_t * pObjectId)  // Output
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_edal_status_t                               edalStatus = FBE_EDAL_STATUS_OK;
    fbe_u32_t                                       driveSlotCount = 0;
    fbe_u32_t                                       totalDriveSlotCount = 0;
    fbe_topology_control_get_enclosure_by_location_t enclosure_by_location = {0};
    fbe_base_object_mgmt_set_enclosure_control_t    enclosureControlInfoBuffer;
    fbe_edal_block_handle_t                         enclosureControlInfo = &enclosureControlInfoBuffer;
    fbe_u32_t                                       component = 0;

    *pObjectId = FBE_OBJECT_ID_INVALID;
    status = fbe_api_get_enclosure_object_ids_array_by_location(bus, encl, &enclosure_by_location);

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
    status = fbe_api_enclosure_get_info(enclosure_by_location.enclosure_object_id, 
                                        (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);

    fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)&enclosureControlInfoBuffer);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    edalStatus = fbe_edal_getSpecificComponentCount(enclosureControlInfo, FBE_ENCL_DRIVE_SLOT, &driveSlotCount);

    MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);

    totalDriveSlotCount += driveSlotCount;

    if(*pDrive < totalDriveSlotCount) 
    {
        *pObjectId = enclosure_by_location.enclosure_object_id;
        return FBE_STATUS_OK;
    }
    else
    {
        for(component = 0; component < FBE_API_MAX_ENCL_COMPONENTS; component ++)
        {
            if(enclosure_by_location.comp_object_id[component] != FBE_OBJECT_ID_INVALID)
            {
                fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));

                status = fbe_api_enclosure_get_info(enclosure_by_location.comp_object_id[component], 
                                                   (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);

                fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)&enclosureControlInfoBuffer);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

                edalStatus = fbe_edal_getSpecificComponentCount(enclosureControlInfo, FBE_ENCL_DRIVE_SLOT, &driveSlotCount);

                MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);

                if(*pDrive < totalDriveSlotCount + driveSlotCount) 
                {
                    // Find the correct component
                    *pObjectId = enclosure_by_location.comp_object_id[component];
                    *pDrive = *pDrive - totalDriveSlotCount;
                    return FBE_STATUS_OK;
                }

                totalDriveSlotCount += driveSlotCount;
            }
        }
    }
   
    return FBE_STATUS_GENERIC_FAILURE; 
}


fbe_status_t kamphaengphet_run(fbe_test_params_t *test)
{
    fbe_u32_t                                       object_handle_p;
    fbe_status_t                                    status;
    fbe_edal_status_t                               edalStatus;
    fbe_u32_t                                       originalStateChangeCount;
    fbe_u32_t                                       newStateChangeCount;
    fbe_bool_t                                      driveBypassed;
    fbe_base_object_mgmt_set_enclosure_control_t    enclosureControlInfoBuffer;
    fbe_edal_block_handle_t                         enclosureControlInfo = &enclosureControlInfoBuffer;
    fbe_base_object_mgmt_get_enclosure_basic_info_t basicEnclInfo;
    fbe_u8_t                                        requestCount = 0;
    fbe_u8_t                                        testPassCount;
    fbe_u8_t                                        driveNumber, enclNumber;
    fbe_bool_t                                      statusChangeFound = FALSE;

    mut_printf(MUT_LOG_LOW, "**********************************\n");
    mut_printf(MUT_LOG_LOW, "**** Test Bypassing of Drives ****\n");
    mut_printf(MUT_LOG_LOW, "**********************************\n\n");
    mut_printf(MUT_LOG_LOW, "****Kamphaengphet run test for %s ****\n",test->title);

    for (testPassCount = 0; testPassCount < DRIVE_BYPASS_TABLE_COUNT; testPassCount++)
    {
        /*
         * Bypass the appropriate Drive from table
         */
        enclNumber = kamphaengphetDriveBypassTable[testPassCount].enclNumber;
        driveNumber = kamphaengphetDriveBypassTable[testPassCount].driveNumber;
        mut_printf(MUT_LOG_LOW, "**** Pass %d, BYPASS ENCL %d, DRIVE %d ****\n",
            testPassCount, enclNumber, driveNumber);
        // Get handle to enclosure object
        status = kamphaengphet_get_enclosure_object_id(test->backend_number, enclNumber, &driveNumber, &object_handle_p);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

        // Get Basic (LCC_POLL) info & save State change count
        status = fbe_api_enclosure_get_basic_info(object_handle_p, 
                                                  (fbe_base_object_mgmt_get_enclosure_basic_info_t *)&basicEnclInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        originalStateChangeCount = basicEnclInfo.enclChangeCount;
        mut_printf(MUT_LOG_LOW, "**** Encl %d , originalStateChangeCount %d****\n",
            enclNumber, originalStateChangeCount);

        // Request current enclosure data
        fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
        status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
        fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)&enclosureControlInfoBuffer);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        // Set PHY Disable attribute
        edalStatus = fbe_edal_setDrivePhyBool(enclosureControlInfo,
                                              FBE_ENCL_EXP_PHY_DISABLE,
                                              driveNumber,
                                              TRUE);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);

        // Send Control Request to Enclosure 0
        status = fbe_api_enclosure_setControl(object_handle_p, &enclosureControlInfoBuffer);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        // Verify that Drive gets Bypassed
        statusChangeFound = FALSE;
        while (requestCount < MAX_REQUEST_STATUS_COUNT)
        {
            // Request current enclosure data
            requestCount++;
            status = fbe_api_enclosure_get_basic_info(object_handle_p, 
                                                     (fbe_base_object_mgmt_get_enclosure_basic_info_t *)&basicEnclInfo);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            newStateChangeCount = basicEnclInfo.enclChangeCount;

            mut_printf(MUT_LOG_LOW, "**** Encl %d , requestCount %d, newStateChangeCount %d****\n",
                enclNumber, requestCount, newStateChangeCount);
            if (newStateChangeCount > originalStateChangeCount)
            {
                fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
                status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

                //State Change detected, so time to check if Drive is bypassed
                edalStatus = fbe_edal_getBool(enclosureControlInfo,
                                              FBE_ENCL_DRIVE_BYPASSED,
                                              FBE_ENCL_DRIVE_SLOT,
                                              driveNumber,
                                              &driveBypassed);
                MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
                if (driveBypassed)
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
        MUT_ASSERT_TRUE(statusChangeFound == TRUE);

        /*
         * Unbypass Drive in Enclosure
         */
        mut_printf(MUT_LOG_LOW, "**** Pass %d, UNBYPASS ENCL %d, DRIVE %d ****\n",
            testPassCount, enclNumber, driveNumber);

        // Get Basic (LCC_POLL) info & save State change count
        status = fbe_api_enclosure_get_basic_info(object_handle_p, 
                                                  (fbe_base_object_mgmt_get_enclosure_basic_info_t *)&basicEnclInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        originalStateChangeCount = basicEnclInfo.enclChangeCount;
        mut_printf(MUT_LOG_LOW, "**** Encl %d , originalStateChangeCount %d****\n",
            enclNumber, originalStateChangeCount);

        // Request current enclosure data
        fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
        status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        // Set PHY Disable attribute
        edalStatus = fbe_edal_setDrivePhyBool(enclosureControlInfo,
                                              FBE_ENCL_EXP_PHY_DISABLE,
                                              driveNumber,
                                              FALSE);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);

        // Send Control Request to Enclosure
        status = fbe_api_enclosure_setControl(object_handle_p, &enclosureControlInfoBuffer);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        // Verify that Drive gets Bypassed
        requestCount = 0;
        statusChangeFound = FALSE;
        while (requestCount < MAX_REQUEST_STATUS_COUNT)
        {
            // Request current enclosure data
            requestCount++;
            status = fbe_api_enclosure_get_basic_info(object_handle_p, 
                                                      (fbe_base_object_mgmt_get_enclosure_basic_info_t *)&basicEnclInfo);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            newStateChangeCount = basicEnclInfo.enclChangeCount;

            mut_printf(MUT_LOG_LOW, "**** Encl %d , requestCount %d, newStateChangeCount %d****\n",
                enclNumber, requestCount, newStateChangeCount);
            if (newStateChangeCount > originalStateChangeCount)
            {
                fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
                status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

                //State Change detected, so time to check if Drive is bypassed
                edalStatus = fbe_edal_getBool(enclosureControlInfo,
                                              FBE_ENCL_DRIVE_BYPASSED,
                                              FBE_ENCL_DRIVE_SLOT,
                                              driveNumber,
                                              &driveBypassed);
                MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
                if (!driveBypassed)
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
        MUT_ASSERT_TRUE(statusChangeFound == TRUE);

    }   // end of for loop
     return status;
}   // end of kamphaengphet

/*!***************************************************************
 * @fn kamphaengphet()
 ****************************************************************
 * @brief:
 *  Function to load the config and run the 
 *  kamphaengphet scenario.
 *
 * @return 
 *  FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 * 07/28/2011 - Created. Trupti Ghate
 ****************************************************************/
void kamphaengphet(void)
{
    fbe_status_t run_status;
#if 1
    fbe_u32_t test_num;
    for(test_num = 0; test_num < KAMPHAENGPHETI_TEST_ENUM_LIMIT; test_num++)
    {
        /* Load the configuration for test
        */        
        maui_load_and_verify_table_driven(&test_table[test_num]);
        run_status = kamphaengphet_run(&test_table[test_num]);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);
        /* Unload the configuration
        */
        fbe_test_physical_package_tests_config_unload();        
    }
#else
    maui_load_and_verify_table_driven(&test_table[KAMPHAENGPHETI_TEST_VIKING]);
    run_status = kamphaengphet_run(&test_table[KAMPHAENGPHETI_TEST_VIKING]);
    MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);
    // Unload the configuration
    fbe_test_physical_package_tests_config_unload();        
#endif

}


