// userdefs.h
//
// Copyright (C) 1997-2009	EMC Corporation
//
//
// Various global definitions
//
//
//	Revision History
//	----------------
//	10 Jul 97	D. Zeryck	Initial version.
//	29 Sep 98	D. Zeryck	Added byteswap routines
//	21 Dec 98	D. Zeryck	Make THROW_MEMORY_ERROR throw a K10 exception
//	13 Jan 99	D. Zeryck	Add THROW_DB_INCONSISTANCY_ERROR
//	23 Sep 99	D. Zeryck	Add swap64
//	14 Oct 99	D. Zeryck	Move K10Exception out for Navi
//	28 Feb 00	H. Weiner	Add THROW_K10_EXCEPTION_EX()
//	20 Jun 00	H. Weiner	Add THROW_K10_DRIVER_EXCEPTION()
//	29 Jun 00	D. Zeryck	Add safe sprintfs
//	16 Aug 00	D. Zeryck	Move INIT_K10TRACE to userdefs & add string default
//	27 Oct 00	H. Weiner	Add 3, 4, 5 arg versions of the SAFE_SPRINTF_ macro
//	31 Oct 00	H. Weiner	Add 6, 7, 8 arg versions of the SAFE_SPRINTF_ macro
//	 3 Aug 01	H. Weiner	Add exception macros with the "hide" parameter
//  29 Oct 01   K. Keisling	Expand TLD_GETNUMBER to handle values > 32-bits
//  21 Nov 01   K. Keisling Add TLD_SETNUMBER
//	19 Mar 02	H. Weiner	DIMS 70273: AA_GET_NUMBER to fix endian problem
//   1 Apr 03   MSZ         Added SAFE_SPRINTF_10 to handle DumpManager enhancements in Release 12 (DIMS 85465)
//	11 Aug 03	M. Salha	DIMS 90636: Added GetFullPathProcName to extract full path name for the process 
//	24 Nov 08	E. Carter	Added new/delete operator methods
//	28 Oct 09	E. Carter	Added SAFE_STRNCAT macro
//	09 Feb 11	M. Gupta	Coverity changes.

#ifndef USERDEFS_H
#define USERDEFS_H

#include "K10Exception.h"
#include <stdio.h>			// snprintf
//#include <wchar.h>
#include "swap_exports.h"
#include "EmcPAL.h"
#include "EmcUTIL.h"
#include "k10defs.h"

#ifdef ALAMOSA_WINDOWS_ENV
#ifndef ASSERT
#ifdef _DEBUG
#define ASSERT( expr ) if (!(expr)) { __asm { int 3}}
#else
#define ASSERT( expr ) ;
#endif //_DEBUG
#endif // defined ASSERT
#else
#ifndef ASSERT
#define ASSERT(expr) CSX_ASSERT_H_DC(expr)
#endif
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE */

#define dosPrefix	"\\\\.\\"

// Length of "\Device\".
// Used to strip it off for replacement with dosPrefix.
#define K10_DEVICE_PREFIX_LENGTH 8

#ifdef _MSC_VER
// disable the bool to int warning
#pragma warning( disable : 4800 )
// Inlines for byteswapping
#endif //_MSC_VER

////////////////////////////////////////////////////
// Macro for initializing K10TraceMsg
// Assumes CaptivePointer is already included, as it is an arg
#include <string.h>

// Get full path and file name of the process (using ANSI format) 
inline void GetFullPathProcNameA(char *pCmdLine, char **pProcName){
    char ideSeps[]   = "\"\0";
    char cmdLineSeps[] = " \t\n\r\0";
    *pProcName = pCmdLine;

    /* When using the MSVC IDE debugger the command line was prefixed with double-quote
       until the end of process name. ie. "psmtool" get src dest
       we have to take care of this condition*/
    if (**pProcName == '\"')
    {
        /* skip the first double-quote */
        (*pProcName)++;
        /* find the text before closing double-qoute or terminater
         the resulting text is our full path process name */
        strtok(*pProcName, ideSeps);
     }
     else
     {
        /* if runing process from the command line, the command line looks
           like this: <psmtool get src dest> so we look for the first white space
           or a terminator*/
        strtok(*pProcName, cmdLineSeps);
     }
}

#define INIT_K10TRACE( npTrace, pProc, defaultString ) {\
if (npTrace.IsNull()) {\
        char *pFullPathProcName = NULL;\
	char *pProcName = NULL;\
	csx_p_native_command_line_get(&pProcName);\
        if (pProcName == NULL) {\
            pProc = defaultString; \
        } \
        else { \
            GetFullPathProcNameA(pProcName,&pFullPathProcName);\
            pProc = strrchr(pFullPathProcName, '\\'); \
            if ( pProc == NULL ) \
            { \
                pProc = strrchr(pFullPathProcName, '/'); \
            } \
            if( pProc != NULL ) \
            { \
                pProc++; \
            } \
            else { \
                pProc = pProcName;\
            }\
            strtok(pProc, "."); \
        } \
        npTrace = new K10TraceMsg(pProc); \
        if (npTrace.IsNull()) { \
            THROW_MEMORY_ERROR(); \
        } \
    } \
} 

