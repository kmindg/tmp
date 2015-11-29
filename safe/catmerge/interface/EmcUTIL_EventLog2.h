#ifndef _EMCUTIL_EVENTLOG2_H_
#define _EMCUTIL_EVENTLOG2_H_

/****************************************************************************
 * Copyright (C) 2011 EMC Corporation
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*! @file  EmcUTIL_EventLog2.h
 *
 **/

/*******************************************************/

// Header file for the EmcUTIL EventLog2 APIs.

/*! @addtogroup emcutil_eventlog2
 *  @{
 *  @brief
 *   Set of functions that interact with the OS event logging mechanism.
 *   The idea is to provide an abstraction that simplifies the act of
 *   reporting significant events via the OS event log, whatever that happens to be.
 */

#include "EmcUTIL.h"

/*!
 * @brief
 * @param MessageId Message-ID - values are 32 bit values laid out as follows:\n
 *   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1\n
 *   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0\n
 *  +---+-+-+-----------------------+-------------------------------+\n
 *  |Sev|C|R|     Facility          |               Code            |\n
 *  +---+-+-+-----------------------+-------------------------------+\n
 *  Where:\n
 *      Sev - is the severity code: 
 *          00 - Success, 
 *          01 - Informational, 
 *          10 - Warning, 
 *          11 - Error\n
 *      C - is the Customer code flag\n
 *      R - is a reserved bit\n
 *      Facility - is the facility code\n
 *      Code - is the facility's status code\n
 *
 *  For VNXe/NEO bits C & R can define facility (0=system 1=user 2=audit)\n
 *
 * @param DumpDataSize Size of dump data
 * @param DumpData Pointer to dump data
 * @param InsertStringsFormat Insert strings format
 * @return Zero if success, error code if failure
 *
 */

CSX_CDECLS_BEGIN

/*******************************************************/

// APIs similar to Win32 RegisterEventSource, DeregisterEventSource

typedef csx_pvoid_t EMCUTIL_EVENTLOG2_SOURCE_HANDLE;

EMCUTIL_API EMCUTIL_EVENTLOG2_SOURCE_HANDLE EMCUTIL_CC
EmcutilEventLog2SourceHandleCreate(
    csx_cstring_t SourceName);

EMCUTIL_API csx_void_t EMCUTIL_CC
EmcutilEventLog2SourceHandleDestroy(
    EMCUTIL_EVENTLOG2_SOURCE_HANDLE SourceHandle);

/*******************************************************/

// constants for facility (a C4 concept)

#define EMCUTIL_EVENTLOG2_FACILITY_SYSTEM          CSX_XLOG_SYSTEM_FACILITY
#define EMCUTIL_EVENTLOG2_FACILITY_USER            CSX_XLOG_USER_FACILITY
#define EMCUTIL_EVENTLOG2_FACILITY_AUDIT           CSX_XLOG_AUDIT_FACILITY
#define EMCUTIL_EVENTLOG2_FACILITY_DERIVE_FROM_ID  8    // as if C & R bits define facility

// constants for severity (a C4 concept, also has meaning in Windows user space)

#define EMCUTIL_EVENTLOG2_SEVERITY_SUCCESS         0
#define EMCUTIL_EVENTLOG2_SEVERITY_INFORMATIONAL   1
#define EMCUTIL_EVENTLOG2_SEVERITY_WARNING         2
#define EMCUTIL_EVENTLOG2_SEVERITY_ERROR           3
#define EMCUTIL_EVENTLOG2_SEVERITY_DERIVE_FROM_ID  8    // as if NT Sev bits define severity

/*******************************************************/

// for data path use cases - generally you pass this to cover the SourceNativeDriverHandle, SourceName, SourceHandle parameters below
// so the right selections get made based on use case

#ifndef UMODE_ENV
#if defined(ALAMOSA_WINDOWS_ENV) && !defined(SIMMODE_ENV)
EMCUTIL_API csx_pvoid_t EMCUTIL_CC
EmcutilEventLog2GetCurrentNativeDriverObject(
    csx_void_t);
#else
EMCUTIL_API csx_string_t EMCUTIL_CC
EmcutilEventLog2GetCurrentDriverName(
    csx_void_t);
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - driver_model differences */
#endif

