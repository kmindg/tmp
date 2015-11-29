#ifndef EMCUTIL_RESOURCE_H_
#define EMCUTIL_RESOURCE_H_

//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*!
 * @file EmcUTIL_Resource.h
 * @addtogroup emcutil_resource
 * @{
 * @brief
 *
 */

#include "EmcUTIL.h"

#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV))

#ifndef ALAMOSA_WINDOWS_ENV

typedef PVOID EMCUTIL_RESOURCE_THREAD;
typedef struct {
    LONG ActiveCount;
    PVOID lowner_exclusive;
    PVOID lowner_shared;
    csx_nsint_t ldepth_exclusive;
    csx_nsint_t ldepth_shared;
    csx_p_raw_rwmut_t backing_raw_rwmut;
} EMCUTIL_RESOURCE,
*PEMCUTIL_RESOURCE;

CSX_STATIC_INLINE EMCUTIL_RESOURCE_THREAD
EmcutilGetCurrentResourceThread(VOID)
{
    return (EMCUTIL_RESOURCE_THREAD)csx_p_get_current_thread_handle();
}

CSX_STATIC_INLINE csx_status_e
EmcutilInitializeResource(
    PEMCUTIL_RESOURCE Resource)
{
    csx_status_e status;
    status = csx_p_raw_rwmut_create_maybe(&Resource->backing_raw_rwmut, CSX_NULL);
    if (CSX_SUCCESS(status)) {
        Resource->lowner_exclusive = CSX_NULL;
        Resource->lowner_shared = CSX_NULL;
        Resource->ldepth_exclusive = CSX_ZERO;
        Resource->ldepth_shared = CSX_ZERO;
        Resource->ActiveCount = 0;
    }
    CSX_ASSERT_SUCCESS_H_RDC(status);
    return CSX_STATUS_SUCCESS;
}

CSX_STATIC_INLINE csx_status_e
EmcutilDeleteResource(
    PEMCUTIL_RESOURCE Resource)
{
    Resource->lowner_exclusive = CSX_NULL;
    Resource->lowner_shared = CSX_NULL;
    Resource->ldepth_exclusive = CSX_ZERO;
    Resource->ldepth_shared = CSX_ZERO;
    Resource->ActiveCount = 0;
    csx_p_raw_rwmut_destroy(&Resource->backing_raw_rwmut);
    return CSX_STATUS_SUCCESS;
}

CSX_STATIC_INLINE BOOLEAN
EmcutilAcquireResourceExclusive(
    PEMCUTIL_RESOURCE Resource,
    BOOLEAN Wait)
{
    csx_status_e status;
    if (Resource->lowner_exclusive == EmcutilGetCurrentResourceThread()) {
        CSX_ASSERT_S_DC(Resource->ldepth_exclusive > 0);
        Resource->ldepth_exclusive++;
        csx_p_atomic_inc((volatile csx_atomic_t *) &Resource->ActiveCount);
    } else {
        if (!Wait) {
            status = csx_p_raw_rwmut_trywlock(&Resource->backing_raw_rwmut);
        } else {
            csx_p_raw_rwmut_wlock(&Resource->backing_raw_rwmut);
            status = CSX_STATUS_SUCCESS;
        }
        if (status == CSX_STATUS_SUCCESS) {
            CSX_ASSERT_S_DC(Resource->ldepth_exclusive == CSX_ZERO);
            Resource->lowner_exclusive = EmcutilGetCurrentResourceThread();
            Resource->ldepth_exclusive++;
            csx_p_atomic_inc((volatile csx_atomic_t *) &Resource->ActiveCount);
        } else {
            return FALSE;
        }
    }
    return TRUE;
}

CSX_STATIC_INLINE BOOLEAN
EmcutilAcquireResourceShared(
    PEMCUTIL_RESOURCE Resource,
    BOOLEAN Wait)
{
    csx_status_e status;
    if (Resource->lowner_shared == EmcutilGetCurrentResourceThread()) {
        CSX_ASSERT_S_DC(Resource->ldepth_shared > 0);
        Resource->ldepth_shared++;
        csx_p_atomic_inc((volatile csx_atomic_t *) &Resource->ActiveCount);
    } else {
        if (!Wait) {
            status = csx_p_raw_rwmut_tryrlock(&Resource->backing_raw_rwmut);
        } else {
            csx_p_raw_rwmut_rlock(&Resource->backing_raw_rwmut);
            status = CSX_STATUS_SUCCESS;
        }
        if (status == CSX_STATUS_SUCCESS) {
            CSX_ASSERT_S_DC(Resource->ldepth_shared == CSX_ZERO);
            Resource->lowner_shared = EmcutilGetCurrentResourceThread();
            Resource->ldepth_shared++;
            csx_p_atomic_inc((volatile csx_atomic_t *) &Resource->ActiveCount);
        } else {
            return FALSE;
        }
    }
    return TRUE;
}

CSX_STATIC_INLINE VOID
EmcutilReleaseResourceForThread(
    PEMCUTIL_RESOURCE Resource,
    EMCUTIL_RESOURCE_THREAD ResourceThreadId)
{
    if (ResourceThreadId == Resource->lowner_exclusive) {
        CSX_ASSERT_S_DC(Resource->ldepth_exclusive > 0);
        Resource->ldepth_exclusive--;
        csx_p_atomic_dec((volatile csx_atomic_t *) &Resource->ActiveCount);
        if (Resource->ldepth_exclusive == CSX_ZERO) {
            Resource->lowner_exclusive = CSX_NULL;
            csx_p_raw_rwmut_wunlock(&Resource->backing_raw_rwmut);
        } else {
        }
    } else if (ResourceThreadId == Resource->lowner_shared) {
        CSX_ASSERT_S_DC(Resource->ldepth_shared > 0);
        Resource->ldepth_shared--;
        csx_p_atomic_dec((volatile csx_atomic_t *) &Resource->ActiveCount);
        if (Resource->ldepth_shared == CSX_ZERO) {
            Resource->lowner_shared = CSX_NULL;
            csx_p_raw_rwmut_runlock(&Resource->backing_raw_rwmut);
        } else {
        }
    } else {
        CSX_PANIC();
    }
}

CSX_STATIC_INLINE VOID
EmcutilResourceEnterCriticalRegion(
    VOID)
{   
/* XXX - OK TO DO NOTHING - XXX */
}

CSX_STATIC_INLINE VOID
EmcutilResourceLeaveCriticalRegion(
    VOID)
{   
/* XXX - OK TO DO NOTHING - XXX */
}

#else
#define EMCUTIL_RESOURCE_THREAD ERESOURCE_THREAD
typedef ERESOURCE EMCUTIL_RESOURCE, *PEMCUTIL_RESOURCE;
#define EmcutilGetCurrentResourceThread ExGetCurrentResourceThread
#define EmcutilInitializeResource ExInitializeResourceLite
#define EmcutilDeleteResource ExDeleteResourceLite
#define EmcutilAcquireResourceExclusive ExAcquireResourceExclusiveLite
#define EmcutilAcquireResourceShared ExAcquireResourceSharedLite
#define EmcutilReleaseResourceForThread ExReleaseResourceForThreadLite
#define EmcutilResourceEnterCriticalRegion KeEnterCriticalRegion
#define EmcutilResourceLeaveCriticalRegion KeLeaveCriticalRegion
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

#endif

//++
//.End file EmcUTIL_Resource.h
//--

/*!
 * @} end group emcutil_resource
 * @} end file EmcUTIL_Resource.h
 */

#endif                                     /* EMCUTIL_RESOURCE_H_ */
