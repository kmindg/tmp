#ifndef CC_H
#define CC_H 0x00000001         /* from common dir */
#define FLARE__STAR__H

/****************************************************************
 *  Copyright (C)  Data General Corporation  1992-1999          *
 *  All rights reserved.  Licensed material - property          *
 *  of Data General Corporation.                                *
 ****************************************************************/

/********************************************************************
 * $Id: cc.h,v 1.4 1999/08/20 21:17:53 fladm Exp $
 ********************************************************************
 *
 * Description:
 *  This C header file contains the literals used
 *  to describe all of the possible SCSI Check Condition
 *  Request Sense data returned by Flare. 
 *
 *
 * History:
 *
 *  21-Jul-97   Added check condition return values for RNAL
 *                                      CC_PEER_DEAD 0x052702
 *                                      CC_RNAL_DATA_TRANSFER_ERR 0x052703
 *                                      CC_RNAL_TARGET_BUSY 0x052704
 *  30-Dec-96   Changed CC_INSUFFICIENT_xxx values from 0x053B**
 *                              to 0x0582**, old values were reserved SCSI values,
 *                              now vendor-specific.
 *      01-Aug-95       Added CC_LBA_OUT_OF_RANGE.  DGM
 *      18-Nov-94   Added CC_UA_MODE_PARAMS_CHANGED.  DGM
 *  05-Oct-94   Added some new CCs for download drive ucode.  DGM
 *      25-Jan-94       Added module-specific junk
 *                      values, AND re-tabbed the file from
 *                      4 to 8.  Yahoo!         DWD
 *      15-Apr-92       Created.                Dave DesRoches
 *
 *  $Log: cc.h,v $
 *  Revision 1.4  1999/08/20 21:17:53  fladm
 *  User: gpeterson
 *  Reason: SCSI3
 *  Added UNSUPPORTED_ENCLOSURE error, which well need for SCSI-3/SES support.
 *
 *  Revision 1.3  1999/03/31 02:47:30  fladm
 *  User: mogavero
 *  Reason: Storage_Centric
 *  Unit Attentions on Storage Centric configuration changes (exclusive of peer side UATNs).
 *
 *  Revision 1.2  1999/03/04 19:15:35  fladm
 *  User: gpeterson
 *  Reason: Storage_Centric
 *  Added some Storage Centric definies and macros
 *
 *  Revision 1.1  1999/01/05 22:23:10  fladm
 *  User: dibb
 *  Reason: initial
 *  Initial tree population
 *
 *  Revision 1.14  1998/10/02 14:13:49  kdobkows
 *   Added CC_DOWNLOAD_ALREADY_IN_PROGRESS check condition.
 *
 *  Revision 1.13  1998/09/28 17:43:35  aldred
 *   Added #define for CHECK_CONDITION.
 *
 *
 *********************************************************************/

/*
 *  static TEXT unique_file_id[] = "$Header: /fdescm/project/CVS/flare/src/include/cc.h,v 1.4 1999/08/20 21:17:53 fladm Exp $";
 */


/***************************************************************************
 * CHECK CONDITION data.
 *
 *  These values describe the problem encountered with a request.
 *
 *  The values are (left to right): sense key, additional sense code,
 *  additional sense code qualifier. Each field is one byte.
 *
 ***************************************************************************/

/* These macros extract the sense key, ASC, and ASCQ from the values
 * below and returns them in the lower byte.
 */
#define cc_info_sk(x)   (((x) >> 16) & 0xFF)
#define cc_info_asc(x)  (((x) >> 8) & 0xFF)
#define cc_info_ascq(x) ((x) & 0xFF)

#define CC_JUNK_CC_RANGE    0x00000F
#define CC_SFE_JUNK_CC_ERROR    0xFFFFF0	/* to FFFFFFFF */
#define CC_AEP_JUNK_CC_ERROR    0xFFFFE0	/* to FFFFFFEF */
        /* This define serves to provide the means for modules within
         * flare to set their Check Condition descriptors to a known
         * INVALID value.  This allows them to KNOW that there is no
         * valid info for a CC, or perhaps to indicate a particular
         * status that is important to them.
         * Every module that would like to reserve some junky statuses
         * may do so as we have done above.  For example, the SFE can
         * use the values ranging from its junk-error value thru
         * the junk-cc-range.  The modules are to define their own
         * literals to these values as long as they are within their
         * allotted range.  There should be no module-specific values
         * in this file except to establish the base value done for
         * the AEP and SFE above.
         */

