/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * turks_and_caicos.c
 ***************************************************************************
 *
 * DESCRIPTION
 * This test simulates putting an enclosure/expander into Test Mode and
 *  then sending various String Out commands to control drives & PHY's.
 *  This scenario is for a 1 port, 1 enclosures configuration, aka the maui.xml
 *  configuration.
 * 
 *  The test run in this case is:
 * 1. Load and verify the 1-Port, 1-Enclosure configuration. (use Maui configuration)
 * 2. Send String Out command to verify that it is rejected
 * 3. Send command to set Test Mode
 * 4. Send String Out command to remove a drive & verify that it succeeded.
 * 5. Send String Out command to bypass a PHY & verify that it succeeded.
 *
 * HISTORY
 *   2/17/2009:  Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_enclosure_data_access_public.h"

/*!*******************************************************************
 * @enum turks_and_caicos_test_number_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * turks_and_caicos_test.
 *********************************************************************/
typedef enum turks_and_caicos_test_number_e
{
    TURKS_AND_CAICOS_TEST_NAGA,
    TURKS_AND_CAICOS_TEST_VIPER,
    TURKS_AND_CAICOS_DERRINGER,
    TURKS_AND_CAICOS_TABASCO,
    TURKS_AND_CAICOS_TEST_BUNKER,
    TURKS_AND_CAICOS_CITADEL,
    TURKS_AND_CAICOS_TEST_FALLBACK,
    TURKS_AND_CAICOS_TEST_BOXWOOD,
    TURKS_AND_CAICOS_TEST_KNOT,
    TURKS_AND_CAICOS_TEST_ANCHO,
    TURKS_AND_CAICOS_TEST_CALYPSO,
    TURKS_AND_CAICOS_TEST_RHEA,
    TURKS_AND_CAICOS_TEST_MIRANDA,
} turks_and_caicos_test_number_t;

