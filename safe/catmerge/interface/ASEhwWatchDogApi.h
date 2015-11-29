/*****************************************************************************
 * Copyright (C) EMC Corp. 2006-2009
 * All rights reserved.
 * Licensed material - Property of EMC Corporation.
 *****************************************************************************/

/*****************************************************************************
 *  ASEhwWatchDogApi.h
 *****************************************************************************
 *
 * DESCRIPTION:
 *    This file defines public constants, structures, and prototypes used when
 *    interfacing with the ASEhwWatchDog driver.
 *
 * NOTES:
 *
 * HISTORY:
 *      8/16/2006    Raj Hosamani         Created
 *
 *     11/30/2009    Steve Czuba   Add HwErrMonitor table definitions. Enhance thresholding to allow
 *                                 moving error window.
 *
 *     06/11/2010    Steve Czuba   Add unique Error Table Entry for HwErrmon (Armada) platforms - includes Threshold
 *
 *     08/16/2010    Steve Czuba   Add HwErrMon table entries to allow register clear/set. Make actions ENUM.
 *
 *****************************************************************************/
#ifndef ASEHW_WATCHDOG_API_H
#define ASEHW_WATCHDOG_API_H

#include "spid_types.h"
#if !defined(ALAMOSA_WINDOWS_ENV) || defined(UMODE_ENV)
#include "pciConfig.h"                                     //pciConfig contains PCI information from ntddk.h for user space applications
#endif /* ALAMOSA_WINDOWS*/
#include "HardwareAttributesLib.h"

#ifdef __cplusplus
extern "C"

{
#endif

//
// The version of this interface.
//
#define ASEHWWATCHDOG_INITIAL_VERSION      1

//
// Exported function prototype
//
EMCPAL_STATUS EmcpalDriverEntry (
   IN PEMCPAL_DRIVER    pPalDriverObject
);
  
EMCPAL_STATUS ASEhwWatchDogDriverUnload (
   IN PEMCPAL_DRIVER    pPalDriverObject
);

//
// IOCTL control codes
//
#define HWERRMON_BASE_DEVICE_NAME  "HWERRMON"
#define HWERRMON_NT_DEVICE_NAME    "\\Device\\" HWERRMON_BASE_DEVICE_NAME
#define HWERRMON_DOSDEVICES_NAME   "\\DosDevices\\" HWERRMON_BASE_DEVICE_NAME
#define HWERRMON_WIN32_DEVICE_NAME "\\\\.\\" HWERRMON_BASE_DEVICE_NAME

//
// A helper macro used to define our ioctls.  The 0xC00 function code base
// value was chosen for no particular reason other than the fact that it
// lies in the customer-reserved number range (according to <devioctl.h>).
//
#define HWERRMON_CTL_CODE(code, method) (            \
    EMCPAL_IOCTL_CTL_CODE(EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN, 0xC00 + (code),  \
(method), EMCPAL_IOCTL_FILE_ANY_ACCESS) )

