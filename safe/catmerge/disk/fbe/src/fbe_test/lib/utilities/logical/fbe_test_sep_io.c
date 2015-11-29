/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_sep_io.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions for use with the Storage Extent 
 *  Package (SEP).
 * 
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "sep_tests.h"
#include "fbe_test_common_utils.h"
#include "sep_utils.h"
#include "sep_rebuild_utils.h"
#include "sep_test_io.h"
#include "sep_test_io_private.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_transport.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_database_interface.h"

/*****************************************
 * DEFINITIONS
 *****************************************/
/*!*******************************************************************
 * @def FBE_TEST_SEP_IO_RDGEN_WAIT_MS
 *********************************************************************
 * @brief milliseconds rdgen should wait for certain operations.
 *
 *********************************************************************/
#define FBE_TEST_SEP_IO_RDGEN_WAIT_MS 360000

/*!*******************************************************************
 * @def FBE_TEST_SEP_IO_BACKGROUND_OPS_WAIT_MS
 *********************************************************************
 * @brief milliseconds rdgen should wait for certain operations.
 *
 *********************************************************************/
#define FBE_TEST_SEP_IO_BACKGROUND_OPS_WAIT_MS 360000

/*************************
 *   GLOBALS
 *************************/

/*!*******************************************************************
 * @var fbe_test_sep_io_small_io_count
 *********************************************************************
 * @brief Default number of small I/Os we will perform
 *
 *********************************************************************/
static fbe_u32_t fbe_test_sep_io_small_io_count = 5;

/*!*******************************************************************
 * @var fbe_test_sep_io_large_io_count
 *********************************************************************
 * @brief Number of large I/Os we will perform before finishing the test.
 *
 *********************************************************************/
static fbe_u32_t fbe_test_sep_io_large_io_count = 5;

/*!*******************************************************************
 * @var fbe_test_sep_io_thread_count
 *********************************************************************
 * @brief Number of threads we will run for I/O.
 *
 *********************************************************************/
static fbe_u32_t fbe_test_sep_io_thread_count = 5;

/*!*******************************************************************
 * @var fbe_test_sep_io_seconds
 *********************************************************************
 * @brief Number of seconds we will run I/O for .
 *
 *********************************************************************/
static fbe_u32_t fbe_test_sep_io_seconds = 0;

/*!*******************************************************************
 * @var fbe_test_sep_io_current_abort_msecs
 *********************************************************************
 * @brief Our one copy of the abort msecs value
 *
 *********************************************************************/
static fbe_test_random_abort_msec_t fbe_test_sep_io_current_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;

/*!*******************************************************************
 * @var fbe_test_sep_io_current_peer_options
 *********************************************************************
 * @brief Our one copy of the options for sending I/O to the peer.
 *
 *********************************************************************/
static fbe_api_rdgen_peer_options_t fbe_test_sep_io_current_peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;

/*!*******************************************************************
 * @var fbe_test_sep_io_b_background_op_enable
 *********************************************************************
 * @brief A bool that detemines if background operations are enabled
 *        during I/O.
 *
 * @note  Background operation(s) during I/O are disabled by default
 *********************************************************************/
static fbe_bool_t fbe_test_sep_io_b_background_op_enable = FBE_FALSE;

/*!*******************************************************************
 * @var fbe_test_sep_io_current_background_ops
 *********************************************************************
 * @brief Bitmask of current background operations that will run during
 *        I/O.
 *
 * @note  Background operations must be enabled for this mask to mean
 *        anything.
 *********************************************************************/
static fbe_test_sep_io_background_op_enable_t fbe_test_sep_io_current_background_ops = FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_INVALID;

/*!*******************************************************************
 * @var fbe_test_sep_io_background_ops_small_io_count
 *********************************************************************
 * @brief Default number of small I/Os we will perform
 *
 *********************************************************************/
static fbe_u32_t fbe_test_sep_io_background_ops_small_io_count = (5 * 10);

/*!*******************************************************************
 * @var fbe_test_sep_io_background_ops_large_io_count
 *********************************************************************
 * @brief Number of large I/Os we will perform before finishing the test.
 *
 *********************************************************************/
static fbe_u32_t fbe_test_sep_io_background_ops_large_io_count = (5 * 10);

/*!*******************************************************************
 * @var fbe_test_sep_io_test_sequence_configuration
 *********************************************************************
 * @brief The current (i.e. the one executing) test sequence 
 *        configuration.
 *
 *********************************************************************/
static fbe_test_sep_io_sequence_config_t  fbe_test_sep_io_test_sequence_config = {0};
static fbe_test_sep_io_sequence_config_t *fbe_test_sep_io_test_sequence_config_p = &fbe_test_sep_io_test_sequence_config;

/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************************
 *          fbe_test_sep_io_get_sequence_config()
 *****************************************************************************
 *
 * @brief   Return the pointer to the current test sequence configuration
 *
 * @param   None       
 *
 * @return  Pointer to the the current test sequence configuraiton.
 *
 *****************************************************************************/
static fbe_test_sep_io_sequence_config_t *fbe_test_sep_io_get_sequence_config(void)
{
    return(fbe_test_sep_io_test_sequence_config_p);
}
/*********************************************
 * end fbe_test_sep_io_get_sequence_config()
 *********************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_get_sequence_io_state()
 *****************************************************************************
 *
 * @brief   Return the current value of the sequence io state.
 *
 * @param   sequence_config_p - Pointer to test sequence configuration 
 *
 * @return  The current state of the sep I/O sequence (ready for I/O etc)...
 *
 *****************************************************************************/
fbe_test_sep_io_sequence_state_t fbe_test_sep_io_get_sequence_io_state(fbe_test_sep_io_sequence_config_t *sequence_config_p)
{
    return(sequence_config_p->sequence_state);
}
/*********************************************
 * end fbe_test_sep_io_get_sequence_io_state()
 *********************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_set_sequence_io_state()
 *****************************************************************************
 *
 * @brief   Return the current value of the sequence io state.
 *
 * @param   sequence_config_p - Pointer to test sequence configuration to change
 *              state for.
 * @param   new_sequence_io_state - New (next) sequence I/O state to enter      
 *
 * @return  status - Not allow to go back a state
 *
 * @note    Currently the sequence I/O state can only increase
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_io_set_sequence_io_state(fbe_test_sep_io_sequence_config_t *sequence_config_p,
                                                          fbe_test_sep_io_sequence_state_t new_sequence_io_state)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_test_sep_io_sequence_state_t current_sequence_state = fbe_test_sep_io_get_sequence_io_state(sequence_config_p);

    /*! @note Sequence state is only allowed to stay the same or increase or
     *        reset (i.e. set to `Invalid').
     */
    if ((new_sequence_io_state != FBE_TEST_SEP_IO_SEQUENCE_STATE_INVALID) &&
        (new_sequence_io_state < current_sequence_state)                     )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "set sequence: attempt to lower from: %d to: %d",
                   current_sequence_state, new_sequence_io_state);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Else simply update the sequence state
     */
    mut_printf(MUT_LOG_TEST_STATUS, "set sequence: setting sequence io state to: %d",
               new_sequence_io_state);
    sequence_config_p->sequence_state = new_sequence_io_state;

    return status;
}
/*********************************************
 * end fbe_test_sep_io_get_sequence_io_state()
 *********************************************/

/*!**************************************************************
 * fbe_test_sep_io_get_small_io_count()
 ****************************************************************
 * @brief
 *  Return the number of I/Os to perform during this test.
 *
 * @param - default_io_count - default io count to use if parameter
 *                             is not set.            
 *
 * @return - Number of I/Os that we should perform in this test.
 *
 ****************************************************************/

fbe_u32_t fbe_test_sep_io_get_small_io_count(fbe_u32_t default_io_count)
{
    /* Start with the default above.
     */
    fbe_u32_t io_count;

    if (default_io_count != FBE_U32_MAX)
    {
        io_count = default_io_count;
    }
    else
    {
        io_count = fbe_test_sep_io_small_io_count;
    }
    if (fbe_test_sep_util_get_cmd_option_int("-fbe_small_io_count", &io_count))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using small io count of: %d", io_count);
    }
    return io_count;
}
/******************************************
 * end fbe_test_sep_io_get_small_io_count()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_io_get_large_io_count()
 ****************************************************************
 * @brief
 *  Return the number of large I/Os to perform during this test.
 *
 * @param - default_io_count - default io count to use if parameter
 *                             is not set.              
 *
 * @return - Number of large I/Os that we should perform in this test.
 *
 ****************************************************************/

fbe_u32_t fbe_test_sep_io_get_large_io_count(fbe_u32_t default_io_count)
{
    /* Start with the default above.
     */
    fbe_u32_t io_count;
    if (default_io_count != FBE_U32_MAX)
    {
        io_count = default_io_count;
    }
    else
    {
        io_count = fbe_test_sep_io_large_io_count;
    }
    if (fbe_test_sep_util_get_cmd_option_int("-fbe_large_io_count", &io_count))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using large io count of: %d", io_count);
    }
    return io_count;
}
/******************************************
 * end fbe_test_sep_io_get_large_io_count()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_io_set_io_seconds()
 ****************************************************************
 * @brief
 *  Set the number of seconds to perform I/Os.
 *
 * @param   io_seconds - Number of seconds to run I/O for            
 *
 * @return  status
 *
 ****************************************************************/

fbe_status_t fbe_test_sep_io_set_io_seconds(fbe_u32_t io_seconds)
{
    /* Trace a message if changing.
     */
    if (io_seconds != fbe_test_sep_io_seconds)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "changing I/O seconds from: %d to %d", fbe_test_sep_io_seconds, io_seconds);
        fbe_test_sep_io_seconds = io_seconds;
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_sep_io_set_io_seconds()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_io_get_io_seconds()
 ****************************************************************
 * @brief
 *  Return the number of seconds to perform I/Os.
 *
 * @param  - None.               
 *
 * @return - Number of I/Os that we should perform in this test.
 *
 ****************************************************************/

fbe_u32_t fbe_test_sep_io_get_io_seconds(void)
{
    /* Start with the default.
     */
    fbe_u32_t io_seconds = fbe_test_sep_io_seconds;

    if (fbe_test_sep_util_get_cmd_option_int("-fbe_io_seconds", &io_seconds))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using io time of: %d seconds", io_seconds);
    }
    return io_seconds;
}
/******************************************
 * end fbe_test_sep_io_get_io_seconds()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_io_set_threads()
 ****************************************************************
 * @brief
 *  Set the number of threads to perform during this test.
 *
 * @param io_thread_count - Number of threads to use.
 *
 * @return  status
 *
 ****************************************************************/

fbe_status_t fbe_test_sep_io_set_threads(fbe_u32_t io_thread_count)
{
    /* Trace a message if changing.
     */
    if (io_thread_count != fbe_test_sep_io_thread_count)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "changing I/O threads from: %d to %d", fbe_test_sep_io_thread_count, io_thread_count);
        fbe_test_sep_io_thread_count = io_thread_count;
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_sep_io_set_threads()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_io_get_threads()
 ****************************************************************
 * @brief
 *  Return the number of threads to perform during this test.
 *
 * @param default_thread_count - Number of threads to issue if
 *                               parameter is not set.               
 *
 * @return - Number of threads that we should perform in this test.
 *
 ****************************************************************/

fbe_u32_t fbe_test_sep_io_get_threads(fbe_u32_t default_thread_count)
{
    /* Start with the default above.
     */
    fbe_u32_t thread_count;

    if (default_thread_count != FBE_U32_MAX)
    {
        thread_count = default_thread_count;
    }
    else
    {
        thread_count = fbe_test_sep_io_thread_count;
    }
    if (fbe_test_sep_util_get_cmd_option_int("-fbe_threads", &thread_count))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using thread count of: %d", thread_count);
    }
    return thread_count;
}
/******************************************
 * end fbe_test_sep_io_get_threads()
 ******************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_is_background_ops_enabled()
 *****************************************************************************
 *
 * @brief   Simply determines if background operation(s) are enabled or not
 *
 * @param   None
 *
 * @return  bool - FBE_TRUE - One or more background operation(s) are enabled.
 *
 *****************************************************************************/
