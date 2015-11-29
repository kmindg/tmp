/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_iots.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions related to the raid iots structure.
 *
 * @version
 *   5/14/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_queue.h"
#include "fbe_raid_library_proto.h"
#include "fbe/fbe_time.h"
#include "fbe_raid_library.h"
#include "fbe/fbe_extent_pool.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

fbe_status_t fbe_raid_iots_mark_unquiesced(fbe_raid_iots_t *iots_p)
{
    fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE);
    return FBE_STATUS_OK;
}
fbe_status_t fbe_raid_iots_mark_quiesced(fbe_raid_iots_t *iots_p)
{
    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE);
    return FBE_STATUS_OK;
}
void fbe_raid_iots_mark_complete(fbe_raid_iots_t *iots_p)
{
    fbe_raid_iots_set_status(iots_p, FBE_RAID_IOTS_STATUS_COMPLETE);
    fbe_raid_iots_transition(iots_p, fbe_raid_iots_complete_iots);

    /* We set the flag with lock held so that no other iots can complete the packet. 
     */
    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_STATUS_SENT);
    return;
}

void fbe_raid_iots_reset_generate_state(fbe_raid_iots_t *iots_p)
{
    fbe_raid_common_set_state((fbe_raid_common_t*) iots_p, 
                              (fbe_raid_common_state_t) fbe_raid_iots_generate_siots);
    return;
}
/*!***********************************************************
 * fbe_raid_iots_is_marked_quiesce()
 ************************************************************
 * @brief
 *  Determines if this iots has been marked for quiesce
 * 
 * @param iots_p - The iots to check.
 * 
 * @return FBE_TRUE - If this iots is marked for quiesce
 *        FBE_FALSE - Otherwise, not marked for quiesce
 * 
 ************************************************************/
fbe_bool_t fbe_raid_iots_is_marked_quiesce(fbe_raid_iots_t *iots_p)
{
    /*  - If the quiesce flag is set it is marked quiesced 
     */
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE))
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/****************************************
 * end fbe_raid_iots_is_marked_quiesce()
 ****************************************/

/*!***********************************************************
 * fbe_raid_iots_should_check_checksums()
 ************************************************************
 * @brief
 *  Determine if we should check checksums or not.
 * 
 * @param iots_p - The iots to check.
 * 
 * @return FBE_TRUE - Check Checksums
 *         FBE_FALSE - Otherwise, do not check checksums.
 * 
 ************************************************************/
fbe_bool_t fbe_raid_iots_should_check_checksums(fbe_raid_iots_t *iots_p)
{
    fbe_bool_t b_status = FBE_FALSE;

    fbe_payload_block_operation_flags_t payload_flags;

    /* Get the payload flags.
     */
    fbe_payload_block_get_flags(iots_p->block_operation_p, &payload_flags);

    if (payload_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM)
    {
        b_status = FBE_TRUE;
    }

    return b_status;
}
/**************************************
 * end fbe_raid_iots_should_check_checksums()
 **************************************/

/*!***********************************************************
 * fbe_raid_iots_is_corrupt_crc()
 ************************************************************
 * @brief
 *  Determine if this operation contains invalidated blocks.
 * 
 * @param iots_p - The iots to check.
 * 
 * @return FBE_TRUE - Buffer contains invalidated blocks.
 *         FBE_FALSE - Otherwise, it does not.
 * 
 ************************************************************/
fbe_bool_t fbe_raid_iots_is_corrupt_crc(fbe_raid_iots_t *iots_p)
{
    fbe_bool_t b_status = FBE_FALSE;

    fbe_payload_block_operation_flags_t payload_flags;

    /* Get the payload flags.
     */
    fbe_payload_block_get_flags(iots_p->block_operation_p, &payload_flags);

    if (payload_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CORRUPT_CRC)
    {
        b_status = FBE_TRUE;
    }

    return b_status;
}
/**************************************
 * end fbe_raid_iots_is_corrupt_crc()
 **************************************/
/*!**************************************************
 * @fn fbe_raid_iots_inc_errors
 ***************************************************
 * @brief Increment the number of errors for this iots
 *        we set the bit in the iots and also increment
 *        the count in the rgdb.
 *                          
 ***************************************************/
void fbe_raid_iots_inc_errors(fbe_raid_iots_t *iots_p)
{
    /* Only increment if the flag is not set yet.
     */
    if (!fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_ERROR))
    {
        //RAID_GROUP_DB *rgdb_p = RG_COMMON_RGDB(iots_p);
        //rgdb_p->unfinished_iots_with_errors++;
        fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_ERROR);
    }
    return;
}
/* end fbe_raid_iots_inc_errors() */

/*!**************************************************
 * @fn fbe_raid_iots_dec_errors
 ***************************************************
 * @brief Increment the number of errors for this iots
 *        we clear the bit in the iots and also
 *        decrement the count in the rgdb.
 *                          
 ***************************************************/
void fbe_raid_iots_dec_errors(fbe_raid_iots_t *iots_p)
{
    /* Only clear if the flag is set.
     */
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_ERROR))
    {
        //RAID_GROUP_DB *rgdb_p = RG_COMMON_RGDB(iots_p);
        //assert(rgdb_p->unfinished_iots_with_errors > 0);
        //rgdb_p->unfinished_iots_with_errors--;
        fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_ERROR);
    }
    return;
}
/* end fbe_raid_iots_dec_errors() */
/*!***********************************************************
 *          fbe_raid_iots_is_background_request()
 *************************************************************
 * @brief
 *  Determines if this is a background (i.e. request has been
 *  issued by the raid group monitor or not).  This method is 
 *  needed since some error handling (such as newly detected
 *  dead positions) must be different for background requests.
 * 
 * @param siots_p - The siots to check.
 * 
 * @return FBE_TRUE - Is a background request
 *        FBE_FALSE - Normla user request
 * 
 ************************************************************/

fbe_bool_t fbe_raid_iots_is_background_request(fbe_raid_iots_t *iots_p)
{
    fbe_bool_t b_status;
    fbe_payload_block_operation_opcode_t opcode;

    /*!  Currently all `disk based' request are background request
     */
    fbe_raid_iots_get_opcode(iots_p, &opcode);
    b_status = fbe_raid_library_is_opcode_lba_disk_based(opcode);
    
    return b_status;
}
/********************************************
 * end fbe_raid_iiots_is_background_request()
 ********************************************/

/*!***********************************************************
 *          fbe_raid_iots_is_metadata_request()
 *************************************************************
 * @brief
 *  Determines if this is a metadata request (i.e. request has been
 *  issued by metadata service or not).  This method is 
 *  needed since raidgroup quiesce code needs to account for Master
 *  request of each metadata request.
 *
 *  Note: Metadata requests donot fail immediately with Dead error 
 *  but wait for Continue like Normal user request.  
 * 
 * @param iots_p - The iots to check.
 * 
 * @return FBE_TRUE - Is a metadata request
 *        FBE_FALSE - Normla user/background request
 *
 * @author
 *  2/07/2012 - Created. Vamsi V
 * 
 ************************************************************/

fbe_bool_t fbe_raid_iots_is_metadata_request(fbe_raid_iots_t *iots_p)
{
    fbe_lba_t                               lba;    // start LBA of I/O
    fbe_payload_block_operation_opcode_t    opcode;    // opcode of the I/O
    fbe_raid_geometry_t*                    raid_geometry_p;    // pointer to raid geometry

    //  Get the LBA, block count, and opcode from the iots 
    fbe_raid_iots_get_current_op_lba(iots_p, &lba);
    fbe_raid_iots_get_opcode(iots_p, &opcode);
  
    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD) || 
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED) || 
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY))
    {
        /* monitor-initiated I/O (a rebuild or verify) */
        return FBE_FALSE;
    }

    /*  Determine if the "parent" I/O was to a paged metadata LBA */ 
    raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    if (fbe_raid_geometry_is_metadata_io(raid_geometry_p, lba))
    {
        return FBE_TRUE; 
    }

    return FBE_FALSE;
}
/********************************************
 * end fbe_raid_iots_is_metadata_request()
 ********************************************/

/*!***********************************************************
 * fbe_raid_iots_is_expired()
 ************************************************************
 * @brief
 *  A iots is aborted if the iots is timed out.
 * 
 * @param iots_p - The iots to check.
 * 
 * @return FBE_TRUE - If this is timed out.
 *        FBE_FALSE - Otherwise, not timed out.
 * 
 ************************************************************/
fbe_bool_t fbe_raid_iots_is_expired(fbe_raid_iots_t *const iots_p)
{
    fbe_bool_t b_status = FBE_FALSE;
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);

    /*! Iots is expired if the packet is expired.
     */
    if (fbe_transport_is_packet_expired(packet_p))
    {
        b_status = FBE_TRUE;
    }
    return b_status;
}
/**************************************
 * end fbe_raid_iots_is_expired()
 **************************************/
/*!***********************************************************
 * fbe_raid_iots_is_marked_aborted()
 ************************************************************
 * @brief
 *  Determines if this iots has been marked aborted.
 * 
 * @param iots_p - The iots to check.
 * 
 * @return FBE_TRUE - If this is marked aborted.
 *        FBE_FALSE - Otherwise, not aborted.
 * 
 ************************************************************/
fbe_bool_t fbe_raid_iots_is_marked_aborted(fbe_raid_iots_t *iots_p)
{
    fbe_bool_t b_status = FBE_FALSE;
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);

    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_ABORT))
    {
        b_status = FBE_TRUE;
    }
    else if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_ABORT_FOR_SHUTDOWN))
    {
        b_status = FBE_TRUE;
    }
    else if (fbe_transport_is_packet_cancelled(packet_p) == FBE_TRUE)
    {
        /* The packet is cancelled, do an abort for this iots.
         */
        fbe_raid_iots_abort(iots_p);
        b_status = FBE_TRUE;
    }
    return b_status;
}
/**************************************
 * end fbe_raid_iots_is_marked_aborted()
 **************************************/

/*!**************************************************************
 * fbe_raid_iots_set_as_not_used()
 ****************************************************************
 * @brief
 *  This routine is used to set the iots as not used.
 *
 * @param   iots_p - Pointer to iots for request.           
 *
 * @return FBE_RAID_STATE_STATUS_DONE
 *
 ****************************************************************/
fbe_status_t fbe_raid_iots_set_as_not_used(fbe_raid_iots_t * iots_p)
{
    fbe_raid_iots_set_status(iots_p, FBE_RAID_IOTS_STATUS_NOT_USED);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_iots_set_as_not_used()
 **************************************/

/*!***********************************************************
 * fbe_raid_iots_get_expiration_time()
 ************************************************************
 * @brief
 *  return expiration time for an iots.
 * 
 * @param iots_p - The iots to check.
 * @param expiration_time_p - expiration time to return.
 * 
 * @return FBE_STATUS_OK
 * 
 ************************************************************/
fbe_status_t fbe_raid_iots_get_expiration_time(fbe_raid_iots_t *const iots_p,
                                               fbe_time_t *expiration_time_p)
{
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);
    return fbe_transport_get_expiration_time(packet_p, expiration_time_p);
}
/**************************************
 * end fbe_raid_iots_get_expiration_time()
 **************************************/
/*!**************************************************************
 * fbe_raid_library_is_opcode_lba_disk_based()
 ****************************************************************
 * @brief
 *  determine if this opcode uses an lba that is relative to a
 *  single disk or not.  Lbas that are not relative to a
 *  disk are host or striped lbas.
 *
 * @param opcode               
 *
 * @return FBE_TRUE - Yes a single disk relative lba.
 *                    The lba is an offset from the start
 *                    of the raid group.
 *        FBE_FALSE - No, ahost based address.
 *
 * @author
 *  3/12/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_raid_library_is_opcode_lba_disk_based(fbe_payload_block_operation_opcode_t opcode)
{
    fbe_bool_t b_return;

    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY)  ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED)  ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_MARK_FOR_REBUILD)  ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY)  ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY)  ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA) || 
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA) || 
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD))
    {
        b_return = FBE_TRUE;
    }
    else
    {
        b_return = FBE_FALSE;
    }
    return b_return;
}
/******************************************
 * end fbe_raid_library_is_opcode_lba_disk_based()
 ******************************************/
/*!**************************************************************
 * fbe_raid_iots_start()
 ****************************************************************
 * @brief
 *  Start a new iots by kicking it off in the state machine.
 *
 * @param iots_p - The iots to start.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_start(fbe_raid_iots_t* iots_p)
{
    /* Set this flag so that if we are immediately going shutdown/quiesced,
     * the monitor will not try to complete us or go quiesced. 
     * This gets cleared when the first siots gets kicked off, 
     * but by that time we have other things set (oustanding_requests) so that 
     * the monitor will not complete us. 
     */
    fbe_raid_iots_lock(iots_p);
    
    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS);

    fbe_raid_iots_unlock(iots_p);
    /* We set the status to clear out any other status the object may have set.
     * This allows any code scanning the termination queue to know that this iots is active.
     */
    fbe_raid_iots_set_status(iots_p, FBE_RAID_IOTS_STATUS_AT_LIBRARY);

    /* Set our state to generate since it might have been set to something else 
     * in the object if we were queued in the object. 
     */
    fbe_raid_common_set_state((fbe_raid_common_t*)iots_p, (fbe_raid_common_state_t)fbe_raid_iots_generate_siots);

    /* 
     * increment stripe crossings for striped type of raid group
     */
    fbe_raid_iots_increment_stripe_crossings(iots_p);

    fbe_raid_common_state((fbe_raid_common_t*)iots_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_start()
 ******************************************/

/*!**************************************************************
 * fbe_raid_iots_basic_init_common()
 ****************************************************************
 * @brief
 *  Initialize all the common fields of the iots.
 *
 * @param iots_p - iots to init
 *
 * @return fbe_status_t 
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_basic_init_common(fbe_raid_iots_t* iots_p)
{
    /* Initialize the common struct.
     */
    fbe_raid_common_init(&iots_p->common);

    fbe_raid_common_init_flags(&iots_p->common, FBE_RAID_COMMON_FLAG_TYPE_IOTS);
    fbe_raid_common_init_magic_number(&iots_p->common, FBE_MAGIC_NUMBER_IOTS_REQUEST);
    fbe_raid_iots_transition(iots_p, fbe_raid_iots_generate_siots);

    /* Always use the accessor when getting or setting the IOTS status.
     */
    fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID);
    fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);
    fbe_raid_iots_set_status(iots_p, FBE_RAID_IOTS_STATUS_INVALID);

    fbe_raid_iots_init_flags(iots_p, FBE_RAID_IOTS_FLAG_GENERATING);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_basic_init_common()
 ******************************************/