/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/
static fbe_test_params_t test_table[] = 
{
    {TURKS_AND_CAICOS_TEST_NAGA,
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
    {TURKS_AND_CAICOS_TEST_VIPER,
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
    {TURKS_AND_CAICOS_DERRINGER,
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
    {TURKS_AND_CAICOS_TABASCO,
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
    {TURKS_AND_CAICOS_TEST_BUNKER,
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
    {TURKS_AND_CAICOS_CITADEL,
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
    {TURKS_AND_CAICOS_TEST_FALLBACK,
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
    {TURKS_AND_CAICOS_TEST_BOXWOOD,
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
    {TURKS_AND_CAICOS_TEST_KNOT,
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
    {TURKS_AND_CAICOS_TEST_ANCHO,
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
    {TURKS_AND_CAICOS_TEST_CALYPSO,
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
    {TURKS_AND_CAICOS_TEST_RHEA,
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
    {TURKS_AND_CAICOS_TEST_MIRANDA,
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
} ;

/*!*******************************************************************
 * @def TURKS_AND_CAICOS_TEST_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define TURKS_AND_CAICOS_TEST_MAX     sizeof(test_table)/sizeof(test_table[0])

static fbe_status_t find_first_trace_buf_id(fbe_u8_t * response_buf_p, 
                                            fbe_eses_trace_buf_action_status_t buffer_action, 
                                            fbe_u8_t * buf_id_p);

/*
 * Structure/table of info on what things to test
 */
// drvovr tests
typedef struct
{
    fbe_u8_t        enclNumber;
    fbe_u8_t        driveNumber;
    char            *command;
} drive_string_out_info_t;

#define STRING_OUT_DRIVE_TABLE_COUNT    2
#define STRING_OUT_COMMAND_SIZE         23
drive_string_out_info_t stringOutDriveCommandTable[STRING_OUT_DRIVE_TABLE_COUNT] =
{   
    {   0,  1,      "drvovr 200000000000000\n"   },         // remove drive 1
    {   0,  7,      "drvovr 000200000000000\n"   },         // remove drive 7
};
// phyovr tests
typedef struct
{
    fbe_u8_t        enclNumber;
    fbe_u8_t        phyNumber;
} phy_string_out_info_t;

#define STRING_OUT_PHY_TABLE_COUNT    2
phy_string_out_info_t stringOutPhyCommandTable[STRING_OUT_PHY_TABLE_COUNT] =
{   
    {   0,      1   },
    {   0,      14  }
};

#define MAX_REQUEST_STATUS_COUNT    10

static fbe_status_t turks_and_caicos_run(fbe_test_params_t *test)
{
    fbe_u32_t                                       object_handle_p;
    fbe_status_t                                    status;
    fbe_u8_t                                        testPassCount;
    fbe_u8_t                                        driveNumber, enclNumber;
    fbe_enclosure_mgmt_ctrl_op_t               esesPageOp;
    fbe_enclosure_mgmt_ctrl_op_t               expTestModeControl;
    fbe_u8_t                                        buf_id = 0;
    fbe_u8_t                                        *response_buf_p = NULL;
    fbe_u32_t                                       cmp_result = 0;
    fbe_u32_t                                       response_buf_size = 0;
    fbe_u32_t                                       number_of_trace_bufs = 5;
    fbe_u8_t                                        activeTraceBuffer[FBE_BASE_EXP_STRING_OUT_CMD_MAX_LENGTH];
    fbe_api_terminator_device_handle_t                  encl_device_id;
    fbe_u32_t                                       stringOutLength;
    fbe_enclosure_status_t                          enclosure_status;

    mut_printf(MUT_LOG_LOW, "**********************************\n");
    mut_printf(MUT_LOG_LOW, "**** turks_and_caicos_run: Test String Out Commands for %s ****\n", test->title);
    mut_printf(MUT_LOG_LOW, "**********************************\n\n");

    for (testPassCount = 0; testPassCount < STRING_OUT_DRIVE_TABLE_COUNT; testPassCount++)
    {
        enclNumber = stringOutDriveCommandTable[testPassCount].enclNumber;
        driveNumber = stringOutDriveCommandTable[testPassCount].driveNumber;
        mut_printf(MUT_LOG_LOW, "**** Pass %d, BYPASS ENCL %d, DRIVE %d ****\n",
            testPassCount, enclNumber, driveNumber);

        /*
         *  Get handle to enclosure object
         */
        status = fbe_api_get_enclosure_object_id_by_location(test->backend_number, enclNumber, &object_handle_p);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

        /*
         * Enable Test Mode on Expander
         */
        fbe_zero_memory(&expTestModeControl, sizeof(fbe_enclosure_mgmt_ctrl_op_t));
        expTestModeControl.cmd_buf.testModeInfo.testModeAction = FBE_EXP_TEST_MODE_ENABLE;
        status = fbe_api_enclosure_send_exp_test_mode_control(object_handle_p, &expTestModeControl);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "Test Mode Enabled Successfully\n");

        status = fbe_api_terminator_get_enclosure_handle(test->backend_number, test->encl_number, &encl_device_id);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        fbe_zero_memory(&activeTraceBuffer, FBE_BASE_EXP_STRING_OUT_CMD_MAX_LENGTH);
        status = fbe_api_terminator_set_buf(encl_device_id,         // enclosure 0
                                            SES_SUBENCL_TYPE_LCC,
                                            0,                      // SIDE_A
                                            SES_BUF_TYPE_ACTIVE_TRACE,
                                            activeTraceBuffer,
                                            FBE_BASE_EXP_STRING_OUT_CMD_MAX_LENGTH);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        /*
         * Bypass the appropriate Drive from table
         */
        // send String Out command
        fbe_zero_memory(&esesPageOp, sizeof(fbe_enclosure_mgmt_ctrl_op_t));
        strcpy(esesPageOp.cmd_buf.stringOutInfo.stringOutCmd,
            stringOutDriveCommandTable[testPassCount].command);
        stringOutLength = (fbe_u32_t)strlen(esesPageOp.cmd_buf.stringOutInfo.stringOutCmd);
        status = fbe_api_enclosure_send_exp_string_out_control(object_handle_p, &esesPageOp);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "StringOut Sent Successfully\n");

        // Delay before requesting Read Buffer
        fbe_api_sleep (5000);

        /* Allocate memory for the response buffer which will contain the trace buffer info elements retrieved. */
        response_buf_size = number_of_trace_bufs * sizeof(ses_trace_buf_info_elem_struct) + 1;
        response_buf_p = (fbe_u8_t *)malloc(response_buf_size);
    
        fbe_api_enclosure_build_trace_buf_cmd(&esesPageOp, 
                                              FBE_ENCLOSURE_BUF_ID_UNSPECIFIED, 
                                              FBE_ENCLOSURE_TRACE_BUF_OP_GET_STATUS,
                                              response_buf_p, 
                                              response_buf_size);

        /* Send the command down to the physical package. */
        enclosure_status = FBE_ENCLOSURE_STATUS_OK;
        status = fbe_api_enclosure_get_trace_buffer_info(object_handle_p, &esesPageOp, &enclosure_status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(enclosure_status == FBE_ENCLOSURE_STATUS_OK);

        /* Find the first trace buffer id with the buffer action as FBE_ESES_TRACE_BUF_ACTION_STATUS_CLIENT_INIT_SAVE_BUF. */ 
        status = find_first_trace_buf_id(esesPageOp.response_buf_p, 
                                         FBE_ESES_TRACE_BUF_ACTION_STATUS_ACTIVE,  //the trace buffer action in status element, this is a saved trace buffer.
                                         &buf_id);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        /* Read the trace buffer and compare the content. */
        fbe_api_enclosure_build_read_buf_cmd(&esesPageOp, 
                                             buf_id,
//                                             FBE_ENCLOSURE_BUF_ID_UNSPECIFIED, 
                                             FBE_ESES_READ_BUF_MODE_DATA, 
                                             0, 
                                             response_buf_p, 
                                             response_buf_size);
        status = fbe_api_enclosure_read_buffer(object_handle_p, &esesPageOp, &enclosure_status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(enclosure_status == FBE_ENCLOSURE_STATUS_OK);

        cmp_result = memcmp(&stringOutDriveCommandTable[testPassCount].command[0], 
                            esesPageOp.response_buf_p, 
                            stringOutLength);
        mut_printf(MUT_LOG_LOW, "Active Trace Buffer Successfully Verified StringOut\n");

    }   // end of for loop
    return FBE_STATUS_OK;
}   // end of turks_and_caicos


/*!***************************************************************
 * @fn find_first_trace_buf_id(fbe_u8_t * response_buf_p, 
 *                                              fbe_eses_trace_buf_action_status_t buffer_action, 
 *                                              fbe_u8_t * buf_id_p)
 ****************************************************************
 * @brief
 *  Function to find the first trace buffer id with the specified
 *  buffer action.
 *
 * @param  response_buf_p - The pointer to the trace buffer info elements block.
 * @param  buffer_action - The specified buffer action.
 *
 * @return  fbe_status_t
 *
 * HISTORY:
 *  22-Jan-2009 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t find_first_trace_buf_id(fbe_u8_t * response_buf_p, 
                                               fbe_eses_trace_buf_action_status_t buffer_action, 
                                               fbe_u8_t * buf_id_p)
{
    ses_trace_buf_info_elem_struct  * trace_buf_info_elem_p;
    fbe_u8_t number_of_trace_buf_info_elems = 0;
    fbe_u8_t i = 0;

    if(response_buf_p == NULL)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "=== %s: NULL responser buffer ===\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    number_of_trace_buf_info_elems = *response_buf_p;
    trace_buf_info_elem_p = (ses_trace_buf_info_elem_struct *)(response_buf_p + 1);

    for(i = 0; i< number_of_trace_buf_info_elems; i++)
    {
        if(trace_buf_info_elem_p->buf_action == buffer_action)
        {
            *buf_id_p = trace_buf_info_elem_p->buf_id;
            return FBE_STATUS_OK;
        }
        trace_buf_info_elem_p = fbe_eses_get_next_trace_buf_info_elem_p(trace_buf_info_elem_p, FBE_ESES_EMC_CONTROL_TRACE_BUF_INFO_ELEM_SIZE);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: the trace buffer is not found. ===\n", __FUNCTION__);
    return FBE_STATUS_GENERIC_FAILURE;
}// End of function find_first_trace_buf_id()  

/*!***************************************************************
 * turks_and_caicos()
 ****************************************************************
 * @brief
 *  Function to run the turks_and_caicos scenario.
 *
 * @author
 *  08/02/2010 - Created. Trupti Ghate(to support table driven methodology)
 *
 ****************************************************************/

void turks_and_caicos(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;

    for(test_num = 0; test_num < TURKS_AND_CAICOS_TEST_MAX; test_num++)
    {
        /* Load the configuration for test
         */
        maui_load_and_verify_table_driven(&test_table[test_num]);
        run_status = turks_and_caicos_run(&test_table[test_num]);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
         */
        fbe_test_physical_package_tests_config_unload();
    }
}

