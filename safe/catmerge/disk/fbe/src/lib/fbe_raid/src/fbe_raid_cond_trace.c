/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_cond_trace.c
 *
 * @brief
 *  This file contains function used to test the unexpected raid 
 *  error paths
 *
 * @version
 *   06/15/2010:  Created. Jyoti Ranjan
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_transport_memory.h"
#include "fbe_raid_library_proto.h"
#include "fbe_raid_cond_trace.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_random.h"

/* @brief 
 * Currently, we will be using queue to store the condition-element 
 * traced. We can change the internal implementation of condition-log-database, 
 * if required in future.
 */
static fbe_queue_head_t fbe_raid_cond_log_head; 
static fbe_spinlock_t fbe_raid_cond_log_lock;

static int fbe_raid_cond_trace_native_alloc_count = 0;
static int fbe_raid_cond_trace_native_release_count = 0;
static fbe_u32_t fbe_raid_cond_random_errors_percentage = 0;

/*************************
 * FUNCTION DEFINITIONS
 *************************/


/**************************************************************************
 * fbe_raid_cond_init_log()
 **************************************************************************
 * @brief
 *   This function is called to initialize condition-log database
 * 
 * @param  - None
 *
 * @return - None.
 *
 **************************************************************************/
void fbe_raid_cond_init_log(void)
{
    fbe_queue_init(&fbe_raid_cond_log_head);
	fbe_spinlock_init(&fbe_raid_cond_log_lock);
}
/******************************************
 * end fbe_raid_cond_init_log()
 ******************************************/

/**************************************************************************
 * fbe_raid_cond_destroy_log()
 **************************************************************************
 * @brief
 *   This function is called to destroy condition-log database
 * 
 * @param  - None
 *
 * @return - None.
 *
 **************************************************************************/
void fbe_raid_cond_destroy_log(void)
{
    /* stops tracing and release all the memory */
    fbe_raid_cond_reset_library_error_testing_stats();
    /* release resource */
    fbe_queue_destroy(&fbe_raid_cond_log_head);
    fbe_spinlock_destroy(&fbe_raid_cond_log_lock);
}
/******************************************
 * end fbe_raid_cond_destroy_log()
 ******************************************/


/**************************************************************************
 * fbe_raid_cond_create_log_element()
 **************************************************************************
 * @brief
 *   This function is called to create a condition-element, which is 
 *   stored in condition-log database.
 *
 * @param  cond_info_p          - condition information
 * @param  cond_log_element_pp  - pointer to pointer to the new condition-log element
 *
 * @return fbe_status_t.
 *
 **************************************************************************/
fbe_status_t fbe_raid_cond_create_log_element(fbe_raid_cond_info_t * cond_info_p,
                                              fbe_raid_cond_log_element_t ** cond_log_element_pp)
{
      fbe_raid_cond_log_element_t * cond_element_p = NULL;

      /* Allocate memeory */
      cond_element_p = (fbe_raid_cond_log_element_t *) fbe_memory_native_allocate(sizeof(fbe_raid_cond_log_element_t));

      if (cond_element_p == NULL)
      {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "could not allocate memory to create condition-element\n");

            return FBE_STATUS_INSUFFICIENT_RESOURCES;
      }

      fbe_raid_cond_trace_native_alloc_count += 1;

      /* Initialize element */
      cond_element_p->cond_info_p = cond_info_p;

      /* Return the condition log queue element */
      *cond_log_element_pp = cond_element_p;

      return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_cond_create_log_element()
 ******************************************/






/**************************************************************************
 * fbe_raid_cond_add_new_element_to_log()
 **************************************************************************
 * @brief
 *   This function adds the condition-element to condition-log
 *
 * @param  cond_element_p - pointer to condition-element to be added
 *
 * @return fbe_status_t.
 *
 **************************************************************************/