#ifndef UMODE_ENV
#if defined(ALAMOSA_WINDOWS_ENV) && !defined(SIMMODE_ENV)
#define EMCUTIL_EVENTLOG2_SOURCE_SPEC EmcutilEventLog2GetCurrentNativeDriverObject(), CSX_NULL, CSX_NULL
#else
#define EMCUTIL_EVENTLOG2_SOURCE_SPEC CSX_NULL, EmcutilEventLog2GetCurrentDriverName(), CSX_NULL
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - driver_model differences */
#else
#define EMCUTIL_EVENTLOG2_SOURCE_SPEC CSX_NULL, "Unknown", CSX_NULL
#endif

// for non data path use cases - generally you pass SourceName, our use the APIs to create/destroy a SourceHandle

/*******************************************************/

// the main logging APIs - ASCII style

EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC
EmcutilEventLog2LogInfoA(
    csx_u32_t MessageId,                   // always required
    csx_u32_t Facility,                    // always required, but you can ask to be derived from MessageId
    csx_u32_t Severity,                    // always required, but you can ask to be derived from MessageId
    csx_pvoid_t SourceNativeDriverHandle,  // can be NULL - but one of SourceNativeDriverHandle, SourceName, SourceHandle should be supplied
    csx_cstring_t SourceName,              // can be NULL - but one of SourceNativeDriverHandle, SourceName, SourceHandle should be supplied
    EMCUTIL_EVENTLOG2_SOURCE_HANDLE SourceHandle,   // can be NULL - but one of SourceNativeDriverHandle, SourceName, SourceHandle should be supplied
    csx_size_t DumpDataSize,               // can be 0
    csx_cpvoid_t DumpDataBase,             // can be NULL
    csx_cstring_t InsertStringsFormat,     // can be NULL
    ...) __attribute__ ((format(__printf_func__, 9, 10)));

EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC
EmcutilEventLog2LogInfoVA(
    csx_u32_t MessageId,                   // always required
    csx_u32_t Facility,                    // always required, but you can ask to be derived from MessageId
    csx_u32_t Severity,                    // always required, but you can ask to be derived from MessageId
    csx_pvoid_t SourceNativeDriverHandle,  // can be NULL - but one of SourceNativeDriverHandle, SourceName, SourceHandle should be supplied
    csx_cstring_t SourceName,              // can be NULL - but one of SourceNativeDriverHandle, SourceName, SourceHandle should be supplied
    EMCUTIL_EVENTLOG2_SOURCE_HANDLE SourceHandle,   // can be NULL - but one of SourceNativeDriverHandle, SourceName, SourceHandle should be supplied
    csx_size_t DumpDataSize,               // can be 0
    csx_cpvoid_t DumpDataBase,             // can be NULL
    csx_cstring_t InsertStringsFormat,     // can be NULL
    va_list ap) __attribute__ ((format(__printf_func__, 9, 0)));

/*******************************************************/

// the main logging APIs - UNICODE (Wide) style

EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC
EmcutilEventLog2LogInfoW(
    csx_u32_t MessageId,                   // always required
    csx_u32_t Facility,                    // always required, but you can ask to be derived from MessageId
    csx_u32_t Severity,                    // always required, but you can ask to be derived from MessageId
    csx_pvoid_t SourceNativeDriverHandle,  // can be NULL - but one of SourceNativeDriverHandle, SourceName, SourceHandle should be supplied
    csx_cstring_t SourceName,              // can be NULL - but one of SourceNativeDriverHandle, SourceName, SourceHandle should be supplied
    EMCUTIL_EVENTLOG2_SOURCE_HANDLE SourceHandle,   // can be NULL - but one of SourceNativeDriverHandle, SourceName, SourceHandle should be supplied
    csx_size_t DumpDataSize,               // can be 0
    csx_cpvoid_t DumpDataBase,             // can be NULL
    csx_cwstring_t InsertStringsFormat,    // can be NULL
    ...);

EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC
EmcutilEventLog2LogInfoVW(
    csx_u32_t MessageId,                   // always required
    csx_u32_t Facility,                    // always required, but you can ask to be derived from MessageId
    csx_u32_t Severity,                    // always required, but you can ask to be derived from MessageId
    csx_pvoid_t SourceNativeDriverHandle,  // can be NULL - but one of SourceNativeDriverHandle, SourceName, SourceHandle should be supplied
    csx_cstring_t SourceName,              // can be NULL - but one of SourceNativeDriverHandle, SourceName, SourceHandle should be supplied
    EMCUTIL_EVENTLOG2_SOURCE_HANDLE SourceHandle,   // can be NULL - but one of SourceNativeDriverHandle, SourceName, SourceHandle should be supplied
    csx_size_t DumpDataSize,               // can be 0
    csx_cpvoid_t DumpDataBase,             // can be NULL
    csx_cwstring_t InsertStringsFormat,    // can be NULL
    va_list ap);

