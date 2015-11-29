/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * mount_vesuvius.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Mount Vesuvius scenario.
 * 
 *  This scenario is for a 1 port, 1 enclosure configuration, aka the maui
 *  configuration.
 * 
 *  The test run in this case is:
 *      - Initialize the 1-Port, 1-Enclosure configuration and 
 *          verify that the topology is correct.
 *      - Check the enclosure and the drives are in READY state
 *      - Query the trace buffer in the EMC Enclosure status Page. 
 *      - Check the status of each trace buffer.
 *      - Save the trace buffer control with the page code 
 *      - Shutdown the physical package. 
 *
 * HISTORY
 *   12/18/2008:  Created. ArunKumar Selvapillai
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "physical_package_tests.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_enclosure_data_access_public.h"


/*************************
 *   MACRO DEFINITIONS
 *************************/

/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

/*!*******************************************************************
 * @enum  mount_vesuvius_tests_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 *  mount_vesuvius_tests.
 *********************************************************************/

typedef enum mount_vesuvius_e
{
    MOUNT_VESUVIUS_TEST_NAGA,
    MOUNT_VESUVIUS_TEST_VIPER,
    MOUNT_VESUVIUS_TEST_DERRINGER,
    MOUNT_VESUVIUS_TEST_VOYAGER,
    MOUNT_VESUVIUS_TEST_BUNKER,
    MOUNT_VESUVIUS_TEST_CITADEL,
    MOUNT_VESUVIUS_TEST_FALLBACK,
    MOUNT_VESUVIUS_TEST_BOXWOOD,
    MOUNT_VESUVIUS_TEST_KNOT,
    MOUNT_VESUVIUS_TEST_CALYPSO,
    MOUNT_VESUVIUS_TEST_RHEA,
    MOUNT_VESUVIUS_TEST_MIRANDA,
    MOUNT_VESUVIUS_TEST_TABASCO,
} mount_vesuvius_tests_t;


/*********************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 *********************************************************************/

