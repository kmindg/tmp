/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_striper_verify_util.c
 ***************************************************************************
 *
 * @brief
 *   This file contains the utility functions of the
 *   striper verify state machine.
 *
 * @author
 *   2009 created for logical package. Rob Foley
 *
 ***************************************************************************/

/*************************
 **    INCLUDE FILES    **
 *************************/

#include "fbe_raid_library_proto.h"
#include "fbe_striper_io_private.h"
#include "fbe_raid_library.h"
#include "fbe_raid_error.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe/fbe_api_lun_interface.h"

/********************************
 *  static STRING LITERALS
 ********************************/

/*****************************************************
 *  static FUNCTIONS 
 *****************************************************/

static fbe_status_t fbe_striper_verify_strip_setup(fbe_raid_siots_t * siots_p,
                                                   fbe_xor_striper_verify_t *xor_verify_p);
fbe_status_t fbe_striper_verify_get_verify_count(fbe_raid_siots_t *siots_p,
                                                fbe_block_count_t *verify_count_p);
fbe_status_t fbe_striper_verify_setup_fruts(fbe_raid_siots_t * siots_p, 
                                           fbe_raid_fru_info_t *read_p,
                                           fbe_u32_t opcode,
                                           fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_striper_verify_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                                  fbe_u16_t *num_sgs_p,
                                                  fbe_raid_fru_info_t *read_p);
fbe_status_t fbe_striper_verify_setup_sgs_normal(fbe_raid_siots_t * siots_p,
                                                 fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_striper_rvr_setup_sgs(fbe_raid_siots_t *siots_p,
                                       fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_striper_verify_setup_sgs(fbe_raid_siots_t * siots_p,
                                          fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_striper_verify_count_sgs(fbe_raid_siots_t * siots_p,
                                          fbe_raid_fru_info_t *read_p);
fbe_status_t fbe_striper_rvr_count_sgs(fbe_raid_siots_t *siots_p, 
                                       fbe_raid_fru_info_t *read_p);

/****************************
 *  GLOBAL VARIABLES
 ****************************/

/*************************************
 *  EXTERNAL GLOBAL VARIABLES
 *************************************/
/*!**************************************************************
 * fbe_striper_verify_validate()
 ****************************************************************
 * @brief
 *  Make sure the verify is properly formed.
 *
 * @param siots_p - Current I/O.               
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE on failure.
 *
 ****************************************************************/

fbe_status_t fbe_striper_verify_validate(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_u32_t optimal_size;
    fbe_raid_siots_get_optimal_size(siots_p, &optimal_size);

    if (RAID_COND(fbe_raid_geometry_is_raid10(fbe_raid_siots_get_raid_geometry(siots_p))))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: mirror not expected algorithm: %d\n", siots_p->algorithm);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Validate the algorithm.
     */
    if (RAID_COND((siots_p->algorithm != RAID0_VR) && 
                  (siots_p->algorithm != RAID0_BVA_VR) &&
                  (siots_p->algorithm != RAID0_RD_VR)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: unexpected algorithm: %d\n", siots_p->algorithm);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate verify request is aligned to optimal block size.
     */
    if (RAID_COND((siots_p->parity_start % optimal_size) ||
                  (siots_p->parity_count % optimal_size)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "striper: unexpected alignment: ps: 0x%llx pc: 0x%llx alg: %d\n", 
                                            (unsigned long long)siots_p->parity_start, (unsigned long long)siots_p->parity_count, siots_p->algorithm);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Call the generic striper validate function.
     */
    status = fbe_striper_generate_validate_siots(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to validate field of siots: status = 0x%x, siots_p = 0x%p\n", 
                             status, siots_p);
        return (status);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_striper_verify_validate()
 ******************************************/
/*!***************************************************************
 * fbe_striper_verify_get_fruts_info()
 ****************************************************************
 * @brief
 *  This function initializes the fru info for verify.
 *
 * @param siots_p - Current sub request.
 * @param read_p - Array of read information.
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_striper_verify_get_fruts_info(fbe_raid_siots_t * siots_p,
                                               fbe_raid_fru_info_t *read_p)
{
    fbe_status_t status;
    fbe_block_count_t parity_range_offset;
    fbe_u16_t data_pos;
    fbe_u8_t *position = siots_p->geo.position;
    fbe_block_count_t verify_count;

    status = fbe_striper_verify_get_verify_count(siots_p, &verify_count);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) { return status; }
    
    parity_range_offset = siots_p->geo.start_offset_rel_parity_stripe;

    /* Initialize the fru info for data drives.
     */
    for (data_pos = 0; data_pos < siots_p->data_disks; data_pos++)
    {
        read_p->lba = siots_p->parity_start;
        read_p->blocks = verify_count;
        read_p->position = position[data_pos];
        read_p++;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_striper_verify_get_fruts_info
 **************************************/
/*!**************************************************************************
 * fbe_striper_verify_calculate_memory_size()
 ****************************************************************************
 * @brief  This function calculates the total memory usage of the siots for
 *         this state machine.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param read_info_p - Array of read information.
 * 
 * @return None.
 *
 **************************************************************************/

fbe_status_t fbe_striper_verify_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                      fbe_raid_fru_info_t *read_info_p)
{
    fbe_u16_t num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_u32_t num_fruts = 0;
    fbe_block_count_t total_blocks;

    fbe_status_t status;

    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));
    /* Then calculate how many buffers are needed.
     */
    status = fbe_striper_verify_get_fruts_info(siots_p, read_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) { return status; }

    num_fruts = fbe_raid_siots_get_width(siots_p);

    /* We get the sgs, so continue to allocate buffers.
     * Both types of verify simply verify a `block' worth
     * of data on each component.
     */
    if (1 || (siots_p->algorithm == RAID0_VR) ||
        (siots_p->algorithm == RAID0_BVA_VR))
    {
        total_blocks = fbe_raid_siots_get_width(siots_p) * siots_p->parity_count;
    }
    else
    {
        /* We allocate the max region size we will be performing, 
         * since it might not be the first region size. 
         */
        total_blocks = fbe_raid_siots_get_width(siots_p) * fbe_raid_siots_get_region_size(siots_p);

        /* Else recovery verify.  We will stitch together original buffers
         * so there is no need to allocate an sgl.
         */
    }
        
    siots_p->total_blocks_to_allocate = total_blocks;
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts, siots_p->total_blocks_to_allocate);

    /* Next calculate the number of sgs of each type.
     */
    status = fbe_striper_verify_calculate_num_sgs(siots_p, 
                                                  &num_sgs[0], 
                                                  read_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) { return status; }

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, &num_sgs[0],
                                                FBE_FALSE /* No nested siots is required*/);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) { return status; }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_striper_verify_calculate_memory_size()
 **************************************/

/*!**************************************************************
 * fbe_striper_verify_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param siots_p - current I/O.
 * @param read_info_p - Array of read information.
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_striper_verify_setup_resources(fbe_raid_siots_t * siots_p, 
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
                             "striper: failed to init memory info with status 0x%x, siots_p = 0x%p\n",
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

    /* Set up the FRU_TS.
     */
    status = fbe_striper_verify_setup_fruts(siots_p, 
                                            read_p,
                                            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                            &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to setup fruts with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }
    
    /* Initialize the RAID_VC_TS now that it is allocated.
     */
    status = fbe_raid_siots_init_vcts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to init vcts with status: 0x%x, siots_p = 0x%p\n",
                             status, siots_p);                              
        return (status);
    }

    /* Initialize the VR_TS.
     * We are allocating VR_TS structures WITH the fruts structures.
     */
    status = fbe_raid_siots_init_vrts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to init vrts with status: 0x%x, siots_p = 0x%p\n",
                             status, siots_p);                              
        return (status);
    }

    /* Plant the allocated sgs into the locations calculated above.
     */
    status = fbe_raid_fru_info_array_allocate_sgs(siots_p, &memory_info,
                                                  read_p, NULL, NULL);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to allocte sgs with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);                              
        return (status);
    }

    /* Assign buffers to sgs.
     * We reset the current ptr to the beginning since we always 
     * allocate buffers starting at the beginning of the memory. 
     */
    status = fbe_striper_verify_setup_sgs(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to setup sgs with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    return(status);
}
/******************************************
 * end fbe_striper_verify_setup_resources()
 ******************************************/

