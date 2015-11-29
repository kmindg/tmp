/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_parity_write_log_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions for write_logging.
 *
 * @author
 *  01/13/2011 Daniel Cummins Created.
 *
 ***************************************************************************/

/*************************
 **  INCLUDE FILES 
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"
#include "fbe/fbe_payload_metadata_operation.h"
#include "fbe_raid_library.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe_cmi.h"


/*!***************************************************************
 *          fbe_parity_journal_build_data()
 *****************************************************************
 *
 * @brief   Build a journal data structure based on 
 *          the physical lba, and described by the fruts chain provided.
 *
 * @param   siots_p - contains the write data to log (write_fruts_head)
 *                    and the pointer to the journal data entry that
 *                    must describe the log operation
 *
 * @return  FBE_STATUS_OK if journal data accessible
 *          FBE_STATUS_GENERIC_FAILURE if journal data not found
 *
 * @author
 *  08/04/2010  Kevin Schleicher - Created
 *  03/07/2011  Daniel Cummins - do not log dead positions.
 *  02/28/2012  Fill in journal entry in place. Dave Agans
 *  06/25/2012 - Use dynamically allocated buffers. Dave Agans
 ****************************************************************/
fbe_status_t fbe_parity_journal_build_data(fbe_raid_siots_t *siots_p)
{
    fbe_status_t status;
    fbe_u32_t slot_ID;
    fbe_parity_write_log_slot_t *slot_p;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_queue_head_t *fruts_queue;
    fbe_raid_fruts_t *current_fruts_p = NULL;
    fbe_parity_write_log_header_t * write_log_header_p;
    fbe_u8_t header_buff[FBE_PARITY_WRITE_LOG_HEADER_SIZE];
    fbe_parity_write_log_header_t * header_buff_p = (fbe_parity_write_log_header_t *)header_buff;
    csx_precise_time_t precise_time;
    fbe_lba_t parity_start;
    fbe_block_count_t parity_count;

    /* Get the write log slot from the siots.
     */
    fbe_raid_siots_get_journal_slot_id(siots_p, &slot_ID);
    if (slot_ID == FBE_RAID_INVALID_JOURNAL_SLOT)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the geometry from the siots, and the slot from the geometry and slot ID */
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    slot_p = &raid_geometry_p->raid_type_specific.journal_info.write_log_info_p->slot_array[slot_ID];

    /* First clear out the common header buffer. */
    fbe_zero_memory(&header_buff, FBE_PARITY_WRITE_LOG_HEADER_SIZE);

    /* Set header's version */
    header_buff_p->header_version = FBE_PARITY_WRITE_LOG_CURRENT_HEADER_VERSION;

    /* Set header's state */
    header_buff_p->header_state = FBE_PARITY_WRITE_LOG_HEADER_STATE_VALID;

    /* Set the log_entry_timestamp to the current precise time */
    csx_p_get_precise_time_info(&precise_time);
    header_buff_p->header_timestamp.sec = precise_time.tv_sec;
    header_buff_p->header_timestamp.usec = precise_time.tv_usec;

    /* siots_p->start_lba is logical lba where the IO begins */
    header_buff_p->start_lba = siots_p->start_lba;

    /* Set number of data blocks involved in the write operation */
    header_buff_p->xfer_cnt = (fbe_u16_t) siots_p->xfer_count;

    parity_start = siots_p->parity_start;
    parity_count = siots_p->parity_count;
    fbe_raid_geometry_align_io(raid_geometry_p, &parity_start, &parity_count);

    /* siots_p->parity_start is physical lba where the IO begins */
    header_buff_p->parity_start = parity_start;

    /* Set number of parity blocks involved in the write operation */
    header_buff_p->parity_cnt = (fbe_u16_t) parity_count;

    /* Loop through all the fruts's in the chain and build
     * the journal data.
     */
    fbe_raid_siots_get_write_fruts_head(siots_p, &fruts_queue);
    current_fruts_p = (fbe_raid_fruts_t *)fbe_queue_front(fruts_queue);
    while (current_fruts_p != NULL)
    {
        /* Set drive specific info: offset, blk count, write_bitmap */
        if ((current_fruts_p->position != siots_p->dead_pos) &&
            (current_fruts_p->position != siots_p->dead_pos_2))
        {
            header_buff_p->disk_info[current_fruts_p->position].offset = (fbe_u16_t) (current_fruts_p->lba - parity_start);
            header_buff_p->disk_info[current_fruts_p->position].block_cnt = (fbe_u16_t) current_fruts_p->blocks;
            header_buff_p->write_bitmap |= ((fbe_u16_t)0x1 << current_fruts_p->position);
        }

        /* Advance to the next fruts.
         */
        current_fruts_p = (fbe_raid_fruts_t *)fbe_queue_next(fruts_queue, (fbe_queue_element_t*)current_fruts_p);       
    }

    /* Loop through all the fruts and set the now-complete header information
     */
    current_fruts_p = (fbe_raid_fruts_t *)fbe_queue_front(fruts_queue);
    while (current_fruts_p != NULL)
    {
        status = fbe_parity_get_fruts_write_log_header(current_fruts_p, &write_log_header_p);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        fbe_copy_memory(write_log_header_p, &header_buff, FBE_PARITY_WRITE_LOG_HEADER_SIZE);

        fbe_raid_siots_object_trace(siots_p,
                                    FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                    "wr_log wr: s_id 0x%x, pos: 0x%x, hdr st: %d, lba 0x%llx cnt 0x%x ts: 0x%llx 0x%llx wr_bm 0x%x \n",
                                    siots_p->journal_slot_id, current_fruts_p->position, write_log_header_p->header_state,
                                    write_log_header_p->parity_start, write_log_header_p->parity_cnt, write_log_header_p->header_timestamp.sec,
									write_log_header_p->header_timestamp.usec, write_log_header_p->write_bitmap);

        /* Advance to the next fruts.
         */
        current_fruts_p = (fbe_raid_fruts_t *)fbe_queue_next(fruts_queue, (fbe_queue_element_t*)current_fruts_p);       
    }

    return FBE_STATUS_OK;
} /* End fbe_parity_journal_build_data() */


/*************************************************************
 * fbe_parity_prepare_journal_write
 *************************************************************
 * @brief
 * For each fruts in the write chain, calculate the lba for doing journal write.
 * Also prepare write_log slot headers by doing following three things:
 * 1. Checksum of checksums of all fruts blocks is calculated and set in the header.
 * 2. Slot header is pre-pended to fruts SGlist.
 * 3. Checksum and Lba stamp are calculated on the slot header.
 * Fields are duplicated across all headers for redundancy purposes.
 *
 *  @param siots_p, A ptr to the current SUB_IOTS prepare for a journal write.
 *
 * @return:
 *  fbe_status_t - FBE_STATUS_OK - success
 *
 * @author
 *  7/26/2010 - Created. Kevin Schleicher
 *  03/08/2012 - Vamsi V. Rewrote to prepare slot headers. 
 *  06/25/2012 - Use dynamically allocated buffers. Dave Agans
 * 
 *************************************************************/
fbe_status_t fbe_parity_prepare_journal_write(fbe_raid_siots_t *const siots_p)
{
    fbe_status_t status;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_lba_t journal_log_start_lba;
    fbe_u32_t journal_log_slot_size;
    fbe_u32_t journal_log_hdr_blocks;
    fbe_raid_fru_info_t write_info;
    fbe_raid_fru_info_t read_info;
    fbe_block_count_t blk_cnt[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH] = {0};
    fbe_u32_t slot_id;

    /* Retrieve the journal log offset, and the slot size;
     */
    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_log_start_lba); 
    fbe_raid_geometry_get_journal_log_slot_size(raid_geometry_p, &journal_log_slot_size);
    fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks);

    /* Get the write log slot from the siots. */
    fbe_raid_siots_get_journal_slot_id(siots_p, &slot_id);
    if (slot_id == FBE_RAID_INVALID_JOURNAL_SLOT)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log: slot_id in siots is invalid %d \n",
                                            slot_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    /* Calculate csum_of_csums for the data blocks and set it in each slot header
     */
    status = fbe_parity_write_log_calc_csum_of_csums(siots_p, write_fruts_p, FBE_TRUE, NULL);

    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log prepare journal write: Invalid calc_csum_of_csums status %d",
                                            status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set up the write fruts chain to prepend write slot header */
    fbe_raid_fruts_chain_set_prepend_header(&siots_p->write_fruts_head, FBE_TRUE);

    while (write_fruts_p != NULL)
    {
        if ((write_fruts_p->position != siots_p->dead_pos) &&
            (write_fruts_p->position != siots_p->dead_pos_2))
        {
            /* Calculate the lba for the journal write given the slot. */
            write_fruts_p->lba = journal_log_start_lba + (slot_id * journal_log_slot_size);

            /* Increment the Fruts block count to account for header write */
            write_fruts_p->blocks += journal_log_hdr_blocks;

            /* Save away write_fruts blk_cnt and change it to one block; because we want XOR lib to
             * calculate stamps on only slot header and not fruts blocks following it.
             */
            blk_cnt[write_fruts_p->position] = write_fruts_p->blocks; 
            write_fruts_p->blocks = journal_log_hdr_blocks; /* slot header block */
        }
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    } /* for each fru. */

    /* Generate checksum and lba stamps on header buffers. 
     * Write stamp and Time stamp will be set to zero */
    (void) fbe_raid_xor_write_log_header_set_stamps(siots_p, FBE_FALSE);

    /* Reset the pointer to start of fruts chain */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    while (write_fruts_p != NULL)
    {
        if ((write_fruts_p->position != siots_p->dead_pos) &&
            (write_fruts_p->position != siots_p->dead_pos_2))
        {
            write_fruts_p->blocks = blk_cnt[write_fruts_p->position];
              
            /* Reset up the preread info for the journal write. Load the read_info with the pre-read values for
             * this write.
             */
            write_info.lba = write_fruts_p->lba;
            write_info.blocks = write_fruts_p->blocks;
            fbe_raid_siots_setup_preread_fru_info(siots_p, &read_info, &write_info, write_fruts_p->position);
            write_fruts_p->write_preread_desc.lba = read_info.lba;
            write_fruts_p->write_preread_desc.block_count = read_info.blocks;

            /* Set the pre-read to be zeroes since there is no data to be preserved in the write
             * journal slots.
             */
            fbe_raid_memory_get_zero_sg(&(write_fruts_p->write_preread_desc.sg_list));
        }

        /* Advance to next fruts.
         */
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    }

    return FBE_STATUS_OK;
}
/* end fbe_parity_prepare_journal_write */

/*************************************************************
 * fbe_parity_restore_journal_write
 *************************************************************
 * @brief
 *  Restore the lba's for each write fruts so 
 *  the normal write may proceed.
 *
 * @param siots_p, A ptr to the current SUB_IOTS restore from a journal write.
 *
 * @return fbe_status_t - FBE_STATUS_OK - success
 *
 * @author
 *  7/26/2010 - Created. Kevin Schleicher
 * 
 *************************************************************/
fbe_status_t fbe_parity_restore_journal_write(fbe_raid_siots_t *const siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_parity_write_log_header_t * slot_header_p = NULL;
    fbe_u32_t journal_log_hdr_blocks;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks);

    while (write_fruts_ptr != NULL)
    {
        if ((write_fruts_ptr->position != siots_p->dead_pos) &&
            (write_fruts_ptr->position != siots_p->dead_pos_2))
        {
            fbe_raid_fru_info_t write_info;
            fbe_raid_fru_info_t read_info;

            /* Remove slot header from blk_cnt (beacuse we are now writing to the live stripe) */
            write_fruts_ptr->blocks -= journal_log_hdr_blocks;

            /* Get pointer to header */
            status = fbe_parity_get_fruts_write_log_header(write_fruts_ptr, &slot_header_p);
            if (status != FBE_STATUS_OK)
            {
                return status;
            }

            /* validate the block count for this position */
            if (fbe_parity_write_log_get_slot_hdr_blk_cnt(slot_header_p, write_fruts_ptr->position) != write_fruts_ptr->blocks)
            {
                /* if we get here it means the fruts block count does not match blk_cnt in header
                 * and ultimately the data described by the sg entry.
                 */
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "write_log_flush: write_log cache blk_cnt %d does not "
                                                    "match current fruts block count %d for position %d\n",
                                                    fbe_parity_write_log_get_slot_hdr_blk_cnt(slot_header_p, write_fruts_ptr->position),
                                                    (int)write_fruts_ptr->blocks,
                                                    write_fruts_ptr->position);

                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Calculate the original lba for the write.
             */
            write_fruts_ptr->lba = slot_header_p->parity_start + fbe_parity_write_log_get_slot_hdr_offset(slot_header_p, write_fruts_ptr->position); 

            /* Restore the preread info for the data write. Load the read_info with the pre-read values for
             * this write.
             */
            write_info.lba = write_fruts_ptr->lba;
            write_info.blocks = write_fruts_ptr->blocks;
            fbe_raid_siots_setup_preread_fru_info(siots_p, &read_info, &write_info, write_fruts_ptr->position);
            write_fruts_ptr->write_preread_desc.lba = read_info.lba;
            write_fruts_ptr->write_preread_desc.block_count = read_info.blocks;

            /* Restore the sg back to the corresponding read fruts sg.
             * Skip if there is not read fruts.
             */
            if (read_fruts_ptr != NULL)
            {
                write_fruts_ptr->write_preread_desc.sg_list = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);
            }

            /* Reset the fruts to prepare for the write to the original lba.
             */
            (void)fbe_raid_fruts_reset_wr(write_fruts_ptr, 0,(fbe_u32_t *)(fbe_ptrhld_t) write_fruts_ptr->blocks);
        }
        /* Advance to the next fruts.
         */
        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    /* Write to journal slot is done; reset the write fruts chain to skip the slot header
     */
    fbe_raid_fruts_chain_set_prepend_header(&siots_p->write_fruts_head, FBE_FALSE);

    return status;
}
/* end fbe_parity_restore_journal_write */

/* Utility function to convert from journal queue element to Siots. 
*/
__forceinline static fbe_raid_siots_t * fbe_parity_journal_q_elem_to_siots(fbe_queue_element_t * queue_element)
{
    return (fbe_raid_siots_t *)((fbe_u8_t *)queue_element - (fbe_u8_t *)(&((fbe_raid_siots_t *)0)->journal_q_elem));
}

/*!***************************************************************
 * fbe_parity_write_log_allocate_slot()
 *****************************************************************
 * @brief
 *  This function is called allocate a free journal slot. If slot
 *  is not currently available, request is queued. Need to support
 *  abort handling. 
 *
 * @return fbe_status_t
 *  Slot number that is allocated to this request. Invalid slot
 * number indicates, currently no slots are available and request
 * is queued.
 *
 * @author
 *  02/22/2012 - Created. Vamsi V
 *  24feb12 - Modified to do allocation. Dave Agans
 *
 ****************************************************************/
