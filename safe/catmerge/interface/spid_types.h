#ifndef __SPID_TYPES__
#define __SPID_TYPES__

//***************************************************************************
// Copyright (C) Data General Corporation 1989 - 2010
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      spid_types.h
//
// Contents:
//      Definitions of the exported IOCTL codes and data structures
//      for the SPID driver.
//
// Revision History:
//  14-Feb-00   Goudreau    Created.
//  02-Nov-01   Owen        Added IOCTL_SPID_PANIC_SP.
//  25-Jul-06   Joe Ash     Added LAST_REBOOT_TYPE and IOCTL_SPID_GET_LAST_REBOOT_TYPE.
//--


//
// Header files
//

#include "EmcPAL.h"
#include "k10defs.h"
#include "gpio_signals.h"
#include "spid_enum_types.h"
#include "familyfruid.h"

//
// Exported constants
//

//++
// Description:
//      The device object pathnames of the SPID pseudo-device.
//      SPID clients must open this device in order to send SPID
//      the ioctl requests described below.
//--
#define SPID_BASE_DEVICE_NAME   "Spid"
#define SPID_NT_DEVICE_NAME     "\\Device\\" SPID_BASE_DEVICE_NAME
#define SPID_DOSDEVICES_NAME    "\\DosDevices\\" SPID_BASE_DEVICE_NAME
#define SPID_WIN32_DEVICE_NAME  "\\\\.\\" SPID_BASE_DEVICE_NAME

//++
// Description:
//      A generic "SP Signature" value that can be used when constructing
//      SPID structures.  See the description of the SPID type below.
//--
#define SPID_GENERIC_SIGNATURE          0

#define MAX_NUMBER_OF_SP    2

#define SP_COUNT_SINGLE    1

//The max length of the SP hardware name
#define SPID_SP_HARDWARE_NAME_MAX_LENGTH    64

//The max length of the SP kernel name
#define SPID_SP_KERNEL_NAME_MAX_LENGTH      64

//
// Exported basic data types
//

typedef enum SP_ID_TAG
{
    SP_A = 0x0,
    SP_B = 0x1,
    SP_ID_MAX = 0x2,
    #ifdef C4_INTEGRATED
    SP_CACHE_CARD = 0x03,
    SP_INDETERMIN = 0xFE,
    #endif /* C4_INTEGRATED - C4HW */
    SP_NA = 0xFD,
    SP_INVALID = 0xFF
} SP_ID;

#if defined(__cplusplus)
/* use extern C++ here to trick other C++ files including this as extern C
 */
extern "C++"{
/* Define a postfix increment operator for SP_ID_TAG
 */
// Define a postfix increment operator for 
inline SP_ID_TAG operator++( SP_ID &sp_id, int )
{
   return sp_id = (SP_ID)((int)sp_id + 1);
}
}
#endif

//++
// Type:
//      SPID
//
// Description:
//      The abstract Storage Processor (SP) identifier type.
//
// Members:
//  node:  opaque identifier for a particular multi-SP storage system.
//  engine:  a specific storage processor within the storage system.
//  signature:  a unique serial number that identifies a particular
//      SP's hardware.  For operations that accept a SPID as an input
//      parameter, a signature value of SPID_GENERIC_SIGNATURE can be
//      specified if no exact signature match is required.
//--
typedef struct _SPID
{
    K10_ARRAY_ID    node;
    ULONG           engine;
    ULONG           signature;
 
} SPID, *PSPID;
//.End

/***************************
 * struct: LAST_REBOOT_TYPE
 *
 * Defines possible reasons
 * for SP to reboot
 ***************************/

typedef enum _LAST_REBOOT_TYPE
{
    NORMAL,
    PANIC,
    REBOOT_TYPE_UNKNOWN
} LAST_REBOOT_TYPE;

