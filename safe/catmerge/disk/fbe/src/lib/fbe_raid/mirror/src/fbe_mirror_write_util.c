/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_mirror_write_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions required for the mirror write
 *  processing.
 *
 * @version
 *   9/03/2009  Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_raid_library.h"
#include "fbe_mirror_io_private.h"
#include "fbe_raid_fruts.h"

/*************************
 *   FORWARD FUNCTION DEFINITIONS
 *************************/


/*!***************************************************************************
 *          fbe_mirror_write_setup_fruts()
 *****************************************************************************
 *
 * @brief   This method initializes the fruts for a mirror write request.
 *          All positions in the raid group are configured as a write but
 *          but we may need to allocate a pre-read fruts also.
 *
 * @param   siots_p    - ptr to the fbe_raid_siots_t
 * @param   preread_info_p - Array of information about fruts 
 * @param   write_info_p - Array of information about fruts 
 * @param   requested_opcode - Opcode to initialize fruts with
 * @param   memory_info_p - Pointer to the memory request information
 *
 * @return fbe_status_t
 *
 * @author
 *  12/15/2009  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t fbe_mirror_write_setup_fruts(fbe_raid_siots_t * siots_p, 
                                                 fbe_raid_fru_info_t *preread_info_p,
                                                 fbe_raid_fru_info_t *write_info_p,
                                                 fbe_payload_block_operation_opcode_t requested_opcode,
                                                 fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_fruts_t           *read_fruts_p = NULL;
    fbe_raid_fruts_t           *write_fruts_p = NULL;
    fbe_u32_t                   index;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);

    /* Determine if this is a write that require a pre-read.
     */
    for (index = 0; index < width; index++) {
        /* If there is a pre-read required.
         */
        if (preread_info_p->lba != FBE_LBA_INVALID) {
            /* Validate we have populated the fru info.
             */
            if ((read_fruts_p != NULL)                              ||
                (preread_info_p->blocks == 0)                       ||
                (preread_info_p->sg_index >= FBE_RAID_SG_INDEX_MAX)    ) {
                /* Pre-read fru info is bad 
                 */
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: write setup fruts second pre-read: %d lba: 0x%llx blks: 0x%llx sg: %d\n",
                                     (read_fruts_p != NULL) ? FBE_TRUE : FBE_FALSE,
                                     (unsigned long long)preread_info_p->lba,
                                     (unsigned long long)preread_info_p->blocks,
                                     preread_info_p->sg_index);
                return(FBE_STATUS_GENERIC_FAILURE);
            }

            /* Setup the pre-read fruts.
             */
            read_fruts_p = fbe_raid_siots_get_next_fruts(siots_p,
                                                         &siots_p->read_fruts_head,
                                                         memory_info_p);
              
            /* If the setup failed something is wrong.
             */
            if (read_fruts_p == NULL) {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: write setup fruts siots: 0x%llx 0x%llx for position: %d failed \n",
                                     (unsigned long long)siots_p->start_lba,
                                     (unsigned long long)siots_p->xfer_count,
                                     preread_info_p->position);
                return(FBE_STATUS_GENERIC_FAILURE);
            }

            /* Now initialize the read fruts from the pre-read info
             */
            status = fbe_raid_fruts_init_fruts(read_fruts_p,
                                               FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                               preread_info_p->lba,
                                               preread_info_p->blocks,
                                               preread_info_p->position);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {  
                fbe_raid_siots_trace(siots_p, 
                                    FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "mirror: failed in read fruts initialization for siots_p 0x%p, status 0x%x\n",
                                    siots_p, status);
                return(status);
            }
        }

        /* Goto next.
         */
        preread_info_p++;
    }

    /* Initialize the write fruts.
     */
    for (index = 0; index < width; index++) {
        /* Else initialize the write fruts from the fru info.
         */
        write_fruts_p = fbe_raid_siots_get_next_fruts(siots_p,
                                                      &siots_p->write_fruts_head,
                                                      memory_info_p);
                
        /* If the setup failed something is wrong.
         */
        if (write_fruts_p == NULL) {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: write setup fruts siots: 0x%llx 0x%llx for position: %d failed \n",
                                 (unsigned long long)siots_p->start_lba,
                                 (unsigned long long)siots_p->xfer_count,
                                 write_info_p->position);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* Now initialize the write fruts and set the opcode to the
         * requested value.
         */
        status = fbe_raid_fruts_init_fruts(write_fruts_p,
                                  requested_opcode,
                                  write_info_p->lba,
                                  write_info_p->blocks,
                                  write_info_p->position);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {  
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: failed in write fruts intialization for siots_p = 0x%p, "
                                 "status = 0x%x\n",
                                 siots_p, status);
            return(status);
        }

        /* This original write size will be needed later when we do the XOR.
         */
        write_fruts_p->write_preread_desc.lba = write_info_p->lba;
        write_fruts_p->write_preread_desc.block_count = write_info_p->blocks;
        
        /* Goto the next fruts info.
         */
        write_info_p++;
    }

    /* Always return the execution status.
     */
    return(status);
}
/* end of fbe_mirror_write_setup_fruts() */

/*!**************************************************************************
 *          fbe_mirror_write_get_num_preread_fruts()
 ****************************************************************************
 *
 * @brief   This function determines the number of pre-read fruts simply by
 *          the setting of `unaligned request' in the siots.
 *
 * @param   siots_p - Pointer to siots to check for unaligne request
 * 
 * @return  num_of_preread_fruts - Either 0 or 1
 *
 * @author
 *  02/25/2010  Ron Proulx  - Created.
 *
 **************************************************************************/
static __forceinline fbe_u32_t fbe_mirror_write_get_num_preread_fruts(fbe_raid_siots_t *siots_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* If the raid group contains any 4K drives assuem there is a pre
     */
    if (fbe_raid_geometry_needs_alignment(raid_geometry_p)) {
        return(1);
    }
    
    /* Else no pre-read is required.
     */
    return(0);
}
/* end of fbe_mirror_write_get_num_preread_fruts() */

/*!***************************************************************************
 *          fbe_mirror_write_get_unaligned_fru_info()
 *****************************************************************************
 * 
 * @brief   This function initializes the fru info array for an unaligned 
 *          mirror write request.  Although we don't need to write to disabled 
 *          positions, the current algorithms still process the fruts for those 
 *          positions. Therefore we initialize and configure write fruts for every
 *          position in the mirror raid group.  We only configure the sg_index
 *          for the pre-read fruts if required.  Except for very large requests
 *          the host offset will be zero.
 *
 * @param   siots_p - Current sub request.
 * @param   read_info_p - Pointer to array of fru info to populate
 * @param   write_info_p - Pointer to array of fru info to populate
 * @param   num_sgs_p - Array sgs indexes to populate
 * @param   b_log - If FBE_TRUE any error is unexpected
 *                  If FBE_FALSE we are determining if the siots should be split
 * @param   read_lba - Aligned pre-read start lba
 * @param   read_blocks - Aligned pre-read blocks
 *
 * @return  status - FBE_STATUS_OK - read fruts information successfully generated
 *                   FBE_STATUS_INSUFFICIENT_RESOURCES - Request is too large
 *                   other - Unexpected failure
 *
 * @author
 *  03/27/2014  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_write_get_unaligned_fru_info(fbe_raid_siots_t * siots_p,
                                                            fbe_raid_fru_info_t *read_info_p,
                                                            fbe_raid_fru_info_t *write_info_p,
                                                            fbe_u16_t *num_sgs_p,
                                                            fbe_bool_t b_log,
                                                            fbe_lba_t read_lba,
                                                            fbe_block_count_t read_blocks)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t           index;
    fbe_raid_fru_info_t *read_position_info_p = NULL;
    fbe_u32_t           read_position = fbe_mirror_get_primary_position(siots_p);
    fbe_sg_element_with_offset_t host_sg_desc;
    fbe_block_count_t   host_blks_remaining = siots_p->data_page_size;
    fbe_u32_t           host_sg_count = 0;
    fbe_block_size_t    bytes_per_blk; 
    fbe_block_count_t   blks_remaining = siots_p->data_page_size;
    fbe_u32_t           read_sg_count = 0;
    fbe_raid_position_bitmask_t bitmask_4k;

    /* The pre-read will be planted first.  Therefore start with pre-read fruts.
     */
    blks_remaining = fbe_raid_sg_count_uniform_blocks(read_blocks,
                                                      siots_p->data_page_size,
                                                      blks_remaining,
                                                      &read_sg_count);

    /* Setup the pre-read fruts.
     */
    index = fbe_mirror_get_index_from_position(siots_p, read_position);
    read_position_info_p = &read_info_p[index];
    read_position_info_p->position = read_position;
    read_position_info_p->lba = read_lba;
    read_position_info_p->blocks = read_blocks;
    status = fbe_raid_fru_info_set_sg_index(read_position_info_p,
                                            read_sg_count,
                                            b_log);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: write get unaligned failed to set read sg index status: 0x%x\n",
                             status);
        return status;
    }

    /* Increment the sg type count.
     */
    num_sgs_p[read_position_info_p->sg_index]++;

    /* If this is not a buffered operation we will the data supplied by cache.
     */
    if (!fbe_raid_siots_is_buffered_op(siots_p)) {
        /* Use the host descriptor  to determine how many sgs are required.
         * We will simply add more (non-uniform) for the pre-read and/or 
         * post-read data.
         */
        status = fbe_raid_siots_setup_sgd(siots_p, &host_sg_desc);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: write get unaligned failed to get host buffer status: 0x%x\n",
                                 status);
            return status;
        }
        fbe_raid_adjust_sg_desc(&host_sg_desc);
        bytes_per_blk = FBE_RAID_SECTOR_SIZE(host_sg_desc.sg_element->address);
        host_blks_remaining = host_sg_desc.sg_element->count / bytes_per_blk;
        status = fbe_raid_sg_count_nonuniform_blocks(siots_p->parity_count,
                                                     &host_sg_desc,
                                                     bytes_per_blk,
                                                     &host_blks_remaining,
                                                     &host_sg_count);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {  
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: write get unaligned failed to count host sgs status: 0x%x\n",
                                 status);
            return status;
        }
    }

    /* Get the bitmask of those positions that need to be aligned.
     */
    fbe_raid_geometry_get_4k_bitmask(raid_geometry_p, &bitmask_4k);

    /* Now populate the writes.  Yes we even populate for degraded positions.
     */
    for (index = 0; index < width; index++) {
        /* Setup all write positions.
         */
        write_info_p->position = fbe_mirror_get_position_from_index(siots_p, index);

        /* If it is a buffered operation we will use the allocated blocks.  Use the
         * standard sg count starting after the pre-read or last write buffer.
         */
        if (fbe_raid_siots_is_buffered_op(siots_p)) {
            blks_remaining = fbe_raid_sg_count_uniform_blocks(siots_p->parity_count,
                                                              siots_p->data_page_size,
                                                              blks_remaining,
                                                              &host_sg_count);
        }
    
        /* If this position is unaligned use the host sg count plus `extra' for
         * any split buffers.
         */
        if ((1 << write_info_p->position) & bitmask_4k) {
            /* Simply add extra for alignment.
             */
            write_info_p->lba = read_lba;
            write_info_p->blocks = read_blocks;
            status = fbe_raid_fru_info_set_sg_index(write_info_p,
                                                    (read_sg_count + host_sg_count + FBE_RAID_EXTRA_ALIGNMENT_SGS),
                                                    b_log);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: write get unaligned failed to set write sg index status: 0x%x\n",
                             status);
                return status;
            }
        } else {
            /* Standard block size, use siots data.
             */
            write_info_p->lba = siots_p->parity_start;
            write_info_p->blocks = siots_p->parity_count;

            /* Set sg index returns a status.  The reason is that the sg_count
             * may exceed the maximum sgl length.  Due to the fact that the request
             * may have been changed (to keep the pre,post read within the max
             * per-drive limit) we need (1) additional sg in case the last buffer
             * is split.
             */
            status = fbe_raid_fru_info_set_sg_index(write_info_p, (host_sg_count + 1), b_log);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
                fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: write get unaligned failed to set write pos: %d sg indexstatus: 0x%x\n",
                             write_info_p->position, status);
                return status;
            }
        }
        num_sgs_p[write_info_p->sg_index]++;

        /* Clear the pre-read data for all positions except the pre-read position.
         */
        if (write_info_p->position != read_position) {
            /* All other read positions are set to invalid
             */
            read_info_p->lba = FBE_LBA_INVALID;
            read_info_p->blocks = 0;
            read_info_p->sg_index = FBE_RAID_SG_INDEX_INVALID;
        }

        /* Goto next fruts info.
         */
        read_info_p++;
        write_info_p++;
    }

    /* Always return the execution status.
     */
    return(status);
}
/***********************************************
 * end fbe_mirror_write_get_unaligned_fru_info()
 ***********************************************/

