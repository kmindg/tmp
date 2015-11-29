
/***********************************************************************
 * Copyright (C) EMC Corp. 2000
 * All rights reserved.
 * Licensed material - Property of EMC Corporation.
 ***********************************************************************/

/***********************************************************************
 *  fc_interface.h
 ***********************************************************************
 *
 *  DESCRIPTION:
 *      This file contains definitions required by drivers that need
 *      to interface with the Dual-Mode OS Wrapper.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      20 Nov 00   DCW     Created
 *
 ***********************************************************************/

/* Make sure we only include this file once */

#ifndef FC_INTERFACE_H
#define FC_INTERFACE_H

#ifdef DPCPD_DEBUG

/***********************************************************************
 * INCLUDE FILES
 ***********************************************************************/

#include "generics.h"
#include "core_config.h"    /* for MAX_INITIATORS_PER_PORT for minishare.h */
#include "tcdmini.h"
#include "icdmini.h"
#include "minishare.h"
#include "fcb_loopmap.h"
#include "k10defs.h"        /* in inc - for K10_WWN */


/***********************************************************************
 * CONSTANT DEFINES
 ***********************************************************************/

/* Define a device type for Flare Boot devices -
 * used as vendor-specific device type in inquiry string.
 */

#define FLARE_NT_BOOT_DEVICE                0x1D
#define SCSIINIT_DEVICE                     0x1E


/* Define connect types for FC_IOCTL_GET_PORT_INFO and ET_LINK_UP event */

#define FC_CT_OFFLINE                       CT_NOT_CONNECTED
                                            // 5
#define FC_CT_NON_PARTICIPATING             6
#define FC_CT_POINT_TO_POINT                CT_POINT_TO_POINT
                                            // 1
#define FC_CT_PRIVATE_LOOP                  CT_ARBITRATED_LOOP
                                            // 2
/* SWITCH_PRESENT is a bit setting, and not a connect type in and of itself */
#define FC_CT_SWITCH_PRESENT                0x10
#define FC_CT_SWITCH_PTP                    (FC_CT_POINT_TO_POINT |     \
                                             FC_CT_SWITCH_PRESENT)
#define FC_CT_PUBLIC_LOOP                   (FC_CT_PRIVATE_LOOP |       \
                                             FC_CT_SWITCH_PRESENT)

/* This macro returns non-zero if the connect type (as defined above)
 * passed into it indicates we're connected to a switch (i.e., we're
 * in a Fabric environment).
 */
#define FC_CT_IS_FABRIC(m_ct)               ((m_ct) & FC_CT_SWITCH_PRESENT)


/* Define FCP_CMND FCP_CNTL Task Attribute settings to be passed up */

#define FC_UNTAGGED                         0   // Note TCD uses bool tests
                                                // for untagged - so must be 0
#define FC_SIMPLE_QUEUE_TAG                 SCSIMESS_SIMPLE_QUEUE_TAG
#define FC_HEAD_OF_QUEUE_TAG                SCSIMESS_HEAD_OF_QUEUE_TAG
#define FC_ORDERED_QUEUE_TAG                SCSIMESS_ORDERED_QUEUE_TAG
#define FC_ACA_QUEUE_TAG                    0x2f

 
/* Define link speeds for FC_IOCTL_GET_CONFIG2, FC_IOCTL_GET_PORT_INFO,
 * and FC_IOCTL_SET_LINK_SPEED.  Speeds are defined as USHORT/UINT_16E.
 * Set up speeds as a bit-encoded field so multiple may be specified
 * as for "supported link speeds".  Also use bits so don't get confused
 * between encodings are speeds, as this won't map nice in the future.
 */

/* Refer to speeds.h for new speed defines
*/

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


/* IOCTL codes */

#define FC_IOCTL_OPCODE_MASK                0xff00ffff

/* Target mode IOCTLs
 * defined in host\inc\tcdmini.h
 * values are 0x80000001 - 0x80000011
 * bits 23-16 may be used to indicate path
 */

#define FC_IOCTL_PROCESS_TARGET_CCB         CTM_MORE_WORK_PENDING
#define FC_IOCTL_GET_CONFIG                 CTM_GET_TARGET_MINI_CONFIG
#define FC_IOCTL_GET_CONFIG2                CTM_GET_TARGET_MINI_CFG2
#define FC_IOCTL_GET_WWN                    CTM_GET_TARGET_WWN
#define FC_IOCTL_SET_WWN                    CTM_SET_TARGET_WWN
#define FC_IOCTL_GET_SOFT_ADDRESSING        CTM_GET_TARGET_SOFT_ADDRESSING
#define FC_IOCTL_SET_SOFT_ADDRESSING        CTM_SET_TARGET_SOFT_ADDRESSING
#define FC_IOCTL_GET_LOOP_ID                CTM_GET_SCSI_TARGET_ID
#define FC_IOCTL_SET_LOOP_ID                CTM_SET_SCSI_TARGET_ID
#define FC_IOCTL_ENABLE_TARGET              CTM_ENABLE_TARGET
#define FC_IOCTL_DISABLE_TARGET             CTM_DISABLE_TARGET
#define FC_IOCTL_DECREMENT_TARGET_ASYNC_EVENT       \
                                            CTM_ASYNC_EVENT_DECREMENT
