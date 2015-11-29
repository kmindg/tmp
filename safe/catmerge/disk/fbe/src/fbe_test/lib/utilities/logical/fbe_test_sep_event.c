/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_sep_event.c
 ***************************************************************************
 *
 * @brief
 *  This file contains event handling functions for use with the Storage Extent 
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/

#include "sep_tests.h"
#include "sep_event.h"
#include "fbe/fbe_api_event_log_interface.h"


/*****************************************
 * DEFINITIONS
 *****************************************/

/*!*******************************************************************
 * @def FBE_TEST_SEP_WAIT_MSEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/

#define FBE_TEST_SEP_WAIT_MSEC 30000



/*************************
 *   GLOBALS
 *************************/

/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************************
 *          fbe_test_sep_event_validate_event_generated()
 *****************************************************************************
 * 
 * @brief   Simply validates that the specified event was generated.
 *
 * @param   event_message_id - The message identifier of the event to check for.
 * @param   b_event_found_p - Pointer to bool that states if event found or not
 * @param   b_reset_event_log_if_found - FBE_TRUE - reset the event log if found
 *                                       FBE_FALSE - Event log is not cleared
 * @param   wait_tmo_ms - Timeout in milliseconds to wait for event
 *
 * @return  status
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_event_validate_event_generated(fbe_u32_t event_message_id,
                                                         fbe_bool_t *b_event_found_p,
                                                         fbe_bool_t b_reset_event_log_if_found,
                                                         fbe_u32_t  wait_tmo_ms)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_bool_t                  b_event_found = FBE_FALSE;    
    fbe_event_log_statistics_t  event_log_statistics = {0};
    fbe_u32_t                   event_log_index = 0;
    fbe_u32_t                   number_of_entries_to_check = 0;
    fbe_event_log_get_event_log_info_t event_log = {0};
    fbe_u32_t                   wait_interval_ms = 100;
    fbe_u32_t                   wait_tmo_count = 0;
    fbe_u32_t                   max_wait_tmo_count = 0;

    /* Initialize result to `not found'
     */
    *b_event_found_p = FBE_FALSE;
    if (wait_tmo_ms < wait_interval_ms)
    {
        max_wait_tmo_count = 1;
    }
    else
    {
        max_wait_tmo_count = wait_tmo_ms / wait_interval_ms;
    }

    /* While we haven't waited too long.
     */
    while ((b_event_found == FBE_FALSE)          &&
           (wait_tmo_count < max_wait_tmo_count)    )
    {
        /* Validate the reported event-log messages against the requested.
         */
        status = fbe_api_get_event_log_statistics(&event_log_statistics,FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "== Error: failed to get event log statistics == \n");
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    
        /* Check all the events logged.
         */
        number_of_entries_to_check = event_log_statistics.current_msgs_logged;
        for (event_log_index = 0; event_log_index < number_of_entries_to_check; event_log_index++)
        {
            event_log.event_log_index = event_log_index;
            status = fbe_api_get_event_log(&event_log, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "== Error : failed to get event log info ==");
            if (status != FBE_STATUS_OK)
            {
                return status;
            }
    
            /* Check if the request message id is present or not
             */
            if (event_log.event_log_info.msg_id == event_message_id)
            {
                /* Print a message, mark the event found and break.
                 */
                mut_printf(MUT_LOG_TEST_STATUS,
                           "validate event: message id: 0x%08X located at index: %d text: %s ",
                           event_message_id, event_log_index, event_log.event_log_info.msg_string);
                b_event_found = FBE_TRUE;
                *b_event_found_p = FBE_TRUE;
                break;
            }
        }

        /* If event not found sleep.
         */
        if (b_event_found == FBE_FALSE)
        {
            fbe_api_sleep(wait_interval_ms);
            wait_tmo_count++;
        }

    } /* while event not found and timeout did not occur. */

    /* If the message wasn't found print a message and fail.
     */
    if (b_event_found == FBE_FALSE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "validate event: message id: 0x%08X not located in the: %d messages found.",
                   event_message_id, event_log_statistics.total_msgs_logged);
        MUT_ASSERT_TRUE(b_event_found == FBE_TRUE);
    }
    else if (b_reset_event_log_if_found)
    {
        /* If requested reset the event log
         */
        status = fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "== %s Failed to clear SEP event log statistics! == ");

        status = fbe_api_clear_event_log_statistics(FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "== %s Failed to clear SEP event statistics! == "); 
    }

    /* Return the status
     */
    return status;
}
/***************************************************
 * end fbe_test_sep_event_validate_event_generated()
 ***************************************************/


/*******************************
 * end file fbe_test_sep_event.c
 *******************************/