fbe_status_t fbe_parity_write_log_allocate_slot(fbe_raid_siots_t *siots_p)
{
    fbe_u32_t slot_start, slot_end, slot_idx;
    fbe_cmi_sp_id_t this_sp_id = FBE_CMI_SP_ID_INVALID;
    fbe_raid_geometry_t *geo_p = fbe_raid_siots_get_raid_geometry(siots_p); 
    fbe_parity_write_log_info_t *write_log_info_p = geo_p->raid_type_specific.journal_info.write_log_info_p;
    fbe_cmi_get_sp_id(&this_sp_id);

    /* Slots are evenly divided between SPA and SPB 
    *  Determine slots belonging to this SP.
     */
    if (this_sp_id == FBE_CMI_SP_ID_A)
    {
        slot_start = 0;
        slot_end = write_log_info_p->slot_count/2;
    }
    else
    {
        slot_start = write_log_info_p->slot_count/2;
        slot_end = write_log_info_p->slot_count;
    }

    /* Acquire spinlock and loop over all slots belonging to this SP */
    fbe_spinlock_lock(&write_log_info_p->spinlock);

    /* If log is quiesced, just return an invalid slot */
    if (fbe_parity_write_log_is_flag_set(write_log_info_p, FBE_PARITY_WRITE_LOG_FLAGS_QUIESCE))
    {
        siots_p->journal_slot_id = FBE_PARITY_WRITE_LOG_INVALID_SLOT;
        fbe_spinlock_unlock(&write_log_info_p->spinlock);
        return FBE_STATUS_OK;
    }

    for (slot_idx = slot_start; slot_idx < slot_end; slot_idx++)
    {
        if (fbe_parity_write_log_get_slot_state(write_log_info_p, slot_idx) == FBE_PARITY_WRITE_LOG_SLOT_STATE_FREE)
        {
            /* Found a free slot. Assign it to Siots and return */
            fbe_parity_write_log_set_slot_state(write_log_info_p, slot_idx, FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED);
            siots_p->journal_slot_id = slot_idx;
            fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_JOURNAL_SLOT_ALLOCATED);
            fbe_spinlock_unlock(&write_log_info_p->spinlock);
            return FBE_STATUS_OK;
        }
    }

    /* No free slot currently available. Push Siots onto waiting queue */
    siots_p->journal_slot_id = FBE_PARITY_WRITE_LOG_INVALID_SLOT;
    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WAIT_WRITE_LOG_SLOT);
    fbe_queue_push(&write_log_info_p->request_queue_head, &siots_p->journal_q_elem);     

    /* Release spinlock and return */
    fbe_spinlock_unlock(&write_log_info_p->spinlock);
    return FBE_STATUS_PENDING;
}

/*!***************************************************************
 * fbe_parity_write_log_release_slot()
 ****************************************************************
 * @brief
 *  This function is called to release a journal slot. If any requests
 *  are pending for journal slots, initiate them with this slot.    
 *
 * @return Void
 *
 * @author
 *  02/22/2012 - Created. Vamsi V
 *
 ****************************************************************/