/*!***************************************************************************
 *          fbe_mirror_write_get_fru_info()
 *****************************************************************************
 * 
 * @brief   This function initializes the fru info array for a mirror write
 *          request.  Although we don't need to write to disabled positions,
 *          the current algorithms still process the fruts for those positions.
 *          Therefore we initialize and configure write fruts for every
 *          position in the mirror raid group.  We only configure the sg_index
 *          for the pre-read fruts if required.  Except for very large requests
 *          the host offset will be zero.
 *
 * @param   siots_p - Current sub request.
 * @param   preread_info_p - Pointer to array of fru info to populate
 * @param   write_info_p - Pointer to array of fru info to populate
 * @param   num_sgs_p - Array sgs indexes to populate
 * @param   b_log - If FBE_TRUE any error is unexpected
 *                  If FBE_FALSE we are determining if the siots should be split
 *
 * @return  status - FBE_STATUS_OK - read fruts information successfully generated
 *                   FBE_STATUS_INSUFFICIENT_RESOURCES - Request is too large
 *                   other - Unexpected failure
 *
 * @note    We could allocate a use a single write sgl for large (i.e. non-zero
 *          host offset) writes.  This is an optimization.
 *
 * @author
 *  02/10/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_write_get_fru_info(fbe_raid_siots_t * siots_p,
                                           fbe_raid_fru_info_t *preread_info_p,
                                           fbe_raid_fru_info_t *write_info_p,
                                           fbe_u16_t *num_sgs_p,
                                           fbe_bool_t b_log)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t           index;
    fbe_block_count_t   blks_remaining = siots_p->data_page_size;
    fbe_u32_t           sg_count = 0;
     
    /* Page size must be set.
     */
    if (!fbe_raid_siots_validate_page_size(siots_p)) {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: write get fruts info - page size not valid \n");
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    
    /* If this is an unaligned write request we must count the sgs for the
     * pre-read prior to counting the writes.
     */
    if (fbe_raid_geometry_needs_alignment(raid_geometry_p)) {
        fbe_lba_t           preread_lba = siots_p->parity_start;
        fbe_block_count_t   preread_blocks = siots_p->parity_count;

        /* Align the request if neccessary.
         */
        fbe_raid_geometry_align_io(raid_geometry_p, &preread_lba, &preread_blocks);
        if ((preread_lba != siots_p->parity_start)    ||
            (preread_blocks != siots_p->parity_count)    ) {
            /* Invoke the method to generate an unaligned write fru info.
             */
            status = fbe_mirror_write_get_unaligned_fru_info(siots_p,
                                                             preread_info_p,
                                                             write_info_p,
                                                             num_sgs_p,
                                                             b_log,
                                                             preread_lba,
                                                             preread_blocks);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {  
                fbe_raid_siots_trace(siots_p,
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: write get unaligned fru info failed - status: 0x%x\n",
                                     status);
            }
            return(status);
        }
    }

    /* We cannot use the parent's sg simply because the PVD does not expect to get an sg list that 
     * has more data in it.  In some cases of ZOD, the PVD needs to use the pre/post SGs in the payload, thus the 
     * host SG must have exactly the amount of data in it specified by the client. 
     */
    if (!fbe_raid_siots_is_buffered_op(siots_p)) {
        status = fbe_mirror_count_sgs_with_offset(siots_p,
                                                  &sg_count);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {  
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: failed to count sgs with offset with status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return (status); 
        }
    }

    /* Populate all pre-read and write positions with either valid data or
     * initialized
     */
    for (index = 0; index < width; index++) {
        /* Setup all write positions.
         */
        write_info_p->lba = siots_p->parity_start;
        write_info_p->blocks = siots_p->parity_count;
        write_info_p->position = fbe_mirror_get_position_from_index(siots_p, index);

        /*! If it is a buffered (i.e. zero) operation we must handle split 
         *  buffers because each position has a unique buffer.
         *
         *  @note Although the number of unaligned zero request should be small,
         *        we could up the resource allocation code to us the same sg and buffer.
         */
        if (fbe_raid_siots_is_buffered_op(siots_p)) {
            sg_count = 0;
            blks_remaining = fbe_raid_sg_count_uniform_blocks(write_info_p->blocks,
                                                              siots_p->data_page_size,
                                                              blks_remaining,
                                                              &sg_count);
        }
        
        /* Only if the sg count is non-zero
         */
        if (sg_count != 0) {
            /* Set sg index returns a status.  The reason is that the sg_count
             * may exceed the maximum sgl length.
             */
            status = fbe_raid_fru_info_set_sg_index(write_info_p, sg_count, b_log);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
                 fbe_raid_siots_trace(siots_p,
                                      FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                      "mirror: failed to set sg index: status = 0x%x, siots_p = 0x%p, "
                                      "write_info_p = 0x%p\n", 
                                      status, siots_p, write_info_p);

                 return  status;
            } else {
                /* Else increment the sg type count.
                 */
                num_sgs_p[write_info_p->sg_index]++;
            }
        } else {
            /* Else there was no data.
             */
            write_info_p->sg_index = FBE_RAID_SG_INDEX_INVALID;
        }

        /* All read positions are set to invalid
         */
        preread_info_p->lba = FBE_LBA_INVALID;
        preread_info_p->blocks = 0;
        preread_info_p->sg_index = FBE_RAID_SG_INDEX_INVALID;

        /* Goto next fruts info.
         */
        preread_info_p++;
        write_info_p++;
    }

    /* Always return the execution status.
     */
    return(status);
}
/**************************************
 * end fbe_mirror_write_get_fru_info()
 **************************************/

/*!**************************************************************************
 *          fbe_mirror_write_calculate_memory_size()
 ****************************************************************************
 *
 * @brief   This function calculates the total memory usage of the siots for
 *          this state machine.
 *
 * @param   siots_p - Pointer to siots that contains the allocated fruts
 * @param   preread_info_p - Pointer to array to populate for fruts information
 * @param   write_info_p - Pointer to array to populate for fruts information
 * 
 * @return  status - Typically FBE_STATUS_OK
 *                             Other - This shouldn't occur
 *
 * @author
 *  02/05/2010  Ron Proulx  - Created.
 *
 **************************************************************************/
fbe_status_t fbe_mirror_write_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                    fbe_raid_fru_info_t *preread_info_p,
                                                    fbe_raid_fru_info_t *write_info_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u16_t           num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_block_count_t   blocks_to_allocate = 0;
    fbe_u32_t           num_fruts;

    /* Zero the number of sgs by type
     */
    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));

    /* Buffered transfers needs blocks allocated.
     */
    if (fbe_raid_siots_is_buffered_op(siots_p)) {
        blocks_to_allocate += siots_p->parity_count * width;
    }
    num_fruts = siots_p->data_disks;

    /* If this is an unaligned write request flag it in the siots and
     * and get the pre-read range.  We need to allocate this amount.
     */
    if (fbe_raid_geometry_needs_alignment(raid_geometry_p)) {
        fbe_lba_t           preread_lba = siots_p->parity_start;
        fbe_block_count_t   preread_blocks = siots_p->parity_count;

        /* Align the request if neccessary.
         */
        fbe_raid_geometry_align_io(raid_geometry_p, &preread_lba, &preread_blocks);
        if ((preread_lba != siots_p->parity_start)    ||
            (preread_blocks != siots_p->parity_count)    ) {
            /* This is an unaligned request.
             */
            num_fruts += fbe_mirror_write_get_num_preread_fruts(siots_p);
            blocks_to_allocate += preread_blocks; 
        }
    }

    /* Setup the the blocks to allocate and the standard page size for the
     * allocation.  The first parameter to calculate page size is the number
     * of fruts which is the width of the raid group.
     */
    siots_p->total_blocks_to_allocate = blocks_to_allocate;
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts,
                                         siots_p->total_blocks_to_allocate);

    /* Now generate the fruts information including any pre-read.
     * We pass true for the log error since we should not fail.
     */
    status = fbe_mirror_write_get_fru_info(siots_p, 
                                           preread_info_p,
                                           write_info_p,
                                           &num_sgs[0],
                                           FBE_TRUE);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to get write fru info with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, &num_sgs[0],
                                                FBE_TRUE /* In-case recovery verify is required*/);
    /* Always return the execution status.
     */
    return(status);
}
/**********************************************
 * end fbe_mirror_write_calculate_memory_size()
 **********************************************/

/*!***************************************************************************
 * fbe_mirror_write_plant_sgs_for_buffered_op()
 *****************************************************************************
 * @brief Setup the sgs for a buffered operation.
 *
 * @param   siots_p - Pointer to siots that contains the allocated fruts
 * @param   memory_info_p - Pointer to memory request information
 * @param   write_info_p - Pointer to array of write fru infomation
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  3/9/2010 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_write_plant_sgs_for_buffered_op(fbe_raid_siots_t *siots_p,
                                                        fbe_raid_memory_info_t *memory_info_p,
                                                        fbe_raid_fru_info_t *write_info_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_fruts_t       *write_fruts_p = NULL;
    fbe_u32_t               width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t               write_fruts_count;
    fbe_sg_element_t       *write_sg_p = NULL;
    fbe_u32_t               index;
                              
    /* Get the write fruts chain and count
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    write_fruts_count = fbe_raid_siots_get_write_fruts_count(siots_p);

    /* Validate the write fruts count
     */
    if (write_fruts_count != width)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: write plant buffered sg siots lba: %llx blks: 0x%llx fruts count: 0x%x width: 0x%x\n",
                             (unsigned long long)siots_p->parity_start,
                 (unsigned long long)siots_p->parity_count,
                             write_fruts_count, width);
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    
    /*! Plant the buffers we allocated into the write fruts.
     *
     *  @note we could optimize this to only allocate an sg for (1) position and
     *        use it for all positions.
     */
    for (index = 0; index < width; index++)
    {
         fbe_u16_t sg_total = 0;
        fbe_u32_t dest_byte_count =0;
        /* Validate the write fruts
         */
        if (write_fruts_p == NULL)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: write plant buffered sg siots lba: %llx blks: 0x%llx fruts count: 0x%x width: 0x%x\n",
                                 (unsigned long long)siots_p->parity_start,
                 (unsigned long long)siots_p->parity_count,
                                 write_fruts_count, width);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* If we didn't allocate an sg something is wrong.
         */
        write_sg_p = fbe_raid_fruts_return_sg_ptr(write_fruts_p);
        if ( write_sg_p == NULL )
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror:  write plant buffered sg siots lba: %llx blks: 0x%llx position: %d no sg allocated\n",
                                 (unsigned long long)siots_p->parity_start,
                 (unsigned long long)siots_p->parity_count, 
                 write_fruts_p->position);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        if(!fbe_raid_get_checked_byte_count(write_fruts_p->blocks, &dest_byte_count))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "%s: byte count exceeding 32bit limit \n",
                                 __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        /* Put the memory into the fruts sg.
         */
        sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                                                     write_sg_p, 
                                                                                     dest_byte_count);
        if (sg_total == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "%s: failed to populate sg for: memory_info_p = 0x%p\n",
                                 __FUNCTION__,memory_info_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Goto next write info and write fruts.
         */
        write_info_p++;
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    
    } /* end for all write positions */

    /* Always return the execution status.
     */
    return(status);
}
/****************************************************
 * end of fbe_mirror_write_plant_sgs_for_buffered_op()
 ****************************************************/

