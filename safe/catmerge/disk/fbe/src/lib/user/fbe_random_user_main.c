/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_random_user_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains user mode definitions of the fbe random methods.
 *  This code was ported from flare's timestamp.c and timestamp.h.
 *
 * @version
 *   7/20/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @var b_set_seed
 *********************************************************************
 * @brief TRUE if we have setup the seed already.
 *
 *********************************************************************/
static fbe_bool_t b_set_seed = FBE_FALSE;

/*!***************************************************************
 * fbe_random_set_seed()
 ****************************************************************
 * @brief
 *   Set the seed for our random numbers.
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
    void __cdecl srand(unsigned int seed);

    srand(seed);
    b_set_seed = FBE_TRUE;
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
    fbe_u32_t random_value = 0;
    int __cdecl rand (void);
    void __cdecl srand (unsigned int seed);

    if (!b_set_seed)
    {
        srand(0);
        b_set_seed = FBE_TRUE;
    }
    
    /* rand() returns from 1 to RAND_MAX (0x7FFF), which only provides a 15 bit
     * random value.  Call it multiple times and shift the bits to provide 
     * a 32 bit random value.
     */
    random_value = rand();
    random_value |= (rand() << 15);
    random_value |= (rand() << 30);

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
    fbe_u64_t low_32_bits;
    fbe_u64_t high_32_bits;
/* A random increment value...this can be any fixed nonzero value.
 */
    /* Just use the canned fl_random to get two random 32 bit words.
     * This works quite well in user mode, so we can simply put 
     * these two together to get a 64 bit random number. 
     */
    low_32_bits = fbe_random();
    high_32_bits = fbe_random();
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

/****************************************************************
 * fbe_rtl_random()
 ****************************************************************
 * @brief
 *   Generate a seeded random number using OS calls.
 *
 * @param seed_p - Ptr to seed value.
 * 
 * @return fbe_u64_t
 *   This function returns a 32 bit random number.
 *
 * @author
 *  4/23/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_u32_t fbe_rtl_random(fbe_u32_t *seed_p)
{    
    return fbe_random();
}
/**************************************
 * end fbe_rtl_random()
 **************************************/
/****************************************************************
 * fbe_rtl_random_64()
 ****************************************************************
 * @brief
 *   Generate a seeded random number using OS calls.
 *
 * @param seed_p - Ptr to seed value.
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
 *  4/19/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_u64_t fbe_rtl_random_64(fbe_u32_t *seed_p)
{ 
    return fbe_random_64();
}
/**************************************
 * end fbe_rtl_random_64()
 **************************************/
/*************************
 * end file fbe_random_user_main.c
 *************************/


