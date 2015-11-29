/***************************************************************************
 * Copyright (C) EMC Corp 2003 - 2014
 *
 * All rights reserved.
 * 
 * This software contains the intellectual property of EMC Corporation or
 * is licensed to EMC Corporation from third parties. Use of this software
 * and the intellectual property contained therein is expressly limited to
 * the terms and conditions of the License Agreement under which it is
 * provided by or on behalf of EMC
 **************************************************************************/

/***********************************************************************
 *  cpd_interface.h
 ***********************************************************************
 *
 *  DESCRIPTION:
 *      This file contains definitions required by drivers that need
 *      to interface with the Multi-Protocol Dual-Mode OS Wrapper.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      20 Nov 02   DCW     Created
 *      07 Feb 07   Cloke   64Bit Porting
 *      02 May 08   JSK     vlan code merge
 ***********************************************************************/

/* Make sure we only include this file once */

#ifndef CPD_INTERFACE_H
#define CPD_INTERFACE_H

/* Enable support for old interface for initiators (i.e., FC_xxx IOCTLs) */
#define CPD_INITIATOR_COMPATIBILITY




/***********************************************************************
 * INCLUDE FILES
 ***********************************************************************/
#include "generics.h"
#include "cpd_user.h"
#include "cpd_interface_extension.h"
#ifdef ALAMOSA_WINDOWS_ENV
#include "storport.h"         /* For SCSI_REQUEST_BLOCK */
#include <ntddstor.h>         /* For STORAGE_DEVICE_DESCRIPTOR */
#endif /* ALAMOSA_WINDOWS_ENV */
#ifdef CPD_INITIATOR_COMPATIBILITY

#ifndef SYMM_FAMILY

#include "core_config.h"    /* for MAX_INITIATORS_PER_PORT for minishare.h */
#include "icdmini.h"        /* for NTBE_SRB, NTBE_SGL */
#include "k10defs.h"        /* for K10_WWN and K10_WWID */

#else

typedef UINT_64E K10_WWN // original in  k10defs.h.  Redefined for SYMM.

#endif //SYMM_FAMILY


#ifdef ALAMOSA_WINDOWS_ENV 
#include "ntddscsi.h"       /* For SRB_IO_CONTROL */

typedef STORAGE_BUS_TYPE                CPD_STORAGE_BUS_TYPE;
typedef PSTORAGE_BUS_TYPE               PCPD_STORAGE_BUS_TYPE;

typedef STORAGE_DEVICE_DESCRIPTOR       CPD_DEVICE_DESCRIPTOR;
typedef PSTORAGE_DEVICE_DESCRIPTOR      PCPD_DEVICE_DESCRIPTOR;

typedef STORAGE_ADAPTER_DESCRIPTOR      CPD_ADAPTER_DESCRIPTOR;
typedef PSTORAGE_ADAPTER_DESCRIPTOR     PCPD_ADAPTER_DESCRIPTOR;

typedef STORAGE_DEVICE_DESCRIPTOR       CPD_DEVICE_DESCRIPTOR;
typedef PSTORAGE_DEVICE_DESCRIPTOR      PCPD_DEVICE_DESCRIPTOR;

typedef STORAGE_ADAPTER_DESCRIPTOR      CPD_ADAPTER_DESCRIPTOR;
typedef PSTORAGE_ADAPTER_DESCRIPTOR     PCPD_ADAPTER_DESCRIPTOR;


#define CPD_S_PASS_THROUGH           SCSI_PASS_THROUGH
#define CPD_S_PASS_THROUGH_DIRECT    SCSI_PASS_THROUGH_DIRECT
#define CPD_S_IO_CAPABILITIES        IO_SCSI_CAPABILITIES
#define CPD_STOR_PROPERTY_ID         STORAGE_PROPERTY_ID
#define CPD_STOR_PROPERTY_QUERY      STORAGE_PROPERTY_QUERY
#define CPD_SCSI_ADDRESS             SCSI_ADDRESS

typedef SCSI_ADDRESS *               PCPD_SCSI_ADDRESS;

#else //Linux builds

typedef enum CPD_STORAGE_BUS_TYPE_TAG {
    BusTypeUnknown = 0x00,
    BusTypeScsi,
    BusTypeAtapi,
    BusTypeAta,
    BusType1394,
    BusTypeSsa,
    BusTypeFibre,
    BusTypeUsb,
    BusTypeRAID,
    BusTypeiScsi,
    BusTypeSas,
    BusTypeSata,
    BusTypeMaxReserved = 0x7F
} CPD_STORAGE_BUS_TYPE,
*PCPD_STORAGE_BUS_TYPE;

typedef struct CPD_DEVICE_DESCRIPTOR_TAG {
    ULONG Version;
    ULONG Size;
    UCHAR DeviceType;
    UCHAR DeviceTypeModifier;
    BOOLEAN RemovableMedia;
    BOOLEAN CommandQueueing;
    ULONG VendorIdOffset;
    ULONG ProductIdOffset;
    ULONG ProductRevisionOffset;
    ULONG SerialNumberOffset;
    CPD_STORAGE_BUS_TYPE BusType;
    ULONG RawPropertiesLength;
    UCHAR RawDeviceProperties[1];
} CPD_DEVICE_DESCRIPTOR,
*PCPD_DEVICE_DESCRIPTOR;

typedef struct CPD_ADAPTER_DESCRIPTOR_TAG {
    ULONG Version;
    ULONG Size;
    ULONG MaximumTransferLength;
    ULONG MaximumPhysicalPages;
    ULONG AlignmentMask;
    BOOLEAN AdapterUsesPio;
    BOOLEAN AdapterScansDown;
    BOOLEAN CommandQueueing;
    BOOLEAN AcceleratedTransfer;
    UCHAR BusType;
    USHORT BusMajorVersion;
    USHORT BusMinorVersion;
} CPD_ADAPTER_DESCRIPTOR,
*PCPD_ADAPTER_DESCRIPTOR;


/* For SCSI IO OPERATION */
typedef struct {
  ULONG  Length;
  ULONG  MaximumTransferLength;
  ULONG  MaximumPhysicalPages;
  ULONG  SupportedAsynchronousEvents;
  ULONG  AlignmentMask;
  BOOLEAN  TaggedQueuing;
  BOOLEAN  AdapterScansDown;
  BOOLEAN  AdapterUsesPio;
} CPD_S_IO_CAPABILITIES;

typedef struct {
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG_PTR DataBufferOffset;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
} CPD_S_PASS_THROUGH;

typedef struct {
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    PVOID DataBuffer;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
} CPD_S_PASS_THROUGH_DIRECT;

typedef enum {
    StorageDeviceProperty = 0,
    StorageAdapterProperty,
    StorageDeviceIdProperty
} CPD_STOR_PROPERTY_ID;

typedef enum {
    PropertyStandardQuery = 0,
    PropertyExistsQuery,
    PropertyMaskQuery,
    PropertyQueryMaxDefined
} CPD_STOR_QUERY_TYPE;

typedef struct {
    CPD_STOR_PROPERTY_ID PropertyId;
    CPD_STOR_QUERY_TYPE QueryType;
    UCHAR AdditionalParameters[1];
} CPD_STOR_PROPERTY_QUERY;

typedef struct {
    ULONG Length;
    UCHAR PortNumber;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
} CPD_SCSI_ADDRESS, *PCPD_SCSI_ADDRESS;

#define RequestComplete 0
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - scsi */

/*! scsi notification type */
typedef enum {
    REQUEST_COMPLETE = RequestComplete,
    NEXT_REQUEST,
    NEXT_LU_REQUEST,
    RESET_DETECTED,
    CALL_DISABLE_INTERRUPTS,
    CALL_ENABLE_INTERRUPTS,
    REQUEST_TIMER_CALL,
    BUS_CHANGE_DETECTED,
    WMI_EVENT,
    WMI_REREGISTER,
    LINK_UP,
    LINK_DOWN,
    QUERY_TICK_COUNT,
    BUFFER_OVERRUN_DETECTED,
    ACQUIRE_SPINLOCK,
    RELEASE_SPINLOCK,
    ISSUE_DPC,
    ENABLE_PASSIVE_INIT
} CPD_NOTIFICATION_TYPE;

#endif  /* CPD_INITIATOR_COMPATIBILITY */


/***********************************************************************
 * EXTERNAL STRUCTURE FORWARD DEFINITIONS
 ***********************************************************************/

struct _SCSI_REQUEST_BLOCK;
struct _ACCEPT_CCB;
struct _CCB;
struct _SGL;
struct FC_DMRB_TAG;
struct CPD_DMRB_TAG;


/***********************************************************************
 * CONSTANT DEFINES
 ***********************************************************************/

/* Define a device type for Flare Boot devices -
 * used as vendor-specific device type in inquiry string.
 */

#define FLARE_NT_BOOT_DEVICE                0x1D
#define SCSIINIT_DEVICE                     0x1E
/* from minishare.h
 * #define SCSITARG_DEVICE     0x1F        // TCD Target device.
 * #define FLAREDISK_DEVICE    0x1E        // NTBE Disk device.
*/
/* The loop ID for which we return a SCSITART or CMI inquiry string */
#define CPD_SCSITARG_LOOP_ID                126
#define CPD_CMI_LOOP_ID                     127
#define CPD_MAX_INQUIRY_LOOP_ID             127


#ifdef CPD_INITIATOR_COMPATIBILITY

/* Define FCP_CMND FCP_CNTL Task Attribute settings to be passed up */

#define FC_UNTAGGED                         0   // Note TCD uses bool tests
                                                // for untagged - so must be 0
#define FC_SIMPLE_QUEUE_TAG                 SCSIMESS_SIMPLE_QUEUE_TAG
#define FC_HEAD_OF_QUEUE_TAG                SCSIMESS_HEAD_OF_QUEUE_TAG
#define FC_ORDERED_QUEUE_TAG                SCSIMESS_ORDERED_QUEUE_TAG
#define FC_ACA_QUEUE_TAG                    0x2f

 
/* Define aborts that may be used by FC_IOCTL_ABORT_IO */
#define FC_ABORT_ABTS                       0x7001
#define FC_ABORT_TERMINATE_TASK             0x7002
#define FC_ABORT_ABORT_TASK_SET             0x7003
#define FC_ABORT_CLEAR_TASK_SET             0x7004
#define FC_ABORT_TARGET_RESET               0x7005
#define FC_ABORT_TPRLO                      0x7006
#define FC_ABORT_GTPRLO                     0x7007
#define FC_ABORT_LIP                        0x7008
#define FC_ABORT_TARGETED_LIP_RESET         0x7009


/*  Return codes for FC_IOCTL_EXECUTE_IO and FC_IOCTL_ABORT_IO */
#define FC_REQUEST_STATUS_SUCCESS           0x8001
#define FC_REQUEST_STATUS_FAILED            0x8002
#define FC_REQUEST_STATUS_TIMEOUT           0x8003

#endif  /* CPD_INITIATOR_COMPATIBILITY */

/***********************************************************************
 * IOCTL codes
 ***********************************************************************/

/* Initiator mode IOCTLs
 * defined in miniport\inc\icdmini.h
 * values are 0x90000081 - 0x90000086 and 0x900000F0
 */

#ifdef CPD_INITIATOR_COMPATIBILITY

#define FC_IOCTL_INITIATOR_LOOP_COMMAND     CIM_CM_LOOP_COMMAND
#define     FC_LOOP_COMMAND_DISCOVER_LOOP       CM_DISCOVER_LOOP
#define     FC_LOOP_COMMAND_ENABLE_LOOP         CM_ENABLE_LOOP
#define     FC_LOOP_COMMAND_DISABLE_LOOP        CM_DISABLE_LOOP
#define     FC_LOOP_COMMAND_RESET_LOOP          CM_RESET_LOOP
#define     FC_LOOP_COMMAND_LOGIN_LOOP          CM_LOGIN_LOOP
#define     FC_LOOP_COMMAND_HOLD_LOOP           CM_HOLD_LOOP
#define     FC_LOOP_COMMAND_RESUME_LOOP         CM_RESUME_LOOP
#define     FC_LOOP_COMMAND_FAIL_LOOP           CM_FAIL_LOOP
#define FC_IOCTL_INITIATOR_RESET_DEVICE     CIM_RESET_DEVICE
#define FC_IOCTL_RESET_BUS                  CIM_BUS_RESET