/*!***************************************************************************
 *          fbe_mirror_write_plant_parent_sgs()
 *****************************************************************************
 *
 * @brief   This method plants the sgs associated with a mirror write where the
 *          buffers have been allocated by the parent (upstream object).
 *          We plant the parent buffers into all the write fruts.  We have
 *          already taken care of the pre-read sg (if any).
 *
 * @param   siots_p - Pointer to siots that contains the allocated fruts
 * @param   write_info_p - Pointer to array of write fru infomation
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  02/10/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_write_plant_parent_sgs(fbe_raid_siots_t *siots_p,
                                               fbe_raid_fru_info_t *write_info_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_fruts_t       *write_fruts_p = NULL;
    fbe_u32_t               width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t               write_fruts_count;
    fbe_sg_element_t       *write_sg_p = NULL;
    fbe_u32_t               index;
                              
    /* Get the write fruts chain and count
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    write_fruts_count = fbe_raid_siots_get_write_fruts_count(siots_p);

    /* Validate the write fruts count
     */
    if (write_fruts_count != width)
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: write plant parent sg siots lba: %llx blks: 0x%llx fruts count: 0x%x width: 0x%x\n",
                             (unsigned long long)siots_p->parity_start,
                 (unsigned long long)siots_p->parity_count,
                             write_fruts_count, width);
        return(FBE_STATUS_GENERIC_FAILURE);
    }
            
    /* Default to using the host buffer as is
     */
    fbe_raid_siots_get_sg_ptr(siots_p, &write_sg_p);

    /* Validate the sg was properly set up.
     */
    if (write_sg_p == NULL)
    {
        /* Report the error and fail the request.
         */
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: write plant parent sg Error planting fruts: %p with parent sg: %p\n",
                             write_fruts_p, write_sg_p);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /*! Walk thru all the write fruts and plant the host data into each
     *  write sgl. (Again we have already taken care of any pre-read)
     *
     *  @note we could use same sg for all positions (even when there is a host offset)
     */
    for (index = 0; index < width; index++)
    {
        /* Validate the write fruts
         */
        if (write_fruts_p == NULL)
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: write plant parent sg siots lba: %llx blks: 0x%llx fruts count: 0x%x width: 0x%x\n",
                                 (unsigned long long)siots_p->parity_start,
                 (unsigned long long)siots_p->parity_count,
                                 write_fruts_count, width);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* If we didn't allocate an sg something is wrong.
         */
        if ( (write_sg_p = fbe_raid_fruts_return_sg_ptr(write_fruts_p)) == NULL )
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror:  write plant parent sg siots lba: %llx blks: 0x%llx position: %d no sg allocated\n",
                                 (unsigned long long)siots_p->parity_start,
                 (unsigned long long)siots_p->parity_count,
                 write_fruts_p->position);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* Else plant the host data into the allocated sg.
         */
        if ( (status = fbe_mirror_plant_sg_with_offset(siots_p,
                                                       write_info_p,
                                                       write_sg_p))     != FBE_STATUS_OK )
        {
            /* Something is seriously wrong.
             */
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror:  write plant parent sg siots lba: %llx blks: 0x%llx position: %d plant failed\n",
                                 (unsigned long long)siots_p->parity_start,
                 (unsigned long long)siots_p->parity_count,
                 write_fruts_p->position);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* Goto next write info and write fruts.
         */
        write_info_p++;
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    
    } /* end for all write positions */

    /* Always return the execution status.
     */
    return(status);
}
/*******************************************
 * end of fbe_mirror_write_plant_parent_sgs()
 *******************************************/

/*!***************************************************************************
 *          fbe_mirror_write_plant_parent_sgs_for_unaligned()
 *****************************************************************************
 *
 * @brief   This method plants the sgs for an `unaligned' mirror write.  For
 *          positions that are aligned we populate the sg with the parent
 *          buffer information.  For unaligned positions we use the pre-read
 *          buffer.
 *
 * @param   siots_p - Pointer to siots that contains the allocated fruts
 * @param   write_info_p - Pointer to array of write fru infomation
 * @param   memory_info_p - Pointer to memory information for buffered requests
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  03/28/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_write_plant_parent_sgs_for_unaligned(fbe_raid_siots_t *siots_p,
                                                             fbe_raid_fru_info_t *write_info_p,
                                                             fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_position_bitmask_t unaligned_bitmask;
    fbe_raid_fruts_t       *read_fruts_p = NULL;
    fbe_raid_fruts_t       *write_fruts_p = NULL;
    fbe_u32_t               width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t               read_fruts_count;
    fbe_u32_t               write_fruts_count;
    fbe_sg_element_t       *write_sg_p = NULL;
    fbe_sg_element_t       *read_sg_p = NULL;
    fbe_lba_t               read_end_lba;
    fbe_lba_t               parity_end;
    fbe_block_size_t        bytes_per_block;
    fbe_u32_t               preread_blocks;
    fbe_u32_t               postread_blocks;
    fbe_u32_t               postread_offset;
    fbe_u32_t               buffered_byte_count;
    fbe_sg_element_with_offset_t read_sg_desc;
    fbe_sg_element_with_offset_t host_sg_desc;
    fbe_u16_t               num_sgs;
    fbe_u32_t               index;
      
    /* Get the `unaligned' position bitmask.  At least (1) position should
     * be unaligned.
     */
    fbe_raid_geometry_get_4k_bitmask(raid_geometry_p, &unaligned_bitmask);

    /* Get the pre-read fruts
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    read_fruts_count = fbe_raid_siots_get_read_fruts_count(siots_p);

    /* Validate the preread fruts count
     */
    if (read_fruts_count != 1) {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: write plant unaligned preread fruts count: 0x%x width: 0x%x\n",
                             read_fruts_count, width);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the pre-read sgl
     */
    read_sg_p = fbe_raid_fruts_return_sg_ptr(read_fruts_p);
    if (read_sg_p == NULL) {
        /* Report the error and fail the request.
         */
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: write plant unaligned preread fruts: %p with sg: %p\n",
                             read_fruts_p,  read_sg_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    bytes_per_block = FBE_RAID_SECTOR_SIZE(read_sg_p->address);

    /* The read will be allocated first.  We need to take into account
     * the host buffers that will b
     */
    read_end_lba = read_fruts_p->lba + read_fruts_p->blocks - 1;
    parity_end = siots_p->parity_start + siots_p->parity_count - 1;
    preread_blocks = (fbe_u32_t)(siots_p->parity_start - read_fruts_p->lba);
    postread_blocks = (fbe_u32_t)(read_end_lba - parity_end);

    /* Get the write fruts chain and count
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    write_fruts_count = fbe_raid_siots_get_write_fruts_count(siots_p);

    /* Validate the write fruts count
     */
    if (write_fruts_count != width) {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: write plant unaligned write fruts count: %d width: %d\n",
                             write_fruts_count, width);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Walk thru all the write fruts and determine if this position is
     * unaligned or not.
     *  o If this is an unaligned position copy the pre-read sgl to the
     *    write sgl.
     *  o If this write is aligned simply populate the write sgl with
     *    the parent buffer information.
     */
    for (index = 0; index < width; index++) {
        /* Validate the write fruts
         */
        if (write_fruts_p == NULL) {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: write plant unaligned index: %d write fruts is NULL\n",
                                 index);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* If we didn't allocate an sg something is wrong.
         */
        if ( (write_sg_p = fbe_raid_fruts_return_sg_ptr(write_fruts_p)) == NULL ) {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: write plant unaligned index: %d position: %d no sg\n",
                                 index, write_fruts_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* If this is an unaligned request then use the pre-read sg.
         */
        if ((1 << write_fruts_p->position) & unaligned_bitmask)  {
            /* If needed use the pre-read to populate the first portion of
             * the write sg.
             */
            if (preread_blocks != 0) {
                /* ******** <---- Read start
                 * *      *            /\
                 * *      * <--- This middle area needs to be added to the write.
                 * *      *            \/
                 * ******** <---- Write start
                 * *      *
                 * *      *
                 * The write lba at this point represents where the user data starts. 
                 * Our read starts before the new write data                  .
                 * We will need to write out these extra blocks to align to 4k. 
                 * Copy these extra blocks from the read to write sg. 
                 */
                fbe_sg_element_with_offset_init(&read_sg_desc, 0, read_sg_p, NULL);
                status = fbe_raid_sg_clip_sg_list(&read_sg_desc,
                                                  write_sg_p,
                                                  (preread_blocks * bytes_per_block),
                                                  &num_sgs);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
                    fbe_raid_siots_trace(siots_p,
                                         FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "mirror: write plant unaligned index: %d position: %d preread clip status: 0x%x\n",
                                         index, write_fruts_p->position, status);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* Increment the sg pointer for this position to the start of
                 * the host write.
                 */
                write_sg_p += num_sgs;

            } /* end if there are pre-read blocks*/

            /* If this is a buffered (i.e. zero) operation we must use the
             * allocated memory.
             */
            if (fbe_raid_siots_is_buffered_op(siots_p)) {
                /*! @note We use the original block count not the updated.
                 */
                if(!fbe_raid_get_checked_byte_count(siots_p->parity_count, &buffered_byte_count)) {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "%s: byte count exceeding 32bit limit \n",
                                         __FUNCTION__);
                    return FBE_STATUS_GENERIC_FAILURE; 
                }

                /* Put the memory into the fruts sg.
                 */
                num_sgs = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                           write_sg_p, 
                                                           buffered_byte_count);
                if (num_sgs == 0) {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "%s: failed to populate sg for: memory_info_p = 0x%p\n",
                                         __FUNCTION__,memory_info_p);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            } else {
                /* Now plant the host buffers.
                 */
                status = fbe_raid_siots_setup_cached_sgd(siots_p, &host_sg_desc);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
                    fbe_raid_siots_trace(siots_p,
                                         FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "mirror: write plant unaligned index: %d position: %d setup status: 0x%x\n",
                                         index, write_fruts_p->position, status);
                        return FBE_STATUS_GENERIC_FAILURE;
                }
                status = fbe_raid_sg_clip_sg_list(&host_sg_desc,
                                                  write_sg_p,
                                                  (fbe_u32_t)(siots_p->parity_count * bytes_per_block),
                                                  &num_sgs);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
                    fbe_raid_siots_trace(siots_p,
                                         FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "mirror: write plant unaligned index: %d position: %d host clip status: 0x%x\n",
                                         index, write_fruts_p->position, status);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }

            /* If needed use the pre-read to populate the last portion of
             * the write sg.
             */
            if (postread_blocks != 0) {
                /* Increment the sg pointer for this position to the start of
                 * the post-read data.
                 */
                write_sg_p += num_sgs;

                /* ******** <---- I/O start
                 * *      *
                 * *      *
                 * ******** <---- Write end
                 * *      *            /\
                 * *      * <--- This middle area needs to be added to the write.
                 * *      *            \/
                 * ******** <---- Read end 
                 *  
                 * The write lba at this point represents where the user data starts 
                 * Our read ends after the new write data ends. 
                 * We will need to write out these extra blocks to align to 4k 
                 * Copy these extra blocks from the read to write sg.
                 */
                postread_offset = (fbe_u32_t)(parity_end - read_fruts_p->lba + 1);
                postread_offset = (postread_offset * bytes_per_block);
                fbe_sg_element_with_offset_init(&read_sg_desc, postread_offset, read_sg_p, NULL);
                status = fbe_raid_sg_clip_sg_list(&read_sg_desc,
                                                  write_sg_p,
                                                  (postread_blocks * bytes_per_block),
                                                  &num_sgs);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
                    fbe_raid_siots_trace(siots_p,
                                         FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "mirror: write plant unaligned index: %d position: %d postread clip status: 0x%x\n",
                                         index, write_fruts_p->position, status);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

            } /* end if post-read blocks */

            /* we have to write the pre-read data to disk 
            */
            write_fruts_p->lba = read_fruts_p->lba;
            write_fruts_p->blocks = read_fruts_p->blocks;

        /* Else this is an aligned request. */
        } else if (fbe_raid_siots_is_buffered_op(siots_p)) {
            /* Buffered op check the byte count.
             */
            if(!fbe_raid_get_checked_byte_count(write_fruts_p->blocks, &buffered_byte_count)) {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "%s: byte count exceeding 32bit limit \n",
                                     __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE; 
            }

            /* Put the memory into the fruts sg.
             */
            num_sgs = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                       write_sg_p, 
                                                       buffered_byte_count);
            if (num_sgs == 0) {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "%s: failed to populate sg for: memory_info_p = 0x%p\n",
                                     __FUNCTION__,memory_info_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        } else {
            /* Else plant the host data into the allocated sg.
             */
            status = fbe_mirror_plant_sg_with_offset(siots_p,
                                                     write_info_p,
                                                     write_sg_p);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
                fbe_raid_siots_trace(siots_p,
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: write plant unaligned index: %d position: %d plant with offset status: 0x%x\n",
                                     index, write_fruts_p->position, status);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }

        /* Goto next write info and write fruts.
         */
        write_info_p++;
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    
    } /* end for all write positions */

    /* Always return the execution status.
     */
    return status;
}
/**********************************************************
 * end of fbe_mirror_write_plant_parent_sgs_for_unaligned()
 **********************************************************/

