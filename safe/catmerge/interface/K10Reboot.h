//***************************************************************************
// Copyright (C) Data General Corporation 1989-2001
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/
#ifndef _K10_REBOOT_H_
#define _K10_REBOOT_H_

//++
// File Name:
//      K10Reboot.h
//
// Contents:
//      Constants used to Read/Write Reboot Data from/to Registry or NvRam
//
//
// Exported:
//
// Revision History:
//      3/19/01    MWagner   Created
//
//--

//++
//  DEBUGGING
//
//  If the Reboot Driver's global RebootForceDriverLoadFailure is set to a non-zero value,
//  then the Reboot Driver will return STATUS_DRIVER_UNABLE_TO_LOAD, regardless of the 
//  Reboot Count
//
// Revision History:
//      04/06/01   MWagner    Created.
//--

//++
// Include Files
//--
#include "K10NvRam.h"
#include "rebootGlobal.h"

//++
// End Includes
//--

//++
// Macro:
//      K10_REBOOT_USING_NVRAM_PARAMETER_FLAG_NAME_W
//
// Description:
//      The name of a Registry Parameters such that a missing Parameter, or a 
//      Value of zero will cause the Reboot Driver to use the Registry Reboot
//      Count and State Values. A non-zero Value will cause the Reboot Driver 
//      to use the NvRam Count and State values.
//
// Arguments:
//      None.
//
// Return Value
//      None.
//
// Revision History:
//      3/19/01   MWagner    Created.
//
//--
#define K10_REBOOT_USING_NVRAM_PARAMETER_FLAG_NAME_W           "UseNvRam"
// .End K10_REBOOT_USING_NVRAM_PARAMETER_FLAG_NAME_W


//++
// Macro:
//      K10_REBOOT_FORCE_NVRAM_REBOOT_COUNT_TO_ZERO_PARAMETER_FLAG_NAME_W
//
// Description:
//      The name of a Registry Parameters such that a missing Parameter, or a 
//      Value of zero will cause the Reboot Driver to use the Registry Reboot
//      Count and State Values. A non-zero Value will cause the Reboot Driver 
//      to use the NvRam Count and State values.
//
// Arguments:
//      None.
//
// Return Value
//      None.
//
// Revision History:
//      3/19/01   MWagner    Created.
//
//--
#define K10_REBOOT_FORCE_NVRAM_REBOOT_COUNT_TO_ZERO_PARAMETER_FLAG_NAME_W           "ForceNvRamRebootCountToZero"
// .End K10_REBOOT_FORCE_NVRAM_REBOOT_COUNT_TO_ZERO_PARAMETER_FLAG_NAME_W


//++
// Registry Access
//--

//++
// Macro:
//      K10_REBOOT_REBOOT_STATE_PARAMETER_NAME_W
//
// Description:
//      The WCHAR version of the "Reboot State" Registry
//      parameter.
//
// Arguments:
//      None.
//
// Return Value
//      None.
//
// Revision History:
//      7/21/00     kmo     Created.
//
//--
#define K10_REBOOT_REBOOT_STATE_PARAMETER_NAME_W    "RebootState"
// .End K10_REBOOT_REBOOT_STATE_PARAMETER_NAME

//++
// Macro:
//      K10_REBOOT_REBOOT_COUNT_PARAMETER_NAME_W
//
// Description:
//      The WCHAR version of the "Reboot Count" Registry
//      parameter.
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
#define K10_REBOOT_REBOOT_COUNT_PARAMETER_NAME_W   "RebootCount"
// .End K10_REBOOT_REBOOT_COUNT_PARAMETER_NAME

//++
// Macro:
//      K10_REBOOT_REBOOT_REVISION_PATH_W
//
// Description:
//      Path to the revsions of packages loaded on the SP.
//
// Arguments:
//      None.
//
// Return Value
//      None.
//
// Revision History:
//      05/29/02   Abhi    Created.
//
//--
#define K10_REBOOT_REBOOT_REVISION_PATH_W   "\\Registry\\Machine\\system\\CurrentControlSet\\Services\\K10DriverConfig\\Revisions"
// .End K10_REBOOT_REBOOT_REVISION_PATH

//++
// Variable:
//      K10_REBOOT_REBOOT_REVISION_BUFFER_SIZE
//
// Description:
//      The length in bytes of the size of the buffer that holds the size of package.
//
// Revision History:
//      05-29-02    Abhi  Created.
//
//--
#define K10_REBOOT_REBOOT_REVISION_BUFFER_SIZE 256
// .End K10_REBOOT_REBOOT_REVISION_BUFFER_SIZE

//++
// NvRam Access
//--

