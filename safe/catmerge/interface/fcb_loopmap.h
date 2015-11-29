#ifndef FCB_LOOPMAP_H
#define FCB_LOOPMAP_H 0x00000001    /* from common dir */
#define FLARE__STAR__H

/***************************************************************************
 * Copyright (C) Data General Corporation 1989-2008
 * All rights reserved.
 * Licensed material -- property of Data General Corporation
 ***************************************************************************/

/****************************************************************
*  $Id: fcb_loopmap.h,v 1.2 1999/01/12 13:59:42 fladm Exp $
*****************************************************************
*
*   This header file contains global literals and type definitions
*   for the loopmap's descriptor block.  This may get moved to a
*   generic location at a later time.
*
*   History:
*
*   12-05-95    ral Created file.
*   03-13-96    ral change loop completion status from bits to values.
*   09-03-96    ral Added literals to support necessary loop manager
*                   upgrades for the cm.
*
* $Log: fcb_loopmap.h,v $
* Revision 1.2  1999/01/12 13:59:42  fladm
* User: modonnell
* Reason: fcb_support
* Fix for EPR 207.
*
* Revision 1.1  1999/01/05 22:25:49  fladm
* User: dibb
* Reason: initial
* Initial tree population
*
* Revision 1.13  1998/08/31 18:16:27  lathrop
*  use CALLBACK_FCN
*
* Revision 1.12  1998/03/16 19:35:17  tolvanen
*  Bandwidth vs Mixed systems)
*
* Revision 1.11  1997/10/13 14:43:19  gordon
* ADDED to PRODUCT k5ph3 from PRODUCT k5ph2 on Mon Nov 10 10:21:06 EST 1997
*
* Revision 1.10  1997/07/25 15:19:46  tolvanen
*  added CM_UPDATE_TACHYON_ERROR_COUNTERS value
*
* Revision 1.9  1997/05/21 15:29:00  luongo
*  Added CM_RESET_LOOP command code.
*
* Revision 1.8  1997/05/13 14:26:39  tolvanen
*  added cm->backend cmds CM_TASKET_PREFETCH_OFF, CM_TASKET_PREFETCH_ON
*
* Revision 1.7  1997/05/07 15:23:08  luongo
* ADDED to PRODUCT k5ph2 from PRODUCT k5 on Mon May 12 15:32:38 EDT 1997
*  Added plogi, prli and pdisc stalled status codes.
*
* Revision 1.6  1996/12/03 20:22:58  jwentwor
*  Initial loop failover support
*
* Revision 1.5  1996/11/06 18:40:57  jwentwor
*  Added next/prev pointers to FCB_CM_MESSAGE_PARMS for queueing them
*
* Revision 1.4  1996/10/15 19:52:57  luongo
*  Added literals to support necessary loop manager upgrades for the cm.
*
****************************************************************/
#include "cpd_interface.h"

/*
 *  static TEXT unique_file_id[] = "$Header: /uddsw/fladm/repositories/project/CVS/flare/src/include/fcb_loopmap.h,v 1.2 1999/01/12 13:59:42 fladm Exp $ */


/* Port status constants */
typedef enum _FCB_DPDB_PORT_STATUS
{
    FCB_LPM_PORT_DISABLED = 0,  /* Device is not controlled by this SP. */
    FCB_LPM_PORT_ENABLED = 1    /* Device is contolled by this SP. */
}
FCB_DPDB_PORT_STATUS;

/* Drive Status constants for the DPDB */
typedef enum _FCB_DPDB_DRIVE_STATUS
{
    FCB_LPM_GOOD = 0x00,            /* The port is functional and discovered */
    FCB_LPM_PLOGI_FAILED = 0x01,    /* N-Port login was unsuccessful. */
    FCB_LPM_PRLI_FAILED = 0x02,     /* Process login was unsuccessful. */
    FCB_LPM_PDISC_FAILED = 0x03,    /* Target discovery was unsuccessful. */
    FCB_LPM_DEVICE_MISSING = 0x04,  /* Open to the device failed on the loop */
    FCB_LPM_PLOGI_STALLED = 0x05,   /* Failed the drive because it stalled */
    FCB_LPM_PRLI_STALLED = 0x06,    /* Failed the drive because it stalled */
    FCB_LPM_PDISC_STALLED = 0x07    /* Failed the drive because it stalled */
}
FCB_DPDB_DRIVE_STATUS;

