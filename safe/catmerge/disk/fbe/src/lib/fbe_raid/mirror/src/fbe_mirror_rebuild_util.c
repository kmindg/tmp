/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file    fbe_mirror_rebuild_util.c
 ***************************************************************************
 *
 * @brief   This file contains utility functions for the mirror rebuild
 *          algorithms.
 *
 * @notes
 *
 * @author
 *  12/16/2009  Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 **  INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_mirror_io_private.h"

/*****************************************************
 *  LOCAL FUNCTIONS
 *****************************************************/

/*!**************************************************************
 *          fbe_mirror_rebuild_validate()
 ****************************************************************
 * @brief
 *  Validate the algorithm and some initial parameters to
 *  the rebuild state machine.
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
 *  12/16/2009  Ron Proulx - Created
 *
 ****************************************************************/
fbe_status_t fbe_mirror_rebuild_validate(fbe_raid_siots_t * siots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_block_size_t    optimal_block_size;
    fbe_u32_t           fruts_count;

    /* First make sure we support the algorithm.
     */
    switch (siots_p->algorithm)
    {
        case MIRROR_RB:
        case MIRROR_COPY:
        case MIRROR_PACO:
            /* All the above algorithms are supported.
             */
            break;

        default:
            /* The algorithms isn't supported.  Trace some information and
             * fail the request.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: rebuild validate - Unsupported algorithm: 0x%x\n", 
                                 siots_p->algorithm);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    /* Make sure the verify request is a multiple of the optimal block size.
     * Anyone calling this code should align the request to the optimal 
     * block size.
     */
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &optimal_block_size);
    if ( (siots_p->parity_count % optimal_block_size) != 0 )
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: rebuild validate - parity_count: 0x%llx isn't a multiple of optimal block size: 0x%x\n", 
                             (unsigned long long)siots_p->parity_count,
			     optimal_block_size);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if ( (siots_p->parity_start % optimal_block_size) != 0)
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: rebuild validate - parity_start: 0x%llx isn't a multiple of optimal block size: 0x%x\n", 
                             (unsigned long long)siots_p->parity_start,
			     optimal_block_size);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that we have at least (1) read position and at least (1) 
     * degraded (i.e. write) position.
     */
    fruts_count = siots_p->data_disks;
    if ( (fruts_count < 1) || (fruts_count > (width - 1)) ) 
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: rebuild validate - Unexpected read position count: 0x%x\n", 
                             fruts_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    fruts_count = width - siots_p->data_disks;
    if ( (fruts_count < 1) || (fruts_count > (width - 1)) )
    {
        /* Although this is unexpected (i.e. receiving a rebuild request without
         * any degraded positions) it is allowed and therefore we don't set the
         * error status.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: rebuild validate - Unexpected write position count: 0x%x\n", 
                             fruts_count);
    }

    /* Always return the status.
     */
    return(status);
}
/******************************************
 * end fbe_mirror_rebuild_validate()
 ******************************************/

