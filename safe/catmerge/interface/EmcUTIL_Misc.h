#ifndef EMCUTIL_MISC_H_
#define EMCUTIL_MISC_H_

//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*! @file  EmcUTIL_Misc.h */
//
// Contents:
//      The header file for the utility "miscellaneous" APIs.

////////////////////////////////////

/*! @addtogroup emcutil_misc
 *  @{
 */

#include "EmcUTIL.h"

////////////////////////////////////

/*
 * Explain here
 */

CSX_CDECLS_BEGIN

////////////////////////////////////

/*!
 * @brief Create narrow string from wide
 * @param wideString Wide string
 * @param narrowStringRv [OUT] Pointer to narrow sting
 * @return Zero (EMCUTIL_STATUS_OK) if success, error-code if failure
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilNarrowStringMake(
    csx_cwstring_t wideString,
    csx_string_t * narrowStringRv);

/*!
 * @brief Free narrow string (created by EmcutilNarrowStringMake())
 * @param narrowString Narrow string
 */
EMCUTIL_API csx_void_t EMCUTIL_CC EmcutilNarrowStringFree(
    csx_string_t narrowString);

/*!
 * @brief Create wide string from narrow
 * @param narrowString Narrow string
 * @param wideStringRv [OUT] Pointer to wide sting
 * @return Zero (EMCUTIL_STATUS_OK) if success, error-code if failure
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilWideStringMake(
    csx_cstring_t narrowString,
    csx_wstring_t * wideStringRv);

/*!
 * @brief Free wide string (created by EmcutilWideStringMake())
 * @param wideString Wide string
 */
EMCUTIL_API csx_void_t EMCUTIL_CC EmcutilWideStringFree(
    csx_wstring_t wideString);

////////////////////////////////////

/*!
 * @brief Get string size
 * @param s String (includes terminating NUL)
 * @return Size of string (in bytes)
 */
EMCUTIL_API EMCUTIL_SIZE_T EMCUTIL_CC EmcutilStringSize(
    csx_cstring_t s); 

/*!
 * @brief Get wide sting size in bytes
 * @param s Wide string (includes terminating NUL)
 * @return Size of string (in bytes)
 */
EMCUTIL_API EMCUTIL_SIZE_T EMCUTIL_CC EmcutilWideStringSizeBytes(
    csx_cwstring_t s);

/*!
 * @brief Get wide string siz in characters
 * @param s Wide string (includes terminating NUL)
 * @return Size of string (in characters)
 */
EMCUTIL_API EMCUTIL_SIZE_T EMCUTIL_CC EmcutilWideStringSizeChars(
    csx_cwstring_t s);

/*!
 * @brief Get multi-string size
 * @param ms Multi-string (includes terminating NUL)
 * @return Size of multi-string (in bytes)
 */
EMCUTIL_API EMCUTIL_SIZE_T EMCUTIL_CC EmcutilMultiStringSize(
    csx_cstring_t ms);

/*!
 * @brief Get size of wide multi-string in bytes
 * @param ms Multi-string (includes terminating NUL)
 * @return Size of wide multi-string (in bytes)
 */
EMCUTIL_API EMCUTIL_SIZE_T EMCUTIL_CC EmcutilWideMultiStringBytes(
    csx_cwstring_t ms);                    // includes terminating NULs

/*!
 * @brief Get size of wide multi-string in characters
 * @param ms Multi-string (includes terminating NUL)
 * @return Size of wide multi-string (in characters)
 */
EMCUTIL_API EMCUTIL_SIZE_T EMCUTIL_CC EmcutilWideMultiStringChars(
    csx_cwstring_t ms);                    // includes terminating NULs

/*!
 * @brief Return string pointer to build type
 * @param None
 * @return pointer to "Checked/Debug" or "Free/Retail"
 */
EMCUTIL_API csx_cstring_t EMCUTIL_CC EmcutilBuildType(void);

/*! @} END emcutil_misc */


////////////////////////////////////
/*! @addtogroup emcutil_status
 *  @{
 */