/* Defines the individual blades (FRUs) of a hammer-series SAN */
typedef enum _FRU_MODULES
{
    INVALID_FRU_MODULE,

    FRU_LOCAL_SP,
    FRU_LOCAL_MGMT,
    FRU_LOCAL_PS0,
    FRU_LOCAL_PS1,
    FRU_LOCAL_PIB,
    FRU_LOCAL_BATT0,
    FRU_LOCAL_BATT1,
    FRU_LOCAL_BATT2,
    FRU_LOCAL_FAN0,
    FRU_LOCAL_FAN1,
    FRU_LOCAL_FAN2,
    FRU_LOCAL_FAN3,
    FRU_LOCAL_FAN4,
    FRU_LOCAL_FAN5,
    FRU_LOCAL_MSHD,
    FRU_LOCAL_BEM,
    FRU_MIDPLANE,
    FRU_CACHE_CARD,
    FRU_PEER_SP,
    FRU_PEER_MGMT,
    FRU_PEER_PS0,
    FRU_PEER_PS1,

    MAX_FRU_MODULES_CNT
} FRU_MODULES, *PFRU_MODULES;

typedef struct _SPID_PLATFORM_INFO
{
    HW_FAMILY_TYPE      familyType;
    SPID_HW_TYPE        platformType;
    ULONG               processorCount;
    CHAR                hardwareName[SPID_SP_HARDWARE_NAME_MAX_LENGTH];
    HW_MODULE_TYPE      uniqueType;
    HW_CPU_MODULE       cpuModule;
    HW_ENCLOSURE_TYPE   enclosureType;
    HW_MIDPLANE_TYPE    midplaneType;
    BOOLEAN             basicBMC;
    BOOL                isVirtual;              // Tells that the platform is virtualized
    BOOL                isSingleSP;             // Tells that the platform is SingleSP
} SPID_PLATFORM_INFO, *PSPID_PLATFORM_INFO;


//++
// Type:
//      SPID_RESUME
//
// Description:
//      Structure that describes the PROM Resume information stored on
//      CLARiiON SPs.
//
// Members:
//
//  wwn:  World-Wide Name of the SP.
//  manufacturing_locale:  string describing where the SP was made.
//  manufacturing_date:  string describing when the SP was made.
//  vendor_name:  string describing who made the SP.
//  serial_number:  string describing the company-assigned serial number of the SP.
//  revision:  string describing the manufacturing revision of the SP.
//  part_number:  string describing the company-assigned part number of the SP.
//
//--
typedef struct _SPID_RESUME
{
    CHAR    wwn[8];
    CHAR    manufacturing_locale[16];
    CHAR    manufacturing_date[10];
    CHAR    vendor_name[20];
    CHAR    serial_number[12];
    CHAR    revision[8];
    CHAR    part_number[12];

 } SPID_RESUME, *PSPID_RESUME;
//.End

//++
// Type:
//      SPID_HW_TYPE
//
// Note: This type is now defined in spid_enum_types.h
//.End

//++
// Type:
//      SPID_KERNEL_TYPE
//
// Description:
//      Enumeration that describes possible kernel build types
//
// Values:
//
//  KernelTypeUnknown: kernel build type is unknown.
//  UniProcessorFreeKernelType: kernel build type is Free UP.
//  MultiProcessorFreeKernelType: kernel build type is Free MP.
//  CheckedKernelType: kernel build type is Checked MP.
//
//--
typedef enum _SPID_KERNEL_TYPE
{
    KernelTypeUnknown = -1,
    UniProcessorFreeKernelType,
    MultiProcessorFreeKernelType,
    CheckedKernelType
} SPID_KERNEL_TYPE;
//.End

/*** Software defined Fault Status Codes ***/
#define DEGRADED_MODE           0x25    // written by NDUMon
#define APPLICATION_RUNNING     0x2C    // written by NDUMon
#define OS_RUNNING              0x2D    // written by MIR
#define POST_DONE               0x3A    // checked in cm
#define CORE_DUMPING            0x2B    // checked in cm
#define ILLEGAL_MEMORY_CONFIG   0x7D    // written by NDUMon