/////////////////////////////////////////////////////////////////////////////
// strncat will not add a null if string len == 'count'
//
#define SAFE_STRNCAT( dest, source, destLen ) \
    strncat(dest, source, destLen - strlen( dest) - 1);

#define SAFE_TCSNCAT( dest, source, destLen ) \
	 _tcsncat(dest, source, destLen - _tcslen( dest) - 1);
/////////////////////////////////////////////////////////////////////////////
// strncpy will not add a null if string len == 'count'
//
#define SAFE_STRNCPY( dest, source, destLen ) \
    {memset(dest, 0, destLen); \
    strncpy(dest, source, destLen -1);}

#define SAFE_TCSNCPY( dest, source, destLen ) \
	    {memset(dest, 0, destLen); \
    _tcsncpy(dest, source, destLen -1);}
/////////////////////////////////////////////////////////////////////////////
// sprintf will not add a null if string len == 'count'
//
#ifdef ALAMOSA_WINDOWS_ENV
#define SAFE_SPRINTF_0( dest, destLen, spec ) \
    {memset(dest, 0, destLen); \
    _snprintf(dest, destLen-1, spec);}

#define SAFE_SPRINTF_1( dest, destLen, spec, arg ) \
    {memset(dest, 0, destLen); \
    _snprintf(dest, destLen-1, spec, arg);}

#define SAFE_SPRINTF_2( dest, destLen, spec, arg1, arg2 ) \
    {memset(dest, 0, destLen); \
    _snprintf(dest, destLen-1, spec, arg1, arg2);}

#define SAFE_SPRINTF_3( dest, destLen, spec, arg1, arg2, arg3 ) \
    {memset(dest, 0, destLen); \
    _snprintf(dest, destLen-1, spec, arg1, arg2,arg3);}

#define SAFE_SPRINTF_4( dest, destLen, spec, arg1, arg2, arg3, arg4 ) \
    {memset(dest, 0, destLen); \
    _snprintf(dest, destLen-1, spec, arg1, arg2, arg3, arg4);}

#define SAFE_SPRINTF_5( dest, destLen, spec, arg1, arg2, arg3, arg4, arg5 ) \
    {memset(dest, 0, destLen); \
    _snprintf(dest, destLen-1, spec, arg1, arg2, arg3, arg4, arg5);}

#define SAFE_SPRINTF_6( dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6 ) \
    {memset(dest, 0, destLen); \
    _snprintf(dest, destLen-1, spec, arg1, arg2, arg3, arg4, arg5, arg6);}

#define SAFE_SPRINTF_7( dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7 ) \
    {memset(dest, 0, destLen); \
    _snprintf(dest, destLen-1, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7);}

#define SAFE_SPRINTF_8( dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 ) \
    {memset(dest, 0, destLen); \
    _snprintf(dest, destLen-1, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);}

#define SAFE_SPRINTF_9( dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 ) \
    {memset(dest, 0, destLen); \
    _snprintf(dest, destLen-1, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);}

#define SAFE_SPRINTF_10( dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10 ) \
    {memset(dest, 0, destLen); \
    _snprintf(dest, destLen-1, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);}

#define SAFE_SPRINTF_11( dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11 ) \
	{memset(dest, 0, destLen); \
	_snprintf(dest, destLen-1, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);}
#else /* SAFE */
static __inline void
SAFE_SPRINTF_CHECK(const char *spec, ...) __attribute__((format(__printf_func__,1,2)));
static __inline void
SAFE_SPRINTF_CHECK(const char *spec, ...)
{
        return ;
}
#define SAFE_SPRINTF_0( dest, destLen, spec ) \
    {memset(dest, 0, destLen); \
    SAFE_SPRINTF_CHECK(spec);  \
    _snprintf(dest, destLen, spec);}

#define SAFE_SPRINTF_1( dest, destLen, spec, arg ) \
    {memset(dest, 0, destLen); \
    SAFE_SPRINTF_CHECK(spec, arg);  \
    _snprintf(dest, destLen, spec, arg);}

#define SAFE_SPRINTF_2( dest, destLen, spec, arg1, arg2 ) \
    {memset(dest, 0, destLen); \
    SAFE_SPRINTF_CHECK(spec, arg1, arg2);  \
    _snprintf(dest, destLen, spec, arg1, arg2);}