/* Remote access mode IOCTLs
 * defined in this file
 * values are 0xA0000001 - 0xA0000004
 */

#define FC_IOCTL_GET_REGISTERED_WWID_LIST_SIZE  0xA0000001
#define FC_IOCTL_GET_REGISTERED_WWID_LIST       0xA0000002
#define FC_IOCTL_ADD_WWID_PATH                  0xA0000003
#define FC_IOCTL_OPEN_WWID_PATH                 0xA0000004
#define FC_IOCTL_GET_WWID_PATH                  0xA0000005
#define FC_IOCTL_ABORT_IO                       0xA0000009
#define FC_IOCTL_GET_LESB                       0xA000000A

#define FC_IOCTL_PROCESS_DMRB                   0xA0000012
#define FC_IOCTL_REGISTER_REDIRECTION           0xA0000013
#define FC_IOCTL_DEREGISTER_REDIRECTION         0xA0000014
#define FC_IOCTL_ERROR_INSERTION                0xA0000015

#endif  /* CPD_INITIATOR_COMPATIBILITY */


#ifdef CPD_INITIATOR_COMPATIBILITY

/* Allow a default FC_TYPE to be specified without knowing
 * anything about the details of Fibre Channel.
 * This default type will be used to set the type to FC_TYPE_FCP.
 */

#define FC_TYPE_DEFAULT                     0xFFFF0DEF

#endif  /* CPD_INITIATOR_COMPATIBILITY */


/***********************************************************************
 * IOCTL error return codes returned in SRB_IO_CONTROL.ReturnCode
 ***********************************************************************/

#define CPD_RC_BASE                             0x01806000

#define CPD_RC_NO_ERROR                         0x0

#define CPD_RC_PARAMETER_BLOCK_TOO_SMALL        (CPD_RC_BASE + 0x001)
#define CPD_RC_IOCTL_TIMEOUT                    (CPD_RC_BASE + 0x002)
#define CPD_RC_LINK_DOWN                        (CPD_RC_BASE + 0x003)
#define CPD_RC_BUSY                             (CPD_RC_BASE + 0x004)
#define CPD_RC_BAD_RESPONSE                     (CPD_RC_BASE + 0x005)
#define CPD_RC_TARGET_ENABLED                   (CPD_RC_BASE + 0x006)
#define CPD_RC_TARGET_ALREADY_REGISTERED        (CPD_RC_BASE + 0x007)
#define CPD_RC_TARGET_NOT_ENABLED               (CPD_RC_BASE + 0x008)
#define CPD_RC_DIFFERENT_TARGET_REGISTERED      (CPD_RC_BASE + 0x009)
#define CPD_RC_DRIVER_NOT_ENABLED               (CPD_RC_BASE + 0x00a)
#define CPD_RC_NO_CALLBACK_SLOT_AVAILABLE       (CPD_RC_BASE + 0x00b)
#define CPD_RC_CALLBACK_NOT_FOUND               (CPD_RC_BASE + 0x00c)
#define CPD_RC_INVALID_SIGNATURE                (CPD_RC_BASE + 0x00d)
#define CPD_RC_INVALID_TARGET_CALLBACK          (CPD_RC_BASE + 0x00e)
#define CPD_RC_COUNT_ALREADY_ZERO               (CPD_RC_BASE + 0x00f)
#define CPD_RC_INVALID_LOOP_COMMAND             (CPD_RC_BASE + 0x010)
#define CPD_RC_UNKNOWN_DEVICE                   (CPD_RC_BASE + 0x011)
#define CPD_RC_DEVICE_NOT_READY                 (CPD_RC_BASE + 0x012)
#define CPD_RC_NO_FREE_DEVICE_ID                (CPD_RC_BASE + 0x013)
#define CPD_RC_DEVICE_ID_NOT_FOUND              (CPD_RC_BASE + 0x014)
#define CPD_RC_DEVICE_NOT_FOUND                 (CPD_RC_BASE + 0x015)
#define CPD_RC_IOCTL_NOT_SUPPORTED              (CPD_RC_BASE + 0x016)
#define CPD_RC_COMMAND_FAILED                   (CPD_RC_BASE + 0x017)
#define CPD_RC_SRB_NOT_FOUND                    (CPD_RC_BASE + 0x018)
#define CPD_RC_DEVICE_ALREADY_LOGGED_IN         (CPD_RC_BASE + 0x019)
#define CPD_RC_INVALID_TIMEOUT                  (CPD_RC_BASE + 0x01a)
#define CPD_RC_REDIRECTION_ALREADY_REGISTERED                \
                                                (CPD_RC_BASE + 0x01b)
#define CPD_RC_NO_REDIRECTION_SLOT_AVAILABLE    (CPD_RC_BASE + 0x01c)
#define CPD_RC_TARGET_NOT_REDIRECTED            (CPD_RC_BASE + 0x01d)
#define CPD_RC_NO_MATCHING_REDIRECTION          (CPD_RC_BASE + 0x01e)
#define CPD_RC_NO_TARGET_CONNECTION             (CPD_RC_BASE + 0x01f)
#define CPD_RC_NO_TYPE_SPECIFIED                (CPD_RC_BASE + 0x020)
#define CPD_RC_CM_INTERFACE_ALREADY_REGISTERED               \
                                                (CPD_RC_BASE + 0x021)
#define CPD_RC_ILLEGAL_CALLBACKS                (CPD_RC_BASE + 0x022)
#define CPD_RC_NO_CALLBACKS                     (CPD_RC_BASE + 0x023)
#define CPD_RC_DEVICE_NOT_LOGGED_IN             (CPD_RC_BASE + 0x024)
#define CPD_RC_AUTHENTICATION_ALREADY_REGISTERED            \
                                                (CPD_RC_BASE + 0x025)
#define CPD_RC_SEPARATE_NAMES_NOT_SUPPORTED     (CPD_RC_BASE + 0x026)
#define CPD_RC_LOGIN_NAME_REQUIRED              (CPD_RC_BASE + 0x027)
#define CPD_RC_NO_ERROR_CALLBACK_PENDING        (CPD_RC_BASE + 0x028)
#define CPD_RC_BUFFER_TOO_SMALL                 (CPD_RC_BASE + 0x029)
#define CPD_RC_NO_RESOURCES                     (CPD_RC_BASE + 0x02a)
#define CPD_RC_INITIATOR_NOT_ENABLED            (CPD_RC_BASE + 0x02b)
#define CPD_RC_LOGIN_IN_PROGRESS                (CPD_RC_BASE + 0x02c)
#define CPD_RC_INVALID_PORTAL                   (CPD_RC_BASE + 0x02d)
#define CPD_RC_DUPLICATE_REJECTED               (CPD_RC_BASE + 0x02e)
#define CPD_RC_FEATURE_NOT_SUPPORTED            (CPD_RC_BASE + 0x02f)
#define CPD_RC_INVALID_OPERATION                (CPD_RC_BASE + 0x030)
#define CPD_RC_INVALID_STATE                    (CPD_RC_BASE + 0x031)
#define CPD_RC_INVALID_KEY                      (CPD_RC_BASE + 0x032)
#define CPD_RC_INCOMPATIBLE_HW_OR_FW            (CPD_RC_BASE + 0x033)
#define CPD_RC_IOCTL_REVISION_MISMATCH          (CPD_RC_BASE + 0x034)
#define CPD_RC_DATA_UNDERRUN                    (CPD_RC_BASE + 0x035)
#define CPD_RC_DATA_OVERRUN                     (CPD_RC_BASE + 0x036)

#define CPD_RC_INVALID_PARAMETER_1              (CPD_RC_BASE + 0x081)
#define CPD_RC_INVALID_PARAMETER_2              (CPD_RC_BASE + 0x082)
#define CPD_RC_INVALID_PARAMETER_3              (CPD_RC_BASE + 0x083)
#define CPD_RC_INVALID_PARAMETER_4              (CPD_RC_BASE + 0x084)
#define CPD_RC_INVALID_PARAMETER_5              (CPD_RC_BASE + 0x085)
#define CPD_RC_INVALID_FLAG                     (CPD_RC_BASE + 0x091)

#ifdef CPD_INITIATOR_COMPATIBILITY

/***********************************************************************
 * Define flags for use with FC_IOCTL_ADD_WWID_PATH
 ***********************************************************************/

/* Function codes for use with FC_DMRB */
#define FC_LOGIN_DEVICE                     0x00000001
#define FC_INITIATOR_NO_DATA                0x00000002
#define FC_INITIATOR_DATA_IN                0x00000003
#define FC_INITIATOR_DATA_OUT               0x00000004
#define FC_TARGET_COMMAND                   0x00000005
#define FC_TARGET_DATA_OUT                  0x00000006
#define FC_TARGET_DATA_IN                   0x00000008
#define FC_TARGET_STATUS                    0x00000010
#define FC_TARGET_DATA_IN_AND_STATUS        \
                                (FC_TARGET_DATA_IN | FC_TARGET_STATUS)

/* Flags for use with FC_DMRB */
#define FC_DMRB_FLAGS_PHYS_SGL              0x00000001
#define FC_DMRB_FLAGS_INITIATOR_AND_TARGET  0x00000002
#define FC_DMRB_FLAGS_QUICK_RECOVERY        0x00000004
#define FC_DMRB_FLAGS_SKIP_SECTOR_OVERHEAD_BYTES    0x00000008
#define FC_DMRB_FLAGS_ABORTED_BY_UL         0x00010000

/* Statuses for use with FC_DMRB */
    // Immediate or completion return values
#define FC_STATUS_GOOD                      0x00010001
#define FC_STATUS_INVALID_REQUEST           0x00010002
    // Immediate return values
#define FC_STATUS_PENDING                   0x00020001
#define FC_STATUS_INSUFFICIENT_RESOURCES    0x00020002
#define FC_STATUS_UNKNOWN_DEVICE            0x00020003
#define FC_STATUS_DEVICE_NOT_FOUND          0x00020004
#define FC_STATUS_DEVICE_NOT_LOGGED_IN      0x00020005
#define FC_STATUS_DEVICE_BUSY               0x00020006
    // Completion return values
#define FC_STATUS_BAD_REPLY                 0x00030001
#define FC_STATUS_TIMEOUT                   0x00030002
#define FC_STATUS_DEVICE_NOT_RESPONDING     0x00030003
#define FC_STATUS_LINK_FAIL                 0x00030004
#define FC_STATUS_OVERRUN                   0x00030005
#define FC_STATUS_UNDERRUN                  0x00030006
#define FC_STATUS_ABORTED_BY_UPPER_LEVEL    0x00030007
#define FC_STATUS_ABORTED_BY_DEVICE         0x00030008
#define FC_STATUS_UNSUPPORTED               0x00030009
#define FC_STATUS_SCSI_STATUS               0x0003000A
#define FC_STATUS_CHECK_CONDITION           0x0003000B

/* This flag specifies target functionality in addition to the default
 * initiator functionality.
 */
#define FC_ADD_PATH_INITIATOR_ONLY          0x00000000
#define FC_ADD_PATH_INITIATOR_AND_TARGET    0x00000001

/* This flag changes the nature of the FC_IOCTL_ADD_WWID_PATH from
 * a synchronous command to an asynchronous one.
 */
#define FC_ADD_PATH_ASYNCHRONOUS            0x00000000
#define FC_ADD_PATH_SYNCHRONOUS             0x00000002

/* This flag indicates that the ADD_WWID_PATH IOCTL was given the
 * WWID path for a device already logged in
 */
#define FC_ADD_PATH_DEVICE_ALREADY_LOGGED_IN        \
                                            0x80000000

/* Flags for use with FC_IOCTL_REGISTER_REDIRECTION */
#define FC_REDIRECTION_STD_NT_IO_ONLY       0x0000
#define FC_REDIRECTION_ALL_IO               0x0001

/* Flags for FC_IOCTL_GET_LESB */
#define FC_LESB_CLEAR_STATS                 0x00000001

#endif  /* CPD_INITIATOR_COMPATIBILITY */

/* Port Failover flags used in
 * GET_PORT_INFO
 */

/* Does this physical port support failover (based on protocol) ?
 * -NA- for virtual ports.
 */
#define CPD_PORT_INFO_PORT_FAILOVER_CAPABLE             0x00000001