fbe_status_t fbe_parity_write_log_release_slot(fbe_raid_siots_t *siots_p,
                                               const char *function,
                                               fbe_u32_t line)
{
    fbe_raid_geometry_t *geo_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_parity_write_log_info_t *write_log_info_p = geo_p->raid_type_specific.journal_info.write_log_info_p;
    fbe_queue_element_t * siots_q_element;
    fbe_raid_siots_t * pending_siots_p;
    fbe_packet_t *packet_p;
    fbe_raid_fruts_t *fruts_p;

    if (siots_p->journal_slot_id == FBE_RAID_INVALID_JOURNAL_SLOT)
    {
        /* Nothing to do */
        return FBE_STATUS_OK;
    }
    else if (siots_p->journal_slot_id >= write_log_info_p->slot_count)
    {
        fbe_raid_siots_object_trace(siots_p,
                                    FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "%s line %d garbage slot_id, siots %p, siots_p->flags 0x%x\n",
                                    function, line, siots_p, siots_p->flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* acquire spinlock */
    fbe_spinlock_lock(&write_log_info_p->spinlock);

    /* Sanity check */
    if ( !(   (FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED == fbe_parity_write_log_get_slot_state(write_log_info_p, siots_p->journal_slot_id))
              || (FBE_PARITY_WRITE_LOG_SLOT_STATE_FLUSHING  == fbe_parity_write_log_get_slot_state(write_log_info_p, siots_p->journal_slot_id))))
    {
        /* Release spinlock before starting the siots */
        fbe_spinlock_unlock(&write_log_info_p->spinlock);
        fbe_raid_siots_object_trace(siots_p,
                                    FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "%s line %d releasing unused slot, siots %p, siots_p->flags 0x%x\n",
                                    function, line, siots_p, siots_p->flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check for any pending siots -- if the log is quiesced, there should not be any! */
    siots_q_element = fbe_queue_pop(&write_log_info_p->request_queue_head);

    if (siots_q_element)
    {
        /* Allocate slot to pending Siots */
        pending_siots_p = fbe_parity_journal_q_elem_to_siots(siots_q_element);
        fbe_parity_write_log_set_slot_state(write_log_info_p, siots_p->journal_slot_id, FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED);
        fbe_parity_write_log_set_slot_inv_state(write_log_info_p, siots_p->journal_slot_id, FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_SUCCESS);
        pending_siots_p->journal_slot_id = siots_p->journal_slot_id;
        fbe_raid_siots_set_flag(pending_siots_p, FBE_RAID_SIOTS_FLAG_JOURNAL_SLOT_ALLOCATED);
        fbe_raid_siots_clear_flag(pending_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_WRITE_LOG_SLOT);

        /* Clear slot info from the siots */
        siots_p->journal_slot_id = FBE_PARITY_WRITE_LOG_INVALID_SLOT;
        fbe_raid_siots_clear_flag(siots_p, FBE_RAID_SIOTS_FLAG_JOURNAL_SLOT_ALLOCATED);

        /* Release spinlock before starting the siots */
        fbe_spinlock_unlock(&write_log_info_p->spinlock);

        /* Get the first write fruts and the packet associated with it,
         * and use that to requeue the pending siots to the correct core.
         */
        fbe_raid_siots_get_write_fruts(pending_siots_p, &fruts_p);
        packet_p = fbe_raid_fruts_get_packet(fruts_p);
        fbe_transport_run_queue_push_packet(packet_p, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
    }
    else
    {
        /* Mark slot as free */
        fbe_parity_write_log_set_slot_state(write_log_info_p, siots_p->journal_slot_id, FBE_PARITY_WRITE_LOG_SLOT_STATE_FREE);
        fbe_parity_write_log_set_slot_inv_state(write_log_info_p, siots_p->journal_slot_id, FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_SUCCESS);

        /* Clear slot info from the siots */
        siots_p->journal_slot_id = FBE_PARITY_WRITE_LOG_INVALID_SLOT;
        fbe_raid_siots_clear_flag(siots_p, FBE_RAID_SIOTS_FLAG_JOURNAL_SLOT_ALLOCATED);

        /* Release spinlock */
        fbe_spinlock_unlock(&write_log_info_p->spinlock);
    }
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_parity_write_log_set_slot_invalidate_state()
 ****************************************************************
 * @brief
 *  This function is called to set the invalidate state of a slot, which indicates why the
 *  slot is being invalidated and what state to go to next from the common invalidation state
 *  
 * @param siots_p - ptr to the current SUB_IOTS owning the slot to set the invalidate state of
 *        state - invalidate state to set
 *        function and line for trace diagnostics
 * 
 * @return FBE_OKAY if good or invalid journal slot
 *         FBE_STATUS_GENERIC_FAILURE if slot_id was out of range or slot was not allocated
 *
 * @author
 *  24 Apr 12 - Created. Dave Agans
 *
 ****************************************************************/
fbe_status_t fbe_parity_write_log_set_slot_invalidate_state(fbe_raid_siots_t *siots_p,
                                                            fbe_parity_write_log_slot_invalidate_state_t state,
                                                            const char *function,
                                                            fbe_u32_t line)
{
    fbe_raid_geometry_t *geo_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_parity_write_log_info_t *write_log_info_p = geo_p->raid_type_specific.journal_info.write_log_info_p;

    if (siots_p->journal_slot_id == FBE_RAID_INVALID_JOURNAL_SLOT)
    {
        /* Nothing to do */
        return FBE_STATUS_OK;
    }
    else if (siots_p->journal_slot_id >= write_log_info_p->slot_count)
    {
        fbe_raid_siots_object_trace(siots_p,
                                    FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "%s line %d garbage slot_id, siots %p, siots_p->flags 0x%x\n",
                                    function, line, siots_p, siots_p->flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Sanity check */
    if ( !(   (FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED == fbe_parity_write_log_get_slot_state(write_log_info_p, siots_p->journal_slot_id))
              || (FBE_PARITY_WRITE_LOG_SLOT_STATE_FLUSHING  == fbe_parity_write_log_get_slot_state(write_log_info_p, siots_p->journal_slot_id))))
    {
        fbe_raid_siots_object_trace(siots_p,
                                    FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "%s line %d releasing unused slot, siots %p, siots_p->flags 0x%x\n",
                                    function, line, siots_p, siots_p->flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_parity_write_log_set_slot_inv_state(write_log_info_p, siots_p->journal_slot_id, state);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_parity_write_log_get_slot_invalidate_state()
 ****************************************************************
 * @brief
 *  This function is called to get the invalidate state of a slot, which indicates why the
 *  slot is being invalidated and what state to go to next from the common invalidation state
 *  
 * @param siots_p - ptr to the current SUB_IOTS owning the slot to set the invalidate state of
 *        state - pointer to invalidate state which will be set as a side effect
 *        function and line for trace diagnostics
 * 
 * @return FBE_OKAY if good or invalid journal slot
 *         FBE_STATUS_GENERIC_FAILURE if slot_id was out of range or slot was not allocated
 *
 * @author
 *  24 Apr 12 - Created. Dave Agans
 *
 ****************************************************************/
fbe_status_t fbe_parity_write_log_get_slot_invalidate_state(fbe_raid_siots_t *siots_p,
                                                            fbe_parity_write_log_slot_invalidate_state_t *state,
                                                            const char *function,
                                                            fbe_u32_t line)
{
    fbe_raid_geometry_t *geo_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_parity_write_log_info_t *write_log_info_p = geo_p->raid_type_specific.journal_info.write_log_info_p;

    /* Preset the target with something coherent in case we bail out
     */
    *state = FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_SUCCESS;

    if (siots_p->journal_slot_id == FBE_RAID_INVALID_JOURNAL_SLOT)
    {
        /* Nothing to do */
        return FBE_STATUS_OK;
    }
    else if (siots_p->journal_slot_id >= write_log_info_p->slot_count)
    {
        fbe_raid_siots_object_trace(siots_p,
                                    FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "%s line %d garbage slot_id, siots 0x%x, siots_p->flags 0x%x\n",
                                    function, line, (unsigned int)siots_p, siots_p->flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Sanity check */
    if ( !(   (FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED == fbe_parity_write_log_get_slot_state(write_log_info_p, siots_p->journal_slot_id))
              || (FBE_PARITY_WRITE_LOG_SLOT_STATE_FLUSHING  == fbe_parity_write_log_get_slot_state(write_log_info_p, siots_p->journal_slot_id))))
    {
        fbe_raid_siots_object_trace(siots_p,
                                    FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "%s line %d releasing unused slot, siots 0x%x, siots_p->flags 0x%x\n",
                                    function, line, (unsigned int)siots_p, siots_p->flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    *state = fbe_parity_write_log_get_slot_inv_state(write_log_info_p, siots_p->journal_slot_id);

    return FBE_STATUS_OK;
}


/*!***************************************************************
 * fbe_parity_write_log_quiesce()
 *****************************************************************
 * @brief
 *  This function is called to remove all items from the write log
 *  pending queue.  When they are checked for quiesced, the WAIT_LOG
 *  siots flag will indicated quiesced; when restarted, the invalid
 *  slot will indicate it used to be on this queue and thus will
 *  be restarted from the previous state to ask for a slot again.
 *
 * @param write_log_info_p - pointer to in memory write_log struc 
 *
 * @return None
 *
 * @author
 *  11 Apr 2012 - Created. Dave Agans
 *
 ****************************************************************/
void fbe_parity_write_log_quiesce(fbe_parity_write_log_info_t * write_log_info_p)
{
    /* Acquire spinlock and delete all items from queue */
    fbe_spinlock_lock(&write_log_info_p->spinlock);

    if (!fbe_parity_write_log_is_flag_set(write_log_info_p, FBE_PARITY_WRITE_LOG_FLAGS_QUIESCE))
    {
        fbe_queue_element_t * queue_element;
        fbe_u32_t cnt = 0;

        /* Set quiesced flag, must be done under lock! */
        fbe_parity_write_log_set_flag(write_log_info_p, FBE_PARITY_WRITE_LOG_FLAGS_QUIESCE);

        /* For every queue element simply set quiesced to transition siots to a waiting state. 
         */
        queue_element = fbe_queue_pop(&write_log_info_p->request_queue_head);
        while (queue_element)
        {
            fbe_raid_siots_t *siots_p = fbe_parity_journal_q_elem_to_siots(queue_element);
            fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_QUIESCED);
            queue_element = fbe_queue_pop(&write_log_info_p->request_queue_head);
            cnt++;
        }

        fbe_raid_library_trace_basic(FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                         "%s num siots quiesced %d\n",
                                         __FUNCTION__, cnt);
    }

    /* Release spinlock and return */
    fbe_spinlock_unlock(&write_log_info_p->spinlock);

    return;
}
/* end fbe_parity_write_log_quiesce */

/*!***************************************************************
 * fbe_parity_write_log_unquiesce()
 *****************************************************************
 * @brief
 *  This function is called to unquiesce the write log, allowing it
 *  to accept more requests for slots.
 *
 * @param write_log_info_p - pointer to in memory write_log struc 
 *
 * @return None
 *
 * @author
 *  11 Apr 2012 - Created. Dave Agans
 *
 ****************************************************************/
void fbe_parity_write_log_unquiesce(fbe_parity_write_log_info_t * write_log_info_p)
{

    /* Acquire spinlock and delete all items from queue */
    fbe_spinlock_lock(&write_log_info_p->spinlock);

    /* Set quiesced flag, must be done under lock! */
    fbe_parity_write_log_clear_flag(write_log_info_p, FBE_PARITY_WRITE_LOG_FLAGS_QUIESCE);

    /* Release spinlock and return */
    fbe_spinlock_unlock(&write_log_info_p->spinlock);

    return;
}
/* end fbe_parity_write_log_unquiesce */

/*!***************************************************************
 * fbe_parity_write_log_abort()
 *****************************************************************
 * @brief
 *  This function is called to remove all items from the write log
 *  pending queue.  When they are checked for quiesced, the WAIT_LOG
 *  siots flag will indicated quiesced.
 *
 * @param write_log_info_p - pointer to in memory write_log struc 
 *
 * @return None
 *
 * @author
 *  11 Apr 2013 - Created. Jibing Dong
 *
 ****************************************************************/
void fbe_parity_write_log_abort(fbe_parity_write_log_info_t * write_log_info_p)
{
    fbe_queue_element_t * queue_element;
    fbe_u32_t cnt = 0;

    /* Acquire spinlock and delete all items from queue */
    fbe_spinlock_lock(&write_log_info_p->spinlock);

    /* For every queue element simply set quiesced to transition siots to a waiting state. 
    */
    queue_element = fbe_queue_pop(&write_log_info_p->request_queue_head);
    while (queue_element)
    {
        fbe_raid_siots_t *siots_p = fbe_parity_journal_q_elem_to_siots(queue_element);
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_QUIESCED);
        queue_element = fbe_queue_pop(&write_log_info_p->request_queue_head);
        cnt++;
    }

    if (cnt > 0)
    {
        fbe_raid_library_trace_basic(FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                     "%s num siots aborted %d\n",
                                     __FUNCTION__, cnt);
    }

    /* Release spinlock and return */
    fbe_spinlock_unlock(&write_log_info_p->spinlock);

    return;
}
/* end fbe_parity_write_log_abort */

/*!***************************************************************
 * fbe_parity_is_write_logging_required()
 ****************************************************************
 * @brief
 * Function checks if this siots has to do write_logging.    
 *
 * @return FBE_TRUE if write_logging required
 *         FBE_FALSE otherwise
 *
 * @author
 *  05/21/2012 - Created. Vamsi V
 *  03/22/2012 - Modified to skip logging when possible 
 ****************************************************************/
fbe_bool_t fbe_parity_is_write_logging_required(fbe_raid_siots_t *siots_p)
{
    fbe_raid_geometry_t *geo_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_u32_t width;
    fbe_bool_t  b_aligned;
    fbe_u16_t parity_count;
    fbe_u16_t dead_parity_count = 0;

    /* Disable write_logging for metadata writes as paged metadata can be regenerated */
    if (fbe_raid_siots_is_metadata_operation(siots_p))
    {
        return FBE_FALSE;
    }

    /* Check if write_logging is enabled for this RG */
    if (!fbe_raid_geometry_is_write_logging_enabled(geo_p))
    {
        return FBE_FALSE;
    }

    /* If this is a non-degraded write, no need to log
     * Note, typically this function is not called if non-degraded,
     * this is included to future-proof in case someone adds a more general call
     */
    if (siots_p->dead_pos == FBE_RAID_INVALID_DEAD_POS)
    {
        return FBE_FALSE;
    }

    /* If debug flag to disable skip optimizations is set, don't test for skip cases below here.
     */
    if (fbe_raid_geometry_is_debug_flag_set(geo_p, FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_WRITE_LOG_SKIP))
    {
        return FBE_TRUE;
    }

    /* *** Note that all of the optimizations below will create uncorrectables in the "torn write" case,
     *       where the drive does not fully write the sector and CRC, leaving a bad sector on disk.
     *     However, in all these cases, the data was about to be overwritten, so a host or cache can recover
     *       since the io will not be acknowledged complete.
     *     This is not as good as full write logging, which would recover the torn write,
     *       but better than parity shedding, which could lose data that was not involved in the io.
     *     This is a problem if cache writes backfill data that is not mirrored, since
     *       the lost data may not be part of the original host io, and the cache cannot recover it.
     *       This would be considered a cache bug, but could be prevented if write logging were not optimized.
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    b_aligned = fbe_raid_geometry_is_stripe_aligned(geo_p, siots_p->start_lba, siots_p->xfer_count);

    /* If this is a cached write (or bva write) and the originating packet is fully stripe aligned, 
     * then this does not need write logging since the cache is buffering the data.
     * Note, this requires the cache to mirror backfill sectors so they will be reapplied after
     * an interrupted write without needing to re-read from disk (which might not be consistent any more.)
     */
    if (((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)        ||
         (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE)    ) && 
        (b_aligned == FBE_TRUE)                                              )
    {
        return FBE_FALSE;
    }

    /* If this is a non-cached write and the originating packet is fully stripe aligned, 
     * then this does not need write logging since the host can always reapply the write if it doesn't complete
     * *** Note that this can cause uncorrectable errors in the log on recovery from partial writes,
     *     this was considered a reasonable price to speed high-bandwidth write-through, since
     *     no old data is actually lost, and new data can be reapplied.
     */
    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED) &&
        (b_aligned == FBE_TRUE)                                           ) 
    {
        return FBE_FALSE;
    }

    /* During the encryption incomplete write invalidate we might issue an I/O degraded. 
     * We expect this I/O will not journal since we are invalidating and this will get 
     * replayed by the monitor in case of an incomplete write.. 
     */
    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE) &&
        (b_aligned == FBE_TRUE) ) 
    {
        return FBE_FALSE;
    }

    /*! @todo temporarily we will not journal on unaligned RGs.
     */
    if ( 0 && fbe_raid_geometry_needs_alignment(geo_p)){
        return FBE_FALSE;
    }
    /* See how many parities are dead 
     */
    width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_geometry_get_parity_disks(geo_p, &parity_count);

    if (   (siots_p->parity_pos == siots_p->dead_pos)
        || (siots_p->parity_pos == siots_p->dead_pos_2))
    {
        dead_parity_count++;
    }
    if (   (parity_count == 2)
        && (   (((siots_p->parity_pos + 1) % width) == siots_p->dead_pos)
            || (((siots_p->parity_pos + 1) % width) == siots_p->dead_pos_2)))
    {
        dead_parity_count++;
    }

    /* If all and only parity positions are dead, there is no need to log, since the array
     * is essentially RAID0 and incomplete write will not create inconsistent stripe;
     * write will only affect data intended to be written, leaving old or new data.
     */
    if (parity_count == dead_parity_count)
    {
        return FBE_FALSE;
    }

    /* If this is a unaligned MR3 we need to update the stamps 
     * (time stamp valid for all positions) on all positions.  Therefore we
     * must journal.
     */
    if ((b_aligned == FBE_FALSE)       &&
        (siots_p->algorithm == R5_MR3)    )
    {
        /* We must journal since multiple positions are being written.
         */
        return FBE_TRUE;
    }

    /* If raid is only accessing a single disk (only one fruts), then there can be no incomplete write
     * This happens when we write a dead position only, and there is no second parity available.
     * In raid 3 and 5, the fruts goes to the 1 parity drive
     * In raid 6, the fruts goes to the 1 remaining parity drive
     */
    if (   (fbe_raid_geometry_is_single_position(geo_p, siots_p->start_lba, siots_p->xfer_count))
        && (   (siots_p->start_pos == siots_p->dead_pos)    /* lba is on dead position */
            || (siots_p->start_pos == siots_p->dead_pos_2))
        && (dead_parity_count == (parity_count - 1)))       /* only one parity position not dead */
    {
        return FBE_FALSE;
    }

    return FBE_TRUE;
}
/* end fbe_parity_is_write_logging_required */

/*!****************************************************************************
 * fbe_parity_journal_flush_get_fru_info()
 ******************************************************************************
 * @brief This function populates the fru info structs needed to read from the
 * journal area and to preread the live stripe if the ultimate flush will be
 * unaligned. 
 *
 * @param siots_p - ptr to sub iots struct containing sep payload with valid
 *                  journal slot metadta in its metadata payload.
 * @param fru_info_size - the size of the fru_info array.  This is used to
 *  				zero the fru_info array.
 * @param write_fru_info_p - the fru_info array we will populate with journal 
 *                  read info.  This source of the read will be the journal 
 *                  area in the storage extent metadata.
 * @param read_fru_info_p - the fru_info array we will populate preread info.
 *                  The area to preread is the live stripe.
 * @param read_fru_info_count_p - ptr to the number of preread fru info structs
 *                  that have been initialized by this function. If 0 is returned
 *                  then the journal writes to the live stripe are aligned.
 * @param write_blocks_p - returns the amount of buffer space needed to read
 *                  the journal data in units of backend sectors.
 * @param read_blocks_p - returns the amount of buffer space needed to preread
 *                  the live stripe for an unaligned journal flush to the live
 *                  stripe.
 * 
 * @return FBE_STATUS_OK - indicates fru_info_p, blocks_to_allocate_p, and
 *  					   fruts_to_allocate_p are all valid.
 * 
 * @return FBE_STATUS_GENERIC_FAILURE - slot header block count was 0.
 * 
 * @author
 *  1/24/2011 - Created. Daniel Cummins
 *  3/1/2012 - Vamsi. Modified to use slot Headers.
 *  6/20/2012 - Modified to use dynamic headers. Dave Agans
 *
 *****************************************************************************/
fbe_status_t
fbe_parity_journal_flush_get_fru_info(fbe_raid_siots_t    *siots_p,
                                      fbe_u32_t           fru_info_size,
                                      fbe_raid_fru_info_t *write_fru_info_p,
                                      fbe_raid_fru_info_t *read_fru_info_p,
                                      fbe_u32_t           *read_fru_info_count_p,
                                      fbe_block_count_t   *write_blocks_p,
                                      fbe_block_count_t   *read_blocks_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_lba_t write_log_start_lba;
    fbe_u32_t write_log_slot_size;
    fbe_u32_t journal_log_hdr_blocks;
    fbe_u32_t i;
    fbe_u32_t num_write_fruts = 0;
    fbe_lba_t lba;
    fbe_block_count_t blocks = 0;
    fbe_parity_write_log_header_t * header_p;
    fbe_status_t status;

    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &write_log_start_lba); 
    fbe_raid_geometry_get_journal_log_slot_size(raid_geometry_p, &write_log_slot_size);
    fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks); 

    *write_blocks_p = 0;
    *read_blocks_p = 0;
    *read_fru_info_count_p = 0;

    /* Initialize the fru info structs
     */
    fbe_raid_fru_info_init_arrays(fru_info_size,
                                  read_fru_info_p,
                                  write_fru_info_p,
                                  NULL);

    status = fbe_parity_write_log_get_iots_memory_header(siots_p, &header_p);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to find saved header with memory allocation status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status); 
    }

    /* Populate the write_log fru info
     */
    for (i = 0; i < raid_geometry_p->width; i++)
    {
        /* Only use positions that are in the write bitmap and are not degraded.
         */
        if ((header_p->write_bitmap & (0x1 << i)) &&
            ((siots_p->dead_pos == FBE_RAID_INVALID_DEAD_POS) ? 1 : (siots_p->dead_pos != i)) &&
            ((siots_p->dead_pos_2 == FBE_RAID_INVALID_DEAD_POS) ? 1 : (siots_p->dead_pos_2 != i)))
        {
            fbe_raid_write_journal_trace(siots_p,
                                         FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                         "slot %d, fru %d, original_lba 0x%llX, lba 0x%X, blocks %d\n",
                                         siots_p->journal_slot_id, 
                                         i, 
                                         header_p->parity_start, 
                                         header_p->disk_info[i].offset, 
                                         header_p->disk_info[i].block_cnt);

            /* sanity check the block count */
            if (header_p->disk_info[i].block_cnt == 0)
            {
                fbe_raid_write_journal_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                             "valid header with 0 block count: slot %d, fru %d\n",
                                             siots_p->journal_slot_id, i);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Lba from which to read write_log slot data. */
            /* Increment past the write_log slot header. */
            lba = write_log_start_lba + (siots_p->journal_slot_id * write_log_slot_size);           
            lba += journal_log_hdr_blocks;

            blocks = header_p->disk_info[i].block_cnt;
            *write_blocks_p += blocks;

            /* initialize the fru info
             */
            fbe_raid_fru_info_init(&write_fru_info_p[num_write_fruts++],
                                   lba,
                                   blocks,
                                   i);

            /* calculate the LBA we will use to write to the live area of this fru
             */ 
            lba = header_p->parity_start + header_p->disk_info[i].offset;

        }
    }

    return FBE_STATUS_OK;
}
/****************************************************************************** 
 * end fbe_parity_journal_flush_get_read_fru_info()
 *****************************************************************************/

/*!****************************************************************************
 * fbe_parity_journal_flush_setup_fruts()
 ******************************************************************************
 *
 * @brief
 *   This function is responsible for allocating and intializing the fruts for
 * the write and read portion of a journal flush operation.  The preread is
 * needed if the flush to the live stripe will be unaligned.
 *
 * @param siots_p - pointer to I/O.
 * @param write_fru_info_p - write fru info.
 * @param read_fru_info_p - read fru info.
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @notes
 *
 * @author
 *  1/24/2011 - Created. Daniel Cummins
 *  
 *****************************************************************************/
static fbe_status_t 
fbe_parity_journal_flush_setup_fruts(fbe_raid_siots_t * siots_p, 
									 fbe_raid_fru_info_t *write_fru_info_p,
									 fbe_raid_fru_info_t *read_fru_info_p,
									 fbe_raid_memory_info_t *memory_info_p)
{
	fbe_status_t status;

	/* allocate the write fruts from the write_fru_info chain
	 */
	while (write_fru_info_p->blocks)
	{
		/* use the write_fruts_head for journal read - we will modify the lba offset later to write
		 * to the live stripe.
		 */
		status = fbe_raid_setup_fruts_from_fru_info(siots_p,
													write_fru_info_p,
													&siots_p->write_fruts_head,
													FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
													memory_info_p);

		if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
		{
			fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
								 "journal flush: failed to setup write fruts: siots_p = 0x%p, "
								 "journal_fru_info_p = 0x%p, status = 0x%x\n",
								 siots_p, write_fru_info_p, status);
			return status;
		}

		write_fru_info_p++;
	}

	/* allocate the read fruts from the read_fru_info chain
	 */
	while (read_fru_info_p->blocks)
	{
		/* use read_fruts_head for live preread.
		 */
		status = fbe_raid_setup_fruts_from_fru_info(siots_p,
													read_fru_info_p,
													&siots_p->read_fruts_head,
													FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
													memory_info_p);

		if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
		{
			fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
								 "journal flush: failed to setup read fruts: siots_p = 0x%p, "
								 "read_fru_info_p = 0x%p, status = 0x%x\n",
								 siots_p, read_fru_info_p, status);
			return status;
		}
		read_fru_info_p++;
	}

	return FBE_STATUS_OK;
}
/*****************************************************************************
 * end fbe_parity_journal_flush_setup_fruts()
 *****************************************************************************/
/*!**************************************************************************
 * fbe_parity_journal_flush_setup_sgs()
 ***************************************************************************
 * @brief 
 *  This function is responsible for initializing the S/G
 *  lists allocated earlier 
 *
 *  This function is executed from the journal flush state
 *  machine once resources are allocated.
 *
 * @param siots_p - pointer to I/O.
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *  1/24/2011:  Created. Daniel Cummins
 *
 **************************************************************************/
static fbe_status_t 
fbe_parity_journal_flush_setup_sgs(fbe_raid_siots_t * siots_p,
								   fbe_raid_memory_info_t *memory_info_p)
{
	fbe_status_t status;
	fbe_raid_fruts_t *write_fruts_p = NULL;
	fbe_raid_fruts_t *read_fruts_p = NULL;
	fbe_sg_element_t *write_sgl_ptr = NULL;
	fbe_sg_element_t *read_sgl_ptr = NULL;

	fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
	fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

	if (RAID_COND(write_fruts_p == NULL))
	{
		fbe_raid_siots_trace(siots_p,
							 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
							 "journal_flush: no write fruts\n");
		return FBE_STATUS_GENERIC_FAILURE;
	}

	while (write_fruts_p != NULL)
	{
		if (RAID_COND(write_fruts_p->sg_id == 0))
		{
			fbe_raid_siots_trace(siots_p,
								 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
								 "journal_flush: write_fruts_p->sg_id == 0\n");
			return FBE_STATUS_GENERIC_FAILURE;
		}

		if (RAID_COND(write_fruts_p->blocks == 0))
		{
			fbe_raid_siots_trace(siots_p,
								 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
								 "journal_flush: write_fruts_p->blocks == 0\n");

			return FBE_STATUS_GENERIC_FAILURE;
		}

		/* allocate and initialize the write fruts scatter gather list
		 */
		write_sgl_ptr = fbe_raid_fruts_return_sg_ptr(write_fruts_p);

		fbe_raid_sg_populate_with_memory(memory_info_p,
										 write_sgl_ptr, 
										 (fbe_u32_t) (write_fruts_p->blocks * FBE_BE_BYTES_PER_BLOCK));

		/* allocate the read descriptor sgl and setup the write_fruts_p->write_preread_desc if it
		 * matches the position.
		 */
		if (read_fruts_p)
		{
			if (RAID_COND(read_fruts_p->sg_id == 0))
			{
				fbe_raid_siots_trace(siots_p,
									 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
									 "journal_flush: read_fruts_p->sg_id not valid\n");
				return FBE_STATUS_GENERIC_FAILURE;
			}

			if (RAID_COND(read_fruts_p->blocks == 0))
			{
				fbe_raid_siots_trace(siots_p,
									 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
									 "journal_flush: read_fruts_p->blocks == 0\n");

				return FBE_STATUS_GENERIC_FAILURE;
			}

			/* setup the write fruts preread descriptor.  This must be done in the same order as
			 * when we calculated the memory requirements for the sg descriptors.
			 */
			if (write_fruts_p->position == read_fruts_p->position)
			{
				/* allocate and initialize the read fruts scatter gather list
				 */
				read_sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_p);

				fbe_raid_sg_populate_with_memory(memory_info_p,
												 read_sgl_ptr, 
												 (fbe_u32_t) (read_fruts_p->blocks * FBE_BE_BYTES_PER_BLOCK));

				fbe_raid_fruts_set_flag(write_fruts_p, FBE_RAID_FRUTS_FLAG_UNALIGNED_WRITE);
				write_fruts_p->write_preread_desc.lba = read_fruts_p->lba;
				write_fruts_p->write_preread_desc.block_count = read_fruts_p->blocks;
				fbe_payload_pre_read_descriptor_set_sg_list(&write_fruts_p->write_preread_desc,
															read_sgl_ptr);

				read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
			}
		}

		write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
	}

	/* Make sure the fruts chains are sane.
	 */
	fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
	status = fbe_raid_fruts_for_each(FBE_RAID_FRU_POSITION_BITMASK, read_fruts_p, fbe_raid_fruts_validate_sgs, 0, 0);

	if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
	{
		fbe_raid_siots_trace(siots_p, 
							 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
							 "journal_flush: failed to validate sgs of read_fruts_p 0x%p "
							 "for siots_p 0x%p; failed status = 0x%x\n",
							 read_fruts_p, siots_p, status);
		return  status;
	}

	fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
	status = fbe_raid_fruts_for_each(FBE_RAID_FRU_POSITION_BITMASK, write_fruts_p, fbe_raid_fruts_validate_sgs, 0, 0);

	if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
	{
		fbe_raid_siots_trace(siots_p, 
							 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
							 "journal_flush: failed to validate sgs of write_fruts_p 0x%p "
							 "for siots_p 0x%p; failed status = 0x%x\n",
							 write_fruts_p, siots_p, status);
		return  status;
	}

	return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_parity_journal_flush_setup_sgs()
 *****************************************************************************/

/*!****************************************************************************
 * fbe_parity_journal_flush_setup_resources()
 ******************************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param siots_p - current I/O.
 * @param write_fru_info_p - journal flush fru info.
 * @param read_fru_info_p - preread fru info for flush.
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *  1/24/2011 - Created. Daniel Cummins
 *  3/1/2012 - Vamsi V. Modified to use Slot headers instead of metadata service
 *
 ******************************************************************************/
fbe_status_t 
fbe_parity_journal_flush_setup_resources(fbe_raid_siots_t * siots_p, 
										 fbe_raid_fru_info_t * write_fru_info_p,
										 fbe_raid_fru_info_t * read_fru_info_p)
{
	fbe_raid_memory_info_t  memory_info;
    fbe_raid_memory_info_t  data_memory_info;
	fbe_status_t            status;

	/* Initialize our memory request information
	 */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);

	if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
	{
		fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
							 "journal_flush: failed to init memory info: status = 0x%x, siots_p = 0x%p, "
							 "siots_p->start_lba = 0x%llx, siots_p->xfer_count = 0x%llx\n",
							 status, siots_p, siots_p->start_lba, siots_p->xfer_count);
		return(status); 
	}
    status = fbe_raid_siots_init_data_mem_info(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to init data memory info: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

	/* Set up the the read and write fruts 
	 */
	status = fbe_parity_journal_flush_setup_fruts(siots_p, 
												  write_fru_info_p, 
												  read_fru_info_p,
												  &memory_info);

	if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
	{
		fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
							 "journal_flush: failed to setup fruts with status = 0x%x, siots_p = 0x%p\n",
							 status, siots_p);
		return(status);
	}

	/* Plant the nested siots in case it is needed for recovery verify.
	 * We don't initialize it until it is required.
	 */
	status = fbe_raid_siots_consume_nested_siots(siots_p, &memory_info);
	if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
	{
		fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
							 "journal_flush: failed to consume nested siots: siots_p = 0x%p, status = 0x%x\n",
							 siots_p, status);
		return(status);
	}

	/* Initialize the RAID_VC_TS now that it is allocated.
	 */
	status = fbe_raid_siots_init_vcts(siots_p, &memory_info);
	if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
	{
		fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
							 "journal_flush: failed to init vcts: status = 0x%x, siots_p = 0x%p\n",
							 status, siots_p);

		return  status;
	}

	/* Initialize the VR_TS.
	 * We are allocating VR_TS structures WITH the fruts structures.
	 */
	status = fbe_raid_siots_init_vrts(siots_p, &memory_info);
	if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
	{
		fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
							 "journal_flush: failed to init vrts: status = 0x%x, siots_p = 0x%p\n",
							 status, siots_p);
		return  status;
	}

	/* Plant the allocated sgs into the locations calculated above.
	 */
	status = fbe_raid_fru_info_array_allocate_sgs(siots_p, 
												  &memory_info,
												  read_fru_info_p,
												  NULL,
												  write_fru_info_p);

	if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
	{
		fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
							 "journal_flush: failed to init sgs: status = 0x%x, siots_p = 0x%p\n",
							 status, siots_p);
		return  status;
	}

	/* Assign buffers to sgs.
	 * We reset the current ptr to the beginning since we always 
	 * allocate buffers starting at the beginning of the memory. 
	 */
	status = fbe_parity_journal_flush_setup_sgs(siots_p, &data_memory_info);
	if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
	{
		fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
							 "journal_flush: journal flush failed to setup sgs with status = 0x%x, siots_p = 0x%p\n",
							 status, siots_p);
		return(status); 
	}

	/* Make sure the buffer memory we just used does not overlap into 
	 * the area used for other resources. 
	 */
	return FBE_STATUS_OK;
}
/******************************************************************************
 * fbe_parity_journal_flush_setup_resources()
 *****************************************************************************/