//++
// Enum:
//      K10_REBOOT_NVRAM_DATUM_TYPE
//
// Description:
//      The "types" of data stored in NvRam by the Reboot Driver.
//      
//
// Members:
//
//      K10_Reboot_NvRam_Datum_Type_Invalid  = -1    An invalid K10_REBOOT_NVRAM_DATUM_TYPE
//      K10_Reboot_NvRam_Datum_Type_Signature        Reboot Signature
//      K10_Reboot_NvRam_Datum_Type_Count            Reboot Count
//      K10_Reboot_NvRam_Datum_Type_State            Reboot State
//      K10_Reboot_NvRam_Datum_Type_Reserved         Reboot Reserved
//      K10_Reboot_NvRam_Datum_Type_Max              Dummy
//
// Revision History:
//      11/15/00   MWagner    Created. 
//
//--
typedef enum _K10_REBOOT_NVRAM_DATUM_TYPE
{
        K10_Reboot_NvRam_Datum_Type_Invalid    = -1,
        K10_Reboot_NvRam_Datum_Type_Signature = 0,
        K10_Reboot_NvRam_Datum_Type_Count,
        K10_Reboot_NvRam_Datum_Type_State,
        K10_Reboot_NvRam_Datum_Type_Reserved,
        K10_Reboot_NvRam_Datum_Type_Invalid_Max

} K10_REBOOT_NVRAM_DATUM_TYPE, *PK10_REBOOT_NVRAM_DATUM_TYPE;
//.End                                                                        

//++
// The OFFSETs are index by the K10_REBOOT_NVRAM_DATUM_TYPE times
// the size of an unsigned int, since they are all unsigned ints
// Since the Singature and Reserved types are "private", their OFFSETs
// are not defined here.
//--

//++
// Macro:
//      K10_REBOOT_NVRAM_COUNT_OFFSET
//
// Description:
//      The Offset of the Reboot Count in NvRam
//
// Arguments:
//      None
//
// Return Value
//      None
//
// Revision History:
//      3/19/01   MWagner    Created.
//
//--
#define K10_REBOOT_NVRAM_COUNT_OFFSET                                          \
    (K10_NVRAM_REBOOT_DATA  +                                                  \
    (K10_Reboot_NvRam_Datum_Type_Count * sizeof(unsigned int)))

// .End K10_REBOOT_NVRAM_COUNT_OFFSET


//++
// Macro:
//      K10_REBOOT_NVRAM_COUNT_LENGTH
//
// Description:
//      The Length of the Reboot Count in NvRam
//
// Arguments:
//      None
//
// Return Value
//      None
//
// Revision History:
//      3/19/01   MWagner    Created.
//
//--
#define K10_REBOOT_NVRAM_COUNT_LENGTH                    (sizeof(unsigned long))

//++
// Macro:
//      K10_REBOOT_NVRAM_STATE_OFFSET
//
// Description:
//      The Offset of the Reboot State in NvRam
//
//      The Reboot State is set to 0 on successful reboot, 1 if 
//      the Reboot Count is exceeded.  It is used by NDU to determine 
//      degraded mode.
//
// Arguments:
//      None
//
// Return Value
//      None
//
// Revision History:
//      3/19/01   MWagner    Created.
//
//--
#define K10_REBOOT_NVRAM_STATE_OFFSET                                          \
    (K10_NVRAM_REBOOT_DATA  +                                                  \
    (K10_Reboot_NvRam_Datum_Type_State * sizeof(unsigned int)))

// .End K10_REBOOT_NVRAM_STATE_OFFSET


//++
// Macro:
//      K10_REBOOT_NVRAM_STATE_LENGTH
//
// Description:
//      The Length of the Reboot State in NvRam
//
// Arguments:
//      None
//
// Return Value
//      None
//
// Revision History:
//      3/19/01   MWagner    Created.
//
//--
#define K10_REBOOT_NVRAM_STATE_LENGTH                    (sizeof(unsigned long))
// .End K10_REBOOT_NVRAM_STATE_LENGTH

//++
// Macro:
//      K10_REBOOT_USE_EXTENDED_REBOOT_COUNT_LIMIT_PARAMETER_FLAG_NAME_W
//
// Description:
//      If this flag is set to any value other than 0, the reboot count will
//      trip after K10_REBOOT_MAXIMUM_EXTENDED_CONSECUTIVE_FUTILE_BOOTS
//      unsuccessful reboots.
//
//      If this flag is set to 0, the reboot count will trip after 
//      K10_REBOOT_MAXIMUM_CONSECUTIVE_FUTILE_BOOTS unsuccessful reboots
//      (which is a smaller number of reboots).
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
#define K10_REBOOT_USE_EXTENDED_REBOOT_COUNT_LIMIT_PARAMETER_FLAG_NAME_W           "UseExtendedRebootCountLimit"
// .End K10_REBOOT_USE_EXTENDED_REBOOT_COUNT_LIMIT_PARAMETER_FLAG_NAME_W


#endif

// End K10Reboot.h