/* Is this physical port ready to accept a failover ?
 * -NA- for virtual ports.
 */
#define CPD_PORT_INFO_PORT_FAILOVER_ACCEPT_READY        0x00000002

/* Is this virtual port successfully set up ?
 * -NA- for physical ports.
 */
#define CPD_PORT_INFO_PORT_FAILOVER_ACTIVE              0x00000004

/***********************************************************************
 * STRUCTURE DEFINES
 ***********************************************************************/

typedef enum
{
    CPD_CM_LOOP_DRIVE_CHANGE,
    CPD_CM_LOOP_EXPANDER_CHANGE,
    CPD_CM_LOOP_DISCOVERY_COMPLETE
}
CPD_CM_LOOP_CHANGE_TYPE;


#ifdef CPD_INITIATOR_COMPATIBILITY

/***********************************************************************
 * Structure used as DMD context for either initiator or target I/Os.
 ***********************************************************************/

typedef struct FC_DMRB_TAG
{
    struct FC_DMRB_TAG * prev_ptr;      /* used by miniport to . . .   */
    struct FC_DMRB_TAG * next_ptr;      /* . . . put on XCB wait queue */
    UINT_32E        revision;           /* currently 1 */
    UINT_32E        function;           /* request type */
    UINT_32E        target;             /* loop ID of target device */
    UINT_32E        lun;                /* target LUN */
    UINT_8E         access_mode;        /* type of LUN addressing */
    UINT_8E         queue_action;       /* type of queueing */
    UINT_8E         cdb_length;         /* number of valid bytes in CDB */
    UINT_8E         cdb[16];            /* CDB! */
    UINT_32E        flags;
    UINT_32E        timeout;            /* timeout value in seconds */
    void          * miniport_context;   /* DMD exchange context */
    UINT_32E        data_length;        /* number of bytes to be transferred */
    UINT_32E        data_transferred;   /* actual number of  bytes xferred */
    struct _SGL   * sgl;                /* pointer to Flare SGL */
    UINT_32E        sgl_offset;         /* byte offset into 1st SGL element */
    UINT_64E        physical_offset;    /* offset of phys addr from virtual */
    void            (* callback) (void *, void *);  /* completion callback */
    void          * callback_arg1;      /* 1st argument to callback function */
    void          * callback_arg2;      /* 2nd argument to callback function */
    UINT_32E        status;             /* request status */
    UINT_8E         scsi_status;        /* SCSI status byte */
    UINT_8E         sense_length;       /* nr of valid bytes in sense_buffer */
    void          * sense_buffer;       /* SCSI sense informaton */
} FC_DMRB;


/***********************************************************************
 * Structure used by FC_IOCTL_GET_REGISTERED_WWID_LIST and
 * FC_IOCTL_GET_WWID_PATH
 ***********************************************************************/

typedef struct FC_WWID_TAG
{
    K10_WWN         port_name;          /* 64-bit FC port world-wide name */
    K10_WWN         node_name;          /* 64-bit FC node world-wide name */
    UINT_32E        address_id;         /* 24-bit FC address ID for device.
                                         * Used by miniport only.
                                         */
} FC_WWID, * PFC_WWID;


/***********************************************************************
 * Structure passed by FC_IOCTL_GET_REGISTERED_WWID_LIST_SIZE and
 * FC_IOCTL_GET_REGISTERED_WWID_LIST.
 ***********************************************************************/

typedef struct FC_IOCTL_WWID_LIST_BLOCK_TAG
{
    UINT_32E        query_type;         /* Type of query (from FC header).
                                         * Filled in by caller.
                                         */
    UINT_32E        list_size;          /* Number of bytes in list.
                                         * Filled in by caller or by call to
                                         * GET_REGISTERED_WWID_LIST_SIZE
                                         */
    FC_WWID       * device_list_ptr;    /* Pointer to list of devices.
                                         * Pointer filled in by caller;
                                         * List filled in by call to
                                         * GET_REGISTERED_WWID_LIST
                                         */
} FC_IOCTL_WWID_LIST_BLOCK, * PFC_IOCTL_WWID_LIST_BLOCK;


/***********************************************************************
 * Structure passed by FC_IOCTL_REMOVE_WWID_PATH and
 * used by FC_IOCTL_GET_WWID_PATH
 ***********************************************************************/

typedef struct FC_DEVICE_PATH_TAG
{
//    UINT_PTR        path;
//    UINT_PTR        target;
    UINT_8E         path;
    UINT_8E         target;
    UINT_8E         lun;                /* unused by miniport */
    UINT_8E         reserved;
} FC_DEVICE_PATH, * PFC_DEVICE_PATH;


/***********************************************************************
 * Structure passed by FC_IOCTL_ADD_WWID_PATH and FC_IOCTL_GET_WWID_PATH
 ***********************************************************************/

typedef struct FC_IOCTL_WWID_PATH_BLOCK_TAG
{
    FC_WWID       * wwid_ptr;           /* Pointer to device information
                                         * of device to login.
                                         * Filled in by caller.
                                         */
    FC_DEVICE_PATH  path;               /* Path of device described by
                                         * wwid_ptr.  Filled in by miniport.
                                         */
    UINT_32E        flags;              /* Option flags for ADD_WWID_PATH
                                         * operation.  Filled in by caller.
                                         */
    void          * callback_handle;    /* Used with REMOVE_WWID_PATH to
                                         * indicate which driver is requesting
                                         * removal.
                                         */
} FC_IOCTL_WWID_PATH_BLOCK, * PFC_IOCTL_WWID_PATH_BLOCK;


/***********************************************************************
 * Structure passed by FC_IOCTL_DEREGISTER_DM_CALLBACKS and
 * FC_IOCTL_PROCESS_DMRB
 ***********************************************************************/

typedef struct FC_CALLBACK_HANDLE_TAG
{
    void *      callback_handle;
} FC_CALLBACK_HANDLE;


/***********************************************************************
 * Structure passed by FC_IOCTL_REGISTER_REDIRECTION
 ***********************************************************************/

typedef struct FC_REDIRECTION_BLOCK_TAG
{
    UINT_16E    revision;           /* structure revision - currently 1 */
    UINT_16E    flags;
    UINT_32E    target;             /* destination loop ID */
    void      * context;            /* class driver context */
    void        (*redirect) (void *, CPD_SCSI_REQ_BLK *, void *);
    void        (*execute) (void *, FC_DMRB *);
    void        (*completion) (void *);
} FC_REDIRECTION_BLOCK;


/***********************************************************************
 * Structure passed by FC_IOCTL_DEREGISTER_REDIRECTION
 ***********************************************************************/

typedef struct FC_UNSET_REDIRECTION_BLOCK_TAG
{
    UINT_32E    target;             /* destination loop ID */
    void      * context;            /* class driver context */
} FC_UNSET_REDIRECTION_BLOCK;


/***********************************************************************
 * Structure passed by FC_IOCTL_ABORT_IO
 ***********************************************************************/

typedef struct FC_ABORT_IO_BLOCK_TAG
{
    UINT_32E        abort_code;         /* Specifies type of abort to send */
    UINT_32E        target;             /* Loop ID of target of abort */
    UINT_32E        lun;                /* Target LUN of abort - used for
                                         * ATS and CTS only.
                                         */
    CPD_SCSI_REQ_BLK *
                    srb_ptr;            /* Pointer to SRB affected by abort -
                                         * used by ABTS and TERMINATE_TASK
                                         * only.
                                         */
    UINT_32E        timeout_value;      /* Number of seconds after which if
                                         * completion callback hasn't been sent
                                         * we will send notification of timeout
                                         */
    UINT_32E        attempts;           /* Number of times to try abort if
                                         * fails or timesout */

    void            (* completion_callback) (OPAQUE_PTR, OPAQUE_PTR);

  /*     void            (* completion_callback) (void *, void *, UINT_32);*/
                                        /* completion callback made by miniport
                                         * after abort request has completed.
                                         */
    OPAQUE_PTR      callback_arg1;      /* First argument to callback */
    OPAQUE_PTR      callback_arg2;      /* Second argument to callback */
} FC_ABORT_IO_BLOCK, * PFC_ABORT_IO_BLOCK;

#endif  /* CPD_INITIATOR_COMPATIBILITY */


        // Revision information is not applicable
#define CPD_INFO_REVISION_NOT_VALID             0xffffffff



/***********************************************************************
 * CPD_REMOTE_TARGET_DATA structure
 ***********************************************************************/

typedef struct CPD_REMOTE_TARGET_DATA_TAG
{
    UINT_16E portal_nr;
    CPD_REMOTE_TARGET_ACTION action;
    BITS_32E type;
    UINT_32E interval;
    UINT_32E command_timeout;
    UINT_32E min_target;
    UINT_32E max_target;
    UINT_32E buffer_size;
    UINT_32E number_of_entries;
    UINT_32E reserved_1[4];
    union 
    {
        CPD_TARGET_INFO_POLL_CONFIG * types;
        CPD_NAME * name;
        CPD_RLS  * rls;
        CPD_RPSC * rpsc;
    } data;
} CPD_REMOTE_TARGET_DATA, CPD_QUERY_TARGET_DEVICE_DATA;

#pragma pack(4)
typedef struct S_CPD_QUERY_TARGET_DEVICE_DATA_TAG
{
    CPD_IOCTL_HEADER      ioctl_hdr;
    CPD_REMOTE_TARGET_DATA   param;
} S_CPD_QUERY_TARGET_DEVICE_DATA, * PS_CPD_QUERY_TARGET_DEVICE_DATA;
#pragma pack()


/***********************************************************************
 * CPD_LUN_ABORTED_PROPERTIES structures
 ***********************************************************************/

typedef struct S_CPD_LUN_ABORTED_PROPERTIES_TAG
{
    CPD_IOCTL_HEADER          ioctl_hdr;
    FC_NAME                 aborted_lun;  // The WWN of the LUN being aborted.
} S_CPD_LUN_ABORTED_PROPERTIES, * PS_CPD_LUN_ABORTED_PROPERTIES;


/***********************************************************************
 * xx_LOGIN_ID structures
 ***********************************************************************/

typedef struct FC_LOGIN_ID_TAG
{
    BITS_32E        flags;                          // field validity bits
    UINT_32E        originator_process_associator;  // from PRLI payload
    UINT_32E        responder_process_associator;   // from PRLI payload
} FC_LOGIN_ID, * PFC_LOGIN_ID;

#pragma pack(4)

typedef struct ISCSI_LOGIN_ID_TAG
{
    UINT_64         initiator_session_id;   // ISID
    UINT_16E        connection_id;          // CID
} ISCSI_LOGIN_ID, * PISCSI_LOGIN_ID;

#pragma pack()

typedef struct SAS_LOGIN_ID_TAG
{
    UINT_8E          unused;
} SAS_LOGIN_ID, * PSAS_LOGIN_ID;

typedef struct FCOE_LOGIN_ID_TAG
{
    BITS_32E    flags;
    UINT_32E    originator_process_associator;
    UINT_32E    responder_process_associator;
} FCOE_LOGIN_ID, *PFCOE_LOGIN_ID;

typedef struct CPD_LOGIN_ID_TAG
{
    union
    {
        FC_LOGIN_ID     fc;
        ISCSI_LOGIN_ID  iscsi;
        SAS_LOGIN_ID    sas;
        FCOE_LOGIN_ID   fcoe;
    } login_id;
} CPD_LOGIN_ID, * PCPD_LOGIN_ID;


/* FC_LOGIN_ID flags bit definitions */

    // originator_process_associator field valid
#define FC_LOGIN_ID_OPA_VALID                   0x00000001

    // responder_process_associator field valid
#define FC_LOGIN_ID_RPA_VALID                   0x00000002



/***********************************************************************
 * CPD_PORT_LOGIN structure
 ***********************************************************************/

/**** CPD_PORT_LOGIN parent_context definitions ****/
#define CPD_LOGIN_DIRECT_ATTACHED               -1

/**** CPD_PORT_LOGIN flags bit definitions ****/
        // Array is initiator of login
#define CPD_LOGIN_INITIATOR                     0x00000001
        // Array is target of login
#define CPD_LOGIN_TARGET                        0x00000002
        // Indicates login is both initiator and target
