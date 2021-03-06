/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * fbe_parity_rekey_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the utility functions of the parity rekey state machine.
 *
 * @author
 *   10/15/2013 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"
#include "fbe_raid_library.h"
#include "fbe_raid_error.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe/fbe_api_lun_interface.h"

/********************************
 *      static STRING LITERALS
 ********************************/

/*****************************************************
 *      static FUNCTIONS DECLARED GLOBALLY FOR VISIBILITY
 *****************************************************/

/****************************
 *      GLOBAL VARIABLES
 ****************************/

/*************************************
 *      EXTERNAL GLOBAL VARIABLES
 *************************************/


/*!**************************************************************
 * fbe_parity_rekey_validate()
 ****************************************************************
 * @brief
 *  Validate the algorithm and some initial parameters to
 *  the rekey state machine.
 *
 * @param  siots_p - current I/O.               
 *
 * @return FBE_TRUE on success, FBE_FALSE on failure. 
 *
 ****************************************************************/

fbe_bool_t fbe_parity_rekey_validate(fbe_raid_siots_t * siots_p)
{
    fbe_bool_t b_return; 

    /* First make sure we support the algorithm.
     */
    switch (siots_p->algorithm)
    {
        case R5_REKEY:
            b_return = FBE_TRUE;
            break;
        default:
            b_return = FBE_FALSE;
            break;
    };

    if (b_return)
    {
         if (!fbe_parity_generate_validate_siots(siots_p))
         {
             fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                  "parity: rekey generate validate failed\n");
             b_return = FBE_FALSE;
         }
    }
    return b_return;
}
/******************************************
 * end fbe_parity_rekey_validate()
 ******************************************/

/*!***************************************************************
 * fbe_parity_rekey_get_fruts_info()
 ****************************************************************
 * @brief
 *  This function initializes the fru info for rekey.
 *
 * @param siots_p - Current sub request.
 *
 * @return
 *  None
 *
 * @author
 *  10/15/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_rekey_get_fruts_info(fbe_raid_siots_t * siots_p,
                                              fbe_raid_fru_info_t *read_p)
{
    fbe_status_t status;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_block_count_t parity_range_offset;
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;
    fbe_raid_position_t pos;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    status = fbe_raid_get_rekey_pos(raid_geometry_p, iots_p->rekey_bitmask, &pos);

    if (status != FBE_STATUS_OK){ 
        return status; 
    }
    parity_range_offset = siots_p->geo.start_offset_rel_parity_stripe;

    /* We only rekey one drive at a time.  Rekey this drive.
     */
    read_p->lba = logical_parity_start + parity_range_offset;
    read_p->blocks = siots_p->xfer_count;
    read_p->position = pos;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_rekey_get_fruts_info
 **************************************/
/*!**************************************************************
 * fbe_parity_rekey_count_sgs()
 ****************************************************************
 * @brief
 *  Count how many sgs we need for the rekey operation.
 *
 * @param siots_p - this I/O.               
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_parity_rekey_count_sgs(fbe_raid_siots_t * siots_p,
                                        fbe_u16_t *num_sgs_p,
                                        fbe_raid_fru_info_t *read_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_block_count_t mem_left = 0;
    fbe_u32_t sg_count = 0;

    mem_left = fbe_raid_sg_count_uniform_blocks(read_p->blocks,
                                                siots_p->data_page_size,
                                                mem_left,
                                                &sg_count);
    /* Always add on an sg since if we drop down to a region mode, 
     * then it is possible we will need an extra sg to account for region alignment. 
     */
    if ((sg_count + 1 < FBE_RAID_MAX_SG_ELEMENTS))
    {
        sg_count++;
    }
    /* Save the type of sg we need.
     */
    status = fbe_raid_fru_info_set_sg_index(read_p, sg_count, FBE_TRUE    /* yes log */);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:fail to set sg index:stat=0x%x,siots=0x%p, "
                             "read=0x%p\n", 
                             status, siots_p, read_p);

        return status;
    }
    num_sgs_p[read_p->sg_index]++;
    return FBE_STATUS_OK;
} 
/******************************************
 * end fbe_parity_rekey_count_sgs()
 ******************************************/