#define CC_NO_SENSE 0
        /* This means no problem.  This is what we would return if an
         * initiator sent us a Request Sense CDB when we had no Check
         * Conditions or Unit Attentions to report.
         */

        /*************************************************
     * THE DEFINES FROM HERE DOWN SHOULD BE PLACED IN
     * NUMERICAL ORDER
     *************************************************/

#define CC_LUN_BECOMING_ASSIGNED 0x020401
        /* The LUN is becoming ready by being assigned.  This doesn't
         * take too long, so the initiator is urged to retry in a short
         * amount of time.
         */

#define CC_NOT_READY_LUN_DOWN 0x020403
        /* This LUN is inoperable, and operator intervention may be
         * required.
         */

#define CC_UNSUPPORTED_ENCLOSURE_FUNCTION 0x023501
        /* The host attempted to execute a SEND DIAGNOSTIC page that
         * we don't support, due to the fact that we currently use the
         * short enclosure status page only.
         */

#define CC_MEDIUM_ERROR 0x031104
#define CC_VERIFY_MEDIUM_ERROR 0x03110B

#define CC_HARDWARE_ERROR 0x040000
        /* Sauna encountered a hard error trying to satisfy the request.
         * If there is an associated Sunburst Error Code, then it is
         * embedded in the information bytes.
         */

#define CC_HW_ERR_SEL_TO 0x040500
        /* Indicates that the "LUN does not respond to selection".
         * Today we are doing this for HP machines only; perhaps we'll
         * want to do this for all vendors eventually.
         */

#define CC_NO_INSTALL_DISK 0x043A00
        /* The initiator wanted to install new Flare microcode but
         * there was no disk available in the eggcrate to accept the
         * new microcode (must be more than one disk in the eggcrate
         * that is a microcode holder).
         */

#define CC_DRIVE_DOWNLOAD_FAILURE 0x044400
        /* This is an "Internal Target Failure" and is used on
         * download drive microcode to indicate that an install
         * we attempted to one of our backend disks failed.
         */

#define CC_LUN_NOT_DEF_ASSIGNED 0x050400
        /* The Glut showed that the Lun was not assigned to this controller.
         */

#define CC_PARAM_LIST_LEN_ERROR 0x051A00
        /* The length of a parameter (Mode Select pages for example)
         * doesn't match what it should be.
         */

#define CC_ILL_REQ_INVALID_OPCODE 0x052000
        /* CDB had an opcode we don't support.
         */

#define CC_NOT_SUPPORTED 0x052001
        /* CDB tried to perform an operation that is not implemented
         * or not supported with the current array status.
         */
         
#define CC_LBA_OUT_OF_RANGE 0x052100
        /* The LBA specified (typically on a Read or Write) is
         * not valid (off the end of the media).
         */

#define CC_INVALID_FIELD_IN_CDB 0x052400
        /* Some field in the CDB was set to an illegal value.
         */

#define CC_LUN_DOESNT_EXIST 0x052500
        /* The Glut showed that no unit with this LUN value exists.
         */

#define CC_HOTSPARE_DOESNT_EXIST 0x052501
        /* This LUN is a hot spare.  To the host, it doesn't exist. But,
         * the disk array qualifier would like to know its there.  The
         * qualifier of 01 is its clue that this lun really is a hot spare.
         */

#define CC_PARAM_VALUE_INVALID 0x052602
        /* A parameter was set to an illegal value, such as a non-changeable
         * field being changed on a Mode Select.
         */

#define CC_NOT_MANAGEMENT_LOGIN 0x052680
        /* (Storage Centric) A command that required Management Login was
         * attempted without the Management Login being performed.
         */

#define CC_PEER_NOT_DISABLED 0x052700
        /* This is a "Write Protect", which indicates we cannot perform
         * the requested operation, because the Peer SP in the chassis
         * has not been disabled.  This error may be used in response
         * to a download drive microcode request.
         */