#define IOCTL_NEW_ERROR_ACTION              \
HWERRMON_CTL_CODE(1, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_REGISTER_CALLBACK             \
HWERRMON_CTL_CODE(2, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_DEREGISTER_CALLBACK           \
HWERRMON_CTL_CODE(3, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_SHUTDOWN_ENGINE               \
HWERRMON_CTL_CODE(4, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_INDICT_FRU                    \
HWERRMON_CTL_CODE(5, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_GET_ERROR_COUNT               \
HWERRMON_CTL_CODE(6, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_ENABLE_SIM                    \
HWERRMON_CTL_CODE(7, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_DISABLE_SIM                   \
HWERRMON_CTL_CODE(8, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_SIM_BIT_ERROR                 \
HWERRMON_CTL_CODE(9, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_SIM_STATUS                    \
HWERRMON_CTL_CODE(10, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_SET_ERROR_COUNT               \
HWERRMON_CTL_CODE(11, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_INDICT_BDF                    \
HWERRMON_CTL_CODE(12, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_CLEAR_ERROR_COUNT             \
HWERRMON_CTL_CODE(13, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_GET_DEVICES                   \
HWERRMON_CTL_CODE(14, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_GET_NUM_DEVICES               \
HWERRMON_CTL_CODE(15, EMCPAL_IOCTL_METHOD_BUFFERED)


#define HWERRMON_MAX_ERR_NAME_LEN  128
#define ASEHWWATCHDOG_MAX_INDICT_STRING_LEN 16

#define ASEHWWATCHDOG_ANY_BUS 1
#define TCOBASE_PLUS_8			    	      0x1068	//TCOBASE_+ 8 register this is the TC01_CNT-TCO Control Register
#define NMI_NOW			    			      0x100	    //bit 8



/*------------------------------------------------------------------------------------
 * Armada/HwErrMonitor Error Tables 
 *
 *------------------------------------------------------------------------------------
 */

/* Define Error thersholding limits
 * MINUTE COUNT is how many polls we will peform in one minute based upon the POLL interval
 *
 * From the HW folks - the Thresholding period will be 24 HOURS long  
 * Store errors in 24 periods of 1 hour each
 * 
 * Errors will be counted inside of these blocks and timed out as such. 
 */
#define HWERRMON_THRESHOLD_MINUTE_COUNT   (60/HWERRMON_POLL_INTERVAL)
#define HWERRMON_THRESHOLD_PERIOD_LENGTH  60*24*HWERRMON_THRESHOLD_MINUTE_COUNT
#define HWERRMON_THRESHOLD_BLOCKS         24    /* Divide up total time into N blocks */

typedef struct _HWERRMON_T_BLOCK
{
    ULONG                          BlockTimeCount;  /* Time to stop adding errors into this block */
    ULONG                          ErrorCount;      /* Errors seen during this block of time */
    struct _HWERRMON_T_BLOCK       *nextBlock;      /* Pointer to next error block */
} HWERRMON_THRESH_BLOCK, *PHWERRMON_THRESH_BLOCK;


// Advice to move these types some day into the Action field #defines
typedef enum
{
    HWERRMON_SIMPLE,  // Since we dont fill in Simple entries - make sure 0 is never used by below cases
    HWERRMON_COMPLEX,             // Generic Complex correctable error - threshold does not flush on 24 hour period
    HWERRMON_CMI,                   // CMI error - mask if Link down
    HWERRMON_COMPLEX_CMI,           // CMI Complex error - mask if link down, threshold does not update 24 period
    HWERRMON_LINK_BW,               // Link BW - qualify degraded or not
    HWERRMON_LINK_BW_CMI,            // Link BW - qualify degraded or not
    HWERRMON_UNKNOWN
} HWERRMON_ERROR_TYPE;

/* Unique Table Entry for Armada platforms with ThresholdDefault */
typedef struct HwErrMon_error_table_entry
{
    ULONG                         Offset;
    ULONG                         Bits;
    ULONG                         Access;
    ULONG                         Action;
    BOOLEAN                       Clear;
    BOOLEAN                       Fatal;
    ULONG                         smi_disable_offset;
    ULONG                         smi_disable_bit;
    CHAR                          ErrorName[HWERRMON_MAX_ERR_NAME_LEN];
    ULONG                         ThresholdDefault;
    HWERRMON_ERROR_TYPE           ErrorType;
//    BOOLEAN                       CheckDegraded;     // if check link degraded when this error detected?
//    BOOLEAN                       CmiError;     // if mask this error when link training not completed?
} HWERRMON_ERROR_TABLE_ENTRY, *PHWERRMON_ERROR_TABLE_ENTRY;

#define HWERRMON_END_OF_LIST 0xFFFF

typedef struct _HWERRMON_ERROR_DEFINE
{
    ULONG                               sp_id;
    BDF_FBE_INFO                        FbeDeviceInfo;  
    ULONG                               busNumber;
    ULONG                               deviceNumber;
    ULONG                               functionNumber;
    UINT_64                             faulted_device_type;
    fbe_device_state_t                  faulted_device_state;
    HWERRMON_ERROR_TYPE                 Type_of_Error;
    CHAR                                ErrorName[HWERRMON_MAX_ERR_NAME_LEN];
} HWERRMON_ERROR_DEFINE, *PHWERRMON_ERROR_DEFINE;



typedef struct HEERRMON_ERROR_TABLE_ENTRY_IND_s
{
    USHORT                        deviceID;
    USHORT                        vendorID;
    USHORT                        Bus;
    USHORT                        Device;
    USHORT                        Function;
    ULONG                         Offset;
    ULONG                         Bits;
    ULONG                         Access;
    ULONG                         Action;
    BOOLEAN			  Clear;
    BOOLEAN			  Fatal;
    ULONG			  smi_disable_offset;
    ULONG			  smi_disable_bit;
    ULONG                         headerLogOffset;
    UCHAR                         ErrorName[HWERRMON_MAX_ERR_NAME_LEN];
} HWERRMON_ERROR_TABLE_ENTRY_IND, *PHWERRMON_ERROR_TABLE_ENTRY_IND;

#define MAX_CHIPS_PER_PLATFORM     80
#define MAX_ERRORS_PER_CHIP       120
#define MAX_TABLES_PER_PLATFORM    10
#define MAX_MSR_ERRORS_PER_CORE    80

typedef struct _HWERRMON_THRESHOLD
{
    ULONG                          Status;          // status of the threshold action
    ULONG                          Threshold;       // Threshold level
    ULONG                          PollInterval;    
    ULONG                          ThresholdAction; // Action to be taken when thresold reached. 
    HWERRMON_THRESH_BLOCK          ErrorHistory[HWERRMON_THRESHOLD_BLOCKS];    // Running count of the errors. 
} HWERRMON_THRESHOLD, *PHWERRMON_THRESHOLD;

/* Used to define inclusive list of all SLICS supported in Armada */
typedef struct _HWERRMON_SLIC_LIST
{
    USHORT deviceID;
    USHORT vendorID;
    USHORT tableID;
} HWERRMON_SLIC_LIST;

/* used to discover/validate SLICs when HwErrMon is booting */
typedef struct _HWERRMON_SLIC_INFO
{
    USHORT deviceID;
    USHORT vendorID;
    USHORT bus;
    USHORT device;
    USHORT function;
} HWERRMON_SLIC_INFO;

typedef enum
{
    HWERRMON_COMMON_DEVICE              = 0x0,
    HWERRMON_DEVICE_SLIC0,              // device connecting to SLIC0
    HWERRMON_DEVICE_SLIC1,              // device connecting to SLIC1
    HWERRMON_DEVICE_SLIC2,              // device connecting to SLIC2
    HWERRMON_DEVICE_SLIC3,              // device connecting to SLIC3
    HWERRMON_DEVICE_SLIC4,              // device connecting to SLIC4
    HWERRMON_DEVICE_SLIC5,              // device connecting to SLIC5
    HWERRMON_DEVICE_SLIC6,              // device connecting to SLIC6
    HWERRMON_DEVICE_SLIC7,              // device connecting to SLIC7
    HWERRMON_DEVICE_SLIC8,              // device connecting to SLIC8
    HWERRMON_DEVICE_SLIC9,              // device connecting to SLIC9
    HWERRMON_DEVICE_SLIC10,             // device connecting to SLIC10
    HWERRMON_DEVICE_PLX_UP,             // PLX Upstream port
    HWERRMON_DEVICE_PLX_CMI,            // PLX CMI port
    HWERRMON_DEVICE_INTEL_SB_UP,
    HWERRMON_DEVICE_INTEL_SB_CMI,
} HWERRMON_PCI_DEVICE_TYPE;

/* Used to define the Fixed Chip information on Armada */
typedef struct _HWERRMON_CHIP_INFO
{
    USHORT deviceID;
    USHORT vendorID;
    USHORT tableID;
    USHORT bus;
    USHORT device;
    USHORT function;
    USHORT validateBDF;
    HWERRMON_PCI_DEVICE_TYPE deviceType;     // The device type, e.g., PLX UP, PLX CMI, or a device terminating to a SLIC.
                                             // May check if device in bifurcation situation and adjust BDF accordingly.
    BOOLEAN skip;                            // Some devices are not exposed if not in bifurcation.

} HWERRMON_CHIP_INFO;

/* This will list all supported Armada platforms and the chips involved */
typedef struct _HWERRMON_PLATFORM_LIST
{
    HW_CPU_MODULE cpuModule;
    HWERRMON_CHIP_INFO chips[MAX_CHIPS_PER_PLATFORM];
} HWERRMON_PLATFORM_LIST;

/* Error Lists - includes new functionality to allow for multiple address space mapping */
typedef struct HwErrMon_error_table_entry_armada
{
    USHORT tableID;
    USHORT bus;
    USHORT device;
    USHORT function;
    UINT32 enhancedConfigLimit;
    UINT32 mappedBaseOffset;
    UINT32 spaceNeeded;
    USHORT barOffset;
    HWERRMON_ERROR_TABLE_ENTRY chipErrList[MAX_ERRORS_PER_CHIP]; 
} HWERRMON_ERROR_TABLE_ENTRIES, *PHWERRMON_ERROR_TABLE_ENTRIES;


typedef enum
{
    HWERRMON_NOT_CLEAR          = 0x0,
    HWERRMON_CLEAR_ALL_0,              // write all 0 to clear, for Sandy Bridge MC MSRs
    HWERRMON_CLEAR_BITS_0,             // write 0 to certain bits to clear
    HWERRMON_CLEAR_BITS_1,             // write 1 to certain bits to clear
} HWERRMON_CLEAR_ACTION;


typedef struct _HWERRMON_PLATFORM_MSR_ENTRY
{
    USHORT tableID;
    ULONG  coreNum;
} HWERRMON_PLATFORM_MSR_ENTRY;

typedef struct _HWERRMON_PLATFORM_MSR_LIST
{
    HW_CPU_MODULE cpuModule;
    HWERRMON_PLATFORM_MSR_ENTRY entries[MAX_TABLES_PER_PLATFORM];
} HWERRMON_PLATFORM_MSR_LIST;

enum
{
    MSR_UNREAD,
    MSR_READ,
    MSR_REPORTED,
    MSR_CLEARED,
    MSR_IGNORED
};

typedef struct _MSR_DATA
{
    BOOLEAN   regMiscReport; // Have we read (reported) the DBG registers yet ?
    ULONG     regState;
    ULONGLONG regValue;      // Value read from HW (overwritten if in RAM test mode)
} MSR_DATA;


// ACCESS Methods for Registers
#define ASEHWWATCHDOG_ACCESS_METHOD_DEFAULT    0x0
#define ASEHWWATCHDOG_ACCESS_METHOD_MAPPED     0x1

// Error Actions
typedef enum
{
        ASEHWWATCHDOG_ACTION_IGNORE      =    0x0,
        ASEHWWATCHDOG_ACTION_KTRACE,
        ASEHWWATCHDOG_ACTION_NTEVENT,
        ASEHWWATCHDOG_ACTION_TRACK,
        ASEHWWATCHDOG_ACTION_TRACK_DT,  // Do not threshold ?
        ASEHWWATCHDOG_ACTION_VALUE,     // Look for a specific value within a BitMask 
        ASEHWWATCHDOG_ACTION_CALL_SMI,
        ASEHWWATCHDOG_ACTION_INITCLEAR, /* Clear register bits only - no error added */
        ASEHWWATCHDOG_ACTION_INITSET,   /* Set register bits only - no error added   */
        ASEHWWATCHDOG_ACTION_TRACK_CHANGE,    /* Register is read-only, track it only when count changes */
        ASEHWWATCHDOG_ACTION_RESET,
} ASEHWWATCHDOG_ACTION_TYPES;

// Threshold status
#define ASEHWWATCHDOG_THRESHOLD_IGNORE        0x0
#define ASEHWWATCHDOG_THRESHOLD_HISTORY       0x1  // Ignore threshold but keep historical data
#define ASEHWWATCHDOG_THRESHOLD_OBSERVE       0x2  // Keep historical data and take action

typedef struct ASEhwwatchdog_error_table_entry
{
    ULONG                         Offset;
    ULONG                         Bits;
    ULONG                         Access;
    ULONG                         Action;
	BOOLEAN						  Clear;
	BOOLEAN						  Fatal;
	ULONG						  smi_disable_offset;
	ULONG						  smi_disable_bit;
    UCHAR                         ErrorName[HWERRMON_MAX_ERR_NAME_LEN];
} ASEHWWATCHDOG_ERROR_TABLE_ENTRY, *PASEHWWATCHDOG_ERROR_TABLE_ENTRY;


/* Enhance Thresholding to contain error history blocks- ok to leave as ASEWATCHDOG structure for now 
 * since Fleet does not use but may be enhanced.
 */
typedef struct _ASEHWWATCHDOG_THRESHOLD
{
    ULONG                          Status;          // status of the threshold action
    ULONG                          LastCount;       /* For registers which are counting based - remember for comparison */
    ULONG                          Threshold;       // Threshold level
    ULONG                          PollInterval;    
    ULONG                          ThresholdAction; // Action to be taken when thresold reached. 
    HWERRMON_THRESH_BLOCK          ErrorHistory[HWERRMON_THRESHOLD_BLOCKS];    // Running count of the errors. 
    HWERRMON_THRESH_BLOCK          *activeBlock;
} ASEHWWATCHDOG_THRESHOLD, *PASEHWWATCHDOG_THRESHOLD;

#define ASEHWWATCHDOG_NO_HLOG 0
typedef struct _ASEHWWATCHDOG_ERROR_DEFINE
{
	USHORT				Device_Id;
	USHORT				Vendor_Id;
	ULONG			    Bus;
	PCI_SLOT_NUMBER		SlotData;                        //defined in ntddk.h
    ASEHWWATCHDOG_ERROR_TABLE_ENTRY ErrorInfo;             // Error information
    ASEHWWATCHDOG_THRESHOLD		Threshhold;              // Threshold information

} ASEHWWATCHDOG_ERROR_DEFINE, *PASEHWWATCHDOG_ERROR_DEFINE;


#define HwErrMonValidCallBackClients  (FBE_DEVICE_TYPE_SP | FBE_DEVICE_TYPE_IOMODULE | \
				       FBE_DEVICE_TYPE_DIMM | FBE_DEVICE_TYPE_MEZZANINE |   \
				       FBE_DEVICE_TYPE_BACK_END_MODULE)

typedef struct _HWERRMON_REG_CALLBACK_IOCTL_INFO
{
       UINT_64       notify_for_this_device;
       VOID		       (*Error_Callback)(PVOID context,
					      ULONG value,
					      HWERRMON_ERROR_DEFINE	Error);
     
	PVOID				Context;
} HWERRMON_REG_CALLBACK_IOCTL_INFO, *PHWERRMON_REG_CALLBACK_IOCTL_INFO;

typedef struct _ASEHWWATCHDOG_NEWERRORACTION_IOCTL_INFO
{
    ASEHWWATCHDOG_ERROR_DEFINE	Error;
	ULONG					Action;
} ASEHWWATCHDOG_NEWERRORACTION_IOCTL_INFO, *PASEHWWATCHDOG_NEWERRORACTION_IOCTL_INFO;


/* Sandy Bridge MSR table errors */
typedef struct HwErrMon_MSR_error_table_entry
{
    ULONG                         Offset;
    ULONGLONG                     BitMask;
    ULONGLONG                     ValueMatch;
    ULONG                         Action;
    BOOLEAN                       Fatal; 
    CHAR                          ErrorName[HWERRMON_MAX_ERR_NAME_LEN];
    CHAR                          IndictStr[ASEHWWATCHDOG_MAX_INDICT_STRING_LEN];
    ULONG                         ThresholdDefault;
    //MSR_DATA                    *msrPtr;
    //BOOLEAN                     errDetected;
} HWERRMON_ERROR_MSR_TABLE_ENTRY, *PHWERRMON_ERROR_MSR_TABLE_ENTRY;

/* Sandy Bridge MSR table errors */
typedef struct HwErrMon_MSR_error_runtime_entry
{
    HWERRMON_ERROR_MSR_TABLE_ENTRY eInfo; 
    MSR_DATA                      *msrPtr;
    BOOLEAN                       errDetected;
} HWERRMON_ERROR_MSR_RUNTIME_ENTRY, *PHWERRMON_ERROR_MSR_RUNTIME_ENTRY;

typedef struct HwErrMon_error_table_entry_transformer
{
    USHORT tableID;
    //ULONG  coreNum;
    HWERRMON_ERROR_MSR_TABLE_ENTRY msrErrList[MAX_MSR_ERRORS_PER_CORE]; 
} HWERRMON_MSR_ERROR_TABLE_ENTRIES, *PHWERRMON_MSR_ERROR_TABLE_ENTRIES;

typedef enum _HWERRMON_STATUS
{
    SIMULATION_NOT_ENABLED,
    SIMULATION_ENABLED,
    SIMULATION_ALREADY_ENABLED,
    SIMULATION_ALREADY_DISABLED,
    HWERRMON_DEVICE_NOT_FOUND,
    HWERRMON_ERROR_NOT_FOUND,
    HWERRMON_ABOVE_THRESHOLD,
    HWERRMON_SUCCESS,
}HWERRMON_STATUS;

typedef struct _HWERRMON_SIM_STATUS
{
    HWERRMON_STATUS         status;
} HWERRMON_SIM_STATUS, *PHWERRMON_SIM_STATUS;

typedef struct _HWERRMON_BDF_ERROR_INFO
{
    USHORT                  Bus;
    USHORT                  Device;
    USHORT                  Function;
    ULONG                   Offset;
    ULONG                   Bit;
} HWERRMON_BDF_ERROR_INFO, *PHWERRMON_BDF_ERROR_INFO;

typedef struct _HWERRMON_INDICT_INFO
{
    UINT_64                 fbeDevice;
    BDF_FBE_MODULE_NUM      fbeNum;
    HWERRMON_BDF_ERROR_INFO bdfInfo;
    HWERRMON_ERROR_TYPE     errorType;
    HWERRMON_STATUS         status;
} HWERRMON_INDICT_INFO, *PHWERRMON_INDICT_INFO;

typedef struct _HWERRMON_DEVICE_INFO
{
    ULONG                  Bus;
    ULONG                  Device;
    ULONG                  Function;
    ULONG                  numDevices;
} HWERRMON_DEVICE_INFO, *PHWERRMON_DEVICE_INFO;

EMCPAL_STATUS  RegisterClientWithHwErrMon(HWERRMON_REG_CALLBACK_IOCTL_INFO *client_callback);
EMCPAL_STATUS DeRegisterClientWithHwErrMon(HWERRMON_REG_CALLBACK_IOCTL_INFO);

#ifdef __cplusplus
}
#endif

#endif