/*!**************************************************************************
 * fbe_parity_rekey_calculate_memory_size()
 ****************************************************************************
 * @brief  This function calculates the total memory usage of the siots for
 *         this state machine.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param num_pages_to_allocate_p - Number of memory pages to allocate
 * 
 * @return None.
 *
 * @author
 *  10/15/2013 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_parity_rekey_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                     fbe_raid_fru_info_t *read_info_p,
                                                     fbe_u32_t *num_pages_to_allocate_p)
{
    fbe_u16_t num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_u32_t num_fruts = 0;
    fbe_block_count_t total_blocks;

    fbe_status_t status;

    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));
    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_rekey_get_fruts_info(siots_p, read_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: rekey get fruts info failed with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);                               
        return (status);
    }

    num_fruts = fbe_raid_siots_get_width(siots_p);

    total_blocks = siots_p->xfer_count;
    siots_p->total_blocks_to_allocate = total_blocks;
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts, siots_p->total_blocks_to_allocate);

    /* Next calculate the number of sgs of each type.
     */
    status = fbe_parity_rekey_count_sgs(siots_p, &num_sgs[0], read_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    { 
         fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "parity: failed to calculate number of sgs: status = 0x%x, siots_p = 0x%p\n",
                              status, siots_p);

         return status;
    }

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, &num_sgs[0],
                                                FBE_FALSE /* No nested siots is required*/);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    { 
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "parity: failed to calculate number of pages: status = 0x%x, siots_p = 0x%p\n",
                              status, siots_p);

        return status;
    }
    *num_pages_to_allocate_p = siots_p->num_ctrl_pages;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_rekey_calculate_memory_size()
 **************************************/

/*!***************************************************************
 * fbe_parity_rekey_setup_fruts()
 ****************************************************************
 * @brief
 *  This function will set up fruts for a rekey.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 * @param read_p  - Ptr to read information.
 * @param opcode  - type of op to put in fruts.
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 *
 * @author
 *  10/15/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_parity_rekey_setup_fruts(fbe_raid_siots_t * siots_p, 
                                          fbe_raid_fru_info_t *read_p,
                                          fbe_u32_t opcode,
                                          fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;

    read_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p,
                                                   &siots_p->read_fruts_head,
                                                   memory_info_p);
    status = fbe_raid_fruts_init_fruts(read_fruts_ptr,
                                       opcode,
                                       read_p->lba,
                                       read_p->blocks,
                                       read_p->position);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity : failed to init fruts 0x%p for siots_p = 0x%p, status = 0x%x\n",
                             read_fruts_ptr, siots_p, status);
        return status;
    }

    if (read_p->blocks == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: rekey read blocks is 0x%llx lba: %llx pos: %d\n",
                             (unsigned long long)read_p->blocks,
                             (unsigned long long)read_p->lba,
                             read_p->position);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    read_p++;
    return FBE_STATUS_OK;
}
/* end of fbe_parity_rekey_setup_fruts() */
/*!**************************************************************
 * fbe_parity_rekey_setup_sgs()
 ****************************************************************
 * @brief
 *  Setup the sg lists for the rekey operation.
 *
 * @param siots_p - this I/O.     
 * @param memory_info_p - Pointer to memory request information          
 *
 * @return fbe_status_t
 *
 * @author
 *  10/15/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_rekey_setup_sgs(fbe_raid_siots_t * siots_p,
                                        fbe_raid_memory_info_t *memory_info_p)
{
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_status_t status;
    fbe_sg_element_t *sgl_ptr;
    fbe_u16_t  sg_total =0;
    fbe_u32_t dest_byte_count =0;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);

    if (!fbe_raid_get_checked_byte_count(read_fruts_ptr->blocks, &dest_byte_count))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "%s: byte count exceeding 32bit limit \n",
                             __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    /* Assign newly allocated memory for the whole region.
     */
    sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                sgl_ptr, 
                                                dest_byte_count);
    if (sg_total == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to populate sg for siots_p = 0x%p\n",
                             siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Debug code to detect scatter gather overrun.
     */
    status = fbe_raid_fruts_for_each(FBE_RAID_FRU_POSITION_BITMASK, read_fruts_ptr, fbe_raid_fruts_validate_sgs, 0, 0);

    return status;
}
/******************************************
 * end fbe_parity_rekey_setup_sgs()
 ******************************************/
/*!**************************************************************
 * fbe_parity_rekey_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param siots_p - current I/O.               
 *
 * @return fbe_status_t
 *
 * @author
 *  10/15/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_rekey_setup_resources(fbe_raid_siots_t * siots_p, 
                                              fbe_raid_fru_info_t *read_p)
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
                             "parity: failed to initialize memory request info with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }
    status = fbe_raid_siots_init_data_mem_info(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to init data memory info: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Set up the FRU_TS.
     */
    status = fbe_parity_rekey_setup_fruts(siots_p, 
                                           read_p,
                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                           &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity:fail to setup fruts with stat=0x%x,siots=0x%p,read=0x%p\n",
                             status, siots_p, read_p);
        return (status); 
    }
    
    /* Initialize the RAID_VC_TS now that it is allocated.
     */
    status = fbe_raid_siots_init_vcts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                            "parity : failed to init vcts for siots_p = 0x%p, status = 0x%x\n",
                            siots_p, status);
        return status;
    }

    /* Initialize the VR_TS.
     * We are allocating VR_TS structures WITH the fruts structures.
     */
    status = fbe_raid_siots_init_vrts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                            "parity : failed to init vrts for siots_p = 0x%p, status = 0x%x\n",
                            siots_p, status);
        return status;
    }

    /* Plant the allocated sgs into the locations calculated above.
     */
    status = fbe_raid_fru_info_array_allocate_sgs(siots_p, &memory_info,
                                                  read_p, NULL, NULL);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    { 
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:fail alloc sg.siots=0x%p,stat=0x%x,read=0x%p\n",
                             siots_p, status, read_p);
        return status;
    }

    /* Assign buffers to sgs.
     */
    status = fbe_parity_rekey_setup_sgs(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
    { 
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                            "parity:fail setup sgs for siots=0x%p,stat=0x%x\n",
                            siots_p, status);
        return status;
    }

    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_parity_rekey_setup_resources()
 ******************************************/