#define FC_IOCTL_CALL_FUNCTION_IN_ISR_CONTEXT       \
                                            CTM_CALL_FUNCTION_IN_ISR_CONTEXT
#define FC_IOCTL_GET_PORT_INFO              CTM_GET_PORT_INFO
#define FC_IOCTL_GET_LOOP_IDS               CTM_GET_LOOP_IDS
#define FC_IOCTL_GET_TARGET_QFULL_RESPONSE  CTM_GET_TARGET_QFULL_RESPONSE
#define FC_IOCTL_SET_TARGET_QFULL_RESPONSE  CTM_SET_TARGET_QFULL_RESPONSE

/* Initiator mode IOCTLs
 * defined in miniport\inc\icdmini.h
 * values are 0x90000081 - 0x90000086 and 0x900000F0
 */

#define FC_IOCTL_REGISTER_CALLBACK          CIM_REGISTER_CALLBACK
#define FC_IOCTL_DEREGISTER_CALLBACK        CIM_DEREGISTER_CALLBACK
#define FC_IOCTL_GET_EXTENSIONS             CIM_GET_EXTENSIONS
#define FC_IOCTL_SET_INITIATOR_RECOVERY_CONFIG      \
                                            CIM_RECOVERY_CONFIG
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

#define FC_IOCTL_GET_REGISTERED_WWID_LIST_SIZE      \
                                            0xA0000001
#define FC_IOCTL_GET_REGISTERED_WWID_LIST   0xA0000002
#define FC_IOCTL_ADD_WWID_PATH              0xA0000003
#define FC_IOCTL_OPEN_WWID_PATH             0xA0000004
#define FC_IOCTL_GET_WWID_PATH              0xA0000005
#define FC_IOCTL_REMOVE_WWID_PATH           0xA0000006

#define FC_IOCTL_SET_LINK_SPEED             0xA0000007
#define FC_IOCTL_ABORT_IO                   0xA0000009
#define FC_IOCTL_GET_LESB                   0xA000000A

#define FC_IOCTL_REGISTER_DM_CALLBACKS      0xA0000010
#define FC_IOCTL_DEREGISTER_DM_CALLBACKS    0xA0000011
#define FC_IOCTL_PROCESS_DMRB               0xA0000012
#define FC_IOCTL_REGISTER_REDIRECTION       0xA0000013
#define FC_IOCTL_DEREGISTER_REDIRECTION     0xA0000014
#define FC_IOCTL_ERROR_INSERTION            0xA0000015


/* Allow a default FC_TYPE to be specified without knowing
 * anything about the details of Fibre Channel.
 * This default type will be used to set the type to FC_TYPE_FCP.
 */

#define FC_TYPE_DEFAULT                     0xFFFF0DEF


/********
 * IOCTL error return codes returned in SRB_IO_CONTROL.ReturnCode
 ********/

#define FC_RC_BASE                          0x01805000

#define FC_RC_NO_ERROR                      0x0

#define FC_RC_PARAMETER_BLOCK_TOO_SMALL     (FC_RC_BASE + 0x001)
#define FC_RC_IOCTL_TIMEOUT                 (FC_RC_BASE + 0x002)
#define FC_RC_LINK_DOWN                     (FC_RC_BASE + 0x003)
#define FC_RC_BUSY                          (FC_RC_BASE + 0x004)
#define FC_RC_BAD_RESPONSE                  (FC_RC_BASE + 0x005)
#define FC_RC_TARGET_ENABLED                (FC_RC_BASE + 0x006)
#define FC_RC_TARGET_ALREADY_REGISTERED     (FC_RC_BASE + 0x007)
#define FC_RC_TARGET_NOT_ENABLED            (FC_RC_BASE + 0x008)
#define FC_RC_DIFFERENT_TARGET_REGISTERED   (FC_RC_BASE + 0x009)
#define FC_RC_DRIVER_NOT_ENABLED            (FC_RC_BASE + 0x00a)
#define FC_RC_NO_CALLBACK_SLOT_AVAILABLE    (FC_RC_BASE + 0x00b)
#define FC_RC_CALLBACK_NOT_FOUND            (FC_RC_BASE + 0x00c)
#define FC_RC_INVALID_SIGNATURE             (FC_RC_BASE + 0x00d)
#define FC_RC_INVALID_TARGET_CALLBACK       (FC_RC_BASE + 0x00e)
#define FC_RC_COUNT_ALREADY_ZERO            (FC_RC_BASE + 0x00f)
#define FC_RC_INVALID_LOOP_COMMAND          (FC_RC_BASE + 0x010)
#define FC_RC_UNKNOWN_DEVICE                (FC_RC_BASE + 0x011)
#define FC_RC_DEVICE_NOT_READY              (FC_RC_BASE + 0x012)
#define FC_RC_NO_FREE_LOOP_ID               (FC_RC_BASE + 0x013)
#define FC_RC_LOOP_ID_NOT_FOUND             (FC_RC_BASE + 0x014)
#define FC_RC_DEVICE_NOT_FOUND              (FC_RC_BASE + 0x015)
#define FC_RC_IOCTL_NOT_SUPPORTED           (FC_RC_BASE + 0x016)
#define FC_RC_COMMAND_FAILED                (FC_RC_BASE + 0x017)
#define FC_RC_SRB_NOT_FOUND                 (FC_RC_BASE + 0x018)
#define FC_RC_DEVICE_ALREADY_LOGGED_IN      (FC_RC_BASE + 0x019)
#define FC_RC_INVALID_TIMEOUT               (FC_RC_BASE + 0x01a)
#define FC_RC_REDIRECTION_ALREADY_REGISTERED                \
                                            (FC_RC_BASE + 0x01b)
