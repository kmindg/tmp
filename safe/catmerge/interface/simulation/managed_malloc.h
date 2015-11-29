/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               managed_malloc.h
 ***************************************************************************
 *
 * DESCRIPTION:  replacemetns for malloc/free used to monitor memory management
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    03/01/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _MMALLOC_
#define _MMALLOC_

#if defined(SIMMODE_ENV) || defined(UMODE_ENV)

# include "generic_types.h"
# include "csx_ext.h"
# include "EmcPAL_Misc.h"

#ifdef MANAGEDMALLOCEXPORT
#define MANAGEDMALLOCPUBLIC CSX_MOD_EXPORT 
#else
#define MANAGEDMALLOCPUBLIC CSX_MOD_IMPORT 
#endif


#ifdef __cplusplus
class AbstractMemoryManager {
public:
    virtual BOOL   isEnabled() = 0;
    virtual UINT_32 getNoTargets() = 0;

    virtual void * managed_malloc(UINT_32 size, UINT_32 pad) = 0;
    virtual void   managed_free(void *pBuffer) = 0;
    virtual void * managed_new(UINT_32 size) = 0;;
    virtual void   managed_delete(void *pBuffer) = 0;
    virtual void   managed_init(BOOL b) = 0;
    virtual void   validate() = 0;
    virtual void report() = 0;
    virtual BOOL   getTarget(UINT_32 index, UINT_32 *size, void **address) = 0;
};

MANAGEDMALLOCPUBLIC AbstractMemoryManager *getMemoryManager();
#endif


#ifdef __cplusplus
extern "C" {
#endif

MANAGEDMALLOCPUBLIC void * managed_malloc(UINT_32 size);
MANAGEDMALLOCPUBLIC void * managed_malloc_padded(UINT_32 size, UINT_32 pad);
MANAGEDMALLOCPUBLIC void * managed_realloc(void *pBuffer, UINT_32 size);
MANAGEDMALLOCPUBLIC void managed_free(void *pBuffer);

MANAGEDMALLOCPUBLIC void managed_malloc_init(BOOL b);
MANAGEDMALLOCPUBLIC void managed_malloc_report();
MANAGEDMALLOCPUBLIC void managed_malloc_validate();
MANAGEDMALLOCPUBLIC void managed_malloc_register_callbacks(void (*mallocBegin)(), void (*mallocEnd)(), void (*freeBegin)(), void(*freeEnd)());

#ifdef __cplusplus
};
#endif

/* CGCG - there is a bad issue with this style of override and the EmcUTIL Shell mechanism */
#ifdef USE_MANAGED_MALLOC
#undef malloc
#define malloc(a)   managed_malloc((UINT_32)a)

#undef free
#define free(a)     managed_free(a)

#undef realloc
#define realloc(a,b) managed_realloc(a, b)
#endif /* USE_MANAGED_MALLOC */

#endif  // SIMMODE_ENV

#endif // _MMALOC_