/*!**************************************************************
 *          fbe_mirror_write_setup_resources()
 ****************************************************************
 *
 * @brief   Setup the newly allocated resources for a mirror write.
 *
 * @param siots_p - current I/O.
 * @param preread_info_p - Pointer to pre-read info pointer (if any)
 * @param write_info_p- - Pointer to write fru info array (if any)              
 *
 * @return fbe_status_t
 *
 * @author
 *  02/08/2009  Ron Proulx - Created
 *
 ****************************************************************/
fbe_status_t fbe_mirror_write_setup_resources(fbe_raid_siots_t * siots_p, 
                                              fbe_raid_fru_info_t *preread_info_p,
                                              fbe_raid_fru_info_t *write_info_p)
{
    /* We initialize status to failure since it is expected to be populated. */
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_memory_info_t  memory_info;
    fbe_raid_memory_info_t  data_memory_info;
    fbe_payload_block_operation_opcode_t opcode;

    /* Initialize our memory request information
     */    
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "mirror_wt_setup_resource",
                             "mirror: fail to init mem info stat=0x%x,siots=0x%p\n",
                             status, siots_p);
        return (status); 
    }
    status = fbe_raid_siots_init_data_mem_info(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to init data memory info: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Get the opcode for this operation. 
     * Raw mirror writes may be a write-verify.
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)
    {
        /* Setup all positions for a write.  If any positions are disabled
         * we won't issue the write.
         */
        status = fbe_mirror_write_setup_fruts(siots_p, 
                                              preread_info_p,
                                              write_info_p,
                                              opcode,
                                              &memory_info);
    }
    else
    {
        /* Setup all positions for a write.  If any positions are disabled
         * we won't issue the write.
         */
        status = fbe_mirror_write_setup_fruts(siots_p, 
                                              preread_info_p,
                                              write_info_p,
                                              FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                              &memory_info);
    }

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "mirror_wt_setup_resource",
                             "mirror:fail to setup write fruts stat=0x%x,siots=0x%p\n",
                             status, siots_p);
        return (status);
    }

    /* Plant the nested siots in case it is needed for recovery verify.
     * We don't initialize it until it is required.
     */
    status = fbe_raid_siots_consume_nested_siots(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "mirror_wt_setup_resource",
                             "mirror:fail to consume nest siots:siots=0x%p,stat=0x%x\n",
                             siots_p, status);
        return (status);
    }
        
    /* Initialize the RAID_VC_TS now that it is allocated.
     */
    status = fbe_raid_siots_init_vcts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, 
                             FBE_TRACE_LEVEL_ERROR, __LINE__, "mirror_wt_setup_resource",
                             "mirror:fail in vcts intial for siots 0x%p, status = 0x%x\n",
                             siots_p, status);
        return  status;
    }

    /* Initialize the VR_TS.
     * We are allocating VR_TS structures WITH the fruts structures.
     */
    status = fbe_raid_siots_init_vrts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, 
                             FBE_TRACE_LEVEL_ERROR, __LINE__, "mirror_wt_setup_resource",
                             "mirror: fail in vrts intial for siots 0x%p,status=0x%x\n",
                             siots_p, status);
        return  status;
    }
        
    /* Using the sg_index information from the fru info array allocate
     * any pre-read sgs and any non-zero host write sgs.
     */
    status = fbe_raid_fru_info_array_allocate_sgs_sparse(siots_p, 
                                                         &memory_info, 
                                                         preread_info_p, 
                                                         NULL, 
                                                         write_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "mirror_wt_setup_resource",
                             "mirror:fail to alloc sgs stat=0x%x,siots=0x%p\n",
                             status, siots_p);
        return(status); 
    }

    /* Plant the optional pre-read buffers as required
     */
    if (fbe_raid_siots_get_read_fruts_count(siots_p) > 0)
    {
        /* Plant the memory for the pre-read
         */
        status = fbe_mirror_read_setup_sgs_normal(siots_p, &data_memory_info);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "mirror_wt_setup_resource",
                                 "mirror:write setup pre-read sgs fail stat=0x%x,siots=0x%p\n",
                                 status, siots_p);
            return (status); 
        }

        /* Mirror write sgs come from the upstream object.  Invoke method
         * that validates and configures the write fruts sgs.
         */
        status = fbe_mirror_write_plant_parent_sgs_for_unaligned(siots_p, write_info_p, &data_memory_info);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {  
            fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "mirror_wt_setup_resource",
                                 "mirror:write plant parent unaligned sgs fail stat=0x%x,siots=0x%p\n",
                                 status, siots_p);
            return (status); 
        }
    } else if (fbe_raid_siots_is_buffered_op(siots_p)) {
        /* Use a specific method for buffered (i.e. non-immediate) operations
         */
        status = fbe_mirror_write_plant_sgs_for_buffered_op(siots_p,
                                                            &data_memory_info, 
                                                            write_info_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {  
            fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "mirror_wt_setup_resource",
                                 "mirror:write plant sgs for buffered op fail stat=0x%x,siots=0x%p\n",
                                 status, siots_p);
            return (status); 
        }
    } else {
        /* Mirror write sgs come from the upstream object.  Invoke method
         * that validates and configures the write fruts sgs.
         */
        status = fbe_mirror_write_plant_parent_sgs(siots_p, write_info_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {  
            fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "mirror_wt_setup_resource",
                                 "mirror:write plant parent sgs fail stat=0x%x,siots=0x%p\n",
                                 status, siots_p);
            return (status); 
        }
    }
    
    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    status = fbe_mirror_validate_fruts(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {  
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "mirror_wt_setup_resource",
                             "mirror: fail to validate sgs status: 0x%x \n",
                             status);
        return(status); 
    }

    return(status);
}
/******************************************
 * end fbe_mirror_write_setup_resources()
 ******************************************/

/*!***************************************************************************
 *          fbe_mirror_write_reinit_fruts_from_bitmask()
 *****************************************************************************
 *
 * @brief   Use the supplied position bitmask to populate the write fruts chain.  
 *          This includes moving any read fruts found in the bitmask to the write 
 *          fruts chain.  Since the I/O range might have changed we update both
 *          the read and write chains with new parity count.
 *          
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 * @param   write_bitmap - bitmap of positions to write.
 *
 * @return fbe_status_t
 *
 * @notes
 *  The rebuild position is always first in the write fruts list.
 *
 *  @author
 *  01/07/2010  Ron Proulx  - Copied from fbe_parity_rebuild_multiple_writes
 *
 ****************************************************************/