// The following Slave Port registers are used to track the extended peer boot state.
//    Component Code (reg TCO1) - indicates which software component last updated 
//          these registers.
//    Component Extended Code (reg TCO2) - a code to provide details of the level
//          of progress or fault information for software component referenced 
//          by the Component Code (TC01).
//    General Code (reg WDSTAT) - currently unused, available for future expansion
//
typedef enum _SLAVE_PORT_STS_CODE
{
    SLAVE_PORT_STS_COMPONENT_CODE       = 0x0C, // ICH TCO Message 1 - register offset
    SLAVE_PORT_STS_COMPONENT_EXT_CODE   = 0x0D, // ICH TCO Message 2 - register offset
    SLAVE_PORT_STS_GENERAL_CODE         = 0x0E, // ICH WDSTAT - register offset
    SLAVE_PORT_STS_INVALID_CODE         = 0xFF
} SLAVE_PORT_STS_CODE;

// These codes identify the sw component that is updating the register. 
// These codes are written/read from the ICH TCO Message1 register.
// Range is 0-0xff.
// To add support for a new component identifier:
//      1) Add an entry to BOOT_SW_COMPONENT_CODE. Insert the new entry
//         before SW_LAST_DRIVER_NAME. Do not exceed SW_COMPONENT_LIMIT (0xFF).
//      2) Update the entries in generic_utils_lib.c with the corresponding text description.
//
// DO NOT INSERT ITEMS IN THE MIDDLE OF THIS LIST
typedef enum _boot_component_code
{
    SW_COMPONENT_POST          = 0,
    SW_COMPONENT_AGGREGATE,
    SW_COMPONENT_APM,
    SW_COMPONENT_ASEHWWATCHDOG,
    SW_COMPONENT_ASIDC,
    SW_COMPONENT_CLUTCHES,
    SW_COMPONENT_CMIPCI,
    SW_COMPONENT_CMID,
    SW_COMPONENT_CMISCD,
    SW_COMPONENT_CMM,
    SW_COMPONENT_CRASHCOORDINATOR,
    SW_COMPONENT_DISKTARG,
    SW_COMPONENT_DLS,
    SW_COMPONENT_DLSDRV,
    SW_COMPONENT_DLU,
    SW_COMPONENT_DUMPMANAGER,
    SW_COMPONENT_ESP,
    SW_COMPONENT_EXPATDLL,
    SW_COMPONENT_FCT,
    SW_COMPONENT_FEDISK,
    SW_COMPONENT_FLAREDRV,
// DO NOT INSERT ITEMS IN THE MIDDLE OF THIS LIST
    SW_COMPONENT_HBD,
    SW_COMPONENT_IDM,
    SW_COMPONENT_INTERSPLOCK,
    SW_COMPONENT_IZDAEMON,
    SW_COMPONENT_K10DGSSP,
    SW_COMPONENT_K10GOVERNOR,
    SW_COMPONENT_LOCKWATCH,
    SW_COMPONENT_MSGDISPATCHER,
    SW_COMPONENT_MIGRATION,
    SW_COMPONENT_MINIPORT,
    SW_COMPONENT_MINISETUP,
    SW_COMPONENT_MIR,
    SW_COMPONENT_MLU,
    SW_COMPONENT_MPS,
    SW_COMPONENT_MPSDLL,
    SW_COMPONENT_NDUAPP,
    SW_COMPONENT_NDUMON,
    SW_COMPONENT_NEWSP,
    SW_COMPONENT_PCIHALEXP,
    SW_COMPONENT_PEERWATCH,
    SW_COMPONENT_PSM,
// DO NOT INSERT ITEMS IN THE MIDDLE OF THIS LIST
    SW_COMPONENT_REBOOT,
    SW_COMPONENT_REDIRECTOR,
    SW_COMPONENT_REMOTEAGENT,
    SW_COMPONENT_RPSPLITTER,
    SW_COMPONENT_SAFETYNET,
    SW_COMPONENT_SCSITARG,
    SW_COMPONENT_SMBUS,
    SW_COMPONENT_SMD,
    SW_COMPONENT_SPCACHE,
    SW_COMPONENT_SPECL,
    SW_COMPONENT_SPID,
    SW_COMPONENT_SPM,
    SW_COMPONENT_WDT,
    SW_COMPONENT_ZMD,
    SW_COMPONENT_CMISOCK,
    SW_COMPONENT_SEP,
    SW_COMPONENT_DPCMON,
    SW_LAST_DRIVER_NAME,
// DO NOT INSERT ITEMS IN THE MIDDLE OF THIS LIST
    SW_COMPONENT_LIMIT          = 0xFF
} BOOT_SW_COMPONENT_CODE;