/*!**************************************************************
 * fbe_striper_verify_count_sgs()
 ****************************************************************
 * @brief
 *  Count how many sgs we need for the verify operation.
 *
 * @param siots_p - this I/O.
 * @param read_info_p - Array of read information.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_striper_verify_count_sgs(fbe_raid_siots_t * siots_p,
                                          fbe_raid_fru_info_t *read_p)
{
    fbe_block_count_t mem_left = 0;
    fbe_u32_t width;
    fbe_u32_t data_pos;

    width = fbe_raid_siots_get_width(siots_p);

    for (data_pos = 0; data_pos < width; data_pos++)
    {
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
        read_p->sg_index = fbe_raid_memory_sg_count_index(sg_count);
        if (RAID_COND(read_p->sg_index >= FBE_RAID_MAX_SG_TYPE))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "striper: read sg index %d unexpected for pos %d, read_p = 0x%p, siots_p = 0x%p\n", 
                                 read_p->sg_index, read_p->position, read_p, siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }   
        read_p++;
    }
    return FBE_STATUS_OK;
} 
/******************************************
 * end fbe_striper_verify_count_sgs()
 ******************************************/
/*!***************************************************************
 * fbe_striper_verify_setup_fruts()
 ****************************************************************
 * @brief
 *  This function will set up fruts for external verify.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 * @param read_p - Read info array.
 * @param opcode - opcode to set in fruts.
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_striper_verify_setup_fruts(fbe_raid_siots_t * siots_p, 
                                            fbe_raid_fru_info_t *read_p,
                                            fbe_u32_t opcode,
                                            fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status;
    fbe_u16_t data_pos;
    fbe_u16_t width = fbe_raid_siots_get_width(siots_p);

    /* Initialize the fruts for data drives.
     */
    for (data_pos = 0; data_pos < width; data_pos++)
    {
        status = fbe_raid_setup_fruts_from_fru_info(siots_p,
                                                    read_p,
                                                    &siots_p->read_fruts_head,
                                                    opcode,
                                                    memory_info_p);

        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "striper: setup fruts failed for pos %d with status 0x%x, "
                                 "siots_p = 0x%p, read_p = 0x%p\n",
                                 read_p->position, status, siots_p, read_p);
            return status;
        }
        read_p++;
    }
    return FBE_STATUS_OK;
}
/* end of fbe_striper_verify_setup_fruts() */

/*!**************************************************************************
 * fbe_striper_verify_calculate_num_sgs()
 ****************************************************************************
 * @brief  This function calculates the number of sg lists needed.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param num_sgs_p - Array of sgs we need to allocate, one array
 *                    entry per sg type.
 * @param read_p - Read information.
 * 
 * @return fbe_status_t
 *
 **************************************************************************/

fbe_status_t fbe_striper_verify_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                                 fbe_u16_t *num_sgs_p,
                                                 fbe_raid_fru_info_t *read_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t data_pos;
    fbe_u16_t width = fbe_raid_siots_get_width(siots_p);

    /* Page size must be set.
     */
    if (RAID_COND(!fbe_raid_siots_validate_page_size(siots_p)))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Count and allocate sgs first.
     */
    if ((siots_p->algorithm == RAID0_VR) ||
        (siots_p->algorithm == RAID0_BVA_VR))
    {
        /* Count the number of sgs for normal verify.
         */
        status = fbe_striper_verify_count_sgs(siots_p, read_p);    
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "striper: failed to count no of sgs for normal verify: "
                                 "status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);

            return status; 
        }
    }
    else
    {
        /* Else need to determine, count and generate
         * the number of sgs for each verify fruts.
         */
        status = fbe_striper_rvr_count_sgs(siots_p, read_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "striper: failed to count no of sgs for verify: "
                                 "status 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status; 
        }
    }

    /* Now take the sgs we just counted and add them to the input counts.
     */
    for (data_pos = 0; data_pos < width; data_pos++)
    {
        /* First determine the read sg type so we know which type of sg we
         * need to increase the count of. 
         */
        if (RAID_COND(read_p->blocks == 0))
        {
             fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                  "striper: verify read blocks is 0x%llx lba: %llx pos: %d\n",
                                  (unsigned long long)read_p->blocks,
				  (unsigned long long)read_p->lba,
				  read_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (RAID_COND(read_p->sg_index >= FBE_RAID_MAX_SG_TYPE))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "striper: read sg index %d unexpected for pos %d 0x%p \n", 
                                 read_p->sg_index, read_p->position, read_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }   
        num_sgs_p[read_p->sg_index]++;
        read_p++;
    }
    return status;
}
/**************************************
 * end fbe_striper_verify_calculate_num_sgs()
 **************************************/
/*!**************************************************************
 * fbe_striper_verify_setup_sgs_normal()
 ****************************************************************
 * @brief
 *  Setup the sg lists for the verify operation.
 *
 * @param siots_p - this I/O.   
 * @param memory_info_p - Pointer to memory request information          
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_striper_verify_setup_sgs_normal(fbe_raid_siots_t * siots_p,
                                                 fbe_raid_memory_info_t *memory_info_p)
{
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_status_t status;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    while (read_fruts_ptr != NULL)
    {
        fbe_sg_element_t *sgl_ptr;
        fbe_u16_t sg_total = 0;
        fbe_u32_t dest_byte_count =0;

        sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);

        if(!fbe_raid_get_checked_byte_count(read_fruts_ptr->blocks, &dest_byte_count))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "%s:byte count exceeding 32bit limit\n", __FUNCTION__);
            
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        /* Assign newly allocated memory for the whole verify region.
         */
        sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                    sgl_ptr, 
                                                    dest_byte_count);
        if (sg_total == 0)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "%s:failed to populate sg for: memory_info_p = 0x%p\n",
                                        __FUNCTION__, memory_info_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    /* Debug code to detect scatter gather overrun.
     * Typically this will get caught when we try to free the 
     * sg lists to the BM, but it is better to try and catch it sooner. 
     */
    status = fbe_raid_fruts_for_each(FBE_RAID_FRU_POSITION_BITMASK, read_fruts_ptr, fbe_raid_fruts_validate_sgs, 0, 0);
    
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to validate sgs: read_fruts_ptr 0x%p "
                             "siots_p 0x%p. status = 0x%x\n",
                             read_fruts_ptr, siots_p, status);
        return status;
    }
    return status;
}
/******************************************
 * end fbe_striper_verify_setup_sgs_normal()
 ******************************************/
/*!**************************************************************
 * fbe_striper_verify_setup_sgs_for_next_pass()
 ****************************************************************
 * @brief
 *  Setup the sgs for another strip mine pass.
 *
 * @param siots_p - current I/O.               
 *
 * @return fbe_status_t
 *
 * @author
 *  5/3/2010 - Created. kls
 *
 ****************************************************************/
fbe_status_t fbe_striper_verify_setup_sgs_for_next_pass(fbe_raid_siots_t * siots_p)
{
    fbe_raid_memory_info_t  memory_info;
    fbe_status_t            status;

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_data_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to init memory info with status: 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Assign buffers to sgs.
     * We reset the current ptr to the beginning since we always 
     * allocate buffers starting at the beginning of the memory. 
     */
    status = fbe_striper_verify_setup_sgs(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to setup sgs with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    return status;
}
/******************************************
 * end fbe_striper_verify_setup_sgs_for_next_pass()
 ******************************************/

/*!**************************************************************
 * fbe_striper_verify_setup_sgs()
 ****************************************************************
 * @brief
 *  Setup the sg lists for the verify operation.
 *
 * @param siots_p - this I/O. 
 * @param memory_info_p - Pointer to memory request information           
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_striper_verify_setup_sgs(fbe_raid_siots_t * siots_p,
                                          fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status;

    if (siots_p->algorithm == RAID0_RD_VR)
    {
        /* For recovery verify we need to handle the original host data.
         */
        status = fbe_striper_rvr_setup_sgs(siots_p, memory_info_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {  
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "striper: failed to setup sgs with status 0x%x, siots_p = 0x%p \n",
                                 status, siots_p);
        }
    }
    else
    {
        /* Set up the sgs.
         */
        status = fbe_striper_verify_setup_sgs_normal(siots_p, memory_info_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {  
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "striper: failed to setup sgs with status 0x%x, siots_p = 0x%p \n",
                                 status, siots_p);
        }
    }
    return status;
}
/******************************************
 * end fbe_striper_verify_setup_sgs()
 ******************************************/