/****************************************************************
 *  fbe_parity_journal_rd_fruts_validate_sgs()
 ****************************************************************
 * @brief
 *  Validate the sanity of a journal header read sg.
 *
 * @param fruts_p - the single fruts to act on
 * @param unused_1 - 
 * @param unused_2 - 
 *
 * @return
 *    none
 *
 * @author
 *  4/17/2014 - Created. Rob Foley   
 *
 ****************************************************************/
fbe_status_t fbe_parity_journal_rd_fruts_validate_sgs(fbe_raid_fruts_t * fruts_p,
                                                      fbe_lba_t unused_1,
                                                      fbe_u32_t * unused_2)
{
    /* Make sure the number of blocks in the fruts matches
     * the number of blocks in the sg. 
     * Also make sure we have not exceeded the sg length. 
     */
    fbe_u32_t sgs = 0;
    fbe_block_count_t curr_sg_blocks = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t journal_log_hdr_blocks;
    FBE_UNREFERENCED_PARAMETER(unused_1);
    FBE_UNREFERENCED_PARAMETER(unused_2);

    fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks);

    if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)fruts_p->sg_id)) {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid header invalid for fruts 0x%p, sg_id: 0x%p\n",
                             fruts_p, fruts_p->sg_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Make sure the number of sgs in the fruts is less than or equal to
     * the max number allowed. 
     */
    if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)fruts_p->sg_id)) {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid header invalid for fruts 0x%p, sg_id: 0x%p\n",
                             fruts_p, fruts_p->sg_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Make sure the fruts has blocks in it.
     */
    if (fruts_p->blocks == 0) {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid fruts blocks 0x%llx not expected fruts: 0x%p\n",
                             (unsigned long long)fruts_p->blocks, fruts_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Count the number of sg elements and blocks in the sg. 
     */
    status = fbe_raid_sg_count_sg_blocks(fbe_raid_fruts_return_sg_ptr(fruts_p), 
                                         &sgs,
                                         &curr_sg_blocks);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid: failed to count sg blocks: status = 0x%x, curr_sg_blocks 0x%llx\n",
                             status, (unsigned long long)curr_sg_blocks);
        return status;
    }

    /* Make sure the fruts has an SG for the correct block count.
     */
    if ((curr_sg_blocks == 0) || (curr_sg_blocks > journal_log_hdr_blocks)) {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid fruts blocks 0x%llx not equal to sg blocks 0x%llx fruts: 0x%p\n",
                             (unsigned long long)fruts_p->blocks,
                             (unsigned long long)curr_sg_blocks, fruts_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/***************************
 * fbe_parity_journal_rd_fruts_validate_sgs()
 ***************************/
/*!****************************************************************************
 * fbe_parity_journal_header_read_setup_resources()
 ******************************************************************************
 * @brief
 *  Setup resources from the newly allocated memory.
 *
 * @param siots_p - current I/O.
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *  2/28/2012 - Created. Vamsi V
 *  6/25/2012 - Modified to use dynamically-allocated header buffers. Dave Agans
 *
 ******************************************************************************/
fbe_status_t 
fbe_parity_journal_header_read_setup_resources(fbe_raid_siots_t * siots_p)
{
    fbe_raid_memory_info_t  memory_info;
    fbe_raid_memory_info_t  data_memory_info;
    fbe_status_t status;
    fbe_u32_t  idx = 0;
    fbe_u32_t  position = 0;
    fbe_raid_fru_info_t write_fru_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_raid_fru_info_t read_fru_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_lba_t lba;
    fbe_lba_t write_log_start_lba;
    fbe_u32_t write_log_slot_size;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_sg_element_t *read_sgl_ptr = NULL;
    fbe_u32_t journal_log_hdr_blocks;

    fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks);

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "journal_flush_header_read: failed to init memory info: status = 0x%x, siots_p = 0x%p, "
                             "siots_p->start_lba = 0x%llx, siots_p->xfer_count = 0x%llx\n",
                             status, siots_p, siots_p->start_lba, siots_p->xfer_count);
        return status; 
    }
    status = fbe_raid_siots_init_data_mem_info(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to init data memory info: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Determine slot header start lba */
    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &write_log_start_lba); 
    fbe_raid_geometry_get_journal_log_slot_size(raid_geometry_p, &write_log_slot_size); 
    lba = write_log_start_lba + (siots_p->journal_slot_id * write_log_slot_size);

    /* Initialize the fru info structs
     */
    fbe_raid_fru_info_init_arrays(FBE_RAID_MAX_DISK_ARRAY_WIDTH,
                                  &read_fru_info[0],
                                  &write_fru_info[0],
                                  NULL);

    /* Initialize header read fru info
     * We skip the dead drives because we won't be reading them
     */
    for (position = 0, idx = 0; position < raid_geometry_p->width; position++)
    {
        if (   (position != siots_p->dead_pos)
            && (position != siots_p->dead_pos_2))
        {
            fbe_raid_fru_info_init(&read_fru_info[idx],
                                   lba,
                                   1,
                                   position);
            read_fru_info[idx].sg_index = FBE_RAID_SG_INDEX_SG_1; 
            idx++;
        }
    }

    /* Set up the the read fruts */
    status = fbe_parity_journal_flush_setup_fruts(siots_p, 
                                                  &write_fru_info[0], 
                                                  &read_fru_info[0],
                                                  &memory_info);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "journal_flush: failed to setup fruts with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }

    /* We dont need nested siots as header on parity drive is not Xor of
     * headers on data drives so we cannot run verify.
     */

    /* Initialize the RAID_VC_TS now that it is allocated.
     */
    status = fbe_raid_siots_init_vcts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "journal_flush: failed to init vcts: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);

        return status;
    }

    /* Initialize the VR_TS.
     * We are allocating VR_TS structures WITH the fruts structures.
     */
    status = fbe_raid_siots_init_vrts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "journal_flush: failed to init vrts: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }

    /* Plant the allocated sgs into the locations calculated above.
     */
    status = fbe_raid_fru_info_array_allocate_sgs(siots_p, 
                                                  &memory_info,
                                                  &read_fru_info[0],
                                                  NULL,
                                                  NULL);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "journal_flush: failed to init sgs: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }
    /* Assign header buffers to sgs.
     * We reset the current ptr to the beginning since we always 
     * allocate buffers starting at the beginning of the memory.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    if (RAID_COND(read_fruts_p == NULL))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "write_log_flush: no read fruts\n");
    }

    while (read_fruts_p != NULL)
    {
        /* allocate and initialize the read fruts scatter gather list
         * Allocate the amount we will need on the header invalidate. 
         */
        read_sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_p);
        fbe_raid_sg_populate_with_memory(&data_memory_info,
                                         read_sgl_ptr, 
                                         FBE_BE_BYTES_PER_BLOCK * journal_log_hdr_blocks);  /* One block header read */

        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
    }

    /* Make sure the fruts chains are sane.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    status = fbe_raid_fruts_for_each(FBE_RAID_FRU_POSITION_BITMASK, read_fruts_p, fbe_parity_journal_rd_fruts_validate_sgs, 0, 0);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "write_log_flush: failed to validate sgs of read_fruts_p 0x%p "
                             "for siots_p 0x%p; failed status = 0x%x\n",
                             read_fruts_p, siots_p, status);
    }

    /* Initialize the read buffers so unread headers don't look valid 
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    status = fbe_raid_fruts_for_each(FBE_RAID_FRU_POSITION_BITMASK, read_fruts_p,
                                     fbe_parity_invalidate_fruts_write_log_header_buffer, 0, 0);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "write_log_flush: failed to invalidate write log header buffers of read_fruts_p 0x%p "
                             "for siots_p 0x%p; failed status = 0x%x\n",
                             read_fruts_p, siots_p, status);
    }

    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * fbe_parity_journal_header_read_setup_resources()
 *****************************************************************************/

/*!****************************************************************************
 * fbe_parity_journal_flush_generate_write_fruts()
 ******************************************************************************
 * @brief
 * We have read the log data into the buffers allocated to the fruts chain.
 * This function will reinitialize the fruts chain as a write operation,
 * preserving the sg info.
 *
 * @param siots_p - siots_p containing journal fruts that after the read portion
 *                  of the journal flush operation.
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *  1/25/2011 - Created. Daniel Cummins
 *  3/05/2012 - Vamsi V. Modified to use Slot headers instead of metadata service.
 *
 ******************************************************************************/
fbe_status_t fbe_parity_journal_flush_generate_write_fruts(fbe_raid_siots_t *siots_p)
{
    fbe_parity_write_log_header_t * slot_header_p = NULL;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_status_t status;
    fbe_u32_t slot_id;

    /* Get the write log slot from the siots. */
    fbe_raid_siots_get_journal_slot_id(siots_p, &slot_id);
    if (slot_id == FBE_RAID_INVALID_JOURNAL_SLOT)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log_flush: slot_id in siots is invalid %d \n",
                                            slot_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    while (write_fruts_p != NULL)
    {
        if ((write_fruts_p->position == siots_p->dead_pos) || (write_fruts_p->position == siots_p->dead_pos_2))
        {
            /* ensure we are not trying to flush to dead positions - this might happen if we were degraded (n-1)
             * raid 6 and just went n-2 before the log could be flushed.  We need to ensure that we flush what
             * we can before we can rebuild missing data correctly.
             */
            fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_p);
        }
        else
        {
            status = fbe_parity_write_log_get_iots_memory_header(siots_p, &slot_header_p);

            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: failed to find saved header with memory allocation status 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                return(status); 
            }

            /* turn this fruts into a flush operation
             */
            status = fbe_raid_fruts_init_request(write_fruts_p,
                                                 FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                                 (slot_header_p->parity_start + fbe_parity_write_log_get_slot_hdr_offset(slot_header_p, write_fruts_p->position)),
                                                 write_fruts_p->blocks,
                                                 write_fruts_p->position);

            if ( RAID_COND(status != FBE_STATUS_OK) )
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "write_log_flush: Journal Flush failed to intialize WRITE fruts 0x%p "
                                     "position %d, for siots_p 0x%p; faild status = 0x%x\n",
                                     write_fruts_p, write_fruts_p->position, siots_p, status);
                return  status;

            }
        }

        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_parity_journal_flush_generate_write_fruts()
 *****************************************************************************/