#define SAFE_SPRINTF_3( dest, destLen, spec, arg1, arg2, arg3 ) \
    {memset(dest, 0, destLen); \
    SAFE_SPRINTF_CHECK(spec, arg1, arg2, arg3);  \
    _snprintf(dest, destLen, spec, arg1, arg2,arg3);}

#define SAFE_SPRINTF_4( dest, destLen, spec, arg1, arg2, arg3, arg4 ) \
    {memset(dest, 0, destLen); \
    SAFE_SPRINTF_CHECK(spec, arg1, arg2, arg3, arg4);  \
    _snprintf(dest, destLen, spec, arg1, arg2, arg3, arg4);}

#define SAFE_SPRINTF_5( dest, destLen, spec, arg1, arg2, arg3, arg4, arg5 ) \
    {memset(dest, 0, destLen); \
    SAFE_SPRINTF_CHECK(spec, arg1, arg2, arg3, arg4, arg5);  \
    _snprintf(dest, destLen, spec, arg1, arg2, arg3, arg4, arg5);}

#define SAFE_SPRINTF_6( dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6 ) \
    {memset(dest, 0, destLen); \
    SAFE_SPRINTF_CHECK(spec, arg1, arg2, arg3, arg4, arg5, arg6);  \
    _snprintf(dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6);}

#define SAFE_SPRINTF_7( dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7 ) \
    {memset(dest, 0, destLen); \
    SAFE_SPRINTF_CHECK(spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7);  \
    _snprintf(dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7);}

#define SAFE_SPRINTF_8( dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 ) \
    {memset(dest, 0, destLen); \
    SAFE_SPRINTF_CHECK(spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);  \
    _snprintf(dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);}

#define SAFE_SPRINTF_9( dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 ) \
    {memset(dest, 0, destLen); \
    SAFE_SPRINTF_CHECK(spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);  \
    _snprintf(dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);}

#define SAFE_SPRINTF_10( dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10 ) \
    {memset(dest, 0, destLen); \
    SAFE_SPRINTF_CHECK(spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);  \
    _snprintf(dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);}

#define SAFE_SPRINTF_11( dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11 ) \
	{memset(dest, 0, destLen); \
         SAFE_SPRINTF_CHECK(spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);  \
	_snprintf(dest, destLen, spec, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);}

#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - the CSX snprintf does the intent here already */

/////////////////////////////////////////////////////////////////////////////
// strdup can overwrite an existing pointer.  Only use this macro on pointers
// that are initialized to some value (or NULL).
//

#define SAFE_STRDUP( destination, source ) \
    {if ( NULL != destination ) { free( destination ); } \
	destination = _strdup( source );}

// Used to break processing at predefined points
//
#ifdef _DEBUG
typedef enum K10_USER_BREAK {
    K10_USER_BREAK_NONE = 0,
    K10_USER_BREAK_INSERT_1,	// SysAdmin: top guy unbound
    K10_USER_BREAK_INSERT_2,	// SysAdmin: top guy unbound, middle bound in
    K10_USER_BREAK_INSERT_3,	// SysAdmin: finished inserting but txn still extant.
    K10_USER_BREAK_REMOVE_1,	// SysAdmin: top guy unbound
    K10_USER_BREAK_REMOVE_2,	// SysAdmin: top guy unbound, middle unbound in
    K10_USER_BREAK_REMOVE_3,	// SysAdmin: finished inserting but txn still extant.
    K10_USER_BREAK_TXN_OPEN,	// GlobalMgmt: just opened the txn file
    K10_USER_BREAK_TXN_WRITE,	// GlobalMgmt: just wrote the txn file
    K10_USER_BREAK_TXN_READ,	// GlobalMgmt: just read the txn file
    K10_USER_BREAK_BIND_1,		// Redirector: after bind to flare & persist of txn, before devmap or assign
    K10_USER_BREAK_POST_REGEN,	// Redirector: before regen in post-op
    K10_USER_BREAK_SPOOF_1,		// SysAdmin: 
    K10_USER_BREAK_SPOOF_2,		// SysAdmin: 
    K10_USER_BREAK_SPOOF_3,		// SysAdmin: 
    K10_USER_BREAK_SPOOF_4,		// SysAdmin:
    K10_USER_BREAK_SPOOF_5,		// SysAdmin: 
    K10_USER_BREAK_SPOOF_6,		// SysAdmin: 
    K10_USER_BREAK_SPOOF_7,		// SysAdmin: 
    K10_USER_BREAK_CONS_1		// SysAdmin: 

};

#define K10_USER_BREAK_SAVE_WL_TXN	0x80000000  // SysAdmin: leave worklist transaction in place for testing
#endif


#define RETURN_IF_NOK( hr ) if (hr != 0L) { return hr;}

#define BUF_SIZE 2048

#define K10_REG_PATH "SOFTWARE\\" EMC_STRING

//
// Macros for throwing exceptions in an orderly fashion
//