/****************************************************************
 *   FCB_DPDB
 *****************************************************************
 *
 *  Description:    The drive port descriptor block is the element for each
 *                  entry in the loopmap and contains all the information for
 *                  a device on the loop.
 *
 *  Fields:
 *
 *  loop_id:        This is a static field. This is the FRU number assigned
 *                  to the drive by Flare.   The driver will map this ID to
 *                  an ALPA address.  The loop_id is not needed if it is a
 *                  one to one relation with the enclosure#/FRU# designation.
 *                  Since this has not been finalized, it is simplier to keep
 *                  the loop ID in this table until that decision is made.
 *
 *  port_status:    This field is read only by the driver.  The CM will set
 *                  this field to let the driver know that it is controlling
 *                  that device.
 *
 *  drive_status:   The driver maintains the status of the device in this field.
 *                  It is directly related to the results of the port discovery.
 *
 *  additional_status:
 *                  Additional drive status information for informational
 *                  purposes.  Not sure if needed at this time.
 *
 *  world_wide_name_mismatch:
 *                  This is a boolean flag used to indicate whether the
 *                  world wide name has changed for the device.
 *
 *  world_wide_name_high:
 *                  This field contains the most siginificant 32-bit word of
 *                  the world wide name of the port that is logged in.
 *  world_wide_name_low:
 *                  This field contains the least siginificant 32-bit word of
 *                  the world wide name of the port that is logged in.
 *  
 ****************************************************************/

typedef struct fcb_dpdb
{
    UINT_8E loop_id;
    FCB_DPDB_PORT_STATUS    port_status;
    FCB_DPDB_DRIVE_STATUS   drive_status;
    UINT_32 additional_status;
    BOOL    world_wide_name_mismatch;
    UINT_32 world_wide_name_hi;     /* Does CM need WWN?  If not, we could */
    UINT_32 world_wide_name_lo;     /* leave it in the FCB_PORTS_LOGGED_IN */
                                    /* structure. */
}
FCB_DPDB;


/* These status values are set when loop events cause a failure or a loop hang.
 * This values will be sent to the CM when a discovery fails to indicate what
 * caused the discovery to terminate.  This status value is loaded into the
 * completion status field of the discovery response block.
 */
#define FCB_LOOP_DISCOVERED    0x00 /* The loop is OK. */
#define FCB_INVALID_LIP        0x01 /* The link did not come up in the loop
                                     * participating state.  This is invalid for
                                     * the back-end driver which should always
                                     * comes up as participating on the loop.
                                     * The loop fail status in the discovery
                                     * response block will contain the LUP
                                     * status received from the link manager.
                                     */
#define FCB_LOOP_FAILURE       0x02 /* A Link Down occurred while discovering.
                                     * The loop fail status location in the
                                     * command block will contain what loop
                                     * event caused the link down
                                     */
#define FCB_LOOP_HUNG          0x03 /* The loop is in a hung state. */
#define FCB_LOOP_OFFLINE       0x04 /* The loop has been taken offline by the
                                     * link manager.
                                     */

/* CM status and command information
 */
#define COMMAND_ACCEPTED          0 /* return: command was accepted. */
#define COMMAND_REJECTED          1 /* return: command could not be processed */
#define COMMAND_UNSUPPORTED       2 /* return: command is not supported */

#define SUCCESSFUL      FCB_LOOP_DISCOVERED
                                    /* Successful status returned back */

#define FCB_NULL_LOOP_ID       0xff /* Indicates that the loop ID is not valid
                                     */

/*************************************************************************/
/* Literals for command type field of the FCB_CM_MESSAGE_PARMS structure */
/*************************************************************************/

/* commands issued by the CM */
#define CM_GET_LOOPMAP              0x0000  /* CM request to fetch the pointer
                                             * to the loopmap
                                             */
#define CM_SET_CALLBACK             0x0001  /* CM sends pointer to the callback
                                             * for unsolicited events
                                             */
#define CM_DISCOVER_LOOP            0x0002  /* CM request to initialize and
                                             * port discover
                                             */
#define CM_LOOP_DISCOVERED_RSP      0x0003  /* Response to the CM when the loop
                                             * discovery is completed
                                             */
#define CM_RESUME_LOOP              0x0004  /* CM request to resume the loop
                                             * and allow the task mgr to
                                             * process I/O.
                                             */
