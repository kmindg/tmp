#ifndef CPPCMM_H
#define CPPCMM_H

#if defined(CPPSH_DRIVER_H)
#error "cppshDriver.h must NOT be included before cppcmm.h"
#endif // defined(CPPSH_DRIVER_H)

//**********************************************************************
// Copyright  (C) EMC Corporation  2001
// All Rights Reserved
// Licensed material - Property of EMC Corporation
//**********************************************************************

//**********************************************************************
// $id:$
//**********************************************************************
//
// DESCRIPTION:
//	  Separates the usage of CMM from the usage of the CPP shell driver
//	  for those C++ drivers that do not want to or cannot use CMM.
//
// NOTES:
//
// HISTORY:
//    $Log:$
//
//**********************************************************************

//**********************************************************************
// INCLUDE FILES
//**********************************************************************
#ifdef __cplusplus
extern "C" {
#endif

#include "k10ntddk.h"
#include "cmm.h"

extern			EMCPAL_STATUS	CppInitializeCmm( const PCHAR );
extern CMM_CLIENT_ID __cdecl CppGetCmmClientID();

#ifdef __cplusplus
}
#endif

extern __inline BOOLEAN		CppAllocCmm(PVOID *pBuffer, size_t allocSize);
extern __inline BOOLEAN		CppAllocCmmAligned(PVOID *pBuffer,
                                               PVOID *pOrigBuffer,
                                               size_t allocSize,
                                               size_t alignment);
extern __inline BOOLEAN		CppFreeCmm(PVOID buffer);
extern			BOOLEAN		cppshDestroyCmm(void);
extern			BOOLEAN		CppInitializeCmmPool( CMM_MEMORY_SIZE mMemoryNeeded,
									              char *pLongTermClient,
												  char *pPoolClient );

extern PVOID		_cdecl cppmalloc(ULONG size, ULONG tag, EMCPAL_POOL_TYPE pool);
extern PVOID        _cdecl cppmalloc_aligned(ULONG size, ULONG align, PVOID *pPtrToFree);
extern void		    _cdecl cppfree(PVOID pBuffer);
void	cppSetCmmInitialized(BOOLEAN	initialized);
BOOLEAN	cppIsCmmInitialized(void);

#include "cppshdriver.h"

//**********************************************************************
// MACRO DEFINITIONS
//**********************************************************************

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

//**********************************************************************
// IMPORTED CLASS DEFINITIONS
//**********************************************************************

//**********************************************************************
// FUNCTION DEFINITIONS
//**********************************************************************

//
// Allows drivers to setup their own CMM POOL.
// By default the cppsh shell uses NT.
// However, by calling this function, the driver can 
// setup it's own CMM pool.
//
BOOLEAN CppInitializeCmmPool( CMM_MEMORY_SIZE mMemoryNeeded,
                              char *pLongTermClient,
                              char *pPoolClient );

extern CMM_POOL_ID __cdecl CppGetCmmPoolID();

//**********************************************************************
// IMPORTED VARIABLE DEFINITIONS
//**********************************************************************

//**********************************************************************
// CLASS DEFINITIONS
//**********************************************************************


#endif // CPPCMM_H
