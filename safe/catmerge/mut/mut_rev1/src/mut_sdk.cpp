/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_thread.c
 ***************************************************************************
 *
 * DESCRIPTION: MUT Framework threads
 *
 * FUNCTIONS:
 * NOTES:
 *
 * HISTORY:
 *     11/22/2007 Igor Bobrovnikov initial version
 *    01/18/2008 Igor Bobrovnikov mutexes added
 *
 **************************************************************************/

#include "mut_sdk.h"
#include "mut_private.h"
#include "mut_test_control.h"
#include "EmcPAL.h"

#include "EmcPAL_Misc.h"

extern Mut_test_control *mut_control;


/* The documentation for this function is specified in mut_sdk.h */
MUT_API void mut_mutex_init(mut_mutex_t *mutex)
{
    csx_p_mut_create_always(CSX_MY_MODULE_CONTEXT(), &mutex->mut, "mut_mutex");
}

/* The documentation for this function is specified in mut_sdk.h */
MUT_API void mut_mutex_destroy(mut_mutex_t *mutex)
{
    csx_p_mut_destroy(&mutex->mut);
}

/* The documentation for this function is specified in mut_sdk.h */
MUT_API void mut_mutex_acquire(mut_mutex_t *mutex)
{
    csx_p_mut_lock(&mutex->mut);
}

/* The documentation for this function is specified in mut_sdk.h */
MUT_API void mut_mutex_release(mut_mutex_t *mutex)
{
    csx_p_mut_unlock(&mutex->mut);
}

/* The documentation for this function is specified in mut_sdk.h */
MUT_API void mut_sleep(unsigned long ms)
{
    csx_p_thr_sleep_msecs(ms);
}

/* The documentation for this function is specified in mut_sdk.h */
MUT_API unsigned long mut_tickcount_get(void)
{
    csx_u64_t current_time_msecs;
    csx_p_get_clock_time_msecs(&current_time_msecs); 

    return (unsigned long)current_time_msecs;
}

/* The documentation for this function is specified in mut_sdk.h */
MUT_API char *mut_get_host_name(void)
{
    DWORD size;
    char buffer[MUT_STRING_LEN];

    size = sizeof(buffer) - 1;
    csx_p_native_system_name_get(buffer, size, 0);
    return  _strdup(buffer);
}

/* The documentation for this function is specified in mut_sdk.h */
MUT_API char *mut_get_user_name(void)
{
    DWORD size;

    char buffer[MUT_STRING_LEN];

    size = sizeof(buffer) - 1;
    csx_p_native_user_name_get(buffer, size, 0);
    return  _strdup(buffer);
}