/****************************************************************
 * fbe_striper_verify_strip()
 ****************************************************************
 * @brief
 *  This function goes through the request in the
 *  siots strip by strip to find any crc errors.  The
 *  errors that are found, are invalidated, and all
 *  error boards are updated.  Returns a bitmask
 *  representing the uncorectable fru's.
 *
 * @param siots - this io.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_striper_verify_strip(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_xor_striper_verify_t xor_verify;

    /***************************************************
     * Initialize the sg desc and bitkey vectors.
     ***************************************************/
    status = fbe_striper_verify_strip_setup(siots_p, &xor_verify);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) { return status; }

    /* The current pass counts for the vcts should be cleared since
     * we are beginning another XOR.  Later on, the counts from this pass
     * will be added to the counts from other passes.
     */
    fbe_raid_siots_init_vcts_curr_pass( siots_p );

    status = fbe_xor_lib_raid0_verify(&xor_verify);

    return status;
}
/**************************************
 * end fbe_striper_verify_strip()
 **************************************/
/****************************************************************
 * fbe_striper_verify_strip_setup()
 ****************************************************************
 * @brief
 *  This function initializes the sgd and bitkey for the strip mine
 *
 * @param siots - this io.
 * @param xor_verify_p - Ptr to info to return.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
static fbe_status_t fbe_striper_verify_strip_setup(fbe_raid_siots_t * siots_p,
                                                   fbe_xor_striper_verify_t *xor_verify_p)
{
    fbe_raid_fruts_t *fruts_p = NULL;

    fbe_raid_siots_get_read_fruts(siots_p, &fruts_p);

    /* According to the interface,
     * the seed is the same for all positions since we're doing a verify.
     */
    xor_verify_p->seed = fruts_p->lba;

    if (siots_p->parity_count > FBE_U32_MAX)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: verify count 0x%llx exceeding 32bit limit \n",
                              (unsigned long long)siots_p->parity_count);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    xor_verify_p->count = (fbe_u16_t)siots_p->parity_count;
    xor_verify_p->array_width = (fbe_u16_t)fbe_raid_siots_get_width(siots_p);
    xor_verify_p->eboard_p = &siots_p->misc_ts.cxts.eboard;
    xor_verify_p->error_counts_p = &siots_p->vcts_ptr->curr_pass_eboard;
    xor_verify_p->eregions_p = &siots_p->vcts_ptr->error_regions;

    /* Invalidate parity on CRC error?
     *  Yes if we already retried, no if not yet retried.
     */
    xor_verify_p->b_retrying = fbe_raid_siots_is_retry(siots_p);

    /* Init sg and position mask.
     */
    while (fruts_p != NULL)
    {
        if (RAID_COND(fruts_p->position >= xor_verify_p->array_width))
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "fruts_p->position 0x%x >= xor_verify_p->array_width 0x%x\n",
                                 fruts_p->position,
                                 xor_verify_p->array_width);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        xor_verify_p->fru[fruts_p->position].sg_p = fbe_raid_fruts_return_sg_ptr(fruts_p);

        /* Init the bitkey for this position.
         */
        xor_verify_p->position_mask[fruts_p->position] = (1 << fruts_p->position);

        fruts_p = fbe_raid_fruts_get_next(fruts_p);
    }
    return FBE_STATUS_OK;
}
/* end fbe_striper_verify_strip_setup */



/*!**************************************************************
 * fbe_striper_report_corrupt_operations()
 ****************************************************************
 * @brief
 *  This function will report errors at the time of corrupt
 *  CRC as well as DATA operation.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_status_t
 *
 * @author
 *  03/08/11 - Created - Jyoti Ranjan
 ****************************************************************/
static fbe_status_t fbe_striper_report_corrupt_operations(fbe_raid_siots_t * siots_p)
{
    fbe_u32_t region_index;
    fbe_u32_t error_code;
    fbe_u32_t vr_error_bits;
    fbe_u32_t array_pos;
    fbe_u16_t array_pos_mask;
    const fbe_xor_error_regions_t * regions_p;


    vr_error_bits = 0;
    regions_p = &siots_p->vcts_ptr->error_regions;

    for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++)
    {
        /* array_pos_mask has the current position in the error bitmask 
         * that we need to check below.
         */
        array_pos_mask = 1 << array_pos;
        
        /* traverse the xor error region for current disk position
         */
        for (region_index = 0; region_index < regions_p->next_region_index; ++region_index)
        {
            if ((regions_p->regions_array[region_index].error != 0) &&
                (regions_p->regions_array[region_index].pos_bitmask & array_pos_mask))
            {
                /* Determine the error reason code.
                 */
                error_code = regions_p->regions_array[region_index].error & FBE_XOR_ERR_TYPE_MASK;
                switch(error_code)
                {
                    case FBE_XOR_ERR_TYPE_CORRUPT_CRC:
                    case FBE_XOR_ERR_TYPE_CORRUPT_CRC_INJECTED:
                        vr_error_bits |= VR_CORRUPT_CRC;
                        break;

                    case FBE_XOR_ERR_TYPE_CORRUPT_DATA:
                    case FBE_XOR_ERR_TYPE_CORRUPT_DATA_INJECTED:
                        vr_error_bits |= VR_CORRUPT_DATA;
                        break;
                }

                /* We will log message even if drive is dead as we want to
                 * notify that correspondign sector of drive is lost.
                 */
                fbe_raid_send_unsol(siots_p,
                                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,
                                    array_pos,
                                    regions_p->regions_array[region_index].lba,
                                    regions_p->regions_array[region_index].blocks,
                                    vr_error_bits,
                                    fbe_raid_extra_info(siots_p));

            } /* end if ((regions_p->regions_array[region_idx].error != 0) ... */
        } /* end for (region_idx = 0; region_idx < regions_p->next_region_index; ++region_idx) */
    } /* end for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++) */ 

    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_striper_report_corrupt_operations()
 **********************************************/


/*!***************************************************************
 * fbe_striper_report_errors()
 ****************************************************************
 * @brief
 *  This function will record the errors encountered in verify
 *  to the Flare ulog.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 * @param b_update_eboard - a flag indicating whether vp eboard has to
 *                          be updated or not with siot's vcts' overall 
 *                          eboard. We will do update only if flag
 *                          is FBE_TRUE.
 *
 * @return fbe_status_t
 *
 * @author
 *  01/28/11 - Created. Jyoti Ranjan
 *
 ****************************************************************/