fbe_status_t fbe_raid_cond_add_new_element_to_log(fbe_raid_cond_log_element_t * cond_element_p)
{
    fbe_spinlock_lock(&fbe_raid_cond_log_lock);
    fbe_queue_push(&fbe_raid_cond_log_head, &cond_element_p->queue_element); 
    fbe_spinlock_unlock(&fbe_raid_cond_log_lock);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_cond_add_new_element_to_log()
 ******************************************/




/**************************************************************************
 * fbe_raid_cond_remove_element_from_log()
 **************************************************************************
 * @brief
 *   This function removes the condition-element from condition-log
 *
 * @param  cond_element_p - pointer to condition element to be removed
 *
 * @return fbe_status_t.
 *
 **************************************************************************/
fbe_status_t fbe_raid_cond_remove_element_from_log(fbe_raid_cond_log_element_t *cond_element_p)
{
    fbe_queue_remove(&cond_element_p->queue_element);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_cond_remove_element_from_log()
 ******************************************/



/**************************************************************************
 * fbe_raid_cond_get_log_element_count()
 **************************************************************************
 * @brief
 *   This function gets the number of condition-element stored in
 *   condition-log.
 *
 * @param  count_p - pointer to number of condition-elements to return
 *
 * @return fbe_status_t.
 *
 **************************************************************************/
fbe_status_t fbe_raid_cond_get_log_element_count(fbe_u32_t * const count_p)
{
    fbe_spinlock_lock(&fbe_raid_cond_log_lock);
    *count_p = fbe_queue_length(&fbe_raid_cond_log_head);
    fbe_spinlock_unlock(&fbe_raid_cond_log_lock);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_cond_get_log_element_count()
 *************************************/



/**************************************************************************
 * fbe_raid_cond_is_cond_logged()
 **************************************************************************
 * @brief
 *   This function is used to determine whether a given condition identified 
 *   by line/function is already logged in condition-log or not
 *
 * @param  line           - line number at which condition exists
 * @param  function_p     - pointer to function name (a string) in which condition exists
 * @param  b_logged_p     - pointer to logged-result to return
 * 
 * @return fbe_status_t.
 *
 **************************************************************************/
fbe_status_t fbe_raid_cond_is_cond_logged(fbe_u32_t line,
                                          const char * const function_p,
                                          fbe_bool_t * const b_logged_p)
{
    fbe_raid_cond_log_element_t *element_p;

    fbe_spinlock_lock(&fbe_raid_cond_log_lock);
    *b_logged_p = FBE_FALSE;
    if(fbe_queue_is_empty(&fbe_raid_cond_log_head))
    {
       /* Return immediately if the queue is empty.
        */
       *b_logged_p = FBE_FALSE;
    }

    element_p = (fbe_raid_cond_log_element_t *) fbe_queue_front(&fbe_raid_cond_log_head);

    while (element_p != NULL)
    {
       /* match current condition-element of condition-log with 
        * given condition details (primairly line number and function
        * name where condition is used).
        */
       if ((element_p->cond_info_p->line == line) &&
           (strcmp(element_p->cond_info_p->fn_name, function_p) == 0))
       {
           *b_logged_p = FBE_TRUE;
           break;
       }

       /* get the next condition-element stored in condition-log
        */
       element_p = (fbe_raid_cond_log_element_t *) fbe_queue_next(&fbe_raid_cond_log_head,  
                                                                  (fbe_queue_element_t*) element_p);
    }
       
    fbe_spinlock_unlock(&fbe_raid_cond_log_lock);

    return FBE_STATUS_OK;
}
/***************************
 * end fbe_raid_cond_is_cond_logged()
 **************************/




/**************************************************************************
 * fbe_raid_cond_register_cond()
 **************************************************************************
 * @brief
 *   This function is used to determine whether condition is already
 *   traced or not. If not then it will log the condition in 
 *   condition-log
 * 
 *   Each condition is uniquely identified by the line number and 
 *   function in which it is defined.
 *
 * @param  line           - line number at which condition exists
 * @param  function_p     - pointer to function name (a string) in which condition exists
 * @param  b_result_p     - pointer to return FBE_TRUE or FBE_FALSE
 * 
 * @return fbe_status_t.
 *
 **************************************************************************/
fbe_status_t fbe_raid_cond_register_cond(fbe_u32_t line,
                                         const char * const function_p,
                                         fbe_bool_t * const b_result_p)
{
    fbe_bool_t b_logged = FBE_FALSE;
    fbe_status_t status;

    if (fbe_raid_cond_random_errors_percentage != 0)
    {
        /* Inject some percentage of the time.
         */
        *b_result_p = (fbe_random() % 100) <= fbe_raid_cond_random_errors_percentage;
        return FBE_STATUS_OK;
    }
    status = fbe_raid_cond_is_cond_logged(line,
                                          function_p,
                                          &b_logged);

    if (status != FBE_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (b_logged == FBE_FALSE)
    {
        fbe_raid_cond_info_t  *cond_info_p;
        fbe_raid_cond_log_element_t * element_insert_p;

        /* Condition-log does not contain the 
         * condition-element corresponding to given line and 
         * function name. We will return true which will make
         * to fail the condition since line/function has not 
         * seen covered yet. Also, we will log the line/function 
         * in our condition-log.
         */
        *b_result_p = FBE_TRUE;

        cond_info_p = (fbe_raid_cond_info_t *) fbe_memory_native_allocate(sizeof(fbe_raid_cond_info_t));
        if (cond_info_p == NULL)
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "could not allocate memory to create condition-element\n");

            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }

        fbe_raid_cond_trace_native_alloc_count += 1;
        cond_info_p->line = line;
        strncpy(cond_info_p->fn_name, function_p, sizeof(cond_info_p->fn_name)-1); 
        cond_info_p->fn_name[sizeof(cond_info_p->fn_name)-1] = '\0';

        status = fbe_raid_cond_create_log_element(cond_info_p,
                                                  &element_insert_p);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }

        status = fbe_raid_cond_add_new_element_to_log(element_insert_p);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
    }
    else
    {
        *b_result_p = FBE_FALSE;
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_cond_register_cond()
 *************************************/

fbe_raid_cond_function_register_error_path_t fbe_raid_cond_execute_register_cond = NULL;
fbe_bool_t fbe_raid_cond_inject_on_all_rgs = FBE_TRUE;


/*****************************************************************************
 *          fbe_raid_cond_set_library_error_testing()
 *****************************************************************************
 *
 * @brief   Enables the unexpected error testing path.
 *
 * @param   None
 *
 * @return  status - Always FBE_STATUS_OK
 *
 * @author
 *  07/15/2010  Jyoti Ranjan - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_cond_set_library_error_testing(void)
{
    if (fbe_raid_cond_execute_register_cond == NULL)
    {
        fbe_raid_cond_execute_register_cond = fbe_raid_cond_register_cond;
    }

    fbe_raid_cond_trace_native_alloc_count = 0;
    fbe_raid_cond_trace_native_release_count = 0;
    fbe_raid_cond_random_errors_percentage = 0;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_cond_set_library_error_testing()
 *************************************/

/*****************************************************************************
 *    fbe_raid_cond_set_library_random_errors()
 *****************************************************************************
 *
 * @brief   Enables the injection of random unexpected errors.
 *
 * @param  percentage - Percentage of the time to inject errors.
 * @param b_inject_on_all_conditions - FBE_FALSE No only inject on user rgs.
 *                                     FBE_TRUE Otherwise inject on all rgs.
 *
 * @return  status - Always FBE_STATUS_OK
 *
 * @author
 *  6/27/2012 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_raid_cond_set_library_random_errors(fbe_u32_t percentage,
                                                     fbe_bool_t b_inject_on_all_rgs)
{
    fbe_raid_cond_set_library_error_testing();
    fbe_raid_cond_random_errors_percentage = percentage;
    fbe_raid_cond_inject_on_all_rgs = b_inject_on_all_rgs;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_cond_set_library_random_errors()
 *************************************/


/*****************************************************************************
 *    fbe_raid_cond_set_inject_on_user_raid_only()
 *****************************************************************************
 *
 * @brief   Set injection only on user raid groups.
 *
 * @param  percentage - Percentage of the time to inject errors.
 *
 * @return  status - Always FBE_STATUS_OK
 *
 * @author
 *  10/5/2012 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_raid_cond_set_inject_on_user_raid_only(void)
{
    fbe_raid_cond_inject_on_all_rgs = FBE_FALSE;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_cond_set_inject_on_user_raid_only()
 *************************************/



/*****************************************************************************
 *          fbe_raid_cond_reset_library_error_testing_stats()
 *****************************************************************************
 *
 * @brief   Resets the error-testing stats
 *
*  @param   None
 *
 * @return  fbe_status_t
 *
 * @author
 *  07/15/2010  Jyoti Ranjan - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_cond_reset_library_error_testing_stats(void)
{
    fbe_u32_t index = 0;
    fbe_raid_cond_log_element_t * curr_cond_log_element_p = NULL;

    /* Disable the error testing functionality. Also, reset error testing level.
     */
    fbe_raid_cond_execute_register_cond = NULL;
    fbe_raid_cond_random_errors_percentage = 0;

    fbe_spinlock_lock(&fbe_raid_cond_log_lock);
    /* Purge condition-log entries
     */
    while (fbe_queue_is_empty(&fbe_raid_cond_log_head) == FALSE)
    {
        curr_cond_log_element_p = (fbe_raid_cond_log_element_t*)fbe_queue_pop(&fbe_raid_cond_log_head);
        if (curr_cond_log_element_p->cond_info_p != NULL)
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "error: condition traced [%d]: line = %d function: %s\n", 
                                   index++,
                                   curr_cond_log_element_p->cond_info_p->line,
                                   curr_cond_log_element_p->cond_info_p->fn_name);

            fbe_memory_native_release(curr_cond_log_element_p->cond_info_p);
            fbe_raid_cond_trace_native_release_count += 1;
        }
        fbe_memory_native_release(curr_cond_log_element_p);
        fbe_raid_cond_trace_native_release_count += 1;
    }
    fbe_spinlock_unlock(&fbe_raid_cond_log_lock);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_cond_reset_library_error_testing_stats()
 *************************************/


/*****************************************************************************
 *          fbe_raid_cond_get_library_error_testing_stats()
 *****************************************************************************
 *
 * @brief   Enables the unexpected error testing path. 
 *
 * @param   error_testing_stats_p - pointer to return error testing statistics
 *
 * @return  fbe_status_t
 *
 * @author
 *  07/15/2010  Jyoti Ranjan - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_cond_get_library_error_testing_stats
(
    fbe_raid_group_raid_lib_error_testing_stats_t * error_testing_stats_p
)
{
    fbe_u32_t traced_error;
    fbe_status_t status;


    if (error_testing_stats_p == NULL)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "error-testing: invalid argument passed\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_raid_cond_get_log_element_count(&traced_error);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    error_testing_stats_p->error_count = traced_error; 
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_cond_get_library_error_testing_stats()
 *************************************/





/*************************
 * end file fbe_raid_cond_trace.c
 *************************/