#define CPD_LOGIN_BIDIRECTIONAL                 0x00000004
        // Indicate that this async callback is informational only,
        // and no action should be taken based on this event (other than
        // possible logging)
#define CPD_LOGIN_INFORMATION_ONLY              0x00000008

        // This login is for second (or more) connection of an existing session
#define CPD_LOGIN_EXISTING_ISCSI_SESSION        0x00000010
        // This login is for an ISCSI discovery session
#define CPD_LOGIN_ISCSI_DISCOVERY_SESSION       0x00000020
        // Indicates initiator login supports CHAP authentication
#define CPD_LOGIN_AUTH_CHAP                     0x00000040
        // Enable reverse authentication on a per-target basis
#define CPD_LOGIN_AUTHENTICATE_TARGET           0x00000080

        // Enable iscsi negotiations to use header
#define CPD_LOGIN_HEADER_DIGESTS                0x00000100
        // Enable iSCSI negotiations to use data
#define CPD_LOGIN_DATA_DIGESTS                  0x00000200
        // This login will logout an existing session
#define CPD_LOGIN_REESTABLISH_LOGIN             0x00000400
        // Perform quicker recovery at the cost of less exhausting recovery
#define CPD_LOGIN_QUICK_RECOVERY                0x00000800

        // Use the miniport_login_context field as a loopmap index instead of
        //   the address field to identify the target device
#define CPD_LOGIN_LOOPMAP_INDEX_VALID           0x00001000
        // The name field in CPD_PORT_LOGIN is valid
#define CPD_LOGIN_NAME_VALID                    0x00002000

        // The device logging in is a SAS expander.
#define CPD_LOGIN_EXPANDER_DEVICE               0x00004000
        // The device logging in is a virtual device. (typically an expander SES port)
#define CPD_LOGIN_VIRTUAL_DEVICE                0x00008000
        // The device logging in is a SATA end device.
#define CPD_LOGIN_SATA_DEVICE                   0x00010000

        // This flag when set on a CPD_DEVICE_EVENT_LOGIN means that the login
        // was not totally successful, and the reason is specified in the
        // CPD_PORT_LOGIN.more_info.reason field.
#define CPD_LOGIN_PLACEHOLDER                   0x00020000

        // This flag is sent by NTmirror to indicate this is a login
        // request for one of the boot devices [CPD_IOCTL_ADD_DEVICE]. This flag is 
        // then interpretted by PPFD, to modify the retry count for non-boot devices,
        // thus eliminating unneccesary delays during boot. 
#define CPD_LOGIN_BOOT_DEVICE                   0x00040000

        // let MCR drivers know the OS requested us to shiut dwon so any logouts it may
        // see are because we are going down and not a topology failure
#define CPD_LOGIN_SHUTDOWN_LOGOUT               0x00080000

        // Don't send an immediate LOGIN notification if device already
        // logged in         **** FOR INTERNAL USE ONLY ****
#define CPD_LOGIN_DONT_SEND_IMMEDIATE_LOGIN_NOTIFICATION    0x80000000

        // Field (miniport_login_context, login_qualifier) is invalid
        // because establishing new login
//#ifdef _AMD64_
//#define CPD_LOGIN_NEW_LOGIN                     (void *)0xffffffffffffffff
//#else
//#define CPD_LOGIN_NEW_LOGIN                     (void *)0xffffffff
//#endif

// This is cleaner than the above and eliminates compilation issues automatically - CLoke 02/07/07
#define CPD_LOGIN_NEW_LOGIN                     (UINT_PTR)0xffffffffffffffffULL

        // Login qualifier field is unused for Fibre Channel
        // because establishing new login
#define CPD_NO_LOGIN_QUALIFIER                  0
#define CPD_LOGIN_NEW_LOGIN_QUALIFIER           0xffffffff

typedef struct CPD_PORT_LOGIN_TAG
{
    UINT_16E            portal_nr;
    CPD_NAME          * name;
    CPD_ADDRESS       * address;
    void              * portal_context;
    void              * miniport_login_context;
    void              * parent_context;
    UINT_32E            login_qualifier;
    CPD_LOGIN_ID        login_id;
    BITS_32E            flags;
    void              * authentication_context;
    union
    {
        CPD_AUTHENTICATE  * authentication;     // CPD_REQUEST_AUTHENTICATION
        CPD_LOGIN_REASON    reason;             // CPD_REQUEST_LOGIN
                                                // CPD_EVENT_LOGOUT
                                                // CPD_EVENT_LOGIN_FAILED
                                                // CPD_EVENT_DEVICE_FAILED
        void          * target_login_context;   // CPD_EVENT_LOGIN
        void          * callback_handle;        // CPD_IOCTL_ADD_DEVICE
                                                // CPD_IOCTL_REMOVE_DEVICE
    } more_info;

} CPD_PORT_LOGIN, * PCPD_PORT_LOGIN;

#pragma pack(4)
typedef struct S_CPD_PORT_LOGIN_TAG
{
    CPD_IOCTL_HEADER   ioctl_hdr;
    CPD_PORT_LOGIN   param;
} S_CPD_PORT_LOGIN, * PS_CPD_PORT_LOGIN;
#pragma pack()

/***********************************************************************
 * CPD_IOCTL_GET_DRIVE_CAPABILITY structure
 ***********************************************************************/

/* CPD_IOCTL_GET_DRIVE_CAPABILITY flags bit definitions */

#define CPD_CAPABILITY_LOOPMAP_INDEX_VALID           0x00000020
#define CPD_CAPABILITY_SUPPORTS_REMAP                0x00000040
#define CPD_CAPABILITY_NEEDS_EDGE_RECONSTRUCT        0x00000080
#define CPD_CAPABILITY_USE_DEFAULT_OS_TIMEOUT        0x00000200
#define CPD_CAPABILITY_USE_DEFAULT_CPD_TIMEOUT       0x00000400

typedef struct CPD_DRIVE_CAPABILITY_TAG
{
    void              * miniport_login_context;
    BITS_32E            flags;
    BITS_32E            capability_flags;
    UINT_32E            cpd_min_time_out; /*Minimum timeout value required for the drive.*/
    UINT_32E            os_min_time_out_delta; /* Minimum amount of additional time required
                                                 for cleanup if cpd timeout has expired.*/
} CPD_DRIVE_CAPABILITY, * PCPD_DRIVE_CAPABILITY;

#pragma pack(4)
typedef struct S_CPD_DRIVE_CAPABILITY_TAG
{
    CPD_IOCTL_HEADER         ioctl_hdr;
    CPD_DRIVE_CAPABILITY   param;
} S_CPD_DRIVE_CAPABILITY, * PS_CPD_DRIVE_CAPABILITY;
#pragma pack(4)

/***********************************************************************
 * Structure passed by CPD_IOCTL_REGISTER_CALLBACKS and
 * CPD_IOCTL_PROCESS_IO
 ***********************************************************************/

typedef struct CPD_CALLBACK_HANDLE_TAG
{
    void *      callback_handle;
} CPD_CALLBACK_HANDLE, * PCPD_CALLBACK_HANDLE;

#pragma pack(4)
typedef struct S_CPD_CALLBACK_HANDLE_TAG
{
    CPD_IOCTL_HEADER          ioctl_hdr;
    CPD_CALLBACK_HANDLE     param;
} S_CPD_CALLBACK_HANDLE, * PS_CPD_CALLBACK_HANDLE;
#pragma pack()


/***********************************************************************
 * CPD_DOWNLOAD structure
 ***********************************************************************/

typedef enum CPD_DOWNLOAD_TARGET_TAG
{
    CPD_DOWNLOAD_HBA_FIRMWARE
} CPD_DOWNLOAD_TARGET;

typedef enum CPD_BUFFER_TYPE_TAG
{
    CPD_BUFFER_PHYSICAL,
    CPD_BUFFER_LOGICAL,
    CPD_BUFFER_SGL
} CPD_BUFFER_TYPE;

typedef struct CPD_DOWNLOAD_TAG
{
    CPD_DOWNLOAD_TARGET target;
    UINT_32E            offset;         // offset into target area
    CPD_BUFFER_TYPE     buffer_type;
    UINT_32E            download_len;   // # bytes to download
    void              * buffer_ptr;     // pointer to buffer/SGL
    UINT_64E            physical_offset;// phys offset of SGL virt addr
    BITS_32E            flags;
} CPD_DOWNLOAD, * PCPD_DOWNLOAD;

#pragma pack(4)
typedef struct S_CPD_DOWNLOAD_TAG
{
    CPD_IOCTL_HEADER  ioctl_hdr;
    CPD_DOWNLOAD    param;
} S_CPD_DOWNLOAD, * PS_CPD_DOWNLOAD;
#pragma pack()


/***********************************************************************
 * iscsi port information misc bit definitions
 ***********************************************************************/
#define ISCSI_DUP_FULL                 0x00000001


/***********************************************************************
 * xx_TARGET_INFO structures
 ***********************************************************************/
typedef struct ISCSI_TARGET_PORTAL_ENTRY_TAG
{
    ISCSI_ADDRESS_EXT           address;
    UINT_16E                    portal_group_tag;
} ISCSI_TARGET_PORTAL_ENTRY, * PISCSI_TARGET_PORTAL_ENTRY;

typedef struct ISCSI_TARGET_INFO_ENTRY_TAG
{
    ISCSI_NAME                  name;
    UINT_32E                    nr_portals;
    ISCSI_TARGET_PORTAL_ENTRY * portal_list;
} ISCSI_TARGET_INFO_ENTRY, * PISCSI_TARGET_INFO_ENTRY;

typedef struct CPD_TARGET_INFO_ENTRY_TAG
{
    union
    {
        ISCSI_TARGET_INFO_ENTRY iscsi;
    } entry;
    struct CPD_TARGET_INFO_ENTRY_TAG    * next_entry;
} CPD_TARGET_INFO_ENTRY, * PCPD_TARGET_INFO_ENTRY;

typedef struct CPD_TARGET_INFO_TAG
{
    void                      * request_context;
    UINT_32E                    nr_entries;
    CPD_TARGET_INFO_ENTRY     * first_entry;
} CPD_TARGET_INFO, * PCPD_TARGET_INFO;

#pragma pack(4)
typedef struct S_CPD_TARGET_INFO_TAG
{
    CPD_IOCTL_HEADER      ioctl_hdr;
    CPD_TARGET_INFO     param;
} S_CPD_TARGET_INFO, * PS_CPD_TARGET_INFO;
#pragma pack()

/***********************************************************************
 * CPD_GET_TARGET_INFO structure
 ***********************************************************************/

typedef struct CPD_GET_TARGET_INFO_TAG
{
    UINT_16E                portal_nr;
    void                  * login_context;
    UINT_32E                login_qualifier;
    void                  * request_context;
    CPD_TARGET_INFO_TYPE    request_type;
    CPD_NAME              * target_name;
    CPD_TARGET_INFO       * target_info;
    UINT_32E                info_size;      // size of target_info in bytes
    BITS_32E                flags;
} CPD_GET_TARGET_INFO, * PCPD_GET_TARGET_INFO;

typedef struct S_CPD_GET_TARGET_INFO_TAG
{
    CPD_IOCTL_HEADER      ioctl_hdr;
    CPD_GET_TARGET_INFO     param;
} S_CPD_GET_TARGET_INFO, * PS_CPD_GET_TARGET_INFO;

/* CPD_GET_TARGET_INFO flags bit definitions */

    // Reply will come via separate CPD_IOCTL_RETURN_TARGET_INFO call
#define CPD_TARGET_REPLY_PENDING                0x00000001

    // size of target_info buffer is too small
#define CPD_TARGET_BUFFER_TOO_SMALL             0x00000002

    // target info request timed out
#define CPD_TARGET_TIMEOUT                      0x00000004

    // format of target info undecipherable
#define CPD_TARGET_BAD_REPLY                    0x00000008

    // command was aborted (e.g., because of logout)
#define CPD_TARGET_ABORTED                      0x00000010


/***********************************************************************
 * CPD_DMRB structure
 ***********************************************************************/

#define CPD_DMRB_REVISION            5  /* inc'd (4)4K sectors */