fbe_status_t fbe_striper_report_errors(fbe_raid_siots_t * siots_p, fbe_bool_t b_update_eboard)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t disk_pba;
    fbe_u32_t blocks;
    fbe_u16_t array_pos;


    
    /* Report event log message because of corrupt CRC/DATA operation.
     */
    if ((fbe_raid_siots_is_corrupt_operation(siots_p)) &&
        (siots_p->vcts_ptr != NULL))
    {
        const fbe_xor_error_regions_t * const regions_p = &siots_p->vcts_ptr->error_regions;
        if (!FBE_XOR_ERROR_REGION_IS_EMPTY(regions_p))
        {
            status = fbe_striper_report_corrupt_operations(siots_p);
            if (status != FBE_STATUS_OK) { return status; }
        } 
    } /* end if (fbe_raid_siots_is_corrupt_operation(siots_p)) ... */
    else
    {
        /* If we are here then current operation is other than corrupt CRC/DATA
         * operation. Let us report event log messages.
         *
         * When we are logging the error from eboard, we will only have the 
         * fru position and there is not information available about the number of 
         * blocks, hence it is initialized to one. 
         * 
         * @technicaldebt In future we will use XOR error region to report event log messages for RAID 0 also.
         */
        blocks = FBE_RAID_MIN_ERROR_BLOCKS;
        disk_pba = siots_p->geo.start_offset_rel_parity_stripe;


        /* Loop through all valid disk position and report errors if needed.
         */
        for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++)
        {
            fbe_u32_t sunburst = 0;
            fbe_u32_t uc_err_bits, c_err_bits;
            fbe_u16_t array_pos_mask = 1 << array_pos;        
            
            /* Determine if there are correctable errors, since we need to log them.
             */
            uc_err_bits = fbe_raid_get_uncorrectable_status_bits( &siots_p->vrts_ptr->eboard, array_pos_mask );
            if (uc_err_bits != 0 )
            {
                /* Un-correctable errors.
                 *
                 * Log both an uncorrectable sector and a sector invalidated.
                 * We do this for historical reasons, this is what we have always done.
                 *
                 * It also indicates 2 things, 1) we took an uncorrectable sector
                 * and 2) we intentionally wrote a bad checksum to that sector.
                 *
                 */
                if ((siots_p->algorithm == RAID0_RD) &&
                    ((uc_err_bits == VR_CORRUPT_CRC) || 
                     (uc_err_bits == VR_INVALID_CRC) ||
                     (uc_err_bits == VR_RAID_CRC)       )     )
                {
                       /* If this is a read operation and the block has been invalidated 
                        * already do not log anything. This will cause errors on  reads 
                        * to be logged on the first encounter and no future ones. So.
                        * give a pass to these cases.
                        */
                }
                else
                {
                       fbe_raid_send_unsol(siots_p,
                                           SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,
                                           array_pos,
                                           disk_pba,
                                           blocks,
                                           fbe_raid_error_info(uc_err_bits),
                                           fbe_raid_extra_info(siots_p));

                       fbe_raid_send_unsol(siots_p,
                                           SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,
                                           array_pos,
                                           disk_pba,
                                           blocks,
                                           fbe_raid_error_info(uc_err_bits),
                                           fbe_raid_extra_info(siots_p));
                }

            } /* end if (uc_err_bits != 0 ) */

            /* Report event log messages because of correctable errors if we have.
             * At this point of time, the only correctable errors on R0 is a retryable 
             * crc error. And hence, we are not checking for unexepced CRC error
             * and to kill the drive if required.
             */
            c_err_bits = fbe_raid_get_correctable_status_bits( siots_p->vrts_ptr, array_pos_mask );
            if (c_err_bits != 0 )
            {
                /* Correctable errors are reported from highest to lowest priority. 
                 * Thus LBA stamp errors are the highest.
                 */
                if ((c_err_bits & VR_REASON_MASK) == VR_LBA_STAMP)
                {
                    /* We want to cause a phone-home for LBA stamp errors since it 
                     * could indicate that the drive is doing something bad.
                     */
                    sunburst = SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR;
                }
                else
                {
                    sunburst = SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED;
                }

                fbe_raid_send_unsol(siots_p,
                                    sunburst,
                                    array_pos,
                                    disk_pba,
                                    blocks,
                                    fbe_raid_error_info(c_err_bits),
                                    fbe_raid_extra_info(siots_p));
            } /* end if (c_err_bits != 0 ) */ 
        } /* end of for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++) */
    } /* end else-if (fbe_raid_siots_is_corrupt_operation(siots_p))... */

    /* Update the IOTS' error counts with the totals from this siots.
     * We will do so only if we are asked to do so.
     */
    if (b_update_eboard)
    {
        fbe_raid_siots_update_iots_vp_eboard( siots_p );
    }

    return FBE_STATUS_OK;
}                               
/* end of fbe_striper_report_errors() */

/*!***************************************************************
 * fbe_striper_bva_setup_nsiots()
 ****************************************************************
 * @brief
 *      This function set up the field needed for nested SIOTS
 *      for BVA verify.
 *
 * @param siots_p - pointer to fbe_raid_siots_t data
 *
 * @return fbe_status_t.
 ****************************************************************/
fbe_status_t fbe_striper_bva_setup_nsiots(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_siots_t       *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(parent_siots_p);

    /* Set up the geo info.
     */
    siots_p->geo = parent_siots_p->geo;

    /* Fill out parity range.  If the request needs to be aligned then
     * re-generate an aligned request.  The lock is always aligned to
     * the required alignment.
     */
    if (fbe_raid_geometry_io_needs_alignment(raid_geometry_p,
                                             parent_siots_p->parity_start,
                                             parent_siots_p->parity_count)) {
        siots_p->parity_start = parent_siots_p->parity_start;
        siots_p->parity_count = parent_siots_p->parity_count;
        fbe_raid_geometry_align_io(raid_geometry_p,
                                   &siots_p->parity_start,
                                   &siots_p->parity_count);
    } else {
        siots_p->parity_start = parent_siots_p->parity_start;
        siots_p->parity_count = parent_siots_p->parity_count;
    }

    /* Set the start and xfer count.  For a physical request it is the
     * same as the parity.
     */
    siots_p->start_lba = siots_p->parity_start;
    siots_p->xfer_count = siots_p->parity_count;

    /* During BVA we will verify the full stripe affected by write
     */
    siots_p->data_disks =  fbe_raid_siots_get_width(siots_p);

    if (RAID_COND((siots_p->xfer_count <= 0) ||
                  (siots_p->parity_count <= 0) ||
                  (siots_p->parity_start == FBE_LBA_INVALID))) {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "Striper: Invalid parity range, xfer count 0x%llx"
                             "parity count 0x%llx, parity start %llx \n",
			     (unsigned long long)siots_p->xfer_count,
			     (unsigned long long)siots_p->parity_count,
			     (unsigned long long)siots_p->parity_start);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}                          /* end of fbe_striper_bva_setup_nsiots() */


/*!***************************************************************
 * fbe_striper_generate_expanded_request() 
 ****************************************************************
 *
 * @brief   The routine expands the original (logical) request to the 
 *          physical range required by a verify operation.  It is assumed that
 *          the element size is a multiple of the optimal block size.
 *
 * @param   siots_p - Pointer to recovery verify siots
 *
 * @return  None
 *
 * @note    Although the updated request is updated to the physical
 *          stripe boundry, the start_lba and xfer_count are still
 *          logical values.  This is done because the iots lock request
 *          is also logical.  Later when we need to issue the verify
 *          request the siots range will be converted to a physical
 *          verify range.
 *
 * @note    Expand the logical request to the physical stripe boundry.
 *          By definition the PBA for each element in a stripe is the same.
 *                          PBA            LBA                 LBA                 LBA
 *                               +--------+          +--------+          +--------+
 *                               :        :          :        :          :        :
 * address_offset------->0x11200 +========+0x00000   +========+0x00080   +========+0x00100
 * logical_parity_start->0x00000 |        |          |ioioioio|          |ioioioio|end lba:
 * stripe_index = 0              |        |          |ioioioio|          |ioio    |0x0012F
 * original start_lba = 0x00050  |    ioio|0x00050   |ioioioio|          |        |
 * stripe_size = 0x180           |ioioioio|          |ioioioio|          |        |
 * logical_parity_start->0x00080 +--------+0x00180   +--------+0x00200   +--------+0x00280
 *                               :        :          :        :          :        :
 * In the example above the original logical request with a start_lba of 0x00050
 * and a xfer_count of 0x000E0 (end lba of 0x0012F) is expanded to the logical
 * stripe range of lba 0x00000 thru 0x0017F.  This corresponds with a physical
 * logical_parity_start value of 0x00000 and a physical transfer count of 0x00080.
 *
 ****************************************************************/