/*!****************************************************************************
 * fbe_parity_write_log_generate_header_invalidate_fruts()
 ******************************************************************************
 * @brief
 * This function reinitializes Write Fruts to do one block write to slot header
 * (used to invalidate the header).
 *
 * @param siots_p - siots_p containing journal fruts that after the read portion
 *                  of the journal flush operation.
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *  3/05/2012 - Created. Vamsi V.
 *
 ******************************************************************************/
fbe_status_t fbe_parity_write_log_generate_header_invalidate_fruts(fbe_raid_siots_t *siots_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_parity_write_log_header_t * slot_header_p = NULL;
    fbe_parity_write_log_header_t * fruts_header_p = NULL;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_sg_element_t *write_sgl_ptr = NULL;
    fbe_raid_fru_info_t write_info;
    fbe_raid_fru_info_t read_info;
    fbe_status_t status;
    fbe_u32_t slot_id;
    fbe_lba_t slot_lba;
    fbe_u32_t slot_size;
    fbe_u32_t journal_log_hdr_blocks;

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &slot_lba);
    fbe_raid_geometry_get_journal_log_slot_size(raid_geometry_p, &slot_size);
    fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks);

    /* Get the write log slot from the siots. */
    fbe_raid_siots_get_journal_slot_id(siots_p, &slot_id);
    if (slot_id == FBE_RAID_INVALID_JOURNAL_SLOT)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log: slot_id in siots is invalid %d \n",
                                            slot_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set up the write fruts chain sg lists to point to write slot header */
    fbe_raid_fruts_chain_set_prepend_header(&siots_p->write_fruts_head, FBE_TRUE);

    while (write_fruts_p != NULL)
    {
        /* Get pointer to header */
        status = fbe_parity_get_fruts_write_log_header(write_fruts_p, &fruts_header_p);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        if (siots_p->algorithm == RG_FLUSH_JOURNAL)
        {
            status = fbe_parity_write_log_get_iots_memory_header(siots_p, &slot_header_p);

            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: failed to find saved header with memory allocation status 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                return(status); 
            }
            /* Zero the fruts header buffer, then copy the saved header into it, then set invalidate pattern on it */
            fbe_zero_memory(fruts_header_p, FBE_BE_BYTES_PER_BLOCK);
            *fruts_header_p = *slot_header_p;
        }

		fbe_parity_write_log_set_header_invalid(fruts_header_p);

        /* If flush operation encountered media errors set the below header state. 
         * Writing it out to headers on disk helps us remember even across SP 
         * reboots when subsequent flush operation is performed.   
         */
        if (fbe_raid_iots_get_flags(iots_p) & FBE_RAID_IOTS_FLAG_REMAP_NEEDED)
        {
            fruts_header_p->header_state = FBE_PARITY_WRITE_LOG_HEADER_STATE_NEEDS_REMAP; 
        }

        if (siots_p->algorithm == RG_FLUSH_JOURNAL)
        {
            fbe_raid_siots_object_trace(siots_p,
                                        FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                        "wr_log hdr inv: s_id 0x%x, pos: 0x%x, hdr st: %d, lba 0x%llx wr_bm 0x%x \n",
                                        siots_p->journal_slot_id, write_fruts_p->position, fruts_header_p->header_state,
                                        fruts_header_p->parity_start, fruts_header_p->write_bitmap);
        }
        else
        {
            fbe_raid_siots_object_trace(siots_p,
                                        FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                        "wr_log hdr inv: s_id 0x%x, pos: 0x%x, hdr st: %d, lba 0x%llx wr_bm 0x%x \n",
                                        siots_p->journal_slot_id, write_fruts_p->position, fruts_header_p->header_state,
                                        fruts_header_p->parity_start, fruts_header_p->write_bitmap);
        }
		/* Set count and terminate the S/G list properly; it already points to the header. */
		fbe_raid_fruts_get_sg_ptr(write_fruts_p, &write_sgl_ptr);

        write_sgl_ptr->count = (journal_log_hdr_blocks * FBE_BE_BYTES_PER_BLOCK); /* One block header read */
        write_sgl_ptr++;
        write_sgl_ptr->address = 0;
        write_sgl_ptr->count = 0;

        /* turn this fruts into a header invalidation operation */
        status = fbe_raid_fruts_init_request(write_fruts_p,
                                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                             (slot_lba + (slot_id * slot_size)),
                                             journal_log_hdr_blocks, /*  header write */
                                             write_fruts_p->position);

        if ( RAID_COND(status != FBE_STATUS_OK) )
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "write_log_flush: Journal Flush failed to intialize WRITE fruts 0x%p "
                                 "position %d, for siots_p 0x%p; faild status = 0x%x\n",
                                 write_fruts_p, write_fruts_p->position, siots_p, status);
            return  status;

        }

        /* Reset preread info for the journal invalidation. Load the read_info with the pre-read values for
         * this write.
         */
        write_info.lba = write_fruts_p->lba;
        write_info.blocks = write_fruts_p->blocks;
        fbe_raid_siots_setup_preread_fru_info(siots_p, &read_info, &write_info, write_fruts_p->position);
        write_fruts_p->write_preread_desc.lba = read_info.lba;
        write_fruts_p->write_preread_desc.block_count = read_info.blocks;

        /* Set the pre-read to be zeroes since there is no data to be preserved in the write
         * journal slots.
         */
        fbe_raid_memory_get_zero_sg(&(write_fruts_p->write_preread_desc.sg_list));

        if ((write_fruts_p->position == siots_p->dead_pos) || (write_fruts_p->position == siots_p->dead_pos_2))
        {
            /* do not send any operations to drives that are dead */
            fbe_raid_fruts_set_fruts_degraded_nop(write_fruts_p);
        }

        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    }

    /* Generate checksum and lba stamps on header buffers. 
     * Write stamp and Time stamp will be set to zero */
    status = fbe_raid_xor_write_log_header_set_stamps(siots_p, FBE_FALSE);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_parity_write_log_generate_header_invalidate_fruts()
 *****************************************************************************/

/*!****************************************************************************
 * fbe_parity_journal_flush_restore_dest_lba()
 ******************************************************************************
 * @brief
 * This function restores the write fruts lba after reading the journal log in
 * preparation for a subsequent lba/checksum validation.
 *
 * @param siots_p - siots_p containing journal fruts that after the read portion
 *                  of the journal flush operation.
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *  1/25/2011 - Created. Daniel Cummins
 *  3/05/2012 - Vamsi V. Modified to use Slot headers instead of metadata service.
 *
 ******************************************************************************/
fbe_status_t
fbe_parity_journal_flush_restore_dest_lba(fbe_raid_siots_t *siots_p)
{
    fbe_raid_fruts_t * write_fruts_p = NULL;
    fbe_parity_write_log_header_t * slot_header_p = NULL;
    fbe_status_t status;

    /* we want to simply adjust the LBA in the write fruts chain.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    while (write_fruts_p != NULL)
    { 
        status = fbe_parity_write_log_get_iots_memory_header(siots_p, &slot_header_p);

        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: failed to find saved header with memory allocation status 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return(status); 
        }
        /* validate the block count for this position
         */
         if (fbe_parity_write_log_get_slot_hdr_blk_cnt(slot_header_p, write_fruts_p->position) != write_fruts_p->blocks)
        {
            /* if we get here it means the fruts block count does not match blk_cnt in header
             * and ultimately the data described by the sg entry.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "write_log_flush: write_log cache blk_cnt %d does not "
                                                "match current fruts block count %d for position %d\n",
                                                fbe_parity_write_log_get_slot_hdr_blk_cnt(slot_header_p, write_fruts_p->position),
                                                (int)write_fruts_p->blocks,
                                                write_fruts_p->position);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* adjust the write fruts lba so we can verify the lba stamps
         */
        write_fruts_p->lba = slot_header_p->parity_start + fbe_parity_write_log_get_slot_hdr_offset(slot_header_p, write_fruts_p->position);
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_parity_journal_flush_restore_dest_lba()
 *****************************************************************************/

/***************************************************************************
 * fbe_parity_journal_flush_calculate_num_sgs()
 ***************************************************************************
 *
 * @brief
 *  This function is responsible for counting the S/G lists needed
 *  in the parity journal flush state machine.  
 *
 * @param siots_p - Pointer to current I/O.
 * @param num_sgs_p - Array of types of sgs we will need to allocate.
 * @param write_fru_info_p - Journal fru info
 * @param read_fru_info_p - Preread fru info
 *
 * @return FBE_STATUS_OK num_sgs_p will be populated.
 *
 * @notes
 * 
 **************************************************************************/
fbe_status_t fbe_parity_journal_flush_calculate_num_sgs(fbe_raid_siots_t *siots_p,
														fbe_u16_t *num_sgs_p,
														fbe_raid_fru_info_t *write_fru_info_p,
														fbe_raid_fru_info_t *read_fru_info_p)
{   fbe_u32_t i;
	fbe_u32_t j = 0;
	fbe_u32_t sg_count;
	fbe_block_count_t blks_remaining = 0;
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

	/* Page size must be set.
	 */
	if (!fbe_raid_siots_validate_page_size(siots_p))
	{
		fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
							 "journal_flush: page size invalid\n");
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_zero_memory(num_sgs_p, (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));

	/* Next calculate the number of sgs of each type and update the fru info.
	 */
	for (i=0;i<siots_p->drive_operations;i++)
	{
		/* determine the type of sg we will need for the journal read from this fru
		 */
		if (write_fru_info_p[i].blocks)
		{
			sg_count = 0;

			blks_remaining = fbe_raid_sg_count_uniform_blocks(write_fru_info_p[i].blocks,
															  siots_p->data_page_size,
															  blks_remaining,
															  &sg_count);

			status = fbe_raid_fru_info_set_sg_index(&write_fru_info_p[i], sg_count, FBE_TRUE);

			if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
			{
				fbe_raid_siots_trace(siots_p,
									 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
									 "journal_flush: failed to set sg index: status = 0x%x, siots_p = 0x%p, "
									 "write_fru_info_p = 0x%p\n", 
									 status, siots_p, &write_fru_info_p[i]);
				break;
			}

			num_sgs_p[write_fru_info_p[i].sg_index]++;
		}

		/* determine the type of sg we will need for the preread read from this fru
		 */
		if (read_fru_info_p[j].blocks)
		{
			if ( read_fru_info_p[j].position == write_fru_info_p[i].position)
			{
				sg_count = 0;

				blks_remaining = fbe_raid_sg_count_uniform_blocks(read_fru_info_p[j].blocks,
																  siots_p->data_page_size,
																  blks_remaining,
																  &sg_count);

				status = fbe_raid_fru_info_set_sg_index(&read_fru_info_p[j], sg_count, FBE_TRUE);

				if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
				{
					fbe_raid_siots_trace(siots_p,
										 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
										 "journal_flush: failed to set sg index: status = 0x%x, siots_p = 0x%p, "
										 "read_fru_info_p = 0x%p\n", 
										 status, siots_p, &read_fru_info_p[j]);
					break;
				}

				num_sgs_p[read_fru_info_p[j].sg_index]++;
				j++;
			}
		}
	}

	return status;
}
/******************************************************************************
 * end fbe_parity_journal_flush_calculate_num_sgs()
 *****************************************************************************/

/*!****************************************************************************
 * fbe_parity_journal_flush_calculate_memory_size()
 ******************************************************************************
 * @brief  This function calculates the total memory usage of the siots for
 *         this state machine.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param fru_info_size - number of elements in write_fru_info_p/read_fru_info_p
 * @param write_fru_info_p - Array of read information. (journal read/live stripe write)
 * @param read_fru_info_p - Array of write information. (preread info for live stripe write)
 * 
 * @return fbe_status_t
 *
 **************************************************************************/
fbe_status_t fbe_parity_journal_flush_calculate_memory_size(fbe_raid_siots_t *siots_p,
															fbe_u32_t fru_info_size,
															fbe_raid_fru_info_t *write_fru_info_p,
															fbe_raid_fru_info_t *read_fru_info_p)
{
	fbe_u16_t num_sgs[FBE_RAID_MAX_SG_TYPE];
	fbe_u32_t num_fruts = 0;
	fbe_block_count_t write_blocks = 0;
	fbe_block_count_t read_blocks = 0;
	fbe_u32_t read_fru_info_count = 0;
	fbe_status_t status;

	fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));
	fbe_zero_memory(write_fru_info_p, (sizeof(fbe_raid_fru_info_t) * fru_info_size));
	fbe_zero_memory(read_fru_info_p, (sizeof(fbe_raid_fru_info_t) * fru_info_size));

	/* Then calculate how many buffers are needed.
	 */
	status = fbe_parity_journal_flush_get_fru_info(siots_p,
												   fru_info_size,
												   write_fru_info_p,
												   read_fru_info_p,
												   &read_fru_info_count,
												   &write_blocks,
												   &read_blocks);

	if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
	{
		fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
							 "journal_flush: failed to get fru info: status: 0x%x siots_p = 0x%p, "
							 "siots_p->start_lba = 0x%llx, siots_p->xfer_count = 0x%llx \n",
							 status, siots_p, siots_p->start_lba, siots_p->xfer_count);
		return status; 
	}

	/* We need one fruts for each write position and one for each read position.
	 */
	num_fruts = siots_p->drive_operations + read_fru_info_count;

	siots_p->total_blocks_to_allocate = write_blocks + read_blocks;

	fbe_raid_siots_set_optimal_page_size(siots_p,
										 num_fruts, 
										 siots_p->total_blocks_to_allocate);

	/* Next calculate the number of sgs of each type.
	 */
	status = fbe_parity_journal_flush_calculate_num_sgs(siots_p, 
														&num_sgs[0], 
														write_fru_info_p,
														read_fru_info_p);

	if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
	{
		fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
							 "journal_flush: failed to calculate number of sgs: status = 0x%x, siots_p = 0x%p\n",
							 status, siots_p);
		return status; 
	}

	/* Calculate how many memory pages we will need to allocate 
	 */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, &num_sgs[0],
                                                FBE_TRUE /* In-case recovery verify is required*/);

	if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
	{
		fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
							 "write_log_flush: failed to calculated number of pages: status = 0x%x, siots_p = 0x%p\n",
							 status, siots_p);
		return status; 
	}

	return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_parity_journal_flush_calculate_memory_size()
 *****************************************************************************/

/*!****************************************************************************
 * fbe_parity_write_log_calc_csum_of_csums()
 ******************************************************************************
 * @brief
 *  This function loops thru all blocks in each fruts and calculates 
 * csum of csums and sets/verifies csum_of_csums filed in slot header.
 * Note: This function assumes that slot header is not prepended to Fruts SGlist.
 *
 * @param siots_p - current I/O.
 * @param fruts_p - chain of fruts on which to calculate csum of csums
 * @param set_csum - if true set the field in slot header; else verifies it.
 * @param chksum_err_bitmap - pointer to returned bitmap of FRU positions on
 *                            which csum verify failed.
 *                            Bitmap bit = 1 if failed.
 * @return status
 *
 * @author
 *  03/07/2012 - Created. Vamsi V
 *  06/25/2012 - Use dynamically allocated buffers. Dave Agans
 *  11/05/2012 - Added return status. Dave Agans
 *
 ******************************************************************************/