/*!***************************************************************************
 *          fbe_mirror_rebuild_get_fru_info()
 *****************************************************************************
 * 
 * @brief   This function initializes the fru info array for a mirror rebuild 
 *          request.  Each position represents the same range of blocks.  
 *          Therefore the start and ending lbas are the same for each position.
 *
 * @param   siots_p - Current sub request.
 * @param   read_info_p - Pointer to array of read info to populate
 * @param   write_info_p - Pointer to array of write (i.e. degraded positions) info
 * @param   num_sgs_p - Pointer to array of sg count by sg index array
 * @param   b_log - FBE_TRUE - Generate a trace log if sg generation fails
 *                  FBE_FALSE - Method invoked to limit request size
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  11/25/2009  Ron Proulx  - Created from fbe_parity_verify_get_fruts_info
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_rebuild_get_fru_info(fbe_raid_siots_t *siots_p,
                                             fbe_raid_fru_info_t *read_info_p,
                                             fbe_raid_fru_info_t *write_info_p,
                                             fbe_u16_t *num_sgs_p,
                                             fbe_bool_t b_log)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t                   index;
    fbe_raid_position_bitmask_t degraded_bitmask;
    fbe_block_count_t        mem_left = 0;

    /* Page size must be set.
     */
    if (!fbe_raid_siots_validate_page_size(siots_p))
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* For a mirror rebuild request we will populate (i.e. generate a fruts)
     * for all positions.  The non-degraded positions get put on the read
     * fruts chain and the degraded (i.e. those positions that need to be
     * rebuilt) get put on the write fruts chain.  We allocate memory for
     * all positions since the source and destination buffers cannot be the
     * same when using the XOR methods.
     */
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    for (index = 0; index < width; index++)
    {
        /* Always set the position for both read and write infos
         */
        read_info_p->position = fbe_mirror_get_position_from_index(siots_p, index);
        write_info_p->position = fbe_mirror_get_position_from_index(siots_p, index);
        
        /* If this position is degraded then populate the read fru info as
         * invalid and set the write fru info with an invalid index (i.e.
         * there is no need to allocate an sg for a degraded position).
         */
        if ((1 << read_info_p->position) & degraded_bitmask)
        {
            /* Mark read info as invalid.
             */
            read_info_p->lba = FBE_LBA_INVALID;
            read_info_p->blocks = 0;
            read_info_p->sg_index = FBE_RAID_SG_INDEX_INVALID;

            /* Mark the write info as valid and populate sg info
             */
            write_info_p->lba = siots_p->parity_start;
            write_info_p->blocks = siots_p->parity_count;

            /* Set sg index returns a status.  The reason is that the sg_count
             * may exceed the maximum sgl length.
             */
            if ( (status = fbe_raid_fru_info_populate_sg_index(write_info_p,
                                                               siots_p->data_page_size,
                                                               &mem_left,
                                                               b_log)) != FBE_STATUS_OK )
            {
                /* Always return the execution status.
                 */
                return(status);
            }
            else
            {
                /* Else increment the sg type count.
                 */
                num_sgs_p[write_info_p->sg_index]++;
            }
        }
        else
        {
            /* Else populate the read sg information.
             */
            read_info_p->lba = siots_p->parity_start;
            read_info_p->blocks = siots_p->parity_count;

            /* Set sg index returns a status.  The reason is that the sg_count
             * may exceed the maximum sgl length.
             */
            if ( (status = fbe_raid_fru_info_populate_sg_index(read_info_p,
                                                               siots_p->data_page_size,
                                                               &mem_left,
                                                               b_log)) != FBE_STATUS_OK )
            {
                /* Always return the execution status.
                 */
                return(status);
            }
            else
            {
                /* Else increment the sg type count.
                 */
                num_sgs_p[read_info_p->sg_index]++;
            }
            
            /* Set the write info to invalid.
             */
            write_info_p->lba = FBE_LBA_INVALID;
            write_info_p->blocks = 0;
            write_info_p->sg_index = FBE_RAID_SG_INDEX_INVALID;
        
        } /* end else if position isn't degraded */

        /* Goto next fru info.
         */
        read_info_p++;
        write_info_p++;
    }

    /* Always return the execution status.
     */
    return(status);
}
/***************************************
 * end fbe_mirror_rebuild_get_fru_info()
 ***************************************/

