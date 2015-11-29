#if !defined(GENERIC_TYPES_H)
#define	GENERIC_TYPES_H 0x00000001

/***********************************************************************
 *  Copyright (C)  EMC Corporation 1989-1999,2001,2009
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ***********************************************************************/

/***************************************************************************
 * generic_types.h
 ***************************************************************************
 *
 * DESCRIPTION:
 *      Seperated from generics.h, these are only the types and definitions
 *      that are environment (userspace, kernelspace, C, C++, etc...) agnostic.
 *      This file may be included directly if the generic types are need, but
 *      environment independence is desired.
 *
 * NOTES:
 *      Do not add anything to this file with an external dependency or that
 *      breaks compatibility with either user space or kernel space modules.
 *
 * HISTORY:
 *      28-Jul-2005     Created.       -Matt Yellen
 *
 ***************************************************************************/

#ifdef ALAMOSA_WINDOWS_ENV
#include <basetsd.h>
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE */
#include "csx_ext.h"

/*******************************
 * LITERAL DEFINITIONS
 *******************************/

/*
 * General purpose sizes
 */
#define KILOBYTE        1024
#define MEGABYTE        1048576
#define GIGABYTE        1073741824
#define TERABYTE        1099511627776ULL

#define BITS_PER_BYTE   8

#define SECS_PER_HOUR               3600

#define UNSIGNED_MINUS_1            0xFFFFFFFFu
#define UNSIGNED_MINUS_2            0xFFFFFFFEu
#define UNSIGNED_MINUS_3            0xFFFFFFFDu
#define UNSIGNED_CHAR_MINUS_1       0xFFu
#define UNSIGNED_64_MINUS_1         CSX_CONST_U64(0xFFFFFFFFFFFFFFFF)
#define MAX_INT_32                  0xFFFFFFFF
#define MAX_UINT_32                 0xFFFFFFFFu
#define MAX_INT_64                  0xFFFFFFFFFFFFFFFF
#define MAX_UINT_64                 0xFFFFFFFFFFFFFFFFu

/*
 * Value "returned" by functions that return nothing
 * meaningful but have return values because they are part
 * of a family of function. (Typically, functions that have their
 * addresses stored in pointers.)
 */
#define DUMMY_VALUE                 0xDEADBEEF

#define TRUE  1
#define FALSE 0

#ifndef NULL
#define NULL  0
#endif

/* These boolean values are used to indicate the successful (or
 * unsuccessful) completion of an operation or routine.
 */
#ifdef ALAMOSA_WINDOWS_ENV
#define OK       TRUE
#define NOT_OK   FALSE
#else
enum {
    OK = TRUE,
    NOT_OK = FALSE
};
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - using defines causes collision with Unisphere use of the token OK */

/*  Generic BIT definitions
 */
//For 64-bit numbers
#define BIT63 ((ULONGLONG)1 << 63)
#define BIT62 ((ULONGLONG)1 << 62)
#define BIT61 ((ULONGLONG)1 << 61)
#define BIT60 ((ULONGLONG)1 << 60)
#define BIT59 ((ULONGLONG)1 << 59)
#define BIT58 ((ULONGLONG)1 << 58)
#define BIT57 ((ULONGLONG)1 << 57)
#define BIT56 ((ULONGLONG)1 << 56)
#define BIT55 ((ULONGLONG)1 << 55)
#define BIT54 ((ULONGLONG)1 << 54)
#define BIT53 ((ULONGLONG)1 << 53)
#define BIT52 ((ULONGLONG)1 << 52)
#define BIT51 ((ULONGLONG)1 << 51)
#define BIT50 ((ULONGLONG)1 << 50)
#define BIT49 ((ULONGLONG)1 << 49)
#define BIT48 ((ULONGLONG)1 << 48)
#define BIT47 ((ULONGLONG)1 << 47)
#define BIT46 ((ULONGLONG)1 << 46)
#define BIT45 ((ULONGLONG)1 << 45)
#define BIT44 ((ULONGLONG)1 << 44)
#define BIT43 ((ULONGLONG)1 << 43)
#define BIT42 ((ULONGLONG)1 << 42)
#define BIT41 ((ULONGLONG)1 << 41)
#define BIT40 ((ULONGLONG)1 << 40)
#define BIT39  ((ULONGLONG)1 <<  39)
#define BIT38  ((ULONGLONG)1 <<  38)
#define BIT37  ((ULONGLONG)1 <<  37)
#define BIT36  ((ULONGLONG)1 <<  36)
#define BIT35  ((ULONGLONG)1 <<  35)
#define BIT34  ((ULONGLONG)1 <<  34)
#define BIT33  ((ULONGLONG)1 <<  33)
#define BIT32  ((ULONGLONG)1 <<  32)

//For 32-bit numbers
#define BIT31 (1 << 31)
#define BIT30 (1 << 30)
#define BIT29 (1 << 29)
#define BIT28 (1 << 28)
#define BIT27 (1 << 27)
#define BIT26 (1 << 26)
#define BIT25 (1 << 25)
#define BIT24 (1 << 24)
#define BIT23 (1 << 23)
#define BIT22 (1 << 22)
#define BIT21 (1 << 21)
#define BIT20 (1 << 20)
#define BIT19 (1 << 19)
#define BIT18 (1 << 18)
#define BIT17 (1 << 17)
#define BIT16 (1 << 16)
#define BIT15 (1 << 15)
#define BIT14 (1 << 14)
#define BIT13 (1 << 13)
#define BIT12 (1 << 12)
#define BIT11 (1 << 11)
#define BIT10 (1 << 10)
#define BIT9  (1 <<  9)
#define BIT8  (1 <<  8)
#define BIT7  (1 <<  7)
#define BIT6  (1 <<  6)
#define BIT5  (1 <<  5)
#define BIT4  (1 <<  4)
#define BIT3  (1 <<  3)
#define BIT2  (1 <<  2)
#define BIT1  (1 <<  1)
#define BIT0  (1 <<  0)