#define CC_SYSTEM_CANNOT_PERFORM_DOWNLOAD 0x052701
        /* This is a "Write Protect", which indicates we cannot perform
         * the requested operation, because one of the following conditions
         * exist:  the write cache is enabled or the Peer SP has a bind,
         * rebuild or equalize in progress. This error may be used in response
         * to a download drive microcode request.
         */

#ifdef RNAL_ENV
#define CC_PEER_DEAD 0x052702

        /* The peer is dead which means we cannot service a command which
         * requires both SPs to be present.  This error may be used in
         * response to a Read Non-Assigned Lun Command when only one SP
         * is present.
         */
#define CC_RNAL_DATA_TRANSFER_ERR 0x052703
        /* This is a Read Non-Assigned Lun CC which occurs when the target SP
         * could not transfer the RNAL data to the originator.  This is an
         * obscure error that has the potential to occur while the cache
         * is being disabled.
         */

#define CC_RNAL_TARGET_BUSY 0x052704
        /* This is a Read Non-Assigned Lun CC which occurs when the target SP
         * receives a new rnal request while it is already servicing one.  This
         * condition occurs when the originator dies, starts up again, and sends
         * a new RNAL request before the target services the original request.
         * Such a scenario is extremely unlikely but we are covering all the
         * bases here.
         */

#endif /* RNVAL_ENV */

#define CC_DOWNLOAD_ALREADY_IN_PROGRESS 0x052705
        /* This is a "Write Protect" which indicates we cannot perform
         * the requested write buffer command, because the peer SP is already
         * performing a download.
         */

#define CC_NO_SPECIFIED_DRIVES_PRESENT 0x053A00
        /* This is a "Medium Not Present" and is used on a download
         * drive microcode if we do not have any drives in the chassis
         * that match the info provided in the image file header.
         */

#ifdef RNAL_ENV
#define CC_LUN_ASSIGNED 0x058100

        /* The LUN specified in the identify message accompanying a "Read
         * non-assigned LUN" CDB is assigned to this SP.  The LUN must be
         * assigned to the peer SP for this CDB.
         */
#endif

/*
 * The following check conditions are returned from the Memory System
 * Configuration page (38h) when there is an insufficient amount of
 * some type of memory for the request.  The different codes
 * correspond to different combinations of memory types (Control RAM,
 * Front-End Stache, Back-End Stache).
 *
 * Note that these check conditions are intended to be OR'ed together:
 * CPU is bit 0, FRONT_END is bit 1, and BACK_END is bit 2.  Please
 * bear this in mind if adding new condition codes for insufficient
 * memory.
 */
#define CC_INSUFFICIENT_RAM_UNKNOWN_TYPE           0x058200
#define CC_INSUFFICIENT_CPU_RAM                    0x058201
#define CC_INSUFFICIENT_FRONT_END_RAM              0x058202
#define CC_INSUFFICIENT_BACK_END_RAM               0x058204


#define CC_ILLEGAL_GROUP_RESERVE    0x058018
        /* Received a group reserve command, from an initiator which is not
         * part of a defined group.
         */
#define CC_ILLEGAL_GROUP_DEFINE     0x058019
        /* Received a define group command and we already have 4 groups defined
         */

#define CC_GENERIC_UNIT_ATN 0x060000
#define CC_UNSOL_UATN CC_GENERIC_UNIT_ATN
        /* This value is typically used when we AEN an unsolicited.
         * We say Unit Attention, but offer no ASC or ASCQ.
         */

#define CC_POWER_ON_RESET 0x062900
        /* Either we have just been powered on, or we encountered a host
         * SCSI bus reset.
         */

#define CC_UA_MODE_PARAMS_CHANGED 0x062A01
        /* A Mode Select parameter which is common to all initiators has
         * been changed.  This is to let all the *other* initiators
         * know that.
         */

#define CC_INITIATOR_PARMETER_MODIFIED  0x062A80
        /* Storage Centric Management Access Privileges for one initiator have 
         * been modified by another initiator. This is to let the initiator 
         * whose privileges have been modified know about those changes.
         */

#define CC_VA_PARAMETER_MODIFIED        0x062A81
        /* Storage Centric Virtual Array parameters have been modified by an
         * initiator that is not assocaited with that Virtual Array. This is
         * to let the initiator associated with the VA know about the changes.
         */