fbe_status_t fbe_mirror_write_reinit_fruts_from_bitmask(fbe_raid_siots_t * siots_p, 
                                                        fbe_raid_position_bitmask_t write_bitmask)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_fruts_t           *write_fruts_p = NULL;
    fbe_raid_position_bitmask_t existing_write_bitmask = 0;
    fbe_raid_fruts_t           *read_fruts_p = NULL;
    fbe_raid_position_bitmask_t read_bitmask = 0;
    fbe_raid_position_bitmask_t move_bitmask = 0;
    fbe_payload_block_operation_opcode_t write_opcode = fbe_mirror_get_write_opcode(siots_p);

    /* We need to move any read positions specified to the write chain.
     * It is assumed that there is at least (1) position to write.
     */
    if(RAID_COND(write_bitmask == 0))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: reinit write from bitmask - no positions to write: 0x%x \n",
                             write_bitmask);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Get the read and write fruts chains and bitmask.  The `move'
     * bitmask is the write bitmask OR with the bitmask of positions on
     * the read chain.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_read_fruts_bitmask(siots_p, &read_bitmask);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    fbe_raid_siots_get_write_fruts_bitmask(siots_p, &existing_write_bitmask);
    move_bitmask = (write_bitmask & read_bitmask);
    
    /* It is legal (and in-fact typical) not to have any read fruts
     * that need to be moved. 
     */
    if (move_bitmask == 0)
    {
        /* Validate that there is at least exiting write fruts
         * specified in the bitmask.  If there isn't we assume that there
         * is a logic error and thus report the failure.
         */
        if ((existing_write_bitmask & write_bitmask) == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: reinit write from bitmask - no positions to write: 0x%x existing: 0x%x \n",
                                 write_bitmask, existing_write_bitmask);
            return(FBE_STATUS_GENERIC_FAILURE);
        }
    }

    /* Move the associated fruts from the read chain to the write chain.
     */
    while (read_fruts_p != NULL)
    {
        fbe_raid_fruts_t   *next_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);

        /* If this position is in the write bitmask move it to the
         * write fruts chain.
         */
        if ((1 << read_fruts_p->position) & move_bitmask)
        {
            /* Move to read chain.
             */
            status = fbe_raid_siots_remove_read_fruts(siots_p, read_fruts_p);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: failed to remove read fruts: siots_p = 0x%p, "
                                                    "status = 0x%x, read_fruts_p = 0x%p\n",
                                                    siots_p, status,
                                                    read_fruts_p);

                return  status;
            }
            fbe_raid_siots_enqueue_tail_write_fruts(siots_p, read_fruts_p);
        }

        /* Goto next.
         */
        read_fruts_p = next_fruts_p;
    }

    
    /*************************************************************
     * Loop through all the read fruts.
     * Re-init the fruts for each position.
     *************************************************************/
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    while (read_fruts_p != NULL)
    {
        /* Re-initialize each read fruts with the latest parity range.
         */
        status = fbe_raid_fruts_init_read_request(read_fruts_p,
                                    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                    siots_p->parity_start,
                                    siots_p->parity_count,
                                    read_fruts_p->position);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "mirror: failed to init fruts: siots_p = 0x%p, status = 0x%x, fruts = 0x%p\n",
                                 siots_p, status, read_fruts_p);

            return  status;
        }
        
        /* Goto next read fruts.
         */
        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
    }

    /* Remove any write fruts for any position that doesn't need a write
     * and re-initialize the remaining writes.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    while (write_fruts_p != NULL)
    {
        fbe_raid_fruts_t   *next_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);

        /* If this position no longer needs to be written remove it from
         * the write chain.
         */
        if ( !((1 << write_fruts_p->position) & write_bitmask) )
        {
            /* Remove from write chain and place on the freed_chain
             */
            status = fbe_raid_siots_move_fruts(siots_p, 
                                               write_fruts_p,
                                               &siots_p->freed_fruts_head);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: failed to remove write fruts: siots_p = 0x%p, "
                                                    "status = 0x%x, write_fruts_p = 0x%p\n",
                                                    siots_p, status, write_fruts_p);

                return  status;
            }
        }
        else
        {
            /* Else re-initialize each write fruts with the latest parity range.
             */
            status = fbe_raid_fruts_init_request(write_fruts_p,
                                        write_opcode,
                                        siots_p->parity_start,
                                        siots_p->parity_count,
                                        write_fruts_p->position);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "mirror: failed to init fruts: fruts = 0x%p, status = 0x%x, siots_p = 0x%p\n",
                                     read_fruts_p, status, siots_p);

                return  status;
            }
        }
        
        /* Goto next write fruts.
         */
        write_fruts_p = next_fruts_p;
    }
        
    /* Determine the count of writes to submit.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    siots_p->wait_count = fbe_raid_fruts_count_active(siots_p, write_fruts_p);
    if(RAID_COND(siots_p->wait_count == 0))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: siots_p->wait_count 0x%llx < 1\n",
                             siots_p->wait_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Always return the execution status.
     */
    return(status);
}
/***************************************************
 * end fbe_mirror_write_reinit_fruts_from_bitmask()
 ***************************************************/

/*!***************************************************************************
 *          fbe_mirror_write_reset_fruts_from_write_chain()
 *****************************************************************************
 *
 * @brief   This function is used to move the elements from the write fruts
 *          chain to the read fruts chain to undo the work performed by
 *          fbe_mirror_write_reinit_fruts_from_bitmask().  This is needed
 *          for possible error reporting.         
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_status_t
 *
 *  @author
 *  12/2011  Susan Rundbaken  - Created from fbe_mirror_write_reinit_fruts_from_bitmask()
 *
 ****************************************************************/
fbe_status_t fbe_mirror_write_reset_fruts_from_write_chain(fbe_raid_siots_t * siots_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_fruts_t           *write_fruts_p = NULL;

    /* Get the write fruts chain.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    /* Move the associated fruts from the write chain to the read chain.
     */
    while (write_fruts_p != NULL)
    {
        fbe_raid_fruts_t   *next_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);

        /* Move to read chain.
         */
        status = fbe_raid_siots_remove_write_fruts(siots_p, write_fruts_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: failed to remove write fruts: siots_p = 0x%p, "
                                                "status = 0x%x, write_fruts_p = 0x%p\n",
                                                siots_p, status,
                                                write_fruts_p);

            return  status;
        }
        fbe_raid_siots_enqueue_tail_read_fruts(siots_p, write_fruts_p);

        /* Goto next.
         */
        write_fruts_p = next_fruts_p;
    }

    return(status);
}
/***************************************************
 * end fbe_mirror_write_reset_fruts_from_write_chain()
 ***************************************************/

/*!***************************************************************************
 *          fbe_mirror_write_remove_degraded_write_fruts()
 *****************************************************************************
 *
 * @brief   This method removes a degraded fruts from the write chain (places
 *          on the freed list so that we don't leak it).
 *
 * @param   siots_p - ptr to siots.
 * @param   b_write_degraded - FBE_TRUE - Move any degraded read positions to the
 *                                        write chain and set the opcode to write
 *                             FBE_FALSE - Move any degraded read to write chain
 *                                        and set opcode to NOP.
 *
 * @return  None
 *
 * @note    All `disabled' positions are removed from the write chain
 *
 * @author
 *  03/08/2010  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_mirror_write_remove_degraded_write_fruts(fbe_raid_siots_t *siots_p,
                                                          fbe_bool_t b_write_degraded)
                                                               
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_position_bitmask_t degraded_bitmask = 0;
    fbe_raid_position_bitmask_t disabled_bitmask = 0;
    fbe_raid_fruts_t           *write_fruts_p = NULL;

    /* Get the degraded and disabled bitmask
     */
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    fbe_raid_siots_get_disabled_bitmask(siots_p, &disabled_bitmask);

    /* Walk write fruts chain and remove specified fruts.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    while(write_fruts_p != NULL)
    {
        fbe_raid_fruts_t   *next_write_fruts_p;

        /* In-case we remove it we need the next pointer.
         */
        next_write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);

        /* If this is the position to remove, it.
         */
        if ((1 << write_fruts_p->position) & degraded_bitmask)
        {
            /* Change the opcode to either `NOP' or `WRITE' based on b_write_degraded
             * and whether the position is disabled or not.  Then enqueue to the tail
             * of the write chain
             */
            if (((1 << write_fruts_p->position) & disabled_bitmask) ||
                (b_write_degraded == FBE_FALSE)                        )
            {
                /* Either disabled or set write not set.  Set the opcode to NOP
                 */
                fbe_raid_fruts_set_fruts_degraded_nop(write_fruts_p);

                /* Only if the continue flag is set (meaning that there was a 
                 * position change) do we log a message
                 */
                if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_RECEIVED_CONTINUE))
                {
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                                "mirror: siots lba: 0x%llx blks: 0x%llx marking degraded position: %d on write chain \n",
                                                (unsigned long long)siots_p->start_lba,
                                (unsigned long long)siots_p->xfer_count,
                                write_fruts_p->position);
                }
            }
            else
            {
                /* Else set the opcode to write
                 */
                write_fruts_p->opcode = fbe_mirror_get_write_opcode(siots_p);
            }

        } /* end if this position is degraded */

        /* Goto next fruts.
         */
        write_fruts_p = next_write_fruts_p;
    
    } /* while more write fruts to check */

    /* Data disks is whatever is active in the chain.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    siots_p->data_disks = fbe_raid_fruts_count_active(siots_p, write_fruts_p);
    
    /* Return status
     */
    return(status);
}
/************************************************
 * fbe_mirror_write_remove_degraded_write_fruts()
 ************************************************/

/*!***************************************************************************
 *          fbe_mirror_write_setup_sgs_normal()
 *****************************************************************************
 *
 * @brief   Setup the sg lists for a `normal' write operation.  For each 
 *          write position setup the sgl entry.
 *
 * @param   siots_p - this I/O.
 * @param   memory_info_p - Pointer to memory request information
 *
 * @return  fbe_status_t
 *
 * @note    The sgs for this siots should be validated outside this method
 *
 * @author
 *  03/12/2009  Ron Proulx - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_write_setup_sgs_normal(fbe_raid_siots_t * siots_p,
                                               fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_fruts_t           *write_fruts_p = NULL;

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    while (write_fruts_p != NULL)
    {
        fbe_sg_element_t *sgl_p;
        fbe_u16_t sg_total = 0;
        fbe_u32_t dest_byte_count =0;

        sgl_p = fbe_raid_fruts_return_sg_ptr(write_fruts_p);

        if(!fbe_raid_get_checked_byte_count(write_fruts_p->blocks, &dest_byte_count))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                               "%s: byte count exceeding 32bit limit \n",
                                               __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        /* Assign newly allocated memory for the whole region.
         */
        sg_total =fbe_raid_sg_populate_with_memory(memory_info_p,
                                                                      sgl_p, 
                                                                      dest_byte_count);
        if (sg_total == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                    "%s: failed to populate sg for: siots = 0x%p\n",
                     __FUNCTION__,siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    }

    /* Always return the execution status.
     */
    return(status);
}
/******************************************
 * end fbe_mirror_write_setup_sgs_normal()
 ******************************************/

/*!***************************************************************************
 *          fbe_mirror_write_process_fruts_error()
 *****************************************************************************
 *
 * @brief   We have encountered a write error from one or more read fruts.
 *          Except for algorithms that use the `write and verify' opcode, we
 *          don't expect or allow an media (i.e. `hard errors') errors.
 *
 * @param   siots_p - Pointer to siots that encounter write error
 * @param   eboard_p - Pointer to eboard that will help determine what actions
 *                     to take.
 *
 * @return  raid status - Typically FBE_RAID_STATUS_OK 
 *                             Other - Simply means that we can no longer
 *                                     continue processing this request.
 *
 * @note    We purposefully don't change the state here so that all state
 *          changes are visable in the state machine (based on the status
 *          returned).
 *
 * @note    Since it is perfectly acceptable for a `write and verify' request
 *          to get media errors we do not fail the request and instead report
 *          a specific error. 
 *
 * @author
 *  01/08/2009  Ron Proulx - Created.
 *
 *****************************************************************************/
fbe_raid_status_t fbe_mirror_write_process_fruts_error(fbe_raid_siots_t *siots_p, 
                                                      fbe_raid_fru_eboard_t *eboard_p)