#define CM_FAIL_LOOP                0x0005  /* CM notification that the loop
                                             * has failed.
                                             */
#define CM_ENABLE_LOOP              0x0006  /* CM request to enable the desired
                                             * port.
                                             */
#define CM_DISABLE_LOOP             0x0007  /* CM request to disable the desired
                                             * port
                                             */
#define CM_HOLD_LOOP                0x0008  /* CM request to idle the DHs
                                             */
#define CM_LOGIN_LOOP               0x0009  /* CM request to idle the DHs
                                             */
#define CM_TASKET_PREFETCH_OFF      0x000A  /* CM request to disable prefetch
                                             */

#define CM_TASKET_PREFETCH_ON       0x000B  /* CM request to enable prefetch
                                             */
#define CM_RESET_LOOP               0x000C  /* CM request to reset the loop
                                             */
#define CM_SET_ALPA_TABLES          0x000D  /* CM request to use the specified
                                             * ALPA tables
                                             */
#define CM_EMALLOC_SIZE_REQUEST     0x000E  /* CM request for allocation for
                                             * given R3 size.
                                             */

#define CM_EXP_LIST_DISCOVERED_RSP  0x000F  /* Response to the CM when the expander
                                             * list discovery is completed
                                             */
#define CM_EXP_LIST_UPDATE_RSP      0x0010  /* Response to the CM when the expander
                                             * list update is completed
                                             */

/* unsolicited commands issued by the FCB driver
 */
#define FCB_LOOP_REDISCOVERED       0x0007  /* TEMPORARY UNTIL I GET JAY'S CM
                                             */
#define FCB_LOOP_NEEDS_DISCOVERY    0x0007  /* Unsolicited event to the CM to
                                             * indicate that the loop needs to
                                             * be rediscovered due to another
                                             * device issuing LIP.
                                             */
#define FCB_TEST_LOOP               0x0008  /* Unsolicited event to notify the
                                             * CM to test the loop
                                             */
#define FCB_TEST_TOPOLOGY           0x0009  /* Unsolicited event to notify 
                                             * the CM to test the topology.
                                             * The driver is indicating that
                                             * the topology may be invalid
                                             */
#define FCB_LOOP_IS_HUNG            0x000A  /* Unsolicited event to notify the
                                             * CM that a loop state time-out
                                             * or Credit error hung the loop.
                                             */
#define FCB_SUSPEND                 0x000B
#define FCB_RESUME                  0x000C

#define CM_UPDATE_TACHYON_ERROR_COUNTERS       0x000A
/* request to read the Tachyon Error Status counters and increment counters */

/* Literals for the port to enable field in the enable loop command structure.
 */
#define FCB_LOCAL_PORT          0x0000  /* Enable/Disable the local port */
#define FCB_REMOTE_PORT         0x0001  /* Enable/Disable the remote port */

/*
 * K10 Additions to unsolicited FCB events
 */
#define FCB_LINK_UP             0x10    /* FC is functioning again */
#define FCB_LINK_DOWN           0x11    /* Hold off further requests */
#define FCB_LINK_DEAD           0x12    /* Fail over to redundant bus */
#define FCB_PORT_LOGIN          0x13    /* Login of specific port */
#define FCB_PORT_LOGOUT         0x14    /* Logout of specific port */
#define FCB_SFP_EVENT           0x15    /* SFP Event */
#define FCB_TARGET_POLL_DATA_COMPLETE 0x16 /* Polling of all devices is complete */

#define FCB_LINK_STATE_CHANGE   0x17    // state change for this Link

/****************************************************************
 *   FCB_CM_MESSAGE_PARMS
 *****************************************************************
 *
 *   Description:   This is the message block structure received from the
 *                  CM when it issues a request via the IOCTL driver interface.
 *                  The driver is also required to use this structure to return
 *                  to the CM (via the callback function ptr provided by the CM)
 *                  command completion for the request.  For unsolicited event
 *                  notifications to the CM, the driver will be responsible for
 *                  building a message block to send to the CM via the
 *                  scoreboard.
 *
 *                  This structure is temporarily placed here until
 *                  it finds its real home.
 *
 *   Fields:
 *
 *   cmd_type       Opcode for the commmand received by the CM or the command
 *                  sent to the CM.
 *
 *   call_back_ptr  Callback function pointer used to send responses
 *                  back to the CM.
 *
 *   notify_argx    Callback function parameters set up by the CM and
 *                  used by the driver when invoking the callback function.
 *
 *   cmd_info       This is a union structure.  The type changes based
 *                  off of the cmd type.  It is not necessary to use
 *                  the union if a command does not require any information.
 *                  Please refer to the union types for a description of
 *                  their usage.
 *
 ****************************************************************/