#define CC_FLU_NAME_MODIFIED            0x062A82
        /* A Storage Centric FLU name has been modified. This is to let all
         * initiators that are mapped to that FLU know of the changes.
         */

#define CC_ARRAY_NAME_MODIFIED          0x062A83
        /* The Storage Centric Array Name has been modified. This is to let 
         * all initiators that access the array know of the change.
         */

#define CC_COMMANDS_CLEARED_BY_ANOTHER_INITIATOR 0x062F00
        /* This is the Unit Attention set up when a Clear Queue message
         * has aborted requests for a given initiator (and the Clear Q
         * message came from a DIFFERENT initiator).
         */

#define CC_FORCE_RESERVE 0x068001
   /* we encountered a force reserve command
    */

#define CC_ABORTED_COMMAND 0x0B0000
        /* This is returned when AEP runs into an ongoing request.
         */

#define CC_IO_PROCESS_TERMINATED 0x0B0006

#define CC_ABORTED_CMD_SEQUENCE_ERR 0x0B2C00
        /* The initiator issued a non-tagged IO as the Contingent Allegiance
         * CDB while tagged IO for the ITL nexus was present on the board and
         * the non-tagged Contingent Allegiance CDB was NOT a Request Sense.
         * Flare consider's this an invalid sequence of commands.  The
         * initiator is encouraged to reissue the command with a valid tag.
         */

#define CC_ABORTED_CMD_MSG_ERR 0x0B4300
        /* Sauna aborted the command because it attempted to use a SCSI
         * message on the host bus but it was rejected.  The message
         * would have to have been necessary to complete the operation
         * to cause Sauna to abort the command.
         */

#define CC_ABORTED_CMD_PAR_ERR 0x0B4700
        /* Sauna aborted the command due to there being more Parity Errors
         * encountered during its servicing than retries allowed.
         */

#define CC_INIT_DET_ERR_MSG_RECEIVED 0x0B4800

#define CC_INVALID_MESSAGE_ERROR 0x0B4900

#define CC_ABORTED_CMD_OVERLAPPED_OPS 0x0B4E00
        /* This is returned when Flare encounters an "Incorrect Initiator
         * Connection".  Two examples of this is a TAG error such that
         * there are 2 or more IO processes for a given ITL nexus with the
         * same tag, and the mixing of tagged and untagged IO in the absense
         * of a Contingent Allegiance.
         * All other IO's for this ITL nexus are aborted per the SCSI Spec
         * section 6.5.2.
         */

/************************************************************************
 *
 *  FED Error List, Check Condition Translation Table.
 *
 *  This table translates all possible Front End Driver (FED) errors
 *  that warrant a Check Condition into a Check Condition Descriptor.
 *  The translation is needed because the path from the DH to the HI
 *  modules in Flare is only a byte wide (dh_code in the hi_ack msg).
 *  So, we must provide a 1 byte id value for every 3-byte Check
 *  Condition that FED could generate.
 *
 *
 *  ALL CHECK CONDITION FED ERRORS MUST BE LISTED IN THIS TABLE!
 *
 ************************************************************************/


        /* This is the list of FED Error values that are meant to
         * generate a Check Condition to the AEP.
         *
         * NOTES!
         *      Since they can be reported via the DH Ext Status value,
         *      we need to make sure our values don't match any of the DH's 
         *      values.
         *
         *      For every entry in the list, the offset to CC_MIN_FED_CC
         *      serves as the offset into the TABLE!  Thus they are used as
         *      table indeces.  So, MAKE SURE THE OFFSETS IN THE LIST MATCH THE
         *      TABLE's DEFINITION.     
         *
         *      As table and list entries are added/deleted, be sure to
         *      adjust the NUM_FED_TABLE_ENTRIES define below.
         *
         *      30-Jun-94:  AMD - Changed the FED error code to descend
         *                  from 0xBF.  This was brought about due to
         *                  the sum of DH error codes and FED error codes
         *                  expanding to over 0x40.  Unfortunately, the
         *                  DH "or's in" a 0x40 if it ever detects a hard
         *                  error.  This naturally caused confusion between
         *                  FED errors and hard DH errors.  Since the DH hard
         *                  error bit could not be changed from documentation
         *                  reasons, I moved the FED error codes to a range
         *                  I felt would leave the most room for future
         *                  expansion.  WARNINGS:  1) Since the DH error
         *                  codes are growing upward and the FED error codes
         *                  are growing downward, one MUST confirm that the
         *                  ranges don't overlap (the DH error codes are
         *                  referred to as Extended Status codes ane are
         *                  defined in dh.h).  2)  No error code must ever
         *                  have the 0x40 bit set since there is code that
         *                  expects that to be the hard error bit (eg.
         *                  the BEM masks out the hard error bit before it
         *                  decodes what error it is dealing with).  This
         *                  implies the two ranges of valid error codes are:
         *                  0x01 - 0x3F and 0x80 - 0xBF.
         *
         */