{
    fbe_raid_status_t   status = FBE_RAID_STATUS_INVALID;
    fbe_status_t        fbe_status = FBE_STATUS_OK;
    fbe_raid_position_bitmask_t dead_bitmask = 0;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    fbe_payload_block_operation_opcode_t write_opcode = fbe_mirror_get_write_opcode(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);


    /* First update the local `dead' bitmask.
     */
    dead_bitmask = eboard_p->dead_err_bitmap;

    /* We currently (will no longer?) support the `dropped' error.
     * If it is set fail the request.
     */
    if (eboard_p->drop_err_count != 0)
    {
        /* This is unexpected.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: drop_err_count non-zero: 0x%x \n", 
                             eboard_p->drop_err_count);
        return(FBE_RAID_STATUS_UNSUPPORTED_CONDITION);
    }

    /* Next check the dead error count.  In this case the only recovery is
     * to locate another read position (i.e. one that isn't
     * degraded).  If we locate another read position that isn't dead and does
     * contain any errors.  We simply use that position for the read data.
     */
    if (eboard_p->dead_err_count > 0)
    {
        /* We should have already invoked `process dead fru error'.
         * Therefore the `waiting for monitor' flag should be set and
         * all dead positions should have been acknowledged.
         */
        if ( fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE) ||
            !fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_RECEIVED_CONTINUE))
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: flags: 0x%x or dead_err_bitmap: 0x%x or c_bitmask: 0x%x unexpected \n",
                                 fbe_raid_siots_get_flags(siots_p), eboard_p->dead_err_bitmap, siots_p->continue_bitmask);
            return(FBE_RAID_STATUS_UNEXPECTED_CONDITION);
        }

        /* Remove the dead write fruts.
         */
        fbe_status = fbe_mirror_write_remove_degraded_write_fruts(siots_p, FBE_FALSE);
        if (RAID_COND_STATUS((fbe_status != FBE_STATUS_OK), fbe_status))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: failed to remove dead fruts: siots_p = 0x%p "
                                                "status = 0x%x, dead_bitmask = 0x%x\n",
                                                siots_p, fbe_status, dead_bitmask);

            return  FBE_RAID_STATUS_UNEXPECTED_CONDITION;
        }

        /* Now check if there are sufficient fruts to continue.
         */
        if (fbe_mirror_is_sufficient_fruts(siots_p) == FBE_FALSE)
        {
            /* Too many dead positions to continue.
             */
            return(FBE_RAID_STATUS_TOO_MANY_DEAD);
        }

        /* Set the status to `ok to continue' degraded
         */
        status = FBE_RAID_STATUS_OK_TO_CONTINUE;
    }

    /* Since it is perfectly acceptable for a `write and verify' request
     * to get media errors we do not fail the request and instead report
     * a specific error.
     */ 
    if (eboard_p->hard_media_err_count > 0)
    {
        /* Flag the fact that a media error was detected.
         */
        status = FBE_RAID_STATUS_MEDIA_ERROR_DETECTED;
        
        /*! @note Currently raid doesn't expect `hard error' (i.e. media errors)
         *        for write operations.  The exception is the `write and verify'
         *        opcode which can result in media errors.
         */
        if (write_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)
        {
            /* Sanity check the hard error count.  If it is greater than the width
             * something is wrong.
             */
            if (eboard_p->hard_media_err_count > width)
            {
                /* This is unexpected.
                 */
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: process fruts error - hard_media_err_count: 0x%x greater than width: 0x%x \n", 
                                     eboard_p->hard_media_err_count, width);
                return(FBE_RAID_STATUS_UNEXPECTED_CONDITION);
            }

            if (!fbe_raid_geometry_no_object(raid_geometry_p)){            

                /*! A media error has occurred but the fruts error processing should
                 *  have generated a request to the remap process and since this was
                 *  a `write' request we simply continue and ignore the error.
                 */
                status = FBE_RAID_STATUS_OK_TO_CONTINUE;
            }
        }
        else
        {
            /* Else we don't expect media errors for write requests.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: process fruts error - hard_media_err_count: 0x%x unexpected for write operations\n", 
                                 eboard_p->hard_media_err_count);
            status = FBE_RAID_STATUS_UNEXPECTED_CONDITION;
        }

    }  /* end if hard_media_err_count */

    /* Now we need to handle retryable errors.  Since we could have all (3)
     * types of errors: dead errors, media errors and retryable errors.
     */
    if (eboard_p->retry_err_count > 0)
    {
        /* If any write position encountered a retryable error, simply
         * return retryable
         */
        status = FBE_RAID_STATUS_RETRY_POSSIBLE;
    }

    /* Always return the execution status.
     */
    return(status);
}
/********************************************
 * end fbe_mirror_write_process_fruts_error()
 ********************************************/

/*!**************************************************************
 *          fbe_mirror_write_validate()
 ****************************************************************
 *
 * @brief   Validate the algorithm and some initial parameters to
 *          the write state machine.
 *
 * @param  siots_p - current I/O.          
 *
 * @note    We continue checking even after we have detected an error.
 *          This may help determine the source of the error.    
 *
 * @return FBE_STATUS_OK - Mirror verify request valid
 *         Other         - Malformed mirror verify request
 *
 * @author
 *  12/09/2009  Ron Proulx - Created
 *
 ****************************************************************/
fbe_status_t fbe_mirror_write_validate(fbe_raid_siots_t * siots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);

    /* First make sure we support the algorithm.
     */
    switch (siots_p->algorithm)
    {
        case MIRROR_VR_WR:
        case MIRROR_WR:
        case MIRROR_CORRUPT_DATA:
            /* All the above algorithms are supported.
             */
            break;

        default:
            /* The algorithm isn't supported.  Trace some information and
             * fail the request.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: Unsupported algorithm: 0x%x\n", 
                                 siots_p->algorithm);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    /* By the time the mirror code is invoked (either by the monitor for
     * background requests or for user request) the request has been translated
     * into a physical mirror request.  Therefore the logical and physical
     * fields should match.  Also the data disks should match the width.
     */
    if (siots_p->start_lba != siots_p->parity_start)
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: start_lba: 0x%llx and parity_start: 0x%llx don't agree.\n", 
                             (unsigned long long)siots_p->start_lba,
                 (unsigned long long)siots_p->parity_start);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (siots_p->xfer_count != siots_p->parity_count)
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: xfer_count: 0x%llx and parity_count: 0x%llx don't agree.\n", 
                             (unsigned long long)siots_p->xfer_count,
                 (unsigned long long)siots_p->parity_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (siots_p->data_disks != width)
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: data_disks: 0x%x and width: 0x%x don't agree\n", 
                             siots_p->data_disks, width);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* Always return the status.
     */
    return(status);
}
/******************************************
 * end fbe_mirror_write_validate()
 ******************************************/

/*!***************************************************************************
 *          fbe_mirror_write_limit_request()
 *****************************************************************************
 *
 * @brief   Limits the size of a parent mirror write request.  There are
 *          (2) values that will limit the request size:
 *              1. Maximum per drive request
 *              2. Maximum amount of buffers that cna be allocated
 *                 per request.
 *          For unaligned write requests we will need to allocate the pre-read
 *          fruts.  Therefore we need to make sure the pre-read doesn't exceed
 *          the memory infrastructure.
 *
 * @param   siots_p - this I/O.
 *
 * @return  status - FBE_STATUS_OK - The request was modified as required
 *                   Other - Something is wrong with the request
 *
 * @note    Since we use the parent sgs if the request is too big we fail
 *          the request.
 *
 * @author  
 *  02/09/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_write_limit_request(fbe_raid_siots_t *siots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    
    /* Use the standard method to determine the page size.  For mirror writes
     * we use the width for the number of fruts and 0 for the number of blocks
     * to allocate.
     */
    status = fbe_mirror_limit_request(siots_p,
                                      width,
                                      0,
                                      fbe_mirror_write_get_fru_info);

    /* Always return the execution status.
     */
    return(status);
}
/******************************************
 * end of fbe_mirror_write_limit_request()
 ******************************************/

