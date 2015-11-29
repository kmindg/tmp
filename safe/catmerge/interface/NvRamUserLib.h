//***************************************************************************
// Copyright (C)  EMC Corporation 2001
// All rights reserved.
// Licensed material - property of EMC Corporation. * All rights reserved.
//**************************************************************************/
#ifndef _NVRAM_USER_LIB_H_
#define _NVRAM_USER_LIB_H_

//++
// File Name:
//      NvRamUserLib.h
//
// Contents:
//      description of functionality
//
// Internal:
//
// Exported:
//
// NvRamToolCallDriver
//
// Revision History:
//       4/10/01    Bmyers   Created from several .h files by MWagner
//
//--

//++
// Enum:
//      NVRAM_OPERATION
//
// Description:
//      
//
// Members:
//
//      NvRam_Operation_Invalid invalid NVRAM_OPERATION
//
// Revision History:
//      11/29/00   MWagner    Created. 
//
//--

typedef enum _NVRAM_OPERATION
{
        NvRam_Operation_Invalid  = -1,
        NvRam_Operation_Read,
        NvRam_Operation_Write,
        NvRam_Operation_Max,
} NVRAM_OPERATION, *PNVRAM_OPERATION;

int __cdecl
NvRamCallDriver(
    NVRAM_OPERATION   Operation,
    NVRAM_SECTIONS    Section,
    char*             PBytes);

BOOLEAN _cdecl
NvRamGetSectionInfo(
    NVRAM_SECTIONS          nvramSection,
    PNVRAM_SECTION_RECORD   nvramSectionInfo);

#endif // _NVRAM_USER_LIB_H_

//.End