fbe_status_t fbe_parity_write_log_calc_csum_of_csums(fbe_raid_siots_t * siots_p,
                                                     fbe_raid_fruts_t * fruts_p,
                                                     fbe_bool_t set_csum,
                                                     fbe_u16_t *chksum_err_bitmap_p)
{
    fbe_status_t status;
    fbe_raid_fruts_t * current_fruts_p = fruts_p;
    fbe_raid_fruts_t * header_fruts_p = NULL;
    fbe_u32_t slot_id;
    fbe_parity_write_log_header_t * slot_header_p = NULL;
    fbe_sg_element_t *sgl_ptr = NULL;
    fbe_sg_element_t sgl_temp;
    fbe_u32_t block;
    fbe_u16_t csum_of_csums = 0;
    fbe_u16_t ret_bitmap = 0;

    /* Get the write log slot from the siots.
     */
    fbe_raid_siots_get_journal_slot_id(siots_p, &slot_id);
    if (slot_id == FBE_RAID_INVALID_JOURNAL_SLOT)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log: slot_id in siots is invalid %d \n",
                                            slot_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check the checksum of each fruts in the chain.
     */
    while (current_fruts_p != NULL)
    {
        if (current_fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID)
        {
            /* Initialize the checksum field */
            csum_of_csums = 0;

            /* Get pointer to SG list */
            fbe_raid_fruts_get_sg_ptr(current_fruts_p, &sgl_ptr);

            /* Make a local copy of SGL */
            sgl_temp.address = sgl_ptr->address;
            sgl_temp.count = sgl_ptr->count; 

            for (block = 0; block < current_fruts_p->blocks; block++)
            {
                fbe_sector_t *blkptr = (fbe_sector_t *) sgl_temp.address;

                /* Running checksum of sector checksums. */
                csum_of_csums ^= blkptr->crc; 

                /* Goto the next block */
                if (sgl_temp.count > FBE_BE_BYTES_PER_BLOCK)
                {
                    sgl_temp.address += FBE_BE_BYTES_PER_BLOCK;
                    sgl_temp.count -= FBE_BE_BYTES_PER_BLOCK;
                }
                else
                {
                    sgl_ptr++;
                    /* Make a local copy of SGL */
                    sgl_temp.address = sgl_ptr->address;
                    sgl_temp.count = sgl_ptr->count;
                }
            } /* for each block */

            /* Cook Csum */
            csum_of_csums ^= FBE_PARITY_WRITE_LOG_HEADER_CSUM_SEED;

            if (set_csum)
            {
                /* This is done when we are writing out header+data to write_log slot
                 * Write it out to all headers for redundancy purposes
                 */
                header_fruts_p = fruts_p;
                while (header_fruts_p)
                {   
                    /* Get pointer to slot header */
                    status = fbe_parity_get_fruts_write_log_header(header_fruts_p, &slot_header_p);
                    if (status != FBE_STATUS_OK)
                    {
                        return status;
                    }

                    /* Set csum field and increment to next fruts */
                    fbe_parity_write_log_set_slot_hdr_csum_of_csums(slot_header_p, current_fruts_p->position, csum_of_csums);

                    header_fruts_p = fbe_raid_fruts_get_next(header_fruts_p);
                }
            }
            else
            {
                /* Get pointer to stored slot header
                 */
                status = fbe_parity_write_log_get_iots_memory_header(siots_p, &slot_header_p);

                if (status != FBE_STATUS_OK)
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "parity: failed to find saved header with memory allocation status 0x%x, siots_p = 0x%p\n",
                                         status, siots_p);
                    return(status); 
                }
                /* This is done when we are reading data back from write_log slot. 
                 * Most common case of checksum mismatch is Incomplete writes but
                 * can also happen if media is bad.
                 */
                if (fbe_parity_write_log_get_slot_hdr_csum_of_csums(slot_header_p, current_fruts_p->position) != csum_of_csums)
                {
                    /* set the bitmap to indicate that data from this fruts position is no good */
                    ret_bitmap |=  (1 << current_fruts_p->position); 
                }
            }
        } /* If valid opcode */

        current_fruts_p = fbe_raid_fruts_get_next(current_fruts_p);
    } /* for each fru. */

    if (set_csum)
    {
        return FBE_STATUS_OK;
    }
    else
    {
        if (chksum_err_bitmap_p)
        {
            *chksum_err_bitmap_p = ret_bitmap;
            return FBE_STATUS_OK;
        }
        else
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "write_log: chksum_err_bitmap pointer is NULL\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
}

/******************************************************************************
 * end fbe_parity_write_log_calc_csum_of_csums()
 *****************************************************************************/

/****************************************************************
 *  fbe_parity_write_log_remove_failed_read_fruts()
 ****************************************************************
 * @brief
 * This function removes failed fruts from read_fruts_chain
 * and puts them on read2_fruts_chain.    
 *
 * @param siots_p - pointer to Siots being used for write_log flush.
 * @param eboard_p - records of all erros seen on write_log header read.
 *
 * @return
 *  none
 *
 * @author
 *  04/04/2012 - Created. Vamsi V
 *
 ****************************************************************/
void fbe_parity_write_log_remove_failed_read_fruts( fbe_raid_siots_t *const siots_p, fbe_raid_fru_eboard_t * eboard_p)
{
    fbe_raid_fruts_t *cur_fruts_p = NULL;
    fbe_raid_fruts_t *next_fruts_p = NULL;
    fbe_u16_t err_bitmap = 0;

    fbe_raid_siots_get_read_fruts(siots_p, &cur_fruts_p);

    if(eboard_p)
    {
        /* Bitmap of drive positions that failed on header read */
        err_bitmap = (eboard_p->dead_err_bitmap | eboard_p->hard_media_err_bitmap);
    }
    else
    {
        /* Bitmap of positions that had checksum/lba stamp errors */
        err_bitmap = siots_p->misc_ts.cxts.eboard.u_crc_bitmap; 
    }

    /* Loop over all fruts in the chain and if we hit any dead or
     * media_error positions, move them to read2_fruts_chain. 
     */
    while (cur_fruts_p)
    {
        next_fruts_p = fbe_raid_fruts_get_next(cur_fruts_p);

        if ((err_bitmap & (1 << cur_fruts_p->position)) == (1 << cur_fruts_p->position))
        {
            fbe_raid_siots_remove_read_fruts(siots_p, cur_fruts_p);
            fbe_raid_siots_enqueue_tail_read2_fruts(siots_p, cur_fruts_p);
        }
        cur_fruts_p = next_fruts_p;
    }
    return;
}
/**********************************************************
 * end fbe_parity_write_log_remove_failed_read_fruts()
 *********************************************************/

/****************************************************************
 *  fbe_parity_write_log_move_read_fruts_to_write_chain()
 ****************************************************************
 * @brief
 * Utility function used by Flush Statemachine    
 *
 * @param siots_p - pointer to Siots being used for write_log flush.
 *
 * @return
 *  none
 *
 * @author
 *  05/10/2012 - Created. Vamsi V
 *
 ****************************************************************/
void fbe_parity_write_log_move_read_fruts_to_write_chain( fbe_raid_siots_t *const siots_p)
{
    fbe_raid_fruts_t *cur_fruts_p = NULL;
    fbe_raid_fruts_t *next_fruts_p = NULL;

    /* Sanity check */
    fbe_raid_siots_get_write_fruts(siots_p, &cur_fruts_p);
    if (cur_fruts_p != NULL)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "%s write_log: write_fruts chain not NULL\n",
                                            __FUNCTION__);
    }

    /* Loop over all fruts in read_fruts chain. 
     */
    fbe_raid_siots_get_read_fruts(siots_p, &cur_fruts_p);
    while (cur_fruts_p)
    {
        next_fruts_p = fbe_raid_fruts_get_next(cur_fruts_p);
        fbe_raid_siots_remove_read_fruts(siots_p, cur_fruts_p);
        fbe_raid_siots_enqueue_tail_write_fruts(siots_p, cur_fruts_p);
        cur_fruts_p = next_fruts_p;
    }

    /* Loop over all fruts in read2_fruts chain. 
     */
    fbe_raid_siots_get_read2_fruts(siots_p, &cur_fruts_p);
    while (cur_fruts_p)
    {
        next_fruts_p = fbe_raid_fruts_get_next(cur_fruts_p);
        fbe_raid_siots_remove_read2_fruts(siots_p, cur_fruts_p);
        fbe_raid_siots_enqueue_tail_write_fruts(siots_p, cur_fruts_p);
        cur_fruts_p = next_fruts_p;
    }
    return;
}
/**********************************************************
 * end fbe_parity_write_log_move_read_fruts_to_write_chain()
 *********************************************************/

/****************************************************************
 *  fbe_parity_write_log_get_iots_memory_header()
 ****************************************************************
 * @brief
 * Gets a pointer to the parity write log header stored in iots memory request    
 *
 * @param siots_p - pointer to siots being used for write_log flush
 *        headerpp - points to the pointer that will point to the header
 *
 * @return
 *  FBE_STATUS_OK if memory was found, error status if not
 *
 * @author
 *  06/29/2012 - Created. Dave Agans
 *
 ****************************************************************/
fbe_status_t fbe_parity_write_log_get_iots_memory_header (fbe_raid_siots_t *siots_p,
                                                          fbe_parity_write_log_header_t **header_pp)
{

    fbe_status_t status;
    fbe_raid_memory_info_t memory_info;
    fbe_memory_id_t memory_p = NULL;
    fbe_u32_t bytes_to_allocate = sizeof(fbe_parity_write_log_header_t);
    fbe_u32_t bytes_allocated;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    /* Initialize our memory request information
     */
    status = fbe_raid_memory_init_memory_info(&memory_info, &iots_p->memory_request, FBE_TRUE /* yes control */);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to init iots memory info with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status); 
    }

    /* Get the next piece of memory from the current location in the memory queue.
     */
    memory_p = fbe_raid_memory_allocate_contiguous(&memory_info,
                                                   bytes_to_allocate,
                                                   &bytes_allocated);

    if(RAID_COND(bytes_allocated != bytes_to_allocate))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: siots 0x%x flush header bytes_allocated: %d != bytes_to_allocate: %d \n",
                             (unsigned int)siots_p, bytes_allocated, bytes_to_allocate);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *header_pp = memory_p;
    return FBE_STATUS_OK;
}

/**********************************************************
 * end fbe_parity_write_log_get_iots_memory_header()
 *********************************************************/

/****************************************************************
 *  fbe_parity_write_log_validate_headers()
 ****************************************************************
 * @brief
 * This function validates slot headers that are read from the disk and
 * also generates headers for disk positions where header read failed
 * (for e.g. due to media errors)     
 *
 * @param siots_p - Pointer to Siots being used for write_log flush.
 * @param num_drive_ops - Set by this function based on number of valid slot headers.  
 *
 * @return
 *  none
 *
 * @author
 *  04/10/2012 - Created. Vamsi V
 *
 ****************************************************************/