/*!***************************************************************************
 *          fbe_mirror_write_validate_unaligned_write_buffers()
 *****************************************************************************
 *
 * @brief   Validate that the aligned write positions are using the host sg
 *          and validate that the unaligned writes have the host sg located
 *          in the proper location.
 *
 * @param   siots_p - Pointer to write request to check buffers for
 *
 * @return  status - FBE_STATUS_OK - The request was modified as required
 *                   Other - Something is wrong with the request
 *
 * @author  
 *  04/02/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_write_validate_unaligned_write_buffers(fbe_raid_siots_t *siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_position_bitmask_t unaligned_bitmask;
    fbe_raid_fruts_t       *read_fruts_p = NULL;
    fbe_raid_fruts_t       *write_fruts_p = NULL;
    fbe_sg_element_t       *write_sg_p = NULL;
    fbe_sg_element_t       *read_sg_p = NULL;
    fbe_lba_t               read_end_lba;
    fbe_lba_t               parity_end;
    fbe_block_size_t        bytes_per_block;
    fbe_u32_t               preread_blocks;
    fbe_u32_t               postread_blocks;
    fbe_u32_t               postread_offset;
    fbe_sg_element_with_offset_t read_sg_desc;
    fbe_sg_element_with_offset_t write_sg_desc;
    fbe_sg_element_with_offset_t host_sg_desc;
    fbe_sg_element_t        host_sg_element;
      
    /* Get the `unaligned' position bitmask.  At least (1) position should
     * be unaligned.
     */
    fbe_raid_geometry_get_4k_bitmask(raid_geometry_p, &unaligned_bitmask);

    /* Get the host buffer
     */
    status = fbe_raid_siots_setup_cached_sgd(siots_p, &host_sg_desc);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: write validate unaligned setup cached sgd failed - status: 0x%x\n",
                             status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now adjust but do not modify the original.
     */
    fbe_raid_adjust_sg_desc(&host_sg_desc);
    host_sg_element = *host_sg_desc.sg_element;
    host_sg_element.address += host_sg_desc.offset;
    host_sg_element.count -= host_sg_desc.offset;

    /*! @note We have already validated the number and fruts.  This
     *        routine only validates the sgs.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    read_sg_p = fbe_raid_fruts_return_sg_ptr(read_fruts_p);
    bytes_per_block = FBE_RAID_SECTOR_SIZE(read_sg_p->address);

    /* The read will be allocated first.  We need to take into account
     * the host buffers that will b
     */
    read_end_lba = read_fruts_p->lba + read_fruts_p->blocks - 1;
    parity_end = siots_p->parity_start + siots_p->parity_count - 1;
    preread_blocks = (fbe_u32_t)(siots_p->parity_start - read_fruts_p->lba);
    postread_blocks = (fbe_u32_t)(read_end_lba - parity_end);
    postread_offset = (fbe_u32_t)(parity_end - read_fruts_p->lba + 1);

    /* Get the write fruts chain and count
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    /* Walk thru all the write fruts and determine if this position is
     * unaligned or not.
     *  o If this is an unaligned position validate the write sg matches
     *    both the host and pre-read
     *  o If this write is aligned validate that the sg matches the host
     */
    while (write_fruts_p != NULL) {
        /* If we didn't allocate an sg something is wrong.
         */
        if ( (write_sg_p = fbe_raid_fruts_return_sg_ptr(write_fruts_p)) == NULL ) {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: write validate unaligned position: %d no sg\n",
                                 write_fruts_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* If this is an unaligned request then use the pre-read sg.
         */
        if ((1 << write_fruts_p->position) & unaligned_bitmask) {
            /* Based of the pre-read and post-read block validate the write
             * sgs.
             */
            fbe_sg_element_with_offset_init(&write_sg_desc, (preread_blocks * bytes_per_block), write_sg_p, NULL);
            fbe_raid_adjust_sg_desc(&write_sg_desc);

            /* Make sure sg matches the host.
             */
            if ((write_sg_desc.sg_element->address != host_sg_element.address) ||
                (write_sg_desc.sg_element->count > host_sg_element.count) ) {
                fbe_raid_siots_trace(siots_p,
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: write validate unaligned position: %d sg doesn't match host\n",
                                     write_fruts_p->position);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* If the pre-read blocks doesn't match the pre-read fruts something is wrong.
             */
            if (preread_blocks != 0) {
                /* ******** <---- Read start
                 * *      *            /\
                 * *      * <--- This middle area needs to be added to the write.
                 * *      *            \/
                 * ******** <---- Write start
                 * *      *
                 * *      *
                 * The write lba at this point represents where the user data starts. 
                 * Our read starts before the new write data                  .
                 * The write count may be less than the pre-read.
                 */
                if ((write_sg_p->address != read_sg_p->address) ||
                    (write_sg_p->count > read_sg_p->count)         ) {
                    fbe_raid_siots_trace(siots_p,
                                         FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "mirror: write validate unaligned position: %d sg doesn't match pre-read\n",
                                         write_fruts_p->position);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }

            /* If there is a post read validate the write post-read matches
             * the associated pre-read.
             */
            if (postread_blocks != 0) {
                /* Increment both the pre-read nad write to the post-read offset.
                 */
                fbe_sg_element_with_offset_init(&read_sg_desc, (postread_offset * bytes_per_block), read_sg_p, NULL);
                fbe_raid_adjust_sg_desc(&read_sg_desc);
                fbe_sg_element_with_offset_init(&write_sg_desc,(postread_offset * bytes_per_block), write_sg_p, NULL);
                fbe_raid_adjust_sg_desc(&write_sg_desc);
                /* ******** <---- I/O start
                 * *      *
                 * *      *
                 * ******** <---- Write end
                 * *      *            /\
                 * *      * <--- This middle area needs to be added to the write.
                 * *      *            \/
                 * ******** <---- Read end 
                 *  
                 * The write lba at this point represents where the user data starts 
                 * Our read ends after the new write data ends. 
                 * The post-write count maybe less that the post-read.
                 */
                if ((write_sg_desc.sg_element->address != (read_sg_desc.sg_element->address + read_sg_desc.offset)) ||
                    (write_sg_desc.sg_element->count > read_sg_desc.sg_element->count)                                 ) {
                    fbe_raid_siots_trace(siots_p,
                                         FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "mirror: write validate unaligned position: %d sg doesn't match post-read\n",
                                         write_fruts_p->position);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            } /* end if post-read blocks */
        } else {
            /* Else validate the write matches the host.
             */
            if ((write_sg_p->address != host_sg_element.address) ||
                (write_sg_p->count != host_sg_element.count)        ) {
                fbe_raid_siots_trace(siots_p,
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: write validate unaligned position: %d sg doesn't match host\n",
                                     write_fruts_p->position);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }

        /* Goto next write info and write fruts.
         */
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    
    } /* end for all write positions */

    return status;
}
/************************************************************
 * end of fbe_mirror_write_validate_unaligned_write_buffers()
 ************************************************************/

/*!***************************************************************************
 *          fbe_mirror_write_validate_write_buffers()
 *****************************************************************************
 *
 * @brief   Validate that there is at least (1) write position and it has a
 *          valid buffer.  Then validate that all write positions are pointing
 *          to the same buffer.
 *
 * @param   siots_p - Pointer to write request to check buffers for
 *
 * @return  status - FBE_STATUS_OK - The request was modified as required
 *                   Other - Something is wrong with the request
 *
 * @author  
 *  02/09/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_write_validate_write_buffers(fbe_raid_siots_t *siots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_fruts_t   *write_fruts_p = NULL;
    fbe_sg_element_t   *first_sg_p = NULL;

    /* Only run for checked builds
     */
    if (RAID_DBG_ENABLED == FBE_FALSE) {
        return FBE_STATUS_OK;
    }

    /* Check if this write is unaligned.
     */
    if (fbe_raid_siots_get_read_fruts_count(siots_p) > 0) {
        status = fbe_mirror_write_validate_unaligned_write_buffers(siots_p);
        return status;
    }

    /* Get first write fruts and sg pointer.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    if (write_fruts_p == NULL) {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: write validate buffers siots: 0x%llx 0x%llx no write fruts \n",
                             (unsigned long long)siots_p->start_lba,
                             (unsigned long long)siots_p->xfer_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_raid_fruts_get_sg_ptr(write_fruts_p, &first_sg_p);
    if (first_sg_p == NULL) {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: write validate buffers siots: 0x%llx 0x%llx no buffer for position: %d\n",
                             (unsigned long long)siots_p->start_lba,
                             (unsigned long long)siots_p->xfer_count,
                             write_fruts_p->position);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now walk fruts chain and validate buffers are the same.
     */
    write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    while(write_fruts_p != NULL) {
        fbe_sg_element_t   *sg_p;

        /*! If the buffers aren't the same return an error.
         */
        fbe_raid_fruts_get_sg_ptr(write_fruts_p, &sg_p);
        if ((sg_p != first_sg_p)                         &&
            ((sg_p->count != first_sg_p->count)     ||
             (sg_p->address != first_sg_p->address)    )    ) {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: write validate buffers siots: 0x%llx 0x%llx buffer for position: %d doesn't match\n",
                                 (unsigned long long)siots_p->start_lba,
                                 (unsigned long long)siots_p->xfer_count,
                                 write_fruts_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Goto next write fruts.
         */
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    }

    /* Always return the exection status.
     */
    return(status);

}
/**************************************************
 * end of fbe_mirror_write_validate_write_buffers()
 **************************************************/

/*****************************************************************************
 *          fbe_mirror_write_populate_buffered_request()
 *****************************************************************************
 *
 * @brief   This function populates the write buffers for a `buffered' request.
 *
 * @param   siots_p - Ptr to current fbe_raid_siots_t.
 *   
 * @return  fbe_status_t
 *
 * @note    Current only `Zero' buffered request are supported.
 *
 * @author
 *  03/31/2010  Ron Proulx  - Created from fbe_parity_mr3_get_buffered_data()
 *
 **************************************************************************/
fbe_status_t fbe_mirror_write_populate_buffered_request(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_fruts_t       *write_fruts_p = NULL;
    fbe_xor_zero_buffer_t   zero_buffer;
    fbe_sg_element_t       *sg_p;
    fbe_u32_t               index;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Clear the zero buffer.
     */
    fbe_zero_memory(&zero_buffer, sizeof(fbe_xor_zero_buffer_t));

    /* Fill out the zero buffer structure from the write fruts.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    for (index = 0; index < width; index++)
    {
        fbe_lba_t write_start_lba = write_fruts_p->write_preread_desc.lba;
        fbe_block_count_t write_block_count = write_fruts_p->write_preread_desc.block_count;

        /* Populate the zero buffer request from the write fruts
        */
        fbe_raid_fruts_get_sg_ptr(write_fruts_p, &sg_p);

        if (write_start_lba > write_fruts_p->lba) {
            /* Our write sg list needs to get incremented to the point where the write data starts.
            */
            fbe_sg_element_with_offset_t sg_descriptor;
            fbe_sg_element_with_offset_init(&sg_descriptor, 
                                            (fbe_u32_t)(write_start_lba - write_fruts_p->lba) * FBE_BE_BYTES_PER_BLOCK, 
                                            sg_p, NULL);
            fbe_raid_adjust_sg_desc(&sg_descriptor);
            zero_buffer.fru[index].sg_p = sg_descriptor.sg_element;
            zero_buffer.fru[index].offset = sg_descriptor.offset / FBE_BE_BYTES_PER_BLOCK;
        } else {
            zero_buffer.fru[index].sg_p = sg_p;
            zero_buffer.fru[index].offset = 0;
        }
        zero_buffer.fru[index].count = write_block_count;
        zero_buffer.fru[index].seed = write_start_lba;

        /* Goto next.
        */
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    }

    /* Execute the zero request.
     */
    zero_buffer.disks = width;
    zero_buffer.offset = fbe_raid_siots_get_eboard_offset(siots_p);

    if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p))
    {
        status = fbe_xor_lib_zero_buffer_raw_mirror(&zero_buffer);
    }
    else
    {
        status = fbe_xor_lib_zero_buffer(&zero_buffer);
    }
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "%s status of %d from fbe_xor_zero_buffer \n",
                             __FUNCTION__, status);
    }

    /* Save the status for later use.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, zero_buffer.status);
    return status;
}
/**************************************************
 * end fbe_mirror_write_populate_buffered_request()
 **************************************************/

/*****************************************************************************
 *          fbe_mirror_update_error_region_for_corrupt_data()
 *****************************************************************************
 *
 * @brief   This function updates xor error region during corrupt data 
 *          operation. 
 *
 * @param   siots_p  - pointer to current fbe_raid_siots_t
 * @param   wpos     - position at which invalid pattern is being written
 *   
 * @return  fbe_status_t
 *
 * @author
 *  03/28/2011  Jyoti Ranjan
 *
 **************************************************************************/
