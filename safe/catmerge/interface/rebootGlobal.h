//***************************************************************************
// Copyright (C) EMC Corporation 1989-2001
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//++
// File Name:
//      rebootGlobal.h
//
// Contents:
//      Data structures for accessing Reboot Section of NvRam.
//
// Exported:
//      K10_REBOOT_DATA
//
// Revision History:
//      12/13/01    Joe Perry   Created to make structures accessible from
//                              outside of Reboot.
//
//--
#ifndef _REBOOT_GLOBAL_H_
#define _REBOOT_GLOBAL_H_

// Use Cases for Reboot count
//
// If you are modifying either K10_REBOOT_MAXIMUM_CONSECUTIVE_FUTILE_BOOTS
// or K10_REBOOT_MAXIMUM_EXTENDED_CONSECUTIVE_FUTILE_BOOTS, be sure to 
// consider these use cases and update them if needed.
//
// The reboot driver will bring the SP up degraded if the reboot count 
// exceeds the current "consecutive futile boots" limit.  We have historically
// set the limit to the maximum expected number of reboots +2 to provide some
// margin for error.
//
/* **** These use cases apply to the Fleet platforms ****
Re-imaging (6)
1.	chkdsk 
2.	Application SW power on/off SLICs
3.	2Gb/4Gb speed check 
4.	Flare persisting ports 
5.	newSP (WWN Seed, IP address, etc.) 
6.	SP up and running normally 
 
NDU Flare Bundle (2)
1.	chkdsk 
2.	SP up and running normally 

Persist ports (3) - i.e. NDU of PortCommit package
1.	Flare persisting ports 
2.	newSP (iSCSI settings) 
3.	SP up and running normally 
 
"Regular reboot" (1)
1.	SP up and running normally 
*/

// end of Reboot count use cases

//++
// Macro:
//      K10_REBOOT_MAXIMUM_CONSECUTIVE_FUTILE_BOOTS
//
// Description:
//      The maximum consecutive number of times that the SP will boot
//      and fail to appease the K10 Governor
//
// Arguments:
//      None.
//
// Return Value
//      None.
//
// Revision History:
//      6/8/00   MWagner    Created.
//
//--
#define K10_REBOOT_MAXIMUM_CONSECUTIVE_FUTILE_BOOTS   0x000000005
// .End K10_REBOOT_MAXIMUM_CONSECUTIVE_FUTILE_BOOTS

//++
// Macro:
//      K10_REBOOT_MAXIMUM_EXTENDED_CONSECUTIVE_FUTILE_BOOTS
//
// Description:
//      Similar to K10_REBOOT_MAXIMUM_CONSECUTIVE_FUTILE_BOOTS, but this
//      value is used only temporarily, after an SP has been imaged or
//      re-imaged (or after a "fresh install" in our labs).
//
//      As soon as NDUMon resets the reboot count, the reboot count limit
//      is set back to K10_REBOOT_MAXIMUM_CONSECUTIVE_FUTILE_BOOTS. 
//
// Arguments:
//      None.
//
// Return Value
//      None.
//
// Revision History:
//      12/13/07   MWH    Created.
//
//--
#define K10_REBOOT_MAXIMUM_EXTENDED_CONSECUTIVE_FUTILE_BOOTS   0x00000000A
// .End K10_REBOOT_MAXIMUM_EXTENDED_CONSECUTIVE_FUTILE_BOOTS

// The following defines are used to decode the limit flag that is stored
// in the reboot section of the registry.
#define K10_REBOOT_LIMIT_FLAG_USE_NORMAL_COUNT  (0)
#define K10_REBOOT_LIMIT_FLAG_USE_EXTENDED_COUNT (1)

//++
// Type:
//      K10_REBOOT_DATA
//
// Description:
//      The Reboot Data stored in Non Volatile RAM.  The size of this structure
//      should match the size of the Reboot NvRam section (see K10Plx.h).
//
// Members:
//      Signature  used to verify initialization (false positives are acceptable)
//      Count      
//      State
//      Reserved   unused
//
// Revision History:
//      05-Mar-01   MWagner  Created.
//      31-Jul-03   Added new ForceDegradedMode field.   -Matt Yellen
//      16-Apr-04   Added RebootType field and type enum for piranha. -Ruomu Gao
//      15-Apr-08   Added sysprep status fieds for main and utility partition. -Joe Ash
//      13-Feb-13   Changed K10_REBOOT_DATA_RESERVED_SIZE from 0x36 to 0x2 to save
//                  space in NVRAM for newly added reboot logging section  -- ZHF
//
//--
#define K10_REBOOT_DATA_RESERVED_SIZE   0x2
#define POST_PRINT_STRING_SIZE          256
#define UP_BOOT_SIGNATURE               0x090609    // Unique identifier for the UP boot

typedef struct _K10_REBOOT_DATA_
{
    ULONG      Signature;
    ULONG      Count;
    ULONG      State;
    ULONG      ForceDegradedMode;
    ULONG      RebootType;
    ULONG      CountAlreadyModified;
    ULONG      UseExtendedRebootCountLimit;
    ULONG      FlareSysprepStatus;
    ULONG      UtilitySysprepStatus;
    ULONG      bootIntoUtilityPartition;
    char       PostPrintString[POST_PRINT_STRING_SIZE];
    ULONG      Reserved[K10_REBOOT_DATA_RESERVED_SIZE];
} K10_REBOOT_DATA, *PK10_REBOOT_DATA;
//.End                                                 

//++
// Type:
//      REBOOT_TYPE
//
// Description:
//      Allowed reboot type stored in K10_REBOOT_DATA.
//      There is a copy in ddbs_boot_main.c too.
//
//--
typedef enum _REBOOT_TYPE
{
    NT_REBOOT_TYPE,
    UTIL_REBOOT_TYPE,
    INVALID_REBOOT_TYPE = 0xFFFFFFFF
} REBOOT_TYPE;
//.End                                                 

#endif // #ifndef _REBOOT_GLOBAL_H_

// End rebootGlobal.h
