#ifndef FBE_XML_UTIL_H
#define FBE_XML_UTIL_H

#include "fbe/fbe_api_common_transport.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Do not use these functions.  They are just here to support some legacy XML code.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

__forceinline static void * fbe_reallocate_memory(void* pSrc, ULONG OldSize, ULONG NewSize)
{
    void* pDest = NULL;

    pDest = fbe_api_allocate_memory(NewSize);
    if ( pDest )
    {
        // Now copy from old to new and free the old location.
        fbe_copy_memory(pDest, pSrc, OldSize);
        fbe_api_free_memory(pSrc);
    }

    return pDest;
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Do not use these functions.  They are just here to support some legacy XML code.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

__forceinline static char* fbe_strdup(const char * pSrc)
{
    size_t len = 0;
    char* pDest = NULL;

    len = strlen(pSrc);
    pDest = fbe_api_allocate_memory((ULONG)len + 1);
    if ( pDest )
    {
        fbe_copy_memory(pDest, pSrc, (fbe_u32_t)len);
        pDest[len] = '\0';
    }

    return pDest;
}

#endif /*FBE_XML_UTIL_H*/