/*!**************************************************************
 * fbe_raid_iots_init_common()
 ****************************************************************
 * @brief
 *  Initialize all the common fields of the iots.
 *
 * @param iots_p - iots to init
 *
 * @return fbe_status_t 
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_init_common(fbe_raid_iots_t* iots_p,
                                       fbe_lba_t lba,
                                       fbe_block_count_t blocks)
{
    /* Initialize basic fields common to all operations 
     */
    fbe_raid_iots_basic_init_common(iots_p);
    fbe_raid_iots_set_blocks_transferred(iots_p, 0);
    fbe_raid_iots_set_blocks(iots_p, blocks);
    fbe_raid_iots_set_lba(iots_p, lba);
    fbe_raid_iots_set_current_lba(iots_p, lba);
    fbe_raid_iots_set_blocks_remaining(iots_p, blocks);
    fbe_raid_iots_set_current_op_lba(iots_p, lba);
    fbe_raid_iots_set_current_op_blocks(iots_p, blocks);

    /* The api to the memory service expects the memory request to
     * be in certain state.  Initialize it here.
     */
    fbe_memory_request_init(fbe_raid_iots_get_memory_request(iots_p));

	fbe_transport_memory_request_set_io_master(fbe_raid_iots_get_memory_request(iots_p), iots_p->packet_p);
    fbe_raid_common_set_memory_request_fields_from_parent(&iots_p->common);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_init_common()
 ******************************************/

/*!**************************************************************
 * fbe_raid_iots_mark_updated_request()
 ****************************************************************
 * @brief
 *  Flag this iots as the initial request.
 *
 * @param iots_p - iots to init
 *
 * @return fbe_status_t 
 *
 ****************************************************************/
fbe_status_t fbe_raid_iots_mark_updated_request(fbe_raid_iots_t *iots_p)
{
    fbe_lba_t           packet_lba;
    fbe_block_count_t   packet_blocks;

    /* Simply set or clear last request based on the iots lba and block
     * and the packet_lba and blocks.
     */
    fbe_raid_iots_get_packet_lba(iots_p, &packet_lba);
    fbe_raid_iots_get_packet_blocks(iots_p, &packet_blocks);
    if ((iots_p->lba + iots_p->blocks) >= (packet_lba + packet_blocks))
    {
        /* Mark this as the `last iots of request'.
         */
        fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_LAST_IOTS_OF_REQUEST);
    }
    else
    {
        /* Else clear the `last iots of request' flag.
         */
        fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_LAST_IOTS_OF_REQUEST);
    }

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/**********************************************
 * end of fbe_raid_iots_mark_updated_request()
 **********************************************/

/*!**************************************************************
 * fbe_raid_iots_reinit()
 ****************************************************************
 * @brief
 *  Free any resources allocationed with current operation, then
 *  initialize all the common fields of the iots for the next
 *  operation.
 *
 * @param iots_p - iots to init
 *
 * @return fbe_status_t 
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_reinit(fbe_raid_iots_t* iots_p,
                                  fbe_lba_t lba,
                                  fbe_block_count_t blocks)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /*! First free any allocated memory before re-initializing
     *  for the next iots.
     */
    status = fbe_raid_iots_free_memory(iots_p);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        /* Log the error and continue (leaked memory)
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: iots_p: 0x%p failed to free siots with status: 0x%x \n",
                            iots_p, status);
    }

    /* Initialize the queues.
     */
    fbe_queue_init(&iots_p->available_siots_queue);
    fbe_queue_init(&iots_p->siots_queue);


    /* If this flag is set then we missing decrement of the quiesced count.
     */
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_WAS_QUIESCED))
    {
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: iots_p: 0x%p left was quiesced set: 0x%x\n",
                            iots_p, iots_p->flags);
    }

    /* Now re-initialize the iots for the next portion of the request
     */
    fbe_raid_iots_init_common(iots_p, lba, blocks);

    /* Now mark the updated request.
     */
    fbe_raid_iots_mark_updated_request(iots_p);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_reinit()
 ******************************************/

/*!**************************************************************
 * fbe_raid_iots_init()
 ****************************************************************
 * @brief
 *  Initialize all the fields of the iots.
 *
 * @param iots_p - iots to init
 * @param packet_p - packet that contains this siots.
 * @param raid_group_p - raid group this iots arrived on.
 * @param operation_p - The operation in progress.
 *
 * @return fbe_status_t 
 *
 * @author
 *  10/29/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_init(fbe_raid_iots_t* iots_p,
                                fbe_packet_t *packet_p,
                                fbe_payload_block_operation_t *block_operation_p,
                                fbe_raid_geometry_t *raid_geometry_p,
                                fbe_lba_t lba,
                                fbe_block_count_t blocks)
{
    fbe_payload_block_operation_opcode_t opcode;

    /* Initialize basic fields common to all operations 
     */
    fbe_raid_iots_basic_init(iots_p, packet_p, raid_geometry_p);

    /* Initialize fields specific to Block operations
     */
    fbe_raid_iots_set_block_operation(iots_p, block_operation_p);
    fbe_raid_iots_set_packet_lba(iots_p, lba);
    fbe_raid_iots_set_packet_blocks(iots_p, blocks);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);
    fbe_raid_iots_set_current_opcode(iots_p, opcode);
    fbe_raid_iots_set_host_start_offset(iots_p, 0);
    fbe_raid_iots_set_np_lock(iots_p, FBE_FALSE);

    /* Initialize common fields of the raid iots.
     */
    fbe_raid_iots_init_common(iots_p, lba, blocks);

    /* Mark this as the `original' iots.
     */
    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_LAST_IOTS_OF_REQUEST);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_init()
 ******************************************/

/*!**************************************************************
 * fbe_raid_iots_init_basic()
 ****************************************************************
 * @brief
 *  Initialize all the fields of the iots.
 *
 * @param iots_p - iots to init
 * @param packet_p - packet that contains this siots.
 * @param raid_group_p - raid group this iots arrived on.
 *
 * @return fbe_status_t 
 *
 * @author
 *  10/29/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_basic_init(fbe_raid_iots_t* iots_p,
									  fbe_packet_t *packet_p,
									  fbe_raid_geometry_t *raid_geometry_p)
{
    iots_p->time_stamp = fbe_get_time();
    iots_p->raid_geometry_p = (void *)raid_geometry_p;
    fbe_raid_iots_set_packet(iots_p, packet_p);
    iots_p->callback = NULL;
    iots_p->callback_context = NULL;

	iots_p->outstanding_requests = 0;
	iots_p->error = 0;

    fbe_raid_iots_set_rebuild_logging_bitmask(iots_p, 0);
    fbe_raid_iots_set_rekey_bitmask(iots_p, 0);
    fbe_queue_init(&iots_p->siots_queue);
    fbe_queue_init(&iots_p->available_siots_queue);
    
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_basic_init()
 ******************************************/

/*!**************************************************************
 * fbe_raid_iots_fast_init()
 ****************************************************************
 * @brief
 *  Initialize all the fields of the iots for the fast path.
 *  We leave out initting some fields we know we will not use.
 *
 * @param iots_p - iots to init
 * @param packet_p - packet that contains this siots.
 * @param raid_group_p - raid group this iots arrived on.
 * @param operation_p - The operation in progress.
 *
 * @return fbe_status_t 
 *
 * @author
 *  2/8/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_fast_init(fbe_raid_iots_t* iots_p)
{
    iots_p->time_stamp = fbe_get_time();

	iots_p->outstanding_requests = 0;
	iots_p->error = 0;
    fbe_raid_iots_set_rebuild_logging_bitmask(iots_p, 0);
    fbe_queue_init(&iots_p->siots_queue);
    fbe_queue_init(&iots_p->available_siots_queue);

    /* Initialize fields specific to Block operations
     */
    fbe_raid_iots_set_packet_lba(iots_p, iots_p->block_operation_p->lba);
    fbe_raid_iots_set_packet_blocks(iots_p, iots_p->block_operation_p->block_count);
    fbe_raid_iots_set_current_opcode(iots_p, iots_p->block_operation_p->block_opcode);
    fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID);
    fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);
    fbe_raid_iots_set_host_start_offset(iots_p, 0);
    fbe_raid_iots_set_np_lock(iots_p, FBE_FALSE);

    fbe_raid_iots_set_blocks_transferred(iots_p, 0);
    fbe_raid_iots_set_blocks(iots_p, iots_p->block_operation_p->block_count);
    fbe_raid_iots_set_lba(iots_p, iots_p->block_operation_p->lba);
    fbe_raid_iots_set_current_lba(iots_p, iots_p->block_operation_p->lba);
    fbe_raid_iots_set_blocks_remaining(iots_p, iots_p->block_operation_p->block_count);
    fbe_raid_iots_set_current_op_lba(iots_p, iots_p->block_operation_p->lba);
    fbe_raid_iots_set_current_op_blocks(iots_p, iots_p->block_operation_p->block_count);

    /* Initialize common fields of the raid iots.
     * Optimized since this is the only field the iots that needs to get initted now. 
     * State is initialized later. 
     */
    iots_p->common.flags = FBE_RAID_COMMON_FLAG_TYPE_IOTS;
    //fbe_raid_common_init(&iots_p->common, FBE_RAID_COMMON_FLAG_TYPE_IOTS, (fbe_raid_common_state_t)fbe_raid_iots_generate_siots);

    //fbe_raid_iots_basic_init_common(iots_p);

    /* The api to the memory service expects the memory request to
     * be in certain state.  Initialize it here.
     */
    fbe_memory_request_init(fbe_raid_iots_get_memory_request(iots_p));
    fbe_raid_common_set_memory_request_fields_from_parent(&iots_p->common);

    /* Initialize common fields of the raid iots.
     */
    //fbe_raid_iots_init_common(iots_p, lba, blocks);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_fast_init()
 ******************************************/
/*!**************************************************************
 * fbe_raid_iots_basic_destroy()
 ****************************************************************
 * @brief
 *  Perform basic teardown.
 *
 * @param iots_p - iots to destroy.
 *
 * @return fbe_status_t
 *
 * @author
 *  07/01/2011 - Created. Vamsi V
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_basic_destroy(fbe_raid_iots_t* iots_p)
{
    fbe_queue_destroy(&iots_p->siots_queue);
    fbe_queue_destroy(&iots_p->available_siots_queue);
    //fbe_spinlock_destroy(&iots_p->lock);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_basic_destroy()
 ******************************************/

/*!**************************************************************
 * fbe_raid_iots_destroy()
 ****************************************************************
 * @brief
 *  tear down the iots by destroying the fields within it.
 *
 * @param iots_p - iots to destroy.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/29/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_destroy(fbe_raid_iots_t* iots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_t *siots_p = NULL;
    fbe_raid_siots_t *next_siots_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);

    /* Some low level debug information
     */
    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACING))
    {
        fbe_memory_request_t   *memory_request_p = fbe_raid_iots_get_memory_request(iots_p);

        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_INFO,
                            "raid: iots_p: 0x%p flags: 0x%x mem req: 0x%p state: %d sz: 0x%x objs: %d \n", 
                            iots_p, fbe_raid_iots_get_flags(iots_p), memory_request_p, 
                            memory_request_p->request_state, memory_request_p->chunk_size, 
                            memory_request_p->number_of_objects);
    }

    /*! @note Although we are about to destroy the iots (which includes destroying
     *        and free the siots and it associated resources), there is not need
     *        to take the iots lock since we shouldn't be here if there are siots
     *        in progress.
     */

    /* Validate that this is a valid iots (i.e. haven't destroyed it yet)
     */
    if (RAID_DBG_ENA(!fbe_raid_common_is_flag_set((fbe_raid_common_t*)iots_p,
                                                  FBE_RAID_COMMON_FLAG_TYPE_IOTS)))
    {
        /* Something is wrong, we expected a raid iots.
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: iots_p: 0x%p FBE_RAID_COMMON_FLAG_TYPE_IOTS (0x%x) not set flags: 0x%x \n",
                            iots_p, FBE_RAID_COMMON_FLAG_TYPE_IOTS, fbe_raid_iots_get_flags(iots_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Loop through each siots_p and check for the
     * waiting for shutdown or continue flag.
     */
    siots_p = (fbe_raid_siots_t *)fbe_queue_front(&iots_p->siots_queue);
    while (siots_p != NULL)
    {
        next_siots_p = fbe_raid_siots_get_next(siots_p);
        if (!RAID_DBG_MEMORY_ENABLED) {
            status = fbe_raid_siots_destroy_resources(siots_p);
            if (status != FBE_STATUS_OK) {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "raid: %s failed to free resources of siots_p 0x%p\n",
                                     __FUNCTION__,
                                     siots_p);
            }
        }

        fbe_raid_siots_log_traffic(siots_p, "finished2");
        status = fbe_raid_siots_destroy(siots_p);
        if (status != FBE_STATUS_OK) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: %s failed to destroy siots_p 0x%p\n",
                                 __FUNCTION__,
                                 siots_p);
        }
        siots_p = next_siots_p;
    }

    /* Loop over available siots queue */
    siots_p = (fbe_raid_siots_t *)fbe_queue_front(&iots_p->available_siots_queue);
    while (siots_p != NULL)
    {
        next_siots_p = fbe_raid_siots_get_next(siots_p);
        status = fbe_raid_siots_destroy_resources(siots_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: %s failed to free resources of siots_p 0x%p\n",
                                 __FUNCTION__,
                                 siots_p);
        }
        status = fbe_raid_siots_destroy(siots_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: %s failed to destroy siots_p 0x%p\n",
                                 __FUNCTION__,
                                 siots_p);
        }
        siots_p = next_siots_p;
    }

    /*! Now free any allocated memory.  Although the packet free would take care
     *  of this automatically, it is cleaner to free the resources at the point
     *  they were allocated.
     */
    status = fbe_raid_iots_free_memory(iots_p);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        /* Log the error and continue (leaked memory)
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: iots_p: 0x%p failed to free allocated siots with status: 0x%x \n",
                            iots_p, status);
    }

    fbe_raid_iots_basic_destroy(iots_p);

    /* Init state to make us notice if we try to execute this freed siots.
     */
    fbe_raid_iots_transition(iots_p, fbe_raid_iots_freed);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_destroy()
 ******************************************/
