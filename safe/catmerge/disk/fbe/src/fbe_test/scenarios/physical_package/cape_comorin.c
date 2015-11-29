/***************************************************************************
 * cape_comorin.c (Reading a saved trace buffer and clearing a saved trace buffer)
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the cape comorin scenario that reads a saved trace and
 *  clearing a saved trace buffer.
 * 
 *  This scenario is for a 1 port, 1 enclosure configuration, aka the maui.xml
 *  configuration.
 * 
 *  The test run in this case is:
 *  1) Load and verify the configuration. (1-Port 1-Enclosure)
 *  2) Check the enclosure and drives are in READY state.
 *  3) Retrieve trace buffer information elements from enclosure.
 *  4) Parse the retrieved trace buffer infortion elements to pick a saved trace buffer.
 *  5) Inject the connect into the saved trace buffer which was picked.
 *  6) Read the connent of the saved trace buffer and verify the content matches the one injected.
 *  7) Clear the saved trace buffer.
 *  8) Read the connent of the saved trace buffer and verify the saved trace buffer has been cleared.
 *  9) Shutdown the Terminator/Physical package.
 *
 * HISTORY
 *   22-Jan-2009:  PHE Created.
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/

#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_topology_interface.h"

/*************************
 *   MACRO DEFINITIONS
 *************************/

/*!*******************************************************************
 * @enum cape_comorin_tests_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * cape_comorin_test.
 *********************************************************************/

typedef enum cape_comorin_e
{
    CAPE_COMORIN_TEST_NAGA,
    CAPE_COMORIN_TEST_VIPER,
    CAPE_COMORIN_TEST_DERRINGER,
    CAPE_COMORIN_TEST_VOYAGER,
    CAPE_COMORIN_TEST_BUNKER,
    CAPE_COMORIN_TEST_CITADEL,
    CAPE_COMORIN_TEST_FALLBACK,
    CAPE_COMORIN_TEST_BOXWOOD,
    CAPE_COMORIN_TEST_KNOT,
    CAPE_COMORIN_TEST_ANCHO,
    CAPE_COMORIN_TEST_CALYPSO,
    CAPE_COMORIN_TEST_RHEA,
    CAPE_COMORIN_TEST_MIRANDA,
    CAPE_COMORIN_TEST_TABASCO,
}cape_comorin_tests_t;