#define CC_MAX_FED_CC            0xBF

/* FED Error List */

#define CC_FED_IO_PROC_TERM     (CC_MAX_FED_CC - 0)

        /* FED may return IO process terminated when retries
         *  are exhausted or an unrecoverable error occurs.
         ***************************************************/

#define CC_FED_INIT_DET_ERR_MSG (CC_MAX_FED_CC - 1)

        /* FED returns initiator detected error message when
         *  this message is received from an initiator following
         *  message in phase.  If this message is received following
         *  command, data or status phase, FED retries the operation
         *  after issuing a restore pointers message.
         *********************************************/

#define CC_FED_INV_MSG_ERR      (CC_MAX_FED_CC - 2)
        /*
         */

#define CC_FED_MSG_ERR          (CC_MAX_FED_CC - 3)
        /*
         */

#define CC_FED_OL_CMD_AT        (CC_MAX_FED_CC - 4)

        /* FED returns overlapped commands attempted upon
         *  detection of a tag error or incorrect initiator 
         *  connection.
         ***************/

#define CC_FED_SCSI_PE          (CC_MAX_FED_CC - 5)

        /* FED returns SCSI parity error when the parity error
         *  is unrecoverable.
         *********************/

/***************************************************************************
 * WARNING:  Please read the comment header at the top of the FED Error
 * codes before adding any new ones!  Also, update the below
 * CC_NUM_FED_TABLE_ENTRIES define and the CC_MIN_FED_CC comments.
 ***************************************************************************/

/* Translation Table. */
#define CC_FED_TABLE                                                \
{                                                                   \
    /* CC_FED_IO_PROC_TERM  */      CC_IO_PROCESS_TERMINATED,       \
    /* CC_FED_INIT_DET_ERR_MSG */   CC_INIT_DET_ERR_MSG_RECEIVED,   \
    /* CC_FED_INV_MSG_ERR   */      CC_INVALID_MESSAGE_ERROR,       \
    /* CC_FED_MSG_ERR       */      CC_ABORTED_CMD_MSG_ERR,         \
    /* CC_FED_OL_CMD_AT */          CC_ABORTED_CMD_OVERLAPPED_OPS,  \
    /* CC_FED_SCSI_PE       */      CC_ABORTED_CMD_PAR_ERR          \
}


#define CC_NUM_FED_TABLE_ENTRIES 6
        /* This is the number of entries in the above table.  BE SURE
         * TO KEEP THIS VALUE CURRENT!
         */

#define CC_MIN_FED_CC    ((CC_MAX_FED_CC - CC_NUM_FED_TABLE_ENTRIES) + 1)

        /* The minimum FED error code = CC_FED_SCSI_PE = 0xBA
         */

#define cc_translate_fed_err(m_x,m_y) ((m_x)[(CC_MAX_FED_CC - (m_y))])
        /* This macro will return the corresponding Check Condition descriptor
         * from the table for the given FED error list entry.
         *
         *      m_x: Address of the table in memory.
         *      m_y: FED Error (one of the list members above).
         */

/* define a data type so that we can easily see that a function is returning a
 * check condition and not just a generic UINT_32
 */
#define CHECK_CONDITION UINT_32

/******************
 * END $Id: cc.h,v 1.4 1999/08/20 21:17:53 fladm Exp $
 ******************/

#endif /* CC_H */