/*!**************************************************************
 * fbe_raid_iots_fast_destroy()
 ****************************************************************
 * @brief
 *  Tear down the iots where there is only one siots.
 *
 * @param iots_p - iots to destroy.
 *
 * @return fbe_status_t
 *
 * @author
 *  2/10/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_fast_destroy(fbe_raid_iots_t* iots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_t *siots_p = NULL;
    fbe_raid_siots_t *next_siots_p = NULL;

    /* Make sure this only would have had one iots.
     */
    if (RAID_DBG_ENA((iots_p->host_start_offset != 0) &&
                     (iots_p->blocks != iots_p->current_operation_blocks) &&
                     (iots_p->blocks != iots_p->block_operation_p->block_count)))
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "iots %p is not a fast request hs: 0x%llx bl: 0x%llx co_bl: 0x%llx bo_bl: 0x%llx\n",
                                     iots_p,
				     (unsigned long long)iots_p->host_start_offset,
				     (unsigned long long)iots_p->blocks,
                                     (unsigned long long)iots_p->current_operation_blocks,
				     (unsigned long long)iots_p->block_operation_p->block_count);
    }

    /* Validate that this is a valid iots (i.e. haven't destroyed it yet)
     */
    if (RAID_DBG_ENA(!fbe_raid_common_is_flag_set((fbe_raid_common_t*)iots_p, FBE_RAID_COMMON_FLAG_TYPE_IOTS)))
    {
        /* Something is wrong, we expected a raid iots.
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: iots_p: 0x%p FBE_RAID_COMMON_FLAG_TYPE_IOTS (0x%x) not set flags: 0x%x \n",
                            iots_p, FBE_RAID_COMMON_FLAG_TYPE_IOTS, fbe_raid_iots_get_flags(iots_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }
    siots_p = (fbe_raid_siots_t *)fbe_queue_front(&iots_p->siots_queue);
    while (siots_p != NULL)
    {
        next_siots_p = fbe_raid_siots_get_next(siots_p);
        if (!RAID_DBG_MEMORY_ENABLED)
        {
            status = fbe_raid_siots_destroy_resources(siots_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "raid: %s failed to free resources of siots_p 0x%p\n",
                                     __FUNCTION__,
                                     siots_p);
            }
        }
        status = fbe_raid_siots_destroy(siots_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: %s failed to destroy siots_p 0x%p\n",
                                 __FUNCTION__,
                                 siots_p);
        }
        siots_p = next_siots_p;
    }

    fbe_raid_iots_basic_destroy(iots_p);

    /* Init state to make us notice if we try to execute this freed siots.
     */
    fbe_raid_iots_transition(iots_p, fbe_raid_iots_freed);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_fast_destroy()
 ******************************************/

/*!***************************************************************************
 *          fbe_raid_iots_get_capacity()
 *****************************************************************************
 * 
 * @brief   This routine uses the iots starting lba to determine if this is
 *          a metadata I/O or a user I/O.  It then returns the appropriate
 *          capacity.
 *
 * @param   iots_p - Pointer to iots for request
 * @param   capacity_p - Pointer to capacity value to update
 *
 * @return  status - Currently always FBE_STATUS_OK
 *
 * @author
 *  02/04/2010  Ron Proulx  - Created.
 *
 ****************************************************************************/
fbe_status_t fbe_raid_iots_get_capacity(fbe_raid_iots_t * iots_p,
                                        fbe_lba_t *capacity_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_lba_t           iots_start_lba;
    fbe_lba_t           metadata_start_lba;
    fbe_lba_t           capacity = 0;

    /* Simply use the iots starting lba to determine if this is a metadata I/O
     * then return the correct capacity.
     */
     fbe_raid_iots_get_lba(iots_p, &iots_start_lba);
     fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, &metadata_start_lba);
     
     /* Get the configured capacity
      */     
     fbe_raid_geometry_get_configured_capacity(raid_geometry_p,
                                               &capacity);    

     /* Update the value.
      */
     *capacity_p = capacity;

     /* Always return the execution status.
      */
     return(status);
}
/* end of fbe_raid_iots_get_capacity() */

/*!**************************************************************
 * fbe_raid_iots_get_chunk_range()
 ****************************************************************
 * @brief
 *  Handle fetching the raid iots information.
 * 
 * @param raid_group_p - the raid group object.
 * @param iots_p - Current I/O.
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/30/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_get_chunk_range(fbe_raid_iots_t *iots_p,
                                           fbe_lba_t lba,
                                           fbe_block_count_t blocks,
                                           fbe_chunk_size_t chunk_size,
                                           fbe_chunk_count_t *start_chunk_index_p,
                                           fbe_chunk_count_t *num_chunks_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_chunk_count_t start_chunk_index;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_lba_t extent_capacity;
    fbe_u16_t data_disks;
    fbe_chunk_count_t max_map_chunk;
    fbe_chunk_count_t total_map_chunks;
    fbe_lba_t per_drive_capacity;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_lba_t start_chunk_lba;
    fbe_block_count_t chunk_blocks;

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_raid_iots_get_opcode(iots_p, &opcode);

    /* Determine how much space per disk the raid group consumes.
     */
    fbe_raid_iots_get_capacity(iots_p, &extent_capacity);
    per_drive_capacity = extent_capacity / data_disks;

    /* Calculate the total number of map chunks since we need to 
     * limit our fetch of chunk info to the max number of chunks. 
     */
    total_map_chunks = (fbe_chunk_count_t)(per_drive_capacity / chunk_size);
    if (per_drive_capacity % chunk_size)
    {
         total_map_chunks++;
    }

    if (fbe_raid_library_is_opcode_lba_disk_based(opcode))
    {
        /* These use a per drive lba already
         */
        start_chunk_lba = lba - (lba % chunk_size); /* align down to chunk bounadary */
        chunk_blocks = blocks + (lba % chunk_size); /* include additional block for alignment */
    }
    else
    {
        fbe_raid_extent_t range[FBE_RAID_MAX_PARITY_EXTENTS];
        /* Determine where this I/O hits the raid group so we can 
         * fetch the appropriate map entry for it. 
         */
        fbe_raid_get_stripe_range(lba,
                                  blocks,
                                  raid_geometry_p->element_size,
                                  data_disks,
                                  FBE_RAID_MAX_PARITY_EXTENTS,
                                  range);
        if (range[1].size != 0)
        {
            /* If there are two ranges, then extend the size of the first range to cover
             * the second range.
             */
            if (range[1].start_lba < (range[0].start_lba + range[0].size))
            {
                fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                     "raid: range[1] start lba %llx < range[0] start_lba %llx + size %llx\n",
                                     (unsigned long long)range[1].start_lba,
				     (unsigned long long)range[0].start_lba,
				     (unsigned long long)range[0].size);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            range[0].size = (fbe_block_count_t)((range[1].start_lba + range[1].size) - range[0].start_lba);
        }
        start_chunk_lba = range[0].start_lba - (range[0].start_lba % chunk_size);
        chunk_blocks = (fbe_block_count_t)((range[0].start_lba + range[0].size) - start_chunk_lba);
    }

    /* Round up blocks to a full chunk.
     */
    if (chunk_blocks % chunk_size)
    {
        chunk_blocks += (chunk_size - (chunk_blocks % chunk_size));
    }

    /* If we exceed what can fit in the iots, then cut down to the number of chunks that 
     * fit in the iots. 
     */
    if ((chunk_blocks / chunk_size) > FBE_RAID_IOTS_MAX_CHUNKS)
    {
        chunk_blocks = FBE_RAID_IOTS_MAX_CHUNKS * chunk_size;
    }

    /* Calculate the lba where our chunk starts.
     */
    fbe_raid_iots_set_chunk_lba(iots_p, start_chunk_lba);
    fbe_raid_iots_set_chunk_size(iots_p, chunk_size);

    /*! In the future we need to call the metadata service to get this info.  
     * For now this is a direct copy from our metadata. 
     */
    start_chunk_index = (fbe_chunk_count_t)(start_chunk_lba / chunk_size);
    max_map_chunk = (fbe_chunk_count_t)(start_chunk_index + (chunk_blocks / chunk_size));

    /* We get either the max possible chunks for the iots or 
     * from here to the end of the group. 
     */
    if (max_map_chunk > total_map_chunks)
    {
        max_map_chunk = total_map_chunks;
    }
    if (start_chunk_index > max_map_chunk)
    {
        start_chunk_index = max_map_chunk;
    }
    *start_chunk_index_p = start_chunk_index;
    *num_chunks_p = max_map_chunk - start_chunk_index;

    return status;
}
/**************************************
 * end fbe_raid_iots_get_chunk_range()
 **************************************/

/*!**************************************************************
 * fbe_raid_iots_get_chunk_lba_blocks()
 ****************************************************************
 * @brief
 *  Get the start chunk lba and chunk blocks for a given iots.
 *  This allows us to know where this iots intersects
 *  the chunk information.
 * 
 * @param iots_p - Current I/O.
 * @param lba - lba for this operation.
 * @param blocks - block count of this operation.
 * @param lba_p - output lba that is relative to
 *              and rounded to chunk boundaries.
 * @param blocks_p - block count rounded to chunk boundaries.
 * @param chunk_size - number of blocks in a chunk.
 *
 * @return fbe_status_t.
 *
 * @author
 *  3/10/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_get_chunk_lba_blocks(fbe_raid_iots_t *iots_p,
                                                fbe_lba_t lba,
                                                fbe_block_count_t blocks,
                                                fbe_lba_t *lba_p,
                                                fbe_block_count_t *blocks_p,
                                                fbe_chunk_size_t chunk_size)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_u16_t data_disks;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_lba_t start_chunk_lba;
    fbe_block_count_t chunk_blocks;

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_raid_iots_get_current_opcode(iots_p, &opcode);

    if (fbe_raid_library_is_opcode_lba_disk_based(opcode))
    {
        /* These use a per drive lba already
         */
        start_chunk_lba = lba - (lba % chunk_size); /* round down to a chunk boundary */
        chunk_blocks = blocks + (lba % chunk_size); /* adjust block due to alignment */
    }
    else
    {
        fbe_raid_extent_t range[FBE_RAID_MAX_PARITY_EXTENTS];
        /* Determine where this I/O hits the raid group so we can 
         * fetch the appropriate map entry for it. 
         */
        fbe_raid_get_stripe_range(lba,
                                  blocks,
                                  raid_geometry_p->element_size,
                                  data_disks,
                                  FBE_RAID_MAX_PARITY_EXTENTS,
                                  range);
        if (range[1].size != 0)
        {
            /* If there are two ranges, then extend the size of the first range to cover
             * the second range.
             */
            if (range[1].start_lba < (range[0].start_lba + range[0].size))
            {
                fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                     "raid: range[1] start lba %llx < range[0] start_lba %llx + size %llx\n",
                                     (unsigned long long)range[1].start_lba,
				     (unsigned long long)range[0].start_lba,
				     (unsigned long long)range[0].size);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            range[0].size = (fbe_block_count_t)((range[1].start_lba + range[1].size) - range[0].start_lba);
        }
        start_chunk_lba = range[0].start_lba - (range[0].start_lba % chunk_size);
        chunk_blocks = (fbe_block_count_t)((range[0].start_lba + range[0].size) - start_chunk_lba);
    }

    /* Round up blocks to a full chunk.
     */
    if (chunk_blocks % chunk_size)
    {
        chunk_blocks += (chunk_size - (chunk_blocks % chunk_size));
    }

    *lba_p = start_chunk_lba;
    *blocks_p = chunk_blocks;
    return status;
}
/******************************************
 * end fbe_raid_iots_get_chunk_lba_blocks()
 ******************************************/

/*!**************************************************************
 * fbe_raid_is_iots_aligned_to_chunk()
 ****************************************************************
 * @brief
 *  Checks whether the given request is aligned to the chunk
 *
 * @param lba         - starting lba (physical in this case) of the request
 * @param blocks    - blocks for given request
 *
 * @return fbe_bool_t.
 *
 * @author
 *  21/04/2011 - Created. Mahesh Agarkar
 *
 ****************************************************************/
fbe_bool_t fbe_raid_is_iots_aligned_to_chunk(fbe_raid_iots_t *iots_p,
                                             fbe_chunk_size_t chunk_size)
{
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_u16_t data_disks;
    fbe_payload_block_operation_opcode_t opcode;

    raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_raid_iots_get_current_opcode(iots_p, &opcode);

    /* If this is logical, multiply the chunk size by the number of disks
     */
    if (!fbe_raid_library_is_opcode_lba_disk_based(opcode))
    {
        chunk_size *= data_disks;
    }
    
    if (((iots_p->packet_lba % chunk_size) != 0) ||
        ((iots_p->packet_blocks % chunk_size != 0) ))
    {
        /* this request is not aligned to chunk size
         */
        return FBE_FALSE;
    }
    else
    {
        return FBE_TRUE;
    }
}
/**************************************************************
 * end fbe_raid_is_iots_aligned_to_chunk()
 ***************************************************************/