static fbe_bool_t fbe_test_sep_io_is_background_ops_enabled(void)
{
    return fbe_test_sep_io_b_background_op_enable;
}
/************************************************
 * end fbe_test_sep_io_is_background_ops_enabled()
 ************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_get_background_ops()
 *****************************************************************************
 *
 * @brief   Return the value of background operations.  If background operations
 *          are not enabled it expects that there are not background operations
 *          set.
 *
 * @param   None
 *
 * @return  background ops - Bitmask of background operations that are enabled.
 *
 *****************************************************************************/
static fbe_test_sep_io_background_op_enable_t fbe_test_sep_io_get_background_ops(void)
{
    if (fbe_test_sep_io_b_background_op_enable == FBE_FALSE)
    {
        MUT_ASSERT_INT_EQUAL(FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_INVALID, fbe_test_sep_io_current_background_ops);
    }
    else
    {
        MUT_ASSERT_INT_NOT_EQUAL(FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_INVALID, fbe_test_sep_io_current_background_ops);
    }
    return fbe_test_sep_io_current_background_ops;
}
/************************************************
 * end fbe_test_sep_io_get_background_ops()
 ************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_set_background_operations()
 *****************************************************************************
 *
 * @brief   Set the which background operaitons are enabled during I/O.  This
 *          will validate that the specific operations cna be run (together etc.).
 *
 * @param   b_enable_background_op - FBE_TRUE - Enable the specified background
 *              operations during the I/O test.  By default background operations
 *              are not enabled.
 *                                   FBE_FALSE - Disable all background operations.
 * @param   background_enable_flags - Bitmask of which background operations to
 *              enable.  
 *
 * @note    By default background operations during I/O are disabled.
 *
 * @return  status - FBE_STATUS_OK
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_io_set_background_operations(fbe_bool_t b_enable_background_op,
                                                              fbe_test_sep_io_background_op_enable_t background_enable_flags)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* If background operations are being disabled simply set then to invalid
     */
    if (b_enable_background_op == FBE_FALSE)
    {
        status = fbe_test_sep_background_ops_cleanup();
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_test_sep_io_b_background_op_enable = FBE_FALSE;
        fbe_test_sep_io_current_background_ops = FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_INVALID;
        return FBE_STATUS_OK;
    }

    /*! @note Validate that the selected background operations can run together.
     * 
     *  @note Currently we only allow (1) background operation to run at a time.
     */
    switch(background_enable_flags)
    {
        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_VERIFY:
            /*! @note Currently verify is not supported.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s Verify background op: 0x%x is not supported.",
                       __FUNCTION__, background_enable_flags);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_USER_COPY:
            /* Run a user copy during the I/O.
             */
            fbe_test_sep_io_current_background_ops = background_enable_flags;
            break;

        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_PROACTIVE_COPY:
            /* Run a proactive copy during the I/O.
             */
            fbe_test_sep_io_current_background_ops = background_enable_flags;
            break;

        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_USER_PROACTIVE_COPY:
            /*! @note Currently user proactive copy is not supported.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s User proactive copy background op: 0x%x is not supported.",
                       __FUNCTION__, background_enable_flags);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_DRIVE_GLITCH:
            /* `Glitch' a drive during I/O.
             */
            fbe_test_sep_io_current_background_ops = background_enable_flags;
            break;

        default:
            /* Invalid/unsupported combination of background operations.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s Unsupported setting for background ops: 0x%x",
                       __FUNCTION__, background_enable_flags);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    /* If the specified flags are valid, enable the background operations.
     */
    if (status == FBE_STATUS_OK)
    {
        fbe_test_sep_io_b_background_op_enable = FBE_TRUE;
    }

    /* Return the execution status.
     */
    return status;
}
/*******************************************************
 * end fbe_test_sep_io_set_background_operations()
 *******************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_configure_background_operations()
 *****************************************************************************
 *
 * @brief   Configure which (if any) background operations to enable during I/O.
 *          I/O will start, then any enabled background operations will be 
 *          started, then prior to stopping the I/O we wait for the background
 *          operation(s) to complete.
 *
 * @param   b_enable_background_op - FBE_TRUE - Enable the specified background
 *              operations during the I/O test.  By default background operations
 *              are not enabled.
 *                                   FBE_FALSE - Disable all background operations.
 * @param   background_enable_flags - Bitmask of which background operations to
 *              enable.  
 *
 * @note    By default background operations during I/O are disabled.
 *
 * @return  status - FBE_STATUS_OK
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_configure_background_operations(fbe_bool_t b_enable_background_op,
                                                             fbe_test_sep_io_background_op_enable_t background_enable_flags)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* It is valid to invoke 
     */
    if (b_enable_background_op == FBE_FALSE)
    {
        /* Set the background operations to invalid
         */
        status = fbe_test_sep_io_set_background_operations(FBE_FALSE,
                                                           FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_INVALID);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);        
        mut_printf(MUT_LOG_TEST_STATUS, "%s Background operations have been disabled.",
                   __FUNCTION__);
        return FBE_STATUS_OK;
    }


    /* If successful display which background operation(s) will run.
     */
    status = fbe_test_sep_io_set_background_operations(FBE_TRUE,
                                                       background_enable_flags);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (status == FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Background ops: 0x%x will be enabled.",
                   __FUNCTION__, background_enable_flags);
    }
    else
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Failed to enable background ops: 0x%x ",
                   __FUNCTION__, background_enable_flags);
    }

    /* Return the execution status.
     */
    return status;
}
/*******************************************************
 * end fbe_test_sep_io_configure_background_operations()
 *******************************************************/

/*!**************************************************************
 * fbe_test_sep_io_background_ops_get_small_io_count()
 ****************************************************************
 * @brief
 *  Return the number of I/Os to perform during this test.
 *
 * @param - default_io_count - default io count to use if parameter
 *                             is not set.            
 *
 * @return - Number of I/Os that we should perform in this test.
 *
 ****************************************************************/
static fbe_u32_t fbe_test_sep_io_background_ops_get_small_io_count(fbe_u32_t default_io_count)
{
    /* Start with the default above.
     */
    fbe_u32_t io_count;

    if (default_io_count != FBE_U32_MAX)
    {
        io_count = default_io_count;
    }
    else
    {
        io_count = fbe_test_sep_io_background_ops_small_io_count;
    }
    if (fbe_test_sep_util_get_cmd_option_int("-fbe_background_ops_small_io_count", &io_count))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using small io count of: %d", io_count);
    }
    return io_count;
}
/*********************************************************
 * end fbe_test_sep_io_background_ops_get_small_io_count()
 *********************************************************/

/*!**************************************************************
 * fbe_test_sep_io_background_ops_get_large_io_count()
 ****************************************************************
 * @brief
 *  Return the number of large I/Os to perform during this test.
 *
 * @param - default_io_count - default io count to use if parameter
 *                             is not set.              
 *
 * @return - Number of large I/Os that we should perform in this test.
 *
 ****************************************************************/