/*******************************************************/

// legacy APIs

#ifdef EMCUTIL_EVENTLOG2_ALL_USERS_ARE_STILL_WIDE

CSX_STATIC_INLINE csx_nsint_t
EmcutilEventLog2(
    csx_u32_t _MessageId,
    csx_size_t _DumpDataSize,
    csx_cpvoid_t _DumpDataBase,
    csx_cwstring_t _InsertStringsFormat,
    ...)
{
    EMCUTIL_STATUS status;
    va_list ap;
    va_start(ap, _InsertStringsFormat);
    status = EmcutilEventLog2LogInfoVW(_MessageId, EMCUTIL_EVENTLOG2_FACILITY_DERIVE_FROM_ID, EMCUTIL_EVENTLOG2_SEVERITY_DERIVE_FROM_ID,
        EMCUTIL_EVENTLOG2_SOURCE_SPEC, _DumpDataSize, _DumpDataBase, _InsertStringsFormat, ap);
    va_end(ap);
    return (csx_nsint_t) status;
}

CSX_STATIC_INLINE csx_nsint_t
EmcutilEventLogVector2(
    csx_u32_t _MessageId,
    csx_size_t _DumpDataSize,
    csx_cpvoid_t _DumpDataBase,
    csx_cwstring_t _InsertStringsFormat,
    va_list ap)
{
    EMCUTIL_STATUS status;
    status = EmcutilEventLog2LogInfoVW(_MessageId, EMCUTIL_EVENTLOG2_FACILITY_DERIVE_FROM_ID, EMCUTIL_EVENTLOG2_SEVERITY_DERIVE_FROM_ID,
        EMCUTIL_EVENTLOG2_SOURCE_SPEC, _DumpDataSize, _DumpDataBase, _InsertStringsFormat, ap);
    return (csx_nsint_t) status;
}

CSX_STATIC_INLINE csx_nsint_t
EmcutilEventLogComponent2(
    csx_u32_t _MessageId,
    csx_cstring_t SourceName,
    csx_size_t _DumpDataSize,
    csx_cpvoid_t _DumpDataBase,
    csx_cwstring_t _InsertStringsFormat,
    ...)
{
    EMCUTIL_STATUS status;
    va_list ap;
    va_start(ap, _InsertStringsFormat);
    status = EmcutilEventLog2LogInfoVW(_MessageId, EMCUTIL_EVENTLOG2_FACILITY_DERIVE_FROM_ID, EMCUTIL_EVENTLOG2_SEVERITY_DERIVE_FROM_ID,
        CSX_NULL, SourceName, CSX_NULL, _DumpDataSize, _DumpDataBase, _InsertStringsFormat, ap);
    va_end(ap);
    return (csx_nsint_t) status;
}

#else

CSX_STATIC_INLINE csx_nsint_t
EmcutilEventLog2(
    csx_u32_t _MessageId,
    csx_size_t _DumpDataSize,
    csx_cpvoid_t _DumpDataBase,
    csx_cstring_t _InsertStringsFormat,
    ...) __attribute__ ((format(__printf_func__, 4, 5)));
CSX_STATIC_INLINE csx_nsint_t
EmcutilEventLog2(
    csx_u32_t _MessageId,
    csx_size_t _DumpDataSize,
    csx_cpvoid_t _DumpDataBase,
    csx_cstring_t _InsertStringsFormat,
    ...)
{
    EMCUTIL_STATUS status;
    va_list ap;
    va_start(ap, _InsertStringsFormat);
    status = EmcutilEventLog2LogInfoVA(_MessageId, EMCUTIL_EVENTLOG2_FACILITY_DERIVE_FROM_ID, EMCUTIL_EVENTLOG2_SEVERITY_DERIVE_FROM_ID,
        EMCUTIL_EVENTLOG2_SOURCE_SPEC, _DumpDataSize, _DumpDataBase, _InsertStringsFormat, ap);
    va_end(ap);
    return (csx_nsint_t) status;
}

CSX_STATIC_INLINE csx_nsint_t
EmcutilEventLogVector2(
    csx_u32_t _MessageId,
    csx_size_t _DumpDataSize,
    csx_cpvoid_t _DumpDataBase,
    csx_cstring_t _InsertStringsFormat,
    va_list ap) __attribute__ ((format(__printf_func__, 4, 0)));