/*!**************************************************************
 * fbe_raid_zero_request_aligned_to_chunk()
 ****************************************************************
 * @brief
 *  Checks whether the given iots is zero request and whether it is aligned to the
 *  chunk.
 *
 * @param iots_p         - the IOTS request 
 * @param chunk_size  - chunk size agains which we will check the alignment
 *
 * @return fbe_bool_t.
 *                            FBE_TRUE - if aligned  
 *                            FBE_FALSE - if not aligned
 *
 * @author
 *  21/04/2011 - Created. Mahesh Agarkar
 *
 ****************************************************************/
fbe_bool_t fbe_raid_zero_request_aligned_to_chunk(fbe_raid_iots_t * iots_p,
                                                  fbe_chunk_size_t chunk_size)
{
    fbe_payload_block_operation_opcode_t opcode;

    fbe_raid_iots_get_opcode(iots_p, &opcode);

    if (((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO) ||
         (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO) ||
         (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO)) &&
        fbe_raid_is_iots_aligned_to_chunk(iots_p, chunk_size))
    {
        /*verify that the request is correctly set
         */
        if ((iots_p->lba + iots_p->blocks) > (iots_p->packet_lba + iots_p->packet_blocks))
        {
            /* we are not suppose to come back here once we have set the blocks
             */
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                 "%s: IOTS (0x%p) blocks not set correctly: IOTS: Lba 0x%llx, blocks 0x%llx,"
                                 "Packet: lba 0x%llx, blocks 0x%llx \n ",
                                 __FUNCTION__,iots_p,
				(unsigned long long)iots_p->lba,
				(unsigned long long)iots_p->blocks,
				(unsigned long long)iots_p->packet_lba,
				(unsigned long long)iots_p->packet_blocks);
            return FBE_FALSE;
        }
        else
        {
            return FBE_TRUE;
        }
    }
    return FBE_FALSE;
}
/*****************************************************
 * end fbe_raid_zero_request_aligned_to_chunk()
 *****************************************************/


/*!**************************************************************************
 *          fbe_raid_iots_determine_num_siots_to_allocate()
 ****************************************************************************
 * @brief   This function determines how many additional (i.e. not counting 
 *          the embedded siots) will be required to complete the iots.
 *
 * @param   iots_p - Pointer to iots that contains the allocated siots
 * 
 * @return  num_siots_required - Number of siots to allocate
 *
 * @author
 *  10/21/2010  Ron Proulx  - Created
 *
 **************************************************************************/

fbe_u32_t fbe_raid_iots_determine_num_siots_to_allocate(fbe_raid_iots_t *iots_p)
{
    fbe_u32_t                   num_siots_required = 1;
    fbe_block_count_t           blocks_remaining = 0;
    fbe_u16_t                   data_disks = 1;
    fbe_block_count_t           max_per_drive_limit = 0;
    fbe_block_count_t           max_per_drive_blocks = 0;
    fbe_class_id_t              class_id = FBE_CLASS_ID_INVALID;
    fbe_element_size_t          element_size;
    fbe_elements_per_parity_t   elements_per_parity;
    fbe_block_count_t           max_per_drive_per_parity_stripe = 0;
    fbe_block_count_t           max_blocks_per_siots = 0;
    fbe_raid_geometry_t        *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_u32_t                   extra_for_alignment = 1;

    /*! @note Now determine the memory type based on an estimate of how many 
     *        siots are required.  We divide the transfer size by the number
     *        of data disks and then divide the result by the maximum per-disk
     *        transfer limit.  Since we are using only the remaining blocks we
     *        always round up by (1) to handle the remainder.
     */
    fbe_raid_iots_get_blocks_remaining(iots_p, &blocks_remaining);
    class_id = fbe_raid_geometry_get_class_id(raid_geometry_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_raid_geometry_get_max_blocks_per_drive(raid_geometry_p, &max_per_drive_limit);
    max_per_drive_limit = (max_per_drive_limit > 0) ? max_per_drive_limit : 0x800;

    /* Now get the blocks per parity stripe
     */
    fbe_raid_geometry_get_element_size(raid_geometry_p, &element_size);
    element_size = (element_size > 0) ? element_size : 128;
    fbe_raid_geometry_get_elements_per_parity(raid_geometry_p, &elements_per_parity);
    elements_per_parity = (elements_per_parity > 0) ? elements_per_parity : 1;
    max_per_drive_per_parity_stripe = ((fbe_block_count_t)element_size) * elements_per_parity;

    /* For parity and striper raid groups, we cannot exceed the parity stripe size
     */
    if ((class_id == FBE_CLASS_ID_PARITY)  ||
        (class_id == FBE_CLASS_ID_STRIPER)    )
    {
        max_per_drive_blocks = FBE_MIN(max_per_drive_limit, element_size);

        /*! zeros are allowed to be very big since they can span parity stripes.
         */
        if (fbe_raid_iots_is_buffered_op(iots_p))
        {
            max_per_drive_blocks = element_size; 
        }

        /* Add extra for degraded.
         */
        if (iots_p->chunk_info[0].needs_rebuild_bits != 0)
        {
            extra_for_alignment += 2;
        }

        /* Add extra for alignment
         */
        if (element_size > 128)
        {
            extra_for_alignment += 1;
        }
    }
    else
    {
        max_per_drive_blocks = max_per_drive_limit;
    }

    /* For a raid group where the LDO must pre-read, we transfer less than the
     * `max_per_drive_blocks' therefore add an additional siots.
     */
    if (fbe_raid_geometry_needs_alignment(raid_geometry_p)) {
        extra_for_alignment += 1;
    }
    /* Sanity check that we don't exceed 32-bits.
     */
    if ((((blocks_remaining / max_per_drive_blocks) +
             extra_for_alignment) > FBE_U32_MAX) ||
        ((((blocks_remaining / data_disks) / max_per_drive_blocks) + 
             extra_for_alignment) > FBE_U32_MAX))
    {
        /* we are crossing the 32bit limit here, we will return error
         */
         return (0);
    }

    /* Background request are physical so they are handled differently.
     */
    if (fbe_raid_iots_is_background_request(iots_p))
    {
        /* For parity we will limit to parity stripe instead of backend limit.
         */
        if (fbe_raid_geometry_is_raid5(raid_geometry_p) ||
            fbe_raid_geometry_is_raid6(raid_geometry_p) ||
            fbe_raid_geometry_is_raid3(raid_geometry_p))
        {
            max_blocks_per_siots = FBE_MIN(max_per_drive_per_parity_stripe,
                                           max_per_drive_limit);
        }
        else
        {
            /* Since this is a `physical' request each block is a strip across the
             * entire raid group.  Therefore don't multiple by the data_disks.
             */
            max_blocks_per_siots = max_per_drive_limit;
        }
        num_siots_required = (fbe_u32_t)((blocks_remaining / max_blocks_per_siots) + extra_for_alignment);
    }
    else
    {
        /* Else this is a `logical' request and this each siots can consume every
         * data position.
         */
        max_blocks_per_siots = (max_per_drive_blocks * data_disks);
        num_siots_required = (fbe_u32_t)((blocks_remaining / max_blocks_per_siots) + extra_for_alignment);
    }
   
    /* If enabled trace some information.
     */
    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACING))
    {
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_INFO,
                            "raid: iots_p: 0x%p siots required: %d blocks remaining: 0x%llx max per siots: 0x%llx\n",
                            iots_p, num_siots_required, (unsigned long long)blocks_remaining, (unsigned long long)max_blocks_per_siots);
    }

    return num_siots_required;
}
/*****************************************************
 * end fbe_raid_iots_determine_num_siots_to_allocate()
 *****************************************************/

/*!***************************************************************************
 *          fbe_raid_iots_activate_available_siots()
 *****************************************************************************
 *
 * @brief   The method `activates' a previously allocated siots simply by
 *          removing it from the `available' queue.
 *
 * @param   iots_p    - ptr to the fbe_raid_iots_t
 * @param   siots_pp - Address of siots pointer to populate and start state for
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  10/21/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_iots_activate_available_siots(fbe_raid_iots_t *iots_p,
                                                    fbe_raid_siots_t **siots_pp)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_siots_t   *siots_p = NULL;

    *siots_pp = siots_p;

    /* Pull the siots from the head of the active queue
     */
    siots_p = (fbe_raid_siots_t *)fbe_queue_pop(&iots_p->available_siots_queue);
    if ((siots_p != NULL) &&
        ((siots_p->common.flags & FBE_RAID_COMMON_FLAG_STRUCT_TYPE_MASK) == FBE_RAID_COMMON_FLAG_TYPE_SIOTS) )
    {
        /* Now `complete' the siots initialization
         */
        *siots_pp = siots_p;
        status = fbe_raid_siots_allocated_activate_init(siots_p, iots_p);
        if(RAID_GENERATE_COND(status != FBE_STATUS_OK))
        {
            /* We failed to initialize siots which we have allocated from
             * pool. As it is possible that another siots of current iots 
             * is under operation. And, hence we can not complete iots.
             * At best, we will set current siots to unexpected state 
             * kick off it for its completion.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                    "raid: Failed to initialize allocated siots with status: 0x%x \n",
                                    status);

            /* Change the return status to ok, so that we activate it
             */
            status = FBE_STATUS_OK;
        }
    }
    else
    {
        /* Else there is logic error.  Mark the iots failed and return the
         * failure status.
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: activate allocated siots failed with status: 0x%x\n",
                            status);
        fbe_raid_iots_memory_allocation_error(iots_p);
    }

    /* Return the execution status
     */
    return status;
}
/**********************************************
 * end fbe_raid_iots_activate_available_siots()
 **********************************************/

/*!***************************************************************************
 *          fbe_raid_iots_process_allocated_siots()
 *****************************************************************************
 * 
 * @brief   This method processes an allocated siots.  It must initialize the
 *          the siots etc.  It adds the siots to iots work queue but it is upto
 *          the caller to start the siots state machine.
 *
 * @param   iots_p - Pointer to iots to process
 * @param   siots_pp - Address of pointer to siots to populate 
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @note    This method should NOT take the iots lock!  The caller should flag
 *          any memory allocation errors.
 *
 * @author
 *  10/14/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_iots_process_allocated_siots(fbe_raid_iots_t *iots_p,
                                                   fbe_raid_siots_t **siots_pp)
{
    /* We initialize status to failure since it is expected to be populated. */
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_memory_info_t  memory_info;
    fbe_memory_request_t   *memory_request_p = fbe_raid_iots_get_memory_request(iots_p);
    fbe_u32_t               bytes_allocated = 0;
    fbe_u32_t               siots_index;
    fbe_u32_t               num_siots = 0;
    fbe_raid_siots_t       *siots_p = NULL;

    /* If the allocation isn't complete we shouldn't be here
     */
    if (!fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE))
    {
        /* Trace some information, flag the error and fail the request
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: process allocated siots failed with status: 0x%x\n",
                            status);
        return status;
    }

    /* Get the number of bytes allocated (for debug) 
     */
    bytes_allocated = memory_request_p->number_of_objects * memory_request_p->chunk_size;

    /* This method can be invoked multiple times, only plant the allocated
     * memory once.  We determine if this by checking the `siots consumed' flag.
     */
    if (!fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_SIOTS_CONSUMED))
    {
        /* Set the `siots consumed' flag.
         */
        fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_SIOTS_CONSUMED);

        /* Determine how many siots we can consume (uses same method as allocation)
        */
        num_siots = fbe_raid_iots_determine_num_siots_to_allocate(iots_p);

        if (num_siots == 0)
        {
           /* we must have crossed the 32bit limit for number of siots to allocate
            */
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: failed to determine siots allocation: num_siots == 0, iots_p = 0x%p\n",
                             iots_p);
            return FBE_STATUS_GENERIC_FAILURE; 
        }

        /* Initialize our memory request information
        */
        status = fbe_raid_memory_init_memory_info(&memory_info, memory_request_p, FBE_TRUE /* yes control */);
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: failed to init memory info : status = 0x%x, iots_p = 0x%p\n",
                            status, iots_p);
            return status; 
        }

        /* For all the allocated siots, initialize them and place them on the
         * `available' queue 
        */
        for (siots_index = 0; siots_index < num_siots; siots_index++)
        {
            status = fbe_raid_iots_consume_siots(iots_p, &memory_info);
            if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: failed to consume siots index: %d status = 0x%x, iots_p = 0x%p\n",
                            siots_index, status, iots_p);
                return status; 
            }
        }
    }

    /* Now `activate' the next available siots
     */
    status = fbe_raid_iots_activate_available_siots(iots_p, siots_pp);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        /* Trace some information, flag the error and continue.
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: activate allocated siots failed with status: 0x%x\n",
                            status);
        return status;
    }

    /* If the siots was available, place it on the active queue
     */
    siots_p = *siots_pp;
    fbe_raid_common_enqueue_tail(&iots_p->siots_queue, &siots_p->common);
    iots_p->outstanding_requests++;

    /* We have completed `allocating' the siots.  Clear the allocating siots
     * flag in the iots now.
     */
    fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS);

    /* Return the status
     */
    return status;
}
/*********************************************
 * end fbe_raid_iots_process_allocated_siots()
 *********************************************/