static fbe_u32_t fbe_test_sep_io_background_ops_get_large_io_count(fbe_u32_t default_io_count)
{
    /* Start with the default above.
     */
    fbe_u32_t io_count;
    if (default_io_count != FBE_U32_MAX)
    {
        io_count = default_io_count;
    }
    else
    {
        io_count = fbe_test_sep_io_background_ops_large_io_count;
    }
    if (fbe_test_sep_util_get_cmd_option_int("-fbe_background_ops_large_io_count", &io_count))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using large io count of: %d", io_count);
    }
    return io_count;
}
/*********************************************************
 * end fbe_test_sep_io_background_ops_get_large_io_count()
 *********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_run_rdgen_tests_with_background_ops()
 *****************************************************************************
 *
 * @brief   Kick off testing for a given context structure array.  Then start
 *          the background operation(s) that are enabled.  Wait for I/O to
 *          complete.  The wait for background operation to complete.
 *
 * @param   context_p - array of structures to test.
 * @param   package_id - package where rdgen service is.
 * @param   num_tests - number of tests in structure.
 *
 * @note    We assume the structure was previously initted via a
 *          call to fbe_api_rdgen_test_context_init().
 *
 * @return  FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  03/22/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_io_run_rdgen_tests_with_background_ops(fbe_api_rdgen_context_t *context_p,
                                                                        fbe_package_id_t package_id,
                                                                        fbe_u32_t num_tests)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               test_index;
    fbe_test_sep_io_sequence_config_t      *sequence_config_p = NULL;
    fbe_u32_t                               background_ops_small_io_count;
    fbe_u32_t                               background_ops_large_io_count;

    /* Get the sequence information, the background operations.
     * type.
     */
    sequence_config_p = fbe_test_sep_io_get_sequence_config();

    /* Step 0: Increase the amount of I/O so there is more overlap with the
     *         background operation.
     */
    background_ops_small_io_count = fbe_test_sep_io_background_ops_get_small_io_count(FBE_U32_MAX);
    background_ops_large_io_count = fbe_test_sep_io_background_ops_get_large_io_count(FBE_U32_MAX);
    if ((sequence_config_p->small_io_count < background_ops_small_io_count) ||
        (sequence_config_p->large_io_count < background_ops_large_io_count)    )
    {
        /* Bump the I/O count
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s Update small/large passes:%d/%d to:%d/%d more overlap.", 
                   __FUNCTION__,
                   sequence_config_p->small_io_count, sequence_config_p->large_io_count,
                   background_ops_small_io_count, background_ops_large_io_count);
        sequence_config_p->small_io_count = background_ops_small_io_count;
        sequence_config_p->large_io_count = background_ops_large_io_count;
    }

    /* Step 1:  Start the I/O
     */
    status = fbe_api_rdgen_start_tests(context_p, package_id, num_tests);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    
    /* Step 2: Start the requested background operation(s).  Reset the setting
     *         that determines if background operations should run or not.
     */
    sequence_config_p->b_should_background_ops_run = fbe_test_sep_io_is_background_ops_enabled();
    status = fbe_test_sep_start_background_ops(sequence_config_p, context_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Step 3:  Wait for background operation(s) to be complete.
     */
    status = fbe_test_sep_wait_for_background_ops_complete(sequence_config_p,
                                                           FBE_TEST_SEP_IO_BACKGROUND_OPS_WAIT_MS,
                                                           context_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Step 4: Wait for all tests to finish.
     */
    for (test_index = 0; test_index < num_tests; test_index++)
    {
        
        fbe_time_t wait_ms = 0;
        if (context_p[test_index].start_io.specification.max_number_of_ios != 0)
        {
            /* We will conservatively guess that it takes one second per I/O as an upper limit.
             */ 
            wait_ms = FBE_TIME_MILLISECONDS_PER_SECOND * context_p[test_index].start_io.specification.max_number_of_ios;
        }
        else if (context_p[test_index].start_io.specification.max_passes != 0)
        {
            /* We will conservatively guess that it takes one second per I/O as an upper limit.
             */ 
            wait_ms = FBE_TIME_MILLISECONDS_PER_SECOND * context_p[test_index].start_io.specification.max_passes;
        }
        else if (context_p[test_index].start_io.specification.msec_to_run)
        {
            /* Just use the milliseconds.
             */
            wait_ms = context_p[test_index].start_io.specification.msec_to_run;
        }

        if (wait_ms < FBE_TEST_SEP_IO_RDGEN_WAIT_MS)
        {
            /* Small number of I/Os or something other than passes, num_ios or msecs specified.  Just wait for a
             * constant time. 
             */
            wait_ms = FBE_TEST_SEP_IO_RDGEN_WAIT_MS;
        }
        status = fbe_semaphore_wait_ms(&context_p[test_index].semaphore, (fbe_u32_t)3600000);

        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s semaphore wait failed with status of 0x%x\n",
                          __FUNCTION__, status);
            return status;
        }
    }

    /* Step 5:  Wait for background operation(s) to be cleaned up.
     */
    status = fbe_test_sep_wait_for_background_ops_cleanup(sequence_config_p,
                                                          FBE_TEST_SEP_IO_BACKGROUND_OPS_WAIT_MS,
                                                          context_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    return status;
}
/**********************************************************
 * end fbe_test_sep_io_run_rdgen_tests_with_background_ops()
 **********************************************************/

/*!**************************************************************
 * fbe_test_sep_io_run_rdgen_tests()
 ****************************************************************
 * @brief
 *  Run the entire test from start to finish.
 *
 * @param context_p - The context structure.
 * @param package_id - package where rdgen service is.
 * @param num_tests - The number of tests to run   
 * @param rg_config_p - Pointer to array of raid groups under test            
 *
 * @return fbe_status_t
 *
 * @author
 *  9/25/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_test_sep_io_run_rdgen_tests(fbe_api_rdgen_context_t *context_p,
                                             fbe_package_id_t package_id,
                                             fbe_u32_t num_tests,
                                             fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       test_index;
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Run all the tests.  If background operations are enabled, use
     * our own start method.
     */
    if (fbe_test_sep_io_is_background_ops_enabled() == FBE_FALSE)
    {
        status = fbe_api_rdgen_run_tests(context_p, package_id, num_tests);
    }
    else
    {
        status = fbe_test_sep_io_run_rdgen_tests_with_background_ops(context_p, package_id, num_tests);
    }
    if (status != FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_TEST_STATUS,
                   "%s failed running tests with status %d \n", __FUNCTION__, status);
        if (status == FBE_STATUS_TIMEOUT)
        {

            /* destroy the context before return */
            for ( test_index = 0; test_index < num_tests; test_index++)
            {
                fbe_status_t destroy_status;
                /* Destroy the test objects. */
                destroy_status = fbe_api_rdgen_test_context_destroy(context_p);
                if (destroy_status != FBE_STATUS_OK)
                {
                    mut_printf(MUT_LOG_TEST_STATUS,
                               "%s destroy failed with status %d\n", __FUNCTION__,  destroy_status);
                }

                /* Goto next test context */
                context_p++;
            }
        
            return status;
        }
    }

    for ( test_index = 0; test_index < num_tests; test_index++)
    {
        fbe_status_t destroy_status;
        fbe_status_t rdgen_status;
        fbe_api_rdgen_display_context_status(context_p, 
                                             (fbe_trace_func_t)mut_printf, 
                                             (fbe_trace_context_t)MUT_LOG_TEST_STATUS);

        /* Get the status from the context structure.
         * Make sure no errors were encountered. 
         */
        rdgen_status = fbe_api_rdgen_get_status(context_p, 1);
        if (rdgen_status != FBE_STATUS_OK)
        {
            mut_printf(MUT_LOG_TEST_STATUS,
                       "%s rdgen status is %d test_index: %d\n", __FUNCTION__, rdgen_status, test_index );
        }

        /* Destroy the test objects. */
        destroy_status = fbe_api_rdgen_test_context_destroy(context_p);
        if (destroy_status != FBE_STATUS_OK)
        {
            mut_printf(MUT_LOG_TEST_STATUS,
                       "%s destroy failed with status %d\n", __FUNCTION__,  destroy_status);
            status = destroy_status;
        }

        /* Goto next test context */
        context_p++;
    }

    /* Validate that no I/Os are stuck.
     */
    if (status == FBE_STATUS_OK)
    {
        status = fbe_test_sep_rebuild_validate_no_quiesced_io(rg_config_p, raid_group_count);
    }

    return status;
}
/**************************************
 * end fbe_test_sep_io_run_rdgen_tests
 **************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_get_user_rdgen_peer_options()
 *****************************************************************************
 * @brief
 *  Fetch the user specified peer options
 *
 * @param   b_is_dualsp_enabled - FBE_TRUE the configuration allows peer I/O
 * @param   rdgen_peer_options_p - Pointer to original peer options and possibly
 *              updated.             
 *
 * @return  status - FBE_STATUS_OK - Parsed user rdgne peer options
 *                   Otherwise pper I/O isn't allowed
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_get_user_rdgen_peer_options(fbe_bool_t b_is_dualsp_enabled,
                                                         fbe_api_rdgen_peer_options_t *rdgen_peer_options_p)
{
    fbe_api_rdgen_peer_options_t original_rdgen_peer_options = *rdgen_peer_options_p;
    fbe_api_rdgen_peer_options_t new_rdgen_peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;
    char *value = mut_get_user_option_value("-rdgen_peer_option");

    /* If there is no value specified simply return
     */
    if (value == NULL)
    {
        return FBE_STATUS_OK;
    }
    else if (b_is_dualsp_enabled == FBE_FALSE)
    {
        /* Else if dual sp isn't enabled, report the error but return success
         */
        mut_printf(MUT_LOG_TEST_STATUS, "rdgen peer option: peer option specified but peer I/O not enabled");
        return FBE_STATUS_OK;
    }

    /* Parse the option value
     */
    if (!strcmp(value, "LOAD_BALANCE"))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "rdgen peer option set to load balance");
        new_rdgen_peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
    }
    else if (!strcmp(value, "RANDOM"))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "rdgen peer option set to random");
        new_rdgen_peer_options = FBE_API_RDGEN_PEER_OPTIONS_RANDOM;
    }
    else if (!strcmp(value, "PEER_ONLY"))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "rdgen peer option set to send through Peer");
        new_rdgen_peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
    }
    else
    {
        mut_printf(MUT_LOG_TEST_STATUS, "-rdgen_peer_option unknown option %s", value);
        MUT_FAIL_MSG("unknown -rdgen_peer_option provided");
    }
    
    /* Simply report the fact that the options are being changed
     */
    *rdgen_peer_options_p = new_rdgen_peer_options;
    if (new_rdgen_peer_options != original_rdgen_peer_options)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "rdgen peer option changed from: %d to %d",
                   original_rdgen_peer_options, new_rdgen_peer_options);
    }

    return FBE_STATUS_OK;
}
/***************************************************
 * end fbe_test_sep_io_get_user_rdgen_peer_options()
 ***************************************************/

/*!***************************************************************************
 * fbe_test_sep_io_set_rdgen_peer_options()
 *****************************************************************************
 * @brief
 *  Set the rdgen peer options for the test sequence
 *
 * @param   b_is_dualsp_enabled   - FBE_TRUE - This test can run dualsp 
 * @param   rdgen_peer_options - The rdgne peer options ot set     
 *
 * @return  status - FBE_STATUS_OK - Peer options updated
 *                   Otherwise - Peer options not valid for single sp setup
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_set_rdgen_peer_options(fbe_bool_t b_is_dualsp_enabled,
                                                    fbe_api_rdgen_peer_options_t rdgen_peer_options)
{
    /* If dualsp I/O mode isn't allowed, simply set the peer options to invalid.
     */
    if (b_is_dualsp_enabled == FBE_FALSE)
    {
        fbe_test_sep_io_current_peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;
        return FBE_STATUS_OK;
    }

    /* Simply set the global peer options
     */
    fbe_test_sep_io_current_peer_options = rdgen_peer_options;
    mut_printf(MUT_LOG_TEST_STATUS, "%s Setting rdgen peer options to: %d ",
               __FUNCTION__, rdgen_peer_options);

    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_test_sep_io_set_rdgen_peer_options()
 **********************************************/

/*!***************************************************************************
 * fbe_test_sep_io_get_configured_rdgen_peer_options()
 *****************************************************************************
 * @brief
 *  Get the configured rdgen peer options for the test sequence
 *
 * @param   None
 *
 * @return  currently configured rdgen peer options
 *
 *****************************************************************************/
fbe_api_rdgen_peer_options_t fbe_test_sep_io_get_configured_rdgen_peer_options(void)
{
    return(fbe_test_sep_io_current_peer_options);
}
/*********************************************************
 * end fbe_test_sep_io_get_configured_rdgen_peer_options()
 *********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_configure_dualsp_io_mode()
 *****************************************************************************
 *
 * @brief   Configure the dual-sp I/O mode based on the requested mode and any
 *          command line over-ride.
 *
 * @param   b_is_dualsp_enabled   - FBE_TRUE - This test can run dualsp 
 * @param   requested_dualsp_mode - Dual sp I/O mode requested     
 *
 * @return  status - FBE_STATUS_OK - Peer options updated
 *                   Otherwise - Peer options not valid for single sp setup
 *
 *****************************************************************************/

fbe_status_t fbe_test_sep_io_configure_dualsp_io_mode(fbe_bool_t b_is_dualsp_enabled,
                                                      fbe_test_sep_io_dual_sp_mode_t requested_dualsp_mode)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_rdgen_peer_options_t    rdgen_peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;

    /* It is valid to invoke this method w/o dualsp mode enabled
     */
    if (b_is_dualsp_enabled != FBE_TRUE)
    {
        /* Set the peer options to invalid
         */
        status = fbe_test_sep_io_set_rdgen_peer_options(b_is_dualsp_enabled,
                                                        rdgen_peer_options);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        return status;
    }

    /* Based on the selected mode configure the rdgen peer options
     */
    switch(requested_dualsp_mode)
    {
        case FBE_TEST_SEP_IO_DUAL_SP_MODE_LOCAL_ONLY:
            /* Invalid means don't send any I/O thru peer */
            rdgen_peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;
            break;

        case FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE:
            rdgen_peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
            break;

        case FBE_TEST_SEP_IO_DUAL_SP_MODE_PEER_ONLY:
            rdgen_peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
            break;

        case FBE_TEST_SEP_IO_DUAL_SP_MODE_RANDOM:
            rdgen_peer_options = FBE_API_RDGEN_PEER_OPTIONS_RANDOM;
            break;

        default:
            /* Unsupported peer I/O mode
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s Unsupported peer I/O mode: %d", 
                       __FUNCTION__, requested_dualsp_mode); 
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Override the rdgen peer I/O mode if specified on command line
     */
    status = fbe_test_sep_io_get_user_rdgen_peer_options(b_is_dualsp_enabled,
                                                         &rdgen_peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Update the global that contains the peer I/O options
     */
    status = fbe_test_sep_io_set_rdgen_peer_options(b_is_dualsp_enabled,
                                                    rdgen_peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return status;
}
/************************************************
 * end fbe_test_sep_io_configure_dualsp_io_mode()
 ************************************************/

/*!***************************************************************************
 * fbe_test_sep_io_get_user_rdgen_abort_msecs()
 *****************************************************************************
 * @brief
 *  Determine if rdgen random aborts should be enabled
 *
 * @param   b_is_abort_enabled - Current value of aborts being enabled
 * @param   rdgen_peer_options_p - Pointer to originsal and updated 
 *              rdgen peer options        
 *
 * @return  status - FBE_STATUS_OK - Peer options updated
 *                   Otherwise - Peer options not valid for single sp setup
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_get_user_rdgen_abort_msecs(fbe_bool_t b_is_abort_enabled,
                                                        fbe_test_random_abort_msec_t *rdgen_abort_msecs_p)
{
    fbe_test_random_abort_msec_t original_rdgen_abort_msecs = *rdgen_abort_msecs_p;
    fbe_test_random_abort_msec_t new_rdgen_abort_msecs  = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    char *value = mut_get_user_option_value("-rdgen_abort_msecs");

    /* If no value specified simply return
     */
    if (value == NULL)
    {
        return FBE_STATUS_OK;
    }
    else if (b_is_abort_enabled == FBE_FALSE)
    {
        /* Else if abort isn't enabled, report the error but return success
         */
        mut_printf(MUT_LOG_TEST_STATUS, "rdgen abort msecs: random abort injection isn't enabled");
        return FBE_STATUS_OK;
    }

    new_rdgen_abort_msecs = atoi(value);
    mut_printf(MUT_LOG_TEST_STATUS, "rdgen abort msecs: Setting abort msecs to: 0x%x",
               new_rdgen_abort_msecs);
    *rdgen_abort_msecs_p = new_rdgen_abort_msecs;

    if (new_rdgen_abort_msecs != original_rdgen_abort_msecs)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "rdgen abort msecs: Changed abort msecs from: 0x%x to: 0x%x",
                   original_rdgen_abort_msecs, new_rdgen_abort_msecs);

    }
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_test_sep_io_get_user_rdgen_abort_msecs()
 **************************************************/