typedef struct CPD_DMRB_TAG
{
    struct CPD_DMRB_TAG   * prev_ptr;   /* used by miniport to . . .   */
    struct CPD_DMRB_TAG   * next_ptr;   /* . . . put on XCB wait queue */
    void          * queue_ptr;          /* to be compat w/CPD_QUEUE_ELEMENT */
    UINT_32E        revision;           /* currently 2 */
    CPD_DMRB_FUNCTION   function;       /* request type */
    CPD_PATH        target;             /* target device */
    BITS_32E        flags;
    UINT_32E        timeout;            /* timeout value in seconds */
    void          * miniport_context;   /* DMD exchange context */
                                        /* also temporarily used to store tick */
                                        /* count when placed on wait q */
    UINT_32E        total_sector_size;  /* size (in bytes) of the media's formatted sector size*/
    UINT_32E        sector_overhead;    /* number of sector overhead bytes used for metadata */
    UINT_32E        data_length;        /* number of bytes to be transferred */
    UINT_32E        data_transferred;   /* actual number of  bytes xferred */
                                        /* also temporarily used in debug builds */
                                        /* to store proc nr when placed on wait q */
    struct _SGL   * sgl;                /* pointer to Flare SGL */
    UINT_32E        sgl_offset;         /* byte offset into 1st SGL element */
    UINT_64E        physical_offset;    /* offset of phys addr from virtual */
    void            (* callback) (void *, void *);  /* completion callback */
    void          *  callback_arg1;      /* 1st argument to callback function */
    void          *  callback_arg2;      /* 2nd argument to callback function */
    CPD_DMRB_STATUS status;             /* request status */
    CPD_DMRB_STATUS recovery_status;    /* status of recovery op, if any */
    void          * authentication_context; /* authentication context*/
    CPD_KEY_HANDLE  enc_key;            /* key used to encrypt/decrypt I/O
                                           or CPD_KEY_PLAINTEXT if no enc */
    UINT_64E        block_addr;         /* starting LBA used by encrypt/DIF */
    void          * callback_handle;    /* link to CPD_REGISTER_CALLBACK op */
    union
    {
        
        struct {
            UINT_8E         cdb[16];            /* CDB ! */
            UINT_8E         cdb_length;         /* nr of valid bytes in CDB */
            CPD_TAG_TYPE    queue_action;       /* type of queueing */
            UINT_8E         scsi_status;        /* SCSI status byte */
            UINT_8E         sense_length;       /* nr of valid bytes in
                                                 * sense_buffer */
            void          * sense_buffer;       /* SCSI sense informaton */
            UINT_64E        sense_phys_offset;  /* offset of sense phys addr
                                                 * from virtual */
        } scsi;

        struct {
            UINT_8E         fis[20];            /* FIS ! */
            UINT_8E         fis_length;         /* nr of valid bytes in FIS */
            CPD_TAG_TYPE    queue_action;       /* type of queueing */
            UINT_8E         rsp_length;         /* nr of valid bytes in
                                                 * rsp_buffer */
            void          * rsp_buffer;         /* FIS Response buffer */
            UINT_64E        rsp_phys_offset;    /* offset of rsp phys addr
                                                 * from virtual */
        } sata;
        
    }u;

} CPD_DMRB, * PCPD_DMRB;


/***********************************************************************
 * CPD_ABORT_PATH structure
 ***********************************************************************/

// if event_context not valid
#define CPD_EVENT_UNSPECIFIED           NULL

typedef struct CPD_ABORT_PATH_TAG
{
    CPD_PATH            path;
    CPD_ABORT_STATUS    abort_status;
    void              * event_context;
} CPD_ABORT_PATH, * PCPD_ABORT_PATH;


/***********************************************************************
 * CPD_ASYNC_COMPLETION structure
 ***********************************************************************/

typedef struct CPD_ASYNC_COMPLETION_TAG
{
    UINT_16E            portal_nr;
    void              * event_context;
    UINT_32E            async_event_count;
} CPD_ASYNC_COMPLETION, * PCPD_ASYNC_COMPLETION;


typedef struct S_CPD_ASYNC_COMPLETION_TAG
{
    CPD_IOCTL_HEADER          ioctl_hdr;
    CPD_ASYNC_COMPLETION    param;
} S_CPD_ASYNC_COMPLETION, * PS_CPD_ASYNC_COMPLETION;


/***********************************************************************
 * CPD_SFP_CONDITION structure
 ***********************************************************************/

typedef struct CPD_SFP_CONDITION_TAG
{
    CPD_SFP_CONDITION_TYPE condition;
    CPD_SFP_CONDITION_SUB_TYPE sub_condition;
} CPD_SFP_CONDITION, * PCPD_SFP_CONDITION;


/***********************************************************************
 * CPD_PORT_FAILURE structures
 ***********************************************************************/
typedef enum CPD_PORT_FAILURE_TYPE_TAG
{
    CPD_PORT_FAILURE_DUPLICATE_IP                = 0x00000001,
    CPD_PORT_FAILURE_INTERNAL_CTLR_ERROR         = 0x00000002,
    CPD_PORT_FAILURE_DRIVER_FAILURE              = 0x00000003,
    CPD_PORT_FAILURE_DRIVER_READY                = 0x00000004,
    CPD_PORT_FAILURE_RX_FIFO_RECOVERED           = 0x00000005,
    CPD_PORT_FAILURE_RECOVERED_CTLR_ERROR        = 0x00000006,
    CPD_PORT_FAILURE_FATAL_CTLR_ERROR            = 0x00000007,
    CPD_PORT_FAILURE_CTLR_READY                  = 0x00000008
} CPD_PORT_FAILURE_TYPE;

typedef struct CPD_PORT_FAILURE_TAG
{
    CPD_PORT_FAILURE_TYPE failure;
    UINT_16E portal_nr;
    UINT_64E reason;
    
    struct 
    {
        BITS_32E   port_down   : 1;
        BITS_32E   port_reset  : 1;
    } flags;

    union
    {
        UINT_8E    dup_ip_mac_addr[CPD_MAC_ADDR_LEN];
        UINT_32E   ctlr_error[CPD_CTLR_ERROR_INFO_SIZE];
        UINT_PTR   driver_failure[CPD_DRIVER_FAILURE_INFO_SIZE];
    }u;
} CPD_PORT_FAILURE, *PCPD_PORT_FAILURE;


/***********************************************************************
 * CPD_CM_LOOP_EVENT_INFO structure
 ***********************************************************************/

typedef struct CPD_CM_LOOP_EVENT_INFO_TAG
{
    UINT_16E            portal_nr;
    UINT_32E            loopmap_index;
} CPD_CM_LOOP_EVENT_INFO, * PCPD_CM_LOOP_EVENT_INFO;

typedef struct CPD_FRAME_INFO_TAG
{
    UINT_32E            s_id;
    UINT_32E            d_id;
    UINT_32E            ox_id;
    UINT_32E            rx_id;
    UINT_32E            rel_offset;
} CPD_FRAME_INFO, * PCPD_FRAME_INFO;


/***********************************************************************
 * CPD_ENC_xxx event structures
 ***********************************************************************/

typedef struct CPD_ENC_KEY_EVENT_TAG
{
    CPD_KEY_HANDLE      key_handle;     // key handle affected
    UINT_32E            status;         // CPD_RC_xxx status
    void *              caller_context; // context of CPD_IOCTL_ENC_KEY_MGMT
} CPD_ENC_KEY_EVENT, * PCPD_ENC_KEY_EVENT;

#define CPD_ENC_FAILURE_INFO_MAX 64

typedef struct CPD_ENC_TEST_COMP_TAG
{
    UINT_32E            status;         // test status
    UINT_32E            subtest;        // subtest reporting status
    UINT_8E             failure_info[CPD_ENC_FAILURE_INFO_MAX];
} CPD_ENC_TEST_COMP, * PCPD_ENC_TEST_COMP;


/***********************************************************************
 * CPD_EVENT_INFO structure
 * This is the structure passed on all asynchronous events.
 ***********************************************************************/

typedef struct CPD_EVENT_INFO_TAG
{
    CPD_EVENT_TYPE                type;
    union
    {
        CPD_PATH                  path;
        CPD_ABORT_PATH            abort_path;
        CPD_PORT_LOGIN            port_login;
        UINT_PTR                  loopmap_index;
        CPD_AFFECTED_INITIATORS * target_initiators;
        FC_CONNECT_TYPE           connect_type;
        BOOL                      boolean;
        CPD_DMRB                * request;
        CPD_GET_TARGET_INFO       target_info;
        CPD_MEDIA_INTERFACE_INFO  mii;
        CPD_PORT_FAILURE          port_fail;
        UINT_16E                  portal_nr;
        CPD_PORT_INFORMATION      link_info;
        CPD_CM_LOOP_EVENT_INFO    loop_info;
        CPD_CM_LOOP_CHANGE_TYPE   loop_change_type;
        CPD_FAULT_ENTRY           fault_entry;
        CPD_FRAME_INFO            frame_info;
        CPD_XLS_FAILURE_INFO      xls_fail_info;
        CPD_GPIO_CONTROL          gpio;
        CPD_ENC_KEY_EVENT         enc_key;
        CPD_ENC_TEST_COMP         enc_test;
    } u;
} CPD_EVENT_INFO, *PCPD_EVENT_INFO;


/***********************************************************************
 * CPD_REGISTER_CALLBACKS structure
 ***********************************************************************/

#define CPD_REGISTER_TAG_FIELD_SIZE   5
typedef struct CPD_REGISTER_CALLBACKS_TAG
{
    UINT_16E    portal_nr;
    void *      context;            /* Class driver context */
    BOOLEAN     (*async_callback) (void *, CPD_EVENT_INFO *);
    CHAR        tag[CPD_REGISTER_TAG_FIELD_SIZE];   /* Uniquely id driver, e.g. "NTBE" */
    union
    {
        struct
        {
            struct _ACCEPT_CCB *    (*get_free_accept_ccb)
                                        (void * registrant_context,
                                         void * target_login_context);
            struct _CCB *           (*get_pending_ccb)
                                        (void * registrant_context);
            void                    (*process_io) 
                                        (void * miniport_context);
            EMCPAL_STATUS           (*execute_srb_ioctl) 
                                        (void * miniport_context, 
                                         void * ioctl_ptr,
                                         UINT_32E ioctl_size);
            void * miniport_context;
        } ccb;
        struct
        {
            struct CPD_DMRB_TAG *   (*get_free_dmrb)
                                        (void * registrant_context,
                                         void * target_login_context);
            CPD_DMRB_STATUS         (*enqueue_pending_dmrb) (void * miniport_context, 
                                              CPD_DMRB * dmrb_ptr, 
                                              UINT_32 * queue_pos);
            void                    (*process_io) 
                                        (void * miniport_context);
            void * miniport_context;
        } dmrb;
    } target_callbacks;
    void      * callback_handle;    /* miniport context */
    BITS_32E    flags;
} CPD_REGISTER_CALLBACKS, *PCPD_REGISTER_CALLBACKS;

#pragma pack(4)
typedef struct S_CPD_REGISTER_CALLBACKS_TAG
{
    CPD_IOCTL_HEADER          ioctl_hdr;
    CPD_REGISTER_CALLBACKS  param;
} S_CPD_REGISTER_CALLBACKS, * PS_CPD_REGISTER_CALLBACKS;
#pragma pack()

/* Define flags for use with CPD_IOCTL_REGISTER_CALLBACKS */

#define CPD_REGISTER_INITIATOR                      0x00000001
#define CPD_REGISTER_TARGET_USING_DMRBS             0x00000002
#define CPD_REGISTER_TARGET_USING_CCBS              0x00000004
#define CPD_REGISTER_AUTHENTICATION_TARGET_MODE     0x00000008
#define CPD_REGISTER_AUTHENTICATION_INITIATOR_MODE  0x00000010
#define CPD_REGISTER_USE_CM_INTERFACE               0x00000020
#define CPD_REGISTER_POLL_ON_INTERRUPT              0x00000040
#define CPD_REGISTER_POLL_ON_INTERVAL               0x00000080
#define CPD_REGISTER_NOTIFY_ON_TARGET_LOGIN         0x00000100
#define CPD_REGISTER_SFP_EVENTS                     0x00000200
#define CPD_REGISTER_AUTHENTICATION                 0x00000400
#define CPD_REGISTER_DEREGISTER                     0x00001000
#define CPD_REGISTER_FIND_CALLBACK                  0x00004000
#define CPD_REGISTER_DO_NOT_INITIALIZE              0x00008000
#define CPD_REGISTER_NOTIFY_EXISTING_LOGINS         0x00010000
#define CPD_REGISTER_ENCRYPTION                     0x00020000
#define CPD_REGISTER_FILTER_INTERCEPT               0x80000000