fbe_status_t fbe_striper_generate_expanded_request(fbe_raid_siots_t *const siots_p)
{
    fbe_raid_siots_t  *parent_siots_p;
    fbe_element_size_t         element_size, stripe_size;
    fbe_lba_t           new_start_lba, new_end_lba;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_extent_t parity_range[FBE_RAID_MAX_PARITY_EXTENTS];
    fbe_u16_t data_disks;
    fbe_u32_t optimal_size;
    fbe_u16_t array_width;
    fbe_raid_siots_get_optimal_size(siots_p, &optimal_size);

    /* Get the parent siots
     */
    parent_siots_p = fbe_raid_siots_get_parent(siots_p);

    /* Use the logical geometry to get the logical
     * parity start (for the stripe or stripes required for this
     * requests) then calculate and update the logical range for
     * this siots.  The maximum window size is simply the max_blocks
     * from the geometry.  Since this code supports mirrors and ids,
     * we must use the true element size from dba.
     */
    element_size = fbe_raid_siots_get_blocks_per_element(parent_siots_p);
    stripe_size = fbe_raid_siots_get_width(parent_siots_p) * element_size;

    /* Fill out the parity range from the geometry generated.  Round
     * to the stripe multiple.
     */
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    fbe_raid_get_stripe_range(parent_siots_p->start_lba,
                              parent_siots_p->xfer_count,
                              fbe_raid_siots_get_blocks_per_element(siots_p),
                              data_disks,
                              FBE_RAID_MAX_PARITY_EXTENTS,    /* max extents */
                              parity_range);

    /* Fill out the parity range from the
     * values returned by fbe_raid_get_stripe_range
     */
    siots_p->parity_start = parity_range[0].start_lba;

    if (parity_range[1].size != 0)
    {
        /* Discontinuous parity range.
         * For parity count, we just take the entire parity range.
         */
        fbe_lba_t end_pt = (parity_range[1].start_lba +
                            parity_range[1].size - 1);
        siots_p->parity_count =(fbe_u32_t)( end_pt - parity_range[0].start_lba + 1);
    }
    else
    {
        siots_p->parity_count = parity_range[0].size;
    }

    /* Simply use the parent range and round to a stripe multiple.
     */
    new_start_lba = siots_p->parity_start;
    new_end_lba = new_start_lba + (siots_p->parity_count - 1);

    /* Now generate the logical range by rounding the parity values
     * Notice that we don't modify the parent values.
     */
    fbe_raid_geometry_align_lock_request(raid_geometry_p, &new_start_lba, &new_end_lba);
    siots_p->parity_start = new_start_lba;
    siots_p->parity_count = new_end_lba - new_start_lba + 1;

    if (RAID_COND(siots_p->parity_count == 0))
    {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                          "striper: Invalid parity count 0x%llx\n",
                          siots_p->parity_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_striper_get_physical_geometry(raid_geometry_p, siots_p->parity_start, &siots_p->geo, &array_width);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to get geometry from siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return (status);
    }

    siots_p->parity_pos = fbe_raid_siots_get_parity_pos(siots_p);
    if (RAID_COND(siots_p->parity_pos == FBE_RAID_INVALID_PARITY_POSITION))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: siots_p->parity_pos 0x%x == FBE_RAID_INVALID_PARITY_POSITION\n",
                             siots_p->parity_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    siots_p->start_pos = fbe_raid_siots_get_start_pos(siots_p);
    siots_p->data_disks = fbe_raid_siots_get_width(siots_p);

    /* Set the required fields in the siots.
     */
    siots_p->drive_operations = fbe_raid_siots_get_width(siots_p);
    siots_p->algorithm = RAID0_RD_VR;
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;
    fbe_raid_siots_log_traffic(siots_p, "r0 rvr expand");

    return status;
} /* end fbe_striper_generate_expanded_request() */

/*!***************************************************************
 * fbe_striper_rvr_count_sgs ()
 ****************************************************************
 *
 * @brief   Depending on the type of recovery (a.k.a read) verify
 *          we are performing:
 *              o Single block mode
 *              o Full request verify
 *          Count and setup the sgl for each of the verify fruts.
 *
 * @param siots_p - Pointer to verify siots
 * @param read_p - Pointer to read info array.
 *
 * @return fbe_status_t
 *
 * @note    None
 *
 ****************************************************************/