#if defined(EMCUTIL_CONFIG_BACKEND_WU)
/*!
 * @brief Convert the CSX status to EMCUTIL status
 * @param CSX status
 * @return EMCUTIL status
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilNativeStatusToEmcutilStatus(csx_wulong_t status);

#elif defined(EMCUTIL_CONFIG_BACKEND_WK)
/*!
 * @brief Convert the Windows (NTSTATUS) status to EMCUTIL status
 * @param NTSTATUS status
 * @return EMCUTIL status
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilNativeStatusToEmcutilStatus(NTSTATUS status);

#elif defined(EMCUTIL_CONFIG_BACKEND_CSX)
/*!
 * @brief Convert the CSX status to EMCUTIL status
 * @param CSX status
 * @return EMCUTIL status
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilNativeStatusToEmcutilStatus(csx_status_e status);

#else
#error "Error: bad EMCUTIL_CONFIG configuration..."
#endif

////////////////////////////////////

#define EMCUTIL_BASE_CDRIVE	"C"
#define EMCUTIL_BASE_HD0P1	"HD0P1"
#define EMCUTIL_BASE_TEMP	"TEMP"
#define EMCUTIL_BASE_DUMPS	"DUMPS"
#define EMCUTIL_BASE_EMC	"EMC"


EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilFilePathMake(
	csx_string_t pathBuf,
	csx_size_t bufSize,
	csx_string_t base,
	...);

EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilFilePathExtend(
	csx_string_t pathBuf,
	csx_size_t bufSize,
	...);


///////////////////////////////////

/*!
 * @brief Convert EMCUTIL status to string
 * @param status status code
 * @return String representation of the status code
 */
EMCUTIL_API csx_cstring_t EMCUTIL_CC EmcutilStatusToString(
    EMCUTIL_STATUS status);

/*! @} END emcutil_status */


////////////////////////////////////

/*! @addtogroup emcutil_memory
 *  @{
 */

EMCUTIL_API csx_pvoid_t EMCUTIL_CC EmcutilGlobalAlloc(csx_size_t nbytes);

EMCUTIL_API csx_pvoid_t EMCUTIL_CC EmcutilGlobalAllocZeroed(csx_size_t nbytes);

EMCUTIL_API csx_pvoid_t EMCUTIL_CC EmcutilGlobalFree(csx_pvoid_t ptr);

/*! @} END emcutil_memory */

////////////////////////////////////

/*! @addtogroup emcutil_debug
 * @{
 */

EMCUTIL_API csx_nsint_t EMCUTIL_CC EmcutilPrintToDebugger(csx_cstring_t string);

EMCUTIL_API void EMCUTIL_CC EmcutilDebugPathPrint(csx_cstring_t format, ...) __attribute__((format(__printf_func__,1,2)));

EMCUTIL_API void EMCUTIL_CC EmcutilDebugPathPrintv(csx_cstring_t format, va_list ap) __attribute__((format(__printf_func__,1,0)));

/*! @} END emcutil_debug */

////////////////////////////////////

#if defined(EMCUTIL_CONFIG_BACKEND_WU)
#include "stdio.h"
#define EmcutilPrintf  printf
#elif defined(EMCUTIL_CONFIG_BACKEND_WK)
#include "stdio.h"
#define EmcutilPrintf  printf
#elif defined(EMCUTIL_CONFIG_BACKEND_CSX)
#include "csx_ext.h"
#define EmcutilPrintf  csx_p_stdio_printf // Alamosa - this did not work when used directly in Windows user case
#endif

////////////////////////////////////

#define WalkList(_walker, _head) \
    for ((_walker) = (_head)->csx_dlist_field_next; \
         (_walker) != (_head); \
         (_walker) = (_walker)->csx_dlist_field_next)

////////////////////////////////////

extern void __cdecl emcutil_callint3(void);

////////////////////////////////////

CSX_CDECLS_END

//++
//.End file EmcUTIL_Misc.h
//--

#endif                                     /* EMCUTIL_MISC_H_ */