fbe_status_t fbe_parity_write_log_validate_headers( fbe_raid_siots_t *const siots_p, fbe_u32_t * num_drive_ops, 
                                                    fbe_bool_t * b_flush, fbe_bool_t * b_inv)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_fruts_t *read2_fruts_p = NULL;
    fbe_u8_t header_buff[FBE_PARITY_WRITE_LOG_HEADER_SIZE];
    fbe_parity_write_log_header_t * header_p = NULL;
    fbe_parity_write_log_header_t * header_cpy_p = NULL;
    fbe_parity_write_log_header_timestamp_t latest_header_timestamp;
    fbe_u16_t valid_header_bitmap = 0;
    fbe_u16_t write_log_bitmap = 0;

    /* Initialize */
    *num_drive_ops = 0;
    *b_flush = FBE_FALSE;
    *b_inv = FBE_FALSE;
    fbe_zero_memory(&header_buff, FBE_PARITY_WRITE_LOG_HEADER_SIZE);
    latest_header_timestamp.sec = 0;
    latest_header_timestamp.usec = 0;

    /* Loop over all fruts in the chain and find the latest generation
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    while (read_fruts_p != NULL)
    {
        /* Get pointer to header: Note that buffer used to read in slot header (which is in SGL of  
         * each fruts) is the same buffer in the Parity object.   
         */
        status = fbe_parity_get_fruts_write_log_header(read_fruts_p, &header_p);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }

        fbe_raid_siots_object_trace(siots_p,
                                    FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "wr_log hdr vd: s_id 0x%x, pos: 0x%x, hdr st: %d, lba 0x%llx cnt 0x%x ts: 0x%llx 0x%llx wr_bm 0x%x \n",
                                    siots_p->journal_slot_id, read_fruts_p->position, header_p->header_state,
                                    header_p->parity_start, header_p->parity_cnt, header_p->header_timestamp.sec, 
                                    header_p->header_timestamp.usec, header_p->write_bitmap);

        /* Perform sanity check */
        if (header_p->header_version != FBE_PARITY_WRITE_LOG_CURRENT_HEADER_VERSION)
        {
            /* Check if this is Zeroed drive that write_logging never touched. Dont check the stamps */
            if (fbe_equal_memory(&header_buff, header_p, FBE_PARITY_WRITE_LOG_HEADER_SIZE))
            {
                /* Header was never touched. It's invalid, set it that way.
                 * Increment to next Fruts */
                fbe_parity_write_log_set_header_invalid(header_p);
                *b_inv = FBE_TRUE;
                read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
                continue;   
            }
            else
            {
                /* Header may be from old version of software. It's invalid, set it that way.
                 * But since sim should not have old headers on disk, trace an error to fail fbe_tests.
                 * Increment to next Fruts */
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "Unexpected header_version 0x%llx in slot 0x%x assumed invalid\n",
                                     header_p->header_version, siots_p->journal_slot_id);
                fbe_parity_write_log_set_header_invalid(header_p);
                *b_inv = FBE_TRUE;
                header_p->header_timestamp.sec = 0;
                header_p->header_timestamp.usec = 0;
                read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
                continue;   
            }
        }
        /* Version is ok. Check if this header indicates that remap is needed. */
        else if (header_p->header_state == FBE_PARITY_WRITE_LOG_HEADER_STATE_NEEDS_REMAP)
        {
            fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
            fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_REMAP_NEEDED);
        }

        if (fbe_parity_write_log_compare_timestamp(&header_p->header_timestamp, &latest_header_timestamp) > 0)
        {
            /* Found later timestamped header, note it */
            latest_header_timestamp.sec = header_p->header_timestamp.sec;
            latest_header_timestamp.usec = header_p->header_timestamp.usec;
        }

        /* Increment to next Fruts */
        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
    }

    /* If we have latest generation, loop over all fruts in the chain, check the latest generation fruts and invalidate the rest
     */
    if (latest_header_timestamp.sec != 0)
    {
        fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
        while (read_fruts_p != NULL)
        {
            /* Get pointer to header, first buffer in fruts sgl.   
             */
            status = fbe_parity_get_fruts_write_log_header(read_fruts_p, &header_p);
            if (status != FBE_STATUS_OK)
            {
                return status;
            }

            /* Check if this header is valid  */
            if (!fbe_parity_write_log_is_header_valid(header_p))
            {
                /* Invalid already, increment to next Fruts */
                read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
                continue;
            }
            else if (fbe_parity_write_log_compare_timestamp(&header_p->header_timestamp, &latest_header_timestamp) == 0)
            {
                if (header_cpy_p == NULL)
                {
                    /* Found first valid latest-generation header */
                    header_cpy_p = header_p; 
                }
                else
                {
                    /* Compare with model header -- all valid headers are mirror copies
                     */
                    if (!fbe_equal_memory(header_cpy_p, header_p, FBE_PARITY_WRITE_LOG_HEADER_SIZE))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                             "fbe_parity_write_log_validate_headers: valid headers don't match %d\n", siots_p->journal_slot_id);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                }

                /* Check write_bitmap */
                if ((header_p->write_bitmap & ((fbe_u16_t)0x1 << read_fruts_p->position)) != ((fbe_u16_t)0x1 << read_fruts_p->position))
                {
                    /* Valid header but its not accounted for in write_bitmap */
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "fbe_parity_write_log_validate_headers for slot:%d header %d not in write bitmap 0x%x\n",
                                         siots_p->journal_slot_id, read_fruts_p->position, header_p->write_bitmap);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* okay, count it */
                valid_header_bitmap |= ((fbe_u16_t)0x1 << read_fruts_p->position);
                (*num_drive_ops)++;
            }
            else
            {
                /* Must be an older generation, invalidate it so the flush fruts allocation doesn't use it
                 */
                fbe_parity_write_log_set_header_invalid(header_p);
                *b_inv = FBE_TRUE;
            }

            /* Increment to next Fruts */
            read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
        }
    }

    /* Determine if Flush is needed */
    if (header_cpy_p)
    {
        /* If we are already in the flush phase (not just the initial header read phase),
         * save the header in the static slot area so it survives to the read data state
         */
        if (siots_p->algorithm == RG_FLUSH_JOURNAL)
        {

            status = fbe_parity_write_log_get_iots_memory_header(siots_p, &header_p);

            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: failed to save header with memory allocation status 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                return(status); 
            }

            *header_p = *header_cpy_p;
        }

        /* For fruts in read2_fruts chain, header reads failed. Generate headers for them using the model copy. */
        fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_p);
        while (read2_fruts_p != NULL)
        {
            /* Get pointer to header: Note that buffer used to read in slot header (which is in SGL of  
             * each fruts) is the same buffer in the Parity object.   
             */
            status = fbe_parity_get_fruts_write_log_header(read2_fruts_p, &header_p);
            if (status != FBE_STATUS_OK)
            {
                return status;
            }

            /* Check write_bitmap */
            if ((header_cpy_p->write_bitmap & ((fbe_u16_t)0x1 << read2_fruts_p->position)) == ((fbe_u16_t)0x1 << read2_fruts_p->position))
            {
                /* This write_log slot position has valid user data which needs to be flushed */
                valid_header_bitmap |= ((fbe_u16_t)0x1 << read2_fruts_p->position);
                (*num_drive_ops)++;

                /* Generate the header. Since headers are mirrors, do a copy operation */
                fbe_copy_memory(header_p, header_cpy_p, FBE_PARITY_WRITE_LOG_HEADER_SIZE); 
            }

            /* Increment to next Fruts */
            read2_fruts_p = fbe_raid_fruts_get_next(read2_fruts_p);
        }

        /* For R6, we may have to flush even if data for all header_cpy_p->write_bitmap positions is not available. 
         * Example usecase: R6 is single degraded and goes double degraded when writing to write_log and broken 
         * while writing to live-stripe. Second degraded drive is marked for rebuild on live-stripe; so if data
         * is available for rest of the positions, Flush should proceed.   
         */
        write_log_bitmap = header_cpy_p->write_bitmap;
        if (siots_p->dead_pos != FBE_RAID_INVALID_DEAD_POS)
        {
            write_log_bitmap &= ~(1 << siots_p->dead_pos);  
        }
        if (siots_p->dead_pos_2 != FBE_RAID_INVALID_DEAD_POS)
        {
            write_log_bitmap &= ~(1 << siots_p->dead_pos_2);  
        }

        /* Decision about abandoning Flush cannot be made until chunk info for live stripe is available.
         * So if this is header_read, return true.
         */
        if ((write_log_bitmap != valid_header_bitmap) && (siots_p->algorithm == RG_FLUSH_JOURNAL))
        {
            /* Write_log slot has incomplete write transaction. There are two senarios for this:
             * 1. Write of Header+Data to write_log slot did not make to one or more positions due to RG going broken.
             *    In this case live-stripe was not touched and so its consistent old data.
             * 2. Write of Invalid Header after write to live-stripe did not make to one or more positions due to RG going broken.
             *    In this case write to live-stripe completed successfully and so its consistent with new data.
             * In both case we should abandon flush to live-stripe and just Invalidate the headers.       
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO, 
                                        "write_log valid hdrs don't match. slot_id: 0x%x, wr_bm: 0x%x, valid_hdr_bm: 0x%x dpos: 0x%x dpos_2: 0x%x \n", 
                                        siots_p->journal_slot_id, header_cpy_p->write_bitmap, valid_header_bitmap, siots_p->dead_pos, siots_p->dead_pos_2);
            *b_inv = FBE_TRUE;
        }
        else
        {
            /* We have to perform Flush; Set the flag */
            *b_flush = FBE_TRUE;

            /* Copy info about live-stripe from header to siots */
            siots_p->start_lba = header_cpy_p->start_lba;
            siots_p->xfer_count = header_cpy_p->xfer_cnt;
            siots_p->parity_start = header_cpy_p->parity_start;
            siots_p->parity_count = header_cpy_p->parity_cnt;

            /* Copy info about live-stripe from header to iots */
            fbe_raid_iots_set_lba(fbe_raid_siots_get_iots(siots_p), header_cpy_p->start_lba);
            fbe_raid_iots_set_blocks(fbe_raid_siots_get_iots(siots_p), header_cpy_p->xfer_cnt);
            fbe_raid_iots_set_current_lba(fbe_raid_siots_get_iots(siots_p), header_cpy_p->start_lba);
            fbe_raid_iots_set_current_op_blocks(fbe_raid_siots_get_iots(siots_p), header_cpy_p->xfer_cnt);
            fbe_raid_iots_set_packet_lba(fbe_raid_siots_get_iots(siots_p), header_cpy_p->start_lba);
            fbe_raid_iots_set_packet_blocks(fbe_raid_siots_get_iots(siots_p), header_cpy_p->xfer_cnt);
        }
    }

    /* Send a flush abandoned event if we had header read errors but did not find another valid header --
     *   we may have just ignored a valid header
     */
    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_p);
    if (read2_fruts_p && !header_cpy_p)
    {
        fbe_u32_t err_pos_bitmap = 0;
        fbe_u32_t err_count = 0;

        /* Loop through the read2_fruts to get all the bad positions
         */
        while (read2_fruts_p)
        {
            err_pos_bitmap |= (0x1 << read2_fruts_p->position);
            err_count++;
            read2_fruts_p = fbe_raid_fruts_get_next(read2_fruts_p);
        }

        /* If there's only one possible position to flush, there is no damage if we don't flush
         * So no need to alert user unless there are at least two errors
         */
        if (err_count > 1)
        {
            status = fbe_parity_write_log_send_flush_abandoned_event(siots_p,
                                                                     err_pos_bitmap,
                                                                     0, /* This field is for CRC errors, not media errors */ 
                                                                     RG_WRITE_LOG_HDR_RD);
        }
    }

    /* Try to invalidate headers that had errors. We will proceed to invalidate only if b_flush is false. */
    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_p);
    if (read2_fruts_p)
    {
        *b_inv = FBE_TRUE;      
    }

    return status;
}
/**********************************************************
 * end fbe_parity_write_log_validate_headers()
 *********************************************************/

/****************************************************************
 *  fbe_parity_invalidate_fruts_write_log_header_buffer()
 ****************************************************************
 *  @brief
 *    Invalidate the write log header in a fruts sg.
 *
 * @param fruts_p - the single fruts to act on
 * @param unused_1 - 
 * @param unused_2 - 
 *
 * @return
 *    none
 *
 *  @author
 *    06/20/2012 - Created. Dave Agans
 *
 ****************************************************************/
fbe_status_t fbe_parity_invalidate_fruts_write_log_header_buffer(fbe_raid_fruts_t * fruts_p,
                                                                 fbe_lba_t unused_1,
                                                                 fbe_u32_t * unused_2)
{
    fbe_status_t status;
    fbe_parity_write_log_header_t *header_p;

    FBE_UNREFERENCED_PARAMETER(unused_1);
    FBE_UNREFERENCED_PARAMETER(unused_2);

    status = fbe_parity_get_fruts_write_log_header(fruts_p, &header_p);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    header_p->header_state = FBE_PARITY_WRITE_LOG_HEADER_STATE_INVALID;

    return FBE_STATUS_OK;
}
/***************************
 * fbe_parity_invalidate_fruts_write_log_header_buffer()
 ***************************/

/****************************************************************
 *  fbe_parity_get_fruts_write_log_header()
 ****************************************************************
 *  @brief
 *    Get a pointer to the write log header in the first element of a fruts.
 *
 * @param fruts_p - the single fruts to act on
 *        header_pp - pointer to a header pointer
 *                    set to the header assumed to be in the first sg_list element
 *                    regardless of the fruts_p->sg_element_offset
 *
 * @return
 *    status - FBE_STATUS_OK if function succeeded, else error
 *
 *  @author
 *    06/20/2012 - Created. Dave Agans
 *
 ****************************************************************/
fbe_status_t fbe_parity_get_fruts_write_log_header(fbe_raid_fruts_t * fruts_p,
                                                   fbe_parity_write_log_header_t ** header_pp)
{
    fbe_sg_element_t *sg_element;

    if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)fruts_p->sg_id))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid header invalid for fruts 0x%llx, sg_id: 0x%llx\n",
                             (unsigned long long)fruts_p, (unsigned long long)fruts_p->sg_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    sg_element = (fbe_sg_element_t *)fbe_raid_memory_id_get_memory_ptr(fruts_p->sg_id);
    *header_pp = (fbe_parity_write_log_header_t *)sg_element->address;

    return FBE_STATUS_OK;
}
/***************************
 * fbe_parity_get_fruts_write_log_header()
 ***************************/

/****************************************************************
 *  fbe_parity_write_log_send_flush_abandoned_event()
 ****************************************************************
 *  @brief
 *    Send a flush abandoned event to the event log with appropriate params.
 *
 * @param siots_p - the siots for the flush
 *        err_pos_bitmap - bitmap of bad positions that will be displayed
 *        error_info - "standard" crc error info
 *        algorithm - flush phase, either RG_WRITE_LOG_HDR_RD or RG_FLUSH_JOURNAL
 *
 * @return
 *    status - FBE_STATUS_OK if function succeeded, else error
 *
 *  @author
 *    08/02/2012 - Created. Dave Agans
 *
 ****************************************************************/
