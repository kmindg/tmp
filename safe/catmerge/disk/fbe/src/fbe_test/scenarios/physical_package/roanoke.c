/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * roanoke.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the test functions for the "Roanoke" scenario.
 * 
 *  This scenario creates a 1 board, 1 port, 1 enclosure configuration with 12 drivers.

 *  The scenario:
 *  - Send a RESET command to a physical drive object.
 *  - Check the drive object changes to ACTIVATE state immediately.
 *  - Check the drive object goes to ready state. 
 *
 * HISTORY
 *  01/14/2009  Created.  chenl6. 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe_test_io_api.h"
#include "fbe_scsi.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"

/* We have one enclosure with 12 520 drives.
 */

#define ROANOKE_520_DRIVE_NUMBER_FIRST 0
#define ROANOKE_520_DRIVE_NUMBER_LAST 11


/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @enum roanoke_test_number_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * roanoke_test.
 *********************************************************************/
typedef enum roanoke_test_number_e
{
    ROANOKE_TEST_NAGA,
    ROANOKE_TEST_VIPER,
    ROANOKE_TEST_DERRINGER,
    ROANOKE_TEST_BUNKER,
    ROANOKE_TEST_CITADEL,
    ROANOKE_TEST_FALLBACK,
    ROANOKE_TEST_BOXWOOD,
    ROANOKE_TEST_KNOT,
    ROANOKE_TEST_ANCHO,
    ROANOKE_TEST_CALYPSO,
    ROANOKE_TEST_RHEA,
    ROANOKE_TEST_MIRANDA,
    ROANOKE_TEST_TABASCO,
} roanoke_test_number_t;

/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/
static fbe_test_params_t test_table[] = 
{  
    {ROANOKE_TEST_NAGA,
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
    {ROANOKE_TEST_VIPER,
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
    {ROANOKE_TEST_DERRINGER,
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
    {ROANOKE_TEST_BUNKER,
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
    {ROANOKE_TEST_CITADEL,
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
    {ROANOKE_TEST_FALLBACK,
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
    {ROANOKE_TEST_BOXWOOD,
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
    {ROANOKE_TEST_KNOT,
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
    {ROANOKE_TEST_ANCHO,
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
//Moons
    {ROANOKE_TEST_CALYPSO,
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
    {ROANOKE_TEST_RHEA,
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
    {ROANOKE_TEST_MIRANDA,
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
     {ROANOKE_TEST_TABASCO,
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
 * @def STATIC_TEST_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define ROANOKE_TEST_MAX     sizeof(test_table)/sizeof(test_table[0])



 /*************************
  *   FUNCTION DEFINITIONS
  *************************/


/*!**************************************************************
 * @fn roanoke_run()
 ****************************************************************
 * @brief
 *  This function executes the roanoke scenario.
 * 
 *  This scenario will send a RESET command to a physical drive.
 *  
 * @param None.               
 *
 * @return None.
 *
 * HISTORY:
 *  01/14/2009  Created.  chenl6. 
 ****************************************************************/

static fbe_status_t roanoke_run(fbe_test_params_t *test)
{
    fbe_u32_t object_handle;
    fbe_status_t status;
    fbe_api_dieh_stats_t old_dieh_stats, new_dieh_stats;

    status = fbe_api_get_physical_drive_object_id_by_location(
                                                                test->backend_number,
                                                                test->encl_number,
                                                                test->drive_number,
                                                                &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_handle, FBE_OBJECT_ID_INVALID);

    /* Get error counts before reset device. 
     */
    status = fbe_api_physical_drive_get_dieh_info(object_handle, &old_dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
    mut_printf(MUT_LOG_LOW, "%s: get reset counts: %d for %s ", 
                   __FUNCTION__, old_dieh_stats.error_counts.physical_drive_error_counts.reset_count, test->title);

    /* Send RESET to the drive. 
     */
    status = fbe_api_physical_drive_reset(object_handle, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check the drive is in ACTIVATE state. 
     */
    status = fbe_api_wait_for_object_lifecycle_state(object_handle, FBE_LIFECYCLE_STATE_ACTIVATE, 5000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Wait the drive to READY state. 
     */
    status = fbe_api_wait_for_object_lifecycle_state(object_handle, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
   
    /* Get DIEH information after reset device. 
     */
    status = fbe_api_physical_drive_get_dieh_info(object_handle, &new_dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
    mut_printf(MUT_LOG_LOW, "%s: get reset counts: %d for %s", 
               __FUNCTION__, new_dieh_stats.error_counts.physical_drive_error_counts.reset_count,test->title); 
    MUT_ASSERT_TRUE((old_dieh_stats.error_counts.physical_drive_error_counts.reset_count + 1) == new_dieh_stats.error_counts.physical_drive_error_counts.reset_count);

    return  status;
}


void roanoke(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;

    for(test_num = 0; test_num < ROANOKE_TEST_MAX; test_num++)
    {
        /* Load the configuration for test
         */
        maui_load_and_verify_table_driven(&test_table[test_num]);
        run_status = roanoke_run(&test_table[test_num]);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
         */
        fbe_test_physical_package_tests_config_unload();
    }
}

/*************************
 * end file roanoke.c
 *************************/
 
 