/*!***************************************************************************
 *          fbe_mirror_rebuild_limit_request()
 *****************************************************************************
 *
 * @brief   Limits the size of a mirror rebuild request.  There are
 *          (2) values that will limit the request size:
 *              1. Maximum per drive request
 *              2. Maximum amount of buffers that can be allocated
 *                 per request.
 *
 * @param   siots_p - this I/O.
 *
 * @return  status - FBE_STATUS_OK - The request was modified as required
 *                   Other - Something is wrong with the request
 *
 * @author  
 *  03/10/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_rebuild_limit_request(fbe_raid_siots_t *siots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    
    /* Use the standard method to determine the page size.  For mirror rebuilds
     * we use the width for the number of fruts and the transfer size for the
     * number of blocks to allocate.
     */
    status = fbe_mirror_limit_request(siots_p,
                                      width,
                                      (width * siots_p->parity_count),
                                      fbe_mirror_rebuild_get_fru_info);

    /* Always return the execution status.
     */
    return(status);
}
/******************************************
 * end of fbe_mirror_rebuild_limit_request()
 ******************************************/

/*!**************************************************************************
 *          fbe_mirror_rebuild_calculate_memory_size()
 ****************************************************************************
 *
 * @brief   This function calculates the total memory usage of the siots for
 *          this state machine.
 *
 * @param   siots_p - Pointer to siots that contains the allocated fruts
 * @param   read_info_p - Pointer to array of read positions to populate
 * @param   write_info_p - Pointer to array of write (i.e. degraded) positions 
 *                         to populate
 * 
 * @return  status - Typically FBE_STATUS_OK
 *                             Other - This shouldn't occur
 *
 * @author
 *  11/25/2009  Ron Proulx  - Created from fbe_parity_verify_calculate_memory_size
 *
 **************************************************************************/
fbe_status_t fbe_mirror_rebuild_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                      fbe_raid_fru_info_t *read_info_p,
                                                      fbe_raid_fru_info_t *write_info_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    fbe_u16_t           num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_u32_t           num_fruts;
    fbe_block_count_t  blocks_to_allocate;

    /* Initialize the sg index count per position array.
     */
    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));
    
    /* We allocate buffers for both the read and write positions
     */
    num_fruts = width;
    
    /* For rebuild operations we will read from or more positions and write
     * that data to all the degraded positions.  Since the reads are issued
     * simultanteously and may generate different results (i.e. (1) may succeed
     * and (1) may fail, we allocate different buffers for each read position.
     */
    blocks_to_allocate = num_fruts * siots_p->parity_count;

    /* Setup the the blocks to allocate and the standard page size for the
     * allocation.  The number of fruts to allocate MUST be the width of the
     * raid group.
     */
    siots_p->total_blocks_to_allocate = blocks_to_allocate;
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts,
                                         siots_p->total_blocks_to_allocate);

    /* Now determine the sgs required for the rebuild request.
     */
    status = fbe_mirror_rebuild_get_fru_info(siots_p, 
                                             read_info_p,
                                             write_info_p,
                                             &num_sgs[0],
                                             FBE_TRUE /* Report errors */);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror : failed to get fru info: siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return  status;
    }

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, &num_sgs[0],
                                                FBE_FALSE /* No recovery verify is required*/);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror : failed to calculate num of pages: siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return  status;
    }
    /* Always return the execution status.
     */
    return(status);
}
/**********************************************
 * end fbe_mirror_rebuild_calculate_memory_size()
 **********************************************/