/*!***************************************************************************
 * fbe_parity_rekey_check_checksum()
 *****************************************************************************
 * @brief
 *  Check the checksums and lba stamps for a rekey operation.
 *  If this is a parity position just check checksums.
 *  If this is a data position check lba and checksum.
 *  Also handle the split across a parity range.
 *
 * @param siots_p - Ptr to current fbe_raid_siots_t.
 *   
 * @return fbe_status_t
 *
 * @notes
 *
 * @author
 *  11/18/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_parity_rekey_check_checksum(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t             status = FBE_STATUS_OK;
    fbe_raid_fruts_t        *read_fruts_ptr = NULL;
    fbe_u32_t                fru_index = 0;
    fbe_xor_execute_stamps_t execute_stamps;
    fbe_lba_t current_lba = siots_p->parity_start;
    fbe_block_count_t current_blocks = siots_p->geo.blocks_remaining_in_parity;
    fbe_raid_siots_geometry_t geo;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_u32_t parity_pos;
    fbe_u32_t num_parity;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_position_t rekey_pos;

    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_geometry_get_num_parity(raid_geometry_p, &num_parity);

    status = fbe_raid_get_rekey_pos(raid_geometry_p, iots_p->rekey_bitmask, &rekey_pos);

    if (status != FBE_STATUS_OK){ 
        return status; 
    }
    /*******************************************************
     * Check the checksums one fru at a time.
     * This is necessary since in this case, we do not have 
     * a single SG that references the entire range.
     *******************************************************/
    while (current_lba < siots_p->parity_start + siots_p->parity_count) {

        fbe_xor_lib_execute_stamps_init(&execute_stamps);

        fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

        while (read_fruts_ptr != NULL){
            fbe_sg_element_t *sg_p = NULL;
            fbe_raid_fruts_get_sg_ptr(read_fruts_ptr, &sg_p);
            execute_stamps.fru[fru_index].sg = sg_p;
            execute_stamps.fru[fru_index].seed = current_lba;
            execute_stamps.fru[fru_index].count = current_blocks;
            execute_stamps.fru[fru_index].offset = (fbe_u32_t)(current_lba - siots_p->parity_start);
            execute_stamps.fru[fru_index].position_mask = (1 << read_fruts_ptr->position);
            fru_index++;
    
            read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
        }
    
        /* Setup the drive count and eboard pointer.
         */
        execute_stamps.data_disks = fru_index;
        execute_stamps.array_width = fbe_raid_siots_get_width(siots_p);
        execute_stamps.eboard_p = fbe_raid_siots_get_eboard(siots_p);
        execute_stamps.eregions_p = (siots_p->vcts_ptr == NULL) ? NULL : &siots_p->vcts_ptr->error_regions;
    
        /* Default is no options.
         */
        execute_stamps.option = 0;
        status = fbe_parity_get_physical_geometry(raid_geometry_p, current_lba, &geo);
        if (status != FBE_STATUS_OK) {
            return status;
        }

        parity_pos = geo.position[raid_geometry_p->width - num_parity];

        /* As long as we are not parity, check the lba stamps.
         */
        if ( (rekey_pos != parity_pos) &&
             ((num_parity == 1) || (rekey_pos != parity_pos + 1)) ){
            execute_stamps.option |= FBE_XOR_OPTION_CHK_LBA_STAMPS;
        }
    
        /* Check checksums is required for both parity and data.
         */
        execute_stamps.option |= FBE_XOR_OPTION_CHK_CRC;

        /* Set XOR debug options.
         */
        fbe_raid_xor_set_debug_options(siots_p, 
                                       &execute_stamps.option, 
                                       FBE_FALSE    /* Don't generate error trace*/);

        /* Send xor command now. 
         */ 
        status = fbe_xor_lib_execute_stamps(&execute_stamps); 
    
        /* Save the status for later use.
         */
        fbe_raid_siots_set_xor_command_status(siots_p, execute_stamps.status);
        current_lba += current_blocks;
    }
    return status;
}
/**************************************
 * end fbe_parity_rekey_check_checksum()
 **************************************/
/*****************************************
 * End of fbe_parity_rekey_util.c
 *****************************************/