fbe_status_t fbe_striper_rvr_count_sgs(fbe_raid_siots_t *siots_p, 
                                       fbe_raid_fru_info_t *read_p)
{
    fbe_block_count_t verify_count;
    fbe_lba_t verify_start = siots_p->parity_start;
    fbe_status_t status;
    fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
    fbe_raid_fruts_t *parent_read_fruts_ptr = NULL;
    fbe_raid_fruts_t *parent_read2_fruts_ptr = NULL;
    fbe_u32_t sg_size;
    fbe_block_count_t mem_left = 0;
    const fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;
    fbe_u32_t data_pos;
    fbe_u32_t width = fbe_raid_siots_get_width(siots_p);
    fbe_sg_element_t *overlay_sg_ptr = NULL;

    status = fbe_striper_verify_get_verify_count(siots_p, &verify_count);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) { return status; }

    fbe_raid_siots_get_read_fruts(parent_siots_p, &parent_read_fruts_ptr);
    fbe_raid_siots_get_read2_fruts(parent_siots_p, &parent_read2_fruts_ptr);

    for (data_pos = 0; data_pos < width; data_pos++)
    {
        sg_size = 0;

        if ((parent_read_fruts_ptr == NULL) && (parent_read2_fruts_ptr == NULL))
        {
            /* SG to point to the will-be allocated new buffers.
             */
            mem_left = fbe_raid_sg_count_uniform_blocks(verify_count,
                                                        siots_p->data_page_size,
                                                        mem_left,
                                                        &sg_size);
        }
        else if (parent_read_fruts_ptr)
        {
            if (parent_read_fruts_ptr->position != read_p->position)
            {
                /* SG to point to the will-be allocated new buffers.
                 */
                mem_left = fbe_raid_sg_count_uniform_blocks(verify_count,
                                                            siots_p->data_page_size,
                                                            mem_left,
                                                            &sg_size);
            }
            else
            {
                /* 1. calculate from parent_read_futs_ptr,
                 * 2. allocate new ones, 
                 * 3. calculate from parent_read2_fruts_ptr.
                 */
                fbe_lba_t relative_fruts_lba,   /* The Parity relative FRUTS lba. */
                    end_lba,
                    verify_end;
                fbe_block_count_t fruts_blocks; /* The first fruts block count.   */
                fbe_block_count_t blks_assigned = 0;

                if (RAID_COND(verify_start < siots_p->start_lba))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "Striper: verify_start %llx < siots_p->start_lba %llx\n",
                                        (unsigned long long)verify_start,
					(unsigned long long)siots_p->start_lba);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* Since the fruts lba is a physical lba, and the
                 * verify_start is a parity relative logical address,
                 * map the fruts lba to a parity logical address.
                 */
                relative_fruts_lba = logical_parity_start;
                status = fbe_raid_map_pba_to_lba_relative(parent_read_fruts_ptr->lba,
                                                 parent_read_fruts_ptr->position,
                                                 siots_p->parity_pos,
                                                 siots_p->start_lba,
                                                 width,
                                                 &relative_fruts_lba,
                                                 fbe_raid_siots_get_parity_stripe_offset(siots_p),
                                                 fbe_raid_siots_parity_num_positions(siots_p));
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "striper: failed to map pba to lba: status = 0x%x, siots_p = 0x%p\n",
                                         status, siots_p);
                    return status;
                }

                fruts_blocks = parent_read_fruts_ptr->blocks;

                end_lba = relative_fruts_lba + fruts_blocks - 1;

                verify_end = verify_start + verify_count - 1;

                /* Count sgs to overlay first read region.
                 */
                {
                    fbe_u32_t current_sg_size = 0;
                    fbe_raid_fruts_get_sg_ptr(parent_read_fruts_ptr, &overlay_sg_ptr);
                    mem_left = fbe_raid_determine_overlay_sgs(relative_fruts_lba,
                                                              fruts_blocks,
                                                              siots_p->data_page_size,
                                                              overlay_sg_ptr,
                                                              verify_start,
                                                              verify_count,
                                                              mem_left,
                                                              &blks_assigned,
                                                              &current_sg_size);
                    sg_size += current_sg_size;
                }

                /* If there are blocks remaining to be overlayed, then
                 * we will check the 2nd pre-read
                 * or just assign the remaining blocks.
                 */
                if (blks_assigned < verify_count)
                {
                    fbe_block_count_t extra_blocks;
                    fbe_u32_t current_sg_size = 0;

                    extra_blocks = verify_count - blks_assigned;

                    if ((parent_read2_fruts_ptr) &&
                        (parent_read2_fruts_ptr->position == read_p->position))
                    {
                        fbe_block_count_t more_blks = 0;
                        fbe_lba_t relative_fruts2_lba;

                        if (RAID_COND(verify_start < siots_p->start_lba))
                        {
                            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "striper: verify_start %llx < siots_p->start_lba %llx\n",
                                        (unsigned long long)verify_start,
					(unsigned long long)siots_p->start_lba);
                            return FBE_STATUS_GENERIC_FAILURE;
                        }
                        /* Since the fruts lba is a physical lba, and the
                         * verify_start is a parity relative logical address,
                         * map the fruts lba to a parity logical address.
                         */
                        relative_fruts2_lba = logical_parity_start;
                        status = fbe_raid_map_pba_to_lba_relative(parent_read2_fruts_ptr->lba,
                                                       parent_read2_fruts_ptr->position,
                                                       siots_p->parity_pos,
                                                       siots_p->start_lba,
                                                       width,
                                                       &relative_fruts2_lba,
                                                       fbe_raid_siots_get_parity_stripe_offset(siots_p),
                                                       fbe_raid_siots_parity_num_positions(siots_p));
                        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                        {
                            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                                 "striper: failed to map pba to lba: status = 0x%x, siots_p = 0x%p\n",
                                                 status, siots_p);
                            return status;
                        }
                        
                        fbe_raid_fruts_get_sg_ptr(parent_read2_fruts_ptr, &overlay_sg_ptr);
                        mem_left = fbe_raid_determine_overlay_sgs(relative_fruts2_lba,
                                                                  parent_read2_fruts_ptr->blocks,
                                                                  siots_p->data_page_size,
                                                                  overlay_sg_ptr,
                                                                  verify_start + blks_assigned,
                                                                  extra_blocks,
                                                                  mem_left,
                                                                  &more_blks,
                                                                  &current_sg_size);

                        if (RAID_COND(more_blks != extra_blocks))
                        {
                            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                                 "striper: more_blks %llu != extra_blocks %llu\n",
                                                 (unsigned long long)more_blks,
						 (unsigned long long)extra_blocks);
                            return FBE_STATUS_GENERIC_FAILURE;
                        }

                        sg_size += current_sg_size;

                        parent_read2_fruts_ptr = fbe_raid_fruts_get_next(parent_read2_fruts_ptr);
                    }
                    else
                    {
                        /* Need more SGs, if necessary.
                         */
                        mem_left = fbe_raid_sg_count_uniform_blocks(extra_blocks,
                                                              siots_p->data_page_size,
                                                              mem_left,
                                                              &current_sg_size);
                        sg_size += current_sg_size;
                    }
                }
                else
                {
                    if (RAID_COND(blks_assigned != verify_count))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                             "striper: blks_assigned %llu != verify_count %llu\n",
                                             (unsigned long long)blks_assigned,
					     (unsigned long long)verify_count);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }

                    if ((parent_read2_fruts_ptr) &&
                        (parent_read2_fruts_ptr->position == read_p->position))
                    {
                        parent_read2_fruts_ptr = fbe_raid_fruts_get_next(parent_read2_fruts_ptr);
                    }
                }

                parent_read_fruts_ptr = fbe_raid_fruts_get_next(parent_read_fruts_ptr);
            }
        }
        else
        {
            /* Only a parent read2 fruts ptr.
             */
            if (RAID_COND(parent_read2_fruts_ptr == NULL))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "Striper: parent_read2_fruts_ptr == NULL\n");
                return FBE_STATUS_GENERIC_FAILURE;
            }

            if (RAID_COND((siots_p->algorithm != R5_MR3_VR) && (siots_p->algorithm != R5_DEG_VR)))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "striper: unexpected algorithm %d, siots_p = 0x%p\n",
                                     siots_p->algorithm, siots_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            if (read_p->position != parent_read2_fruts_ptr->position)
            {
                /* SG to point to the will-be allocated new buffers.
                 */
                mem_left = fbe_raid_sg_count_uniform_blocks(verify_count,
                                                      siots_p->data_page_size,
                                                      mem_left,
                                                      &sg_size);
            }
            else
            {
                fbe_block_count_t blks_assigned = 0;
                fbe_lba_t relative_fruts2_lba;

                if (RAID_COND(verify_start < siots_p->start_lba))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "striper: verify_start %llx < siots_p->start_lba %llx\n",
                                         (unsigned long long)verify_start,
					 (unsigned long long)siots_p->start_lba);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                if (RAID_COND(read_p->position != parent_read2_fruts_ptr->position))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "striper: read_fruts_ptr->position %d != parent_read2_fruts_ptr->position %d\n",
                                         read_p->position, parent_read2_fruts_ptr->position);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* Since the fruts lba is a physical lba, and the
                 * verify_start is a parity relative logical address,
                 * map the fruts lba to a parity logical address.
                 */
                relative_fruts2_lba = logical_parity_start;
                status = fbe_raid_map_pba_to_lba_relative(parent_read2_fruts_ptr->lba,
                                               parent_read2_fruts_ptr->position,
                                               siots_p->parity_pos,
                                               siots_p->start_lba,
                                               width,
                                               &relative_fruts2_lba,
                                               fbe_raid_siots_get_parity_stripe_offset(siots_p),
                                               fbe_raid_siots_parity_num_positions(siots_p));
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "striper: failed to map pba to lba: status = 0x%x, siots_p = 0x%p\n",
                                         status, siots_p);
                    return status;
                }

                /* Calculate the overlay sgs needed for this read2 base region.
                 */
                fbe_raid_fruts_get_sg_ptr(parent_read2_fruts_ptr, &overlay_sg_ptr);
                mem_left = fbe_raid_determine_overlay_sgs(relative_fruts2_lba,
                                                          parent_read2_fruts_ptr->blocks,
                                                          siots_p->data_page_size,
                                                          overlay_sg_ptr,
                                                          verify_start,
                                                          verify_count,
                                                          mem_left,
                                                          &blks_assigned,
                                                          &sg_size);

                if (RAID_COND(blks_assigned != verify_count))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "striper: blks_assigned 0x%llx != verify_count 0x%llx\n",
                                         (unsigned long long)blks_assigned,
					 (unsigned long long)verify_count);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                parent_read2_fruts_ptr = fbe_raid_fruts_get_next(parent_read2_fruts_ptr);
            }
        }
        /* For case where we have an optimal block size != region size, 
         * it will be possible to have additional sgs for the last region mine. 
         */
        sg_size += 2;
        status = fbe_raid_fru_info_set_sg_index(read_p, sg_size, FBE_TRUE /* yes log */);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "striper: failed to set sg index: status = 0x%x, siots_p = 0x%p, "
                                 "read_p = 0x%p\n", 
                                 status, siots_p, read_p);

            return status;
        }
        read_p++;
    }

    if (RAID_COND(((parent_read_fruts_ptr != NULL) || (parent_read2_fruts_ptr != NULL)) && 
                  (parent_read_fruts_ptr->position != siots_p->parity_pos)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: unexpected error: (parent_read_fruts_ptr or parent_read2_fruts_ptr "
                             "is NULL) and  parent_read_fruts_ptr->position != siots_p->parity_pos\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/* end fbe_striper_rvr_count_sgs */

/*!***************************************************************
 * fbe_striper_rvr_setup_sgs ()
 ****************************************************************
 * 
 * @brief   The routine setsup (i.e. plants) the buffers required
 *          for a RAID-0 recovery verify.  Basically we need to stitch
 *          together the pre-read blocks with the original read blocks.
 *          We are attempting to use the verify result to generate the
 *          requested read data.
 *
 * @param   siots_p    - ptr to the fbe_raid_siots_t
 * @param   memory_info_p - Pointer to memory request information
 *
 * @return  fbe_status_t
 *
 * @note    None
 *
 ****************************************************************/

fbe_status_t fbe_striper_rvr_setup_sgs(fbe_raid_siots_t * siots_p,
                                       fbe_raid_memory_info_t *memory_info_p)
{
    fbe_block_count_t verify_count;
    fbe_lba_t verify_start = siots_p->parity_start;
    fbe_status_t status;
    fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *parent_read_fruts_ptr = NULL;
    fbe_raid_fruts_t *parent_read2_fruts_ptr = NULL;
    const fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;
    fbe_u32_t width = fbe_raid_siots_get_width(siots_p);
    fbe_sg_element_t *overlay_sg_ptr = NULL;
    fbe_u32_t dest_byte_count =0;

    status = fbe_striper_verify_get_verify_count(siots_p, &verify_count);

    /* we will used the destination byte count to populate the sg
     * make sure it is within 32bit boundary
     */
    if(!fbe_raid_get_checked_byte_count(verify_count, &dest_byte_count))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                               "%s: byte count exceeding 32bit limit \n",
                                               __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) { return status; }

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_siots_get_read_fruts(parent_siots_p, &parent_read_fruts_ptr);
    fbe_raid_siots_get_read2_fruts(parent_siots_p, &parent_read2_fruts_ptr);

    while (read_fruts_ptr)
    {

        /* Can only perform a recovery verify for the size of the original request.
         * Therefore only plant sgs for those positions that have work remaining.
         */
        if (read_fruts_ptr->blocks > 0)
        {
            fbe_u32_t sg_total =0;
            fbe_sg_element_t *sgl_ptr;

            sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);

            if ((parent_read_fruts_ptr == NULL) && (parent_read2_fruts_ptr == NULL))
            {
                /* Assign newly allocated memory for the whole verify region.
                 */
                sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                                                             sgl_ptr, 
                                                                                             dest_byte_count);
                if (sg_total == 0)
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                             "%s: failed to populate sg for: memory_info_p = 0x%p\n",
                                             __FUNCTION__,memory_info_p);
                        return FBE_STATUS_GENERIC_FAILURE;
                }
            }
            else if (parent_read_fruts_ptr)
            {
                if (parent_read_fruts_ptr->position != read_fruts_ptr->position)
                {
                    /* Assign newly allocated memory for the whole verify region.
                     */
                    sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                                                                 sgl_ptr, 
                                                                                                 dest_byte_count);
                    if(sg_total == 0)
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                                 "%s: failed to populate sg for: memory_info_p = 0x%p\n",
                                                 __FUNCTION__,memory_info_p);
                            return FBE_STATUS_GENERIC_FAILURE;
                    }
                }
                else
                {
                    /* 1. Assign buffers from parent_read_fruts_ptr,
                     * 2. Assign new ones,
                     * 3. Assign buffers from parent_read2_fruts_ptr.
                     */
                    fbe_lba_t relative_fruts_lba,   /* The Parity relative FRUTS lba. */
                        end_lba,
                        verify_end;
                    fbe_block_count_t fruts_blocks; /* The first fruts block count.   */
                    fbe_block_count_t blks_assigned = 0;

                    if (RAID_COND(parent_read_fruts_ptr->position != read_fruts_ptr->position))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                             "striper: parent_read_fruts_ptr->position %d != read_fruts_ptr->position %d\n",
                                             parent_read_fruts_ptr->position, read_fruts_ptr->position);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                    if (RAID_COND(verify_start < siots_p->start_lba))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                             "striper: verify_start %llx < siots_p->start_lba %llx\n",
                                             (unsigned long long)verify_start,
					     (unsigned long long)siots_p->start_lba);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }

                    /* Since the fruts lba is a physical lba, and the
                     * verify_start is a parity relative logical address,
                     * map the fruts lba to a parity logical address.
                     */
                    relative_fruts_lba = logical_parity_start;
                    status = fbe_raid_map_pba_to_lba_relative(parent_read_fruts_ptr->lba,
                                               parent_read_fruts_ptr->position,
                                               siots_p->parity_pos,
                                               siots_p->start_lba,
                                               width,
                                               &relative_fruts_lba,
                                               fbe_raid_siots_get_parity_stripe_offset(siots_p),
                                               fbe_raid_siots_parity_num_positions(siots_p));
                    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                             "striper: failed to map pba to lba: status = 0x%x, siots_p = 0x%p\n",
                                             status, siots_p);
                        return status;
                    }

                    fruts_blocks = parent_read_fruts_ptr->blocks;

                    end_lba = relative_fruts_lba + fruts_blocks - 1;
                    verify_end = verify_start + verify_count - 1;

                    /* Fill in the FRUTS' sg list by copying the sgs
                     * from the already allocated buffers in the
                     * parent's read_fruts.
                     */
                    fbe_raid_fruts_get_sg_ptr(parent_read_fruts_ptr, &overlay_sg_ptr);
                    status = fbe_raid_verify_setup_overlay_sgs(relative_fruts_lba,
                                                               fruts_blocks,
                                                               overlay_sg_ptr,
                                                               sgl_ptr,
                                                               memory_info_p,
                                                               verify_start,
                                                               verify_count,
                                                               &blks_assigned);
                    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                             "striper: failed to overlay sgs : status = 0x%x, siots_p = 0x%p\n",
                                             status, siots_p);
                        return status;
                    }

                    if (blks_assigned < verify_count)
                    {
                        fbe_block_count_t  extra_blocks;

                        extra_blocks = verify_count - blks_assigned;

                        /* Adjust the sgl_ptr.
                         */
                        if (blks_assigned)
                        {
                            while ((++sgl_ptr)->count != 0);
                        }

                        if ((parent_read2_fruts_ptr) &&
                            (parent_read2_fruts_ptr->position == read_fruts_ptr->position))
                        {
                            fbe_block_count_t  more_blks = 0;
                            fbe_lba_t relative_fruts2_lba;

                            if (RAID_COND(verify_start < siots_p->start_lba))
                            {
                                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                          "striper: verify_start %llx < siots_p->start_lba %llx",
                                           (unsigned long long)verify_start,
					   (unsigned long long)siots_p->start_lba);
                                return FBE_STATUS_GENERIC_FAILURE;
                            }

                            /* Since the fruts lba is a physical lba, and the
                             * verify_start is a parity relative logical address,
                             * map the fruts lba to a parity logical address.
                            */
                            relative_fruts2_lba = logical_parity_start;
                            status = fbe_raid_map_pba_to_lba_relative(parent_read2_fruts_ptr->lba,
                                                       parent_read2_fruts_ptr->position,
                                                       siots_p->parity_pos,
                                                       siots_p->start_lba,
                                                       width,
                                                       &relative_fruts2_lba,
                                                       fbe_raid_siots_get_parity_stripe_offset(siots_p),
                                                       fbe_raid_siots_parity_num_positions(siots_p));
                            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                            {
                                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                                     "striper: failed to map pba to lba: status = 0x%x, siots_p = 0x%p\n",
                                                     status, siots_p);
                                return status;
                            }
                            /* Fill in the FRUTS' sg list by copying the sgs
                             * from the already allocated buffers in the
                             * parent's read2_fruts.
                             */
                            fbe_raid_fruts_get_sg_ptr(parent_read2_fruts_ptr, &overlay_sg_ptr);
                            status = fbe_raid_verify_setup_overlay_sgs(relative_fruts2_lba,
                                                           parent_read2_fruts_ptr->blocks,
                                                           overlay_sg_ptr,
                                                           sgl_ptr,
                                                           memory_info_p,
                                                           verify_start + blks_assigned,
                                                           extra_blocks,
                                                           &more_blks);
                            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                            {
                                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                                     "striper: failed to overlay sgs : status = 0x%x, siots_p = 0x%p\n",
                                                     status, siots_p);
                                return status;
                            }

                            if (RAID_COND(more_blks != extra_blocks))
                            {
                                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                                     "striper:more_blks 0x%llx != extra_blocks 0x%llx\n",
                                                     (unsigned long long)more_blks,
						     (unsigned long long)extra_blocks);
                                return FBE_STATUS_GENERIC_FAILURE;
                            }
                            parent_read2_fruts_ptr = fbe_raid_fruts_get_next(parent_read2_fruts_ptr);
                        }
                        else
                        {
                            fbe_u32_t extra_byte_count =0;
                            if(!fbe_raid_get_checked_byte_count(extra_blocks, &extra_byte_count))
                            {
                                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                     "%s: byte count exceeding 32bit limit \n",__FUNCTION__);
                                return FBE_STATUS_GENERIC_FAILURE; 
                            }
                            /* Assign new buffers.
                             */
                            sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                                                                          sgl_ptr, 
                                                                                                          extra_byte_count);
                            if(sg_total == 0)
                            {
                                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                                         "%s: failed to populate sg for: memory_info_p = 0x%p\n",
                                                         __FUNCTION__,memory_info_p);
                                    return FBE_STATUS_GENERIC_FAILURE;
                            }
                        }
                    }
                    else
                    {
                        if (RAID_COND(blks_assigned != verify_count))
                        {
                            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                                 "striper: blks_assigned 0x%llx != verify_count 0x%llx\n",
                                                 (unsigned long long)blks_assigned,
						 (unsigned long long)verify_count);
                            return FBE_STATUS_GENERIC_FAILURE;
                        }
                        if ((parent_read2_fruts_ptr) &&
                            (parent_read2_fruts_ptr->position == read_fruts_ptr->position))
                        {
                            parent_read2_fruts_ptr = fbe_raid_fruts_get_next(parent_read2_fruts_ptr);
                        }
                    }

                    parent_read_fruts_ptr = fbe_raid_fruts_get_next(parent_read_fruts_ptr);
                }
            }
            else
            {
                if (RAID_COND(parent_read2_fruts_ptr == NULL))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "striper: parent_read2_fruts_ptr == NULL, siots_p = 0x%p\n",
                                         siots_p);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                if (read_fruts_ptr->position != parent_read2_fruts_ptr->position)
                {
                    /* Assign new buffers.
                     */
                    sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                                sgl_ptr, 
                                                                dest_byte_count);
                    if(sg_total == 0)
                    {
                        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                                    "%s: failed to populate sg for: memory_info_p = 0x%p\n",
                                                    __FUNCTION__, memory_info_p);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                }
                else
                {
                    fbe_block_count_t  blks_assigned = 0;
                    fbe_lba_t relative_fruts2_lba;

                    if (RAID_COND(verify_start < siots_p->start_lba))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                             "striper: verify_start %llx < siots_p->start_lba %llx\n",
                                             (unsigned long long)verify_start,
					     (unsigned long long)siots_p->start_lba);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }

                    /* Since the fruts lba is a physical lba, and the
                     * verify_start is a parity relative logical address,
                     * map the fruts lba to a parity logical address.
                     */
                    relative_fruts2_lba = logical_parity_start;
                    status = fbe_raid_map_pba_to_lba_relative(parent_read2_fruts_ptr->lba,
                                               parent_read2_fruts_ptr->position,
                                               siots_p->parity_pos,
                                               siots_p->start_lba,
                                               width,
                                               &relative_fruts2_lba,
                                               fbe_raid_siots_get_parity_stripe_offset(siots_p),
                                               fbe_raid_siots_parity_num_positions(siots_p));
                    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                             "striper: failed to map pba to lba: status = 0x%x, siots_p = 0x%p\n",
                                             status, siots_p);
                        return status;
                    }

                    if (RAID_COND(read_fruts_ptr->position != parent_read2_fruts_ptr->position))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                             "striper: read_fruts_ptr->position %d != parent_read2_fruts_ptr->position %d\n",
                                             read_fruts_ptr->position,parent_read2_fruts_ptr->position);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }

                    /* Fill in the FRUTS' sg list by copying the sgs
                     * from the already allocated buffers in the
                     * parent's read2_fruts.
                     */
                    fbe_raid_fruts_get_sg_ptr(parent_read2_fruts_ptr, &overlay_sg_ptr);
                    status = fbe_raid_verify_setup_overlay_sgs(relative_fruts2_lba,
                                           parent_read2_fruts_ptr->blocks,
                                           overlay_sg_ptr,
                                           sgl_ptr,
                                           memory_info_p,
                                           verify_start,
                                           verify_count,
                                           &blks_assigned);
                    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                             "striper: failed to overlay sgs : status = 0x%x, siots_p = 0x%p\n",
                                             status, siots_p);
                        return status;
                    }
                    if (RAID_COND(blks_assigned != verify_count))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                             "striper: blks_assigned 0x%llx != verify_count 0x%llx\n",
                                             (unsigned long long)blks_assigned,
					     (unsigned long long)verify_count);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                    parent_read2_fruts_ptr = fbe_raid_fruts_get_next(parent_read2_fruts_ptr);
                }
            }

            read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
        
        } /* end if fruts blocks > 0 */
    
    } /* end while there are more read fruts */

    /* If we still have a parent read then it means we forgot to setup some of our sgs.
     */
    if ((parent_read_fruts_ptr != NULL) || (parent_read2_fruts_ptr != NULL))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: unexpected read fruts not null 0x%p 0x%p\n",
                             parent_read_fruts_ptr, parent_read2_fruts_ptr);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Debug code to detect scatter gather overrun.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    status = fbe_raid_fruts_for_each(FBE_RAID_FRU_POSITION_BITMASK, read_fruts_ptr, fbe_raid_fruts_validate_sgs, 0, 0);
    
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to validate sgs: read_fruts_ptr 0x%p "
                             "siots_p 0x%p. status = 0x%x\n",
                             read_fruts_ptr, siots_p, status);
        return status;
    }
    return FBE_STATUS_OK;

} /* end of fbe_striper_rvr_setup_sgs() */

