/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_random_kernel_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains kernel mode definitions of the fbe random methods.
 *  This code was ported from flare's timestamp.c and timestamp.h.
 *
 * @version
 *   7/20/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!***************************************************************
 * fbe_random_set_seed()
 ****************************************************************
 * @brief
 *   Seed is not required for kernel mode generation of
 *   random numbers since we always use the clock.
 *
 * @param seed - seed to set for random numbers
 *
 * @return None
 *
 * @author
 *  8/29/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_random_set_seed(fbe_u32_t seed)
{
    return;
}
/**************************************
 * end fbe_random_set_seed()
 **************************************/

/*!***************************************************************
 * fl_random
 ****************************************************************
 * @brief
 *   Generate a seeded random number using OS calls.
 *
 * @param - None.
 *
 * @return - fbe_u32_t
 *   This function returns a 32 bit random number.
 *
 * @author
 *   07/20/09: Rob Foley - Created.
 *
 ****************************************************************/
fbe_u32_t fbe_random(void)
{
/* A random increment value...this can be any fixed nonzero value.
 */
    csx_timestamp_value_t tempTime; /* Temporary storage variable for systemTime */
    csx_timestamp_frequency_t ClockFrequency;
    fbe_u32_t random_value;

    csx_p_get_timestamp_frequency(&ClockFrequency);
    csx_p_get_timestamp_value(&tempTime);
    random_value = (fbe_u32_t)(tempTime % ClockFrequency);
    return random_value;
}
/**************************************
 * end fbe_random()
 **************************************/
/****************************************************************
 * fl_random_64
 ****************************************************************
 * @brief
 *   Generate a seeded random number using OS calls.
 *
 * @param - none.
 * 
 * @notes
 *   This function does not return a "truly" random 64 bit 
 *   number, since it is concatenating two 32 bit random
 *   numbers together.  This is good enough for our uses.
 *
 * @return fbe_u64_t
 *   This function returns a 64 bit random number.
 *
 * @author
 *   07/20/09: Rob Foley - Created.
 *
 ****************************************************************/
fbe_u64_t fbe_random_64(void)
{    
    static csx_timestamp_value_t last_time;
    fbe_u64_t low_32_bits;
    fbe_u64_t high_32_bits;

    csx_timestamp_value_t current_time; /* Temporary storage variable for systemTime */
    csx_timestamp_frequency_t clock_frequency;
    
    /* We have two random numbers which we will use for different
     * portions of the 64 bit random number.
     * 
     * 1) The difference between the last time and the current time.
     * 2) The remainder of dividing the current ticks
     *    by the clock frequency.
     *
     * Since both of the above values will only use 32 bits of their
     * 64 (really 63 since signed) number, we'll combine them
     * to get a good 64 bit random number.
     */
    csx_p_get_timestamp_value(&current_time);
    csx_p_get_timestamp_frequency(&clock_frequency);
    
    /* ~50% of the time we'll put 1) above in the low word and 
     * otherwise we'll put 1) in the low word.
     */
    if ( current_time % 2 )
    {
        low_32_bits = current_time % clock_frequency;
        high_32_bits = current_time - last_time;
    }
    else
    {
        high_32_bits = current_time % clock_frequency;
        low_32_bits = current_time - last_time;
    }
    
    /* Keep track of the current time for next time.
     */
    last_time = current_time;

    /* Shift over the high 32 bits so they take up the high part
     * of the 64 bit word.
     */
    high_32_bits <<= 32;
    
    /* We intentionally do not mask off the high 32 bits of low_32_bits,
     * it's just not necessary since we're creating a random number.
     */
    return (high_32_bits | low_32_bits);
}
/**************************************
 * end fbe_random_64()
 **************************************/

/*************************
 * end file fbe_random_kernel_main.c
 *************************/


