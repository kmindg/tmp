//***************************************************************************
// Copyright (C) EMC Corporation 2005, 2007
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//++
// File Name:
//      panic_log.h
//
// Contents:
//      Defined the structure of allt he information in the PANIC_LOG
//      section of NVRAM.  Note that this log is currently written to
//      by a bugcheck callback from the reboot driver and read by DGSSP.
//
// Revision History:
//      02/16/05    Matt Yellen   Created.
//
//--
#ifndef _PANIC_LOG_H_
#define _PANIC_LOG_H_

//If set to 0, NvRam panic logging is disabled.
#define NVRAM_PANIC_LOGGING_ENABLED (1)

//This string defines an initialized PANIC_LOG
#define PANIC_LOG_INIT    "$PANIC_LOG_INITIALIZED$"

//Lengths of various fields
#define PANIC_LOG_EXT_DATA_VALUES 4
#define PANIC_LOG_COMPONENT_LENGTH 64
#define PANIC_LOG_INIT_STR_LENGTH 32
#define PANIC_LOG_TOTAL_LOG_ENTRIES 30

#pragma pack(1)

//++
// Type:
//      BUGCHECK_DATA
//
// Description:
//      A bugcheck code and its extended data.
//
// Note:
//        These structures should really be defined with exact
//        length types, but there's no clean way to do that
//        and have it available to both kernel and user space.
//
// Revision History:
//      12-Feb-05   Created.   -Matt Yellen
//      31-May-07   Modified for 64-bit OS - MWH
//
//--
#ifdef _WIN64
typedef struct _BUGCHECK_DATA
{
    /* KeBugCheck and KeBugCheckEx only support using a 32-bit bugcheck code,
     * but the Windows KiBugCheckData structure uses 64-bits to store the bugcheck code.
     * To make copying the data into this structure easy, we'll use ULONG_PTR.
     */
    ULONG_PTR bugcheck_code;
    ULONG_PTR extended_data[PANIC_LOG_EXT_DATA_VALUES];
} BUGCHECK_DATA, *PBUGCHECK_DATA;
#else
// 32-bit clients should use this structure
typedef struct _BUGCHECK_DATA
{
    ULONG bugcheck_code;

    // See comment in BUGCHECK_DATA structure
    ULONG bugcheck_code_unused;

    // Clients that parse/display this information will need to
    // assemble each item as a 64-bit value (see above)
    ULONG extended_data[PANIC_LOG_EXT_DATA_VALUES * 2];
} BUGCHECK_DATA, *PBUGCHECK_DATA;
#endif
//++
// Type:
//      PANIC_LOG_ENTRY
//
// Description:
//      A single entry in the panic log.
//
// Note:
//        These structures should really be defined with exact
//        length types, but there's no clean way to do that
//        and have it available to both kernel and user space.
//
// Revision History:
//      12-Feb-05   Created.   -Matt Yellen
//      31-May-07   Modified for 64-bit OS - MWH
//
//--
#ifdef _WIN64
typedef struct _PANIC_LOG_ENTRY
{
    ULONGLONG timestamp;
    BUGCHECK_DATA bugcheck_data;
    CHAR component[PANIC_LOG_COMPONENT_LENGTH];
    PVOID load_addr;
    PVOID failing_addr;
} PANIC_LOG_ENTRY, *PPANIC_LOG_ENTRY;
#else
// 32-bit clients should use this structure
typedef struct _PANIC_LOG_ENTRY
{
    ULONGLONG timestamp;
    BUGCHECK_DATA bugcheck_data;
    CHAR component[PANIC_LOG_COMPONENT_LENGTH];
    ULONG load_addr_low;
    ULONG load_addr_high;
    ULONG failing_addr_low;
    ULONG failing_addr_high;
} PANIC_LOG_ENTRY, *PPANIC_LOG_ENTRY;
#endif

//++
// Type:
//      PANIC_LOG
//
// Description:
//      A definition for the entire PANIC_LOG in NVRAM.
//
// Note:
//     These structures should really be defined with exact
//     length types, but there's no clean way to do that
//     and have it available to both kernel and user space.
//
// Revision History:
//      12-Feb-05   Created.   -Matt Yellen
//
//--
typedef struct _PANIC_LOG
{
    CHAR initialized_string[PANIC_LOG_INIT_STR_LENGTH];
    ULONG version;
    BOOLEAN panic_flag;
    ULONG normal_reboot_count;
    ULONG panic_count;
    ULONG current_index;
    LONG logged_index;
    PANIC_LOG_ENTRY log_entry[PANIC_LOG_TOTAL_LOG_ENTRIES];
} PANIC_LOG, *PPANIC_LOG;
#pragma pack()

#endif //_PANIC_LOG_H_
