//***************************************************************************
// Copyright (C) Data General Corporation 1989-1999
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

#ifndef K10_LAYERED_DEFS_H
#define K10_LAYERED_DEFS_H

//++
// File Name:
//      K10LayeredDefs.h
//
// Contents:
//      A central repository for macros and constants used by
//      K10 Layered Drivers and Services
//
// Exported:
//      K10_NT_DEVICE_PREFIX_W
//      K10_WIN32_DEVICE_PREFIX_W
//      K10_USER_DEVICE_PREFIX_W
//
// Revision History:
//      6/3/1999    MWagner     Created
//      2/29/2000   ATaylor     Added K10_MPS_FACILITY.
//     10/01/2003   ATaylor     Added K10_LOGICAL_OBJECT_OWNER.
//--

#include "k10csx.h"

//--
// STRINGS
// (wide character strings end in _W)
//++

//++
// Constant:
//      K10_CLARIION_PREFIX
//
// Description:
//      The "root" of the CLARiiON NT Device Tree
//
// Revision History:
//      6/3/1999   MWagner    Created.
//
//--
#define K10_CLARIION_PREFIX        "\\Device\\CLARiiON"

// .End K10_NCLARIION_PREFIX

//++
// Constant:
//      K10_CLARIION_CONTROL_PREFIX
//
// Description:
//      The "root" of the CLARiiON NT Device Tree
//
// Revision History:
//      6/3/1999   MWagner    Created.
//
//--
#define K10_CLARIION_CONTROL_PREFIX  K10_CLARIION_PREFIX "\\Control"

// .End K10_NCLARIION_CONTROL_PREFIX



//++
// Constant:
//      K10_NT_DEVICE_PREFIX_W
//
// Description:
//      The "root" of the CLARiiON NT Device Names
//
// Revision History:
//      6/3/1999   MWagner    Created.
//
//--
#define K10_NT_DEVICE_PREFIX          "\\Device\\CLARiiON\\Control\\"  // regular character version

// .End K10_NT_DEVICE_PREFIX_W

//++
// Constant:
//      K10_WIN32_DEVICE_PREFIX_W
//
// Description:
//      The "root" of the CLARiiON Win32 Device Names
//
// Revision History:
//      6/3/1999   MWagner    Created.
//
//--
#define K10_WIN32_DEVICE_PREFIX       "\\??\\CLARiiON"   // regular character version

// .End K10_WIN32_DEVICE_PREFIX_W


//++
// Constant:
//      K10_USER_DEVICE_PREFIX_W
//
// Description:
//      The "root" of the CLARiiON User Device Names
//
// Revision History:
//      6/3/1999   MWagner    Created.
//
//--
#define K10_USER_DEVICE_PREFIX       "\\\\.\\CLARiiON"

// .End K10_USER_DEVICE_PREFIX_W


//++
// FACILITIES
// (See ntstatus.h for bita avaliable for the "Facilty" code)
//--

//++
// Constant:
//      K10_DLS_FACILITY
//
// Description:
//      Dls Facility code for use in BugCheck Codes
//      and Status Codes
//
// Revision History:
//      6/4/1999   MWagner    Created.
//
//--
#define K10_DLS_FACILITY       0x06660000

// .End K10_DLS_FACILITY

//++
// Constant:
//      K10_DLU_FACILITY
//
// Description:
//      Dls DistLockUtil Facility code for use in BugCheck Codes
//      and Status Codes
//
// Revision History:
//      6/3/1999   MWagner    Created.
//
//--
#define K10_DLU_FACILITY            (K10_DLS_FACILITY + 0x00010000)

// .End K10_DLU_FACILITY


//++
// Constant:
//      K10_DLSDRIVER_FACILITY
//
// Description:
//      User Dls Facility code for use in BugCheckCodes
//      and StatusCodes
//
// Revision History:
//      6/3/1999   MWagner    Created.
//
//--
#define K10_DLSDRIVER_FACILITY       (K10_DLS_FACILITY + 0x00020000)

// .End K10_DLSDRIVER_FACILITY


// .End K10_DLSDRIVER_FACILITY

//++
// DEVICE_TYPES
//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers
//
// Perhaps OR'ing the significant bits of the facility with 0x8000 is a
// reasonable standard for K10 Layered Drivers and Services?
//
//--

