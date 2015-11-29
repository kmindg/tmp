/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * Trivandrum.c
 ***************************************************************************
 *
 * DESCRIPTION
 * This test simulates a drive failed, which is reported as PHY status critical or unrecoverable.
 *  This scenario is for a 1 port, 1 enclosures configuration, aka the maui.xml
 *  configuration.
 * 
 *  The test run in this case is:
 * 1. Load and verify the 1-Port, 1-Enclosure configuration. (use Maui configuration)
 * 2. Use Terminator API to set the slot-0 phy status to bypass.
 * 3. Use Terminator force logout to send a LOGOUT for the slot-0 drive, 
 *    and Terminator set enclosure drive status to updates the ESES 
 *    status page indicating that the slot-0 drive is bypassed (same as clear insert-bit).
 * 4. The enclosure polls for the ESES status page, Terminator returns the updated page.
 * 5. Retrieves the ESES status page from enclosure object, via the FBE API
 *    and verifies the status page. 
 * 6. Verifies that the physical drive in slot-0 is in the ACTIVATE state.
 * 7. Use Terminator API to set the slot-o phy status to unbypass.
 * 8. Terminator force login to send a LOGIN for the slot-0 drive 
 *    and Terminator set enclosure drive status to updates the ESES 
 *    status page indicating that the slot-0 drive is not bypassed (same as set insert-bit).
 * 9. The enclosure polls for the ESES status page and Terminator returns the updated page.
 * 10.Retrieves the ESES status page (via the FBE API) and verifies the 
 *    status page. 
 * 11.Verifies that the physical drive in slot-0 is in the READY state.
 *
 * HISTORY
 *   08/17/2008:  Created. Ashok Rana
 *   10/23/2008:  Updated by Vicky Guo.  Use new Terminator API and FBE API.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_test_configurations.h"

/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @enum trivandrum_test_number_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * trivandrum_test.
 *********************************************************************/
typedef enum trivandrum_test_number_e
{
    TRIVANDRUM_TEST_VIPER,
    TRIVANDRUM_TEST_DERRINGER,
    TRIVANDRUM_TEST_BUNKER,
    TRIVANDRUM_TEST_CITADEL,
    TRIVANDRUM_TEST_FALLBACK,
    TRIVANDRUM_TEST_BOXWOOD,
    TRIVANDRUM_TEST_KNOT,
    TRIVANDRUM_TEST_ANCHO,
    TRIVANDRUM_TEST_CALYPSO,
    TRIVANDRUM_TEST_RHEA,
    TRIVANDRUM_TEST_MIRANDA,
    TRIVANDRUM_TEST_TABASCO,
} trivandrum_test_number_t;

/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/