/*!***************************************************************************
 * fbe_test_sep_io_set_rdgen_abort_msecs()
 *****************************************************************************
 * @brief
 *  Set the rdgen abot msecs for the test sequence
 *
 * @param   b_is_abort_enabled - Is random abort injection enabled or not 
 * @param   rdgen_abort_msecs - Abort msecs to set 
 *
 * @return  status - FBE_STATUS_OK 
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_set_rdgen_abort_msecs(fbe_bool_t b_is_abort_enabled,
                                                   fbe_test_random_abort_msec_t rdgen_abort_msecs)
{
    /* If abort injection isn't allowed, simply set the peer options to invalid.
     */
    if (b_is_abort_enabled == FBE_FALSE)
    {
        fbe_test_sep_io_current_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
        return FBE_STATUS_OK;
    }

    /* Simply set the global abort msecs
     */
    fbe_test_sep_io_current_abort_msecs = rdgen_abort_msecs;
    mut_printf(MUT_LOG_TEST_STATUS, "%s Setting rdgen abort msecs to: 0x%x ",
               __FUNCTION__, rdgen_abort_msecs);

    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_test_sep_io_set_rdgen_abort_msecs()
 **********************************************/

/*!***************************************************************************
 * fbe_test_sep_io_get_configured_rdgen_abort_msecs()
 *****************************************************************************
 * @brief
 *  Get the configured rdgen abort msecs for the test sequence
 *
 * @param   None
 *
 * @return  currently configured rdgen abort msecs
 *
 *****************************************************************************/
fbe_test_random_abort_msec_t fbe_test_sep_io_get_configured_rdgen_abort_msecs(void)
{
    return(fbe_test_sep_io_current_abort_msecs);
}
/*********************************************************
 * end fbe_test_sep_io_get_configured_rdgen_abort_msecs()
 *********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_configure_abort_mode()
 *****************************************************************************
 *
 * @brief   Configure the abort I/O mode based on the requested mode and any
 *          command line over-ride.
 *
 * @param   b_is_abort_enabled   - FBE_TRUE - This test requested to run aborts
 * @param   rdgen_abort_msecs - Value for rdgen abort msecs    
 *
 * @return  status - FBE_STATUS_OK - abot msecs updated
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_configure_abort_mode(fbe_bool_t b_is_abort_enabled,
                                                  fbe_test_random_abort_msec_t rdgen_abort_msecs)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* It is valid to invoke this method w/o abort msecs enabled
     */
    if (b_is_abort_enabled != FBE_TRUE)
    {
        /* Set the abort msecs to invalid
         */
        status = fbe_test_sep_io_set_rdgen_abort_msecs(b_is_abort_enabled,
                                                       rdgen_abort_msecs);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        return status;
    }

    /* Override the rdgen random abort msecs if specified on command line
     */
    status = fbe_test_sep_io_get_user_rdgen_abort_msecs(b_is_abort_enabled,
                                                        &rdgen_abort_msecs);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Update the global that contains the abort msecs
     */
    status = fbe_test_sep_io_set_rdgen_abort_msecs(b_is_abort_enabled,
                                                   rdgen_abort_msecs);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return status;
}
/************************************************
 * end fbe_test_sep_io_configure_abort_mode()
 ************************************************/

/*!***************************************************************************
 * fbe_test_sep_io_validate_verify_error_counts()
 *****************************************************************************
 * @brief
 *  Validate that zero (0) errors are reported for the last rdgen run
 * 
 * @param context_p - context that ran
 * 
 * @return None.   
 *
 * @author
 *  03/24/2011  Ron Proulx  - Created
 *
 *****************************************************************************/

void fbe_test_sep_io_validate_verify_error_counts(fbe_api_rdgen_context_t *context_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       counter;
    fbe_u32_t      *counter_p = NULL;

    /* First scan the entire structure for a non-zero value.
     */
    for(counter = 0, counter_p = (fbe_u32_t *)&context_p->start_io.statistics.verify_errors;
        counter < (sizeof(fbe_raid_verify_error_counts_t) / sizeof(fbe_u32_t)); 
        counter++)
    {
        if (*counter_p != 0 &&
			counter_p != &context_p->start_io.statistics.verify_errors.retryable_errors)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s verify counter: 0x%p contents: %d non-zero", 
                       __FUNCTION__, counter_p, *counter_p);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
        counter_p++;
    }

    /* If a non-zero value was found determine the first counter
     */
    if (status != FBE_STATUS_OK)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.u_crc_count, 0);       
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.c_crc_count, 0);    
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.u_crc_multi_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.c_crc_multi_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.u_crc_single_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.c_crc_single_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.u_coh_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.c_coh_count, 0);    
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.u_ts_count, 0);    
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.c_ts_count, 0);     
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.u_ws_count, 0);     
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.c_ws_count, 0);     
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.u_ss_count, 0);     
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.c_ss_count, 0);     
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.u_media_count, 0);     
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.c_media_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.c_soft_media_count, 0);
        /* We could get retryable errors during PVD quiescing. */
        //MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.retryable_errors, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.non_retryable_errors, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.shutdown_errors, 0);
        
        /* If the counter wasn't found it should be added to the list
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s verify counter: 0x%p contents: %d needs to be added above!!", 
                   __FUNCTION__, counter_p, *counter_p);  
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return;
}
/****************************************************
 * end fbe_test_sep_io_validate_verify_error_counts()
 ****************************************************/

/*!**************************************************************
 * fbe_test_sep_io_validate_status()
 ****************************************************************
 * @brief
 *  Check the status for running rdgen.
 * 
 * @param status - Status of the rdgen operation
 * @param context_p - context to execute.
 * @param b_is_random_abort_enabled - Are random aborts enabled
 * 
 * @return None.   
 *
 * @author
 *  11/3/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_io_validate_status(fbe_status_t status,
                                     fbe_api_rdgen_context_t *context_p,
                                     fbe_bool_t b_is_random_abort_enabled)
{
    /* If we did not run an abort test we should not have errors.
     */
    if (b_is_random_abort_enabled == FBE_FALSE)
    {
        /* We expect success.
         */
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

        /* If background operations are enabled do not check the error report.
         */
        if (fbe_test_sep_io_is_background_ops_enabled() == FBE_FALSE)
        {
            fbe_test_sep_io_validate_verify_error_counts(context_p);
        }
    }
    else
    {
        /* When we run an abort test we know that we will get errors.
         */
        if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_GENERIC_FAILURE)) 
        { 
            mut_printf(MUT_LOG_TEST_STATUS, "%s status 0x%x unexpected\n", __FUNCTION__, status);
            MUT_FAIL_MSG("Unexpected error during abort testing.");
        }

		/* We may split I/O at LUN level and error_count will be greater than 
         * aborted_error_count.
         */
        MUT_ASSERT_TRUE(context_p->start_io.statistics.error_count >= context_p->start_io.statistics.aborted_error_count);
    }
    return;
}
/**************************************
 * end fbe_test_sep_io_validate_status()
 **************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_allocate_rdgen_test_context_for_all_luns()
 *****************************************************************************
 *
 * @brief   Some types of I/O require a separate rdgen test context for each
 *          object.  Allocate a contiguous piece of memory for all luns.
 *
 * @param   context_pp - Address of pointer to update with test context location
 * @param   num_tests_contexts - number test contexts to allocate
 * 
 * @return fbe_status_t
 *
 * @note    Caterpillar to all objects of a class is not supported.
 *
 * @author
 *  03/31/2011  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_allocate_rdgen_test_context_for_all_luns(fbe_api_rdgen_context_t **context_pp,
                                                                      fbe_u32_t num_tests_contexts)
{
    fbe_api_rdgen_context_t     *context_p = NULL;

    /* If there are no luns return a failure
     */
    if (num_tests_contexts == 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "allocate for all luns: no luns found");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Else allocate the memory
     *
     *  @note API allocate memory also zeros the memory
     */
    context_p = (fbe_api_rdgen_context_t *)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_context_t) * num_tests_contexts);
    if (context_p == NULL)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "allocate for all luns: failed to allocate context for: %d contexts",
                   num_tests_contexts);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Update the passed pointer to context and number of luns found
     */
    *context_pp = context_p;

    /* Return success
     */
    return FBE_STATUS_OK;
}
/***************************************************************
 * end fbe_test_sep_io_allocate_rdgen_test_context_for_all_luns()
 ***************************************************************/

