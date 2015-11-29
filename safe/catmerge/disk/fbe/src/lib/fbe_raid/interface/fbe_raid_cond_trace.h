#ifndef __FBE_RAID_LIBRARY_COND_TRACE_H__
#define __FBE_RAID_LIBRARY_COND_TRACE_H__


/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_cond_trace.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions and function prototype declaration 
 *  used to test unexpected error path
 *
 * @version
 *   6/15/2010:  Created. Jyoti Ranjan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_queue.h"
#include "fbe/fbe_types.h"


/*************************
 *  LITERAL DEFINITIONS
 *************************/


/*! @def FBE_COND_MAX_FUNCTION_NAME_CHARS  
 *  
 *  @brief This is the max number of characters a function name is expected to have
 */
#define FBE_COND_MAX_FUNCTION_NAME_CHARS 80

/*!*************************************************************
 * @struct fbe_raid_cond_info_t
 * 
 * @brief
 *  This is the data structure used to store the information 
 *  related to condition detected at the time of execution of 
 *  libarary code
 ******************************************************************/
typedef struct fbe_raid_cond_info_s
{
    fbe_u32_t line;        /*!< line number at which condition exists */
    fbe_char_t fn_name[FBE_COND_MAX_FUNCTION_NAME_CHARS];   /*!< function name (a null terminated string) */
} 
fbe_raid_cond_info_t;




/*!***************************************************************
 * @struct fbe_raid_cond_log_element_t
 * 
 * @brief
 *  This structure is used to represent a condition-element in 
 *  condition-log-database
 ******************************************************************/
typedef struct fbe_raid_cond_log_element_s
{
    fbe_queue_element_t queue_element;  /*!< queue element */
    fbe_raid_cond_info_t * cond_info_p;  /*!< pointer to structure to store condition detail */
}
fbe_raid_cond_log_element_t;



/*!***************************************************************
 * @fn fbe_raid_cond_register_error_path_function_t
 * 
 * @brief
 *  This function pointer does register the unexpected error path 
 *  in condition log, whenever invoked.
 ******************************************************************/
typedef fbe_status_t (* fbe_raid_cond_function_register_error_path_t)(fbe_u32_t line,
                                                                      const char * const function_p,
                                                                      fbe_bool_t * const b_result_p);


/* @brief
 * This function is a global variable which is called at the time of 
 * unexpected error path, if the function is set. 
 */
extern fbe_raid_cond_function_register_error_path_t fbe_raid_cond_execute_register_cond;
/*******************
 * FUNCTION 
 ******************/

extern fbe_bool_t fbe_raid_cond_inject_on_all_rgs;
/**************************************************************************
 * fbe_raid_cond_get_register_fn()
 **************************************************************************
 * @brief
 *   This is an accessor function to get the functions pointer, which 
 *   does log the unexpected error condition.
 * 
 * @param  - None
 *
 * @return - pointer to function which does register the error condition
 *
 **************************************************************************/
static __forceinline fbe_raid_cond_function_register_error_path_t fbe_raid_cond_get_register_fn(void)
{
    if (fbe_raid_cond_inject_on_all_rgs == FBE_FALSE)
    {
        return NULL;
    }
    return fbe_raid_cond_execute_register_cond;
}
/******************************************
 * end fbe_raid_cond_get_register_fn()
 ******************************************/

/**************************************************************************
 * fbe_raid_cond_get_geo_register_fn()
 **************************************************************************
 * @brief
 *   Checks to see if we should inject on only the geo conditions.
 * 
 * @param  - None
 *
 * @return - pointer to function which does register the error condition
 *
 **************************************************************************/
static __forceinline fbe_raid_cond_function_register_error_path_t fbe_raid_cond_get_geo_register_fn(void)
{
    return fbe_raid_cond_execute_register_cond;
}
/******************************************
 * end fbe_raid_cond_get_geo_register_fn()
 ******************************************/

/*****************************************************************************
 *          fbe_raid_cond_evaluate_exp()
 *****************************************************************************
 *
 * @brief   Evaluates the condition handler and return result of condition 
 *          depending upon whether error testing path is enabled or not.
 *
 * @param   exp_result     - result of expression 
 * @param   line           - line at which condition was detected
 * @param   function_p     - the function in which condition was detected
 * @param   b_new_cond_p   - a return value to indicate whether condition 
 *                           was seen first time or not, if error testing path
 *                           is enabled. If error testing path is not enabled 
 *                           then it will always return FBE_FALSE.
 *                           
 *
 * @returns FBE_TRUE or FBE_FALSE.
 *          
 *          If error-testing is enabled and the condition is being evaluated 
 *          first time then we do return FBE_TRUE -ALWAYS- else we do return 
 *          result of expression which can be either FBE_TRUE or FBE_FALSE.
 *
 *          If error-testing is not enabled then we do always return result of 
 *          expression which can be either FBE_TRUE or FBE_FALSE.
 *
 * @author
 *  07/15/2010  Jyoti Ranjan - Created
 *
 *****************************************************************************/