/***********************************************************************
 * CPD_ISR_CALLBACK structure
 ***********************************************************************/

typedef struct CPD_ISR_CALLBACK_TAG
{
    UCHAR               (*callback_function) (void *, void *);
    void              * p1;                 // First arg to function
    void              * p2;                 // second arg to function.
    UCHAR               srb_status;
} CPD_ISR_CALLBACK, *PCPD_ISR_CALLBACK;

#pragma pack(4)
typedef struct S_CPD_ISR_CALLBACK_TAG
{
    CPD_IOCTL_HEADER      ioctl_hdr;
    CPD_ISR_CALLBACK    param;
} S_CPD_ISR_CALLBACK, * PS_CPD_ISR_CALLBACK;
#pragma pack()

/***********************************************************************
 * CPD_CM_LOOP_COMMAND structure
 ***********************************************************************/

#define CPD_CM_MAX_LOOP_DEVICES                 120

typedef enum CPD_CM_COMMAND_TAG
{
    CPD_CM_DISCOVER_LOOP                        = 0x8001,
    CPD_CM_ENABLE_LOOP,
    CPD_CM_DISABLE_LOOP,
    CPD_CM_RESET_LOOP,
    CPD_CM_LOGIN_LOOP,
    CPD_CM_HOLD_LOOP,
    CPD_CM_RESUME_LOOP,
    CPD_CM_FAIL_LOOP,
    CPD_CM_PEER_PWR_LOSS,
    CPD_CM_GET_EXPANDER_LIST,
    CPD_CM_UPDATE_EXPANDER_LIST,
    CPD_CM_GET_PHY_STATE,
    CPD_CM_FW_REVISION,
    CPD_CM_GET_LSI_STATS,
    CPD_CM_GET_MUX_STATS,
    CPD_CM_FW_UPGRADE_IN_PROGRESS,
    CPD_GET_DISCOVERY_INFO    
} CPD_CM_COMMAND;

typedef enum CPD_CM_STATUS_TAG
{
    CPD_CM_COMMAND_ACCEPTED                     = 0x8041,
    CPD_CM_COMMAND_REJECTED
} CPD_CM_STATUS;

typedef enum CPD_CM_LOOP_STATUS_TAG
{
    CPD_LOOP_DISCOVERED                         = 0x8061,
    CPD_LOOP_HUNG,
    CPD_LOOP_FAILURE,
    CPD_LOOP_OFFLINE,
    CPD_LOOP_INVALID_LIP,
    CPD_LOOP_UPDATE_EXP_LIST_UPDATED,
    CPD_LOOP_UPDATE_EXPANDER_UNEXIST,
    CPD_LOOP_PHY_STATE_DISCOVERED,
    CPD_LOOP_DISCOVERY_IN_PROGRESS
} CPD_CM_LOOP_STATUS;

typedef enum CPD_CM_SLOT_TAG
{
    CPD_CM_SLOT_UNKNOWN                         = 0x8081,
    CPD_CM_SLOT_EMPTY,
    CPD_CM_SLOT_FAILED,
    CPD_CM_SLOT_NORMAL,
    CPD_CM_SLOT_SUSPECTED,
    CPD_CM_SLOT_UNSUPPORTED_DRIVE
} CPD_CM_SLOT;

typedef enum CPD_CM_SLOT_DRIVE_TYPE_TAG
{
    CPD_CM_SLOT_DRIVE_TYPE_EMPTY                = 0x0000,
    CPD_CM_SLOT_DRIVE_TYPE_SATA                 = 0x6001,
    CPD_CM_SLOT_DRIVE_TYPE_SAS                  = 0x6002,
    CPD_CM_SLOT_DRIVE_TYPE_UNKNOWN              = 0x6FFF
} CPD_CM_SLOT_DRIVE_TYPE;

typedef struct CPD_CM_SLOT_INFO_TAG
{
    CPD_CM_SLOT             slot_status;
    CPD_CM_SLOT_DRIVE_TYPE  slot_drive_type;
} CPD_CM_SLOT_INFO;

typedef struct CPD_CM_LOOP_COMMAND_TAG
{
    CPD_CM_COMMAND      command_type;
    UINT_16E            portal_nr;
    CPD_CM_STATUS       ioctl_status;
    CPD_CM_LOOP_STATUS  discover_loop_status;
    CPD_CM_LOOP_STATUS  loop_fail_status;
    UINT_32E            port;
    CPD_CM_SLOT         loop_map[CPD_CM_MAX_LOOP_DEVICES];
} CPD_CM_LOOP_COMMAND, * PCPD_CM_LOOP_COMMAND;


#pragma pack(4)
typedef struct S_CPD_CM_LOOP_COMMAND_TAG
{
    CPD_IOCTL_HEADER      ioctl_hdr;
    CPD_CM_LOOP_COMMAND   param;
} S_CPD_CM_LOOP_COMMAND, * PS_CPD_CM_LOOP_COMMAND;
#pragma pack()


/***********************************************************************
 * CPD_ERROR_INSERTION structure
 ***********************************************************************/

#define CPD_ERROR_INSERTION_REVISION            2

/* Define actions for use with CPD_ERROR_INSERTION */

#define CPD_TEST_CLEAR_STATISTICS               0x00000001
#define CPD_TEST_GET_STATISTICS                 0x00000002
#define CPD_TEST_START_LIP_INSERTION            0x00000004
#define CPD_TEST_STOP_LIP_INSERTION             0x00000008
#define CPD_TEST_START_DROPPING_FRAMES          0x00000010
#define CPD_TEST_STOP_DROPPING_FRAMES           0x00000020
#define CPD_TEST_START_DROPPING_INB_CMDS        0x00000040
#define CPD_TEST_STOP_DROPPING_INB_CMDS         0x00000080
#define CPD_TEST_START_DROPPING_OUTB_CMDS       0x00000100
#define CPD_TEST_STOP_DROPPING_OUTB_CMDS        0x00000200
#define CPD_TEST_START_FAILING_OUTB_CMDS        0x00000400
#define CPD_TEST_STOP_FAILING_OUTB_CMDS         0x00000800

typedef struct CPD_ERROR_INSERTION_TAG
{
    UINT_16E            revision;
    BITS_32E            actions;
    UINT_32E            lip_interval;
    UINT_32E            dropped_frame_interval;
    UINT_32E            nr_lips;
    UINT_32E            nr_frames_dropped;
    UINT_32E            nr_inb_cmds_dropped;
    UINT_32E            nr_outb_cmds_dropped;
    UINT_32E            nr_outb_cmds_failed;
} CPD_ERROR_INSERTION, * PCPD_ERROR_INSERTION;

#pragma pack(4)
typedef struct S_CPD_ERROR_INSERTION_TAG
{
    CPD_IOCTL_HEADER       ioctl_hdr;
    CPD_ERROR_INSERTION  param;
} S_CPD_ERROR_INSERTION, * PS_CPD_ERROR_INSERTION;
#pragma pack()

/***********************************************************************
 * CPD_REGISTER_REDIRECTION structure
 ***********************************************************************/

#define CPD_REGISTER_REDIRECTION_REVISION               3
// revision 3 adds the portal_nr field

/* Define flags for use with CPD_REGISTER_REDIRECTION */

#define CPD_REDIRECTION_STD_NT_IO_ONLY                  0x00000001
#define CPD_REDIRECTION_ALL_IO                          0x00000002
#define CPD_REDIRECTION_DEREGISTER                      0x00000004


typedef struct CPD_REGISTER_REDIRECTION_TAG
{
    UINT_16E    revision;           /* structure revision - currently 2 */
    UINT_16E    portal_nr;
    BITS_16E    flags;
    UINT_32E    loopmap_index;      /* destination loop map index */
    void      * context;            /* class driver context */
    void        (*redirect) (void *, CPD_SCSI_REQ_BLK *, void *);
    void        (*execute) (void *, CPD_DMRB *);
    void        (*completion) (void *);
} CPD_REGISTER_REDIRECTION, * PCPD_REGISTER_REDIRECTION;

#pragma pack(4)
typedef struct S_CPD_REGISTER_REDIRECTION_TAG
{
    CPD_IOCTL_HEADER               ioctl_hdr;
    CPD_REGISTER_REDIRECTION     param;
} S_CPD_REGISTER_REDIRECTION, * PS_CPD_REGISTER_REDIRECTION;
#pragma pack()

/***********************************************************************
 * CPD_GET_DEVICE_ADDRESS structure
 ***********************************************************************/

#pragma pack(4)
typedef struct S_CPD_GET_DEVICE_ADDRESS_TAG
{
    CPD_IOCTL_HEADER           ioctl_hdr;
    CPD_GET_DEVICE_ADDRESS   param;
} S_CPD_GET_DEVICE_ADDRESS, * PS_CPD_GET_DEVICE_ADDRESS;
#pragma pack()

/***********************************************************************
 * CPD_REGISTERED_DEVICE_LIST structure
 ***********************************************************************/

/* Define flags for use with CPD_REGISTERED_DEVICE_LIST */

#define CPD_LIST_SIZE_ONLY                  0x00000001
#define CPD_LIST_FABRIC_DEVICES_ONLY        0x00000002
#define CPD_LIST_BUFFER_OVERFLOW            0x00000004
#define CPD_LIST_GET_LOGGED_IN_DEVICES      0x00000008
#define CPD_LIST_ENB_ENH_SRB_ACCESS         0x00000010

typedef struct CPD_REGISTERED_DEVICE_LIST_TAG
{
    UINT_16E                 portal_nr;
    CPD_LIST_TYPE            query_type;
    BITS_32E                 flags;
    void                   * miniport_login_context;
    UINT_32E                 login_qualifier;
    UINT_32E                 list_size;
    CPD_GET_DEVICE_ADDRESS * device_list_ptr;
} CPD_REGISTERED_DEVICE_LIST, * PCPD_REGISTERED_DEVICE_LIST;

#pragma pack(4)
typedef struct S_CPD_REGISTERED_DEVICE_LIST_TAG
{
    CPD_IOCTL_HEADER               ioctl_hdr;
    CPD_REGISTERED_DEVICE_LIST   param;
} S_CPD_REGISTERED_DEVICE_LIST, * PS_CPD_REGISTERED_DEVICE_LIST;
#pragma pack()

/***********************************************************************
 * CPD_EXT_SRB structure
 ***********************************************************************/

/* Define flags for use with SCSI_REQUEST_BLOCK */
#define CPD_SRB_FLARE_CMD                       0x40000000

/* Define ext_flags for use with CPD_EXT_SRB */

#define CPD_EXT_SRB_IOCTL_BYPASS                0x40000000
#define CPD_EXT_SRB_PHYS_SGL                    0x20000000
#define CPD_EXT_SRB_USE_EXTENDED_LUN            0x10000000
#define CPD_EXT_SRB_SKIP_SECTOR_OVERHEAD_BYTES  0x08000000
#define CPD_EXT_SRB_IS_EXT_SRB                  0x04000000
#define CPD_EXT_SRB_REMAP_QFULL                 0x02000000
#define CPD_EXT_SRB_REMAP_BUSY                  0x01000000
#define CPD_EXT_SRB_NO_DATA_TRANSFER            SRB_FLAGS_NO_DATA_TRANSFER
#define CPD_EXT_SRB_DATA_IN                     SRB_FLAGS_DATA_IN
#define CPD_EXT_SRB_DATA_OUT                    SRB_FLAGS_DATA_OUT
#define CPD_EXT_SRB_UNSPECIFIED_DIRECTION       SRB_FLAGS_UNSPECIFIED_DIRECTION

/* Implementation-specific SRB_STATUS #defines for scb_error_code.
 * MAKE SURE THEY DON'T OVERLAP DEFINITIONS IN srb.h !!!!!!!!!!!!!
 */

    // Miniport optionally remaps SCSISTAT_QUEUE_FULL/BUSY in ScsiStatus
    // to these values in scb_error_code field
    // (see ext_flags definitions above)
#define SRB_STATUS_CPD_QFULL                    0x30
#define SRB_STATUS_CPD_BUSY                     0x31