// Extended status codes for the software components.
// These codes are written/read from the ICH TCO Message2 register.
// Range 0-0xff.
// To add support for a new component status:
//      1) Add an entry to SW_COMPONENT_EXT_CODE. For a progress code,
//         insert the new entry immediately before 
//         SW_LAST_EXT_STATUS_ENTRY. Do not exceed SW__DRIVER_ERROR.
//         For an exception code, insert the new entry immediately 
//         before SW_LAST_EXCEPTION_ENTRY. Do not exceed 
//         SW_DRIVER_LIMIT (0xFF).
//      2) For a new progress code, update the array 
//         ComponentProgressEntries in cm_peer.c with the new progress
//         code and a corresponding text description.
//         For a new exception code, update the array 
//         ComponentProgressEntries in cm_peer.c with the new progress
//         code and a corresponding text description.
//
//  Progress codes are used to mark the entry point of the Driver Entry 
//  function for each core software component. Additional progress codes 
//  should be defined and used when lengthy, time consuming operations
//  delay the progress of the driver entry function. For an example, see
//  flaredrv_main.cpp. These codes are intended to aid in the diagnosis
//  of driver related load problems.
//
//  Exception codes are intended to mark non-success status for the 
//  driver entry function. New exception codes should be defined to 
//  indicate specific exception conditions that would be helpful in the
//  diagnosis of driver load problems.
//
typedef enum _sw_component_status
{
    SW_DRIVER_ENTRY         = 0,
    SW_APPLICATION_STARTING,
    SW_FLARE_ReadyInitDLS,
    SW_FLARE_GotHemiInterSpLock,
    SW_FLARE_HemiInit,
    SW_NewSP_BRCMToolStarting,
    SW_NewSP_NetConfStarting,
    SW_NewSP_SyncStarting,
    SW_NewSP_Complete,
    SW_LAST_EXT_STATUS_ENTRY,       // marks the last progress status
    SW_DRIVER_ERROR        = 0x80,  // Progress Codes MUST preceed this value, Error Codes MUST follow
    SW_FLARE_NTBE_FAIL,
    SW_FLARE_ConfigPath_FAIL,
    SW_FLARE_GetSpid_FAIL,
    SW_FLARE_DLSInit_FAIL,
    SW_LAST_EXCEPTION_ENTRY,        // marks the last exception status
    SW_DRIVER_LIMIT
} SW_COMPONENT_EXT_CODE;

typedef enum _sw_general_status_code
{
    SW_GENERAL_STS_NONE = 0,
} SW_GENERAL_STATUS_CODE;
  
typedef struct _FLT_STATUS_REQUEST
{
    UCHAR                   fltStatusCode;
    SLAVE_PORT_STS_CODE     CodeType;
} FLT_STATUS_REQUEST, *PFLT_STATUS_REQUEST;

#define FLT_STS_REQUEST_INIT(m_flt_sts_req_p,m_flt_sts_code,m_code_type)\
    ((m_flt_sts_req_p)->fltStatusCode = (m_flt_sts_code),               \
     (m_flt_sts_req_p)->CodeType = (m_code_type))

typedef struct _GPIO_MULTIPLE_REQUEST
{
    ULONG               pinCount;
    GPIO_REQUEST        gpioRequests[GPIO_MAX_GPIO_PINS_PER_PORT];
} GPIO_MULTIPLE_REQUEST, *PGPIO_MULTIPLE_REQUEST;

#ifndef ALAMOSA_WINDOWS_ENV
typedef struct _FAULT_LED_SET_REQUEST
{
    ULONG            led;
    BOOLEAN          activeLow;
    LED_BLINK_RATE   blink_rate;
} FAULT_LED_SET_REQUEST;
#endif

/* On VNXe these codes need to start at a particular offset in the overall error
 * codes name space
 */
#define SPID_DEGRADED_REASON_CODE_START 0x1b00