/*!***************************************************************************
 *      fbe_raid_iots_request_siots_allocation()
 *****************************************************************************
 *
 * @brief   This method generates a memory request for all possible siots
 *          needed to complete an iots.  That is we allocate all the required 
 *          siots at once.  We split the allocated memory into the number of siots 
 *          required and place them all on the `available' queue.
 *
 * @param   iots_p - Pointer to iots to allocate for (uses memory request from
 *                   the associated packet)
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author  
 *  10/20/2010  Ron Proulx  - Created from fbe_raid_iots_generate_siots() since
 *                            that method was growing to big.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_iots_request_siots_allocation(fbe_raid_iots_t *iots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;

    /*! Now allocate any siots required.  The majority of the time the
     *  resources are immediately available and therefore we will start
     *  the siots from this context.  If the memory allocation is
     *  deferred, we will start the siots from the memory service
     *  callback.
     *
     *  @note Unlike the siots, when an error occurs for the iots at this
     *        point we cannot immediately complete the iots since there
     *        maybe other siots outstanding.  So we simply flag the iots
     *        in error and continue.
     */
    status = fbe_raid_iots_allocate_siots(iots_p);

    /* Check if the allocation completed immediately.
     */
    if (RAID_MEMORY_COND(status == FBE_STATUS_OK))
    {
        /* Allocation completed immediately. Just return the status
         */
    }
    else if (RAID_MEMORY_COND(status == FBE_STATUS_PENDING))
    {
        /* Else this is a deferred allocation.  Just return status
         */
    }
    else
    {
        /* Trace some information, flag the error
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                        "raid: siots allocation failed with status: 0x%x\n",
                        status);
        fbe_raid_iots_memory_allocation_error(iots_p);
    }

    /* Return the status
     */
    return status;
}
/*********************************************
 * end fbe_raid_iots_request_siots_allocation()
 *********************************************/


/* Fetch the siots embedded in the packet.
 */
fbe_raid_siots_t * fbe_raid_iots_get_embedded_siots(fbe_raid_iots_t *iots_p)
{
    fbe_payload_ex_t * payload_p = NULL;
	fbe_raid_siots_t * siots_p = NULL;

    payload_p = fbe_transport_get_payload_ex(iots_p->packet_p);
	fbe_payload_ex_get_siots(payload_p, &siots_p);

    /* Mark this as the embedded siots.
     */
    if (siots_p != NULL)
    {
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_EMBEDDED_SIOTS);
    }
    return siots_p;
}

/*!**************************************************************
 * fbe_raid_iots_is_request_complete()
 ****************************************************************
 * @brief
 *  Determine if this is the last iots of the request and if the
 *  iots is complete.
 *
 * @param iots_p - This I/O.
 *
 * @return fbe_bool_t - FBE_TRUE if finished FBE_FALSE if not finished.
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_bool_t fbe_raid_iots_is_request_complete(fbe_raid_iots_t *iots_p)
{
    fbe_bool_t              b_is_request_complete = FBE_FALSE; /* Default is false */
    fbe_lba_t packet_lba;
    fbe_block_count_t packet_blocks;
    fbe_lba_t current_lba;
    fbe_block_count_t current_blocks;
    fbe_raid_iots_status_t iots_status;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;

    fbe_raid_iots_get_packet_lba(iots_p, &packet_lba);
    fbe_raid_iots_get_packet_blocks(iots_p, &packet_blocks);
    fbe_raid_iots_get_lba(iots_p, &current_lba);
    fbe_raid_iots_get_blocks(iots_p, &current_blocks);

    fbe_raid_iots_get_status(iots_p, &iots_status);
    fbe_raid_iots_get_block_status(iots_p, &block_status);
    fbe_raid_iots_get_block_qualifier(iots_p, &block_qualifier);

    /* Request is complete if:
     *  o The iots is complete
     *          AND
     *  o This is the last iots of the request.
     */
    if ((iots_status == FBE_RAID_IOTS_STATUS_COMPLETE)                               &&
        (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_LAST_IOTS_OF_REQUEST))    )
    {
        b_is_request_complete = FBE_TRUE;
    }

    if ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED) &&
        (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAIDLIB_ABORTED))
    {
        /* Raid library aborted this Iots. Use case is for a write request  
         * which started as non-degraded but has become degraded before issuing
         * writes and now requires write_logging. When Iots is reissued, Siots are
         * setup such that write_logging requirements are taken care of.    
         */
        current_blocks = 0;
        b_is_request_complete = FBE_FALSE;
        fbe_raid_iots_set_blocks(iots_p, current_blocks);
    }
    else if ((block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID) &&
             (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)     &&
             (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)    )
    {
        /* If status is invalid, it's not in use yet. 
         * If status is not success and not invalid then a real error 
         * has occurred and we need to return it immediately. 
         */
        b_is_request_complete = FBE_TRUE;
    }
    

    /* If we are complete but `b_is_request_complete' is False, something 
     * is wrong.
     */
    if ((b_is_request_complete == FBE_FALSE)                             &&
        ((current_lba + current_blocks) >= (packet_lba + packet_blocks))    )
    {
        /* We should be complete but we are not.  Trace an error.
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: iots status: 0x%x packet lba: 0x%llx blks: 0x%llx with complete False.\n",
                            iots_status, (unsigned long long)packet_lba,
			    (unsigned long long)packet_blocks);
    }

    /* Return if this request is complete or not.
     */
    return b_is_request_complete;
}
/********************************************
 * end of fbe_raid_iots_is_request_complete()
 ********************************************/

/*!**************************************************************
 * fbe_raid_iots_needs_upgrade()
 ****************************************************************
 * @brief
 *  Determine if this iots needs to upgrade its lock.
 *
 * @param iots_p - This I/O.
 *
 * @return fbe_bool_t - FBE_TRUE if upgrade needed FBE_FALSE if not.
 *
 * @author
 *  1/27/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_bool_t fbe_raid_iots_needs_upgrade(fbe_raid_iots_t *iots_p)
{
    fbe_bool_t b_return = FBE_FALSE;
    fbe_raid_iots_status_t iots_status;

    fbe_raid_iots_get_status(iots_p, &iots_status);

    /* Iots is not done if its status is not complete.
     */
    if (iots_status == FBE_RAID_IOTS_STATUS_UPGRADE_LOCK)
    {
        b_return = FBE_TRUE;
    }
    return b_return;
}
/******************************************
 * end fbe_raid_iots_needs_upgrade()
 ******************************************/

/*!**************************************************************
 * fbe_raid_iots_lock_upgrade_complete()
 ****************************************************************
 * @brief
 *  The iots has completed being upgraded and the library is
 *  being called back.
 *
 * @param iots_p - This I/O.
 *
 * @return fbe_bool_t - FBE_TRUE if upgrade needed FBE_FALSE if not.
 *
 * @author
 *  1/27/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_iots_lock_upgrade_complete(fbe_raid_iots_t *iots_p)
{
    fbe_raid_siots_t *siots_p = NULL;
    fbe_raid_siots_t *nest_siots_p = NULL;

    fbe_raid_iots_get_siots_queue_head(iots_p, &siots_p);

    if (siots_p == NULL)
    {
        /* Something is very wrong, there are no siots on this iots.
         */
        fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_CRITICAL, 
                                   "%s siots is NULL for iots %p\n",
				   __FUNCTION__, iots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_raid_siots_get_nest_queue_head(siots_p, &nest_siots_p);

    /* Clear out the lock upgrade needed status.
     */
    fbe_raid_iots_set_status(iots_p, FBE_RAID_IOTS_STATUS_AT_LIBRARY);
    
    /* If we have a nest, kick it off, otherwise kick off the head siots. 
     * The head siots is always the one that should proceed. 
     */
    if (nest_siots_p)
    {
        fbe_raid_common_state(&nest_siots_p->common);
    }
    else
    {
        fbe_raid_common_state(&siots_p->common);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_lock_upgrade_complete()
 ******************************************/

/*!**************************************************************
 * fbe_raid_iots_get_next_lba()
 ****************************************************************
 * @brief
 *  Determine if this iots is done generating lbas.
 *
 * @param iots_p - This I/O.
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_iots_get_next_lba(fbe_raid_iots_t *iots_p,
                                        fbe_lba_t *lba_p,
                                        fbe_block_count_t *blocks_p)
{
    fbe_lba_t packet_lba;
    fbe_block_count_t packet_blocks;
    fbe_lba_t current_lba;
    fbe_block_count_t current_blocks;

    fbe_raid_iots_get_packet_lba(iots_p, &packet_lba);
    fbe_raid_iots_get_packet_blocks(iots_p, &packet_blocks);
    fbe_raid_iots_get_lba(iots_p, &current_lba);
    fbe_raid_iots_get_blocks(iots_p, &current_blocks);

    *lba_p = current_lba + current_blocks;
    *blocks_p = (fbe_block_count_t)((packet_lba + packet_blocks) - *lba_p);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_get_next_lba()
 ******************************************/

/*!**************************************************************
 * fbe_raid_iots_init_for_next_lba()
 ****************************************************************
 * @brief
 *  Reinitialize the iots for the next set of lbas.
 *
 * @param iots_p - This I/O.
 *
 * @return fbe_status_t.
 *
 * @author
 *  2/1/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_iots_init_for_next_lba(fbe_raid_iots_t *iots_p)
{
    fbe_lba_t next_lba;
    fbe_block_count_t next_blocks;
    fbe_block_count_t new_offset = iots_p->host_start_offset + iots_p->blocks;

    /* Determine the next iots range.
     */
    fbe_raid_iots_get_next_lba(iots_p, &next_lba, &next_blocks);

    if (next_lba >= (iots_p->block_operation_p->lba + iots_p->block_operation_p->block_count))
    {
        fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                                   "iots: %p next lba %llx is beyond (lba %llx + bl %llx)\n",
                                   iots_p, (unsigned long long)next_lba,
				   (unsigned long long)iots_p->block_operation_p->lba, 
                                   (unsigned long long)iots_p->block_operation_p->block_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Reinitialize the iots for the new range.
     */
    fbe_raid_iots_reinit(iots_p, next_lba, next_blocks);
    /* Setup the offset so the library knows that 
     * there is an offset in any sg it is passed. 
     */
    iots_p->host_start_offset = new_offset;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_init_for_next_lba()
 ******************************************/

/*!***************************************************************
 * fbe_raid_iots_set_packet_status()
 ****************************************************************
 * 
 * @brief   Set the packet status associated with this iots.
 *
 * @param   iots_p, [I] - ptr to completed request.
 * 
 * @return  fbe_status_t
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_iots_set_packet_status(fbe_raid_iots_t * iots_p)
{
    fbe_packet_t   *packet_p = NULL;
    packet_p = fbe_raid_iots_get_packet(iots_p);

    if (packet_p == NULL)
    {
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "packet ptr null for iots: 0x%p\n", iots_p);
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    fbe_raid_iots_set_block_operation_status(iots_p);

    /* Mark the packet as complete since it was transported OK. 
     * The actual block operation status is in the block payload. 
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_iots_set_packet_status()
 *******************************/
/*!**************************************************************
 * fbe_raid_iots_determine_next_blocks()
 ****************************************************************
 * @brief
 *  Determine the next block count for this iots.
 *
 * @param iots_p - current I/O.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  12/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_determine_next_blocks(fbe_raid_iots_t *iots_p,
                                                 fbe_chunk_size_t chunk_size)
{
    fbe_block_count_t max_per_drive_blocks;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_block_count_t blocks;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_u16_t data_disks;

    fbe_raid_iots_get_blocks(iots_p, &blocks);
    fbe_raid_iots_get_opcode(iots_p, &opcode);
    raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);

    max_per_drive_blocks = (chunk_size * FBE_RAID_IOTS_MAX_CHUNKS);

    if ( (fbe_raid_library_is_opcode_lba_disk_based(opcode)) &&
        (opcode != FBE_PAYLOAD_BLOCK_OPERATION_MARK_FOR_REBUILD) )
    {
        blocks = FBE_MIN(blocks, max_per_drive_blocks);
    }
    else
    {
       /* we will not have any upper limit for block count for ZERO request
        * hence we will reduce the blocks only if : 
        *                         (1) it is not ZERO request and 
        *                         (2) it is not aligned to the chunk
        */
        if((!fbe_raid_zero_request_aligned_to_chunk(iots_p, chunk_size)) &&
           (opcode != FBE_PAYLOAD_BLOCK_OPERATION_MARK_FOR_REBUILD) )
        {
            fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
            blocks = FBE_MIN(blocks, max_per_drive_blocks * data_disks); 
        }
    }

    /* First modify the iots range.
     */
    fbe_raid_iots_set_blocks(iots_p, blocks);
    fbe_raid_iots_set_blocks_remaining(iots_p, blocks);
    fbe_raid_iots_set_current_op_blocks(iots_p, blocks);

    /* Now mark this as the `last iots of request' or not.
     */
    fbe_raid_iots_mark_updated_request(iots_p);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_determine_next_blocks()
 ******************************************/

/*!**************************************************************
 * fbe_raid_iots_limit_blocks_for_degraded()
 ****************************************************************
 * @brief
 *  Determine the next block count for this iots.
 *  We take degraded regions into account.
 *
 * @param iots_p - current I/O.               
 * @param chunk_size - Chunks size of raid group             
 *
 * @return fbe_status_t   
 *
 * @author
 *  3/10/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_iots_limit_blocks_for_degraded(fbe_raid_iots_t *iots_p,
                                                     fbe_chunk_size_t chunk_size)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_block_count_t orig_blocks;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_u16_t data_disks;
    fbe_lba_t start_chunk_lba;
    fbe_lba_t end_chunk_lba;
    fbe_block_count_t chunk_blocks;
    fbe_chunk_index_t chunk_index;
    fbe_chunk_index_t last_chunk_index;
    fbe_raid_group_paged_metadata_t *first_chunk_p = NULL;
    fbe_block_count_t max_chunks;

    fbe_raid_iots_get_current_op_blocks(iots_p, &orig_blocks);
    fbe_raid_iots_get_current_op_blocks(iots_p, &blocks);
    fbe_raid_iots_get_current_op_lba(iots_p, &lba);
    fbe_raid_iots_get_current_opcode(iots_p, &opcode);
    raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    if(fbe_raid_zero_request_aligned_to_chunk(iots_p, chunk_size))
    {
        /* for zero request we will pass the block count as it is received in packet 
         * we will not modify it here, we must have already set it.
         */
        return FBE_STATUS_OK;
    } 

    /* First decide which chunks this operation affects.
     */
    fbe_raid_iots_get_chunk_lba_blocks(iots_p, lba, blocks, &start_chunk_lba, &chunk_blocks, chunk_size);
    first_chunk_p = &iots_p->chunk_info[0];

    /* The max number of chunks is the smaller of either the total chunks remaining for 
     * this I/O or the max chunks that can fit in an iots.
     */
    max_chunks = (chunk_blocks / chunk_size);

    if (max_chunks > FBE_RAID_IOTS_MAX_CHUNKS)
    {
        max_chunks = FBE_RAID_IOTS_MAX_CHUNKS;
    }

    for (chunk_index = 1; chunk_index < max_chunks; chunk_index ++)
    {
        if ( (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH) &&
             ( (iots_p->chunk_info[chunk_index].needs_rebuild_bits != first_chunk_p->needs_rebuild_bits) ||
               (iots_p->chunk_info[chunk_index].verify_bits != first_chunk_p->verify_bits) ) )
        {
            fbe_raid_iots_trace(iots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                "flush spanning: idx: %d lba:%llx bl: 0x%llx cur/fst vr: %x:%x nr: %x:%x\n", 
                                (int)chunk_index, lba, blocks, 
                                iots_p->chunk_info[chunk_index].verify_bits, first_chunk_p->verify_bits,
                                iots_p->chunk_info[chunk_index].needs_rebuild_bits, first_chunk_p->needs_rebuild_bits);
        }
        /* We found a difference between the current chunk and the next chunk. 
         * Limit the request here. 
         * We do not break up flush requests for any reason since we need to flush exactly what 
         * was originally logged. 
         */
        if ((opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH) &&
            ( (iots_p->chunk_info[chunk_index].needs_rebuild_bits != first_chunk_p->needs_rebuild_bits) ||
              (iots_p->chunk_info[chunk_index].verify_bits != first_chunk_p->verify_bits) ||
              (iots_p->chunk_info[chunk_index].rekey != first_chunk_p->rekey) ) )
        {
            break;
        }
    }
    /* Calculate the chunk where we should cut off the I/O.
     */
    last_chunk_index = (start_chunk_lba / chunk_size) + chunk_index;

    /* Now that we have the end chunk, turn the chunk number into the appropriate end lba
     * number we can use to limit the request. 
     */
    if (fbe_raid_library_is_opcode_lba_disk_based(opcode))
    {
        end_chunk_lba = chunk_size * last_chunk_index;
    }
    else
    {
        end_chunk_lba = (chunk_size * data_disks) * last_chunk_index;
    }
    if (RAID_COND_GEO(raid_geometry_p, (end_chunk_lba - lba) > FBE_U32_MAX))
    {
        fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

        /* We overflowed on the block count.
         */
        blocks = 1;
        /*split trace into two lines*/
        
        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "iots_limit_block_4_degrade: block cnt too large>\n");
        
        fbe_raid_iots_trace(iots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                            "end_chunk_lba:%llx lba:%llx chunk_size:0x%x<\n", 
                            end_chunk_lba, lba, chunk_size);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* Now that we have the end chunk  lba,
         * limit the request to the end of this chunk.
         */
        fbe_u32_t blocks_to_chunk_end = (fbe_u32_t)(end_chunk_lba - lba);
        if (blocks_to_chunk_end == 0)
        {
            /* End of request is aligned to end of chunk, 
             * Nothing for us to do. 
             */
        }
        else
        {
            /* limit request to end of chunk.
             */
            blocks = FBE_MIN(blocks, blocks_to_chunk_end);
        }
    }

    if (RAID_COND_GEO(raid_geometry_p, (blocks > orig_blocks)))
    {
        fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
        /*Split trace into two lines*/

        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "iots_limit_block_4_degrade: block cnt %d unexpect\n",
                                      (int)blocks);
        fbe_raid_iots_trace(iots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO, 
                            "org_block:%d chunk_size:0x%x<\n", (int)orig_blocks, chunk_size);
        blocks = orig_blocks;
    }

	if (blocks != orig_blocks)
	{
		if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD) ||
			(opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH))
		{
			fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
            /*Split trace into two lines*/

            fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                          "iots_limit_block_4_degrade: block cnt %d unexpect\n",
                                          (int)blocks);
            
            fbe_raid_iots_trace(iots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                "lba:0x%llx bl:%d chunk_size:0x%x end_lba: 0x%llx idx: 0x%llx\n", 
                                iots_p->lba, (int)iots_p->blocks, chunk_size, end_chunk_lba, chunk_index);
        }

        /* First modify the range of the iots.
         */
        fbe_raid_iots_set_blocks(iots_p, blocks);
        fbe_raid_iots_set_blocks_remaining(iots_p, blocks);
        fbe_raid_iots_set_current_op_blocks(iots_p, blocks);

        /* Now mark this as the `last iots of request' or not.
         */
        fbe_raid_iots_mark_updated_request(iots_p);
    }
    return status;
}
/**********************************************
 * end fbe_raid_iots_limit_blocks_for_degraded()
 **********************************************/