fbe_status_t fbe_parity_write_log_send_flush_abandoned_event(fbe_raid_siots_t * siots_p,
                                                             fbe_u32_t err_pos_bitmap,
                                                             fbe_u32_t error_info,
                                                             fbe_raid_siots_algorithm_e algorithm)
{
    fbe_status_t status;
    fbe_parity_write_log_header_t *header_p;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u64_t timestamp_sec = 0;
    fbe_u32_t timestamp_usec = 0;
    fbe_u32_t slot_id;

    fbe_raid_siots_get_journal_slot_id(siots_p, &slot_id);

    if (algorithm == RG_FLUSH_JOURNAL)
    {
        status = fbe_parity_write_log_get_iots_memory_header(siots_p, &header_p);
        if (status != FBE_STATUS_OK)
        {
            /* This status is not expected.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "write_log flush: %s get iots memory header failed 0x%x\n", 
                                                __FUNCTION__, status);
            return (status);
        }
        timestamp_sec = header_p->header_timestamp.sec;
        timestamp_usec = (fbe_u32_t)header_p->header_timestamp.usec;
    }

    status = fbe_event_log_write(SEP_INFO_RAID_PARITY_WRITE_LOG_FLUSH_ABANDONED,
                                 NULL, 0, "%x %x %llx %llx %x %x %x %llx %x", 
                                 raid_geometry_p->object_id,
                                 err_pos_bitmap,
                                 siots_p->start_lba,
                                 siots_p->xfer_count,
                                 fbe_raid_error_info(error_info),
                                 fbe_raid_extra_info_alg(siots_p, algorithm),
                                 slot_id,
                                 timestamp_sec,
                                 timestamp_usec);

    return (status);
}

/******************************************************
 * fbe_parity_write_log_send_flush_abandoned_event()
 ******************************************************/

/* Below functions are moved from parity_main.c */

/*!***************************************************************
 * fbe_parity_write_log_init()
 ****************************************************************
 * @brief
 *  This function is called to initialize write log structure.
 *
 * @return fbe_status_t
 *  STATUS of init operation.
 *
 * @author
 *  02/22/2012 - Created. Vamsi V
 *
 ****************************************************************/
fbe_status_t 
fbe_parity_write_log_init(fbe_parity_write_log_info_t * write_log_info_p)
{
    int slot_idx;
    write_log_info_p->start_lba = 0;
    write_log_info_p->slot_size = 0;
    write_log_info_p->slot_count = 0;
    write_log_info_p->flags = 0;

    fbe_queue_init(&write_log_info_p->request_queue_head);
    fbe_spinlock_init(&write_log_info_p->spinlock);

    /* init the max slots, in case this is a normal raid group, bandwidth will have fewer */
    for (slot_idx = 0; slot_idx < FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_NORM; slot_idx++)
    {
        write_log_info_p->slot_array[slot_idx].state = FBE_PARITY_WRITE_LOG_SLOT_STATE_FREE;
        write_log_info_p->slot_array[slot_idx].invalidate_state = FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_SUCCESS;
    }

    return FBE_STATUS_OK;
}
/* end fbe_parity_write_log_init */

/*!***************************************************************
 * fbe_parity_write_log_destroy()
 ****************************************************************
 * @brief
 *  This function is called to destroy write log structure.
 *
 * @return fbe_status_t
 *  STATUS of destroy operation.
 *
 * @author
 *  02/22/2012 - Created. Vamsi V
 *
 ****************************************************************/
fbe_status_t 
fbe_parity_write_log_destroy(fbe_parity_write_log_info_t * write_log_info_p)
{
    fbe_queue_destroy(&write_log_info_p->request_queue_head);
    fbe_spinlock_destroy(&write_log_info_p->spinlock);

    return FBE_STATUS_OK;
}
/* end fbe_parity_write_log_init */

/*!***************************************************************
 * fbe_parity_write_log_get_allocated_slot()
 *****************************************************************
 * @brief
 *  This function is called to get an allocated journal slot. If no
 *  slots are allocated, returns INVALID slot. 
 *
 * @return fbe_u32_t
 *  Slot number that is allocated to this request. Invalid slot
 * number indicates, currently no slots are allocated
 *
 * @author
 *  02/24/2012 - Created. Dave Agans
 *
 ****************************************************************/
fbe_u32_t fbe_parity_write_log_get_allocated_slot(fbe_parity_write_log_info_t * write_log_info_p)
{
    fbe_u32_t slot_start, slot_end, slot_idx;

	/* Loop over all the slots -- slots belonging to both SPs */
	slot_start = 0;
	slot_end = write_log_info_p->slot_count;

    fbe_spinlock_lock(&write_log_info_p->spinlock);
    for(slot_idx = slot_start; slot_idx < slot_end; slot_idx++)
    {
        if(write_log_info_p->slot_array[slot_idx].state == FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_FLUSH)
        {
            write_log_info_p->slot_array[slot_idx].state = FBE_PARITY_WRITE_LOG_SLOT_STATE_FLUSHING;
            fbe_spinlock_unlock(&write_log_info_p->spinlock);
            return slot_idx;
        }
    }

    fbe_spinlock_unlock(&write_log_info_p->spinlock);
    return FBE_PARITY_WRITE_LOG_INVALID_SLOT;
}
/* end fbe_parity_write_log_get_allocated_slot */

/*!***************************************************************
 * fbe_parity_write_log_set_header_invalid()
 *****************************************************************
 * @brief
 *  This function is called to set header buffer invalid 
 *
 * @param header_p - Pointer to header buffer
 *
 * @return None
 *
 * @author
 *  03/06/2012 - Created. Vamsi V.
 *
 ****************************************************************/
void fbe_parity_write_log_set_header_invalid(fbe_parity_write_log_header_t * header_p)
{
    /* Set the header version and state only, leave everything else for debugging audit trail
     */
    header_p->header_version = FBE_PARITY_WRITE_LOG_CURRENT_HEADER_VERSION;
    header_p->header_state = FBE_PARITY_WRITE_LOG_HEADER_STATE_INVALID;

    return;
}
/* end fbe_parity_write_log_set_header_invalid */

/*!***************************************************************
 * fbe_parity_write_log_is_header_valid()
 *****************************************************************
 * @brief
 *  This function checks a header for a valid version and valid state 
 *
 * @param header_p - Pointer to header buffer
 *
 * @return FBE_TRUE if header if valid, false otherwise
 *
 * @author
 *  06/18/2012 - Created. Dave Agans.
 ****************************************************************/
fbe_bool_t fbe_parity_write_log_is_header_valid(fbe_parity_write_log_header_t * header_p)
{
    /* Set the header state only, leave everything else for debugging audit trail
     */
    /*!@note - When there is more than one version, need to deal with any valid version 
     */
    if (   header_p->header_version == FBE_PARITY_WRITE_LOG_CURRENT_HEADER_VERSION
        && header_p->header_state == FBE_PARITY_WRITE_LOG_HEADER_STATE_VALID)
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}
/* end fbe_parity_write_log_set_header_invalid */

/*!***************************************************************
 * fbe_parity_write_log_set_all_slots_valid()
 *****************************************************************
 * @brief
 *  This function is called to set all slots as valid in memory.
 *  Monitor would then select each valid slot and send it to Flush SM.
 *  Flush SM would load Headers from disk to determine if the slot is
 *  infact valid and if so flushes it. When its done, Flush SM would 
 *  set the slot as invalid both in-memory and on the disk.
 *  Its called when RG is in specialze state.
 *
 * @param write_log_info_p - pointer to in memory write_log struc
 * @param both_sp_slots - Bool which indicates if slots for both SPs
 *                        need to set as Valid.
 *
 * @return None
 *
 * @author
 *  03/13/2012 - Created. Vamsi V.
 *
 ****************************************************************/
void fbe_parity_write_log_set_all_slots_valid(fbe_parity_write_log_info_t * write_log_info_p, fbe_bool_t both_sp_slots)
{
    fbe_u32_t slot_start, slot_end, slot_idx;
    fbe_cmi_sp_id_t this_sp_id = FBE_CMI_SP_ID_INVALID; 

    if (both_sp_slots)
    {
        /* Select all slots */
        slot_start = 0;
        slot_end = write_log_info_p->slot_count;

        /* Init pending requests queue (if there is any junk there from before RG went shutdown) */
        fbe_queue_init(&write_log_info_p->request_queue_head);

        /* Init quiesced flag */
        fbe_parity_write_log_clear_flag(write_log_info_p, FBE_PARITY_WRITE_LOG_FLAGS_QUIESCE);
    }
    else
    {
        /* This is the case of flushing slots belonging to peerSP on contact lost.
         * Slots are evenly divided between SPA and SPB Determine slots belonging to peerSP.
         */
        fbe_cmi_get_sp_id(&this_sp_id);
        if (this_sp_id == FBE_CMI_SP_ID_B)
        {
            slot_start = 0;
            slot_end = write_log_info_p->slot_count/2;
        }
        else
        {
            slot_start = write_log_info_p->slot_count/2;
            slot_end = write_log_info_p->slot_count;
        }

        /* Dont reset request queue or quiesced flag as they are in use on active (this) side */
    }

    /* Acquire spinlock and set all slots as allocated */
    fbe_spinlock_lock(&write_log_info_p->spinlock);

    for (slot_idx = slot_start; slot_idx < slot_end; slot_idx++)
    {
        write_log_info_p->slot_array[slot_idx].state = FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_FLUSH;
    }

    /* Release spinlock and return */
    fbe_spinlock_unlock(&write_log_info_p->spinlock);

    return;
}
/* end fbe_parity_write_log_set_all_slots_valid */

/*!***************************************************************
 * fbe_parity_write_log_set_all_slots_free()
 *****************************************************************
 * @brief
 *  This function is called to set all slots as free in memory.
 *  Its called when RG is in specialize state.
 *
 * @param write_log_info_p - pointer to in memory write_log struc 
 * @param both_sp_slots - Bool which indicates if slots for both SPs
 *                        need to set as Valid.
 *
 * @return None
 *
 * @author
 *  03/14/2012 - Created. Vamsi V.
 *
 ****************************************************************/
void fbe_parity_write_log_set_all_slots_free(fbe_parity_write_log_info_t * write_log_info_p, fbe_bool_t both_sp_slots)
{
    fbe_u32_t slot_start, slot_end, slot_idx;
    fbe_cmi_sp_id_t this_sp_id = FBE_CMI_SP_ID_INVALID; 

    if (both_sp_slots)
    {
        /* Select all slots */
        slot_start = 0;
        slot_end = write_log_info_p->slot_count;

        /* Init pending requests queue (if there is any junk there from before RG went shutdown) */
        fbe_queue_init(&write_log_info_p->request_queue_head);

        /* Init quiesced flag */
        fbe_parity_write_log_clear_flag(write_log_info_p, FBE_PARITY_WRITE_LOG_FLAGS_QUIESCE);
    }
    else
    {
        /* Slots are evenly divided between SPA and SPB 
         * Determine slots belonging to this SP.
         */
        fbe_cmi_get_sp_id(&this_sp_id);
        if (this_sp_id == FBE_CMI_SP_ID_A)
        {
            slot_start = 0;
            slot_end = write_log_info_p->slot_count/2;
        }
        else
        {
            slot_start = write_log_info_p->slot_count/2;
            slot_end = write_log_info_p->slot_count;
        }

        /* Dont reset request queue or quiesced flag as they are in use on active (this) side */
    }

    /* Acquire spinlock and set all slots as allocated */
    fbe_spinlock_lock(&write_log_info_p->spinlock);

    for (slot_idx = slot_start; slot_idx < slot_end; slot_idx++)
    {
        write_log_info_p->slot_array[slot_idx].state = FBE_PARITY_WRITE_LOG_SLOT_STATE_FREE;
        write_log_info_p->slot_array[slot_idx].invalidate_state = FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_SUCCESS;
    }

    /* Release spinlock and return */
    fbe_spinlock_unlock(&write_log_info_p->spinlock);

    return;
}
/* end fbe_parity_write_log_set_all_slots_free */

/* Accessor function */
void fbe_parity_write_log_set_slot_allocated(fbe_parity_write_log_info_t * write_log_info_p, fbe_u32_t slot_idx)
{
	write_log_info_p->slot_array[slot_idx].state = FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_FLUSH; 
}

/* Accessor function */
void fbe_parity_write_log_set_slot_needs_remap(fbe_parity_write_log_info_t * write_log_info_p, fbe_u32_t slot_idx)
{
    /* Note: this function is not lock protected. Its ok in current implementation. */
    //fbe_spinlock_lock(&write_log_info_p->spinlock);
	write_log_info_p->slot_array[slot_idx].state = FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_REMAP; 
    //fbe_spinlock_unlock(&write_log_info_p->spinlock);
}

/* Accessor function */
void fbe_parity_write_log_set_slot_free(fbe_parity_write_log_info_t * write_log_info_p, fbe_u32_t slot_idx)
{
    /* Note: this function is not lock protected. Its ok in current implementation. */
    //fbe_spinlock_lock(&write_log_info_p->spinlock);
	write_log_info_p->slot_array[slot_idx].state = FBE_PARITY_WRITE_LOG_SLOT_STATE_FREE;
	write_log_info_p->slot_array[slot_idx].invalidate_state = FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_SUCCESS;
    //fbe_spinlock_unlock(&write_log_info_p->spinlock);
}

/*!***************************************************************
 * fbe_parity_write_log_compare_timestamp()
 *****************************************************************
 * @brief
 *  This function is called to compare precise timestamps a and b.
 *
 * @param timestamp_a - pointer to precise timestamp 
 * @param timestamp_b - pointer to precise timestamp 
 *
 * @return  0 if timestamps are equal
 *         -1 if timestamp_a < timestamp_b
 *         +1 if timestamp_a > timestamp_b
 *
 * @author
 *  06/18/2012 - Created. Dave Agans
 *
 ****************************************************************/
fbe_s32_t fbe_parity_write_log_compare_timestamp(fbe_parity_write_log_header_timestamp_t *timestamp_a,
                                                 fbe_parity_write_log_header_timestamp_t *timestamp_b)
{
    fbe_s32_t result;

    if (timestamp_a->sec > timestamp_b->sec)
    {
        result = 1;
    }
    else if (timestamp_a->sec < timestamp_b->sec)
    {
        result = -1;
    }
    /* else the seconds are equal, check the u-seconds */
    else if (timestamp_a->usec > timestamp_b->usec)
    {
        result = 1;
    }
    else if (timestamp_a->usec < timestamp_b->usec)
    {
        result = -1;
    }
    else /* timestamps are equal */
    {
        result = 0;
    }
    return (result);
}

/*!***************************************************************
 * fbe_parity_write_log_get_remap_slot()
 *****************************************************************
 * @brief
 *  This function is called to get write_log slot that needs to be  
 *  checked for remap. If no slots need to be checked, returns 
 *  INVALID slot. 
 *  If needs_remap flag is set, it will first mark all
 *  the slots for Remap. needs_remap flag is set by flush operation when 
 *  it encounters media errors. All slots from all drive postions are
 *  checked for remap even if only one slot from one drive position
 *  encounters media errors.  
 *
 * @param write_log_info_p - pointer to in memory write_log struc
 * @param both_sp_slots - Bool which indicates if slots for both SPs
 *                        need to set as Valid.
 *
 * @return fbe_u32_t
 *  Slot number that is allocated to this request. Invalid slot
 * number indicates, currently no slots need to be checked.
 *
 * @author
 *  08/06/2012 - Created. Vamsi V.
 *
 ****************************************************************/
fbe_u32_t fbe_parity_write_log_get_remap_slot(fbe_parity_write_log_info_t * write_log_info_p, fbe_bool_t both_sp_slots)
{
    fbe_u32_t slot_start, slot_end, slot_idx;
    fbe_cmi_sp_id_t this_sp_id = FBE_CMI_SP_ID_INVALID; 

    if (both_sp_slots)
    {
        /* Select all slots */
        slot_start = 0;
        slot_end = write_log_info_p->slot_count;
    }
    else
    {
        /* Slots are evenly divided between SPA and SPB 
         * Determine slots belonging to this SP.
         */
        fbe_cmi_get_sp_id(&this_sp_id);
        if (this_sp_id == FBE_CMI_SP_ID_A)
        {
            slot_start = 0;
            slot_end = write_log_info_p->slot_count/2;
        }
        else
        {
            slot_start = write_log_info_p->slot_count/2;
            slot_end = write_log_info_p->slot_count;
        }
    }

    fbe_spinlock_lock(&write_log_info_p->spinlock);
    if (fbe_parity_write_log_is_flag_set(write_log_info_p, FBE_PARITY_WRITE_LOG_FLAGS_NEEDS_REMAP))
    {
        /* Set all slots as needing check for Remap. */
        for (slot_idx = slot_start; slot_idx < slot_end; slot_idx++)
        {
            write_log_info_p->slot_array[slot_idx].state = FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_REMAP;
            write_log_info_p->slot_array[slot_idx].invalidate_state = FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_SUCCESS;
        }
        fbe_parity_write_log_clear_flag(write_log_info_p, FBE_PARITY_WRITE_LOG_FLAGS_NEEDS_REMAP);
    }

    for (slot_idx = slot_start; slot_idx < slot_end; slot_idx++)
    {
        if (write_log_info_p->slot_array[slot_idx].state == FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_REMAP)
        {
            write_log_info_p->slot_array[slot_idx].state = FBE_PARITY_WRITE_LOG_SLOT_STATE_REMAPPING;
            fbe_spinlock_unlock(&write_log_info_p->spinlock);
            return slot_idx;
        }
    }

    fbe_spinlock_unlock(&write_log_info_p->spinlock);
    return FBE_PARITY_WRITE_LOG_INVALID_SLOT;
}
/* end fbe_parity_write_log_get_remap_slot */

/*!***************************************************************
 * fbe_parity_write_log_release_remap_slot()
 *****************************************************************
 * @brief
 *  This function is used to release write log slot marked as `REMAPPING'.
 *  First check if any request is pending for journal slots, 
 *     -Yes: Initiate it with this slot.
 *     -No: Simply set the slot state as free.
 *
 * @param write_log_info_p  - pointer to in memory write_log struc
 * @param slot_idx          - Slot indexed to release
 *
 * @return None.
 *
 * @author
 *  11/11/2013 - Created. Wenxuan Yin.
 *
 ****************************************************************/
void fbe_parity_write_log_release_remap_slot(fbe_parity_write_log_info_t * write_log_info_p, 
                                                               fbe_u32_t slot_idx)
{
    fbe_queue_element_t         *siots_q_element;
    fbe_raid_siots_t            *pending_siots_p;
    fbe_packet_t                *packet_p;
    fbe_raid_fruts_t            *fruts_p;
    
    /* Acquire spinlock */
    fbe_spinlock_lock(&write_log_info_p->spinlock);

    /* Check for any pending siots on waiting queue */
    siots_q_element = fbe_queue_pop(&write_log_info_p->request_queue_head);

    if (siots_q_element)
    {
        /* Allocate slot to the pending Siots */
        pending_siots_p = fbe_parity_journal_q_elem_to_siots(siots_q_element);
        fbe_parity_write_log_set_slot_state(write_log_info_p, slot_idx, FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED);
        fbe_parity_write_log_set_slot_inv_state(write_log_info_p, slot_idx, FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_SUCCESS);
        pending_siots_p->journal_slot_id = slot_idx;

        /* Set and clear related flags */
        fbe_raid_siots_set_flag(pending_siots_p, FBE_RAID_SIOTS_FLAG_JOURNAL_SLOT_ALLOCATED);
        fbe_raid_siots_clear_flag(pending_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_WRITE_LOG_SLOT);

        /* Release spinlock before starting the siots */
        fbe_spinlock_unlock(&write_log_info_p->spinlock);

        /* Get the first write fruts and the packet associated with it,
         * and use that to requeue the pending siots to the correct core.
         */
        fbe_raid_siots_get_write_fruts(pending_siots_p, &fruts_p);
        packet_p = fbe_raid_fruts_get_packet(fruts_p);
        fbe_transport_run_queue_push_packet(packet_p, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
    }
    else
    {
        /* Mark slot as free */
        fbe_parity_write_log_set_slot_free(write_log_info_p, slot_idx);

        /* Release spinlock */
        fbe_spinlock_unlock(&write_log_info_p->spinlock);
    }

    return;
}
/* end fbe_parity_write_log_release_remap_slot */

/***************************************************
 * end fbe_parity_journal_flush_util.c
 ***************************************************/