/*!**********************************************************************
 *          fbe_mirror_rebuild_setup_fruts()
 ************************************************************************
 *
 * @brief   This method initializes the fruts for a mirror rebuild request.
 *          Normally 1 read fruts is initialized to the opcode requested  
 *          (any other `fully accessible' positions are ignored) and all 
 *          the degraded positions are set to write opcodes.
 *
 * @param   siots_p    - ptr to the fbe_raid_siots_t
 * @param   read_info_p - Array of information about fruts 
 * @param   write_info_p - Array of information about degraded fruts 
 * @param   requested_opcode - Opcode to initialize read fruts with
 * @param   memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 *
 * @note    May allow multiple reads in the future and pick `best' read data.
 *
 * @author
 *  12/15/2009  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t fbe_mirror_rebuild_setup_fruts(fbe_raid_siots_t *siots_p, 
                                                   fbe_raid_fru_info_t *read_info_p,
                                                   fbe_raid_fru_info_t *write_info_p,
                                                   fbe_payload_block_operation_opcode_t requested_opcode,
                                                   fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_fruts_t           *read_fruts_p = NULL;
    fbe_raid_fruts_t           *write_fruts_p = NULL;
    fbe_u32_t                   index;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_position_bitmask_t data_pos_bitmask;
    fbe_raid_position_bitmask_t degraded_bitmask;
    fbe_u32_t                   degraded_count;
    fbe_raid_position_bitmask_t full_access_bitmask;
    fbe_u32_t                   full_access_count;

    /* Get the degraded and full access bitmasks.
     */
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    degraded_count = fbe_raid_siots_get_degraded_count(siots_p);
    status = fbe_raid_siots_get_full_access_bitmask(siots_p, &full_access_bitmask);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: failed to get access bitmask: status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);

        return  status;
    }
    full_access_count = fbe_raid_siots_get_full_access_count(siots_p);

    /* We expect the full access count to be exactly the number of data disks.
     */
    if (full_access_count != siots_p->data_disks)
    {
        /* There is a descrepency between the expected number of
         * fully accessible drives.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: rebuild setup fruts. fully accessible drives: 0x%x unexpected. data disks: 0x%x\n",
                             full_access_count, siots_p->data_disks); 
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    
    /* Validate that the number of degraded positions agrees with
     * the width minus the number of `data disks'.
     */
    if (degraded_count != (width - siots_p->data_disks))
    {
        /* There is a descrepency between the expected number of
         * data disks and the degraded count.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: rebuild setup fruts. degraded count: 0x%x doesn't agree with width: 0x%x or data_disks: 0x%x\n",
                             degraded_count, width, siots_p->data_disks); 
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Initialize the fruts for data drives.
     * First setup the read fruts (limited to 1), then setup
     * the write fruts.
     */
    for (index = 0; index < width; index++)
    {
        /* The degraded bitmask use position not mirror index.
         */
        data_pos_bitmask = (1 << read_info_p->position);

        /* Since rebuild logging is always enabled any position except the
         * primary maybe degraded.  If the position isn't degraded it goes
         * on the read fruts chain.  Else if the position is degraded it 
         * goes on the write fruts chain.
         */
        if (data_pos_bitmask & degraded_bitmask)
        {
            /* Validate that the read block count is 0 and the write
             * isn't.
             */
            if ((read_info_p->blocks != 0)  ||
                (write_info_p->blocks == 0)    )
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: rebuild setup fruts write blocks is 0x%llx lba: %llx pos: %d\n",
                                     (unsigned long long)write_info_p->blocks,
				     (unsigned long long)write_info_p->lba,
				     write_info_p->position);
                return(FBE_STATUS_GENERIC_FAILURE);
            }
            
            /* This is a degraded position, it goes on the write fruts chain.
             */
            write_fruts_p = fbe_raid_siots_get_next_fruts(siots_p,
                                                          &siots_p->write_fruts_head,
                                                          memory_info_p);
                
            /* If the setup failed something is wrong.
             */
            if (write_fruts_p == NULL)
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: rebuild setup fruts siots: 0x%llx 0x%llx for position: 0x%x failed \n",
                                     (unsigned long long)siots_p->start_lba,
				     (unsigned long long)siots_p->xfer_count,
				     write_info_p->position);
                return(FBE_STATUS_GENERIC_FAILURE);
            }

            /* Now initialize the write fruts and set the opcode to write
             */
            status = fbe_raid_fruts_init_fruts(write_fruts_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                      write_info_p->lba,
                                      write_info_p->blocks,
                                      write_info_p->position);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: failed in fruts initialization for siots_p 0x%p, status = 0x%x\n",
                                     siots_p, status);

                return  status;
            }
        }
        else
        {
            /* Else if this position isn't degraded it goes on the read
             * fruts chain.
             */

            /* Validate that the read block count isn't 0 and that the
             * write block count is.
             */
            if ((read_info_p->blocks == 0)  ||
                (write_info_p->blocks != 0)    )
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: rebuild setup fruts read blocks is 0x%llx lba: %llx pos: %d\n",
                                     (unsigned long long)read_info_p->blocks,
				     (unsigned long long)read_info_p->lba,
				     read_info_p->position);
                return(FBE_STATUS_GENERIC_FAILURE);
            }
            
            read_fruts_p = fbe_raid_siots_get_next_fruts(siots_p,
                                                         &siots_p->read_fruts_head,
                                                         memory_info_p);
              
            /* If the setup failed something is wrong.
             */
            if (read_fruts_p == NULL)
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: rebuild setup fruts siots: 0x%llx 0x%llx for position: 0x%x failed \n",
                                     (unsigned long long)siots_p->start_lba,
				     (unsigned long long)siots_p->xfer_count,
				     read_info_p->position);
                return(FBE_STATUS_GENERIC_FAILURE);
            }

            /* Now initialize the read fruts and set the opcode to the requested.
             */
            status = fbe_raid_fruts_init_fruts(read_fruts_p,
                                      requested_opcode,
                                      read_info_p->lba,
                                      read_info_p->blocks,
                                      read_info_p->position);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: failed in fruts initialization for siots_p 0x%p, status = 0x%x\n",
                                     siots_p, status);

                return  status;
            }
        }

        /* Goto the next fruts info.
         */
        read_info_p++;
        write_info_p++;
    }

    /* If a read fruts wasn't found something is wrong.
     */
    if (read_fruts_p == NULL)
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: rebuild setup fruts no valid read fruts found. degraded_bitmask: 0x%x\n",
                             degraded_bitmask);
        status = FBE_STATUS_DEAD;
    }
    
    /* Always return the execution status.
     */
    return(status);
}
/* end of fbe_mirror_rebuild_setup_fruts() */