/*!***************************************************************************
 * fbe_raid_iots_remove_nondegraded()
 *****************************************************************************
 *
 * @brief   Remove the leading non-degraded chunks from this request.
 *
 * @param   iots_p - current I/O.  
 * @param   positions_to_check - Bitmask of positions that we care about
 * @param   chunk_size - Chunks size of raid group             
 *
 * @return  fbe_status_t   
 *
 * @note    The request must be aligned to the physical chunk.
 *
 * @author
 *  01/27/2012  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_iots_remove_nondegraded(fbe_raid_iots_t *iots_p,
                                              fbe_raid_position_bitmask_t positions_to_check,
                                              fbe_chunk_size_t chunk_size)
{
    fbe_lba_t                       orig_lba;
    fbe_block_count_t               orig_blocks;
    fbe_chunk_count_t               orig_chunk_count;
    fbe_chunk_index_t               orig_chunk_index;
    fbe_lba_t                       new_lba;
    fbe_block_count_t               new_blocks;
    fbe_chunk_count_t               new_chunk_count;
    fbe_chunk_index_t               new_chunk_index;
    fbe_chunk_count_t               chunks_removed = 0;
    fbe_raid_geometry_t            *raid_geometry_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_u16_t                       data_disks;
    fbe_object_id_t                 object_id;
    fbe_raid_position_bitmask_t     positions_without_nr = 0;
    fbe_chunk_index_t               chunk_index;

    /* Initialize our locals.
     */
    fbe_raid_iots_get_current_op_blocks(iots_p, &orig_blocks);
    fbe_raid_iots_get_current_op_lba(iots_p, &orig_lba);
    fbe_raid_iots_get_current_opcode(iots_p, &opcode);
    raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    /* This method only works for disk based operations.
     */
    if (fbe_raid_library_is_opcode_lba_disk_based(opcode) == FBE_FALSE)
    {
        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "raid: remove nondegraded: op: %d lba: 0x%llx blks: 0x%llx is not disk op.\n",
                                      opcode, (unsigned long long)orig_lba,
				      (unsigned long long)orig_blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* This method does not support requests that are not aligned to the chunk_size.
     */
    if (fbe_raid_is_iots_aligned_to_chunk(iots_p, chunk_size) == FBE_FALSE)
    {
        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "raid: remove nondegraded: op: %d lba: 0x%llx blks: 0x%llx not aligned to chunk_size: 0x%x\n",
                                      opcode, (unsigned long long)orig_lba,
				      (unsigned long long)orig_blocks, chunk_size);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    /* The original number of chunks is simply the original blocks divided by
     * th chunk size.
     */
    orig_chunk_count = ((fbe_chunk_count_t)orig_blocks / chunk_size);
    orig_chunk_index = 0;
    new_lba = orig_lba;
    new_blocks = orig_blocks;
    new_chunk_count = orig_chunk_count;
    new_chunk_index = orig_chunk_index;

    /* If enabled display some rebuild information.
     */
    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_TRACING))
    {

        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                      "raid: remove nondegraded: start iots_p: 0x%p op: %d positions to remove: 0x%x\n",
                                      iots_p, opcode, positions_to_check);

        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                      "raid: remove nondegraded: start lba: 0x%llx blks: 0x%llx num chunks: %d\n",
                                      (unsigned long long)orig_lba,
				      (unsigned long long)orig_blocks,
				      orig_chunk_count);

        fbe_raid_iots_trace_rebuild_chunk_info(iots_p, orig_chunk_count);
    }

    /* Now walk the valid chunks and find first position with NR set.
     */
    for (chunk_index = 0; chunk_index < orig_chunk_count; chunk_index++)
    {
        /* Locate the first chunk with NR for any of the positions desired.
         */
        positions_without_nr = positions_to_check & ~(iots_p->chunk_info[chunk_index].needs_rebuild_bits);
        if (positions_without_nr == positions_to_check)
        {
            /* Increment the new chunk beyond those that don't have NR set.
             */
            new_chunk_index++;
            continue;
        }
        break;
    }

    /* The chunk_index indicates if we need to modify the iots or not.
     */
    if (new_chunk_index != orig_chunk_index)
    {
        /* We shouldn't be modifying a journal request.
         */
        if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD) ||
			(opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH))
        {
            /* Split trace into two lines.
             */
            fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                          "raid: remove nondegraded: modify for op: %d not supported. \n",
                                          opcode);
            fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                          "raid: remove nondegraded: new chunk index: %llu doesn't match original: %llu\n", 
                                          (unsigned long long)new_chunk_index,
					  (unsigned long long)orig_chunk_index);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* The new chunk count is the orginal count minus the new index.
         */
        new_chunk_count = orig_chunk_count - (fbe_chunk_count_t)new_chunk_index;

        /* Walk thru the original request and do the following and copy the 
         * chunk information from the original location to the new.
         */
        for (chunk_index = 0; chunk_index < new_chunk_count; chunk_index++)
        {
            /* Copy the chunk information to the new position.
             */
            iots_p->chunk_info[chunk_index] = iots_p->chunk_info[(new_chunk_index + chunk_index)];
        }

        /* Now increment the starting lba and decrement the blocks remaining
         * with the number of chunks removed.
         */
        chunks_removed = orig_chunk_count - new_chunk_count;
        new_lba += (chunks_removed * chunk_size);
        new_blocks -= (chunks_removed * chunk_size);

        /* Now modify the range of the iots.
         */
        fbe_raid_iots_set_lba(iots_p, new_lba);
        fbe_raid_iots_set_current_lba(iots_p, new_lba);
        fbe_raid_iots_set_current_op_lba(iots_p, new_lba);
        fbe_raid_iots_set_blocks(iots_p, new_blocks);
        fbe_raid_iots_set_blocks_remaining(iots_p, new_blocks);
        fbe_raid_iots_set_current_op_blocks(iots_p, new_blocks);

        /* Now mark this as the `last iots of request' or not.
         */
        fbe_raid_iots_mark_updated_request(iots_p);

        /* Trace some information if enabled.
         */
        if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_TRACING))
        {
            fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                          "raid: remove nondegraded: modified lba: 0x%llx blks: 0x%llx num chunks: %d\n",
                                          (unsigned long long)new_lba,
					  (unsigned long long)new_blocks,
					  new_chunk_count);

            fbe_raid_iots_trace_rebuild_chunk_info(iots_p, new_chunk_count);
        }

    } /* end if we need to modify the request. */

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_raid_iots_remove_nondegraded()
 **********************************************/