#define FC_RC_NO_REDIRECTION_SLOT_AVAILABLE (FC_RC_BASE + 0x01c)
#define FC_RC_TARGET_NOT_REDIRECTED         (FC_RC_BASE + 0x01d)
#define FC_RC_NO_MATCHING_REDIRECTION       (FC_RC_BASE + 0x01e)
#define FC_RC_NO_TARGET_CONNECTION          (FC_RC_BASE + 0x01f)
#define FC_RC_NO_TYPE_SPECIFIED             (FC_RC_BASE + 0x020)
#define FC_RC_CM_INTERFACE_ALREADY_REGISTERED               \
                                            (FC_RC_BASE + 0x021)
#define FC_RC_ILLEGAL_CALLBACKS             (FC_RC_BASE + 0x022)
#define FC_RC_NO_CALLBACKS                  (FC_RC_BASE + 0x023)
#define FC_RC_DEVICE_NOT_LOGGED_IN          (FC_RC_BASE + 0x024)

#define FC_RC_INVALID_PARAMETER_1           (FC_RC_BASE + 0x081)
#define FC_RC_INVALID_PARAMETER_2           (FC_RC_BASE + 0x082)
#define FC_RC_INVALID_PARAMETER_3           (FC_RC_BASE + 0x083)
#define FC_RC_INVALID_FLAG                  (FC_RC_BASE + 0x091)

#define FC_RC_NO_ERROR_CALLBACK_PENDING     (FC_RC_BASE + 0x101)


/********
 * Define flags for use with FC_IOCTL_REGISTER_DM_CALLBACKS
 ********/

#define FC_REGISTER_INITIATOR               0x00000001
#define FC_REGISTER_TARGET_USING_DMRBS      0x00000002
#define FC_REGISTER_TARGET_USING_CCBS       0x00000004
#define FC_REGISTER_DEREGISTER              0x00000008
#define FC_REGISTER_USE_CM_INTERFACE        0x00000010
#define FC_REGISTER_TRY_TO_INITIALIZE       0x00000020
#define FC_REGISTER_TARGET                  FC_REGISTER_TARGET_USING_DMRBS


/********
 * Define flags for use with FC_IOCTL_ADD_WWID_PATH
 ********/

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


/***********************************************************************
 * STRUCTURE DEFINES
 ***********************************************************************/

/*
 * Structure used as DMD context for either initiator or target I/Os.
 */

typedef struct FC_DMRB_TAG
{
    struct FC_DMRB_TAG *    prev_ptr;   /* used by miniport to . . .   */
    struct FC_DMRB_TAG *    next_ptr;   /* . . . put on XCB wait queue */
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
    void *          miniport_context;   /* DMD exchange context */
    UINT_32E        data_length;        /* number of bytes to be transferred */
    UINT_32E        data_transferred;   /* actual number of  bytes xferred */
    SGL *           sgl;                /* pointer to Flare SGL */
    UINT_32E        sgl_offset;         /* byte offset into 1st SGL element */
    UINT_32E        physical_offset;    /* offset of phys addr from virtual */
    void            (* callback) (void *, void *);  /* completion callback */
    void *          callback_arg1;      /* 1st argument to callback function */
    void *          callback_arg2;      /* 2nd argument to callback function */
    UINT_32E        status;             /* request status */
    UINT_8E         scsi_status;        /* SCSI status byte */
    UINT_8E         sense_length;       /* nr of valid bytes in sense_buffer */
    void *          sense_buffer;       /* SCSI sense informaton */
} FC_DMRB;


