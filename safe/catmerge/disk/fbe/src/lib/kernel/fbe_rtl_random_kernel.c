/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rtl_random_kernel.c.c
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
#include "fbe/fbe_types.h"
#include "fbe/fbe_random.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/
#ifdef ALAMOSA_WINDOWS_ENV
#include <ntifs.h>
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
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
#ifdef ALAMOSA_WINDOWS_ENV
    return RtlRandomEx(seed_p);
#else
    return fbe_random();
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
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
#ifdef ALAMOSA_WINDOWS_ENV
    fbe_u64_t low_32_bits;
    fbe_u64_t high_32_bits;

    low_32_bits = RtlRandomEx(seed_p);
    high_32_bits = RtlRandomEx(seed_p);
    high_32_bits <<= 32;
    return (high_32_bits | low_32_bits);
#else
    return fbe_random_64();
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
}
/**************************************
 * end fbe_rtl_random_64()
 **************************************/

/*************************
 * end file fbe_rtl_random_kernel.c.c
 *************************/