/*!***************************************************************************
 * fbe_raid_iots_verify_remove_unmarked()
 *****************************************************************************
 *
 * @brief   Remove the leading chunks from this request that are not marked for verify.
 *
 * @param   iots_p - current I/O.  
 * @param   chunk_size - Chunks size of raid group             
 *
 * @return  fbe_status_t   
 *
 * @note    The request must be aligned to the physical chunk.
 *
 * @author
 *  4/10/2012 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_raid_iots_verify_remove_unmarked(fbe_raid_iots_t *iots_p,
                                                  fbe_chunk_size_t chunk_size)
{
    fbe_lba_t                       orig_lba;
    fbe_block_count_t               orig_blocks;
    fbe_chunk_count_t               orig_chunk_count;
    fbe_chunk_index_t               orig_chunk_index;
    fbe_lba_t                       new_lba;
    fbe_block_count_t               new_blocks;
    fbe_chunk_count_t               new_chunk_count;
    fbe_chunk_index_t               new_chunk_index;
    fbe_chunk_count_t               chunks_removed = 0;
    fbe_raid_geometry_t            *raid_geometry_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_u16_t                       data_disks;
    fbe_object_id_t                 object_id;
    fbe_chunk_index_t               chunk_index;
    fbe_bool_t b_are_chunks_marked;

    /* Initialize our locals.
     */
    fbe_raid_iots_get_current_op_blocks(iots_p, &orig_blocks);
    fbe_raid_iots_get_current_op_lba(iots_p, &orig_lba);
    fbe_raid_iots_get_current_opcode(iots_p, &opcode);
    raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    /* This method only works for disk based operations.
     */
    if (fbe_raid_library_is_opcode_lba_disk_based(opcode) == FBE_FALSE)
    {
        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "raid: remove nondegraded: op: %d lba: 0x%llx blks: 0x%llx is not disk op.\n",
                                      opcode, (unsigned long long)orig_lba, (unsigned long long)orig_blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* This method does not support requests that are not aligned to the chunk_size.
     */
    if (fbe_raid_is_iots_aligned_to_chunk(iots_p, chunk_size) == FBE_FALSE)
    {
        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "raid: remove nondegraded: op: %d lba: 0x%llx blks: 0x%llx not aligned to chunk_size: 0x%x\n",
                                      opcode, (unsigned long long)orig_lba, (unsigned long long)orig_blocks, chunk_size);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    /* The original number of chunks is simply the original blocks divided by
     * th chunk size.
     */
    orig_chunk_count = ((fbe_chunk_count_t)orig_blocks / chunk_size);
    orig_chunk_index = 0;
    new_lba = orig_lba;
    new_blocks = orig_blocks;
    new_chunk_count = orig_chunk_count;
    new_chunk_index = orig_chunk_index;

    /* If enabled display some rebuild information.
     */
    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_VERIFY_TRACING))
    {

        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                      "raid: remove unmarked vr: start iots_p: 0x%p op: %d\n",
                                      iots_p, opcode);

        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                      "raid: remove unmarked vr: start lba: 0x%llx blks: 0x%llx num chunks: %d\n",
                                      (unsigned long long)orig_lba, (unsigned long long)orig_blocks, orig_chunk_count);

        fbe_raid_iots_trace_verify_chunk_info(iots_p, orig_chunk_count);
    }

    b_are_chunks_marked = FBE_FALSE;
    for (chunk_index = 0; chunk_index < orig_chunk_count; chunk_index++)
    {
        /* Locate the first chunk with verify for any of the positions desired.
         */
        if (iots_p->chunk_info[chunk_index].verify_bits == 0)
        {
            /* Increment the new chunk beyond those that don't have verify set.
             */
            new_chunk_index++;
            continue;
        }
        b_are_chunks_marked = FBE_TRUE;
        break;
    }
    if (!b_are_chunks_marked)
    {
        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                      "raid: remove unmarked verify: no chunks marked\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The chunk_index indicates if we need to modify the iots or not.
     */
    if (new_chunk_index != orig_chunk_index)
    {
        /* The new chunk count is the orginal count minus the new index.
         */
        new_chunk_count = orig_chunk_count - (fbe_chunk_count_t)new_chunk_index;

        /* Walk thru the original request and do the following and copy the 
         * chunk information from the original location to the new.
         */
        for (chunk_index = 0; chunk_index < new_chunk_count; chunk_index++)
        {
            /* Copy the chunk information to the new position.
             */
            iots_p->chunk_info[chunk_index] = iots_p->chunk_info[(new_chunk_index + chunk_index)];
        }

        /* Now increment the starting lba and decrement the blocks remaining
         * with the number of chunks removed.
         */
        chunks_removed = orig_chunk_count - new_chunk_count;
        new_lba += (chunks_removed * chunk_size);
        new_blocks -= (chunks_removed * chunk_size);

        /* Now modify the range of the iots.
         */
        fbe_raid_iots_set_lba(iots_p, new_lba);
        fbe_raid_iots_set_current_lba(iots_p, new_lba);
        fbe_raid_iots_set_current_op_lba(iots_p, new_lba);
        fbe_raid_iots_set_blocks(iots_p, new_blocks);
        fbe_raid_iots_set_blocks_remaining(iots_p, new_blocks);
        fbe_raid_iots_set_current_op_blocks(iots_p, new_blocks);

        /* Now mark this as the `last iots of request' or not.
         */
        fbe_raid_iots_mark_updated_request(iots_p);

        /* Trace some information if enabled.
         */
        if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_VERIFY_TRACING))
        {
            fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                          "raid: remove unmarked verify: modified lba: 0x%llx blks: 0x%llx num chunks: %d\n",
                                          (unsigned long long)new_lba, (unsigned long long)new_blocks, new_chunk_count);

            fbe_raid_iots_trace_verify_chunk_info(iots_p, new_chunk_count);
        }

    } /* end if we need to modify the request. */

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_raid_iots_verify_remove_unmarked()
 **********************************************/

/*!***************************************************************************
 * fbe_raid_iots_mark_piece_complete()
 *****************************************************************************
 *
 * @brief   Simply mark the current iots piece as complete.
 *
 * @param   iots_p - current I/O.  
 *
 * @return  None 
 *
 *****************************************************************************/
void fbe_raid_iots_mark_piece_complete(fbe_raid_iots_t *iots_p)
{
    fbe_raid_iots_set_blocks_remaining(iots_p, 0);
}
/**********************************************
 * end fbe_raid_iots_mark_piece_complete()
 **********************************************/

/*!***************************************************************************
 * fbe_raid_iots_is_piece_complete()
 *****************************************************************************
 *
 * @brief   Simply mark the current iots piece as complete.
 *
 * @param   iots_p - current I/O.  
 *
 * @return  bool
 *
 *****************************************************************************/
fbe_bool_t fbe_raid_iots_is_piece_complete(fbe_raid_iots_t *iots_p)
{
    fbe_block_count_t   blocks_remaining;

    fbe_raid_iots_get_blocks_remaining(iots_p, &blocks_remaining);
    if (blocks_remaining == 0)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/**********************************************
 * end fbe_raid_iots_is_piece_complete()
 **********************************************/

/*!**************************************************************
 * fbe_raid_iots_validate_chunk_info()
 ****************************************************************
 * @brief
 *  Handle fetching the raid iots information.
 * 
 * @param raid_group_p - the raid group object.
 * @param iots_p - Current I/O.
 * @param lba - start lba to get chunk info.
 * @param blocks - total blocks to get chunk info for.
 *
 * @return fbe_status_t.
 *
 ****************************************************************/
static fbe_bool_t fbe_raid_iots_allow_non_degraded = FBE_TRUE;
fbe_status_t fbe_raid_iots_validate_chunk_info(fbe_raid_iots_t *iots_p,
                                               fbe_lba_t lba,
                                               fbe_block_count_t blocks)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_chunk_count_t start_bitmap_chunk_index;
    fbe_chunk_count_t num_chunks;
    fbe_chunk_size_t chunk_size;
    fbe_u32_t iots_chunk_index = 0;
    fbe_raid_position_bitmask_t rebuild_bits;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_object_id_t raid_group_object_id; 
    fbe_u32_t num_bits;

    raid_group_object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    fbe_raid_iots_get_chunk_size(iots_p, &chunk_size);
    /* Get the range of chunks affected by this iots.
     */
    fbe_raid_iots_get_chunk_range(iots_p, lba, blocks, chunk_size, &start_bitmap_chunk_index, &num_chunks);

    /*! Loop over all the affected chunks. 
     */
    rebuild_bits = iots_p->chunk_info[0].needs_rebuild_bits;
    for (iots_chunk_index = 0; iots_chunk_index < num_chunks; iots_chunk_index++)
    {
        /* There is something wrong if any of the bits do not match the first one.
         */
        if(RAID_COND_GEO(raid_geometry_p, iots_p->chunk_info[iots_chunk_index].needs_rebuild_bits != rebuild_bits))
        {
            fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                                "%s status: needs_rebuild_bits 0x%x at chunk 0x%x != rebuild_bits 0x%x\n",
                                __FUNCTION__, iots_p->chunk_info[iots_chunk_index].needs_rebuild_bits,
                                iots_chunk_index, rebuild_bits);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (!fbe_raid_iots_allow_non_degraded && 
            (raid_geometry_p->width != 1) &&
            (iots_p->chunk_info[iots_chunk_index].needs_rebuild_bits == 0))
        {
            fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_CRITICAL, 
                                       "%s raid_geometry_p->width 0x%x is not 1 and needs rebuild bit iots_p->chunk_info 0x%x is not set\n",
                                   __FUNCTION__, raid_geometry_p->width, 
                                   iots_p->chunk_info[iots_chunk_index].needs_rebuild_bits );
        }
        if (iots_p->chunk_info[iots_chunk_index].needs_rebuild_bits != rebuild_bits)
        {

            fbe_raid_library_object_trace(raid_group_object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                          "raid: rebuild bits: 0x%x for chunk[%d] != chunk[0]: 0x%x\n",
                                   iots_p->chunk_info[iots_chunk_index].needs_rebuild_bits, iots_chunk_index,
                                   rebuild_bits);
        }
        num_bits = fbe_raid_get_bitcount(iots_p->chunk_info[iots_chunk_index].needs_rebuild_bits);
        if (num_bits > 1)
        {
            /* We allow mirrors to have multiple `needs rebuild' positions.
             */
            if (fbe_raid_geometry_is_parity_type(raid_geometry_p) ||
                fbe_raid_geometry_is_striper_type(raid_geometry_p))
            {
                /* Raid 6 allows 2 rebuild positions.
                 */
                if (!fbe_raid_geometry_is_raid6(raid_geometry_p) || (num_bits > 2))
                {
                    /* If this isn't a mirror flag an error.
                     */
                    fbe_raid_library_object_trace(raid_group_object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                                  "raid: obj=0x%x; num_bits=%d, has too many iots_p->chunk_info[%d].nr_bits=0x%x\n",
                                                  raid_group_object_id, num_bits, iots_chunk_index,
                                                  iots_p->chunk_info[iots_chunk_index].needs_rebuild_bits);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
        }
    }
    return status;
}
/**************************************
 * end fbe_raid_iots_validate_chunk_info()
 **************************************/

/*!**************************************************************
 * fbe_raid_iots_inc_aborted_shutdown()
 ****************************************************************
 * @brief
 *  Add one to the siots shutdown error counts.
 *
 * @param iots_p - Ptr to I/O to save these counts in.
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  4/12/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_inc_aborted_shutdown(fbe_raid_iots_t *const iots_p)
{
    fbe_raid_verify_error_counts_t *vp_eboard_local_p = NULL;
        
    /* If the client object supplied a vp eboard add one to shutdown counts.
     */
    fbe_raid_iots_get_verify_eboard(iots_p, &vp_eboard_local_p);

    if (vp_eboard_local_p != NULL)
    {
        vp_eboard_local_p->shutdown_errors++;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_inc_aborted_shutdown()
 ******************************************/
/*!**************************************************************
 * fbe_raid_iots_abort()
 ****************************************************************
 * @brief
 *  Abort the iots and any siots.
 *  
 * @param iots_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  4/19/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_abort(fbe_raid_iots_t *const iots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_t *siots_p = NULL;

    if (!fbe_raid_common_is_flag_set((fbe_raid_common_t*)iots_p,
                                     FBE_RAID_COMMON_FLAG_TYPE_IOTS))
    {
        /* Something is wrong, we expected a raid iots.
         */
        fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_CRITICAL, 
                               "%s iots: %p common flags: 0x%x are unexpected\n",
                               __FUNCTION__, iots_p, iots_p->common.flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_raid_iots_lock(iots_p);

    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_ABORT);

    /*! @note Remove this call when the memory service fully supports the abort api
     */
    fbe_raid_memory_mock_change_defer_delay(FBE_FALSE);

    /* Abort any outstanding memory request
     */
    if (fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY))
    {
        /* If we are waiting for memory, abort the memory request
         */
        status = fbe_raid_memory_api_abort_request(fbe_raid_iots_get_memory_request(iots_p));
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        {
            /* Trace a message and continue
             */
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                                "raid: iots_p: 0x%p failed to abort memory request with status: 0x%x \n",
                                iots_p, status);
        }
    }

    /* Loop through each siots_p and abort it.
     */
    siots_p = (fbe_raid_siots_t *)fbe_queue_front(&iots_p->siots_queue);
    while (siots_p != NULL)
    {
        fbe_raid_siots_abort(siots_p);
        siots_p = fbe_raid_siots_get_next(siots_p);
    }
    fbe_raid_iots_unlock(iots_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_abort()
 ******************************************/

/*!**************************************************************
 * fbe_raid_iots_get_failed_io_pos_bitmap()
 ****************************************************************
 * @brief
 *  Get a bitmap of failed positions in the given iots.
 *  
 * @param in_iots_p
 * @param out_failed_io_position_bitmap_p
 * 
 * @return fbe_status_t
 * 
 * @author
 *  7/2010 - Created. rundbs
 *
 ****************************************************************/
fbe_status_t fbe_raid_iots_get_failed_io_pos_bitmap(fbe_raid_iots_t    *const in_iots_p, 
                                                    fbe_u32_t          *out_failed_io_position_bitmap_p)
{
    fbe_raid_siots_t        *siots_p;
    fbe_u32_t               error_bitmap;
    fbe_u32_t               running_total_error_bitmap;


    //  Initialize local variables
    error_bitmap                = 0;
    running_total_error_bitmap  = 0;

    //  For the incoming iots, loop through each siots_p and nested siots, if applicable, 
    //  looking for failed I/O
    fbe_raid_iots_lock(in_iots_p);
    siots_p = (fbe_raid_siots_t *)fbe_queue_front(&in_iots_p->siots_queue);
    while (siots_p != NULL)
    {
        fbe_raid_siots_t *nsiots_p = (fbe_raid_siots_t *) fbe_queue_front(&siots_p->nsiots_q);

        //  Check any nested siots first
        while (nsiots_p != NULL)
        {
            //  Get the failed position bitmap for the nested siots
            fbe_raid_siots_get_failed_io_pos_bitmap(nsiots_p, &error_bitmap);

            //  Add the error bitmap to a running total
            running_total_error_bitmap |= error_bitmap;

            //  Advance to next nested siots
            nsiots_p = fbe_raid_siots_get_next(nsiots_p);
        }

        //  Now check the siots
        fbe_raid_siots_get_failed_io_pos_bitmap(siots_p, &error_bitmap);

        //  Add the error bitmap to a running total
        running_total_error_bitmap |= error_bitmap;

        //  Advance to next siots
        siots_p = fbe_raid_siots_get_next(siots_p);
    }
    fbe_raid_iots_unlock(in_iots_p);

    //  Set the output parameter
    *out_failed_io_position_bitmap_p = running_total_error_bitmap;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_get_failed_io_pos_bitmap()
 ******************************************/


/*!**************************************************************
 * fbe_raid_iots_get_upgraded_lock_range()
 ****************************************************************
 * @brief
 *  Get the range needed for when we upgrade the lock.
 *  This gives a lock range that is aligned to the chunk size.
 *
 * @param iots_p - The current I/O.
 * @param stripe_number_p - the stripe number aligned to a chunk.
 * @param stripe_count_p - the stripe count aligned to a chunk.
 *
 * @return FBE_STATUS_OK - success.
 *         FBE_STATUS_GENERIC_FAILURE - Failure.
 *
 * @author
 *  7/22/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_get_upgraded_lock_range(fbe_raid_iots_t *iots_p,
                                                   fbe_u64_t *stripe_number_p,
                                                   fbe_u64_t *stripe_count_p)
{
    fbe_status_t status;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_lba_t packet_lba;
    fbe_block_count_t packet_blocks;
    fbe_chunk_size_t chunk_size;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);

    fbe_raid_iots_get_chunk_size(iots_p, &chunk_size);
    fbe_raid_iots_get_packet_lba(iots_p, &packet_lba);
    fbe_raid_iots_get_packet_blocks(iots_p, &packet_blocks);

    /* Get the lba range of chunks affected by this iots.
     * This returns an lba/blocks for one disk. 
     */
    status = fbe_raid_iots_get_chunk_lba_blocks(iots_p, packet_lba, packet_blocks, &lba, &blocks, chunk_size);

    if (status != FBE_STATUS_OK) { return status; }

    /* Now that we know the rounded set of lba/blocks for this set of chunks,
     * determine the stripe lock range. 
     */
    status = fbe_raid_geometry_calculate_lock_range_physical(raid_geometry_p,
                                                             lba,
                                                             blocks,
                                                             stripe_number_p,
                                                             stripe_count_p);

    return status;
}
/******************************************
 * end fbe_raid_iots_get_upgraded_lock_range()
 ******************************************/


/*!**************************************************************
 * fbe_raid_iots_set_unexpected_error()
 ****************************************************************
 * @brief
 *  For a given iots transition it to an unexpected error state.
 * 
 * @param iots_p - siots to transition to failed.
 * @param line - The line number in source file where failure occurred.
 * @param function_p - Function that detected error
 * @param fail_string_p - This is the string describing the failure.
 *
 * @return - None.
 *
 * @author
 *  08/16/2010 - Created. Jyoti Ranjan
 *
 ****************************************************************/

void fbe_raid_iots_set_unexpected_error(fbe_raid_iots_t *const iots_p,
                                        fbe_u32_t line,
                                        const char *function_p,
                                        fbe_char_t * fail_string_p, ...)
{
    char buffer[FBE_RAID_MAX_TRACE_CHARS];
    fbe_u32_t num_chars_in_string = 0;
    va_list argList;
    fbe_raid_iots_status_t  iots_status;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    if (fail_string_p != NULL)
    {
        va_start(argList, fail_string_p);
        num_chars_in_string = _vsnprintf(buffer, FBE_RAID_MAX_TRACE_CHARS, fail_string_p, argList);
        buffer[127] = '\0';
        va_end(argList);
    }

    fbe_raid_iots_get_status(iots_p, &iots_status);

    fbe_raid_library_object_trace(object_id, 
                                  FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                  "raid: iots 0x%p lba: 0x%llx blks: 0x%llx remaining: 0x%llx flags: 0x%x iots_status: 0x%x\n",
                                  iots_p, (unsigned long long)iots_p->lba,
				  (unsigned long long)iots_p->blocks,
				  (unsigned long long)iots_p->blocks_remaining, 
                           fbe_raid_iots_get_flags(iots_p), iots_status);

    
    fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                           "error: line: %d file: %s\n", line, function_p);

    /* Display the string, if it has some characters.
     */
    if (num_chars_in_string > 0)
    {
        fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, buffer);
    }

    /* Here instead of panicking, we will print critical error.
     */

    fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                  "raid: function: %s line: %d unexpected error detected\n",
                                  function_p, line);

    /* Transition this iots to a failure state.
     */
    fbe_raid_iots_transition(iots_p, fbe_raid_iots_unexpected_error);
    return;
}
/******************************************
 * end fbe_raid_iots_set_unexpected_error()
 ******************************************/