#define THROW_K10_EXCEPTION( str, err ) {\
    K10Exception aK10Exp( str, __FILE__, __LINE__, err); \
        throw aK10Exp;}

#define THROW_K10_EXCEPTION_HIDE( str, err, HowToHide ) {\
    K10Exception aK10Exp( str, __FILE__, __LINE__, err); \
        aK10Exp.SetHide(HowToHide); throw aK10Exp;}

#define THROW_K10_EXCEPTION_EX( str, err, NTerr ) {\
    K10Exception aK10Exp( str, __FILE__, __LINE__, err); aK10Exp.SetNTErrorCode(NTerr); \
        throw aK10Exp;}

#define THROW_K10_EXCEPTION_HIDE_EX( str, err, NTerr, HowToHide ) {\
    K10Exception aK10Exp( str, __FILE__, __LINE__, err); aK10Exp.SetNTErrorCode(NTerr); \
        aK10Exp.SetHide(HowToHide); throw aK10Exp;}

//hardcode to global mgmt memory error code
//
#define THROW_MEMORY_ERROR() \
    THROW_K10_EXCEPTION_EX( "Memory error", 0x7600800C, EMCUTIL_SYS_ERROR_NOT_ENOUGH_MEMORY );

#define THROW_K10_DRIVER_EXCEPTION( str, err, defaultErr ) {\
    if( IS_K10_DRIVER_ERROR(err) ) \
    {	THROW_K10_EXCEPTION( str, MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(err) ) } \
    else { THROW_K10_EXCEPTION_EX(str, defaultErr, err); } }

// Macro for getting a number. It MUST be ascertained that the tag exists..
// This is only a partial solution as only tld data lengths of 4 bytes or
// smaller are handled correctly.  For those numbers we first get the result
// into a DWORD and cast that into a the container of some size.  We then check
// the container vs the DWORD number via an XOR to see if any significant bits
// were truncated.
//
// For data lengths greater than 4 bytes we simply declare an error if the length
// is greater than the container size even though all the more significant bytes
// may be zero.
//
// It is expected that the vast majority of, if not all, uses of this macro will involve
// DWORD data or smaller.

#define TLD_GETNUMBER( pTld, tag, container) \
{ \
    unsigned int __tldlen = pTld->GetLength(); \
    if (__tldlen == 0) { \
        THROW_K10_EXCEPTION_EX("No data supplied with tag", MMAN_ERROR_BAD_DATA, tag); \
    } \
    if (__tldlen <= 4) { \
        DWORD __tldnum = pTld->GetNumber(tag); \
        container = pTld->GetNumber(tag); \
        if (__tldnum ^ (DWORD)(container)) { \
            THROW_K10_EXCEPTION_EX("Data size too large for container", MMAN_ERROR_BAD_DATA, __tldlen); \
        } \
    } \
    else if (__tldlen <= sizeof(container)) { \
        memset( &(container), 0, sizeof(container) ); \
        memcpy( &(container), pTld->GetData(), __tldlen ); \
    } \
    else { \
        THROW_K10_EXCEPTION_EX("Data size too large for container", MMAN_ERROR_BAD_DATA, __tldlen); \
    } \
}

// Macro for setting numbers into a TLD - for now, Navi can't handle anything
// larger than 4 bytes
#define TLD_SETNUMBER( pTld, tag, value) \
{ \
    ULONGLONG num = (ULONGLONG) value; \
    pTld = new TLD(tag, num, true); \
}
// end TLD_SETNUMBER

// Allocation macro for new/delete as class methods with built in K10Exception throwing capability.
//
#define NEW_AND_DELETE_OPERATORS														\
void* operator new (size_t size)														\
{																						\
    void* p = ::operator new( size );													\
    if ( p == NULL ) {																	\
        THROW_K10_EXCEPTION_EX( "Memory error", 0x7600800C, EMCUTIL_SYS_ERROR_NOT_ENOUGH_MEMORY );	\
    }																					\
    return p;																			\
}																						\
                                                                                        \
void operator delete (void *p)															\
{																						\
    if ( p != NULL ) {																	\
        ::operator delete( p );															\
    }																					\
}

// End of Allocation macro

inline long admin_ioctl( EMCUTIL_RDEVICE_REFERENCE pdev,
						long Ioctl,
						void *inbuf,
						long size_in,
						void *outbuf,
						long size_out,
						csx_size_t *BytesReturned)
{
	long Result;
	Result = EmcutilRemoteDeviceIoctl(	pdev, Ioctl,inbuf, size_in, outbuf, size_out, BytesReturned);
	if ( !(EMCUTIL_SUCCESS(Result)) && IS_K10_DRIVER_ERROR(Result) )
	{
		Result = MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(Result);
	}
	return Result;
}

#endif // USERDEFS_H