static fbe_test_params_t test_table[] = 
{
    {MOUNT_VESUVIUS_TEST_NAGA,
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
    {MOUNT_VESUVIUS_TEST_VIPER,
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
    {MOUNT_VESUVIUS_TEST_DERRINGER,
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
    {MOUNT_VESUVIUS_TEST_VOYAGER,
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
    {MOUNT_VESUVIUS_TEST_BUNKER,
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
    {MOUNT_VESUVIUS_TEST_CITADEL,
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
    {MOUNT_VESUVIUS_TEST_FALLBACK,
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
    {MOUNT_VESUVIUS_TEST_BOXWOOD,
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
    {MOUNT_VESUVIUS_TEST_KNOT,
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
// Moons
    {MOUNT_VESUVIUS_TEST_CALYPSO,
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
   {MOUNT_VESUVIUS_TEST_RHEA,
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
   {MOUNT_VESUVIUS_TEST_MIRANDA,
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
    {MOUNT_VESUVIUS_TEST_TABASCO,
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

/*!***************************************************************
 * @fn  mount_vesuvius_run
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
 *   12/18/2008:  Created. ArunKumar Selvapillai
 *
 ****************************************************************/

fbe_status_t mount_vesuvius_run(fbe_test_params_t *test)
{
    fbe_u8_t            *response_buf;
    fbe_u32_t           object_handle;
    fbe_u32_t           no_of_trace_buffers = 5;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_enclosure_status_t       enclosure_status = FBE_ENCLOSURE_STATUS_OK;

    fbe_enclosure_mgmt_ctrl_op_t           eses_page_op;
    fbe_enclosure_mgmt_trace_buf_cmd_t          trace_buf_cmd; 

    mut_printf(MUT_LOG_LOW, "****Mount Versuvius for %s ****\n",test->title);
    /* Get handle to enclosure object */
    status = fbe_api_get_enclosure_object_id_by_location(test->backend_number, test->encl_number, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    /* Take the structure fbe_enclosure_mgmt_ctrl_op_t and play with it 
        -   Read/Get the Trace buffer status
        -   Change the buf id, action and element index (Active/Save/Clear/Unhandled)
        -   Send diagnostic
        -   Repeat for other buf-ids.
    */
    
    /*******************    Get Trace Buffer Info ******************/

    /* Allocate the response buffer */
    response_buf = (fbe_u8_t *)malloc(no_of_trace_buffers * sizeof(ses_trace_buf_info_elem_struct) + 1);

    fbe_zero_memory(&trace_buf_cmd, sizeof(fbe_enclosure_mgmt_trace_buf_cmd_t));

    trace_buf_cmd.buf_op = FBE_ENCLOSURE_TRACE_BUF_OP_GET_STATUS;
    eses_page_op.cmd_buf.trace_buf_cmd = trace_buf_cmd;
    eses_page_op.response_buf_p = response_buf;
    eses_page_op.response_buf_size = (no_of_trace_buffers * sizeof(ses_trace_buf_info_elem_struct) + 1);

    mut_printf(MUT_LOG_LOW, "%s Request to get the Trace Buffer Info has been sent", __FUNCTION__);

    /* Get the trace buffer Info */
    status = fbe_api_enclosure_get_trace_buffer_info(object_handle, &eses_page_op, &enclosure_status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(enclosure_status == FBE_ENCLOSURE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "%s Received the Trace Buffer Information", __FUNCTION__);


    /*******************    Send Trace Buffer Control ******************/

    /* Set the buffer id and action */
    fbe_zero_memory(&trace_buf_cmd, sizeof(fbe_enclosure_mgmt_trace_buf_cmd_t));
    trace_buf_cmd.buf_id = 0x0;
    trace_buf_cmd.buf_op =  FBE_ENCLOSURE_TRACE_BUF_OP_SAVE;

    mut_printf(MUT_LOG_LOW, "%s Buffer action Control set to SAVE BUFFER", __FUNCTION__);

    fbe_zero_memory(&eses_page_op, sizeof(fbe_enclosure_mgmt_ctrl_op_t));

    /* For send Control buffer, we dont need the response buf and size */
    eses_page_op.cmd_buf.trace_buf_cmd = trace_buf_cmd;
    eses_page_op.response_buf_p = NULL;
    eses_page_op.response_buf_size = 0;

    /* Send the trace buffer control */
    status = fbe_api_enclosure_send_trace_buffer_control(object_handle, &eses_page_op);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "%s SAVE BUFFER successfully Sent", __FUNCTION__);

    /* Exercise FBE_ESES_TRACE_BUF_ACTION_STATUS_ACTIVE and 
     * FBE_ESES_TRACE_BUF_ACTION_STATUS_UNEXPECTED_EXCEPTION in similar fashion
     */

    /* release memory */
    free(response_buf);
    fbe_zero_memory(&trace_buf_cmd, sizeof(fbe_enclosure_mgmt_trace_buf_cmd_t));
    fbe_zero_memory(&eses_page_op, sizeof(fbe_enclosure_mgmt_ctrl_op_t));

    return status;
}

/*!***************************************************************
 * @fn mount_vesuvius()
 ****************************************************************
 * @brief:
 *  Function to load the config and run the 
 *  Mount Vesuvius scenario.
 *
 * @return 
 *  FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *   12/18/2008:  Created. ArunKumar Selvapillai
 *
 ****************************************************************/

void mount_vesuvius(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;

    for(test_num = 0; test_num < STATIC_TEST_MAX; test_num++)
    {
        mut_printf(MUT_LOG_LOW, "Starting Mount Vesuvius for %s ",test_table[test_num].title );
        /* Load the configuration for test
        */
        maui_load_and_verify_table_driven(&test_table[test_num]);
        run_status = mount_vesuvius_run(&test_table[test_num]);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
        */
        fbe_test_physical_package_tests_config_unload();
    }
}