/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/
static fbe_test_params_t test_table[] = 
{
    {CAPE_COMORIN_TEST_NAGA,
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
    {CAPE_COMORIN_TEST_VIPER,
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
    {CAPE_COMORIN_TEST_DERRINGER,
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
    {CAPE_COMORIN_TEST_VOYAGER,
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
    {CAPE_COMORIN_TEST_BUNKER,
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
    {CAPE_COMORIN_TEST_CITADEL,
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
    {CAPE_COMORIN_TEST_FALLBACK,
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
    {CAPE_COMORIN_TEST_BOXWOOD,
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
    {CAPE_COMORIN_TEST_KNOT,
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
    {CAPE_COMORIN_TEST_ANCHO,
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
    {CAPE_COMORIN_TEST_CALYPSO,
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
    {CAPE_COMORIN_TEST_RHEA,
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
    {CAPE_COMORIN_TEST_MIRANDA,
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
    {CAPE_COMORIN_TEST_TABASCO,
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
 * @def CAPE_COMORIN_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define CAPE_COMORIN_MAX     sizeof(test_table)/sizeof(test_table[0])

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

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


/*!**************************************************************
 * @fn cape_comorin_run()
 ****************************************************************
 * @brief
 *      Run the cape comorin test.
 *      This is a 1 port, 1 enclosure test.
 *
 * @param  - none          
 *
 * @return  FBE_STATUS_OK if no errors. 
 *
 * HISTORY:
 *  22-Jan-2009 PHE - Created.
 *
 ****************************************************************/
static fbe_status_t cape_comorin_run(fbe_test_params_t *test)
{
    fbe_u8_t            * response_buf_p = NULL;
    fbe_object_id_t     object_id;
    fbe_u32_t           number_of_trace_bufs = 5;
    fbe_status_t        status = FBE_STATUS_OK;
    char inject_content[] = "This is the cape comorin test senario.";
    fbe_u32_t           cmp_result = 0;
    fbe_u8_t            buf_id = 0;
    fbe_u32_t           response_buf_size = 0;
    fbe_u32_t           inject_content_size = 0;
    fbe_enclosure_status_t enclosure_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_api_terminator_device_handle_t encl_device_id;

    fbe_enclosure_mgmt_ctrl_op_t           eses_page_op;

    mut_printf(MUT_LOG_LOW, "****  Cape_Comorin Scenario: Start for %s  ****",test->title);

    /* Get the handle of the enclosure object. */
    status = fbe_api_get_enclosure_object_id_by_location(test->backend_number, test->encl_number, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
     
    /* (1) Retrieve the trace buffer info elements.*/
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Retrieving the trace buffer info elements ===\n", __FUNCTION__);

    /* Allocate memory for the response buffer which will contain the trace buffer info elements retrieved. */
    response_buf_size = number_of_trace_bufs * sizeof(ses_trace_buf_info_elem_struct) + 1;
    response_buf_p = (fbe_u8_t *)malloc(response_buf_size);
    
    fbe_api_enclosure_build_trace_buf_cmd(&eses_page_op, FBE_ENCLOSURE_BUF_ID_UNSPECIFIED, FBE_ENCLOSURE_TRACE_BUF_OP_GET_STATUS,
                                  response_buf_p, response_buf_size);

    /* Send the command down to the physical package. */
    status = fbe_api_enclosure_get_trace_buffer_info(object_id, &eses_page_op, &enclosure_status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(enclosure_status == FBE_ENCLOSURE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: The trace buffer information elements retrieved successfully ===", __FUNCTION__);

    /*(2) Find the first trace buffer id with the buffer action as FBE_ESES_TRACE_BUF_ACTION_STATUS_CLIENT_INIT_SAVE_BUF. */ 
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Searching for a saved trace buffer id ===", __FUNCTION__);

    status = find_first_trace_buf_id(eses_page_op.response_buf_p, 
                                        FBE_ESES_TRACE_BUF_ACTION_STATUS_CLIENT_INIT_SAVE_BUF,  //the trace buffer action in status element, this is a saved trace buffer.
                                        &buf_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: A saved trace buffer (id: %d) found successfully ===", __FUNCTION__, buf_id);

    /*(3) Inject the content to the saved trace buffer. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Injecting the content to the saved trace buffer (id: %d) ===", __FUNCTION__, buf_id);

    inject_content_size = sizeof(inject_content);
    status = fbe_api_terminator_get_enclosure_handle(test->backend_number, test->encl_number, &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_terminator_set_buf_by_buf_id(encl_device_id, buf_id, (fbe_u8_t *)inject_content, inject_content_size);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: The content is injected to the trace buffer (id: %d) successfully ===", __FUNCTION__, buf_id);

    /*(4) Read the trace buffer and compare the content. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Reading the trace buffer (id: %d) ===", __FUNCTION__, buf_id);

    /* Read the trace buffer. */
    fbe_api_enclosure_build_read_buf_cmd(&eses_page_op, buf_id, FBE_ESES_READ_BUF_MODE_DATA, 0, response_buf_p, inject_content_size);
    status = fbe_api_enclosure_read_buffer(object_id, &eses_page_op, &enclosure_status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(enclosure_status == FBE_ENCLOSURE_STATUS_OK);

    /* Compare the content. */    
    cmp_result = memcmp(&inject_content[0], eses_page_op.response_buf_p, inject_content_size);
    MUT_ASSERT_INT_EQUAL(0, cmp_result);
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: The content of the trace buffer id %d has been read back successfully ===", __FUNCTION__, buf_id);

    /*(5) Clear the trace buffer with the specified buffer id. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Clearing the trace buffer (id: %d) ===", __FUNCTION__, buf_id);

    status = fbe_api_enclosure_build_trace_buf_cmd(&eses_page_op, buf_id, FBE_ENCLOSURE_TRACE_BUF_OP_CLEAR, NULL, 0);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    status = fbe_api_enclosure_send_trace_buffer_control(object_id, &eses_page_op);    
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Request to clear the trace buffer (id: %d) has been sent ===", __FUNCTION__, buf_id);

    /*(6) Verify the trace buffer has been cleared (all 0s). */
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Verifying the trace buffer (id: %d) has been cleared ===", __FUNCTION__, buf_id);

    /* Read the trace buffer. */
    fbe_api_enclosure_build_read_buf_cmd(&eses_page_op, buf_id, FBE_ESES_READ_BUF_MODE_DATA, 0, response_buf_p, inject_content_size);
    status = fbe_api_enclosure_read_buffer(object_id, &eses_page_op, &enclosure_status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(enclosure_status == FBE_ENCLOSURE_STATUS_OK);

    /* Compare the content. */
    fbe_zero_memory(&inject_content[0], inject_content_size);
    cmp_result = memcmp(&inject_content[0], eses_page_op.response_buf_p, inject_content_size);
    MUT_ASSERT_INT_EQUAL(0, cmp_result);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: The trace buffer (id: %d) has been cleared successfully ===", __FUNCTION__, buf_id);

    /* release memory */
    free(response_buf_p);
    mut_printf(MUT_LOG_LOW, "Cape_Comorin Scenario: End for %s",test->title);
    return status;
}// End of function cape_comorin_run()

/*!***************************************************************
 * @fn cape_comorin()
 ****************************************************************
 * @brief
 *  This function executes the cape comorin scenario.
 *
 * @param  - None.               
 *
 * @return  - None.
 *
 * HISTORY:
 *  22-Jan-2009 PHE - Created. 
 *  02-Aug-2-11- Updated. To support table driven approach. 
 ****************************************************************/

void cape_comorin(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;
	
    /* This scenario uses the "maui" configuration, which is a 1 port, 1 
     * enclosure configuration. 
     *  
     * Use the configuration specific load and validate routine. 
     * We always expect it to load and validate successfully. 
     */

	/* Now, run the scenario.  We always expect this to work.
     */
    for(test_num = 0; test_num < CAPE_COMORIN_MAX; test_num++)
    {
        maui_load_and_verify_table_driven(&test_table[test_num]);
        run_status = cape_comorin_run(&test_table[test_num]);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);
        fbe_test_physical_package_tests_config_unload();
    }
    return;
} // End of function cape_comorin()


/* End of file cape_comorin.c */