/*!**************************************************************
 *          fbe_mirror_rebuild_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param siots_p - current I/O.
 * @param read_info_p - Pointer to array of per drive information               
 * @param write_info_p - Pointer to array of degraded information               
 *
 * @return fbe_status_t
 *
 * @author
 *  12/15/2009  Ron Proulx - Copied from fbe_parity_verify_set_resources
 *
 ****************************************************************/
fbe_status_t fbe_mirror_rebuild_setup_resources(fbe_raid_siots_t * siots_p, 
                                                fbe_raid_fru_info_t *read_info_p,
                                                fbe_raid_fru_info_t *write_info_p)
{
    /* We initialize status to failure since it is expected to be populated. */
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_memory_info_t  memory_info;
    fbe_raid_memory_info_t  data_memory_info;

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to initialize memory request info with status = 0x%x, siots_p =0x%p\n",
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

    /* Setup all accessible positions with a fruts.  Only those
     * positions that aren't degraded will be populated with the
     * opcode passed. The degraded positions are populated with
     * invalid opcode (fbe_raid_fruts_send_chain() handles this).  
     * We allow degraded positions to supported because rebuilds
     * are generated from the verify state machine.
     */
    status = fbe_mirror_rebuild_setup_fruts(siots_p, 
                                            read_info_p,
                                            write_info_p,
                                            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                            &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
          fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                              "mirror : failed to setup fruts for siots_p = 0x%p, status = 0x%x\n",
                              siots_p, status);
          return  status;
    }
    
    /* Initialize the RAID_VC_TS now that it is allocated.
     */
    status = fbe_raid_siots_init_vcts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
         fbe_raid_siots_trace(siots_p, 
                              FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "mirror: failed in vcts initialization for siots_p 0x%p, status = 0x%x\n",
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
                              FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "mirror: failed in vrts initialization for siots_p 0x%p, status = 0x%x\n",
                              siots_p, status);

         return  status;
    }

    /*! Plant the allocated sgs into the locations calculated above.
     *
     *  @note We need to use a separate buffer for each position since
     *        the XOR algorithms read for the valid buffer and update
     *        the rebuilt position buffer.
     */
    status = fbe_raid_fru_info_array_allocate_sgs_sparse(siots_p, 
                                                         &memory_info, 
                                                         read_info_p, 
                                                         NULL, 
                                                         write_info_p);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
         fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "mirror: failed to allocate sgs sparse for siots_p 0x%p, status = 0x%x\n",
                              siots_p, status);

         return  status;
    }

    /* Assign buffers to the read sgs.
     */
    status = fbe_mirror_read_setup_sgs_normal(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
         fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "mirror: failed  to setup read sgs for siots_p 0x%p, status = 0x%x\n",
                              siots_p, status);
         return  status;
    }
        
    /* Assign buffers to write sgs.
     * We don't reset to the memory queue head since we want to continue 
     * from after the read buffer space.
     */
    status = fbe_mirror_write_setup_sgs_normal(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
         fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "mirror: failed  to setup write sgs for siots_p 0x%p, status = 0x%x\n",
                              siots_p, status);
         return  status;
    }

    /* Now validate the sgs for this request
     */
    status = fbe_mirror_validate_fruts(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
         fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "mirror: failed to validate sgs for siots_p 0x%p, status = 0x%x\n",
                              siots_p, status);
         return  status;
    }
    
    /* Always return the status.
     */
    return(status);
}
/******************************************
 * end fbe_mirror_rebuild_setup_resources()
 ******************************************/