//++
// Constant:
//      FILE_DEVICE_K10_DLSDRIVER
//
// Description:
//      DEVICE_TYPE for the USer Dls Driver
//
// Revision History:
//      6/14/1999   MWagner    Created.
//
//--
#define FILE_DEVICE_K10_DLSDRIVER       ( 0x8000 | 0x0668 )

// .End FILE_DEVICE_K10_DLSDRIVER


//++
// Constant:
//      K10_DLUDRIVER_FACILITY
//
// Description:
//      User Dls Facility code for use in BugCheckCodes
//      and StatusCodes
//
// Revision History:
//      7/12//1999   MWagner    Created.
//
//--
#define K10_DLUDRIVER_FACILITY       (K10_DLS_FACILITY + 0x00030000)

// .End K10_DLUDRIVER_FACILITY

//++
// Constant:
//      K10_PSM_FACILITY
//
// Description:
//      PDM Facility code for use in BugCheck Codes
//      and Status Codes
//
// Revision History:
//      21-Jul-99   MWagner    Created.
//
//--
#define K10_PSM_FACILITY       0x07660000

// .End K10_DLS_FACILITY

//++
// DEVICE_TYPES
//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers
//
// Perhaps OR'ing the significant bits of the facility with 0x8000 is a
// reasonable standard for K10 Layered Drivers and Services?
//
//--

//++
// Constant:
//      FILE_DEVICE_K10_DLUDRIVER
//
// Description:
//      DEVICE_TYPE for the USer Dls Driver
//
// Revision History:
//      7/12/1999   MWagner    Created.
//
//--
#define FILE_DEVICE_K10_DLUDRIVER       ( 0x8000 | 0x0669 )


//++
//
// Enumeration:
//      K10_LOGICAL_OBJECT_OWNER
//
// Description:
//      This enum is used as the values for the TLD tags
//      TAG_LOGICAL_OBJECT_OWNER and TAG_LOGICAL_OBJECT_FILTER.  This is used
//      for resource tagging to identify which entity created a SnapView
//      session, SAN Copy descriptor, etc...
//
// Revision History:
//      10/01/2003  Ataylor Created.
//
//--

typedef enum _K10_LOGICAL_OBJECT_OWNER
{
    K10_LOGICAL_OBJECT_OWNER_INVALID = -1,

    //
    // The array administrator tag is used to mark logical objects 
    // created by a user.  This includes commands from Navisphere,
    // admsnap, or any other administrative agent.
    //
    // The value of this member is intentionally zero so that existing
    // persistent pad structures can be leveraged.

    K10_LOGICAL_OBJECT_OWNER_ARRAY_ADMINISTRATOR,

    //
    // The Incremental SAN Copy tag is used by Incremental SAN Copy to 
    // claim ownership.
    //

    K10_LOGICAL_OBJECT_OWNER_INCREMENTAL_SAN_COPY,

    //
    // The FAR tag is used by FAR to claim ownership.
    //

    K10_LOGICAL_OBJECT_OWNER_FAR,

    //
    // The SPLITTER tag is used by CLARiiON splitter to claim ownership.
    //

    K10_LOGICAL_OBJECT_OWNER_SPLITTER,

    
    //
    // The tag is used by Auto Migrator - Compression to claim ownership.
    //
	
    K10_LOGICAL_OBJECT_OWNER_COMPRESSION,
	
    //
    // The DEDUPLICATION tag is used by Deduplication to clain ownership.
    //
    K10_LOGICAL_OBJECT_OWNER_DEDUPLICATION,

    //
    // The TDX tag is used by Data Xfere code to claim ownership.
    //
    K10_LOGICAL_OBJECT_OWNER_TDX,

    // The ALL tag is used for filtering when no filter is provided and
    // the status for all items is to be returned.  This value is 126.

    K10_LOGICAL_OBJECT_OWNER_ALL =  CSX_S8_MAX - 1,

    // The MAX tag is defined to be 127 because it is used as a byte
    // in layered drivers.

    K10_LOGICAL_OBJECT_OWNER_MAXIMUM = CSX_S8_MAX 

}  K10_LOGICAL_OBJECT_OWNER, *PK10_LOGICAL_OBJECT_OWNER;


// .End FILE_DEVICE_K10_DLUDRIVER


#endif // K10_LAYERED_DEFS_H

// End K10LayeredDefs.h