static fbe_test_params_t test_table[] = 
{   
    {TRIVANDRUM_TEST_VIPER,
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
     {TRIVANDRUM_TEST_DERRINGER,
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
      {TRIVANDRUM_TEST_BUNKER,
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
      {TRIVANDRUM_TEST_CITADEL,
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
    {TRIVANDRUM_TEST_FALLBACK,
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
    {TRIVANDRUM_TEST_BOXWOOD,
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
    {TRIVANDRUM_TEST_KNOT,
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
    {TRIVANDRUM_TEST_ANCHO,
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
    {TRIVANDRUM_TEST_CALYPSO,
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
    {TRIVANDRUM_TEST_RHEA,
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
    {TRIVANDRUM_TEST_MIRANDA,
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
    {TRIVANDRUM_TEST_TABASCO,
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
 * @def TRIVANDRUM_TEST_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define TRIVANDRUM_TEST_MAX     sizeof(test_table)/sizeof(test_table[0])

/*!***************************************************************
 * trivandrum_run()
 ****************************************************************
 * @brief
 *  This function contains the actual test code for the trivandrum
 *  scenario.
 *  This test simulates a drive failed, which is reported as 
 *  PHY status critical or unrecoverable.
 *
 * @return  FBE_STATUS_OK if no errors.
 *
 * @author
 *  11/27/2011 - Updated. Trupti Ghate(to support table driven approach)
 *
 ****************************************************************/
fbe_status_t trivandrum_run(fbe_test_params_t *test)
{
    fbe_status_t status;
    fbe_u32_t port_number, encl_number, slot_number;
    fbe_api_terminator_device_handle_t encl_device_id;
    fbe_api_terminator_device_handle_t device_id;
    fbe_object_id_t encl_object_id, drive_object_id;

    port_number = test->backend_number;
    encl_number = test->encl_number;
    slot_number = test->drive_number;

    status = fbe_api_terminator_get_enclosure_handle(port_number,
                                                    encl_number, 
                                                    &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_terminator_get_drive_handle(port_number,      
                                                    encl_number,    
                                                    slot_number,
                                                    &device_id); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*set the slot-0 phy status to bypass.*/
    status = fbe_api_terminator_enclosure_bypass_drive_slot(encl_device_id, slot_number);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "****Encl-%d slot-%d phy is bypassed in Terminator for %s****",
                    encl_number, slot_number, test->title);
    
    /*send a LOGOUT for the slot-0 drive*/
    status = fbe_api_terminator_force_logout_device(device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "****Encl-%d Drive %d is logged out from Terminator for %s****",
        encl_number, slot_number, test->title);
	
    /* Wait some reasonable time for states to stabilize.*/
    EmcutilSleep(1000);

    /*get the object id of the enclosure */
    status = fbe_api_get_enclosure_object_id_by_location (port_number, encl_number, &encl_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(encl_object_id != FBE_OBJECT_ID_INVALID);
    mut_printf(MUT_LOG_LOW, "****Encl-0's object Id is %d****", encl_object_id);

    /*verify the drive slot in ESES status page from enclosure object is bypassed*/
    status = fbe_api_wait_for_fault_attr(encl_object_id, FBE_ENCL_DRIVE_BYPASSED, TRUE, FBE_ENCL_DRIVE_SLOT, 0, 5000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "****Successfully Verified: Encl-%d reported drive bypassed****",
        encl_number);

    /*verifies that the physical drive in slot-0 is in the ACTIVATE state*/
    status = fbe_api_get_physical_drive_object_id_by_location (port_number, encl_number, slot_number, &drive_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(drive_object_id != FBE_OBJECT_ID_INVALID);
    
    status = fbe_api_wait_for_object_lifecycle_state(drive_object_id, FBE_LIFECYCLE_STATE_ACTIVATE, 5000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "****Successfully Verified: physical drive in slot-%d is in ACTIVATE state for %s****",
        slot_number, test->title);
	/* Wait some reasonable time for states to stabilize.*/
    EmcutilSleep(1000);
    
    /*set the slot-0 phy status to unbypass.*/
    status = fbe_api_terminator_enclosure_unbypass_drive_slot(encl_device_id, slot_number);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "****Encl-%d slot-%d phy is unbypassed in Terminator for %s ****",
        encl_number, slot_number, test->title);
    /*send a LOGIN for the slot-0 drive */
    status = fbe_api_terminator_force_login_device(device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "****Encl-%d Drive %d is logged in from Terminator fr %s****",
        encl_number, slot_number, test->title);
    /*get the object id of the enclosure */
    status = fbe_api_get_enclosure_object_id_by_location (port_number, encl_number, &encl_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(encl_object_id != FBE_OBJECT_ID_INVALID);

    /*verify the drive slot in ESES status page from enclosure object is unbypassed*/
    status = fbe_api_wait_for_fault_attr(encl_object_id, FBE_ENCL_DRIVE_LOGGED_IN, TRUE, FBE_ENCL_DRIVE_SLOT, 0, 5000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "****Successfully Verified: Encl-%d reported drive logged in for %s****",
        encl_number, test->title);

    /*verifies that the physical drive in slot-0 is in the READY state*/
    status = fbe_api_get_physical_drive_object_id_by_location (port_number, encl_number, slot_number, &drive_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(drive_object_id != FBE_OBJECT_ID_INVALID);
    
    status = fbe_api_wait_for_object_lifecycle_state(drive_object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "****Successfully Verified: physical drive in slot-%d is in READY state for %s****",
        slot_number, test->title);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * trivandrum()
 ****************************************************************
 * @brief
 *  Function to run the trivandrum scenario.
 *
 * @author
 *  07/27/2011 - Created. Trupti Ghate(to support table driven approach)
 *
 ****************************************************************/

void trivandrum(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;

    for(test_num = 0; test_num < TRIVANDRUM_TEST_MAX; test_num++)
    {
        /* Load the configuration for test
         */
        maui_load_and_verify_table_driven(&test_table[test_num]);
        run_status = trivandrum_run(&test_table[test_num]);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
         */
        fbe_test_physical_package_tests_config_unload();
    }
}

