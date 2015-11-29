#ifndef CPPSH_DRIVER_H
#define CPPSH_DRIVER_H

//**********************************************************************
// Copyright  (C) EMC Corporation  2001
// All Rights Reserved
// Licensed material - Property of EMC Corporation
//**********************************************************************

//**********************************************************************
// $id:$
//**********************************************************************
//
//  DESCRIPTION:
//
//      This header file defines the interface to the C++ Library wrapper
//      which is used to implement Kernel drivers using C++.  This
//      allows the creation of global objects and overrides for the basic
//      memory allocation functions.
//
//  NOTES:
//
//  HISTORY:
//      $Log:$
//
//**********************************************************************

//**********************************************************************
// INCLUDE FILES
//**********************************************************************
#include "k10ntddk.h"
#include "EmcPAL_MemoryAlloc.h"
#include "EmcPAL_DriverShell.h"
#include "EmcPAL_Memory.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "k10ntddk.h"
#include "cmm.h"

#ifdef __cplusplus
}
#endif


//**********************************************************************
// MACRO DEFINITIONS
//**********************************************************************
#ifndef CPPSH_NEW_TAG
#define CPPSH_NEW_TAG 'ppc0' /* Default tag: 0cpp */
#endif

//**********************************************************************
// STRING LITERALS
//**********************************************************************

//**********************************************************************
// TYPE DEFINITIONS
//**********************************************************************
//**********************************************************************
// IMPORTED VARIABLE DEFINITIONS
//**********************************************************************

//**********************************************************************
// IMPORTED FUNCTION DEFINITIONS
//**********************************************************************

#ifdef __cplusplus
extern "C" {
#endif


    //
    //  This provides the prototype for the driver which incorporates this
    //  library.  The specified driver must create a function of this name
    //  instead of the standard "DriverEntry" routine.
    //
	EMCPAL_STATUS CppDriverEntry(
        PEMCPAL_NATIVE_DRIVER_OBJECT                          DriverObject,
        PCHAR                        RegistryPath );


#ifdef __cplusplus
}
#endif

//**********************************************************************
// IMPORTED CLASS DEFINITIONS
//**********************************************************************

//**********************************************************************
// FUNCTION DEFINITIONS
//**********************************************************************

#ifdef __cplusplus

#pragma warning(disable : 4291)

#ifndef ALAMOSA_WINDOWS_ENV
#define CPPSH_DRIVER_NEW_DELETE_DEFINED
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - unsure why this is needed */

//
//  Memory allcation support required by VisualC++
//
void * __cdecl operator new(size_t size);
#ifndef ALAMOSA_WINDOWS_ENV
void * __cdecl operator new[](size_t size);
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - need array versions of new/delete */

//
//void * __cdecl operator new[](size_t size);
//
void * __cdecl operator new(
    size_t                                      size, 
    void                                        *pBuffer );


void __cdecl operator delete( 
    void                                        *pBuffer );
#ifndef ALAMOSA_WINDOWS_ENV
void __cdecl operator delete[]( 
    void                                        *pBuffer );
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - need array versions of new/delete */

//void __cdecl operator delete[](void *pBuffer);


//
//  Version of new added for client use
//
void * __cdecl operator new(
    size_t                                      size, 
    ULONG                                       tag, 
    EMCPAL_POOL_TYPE                                   pool = EmcpalNonPagedPool );

//void * __cdecl operator new[](size_t size, ULONG tag, POOL_TYPE pool=EmcpalNonPagedPool);

#endif

extern void __cdecl CppUseNonPagedPool();

#if !defined(UMODE_ENV)
/*
 * Define  DISABLE_CPP_MEMORY_MANAGEMENT can only be used in Test code
 * When set, cppshdriver is disabled, causing allocations to be to the heap
 *
 * This allows usage of C++ Strings and STL templates to be used 
 * Note, once set, instances created within production code can not be
 * free'ed (delete) within test code and instances created within test code
 * can not be freed (delete) in production code.
 * 
 * Instance memory must be managed carefully
 */
#ifdef I_AM_DEBUG_EXTENSION
#define DISABLE_CPP_MEMORY_MANAGEMENT
#endif
#ifndef DISABLE_CPP_MEMORY_MANAGEMENT

PVOID __cdecl cppmalloc(
    ULONG                                     size, 
    ULONG                                       tag = 'ppc0', 
    EMCPAL_POOL_TYPE                                   pool = EmcpalNonPagedPool );


void __cdecl cppfree(PVOID pBuffer);