void fbe_raid_iots_lock(fbe_raid_iots_t *iots_p)
{
	fbe_packet_t * packet = NULL;
	packet = fbe_raid_iots_get_packet(iots_p);
    fbe_transport_lock(packet);

    //fbe_spinlock_lock(&iots_p->lock);
    return;
}

void fbe_raid_iots_unlock(fbe_raid_iots_t *iots_p)
{
	fbe_packet_t * packet = NULL;
	packet = fbe_raid_iots_get_packet(iots_p);
    fbe_transport_unlock(packet);

    //fbe_spinlock_unlock(&iots_p->lock);
    return;
}

/*!**************************************************************
 * fbe_raid_iots_transition_quiesced()
 ****************************************************************
 * @brief
 *  Set the fields in the iots so it appears to be quiesced.
 *
 * @param iots_p - current io.
 * @param state - State for iots to execute when it awakes.
 * @param context - Context to set in the iots callback ptr.
 *
 * @return None.   
 *
 * @author
 *  8/17/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_iots_transition_quiesced(fbe_raid_iots_t *iots_p, 
                                       fbe_raid_iots_state_t state, 
                                       fbe_raid_iots_completion_context_t context)
{
    fbe_raid_iots_set_state(iots_p, state);
    fbe_raid_iots_set_callback(iots_p, NULL, context);
    fbe_raid_iots_set_status(iots_p, FBE_RAID_IOTS_STATUS_NOT_STARTED_TO_LIBRARY);
    fbe_raid_iots_mark_quiesced(iots_p);
    return;
}
/******************************************
 * end fbe_raid_iots_transition_quiesced()
 ******************************************/

/*!***************************************************************
 * fbe_raid_iots_set_block_operation_status()
 ****************************************************************
 * 
 * @brief   Set the packet status associated with this iots.
 *
 * @param   iots_p, [I] - ptr to completed request.
 * @param   block_operation_p, [I] - ptr to block operation to be updated.
 *  
 * @return  fbe_status_t
 *
 * @author
 *  10/31/2011 - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_raid_iots_set_block_operation_status(fbe_raid_iots_t * iots_p)
{
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_block_operation_status_t block_status, payload_block_operation_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
	fbe_packet_t * packet = NULL;
	fbe_payload_ex_t * payload = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

	packet = fbe_raid_iots_get_packet(iots_p);
	payload = fbe_transport_get_payload_ex(packet);


    block_operation_p = fbe_raid_iots_get_block_operation(iots_p);
    if (block_operation_p == NULL)
    {
        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "missing block operation in iots: 0x%p\n", iots_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The error status is determined before we get here. 
     * Either the iots never generated siots (and thus is marked in error). 
     * Or siots finishing set the status in the iots. 
     */
    fbe_raid_iots_get_block_status(iots_p, &block_status);
    fbe_raid_iots_get_block_qualifier(iots_p, &block_qualifier);
    if (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID)
    {
        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                                      "block status not set for iots: 0x%p\n", iots_p);
    }
    fbe_payload_block_get_status(block_operation_p, &payload_block_operation_status);

    /* When an iots within this block operation encountered a media error,
     * we set the packet block operation status to media error to record the fact
     * there was media error.  We do not want to over write this error,
     * so only update the block operation status when there was not media error. 
     */
    if (payload_block_operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
    {
        /*! if we already populated the media error lba then do not change it. 
         * Otherwise populate it with an invalid lba to indicate it is not used. 
         */
        fbe_payload_block_set_status(block_operation_p, block_status, block_qualifier);
        if ((block_qualifier != FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONTINUE_REKEY) &&
            (block_qualifier != FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOTHING_TO_REKEY)) {
            fbe_payload_ex_set_media_error_lba(payload, FBE_RAID_INVALID_LBA);
        }
        fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_DEBUG_HIGH, 
                                   "%s: payload block status 0x%x changes to 0x%x!\n", 
                                   __FUNCTION__, payload_block_operation_status, block_status);
    } else {
        fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_INFO, 
                                   "%s: payload block status 0x%x, iots block status 0x%x!\n", 
                                   __FUNCTION__, payload_block_operation_status, block_status);
    }
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_iots_set_block_operation_status()
 *******************************/


void fbe_raid_iots_get_verify_eboard(fbe_raid_iots_t *const iots_p, 
                                                   fbe_raid_verify_error_counts_t **eboard_p)
{
	fbe_packet_t * packet = NULL;
	fbe_payload_ex_t * payload = NULL;

	packet = fbe_raid_iots_get_packet(iots_p);
	payload = fbe_transport_get_payload_ex(packet);

    fbe_payload_ex_get_verify_error_count_ptr(payload, (void **)eboard_p);
    return;
}

/*!**************************************************************
 * fbe_raid_iots_get_statistics()
 ****************************************************************
 * @brief
 *  Fetch the statistics for this iots.
 *
 * @param None.       
 *
 * @param iots_p
 * @param stats_p
 *
 * @author
 *  4/29/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_iots_get_statistics(fbe_raid_iots_t *iots_p,
                                  fbe_raid_library_statistics_t *stats_p)
{
    fbe_raid_siots_t *siots_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_iots_get_opcode(iots_p, &opcode);

    fbe_raid_iots_lock(iots_p);
    siots_p = (fbe_raid_siots_t *)fbe_queue_front(&iots_p->siots_queue);
    while (siots_p != NULL)
    {
        fbe_raid_siots_t *nsiots_p = (fbe_raid_siots_t *) fbe_queue_front(&siots_p->nsiots_q);

        /* Check all the nested sub-iots and check if they are quiesced.
         */
        while (nsiots_p != NULL)
        {
            fbe_raid_siots_get_statistics(nsiots_p, stats_p);
            nsiots_p = fbe_raid_siots_get_next(nsiots_p);
            stats_p->total_nest_siots++;
        }

        fbe_raid_siots_get_statistics(siots_p, stats_p);
        siots_p = fbe_raid_siots_get_next(siots_p);
        stats_p->total_siots++;
    }

    switch (opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            stats_p->read_count++;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
            stats_p->write_count++;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
            stats_p->write_verify_count++;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
            stats_p->verify_write_count++;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
            stats_p->zero_count++;
            break;
        default:
            stats_p->unknown_count++;
            stats_p->unknown_opcode = opcode;
            break;
    };
    stats_p->total_iots++;
    fbe_raid_iots_unlock(iots_p);
}
/**************************************
 * end fbe_raid_iots_get_statistics
 **************************************/

/*!**************************************************************
 * fbe_raid_iots_is_metadata_operation()
 ****************************************************************
 * @brief
 *  Paged metadata operations handle error paths differently in some cases.
 * 
 * @param siots_p - current I/O.               
 *
 * @return fbe_bool_t FBE_TRUE if metadata op
 *                   FBE_FALSE otherwise.
 *
 * @author
 *  9/11/2013 - copied from siots routine. NCHIU
 *
 ****************************************************************/

fbe_bool_t fbe_raid_iots_is_metadata_operation(fbe_raid_iots_t *iots_p)
{
    fbe_bool_t b_status;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_u16_t data_disks;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_lba_t metadata_start_lba;

    fbe_raid_iots_get_lba(iots_p, &lba);
    fbe_raid_iots_get_blocks(iots_p, &blocks);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, &metadata_start_lba);

    if (fbe_raid_iots_is_background_request(iots_p))
    {
        /* Go from a single drive lba to a host lba.
         */
        lba = lba * data_disks;
        blocks = blocks * data_disks;
    }
    /* If any part of the I/O is in the metadata area it is a metadata I/O.
     */
    if ((lba + blocks) > metadata_start_lba)
    {
        b_status = FBE_TRUE;
    }
    else
    {
        b_status = FBE_FALSE;
    }
    return b_status;
}
/******************************************
 * end fbe_raid_iots_is_metadata_operation()
 ******************************************/
/*!**************************************************************
 * fbe_raid_get_rekey_pos()
 ****************************************************************
 * @brief
 *  This function checks to see if a rekey is required.
 *
 * @param raid_geometry_p
 * @param rekey_bitmask
 * @param rekey_pos_p - output next position.
 *
 * @return bool status - returns FBE_TRUE if the operation needs to run
 *                       otherwise returns FBE_FALSE
 *
 * @author
 *  11/5/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t
fbe_raid_get_rekey_pos(fbe_raid_geometry_t *raid_geometry_p,
                       fbe_raid_position_bitmask_t rekey_bitmask,
                       fbe_raid_position_t *rekey_pos_p)
{
    fbe_raid_position_bitmask_t         rekey_pos_bitmask;
    fbe_raid_position_t                 pos;
    fbe_u32_t                           width = 0;
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    
    *rekey_pos_p = FBE_RAID_POSITION_INVALID;
    rekey_pos_bitmask =rekey_bitmask;

    for (pos = 0; pos < width; pos++) {
        if ((1 << pos) & rekey_pos_bitmask) {
            *rekey_pos_p = pos;
        }
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_get_rekey_pos()
 ******************************************************************************/

/*!**************************************************************
 * fbe_raid_iots_get_block_edge()
 ****************************************************************
 * @brief
 *  Fetch the block edge using the information in the iots.
 *
 * @param iots_p - iots ptr.
 *
 * @return bool status - returns FBE_TRUE if the operation needs to run
 *                       otherwise returns FBE_FALSE
 *
 * @author
 *  6/11/2014 - Created. Rob Foley 
 *  
 ****************************************************************/
fbe_status_t
fbe_raid_iots_get_block_edge(fbe_raid_iots_t *iots_p,
                             fbe_block_edge_t **block_edge_p,
                             fbe_u32_t position)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);

    fbe_extent_pool_slice_get_drive_position(iots_p->extent_p, iots_p->extent_context_p, &position);
    /* Use the slice information to determine the position to access.
     */
    fbe_raid_geometry_get_block_edge(raid_geometry_p, block_edge_p, position);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_iots_get_block_edge()
 ******************************************************************************/

/*!**************************************************************
 * fbe_raid_iots_get_extent_offset()
 ****************************************************************
 * @brief
 *  Determine the offset for this particular position.
 *
 * @param iots_p - iots ptr.
 *
 * @return bool status - returns FBE_TRUE if the operation needs to run
 *                       otherwise returns FBE_FALSE
 *
 * @author
 *  6/11/2014 - Created. Rob Foley 
 *  
 ****************************************************************/
fbe_status_t
fbe_raid_iots_get_extent_offset(fbe_raid_iots_t *iots_p,
                                fbe_u32_t position,
                                fbe_lba_t *offset_p)
{
    fbe_extent_pool_slice_get_position_offset(iots_p->extent_p, iots_p->extent_context_p, position, offset_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_iots_get_extent_offset()
 ******************************************************************************/

/*************************
 * end file fbe_raid_iots.c
 *************************/