typedef enum _spid_degraded_reason
{
    /*
     **NOTE** These codes are processed by Unsiphere in KH and so not do not change the codes
     * arbitrarily. Please follow the process (TBD) to add new codes here
     */
    /* The reason why we are explicitly assigning values is to reduce the error during merges from
     * various streams or coding error where codes just sequenced incorrectly and thus changing the values of 
     * enum
     */
    INVALID_DEGRADED_REASON = SPID_DEGRADED_REASON_CODE_START ,
    DATABASE_CONFIG_LOAD_RAILURE_OR_DATA_CORRUPT = (SPID_DEGRADED_REASON_CODE_START + 1),                                                                                                                            
    CHASSIS_MISMATCHED_WITH_SYSTEM_DRIVES = (SPID_DEGRADED_REASON_CODE_START + 2),
    SYSTEM_DRIVES_DISORDER = (SPID_DEGRADED_REASON_CODE_START + 3),
    THREE_MORE_SYSTEM_DRIVES_INVALID = (SPID_DEGRADED_REASON_CODE_START + 4),
    TWO_SYSTEM_DRIVES_INVALID_AND_IN_OTHER_SLOT = (SPID_DEGRADED_REASON_CODE_START + 5),
    SET_ILLEGAL_WWNSEED_FLAG = (SPID_DEGRADED_REASON_CODE_START + 6),
    SYSTEM_DB_HEADER_IO_ERROR = (SPID_DEGRADED_REASON_CODE_START + 7),
    SYSTEM_DB_HEADER_TOO_LARGE = (SPID_DEGRADED_REASON_CODE_START + 8),
    SYSTEM_DB_HEADER_DATA_CORRUPT = (SPID_DEGRADED_REASON_CODE_START + 9),
    MEMORY_CONFIG_INVALID = (SPID_DEGRADED_REASON_CODE_START + 0xa),
    PROBLEMATIC_DATABASE_VERSION = (SPID_DEGRADED_REASON_CODE_START + 0xb),
    SMALL_SYSTEM_DRIVE = (SPID_DEGRADED_REASON_CODE_START + 0xc),
    NOT_ALL_DRIVE_SET_ICA = (SPID_DEGRADED_REASON_CODE_START + 0xd),
    INCORRECT_PLATFORM_TYPE = (SPID_DEGRADED_REASON_CODE_START + 0xe),
    REBOOT_COUNT_EXCEEDED = (SPID_DEGRADED_REASON_CODE_START + 0xf),
    MEMORY_CONFIG_ILLEGAL = (SPID_DEGRADED_REASON_CODE_START + 0x10),
    FAILED_TO_VALIDATE_MEMORY_CONFIG = (SPID_DEGRADED_REASON_CODE_START + 0x11),
    DATABASE_VALIDATION_FAILED = (SPID_DEGRADED_REASON_CODE_START + 0x12),

    /*
     **NOTE** These codes are processed by Unsiphere in KH and so not do not change the codes
     * Please follow the process (TBD) to add new codes here
     */
    
} SPID_DEGRADED_REASON, *PSPID_DEGRADED_REASON;


//
// IOCTL control codes
//

//
// A helper macro used to define our ioctls.  The 0xA00 function code base
// value was chosen for no particular reason other than the fact that it
// lies in the customer-reserved number range (according to <devioctl.h>).
//
#define SPID_CTL_CODE(code, method) (\
    EMCPAL_IOCTL_CTL_CODE(EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN, 0xA00 + (code),   \
              (method), EMCPAL_IOCTL_FILE_ANY_ACCESS) )

#define IOCTL_SPID_GET_SPID                \
    SPID_CTL_CODE(1, EMCPAL_IOCTL_METHOD_BUFFERED)
    //  Retrieve the local SP's ID.  The address of a SPID structure
    //  must be passed as the output packet for the ioctl.

#define IOCTL_SPID_SET_ARRAY_WWN                \
    SPID_CTL_CODE(3, EMCPAL_IOCTL_METHOD_BUFFERED)
    //  Modify the Array WWN component of the local SP's ID.  The address of a 
    //  K10_ARRAY_ID structure must be passed as the input packet for the ioctl.

#define IOCTL_SPID_PANIC_SP \
    SPID_CTL_CODE(4, EMCPAL_IOCTL_METHOD_BUFFERED)
    //  Panic the SP.