#ifndef USER_MODE

typedef struct CPD_EXT_SRB_TAG
{
    CPD_SCSI_REQ_BLK        srb;
    BITS_32E                ext_flags;
    struct _SGL           * sgl;
    UINT_32E                sgl_offset;
    UINT_64E                phys_offset;
    UINT_32E                time_out_value;
    INT_32E                 scb_error_code;
    CPD_LUN                 lun;
    UINT_32E                login_qualifier;
} CPD_EXT_SRB, * PCPD_EXT_SRB;


/***********************************************************************
 * CPD_ABORT_IO structure
 ***********************************************************************/

/* Define flags for use with CPD_ABORT_IO */

    // Abort Task is aborting a CPD-extended SRB
#define CPD_ABORT_OS_IO                         0x00000001
    // Abort Task is aborting a DMRB
#define CPD_ABORT_DMRB                          0x00000002
    // Send iSCSI NOP as immediate command
#define CPD_ABORT_NOP_IMMEDIATE                 0x00000004


typedef struct CPD_ABORT_IO_TAG
{
    CPD_ABORT_CODE              abort_code;
    BITS_32E                    flags;
    CPD_PATH                    path;
    union
    {
        CPD_EXT_SRB           * cpd_os_io_ptr;
        CPD_DMRB              * dmrb_ptr;
        UINT_8E                 async_event_code[2];
    } request;
    UINT_32E                    timeout_value;
    UINT_32E                    attempts;
    void                        (* completion_callback)(void *, void *);
                                //    (void *, void *, CPD_ABORT_STATUS);
    void                      * callback_arg1;
    void                      * callback_arg2;
} CPD_ABORT_IO, * PCPD_ABORT_IO;

#pragma pack(4)
typedef struct S_CPD_ABORT_IO_TAG
{
    CPD_IOCTL_HEADER   ioctl_hdr;
    CPD_ABORT_IO     param;
} S_CPD_ABORT_IO, * PS_CPD_ABORT_IO;
#pragma pack()

#endif

/***********************************************************************
 * CPD_HW_PARAM structure
 ***********************************************************************/

typedef struct CPD_HW_IMPED_PREEMP_TAG
{
    CPD_HW_IMPEDANCE termination_impedance;
    CPD_HW_VOLTAGE output_voltage;
    CPD_HW_PREEMPHASIS preemphasis;
} CPD_HW_IMPED_PREEMP, * PCPD_HW_IMPED_PREEMP;

typedef struct CPD_HW_PARAM_TAG
{
    CPD_HW_PARAM_TYPE param_type;
    /* union allows for other hardware parameters to be passed in similar
     * fashion  
     */
    union
    {
        CPD_HW_IMPED_PREEMP params;
    }u;

} CPD_HW_PARAM, * PCPD_HW_PARAM;

#pragma pack(4)
typedef struct S_CPD_HW_PARAM_TAG
{
    CPD_IOCTL_HEADER  ioctl_hdr;
    CPD_HW_PARAM    param;
} S_CPD_HW_PARAM, * PS_CPD_HW_PARAM;
#pragma pack()

/***********************************************************************
 * CPD_MGMT_PASS_THRU structure
 ***********************************************************************/

#define CPD_MGMT_PASS_THRU_REVISION            1


/* Define types for use with CPD_MGMT_PASS_THRU */

#define CPD_MGMT_SAS_SMP              0x00000001
#define CPD_MGMT_FC_XLS               0x00000002
#define CPD_MGMT_GET_SUPPORTED_TYPES  0x00000004

typedef struct CPD_MGMT_PASS_THRU_TAG
{
    UINT_16E        portal_nr;
    BITS_32E        type;
    void          * miniport_login_context;
    void          * req_buf_ptr;
    UINT_32E        req_len;
    void          * rsp_buf_ptr;
    UINT_32E        rsp_len;
    UINT_64E        req_physical_offset;
    UINT_64E        rsp_physical_offset;
    void         (* callback)
                    (void *, CPD_DMRB_STATUS);
    void          * callback_arg;
    UINT_32E        timeout;
} CPD_MGMT_PASS_THRU, * PCPD_MGMT_PASS_THRU;

#pragma pack(4)
typedef struct S_CPD_MGMT_PASS_THRU_TAG
{
    CPD_IOCTL_HEADER       ioctl_hdr;
    CPD_MGMT_PASS_THRU   param;
} S_CPD_MGMT_PASS_THRU, *PS_CPD_MGMT_PASS_THRU;
#pragma pack()

#pragma pack(4)
typedef struct S_CPD_SET_SYSTEM_DEVICES_TAG
{
    CPD_IOCTL_HEADER ioctl_hdr;
    CPD_SET_SYSTEM_DEVICES param;
} S_CPD_SET_SYSTEM_DEVICES, * PS_CPD_SET_SYSTEM_DEVICES;
#pragma pack()


/***********************************************************************
 * CPD_ENC_KEY_MGMT structure
 ***********************************************************************/

  /* target field definitions */
#define CPD_ENC_CTLR_HMAC           0x01
#define CPD_ENC_KEK                 0x02
#define CPD_ENC_DEK                 0x03

  /* operation field definitions */
#define CPD_ENC_SET_KEY             0x08
#define CPD_ENC_SET_AND_PERSIST_KEY 0x09
#define CPD_ENC_INVALIDATE_KEY      0x0a
#define CPD_ENC_FLUSH_ALL_KEYS      0x0b
#define CPD_ENC_REESTABLISH_KEY     0x0c
#define CPD_ENC_SET_MODE            0x0d
#define CPD_ENC_CLEAR_PERSISTED_KEYS_CTLR_WIDE  0x0e
#define CPD_ENC_REBASE_ALL_KEYS     0x0f

    // for test purposes (from user mode)
#define CPD_ENC_OP_FORCE_CALLBACK   0x80
#define CPD_ENC_OP_NO_CALLBACK      0x40    // force IOCTL to pend until completion
#define CPD_ENC_OP_DROP_CALLBACK    0x20    // no callback after completion

#define CPD_ENC_FLUSH_ALL_KEYS_NO_CALLBACK \
            (CPD_ENC_FLUSH_ALL_KEYS | CPD_ENC_OP_NO_CALLBACK)
#define CPD_ENC_SET_MODE_NO_CALLBACK \
            (CPD_ENC_SET_MODE | CPD_ENC_OP_NO_CALLBACK)
#define CPD_ENC_CLEAR_PERSISTED_KEYS_CTLR_WIDE_NO_CALLBACK \
            (CPD_ENC_CLEAR_PERSISTED_KEYS_CTLR_WIDE | CPD_ENC_OP_NO_CALLBACK)
#define CPD_ENC_SET_MODE_DROP_CALLBACK \
            (CPD_ENC_SET_MODE | CPD_ENC_OP_DROP_CALLBACK)


  /* format field definitions */
// see cpd_generics.h

typedef struct CPD_ENC_KEY_MGMT_TAG
{
    UINT_16E    portal_nr;
    UINT_8E     target;         // HMAC / KEK / DEK
    UINT_8E     operation;      // set / persist / invalidate / flush
    UINT_8E     format;         // wrapped / key_only / plaintext
    UINT_8E     nr_keys;        // nr of keys to operate on
    UINT_16E    key_size;       // size of key_ptr array element
    CPD_KEY_HANDLE      unwrap_key_handle;  // KEK handle for DEK or KEK
    CPD_KEY_HANDLE *    key_handle_ptr;     // in/out array of key handles
    UINT_8E *   key_ptr;        // ptr to array of keys
    void *      caller_context; // context returned in callbacks
    CPD_ENC_MODE        enc_mode;           // new mode for CPD_ENC_SET_MODE
} CPD_ENC_KEY_MGMT, * PCPD_ENC_KEY_MGMT;

typedef struct S_CPD_ENC_KEY_MGMT_TAG
{
    CPD_IOCTL_HEADER ioctl_hdr;
    CPD_ENC_KEY_MGMT param;
} S_CPD_ENC_KEY_MGMT, * PS_CPD_ENC_KEY_MGMT;


/***********************************************************************
 * CPD_ENC_SELF_TEST structure
 ***********************************************************************/

  /* test_set field definitions */
#define CPD_ENC_AES_POSITIVE            0x00000001
#define CPD_ENC_AES_NEGATIVE            0x00000002
#define CPD_ENC_KWP_POSITIVE            0x00000004
#define CPD_ENC_KWP_NEGATIVE            0x00000008
#define CPD_ENC_HMAC_POSITIVE           0x00000010
#define CPD_ENC_HMAC_NEGATIVE           0x00000020

#define CPD_ENC_SELF_TEST_NO_CALLBACK   0x80000000

typedef struct CPD_ENC_SELF_TEST_TAG
{
    BITS_32E    test_set;       // AES,KWP,HMAC / POSITIVE,NEGATIVE
    UINT_32E    status;         // no callback return only
    UINT_32E    subtest;        // no callback return only
    UINT_32E    info_size;      // space alloc'd/bytes ret'd for failure_info
                                //   appended to IOCTL (no callback only)
    void *      caller_context; // context returned in callbacks
} CPD_ENC_SELF_TEST, * PCPD_ENC_SELF_TEST;

typedef struct S_CPD_ENC_SELF_TEST_TAG
{
    CPD_IOCTL_HEADER   ioctl_hdr;
    CPD_ENC_SELF_TEST  param;
} S_CPD_ENC_SELF_TEST, * PS_CPD_ENC_SELF_TEST;


/***********************************************************************
 * CPD_ENC_ALG_TEST structure
 ***********************************************************************/

typedef enum CPD_ENC_ALG_TYPE_TAG
{
    CPD_ENC_SHA = 0x77000001,
    CPD_ENC_XTS,
    CPD_ENC_ECB,
    CPD_ENC_ENG_SEL_OP
} CPD_ENC_ALG_TYPE;


typedef struct CPD_ENC_SHA_TEST_TAG
{
    CPD_ENC_SHA_TYPE    algorithm;      // CPD_ENC_SHA_xxx or
                                // CPD_ENC_HMAC_SHA_xxx - see cpd_generics.h
    UINT_16E            key_length;     // nr bytes in key (zero for non-HMAC)
    UINT_32E            msg_length;     // nr bytes in message to hash
    UINT_16E            digest_length;  // nr bytes in digest
    UINT_8E *           digest_ptr;     // ptr to put digest (NULL if appended)

    // key goes here after digest_ptr

    // message goes here after key

    // if digest_ptr is NULL, digest is put here following message

} CPD_ENC_SHA_TEST, * PCPD_ENC_SHA_TEST;


typedef struct CPD_ENC_XTS_TEST_TAG
{
    // not currently implemented
    UINT_8E     reserved;
} CPD_ENC_XTS_TEST, * PCPD_ENC_XTS_TEST;


typedef struct CPD_ENC_ECB_TEST_TAG
{
    // not currently implemented
    UINT_8E     reserved;
} CPD_ENC_ECB_TEST, * PCPD_ENC_ECB_TEST;


typedef struct CPD_ENC_ENG_SEL_TAG
{
    BITS_32E    eng_list;
} CPD_ENC_ENG_SEL, * PCPD_ENC_ENG_SEL;


typedef struct CPD_ENC_ALG_TEST_TAG
{
    CPD_ENC_ALG_TYPE    test_type;
    void *              caller_context; // context returned in callbacks
    union
    {
        CPD_ENC_SHA_TEST    sha;
        CPD_ENC_XTS_TEST    xts;
        CPD_ENC_ECB_TEST    ecb;
        CPD_ENC_ENG_SEL     eng_sel;
    } sub;

} CPD_ENC_ALG_TEST, * PCPD_ENC_ALG_TEST;

typedef struct S_CPD_ENC_ALG_TEST_TAG
{
    CPD_IOCTL_HEADER        ioctl_hdr;
    CPD_ENC_ALG_TEST        param;
} S_CPD_ENC_ALG_TEST, * PS_CPD_ENC_ALG_TEST;



/***********************************************************************
 * MACROS
 ***********************************************************************/

/* This macro returns non-zero if the connect type (of FC_CONNECT_TYPE)
 * passed into it indicates we're connected to a switch (i.e., we're
 * in a Fabric environment).
 */
#define FC_CT_IS_FABRIC(m_ct)               ((m_ct) & FC_CT_SWITCH_PRESENT)