/*********************************
 * STRUCTURE DEFINITIONS
 *********************************/

/* The following types are used to indicate both type and size of the
 * data storage.  The amount of storage will be at least the size indicated.
 * However, it may be more.  If the exact size is required, refer to the
 * next set of types.  These types may actually be defined to a larger type
 * if there are architectural reasons (such as performance) to make them that
 * that way.
 */
typedef __int64 INT_64;
typedef int INT_32;
typedef int INT_16;
#ifdef NEVER
typedef signed char INT_8;
#endif

typedef unsigned __int64 UINT_64;
typedef unsigned int UINT_32;
typedef unsigned UINT_16;
typedef unsigned char UINT_8;

typedef char TEXT;
typedef char XTEXT;		/* default char signedness */

/*******************************
 * WINDOWS-LIKE TYPES
 *******************************/

#ifdef ALAMOSA_WINDOWS_ENV

typedef unsigned short USHORT;
typedef unsigned char UCHAR, *PUCHAR;
typedef int BOOL;
typedef char CHAR;
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef csx_wulong_t ULONG;
typedef csx_wlong_t  LONG;
#define VOID void
typedef void *PVOID;
typedef short SHORT;
typedef int INT;
#define VOLATILE extern volatile

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#ifndef _WCHAR_DEFINED
#define _WCHAR_DEFINED
typedef wchar_t WCHAR, *PWCHAR;
#endif

#ifndef _TCHAR_DEFINED
#ifdef UNICODE
typedef WCHAR TCHAR;
#else
typedef char TCHAR;
#endif
#define _TCHAR_DEFINED
#endif

typedef ULONG LOGICAL;

#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - need to define some Windows types locally here - ugly */

/*******************************
 * OTHER TYPES
 *******************************/

typedef UINT_64 PHYS_ADDR;  

typedef UINT_64 BITS_64;
typedef unsigned int BITS_32;
typedef unsigned int BITS_16;
typedef unsigned char BITS_8;

typedef UINT_64 OPAQUE_64;
typedef unsigned int OPAQUE_32;
typedef unsigned int OPAQUE_16;
typedef unsigned char OPAQUE_8;

/* The following set of types are defined to contain the exact amount of
 * data storage indicated, hence the 'E' at the end of each name.
 */

typedef __int64 INT_64E;
typedef int INT_32E;
typedef short INT_16E;

typedef unsigned __int64 UINT_64E;
typedef unsigned int UINT_32E;
typedef unsigned short UINT_16E;
typedef unsigned char UINT_8E;

typedef UINT_64E BITS_64E;
typedef unsigned int BITS_32E;
typedef unsigned short BITS_16E;
typedef unsigned char BITS_8E;

typedef UINT_64E OPAQUE_64E;
typedef unsigned int OPAQUE_32E;
typedef unsigned short OPAQUE_16E;
typedef unsigned char OPAQUE_8E;

typedef char CHAR_8E;
typedef unsigned char BOOL_8E;

typedef UINT_32 lu_t;
typedef UINT_32 disk_t;
typedef UINT_32 raid_group_t;

/* The following set of types are defined as "pointer precision" types.
 * The data storage they contain is at least 32-bits and exactly the same
 * as a pointer.  These should be used when doing pointer arithmetic or
 * when storing pointers in opaque data fields.
 */

typedef ULONG_PTR BITS_PTR;
typedef ULONG_PTR OPAQUE_PTR;

/*******************************
 * MISC JUNK
 *******************************/

/* The following definition should be used in place of the extern "C" directive
 * in header files.  The extern "C" directive will cause the C2054 compilation error
 * in C files.
 */
#ifdef __cplusplus 
#define EXTERNC extern "C" 
#else
#define EXTERNC
#endif

/*
 * TRI_STATE is similar to BOOL, but with an INVALID state in addition
 * to TRUE & FALSE.
 */
typedef enum tri_state_enum
{
    TRI_FALSE   = FALSE,
    TRI_TRUE    = TRUE,
    TRI_INVALID = 0xffffffff
}
TRI_STATE;

/*******************************************
 * NULL DEFINITIONS (MUCH ADO ABOUT NOTHING)
 *******************************************/

/* NULL is a pointer */

/* Use NULL_ID when you want to 'null out' an ID (OPAQUE_32) */
#define NULL_ID ((OPAQUE_32) 0)

/* NULL_INT is a generic integer, 0 */
#define NULL_INT 0

/*******************************
 * MISC JUNK
 *******************************/

#define _64BIT_ADDRESSES

typedef UINT_64 LBA_T;
#define MAX_SUPPORTED_LBA CSX_CONST_U64(0x7FFFFFFFFFFFFF)
#define MAX_SUPPORTED_CAPACITY CSX_CONST_U64(0x80000000000000)
#define MAX_DISK_LBA MAX_SUPPORTED_CAPACITY

/* For IO tags */
typedef ULONG IO_TAG_TYPE;

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

#ifndef MAXULONG
#define MAXULONG 0xffffffff
#endif

#endif //GENERIC_TYPES_H