#define IOCTL_SPID_GET_HW_TYPE   \
    SPID_CTL_CODE(8, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL determines if the hardware is
    // X1 or X1 Lite

#define IOCTL_SPID_GET_PEER_INSERTED   \
    SPID_CTL_CODE(10, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL determines if the peer SP is inserted

#define IOCTL_SPID_GET_HW_NAME   \
    SPID_CTL_CODE(11, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_SPID_GET_KERNEL_TYPE   \
    SPID_CTL_CODE(12, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL determines if the kernel type is 
    // Free Uniprocessor, Free Multiprocessor or Checked

#define IOCTL_SPID_GET_KERNEL_NAME   \
    SPID_CTL_CODE(13, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_SPID_IS_SINGLE_SP_SYSTEM   \
    SPID_CTL_CODE(14, EMCPAL_IOCTL_METHOD_BUFFERED)

#define IOCTL_SPID_GET_GPIO   \
    SPID_CTL_CODE(15, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL returns the value of a given GPIO pin

#define IOCTL_SPID_SET_GPIO   \
    SPID_CTL_CODE(16, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL sets a specified GPIO pin to a specified value

#define IOCTL_SPID_GET_GPIO_ATTR   \
    SPID_CTL_CODE(17, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL gets the attributes of a given GPIO

#define IOCTL_SPID_GET_LAST_REBOOT_TYPE   \
    SPID_CTL_CODE(20, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL gets the type of last reboot

#define IOCTL_SPID_SET_FAULT_STATUS_CODE   \
    SPID_CTL_CODE(21, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL write a status code to be
    // read from the fault status register

#define IOCTL_SPID_IS_IO_MODULE_SUPPORTED \
    SPID_CTL_CODE(23, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL returns whether a particular IO Module type is supported
    // by the OS image on the SP.  It does NOT imply that Flare supports
    // that IO Module type.

#define IOCTL_SPID_SET_FLUSH_EVENT_HANDLE \
    SPID_CTL_CODE(24, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL is used to pass a user mode event handle to SPID which can be used to signal
    // K10_DGSSP application flush the file system/registry before doing a hard reset or shutdown

#define IOCTL_SPID_GET_HW_TYPE_EX   \
    SPID_CTL_CODE(27, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL determines the hardware type

#define IOCTL_SPID_SET_DEGRADED_MODE_REASON \
    SPID_CTL_CODE(28, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL sets the degraded mode reason.

#define IOCTL_SPID_GET_DEGRADED_MODE_REASON \
    SPID_CTL_CODE(29, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL gets the degraded mode reason

#define IOCTL_SPID_IS_OOBE_PHASE_IN_PROGRESS \
    SPID_CTL_CODE(30, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL gets the OOBE Phase status

#define IOCTL_SPID_CLEAR_OOBE_PHASE_IN_PROGRESS_FLAG \
    SPID_CTL_CODE(31, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL clears the OOBE phase in progress

#define IOCTL_SPID_SET_FORCE_DEGRADED_MODE \
    SPID_CTL_CODE(32, EMCPAL_IOCTL_METHOD_BUFFERED)
    // This IOCTL sets the force degraded mode flag

// This IOCTL is used to determine the build type
#define IOCTL_SPID_IS_RELEASE_BUILD \
    SPID_CTL_CODE(33, EMCPAL_IOCTL_METHOD_BUFFERED)

#ifdef C4_INTEGRATED
// This IOCTL determines if the peer SP is cache card
#define IOCTL_SPID_PEER_IS_CACHE_CARD \
    SPID_CTL_CODE(34, EMCPAL_IOCTL_METHOD_BUFFERED)
#endif /* C4_INTEGRATED - C4HW */

#ifndef ALAMOSA_WINDOWS_ENV
#define IOCTL_SPIDX_GPIO_SET SPID_CTL_CODE(64, EMCPAL_IOCTL_METHOD_BUFFERED)
#define IOCTL_SPIDX_GPIO_GET SPID_CTL_CODE(65, EMCPAL_IOCTL_METHOD_BUFFERED)
#endif /* C4_INTEGRATED - C4ARCH *//////////////////////////////////////////////////////////////////////////////

#endif // __SPID_TYPES__