/*!***************************************************************************
 * fbe_test_sep_io_get_num_luns()
 *****************************************************************************
 * @brief Determine how many LUNs thre are for this raid group config. 
 *
 * @param rg_config_p - Pointer to array of raid group configurations to get
 *              lun count from.
 * @param num_luns_p - Pointer to number of luns.
 * 
 * @return fbe_status_t
 *
 * @author
 *  9/19/2011 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_get_num_luns(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t *num_luns_p)
{
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_u32_t                   total_num_luns = 0;

    /* Get the number of raid groups to run I/O against
     */
    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Iterate over all the raid groups
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            /* Simply add the number of luns in this raid group
             */
            total_num_luns += current_rg_config_p->number_of_luns_per_rg;
        }
        /* Goto next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups configured */
    *num_luns_p = total_num_luns;

    /* Return success
     */
    return FBE_STATUS_OK;
}
/***************************************************************
 * end fbe_test_sep_io_get_num_luns()
 ***************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_allocate_rdgen_test_context_for_all_luns()
 *****************************************************************************
 *
 * @brief   Free a contiguous piece of memory for all luns.
 *
 * @param   context_p - Address of pointer to array of tests contexts to free
 * @param   num_tests_contexts - Number of tests contexts (for sanity check)
 * 
 * @return fbe_status_t
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_free_rdgen_test_context_for_all_luns(fbe_api_rdgen_context_t **context_pp,
                                                                  fbe_u32_t num_tests_contexts)
{
    /* If there are no luns return a failure
     */
    if (num_tests_contexts == 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "free for all luns: rg_config_p:no luns found");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Free the memory and release the handle to it
     */
    fbe_api_free_memory((void *)*context_pp);
    *context_pp = NULL;

    /* Return success
     */
    return FBE_STATUS_OK;

}
/************************************************************
 * end fbe_test_sep_io_free_rdgen_test_context_for_all_luns()
 ************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_setup_rdgen_test_context_for_all_luns()
 *****************************************************************************
 *
 * @brief   Run the rdgen test setup method passed against all luns for all
 *          raid groups specified.
 *
 * @param   rg_config_p - Pointer to array of raid groups configurations to run
 *              I/O against (all luns in each raid group)
 * @param   context_p - Pointer to array of previously allocated test contexts
 * @param   num_contexts - Size of context array supplied
 * @param   rdgen_operation - operation to start.
 * @param   max_passes - Number of passes to execute.
 * @param   io_size_blocks - io size to fill with in blocks.
 * @param   b_random - FBE_TRUE - random operations, otherwise sequential
 * @param   b_increasing - FBE_TRUE - Increasing sequential requests
 *                       FBE_FALSE - Decreasing sequential requests
 * @param   b_inject_random_aborts - FBE_TRUE - Inject random aborts using the
 *              configured random abort msecs value
 * @param   b_dualsp_io - Run dualsp I/O using the previously configured peer
 *              options
 * 
 * @return  fbe_status_t
 *
 * @note    Capacity is the entire capacity of each lun
 *
 * @author
 *  04/01/2011  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_setup_rdgen_test_context_for_all_luns(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_api_rdgen_context_t *context_p,
                                             fbe_u32_t num_contexts,
                                             fbe_rdgen_operation_t rdgen_operation,
                                             fbe_u32_t max_passes,
                                             fbe_u32_t threads,
                                             fbe_u64_t io_size_blocks,
                                             fbe_bool_t b_random,
                                             fbe_bool_t b_increasing,
                                             fbe_bool_t b_inject_random_aborts,
                                             fbe_bool_t b_dualsp_io,
                                             fbe_status_t operation_setup_fn(fbe_api_rdgen_context_t *context_p,
                                                                             fbe_object_id_t object_id,
                                                                             fbe_rdgen_operation_t rdgen_operation,
                                                                             fbe_lba_t capacity,
                                                                             fbe_u32_t max_passes,
                                                                             fbe_u32_t threads,
                                                                             fbe_u64_t io_size_blocks,
                                                                             fbe_bool_t b_random,
                                                                             fbe_bool_t b_increasing,
                                                                             fbe_bool_t b_inject_random_aborts,
                                                                             fbe_bool_t b_dualsp_io)  )
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_api_rdgen_context_t     *current_context_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_u32_t                   lun_index = 0;
    fbe_object_id_t             lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                   total_luns_processed = 0;

    /* Get the number of raid groups to run I/O against
     */
    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Iterate over all the raid groups
     */
    current_rg_config_p = rg_config_p;
    current_context_p = context_p;
    for (rg_index = 0; 
         (rg_index < rg_count) && (status == FBE_STATUS_OK); 
         rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            /* For this raid group iterate over all the luns
             */
            for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++)
            {
                fbe_lun_number_t lun_number = current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number;

                /* Validate that we haven't exceeded the number of context available.
                 */
                if (total_luns_processed >= num_contexts) 
                {
                    mut_printf(MUT_LOG_TEST_STATUS, 
                               "setup context for all luns: rg_config_p: 0x%p luns processed: %d exceeds contexts: %d",
                               rg_config_p, total_luns_processed, num_contexts); 
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* Get the lun object id from the lun number
                 */
                MUT_ASSERT_INT_EQUAL(current_rg_config_p->logical_unit_configuration_list[lun_index].raid_group_id, 
                                     current_rg_config_p->raid_group_id);
                status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                /* Now simply run setup context supplied
                 */
                status = operation_setup_fn(current_context_p,
                                            lun_object_id,
                                            rdgen_operation,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            max_passes,
                                            threads,
                                            io_size_blocks,
                                            b_random,
                                            b_increasing,
                                            b_inject_random_aborts,
                                            b_dualsp_io);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                /* Increment the number of luns processed and the test context pointer
                 */
                total_luns_processed++;
                current_context_p++;

            } /* end for all luns in a raid group */
        }
        /* Goto next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups configured */

    /* Else return success
     */
    return FBE_STATUS_OK;
}
/***************************************************************
 * end of fbe_test_sep_io_setup_rdgen_test_context_for_all_luns() 
 ***************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_run_verify_on_lun()
 *****************************************************************************
 *
 * @brief   Run a verify on the lun specified by the object id.
 *
 * @param   lun_object_id - The object id of the lun to verify
 * @param   b_verify_entire_lun - FBE_TRUE - Verify the entire lun range
 * @param   start_lba - Starting lba of lun to start verify at
 * @param   blocks - Number of blocks to verify at starting lba
 * 
 * @return  status - Determine if test passed or not
 *
 * @author
 *  03/25/2011  Ron Proulx  - Created 
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_run_verify_on_lun(fbe_object_id_t lun_object_id,
                                               fbe_bool_t b_verify_entire_lun,
                                               fbe_lba_t start_lba,
                                               fbe_block_count_t blocks)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_time_t                  start_time_for_sp;
    fbe_time_t                  elapsed_time_for_sp;
    fbe_transport_connection_target_t target_invoked_on = FBE_TRANSPORT_SP_A;
    fbe_transport_connection_target_t peer_target = FBE_TRANSPORT_SP_B;
    fbe_sim_transport_connection_target_t active_target = FBE_SIM_SP_A;
    fbe_u32_t                   pass_count = 0;
    fbe_lun_verify_report_t     verify_report;
    fbe_lun_verify_report_t    *verify_report_p = &verify_report;
    fbe_lba_t                   verify_start_lba = start_lba;
    fbe_block_count_t           verify_blocks = blocks;
    fbe_u32_t                   reported_error_count = 0;

    /* If `verify the entire lun' is set, setup verify_start lba and verify_blocks.
     */
    if (b_verify_entire_lun == FBE_TRUE)
    {
        verify_start_lba = FBE_LUN_VERIFY_START_LBA_LUN_START;
        verify_blocks = FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN;
    }

    /* First get the sp id this was invoked on.
     */
    target_invoked_on = fbe_api_transport_get_target_server();
    peer_target = (target_invoked_on == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;

    /* Determine the `active' (i.e. this is for debug purposes only since
     * the `active' sp could vary by object).
     */
    fbe_test_sep_get_active_target_id(&active_target);

    mut_printf(MUT_LOG_TEST_STATUS, 
               "run verify on lun: Start verify for obj: 0x%x local sp: %d active sp: %d peer sp: %d", 
               lun_object_id, target_invoked_on, active_target, peer_target);

    /* Initialize the verify report data
     */
    status = fbe_test_sep_util_lun_clear_verify_report(lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_zero_memory((void *)verify_report_p, sizeof(*verify_report_p));

    /* Now initiate a verify for the entire capacity of the lun
     */
    start_time_for_sp = fbe_get_time();
    fbe_test_sep_util_lun_initiate_verify(lun_object_id, 
                                         FBE_LUN_VERIFY_TYPE_USER_RW,
                                         b_verify_entire_lun, 
                                         verify_start_lba, /* Verify the range requested */
                                         verify_blocks);

   /* Wait for the verify to finish on this LUN
    */  
   fbe_test_sep_util_wait_for_lun_verify_completion(lun_object_id, verify_report_p, pass_count);

   /* Validate that there were 0 errors found
    */
   fbe_test_sep_util_sum_verify_results_data(&verify_report_p->totals, &reported_error_count);
   MUT_ASSERT_TRUE(reported_error_count == 0);

   /* Get the elapsed time
    */
   elapsed_time_for_sp = fbe_get_time() - start_time_for_sp;
   target_invoked_on = fbe_api_transport_get_target_server();
   mut_printf(MUT_LOG_TEST_STATUS, 
               "run verify on lun: for obj: 0x%x local sp: %d took: %llu ms with status: 0x%x", 
               lun_object_id, target_invoked_on,
	       (unsigned long long)elapsed_time_for_sp, status);

   return status;
}
/********************************************
 * end of fbe_test_sep_io_run_verify_on_lun() 
 ********************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_run_verify_on_all_luns()
 *****************************************************************************
 *
 * @brief   Run a verify to all luns in all configured raid groups.
 *
 * @param   rg_config_p - Pointer to array of raid groups with the luns to run
 *              run verify against.
 * @param   b_verify_entire_lun - FBE_TRUE - Verify the entire lun range
 * @param   start_lba - Starting lba of lun to start verify at
 * @param   blocks - Number of blocks to verify at starting lba
 * 
 * @return  status - Determine if test passed or not
 *
 * @author
 *  03/25/2011  Ron Proulx  - Created 
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_run_verify_on_all_luns(fbe_test_rg_configuration_t *rg_config_p,
                                                    fbe_bool_t b_verify_entire_lun,
                                                    fbe_lba_t start_lba,
                                                    fbe_block_count_t blocks)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_u32_t                   lun_index = 0;
    fbe_object_id_t             lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_time_t                  start_time_for_sp;
    fbe_time_t                  elapsed_time_for_sp;
    fbe_transport_connection_target_t target_invoked_on = FBE_TRANSPORT_SP_A;
    fbe_transport_connection_target_t peer_target = FBE_TRANSPORT_SP_B;
    fbe_sim_transport_connection_target_t active_target = FBE_SIM_SP_A;
    fbe_u32_t                   pass_count = 0;
    fbe_lun_verify_report_t     verify_report;
    fbe_lun_verify_report_t    *verify_report_p = &verify_report;
    fbe_lba_t                   verify_start_lba = start_lba;
    fbe_block_count_t           verify_blocks = blocks;
    fbe_u32_t                   reported_error_count = 0;

    /* If `verify the entire lun' is set, setup verify_start lba and verify_blocks.
     */
    if (b_verify_entire_lun == FBE_TRUE)
    {
        verify_start_lba = FBE_LUN_VERIFY_START_LBA_LUN_START;
        verify_blocks = FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN;
    }

    /* First get the sp id this was invoked on.
     */
    target_invoked_on = fbe_api_transport_get_target_server();
    peer_target = (target_invoked_on == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;

    /* Determine the `active' (i.e. this is for debug purposes only since
     * the `active' sp could vary by object).
     */
    fbe_test_sep_get_active_target_id(&active_target);

    mut_printf(MUT_LOG_TEST_STATUS, 
               "run verify on all luns: Start verify for local sp: %d active sp: %d peer sp: %d", 
               target_invoked_on, active_target, peer_target);

    /* Get the number of raid groups to run I/O against
     */
    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Iterate over all the raid groups
     */
    current_rg_config_p = rg_config_p;
    start_time_for_sp = fbe_get_time();
    for (rg_index = 0; 
         (rg_index < rg_count) && (status == FBE_STATUS_OK); 
         rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            /* For this raid group iterate over all the luns
             */
            for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++)
            {
                fbe_lun_number_t lun_number = current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number;

                /* Get the lun object id from the lun number
                 */
                MUT_ASSERT_INT_EQUAL(current_rg_config_p->logical_unit_configuration_list[lun_index].raid_group_id, 
                                     current_rg_config_p->raid_group_id);
                status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                /* Initialize the verify report data
                 */
                fbe_test_sep_util_lun_clear_verify_report(lun_object_id);
                fbe_zero_memory((void *)verify_report_p, sizeof(*verify_report_p));

                /* Now initiate a verify for the entire capacity of the lun
                 */
                fbe_test_sep_util_lun_initiate_verify(lun_object_id, 
                                                      FBE_LUN_VERIFY_TYPE_USER_RW,
                                                      b_verify_entire_lun, 
                                                      verify_start_lba, /* Verify the range requested */
                                                      verify_blocks);

                /* Wait for the verify to finish on this LUN
                 */
                fbe_test_sep_util_wait_for_lun_verify_completion(lun_object_id, verify_report_p, pass_count);

                /* Validate that there were 0 errors found
                 */
                fbe_test_sep_util_sum_verify_results_data(&verify_report_p->totals, &reported_error_count);
                MUT_ASSERT_TRUE(reported_error_count == 0);
			    mut_printf(MUT_LOG_TEST_STATUS, "LUN %x verify complete \n", lun_object_id);


            } /* end for all luns in a raid group */
        }
        /* Goto next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups configured */

    /*! Now report status
     */
    elapsed_time_for_sp = fbe_get_time() - start_time_for_sp;
    target_invoked_on = fbe_api_transport_get_target_server();
    mut_printf(MUT_LOG_TEST_STATUS, 
               "run verify on all luns: for local sp: %d took: %llu ms with status: 0x%x", 
               target_invoked_on, (unsigned long long)elapsed_time_for_sp,
	       status);

    return status;
}
/***********************************************
 * end fbe_test_sep_io_run_verify_on_all_luns()
 ***********************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_wait_for_all_luns_ready()
 *****************************************************************************
 *
 * @brief   Wait for all luns to be ready before starting I/O.
 *
 * @param   rg_config_p - Pointer to array of raid groups with the luns to run
 *              run I/O against.  This method validates that all the luns are
 *              in the ready state.
 * @param   b_run_dual_sp - FBE_TRUE - Run non-prefill I/O in dual SP mode
 *                          FBE_FALSE - Run all I/O on this SP (single SP mode)
 * 
 * @return  status - Determine if test passed or not
 *
 * @note    Although rdgen could validate the state of the objects (in this case
 *          luns) and wait for them to be READY that would be wrong.  This is
 *          because RDGEN simulates an actual host and therefore doesn't `wait'
 *          for the LUNs to enter a certain state.
 *
 * @author
 *  03/24/2011  Ron Proulx  - Created 
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_io_wait_for_all_luns_ready(fbe_test_rg_configuration_t *rg_config_p,
                                                            fbe_bool_t b_run_dual_sp)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_time_t                  start_time_for_sp;
    fbe_time_t                  elapsed_time_for_sp;
    fbe_transport_connection_target_t target_invoked_on = FBE_TRANSPORT_SP_A;
    fbe_transport_connection_target_t peer_target = FBE_TRANSPORT_SP_B;

    /* First get the sp id this was invoked on.
     */
    target_invoked_on = fbe_api_transport_get_target_server();

    /* Determine the peer sp (for debug)
     */
    peer_target = (target_invoked_on == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A; 

    /* Get the number of raid groups to run I/O against
     */
    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    /* First iterate over the raid groups on `this' sp. Then if neccessary iterate
     * over the raid groups on the other sp.
     */
    current_rg_config_p = rg_config_p;
    start_time_for_sp = fbe_get_time();
    for (rg_index = 0; 
         (rg_index < rg_count) && (status == FBE_STATUS_OK); 
         rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            /* Wait for all lun objects of this raid group to be ready
             */
            status = fbe_test_sep_util_wait_for_all_lun_objects_ready(current_rg_config_p,
                                                                      target_invoked_on);
            if (status != FBE_STATUS_OK)
            {
                /* Failure, break out and return the failure.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "wait for all luns ready: raid_group_id: 0x%x failed to come ready", 
                           current_rg_config_p->raid_group_id);
                break;
            }
        }

        /* Goto next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups configured */
    elapsed_time_for_sp = fbe_get_time() - start_time_for_sp;
    mut_printf(MUT_LOG_TEST_STATUS, 
               "wait for all luns ready: for local sp: %d took: %llu ms with status: 0x%x", 
               target_invoked_on, (unsigned long long)elapsed_time_for_sp,
	       status);

    /* Now validate the lun state on the other sp if required
     */
    if ((b_run_dual_sp == FBE_TRUE) &&
        (status == FBE_STATUS_OK)      )
    {
        /* Reset time so that we know how long each sp took
         */
        current_rg_config_p = rg_config_p;
        start_time_for_sp = fbe_get_time();
        for (rg_index = 0; 
             (rg_index < rg_count) && (status == FBE_STATUS_OK); 
             rg_index++)
        {
            /* Wait for all lun objects of this raid group to be ready
             */
            if (fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                status = fbe_test_sep_util_wait_for_all_lun_objects_ready(current_rg_config_p,
                                                                          peer_target);
                if (status != FBE_STATUS_OK)
                {
                    /* Failure, break out and return the failure.
                     */
                    mut_printf(MUT_LOG_TEST_STATUS, 
                               "wait for all luns ready: raid_group_id: 0x%x failed to come ready", 
                               current_rg_config_p->raid_group_id);
                    break;
                }
    
            }
            /* Goto next raid group
             */
            current_rg_config_p++;

        } /* end for all raid groups configured */
        elapsed_time_for_sp = fbe_get_time() - start_time_for_sp;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "wait for all luns ready: for peer  sp: %d took: %llu ms with status: 0x%x", 
                   peer_target, (unsigned long long)elapsed_time_for_sp, status);

    } /* if dualsp is enabled */

    return status;
}
/***********************************************
 * end fbe_test_sep_io_wait_for_all_luns_ready()
 ***********************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_init_sequence_config()
 *****************************************************************************
 *
 * @brief   Using the parameters supplied, initialize the test configuration
 *          that will be used for the duration of the test sequence.  This
 *          method also uses any command line parameters to determine if this
 *          test sequence should be skipped or not.
 *
 * @param   sequence_config_p - Pointer to test sequence configuration to
 *              populate.
 * @param   rg_config_p - Pointer to array of raid groups that contain the luns
 *              to run I/O against.  May also be used to run rebuild on (1) sp
 *              while I/O is running on the other etc.
 * @param   sep_io_sequence_type - Identifies the I/O sequence to run
 * @param   max_blocks_for_small_io - Maximum blocks per small I/O request
 * @param   max_blocks_for_large_io - Maximum blocks per large I/O request
 * @param   b_inject_random_aborts - FBE_TRUE - Use random_abort_time_msecs
 *              to determine and abort random requests
 *                                   FBE_FALSE - Don't inject aborts
 * @param   random_abort_time_msecs - Random time to abort in milliseconds 
 * @param   b_run_dual_sp - FBE_TRUE - Run non-prefill I/O in dual SP mode
 *                          FBE_FALSE - Run all I/O on this SP (single SP mode)
 * 
 * @return  status - FBE_STATUS_OK - Test sequence is initialized and ready to
 *                      proceed.
 *                   FBE_STATUS_EDGE_NOT_ENABLED - This test is disabled due to
 *                      the command line parameter specified.
 *                   other - Unexpected error
 *
 * @author
 *  03/29/2011  Ron Proulx  - Created 
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_io_init_sequence_config(fbe_test_sep_io_sequence_config_t *sequence_config_p,
                                                         fbe_test_rg_configuration_t *rg_config_p,
                                                         fbe_test_sep_io_sequence_type_t sep_io_sequence_type,
                                                         fbe_block_count_t max_blocks_for_small_io,
                                                         fbe_block_count_t max_blocks_for_large_io, 
                                                         fbe_bool_t b_inject_random_aborts, 
                                                         fbe_test_random_abort_msec_t random_abort_time_msecs,
                                                         fbe_bool_t b_run_dual_sp,
                                                         fbe_test_sep_io_dual_sp_mode_t dual_sp_mode)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_bool_t      b_run_abort_cases_only = fbe_test_sep_util_get_cmd_option("-abort_cases_only");

    /* Now populate the fields that cannot be changed from the command line
     */
    sequence_config_p->rg_config_p = rg_config_p;
    sequence_config_p->sequence_type = sep_io_sequence_type;
    sequence_config_p->sequence_state = FBE_TEST_SEP_IO_SEQUENCE_STATE_INVALID;
    sequence_config_p->max_blocks_for_small_io = max_blocks_for_small_io;
    sequence_config_p->max_blocks_for_large_io = max_blocks_for_large_io;
    sequence_config_p->abort_enabled = b_inject_random_aborts;
    sequence_config_p->dualsp_enabled = b_run_dual_sp;
    sequence_config_p->b_should_background_ops_run = fbe_test_sep_io_is_background_ops_enabled();
    sequence_config_p->background_ops = fbe_test_sep_io_get_background_ops();
    sequence_config_p->b_have_background_ops_run = FBE_FALSE;

    /* Now configure the fields that can be overridden from the command line.
     */
    if (sep_io_sequence_type == FBE_TEST_SEP_IO_SEQUENCE_TYPE_CATERPILLAR)
    {
        /* Each pass in a caterpillar goes over the entire range of the LUN. 
         * So we limit these I/Os appropriately. 
         */
        sequence_config_p->small_io_count = fbe_test_sep_io_caterpillar_get_small_io_count();
        sequence_config_p->large_io_count = fbe_test_sep_io_caterpillar_get_large_io_count();
    }
    else
    {
        sequence_config_p->small_io_count = fbe_test_sep_io_get_small_io_count(FBE_U32_MAX);
        sequence_config_p->large_io_count = fbe_test_sep_io_get_large_io_count(FBE_U32_MAX);
    }
    sequence_config_p->io_thread_count = fbe_test_sep_io_get_threads(FBE_U32_MAX);
    sequence_config_p->io_seconds = fbe_test_sep_io_get_io_seconds();
    status = fbe_test_sep_io_configure_dualsp_io_mode(b_run_dual_sp, dual_sp_mode);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    sequence_config_p->current_peer_options = fbe_test_sep_io_get_configured_rdgen_peer_options();
    sequence_config_p->dualsp_mode = dual_sp_mode;
    status = fbe_test_sep_io_configure_abort_mode(b_inject_random_aborts, random_abort_time_msecs);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    sequence_config_p->current_abort_msecs = fbe_test_sep_io_get_configured_rdgen_abort_msecs();
    sequence_config_p->abort_msecs = random_abort_time_msecs;

    /* Update the sequence I/O state
     */
    status = fbe_test_sep_io_set_sequence_io_state(sequence_config_p,
                                                   FBE_TEST_SEP_IO_SEQUENCE_STATE_CONFIGURED);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Now check if this test is disabled due to a command line option
     */
    if ((b_run_abort_cases_only == FBE_TRUE)  &&
        (b_inject_random_aborts == FBE_FALSE)    )
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "init sequence: sep I/O sequence type: %d aborts: %d dual-sp: %d skipped due to abort_cases_only being set", 
                   sep_io_sequence_type, b_inject_random_aborts, b_run_dual_sp);
        status = FBE_STATUS_EDGE_NOT_ENABLED;
    }

    /*! @todo Currently caterpillar is disabled for single sp tests
     */
    if ((sep_io_sequence_type == FBE_TEST_SEP_IO_SEQUENCE_TYPE_CATERPILLAR) &&
        (b_run_dual_sp == FBE_FALSE)                                           )
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "init sequence: sep I/O sequence type: %d aborts: %d dual-sp: %d currently caterpillar disabled for singlesp", 
                   sep_io_sequence_type, b_inject_random_aborts, b_run_dual_sp);
        status = FBE_STATUS_EDGE_NOT_ENABLED;
    }

    /*! @todo This currently does not work on hardware.  We need to fix this, but will work around this for now.
     */
    if ((sep_io_sequence_type == FBE_TEST_SEP_IO_SEQUENCE_TYPE_TEST_BLOCK_OPERATIONS) &&
        (fbe_test_util_is_simulation() == FBE_FALSE)                                     )
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "init sequence: sep I/O sequence type: %d aborts: %d dual-sp: %d currently test block operations doesn't run on hw", 
                   sep_io_sequence_type, b_inject_random_aborts, b_run_dual_sp);
        status = FBE_STATUS_EDGE_NOT_ENABLED;
    }

    /* Always return the status
     */
    return status;
}
/********************************************
 * end fbe_test_sep_io_init_sequence_config()
 ********************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_validate_io_ready()
 *****************************************************************************
 *
 * @brief   Validate that all the requested luns (i.e. all the luns in 
 *          the raid groups specified) on either only this sp or both sps 
 *          (dualsp mode) are ready to receive I/O.
 *
 * @param   rg_config_p - Pointer to array of raid groups to run I/O against
 *              This is used to validate that all the associated luns are in
 *              the ready state.
 * @param   sequence_config_p - Pointer to test sequence configuration
 * 
 * @return  status - Determine if test passed or not
 *
 * @author
 *  03/24/2011  Ron Proulx  - Created 
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_io_validate_io_ready(fbe_test_rg_configuration_t *rg_config_p,
                                                      fbe_test_sep_io_sequence_config_t *sequence_config_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_test_sep_io_sequence_state_t    sequence_io_state;

    /* Wait for I/O ready as required
     */
    sequence_io_state = fbe_test_sep_io_get_sequence_io_state(sequence_config_p);
    if ((sequence_io_state <  FBE_TEST_SEP_IO_SEQUENCE_STATE_IO_READY)             &&
        (sequence_io_state != FBE_TEST_SEP_IO_SEQUENCE_STATE_WAITING_FOR_IO_READY) &&
        (sequence_io_state != FBE_TEST_SEP_IO_SEQUENCE_STATE_FAILED)                   )
    {
        /* Set the state to waiting
         */
        status = fbe_test_sep_io_set_sequence_io_state(sequence_config_p,
                                                       FBE_TEST_SEP_IO_SEQUENCE_STATE_WAITING_FOR_IO_READY);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Wait for all non-private luns to become ready
         */
        status = fbe_test_sep_io_wait_for_all_luns_ready(rg_config_p,
                                                         sequence_config_p->dualsp_enabled);
        if (status != FBE_STATUS_OK)
        {
            /* If all luns failed to become ready
             */
            fbe_test_sep_io_set_sequence_io_state(sequence_config_p,
                                                  FBE_TEST_SEP_IO_SEQUENCE_STATE_FAILED);
        }
        else
        {
            /* Else set the state to I/O ready
             */
            status = fbe_test_sep_io_set_sequence_io_state(sequence_config_p,
                                                           FBE_TEST_SEP_IO_SEQUENCE_STATE_IO_READY);
        }
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return status;
}
/*****************************************
 * end fbe_test_sep_io_validate_io_ready()
 *****************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_destroy_sequence_config()
 *****************************************************************************
 *
 * @brief   At the end of each test sequence we reset the configured I/O modes
 *          and mark the I/O state and `Not Ready'.  This forces the next test
 *          sequence to re-initialize these values. 
 *
 * @param   sequence_config_p - Pointer to test sequence configuration
 * 
 * @return  status - Determine if test passed or not
 *
 * @author
 *  03/28/2011  Ron Proulx  - Created 
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_io_destroy_sequence_config(fbe_test_sep_io_sequence_config_t *sequence_config_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_test_sep_io_sequence_state_t    sequence_io_state;

    /* First set the I/O sequence state to `not configured'
     */
    sequence_io_state = fbe_test_sep_io_get_sequence_io_state(sequence_config_p);

    /* Now zero the entire structure
     */
    fbe_zero_memory((void *)sequence_config_p, sizeof(*sequence_config_p));

    /* Set the dualsp mode to the default (i.e. dualsp disabled)
     */
    status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_FALSE, FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Set the abort mode to the default (i.e. no abort injection)
     */
    status = fbe_test_sep_io_configure_abort_mode(FBE_FALSE, FBE_TEST_RANDOM_ABORT_TIME_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Update the sequence I/O state to `Invalid'
     */
    status = fbe_test_sep_io_set_sequence_io_state(sequence_config_p,
                                                   FBE_TEST_SEP_IO_SEQUENCE_STATE_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return status;
}
/***********************************************
 * end fbe_test_sep_io_destroy_sequence_config()
 ***********************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_run_test_sequence()
 *****************************************************************************
 *
 * @brief   Run an I/O sequence against all the non-private LUNs
 * 
 * @param   rg_config_p - Pointer to array of raid groups that contain the luns
 *              to run I/O against.  May also be used to run rebuild on (1) sp
 *              while I/O is running on the other etc.
 * @param   sep_io_sequence_type - Identifies the I/O sequence to run
 * @param   max_blocks_for_small_io - Maximum blocks per small I/O request
 * @param   max_blocks_for_large_io - Maximum blocks per large I/O request
 * @param   b_inject_random_aborts - FBE_TRUE - Use random_abort_time_msecs
 *              to determine and abort random requests
 *                                   FBE_FALSE - Don't inject aborts
 * @param   random_abort_time_msecs - Random time to abort in milliseconds 
 * @param   b_run_dual_sp - FBE_TRUE - Run non-prefill I/O in dual SP mode
 *                          FBE_FALSE - Run all I/O on this SP (single SP mode)
 * @param   dual_sp_mode - Dual SP mode (only used with b_run_dual_sp is true)
 * 
 * @return  None (assumes MUT ASSERTs will catch errors) 
 *
 * @author
 *  03/23/2011  Ron Proulx  - Created from elmo_run_io_test().
 *
 *****************************************************************************/

void fbe_test_sep_io_run_test_sequence(fbe_test_rg_configuration_t *rg_config_p,
                                       fbe_test_sep_io_sequence_type_t sep_io_sequence_type,
                                       fbe_block_count_t max_blocks_for_small_io,
                                       fbe_block_count_t max_blocks_for_large_io, 
                                       fbe_bool_t b_inject_random_aborts, 
                                       fbe_test_random_abort_msec_t random_abort_time_msecs,
                                       fbe_bool_t b_run_dual_sp,
                                       fbe_test_sep_io_dual_sp_mode_t dual_sp_mode)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t             context;
    fbe_api_rdgen_context_t            *context_p = &context;
    fbe_test_sep_io_sequence_state_t    sequence_io_state;
    fbe_test_sep_io_sequence_config_t  *sequence_config_p = NULL;

    /* Step 1)  Initialize the test sequence configuraiton
     */
    sequence_config_p = fbe_test_sep_io_get_sequence_config();
    status = fbe_test_sep_io_init_sequence_config(sequence_config_p,
                                                  rg_config_p,
                                                  sep_io_sequence_type,
                                                  max_blocks_for_small_io,
                                                  max_blocks_for_large_io, 
                                                  b_inject_random_aborts, 
                                                  random_abort_time_msecs,
                                                  b_run_dual_sp,
                                                  dual_sp_mode);
    switch(status)
    {
        case FBE_STATUS_EDGE_NOT_ENABLED:
            mut_printf(MUT_LOG_TEST_STATUS, "%s sep I/O sequence type: %d aborts: %d dual-sp: %d skipped", 
                       __FUNCTION__, sep_io_sequence_type, b_inject_random_aborts, b_run_dual_sp);
            return;
            
        case FBE_STATUS_OK:
            /* Continue
             */
            break;

        default:
            /* Unexpected status
             */
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            return;
    }

    /* Step 2)  Always invoke method to validate I/O ready
     */
    sequence_io_state = fbe_test_sep_io_get_sequence_io_state(sequence_config_p);
    status = fbe_test_sep_io_validate_io_ready(rg_config_p, sequence_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    sequence_io_state = fbe_test_sep_io_get_sequence_io_state(sequence_config_p);
    MUT_ASSERT_TRUE(sequence_io_state >= FBE_TEST_SEP_IO_SEQUENCE_STATE_IO_READY);

    /* Although each I/O group could allocate it's own context, for debug 
     * puposes the memory comes from this method.
     */
    fbe_zero_memory((void *)context_p, sizeof(fbe_api_rdgen_context_t));

    /* Step 3)  Now simply run the I/O sequence specified
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Starting sep I/O sequence type: %d aborts: %d dual-sp: %d", 
               __FUNCTION__, sequence_config_p->sequence_type, sequence_config_p->abort_enabled, sequence_config_p->dualsp_enabled);
    switch(sep_io_sequence_type)
    {
        case FBE_TEST_SEP_IO_SEQUENCE_TYPE_WRITE_FIXED_PATTERN:
            /* Write a fixed pattern.
             */
            status = fbe_test_sep_io_run_write_pattern(sequence_config_p,
                                                       context_p);
            break; 

        case FBE_TEST_SEP_IO_SEQUENCE_TYPE_READ_FIXED_PATTERN:
            /* Read a fixed pattern.
             */
            status = fbe_test_sep_io_run_read_pattern(sequence_config_p,
                                                      context_p);
            break; 

        case FBE_TEST_SEP_IO_SEQUENCE_TYPE_VERIFY_DATA:            
            /* Verify no errors in the data.
             */
            status = fbe_test_sep_io_run_verify(sequence_config_p);
            break;

        case FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_PREFILL:
            /* Run pre-fill and if successful fall-thru.  Currently pre-fill
             * doesn't support dual-sp.
             */
            status = fbe_test_sep_io_run_prefill(sequence_config_p,
                                                 context_p); 
            if (status != FBE_STATUS_OK)
            {
                break;
            }
            // Fall-thru

        case FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD:
            /* Run the standard I/O sequence using the supplied parameters.
             */
            status = fbe_test_sep_io_run_standard_sequence(sequence_config_p,
                                                           context_p);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Now run a verify against all luns
             */
            status = fbe_test_sep_io_run_verify_on_all_luns(rg_config_p,
                                                            FBE_TRUE, /* Verify the entire lun */
                                                            FBE_LUN_VERIFY_START_LBA_LUN_START,
                                                            FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN);
            break;

        case FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_ABORTS:
            /* Run the standard I/O sequence with aborts
             */
            status = fbe_test_sep_io_run_standard_abort_sequence(sequence_config_p,
                                                                 context_p);
            break;

        case FBE_TEST_SEP_IO_SEQUENCE_TYPE_CATERPILLAR:
            /*! Run the caterpillar (a.k.a. overlapping I/O) as specified. 
             *  
             *  @note Each lun requires it's own test context so don't use the
             *        one from the stack.
             */
            status = fbe_test_sep_io_run_caterpillar_sequence(sequence_config_p);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Now run a verify against all luns
             */
            status = fbe_test_sep_io_run_verify_on_all_luns(rg_config_p,
                                                            FBE_TRUE, /* Verify the entire lun */
                                                            FBE_LUN_VERIFY_START_LBA_LUN_START,  
                                                            FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN);   
            break;

        case FBE_TEST_SEP_IO_SEQUENCE_TYPE_TEST_BLOCK_OPERATIONS:
            /* Test all the support block operations against the configurations.
             */
            status = fbe_test_sep_io_run_block_operations_sequence(sequence_config_p);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;

        default:
            /* Unsupported I/O sequnece
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s Unsupported sep I/O sequence type: %d", 
                       __FUNCTION__, sep_io_sequence_type);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;

    }

    /* Validate success
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s sep I/O sequence type: %d complete with status: 0x%x", 
               __FUNCTION__, sequence_config_p->sequence_type, status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Step 3)  Now reset the I/O modes and state
     */
    status = fbe_test_sep_io_destroy_sequence_config(sequence_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/******************************************
 * end fbe_test_sep_io_run_test_sequence()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_io_check_status()
 ****************************************************************
 * @brief
 *  Confirms that either status is good, or if we were aborting,
 *  then the only errors we received were abort errors.
 *
 * @param context_p - Array of contexts.
 * @param num_contexts - Number of entries in array.
 * @param ran_abort_msecs - number of msecs to abort after or
 *                          FBE_TEST_RANDOM_ABORT_TIME_INVALID
 *                          if aborting is not enabled.
 * 
 * @return fbe_status_t
 *
 * @note
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  9/23/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_sep_io_check_status(fbe_api_rdgen_context_t *context_p,
                                  fbe_u32_t num_contexts,
                                  fbe_u32_t ran_abort_msecs)
{
    fbe_status_t status;
    /* Make sure there were no errors.
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        status = fbe_api_rdgen_context_check_io_status(context_p, num_contexts, FBE_STATUS_OK, 0,    /* err count*/
                                                       FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                       FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
        /* Can this fail if no I/O runs before we stop?  */
    }
    else
    {
        /* We can expect abort errors when we generate random aborts during I/Os
         */
        status = fbe_api_rdgen_context_validate_aborted_status(context_p, num_contexts);
    }
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/**************************************
 * end fbe_test_sep_io_check_status()
 **************************************/

/*!**************************************************************
 * fbe_test_sep_io_validate_rdgen_ran()
 ****************************************************************
 * @brief
 *  Make sure rdgen ran on at least SP A.
 *  For a dualsp test make sure we also ran through SP B via SP A.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/8/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_io_validate_rdgen_ran(void)
{
    fbe_status_t status;
    fbe_api_rdgen_get_stats_t rdgen_stats;
    fbe_rdgen_filter_t filter;

    /* Initialize the filter to get all active requests also.
     */
    fbe_api_rdgen_filter_init(&filter, FBE_RDGEN_FILTER_TYPE_INVALID, 
                              FBE_OBJECT_ID_INVALID, 
                              FBE_CLASS_ID_INVALID, 
                              FBE_PACKAGE_ID_INVALID, 0 /* edge index */);

    /* Get stats from SP A and make sure we did not get any requests from the peer.
     */
    status = fbe_api_rdgen_get_stats(&rdgen_stats, &filter);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    MUT_ASSERT_INT_EQUAL(rdgen_stats.historical_stats.requests_from_peer_count, 0);

    /* In dual sp mode we expect that SPA sent requests to the peer. 
     * We expect that SP B got requests from the peer. 
     */
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== rdgen stats A: to_peer: %d from_peer: %d ==", 
                   rdgen_stats.historical_stats.requests_to_peer_count,
                   rdgen_stats.historical_stats.requests_from_peer_count);
        MUT_ASSERT_INT_NOT_EQUAL(rdgen_stats.historical_stats.requests_to_peer_count, 0);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_api_rdgen_get_stats(&rdgen_stats, &filter);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        mut_printf(MUT_LOG_TEST_STATUS, "== rdgen stats B: to_peer: %d from_peer: %d ==", 
                   rdgen_stats.historical_stats.requests_to_peer_count,
                   rdgen_stats.historical_stats.requests_from_peer_count);
        MUT_ASSERT_INT_EQUAL(rdgen_stats.historical_stats.requests_to_peer_count, 0);
        MUT_ASSERT_INT_NOT_EQUAL(rdgen_stats.historical_stats.requests_from_peer_count, 0);
    }
}
/******************************************
 * end fbe_test_sep_io_validate_rdgen_ran()
 ******************************************/

/*!***************************************************************************
 *          fbe_test_rg_setup_rdgen_test_context()
 *****************************************************************************
 *
 * @brief   Setup the standard rdgen context for all luns under test.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   b_sequential_fixed
 * @param   rdgen_operation 
 * @param   pattern 
 * @param   threads
 * @param   random_io_max_blocks
 * @param   b_inject_random_aborts
 * @param   b_run_peer_io
 * @param   rdgen_peer_options
 *
 * @return  status
 *
 * @author
 *  04/04/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_rg_setup_rdgen_test_context(fbe_test_rg_configuration_t *const rg_config_p,
                                                  fbe_api_rdgen_context_t *context_p,
                                                  fbe_bool_t b_sequential_fixed,
                                                  fbe_rdgen_operation_t rdgen_operation,
                                                  fbe_rdgen_pattern_t pattern,
                                                  fbe_u32_t threads,
                                                  fbe_u64_t random_io_max_blocks,
                                                  fbe_bool_t b_inject_random_aborts,
                                                  fbe_bool_t b_run_peer_io,
                                                  fbe_api_rdgen_peer_options_t rdgen_peer_options,
                                                  fbe_lba_t lba,
                                                  fbe_block_count_t blocks)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_u32_t                               raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_object_id_t                         rg_object_id;
    fbe_test_logical_unit_configuration_t  *lu_config_p = NULL;
    fbe_object_id_t                         lu_object_id;
    fbe_u32_t                               rg_index;
    fbe_u32_t                               lu_index;
    fbe_u32_t                               io_seconds;
    fbe_time_t                              io_time;
    fbe_api_raid_group_get_info_t           rg_info;
    fbe_u64_t                               logical_blocks_per_stripe;
    fbe_bool_t                              b_is_dualsp_test = fbe_test_sep_util_get_dualsp_test_mode();

    /* First determine if a run time is enabled or not.  If it is, it overrides
     * the maximum number of passes.
     */
    io_seconds = fbe_test_sep_io_get_io_seconds();
    io_time = io_seconds * FBE_TIME_MILLISECONDS_PER_SECOND;
    if (io_seconds > 0) {
        mut_printf(MUT_LOG_TEST_STATUS, "%s using I/O seconds of %d", __FUNCTION__, io_seconds);
    }

    /*! Now configure the standard rdgen test context values
     *
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Get some inforamation about the raid group
         */
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        logical_blocks_per_stripe = rg_info.element_size * rg_info.num_data_disk;

        /* For each lun in the raid group.
         */
        for (lu_index = 0; lu_index < current_rg_config_p->number_of_luns_per_rg; lu_index++)
        {
            /* Initialize the rdgen context for each lun in the raid group.
             */
            lu_config_p = &current_rg_config_p->logical_unit_configuration_list[lu_index];
            status = fbe_api_database_lookup_lun_by_number(lu_config_p->lun_number, &lu_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Determine if it is a sequential fixed pattern or not.
             */
            if (b_sequential_fixed)
            {
                status = fbe_api_rdgen_test_context_init(context_p,
                                                         lu_object_id,
                                                         FBE_CLASS_ID_INVALID,
                                                         FBE_PACKAGE_ID_SEP_0,
                                                         rdgen_operation,
                                                         pattern,
                                                         1,    /* We do one full sequential pass. */
                                                         0,    /* num ios not used */
                                                         0,    /* time not used*/
                                                         threads,    /* threads */
                                                         FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                                         lba,    /* Start lba*/
                                                         lba,    /* Min lba */
                                                         (blocks == FBE_LBA_INVALID) ? FBE_LBA_INVALID : lba + blocks,               /* max lba */
                                                         FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                                         logical_blocks_per_stripe, /* min blocks per i/o*/
                                                         logical_blocks_per_stripe    /* Max blocks per i/o*/ );
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
            else
            {   
                /* Else we run a random
                 */
                status = fbe_api_rdgen_test_context_init(context_p, 
                                                         lu_object_id,
                                                         FBE_CLASS_ID_INVALID,
                                                         FBE_PACKAGE_ID_SEP_0,
                                                         rdgen_operation,
                                                         pattern,                       /* data pattern specified */
                                                         0,                             /* Run forever */
                                                         0,                             /* io count not used */
                                                         io_time,
                                                         threads,
                                                         FBE_RDGEN_LBA_SPEC_RANDOM,     /* Standard I/O mode is random */
                                                         lba,    /* Start lba*/
                                                         lba,    /* Min lba */
                                                         (blocks == FBE_LBA_INVALID) ? FBE_LBA_INVALID : lba + blocks, 
                                                         FBE_RDGEN_BLOCK_SPEC_RANDOM,   /* Standard I/O transfer size is random */
                                                         1,                             /* Min blocks per request */
                                                         random_io_max_blocks                 /* Max blocks per request */ );
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }

            /* Goto the next rdgen context.
             */
            context_p++;

        } /* for all luns to test*/

        /* Goto the next raid group
         */
        current_rg_config_p++;

    } /* for all raid groups*/

    /* If the injection of random aborts is enabled update the context.
     */
    if (b_inject_random_aborts == FBE_TRUE)
    {
        fbe_test_random_abort_msec_t    rdgen_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_TWO_SEC;

        /* Configure abort
         */
        status = fbe_api_rdgen_set_random_aborts(&context_p->start_io.specification,
                                                 rdgen_abort_msecs);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, 
                                                            FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* If peer I/O is enabled update the context
     */
    if (b_run_peer_io == FBE_TRUE)
    {
        /* Validate dualsp test mode is enabled
         */
        MUT_ASSERT_TRUE(b_is_dualsp_test);
        MUT_ASSERT_TRUE(rdgen_peer_options != FBE_API_RDGEN_PEER_OPTIONS_INVALID)
        status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                                 rdgen_peer_options);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return FBE_STATUS_OK;
}
/*********************************************************
 * end fbe_test_rg_setup_rdgen_test_context()
 *********************************************************/

/*!***************************************************************************
 *          fbe_test_rg_run_sync_io()
 *****************************************************************************
 * @brief   Validate the previously written zero pattern
 *
 * @param   rg_config_p - Pointer to array of raid groups to test
 *
 * @return  status
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from sleepy_hollow_write_background_pattern
 *
 *****************************************************************************/
fbe_status_t fbe_test_rg_run_sync_io(fbe_test_rg_configuration_t *const rg_config_p,
                                         fbe_rdgen_operation_t opcode,
                                         fbe_lba_t lba,
                                         fbe_block_count_t blocks,
                                         fbe_rdgen_pattern_t pattern)

{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_u32_t num_luns;

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    context_p = (fbe_api_rdgen_context_t *)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_context_t) * num_luns);
    /* Read back the pattern and check it was written OK.
     */
    status = fbe_test_rg_setup_rdgen_test_context(rg_config_p,
                                                  context_p,
                                                  FBE_TRUE,    /* This is sequential fixed */
                                                  opcode,
                                                  pattern,
                                                  1, /* Threads */
                                                  128, /* I/O size */
                                                  FBE_FALSE,    /* Don't inject aborts */
                                                  FBE_FALSE,    /* Don't run peer i/o */
                                                  FBE_API_RDGEN_PEER_OPTIONS_INVALID    /* n/a*/,
                                                  lba,
                                                  blocks);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Execute the I/O
     */
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, num_luns);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s - Success lba: 0x%llx bl: 0x%llx op: %d==", __FUNCTION__, lba, blocks, opcode);
    fbe_api_free_memory(context_p);
    return status;
}
/**********************************************
 * end fbe_test_rg_run_sync_io()
 **********************************************/
/*!**************************************************************
 * fbe_test_drain_rdgen_pins()
 ****************************************************************
 * @brief
 *  Wait for unpins to drain.
 *
 * @param wait_seconds - max seconds to wait
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  7/22/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_drain_rdgen_pins(fbe_u32_t wait_seconds)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_seconds * FBE_TIME_MILLISECONDS_PER_SECOND;
    fbe_rdgen_control_get_stats_t get_stats;
    fbe_rdgen_filter_t filter;

    /* Initialize the filter to get all active requests also.
     */
    fbe_api_rdgen_filter_init(&filter, FBE_RDGEN_FILTER_TYPE_INVALID, 
                              FBE_OBJECT_ID_INVALID, 
                              FBE_CLASS_ID_INVALID, 
                              FBE_PACKAGE_ID_INVALID, 0 /* edge index */);

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < target_wait_msec) {
        status = fbe_api_rdgen_get_complete_stats(&get_stats, &filter);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        if (get_stats.in_process_io_stats.pin_count == 0) {
            mut_printf(MUT_LOG_TEST_STATUS, "=== %s pins drained", __FUNCTION__);
            return status;
        }
        /* Display after 2 seconds.
         */
        if ((elapsed_msec % 2000) == 0) {
            mut_printf(MUT_LOG_TEST_STATUS, "=== %s waiting for pins to drain pins outstanding: %u", 
                       __FUNCTION__, get_stats.in_process_io_stats.pin_count);
        }
        EmcutilSleep(500);
        elapsed_msec += 500;
    }

    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= (wait_seconds * 1000)) {
        status = FBE_STATUS_TIMEOUT;
    }
    return status;
}
/******************************************
 * end fbe_test_drain_rdgen_pins()
 ******************************************/
/*************************
 * end file fbe_test_sep_io.c
 *************************/