fbe_status_t fbe_mirror_update_error_region_for_corrupt_data(fbe_raid_siots_t * const siots_p, fbe_u32_t wpos)
{
    fbe_u32_t error_code;
    fbe_u32_t region_index;
    fbe_xor_error_regions_t * regions_p;

    if (siots_p->vcts_ptr != NULL)
    {
        regions_p = &siots_p->vcts_ptr->error_regions;

        /* traverse the xor error region for current disk position
         */
        for (region_index = 0; region_index < regions_p->next_region_index; ++region_index)
        {
            /* Drop error region for all position except one which will be valid only for 
             * wpos (provided in argument). It is so because, we might have created error 
             * region earlier but we are are not going to perform write operation except 
             * given position becaue its a corrupt data operation.
             */
            error_code = regions_p->regions_array[region_index].error & FBE_XOR_ERR_TYPE_MASK;
            if ((error_code == FBE_XOR_ERR_TYPE_CORRUPT_DATA) || 
                (error_code == FBE_XOR_ERR_TYPE_CORRUPT_DATA_INJECTED))
            {
                regions_p->regions_array[region_index].pos_bitmask = (1 << wpos);

                /* Stop here as all other error regions will be ignored. Only 
                 * one error region is sufficient for reporting logs because
                 * of corrupt data operation.
                 */
                break;
            }
        } /* end for (region_index = 0; region_index < regions_p->next_region_index; ++region_index) */ 

        /* terminate the error region list correctly.
         */
        regions_p->next_region_index = region_index + 1;

    } /* end if (siots_p->vcts_ptr != NULL) */


    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_mirror_update_error_region_for_corrupt_data()
 **************************************************/

/*****************************************************************************
 *          fbe_mirror_write_handle_corrupt_data()
 *****************************************************************************
 *
 * @brief   This function handles the corrupt data request for mirrors.  Change
 *          all valid (i.e. opcode != NOP) write positions except (1) to NOP.
 *
 * @param   siots_p - Ptr to current fbe_raid_siots_t.
 *   
 * @return  fbe_status_t
 *
 * @author
 *  01/20/2011  Ron Proulx  - Created from fbe_mirror_write_state3
 *
 **************************************************************************/
fbe_status_t fbe_mirror_write_handle_corrupt_data(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t                   degraded_count = 0;
    fbe_raid_position_bitmask_t degraded_bitmask;
    fbe_raid_fruts_t           *write_fruts_p = NULL;
    fbe_bool_t                  b_writable_fruts_found = FBE_FALSE;
    fbe_raid_position_t         writable_position = FBE_RAID_INVALID_POS;

    /* Change all degraded and disabled positions fruts to NOP.
     */
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    degraded_count = fbe_raid_get_bitcount(degraded_bitmask);
    if (RAID_COND(degraded_count >= width))
    {
        /* We can't continue
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: siots_p: 0x%p degraded_count: %d is bad. width: %d \n",
                             siots_p, degraded_count, width);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Walk write fruts chain and remove specified fruts.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    while(write_fruts_p != NULL)
    {
        fbe_raid_fruts_t   *next_write_fruts_p;

        /* Get the next pointer
         */
        next_write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);

        /* If this position is marked degraded skip it
         */
        if ((1 << write_fruts_p->position) & degraded_bitmask)
        {
            /* Skip
             */
        }
        else if (write_fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID)
        {
            /* Although we can handle this, it is unexpected so log an
             * informational message
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                 "mirror: siots_p: 0x%p unexpected nop position: %d continue \n",
                                 siots_p, write_fruts_p->position);
        }
        else if (b_writable_fruts_found == FALSE)
        {
            /* Else if a writeable fruts wasn't found, use this one
             */
            b_writable_fruts_found = FBE_TRUE;
            writable_position = write_fruts_p->position;
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                 "mirror: siots_p: 0x%p found corrupt data position: %d \n",
                                 siots_p, write_fruts_p->position);
        }
        else
        {
            /* Else change this position to a NOP and decrement data disk
             */
            fbe_raid_fruts_set_fruts_degraded_nop(write_fruts_p);
            if (siots_p->data_disks < 1)
            {
                /* Unexpected data disk count
                 */
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: siots_p: 0x%p attempt to change position: %d to NOP but data_disk: %d bad \n",
                                     siots_p, write_fruts_p->position, siots_p->data_disks);
                return(FBE_STATUS_GENERIC_FAILURE);
            }
            siots_p->data_disks--;
        }

        /* Goto next fruts.
         */
        write_fruts_p = next_write_fruts_p;
    
    } /* while more write fruts to check */

    /* If no writable position was found or the data_disk isn't 1, something is
     * wrong.
     */
    if (RAID_COND((b_writable_fruts_found == FBE_FALSE) ||
                  (siots_p->data_disks != 1)               ))
    {
        /* Log the error and return
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: siots_p: 0x%p writeable found: %d or data_disks: %d unexpected \n",
                             siots_p, b_writable_fruts_found, siots_p->data_disks);
        return (FBE_STATUS_GENERIC_FAILURE);
    }


    /* Modify xor error regio to reflect correctly for corrupt data operation.
     */
    status = fbe_mirror_update_error_region_for_corrupt_data(siots_p, writable_position);
    if (status != FBE_STATUS_OK) { return status; }
    
    /* Return status
     */
    return (FBE_STATUS_OK);
}
/********************************************
 * end fbe_mirror_write_handle_corrupt_data()
 ********************************************/

/*****************************************************************************
 *          fbe_mirror_validate_unaligned_write()     
 ***************************************************************************** 
 * 
 * @brief   Validate the consistency (i.e. the allocated sg is valid and is
 *          setup to contain sufficient buffers for the blocks being accessed)
 *          of the sgls for this request
 *
 * @param siots_p - pointer to SUB_IOTS data
 *
 * @return fbe_status_t
 *
 * @author
 *  04/01/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_validate_unaligned_write(fbe_raid_siots_t *siots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t           alignment = fbe_raid_geometry_get_default_optimal_block_size(raid_geometry_p);
    fbe_block_count_t   curr_sg_blocks;
    fbe_u32_t           sgs = 0;
    fbe_raid_fruts_t   *read_fruts_p = NULL;
    fbe_raid_fruts_t   *read2_fruts_p = NULL;
    fbe_raid_fruts_t   *write_fruts_p = NULL;
    fbe_lba_t           parity_end;
    fbe_lba_t           read_end;
    fbe_raid_position_bitmask_t unaligned_bitmask;

    /* Get the mask of unaligned positions.
     */
    fbe_raid_geometry_get_4k_bitmask(raid_geometry_p, &unaligned_bitmask);

    /* Get any chains associated with this request
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_p);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    /* Mirrors don't use the read2 chain
     */
    if (read2_fruts_p != NULL) {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "siots lba: 0x%llx blks: 0x%llx unexpected non-NULL read2 chain\n",
                                    (unsigned long long)siots_p->start_lba,
                                    (unsigned long long)siots_p->xfer_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Verify aligment.
     */
    if (!fbe_raid_geometry_io_needs_alignment(raid_geometry_p, 
                                              siots_p->parity_start, 
                                              siots_p->parity_count)) {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "siots lba: 0x%llx blks: 0x%llx needs aligment is false\n",
                                    (unsigned long long)siots_p->start_lba,
                                    (unsigned long long)siots_p->xfer_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate algorithm.
     */
    if ((siots_p->algorithm != MIRROR_WR)    &&
        (siots_p->algorithm != MIRROR_VR_WR) &&
        (siots_p->algorithm != MIRROR_CORRUPT_DATA)   ) {
        /* Only the above algorithms are supported.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "siots lba: 0x%llx blks: 0x%llx unexpected alg: 0x%x\n",
                                    (unsigned long long)siots_p->start_lba,
                                    (unsigned long long)siots_p->xfer_count,
                                    siots_p->algorithm);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the pre-read fruts is aligned and spans the parity count.
     */
    if (fbe_raid_siots_get_read_fruts_count(siots_p) != 1) {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "siots lba: 0x%llx blks: 0x%llx pre-read pos: %d blks: 0x%llx unexpected fruts count: %d\n",
                                    (unsigned long long)siots_p->start_lba,
                                    (unsigned long long)siots_p->xfer_count,
                                    read_fruts_p->position,
                                    (unsigned long long)read_fruts_p->blocks,
                                    fbe_raid_siots_get_read_fruts_count(siots_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }
    read_end = read_fruts_p->lba + read_fruts_p->blocks - 1;
    parity_end = siots_p->parity_start + siots_p->parity_count - 1;

    /* Memory validate header validates that the actual number of sg entries
     * doesn't exceed the sg type.
     */
    sgs = 0;
    if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)read_fruts_p->sg_id)) {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "siots lba: 0x%llx blks: 0x%llx read pos: %d invalid sg id\n",
                                    (unsigned long long)siots_p->start_lba,
                    (unsigned long long)siots_p->xfer_count,
                    read_fruts_p->position);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (read_fruts_p->blocks == 0) {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "siots lba: 0x%llx blks: 0x%llx read pos: %d read blocks is 0\n", 
                                    (unsigned long long)siots_p->start_lba,
                    (unsigned long long)siots_p->xfer_count,
                    read_fruts_p->position);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_raid_sg_count_sg_blocks(fbe_raid_fruts_return_sg_ptr(read_fruts_p), 
                                         &sgs,
                                         &curr_sg_blocks );
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "mirror: failed to count sg blocks: status = 0x%x, siots_p = 0x%p\n", 
                                    status, siots_p);
        return status;
    }

    /* Make sure the read has an SG for the correct block count */
    if (read_fruts_p->blocks != curr_sg_blocks) {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "siots lba: 0x%llx blks: 0x%llx read pos: %d blks: 0x%llx doesn't match sg blks: 0x%llx \n",
                                    (unsigned long long)siots_p->start_lba,
                                    (unsigned long long)siots_p->xfer_count,
                                    read_fruts_p->position,
                                    (unsigned long long)read_fruts_p->blocks,
                                    (unsigned long long)curr_sg_blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the pre-read is aligned and contains the original request.
     */
    if ((read_fruts_p->lba % alignment)    ||
        (read_fruts_p->blocks % alignment)    ) {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "siots parity lba: 0x%llx count: 0x%llx not aligned pos: %d lba: %llx blks: 0x%llx \n",
                                    (unsigned long long)siots_p->parity_start,
                                    (unsigned long long)siots_p->parity_count,
                                    read_fruts_p->position, 
                                    (unsigned long long)read_fruts_p->lba,
                                    (unsigned long long)read_fruts_p->blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if ((read_fruts_p->lba > siots_p->parity_start) ||
        (read_end < parity_end)                        ) {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "siots parity lba: 0x%llx count: 0x%llx doesn't match pos: %d lba: %llx blks: 0x%llx \n",
                                    (unsigned long long)siots_p->parity_start,
                                    (unsigned long long)siots_p->parity_count,
                                    read_fruts_p->position, 
                                    (unsigned long long)read_fruts_p->lba,
                                    (unsigned long long)read_fruts_p->blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now validate the consistency of the write chain
     */
    while (write_fruts_p) {
        /* Memory validate header validates that the actual number of sg entries
         * doesn't exceed the sg type.
         */
        sgs = 0;
        if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)write_fruts_p->sg_id)) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "raid header invalid for fruts 0x%p, sg_id: 0x%p",
                                        write_fruts_p, write_fruts_p->sg_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (write_fruts_p->blocks == 0) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots lba: 0x%llx blks: 0x%llx read pos: %d write blocks is 0 \n", 
                                        (unsigned long long)siots_p->start_lba,
                                        (unsigned long long)siots_p->xfer_count,
                                        write_fruts_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        status = fbe_raid_sg_count_sg_blocks(fbe_raid_fruts_return_sg_ptr(write_fruts_p), 
                                             &sgs,
                                             &curr_sg_blocks );
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: failed to count blocks: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return status;
        }

        /* Make sure the write has an SG for the correct block count */
        if (write_fruts_p->blocks != curr_sg_blocks) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots lba: 0x%llx blks: 0x%llx write pos: %d blks: 0x%llx doesn't match sg blks: 0x%llx \n",
                                        (unsigned long long)siots_p->start_lba,
                                        (unsigned long long)siots_p->xfer_count,
                                        write_fruts_p->position,
                                        (unsigned long long)write_fruts_p->blocks,
                                        (unsigned long long)curr_sg_blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Unaligned writes should match the pre-read.  Aligned should match
         * parity.
         */
        if ((1 << write_fruts_p->position) & unaligned_bitmask) {
            if ((write_fruts_p->lba != read_fruts_p->lba)       ||
                (write_fruts_p->blocks != read_fruts_p->blocks)    ) {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots parity lba: 0x%llx count: 0x%llx pos: %d lba: %llx blks: 0x%llx doesnt match preread \n",
                                        (unsigned long long)siots_p->parity_start,
                                        (unsigned long long)siots_p->parity_count,
                                        write_fruts_p->position,
                                        (unsigned long long)write_fruts_p->lba,
                                        (unsigned long long)write_fruts_p->blocks);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        } else if ((write_fruts_p->lba != siots_p->parity_start)    ||
                   (write_fruts_p->blocks != siots_p->parity_count)    ) {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots parity lba: 0x%llx count: 0x%llx doesn't match pos: %d lba: %llx blks: 0x%llx \n",
                                        (unsigned long long)siots_p->parity_start,
                                        (unsigned long long)siots_p->parity_count,
                                        write_fruts_p->position,
                                        (unsigned long long)write_fruts_p->lba,
                                        (unsigned long long)write_fruts_p->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Goto next
         */
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);

    } /* end for all write fruts */

    /* Always return the execution status
     */
    return status;
}
/**********************************************
 * end of fbe_mirror_validate_unaligned_write()
 **********************************************/


/******************************
 * end fbe_mirror_write_util.c
 *****************************/