/*!***************************************************************************
 *          fbe_mirror_rebuild_setup_sgs_for_next_pass()
 *****************************************************************************
 *
 * @brief   Setup the sgs for another strip mine pass.
 *
 * @param   siots_p - current I/O.               
 *
 * @return  fbe_status_t
 *
 * @author
 *  05/07/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_rebuild_setup_sgs_for_next_pass(fbe_raid_siots_t * siots_p)
{
    /* We initialize status to failure since it is expected to be populated. */
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_memory_info_t  memory_info;

    /* We don't re-size the sgls for either the read or write positions.
     * This is ok since we are not growing the size of the request and in-fact
     * we are shrinking the request size.  The sgls will be properly terminated. 
     */
    status = fbe_raid_siots_init_data_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to init memory: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Now setup the sgs for the next pass
     */
    status = fbe_mirror_read_setup_sgs_normal(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
         fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "mirror: failed  to setup read sgs for siots_p 0x%p, status = 0x%x\n",
                              siots_p, status);
         return  status;
    }
        
    /* Assign buffers to write sgs.
     * We don't reset to the memory queue head since we want to continue 
     * from after the read buffer space.
     */
    status = fbe_mirror_write_setup_sgs_normal(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
         fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "mirror: failed to setup write sgs for siots_p 0x%p, status = 0x%x\n",
                              siots_p, status);
         return  status;
    }

    /* Now validate the sgs for this request
     */
    status = fbe_mirror_validate_fruts(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
         fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "mirror: failed to validate sgs for siots_p 0x%p, status = 0x%x\n",
                              siots_p, status);
         return  status;
    }

    /* Always return the status.
     */
    return(status);    
}
/*************************************************
 * end fbe_mirror_rebuild_setup_sgs_for_next_pass()
 *************************************************/

/*!*****************************************************************
 *          fbe_mirror_rebuild_extent()
 *******************************************************************
 *
 * @brief   Rebuild an extent for this siots.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return - fbe_status_t
 *
 * @author
 *  12/18/2009  Ron Proulx - Created from fbe_parity_rebuild_extent 
 *
 *******************************************************************/