/*!**************************************************************
 * fbe_striper_verify_get_verify_count()
 ****************************************************************
 * @brief
 *  Get the number of blocks we will verify in each pass of this
 *  state machine.
 *
 * @param siots_p - this i/o.
 * @param verify_count_p - number of blocks to verify.
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_striper_verify_get_verify_count(fbe_raid_siots_t *siots_p,
                                                 fbe_block_count_t *verify_count_p)
{
    fbe_block_count_t verify_count;

    /* Decide the verify_size for this sub-verify
     * For normal verifies it's the entire transfer size.
     */
    if (((siots_p->algorithm == RAID0_VR) ||
         (siots_p->algorithm == RAID0_BVA_VR) ||
         (siots_p->algorithm == RAID0_RD_VR)) &&
        !fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))
    {
        /* In normal verify, generating function cut the verify into strips.
         * Similarly for degraded verify we assume the size of the
         * region to be verified is small enough.  We do not break it up
         * further for degraded verify since this is normal I/O path
         * for degraded write.
         */
        verify_count = siots_p->xfer_count;
    }
    else
    {
        /* In recovery verify, we have to strip-mine in the state machines.
         */
        fbe_u32_t region_size = fbe_raid_siots_get_region_size(siots_p);
        if (RAID_COND(siots_p->algorithm != RAID0_RD_VR))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "striper: algorithm unexpected %d, siots_p = 0x%p\n", siots_p->algorithm, siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Set the flag that we are already in strip-mine mode.
         */
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE);

        if (siots_p->parity_start % region_size)
        {
            /* If we are not aligned to the region size, then
             * align the end to the region size so the next request is fully aligned. 
             */
            verify_count = (fbe_u32_t)(region_size - (siots_p->parity_start % region_size));
            verify_count = FBE_MIN(verify_count, siots_p->xfer_count);
        }
        else
        {
            /* Start is aligned to optimal size, just do the next piece.
             */ 
            verify_count = FBE_MIN(region_size, siots_p->xfer_count);
        }
        if (RAID_COND(verify_count == 0))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "striper: verify count is zero, siots_p = 0x%p\n", 
                                 siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        siots_p->parity_count = FBE_MIN(verify_count,
                                        (fbe_u32_t)((siots_p->start_lba + siots_p->xfer_count) -
                                                    siots_p->parity_start));
        verify_count = siots_p->parity_count; 
    }
    *verify_count_p = verify_count;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_striper_verify_get_verify_count()
 ******************************************/