CSX_STATIC_INLINE csx_nsint_t
EmcutilEventLogVector2(
    csx_u32_t _MessageId,
    csx_size_t _DumpDataSize,
    csx_cpvoid_t _DumpDataBase,
    csx_cstring_t _InsertStringsFormat,
    va_list ap)
{
    EMCUTIL_STATUS status;
    status = EmcutilEventLog2LogInfoVA(_MessageId, EMCUTIL_EVENTLOG2_FACILITY_DERIVE_FROM_ID, EMCUTIL_EVENTLOG2_SEVERITY_DERIVE_FROM_ID,
        EMCUTIL_EVENTLOG2_SOURCE_SPEC, _DumpDataSize, _DumpDataBase, _InsertStringsFormat, ap);
    return (csx_nsint_t) status;
}

CSX_STATIC_INLINE csx_nsint_t
EmcutilEventLogComponent2(
    csx_u32_t _MessageId,
    csx_cstring_t SourceName,
    csx_size_t _DumpDataSize,
    csx_cpvoid_t _DumpDataBase,
    csx_cstring_t _InsertStringsFormat,
    ...) __attribute__ ((format(__printf_func__, 5, 6)));
CSX_STATIC_INLINE csx_nsint_t
EmcutilEventLogComponent2(
    csx_u32_t _MessageId,
    csx_cstring_t SourceName,
    csx_size_t _DumpDataSize,
    csx_cpvoid_t _DumpDataBase,
    csx_cstring_t _InsertStringsFormat,
    ...)
{
    EMCUTIL_STATUS status;
    va_list ap;
    va_start(ap, _InsertStringsFormat);
    status = EmcutilEventLog2LogInfoVA(_MessageId, EMCUTIL_EVENTLOG2_FACILITY_DERIVE_FROM_ID, EMCUTIL_EVENTLOG2_SEVERITY_DERIVE_FROM_ID,
        CSX_NULL, SourceName, CSX_NULL, _DumpDataSize, _DumpDataBase, _InsertStringsFormat, ap);
    va_end(ap);
    return (csx_nsint_t) status;
}

#endif

#define EmcutilEventLog EmcutilEventLog2
#define EmcutilEventLogVector EmcutilEventLogVector2
#define EmcutilEventLogComponent EmcutilEventLogComponent2

#define EmcutilLogEntry(pDeviceObject, MessageNumber) \
    EmcutilEventLog((MessageNumber), 0, NULL, NULL)

#define EmcutilLogEntryText(pDeviceObject, MessageNumber, pText) \
    EmcutilEventLog((MessageNumber), 0, NULL, "%s", (pText))

#define EmcutilLogEntryTextText(pDeviceObject, MessageNumber, pText1, pText2) \
    EmcutilEventLog((MessageNumber), 0, NULL, "%s %s", (pText1), (pText2))

#define EmcutilLogEntryInt(pDeviceObject, MessageNumber, Parm) \
    EmcutilEventLog((MessageNumber), 0, NULL, "%d", Parm)

#define EmcutilLogEntryData(pDeviceObject, MessageNumber, DataLength, Data) \
    EmcutilEventLog((MessageNumber), (DataLength), (Data), NULL)

#define EmcutilLogEntryTextData(pDeviceObject, MessageNumber, pText, DataLength, Data) \
    EmcutilEventLog((MessageNumber), (DataLength), (Data), "%s", (pText))

/*******************************************************/

CSX_STATIC_INLINE csx_nsint_t
EmcutilEventLogNoData(
    csx_u32_t _MessageId,
    csx_cstring_t _InsertStringsFormat,
    ...) __attribute__ ((format(__printf_func__, 2, 3)));
CSX_STATIC_INLINE csx_nsint_t
EmcutilEventLogNoData(
    csx_u32_t _MessageId,
    csx_cstring_t _InsertStringsFormat,
    ...)
{
    EMCUTIL_STATUS status;
    va_list ap;
    va_start(ap, _InsertStringsFormat);
    status = EmcutilEventLog2LogInfoVA(_MessageId, EMCUTIL_EVENTLOG2_FACILITY_DERIVE_FROM_ID, EMCUTIL_EVENTLOG2_SEVERITY_DERIVE_FROM_ID,
        EMCUTIL_EVENTLOG2_SOURCE_SPEC, (csx_size_t)0, CSX_NULL, _InsertStringsFormat, ap);
    va_end(ap);
    return (csx_nsint_t) status;
}

/*******************************************************/

CSX_CDECLS_END

/*! @} END emcutil_eventlog2 */

//++
//.End file EmcUTIL_EventLog2.h
//--

#endif                                     // define _EMCUTIL_EVENTLOG2_H_