fbe_status_t fbe_mirror_rebuild_extent(fbe_raid_siots_t * siots_p)
{
    fbe_status_t                 status = FBE_STATUS_OK;
    fbe_xor_mirror_reconstruct_t rebuild;
    fbe_raid_geometry_t         *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Initialize the sg desc and bitkey vectors.
     */
    status = fbe_mirror_setup_reconstruct_request(siots_p, &rebuild);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
         fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "mirror: failed to setup reconstruct request: siots_p 0x%p, status = 0x%x\n",
                              siots_p, status);
         return  status;
    }

    /* Init the remaining parameters.  For mirrors the
     * extent is simply the siots parity_start and parity_blocks.
     */
    rebuild.start_lba = siots_p->parity_start;
    rebuild.blocks = siots_p->parity_count;
    rebuild.options = FBE_XOR_OPTION_CHK_LBA_STAMPS;
    rebuild.eboard_p = &siots_p->misc_ts.cxts.eboard;
    rebuild.eregions_p = &siots_p->vcts_ptr->error_regions;

    /* For copy operations the mirror raid group does not `own' the data or
     * metadata.  Therefore we cannot check the lba_stamp for (3) reasons:
     *  o Checking of the lba_stamp automatically changes a stamp of 0x0000
     *    to a valid stamp.  But the parent parity raid group expects this
     *    field to be 0x0000 or unchanged.
     *  o RAID-6 parity positions use the lba_stamp field for the POC.
     *  o The original mirror code never checked the lba_stanp for hot-spares.
     */
    if ( fbe_raid_geometry_is_sparing_type(raid_geometry_p) &&
        !fbe_raid_siots_is_metadata_operation(siots_p)         )
    {
        /* Don't check the lba_stamp.
         */
        rebuild.options &= ~FBE_XOR_OPTION_CHK_LBA_STAMPS;

        /* Don't validate seed for invalidated sectors since parent raid group
         * owns the data.  Simply copy invalidated sectors (i.e. don't report
         * and an error)
         */
        rebuild.options |= (FBE_XOR_OPTION_LOGICAL_REQUEST | FBE_XOR_OPTION_IGNORE_INVALIDS);
    }
    
    /* It isn't up to raid to determine if client verify counters
     * are updated or not.  If the client supplied counters we will
     * udpate them.
     */
    rebuild.error_counts_p = &siots_p->vcts_ptr->curr_pass_eboard;

    /* The width MUST be the width of the raid group.
     */
    rebuild.width = fbe_raid_siots_get_width(siots_p);

    /* The current pass counts for the vcts should be cleared since
     * we are beginning another XOR.  Later on, the counts from this pass
     * will be added to the counts from other passes.
     */
    fbe_raid_siots_init_vcts_curr_pass( siots_p );

    /* Perform the rebuild.  
     */
    status = fbe_xor_lib_mirror_rebuild(&rebuild);
    return(status);
}
/*******************************
 * end fbe_mirror_rebuild_extent()
 *******************************/

/*!*****************************************************************
 *          fbe_mirror_rebuild_invalidate_sectors()
 *******************************************************************
 *
 * @brief   Invalidate sectors for COPY read media error.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return - fbe_status_t
 *
 * @author
 *  03/08/2012  Lili Chen 
 *
 *******************************************************************/
fbe_status_t fbe_mirror_rebuild_invalidate_sectors(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_fruts_t * write_fruts_p = NULL;
    fbe_sg_element_t * sg_list_p = NULL;               // sg list

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    fbe_raid_fruts_get_sg_ptr(write_fruts_p, &sg_list_p);
    if (sg_list_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // set-up invalid pattern in i/o buffer
    fbe_xor_lib_fill_invalid_sectors( sg_list_p, 
                                      siots_p->parity_start, 
                                      siots_p->parity_count, 
                                      0, 
                                      XORLIB_SECTOR_INVALID_REASON_COPY_INVALIDATED,
                                      XORLIB_SECTOR_INVALID_WHO_CLIENT);

    return(status);
}
/*********************************************
 * end fbe_mirror_rebuild_invalidate_sectors()
 *********************************************/


/**************************************************
 * end file fbe_mirror_rebuild_util.c
 **************************************************/
