//***************************************************************************
// Copyright (C) EMC Corporation 2007
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//++
// File Name:
//      K10HWIdentifacation.h
//
// Contents:
//      Defines the structure of the HARDWARE section of NVRAM. 
//      Note that this log is currently written to by POST and only contains 
//      information to identify what hardware is connected to the SP at boot
//      time
//
// Revision History:
//      06/21/07    Joe Ash       Created.
//
//--
#ifndef _K10_HW_IDENTIFICATION_H_
#define _K10_HW_IDENTIFICATION_H_

#include "generic_types.h"

// Maximum number of entries
#define MAX_HW_INFO_ENTRIES 60

// the signature of the section
#define HW_INFO_SIGNATURE "POST HW INFO"

#pragma pack(1)

// define Hardware IDs which will identify the source module for the data in
// the HW_INFO_ENTRY struct, some will be left blank for future use
typedef enum _HW_ID
{
    NO_HW_ID=0x00,
    MB_HW_ID=0x01,
    ENCL_HW_ID=0x02,
    IO0_HW_ID=0x10,
    IO1_HW_ID=0x11,
    IO2_HW_ID=0x12,
    IO3_HW_ID=0x13,
    IO4_HW_ID=0x14,
    IO5_HW_ID=0x15,
}HW_ID;

//++
// Type:
//      HW_INFO_ENTRY
//
// Description:
//      A single entry containing hardware identification information.
//
// Revision History:
//      06/21/07    Joe Ash       Created.
//
//--
typedef struct _HW_INFO_ENTRY
{
    HW_ID       : 16;
    HW_ID hw_id : 16;
    union
    {
        struct
        {   // note that these are swapped to correct endian notation
            UINT_16E fru_id;
            UINT_16E family_id;
        } ff_id;
        UINT_32E family_fru_id;
    }ff;
}HW_INFO_ENTRY;

//++
// Type:
//      HW_INFO
//
// Description:
//      A definition for the entire HW_INFO section in NVRAM.
//
// Revision History:
//      06/21/07    Joe Ash       Created.
//
//--
typedef struct _HW_INFO
{
    UINT_8E signature[16];
    UINT_32E num_entries;
    UINT_32E padding1;
    UINT_32E padding2;
    UINT_32E padding3;
    HW_INFO_ENTRY entries[MAX_HW_INFO_ENTRIES];
}HW_INFO;

#pragma pack()

#endif  // _K10_HW_IDENTIFICATION_H_s
