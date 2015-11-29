/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * amalfi_coast.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Amalfi Coast scenario.
 * 
 *  This scenario is for a 1 port, 1 enclosure configuration, aka the maui
 *  configuration.
 * 
 *  The test run in this case is:
 *      - Increment the config page generation code using TAPI. 
 *          By incrementing the generation code we are telling that 
 *          something has changed in the configuration page. 
 *      - Verify if it is getting reflected in the ESES configuration page. 
 *          The generation code is compared with the generation code of 
 *          other pages (status/control). If it is not equal, we expect 
 *          a CHECK CONDITION with ILLEGAL REQUEST. If equal, we receive 
 *          a UNIT ATTENTION. In this case, we have to again request 
 *          for the configuration page. 
 *
 * HISTORY
 *   12/09/2008:  Created. ArunKumar Selvapillai
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "physical_package_tests.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_api_common.h"

/*************************
 *   MACRO DEFINITIONS
 *************************/


/*!*******************************************************************
 * @enum amalfi_tests_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * amalfi_tests.
 *********************************************************************/

typedef enum amalfi_e
{
    AMALFI_COAST_TEST_NAGA,
    AMALFI_COAST_TEST_VIPER,
    AMALFI_COAST_TEST_DERRINGER,
    AMALFI_COAST_TEST_VOYAGER,
    AMALFI_COAST_TEST_BUNKER,
    AMALFI_COAST_TEST_CITADEL,
    AMALFI_COAST_TEST_FALLBACK,
    AMALFI_COAST_TEST_BOXWOOD,
    AMALFI_COAST_TEST_KNOT,
    AMALFI_COAST_TEST_ANCHO,
    AMALFI_COAST_TEST_TABASCO,
} amalfi_tests_t;

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
    {AMALFI_COAST_TEST_NAGA,
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
    {AMALFI_COAST_TEST_VIPER,
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
    {AMALFI_COAST_TEST_DERRINGER,
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
    {AMALFI_COAST_TEST_VOYAGER,
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
    {AMALFI_COAST_TEST_BUNKER,
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
    {AMALFI_COAST_TEST_CITADEL,
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
    {AMALFI_COAST_TEST_FALLBACK,
     "FALLBACK",
     FBE_BOARD_TYPE_ARMADA,
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
    {AMALFI_COAST_TEST_BOXWOOD,
     "BOXWOOD",
     FBE_BOARD_TYPE_ARMADA,
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
    {AMALFI_COAST_TEST_KNOT,
     "KNOT",
     FBE_BOARD_TYPE_ARMADA,
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
    {AMALFI_COAST_TEST_ANCHO,
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
    {AMALFI_COAST_TEST_TABASCO,
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
};


/*!*******************************************************************
 * @def STATIC_TEST_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define STATIC_TEST_MAX     sizeof(test_table)/sizeof(test_table[0])

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!***************************************************************
 * @fn  amalfi_coast_run
 ****************************************************************
 * @brief:
 *  Function to 
 *      - Increment the config page generation code using TAPI. 
 *      - Verify if it is getting reflected in the ESES configuration page. 
 *
 * @return 
 *  FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *   12/09/2008:  Created. ArunKumar Selvapillai
 *   10/01/2009:  Peter Puhov. Workaround the enclosure enclosureConfigBeingUpdated problem.
 ****************************************************************/

fbe_status_t amalfi_coast_run(fbe_test_params_t *test)
{
    fbe_u32_t                   gen_code = 0;
    fbe_u32_t                   object_handle;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_terminator_device_handle_t    encl_id;
	fbe_u32_t get_attr_retry_count; /* Enclosure may deside to return BUSY status 
									Look at eses_enclosure_getEnclosureInfo function
									- it is very bad, but ... */

    /* Get the device id of the enclosure */
	for(get_attr_retry_count = 0; get_attr_retry_count < 3; get_attr_retry_count ++){
		status = fbe_api_terminator_get_enclosure_handle(test->backend_number, test->encl_number, &encl_id);
		if(status == FBE_STATUS_BUSY){
			fbe_api_sleep(10); /* This should solve the problem.
						   In fact enclosure should use object lock instead of 
						   absolutelly NOT MT SAFE enclosureConfigBeingUpdated flag.
						   */
		}else {
			break;
		}
	}

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Get the object id of the enclosure */
    status = fbe_api_get_enclosure_object_id_by_location (test->backend_number, test->encl_number, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    /* Retrieve the generation code from the enclosure blob */
    status = fbe_api_get_encl_attr(object_handle, FBE_ENCL_GENERATION_CODE, 
										FBE_ENCL_ENCLOSURE, 0, &gen_code);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Increment the Configuration page generation code for the enclosure */
    status = fbe_api_terminator_eses_increment_config_page_gen_code(encl_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Increment the generation code to compare it with the new value */
    gen_code++;

    /* Check the gen_code in EDAL got incremented */
    status = fbe_api_wait_for_encl_attr(object_handle, FBE_ENCL_GENERATION_CODE, 
                                            gen_code, FBE_ENCL_ENCLOSURE, 0, 7000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "GENERATION CODE is updated in EDAL");

    return status;
}

/*!***************************************************************
 * @fn amalfi_coast()
 ****************************************************************
 * @brief:
 *  Function to load the config and run the 
 *  Mount Vesuvius scenario.
 *
 * @return 
 *  FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *   12/09/2008:  Created. ArunKumar Selvapillai
 *
 ****************************************************************/

void amalfi_coast(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;

    for(test_num = 0; test_num < STATIC_TEST_MAX; test_num++)
    {
        /* Load the configuration for test
         */
        maui_load_and_verify_table_driven(&test_table[test_num]);
        run_status = amalfi_coast_run(&test_table[test_num]);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
        */
        fbe_test_physical_package_tests_config_unload();
    }
}