#ifdef __cplusplus
CSX_FINLINE void * __cdecl operator new(size_t size)
{
	PVOID pBuffer;

#if defined(CPPCMM_H)
	// Attempt to allocate memory
	pBuffer = cppmalloc((ULONG)size, CPPSH_NEW_TAG, EmcpalNonPagedPool);
#else
    pBuffer = EmcpalAllocateUtilityPoolWithTag(EmcpalNonPagedPool, size, CPPSH_NEW_TAG);
#endif // defined(CPPCMM_H)

	// If memory allocation was successful, then zero it.
	if (pBuffer != NULL)
	{
		EmcpalZeroMemory(pBuffer, size);
	}

	return pBuffer;
}
#ifndef ALAMOSA_WINDOWS_ENV
CSX_FINLINE void * __cdecl operator new[](size_t size)
{
	PVOID pBuffer;

#if defined(CPPCMM_H)
	// Attempt to allocate memory
	pBuffer = cppmalloc((ULONG)size, CPPSH_NEW_TAG, EmcpalNonPagedPool);
#else
    pBuffer = EmcpalAllocateUtilityPoolWithTag(EmcpalNonPagedPool, size, CPPSH_NEW_TAG);
#endif // defined(CPPCMM_H)

	// If memory allocation was successful, then zero it.
	if (pBuffer != NULL)
	{
		EmcpalZeroMemory(pBuffer, size);
	}

	return pBuffer;
}
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - need array versions of new/delete */


CSX_FINLINE void * __cdecl operator new(
    size_t                                      size, 
    void                                        *pBuffer )
{
	return( pBuffer );
}
#ifndef ALAMOSA_WINDOWS_ENV
CSX_FINLINE void * __cdecl operator new[](
    size_t                                      size, 
    void                                        *pBuffer )
{
	return( pBuffer );
}
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - need array versions of new/delete */


//__inline void * __cdecl operator new[](size_t size)
//{
//	return cppmalloc(size, 'ppc0', EmcpalNonPagedPool);
//}

CSX_FINLINE void * __cdecl operator new(size_t size, ULONG tag, EMCPAL_POOL_TYPE pool)
{
	PVOID pBuffer;

#if defined(CPPCMM_H)
	// Attempt to allocate memory
	pBuffer = cppmalloc((ULONG)size, tag, pool);
#else
    pBuffer = EmcpalAllocateUtilityPoolWithTag(pool, size, tag);
#endif // defined(CPPCMM_H)

	// If memory allocation was successful, then zero it.
	if (pBuffer != NULL)
	{
		EmcpalZeroMemory(pBuffer, size);
	}

	return pBuffer;
}

//__inline void * __cdecl operator new[](size_t size, ULONG tag, EMCPAL_POOL_TYPE pool)
//{
//	return cppmalloc(size, tag, pool);
//}

CSX_FINLINE void  __cdecl operator delete(void *pBuffer)
{
	// If NULL not sent in, then assume it's a valid memory block,
	// free it.
	if (pBuffer != NULL)
	{
#if defined(CPPCMM_H)
		cppfree(pBuffer);
#else
	    EmcpalFreeUtilityPool(pBuffer);
#endif // defined(CPPCMM_H)
	}
}
#ifndef ALAMOSA_WINDOWS_ENV
CSX_FINLINE void  __cdecl operator delete[](void *pBuffer)
{
	// If NULL not sent in, then assume it's a valid memory block,
	// free it.
	if (pBuffer != NULL)
	{
#if defined(CPPCMM_H)
		cppfree(pBuffer);
#else
	    EmcpalFreeUtilityPool(pBuffer);
#endif // defined(CPPCMM_H)
	}
}
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - need array versions of new/delete */

//__inline void  __cdecl operator delete[](void *pBuffer)
//{
//	cppfree(pBuffer);
//}

#endif  // __cplusplus

#endif // DISABLE_CPP_MEMORY_MANAGEMENT
#endif //Disabled for UMODE_ENV

#if defined(__cplusplus)
CSX_FINLINE EMCPAL_STATUS cppInitializeCmmMemory(const PCHAR RegistryPath)
{
	EMCPAL_STATUS	status	= EMCPAL_STATUS_SUCCESS;

#if defined(CPPCMM_H)
	status = CppInitializeCmm(RegistryPath);
#endif // defined(CPPCMM_H)

	return (status);
}

CSX_FINLINE BOOLEAN cppDestroyCmmMemory(void)
{
	BOOLEAN	success = TRUE;

#if defined(CPPCMM_H)
    if(cppIsCmmInitialized()) {
	    success = cppshDestroyCmm();
        cppSetCmmInitialized(FALSE);
    }
#endif // defined(CPPCMM_H)

	return (success);
}

#endif // defined(__cplusplus)

//**********************************************************************
// IMPORTED VARIABLE DEFINITIONS
//**********************************************************************

//**********************************************************************
// CLASS DEFINITIONS
//**********************************************************************


#endif // CPPSH_DRIVER_H