/*!**************************************************************
 * fbe_striper_verify_get_write_bitmap()
 ****************************************************************
 * @brief
 *   Determine what our write bitmap should be for this verify.
 *   into account here.  The write bitmap is the positions which
 *   need to be written to correct errors and keep the stripe consistent.
 *  
 * @param siots_p - Siots to calculate the write bitmap for.
 * 
 * @return fbe_u16_t - Bitmask of positions we need to write.
 *
 ****************************************************************/
fbe_u16_t fbe_striper_verify_get_write_bitmap(fbe_raid_siots_t * const siots_p)
{
    fbe_u16_t write_bitmap;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_position_bitmask_t soft_media_err_bitmap;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* Just write what we were asked to.
     */
    write_bitmap = siots_p->misc_ts.cxts.eboard.w_bitmap;

    if (siots_p->algorithm == RAID0_VR)
    {
        /* If we are a verify, then it is OK to do remaps of soft media errors.
         * Also write out the positions that had soft media errors so these positions get 
         * remapped. 
         */
        fbe_raid_fruts_get_bitmap_for_status(read_fruts_p, &soft_media_err_bitmap, NULL, /* No counts */
                                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED);
        write_bitmap |= soft_media_err_bitmap;
    }

    return write_bitmap;
}
/*!*************************************
 * end fbe_striper_verify_get_write_bitmap()
 **************************************/
/*************************
 * end file fbe_striper_verify_util.c
 *************************/