static __inline fbe_bool_t fbe_raid_cond_evaluate_exp(fbe_u32_t exp_result,
                                      fbe_u32_t line,
                                      const fbe_char_t * const function_p,
                                      fbe_bool_t * b_new_cond_p)
{
     fbe_bool_t b_registered;
     fbe_bool_t b_error_testing;
     fbe_status_t status;
     fbe_raid_cond_function_register_error_path_t register_p;

     b_error_testing = FBE_FALSE;
     b_registered = FBE_FALSE;

     /* Deetermine whether the condition is to be registered or
      * or it is already registerd (in case error testing path is enabled)
      */
     register_p = fbe_raid_cond_get_geo_register_fn();
     if (register_p != NULL)
     {
        status = register_p(line, function_p, &b_registered);
        
        if ((status != FBE_STATUS_OK) || (b_registered == FBE_TRUE))
        {
             b_error_testing = FBE_TRUE;
        }
     }
     
     /* if the condition has been registered first time then
      * return FBE_TRUE else exp_result. Also, return the information
      * that it has been registered first time.
      */
     *b_new_cond_p = b_error_testing;
     return ((b_error_testing) ? FBE_TRUE : exp_result);
}
/**************************************
 * end fbe_raid_cond_evaluate_exp()
 *************************************/




/*****************************************************************************
 *          fbe_raid_cond_evaluate_exp_only()
 *****************************************************************************
 *
 * @brief   Evaluates the condition handler and return result of condition 
 *          depending upon whether error testing path is enabled or not.It also 
 *          change the status to user-passed value, if condition is seen
 *          first time and is registered.
 *
 * @param   exp_result           - result of expression 
 * @param   line                 - line at which condition was detected
 * @param   function_p           - the function in which condition was detectedtatus.
 *
 * @returns FBE_TRUE or FBE_FALSE.
 *          
 *          If error-testing is enabled and the condition is being evaluated 
 *          first time then we do return FBE_TRUE -ALWAYS- else we do return 
 *          result of expression which can be either FBE_TRUE or FBE_FALSE.
 *
 *          If error-testing is not enabled then we do always return result of 
 *          expression which can be either FBE_TRUE or FBE_FALSE.
 *
 * @author
 *  07/15/2010  Jyoti Ranjan - Created
 *
 *****************************************************************************/
static __inline fbe_bool_t fbe_raid_cond_evaluate_exp_only(fbe_u32_t exp_result,
                                           fbe_u32_t line,
                                           const fbe_char_t * const function_p)
{
    fbe_bool_t b_registered = FBE_FALSE;

    return  fbe_raid_cond_evaluate_exp(exp_result, line, function_p, &b_registered);
}
/**************************************
 * end fbe_raid_cond_evaluate_exp_only()
 *************************************/





/*****************************************************************************
 *          fbe_raid_cond_evaluate_exp_and_change_status()
 *****************************************************************************
 *
 * @brief   Evaluates the condition handler and return result of condition 
 *          depending upon whether error testing path is enabled or not.It also 
 *          change the status to user-passed value, if condition is seen
 *          first time and is registered.
 *
 * @param   exp_result           - result of expression 
 * @param   line                 - line at which condition was detected
 * @param   function_p           - the function in which condition was detected
 * @param   b_registered_p  [O]  - a return value to indicate whether condition was seen 
 *                                 first time or not, if error testing path is enabled. If 
 *                                 error testing path is not enabled then it will always 
 *                                 return FBE_FALSE.
 * @param   final_status         - user passed value to which status has to be modified.
 * @param   changed_status_p [O] - a return parameter to modify the status to final_status.
 *
 * @returns FBE_TRUE or FBE_FALSE.
 *          
 *          If error-testing is enabled and the condition is being evaluated 
 *          first time then we do return FBE_TRUE -ALWAYS- else we do return 
 *          result of expression which can be either FBE_TRUE or FBE_FALSE.
 *
 *          If error-testing is not enabled then we do always return result of 
 *          expression which can be either FBE_TRUE or FBE_FALSE.
 *
 * @author
 *  07/15/2010  Jyoti Ranjan - Created
 *
 *****************************************************************************/
static __inline fbe_bool_t fbe_raid_cond_evaluate_exp_and_change_status(fbe_u32_t exp_result,
                                                        fbe_u32_t line,
                                                        const fbe_char_t * const function_p,
                                                        fbe_status_t final_status,
                                                        fbe_status_t * changed_status_p)
{
    fbe_bool_t b_result = FBE_FALSE;
    fbe_bool_t b_registered = FBE_FALSE;
    
    b_result = fbe_raid_cond_evaluate_exp(exp_result, line, function_p, &b_registered);
    if (b_registered)
    {
        /* if we are registering the condition then we do need 
         * to change the status too as asked.
         */
        *changed_status_p = final_status;
    }

    return b_result;
}
/**************************************
 * end fbe_raid_cond_evaluate_exp_and_change_status()
 *************************************/

/*

__inline fbe_bool_t fbe_raid_cond_evaluate_exp_only(fbe_u32_t exp_result,
                                           fbe_u32_t line,
                                           const fbe_char_t * const function_p)


__inline fbe_bool_t fbe_raid_cond_evaluate_exp_and_change_status(fbe_u32_t exp_result,
                                                        fbe_u32_t line,
                                                        const fbe_char_t * const function_p,
                                                        fbe_status_t final_status,
                                                        fbe_status_t * changed_status);
*/

#endif /* __FBE_RAID_LIBRARY_COND_TRACE_H__*/


/*************************
 * end file fbe_raid_cond_trace.h
 *************************/