typedef struct FCB_CM_MESSAGE_PARMS_TAG
{
    struct FCB_CM_MESSAGE_PARMS_TAG *next;
    struct FCB_CM_MESSAGE_PARMS_TAG *prev;

    UINT_32 cmd_type;

    /* The callback is required for all commands issued by the CM  */
    /* except for the set callback init command, get loopmap init command,
     * and the enable loop command (enable_loop added on 09-04-96)
     * and the set ALPA tables command (added on 09-25-97).
     */
    VOID (*call_back_ptr)(VOID*, VOID*);
    VOID *notify_arg1;
    VOID *notify_arg2;

#if defined(K10_ENV)
    /* K10 has multiple back-end buses.  As of 1998/09/08, this is only used
     * for back-end asynchronous events.
     */
    UINT_32 IoCtlStatus;
    UINT_8E TargetPort;
    UINT_8E TargetBus;
    UINT_8E TargetID;
    VOID*   LoopMap;    /* Paul's loopmap. */

    VOID*   pExpList;

#endif

    /* The following union is for commands that require information for
     * processing.  Commands that do not require any parameters will not
     * be in this union.
     */
    union
    {
        /* This command info is used by the CM to send a pointer to the callback
         * function so that the driver can send unsolicited event notifications
         * to the CM.  This command should be received at initialization.  The
         * driver will store the callback function pointer in its global
         * descriptor block.
         */
        struct
        {
            void (*fcn_ptr)(struct FCB_CM_MESSAGE_PARMS_TAG* cm_msg_block_ptr);
        }
        cm_callback;

        /* This command info is used by the CM to tell the driver to enable the
         * appropriate loop port.  The CM must pass a port type parameter which
         * indicates which port (local or remote) should be enabled/disabled.
         */
        struct
        {
            UINT_32 port;
        }
        loop_control;

        /* This command info is used to tell the driver which ALPA tables to
         * use, either CLARiiON custom or ANSII standard.
         */
        struct
        {
            UINT_32 type;
        }
        alpa_table_control;

        /* This command info is used for a generic response to the CM which only
         * requires completion status to be returned to the CM.
         */
        struct
        {
            UINT_32 completion_status;
        }
        generic_rsp;

        /* This command info is used for response to a CM initiated LIP
         * or to send an unsolicited internal driver discovery notification.
         */
        struct
        {
            UINT_32 completion_status;
            UINT_32 loop_fail_status;
            /* Only valid if loop completion status is set as a failure */
            UINT_8E loop_id;
            /* ID if a loop failure was a LIPF or Link Fail */
        }
        discover_rsp;

        /* This cmd info is used to send an unsolicited test loop cmd to CM */
        struct
        {
            UINT_32 loop_fail_status;
            UINT_8E loop_id;
        }
        unsolicited_test;

        /* This Cmd is used to retrieve the extended memory size allocated
         * for the passed in buffer information */
        struct
        {
            UINT_32 scb_count;
            UINT_32 max_xfer_size;
            UINT_32 sest_buff_len;
            UINT_32 emalloc_size;   /* return value from FCB */
        }
        emalloc_size_request;
       
        /* This cmd info is used to send the SFP event information to CM */
       CPD_MEDIA_INTERFACE_INFO sfp_info;

        struct
        {
            UINT_32     loop_change_type;
        }
        need_discovery_req;

        struct
        {
            UINT_32     completion_status;
        }
        update_rsp;
        CPD_LSI_LINK_STATS LSI_Link_Stats;
        CPD_MUX_LINK_STATS Mux_Link_Stats;

        CPD_PORT_INFORMATION portInfo;

    }
    cmd_info;
}
FCB_CM_MESSAGE_PARMS,
FCB_CM_MESSAGE_PARAMS, *PFCB_CM_MESSAGE_PARAMS;

/*
 * End $Id: fcb_loopmap.h,v 1.2 1999/01/12 13:59:42 fladm Exp $
 */

#endif /* ndef FCB_LOOPMAP_H */