/*
 * Structure used by FC_IOCTL_GET_REGISTERED_WWID_LIST and
 * FC_IOCTL_GET_WWID_PATH
 */

typedef struct FC_WWID_TAG
{
    K10_WWN         port_name;          /* 64-bit FC port world-wide name */
    K10_WWN         node_name;          /* 64-bit FC node world-wide name */
    UINT_32E        address_id;         /* 24-bit FC address ID for device.
                                         * Used by miniport only.
                                         */
} FC_WWID, * PFC_WWID;


/*
 * Structure passed by FC_IOCTL_GET_REGISTERED_WWID_LIST_SIZE and
 * FC_IOCTL_GET_REGISTERED_WWID_LIST.
 */

typedef struct FC_IOCTL_WWID_LIST_BLOCK_TAG
{
    UINT_32E        query_type;         /* Type of query (from FC header).
                                         * Filled in by caller.
                                         */
    UINT_32E        list_size;          /* Number of bytes in list.
                                         * Filled in by caller or by call to
                                         * GET_REGISTERED_WWID_LIST_SIZE
                                         */
    FC_WWID *       device_list_ptr;    /* Pointer to list of devices.
                                         * Pointer filled in by caller;
                                         * List filled in by call to
                                         * GET_REGISTERED_WWID_LIST
                                         */
} FC_IOCTL_WWID_LIST_BLOCK, * PFC_IOCTL_WWID_LIST_BLOCK;


/*
 * Structure passed by FC_IOCTL_REMOVE_WWID_PATH and
 * used by FC_IOCTL_GET_WWID_PATH
 */

typedef struct FC_DEVICE_PATH_TAG
{
    UINT_8E         path;
    UINT_8E         target;
    UINT_8E         lun;                /* unused by miniport */
    UINT_8E         reserved;
} FC_DEVICE_PATH, * PFC_DEVICE_PATH;


/*
 * Structure passed by FC_IOCTL_ADD_WWID_PATH and FC_IOCTL_GET_WWID_PATH
 */

typedef struct FC_IOCTL_WWID_PATH_BLOCK_TAG
{
    FC_WWID *       wwid_ptr;           /* Pointer to device information
                                         * of device to login.
                                         * Filled in by caller.
                                         */
    FC_DEVICE_PATH  path;               /* Path of device described by
                                         * wwid_ptr.  Filled in by miniport.
                                         */
    UINT_32E        flags;              /* Option flags for ADD_WWID_PATH
                                         * operation.  Filled in by caller.
                                         */
    void *          callback_handle;    /* Used with REMOVE_WWID_PATH to
                                         * indicate which driver is requesting
                                         * removal.
                                         */
} FC_IOCTL_WWID_PATH_BLOCK, * PFC_IOCTL_WWID_PATH_BLOCK;


/*
 * Structure passed by FC_IOCTL_DEREGISTER_DM_CALLBACKS and
 * FC_IOCTL_PROCESS_DMRB
 */

typedef struct FC_CALLBACK_HANDLE_TAG
{
    void *      callback_handle;
} FC_CALLBACK_HANDLE;


/*
 * Structure passed by FC_IOCTL_REGISTER_REDIRECTION
 */

typedef struct FC_REDIRECTION_BLOCK_TAG
{
    UINT_16E    revision;           /* structure revision - currently 1 */
    UINT_16E    flags;
    UINT_32E    target;             /* destination loop ID */
    void *      context;            /* class driver context */
    void        (*redirect) (void *, CPD_SCSI_REQ_BLK *, void *);
    void        (*execute) (void *, FC_DMRB *);
    void        (*completion) (void *);
} FC_REDIRECTION_BLOCK;


/*
 * Structure passed by FC_IOCTL_DEREGISTER_REDIRECTION
 */

typedef struct FC_UNSET_REDIRECTION_BLOCK_TAG
{
    UINT_32E    target;             /* destination loop ID */
    void *      context;            /* class driver context */
} FC_UNSET_REDIRECTION_BLOCK;


/*
 * Structure passed by FC_IOCTL_ABORT_IO
 */

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



/*
 * Structure passed by FC_IOCTL_GET_LESB
 */

typedef struct FC_GET_LESB_BLOCK_TAG
{
    UINT_32E        flags;                  /* Allow us to clear LESB out */
    UINT_32E        link_failure_count;
    UINT_32E        loss_of_sync_count;
    UINT_32E        loss_of_signal_count;
    UINT_32E        prim_seq_error_count;
    UINT_32E        invalid_xmit_word_count;
    UINT_32E        invalid_crc_count;
} FC_GET_LESB_BLOCK, * PFC_GET_LESB_BLOCK;

#endif /* DPCPD_DEBUG */

#endif  /* FC_INTERFACE_H */
