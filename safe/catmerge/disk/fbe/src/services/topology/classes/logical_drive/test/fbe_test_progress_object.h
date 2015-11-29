#ifndef FBE_TEST_PROGRESS_OBJECT_H
#define FBE_TEST_PROGRESS_OBJECT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_test_progress_object.h
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the test progress object.
 * 
 *  This object allows us to track the number of seconds of the test and
 *  display progress at a fixed interval.
 *
 * HISTORY
 *   11/07/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"

#define MILLISECONDS_PER_SECOND 1000 /* Used for displaying. */

/*! @struct fbe_test_progress_object_t 
 *  
 *  @brief This object is used for keeping track of progress in a test.
 */
typedef struct fbe_test_progress_object_s
{
    /*! Number of times we have displayed.
     */
    fbe_u32_t iterations;

    /*! Seconds when we started the test.
     */
    fbe_u64_t start_seconds;
    fbe_u64_t current_seconds;
    fbe_u64_t last_display_seconds;

    /*! Seconds in between displaying.
     */
    fbe_u32_t seconds_to_display;
}
fbe_test_progress_object_t;

/*!**************************************************************
 * @fn fbe_test_progress_object_init(fbe_test_progress_object_t *const object_p,
 *                                   fbe_u32_t seconds_to_display)
 ****************************************************************
 * @brief   
 *  Initializes the test progress object.  We fill it with the   
 *  Current time so that subsequent calls to   
 *  @ref fbe_test_progress_object_get_seconds_elapsed or   
 *  @ref fbe_test_progress_object_is_time_to_display will know at    
 *  which time the test started.   
 *  
 * @param object_p - Test progress object to init.
 * @param seconds_to_display - Seconds in between displaying that   
 *                             we want to use for this object.   
 *                             Note that we will use this later when   
 *                             other methods of this object are called.  
 *
 * @return - None.   
 *   
 * @remarks This function does not allocate memory, thus we do not have   
 *          a destroy function.   
 *   
 * HISTORY:
 *  11/10/2008 - Created. RPF
 *
 ****************************************************************/

static __inline void fbe_test_progress_object_init(fbe_test_progress_object_t *const object_p,
                                            fbe_u32_t seconds_to_display)
{
    /* Get the time we are starting at.  We'll compare to this over 
     * the test to determine when we are done.
     */
    fbe_u32_t current_seconds;
    current_seconds = (fbe_u32_t)(fbe_get_time() / MILLISECONDS_PER_SECOND);
    object_p->start_seconds = current_seconds;
    object_p->current_seconds = current_seconds;
    object_p->last_display_seconds = current_seconds;
    object_p->iterations = 0;
    object_p->seconds_to_display = seconds_to_display;
    return;
}
/**************************************
 * end fbe_test_progress_object_init()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_progress_object_get_seconds_elapsed(
 *                    fbe_test_progress_object_t *const object_p)
 ****************************************************************
 * @brief
 *   Determine how long the test has been running.
 *  
 * @param object_p - The test progress object to get elapsed time for.
 * 
 * @return fbe_u32_t Number of seconds since the test stared.
 *                   (really since we called fbe_test_progress_object_init())
 *
 * HISTORY:
 *  11/10/2008 - Created. RPF
 *
 ****************************************************************/

static __inline fbe_u32_t fbe_test_progress_object_get_seconds_elapsed(fbe_test_progress_object_t *const object_p)
{
    
    fbe_u32_t current_seconds;
    /* Get the current time and save it in the object.
     */
    current_seconds = (fbe_u32_t)(fbe_get_time() / MILLISECONDS_PER_SECOND);
    object_p->current_seconds = current_seconds;

    /* Time since the test started is the delta between the start of the test
     * and the current time. 
     */
    return(fbe_u32_t)(current_seconds - object_p->start_seconds);
}
/**************************************
 * end fbe_test_progress_object_get_seconds_elapsed()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_progress_object_get_iterations(
 *                     fbe_test_progress_object_t *const object_p)
 ****************************************************************
 * @brief
 *  Returns the number of times we have checked
 *  if it is time to display the progress by calling
 *  @ref fbe_test_progress_object_is_time_to_display.
 *  
 * @param object_p - The object tracking this test's progress.
 * 
 * @return fbe_u32_t - Number of iterations of the test.
 *                     We assume that an iteration is defined by
 *                     a call to @ref fbe_test_progress_object_is_time_to_display.
 *
 * HISTORY:
 *  11/10/2008 - Created. RPF
 *
 ****************************************************************/

static __inline fbe_u32_t fbe_test_progress_object_get_iterations(fbe_test_progress_object_t *const object_p)
{
    return(object_p->iterations);
}
/**************************************
 * end fbe_test_progress_object_get_iterations()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_progress_object_is_time_to_display(fbe_test_progress_object_t *const object_p)
 ****************************************************************
 * @brief
 *   Determine if we should display progress.  It is time
 *   to display progress if the time in between the last display of
 *   time and the current time is greater than or equal to
 *   the seconds_to_display which was setup in the test progress
 *   object at initialization time.
 *  
 * @param object_p - The object to check.
 * 
 * @return fbe_bool_t - TRUE if it is time to display progress
 *                      FALSE otherwise.
 *
 * HISTORY:
 *  11/10/2008 - Created. RPF
 *
 ****************************************************************/

static __inline fbe_bool_t fbe_test_progress_object_is_time_to_display(fbe_test_progress_object_t *const object_p)
{
    fbe_u32_t current_seconds;
    object_p->iterations++;
    /* Refresh the time.
     */
    current_seconds = (fbe_u32_t)(fbe_get_time() / MILLISECONDS_PER_SECOND);
    
    object_p->current_seconds = current_seconds;

    /* If the delta between now and our last display is greater than 
     * the number of seconds to wait in between displaying, then return TRUE. 
     */
    if ((object_p->current_seconds - object_p->last_display_seconds) >= object_p->seconds_to_display)
    {
        object_p->last_display_seconds = object_p->current_seconds;
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}
/**************************************
 * end fbe_test_progress_object_is_time_to_display()
 **************************************/

#endif /* FBE_TEST_PROGRESS_OBJECT_H */
/*****************************************************
 * end fbe_test_progress_object.h
 *****************************************************/
