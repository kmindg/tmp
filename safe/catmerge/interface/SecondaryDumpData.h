/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

#ifndef __SecondaryDumpData__
#define __SecondaryDumpData__

#if defined(CSX_BV_LOCATION_KERNEL_SPACE) && defined(CSX_BV_ENVIRONMENT_WINDOWS) && !defined(KDEXT_64BIT)
#include "k10ntddk.h"
#endif

//
// BugCheckReasonCallback can write page-aligned data, identified by GUID.
// Maximum of 32MB per callback, so may need muliple callbacks and increment GUID.
// Each callback happens twice, first to return size, then to actualy write.
//
struct SecondaryDumpData
{
    GUID    mGuid;
    ULONG64 mPageAligned;
    ULONG   mDumpLen;
#if defined(CSX_BV_LOCATION_KERNEL_SPACE) && defined(CSX_BV_ENVIRONMENT_WINDOWS) && !defined(KDEXT_64BIT)
    PKBUGCHECK_REASON_CALLBACK_RECORD mCallback;
#else
    ULONG64 m_Callback;
#endif
};

// Number of supported extents
#define SECONDARY_MEMORY_EXTENTS 512

#define MAXCB_BYTES  (0x02000000)  // 32MB


// cmm.h declares
// CMMAPI void cmmSecondaryDumpDataRange(PVOID Address, ULONG Length, char *name);

#endif