/* This macro returns non-zero if the connect type (of FCOE_CONNECT_TYPE)
 * passed into it indicates we're connected to a switch (i.e., we're
 * in a Fabric environment).
 */
#define FCOE_CT_IS_FABRIC(m_ct)               ((m_ct) & FCOE_CT_SWITCH_PRESENT)

/* This macro returns TRUE (non-zero) if the miniport_login_context
 * passed into it corresponds to a Fibre Channel fabric device.
 * (Note that this macro only works correctly for Fibre Channel drivers).
 */
// DM 2009-01-09 #define FC_DEVICE_IS_FABRIC_DEVICE(m_mlc)   ((UINT_32)(m_mlc) >= 128)

/* This macro returns the Dual-Mode driver's miniport_login_context
 * given the loopmap_index as used in the CM interface.
 */
#define CPD_GET_LOGIN_CONTEXT_FROM_LOOPMAP_INDEX(m_lmi)             \
            ((void *)(UINT_PTR)m_lmi)

#define CPD_GET_LOGIN_CONTEXT_FROM_LOOPMAP_INDEX_ULONG(m_lmi)       \
            ((ULONG)(UINT_PTR)m_lmi)


#define CPD_TARGET_BITS 5
#define CPD_BUS_BITS 8
#define CPD_TARGET_SHIFT 0
#define CPD_BUS_SHIFT (CPD_TARGET_BITS + CPD_TARGET_SHIFT)
#define CPD_TARGET_MASK  ((1 << CPD_TARGET_BITS) - 1)
#define CPD_BUS_MASK     ((1 << CPD_BUS_BITS) - 1)


/* This macro returns the Windows Path field for a device with
 * the miniport_login_context passed in.
 */
#define CPD_GET_PATH_FROM_CONTEXT(m_mlc)   \
    ((UINT_8E)(((ULONG_PTR)(m_mlc) >> CPD_TARGET_BITS) & CPD_BUS_MASK))

/* This macro returns the Windows Target field for a device with
 * the miniport_login_context passed in.
 */
#define CPD_GET_TARGET_FROM_CONTEXT(m_mlc)  ((UINT_8E)((ULONG_PTR)(m_mlc) & CPD_TARGET_MASK))

/* This macro returns the Windows Target field for a device with
 * the miniport_login_context passed in.
 */
#define CPD_GET_LOGIN_CONTEXT_FROM_PATH_TARGET(m_path, m_target)      \
    ((void *)(ULONG_PTR)((((m_path) & CPD_BUS_MASK) << CPD_BUS_SHIFT) | ((m_target) & CPD_TARGET_MASK)))

#define CPD_GET_LOGIN_CONTEXT_FROM_PATH_TARGET_ULONG(m_path, m_target)      \
    ((ULONG)((((m_path) & CPD_BUS_MASK) << CPD_BUS_SHIFT) | ((m_target) & CPD_TARGET_MASK)))

/* This macro returns the device index portion of the miniport_login_context.
 * The index is a unique, zero-based, consecutive numbering of devices.
 */
#define CPD_CONTEXT_INDEX_MASK     0xFFFF
#define CPD_GET_INDEX_FROM_CONTEXT(m_mlc)                           \
            ((UINT_16E)((UINT_PTR)(m_mlc) & CPD_CONTEXT_INDEX_MASK))

/* This macro tests for success of a CPD IOCTL.  NT_SUCCESS indicates whether
 * the miniport received the IOCTL, while SRB_IO_CONTROL.ReturnCode indicates
 * the miniport status for the IOCTL.
 **** NOTE that the module using this macro is responsible for ****
 **** having an appropriate definition of NT_SUCCESS included. ****
 */
#define CPD_IOCTL_SUCCESS(m_status, m_ioctl_hdr)              \
    (EMCPAL_IS_SUCCESS (m_status) &&                          \
            ((CPD_IOCTL_HEADER_GET_RETURN_CODE(m_ioctl_hdr)   \
                == CPD_RC_NO_ERROR) ||                        \
            (CPD_IOCTL_HEADER_GET_RETURN_CODE(m_ioctl_hdr)    \
                == CPD_RC_NO_ERROR_CALLBACK_PENDING)))

#define CPD_IOCTL_STATUS(m_status, m_ioctl_hdr)               \
           (CPD_IOCTL_SUCCESS(m_status, m_ioctl_hdr) ?        \
                EMCPAL_STATUS_SUCCESS :                       \
                ((EMCPAL_STATUS_SUCCESS == m_status) ?        \
                    EMCPAL_STATUS_INVALID_DEVICE_REQUEST :    \
                    m_status))  
    

/******************************************************************************
 ******************************************************************************
 ******************************************************************************
 ********  EVERYTHING BELOW THIS POINT ARE OBSOLETE MAMBA DEFINTIONS   ********
 ********  Unfortunately, some external references to them still exist ********
 ******************************************************************************
 ******************************************************************************
 *****************************************************************************/


/**************************************************
* Structure for SAS expander list discovery
***************************************************/
#define CPD_MAX_EXPANDERS   10
#define CPD_MAX_LSI_PHYS    8
#define CPD_MAX_MUX_PORTS   4

typedef struct cpd_cm_exp_info_tag
{
    UINT_64     sas_address;
    UINT_64     ses_address;
    /* Peter Puhov */
    /* It'll be useful to have DevHandle and ParentDevHandle of the expander */
    UINT_16E    DevHandle;         /* A unique ID assigned by MPT           */
    UINT_16E    ParentDevHandle;   /* Device's parent's dev handle          */        
    UINT_8      bus;               // Logical bus number
    UINT_8      target_id;
    UINT_8      enclosure_num;     // does not contain a bus component (i.e. 0-7)
    UINT_8      cabling_position;  // position has no bus component (i.e. 0-7)
    BOOLEAN     new_entry;
}
CPD_CM_EXP_INFO;

typedef struct cpd_cm_exp_list_tag
{
    UINT_8          num_expanders;
    CPD_CM_EXP_INFO exp_info[CPD_MAX_EXPANDERS];
}
CPD_CM_EXP_LIST;

/***********************************************************************
 * Link Statistic structures
 ***********************************************************************/
typedef struct CPD_LSI_PHY_PAGE1_DATA 
{
   UINT_32                         InvalidDwordCount;          
   UINT_32                         RunningDisparityErrorCount; 
   UINT_32                         LossDwordSynchCount;        
   UINT_32                         PhyResetProblemCount;
}CPD_LSI_PHY_PAGE1_DATA, *PCPD_LSI_PHY_PAGE1_DATA;

typedef struct CPD_LSI_LINK_STATS
{
    CPD_LSI_PHY_PAGE1_DATA          phy_stats[CPD_MAX_LSI_PHYS];
    UINT_16                         exp_change_count[CPD_MAX_EXPANDERS - 1];
}
CPD_LSI_LINK_STATS, *PCPD_LSI_LINK_STATS;

typedef struct CPD_MUX_LINK_STATS
{
    UINT_32         TargetId;
    UINT_32         InvalidDwordCount[CPD_MAX_MUX_PORTS];          
    UINT_32         DisparityErrorCount[CPD_MAX_MUX_PORTS]; 
    UINT_32         LossDwordSynchCount[CPD_MAX_MUX_PORTS];        
    UINT_32         PhyResetProblemCount[CPD_MAX_MUX_PORTS];
    UINT_32         CodeViolationCount[CPD_MAX_MUX_PORTS];
}
CPD_MUX_LINK_STATS, *PCPD_MUX_LINK_STATS;

#define CPD_PAGE_82_MAX_PARAMETER_DESCRIPTORS    17

typedef struct CPD_PAGE_82_PARAMETER_DESCRIPTOR
{
    UINT_16E     ParameterID;
    UINT_16E     ParameterSubID;
    UINT_32E     Value;
}
CPD_PAGE_82_PARAMETER_DESCRIPTOR, *PCPD_PAGE_82_PARAMETER_DESCRIPTOR;

typedef struct CPD_DISK_DIAGNOSTIC_PAGE_82_DATA
{
    UINT_8E                          PageCode;
    UINT_8E                          Reserved0;
    UINT_16E                         PageLength;
    UINT_64E                         VendorID;
    UINT_32E                         ProductRevision;
    UINT_16E                         NumberOfFeatureDescriptors;
    UINT_16E                         FeatureID;
    UINT_16E                         FeatureSubID;
    UINT_8E                          Reserved1;
    UINT_16E                         NumberOfParameterDescriptors;
    UINT_8E                          ParameterDescriptorSize;
    CPD_PAGE_82_PARAMETER_DESCRIPTOR ParamaterDescriptors[CPD_PAGE_82_MAX_PARAMETER_DESCRIPTORS];
}
CPD_DISK_DIAGNOSTIC_PAGE_82_DATA, *PCPD_DISK_DIAGNOSTIC_PAGE_82_DATA;

typedef enum CPD_CM_PHY_TAG
{
    CPD_CM_PHY_UNKNOWN                         = 0x8181,
    CPD_CM_PHY_3G,
    CPD_CM_PHY_1_5G,
    CPD_CM_PHY_SATA,
    CPD_CM_PHY_FAILED,
    CPD_CM_PHY_NORMAL
} CPD_CM_PHY_STATE,*PCPD_CM_PHY_STATE;


/* New get discovery info command to get both ExpanderTable and HpSasAddressTables */
#define CPD_CM_MAX_TARGETS 128 /* this is defined in file sym_str.h as #define SYM_MAX_TARGETS 128 */

typedef struct S_SAS_DEVICE_INFO_TAG {
    UINT_64E   SASAddress;        /* The SAS address of the device         */
    UINT_16E   DevHandle;         /* A unique ID assigned by MPT           */
    UINT_16E   ParentDevHandle;   /* Device's parent's dev handle          */
    UINT_8E    BusId;             /* BusId for this device                 */
    UINT_8E    TargetId;          /* TargetId for this device              */
    UINT_8E    FlareTargetId;     /* Flare Target Id for this device       */
    UINT_8E    PhyNum;            /* Phy Number this device is attached to */
    UINT_32E   DeviceInfo;        /* Device info - sas/sata/expander etc   */
    UINT_8E    TMFlags;
} S_SAS_DEVICE_INFO;

#pragma pack(4)
typedef struct S_CPD_GET_DISCOVERY_INFO_TAG
{
    S_CPD_CM_LOOP_COMMAND cpd_cm_loop_command; /* Should always be first */
    S_SAS_DEVICE_INFO     sas_device_info[CPD_CM_MAX_TARGETS];
}S_CPD_GET_DISCOVERY_INFO;
#pragma pack()


/***********************************************************************
 * CPD_IOCTL_GET_DRIVE_CAPABILITY structure
 ***********************************************************************/

/* CPD_IOCTL_GET_DRIVE_CAPABILITY flags bit definitions */

#define CPD_CAPABILITY_LOOPMAP_INDEX_VALID           0x00000020
#define CPD_CAPABILITY_SUPPORTS_REMAP                0x00000040
#define CPD_CAPABILITY_NEEDS_EDGE_RECONSTRUCT        0x00000080
#define CPD_CAPABILITY_USE_DEFAULT_OS_TIMEOUT        0x00000200
#define CPD_CAPABILITY_USE_DEFAULT_CPD_TIMEOUT       0x00000400

#if 0
typedef struct CPD_DRIVE_CAPABILITY_TAG
{
    void      * miniport_login_context;
    BITS_32E    flags;
    BITS_32E    capability_flags;
    UINT_32E    cpd_min_time_out; /*Minimum timeout value required for the drive.*/
    UINT_32E    os_min_time_out_delta; /* Minimum amount of additional time required
                                                 for cleanup if cpd timeout has expired.*/
} CPD_DRIVE_CAPABILITY, * PCPD_DRIVE_CAPABILITY;

typedef struct S_CPD_DRIVE_CAPABILITY_TAG
{
    CPD_IOCTL_HEADER         ioctl_hdr;
    CPD_DRIVE_CAPABILITY   param;
} S_CPD_DRIVE_CAPABILITY, * PS_CPD_DRIVE_CAPABILITY;

#endif

/**************************************************************************
 ********************  END OF OBSOLETE MAMBA CONTENT   ********************
 **************************************************************************/


#endif  /* CPD_INTERFACE_H */
