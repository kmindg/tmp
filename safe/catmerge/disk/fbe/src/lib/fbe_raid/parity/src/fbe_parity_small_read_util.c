/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_parity_small_read_util.c
 ***************************************************************************
 *
 * @brief
 *   This file contains the utility functions of the RAID 5 small read algorithm.
 * 
 * @author
 *  10/30/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"


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

/*!**************************************************************************
 * fbe_parity_small_read_calculate_memory_size()
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
 *  10/28/2009 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_parity_small_read_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                         fbe_u32_t *num_pages_to_allocate_p)
{
    fbe_status_t status;

    /* Small read does not need to allocate any memory.  All memory arrived in the sg list 
     * from the client. 
     */
    siots_p->total_blocks_to_allocate = 0;
    fbe_raid_siots_set_optimal_page_size(siots_p,   
                                         1, /* Num fruts */ 
                                         siots_p->total_blocks_to_allocate);

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, 1,/* num_fruts */ NULL, /* no sgs */
                                                FBE_TRUE /* In-case recovery verify is required*/);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: memory calc num pages failed with status: 0x%x\n",
                             status);
        return(status); 
    }
    *num_pages_to_allocate_p = siots_p->num_ctrl_pages;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_small_read_calculate_memory_size()
 **************************************/
/****************************************************************
 * fbe_parity_small_read_setup_fruts()
 ****************************************************************
 * @brief
 *  This function sets up the fruts.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 *
 * @author
 *  10/28/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_small_read_setup_fruts(fbe_raid_siots_t * siots_p,
                                               fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t logical_parity_start;
    fbe_u32_t data_pos = siots_p->start_pos;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_sg_element_t *sg_p = NULL;

    /* Get the fruts we just allocated.
     */

    logical_parity_start = siots_p->geo.logical_parity_start;

    /* Initialize the FRU_TS.
     */
    status = FBE_RAID_DATA_POSITION(siots_p->start_pos,
                                siots_p->parity_pos,
                                fbe_raid_siots_get_width(siots_p),
                                fbe_raid_siots_parity_num_positions(siots_p),
                                &data_pos);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "parity: %s errored: status = 0x%x, siots_p = 0x%p\n",
                                    __FUNCTION__, status, siots_p);
        return  status;;
    }

    read_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p,
                                                   &siots_p->read_fruts_head,
                                                   memory_info_p);

    /* We always expect this to succeed and give us a read ptr.
     */
    if (read_fruts_ptr == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Setup our read fruts.
     */
    status = fbe_raid_fruts_init_fruts(read_fruts_ptr,
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                       logical_parity_start + siots_p->geo.start_offset_rel_parity_stripe,
                                       siots_p->xfer_count,
                                       siots_p->geo.position[data_pos]);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
          fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                              "parity : failed to init fruts for siots_p = 0x%p, status = 0x%x\n",
                              siots_p, status);
          return  status;
    }

    /* Single disk reads will reuse the sgs provided.
     */
    fbe_raid_siots_get_sg_ptr(siots_p, &sg_p);

    /* Cache Op.
     * Use the buffer coming from the ca driver.
     */
    read_fruts_ptr->sg_p = sg_p;

    return FBE_STATUS_OK;
}
/*****************************************
 * end of fbe_parity_small_read_setup_fruts()
 *****************************************/
/*!**************************************************************
 * fbe_parity_small_read_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param siots_p - current I/O.               
 *
 * @return fbe_status_t
 *
 * @author
 *  10/5/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_small_read_setup_resources(fbe_raid_siots_t * siots_p)
{
    fbe_raid_memory_info_t  memory_info;
    fbe_status_t            status;

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to initialize memory request info with status: 0x%x\n",
                             status);
        return(status); 
    }

    /* Set up the FRU_TS.
     */
    status = fbe_parity_small_read_setup_fruts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to setup small read fruts with status: 0x%x\n",
                             status);
        return(status); 
    }

    /* Plant the nested siots in case it is needed for recovery verify.
     * We don't initialize it until it is required.
     */
    status = fbe_raid_siots_consume_nested_siots(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to consume nested siots: siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return (status);
    }
    
    /* Initialize the RAID_VC_TS now that it is allocated.
     */
    status = fbe_raid_siots_init_vcts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to init vcts: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);

        return  status;;
    }

    /* Initialize the VR_TS.
     * We are allocating VR_TS structures WITH the fruts structures.
     */
    status = fbe_raid_siots_init_vrts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to init vrts: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return  status;;
    }

    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_parity_small_read_setup_resources()
 ******************************************/
/**************************************
 * end fbe_parity_small_read_util.c
 **************************************/
