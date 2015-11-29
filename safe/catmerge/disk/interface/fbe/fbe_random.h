#ifndef __FBE_RANDOM_H__
#define __FBE_RANDOM_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_random.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of the fbe random number API.
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
fbe_u32_t fbe_random(void);
fbe_u64_t fbe_random_64(void);
void fbe_random_set_seed(fbe_u32_t seed);
fbe_u64_t fbe_rtl_random_64(fbe_u32_t *seed_p);
fbe_u32_t fbe_rtl_random(fbe_u32_t *seed_p);

/* Return a random number in the range 0..max_number.
 */
static __forceinline fbe_u64_t fbe_random_64_with_range(fbe_u32_t *seed_p, fbe_u64_t max_number)
{
    if (max_number > FBE_U32_MAX)
    {
        return fbe_rtl_random_64(seed_p) % max_number;
    }
    else
    {
        /* We don't need a 64 bit random number so just generate a 32 bit one.
         */
        fbe_u64_t rand_num = fbe_rtl_random(seed_p);
        return rand_num % max_number;
    }
}
/*************************
 * end file fbe_random.h
 *************************/

#endif /* end __FBE_RANDOM_H__ */
