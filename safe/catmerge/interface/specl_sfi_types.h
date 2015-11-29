#ifndef __SPECL_SFI_TYPES_H__
#define __SPECL_SFI_TYPES_H__

/***************************************************************************
 *  Copyright (C)  EMC Corporation 2010
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. 
 ***************************************************************************/

/***************************************************************************
 * specl_sfi_types.h
 ***************************************************************************
 *
 * File Description:
 *  Header file for all internally visible structures, used in conjunction
 *  with SFI funtionality of SPECL. 
 *
 * Author:
 *  Sameer Bodhe
 *  
 * Revision History:
 *  Dec 9, 2009 - Sameer - Created Inital Version
 *
 ***************************************************************************/

#include "specl_types.h"
#include "spid_types.h"

/**********************************
 * SFI mask commands and status codes
 **********************************/
 
typedef enum _SPECL_SFI_CODES
{
    SPECL_SFI_SET_CACHE_DATA        = 0x02,
    SPECL_SFI_GET_CACHE_DATA,
    SPECL_SFI_LOAD_GOOD_DATA,
    SPECL_SFI_MODE_ENABLED,
    SPECL_SFI_MODE_DISABLED,
    SPECL_SFI_PREVIOUS_SCREEN,
    SPECL_SFI_ENABLE_TIMESTAMP,
    SPECL_SFI_DISABLE_TIMESTAMP,
    SPECL_SFI_PARAMETER_NOT_DEFINED = 0xFFF3,
    SPECL_SFI_COMMAND_FAILED        = 0xFFF4,
    SPECL_SFI_COMMAND_EXECUTED      = 0xFFF5,
    SPECL_SFI_INVALID_DATA_RECEIVED = 0xFFF6,
    SPECL_SFI_STRUCTURE_NOT_FOUND   = 0xFFF7,
    SPECL_SFI_SUCCESS               = 0xFFF8,
    SPECL_SFI_ERROR                 = 0xFFF9,
    SPECL_SFI_DATA_RANGE_INCORRECT  = 0xFFFA,
    SPECL_SFI_DATA_TYPE_INCORRECT   = 0xFFFB,
    SPECL_SFI_INVALID_COMMAND       = 0xFFFC,
    SPECL_SFI_USER_ABORT            = 0xFFFD,
    SPECL_SFI_SEND_COMMAND          = 0xFFFE,
    SPECL_SFI_DEFAULT_DATA          = 0xFFFF
}SPECL_SFI_CODES;


/***********************
 * SFI SPECL maskable structures
 ***********************/

typedef enum _SPECL_SFI_STRUCT_NUMBER
{
    SPECL_SFI_INVALID_STRUCT_NUMBER,
    SPECL_SFI_RESUME_STRUCT_NUMBER,
    SPECL_SFI_PS_STRUCT_NUMBER,
    SPECL_SFI_BATTERY_STRUCT_NUMBER,
    SPECL_SFI_SUITCASE_STRUCT_NUMBER,
    SPECL_SFI_BLOWER_STRUCT_NUMBER,
    SPECL_SFI_FAN_STRUCT_NUMBER,
    SPECL_SFI_MGMT_STRUCT_NUMBER,
    SPECL_SFI_IO_STRUCT_NUMBER,
    SPECL_SFI_BMC_STRUCT_NUMBER,
    SPECL_SFI_FLT_EXP_STRUCT_NUMBER,
    SPECL_SFI_FLT_REG_STRUCT_NUMBER,    
    SPECL_SFI_SLAVE_PORT_STRUCT_NUMBER,
    SPECL_SFI_LED_STRUCT_NUMBER,
    SPECL_SFI_DIMM_STRUCT_NUMBER,
    SPECL_SFI_MISC_STRUCT_NUMBER,
    SPECL_SFI_MPLXR_STRUCT_NUMBER,
    SPECL_SFI_TEMPERATURE_STRUCT_NUMBER,
    SPECL_SFI_FIRMWARE_STRUCT_NUMBER,
    SPECL_SFI_PCI_STRUCT_NUMBER,
    SPECL_SFI_MINISETUP_STRUCT_NUMBER,
    SPECL_SFI_SPS_RESUME_STRUCT_NUMBER,
    SPECL_SFI_SPS_STRUCT_NUMBER,
    SPECL_SFI_CACHE_CARD_STRUCT_NUMBER,
    SPECL_SFI_MAX_STRUCT_NUMBER,
}SPECL_SFI_STRUCT_NUMBER;

/***********************
 * SFI fault mask/unmask data
 ***********************/

typedef struct _SPECL_SFI_MASK_DATA
{
    SPECL_SFI_CODES                 maskStatus;
    SPECL_SFI_STRUCT_NUMBER         structNumber;
    SMB_DEVICE                      smbDevice;
    union
    {
        SPECL_RESUME_DATA           resumeStatus;
        SPECL_PS_SUMMARY            psStatus;
        SPECL_FAN_SUMMARY           fanStatus;
        SPECL_BATTERY_SUMMARY       batteryStatus;
        SPECL_DIMM_SUMMARY          spdDimmStatus;
        SPECL_SLAVE_PORT_SUMMARY    slavePortStatus;
        SPECL_IO_SUMMARY            ioModStatus;
        SPECL_BMC_SUMMARY           bmcStatus;
        SPECL_SUITCASE_SUMMARY      suitcaseStatus;
        SPECL_LED_SUMMARY           ledStatus;
        SPECL_MISC_SUMMARY          miscStatus;
        SPECL_MGMT_SUMMARY          mgmtStatus;
        SPECL_FLT_EXP_SUMMARY       fltExpStatus;
        SPECL_MPLXR_SUMMARY         mplxrStatus;
        SPECL_TEMPERATURE_SUMMARY   temperatureStatus;
        SPECL_FIRMWARE_SUMMARY      firmwareStatus;
        SPECL_PCI_SUMMARY           pciStatus;
        SPECL_MINISETUP_SUMMARY     minisetupStatus;
        SPECL_SPS_RESUME            spsResumeStatus;
        SPECL_SPS_SUMMARY           spsStatus;
        SPECL_CACHE_CARD_SUMMARY    cacheCardStatus;
     } sfiSummaryUnions;
}SPECL_SFI_MASK_DATA, *PSPECL_SFI_MASK_DATA;


typedef struct _SPECL_SFI_COMMAND_STATUS
{
    DWORD status;
}SPECL_SFI_COMMAND_STATUS, *PSPECL_SFI_COMMAND_STATUS;
#endif //SPECL_SFI_TYPES_H

/***************************************************************************
 * END specl_sfi_types.h
 ***************************************************************************/

