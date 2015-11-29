/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_fruts.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions related to the raid fruts structure.
 *
 * @version
 *   5/19/2009  Ron Proulx - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_transport_memory.h"
#include "fbe_traffic_trace.h"
#include "fbe/fbe_extent_pool.h"


/*************************
 * LOCAL FUNCTION PROTOTYPES
 *************************/

static fbe_status_t fbe_raid_fruts_usurper_send_control_packet_to_pdo(fbe_packet_t *packet_p,
                                                                      fbe_payload_control_operation_opcode_t control_code,
                                                                      fbe_payload_control_buffer_t buffer,
                                                                      fbe_payload_control_buffer_length_t buffer_length,
                                                                      fbe_object_id_t object_id,
                                                                      fbe_packet_attr_t attr,
                                                                      fbe_raid_fruts_t *fruts_p,
                                                                      fbe_package_id_t package_id);
static fbe_status_t fbe_raid_fruts_usurper_send_control_packet_to_pdo_completion(fbe_packet_t * packet_p, 
                                                                          fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_fruts_send_timeout_usurper(fbe_raid_fruts_t * fruts_p);
static fbe_status_t fbe_raid_fruts_send_crc_usurper(fbe_raid_fruts_t * fruts_p,
                                                    fbe_block_transport_error_type_t error_type);
static fbe_status_t fbe_raid_send_usurper(fbe_raid_fruts_t *fruts_p,
                                          fbe_payload_control_operation_opcode_t control_code,
                                          fbe_payload_control_buffer_t buffer,
                                          fbe_payload_control_buffer_length_t buffer_length,
                                          fbe_packet_attr_t attr);
static fbe_status_t fbe_raid_fruts_usurper_packet_completion(fbe_packet_t * packet_p, 
                                                             fbe_packet_completion_context_t context);

static fbe_packet_t * fbe_raid_fruts_to_master_packet(fbe_raid_fruts_t *fruts_p);
static fbe_bool_t fbe_raid_fruts_block_edge_has_timeout_errors( fbe_raid_fruts_t *fruts_p);

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
 
 /*!**************************************************************
 * fbe_raid_fruts_common_init()
 ****************************************************************
 * @brief
 *  Initialize the common section of a fruts.  This differs from
 *  `fbe_raid_common_init' by the fact that RAID doesn't treat the
 *   queue_element in the same way (i.e. NULL terminated list).
 *
 * @param  common_p - The common raid object to initialize               
 *
 * @return None.
 *
 * @author
 *  05/20/2009  Ron Proulx  - Created.
 *
 ****************************************************************/
void fbe_raid_fruts_common_init(fbe_raid_common_t *common_p)
{
    common_p->flags = 0;
    common_p->queue_element.next = NULL;
    common_p->queue_element.prev = NULL;
    fbe_queue_element_init(&common_p->wait_queue_element);
    common_p->parent_p = NULL;
    common_p->queue_p = NULL;
    common_p->state = NULL;
    return;
}
/******************************************
 * end fbe_raid_fruts_common_init()
 ******************************************/

/*!**************************************************************************
 *          fbe_raid_fruts_invalid_state()
 ***************************************************************************
 *
 * @brief
 *    A fruts has it's state set to this function when it is freed.
 *    If we ever get here we panic since the fruts is bad.
 *
 * @param   common_p - [I], the bogus fruts, not used.
 *
 * @return  FBE_RAID_STATE_STATUS_DONE  
 *
 * @note
 *
 *  HISTORY:
 *    7/19/99: RPF -- Created.
 *
 * @author
 *  05/20/2009  Ron Proulx  - Updated from rg_fruts_freed
 *
 **************************************************************************/

fbe_raid_state_status_t fbe_raid_fruts_invalid_state(fbe_raid_common_t *common_p)
{
    fbe_raid_fruts_t *fruts_p = NULL;
    fbe_raid_siots_t *siots_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_object_id_t object_id;

    fruts_p = (fbe_raid_fruts_t*)common_p;
    siots_p = fbe_raid_fruts_get_siots(fruts_p);

    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                  FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                  "fruts unexpected state: 0x%p\n", fruts_p);

    return (FBE_RAID_STATE_STATUS_DONE);
}
/*****************************************
 * end of fbe_raid_fruts_invalid_state()
 *****************************************/

/*!**************************************************************************
 *          fbe_raid_fruts_init_request()
 ***************************************************************************
 * @brief   This function initializes the block I/O fields of a fruts.
 *
 * @param   fruts_p:  [IO], The fruts to initialize.
 * @param   opcode:   [I], The opcode to set in the fruts.
 * @param   lba:      [I], The lba to set in the fruts.
 * @param   blocks:   [I], The blocks to set in the fruts.
 * @param   position: [I], The relative array position of this fru request.
 *                         valid values are 0..MAX_DISK_ARRAY_WIDTH
 *
 * @return  fbe_status_t
 *
 * @note    None
 *
 * @author
 *  05/20/2009  Ron Proulx  - Updated rg_init_fruts
 *
 **************************************************************************/

fbe_status_t fbe_raid_fruts_init_request(fbe_raid_fruts_t *const fruts_p,
                                 fbe_payload_block_operation_opcode_t opcode,
                                 fbe_lba_t lba,
                                 fbe_block_count_t blocks,
                                 fbe_u32_t position )
{
    fbe_status_t status = FBE_STATUS_OK;
    if (RAID_COND( (fruts_p == NULL) ||
                (!(fbe_raid_common_get_flags(&fruts_p->common) & FBE_RAID_COMMON_FLAG_TYPE_FRU_TS) )) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "%s failed: fruts_p is NULL or fruts_p->common->flag 0x%x \n",
                               __FUNCTION__,
                               (unsigned int)&fruts_p->common.flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }
        
    fruts_p->position = position;
    fruts_p->lba = lba;
    fruts_p->blocks = blocks;
    fruts_p->opcode = opcode;
    fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fruts_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fruts_p->media_error_lba = FBE_RAID_INVALID_LBA;
    fruts_p->pvd_object_id = FBE_OBJECT_ID_INVALID;
    /* We preserve the flag regarding packet initialization.
     */
    if (fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED);
    }
    else
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_INVALID);
    }
    /* Since this routine can be invoked to re-initialize a fruts, we need to
     * reset the common flags.
     */
    fbe_raid_common_init_flags(&fruts_p->common, FBE_RAID_COMMON_FLAG_TYPE_FRU_TS);
    return status;
}
/***********************
 * end fbe_raid_fruts_init_block
 ***********************/

/*!**************************************************************************
 *          fbe_raid_fruts_init_read_request()
 ***************************************************************************
 * @brief   This function initializes the block I/O fields of a fruts.
 *          This function does not reset pdo object id inside fruts.
 *
 * @param   fruts_p:  [IO], The fruts to initialize.
 * @param   opcode:   [I], The opcode to set in the fruts.
 * @param   lba:      [I], The lba to set in the fruts.
 * @param   blocks:   [I], The blocks to set in the fruts.
 * @param   position: [I], The relative array position of this fru request.
 *                         valid values are 0..MAX_DISK_ARRAY_WIDTH
 *
 * @return  fbe_status_t
 *
 * @note    None
 *
 * @author
 *  02/01/2012  Swati Fursule  - Created from fbe_raid_fruts_init_request
 *
 **************************************************************************/

fbe_status_t fbe_raid_fruts_init_read_request(fbe_raid_fruts_t *const fruts_p,
                                 fbe_payload_block_operation_opcode_t opcode,
                                 fbe_lba_t lba,
                                 fbe_block_count_t blocks,
                                 fbe_u32_t position )
{
    fbe_status_t status = FBE_STATUS_OK;
    if (RAID_COND( (fruts_p == NULL) ||
                (!(fbe_raid_common_get_flags(&fruts_p->common) & FBE_RAID_COMMON_FLAG_TYPE_FRU_TS) )) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "%s failed: fruts_p is NULL or fruts_p->common->flag 0x%x \n",
                               __FUNCTION__,
                               (unsigned int)&fruts_p->common.flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }
        
    fruts_p->position = position;
    fruts_p->lba = lba;
    fruts_p->blocks = blocks;
    fruts_p->opcode = opcode;
    fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fruts_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fruts_p->media_error_lba = FBE_RAID_INVALID_LBA;
    /*Do not set pdo object id as invalid, in case earlier io is successful 
     * since we may want to send CRC usurper later*/
    if(fbe_transport_get_status_code(&fruts_p->io_packet) != FBE_STATUS_OK)
    {
       /* set pdo object id as invalid */
        fruts_p->pvd_object_id = FBE_OBJECT_ID_INVALID;
    }
    /* We preserve the flag regarding packet initialization.
     */
    if (fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED);
    }
    else
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_INVALID);
    }
    /* Since this routine can be invoked to re-initialize a fruts, we need to
     * reset the common flags.
     */
    fbe_raid_common_init_flags(&fruts_p->common, FBE_RAID_COMMON_FLAG_TYPE_FRU_TS);
    return status;
}
/***********************
 * end fbe_raid_fruts_init_read_request
 ***********************/
/*!**************************************************************************
 *          fbe_raid_fruts_init_fruts()
 ***************************************************************************
 * 
 * @brief   This function initializes the fields of a fruts for the first time.  
 *          Any requests are marked in the SIOTS bitmask.
 *
 * @param   fruts_p:   [IO], The fruts to initialize.
 * @param   opcode:    [I], The opcode to set in the fruts.
 * @param   lba:       [I], The lba to set in the fruts.
 * @param   blocks:          [I], The blocks to set in the fruts.
 * @param   position:        [I], The relative array position of this fru request.
 *                                valid values are 0..MAX_DISK_ARRAY_WIDTH
 * @return  fbe_status_t
 *  
 *
 * @note
 *  Sets unaligned positions in SIOTS as needed
 *
 * @author
 *  01/01/01: RPF: - Created
 *  09/08/08: RDP: - Updated to support unaligned requests
 *  11/17/08: RDP: - Fix issue with position swap after FRUTS initialization
 **************************************************************************/
fbe_status_t fbe_raid_fruts_init_fruts(fbe_raid_fruts_t *const fruts_p,
                                        fbe_payload_block_operation_opcode_t opcode,
                                        fbe_lba_t lba,
                                        fbe_block_count_t blocks,
                                        fbe_u32_t position )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_t  *siots_p;

    /* First, init our common fields.
     */
    status = fbe_raid_fruts_init_request( fruts_p, opcode, lba, blocks, position );
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                               "raid: %s failed to init fruts_p 0x%p\n", 
                               __FUNCTION__, fruts_p);
        return (status);
    }

    /* Next clear out the sg id, since other parts of the code
     * assumes this is NULL to begin with.
     */
    fruts_p->sg_p = NULL;
    fruts_p->sg_id = NULL;
    fruts_p->sg_element_offset = 0;

    /* Init our retry count for the first time.
     */
    fruts_p->retry_count = 0;

    /* If we need to check for `non-optimal' RGs then use BSV function to
     * calculate the pre-read descriptor.
     */
    siots_p = fbe_raid_fruts_get_siots(fruts_p);

    /* We need to clear out the pre-read descriptor since otherwise the shim 
     * will try to send this downstream. 
     */
    fbe_payload_pre_read_descriptor_set_lba(&fruts_p->write_preread_desc, 0);
    fbe_payload_pre_read_descriptor_set_block_count(&fruts_p->write_preread_desc, 0);
    fbe_payload_pre_read_descriptor_set_sg_list(&fruts_p->write_preread_desc, NULL);
    
    return FBE_STATUS_OK;
}
/***********************
 * end fbe_raid_fruts_init_fruts()
 ***********************/
/*!**************************************************************
 * fbe_raid_fruts_get_block_status()
 ****************************************************************
 * @brief
 *  Fetch the status from the block operation.
 *
 * @param fruts_p - fruts with packet.
 * @param block_status_p - block status to return.
 *
 * @return fbe_status_t   
 *
 * @author
 *  6/16/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_get_block_status(fbe_raid_fruts_t *fruts_p,
                                             fbe_payload_block_operation_status_t *block_status_p)
{
    fbe_payload_block_operation_t *block_payload_p = NULL;
    fbe_payload_ex_t *payload_p  = NULL;
    fbe_packet_t *packet_p = NULL;
    packet_p = fbe_raid_fruts_get_packet(fruts_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);

    if (payload_p == NULL)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "get_bl_status payload is null for fruts: %p pos: %d lba: %llx bl: 0x%llx\n",
                               fruts_p, fruts_p->position,
			       (unsigned long long)fruts_p->lba,
			       (unsigned long long)fruts_p->blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    block_payload_p = fbe_payload_ex_get_block_operation(payload_p);

    if (block_payload_p == NULL)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "get_bl_status block payload is null for fruts: %p pos: %d lba: %llx bl: 0x%llx\n",
                               fruts_p, fruts_p->position,
			       (unsigned long long)fruts_p->lba,
			       (unsigned long long)fruts_p->blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_block_get_status(block_payload_p, block_status_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_fruts_get_block_status()
 ******************************************/
/*!**************************************************************
 * fbe_raid_fruts_get_status()
 ****************************************************************
 * @brief
 *  Fetch all the status info from the fruts packet.
 *
 * @param fruts_p
 * @param transport_status_p
 * @param transport_qualifier_p
 * @param block_status_p
 * @param block_qualifier_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  6/16/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_fruts_get_status(fbe_raid_fruts_t *fruts_p,
                                       fbe_status_t *transport_status_p,
                                       fbe_u32_t *transport_qualifier_p,
                                       fbe_payload_block_operation_status_t *block_status_p,
                                       fbe_payload_block_operation_qualifier_t *block_qualifier_p)
{
    fbe_payload_block_operation_t *block_payload_p = NULL;
    fbe_payload_ex_t *payload_p  = NULL;
    fbe_packet_t *packet_p = NULL;
    packet_p = fbe_raid_fruts_get_packet(fruts_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);

    if (payload_p == NULL)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "get_status payload is null for fruts: %p pos: %d lba: %llx bl: 0x%llx\n",
                               fruts_p, fruts_p->position,
			       (unsigned long long)fruts_p->lba,
			       (unsigned long long)fruts_p->blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    block_payload_p = fbe_payload_ex_get_block_operation(payload_p);

    if (block_payload_p == NULL)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "get_status block payload is null for fruts: %p pos: %d lba: %llx bl: 0x%llx\n",
                               fruts_p, fruts_p->position,
			       (unsigned long long)fruts_p->lba,
			       (unsigned long long)fruts_p->blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_block_get_status(block_payload_p, block_status_p);
    fbe_payload_block_get_qualifier(block_payload_p, block_qualifier_p);
    *transport_status_p = fbe_transport_get_status_code(packet_p);
    *transport_qualifier_p = fbe_transport_get_status_qualifier(packet_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_fruts_get_status()
 ******************************************/
/*!**************************************************************
 * fbe_raid_fruts_get_media_error_lba()
 ****************************************************************
 * @brief
 *  Fetch the lba the media error occurred at.
 *
 * @param fruts_p - fruts with packet.
 * @param media_error_lba_p - ptr to lba to return        
 *
 * @return fbe_status_t   
 *
 * @author
 *  6/16/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_get_media_error_lba(fbe_raid_fruts_t *fruts_p,
                                                fbe_lba_t *media_error_lba_p)
{
    fbe_payload_block_operation_t *block_payload_p = NULL;
    fbe_payload_ex_t *payload_p  = NULL;
    fbe_packet_t *packet_p = NULL;

    packet_p = fbe_raid_fruts_get_packet(fruts_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);

    if (payload_p == NULL)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "get_media_error_lba block payload is null for fruts: %p pos: %d lba: %llx bl: 0x%llx\n",
                               fruts_p, fruts_p->position,
			       (unsigned long long)fruts_p->lba,
			       (unsigned long long)fruts_p->blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    block_payload_p = fbe_payload_ex_get_block_operation(payload_p);

    if (block_payload_p == NULL)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "get_media_error_lba block payload is null for fruts: %p pos: %d lba: %llx bl: 0x%llx\n",
                               fruts_p, fruts_p->position,
			       (unsigned long long)fruts_p->lba,
			       (unsigned long long)fruts_p->blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_ex_get_media_error_lba(payload_p, media_error_lba_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_fruts_get_media_error_lba()
 ******************************************/
/*!**************************************************************
 * fbe_raid_fruts_to_block_operation()
 ****************************************************************
 * @brief
 *  Get the block operation from the fruts packet.
 *
 * @param fruts_p - fruts with packet.
 *
 * @return fbe_status_t 
 *
 * @author
 *  6/16/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_payload_block_operation_t *fbe_raid_fruts_to_block_operation(fbe_raid_fruts_t *fruts_p)
{
    fbe_packet_t   *packet_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;

    /* Get the packet associated with this fruts.
     */
    packet_p = fbe_raid_fruts_get_packet(fruts_p);
    if (packet_p == NULL)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "packet is null for fruts: %p pos: %d lba: %llx bl: 0x%llx\n",
                               fruts_p, fruts_p->position,
			       (unsigned long long)fruts_p->lba,
			       (unsigned long long)fruts_p->blocks);
        return NULL;
    }
    /* Now get the payload and the block operation.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    if (payload_p == NULL)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "sep payload is null for fruts: %p pos: %d lba: %llx bl: 0x%llx\n",
                               fruts_p, fruts_p->position,
			       (unsigned long long)fruts_p->lba,
			       (unsigned long long)fruts_p->blocks);
        return NULL;
    }
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* Return the block operation pointer.
     */
    return(block_operation_p);
}
/******************************************
 * end fbe_raid_fruts_to_block_operation()
 ******************************************/
/*!**************************************************************
 * fbe_raid_fruts_get_retry_wait_msecs()
 ****************************************************************
 * @brief
 *  Get the time we should wait from the packet.
 *
 * @param fruts_p - fruts with packet.
 * @param retry_wait_msecs_p - ptr to msecs to return.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  6/16/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_get_retry_wait_msecs(fbe_raid_fruts_t *fruts_p,
                                                 fbe_lba_t *retry_wait_msecs_p)
{
    fbe_payload_block_operation_t *block_payload_p = NULL;
	fbe_packet_t * packet = NULL;
	fbe_payload_ex_t *payload = NULL;

	packet = fbe_raid_fruts_get_packet(fruts_p);
	payload = fbe_transport_get_payload_ex(packet);

    block_payload_p = fbe_raid_fruts_to_block_operation(fruts_p);
    if (block_payload_p == NULL)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "block payload is null for fruts: 0x%p pos: %d lba: %llx bl: 0x%llx\n",
                               fruts_p, fruts_p->position,
			       (unsigned long long)fruts_p->lba, 
			       (unsigned long long)fruts_p->blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_ex_get_retry_wait_msecs(payload, retry_wait_msecs_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_fruts_get_retry_wait_msecs()
 ******************************************/
/*!***************************************************************
 * fbe_raid_setup_fruts_from_fru_info()
 ****************************************************************
 * @brief
 *  This function will set up fruts using the supplied fru info
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 * @param info_p  - Pointer to read/write per position information 
 * @param queue_head_p - Pointer to head of fruts queue to process 
 * @param opcode     - type of op to put in fruts.
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_raid_setup_fruts_from_fru_info(fbe_raid_siots_t * siots_p, 
                                                fbe_raid_fru_info_t *info_p,
                                                fbe_queue_head_t *queue_head_p,
                                                fbe_u32_t opcode,
                                                fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_fruts_t *fruts_p = NULL;

    fruts_p = fbe_raid_siots_get_next_fruts(siots_p,
                                            queue_head_p,
                                            memory_info_p);
    if (RAID_MEMORY_COND(fruts_p == NULL))
    {
        /* We are here as we failed to peel one fruts off the allocated list. 
         * If we intentionally fail this condition then we must take care 
         * of releasing associated memory.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "raid:fail to peel one fruts off the alloc list:fruts=NULL,siots=0x%p\n", 
                               siots_p);
        return  (FBE_STATUS_GENERIC_FAILURE);
    }
    status = fbe_raid_fruts_init_fruts(fruts_p,
                                       opcode,
                                       info_p->lba,
                                       info_p->blocks,
                                       info_p->position);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "raid: failed to init fruts_p 0x%p, siots_p=0x%p\n", 
                               fruts_p, siots_p);
        return  (status);
    }

    if (info_p->blocks == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "%s failed : verify read blocks is 0x%llx lba: %llx pos: %d\n",
                             __FUNCTION__,
                             (unsigned long long)info_p->blocks,
                             (unsigned long long)info_p->lba,
                             info_p->position);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/* end of fbe_raid_setup_fruts_from_fru_info() */

/*!***************************************************************
 * fbe_raid_fruts_chain_set_prepend_header()
 ****************************************************************
 * @brief
 *  This function sets each frut sg list offset to use (or skip) the write log header
 *
 * @param queue_head_p - Pointer to head of fruts queue to process 
 * @param prepend      - TRUE to use header, FALSE to skip header 
 *
 * @return nothing
 *
 ****************************************************************/
void fbe_raid_fruts_chain_set_prepend_header(fbe_queue_head_t *queue_head_p, fbe_bool_t prepend)
{
    fbe_raid_fruts_t *cur_fruts_p = NULL;
    fbe_raid_fruts_t *next_fruts_p = NULL;

    cur_fruts_p = (fbe_raid_fruts_t *)fbe_queue_front(queue_head_p);
    while (cur_fruts_p != NULL)
    {
        next_fruts_p = fbe_raid_fruts_get_next(cur_fruts_p);
        if (TRUE == prepend)
        {
            cur_fruts_p->sg_element_offset = 0;
        }
        else
        {
            cur_fruts_p->sg_element_offset = 1;
        }
        cur_fruts_p = next_fruts_p;
    }
}

/*!***********************************************************
 * fbe_raid_fruts_propagate_expiration_time()
 ************************************************************
 * @brief
 *  propagate the expiration time from the parent packet to the
 *  fruts packet
 * 
 * @param fruts_p - Pointer to fruts to obtain parent packet
 * @param new_packet_p - Pointer to new packet to propagate timer to
 * 
 * @return FBE_STATUS_OK
 * 
 ************************************************************/
fbe_status_t fbe_raid_fruts_propagate_expiration_time(fbe_raid_fruts_t *const fruts_p,
                                                      fbe_packet_t *new_packet_p)
{
    fbe_raid_siots_t   *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_raid_iots_t    *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_packet_t       *parent_packet_p = fbe_raid_iots_get_packet(iots_p); 

    return(fbe_transport_propagate_expiration_time(new_packet_p, parent_packet_p));
}
/************************************************
 * end fbe_raid_fruts_propagate_expiration_time()
 ************************************************/

/*!***************************************************************************
 *          fbe_raid_fruts_flag_forced_write_request()
 *****************************************************************************
 *
 * @brief   This method determines if this is a `forced write' request or not.
 *          We may need to set this flag for the following reasons:
 *              o The original packet had the flag set
 *              o This is a write for a position being rebuilt
 *              o This is a journal write request
 *          If it is, it sets the `write for rebuild' flag in the block operation 
 *          payload of the fruts packet.
 *
 * @param   fruts_p - Pointer to per-position request to populate
 * @param   fruts_block_operation_p - Pointer to fruts block operation
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  12/19/2011  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_fruts_flag_forced_write_request(fbe_raid_fruts_t *fruts_p,
                                                             fbe_payload_block_operation_t *fruts_block_operation_p)
{
    fbe_raid_siots_t               *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_raid_iots_t                *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_geometry_t            *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_payload_block_operation_t  *block_operation_p = fbe_raid_iots_get_block_operation(iots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_position_bitmask_t     rebuild_bitmask;
    fbe_lba_t                       journal_start_lba;

    /* If this isn't a write request just return.
     */
    if (fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)
    {
        return FBE_STATUS_OK;
    }

    /* If the `forced write' is already set, set it in the fruts packet.
     */
    if (fbe_payload_block_is_flag_set(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE))
    {
        /* Set the `forced write' flag and return.
         */
        fbe_payload_block_set_flag(fruts_block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE);
        return FBE_STATUS_OK;
    }

    /* Get the opcode of the orignal request.
     */
    fbe_payload_block_get_opcode(block_operation_p, &opcode);
             
    /* If this is a write to the position being rebuilt set the `forced write'
     * flag.
     */
    fbe_raid_siots_get_rebuild_bitmask(siots_p, &rebuild_bitmask);
    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD) &&
        ((1 << fruts_p->position) & rebuild_bitmask)              )
    {
        /* Set the `forced write' flag and return.
         */
        fbe_payload_block_set_flag(fruts_block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE);
        return FBE_STATUS_OK;
    }

    /* If this is a journal write set the `forced write' flag.
     */
    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_start_lba);
    if ((journal_start_lba != FBE_LBA_INVALID) &&
        (fruts_p->lba >= journal_start_lba)       )
    {
        /* Set the `forced write' flag and return.
         */
        fbe_payload_block_set_flag(fruts_block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE);
        return FBE_STATUS_OK;
    }

    /* This method always succeeds.
     */
    return FBE_STATUS_OK;
}
/***************************************************
 * end of fbe_raid_fruts_flag_forced_write_request()
 ***************************************************/

/*!***************************************************************************
 *          fbe_raid_fruts_populate_packet()
 *****************************************************************************
 *
 * @brief   This method populates the packet that is embedded in each fruts.
 *          It simply uses the fruts information, initializes and then populates
 *          the packet.
 *
 * @param   fruts_p - Pointer to per-position request to populate
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  05/21/2009  Ron Proulx  - Created from fbe_raid_test_build_packet
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_fruts_populate_packet(fbe_raid_fruts_t *fruts_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_packet_t   *packet_p = NULL;
    fbe_packet_t   *master_packet_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_sg_descriptor_t *sg_desc_p = NULL;
    fbe_sg_element_t *sg_p = NULL;
    fbe_raid_siots_t    *siots_p = fbe_raid_fruts_get_siots(fruts_p);    
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_block_size_t    optimal_block_size;
    fbe_lba_t lba = fruts_p->lba;
    fbe_packet_attr_t packet_attr;
    fbe_time_t pdo_service_time_ms;
    fbe_lba_t journal_log_start_lba;
    fbe_raid_iots_t *iots_p  = fbe_raid_siots_get_iots(siots_p);

    master_packet_p = fbe_raid_fruts_to_master_packet(fruts_p);
    fbe_transport_get_packet_attr(master_packet_p, &packet_attr);   

    /* Get the pdo service time */
    fbe_transport_get_physical_drive_service_time(fbe_raid_fruts_get_packet(fruts_p), &pdo_service_time_ms);

    /* Initialize the packet within the fruts.
     */
    status = fbe_raid_fruts_initialize_transport_packet(fruts_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                "raid: %s packet initialize status %d",
                __FUNCTION__, status);
        return status;
    }

    /* Get the packet associated with this fruts.
     */
    packet_p = fbe_raid_fruts_get_packet(fruts_p);

    /* Pass packet priority
     */
    fbe_transport_get_packet_priority(master_packet_p, &packet_p->packet_priority);

    /* Now populate the block payload.
     */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_raid_fruts_packet_completion,
                                          fruts_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    
	/* This is similar to add subpacket, but without cancelation logic and queueing */
	fbe_transport_set_master_packet(master_packet_p, packet_p);

    /* The edge increment the block operation level.
     */
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

    fbe_raid_fruts_get_sg_ptr(fruts_p, &sg_p);
    fbe_payload_ex_set_sg_list(payload_p, sg_p, 0);

   
    /* Build the block operation.  Use the block edge information to
     * get the block size information.
     */
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &optimal_block_size);

    /* This type of raid group uses an offset that is configured in 
     * the raid geometry.  This offset does not get applied on the edge. 
     */
    if (fbe_raid_geometry_no_object(raid_geometry_p))
    {
        fbe_lba_t offset;
        fbe_raid_geometry_get_raw_mirror_offset(raid_geometry_p, &offset);
        lba += offset;
    } else if (fbe_raid_geometry_is_attribute_set(raid_geometry_p, FBE_RAID_ATTRIBUTE_EXTENT_POOL)) {
        fbe_raid_iots_get_extent_offset(iots_p, fruts_p->position, &lba);
    }

    /* Build the packet payload.
     */
    fbe_payload_block_build_operation(block_operation_p,
                                      fruts_p->opcode,
                                      lba,
                                      fruts_p->blocks,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      optimal_block_size, 
                                      NULL /* Pre-read descriptor is no longer used */);
    
    /* If this is a write or write verify clear the check checksum.
     */
    if ((fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)        ||
        (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)    )
    {
        fbe_payload_block_clear_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);
    }

    /* If this is a write request check and set the `forced write' payload flag.
     */
    if (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)
    {
        fbe_raid_fruts_flag_forced_write_request(fruts_p, block_operation_p);
    }

    /* We need to make sure we do not do anything related to Zero On Demand 
     * for all accesses to the journal.  This speeds up our degraded performance when zeroing. 
     */
    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_log_start_lba); 
    if ((journal_log_start_lba != FBE_LBA_INVALID) &&
        (fruts_p->lba >= journal_log_start_lba))
    {
        fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_CONSUMED);
    }
  
    /* Default our repeat count to 1.
     */
    fbe_payload_block_get_sg_descriptor(block_operation_p, &sg_desc_p);
    sg_desc_p->repeat_count = 1;

    if(packet_attr & FBE_PACKET_FLAG_COMPLETION_BY_PORT)
    {
        if ((fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) &&
            (siots_p->algorithm == R5_SMALL_468 || 
             siots_p->algorithm == R5_MR3 ||
             siots_p->algorithm == R5_468 ||
             siots_p->algorithm == R5_RCW))
        {
            /* If we are doing a pre-read it needs data since we allocated the buffers. 
             * Otherwise we loop back a pre-zeroed buffer that rdgen sent and we don't get errors. 
             */
            fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_COMPLETION_BY_PORT_WITH_ZEROS);
        }
        else
        {
            fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_COMPLETION_BY_PORT);
        }
    }


    if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p))
    {
        if (packet_attr & FBE_PACKET_FLAG_DO_NOT_QUIESCE)
        {
            fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_QUIESCE);
        }
        /* Anything coming out of the raw mirror does not need to quiesce or get stripe locks at PVD. 
         * This is because we need to avoid deadlock since this might have originated as a monitor op 
         * at the PVD. 
         */
        fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_STRIPE_LOCK);
    }
    else if (fbe_raid_geometry_is_c4_mirror(raid_geometry_p))
    {
        /*
         * Anything coming out of the c4mirror does not need to get stripe locks at PVD.
         * This is because we have previously zeroed the range in ICA and the I/Os can only come from 
         * one SP.
         */
        fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_STRIPE_LOCK);
    }

    /* When we are doing retries we intentionally do not reset the expiration time. 
     * This way if the I/O takes too long from the first time we sent it, then the packet 
     * will expire and the PDO will be taken offline.
     */
    if (!fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_RETRY_PHYSICAL_ERROR))
    {
        /* Setup our time stamp only if we are not retrying.
         * The PDO will use a default expiration time in this case. 
         */
        fruts_p->time_stamp = fbe_get_time();
    }
    else
    {
        /* When we are retrying we will set the PDO service time.
         */
        fbe_transport_modify_physical_drive_service_time(packet_p, pdo_service_time_ms);
    }
    
    /* Set any applicable memory request fields from the parent
     */
    fbe_raid_common_set_memory_request_fields_from_parent(&fruts_p->common);

    /* For striped mirror (i.e. RAID-10) we need to popagate any supplied verify
     * structure pointer to the each fruts.
     */
    if (fbe_raid_geometry_is_raid10(raid_geometry_p) == FBE_TRUE)
    {
        fbe_raid_verify_error_counts_t *verify_error_p; 

        /* Copy flags from the iots to our new block_operation
         */
        fbe_payload_block_copy_flags(iots_p->block_operation_p, block_operation_p);
    
        verify_error_p = fbe_raid_siots_get_verify_eboard(siots_p);
        fbe_payload_ex_set_verify_error_count_ptr(payload_p, (void *)verify_error_p);
    }
  
    /* The retry flag means that the fruts needs to use the
     * force unit access to force the device to bypass it's cache.
     */
    if (fbe_raid_common_is_flag_set(&fruts_p->common, FBE_RAID_COMMON_FLAG_REQ_RETRIED))
    {
        fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCE_UNIT_ACCESS);
    }

    if ((fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) &&
        fbe_raid_geometry_is_vault(raid_geometry_p)){
        fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_DO_NOT_QUEUE);
    }
    if ((journal_log_start_lba != FBE_LBA_INVALID) &&
        (fruts_p->lba >= journal_log_start_lba)){
        /* If we are accessing the journal area, then use the new key if we are during encryption.
         */
        if (fbe_raid_geometry_journal_flag_is_set(raid_geometry_p, FBE_PARITY_WRITE_LOG_FLAGS_ENCRYPTED)){
            fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY);
        } 
        
    } else if ((iots_p->rekey_bitmask != 0) &&
               (iots_p->rekey_bitmask & (1 << fruts_p->position))) {

        /* Rekey reads do not use the key so they can read data the old way.
         */
        if (((siots_p->algorithm != R5_REKEY) && (siots_p->algorithm != MIRROR_REKEY)) || 
            (fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)){
            fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY);
        }
    }
    
    /* Always return the status
     */
    return(status);
}
/**************************************
 * end fbe_raid_fruts_populate_packet()
 **************************************/

/*!**************************************************************
 * fbe_raid_fruts_trace_dump_sector()
 ****************************************************************
 * @brief
 *  Log info about data for a sector
 *
 * @param fruts_p - We data for this fruts.  
 * @param siots_p - Pointer to parent siots
 * @param sector_p - Pointer to sector data to dump
 * @param lba - The LBA associated with this sector 
 * 
 * @return fbe_status_t
 * 
 ****************************************************************/
static fbe_status_t fbe_raid_fruts_trace_dump_sector(fbe_raid_fruts_t *fruts_p,
                                                     fbe_raid_siots_t *siots_p,
                                                     fbe_sector_t *sector_p,
                                                     fbe_lba_t lba)
{
    fbe_u32_t   byte_16_count = 0;
    fbe_u32_t   word_16_to_trace = 4;
    fbe_u64_t  *data_ptr = (fbe_u64_t *)sector_p;

    word_16_to_trace = (sizeof(fbe_sector_t) / 16);

    /* trace the fruts and lba */
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                "fru: %08x l:%016llx \n",
                                fbe_raid_memory_pointer_to_u32(fruts_p), lba); 

    for ( byte_16_count = 0; byte_16_count < word_16_to_trace; byte_16_count += 4)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "fru: %08x qw:%02d-%02d %016llx %016llx %016llx %016llx\n",
                                    fbe_raid_memory_pointer_to_u32(fruts_p),
                                    byte_16_count, (byte_16_count + 3),
                                    data_ptr[byte_16_count], data_ptr[byte_16_count + 1], 
                                    data_ptr[byte_16_count + 2], data_ptr[byte_16_count + 3]); 

    }

    /* Always trace the metadata.
     */
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                "fru: %08x crc: 0x%04x ts: 0x%04x ws: 0x%04x lba: 0x%04x\n",
                                fbe_raid_memory_pointer_to_u32(fruts_p),
                                sector_p->crc, sector_p->time_stamp, sector_p->write_stamp, sector_p->lba_stamp);

    return FBE_STATUS_OK;
}
/****************************************** 
 * end fbe_raid_fruts_trace_dump_sector()
 *****************************************/
static fbe_bool_t fbe_raid_fruts_trace_all_sectors = FBE_FALSE;
static fbe_bool_t fbe_raid_fruts_trace_all_sector_data = FBE_FALSE;

/*!**************************************************************
 * fbe_raid_fruts_trace_data()
 ****************************************************************
 * @brief
 *  Log info about data for a fruts.
 *
 * @param fruts_p - We data for this fruts.        
 * 
 * @return fbe_status_t
 * 
 ****************************************************************/
fbe_status_t fbe_raid_fruts_trace_data(fbe_raid_fruts_t *fruts_p,
                                       fbe_bool_t b_trace_all_sectors,
                                       fbe_bool_t b_trace_entire_block)
{
    fbe_raid_siots_t   *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_sg_element_t   *data_sgl_p = NULL;
    fbe_u64_t          *data_ptr = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_TRACE_ALL_DATA)){
        b_trace_all_sectors = FBE_TRUE;
    }

    /* RAID should only issue requests with the memory descriptor.
     */
    fbe_raid_fruts_get_sg_ptr(fruts_p, &data_sgl_p);
    data_ptr = (fbe_u64_t *)fbe_raid_memory_get_first_block_buffer(data_sgl_p);
    if (data_ptr == NULL)
    {
        /* Some request may not have data associated with them.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "fru: %08x l:%016llx b:%08llx op: %d no data to dump\n",
                                    fbe_raid_memory_pointer_to_u32(fruts_p), fruts_p->lba, fruts_p->blocks, fruts_p->opcode);
    }
    else if ((b_trace_all_sectors == FBE_FALSE) && (fbe_raid_fruts_trace_all_sectors == FBE_FALSE))
    {
        /* If requested dump all the data.
         */
        if (b_trace_entire_block)
        {
            fbe_raid_fruts_trace_dump_sector(fruts_p, siots_p, (fbe_sector_t *)data_ptr, fruts_p->lba);
        }
        else
        {
            /* Else dump the first and last 16 bytes of the first and last blocks
             * of the request.  Since stripped mirrors use a different block
             * size, assert if that is the case.
             * Note!! The `:' matters so that we can screen for fru and lba etc!!
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                        "fru: %08x l:%016llx %016llx %016llx %016llx %016llx\n",
                                        fbe_raid_memory_pointer_to_u32(fruts_p), fruts_p->lba, 
                                        data_ptr[0], data_ptr[1], 
                                        data_ptr[FBE_BE_BYTES_PER_BLOCK/sizeof(fbe_u64_t) - 2], 
                                        data_ptr[FBE_BE_BYTES_PER_BLOCK/sizeof(fbe_u64_t) - 1]);
        }

        /* Now dump the last block.
         */
        if (fruts_p->blocks > 1)
        {
            data_ptr = (fbe_u64_t *)fbe_raid_memory_get_last_block_buffer(data_sgl_p);
            if (data_ptr != NULL)
            {
                /* If requested dump all the data.
                 */
                if (b_trace_entire_block)
                {
                    fbe_raid_fruts_trace_dump_sector(fruts_p, siots_p, (fbe_sector_t *)data_ptr, (fruts_p->lba + fruts_p->blocks - 1));
                }
                else
                {
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                             "fru: %08x l:%016llx %016llx %016llx %016llx %016llx\n", 
                                             fbe_raid_memory_pointer_to_u32(fruts_p), (fruts_p->lba + fruts_p->blocks - 1), 
                                             data_ptr[0], data_ptr[1], 
                                             data_ptr[FBE_BE_BYTES_PER_BLOCK/sizeof(fbe_u64_t) - 2], 
                                             data_ptr[FBE_BE_BYTES_PER_BLOCK/sizeof(fbe_u64_t) - 1]);
                }
            }
        }
    } /* end else if trace all sectors is false */
    else
    {
        /* Trace each block.
         */
        fbe_sg_element_t* first_sg_p;
        fbe_sg_element_t* current_sg_p;
        fbe_block_count_t total_blocks = fruts_p->blocks;
        fbe_lba_t current_lba = fruts_p->lba;
        fbe_u32_t blocks_traced = 0;

        /* Get the pointer to the SG elements. 
         * Save the first SG element pointer for debugging purposes. 
         */
        fbe_raid_fruts_get_sg_ptr(fruts_p, &first_sg_p);
        current_sg_p = first_sg_p;
        while ((total_blocks > 0) && (current_sg_p->count != 0))
        {
            /* Get the physical or virtual address of the SG elements.
            */ 
            fbe_block_count_t blocks = current_sg_p->count / FBE_BE_BYTES_PER_BLOCK;
            fbe_sector_t* sector_p = (fbe_sector_t*)fbe_sg_element_address(current_sg_p);
            
            /* Loop over the number of blocks of this SG element.
             */
            while (blocks > 0)
            {

                /* By default we just do one trace of 4 16 byte words. 
                 * If we trace the entire block, then we trace all the words. 
                 */
                if (b_trace_entire_block || fbe_raid_fruts_trace_all_sector_data)
                {
                    fbe_raid_fruts_trace_dump_sector(fruts_p, siots_p, sector_p, current_lba);
                }
                /* Tracing all sectors is too verbose. 
                 * Instead we will trace the first and last 3 sectors as well as every 8 sectors. 
                 */
                else if ((current_lba % 0x8) == 0 ||
                         ((current_lba >= fruts_p->lba) && (current_lba <= (fruts_p->lba + 3))) ||
                         ((current_lba >= (fruts_p->lba + blocks - 4)) && 
                          (current_lba <= (fruts_p->lba + blocks - 1))))
                {
                    data_ptr = (fbe_u64_t *)&sector_p->data_word[0];
                    if (data_ptr != NULL) {
                        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                                    "fru: %08x l:%016llx %016llx %016llx %016llx %016llx\n",
                                                    fbe_raid_memory_pointer_to_u32(fruts_p), current_lba, 
                                                    data_ptr[0], data_ptr[1], 
                                                    data_ptr[FBE_BE_BYTES_PER_BLOCK/sizeof(fbe_u64_t) - 2], 
                                                    data_ptr[FBE_BE_BYTES_PER_BLOCK/sizeof(fbe_u64_t) - 1]);
                    }
                }

                /* Advance to the next block. 
                 */
                current_lba++;
                --blocks;
                total_blocks--;
                sector_p++;
                blocks_traced++;
            }    /* while (block > 0) */

            /* Advance to the next SG element. 
             */
            ++current_sg_p;
        }    /* end of while (current_sg_p->count != 0) */
    } /* end else trace every sector. */
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_fruts_trace_data()
 **************************************/
/*!**************************************************************
 * fbe_raid_fruts_trace()
 ****************************************************************
 * @brief
 *  Log some trace information for this fru.
 *
 * @param fruts_p - We log opcode, lba blocks, and optionally data.               
 * @param b_start - True if we are sending the request
 *                  False if we are completing the request.
 * 
 * @return fbe_status_t
 * 
 ****************************************************************/
fbe_status_t fbe_raid_fruts_trace(fbe_raid_fruts_t *fruts_p,
                                  fbe_bool_t b_start)
{
    fbe_raid_siots_t *siots_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_object_id_t object_id;
    fbe_block_edge_t *block_edge_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    siots_p = fbe_raid_fruts_get_siots(fruts_p);
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
    payload_p = fbe_transport_get_payload_ex(fbe_raid_fruts_get_packet(fruts_p));
    /* Only continue if we have fruts tracing enabled.
     */
    if (!fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING))
    {
        return FBE_STATUS_OK;
    }
    fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, fruts_p->position);

    /* Dump the request information to the console/ktrace
     * Note!! The `:' matters so that we can screen for fru and lba etc!!
     */
    if (b_start)
    {
        /* For the start request we don't dump the data.
         */
        block_operation_p = fbe_payload_ex_get_next_block_operation(payload_p);

        if (!fbe_raid_geometry_is_attribute_set(raid_geometry_p, FBE_RAID_ATTRIBUTE_EXTENT_POOL)) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                        "fru: %08x%s l:%016llx b:%08llx op: %d start "
                                        "rg: 0x%x pos: %d siots: %08x\n",
                                        fbe_raid_memory_pointer_to_u32(fruts_p),
                                        (block_operation_p->block_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY) ? " 1" : "", 
                                        fruts_p->lba, fruts_p->blocks, fruts_p->opcode,
                                        object_id, fruts_p->position, fbe_raid_memory_pointer_to_u32(siots_p));
        } else {
            fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
            fbe_lba_t lba = fruts_p->lba;
            fbe_u32_t pool_pos = fruts_p->position;
            fbe_extent_pool_slice_get_drive_position(iots_p->extent_p, iots_p->extent_context_p, &pool_pos);
            fbe_raid_iots_get_extent_offset(iots_p, fruts_p->position, &lba);

            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                        "fru: %08x%s %016llx/%08llx/%llx %u st"
                                        "rg: 0x%x %u/%u %08x\n",
                                        fbe_raid_memory_pointer_to_u32(fruts_p),
                                        (block_operation_p->block_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY) ? " 1" : "", 
                                        fruts_p->lba, fruts_p->blocks, lba, fruts_p->opcode,
                                        object_id, fruts_p->position, pool_pos, fbe_raid_memory_pointer_to_u32(siots_p));
        }
        
    }
    else
    {
        fbe_payload_block_operation_status_t block_status;
        fbe_payload_block_operation_qualifier_t block_qualifier;
        fbe_status_t transport_status;
        fbe_u32_t transport_qualifier;

        /* Else dump the end information.
         */
        fbe_raid_fruts_get_status(fruts_p,
                                  &transport_status,
                                  &transport_qualifier,
                                  &block_status,
                                  &block_qualifier);
        block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "fru: %08x%s l:%016llx b:%08llx op: %d end   "
                                    "rg: 0x%x pos: %d siots: %08x "
                                    "p/b/q:%d/%d/%d\n",
                                    fbe_raid_memory_pointer_to_u32(fruts_p),
                                    (block_operation_p->block_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY) ? " 1" : "", 
                                    fruts_p->lba, fruts_p->blocks, fruts_p->opcode,
                                    object_id, fruts_p->position, fbe_raid_memory_pointer_to_u32(siots_p),
                                    transport_status, block_status, block_qualifier);

        /*! @note fru traffic tracing must be enabled to dump the data and we will not
         *        dump the data unless the request succeeded.
         */
        if ((fruts_p->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)                                     &&
            fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING)    )
        {
            fbe_bool_t  b_dump_first_and_last = FBE_FALSE;
            fbe_lba_t   journal_log_start_lba = FBE_LBA_INVALID;

            fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_log_start_lba);
            if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_TRACE_PAGED_MD_WRITE_DATA) &&
                fbe_raid_siots_is_metadata_operation(siots_p)                                                               &&
                (fruts_p->lba < journal_log_start_lba)                                                                      &&
                (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)                                                  )
            {
                b_dump_first_and_last = FBE_TRUE;
            }
            fbe_raid_fruts_trace_data(fruts_p, 
                                      FBE_FALSE, /* Trace all sectors or just first and last sectors? */
                                      b_dump_first_and_last /* do not trace all data for every sector. */);
        
        } /* end if data tracing is enabled */
    
    } /* end else if end (fruts completion) */

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_fruts_trace()
 ******************************************/

/*!***************************************************************************
 *          fbe_raid_fruts_validate_fruts()
 *****************************************************************************
 *
 * @brief   This method validates that a fruts has been properly formed before
 *          allowing it to be fowarded to the next level.
 *
 * @param   fruts_p - Pointer to fruts to validate
 *
 * @return  status - FBE_STATUS_OK - fruts validation successful
 *                   Other - Something is wrong with fruts
 *
 * @note    This method assumes that we have validated the raid geometry and
 *          siots.
 *
 * @author
 *  03/09/2009  Ron Proulx  - Created
 *
 *****************************************************************************/
static __forceinline fbe_status_t fbe_raid_fruts_validate_fruts(fbe_raid_fruts_t *fruts_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_siots_t       *siots_p = NULL;
    fbe_raid_geometry_t    *raid_geometry_p = NULL;
    fbe_raid_common_flags_t fruts_flags;
    fbe_bool_t              b_allow_full_journal_access = FBE_FALSE;

    /* NULL fruts not allowed.
     */
    if (fruts_p == NULL)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: %s - NULL fruts pointer\n",
                               __FUNCTION__);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Get the geometry and validate the capacity
     */
    if (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO)
    {
        /* We allow zeroing of the journal area.
         */
        b_allow_full_journal_access = FBE_TRUE;
    }
    siots_p = fbe_raid_fruts_get_siots(fruts_p);
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    if (fbe_raid_geometry_does_request_exceed_extent(raid_geometry_p,
                                                     fruts_p->lba,
                                                     fruts_p->blocks,
                                                     b_allow_full_journal_access) == FBE_TRUE)

    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "fruts %p lba %llx + blocks 0x%llx is beyond capacity \n",
                             fruts_p, (unsigned long long)fruts_p->lba,
			     (unsigned long long)fruts_p->blocks);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Validate fruts flags
     */
    fruts_flags = fbe_raid_common_get_flags(&fruts_p->common);
    if (((fruts_flags & FBE_RAID_COMMON_FLAG_REQ_BUSIED) != 0)  ||
        ((fruts_flags & FBE_RAID_COMMON_FLAG_REQ_STARTED) != 0)    )
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "fruts %p lba %llx + blocks 0x%llx flags: 0x%x invalid \n",
                             fruts_p, (unsigned long long)fruts_p->lba,
			     (unsigned long long)fruts_p->blocks, fruts_flags);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Always return the execution status.
     */
    return(status);
}
/************************************
 * end fbe_raid_fruts_validate_fruts()
 ************************************/

/*!**************************************************************************
 *          fbe_raid_fruts_send()
 ***************************************************************************
 *
 * @brief
 *    Start a single fruts to the DH driver.
 *    If the DH is not ready, we will eventually receive a READY for the fruts
 *
 * @param   fruts_p [IO] - The fruts to begin.
 *                         The fruts flags field has FBE_RAID_COMMON_FLAG_REQ_STARTED once
 *                         the request is successfully submitted to the dh.
 *
 * @param   evaluate_state,  [I]  - If != NULL we set the fruts evaluate state to this.
 *
 * @return
 *   Returns FBE_TRUE if the fruts was successfully started, and
 *           FBE_FALSE if the DH driver responded with a busy.
 *
 * @note
 *   This function is not responsible for queueing fruts',
 *   It is the responsibility of the caller to queue if necessary.
 *
 * @author
 *  05/21/2009  Ron Proulx  - Created
 *
 **************************************************************************/

fbe_bool_t fbe_raid_fruts_send(fbe_raid_fruts_t *fruts_p, void (*evaluate_state))
{
    fbe_bool_t          ret_val = FBE_TRUE;
    fbe_packet_t       *packet_p = NULL;
    fbe_raid_siots_t   *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_block_edge_t   *block_edge_p = NULL;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_block_count_t   max_blocks_per_drive = 0;

    /*! Send the packet.
     */
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Validate the fruts.
     */
    if (RAID_DBG_ENA(fbe_raid_fruts_validate_fruts(fruts_p) != FBE_STATUS_OK))
    {
        /* Generate an error trace and return the status `False'
         * to indicate a failure.
         */
        ret_val = FBE_FALSE;
        if(RAID_FRUTS_COND(ret_val != FBE_TRUE))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "raid_fruts_send failed to validate fruts\n");
            return ret_val;
        }
        return(ret_val);
    }

    /* If we received a continue, then we need to clear out this flag now 
     * so that the next time we see a drive go away, we will wait for the continue. 
     * This comes into play when the flag has already been set such as for a drive 
     * that is coming and going.  Another case where we can see this is if 
     * two drives go away in close succession in R6 or 3 way mirror. 
     */  
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_RECEIVED_CONTINUE))
    {
        fbe_raid_position_bitmask_t degraded_bitmask;
        /* Trace the fact that we are clearing the continue received
         */
        fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "raid:siots: 0x%p clear received cont for send req to pos: %d degrade_bitmask: 0x%x\n",
                             siots_p, fruts_p->position, degraded_bitmask);
        fbe_raid_siots_clear_flag(siots_p, FBE_RAID_SIOTS_FLAG_RECEIVED_CONTINUE);
    }

#if 0
    /* Debug code to catch mis-directed write of invalidated block. 
     * Failing test R6 Cookie Monster. Vamsi V. 7/26/12   
     */
    if ((fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) ||
        (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY))
    {
        fbe_bool_t do_check = FBE_FALSE;
        fbe_raid_geometry_t * geo_p = fbe_raid_siots_get_raid_geometry(siots_p);

        if (fbe_raid_geometry_is_raid6(geo_p))
        {
            if ((fruts_p->position != siots_p->geo.position[geo_p->width - 1]) &&
                ((fruts_p->position != siots_p->geo.position[geo_p->width - 2])) )
            {
                do_check = FBE_TRUE;
            }

        }
        else if (fbe_raid_geometry_is_raid5(geo_p))
        {
            if (fruts_p->position != siots_p->geo.position[geo_p->width - 1])
            {
                do_check = FBE_TRUE;
            }
        }
        else
        {
            do_check = FBE_TRUE;
        }

        if (do_check == FBE_TRUE)
        {

            fbe_sg_element_t *data_sgl_p = NULL;
            fbe_sector_t * blk_ptr = NULL;
            fbe_u16_t lba_stamp = 0;

            /* RAID should only issue requests with the memory descriptor.
             */
            fbe_raid_fruts_get_sg_ptr(fruts_p, &data_sgl_p);
            blk_ptr = (fbe_sector_t *)fbe_raid_memory_get_first_block_buffer(data_sgl_p);

            /* Check lba stamp */
            lba_stamp = xorlib_generate_lba_stamp(fruts_p->lba, siots_p->misc_ts.cxts.eboard.raid_group_offset);
            if ((blk_ptr->lba_stamp != lba_stamp) &&
                (blk_ptr->lba_stamp != 0x0))
            {
                fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                                             "fru: %08x l:%016llx \n",
                                             fbe_raid_memory_pointer_to_u32(fruts_p), fruts_p->lba);
            }
        }
    }
#endif

    

    /* Populate the embedded packet.
     */
    status = fbe_raid_fruts_populate_packet(fruts_p);
    if (status != FBE_STATUS_OK)
    {
        /* Something went very wrong when initializing the packet. 
         * We have no choice but to complete it with an error. 
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "raid: packet not found\n");
        fbe_transport_set_status(&fruts_p->io_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        status = fbe_raid_fruts_packet_completion(&fruts_p->io_packet, fruts_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                        "raid_fruts_send failed with status 0x%x\n", 
                                   status);
            return FBE_FALSE;
        }
        return FBE_FALSE;
    }
    packet_p = fbe_raid_fruts_get_packet(fruts_p);

    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_LOGGING_ENABLED))      
    {
        /* Display information on this fruts. 
         */
        fbe_raid_fruts_trace(fruts_p, FBE_TRUE    /* yes starting */);

        /* We only check sectors if the correct options are set.
         */ 
        if (RAID_FRUTS_COND(fbe_raid_fruts_check_sectors( fruts_p ) != FBE_STATUS_OK))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "raid_fruts_send fruts check sectors failed, fruts_p = 0x%p\n",
                                        fruts_p);
            return FBE_FALSE;
        }

        /* increment the disk statistics prior to sending the packet.
         */
        fbe_raid_fruts_increment_disk_statistics(fruts_p);

        /* trace to RBA buffer
         */
        if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_LOGGING_ENABLED) &&
            fbe_traffic_trace_is_enabled (KTRC_TFBE_RG_FRU))
        {
            fbe_raid_group_number_t rg_number = FBE_RAID_GROUP_INVALID;
            fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
            fbe_payload_block_operation_t* block_operation_p;
            fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
            fbe_packet_t *iots_packet_p = fbe_raid_iots_get_packet(iots_p);
            fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
            fbe_transport_get_object_id(iots_packet_p, &rg_object_id);
            if((rg_object_id != FBE_OBJECT_ID_INVALID) && (rg_object_id != FBE_U32_MAX))
            {
                fbe_database_get_rg_number(rg_object_id, &rg_number);
            }
            /* The payload has been created, but not sent yet, so it is not yet active.
             */
            block_operation_p = (fbe_payload_block_operation_t*)payload_p->next_operation;
            fbe_traffic_trace_block_operation(FBE_CLASS_ID_RAID_GROUP,
                                          block_operation_p,
                                          rg_number,
                                          fruts_p->position,/*extra information*/
                                          FBE_TRUE, /*RBA trace start*/
                                              fbe_traffic_trace_get_priority(iots_packet_p));
        }
    }

    if (fbe_raid_siots_is_quiesced(siots_p) && !fbe_raid_siots_is_metadata_operation(siots_p))
    {
        /*! We do not expect to be here if we are quiesced.  This indicates an issue 
         * with not checking for quiesced prior to sending out the I/O. 
         * Metadata operations do not get quiesced. 
         */

        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                                      "raid_fruts_send siots: 0x%p wait flags (0x%x) are set\n",
                                      siots_p, siots_p->flags);
    }
    if (!fbe_raid_geometry_is_attribute_set(raid_geometry_p, FBE_RAID_ATTRIBUTE_EXTENT_POOL)) {

        fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, fruts_p->position);
    } else {
        fbe_raid_iots_t *iots_p  = fbe_raid_siots_get_iots(siots_p);
        fbe_raid_iots_get_block_edge(iots_p, &block_edge_p, fruts_p->position);
    }

    if (RAID_FRUTS_COND(block_edge_p == NULL) ) {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "raid_fruts_send block_edge_p %p is NULL\n",
                                    block_edge_p);
        return FBE_FALSE;
    }

    if (RAID_DBG_ENABLED)
    {
        fbe_raid_geometry_get_max_blocks_per_drive(raid_geometry_p, &max_blocks_per_drive);
        if (RAID_COND((fruts_p->blocks > max_blocks_per_drive)                             &&
                      (fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO)         &&
                      (fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED) &&
                      (fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO)     ))
        {
            /* Something is wrong with this fruts.  It generated too large. 
             * Instead of sending it to the drive, we complete it with error and trace. 
             * If we sent this to the drive, it could cause the drive to get shot. 
             */
            fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "raid_fruts_send" ,
                                 "raid: siots info for fruts: %p\n", fruts_p);
            // log is too long, should be split into two lines
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "raid_fruts_send fruts: %p blocks 0x%llx > max of 0x%llx>>\n",
                                        fruts_p,
					(unsigned long long)fruts_p->blocks,
					(unsigned long long)max_blocks_per_drive);
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "pos: %d op: 0x%x lba: 0x%llx<<\n",
                                        fruts_p->position, fruts_p->opcode,
					(unsigned long long)fruts_p->lba);
            // end split log
            fbe_raid_fruts_set_flag(fruts_p, FBE_RAID_FRUTS_FLAG_REQUEST_OUTSTANDING);
            {
                fbe_payload_ex_t * sep_payload_p = NULL;
                fbe_payload_block_operation_t *block_operation_p = NULL;
                sep_payload_p = fbe_transport_get_payload_ex(packet_p);
                status = fbe_payload_ex_increment_block_operation_level(sep_payload_p);
                block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);
                fbe_payload_block_set_status(block_operation_p,
                                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_TRANSFER_SIZE_EXCEEDED);
                fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_TRUE;
            }
        }
    }
    /* set the outstanding flag in fruts which we will clear in 
     * evaluate state after successfull completion
     */
    fbe_raid_fruts_set_flag(fruts_p, FBE_RAID_FRUTS_FLAG_REQUEST_OUTSTANDING);

    fbe_block_transport_send_functional_packet(block_edge_p, packet_p);

    return ret_val;
}
/**********************************
 * end fbe_raid_fruts_send()
 **********************************/

/*!**************************************************************************
 * fbe_raid_fruts_send_chain()
 ***************************************************************************
 *
 * @brief
 *  Start a chain of fruts.
 *
 * @param head_p - head of the queue to start.
 *
 * @param   evaluate_state If != NULL we set the fruts evaluate state to this.
 *
 * @return
 *   Returns FBE_TRUE if the fruts was successfully started
 *           FBE_FALSE if we hit an issue.
 *
 * @note
 *   This function is not responsible for queueing fruts',
 *   It is the responsibility of the caller to queue if necessary.
 *
 * @author
 *  08/05/09 Rob Foley Created.
 *
 **************************************************************************/

fbe_bool_t fbe_raid_fruts_send_chain(fbe_queue_head_t *head_p, void (*evaluate_state))
{
    fbe_bool_t b_status = FBE_TRUE;
    fbe_raid_fruts_t *current_fruts_p = NULL;
    fbe_raid_fruts_t *next_fruts_p = NULL;
    fbe_raid_siots_t *siots_p = NULL;
    fbe_atomic_t initial_wait_count = 0;
    fbe_u32_t num_fruts_sent = 0;

    current_fruts_p = (fbe_raid_fruts_t *)fbe_queue_front(head_p);

    if (current_fruts_p != NULL)
    {
        siots_p = fbe_raid_fruts_get_siots(current_fruts_p);
        initial_wait_count = siots_p->wait_count;
    }
    /* Just loop over everything, trying to start them.
     * Note that we do not send more than the initial wait count, since if 
     * we do have nops in the chain at the end, this can cause failures when something finishes 
     * and frees the siots. 
     */
    while ((current_fruts_p != NULL) && (num_fruts_sent < initial_wait_count))
    {
        /* Try to start the next fruts to the edge.
         */
        next_fruts_p = (fbe_raid_fruts_t *)fbe_queue_next(head_p, 
                                                          (fbe_queue_element_t*)current_fruts_p);

        /* If the opcode is invalid we do not want to send this fruts.
         */
        if (current_fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID)
        {
            if(RAID_DBG_ENA(num_fruts_sent >= initial_wait_count) )
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "raid: num_fruts_sent 0x%x >= initial_wait_count 0x%llx, siots_p = 0x%p\n",
                                     num_fruts_sent, initial_wait_count, siots_p);
                return FBE_FALSE;
            }
            b_status = fbe_raid_fruts_send(current_fruts_p, NULL);
            if (b_status != FBE_TRUE)
            {
                break;
            }
            num_fruts_sent++;
        }
        current_fruts_p = next_fruts_p;
    }
    if (RAID_FRUTS_COND(num_fruts_sent > initial_wait_count) )
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: num_fruts_sent 0x%x > initial_wait_count 0x%llx\n",
                             num_fruts_sent,
			     (unsigned long long)initial_wait_count);
        return FBE_FALSE;
    }
    return b_status;
}
/**********************************
 * end fbe_raid_fruts_send_chain()
 **********************************/

/*!**************************************************************************
 * fbe_raid_fruts_send_chain_bitmap()
 ***************************************************************************
 *
 * @brief
 *  Start a chain of fruts.
 *
 * @param head_p - head of the queue to start.
 *
 * @param   evaluate_state If != NULL we set the fruts evaluate state to this.
 * @param start_bitmap - Bitmap of positions to start.
 *
 * @return
 *   Returns FBE_STATUS_OK if the fruts was successfully started
 *           FBE_STATUS_GENERIC_FAILURE if we hit an issue.
 *
 * @note
 *   This function is not responsible for queueing fruts',
 *   It is the responsibility of the caller to queue if necessary.
 *
 * @author
 *  08/05/09 Rob Foley Created.
 *
 **************************************************************************/

fbe_status_t fbe_raid_fruts_send_chain_bitmap(fbe_queue_head_t *head_p, 
                                            void (*evaluate_state),
                                            fbe_raid_position_bitmask_t start_bitmap)
{
    fbe_bool_t b_status = FBE_TRUE;
    fbe_raid_fruts_t *current_fruts_p = NULL;
    fbe_raid_fruts_t *next_fruts_p = NULL;
    fbe_raid_siots_t *siots_p = NULL;
    fbe_atomic_t initial_wait_count = 0;
    fbe_u32_t num_fruts_sent = 0;

    current_fruts_p = (fbe_raid_fruts_t *)fbe_queue_front(head_p);

    if (current_fruts_p != NULL)
    {
        siots_p = fbe_raid_fruts_get_siots(current_fruts_p);
        initial_wait_count = siots_p->wait_count;
    }
    /* Just loop over everything, trying to start them.
     * Do not iterate beyond the number of things we need to send. 
     */
    while ((current_fruts_p != NULL) && (num_fruts_sent < initial_wait_count))
    {
        /* Try to start the next fruts to the edge.
         */
        next_fruts_p = (fbe_raid_fruts_t *)fbe_queue_next(head_p, 
                                                          (fbe_queue_element_t*)current_fruts_p);

        /* If the opcode is invalid we do not want to send this fruts.
         */
        if (((1 << current_fruts_p->position) & start_bitmap) &&
            (current_fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID))
        {
            if (RAID_FRUTS_COND(num_fruts_sent >= initial_wait_count))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "raid: num_fruts_sent 0x%x >= initial_wait_count 0x%llx.\n",
                                      num_fruts_sent, initial_wait_count);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            b_status = fbe_raid_fruts_send(current_fruts_p, NULL);
            if (b_status != FBE_TRUE)
            {
                break;
            }
            num_fruts_sent++;
        }
        current_fruts_p = next_fruts_p;
    }
    
    if (RAID_FRUTS_COND(num_fruts_sent > initial_wait_count))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: num_fruts_sent 0x%x > initial_wait_count 0x%llx\n",
                             num_fruts_sent, initial_wait_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (b_status != FBE_TRUE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        return FBE_STATUS_OK;
    }
}
/**********************************
 * end fbe_raid_fruts_send_chain_bitmap()
 **********************************/

/*!**************************************************************************
 *          fbe_raid_fruts_evaluate()
 ***************************************************************************
 *
 * @brief
 *    This is called from rthe raid group state machine when a packet
 *    completion occurs.  Wakeup the parent siots.
 *
 * @param   fruts_p - [IO], The fruts to evaluate
 *
 * @return  FBE_RAID_STATE_STATUS_WAITING 
 *
 *  NOTES:
 *
 * @author
 *  05/21/2009  Ron Proulx  - Updated from rg_fruts_evaluate
 *
 **************************************************************************/

fbe_raid_state_status_t fbe_raid_fruts_evaluate(fbe_raid_fruts_t *fruts_p)
{
    fbe_raid_siots_t   *siots_p = fbe_raid_fruts_get_siots(fruts_p);
	fbe_packet_t * packet = NULL;
	fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_status_t packet_status;

    /* We should get the outstanding flag set in fruts.
     * We will clear it here
     */
    if (RAID_DBG_ENABLED)
    {
        if (!fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_REQUEST_OUTSTANDING))
        {
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                 "raid: got unexpected error: fruts_p 0x%p flag is"
                                 "not set correctly, siots_p = 0x%p\n",
                                 fruts_p, siots_p);
            return FBE_RAID_STATE_STATUS_WAITING;
        }
    }
    fbe_raid_fruts_clear_flag(fruts_p, FBE_RAID_FRUTS_FLAG_REQUEST_OUTSTANDING);

    /* Make sure we still have something outstanding.
     * We do a lower and upper range check.  The upper range check is fairly arbitrary, 
     * but it is unlikely that we will have more than twice the nubmer of drives worth of 
     * operations outstanding at this time. 
     */
    if (RAID_DBG_ENA(siots_p->wait_count > (fbe_raid_siots_get_width(siots_p) * 2)))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "%s siots_p %p has invalid wait count: %lld\n", __FUNCTION__,
                             siots_p, (long long)siots_p->wait_count);
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    /* Make sure we did not already recive status for this request.
     */
    if (RAID_DBG_ENA(fruts_p->status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "%s fruts: %p status: 0x%x is unexpected position: 0x%x \n", 
                             __FUNCTION__, fruts_p, fruts_p->status, fruts_p->position);
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    packet = fbe_raid_fruts_get_packet(fruts_p);
    if (packet == NULL) {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS, "%s fruts %p Can't get packet\n", __FUNCTION__, fruts_p);
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    /* Now get the payload.*/
    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS, "%s fruts %p packet %p Can't get payload\n", __FUNCTION__, fruts_p, packet);
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    /* Copy the packet status to the fruts. */
    block_operation_p = fbe_raid_fruts_to_block_operation(fruts_p);
    if (block_operation_p == NULL) {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "%s fruts %p packet %p has a null block operation\n",
                             __FUNCTION__, fruts_p, packet);
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    fbe_payload_block_get_status(block_operation_p, &fruts_p->status);
    fbe_payload_block_get_qualifier(block_operation_p, &fruts_p->status_qualifier);
    packet_status = fbe_transport_get_status_code(&fruts_p->io_packet);

    if ((fruts_p->status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) || 
        (packet_status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_ERROR_PENDING);
        fbe_payload_ex_get_media_error_lba(payload, &fruts_p->media_error_lba); 
    }
    fbe_payload_ex_get_pvd_object_id(payload, &fruts_p->pvd_object_id);

    /* Display information on this fruts.
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_LOGGING_ENABLED))      
    {
        raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING))
        {
            fbe_raid_fruts_trace(fruts_p, FBE_FALSE /* not starting */);
        }
        /*
         * trace to RBA buffer
         */
        if (fbe_traffic_trace_is_enabled (KTRC_TFBE_RG_FRU))
        {
            fbe_raid_group_number_t rg_number = FBE_RAID_GROUP_INVALID;
            fbe_object_id_t rg_object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
            fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
            fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);

            if((rg_object_id != FBE_OBJECT_ID_INVALID) && (rg_object_id != FBE_U32_MAX))
            {
                fbe_database_get_rg_number(rg_object_id, &rg_number);
            }
            fbe_traffic_trace_block_operation(FBE_CLASS_ID_RAID_GROUP,
                                          block_operation_p,
                                          rg_number,
                                          fruts_p->position,/*extra information*/
                                          FBE_FALSE, /*RBA trace end*/
                                              fbe_traffic_trace_get_priority(packet_p));
        }
    }

    if (RAID_DBG_ENA(siots_p->wait_count == 0))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "%s siots_p %p has invalid wait count: %lld\n", __FUNCTION__,
                             siots_p, siots_p->wait_count);
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    /* Now invoke complete function
     */
    if (fbe_atomic_decrement(&siots_p->wait_count) == 0)
    {
        if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_COMPLETE_IMMEDIATE))
        {
            fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

            fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
            fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
            /* When this flag is set we will complete the iots immediately.
             */  
            fbe_raid_iots_mark_complete(iots_p);
            fbe_raid_common_state((fbe_raid_common_t *)iots_p);
        }
        else
        {
            /* We are done with all requests simply enqueue the request to 
             * the raid group state queue.
             */
            fbe_raid_common_state((fbe_raid_common_t *)siots_p);
        }
    }

    /* Return waiting which will fetch the next request (either the next
     * fruts or the siots).
     */
    return(FBE_RAID_STATE_STATUS_WAITING);
}
/**********************************
 * end fbe_raid_fruts_evaluate()
 **********************************/

/*!**************************************************************
 *          fbe_raid_fruts_handle_transport_error()
 ****************************************************************
 * @brief
 *  Converts a transport error into a fruts status.  This is needed
 *  to determine if the device is dead to a failed device or if the
 *  RAID group is being shutdown for instance.
 *
 * @param raid_geometry_p - Pointer to raid group to trace for
 * @param fruts_p  - Pointer to fruts to get object id for
 * @param transport_status - The status returned by the I/O transport
 * @param eboard_p - Pointer to ebaord to update
 * @param fruts_status_p - Pointer to fruts status to generate
 * 
 * @return None - Always suceeds
 *
 * @note This should only be invoked when the fruts is in error.
 *
 * @author
 *  09/08/2009  Ron Proulx - Created
 *
 ****************************************************************/

static void fbe_raid_fruts_handle_transport_error(fbe_raid_geometry_t *raid_geometry_p, 
                                                  fbe_raid_fruts_t *fruts_p, 
                                                  fbe_status_t transport_status,
                                                  fbe_u32_t transport_qualifier,
                                                  fbe_raid_fru_eboard_t *eboard_p,
                                                  fbe_raid_fru_error_status_t *fruts_status_p)
{
    fbe_u16_t           fru_bitmask; 
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);

    /* Get the bitmask for this position.
     */
    fru_bitmask = (1 << fruts_p->position);

    if ((transport_status == FBE_STATUS_BUSY) &&
        fbe_raid_siots_is_monitor_initiated_op(siots_p) &&
        fbe_raid_fruts_block_edge_has_timeout_errors(fruts_p))
    {
        /* Monitor ops where we are getting retry errors and the edge is timed out 
         * need to get aliased to non-retryable errorsOthewise we will retry forever and .
         * the monitor will never have a chance to run to process the edge attr change.
         */
        transport_status = FBE_STATUS_DEAD;

        fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                    "BUSY err w/tmeout "
                                    "pos: %d op: 0x%x lba: 0x%llx bl:0x%llx fruts:0x%x\n",
                                    fruts_p->position, fruts_p->opcode, fruts_p->lba, 
                                    fruts_p->blocks, fbe_raid_memory_pointer_to_u32(fruts_p));
    }
    /* Now switch on the transport status.
     */
    switch(transport_status)
    {
         case FBE_STATUS_OK:
            /* No transport error. */
            *fruts_status_p = FBE_RAID_FRU_ERROR_STATUS_SUCCESS;
            break;
        
        case FBE_STATUS_CANCEL_PENDING:
        case FBE_STATUS_CANCELED:
            /* Canceled signifies the I/O has been aborted.
             */
            *fruts_status_p = FBE_RAID_FRU_ERROR_STATUS_ABORTED;
            eboard_p->abort_err_count++;
            break;

        case FBE_STATUS_BUSY:   
            /* The object is in activate or activate pending state.
             * Set the fruts status to retry since this is a 
             * retryable error. 
             */
            *fruts_status_p = FBE_RAID_FRU_ERROR_STATUS_RETRY;
            if ((eboard_p->retry_err_bitmap & fru_bitmask) == 0)
            {
                eboard_p->retry_err_bitmap |= fru_bitmask;
                eboard_p->retry_err_count++;
            }
            /* Needed since later there are cases where we will only be 
             * looking at the fruts block status when checking for retryable errors. 
             */
            fbe_raid_fruts_set_block_status(fruts_p, 
                                            FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                            FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
            fbe_raid_fruts_inc_error_count(fruts_p, FBE_RAID_FRU_ERROR_TYPE_RETRYABLE);
            break;

        case FBE_STATUS_DEAD:   
            /* The object is in destroy or destroy pending state.
             * Set the fruts status to raid group is shutting down. 
             */ 
            *fruts_status_p = FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN;
            if ((eboard_p->dead_err_bitmap & fru_bitmask) == 0)
            {
                eboard_p->dead_err_bitmap |= fru_bitmask;
                eboard_p->dead_err_count++;
            }
            fbe_raid_fruts_inc_error_count(fruts_p, FBE_RAID_FRU_ERROR_TYPE_NON_RETRYABLE);
            break;

        case FBE_STATUS_QUIESCED:   
            /* This IO, coming from a monitor operation, is rejected when the object is quiesced.
             * We want to hand this back to the monitor instead of retrying IO right away.
             */
        case FBE_STATUS_FAILED:
            /* FBE_STATUS_FAILED means that the object below us drained I/O because it went to failed. We will treat
             * this just like an other dead error. 
             */
        case FBE_STATUS_EDGE_NOT_ENABLED: 
            /* For all other transport errors set the fruts status to error.
             */
            *fruts_status_p = FBE_RAID_FRU_ERROR_STATUS_ERROR;

            if ((eboard_p->dead_err_bitmap & fru_bitmask) == 0)
            {
                eboard_p->dead_err_bitmap |= fru_bitmask;
                eboard_p->dead_err_count++;
            }
            fbe_raid_fruts_inc_error_count(fruts_p, FBE_RAID_FRU_ERROR_TYPE_NON_RETRYABLE);
            /* Add this new dead position into the siots.
             */
            fbe_raid_siots_add_degraded_pos(siots_p, fruts_p->position);
            break;

        default:
            fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                          FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                          "raid: Unexpected packet status pkt_st: 0x%x qual: 0x%x "
                                          "pos: %d op: 0x%x lba: 0x%llx bl: 0x%llx\n",
                                          transport_status, transport_qualifier, fruts_p->position, 
                                          fruts_p->opcode,
					  (unsigned long long)fruts_p->lba,
					  (unsigned long long)fruts_p->blocks);
            
            *fruts_status_p = FBE_RAID_FRU_ERROR_STATUS_UNEXPECTED;
            eboard_p->unexpected_err_count++;
            break;
    }; /* end switch on transport status */

    /* Trace this error.
     */
    if (transport_status != FBE_STATUS_OK)
    {
        /* Trace the basic information.
         */
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                      "pkt_st: 0x%x qual: 0x%x pos: %d op: 0x%x lba: 0x%llx bl: 0x%llx fts: 0x%x\n",
                                      transport_status, transport_qualifier,
				      fruts_p->position, fruts_p->opcode,
				      (unsigned long long)fruts_p->lba,
				      (unsigned long long)fruts_p->blocks, 
                                     fbe_raid_memory_pointer_to_u32(fruts_p));
    }
    return;
}
/**********************************************
 * end fbe_raid_fruts_handle_transport_error()
 **********************************************/


/*!**************************************************************
 * fbe_raid_fruts_chain_get_eboard()
 ****************************************************************
 * @brief
 *  Determine the kinds of errors in the chain of fruts.
 *
 * @param fruts_ptr - Head of the chain of fruts.
 * @param eobard_p - eboard to store the results in
 * 
 * @return - FBE_TRUE if no errors.
 *          FBE_FALSE if an unexpected error was encountered.
 *
 * @author
 *  8/18/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_raid_fruts_chain_get_eboard(fbe_raid_fruts_t * fruts_ptr, 
                                           fbe_raid_fru_eboard_t * eboard_p)
{
    fbe_status_t b_status = FBE_TRUE;
    fbe_raid_fruts_t *current_fruts_ptr;
    fbe_raid_fru_error_status_t status = FBE_RAID_FRU_ERROR_STATUS_SUCCESS;
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_ptr);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Otherwise this indicates that there was type of error
     * or error injection is enabled.
     */
    for (current_fruts_ptr = fruts_ptr;
         current_fruts_ptr != NULL;
         current_fruts_ptr = fbe_raid_fruts_get_next(current_fruts_ptr))
    {
        fbe_u16_t current_fru_bitmask = (1 << current_fruts_ptr->position);
        fbe_payload_block_operation_status_t block_status;
        fbe_payload_block_operation_qualifier_t block_qualifier;
        fbe_status_t transport_status = FBE_STATUS_GENERIC_FAILURE;
        fbe_u32_t transport_qualifier;
        fbe_status_t packet_status;

        /* This fruts is a no-op, do not process it (assume success).
         */
        if (current_fruts_ptr->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID)
        {
            continue;
        }

        packet_status = fbe_raid_fruts_get_status(current_fruts_ptr, 
                                                  &transport_status,
                                                  &transport_qualifier,
                                                  &block_status, 
                                                  &block_qualifier);
        
        /* If there is something wrong with this request log an error.
         */
        if (packet_status != FBE_STATUS_OK)
        {
            /* Invoke method to handle transport errors.  This will
             * update the ebaord as required.
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                        "raid: %s %d unable to get packet status. status: 0x%x\n", 
                                   __FUNCTION__, __LINE__, status);
            return FBE_FALSE;
        }

        /* If the transport wasn't successful it means the device wasn't in
         * the ready state.  Set the dead bit and call the routine to translate
         * to the appropriate fruts status.
         */
        if (transport_status != FBE_STATUS_OK)
        {
            /* Invoke method to handle transport errors.  This will
             * update the ebord as required.
             */
            fbe_raid_fruts_handle_transport_error(raid_geometry_p,
                                                  current_fruts_ptr,
                                                  transport_status,
                                                  transport_qualifier,
                                                  eboard_p,
                                                  &status);
            if (status == FBE_RAID_FRU_ERROR_STATUS_UNEXPECTED)
            {
                b_status = FBE_FALSE;
            }
            continue;
        }

        /* Increment the error counts depending on the error
         * we received on this request.
         */
        switch (block_status)
        { 
            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED:
                if ((block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE) &&
                    fbe_raid_siots_is_monitor_initiated_op(siots_p) &&
                    fbe_raid_fruts_block_edge_has_timeout_errors(current_fruts_ptr))
                {
                    /* Monitor ops where we are getting retry errors and the edge is timed out 
                     * need to get aliased to non-retryable errors. 
                     * Othewise we will retry forever and the monitor will never have a chance to run 
                     * to process the edge attr change.
                     */
                    block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE;
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                                "retry err w/tmeout "
                                                "pos: %d op: 0x%x lba: 0x%llx bl:0x%llx fruts:0x%x\n",
                                                current_fruts_ptr->position, current_fruts_ptr->opcode, current_fruts_ptr->lba, 
                                                current_fruts_ptr->blocks, fbe_raid_memory_pointer_to_u32(current_fruts_ptr));
                }
                /*! An I/O failed status no longer implies a non-retryable error.
                 *  Look at the qualifier to determine if it is retryable or not.
                 */
                if (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE)
                {
                    if ((eboard_p->dead_err_bitmap & current_fru_bitmask) == 0)
                    {
                        /* If we have not yet recorded this position,
                         * then record it now.
                         */
                        eboard_p->dead_err_bitmap |= current_fru_bitmask;
                        eboard_p->dead_err_count++;
                    }
                    else
                    {
                        /* The bit is set, so our count better be > 0.
                         */
                        if(RAID_COND(eboard_p->dead_err_count == 0)) 
                        {
                            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                        "eboard_p->dead_err_count 0x%x is zero\n",
                                                        eboard_p->dead_err_count);
                            return FBE_FALSE;
                        }
                    }
                    fbe_raid_fruts_inc_error_count(current_fruts_ptr, FBE_RAID_FRU_ERROR_TYPE_NON_RETRYABLE);
                    /* Add this new dead position into the siots.
                     */
                    fbe_raid_siots_add_degraded_pos(siots_p, current_fruts_ptr->position);
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                                "non-retryable err "
                                                "pos: %d op: 0x%x lba: 0x%llx bl:0x%llx fruts:0x%x\n",
                                                current_fruts_ptr->position,
						current_fruts_ptr->opcode,
						(unsigned long long)current_fruts_ptr->lba, 
                                                (unsigned long long)current_fruts_ptr->blocks,
						fbe_raid_memory_pointer_to_u32(current_fruts_ptr));
                }
                else if ((block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE) ||
                         (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_LOCK_FAILED))
                {
                    if ((eboard_p->retry_err_bitmap & current_fru_bitmask) == 0)
                    {
                        eboard_p->retry_err_bitmap |= current_fru_bitmask;
                        eboard_p->retry_err_count++;
                    }
                    fbe_raid_fruts_inc_error_count(current_fruts_ptr, FBE_RAID_FRU_ERROR_TYPE_RETRYABLE);
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                                "retryable err qual: 0x%x "
                                                "pos: %d op: 0x%x lba: 0x%llx bl:0x%llx fruts:0x%x\n",
                                                block_qualifier,
                                                 current_fruts_ptr->position, current_fruts_ptr->opcode, current_fruts_ptr->lba, 
                                                current_fruts_ptr->blocks, fbe_raid_memory_pointer_to_u32(current_fruts_ptr));
                }
                else if (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CRC_ERROR)
                {
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                                "io failed crc error err qual: 0x%x "
                                                "pos: %d op: 0x%x lba: 0x%llx bl:0x%llx fruts:0x%x\n",
                                                block_qualifier,
                                                current_fruts_ptr->position, current_fruts_ptr->opcode, current_fruts_ptr->lba, 
                                                current_fruts_ptr->blocks, fbe_raid_memory_pointer_to_u32(current_fruts_ptr));
                    eboard_p->bad_crc_count++;
                }
                else if (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_PREFERRED)
                {
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                                "io failed not pref err qual: 0x%x "
                                                "pos: %d op: 0x%x lba: 0x%llx bl:0x%llx fruts:0x%x\n",
                                                block_qualifier,
                                                current_fruts_ptr->position, current_fruts_ptr->opcode, current_fruts_ptr->lba, 
                                                current_fruts_ptr->blocks, fbe_raid_memory_pointer_to_u32(current_fruts_ptr));
                    eboard_p->not_preferred_count++;
                }
                else if (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONGESTED)
                {
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_DEBUG_HIGH,
                                                "io failed congested err qual: 0x%x "
                                                "pos: %d op: 0x%x lba: 0x%llx bl:0x%llx fruts:0x%x\n",
                                                block_qualifier,
                                                current_fruts_ptr->position, current_fruts_ptr->opcode, current_fruts_ptr->lba, 
                                                current_fruts_ptr->blocks, fbe_raid_memory_pointer_to_u32(current_fruts_ptr));
                    eboard_p->reduce_qdepth_count++;
                    /* display this at a verbose level since we might see it often. */
                    break;
                }
                else
                {
                    /* Unknown qualifier encountered
                     */
                    b_status = FBE_FALSE;
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                                "%s line: %d Unknown io failure error: 0x%x qualifier: 0x%x\n", 
                                           __FUNCTION__, __LINE__, block_status, block_qualifier);
                }
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                            "fruts: 0x%x packet: 0x%x opcode:0x%x pos:%d dead_b:0x%x\n",
                                            fbe_raid_memory_pointer_to_u32(current_fruts_ptr), 
                                            fbe_raid_memory_pointer_to_u32(&current_fruts_ptr->io_packet), current_fruts_ptr->opcode, 
                                             current_fruts_ptr->position, eboard_p->dead_err_bitmap);
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                            "fruts: 0x%x alg:%d lba:0x%llx bl:0x%llx dead_pos:%d\n", 
                                            fbe_raid_memory_pointer_to_u32(current_fruts_ptr), 
                                            siots_p->algorithm,
					    (unsigned long long)siots_p->start_lba,
					    (unsigned long long)siots_p->xfer_count,
                                            siots_p->dead_pos);
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:
                if (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST)
                {
                    /* Standard hard media error.
                     */
                    eboard_p->hard_media_err_bitmap |= current_fru_bitmask;
                    eboard_p->hard_media_err_count++;
                    
                    status = FBE_RAID_FRU_ERROR_STATUS_ERROR;
                }
                else if (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_NO_REMAP)
                {
                    /* Add the current error to the MENR bitmap.
                     * Media error no remap bitmap contains every MENR position.
                     */
                    eboard_p->menr_err_bitmap |= current_fru_bitmask;
                    eboard_p->menr_err_count++;

                    eboard_p->hard_media_err_bitmap |= current_fru_bitmask;
                    eboard_p->hard_media_err_count++;
                    
                    status = FBE_RAID_FRU_ERROR_STATUS_ERROR;
                }
                else
                {
                    /* Unknown error encountered.
                     */
                    b_status = FBE_FALSE;
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                                "Unknown fruts media error: 0x%x qualifier: 0x%x fruts: 0x%x\n", 
                                                 block_status, block_qualifier, (fbe_u32_t)current_fruts_ptr);
                }
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED:
                if (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_OPTIONAL_ABORTED_LEGACY)
                {
                    eboard_p->drop_err_bitmap |= current_fru_bitmask;
                    eboard_p->drop_err_count++;
                    
                    status = FBE_RAID_FRU_ERROR_STATUS_ERROR;
                }
                else if ((block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED) ||
                         (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WR_NOT_STARTED_CLIENT_ABORTED))
                {
                    /* The request was aborted.
                     */
                    status = FBE_RAID_FRU_ERROR_STATUS_ABORTED;
                    eboard_p->abort_err_count++;
                }
                else
                {
                    /* Unknown error encountered.
                     */
                    b_status = FBE_FALSE;
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                                "Unknown fruts abort qual bl_st: 0x%x qual: 0x%x fruts: 0x%x\n", 
                                                 block_status, block_qualifier, (fbe_u32_t)current_fruts_ptr);
                }
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS:
                if (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED)
                {
                    /* This was a soft media error.
                     */
                    eboard_p->soft_media_err_count++;
                }
                else if (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ZEROED)
                {
                    /* This was a soft media error.
                     */
                    eboard_p->zeroed_count++;
                    eboard_p->zeroed_bitmap |= current_fru_bitmask;
                }
                else if (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_STILL_CONGESTED)
                {
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                                "success congested err qual: 0x%x "
                                                "pos: %d op: 0x%x lba: 0x%llx bl:0x%llx fruts:0x%x\n",
                                                block_qualifier,
                                                current_fruts_ptr->position, current_fruts_ptr->opcode, current_fruts_ptr->lba, 
                                                current_fruts_ptr->blocks, fbe_raid_memory_pointer_to_u32(current_fruts_ptr));
                    eboard_p->reduce_qdepth_soft_count++;
                }
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT:
                /* The packet expired while it was outstanding.
                 */
                eboard_p->timeout_err_count++;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST:
            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID:
            default:
                eboard_p->unexpected_err_count++;
                /* Unknown error encountered. We will print critical error 
                 * instead of panicking.
                 */
                b_status = FBE_FALSE;
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                            "Unexpected fruts block status bl_st: 0x%x qualifier: 0x%x fruts: 0x%x\n", 
                                             block_status, block_qualifier, (fbe_u32_t)current_fruts_ptr);
                break;
        }
    }

    return b_status;
}
/******************************************
 * end fbe_raid_fruts_chain_get_eboard()
 ******************************************/

/*!**************************************************************
 * fbe_raid_fruts_destroy()
 ****************************************************************
 * @brief
 *  Destroy the fruts and the enclosed packet.
 *
 * @param  fruts_p - the Fruts to destroy.               
 *
 * @return fbe_status_t.   
 *
 * @author
 *  8/7/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_destroy(fbe_raid_fruts_t *fruts_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    /* If the packet was initted, then destroy the packet.
     */
    if (fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        status = fbe_transport_destroy_packet(fbe_raid_fruts_get_packet(fruts_p));
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        { 
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s: fbe_transport_destroy_packet for packet: %p failed with status 0x%x\n", 
                                   __FUNCTION__,
                                   fbe_raid_fruts_get_packet(fruts_p),
                                   status);
            return status; 
        }
        fbe_raid_fruts_clear_flag(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_fruts_destroy()
 ******************************************/
/*!***************************************************************
 *          fbe_raid_fruts_packet_completion()
 ****************************************************************
 * 
 * @brief   This is a test completion function for raid request that
 *          is sent to the next level (either raid or the virtual drive).
 *          It populate the status as desired (from globals) and then
 *          invokes the raid group state machine. 
 * 
 * @param packet_p - The packet being completed.
 * @param context - The fruts ptr.
 *
 * @return fbe_status_t FBE_STATUS_MORE_PROCESSING_REQUIRED
 *                      Since this is the last completion function
 *                      in this packet we need to return this status.
 *
 * @author
 *  05/21/2009  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_raid_fruts_packet_completion(fbe_packet_t * packet_p, 
                                              fbe_packet_completion_context_t context)
{
    fbe_raid_fruts_t   *fruts_p = NULL;
    fbe_raid_siots_t   *siots_p = NULL;
	fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_status_t packet_status;

    /* First get and validate the fruts associated with this packet.
     * Although we could use the packet itself we use the completion context.
     */
    fruts_p = (fbe_raid_fruts_t *)context;
    if(RAID_DBG_ENA(fruts_p == NULL))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: %s errored: fruts_p is NULL.\n",
                               __FUNCTION__);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    if(RAID_DBG_ENA(!(fbe_raid_common_get_flags(&fruts_p->common) & FBE_RAID_COMMON_FLAG_TYPE_FRU_TS)))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: %s errored: fruts_p flags 0x%p is wrong or evaluate state NULL\n",
                               __FUNCTION__, 
                               &fruts_p->common.flags);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    siots_p = fbe_raid_fruts_get_siots(fruts_p);
    if (RAID_DBG_ENABLED)
    {
        fbe_u32_t width;
        fbe_raid_siots_t *siots_p = NULL;
        fbe_raid_geometry_t *raid_geometry_p = NULL;
        /* Now get the raid group associated with the request.
         */
        siots_p = fbe_raid_fruts_get_siots(fruts_p);

        if(RAID_FRUTS_COND(siots_p == NULL))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s errored: SIOTS is NULL.\n",
                                   __FUNCTION__);

            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        if (RAID_FRUTS_COND((siots_p->common.state == (fbe_raid_common_state_t)fbe_raid_siots_freed) ||
                            (siots_p->wait_count == 0) ) )
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s errored: SIOTS common state 0x%p is wrong or wait count 0x%llx == 0\n",
                                   __FUNCTION__, 
                                   siots_p->common.state, 
                                   siots_p->wait_count);

            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        if (RAID_FRUTS_COND(raid_geometry_p == NULL) )
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "raid: %s failed to get raid geometry: raid_geometry_p is null : siots_p = 0x%p\n",
                                        __FUNCTION__, siots_p);

            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        /* Make sure we still have something outstanding.
         * We do a lower and upper range check.  The upper range check is fairly arbitrary, 
         * but it is unlikely that we will have more than twice the nubmer of drives worth of 
         * operations outstanding at this time. 
         */
        fbe_raid_geometry_get_width(raid_geometry_p, &width); 
        if (siots_p->wait_count > (width * 2))
        {
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                 "%s siots_p %p has invalid wait count: %d\n", __FUNCTION__,
                                 siots_p, (int)siots_p->wait_count);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    /* We should get the outstanding flag set in fruts.
     * We will clear it here
     */
    if (!fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_REQUEST_OUTSTANDING))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid: got unexpected error: fruts_p 0x%p flag is"
                             "not set correctly, siots_p = 0x%p\n",
                             fruts_p, siots_p);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_raid_fruts_clear_flag(fruts_p, FBE_RAID_FRUTS_FLAG_REQUEST_OUTSTANDING);

    /* Make sure we did not already recive status for this request.
     */
    if (RAID_DBG_ENA(fruts_p->status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "%s fruts: %p status: 0x%x is unexpected position: 0x%x \n", 
                             __FUNCTION__, fruts_p, fruts_p->status, fruts_p->position);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Now get the payload.*/
    payload_p = fbe_transport_get_payload_ex(packet_p);
    if (payload_p == NULL) {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS, "%s fruts %p packet %p Can't get payload\n", __FUNCTION__, fruts_p, packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Copy the packet status to the fruts. */
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    if (block_operation_p == NULL) {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "%s fruts %p packet %p has a null block operation\n",
                             __FUNCTION__, fruts_p, packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_payload_block_get_status(block_operation_p, &fruts_p->status);
    fbe_payload_block_get_qualifier(block_operation_p, &fruts_p->status_qualifier);
    packet_status = fbe_transport_get_status_code(&fruts_p->io_packet);

    if ((fruts_p->status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) ||
        (fruts_p->status_qualifier != FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE) || 
        (packet_status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_ERROR_PENDING);
        fbe_payload_ex_get_media_error_lba(payload_p, &fruts_p->media_error_lba); 
    }
    fbe_payload_ex_get_pvd_object_id(payload_p, &fruts_p->pvd_object_id);

    /* Display information on this fruts.
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_LOGGING_ENABLED))      
    {
        raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING))
        {
            fbe_raid_fruts_trace(fruts_p, FBE_FALSE /* not starting */);
        }
        /*
         * trace to RBA buffer
         */
        if (fbe_traffic_trace_is_enabled (KTRC_TFBE_RG_FRU))
        {
            fbe_raid_group_number_t rg_number = FBE_RAID_GROUP_INVALID;
            fbe_object_id_t rg_object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
            fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
            fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);

            if((rg_object_id != FBE_OBJECT_ID_INVALID) && (rg_object_id != FBE_U32_MAX))
            {
                fbe_database_get_rg_number(rg_object_id, &rg_number);
            }

            fbe_traffic_trace_block_operation(FBE_CLASS_ID_RAID_GROUP,
                                          block_operation_p,
                                          rg_number,
                                          fruts_p->position,/*extra information*/
                                          FBE_FALSE, /*RBA trace end*/
                                              fbe_traffic_trace_get_priority(packet_p));
        }
    }

    if (RAID_DBG_ENA(siots_p->wait_count == 0))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "%s siots_p %p has invalid wait count: %lld\n", __FUNCTION__,
                             siots_p, (long long)siots_p->wait_count);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Now invoke complete function
     */
    if (fbe_atomic_decrement(&siots_p->wait_count) == 0)
    {
        /* We can complete immediately if immediate completion is enabled and there were no errors.
         */
        if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_COMPLETE_IMMEDIATE) &&
            !fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_ERROR_PENDING))
        {
            fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
            fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
            fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

            fbe_raid_siots_check_traffic_trace(siots_p, FBE_FALSE);
            /* When this flag is set we will complete the iots immediately.
             */  
            //fbe_raid_iots_mark_complete(iots_p);
            //fbe_raid_common_state((fbe_raid_common_t *)iots_p);
            fbe_raid_iots_complete_iots(iots_p);
        }
        else
        {
            /* We are done with all requests simply enqueue the request to 
             * the raid group state queue.
             */
            fbe_raid_common_state((fbe_raid_common_t *)siots_p);
        }
    }

    /* Return waiting which will fetch the next request (either the next
     * fruts or the siots).
     */
    return(FBE_STATUS_MORE_PROCESSING_REQUIRED);
}
/* end fbe_raid_fruts_packet_completion() */

/****************************************************************
 *  fbe_raid_fruts_get_bitmap_for_status()
 ****************************************************************
 *  @brief
 *   Get a bitmask of hard errors encountered
 *
 *  @param fruts_p - head of a list of fruts.
 *  @param status - status to check for.
 *  @param qualifier - qualifier to check for.
 *                     Set to FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID
 *                     and we will not check the status.
 * 
 *  @param output_err_bitmap_p - bitmap of error positions.
 *  @param output_err_count_p - Count of errored positions.
 *
 *  @return None.
 * 
 *  @author
 *   2/22/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_fruts_get_bitmap_for_status(fbe_raid_fruts_t * fruts_p,
                                          fbe_raid_position_bitmask_t * output_err_bitmap_p,
                                          fbe_u16_t * output_err_count_p,
                                          fbe_payload_block_operation_status_t status,
                                          fbe_payload_block_operation_qualifier_t qualifier)
{
    fbe_raid_fruts_t *current_fruts_ptr;
    fbe_u16_t err_bitmap = 0;
    fbe_u16_t err_count = 0;

    for (current_fruts_ptr = fruts_p;
         current_fruts_ptr != NULL;
         current_fruts_ptr = fbe_raid_fruts_get_next(current_fruts_ptr))
    {
        /* We check the status and if the qualifier is not invalid, we check it.
         */
        if ((current_fruts_ptr->status == status) &&
            ((qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID) ||
             (current_fruts_ptr->status_qualifier == qualifier)))
        {
            err_bitmap |= (1 << current_fruts_ptr->position);
            err_count++;
        }
    }

    if (output_err_bitmap_p != NULL)
    {
        *output_err_bitmap_p = err_bitmap;
    }
    if (output_err_count_p != NULL)
    {
        *output_err_count_p += err_count;
    }
    return;
}
/***********************
 * fbe_raid_fruts_get_bitmap_for_status()
 ***********************/

/*!***************************************************************
 *  fbe_raid_fruts_get_media_err_bitmap()
 ****************************************************************
 *  @brief
 *   Get a bitmask of hard errors encountered
 *
 *   fruts_p:           [IO] - head of a list of fruts.
 *   hard_media_err_bitmap_p: [O] - bitmap of hard error positions.
 *
 *  @return
 *   fbe_u16_t - count of hard errors encountered
 *
 *  @author
 *   2/10/00 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_position_bitmask_t fbe_raid_fruts_get_media_err_bitmap(fbe_raid_fruts_t * fruts_p,
                                              fbe_u16_t * input_hard_media_err_bitmap_p)
{
    fbe_u16_t hard_media_err_count = 0;

    /* In the past we just looked for HARD_ERROR.
     * However, we now just look for MEDIA_ERROR,
     * as the DH should never give us a HARD_ERROR.
     */
    fbe_raid_fruts_get_bitmap_for_status(fruts_p, input_hard_media_err_bitmap_p,
                                         &hard_media_err_count,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);
    return hard_media_err_count;
}
/***********************
 * fbe_raid_fruts_get_media_err_bitmap
 ***********************/

/*!***************************************************************
 *  fbe_raid_fruts_get_retry_err_bitmap()
 ****************************************************************
 * @brief
 *  Get a bitmask of retryable errors encountered.
 *
 * @param fruts_p - head of a list of fruts.
 * @param retryable_err_bitmap_p - bitmap of retryable error positions.
 *
 * @return
 *  fbe_status_t FBE_STATUS_OK or FBE_STATUS_GENERIC_FAILURE
 *
 * @author
 *  4/26/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_get_retry_err_bitmap(fbe_raid_fruts_t * fruts_p,
                                                 fbe_raid_position_bitmask_t * input_retryable_err_bitmap_p)
{
    fbe_u16_t retryable_err_count = 0;

    /* Fetch bitmap of errored positions.
     */
    fbe_raid_fruts_get_bitmap_for_status(fruts_p, input_retryable_err_bitmap_p,
                                         &retryable_err_count,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
    return FBE_STATUS_OK;
}
/***********************
 * fbe_raid_fruts_get_retry_err_bitmap
 ***********************/

/*!***************************************************************
 *  fbe_raid_fruts_get_retry_timeout_err_bitmap()
 ****************************************************************
 * @brief
 *  Get a bitmask of retryable errors that have simply taken too long.
 *
 * @param fruts_p - head of a list of fruts.
 * @param retry_err_bitmap_p - bitmap of retryable error positions.
 * 
 * @return
 *  fbe_status_t FBE_STATUS_OK
 *
 * @author
 *  2/5/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_get_retry_timeout_err_bitmap(fbe_raid_fruts_t * fruts_p,
                                                         fbe_raid_position_bitmask_t * retry_err_bitmap_p)
{
    /* Disable timeout error handling since PDO will now be responsible for this.
     */
#if 0
    fbe_u16_t retryable_err_count = 0;

    fbe_raid_fruts_t *current_fruts_p = NULL;
    fbe_u16_t err_bitmap = 0;

    for (current_fruts_p = fruts_p;
         current_fruts_p != NULL;
         current_fruts_p = fbe_raid_fruts_get_next(current_fruts_p))
    {
        /* If the packet is failed with a qualifier of retryable,
         * and the packet is now expired. 
         * Then we consider this a timed out retryable error. 
         */
        if ((current_fruts_p->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED) &&
            (current_fruts_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE) &&
            fbe_raid_fruts_is_flag_set(current_fruts_p, FBE_RAID_FRUTS_FLAG_RETRY_PHYSICAL_ERROR) &&
            fbe_transport_is_packet_expired(&current_fruts_p->io_packet))
        {
            err_bitmap |= (1 << current_fruts_p->position);
        }
    }
    if (retry_err_bitmap_p != NULL)
    {
        *retry_err_bitmap_p = err_bitmap;
    }
#endif
    return FBE_STATUS_OK;
}
/***********************
 * fbe_raid_fruts_get_retry_timeout_err_bitmap
 ***********************/

/*!***************************************************************
 *  fbe_raid_fruts_update_media_err_bitmap()
 ****************************************************************
 *  @brief
 *   Update the specified bitmask of hard errors encountered
 *
 * @param   fruts_p - head of a list of fruts.
 *
 * @return  None
 *
 *  @author
 *  04/15/2010  Ron Proulx  - Created
 *
 ****************************************************************/

void fbe_raid_fruts_update_media_err_bitmap(fbe_raid_fruts_t *fruts_p)
{
    fbe_raid_siots_t   *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_xor_error_t    *eboard_p = fbe_raid_siots_get_eboard(siots_p);
    fbe_u16_t           new_hard_media_err_count = 0;
    fbe_u16_t           new_hard_media_error_bitmap = 0;


    /* In the past we just looked for HARD_ERROR.
     * However, we now just look for MEDIA_ERROR,
     * as the DH should never give us a HARD_ERROR.
     */
    fbe_raid_fruts_get_bitmap_for_status(fruts_p, 
                                         &new_hard_media_error_bitmap,
                                         &new_hard_media_err_count,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);

    /* If required, initialize the eboard before updating the hard media error
     * bitmap.
     */
    fbe_xor_lib_eboard_init(eboard_p);
    eboard_p->hard_media_err_bitmap |= new_hard_media_error_bitmap;
    return;
}
/*****************************************
 * fbe_raid_fruts_update_media_err_bitmap
 *****************************************/

/****************************************************************
 *  fbe_raid_fruts_chain_nop_find()
 ****************************************************************
 *  @brief
 *   Get a bitmask of FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALIDs in chain.
 *
 * @param fruts_p - Head of a list of fruts.
 * @param nop_bitmap_p - Bitmap of hard error positions.
 *                       This parameter is NULL if the user
 *                       does not want to get a bitmap of errs.
 *
 *  @return
 *   fbe_u16_t - bitmask of errors encountered.
 *
 *  @author
 *   10/27/05 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u16_t fbe_raid_fruts_chain_nop_find(fbe_raid_fruts_t * fruts_p,
                                        fbe_u16_t * input_nop_bitmap_p)
{
    fbe_raid_fruts_t * current_fruts_ptr;
    fbe_u16_t nop_bitmap = 0;
    fbe_u16_t nop_count = 0;

    /* Loop over the chain, looking for and keeping track of each
     * NOP.
     */
    for (current_fruts_ptr = fruts_p;
         current_fruts_ptr != NULL;
         current_fruts_ptr = fbe_raid_fruts_get_next(current_fruts_ptr))
    {
        /* Look for a NOP.
         */
        if (current_fruts_ptr->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID )
        {
            /* Found a NOP.  Note it in our bitmap and count.
             */
            nop_bitmap |= (1 << current_fruts_ptr->position);
            nop_count++;
        }
    }

    if (input_nop_bitmap_p != NULL)
    {
        *input_nop_bitmap_p = nop_bitmap;
    }
    return nop_count;
}
/***********************
 * end fbe_raid_fruts_chain_nop_find()
 ***********************/
/****************************************************************
 * fbe_raid_fruts_for_each()
 ****************************************************************
 * @brief
 *  Perform an action specified by a function parameter
 *  for each indicated fruts position in the fruts chain.
 *
 * @param fru_bitmap - the fruts positions to call fn for
 * @param fruts_p - the chain of fruts to traverse
 * @param fn - the function to call for each fruts
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  02/09/00 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_for_each(fbe_raid_position_bitmask_t fru_bitmap,
                                     fbe_raid_fruts_t * fruts_p,
                                     fbe_status_t fn(fbe_raid_fruts_t *, fbe_lba_t, fbe_u32_t *),
                                     fbe_u64_t arg1,
                                     fbe_u64_t arg2)
{
    fbe_raid_fruts_t *cur_fruts_p = NULL;
    fbe_raid_fruts_t *next_fruts_p = NULL;
    fbe_status_t status;

    /* 0 means to iterate over all items in the chain. 
     * Init the bitmap to all FFs. 
     */
    if (fru_bitmap == 0)
    {
        fru_bitmap = FBE_RAID_INVALID_DEAD_POS;
    }

    /* For each fruts in the bitmap,start this fruts.
     */
    cur_fruts_p = fruts_p;
    while ((cur_fruts_p != NULL) && (fru_bitmap != 0))
    {
        next_fruts_p = fbe_raid_fruts_get_next(cur_fruts_p);
        if ((1 << cur_fruts_p->position) & fru_bitmap)
        {
            /* Found a fruts with a position indicated
             * by the bitmap.
             * Invoke fn on this fruts.
             */
            fru_bitmap &= ~(1 << cur_fruts_p->position);
            status = fn(cur_fruts_p, (fbe_lba_t)arg1, (fbe_u32_t *)(fbe_ptrhld_t)arg2);
            if (status != FBE_STATUS_OK) { return status; }
        }
        else
        {
            cur_fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
        }
        cur_fruts_p = next_fruts_p;
    }
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_fruts_for_each()
 *******************************/

/****************************************************************
 * fbe_raid_fruts_for_each_bit()
 ****************************************************************
 * @brief
 *  Perform an action on a fruts specified by its position in a
 *  bitmap.
 * 
 *  The difference between this function and fbe_raid_fruts_for_each()
 *  is that this function does not not modify any fruts not specified
 *  by the bitmap.
 *
 * @param fru_bitmap - the fruts positions to call fn for
 * @param fruts_p - the chain of fruts to traverse
 * @param fn - the function to call for each fruts
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  03/08/11 - Created. Daniel Cummins 
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_for_each_bit(fbe_raid_position_bitmask_t fru_bitmap,
										 fbe_raid_fruts_t * fruts_p,
										 fbe_status_t fn(fbe_raid_fruts_t *, fbe_lba_t, fbe_u32_t *),
										 fbe_u64_t arg1,
										 fbe_u64_t arg2)
{
	fbe_raid_fruts_t *cur_fruts_p = NULL;
	fbe_raid_fruts_t *next_fruts_p = NULL;
	fbe_status_t status;

	/* 0 means to iterate over all items in the chain. 
	 * Init the bitmap to all FFs. 
	 */
	if (fru_bitmap == 0)
	{
		fru_bitmap = FBE_RAID_INVALID_DEAD_POS;
	}

	/* For each fruts in the bitmap,start this fruts.
	 */
	cur_fruts_p = fruts_p;
	while ((cur_fruts_p != NULL) && (fru_bitmap != 0))
	{
		next_fruts_p = fbe_raid_fruts_get_next(cur_fruts_p);
		if ((1 << cur_fruts_p->position) & fru_bitmap)
		{
			/* Found a fruts with a position indicated
			 * by the bitmap.
			 * Invoke fn on this fruts.
			 */
			fru_bitmap &= ~(1 << cur_fruts_p->position);
			status = fn(cur_fruts_p, (fbe_lba_t)arg1, (fbe_u32_t *)(fbe_ptrhld_t)arg2);
			if (status != FBE_STATUS_OK)
			{
				return status;
			}
		}

		cur_fruts_p = next_fruts_p;
	}
	return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_fruts_for_each_bit()
 *******************************/


/****************************************************************
 *  fbe_raid_fruts_reset_rd()
 ****************************************************************
 *  @brief
 *    Set this fruts into single sector mode.
 *
 * @param fruts_p - the single fruts to act on
 * @param lba_inc - amount to increment lba by
 * @param blocks - blocks to set into fruts
 *
 *  @return
 *    none
 *
 *  @author
 *    02/09/00 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_reset_rd(fbe_raid_fruts_t * fruts_p,
                             fbe_lba_t lba_inc,
                             fbe_u32_t * blocks)
{ 
    fbe_u64_t blocks64 = (fbe_u64_t)(fbe_ptrhld_t)blocks;
    fruts_p->common.flags &= ~(FBE_RAID_COMMON_FLAG_REQ_STARTED | FBE_RAID_COMMON_FLAG_REQ_RETRIED);
    fruts_p->opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ;

    fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fruts_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fruts_p->media_error_lba = FBE_RAID_INVALID_LBA;
    /*Do not set pdo object id as invalid, in case earlier io is successful 
     * since we may want to send CRC usurper later*/
    if(fbe_transport_get_status_code(&fruts_p->io_packet) != FBE_STATUS_OK)
    {
       /* set pdo object id as invalid */
        fruts_p->pvd_object_id = FBE_OBJECT_ID_INVALID;
    }

    /* We preserve the flag regarding packet initialization.
     */
    if (fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED);
    }
    else
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_INVALID);
    }
    fruts_p->lba += lba_inc;
    fruts_p->blocks = (fbe_u32_t)blocks64;
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_fruts_reset_rd()
 *******************************/

/****************************************************************
 *  fbe_raid_fruts_reset_wr()
 ****************************************************************
 *  @brief
 *    Set this fruts into single sector mode.
 *
 * @param fruts_p - the single fruts to act on
 * @param lba_inc - amount to increment lba by
 * @param blocks - blocks to set into fruts
 *
 *  @return
 *    none
 *
 *  @author
 *    02/09/00 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_reset_wr(fbe_raid_fruts_t * fruts_p,
                             fbe_lba_t lba_inc,
                             fbe_u32_t * blocks)
{
    fbe_u64_t blocks64 = (fbe_u64_t)(fbe_ptrhld_t)blocks;
    fruts_p->common.flags &= ~(FBE_RAID_COMMON_FLAG_REQ_STARTED | FBE_RAID_COMMON_FLAG_REQ_RETRIED);
    fruts_p->opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;

    fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fruts_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fruts_p->media_error_lba = FBE_RAID_INVALID_LBA;
    fruts_p->pvd_object_id = FBE_OBJECT_ID_INVALID;

    /* We preserve the flag regarding packet initialization.
     */
    if (fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED);
    }
    else
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_INVALID);
    }
    fruts_p->lba += lba_inc;
    fruts_p->blocks = (fbe_u32_t)blocks64;
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_fruts_reset_wr()
 *******************************/

/*!***************************************************************
 * fbe_raid_fruts_reset_vr()
 ****************************************************************
 *
 * @brief   Set this fruts to block mode verify
 *
 * @param   fruts_p,    [I] - the single fruts to act on
 * @param   lba_inc,    [I] - amount to increment lba by
 * @param   blocks,     [I] - blocks to set into fruts
 *
 * @return  None
 *
 * @author
 *  12/09/2008  Ron Proulx  - Created
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_reset_vr(fbe_raid_fruts_t * fruts_p,
                             fbe_lba_t lba_inc,
                             fbe_u32_t * blocks)
{
    fbe_u64_t blocks64 = (fbe_u64_t)(fbe_ptrhld_t)blocks;

    fruts_p->common.flags &= ~(FBE_RAID_COMMON_FLAG_REQ_STARTED | FBE_RAID_COMMON_FLAG_REQ_RETRIED);
    fruts_p->opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ;

    fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fruts_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fruts_p->media_error_lba = FBE_RAID_INVALID_LBA;
    fruts_p->pvd_object_id = FBE_OBJECT_ID_INVALID;

    /* We preserve the flag regarding packet initialization.
     */
    if (fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED);
    }
    else
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_INVALID);
    }
    fruts_p->lba += lba_inc;
    fruts_p->blocks = (fbe_u32_t)blocks64;
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_fruts_reset_vr()
 *******************************/

/****************************************************************
 *  fbe_raid_fruts_reset()
 ****************************************************************
 *  @brief
 *    Set this fruts into single sector mode.
 *
 * @param fruts_p - the single fruts to act on
 * @param blocks - blocks to set into fruts
 * @param opcode - opcode to set into fruts
 *
 *  @return
 *    none
 *
 *  @author
 *    02/09/00 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_reset(fbe_raid_fruts_t * fruts_p,
                          fbe_lba_t blocks,
                          fbe_u32_t * opcode)
{
    fbe_u64_t opcode64 = (fbe_u64_t)(fbe_ptrhld_t)opcode;
    fruts_p->common.flags &= ~(FBE_RAID_COMMON_FLAG_REQ_STARTED | FBE_RAID_COMMON_FLAG_REQ_RETRIED);

    fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fruts_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fruts_p->media_error_lba = FBE_RAID_INVALID_LBA;
    fruts_p->pvd_object_id = FBE_OBJECT_ID_INVALID;

    /* We preserve the flag regarding packet initialization.
     */
    if (fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED);
    }
    else
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_INVALID);
    }
    fruts_p->blocks = (fbe_u32_t) blocks;
    fruts_p->opcode = (fbe_u32_t)opcode64;
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_fruts_reset()
 *******************************/

/****************************************************************
 *  fbe_raid_fruts_clear_flags()
 ****************************************************************
 *  @brief
 *    Set this fruts into single sector mode.
 *
 * @param fruts_p - the single fruts to act on
 * @param blocks - blocks to set into fruts
 * @param opcode - opcode to set into fruts
 *
 *  @return
 *    none
 *
 *  @author
 *    02/09/00 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_clear_flags(fbe_raid_fruts_t * fruts_p,
                                fbe_lba_t unused_1,
                                fbe_u32_t * unused_2)
{
    FBE_UNREFERENCED_PARAMETER(unused_1);
    FBE_UNREFERENCED_PARAMETER(unused_2);

    fruts_p->common.flags &= ~(FBE_RAID_COMMON_FLAG_REQ_STARTED | FBE_RAID_COMMON_FLAG_REQ_RETRIED);

    fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fruts_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fruts_p->media_error_lba = FBE_RAID_INVALID_LBA;
    fruts_p->pvd_object_id = FBE_OBJECT_ID_INVALID;

    /* We preserve the flag regarding packet initialization.
     */
    if (fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED);
    }
    else
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_INVALID);
    }
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_fruts_clear_flags()
 *******************************/

/****************************************************************
 *  fbe_raid_fruts_set_opcode()
 ****************************************************************
 *  @brief
 *    Set this fruts into single sector mode.
 *
 * @param fruts_p - the single fruts to act on
 * @param opcode - opcode to set into fruts
 * @param unused_2 - Not used.
 *
 * @return fbe_status_t
 *
 * @author
 *  02/09/00 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_set_opcode(fbe_raid_fruts_t * fruts_p,
                                       fbe_lba_t opcode,
                                       fbe_u32_t * unused_2)
{
    FBE_UNREFERENCED_PARAMETER(unused_2);

    fruts_p->common.flags &= ~(FBE_RAID_COMMON_FLAG_REQ_STARTED | FBE_RAID_COMMON_FLAG_REQ_RETRIED);

    fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fruts_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fruts_p->media_error_lba = FBE_RAID_INVALID_LBA;
    fruts_p->pvd_object_id = FBE_OBJECT_ID_INVALID;

    /* We preserve the flag regarding packet initialization.
     */
    if (fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED);
    }
    else
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_INVALID);
    }
    fruts_p->opcode = (fbe_u32_t) opcode;
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_fruts_set_opcode()
 *******************************/
/****************************************************************
 *  fbe_raid_fruts_set_opcode_crc_check()
 ****************************************************************
 *  @brief
 *    Set this fruts into single sector mode.
 *    pdo object id is not set to invalid in case crc usurper to send
 *
 * @param fruts_p - the single fruts to act on
 * @param opcode - opcode to set into fruts
 * @param unused_2 - Not used.
 *
 * @return fbe_status_t
 *
 * @author
 *  1/27/2012 - Created. Swati Fursule from fbe_raid_fruts_set_opcode
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_set_opcode_crc_check(fbe_raid_fruts_t * fruts_p,
                                       fbe_lba_t opcode,
                                       fbe_u32_t * unused_2)
{
    FBE_UNREFERENCED_PARAMETER(unused_2);

    fruts_p->common.flags &= ~(FBE_RAID_COMMON_FLAG_REQ_STARTED | FBE_RAID_COMMON_FLAG_REQ_RETRIED);

    fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fruts_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fruts_p->media_error_lba = FBE_RAID_INVALID_LBA;
    /*Do not set pdo object id as invalid, in case earlier io is successful 
     * since we may want to send CRC usurper later*/
    if(fbe_transport_get_status_code(&fruts_p->io_packet) != FBE_STATUS_OK)
    {
       /* set pdo object id as invalid */
        fruts_p->pvd_object_id = FBE_OBJECT_ID_INVALID;
    }

    /* We preserve the flag regarding packet initialization.
     */
    if (fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED);
    }
    else
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_INVALID);
    }
    fruts_p->opcode = (fbe_u32_t) opcode;
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_fruts_set_opcode_crc_check()
 *******************************/
/****************************************************************
 *  fbe_raid_fruts_set_opcode_success()
 ****************************************************************
 *  @brief
 *    Set this fruts into single sector mode.
 *
 * @param fruts_p - the single fruts to act on
 * @param unused_2 - This parameter will never be referenced
 * @param opcode - opcode to set into fruts
 *
 *  @return
 *    none
 *
 *  @author
 *    03/26/07 - Created. SR
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_set_opcode_success(fbe_raid_fruts_t * fruts_p,
                                       fbe_lba_t opcode,
                                       fbe_u32_t * unused_2)
{
    FBE_UNREFERENCED_PARAMETER(unused_2);

    fruts_p->common.flags &= ~(FBE_RAID_COMMON_FLAG_REQ_STARTED | FBE_RAID_COMMON_FLAG_REQ_RETRIED);
    fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
    fruts_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
    fruts_p->media_error_lba = FBE_RAID_INVALID_LBA;
    fruts_p->pvd_object_id = FBE_OBJECT_ID_INVALID;

    /* We preserve the flag regarding packet initialization.
     */
    if (fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED);
    }
    else
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_INVALID);
    }
    fruts_p->opcode = (fbe_u32_t) opcode;
    return FBE_STATUS_OK;
}

/*******************************
 * end fbe_raid_fruts_set_opcode_success()
 *******************************/
/****************************************************************
 *  fbe_raid_fruts_retry()
 ****************************************************************
 * @brief
 *  Retry the fruts by initting some fields and sending the
 *  fruts again.
 *
 * @param fruts_p - the single fruts to act on
 * @param opcode - opcode to set into fruts
 * @param eval_state - new eval state to set in fruts
 *
 * @return fbe_status_t FBE_STATUS_OK
 *
 * @author
 *  02/09/00 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_retry(fbe_raid_fruts_t * fruts_p,
                                  fbe_lba_t opcode,
                                  fbe_u32_t * evaluate_state)
{
    fbe_bool_t b_result = FBE_TRUE;

    fruts_p->common.flags &= ~FBE_RAID_COMMON_FLAG_REQ_STARTED;

    fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fruts_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fruts_p->media_error_lba = FBE_RAID_INVALID_LBA;
    fruts_p->pvd_object_id = FBE_OBJECT_ID_INVALID;

    /* We preserve the flag regarding packet initialization.
     */
    if (fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED);
    }
    else
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_INVALID);
    }

    fruts_p->opcode = (fbe_u32_t)opcode;

    /* Increment our retry count since we are about to retry this fruts.
     */
    fruts_p->retry_count++;

    b_result = fbe_raid_fruts_send(fruts_p, (void *) evaluate_state);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                               "raid: %s failed to start fruts_p fruts: 0x%p, op: %d\n", 
                               __FUNCTION__, 
                               fruts_p, fruts_p->opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_fruts_retry()
 *******************************/

/****************************************************************
 *  fbe_raid_fruts_retry_physical_error()
 ****************************************************************
 * @brief
 *  Retry the fruts by initting some fields and sending the
 *  fruts again.
 *
 * @param fruts_p - the single fruts to act on
 * @param opcode - opcode to set into fruts
 * @param eval_state - new eval state to set in fruts
 *
 * @return fbe_status_t FBE_STATUS_OK
 *
 * @author
 *  11/19/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_retry_physical_error(fbe_raid_fruts_t * fruts_p,
                                                 fbe_lba_t opcode,
                                                 fbe_u32_t * evaluate_state)
{
    fbe_bool_t b_result = FBE_TRUE;

    fruts_p->common.flags &= ~FBE_RAID_COMMON_FLAG_REQ_STARTED;

    fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fruts_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fruts_p->media_error_lba = FBE_RAID_INVALID_LBA;
    fruts_p->pvd_object_id = FBE_OBJECT_ID_INVALID;

    /* We preserve the flag regarding packet initialization.
     */
    if (fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED);
    }
    else
    {
        fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_INVALID);
    }
    /* This needs to be set so that when we send out the fruts, it does 
     * not reset the expiration time. 
     */
    fbe_raid_fruts_set_flag(fruts_p, FBE_RAID_FRUTS_FLAG_RETRY_PHYSICAL_ERROR);

    fruts_p->opcode = (fbe_u32_t)opcode;

    /* Increment our retry count since we are about to retry this fruts.
     */
    fruts_p->retry_count++;

    b_result = fbe_raid_fruts_send(fruts_p, (void *) evaluate_state);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                               "raid: %s failed to start fruts_p fruts: 0x%p, op: %d\n", 
                               __FUNCTION__, 
                               fruts_p, fruts_p->opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_fruts_retry_physical_error()
 *******************************/

/*!**************************************************************
 * fbe_raid_fruts_timer_completion()
 ****************************************************************
 * @brief
 *  This is the timer completion function that runs when
 *  the timer expires.
 *
 * @param timer_p - pointer to the timer.
 * @param timer_completion_context - fruts.
 *
 * @return none.
 *
 * @author
 *  3/30/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_fruts_timer_completion(fbe_packet_t * packet, 
                                                    fbe_packet_completion_context_t completion_context)
{
    fbe_raid_fruts_t *fruts_p = (fbe_raid_fruts_t*)completion_context;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_object_id_t object_id;
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                  "start retry %d service_ms: 0x%llx phys fruts: %p op: %d pos: %d\n",
                                  fruts_p->retry_count,
				  (unsigned long long)packet->physical_drive_service_time_ms, 
                                  fruts_p, fruts_p->opcode, fruts_p->position);
    
    fbe_raid_fruts_retry_physical_error(fruts_p, fruts_p->opcode, (fbe_u32_t*)fbe_raid_fruts_evaluate);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_fruts_timer_completion()
 ******************************************/
/****************************************************************
 * fbe_raid_fruts_delayed_retry()
 ****************************************************************
 * @brief
 *  Retry the given fruts after a delay that was specified in
 *  the error returned from the lower level.
 *
 * @param fruts_p - the single fruts to act on
 * @param msecs - milliseconds after which to retry.
 * @param evaluate_state.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/30/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_delayed_retry(fbe_raid_fruts_t * fruts_p,
                                          fbe_time_t milliseconds)
{
    fbe_status_t status;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_object_id_t object_id;

    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                  FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                  "delayed retry in %llu msecs for fruts: %p op: %d retries: %d\n",
                                  (unsigned long long)milliseconds, fruts_p,
				  fruts_p->opcode, siots_p->retry_count);

    /* We received a retryable error.  Add on the wait seconds to the total PDO service time.
     */
    fbe_transport_increment_physical_drive_service_time(fbe_raid_fruts_get_packet(fruts_p), milliseconds);

    status = fbe_transport_set_timer(fbe_raid_fruts_get_packet(fruts_p),
                                     (fbe_u32_t) milliseconds,
                                     fbe_raid_fruts_timer_completion,
                                     fruts_p);
    if (RAID_FRUTS_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: %s faield to set timer: status = 0x%x, fruts_p = 0x%p\n",
                             __FUNCTION__, status, fruts_p);
        return status; 
    }
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_fruts_delayed_retry()
 *******************************/

/****************************************************************
 *  fbe_raid_fruts_validate_sgs()
 ****************************************************************
 *  @brief
 *    Validate the sanity of a fruts sgs.
 *
 * @param fruts_p - the single fruts to act on
 * @param unused_1 - 
 * @param unused_2 - 
 *
 * @return
 *    none
 *
 *  @author
 *    04/15/00 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_fruts_validate_sgs(fbe_raid_fruts_t * fruts_p,
                                         fbe_lba_t unused_1,
                                         fbe_u32_t * unused_2)
{
    FBE_UNREFERENCED_PARAMETER(unused_1);
    FBE_UNREFERENCED_PARAMETER(unused_2);

    if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)fruts_p->sg_id))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid header invalid for fruts 0x%p, sg_id: 0x%p\n",
                             fruts_p, fruts_p->sg_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (!fbe_raid_fruts_validate_sg(fruts_p))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid sg invalid for fruts 0x%p, sg_id: 0x%p\n",
                             fruts_p, fruts_p->sg_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/***************************
 * fbe_raid_fruts_validate_sgs()
 ***************************/

/****************************************************************
 *  rg_fruts_get_sg_ptrs()
 ****************************************************************
 *  @brief
 *    Extract sg ptrs from the fruts chain, storing the
 *    ptrs in the passed in SG vector.
 *
 * @param fruts_p - chain of fruts to operate on
 * @param end_pos - position to end at.
 * @param width - current disk array width
 *    sgl_vec,[] [O] - array of sg ptrs extracted
 *
 *  @return
 *    fbe_status_t
 *
 *  @author
 *    04/29/00 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t rg_fruts_get_sg_ptrs(fbe_raid_fruts_t * fruts_p,
                          fbe_u32_t end_pos,
                          fbe_u32_t width,
                          fbe_sg_element_t * sgl_vec[])
{
    fbe_status_t status = FBE_STATUS_OK;
    #if 0
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    if (RAID_COND(fruts_p == NULL))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: %s errored: fruts_p == NULL\n",
                                __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (; fruts_p->position != end_pos;
         fruts_p = (fbe_raid_fruts_get_next(fruts_p)))
    {
        fbe_u32_t data_pos;
        status = FBE_RAID_DATA_POSITION(fruts_p->position,
                                        end_pos,
                                        width,
                                        fbe_raid_siots_parity_num_positions(siots_p),
                                        &data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s erored: status = 0x%x, siots_p = %p\n",
                                    __FUNCTION__,status, siots_p);
            return status;
        }
        fbe_sg_element_t *current_sg_ptr = (fbe_sg_element_t *) fbe_raid_memory_id_get_memory_ptr(fruts_p->sg_id);

        if (RAID_COND(data_pos >= (width - 1)))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s errored: data_pos 0x%x >= (width 0x%x - 1)\n",
                                   __FUNCTION__, data_pos, width);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        sgl_vec[data_pos] = current_sg_ptr;
    }
    #endif
    return status;
}
/*****************************************
 * rg_fruts_get_sg_ptrs
 *****************************************/

/****************************************************************
 * rg_fruts_remove_degraded()
 ****************************************************************
 * @brief
 *  Remove any degraded fruts from this fruts.
 *

 * @param ppFruts - ptr ptr to head of fruts chain.
 * @param dead_pos - position to try to remove.
 *
 * @return
 *  fbe_u16_t - the number of fruts removed.
 *
 * @author
 *  08/15/03 - modified from mirror_vr_remove_degraded_fruts  Rob Foley
 *
 ****************************************************************/

fbe_u16_t rg_fruts_remove_degraded( fbe_raid_fruts_t **ppFruts, fbe_u32_t dead_pos  )
{
    fbe_u16_t num_removed = 0;
#if 0
    fbe_raid_fruts_t *current_fruts_ptr = *ppFruts;
    fbe_raid_fruts_t *prev_fruts_ptr = NULL;
    /* This I/O is completely degraded.
     */
    while (current_fruts_ptr != NULL)
    {
        if ( current_fruts_ptr->position == dead_pos )
        {
            num_removed++;
            
            /* Remove this fruts from the chain
             */
            if (prev_fruts_ptr == NULL)
            {
                /* First item in the list
                 */
                *ppFruts = fbe_raid_fruts_get_next(current_fruts_ptr);
            }
            else
            {
                /* Some item in the middle
                 */
                RG_SET_NEXT_FRUTS(prev_fruts_ptr, fbe_raid_fruts_get_next(current_fruts_ptr));
            }
        }
        prev_fruts_ptr = current_fruts_ptr;
        current_fruts_ptr = fbe_raid_fruts_get_next(current_fruts_ptr);
    }
#endif
    return num_removed;
}
/*****************************************
 * rg_fruts_remove_degraded()
 *****************************************/

/****************************************************************
 * rg_get_fruts_retried_bitmap()
 ****************************************************************
 * @brief
 *  Get a bitmask of retried positions.
 *  We simply loop over the fruts chain, setting the position
 *  of a fruts in the bitmask if the FBE_RAID_COMMON_FLAG_REQ_RETRIED is set
 *  in fruts_p->common.flags.
 *
 * @param fruts_p -  head of a list of fruts.
 * @param retried_bitmap_p - bitmap of retried positions.
 *
 * @return
 *  fbe_u16_t - Count of fruts retried.
 *
 * @author
 *  7/26/05 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u16_t rg_get_fruts_retried_bitmap( fbe_raid_fruts_t * const fruts_p,
                                       fbe_u16_t * input_retried_bitmap_p )
{
    fbe_raid_fruts_t *current_fruts_ptr; /* Pointer to current fruts in chain. */
    fbe_u16_t retried_bitmap = 0; /* Positions retried in fruts chain. */
    fbe_u16_t retried_count = 0;  /* Count of number of fruts in chain retried. */

    /* For every fruts that was retried, set a flag in our bitmask.
     */
    for ( current_fruts_ptr = fruts_p;
          current_fruts_ptr != NULL;
          current_fruts_ptr = fbe_raid_fruts_get_next(current_fruts_ptr) )
    {
        if ( current_fruts_ptr->common.flags & FBE_RAID_COMMON_FLAG_REQ_RETRIED )
        {
            retried_bitmap |= ( 1 << current_fruts_ptr->position );
            retried_count++;
        }
    } /* End for loop over all fruts in chain. */

    /* Copy the retried bitmap we calculated locally
     * into the contents of the pointer passed in.
     */
    *input_retried_bitmap_p = retried_bitmap;

    return retried_count;
}
/********************************
 * rg_get_fruts_retried_bitmap()
 ********************************/

/****************************************************************
 *  fbe_raid_fruts_retry_crc_error()
 ****************************************************************
 *  @brief
 *    Retry a fruts that has taken a crc error.
 *    The only difference from a normal retry is that we
 *    will set the FBE_RAID_COMMON_FLAG_REQ_RETRIED bit in the fruts.
 *
 *    This bit will eventually get cleared in the fruts
 *    either by calling rg_fruts_chain_clear_flag( FBE_RAID_COMMON_FLAG_REQ_RETRIED )
 *    or by completing the I/O with error (which frees the fruts).
 *
 * @param fruts_p - The single fruts to act to retry.
 * @param opcode - Opcode to set into fruts.
 * @param eval_state - New eval state to set in fruts.
 *
 *  @return
 *    none
 *
 *  @author
 *    07/27/05 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_retry_crc_error( fbe_raid_fruts_t * fruts_p,
                                             fbe_lba_t opcode,
                                             fbe_u32_t *evaluate_state )
{
    fbe_status_t status;
    /* Set the retried flag so later we will recognize
     * this as one of the retried fruts.
     */
    fbe_raid_common_set_flag(&fruts_p->common, FBE_RAID_COMMON_FLAG_REQ_RETRIED);

    /* Use the standard retry function to re-submit the fruts.
     */
    status = fbe_raid_fruts_retry( fruts_p, opcode, evaluate_state );

    return status;
}
/*******************************
 * end fbe_raid_fruts_retry_crc_error()
 *******************************/

/*!***************************************************************
 * fbe_raid_fruts_get_chain_bitmap()
 ****************************************************************
 *  @brief
 *   Get a bitmask of positions that are active in this chain
 *   We simply loop over the fruts chain, setting the position
 *   of a fruts in the bitmask as long as the opcode is not
 *   FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID.
 *
 *   fruts_p:          [I] - head of a list of fruts.
 *   fruts_bitmap_p: [O] - bitmap of retried positions.
 *
 *  @return
 *   fbe_u16_t - Count of fruts that are active.
 *
 ****************************************************************/

fbe_u16_t fbe_raid_fruts_get_chain_bitmap( fbe_raid_fruts_t * const fruts_p,
                                           fbe_u16_t * const input_fruts_bitmap_p )
{
    fbe_raid_fruts_t *current_fruts_ptr; /* Pointer to current fruts in chain. */
    fbe_u16_t active_bitmap = 0; /* Positions active in fruts chain. */
    fbe_u16_t active_count = 0;  /* Count of number of fruts in chain active. */

    /* For every fruts that was active, set a flag in our bitmask.
     */
    for ( current_fruts_ptr = fruts_p;
          current_fruts_ptr != NULL;
          current_fruts_ptr = fbe_raid_fruts_get_next(current_fruts_ptr) )
    {
        if ( current_fruts_ptr->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID )
        {
            active_bitmap |= ( 1 << current_fruts_ptr->position );
            active_count++;
        }
    } /* End for loop over all fruts in chain. */

    /* Copy the active bitmap we calculated locally
     * into the contents of the pointer passed in.
     */
    if (input_fruts_bitmap_p != NULL)
    {
        *input_fruts_bitmap_p = active_bitmap;
    }

    return active_count;
}
/********************************
 * fbe_raid_fruts_get_chain_bitmap()
 ********************************/

/*!***************************************************************
 * fbe_raid_fruts_get_success_chain_bitmap()
 ****************************************************************
 *  @brief
 *   Get a bitmask of positions that are active in this chain
 *   and completed.
 *   We simply loop over the fruts chain, setting the position
 *   of a fruts in the bitmask as long as the opcode is not
 *   FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID.
 *
 *   fruts_p - head of a list of fruts.
 *   read_mask_p - bitmap of read positions.
 *   write_mask_p - bitmap of write positions.
 *   b_check_start - TRUE to ignore the status of the request
 *                   FALSE to check the status to see if it completed successfully.
 *
 *  @return
 *   fbe_u16_t - Count of fruts that are active.
 *
 ****************************************************************/

fbe_u16_t fbe_raid_fruts_get_success_chain_bitmap( fbe_raid_fruts_t * fruts_p,
                                                   fbe_raid_position_bitmask_t *read_mask_p,
                                                   fbe_raid_position_bitmask_t *write_mask_p,
                                                   fbe_bool_t b_check_start )
{
    fbe_raid_fruts_t *current_fruts_ptr = NULL;
    fbe_u16_t active_count = 0;

    /* For every fruts that was active, set a flag in our bitmask.
     */
    for ( current_fruts_ptr = fruts_p;
          current_fruts_ptr != NULL;
          current_fruts_ptr = fbe_raid_fruts_get_next(current_fruts_ptr) )
    {
        /* If the opcode is valid, 
         * then check the status if we want to check for finished status. 
         * This allows us to know the bitmap of completed I/Os. 
         */
        if ((current_fruts_ptr->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID) &&
            (b_check_start ||
            (current_fruts_ptr->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)))
        {
            if (fbe_payload_block_operation_opcode_is_media_modify(current_fruts_ptr->opcode)){
                (*write_mask_p) |= ( 1 << current_fruts_ptr->position );
            }
            else {
                (*read_mask_p) |= ( 1 << current_fruts_ptr->position );
            }
            active_count++;
        }
    }

    return active_count;
}
/********************************
 * fbe_raid_fruts_get_success_chain_bitmap()
 ********************************/

/****************************************************************
 *  fbe_raid_fruts_for_each_only
 ****************************************************************
 * @brief
 *  Perform an action specified by a function parameter for each
 *  indicated fruts position in the fruts chain.
 *  ONLY start the fruts, do not modify the "non started" status values.
 *
 *  The difference between this function and fbe_raid_fruts_for_each,
 *  is that this function does not modify the status value
 *  of non-started fruts.
 *
 * @param fru_bitmap - the fruts positions to call fn for
 * @param fruts_p - the chain of fruts to traverse
 * @param fn - the function to call for each fruts
 * @param arg1 - first argument to fn
 * @param arg2 - second argumen to fn.
 *
 * @return
 *  fbe_status_t FBE_STATUS_OK or FBE_STATUS_GENERIC_FAILURE
 *
 * @author
 *  10/17/05 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_for_each_only(fbe_u16_t fru_bitmap,
                                          fbe_raid_fruts_t * fruts_p,
                                          fbe_status_t fn(fbe_raid_fruts_t *, fbe_lba_t, fbe_u32_t *),
                                          fbe_u64_t arg1,
                                          fbe_u64_t arg2)
{
    fbe_status_t status;
    fbe_raid_fruts_t *cur_fruts_p = NULL;
    fbe_raid_fruts_t *next_fruts_p = NULL;

    /* For each fruts in the bitmap, start this fruts.
     */
    cur_fruts_p = fruts_p;
    while ((cur_fruts_p != NULL) && (fru_bitmap != 0))
    {
        next_fruts_p = fbe_raid_fruts_get_next(cur_fruts_p);
        if ((1 << cur_fruts_p->position) & fru_bitmap)
        {
            /* Found a fruts with a position indicated by the bitmap
             * Invoke fn on this fruts. 
             */
            status = fn(cur_fruts_p, (fbe_lba_t)arg1, (fbe_u32_t *)(fbe_ptrhld_t)arg2);
            if (status != FBE_STATUS_OK) { return status; }

            fru_bitmap &= ~(1 << cur_fruts_p->position);
        }
        cur_fruts_p = next_fruts_p;
    }
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_fruts_for_each_only()
 *******************************/

/****************************************************************
 *  fbe_raid_fruts_retry_preserve_flags()
 ****************************************************************
 *  @brief
 *    Retry this fruts, but do not modify flags value.
 *    The only difference between this function and fbe_raid_fruts_retry
 *    is that fbe_raid_fruts_retry will clear out the flags to 0.
 *
 * @param fruts_p - the single fruts to act on
 * @param opcode - opcode to set into fruts
 * @param eval_state - new eval state to set in fruts
 *
 *  @return
 *    none
 *
 *  @author
 *    10/19/05 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_retry_preserve_flags(fbe_raid_fruts_t * fruts_p,
                                   fbe_lba_t opcode,
                                   fbe_u32_t * evaluate_state)
{
    fbe_bool_t b_result = FBE_TRUE;
    fruts_p->common.flags &= ~FBE_RAID_COMMON_FLAG_REQ_STARTED;
    fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;

    fruts_p->opcode = (fbe_u32_t)opcode;
    b_result = fbe_raid_fruts_send(fruts_p, (void *) evaluate_state);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                               "raid: %s failed to start fruts_p: 0x%p, op: %d\n", 
                               __FUNCTION__, 
                               fruts_p, fruts_p->opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_raid_fruts_retry_preserve_flags()
 *******************************/

/****************************************************************
 *          fbe_raid_fruts_set_fruts_degraded_nop()
 ****************************************************************
 *
 * @brief   This function sets a single fruts to the `NOP'  
 *
 * @param fruts_p - The current FRUTS chain to set to NOP.
 *
 * @return
 *  none
 *
 * @author
 *  01/07/2010  Ron Proulx  - Created
 *
 ****************************************************************/
void fbe_raid_fruts_set_fruts_degraded_nop(fbe_raid_fruts_t *fruts_p)
{
    /* Set the single fruts to a `NOP'
     */
    fruts_p->opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
    fruts_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
    return;
}
/*******************************************
 * end fbe_raid_fruts_set_fruts_degraded_nop
 *******************************************/

/*!***************************************************************
 * fbe_raid_fruts_set_retryable_error()
 ****************************************************************
 * @brief Setup a fruts with a single retryable error.
 *
 * @param fruts_p - The current FRUTS.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/23/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_fruts_set_retryable_error(fbe_raid_fruts_t *fruts_p)
{
    fbe_status_t status;
    fbe_payload_ex_t *sep_payload_p;
    fbe_packet_t *packet_p = fbe_raid_fruts_get_packet(fruts_p);
    fbe_payload_block_operation_t *block_operation_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    status = fbe_raid_fruts_populate_packet(fruts_p);
    if (RAID_FRUTS_COND(status != FBE_STATUS_OK))
    { 
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid: %s failed to populate packet: status = 0x%x, fruts_p = 0x%p\n",
                             __FUNCTION__, status, fruts_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_payload_ex_increment_block_operation_level(sep_payload_p);
    if (RAID_FRUTS_COND(status != FBE_STATUS_OK))
    { 
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid: %s failed to increment operation level: status = 0x%x, fruts_p = 0x%p\n",
                             __FUNCTION__, status, fruts_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_payload_block_set_status(block_operation_p,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
    fruts_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
    fruts_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_fruts_set_retryable_error
 **************************************/
/****************************************************************
 *  fbe_raid_fruts_set_degraded_nop()
 ****************************************************************
 * @brief
 *  This function sets any degraded position opcodes to nop.  
 *
 * @param siots_p - The current SIOTS to set to NOP.
 * @param fruts_p - The current FRUTS chain to set to NOP.
 *
 * @return
 *  none
 *
 * @author
 *  03/23/06 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_fruts_set_degraded_nop( fbe_raid_siots_t *const siots_p,
                                      fbe_raid_fruts_t * const fruts_p )
{
    fbe_raid_fruts_t *cur_fruts_p = fruts_p;
    
    fbe_u32_t first_dead_pos = fbe_raid_siots_dead_pos_get( siots_p, FBE_RAID_FIRST_DEAD_POS );
    fbe_u32_t second_dead_pos = fbe_raid_siots_dead_pos_get( siots_p, FBE_RAID_SECOND_DEAD_POS );
    
    if ( first_dead_pos == FBE_RAID_INVALID_DEAD_POS )
    {
        /* There are no degraded positions and thus nothing for
         * us to do, so we will return.
         */
        return;
    }
    
    /* Loop over all fruts in the chain and
     * if any hit the dead position, then set
     * their opcode to NOP
     */
    while ( cur_fruts_p )
    {
        if ( cur_fruts_p->position == first_dead_pos ||
             cur_fruts_p->position == second_dead_pos )
        {
            fbe_raid_fruts_set_fruts_degraded_nop(cur_fruts_p);
        }

        cur_fruts_p = fbe_raid_fruts_get_next( cur_fruts_p );
    }
    return;
}
/* end fbe_raid_fruts_set_degraded_nop() */

/****************************************************************
 *  fbe_raid_fruts_set_parity_nop()
 ****************************************************************
 * @brief
 *  This function sets any parity position opcodes to nop.  
 *
 * @param siots_p - The current SIOTS to set to NOP.
 * @param fruts_p - The current FRUTS chain to set to NOP.
 *
 * @return
 *  none
 *
 * @author
 *  05/18/10 - Created. Kevin Schleicher
 *
 ****************************************************************/
void fbe_raid_fruts_set_parity_nop( fbe_raid_siots_t *const siots_p,
                                    fbe_raid_fruts_t * const fruts_p )
{
    fbe_raid_fruts_t *cur_fruts_p = fruts_p;
    
    /* Loop over all fruts in the chain and
     * if any hit a parity position, then set
     * their opcode to NOP
     */
    while ( cur_fruts_p )
    {
        if ( (cur_fruts_p->position == siots_p->parity_pos) ||
                (fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) &&
                 cur_fruts_p->position == siots_p->parity_pos + 1))
                
        {
            fbe_raid_fruts_set_fruts_degraded_nop(cur_fruts_p);
        }

        cur_fruts_p = fbe_raid_fruts_get_next( cur_fruts_p );
    }
    return;
}
/* end fbe_raid_fruts_set_parity_nop() */

/****************************************************************
 *  fbe_raid_fruts_count_active()
 ****************************************************************
 * @brief
 *  This function returns how many fruts in the chain are not set
 *  to a nop
 *
 * @param siots_p - The current SIOTS 
 * @param fruts_p - The current FRUTS chain to count.
 *
 * @return
 *  The number of fruts that are not NOP in the chain
 *
 * @author
 *  05/18/10 - Created. Kevin Schleicher
 *
 ****************************************************************/
fbe_u32_t fbe_raid_fruts_count_active( fbe_raid_siots_t *const siots_p,
                                       fbe_raid_fruts_t * const fruts_p )
{
    fbe_raid_fruts_t *cur_fruts_p = fruts_p;
    fbe_u32_t count = 0;
    
    /* Loop over all fruts in the chain and
     * count any that are not a NOP
     */
    while ( cur_fruts_p )
    {
        if (cur_fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID)
        {
            count++;
        }

        cur_fruts_p = fbe_raid_fruts_get_next( cur_fruts_p );
    }
    return count;
}
/* end fbe_raid_fruts_count_active() */

/****************************************************************
 *  fbe_raid_fruts_chain_set_retryable()
 ****************************************************************
 * @brief
 *  This function sets any parent read fruts retryable errors
 *  as nops with a status of retryable error.
 *
 * @param siots_p - Current I/O.
 * @param fruts_p - fruts chain.
 * @param retryable_bitmap_p - Ptr to bitmap of retry errors.
 *
 * @return
 *  none
 *
 * @author
 *  4/23/2010 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_fruts_chain_set_retryable(fbe_raid_siots_t *const siots_p,
                                        fbe_raid_fruts_t * const fruts_p,
                                        fbe_raid_position_bitmask_t *retryable_bitmap_p)
{
    fbe_raid_fruts_t *cur_fruts_p = fruts_p;
    fbe_raid_fruts_t *parent_fruts_p = NULL;
    fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
    fbe_u32_t first_dead_pos = fbe_raid_siots_dead_pos_get( siots_p, FBE_RAID_FIRST_DEAD_POS );
    fbe_u32_t second_dead_pos = fbe_raid_siots_dead_pos_get( siots_p, FBE_RAID_SECOND_DEAD_POS );

    *retryable_bitmap_p = 0;

    /* For now only read requests will perform this operation.
     */
    if ((siots_p->algorithm != R5_RD_VR) || 
        (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY)))
    {
        return;
    }
    fbe_raid_siots_get_read_fruts(parent_siots_p, &parent_fruts_p);

    /* Find fruts on the parent that took retryable errors.
     * Set these to retryable with nop on the current chain 
     * so that we do not send these out, but we realize there is an error.
     */
    while ( cur_fruts_p && parent_fruts_p)
    {
        if (cur_fruts_p->position == parent_fruts_p->position)
        {
            if ((parent_fruts_p->position != first_dead_pos) &&
                (parent_fruts_p->position != second_dead_pos) &&
                (parent_fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID) &&
                (cur_fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID) &&
                (parent_fruts_p->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED) &&
                (parent_fruts_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE))
            {
                fbe_raid_fruts_set_flag(cur_fruts_p, FBE_RAID_FRUTS_FLAG_ERROR_INHERITED);
                fbe_raid_fruts_set_retryable_error(cur_fruts_p);
                *retryable_bitmap_p |= (1 << cur_fruts_p->position);
            }
            parent_fruts_p = fbe_raid_fruts_get_next(parent_fruts_p);
        }
        cur_fruts_p = fbe_raid_fruts_get_next(cur_fruts_p);
    }
    return;
}
/**************************************
 * end fbe_raid_fruts_chain_set_retryable
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_retry_fruts_chain()
 ****************************************************************
 * @brief
 *  Send out retries for a given fruts chain.
 *
 * @param siots_p - Current I/O.
 * @param eboard_p - Contains the errors that were just discovered.
 * @param fruts_p - Chain of fruts to retry.               
 *
 * @return fbe_status_t  
 *
 * @author
 *  3/16/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_retry_fruts_chain(fbe_raid_siots_t * siots_p,
                                              fbe_raid_fru_eboard_t * eboard_p,
                                              fbe_raid_fruts_t *fruts_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_time_t msecs = FBE_RAID_MIN_RETRY_MSECS;
    fbe_time_t current_msecs = FBE_RAID_MIN_RETRY_MSECS;
    fbe_raid_fruts_t *cur_fruts_p = NULL;
    fbe_raid_fruts_t *next_fruts_p = NULL;
    fbe_raid_position_bitmask_t fru_bitmask = eboard_p->retry_err_bitmap;

    if (eboard_p->retry_err_bitmap == 0)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "%s retry err bitmap is 0 fruts: %p siots: %p\n",
                               __FUNCTION__, fruts_p, siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Get the max msecs of all the fruts.
     */
    for (cur_fruts_p = fruts_p; cur_fruts_p != NULL; cur_fruts_p = fbe_raid_fruts_get_next(cur_fruts_p))
    {
        /* Clear this flag since we might have inherited this error.
         */
        fbe_raid_fruts_clear_flag(fruts_p, FBE_RAID_FRUTS_FLAG_ERROR_INHERITED);
        if (cur_fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID)
        {
            fbe_raid_fruts_get_retry_wait_msecs(cur_fruts_p, &current_msecs);
            msecs = FBE_MAX(msecs, current_msecs);
        }
    }

    /* If the time we got back is not within our limits,
     * then use our default upper or lower limit. 
     */
    if (msecs > FBE_RAID_MAX_RETRY_MSECS)
    {
        msecs = FBE_RAID_MAX_RETRY_MSECS;
    }
    if ((msecs == 0) || (msecs < FBE_RAID_MIN_RETRY_MSECS))
    {
        msecs = FBE_RAID_MIN_RETRY_MSECS;
    }
    /* Set our wait count first. 
     * Save a copy of the retry count. 
     */
    if (eboard_p->retry_err_count != 0)
    {
        siots_p->wait_count = eboard_p->retry_err_count;
    }
    siots_p->retry_count++;

    cur_fruts_p = fruts_p;
    /* Keep looping until we either hit the end of the chain or
     * have no more to do. 
     */
    while((fru_bitmask != 0) && (cur_fruts_p != NULL))
    {
        next_fruts_p = fbe_raid_fruts_get_next(cur_fruts_p);
        if ((1 << cur_fruts_p->position) & fru_bitmask)
        {
            /* Found a fruts with a position indicated
             * by the bitmap.
             * Invoke fn on this fruts.
             */
            fru_bitmask &= ~(1 << cur_fruts_p->position);
            status = fbe_raid_fruts_delayed_retry(cur_fruts_p, msecs);
            if (status != FBE_STATUS_OK) { return status; }
        }
        cur_fruts_p = next_fruts_p;
    }
    return FBE_STATUS_OK;
 }  
/******************************************
 * end fbe_raid_siots_retry_fruts_chain()
 ******************************************/

/*!**************************************************************
 * fbe_raid_siots_send_timeout_usurper()
 ****************************************************************
 * @brief
 *  Send out the timeout usurper for the timed out retry errors
 *  in the current chain.
 * 
 *  This allows us to kick in rebuild logging and give up on retries.
 *
 * @param siots_p - Current I/O.
 * @param fruts_p - Chain of fruts to send usurpers for.               
 *
 * @return fbe_status_t FBE_STATUS_OK if usurpers were sent
 *                      FBE_STATUS_GENERIC_FAILURE if no
 *                      timed out retries found in the chain
 *                      and thus no usurpers need to be sent.
 *
 * @author
 *  2/9/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_send_timeout_usurper(fbe_raid_siots_t *siots_p,
                                                 fbe_raid_fruts_t *fruts_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_position_bitmask_t timeout_bitmask = 0;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* If we have timeouts then inform the lower level of the timeout.
     */
    fbe_raid_fruts_get_retry_timeout_err_bitmap(fruts_p, &timeout_bitmask);

    if (timeout_bitmask != 0)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                    "parity: Send timeout usurper,mask: 0x%x\n",
                                     timeout_bitmask);
        /* If we have this error as a metadata operation, then we will always 
         * transition state, and when the usurpers are finished we will 
         * fail the I/O.  Monitor operations cannot wait for quiesce or 
         * deadlock will result. 
         */
        if (fbe_raid_siots_is_monitor_initiated_op(siots_p))
        {
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);
        }

        /* The siots wait count is set for the fruts we will wait for.
         */
        siots_p->wait_count = fbe_raid_get_bitcount(timeout_bitmask);

        /* Kick off the usurper packets to inform the lower level. 
         * When the packets complete, this siots will go waiting. 
         */
        fbe_raid_fruts_chain_send_timeout_usurper(fruts_p, timeout_bitmask);
        status = FBE_STATUS_OK;
    }
    else
    {
        /* No retries to send.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "no timeout errors rg: 0x%x",
                                     fbe_raid_geometry_get_object_id(raid_geometry_p));
            
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_raid_siots_send_timeout_usurper 
 **************************************/
/****************************************************************
 * fbe_raid_fruts_unchain()
 ****************************************************************
 * @brief
 *  Remove one or more fruts from a chain,
 *  returning the fruts pointer.
 *
 * @param fruts_p - head of a list of fruts.
 * @parram fru_bitmap - bitmap of positions to remove.
 *
 * @return
 *   fbe_raid_fruts_t* - Non-NULL if a fru from the bitmap
 *          was found.  NULL if zero fruts found.
 *          In addition, as fruts are removed from the chain,
 *          the bit for that fru is cleared.
 *
 * @author
 *  06/01/00 - Re-Created. RPF.
 *
 ****************************************************************/

fbe_raid_fruts_t *fbe_raid_fruts_unchain(fbe_raid_fruts_t * fruts_p, fbe_u16_t fru_bitmap)
{
    fbe_raid_fruts_t *cur_p = NULL;
    fbe_raid_fruts_t *prev_p;

    if(RAID_COND(fru_bitmap == 0) )
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid fru_bitmap 0x%x NULL\n",
                             fru_bitmap);
        return cur_p;
    }

    for (cur_p = fruts_p, prev_p = NULL;
         cur_p != NULL;
         prev_p = cur_p, cur_p = fbe_raid_fruts_get_next(cur_p))
    {
        if ((1 << cur_p->position) & fru_bitmap)
        {
            fbe_queue_remove((fbe_queue_element_t*)cur_p);
            break;
        }
    }
    return cur_p;
}
/* fbe_raid_fruts_unchain() */
/*!**************************************************************
 * @fn fbe_raid_fruts_validate_sg(fbe_raid_fruts_t *fruts_p)
 ****************************************************************
 * @brief
 *  Make sure the fruts sg has the appropriate number of
 *  blocks and that we have not exceeded the length of the sg list.
 *  
 * @param fruts_p - The fruts to check.
 *
 * @return - None.   
 *
 * @author
 *  12/19/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_validate_sg(fbe_raid_fruts_t *const fruts_p)
{
    /* Make sure the number of blocks in the fruts matches
     * the number of blocks in the sg. 
     * Also make sure we have not exceeded the sg length. 
     */
    fbe_u32_t sgs = 0;
    fbe_block_count_t curr_sg_blocks = 0;
    fbe_status_t status = FBE_STATUS_OK;
    /* Make sure the number of sgs in the fruts is less than or equal to
     * the max number allowed. 
     */
    if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)fruts_p->sg_id))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid header invalid for fruts 0x%p, sg_id: 0x%p\n",
                             fruts_p, fruts_p->sg_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Make sure the fruts has blocks in it.
     */
    if (fruts_p->blocks == 0)
    {
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
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid: failed to count sg blocks: status = 0x%x, curr_sg_blocks 0x%llx\n",
                             status, (unsigned long long)curr_sg_blocks);
        return status;
    }

    /* Make sure the fruts has an SG for the correct block count.
     */
    if (fruts_p->blocks != curr_sg_blocks)
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid fruts blocks 0x%llx not equal to sg blocks 0x%llx fruts: 0x%p\n",
                             (unsigned long long)fruts_p->blocks,
			     (unsigned long long)curr_sg_blocks, fruts_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_fruts_validate_sg()
 **************************************/
/*!**************************************************************
 * fbe_raid_fruts_chain_validate_sg()
 ****************************************************************
 * @brief
 *  Make sure the fruts sg has the appropriate number of
 *  blocks and that we have not exceeded the length of the sg list.
 *  
 * @param fruts_p - The fruts to check.
 *
 * @return - fbe_status_t 
 *
 * @author
 *  12/19/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_chain_validate_sg(fbe_raid_fruts_t *fruts_p)
{
    fbe_status_t status;
    while (fruts_p)
    {
        status = fbe_raid_fruts_validate_sg(fruts_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                 "raid: %s failed to validate sg for fruts 0x%p\n", __FUNCTION__, fruts_p);
            return status;
        }
        fruts_p = fbe_raid_fruts_get_next(fruts_p);
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_fruts_chain_validate_sg()
 **************************************/

/*!**************************************************************
 * fbe_raid_fruts_chain_get_by_position()
 ****************************************************************
 * @brief
 *  Get the given position within the fruts chain.
 *  
 * @param head_p - The ptr to the head of the chain.
 * @param position - The position within the chain to fetch.
 * 
 * @return fbe_raid_fruts_t* The first fruts in the chain with this position. 
 *
 * @author
 *  12/22/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_fruts_t * fbe_raid_fruts_chain_get_by_position(fbe_queue_head_t *head_p,
                                                        fbe_u16_t position)
{
    fbe_raid_fruts_t *fruts_p = NULL;

    if (fbe_queue_is_empty(head_p))
    {
        return NULL;
    }

    /* Simply loop over the chain and break when we hit 
     * the fruts with the position we are looking for. 
     */
    for (fruts_p = (fbe_raid_fruts_t*)fbe_queue_front(head_p);
          fruts_p != NULL;
          fruts_p = fbe_raid_fruts_get_next(fruts_p))
    {
        if (fruts_p->position == position)
        {
            break;
        }
    }
    return fruts_p;
}
/**************************************
 * end fbe_raid_fruts_chain_get_by_position()
 **************************************/

/*!***************************************************************************
 *          fbe_raid_fruts_get_min_blocks_transferred()
 *****************************************************************************
 *
 * @brief   Determine the number of blocks transferred for the specified fruts
 *          chain.
 *  
 * @param start_fruts_p - The head of the fruts chain. 
 * @param   start_lba - Used to validate fruts and media lba 
 * @param min_blocks_p - The ptr to the minimum number of blocks transferred
 *                       from all the fruts in the chain.
 * @param errored_bitmask_p - The ptr to the error bitmask to return.
 *
 * @return  status - FBE_STATUS_OK - fruts status as expected
 *                   Other - Unexpected fruts status
 *
 * @author
 *  2/5/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_fruts_get_min_blocks_transferred(fbe_raid_fruts_t *const start_fruts_p,
                                                       const fbe_lba_t start_lba,
                                                       fbe_block_count_t *const min_blocks_p,
                                                       fbe_raid_position_bitmask_t *const errored_bitmask_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_block_count_t        total_blocks = *min_blocks_p;
    fbe_block_count_t min_blocks = *min_blocks_p;
    fbe_raid_position_bitmask_t errored_bitmask = 0;
    fbe_raid_fruts_t *fruts_p = NULL;

    /* Simply find the minimum transfer count from all the fruts.
     * This tells us what to set the transfer count to for 
     * the entire request.  Each fruts contains the media error lba. 
     */
    for ( fruts_p = start_fruts_p;
          fruts_p != NULL;
          fruts_p = fbe_raid_fruts_get_next(fruts_p) )
    {            
        fbe_status_t                            transport_status;
        fbe_u32_t transport_qualifier;
        fbe_payload_block_operation_status_t    block_status;
        fbe_payload_block_operation_qualifier_t block_qualifier;

        /* If this is not being used, then skip over it.
         */
        if (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID)
        {
            continue;
        }
        /* Validate fruts.
         */
        if ((fruts_p->lba != start_lba)       ||
            (fruts_p->blocks != total_blocks)    )
        {
            /* Trace the error and return failure.
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: Position: 0x%x lba: 0x%llx blks: 0x%llx doesn't agree start_lba: 0x%llx 0x%llx \n",
                                   fruts_p->position,
				   (unsigned long long)fruts_p->lba,
				   (unsigned long long)fruts_p->blocks,
				   (unsigned long long)start_lba,
				   (unsigned long long)total_blocks);
            return(FBE_STATUS_GENERIC_FAILURE);
        }
        
        /* For each fruts on the chain if the status is 
         * FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR and the qualifier
         * is NOT FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_NO_REMAP
         * update `min_blocks'.
         */
        fbe_raid_fruts_get_status(fruts_p, 
                                  &transport_status, 
                                  &transport_qualifier,
                                  &block_status, 
                                  &block_qualifier);

        /* Sanity check transport status.
         */
        if (transport_status != FBE_STATUS_OK)
        {
            /* Trace the error and return failure.
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s: Detected transport status: 0x%x on position: 0x%x \n",
                                   __FUNCTION__, transport_status, fruts_p->position);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* Only process positions that reported a hard or soft media error.
         */
        if ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR) ||
            ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
             (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED)))
        {
            fbe_lba_t media_error_lba;

            /* If the qualifier is `no remap' something is wrong.
             */
            if (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_NO_REMAP)
            {
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                       "raid: %s: Unexpected `no remap' qualifier on position: 0x%x \n",
                                       __FUNCTION__, fruts_p->position);
                return(FBE_STATUS_GENERIC_FAILURE);
            }

            /* Now get the media lba an udpate the min blocks as required.
             */
            fbe_raid_fruts_get_media_error_lba(fruts_p, &media_error_lba);

            /* If the media error lba isn't valid report a failure.
             */
            if ((media_error_lba < fruts_p->lba)                        ||
                (media_error_lba > (fruts_p->lba + fruts_p->blocks - 1))   )
            {
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                       "raid: %s: Position: 0x%x media_error_lba: 0x%llx incorrect lba: 0x%llx 0x%llx\n",
                                       __FUNCTION__, fruts_p->position,
				       (unsigned long long)media_error_lba,
				       (unsigned long long)fruts_p->lba,
				       (unsigned long long)fruts_p->blocks);
                return(FBE_STATUS_GENERIC_FAILURE);
            }

            /* Set the errored bit for this position.
             */
            errored_bitmask |= (1 << fruts_p->position);

            /* Now update the min_blocks as required.
             */
            if ((media_error_lba - fruts_p->lba) < min_blocks)
            {
                min_blocks = media_error_lba - fruts_p->lba;
            }
        
        } /* end if media error */
    
    } /* end for all positions on chain */
    
    /* Save the values to be returned.
     */
    *errored_bitmask_p = errored_bitmask;
    *min_blocks_p = min_blocks;
    
    /* Always return the execution status.
     */
    return(status);
}
/*************************************************
 * end fbe_raid_fruts_get_min_blocks_transferred()
 *************************************************/

/*!**************************************************************************
 * fbe_raid_fruts_chain_destroy()
 ****************************************************************************
 * @brief   This is the main entry point for freeing memory.
 *
 * @param   queue_head_p - First element of the queue to free.
 *                         Each item on the queue is a fbe_raid_memory_header_t.
 * @param   num_fruts_destroyed_p - Address of number of fruts destroyed
 *
 * @return  FBE_STATUS_GENERIC_FAILURE - Invalid parameter or error freeing.
 *          FBE_STATUS_OK - Memory was allocated successfully.
 * 
 * @author
 *  08/05/2009  Rob Foley - Created   
 *
 **************************************************************************/
fbe_status_t fbe_raid_fruts_chain_destroy(fbe_queue_head_t *queue_head_p,
                                          fbe_u16_t *num_fruts_destroyed_p)
{
    fbe_raid_fruts_t *fruts_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_destroy_failed = FBE_FALSE;

    /* Just loop over all the elements on the queue and free them all.
     */
    while (fbe_queue_is_empty(queue_head_p) == FALSE)
    {
        fruts_p = (fbe_raid_fruts_t*)fbe_queue_pop(queue_head_p);

        if (fbe_raid_common_is_flag_set(&fruts_p->common, FBE_RAID_COMMON_FLAG_TYPE_FRU_TS))
        {
            /* We need to make sure the fruts packet is destroyed.
             */
            status = fbe_raid_fruts_destroy(fruts_p);
            if (RAID_MEMORY_COND(status != FBE_STATUS_OK) )
            {
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                       "%s status: 0x%x fbe raid status fruts destroy fails\n",
                                       __FUNCTION__,
                                       status);
                b_destroy_failed = FBE_TRUE;
            }
            else
            {
                *num_fruts_destroyed_p += 1;
            }
        }
        else
        {
            fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP,
                                     FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "raid: %s item not a fruts common_flags: 0x%x:,",
                                     __FUNCTION__, fruts_p->common.flags);
        }
    }

    if (b_destroy_failed == FBE_TRUE)
    {
        /* freeing of at least one fruts failed.
         */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Done freeing.
     */
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_fruts_chain_destroy()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_get_inherited_error_count()
 ****************************************************************
 * @brief
 *  Get the number of errors inherited from the parent.
 *
 * @param siots_p - Ptr to I/O to save these counts in.
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  4/27/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_raid_siots_get_inherited_error_count(fbe_raid_siots_t *const siots_p)
{
    fbe_u32_t inherited_count = 0;
    fbe_raid_fruts_t *current_fruts_p = NULL;
    fbe_raid_fruts_t *next_fruts_p = NULL;
    if (siots_p->algorithm != R5_RD_VR)
    {
        return 0;
    }

    fbe_raid_siots_get_read_fruts(siots_p, &current_fruts_p);
    while (current_fruts_p != NULL)
    {
        /* Try to start the next fruts to the edge.
         */
        next_fruts_p = (fbe_raid_fruts_t *)fbe_queue_next(&siots_p->read_fruts_head, 
                                                          (fbe_queue_element_t*)current_fruts_p);

        /* If the opcode is invalid we do not want to send this fruts.
         */
        if ((current_fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID) &&
            fbe_raid_fruts_is_flag_set(current_fruts_p, FBE_RAID_FRUTS_FLAG_ERROR_INHERITED))
        {
            inherited_count++;
        }
        current_fruts_p = next_fruts_p;
    }
    return inherited_count;
}
/******************************************
 * end fbe_raid_siots_get_inherited_error_count()
 ******************************************/

/*!**************************************************************
 * fbe_raid_fruts_inc_error_count()
 ****************************************************************
 * @brief
 *  Save the eboard counts in this request.
 *
 * @param fruts_p - fruts to save counts for.
 * @param err_type - Type of error to account for.
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  11/24/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_inc_error_count(fbe_raid_fruts_t *fruts_p,
                                            fbe_raid_fru_error_type_t err_type)
{
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_raid_verify_error_counts_t *vp_eboard_local_p = NULL;

    if (fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_ERROR_INHERITED))
    {
        /* The error was previously accounted for.  Do not count it now.
         */
        return FBE_STATUS_OK;
    }
    /* If the client object supplied a vp eboard, add the local
     * counts.
     */
    vp_eboard_local_p = fbe_raid_siots_get_verify_eboard(siots_p);

    if (vp_eboard_local_p != NULL)
    {
        /* Increment the appropriate count in the eboard.
         */
        switch (err_type)
        {
            case FBE_RAID_FRU_ERROR_TYPE_RETRYABLE:
                vp_eboard_local_p->retryable_errors++;
                break;
            case FBE_RAID_FRU_ERROR_TYPE_NON_RETRYABLE:
                vp_eboard_local_p->non_retryable_errors++;
                break;
			case FBE_RAID_FRU_ERROR_TYPE_SOFT_MEDIA:
                vp_eboard_local_p->c_soft_media_count++;
                break;
			case FBE_RAID_FRU_ERROR_TYPE_HARD_MEDIA:
                vp_eboard_local_p->u_media_count++;
                break;
            default:
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "fru save error counts unknown err type: %d\n",
                                       err_type);
                return FBE_STATUS_GENERIC_FAILURE;
                break;
        };
        /* Set this flag so we will not account for this error again.
         */
        fbe_raid_fruts_set_flag(fruts_p, FBE_RAID_FRUTS_FLAG_ERROR_INHERITED);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_fruts_inc_error_count()
 ******************************************/

/*!**************************************************************
 * fbe_raid_fruts_abort()
 ****************************************************************
 * @brief
 *  Abort the packets for a fruts chain.
 *
 * @param fruts_p - Ptr to the head of the chain.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  4/19/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_abort(fbe_raid_fruts_t *const fruts_p)
{
    /* We will abort only the outstanding fruts here
     */
    if(fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_REQUEST_OUTSTANDING) &&
     (!fbe_transport_is_packet_cancelled(&fruts_p->io_packet)))
    {
        fbe_transport_cancel_packet(&fruts_p->io_packet);
    }
    else
    {
        /* fruts is either (a) Not yet sent to next level OR 
         *                 (b) Completed successfully OR 
         *                 (c) Set as NOP.
         * We will not abort it.
         */
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_fruts_abort()
 ******************************************/

/*!**************************************************************
 * fbe_raid_fruts_abort_chain()
 ****************************************************************
 * @brief
 *  Abort the packets for a fruts chain.
 *
 * @param head_p - Ptr to the head of the chain.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  4/19/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_abort_chain(fbe_queue_head_t *head_p)
{
    fbe_raid_fruts_t *current_fruts_p = NULL;
    fbe_raid_fruts_t *next_fruts_p = NULL;
    fbe_raid_siots_t *siots_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = NULL;

    current_fruts_p = (fbe_raid_fruts_t *)fbe_queue_front(head_p);

    if (current_fruts_p != NULL)
    {
        siots_p = fbe_raid_fruts_get_siots(current_fruts_p);
        raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    }
    /* Just loop over everything, trying to start them.
     */
    while (current_fruts_p != NULL)
    {
        /* Try to start the next fruts to the edge.
         */
        next_fruts_p = (fbe_raid_fruts_t *)fbe_queue_next(head_p, 
                                                          (fbe_queue_element_t*)current_fruts_p);
        /* Do not abort write type operations on parity or mirror,
         * since this could cause a parity/mirror inconsistency. 
         */
        if (fbe_payload_block_operation_opcode_is_media_modify(current_fruts_p->opcode) &&
            (fbe_raid_geometry_is_mirror_type(raid_geometry_p) ||
             fbe_raid_geometry_is_parity_type(raid_geometry_p)))
        {
            /* Don't abort these.
             */
        }
        /* If the opcode is invalid we do not want to send this fruts.
         */
        else if (current_fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID)
        {
            fbe_raid_fruts_abort(current_fruts_p);
        }
        current_fruts_p = next_fruts_p;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_fruts_abort_chain()
 ******************************************/
/*!**************************************************************
 * fbe_raid_fru_eboard_display()
 ****************************************************************
 * @brief
 *  Display a fru eboard.
 *
 * @param eboard_p - Ptr to the eboard to display        
 *
 * @return fbe_status_t   
 *
 * @author
 *  4/26/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fru_eboard_display(fbe_raid_fru_eboard_t *eboard_p,
                                         fbe_raid_siots_t *siots_p, 
                                         fbe_trace_level_t trace_level)
{
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_LEVEL(trace_level),
                                "eboard: siots: 0x%x soft_me_c: %d hard_me_b: 0x%x menr_b: 0x%x unexp: %d\n",
                                fbe_raid_memory_pointer_to_u32(siots_p),
                           eboard_p->soft_media_err_count,
                           eboard_p->hard_media_err_bitmap, 
                           eboard_p->menr_err_bitmap,
                                eboard_p->unexpected_err_count);
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_LEVEL(trace_level),
                                "eboard: siots: 0x%x abort: %d timeout: %d dead_b: 0x%x retry_b: 0x%x\n",
                                fbe_raid_memory_pointer_to_u32(siots_p),
                           eboard_p->abort_err_count,
                           eboard_p->timeout_err_count,
                           eboard_p->dead_err_bitmap,
                           eboard_p->retry_err_bitmap);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_fru_eboard_display()
 ******************************************/

/*!**************************************************************************
 *          fbe_raid_fruts_get_failed_io_pos_bitmap()
 ***************************************************************************
 *
 * @brief
 *    This function returns a bitmap of the failed positions in the given fruts chain.
 *
 * @param   in_fruts_p [I]                      - The fruts chain to check for failed I/Os
 * @param   out_failed_io_position_bitmap_p [O] - The bitmap of positions with failed I/Os
 *
 * @return fbe_status_t   
 *
 * @author
 *  06/2010  rundbs  - Created
 *
 **************************************************************************/
fbe_status_t fbe_raid_fruts_get_failed_io_pos_bitmap(fbe_raid_fruts_t   *const in_fruts_p, 
                                                     fbe_u32_t          *out_failed_io_position_bitmap_p)
{
    fbe_bool_t              out_is_init_b;
    fbe_raid_fru_eboard_t   eboard;
    fbe_bool_t b_result = FBE_TRUE;


    //  Initialize local variables
    out_is_init_b = TRUE;

    //  If the incoming fruts chain is NULL, return right away, there is nothing to check
    if (in_fruts_p == NULL)
    {
        return FBE_STATUS_OK;
    }

    //  If the fruts elements in the chain are not initialized, return right away; the I/O has not been sent yet
    fbe_raid_fruts_is_fruts_initialized(in_fruts_p, &out_is_init_b);
    if (!out_is_init_b)
    {
        return FBE_STATUS_OK;
    }

    //  Initialize the eboard
    fbe_raid_fru_eboard_init(&eboard);

    //  Get the fruts chain eboard
    b_result = fbe_raid_fruts_chain_get_eboard(in_fruts_p, &eboard);
    if (RAID_COND(b_result != FBE_TRUE))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "%s failed to get eborad for fruts chain in_fruts_p = 0x%p\n",
                               __FUNCTION__,
                               in_fruts_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //  From the eboard, set the output parameter to the "dead error" bitmap
    *out_failed_io_position_bitmap_p = eboard.dead_err_bitmap;

    return FBE_STATUS_OK;
}
/**********************************
 * end fbe_raid_fruts_get_failed_io_pos_bitmap()
 **********************************/

/*!**************************************************************************
 *          fbe_raid_fruts_is_fruts_initialized()
 ***************************************************************************
 *
 * @brief
 *    This function returns a boolean indicating if the given fruts chain elements are initialized.
 *    If any element is not initialized, this function sets the output parameter to FALSE.
 *
 * @param   in_fruts_p [I]      - The fruts chain to check
 * @param   out_is_init_b [O]   - The boolean indicating if fruts elements initialized or not
 *
 * @return fbe_status_t   
 *
 * @author
 *  06/2010  rundbs  - Created
 *
 **************************************************************************/
fbe_status_t fbe_raid_fruts_is_fruts_initialized(fbe_raid_fruts_t   *const in_fruts_p, 
                                                 fbe_u32_t          *out_is_init_b)
{
    fbe_raid_fruts_t    *current_fruts_ptr;


    for (current_fruts_ptr = in_fruts_p;
         current_fruts_ptr != NULL;
         current_fruts_ptr = fbe_raid_fruts_get_next(current_fruts_ptr))
    {
        //  If the opcode is invalid, skip this element all together
        if (current_fruts_ptr->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID)
        {
            continue;
        }

        if (!fbe_raid_fruts_is_flag_set(current_fruts_ptr, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
        {
            *out_is_init_b = FALSE;
            return FBE_STATUS_OK;
        }
    }

    *out_is_init_b = TRUE;
    return FBE_STATUS_OK;
}
/**********************************
 * end fbe_raid_fruts_is_fruts_initialized()
 **********************************/

/*!**************************************************************************
 *          fbe_raid_fruts_initialize_transport_packet()
 ***************************************************************************
 *
 * @brief
 *  This function initializes the transport packet within the fruts.
 *
 * @param in_fruts_p - The fruts containing the fruts to inintialize
 *
 * @return fbe_status_t   
 *
 * @author
 *  08/09/2010  Kevin Schleicher  - Created
 *
 **************************************************************************/
fbe_status_t fbe_raid_fruts_initialize_transport_packet(fbe_raid_fruts_t   *const in_fruts_p)
{
    fbe_status_t status;
    fbe_packet_t   *packet_p = NULL;
    fbe_packet_t *master_packet_p = NULL;
    fbe_cpu_id_t cpu_id;

    /* Get the packet associated with this fruts.
     */
    packet_p = fbe_raid_fruts_get_packet(in_fruts_p);
    if(RAID_FRUTS_COND(packet_p == NULL) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                "raid: %s packet NULL\n",
                                __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fbe_raid_fruts_is_flag_set(in_fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        /* Since we already initted the packet, just reinit now.
         */
        status = fbe_transport_reuse_packet(packet_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s packet reuse status %d\n",
                                   __FUNCTION__, status);
            return status;
        }
    }
    else
    {
        /* Now initialize the base packet.
         * Note that we need to clear out the magic number first since we are allocating 
         * from a pool of memory which has unknown content and the init code assumes 
         * the packet is not initted yet. 
         */
        status = fbe_transport_initialize_sep_packet(packet_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s packet initialize status %d\n",
                                   __FUNCTION__, status);
            return status;
        }
        fbe_raid_fruts_set_flag(in_fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED);
        /* Set the same cpu id as in master packet */
        master_packet_p = fbe_raid_fruts_to_master_packet(in_fruts_p);
        fbe_transport_get_cpu_id(master_packet_p, &cpu_id);
        fbe_transport_set_cpu_id(packet_p, cpu_id);
    }
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_fruts_initialize_transport_packet()
 **************************************************/
#if 0
/*!**************************************************************
 * fbe_raid_fruts_usurper_send_logical_error_to_pdo()
 ****************************************************************
 * @brief
 *  This function sends the usurper command to PDO in case 
 *  of CRC error.
 *
 * @param  siots_p - ptr to fbe_raid_siots_t
 * @param  packet_p - ptr to fbe_packet_t
 * @param  error_type - type of CRC error
 * @param  fru_pos - fru position on which error has occured
 *
 * @return status - The status of the operation.
 *
 * @author
 *  8/18/2010 - Created. Pradnya Patankar
 *
 ****************************************************************/
fbe_status_t  fbe_raid_fruts_usurper_send_logical_error_to_pdo(fbe_raid_fruts_t * fruts_p, 
                                                               fbe_packet_t* packet_p,
                                                               fbe_u16_t error_type, 
                                                               fbe_u16_t fru_pos)
{
    
    fbe_block_transport_control_logical_error_t logical_error_to_send;
    fbe_status_t status;
    fbe_object_id_t pdo_object_id;

    /* Make sure packet is initted.
     */
    if (!fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        status = fbe_raid_fruts_initialize_transport_packet(fruts_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s packet initialize status %d\n",
                                   __FUNCTION__, status);
            return status;
        }
    }
    pdo_object_id = fruts_p->pdo_object_id;
    switch (error_type)
    {
        case FBE_XOR_ERR_TYPE_MULTI_BIT_CRC:
            logical_error_to_send.error_type = FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC_MULTI_BITS;
            break;
        case FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC:
            logical_error_to_send.error_type = FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC_SINGLE_BIT;
            break;
        default:
            logical_error_to_send.error_type = FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC;
            break;
    }
    
    status = fbe_raid_fruts_usurper_send_control_packet_to_pdo(packet_p,
                                                               FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS,
                                                               &logical_error_to_send,
                                                               sizeof(fbe_block_transport_control_logical_error_t),
                                                               pdo_object_id, 
                                                               FBE_PACKET_FLAG_NO_ATTRIB,
                                                               fruts_p,
                                                               FBE_PACKAGE_ID_PHYSICAL);

   if (status != FBE_STATUS_OK ) 
   {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                               "rdfru_send_er_pdo failed: packet: err:0x%x\n",
                               status );
        return FBE_STATUS_GENERIC_FAILURE;
    }
   return status;                                                                                                                                                               
} 
/***********************************************************
 * end fbe_raid_fruts_usurper_send_logical_error_to_pdo()
 ***********************************************************/
#endif
/*!**************************************************************
 * fbe_raid_fruts_usurper_send_control_packet_to_pdo()
 ****************************************************************
 * @brief
 *  This function sends the control packet to PDO.
 *
 * @param packet_p - Packet to send to PDO.
 * @param control_code  - control code to be sent.
 * @param buffer - the payload control buffer
 * @param buffer_length - payload control buffer length
 * @param object_id - PDO object id
 * @param attr - Packet attributes
 * @param fruts_p - pointer to fruts
 * @param package_id - Package to which packet will be sent.

 * @return status - The status of the operation.
 *
 * @author
 *  8/18/2010 - Created. Pradnya Patankar
 *
 ****************************************************************/
static fbe_status_t fbe_raid_fruts_usurper_send_control_packet_to_pdo(fbe_packet_t *packet_p,
                                                                      fbe_payload_control_operation_opcode_t control_code,
                                                                      fbe_payload_control_buffer_t buffer,
                                                                      fbe_payload_control_buffer_length_t buffer_length,
                                                                      fbe_object_id_t object_id,
                                                                      fbe_packet_attr_t attr,
                                                                      fbe_raid_fruts_t *fruts_p,
                                                                      fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                     payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;

    if (packet_p == NULL) 
    {
        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                      " %s packet is null\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    payload_p = fbe_transport_get_payload_ex (packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation(control_operation_p, control_code, buffer, buffer_length);

    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet_p, attr);

    /* Next operation is the control operation, no need to increment the operation level
     */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_raid_fruts_usurper_send_control_packet_to_pdo_completion,
                                          fruts_p);

    /* Control packets should be sent via service manager, since we are sending directly to the physical package.
     */
    status = fbe_service_manager_send_control_packet(packet_p);

    return status;
}
/********************************************************************
 * end fbe_raid_fruts_usurper_send_control_packet_to_pdo()
 ********************************************************************/

/*!***************************************************************
 * fbe_raid_fruts_usurper_send_control_packet_to_pdo_completion()
 ****************************************************************
 * 
 * @brief   This is the completion for a usurper sent to the PDO.
 *          We assume the caller set the wait count in the siots
 *          before launching this packet.
 * 
 * @param packet_p - The packet being completed.
 * @param context - The fruts ptr.
 *
 * @return fbe_status_t FBE_STATUS_MORE_PROCESSING_REQUIRED
 *                      Since this is the last completion function
 *                      in this packet we need to return this status.
 *
 * @author
 *  20/10/2011 - Created. Vishnu Sharma
 *
 ****************************************************************/
static fbe_status_t fbe_raid_fruts_usurper_send_control_packet_to_pdo_completion(fbe_packet_t * packet_p, 
                                                                                 fbe_packet_completion_context_t context)
{
    fbe_raid_fruts_t   *fruts_p = NULL;
    fbe_raid_siots_t   *siots_p = NULL;
    fbe_raid_geometry_t   *raid_geometry_p = NULL;
    fbe_object_id_t rg_object_id;
    fbe_payload_ex_t    *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_status_t pkt_status;
    fbe_payload_control_status_t control_status;

    /* First get and validate the fruts associated with this packet.
     * Although we could use the packet itself we use the completion context.
     */
    fruts_p = (fbe_raid_fruts_t *)context;
    if (RAID_FRUTS_COND((fruts_p == NULL) ||
                        !(fbe_raid_common_get_flags(&fruts_p->common) & FBE_RAID_COMMON_FLAG_TYPE_FRU_TS) ) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: %s errored: fruts_p is NULL or flags 0x%x is wrong or evaluate state NULL\n",
                               __FUNCTION__, 
                               (int)fruts_p->common.flags);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Now get the raid group associated with the request.
     */
    siots_p = fbe_raid_fruts_get_siots(fruts_p);
    if (RAID_FRUTS_COND((siots_p == NULL) ||
                        (siots_p->common.state == (fbe_raid_common_state_t)fbe_raid_siots_freed) ||
                        (siots_p->wait_count == 0) ) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: %s errored: siots_p is null or common state 0x%p is wrong or wait count 0x%llx == 0\n",
                               __FUNCTION__, 
                               siots_p->common.state, 
                               (unsigned long long)siots_p->wait_count);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    if (RAID_FRUTS_COND(raid_geometry_p == NULL) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: %s failed to get raid geometry: raid_geometry_p is null : siots_p = 0x%p\n",
                               __FUNCTION__,
                               siots_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Make sure we still have something outstanding.
     * We do a lower and upper range check.  The upper range check is fairly arbitrary, 
     * but it is unlikely that we will have more than twice the nubmer of drives worth of 
     * operations outstanding at this time. 
     */
    if ((siots_p->wait_count == 0) || (siots_p->wait_count > (fbe_raid_siots_get_width(siots_p) * 2)))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "%s siots_p %p has invalid wait count: %lld\n", __FUNCTION__,
                             siots_p, (long long)siots_p->wait_count);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* If the siots state is freed, then something is very wrong.
     */
    if (siots_p->common.state == (fbe_raid_common_state_t)fbe_raid_siots_freed)
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "%s siots_p %p state is freed\n", __FUNCTION__, siots_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Copy the packet status to the fruts.
     */
    payload_p = fbe_transport_get_payload_ex (packet_p);
    
    if (payload_p == NULL)
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "%s fruts %p packet %p has null payload\n",
                             __FUNCTION__, fruts_p, fbe_raid_fruts_get_packet(fruts_p));
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    if (control_operation_p == NULL)
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "%s fruts %p packet %p has a null control operation\n",
                             __FUNCTION__, fruts_p, fbe_raid_fruts_get_packet(fruts_p));
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    rg_object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    //fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, fruts_p->position);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    pkt_status = fbe_transport_get_status_code(&fruts_p->io_packet);
    
    if ((pkt_status != FBE_STATUS_OK) ||
        (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                     "set t/o err failed obj_id: 0x%x pdo_obj: 0x%x pos: %d pkt_st: 0x%x cnt_st: 0x%x\n",
                                     rg_object_id, 
                                     fruts_p->pvd_object_id,
                                     fruts_p->position,
                                     pkt_status, control_status);
    }

    /* We always release the control operation since 
     * there is likely still an active block operation that we will 
     * want to look at to interpret status. 
     */
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

    /* Now check if all requests we sent are done.
     */
    if(fbe_atomic_decrement(&siots_p->wait_count) == 0)
    {
        /* We are done with all requests simply enqueue the request to 
         * the raid group state queue.
         */
         fbe_raid_common_state((fbe_raid_common_t *)siots_p);
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_fruts_usurper_send_control_packet_to_pdo_completion
 **************************************/

/*!**************************************************************
 * fbe_raid_fruts_chain_send_timeout_usurper()
 ****************************************************************
 * @brief
 *  Send timeout usurpers for positions that have retries
 *  which have timed out.
 * 
 *  Raid will send this when it believes that the drive is
 *  taking physical errors and is taking too long to process an I/O.
 *
 * @param fruts_p - Chain of fruts to retry.
 * @param pos_bitmask - bitmask of positions to send on.
 *
 * @return fbe_status_t  FBE_STATUS_OK if usurpers were sent.
 *                       other status implies failure sending usurpers.
 *
 * @author
 *  2/4/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_chain_send_timeout_usurper(fbe_raid_fruts_t *fruts_p,
                                                       fbe_raid_position_bitmask_t pos_bitmask)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_fruts_t *cur_fruts_p = NULL;
    fbe_raid_fruts_t *next_fruts_p = NULL;
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);

    if (pos_bitmask == 0)
    {
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        fbe_object_id_t rg_object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "timeout bitmap is 0 obj: 0x%x fruts: %p siots: %p\n",
                                      rg_object_id, fruts_p, siots_p);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    cur_fruts_p = fruts_p;
    /* Keep looping until we either hit the end of the chain or
     * have no more to do. 
     */
    while((pos_bitmask != 0) && (cur_fruts_p != NULL))
    {
        next_fruts_p = fbe_raid_fruts_get_next(cur_fruts_p);
        if ((1 << cur_fruts_p->position) & pos_bitmask)
        {
            /* Found a fruts with a position indicated
             * by the bitmap.
             * Invoke fn on this fruts.
             */
            pos_bitmask &= ~(1 << cur_fruts_p->position);
            fbe_raid_fruts_send_timeout_usurper(cur_fruts_p);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
        }
        cur_fruts_p = next_fruts_p;
    }
    return FBE_STATUS_OK;
 }  
/******************************************
 * end fbe_raid_fruts_chain_send_timeout_usurper()
 ******************************************/

/*!**************************************************************
 * fbe_raid_fruts_send_timeout_usurper()
 ****************************************************************
 * @brief
 *  This function sends a timeout usurper to a given position.
 *
 * @param  fruts_p - Fruts to send command for.
 *
 * @return fbe_status_t  FBE_STATUS_OK if usurpers were sent.
 *                       other status implies failure sending usurpers.
 *
 * @author
 *  2/4/2011 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_fruts_send_timeout_usurper(fbe_raid_fruts_t * fruts_p)
{
    fbe_status_t status;

    /* Initialize the packet within the fruts.
     */
    if (!fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        status = fbe_raid_fruts_initialize_transport_packet(fruts_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s packet initialize status %d\n",
                    __FUNCTION__, status);
            return status;
        }
    }

    /* We setup the logical error structure with this timeout flavor of error. 
     * We also setup with the object id of the PDO which took this error. 
     * This ensures that we do not send this usurper to a PDO which might have 
     * been swapped in by the lower level. 
     */
    fruts_p->logical_error.error_type = FBE_BLOCK_TRANSPORT_ERROR_TYPE_TIMEOUT;
    fruts_p->logical_error.pvd_object_id = fruts_p->pvd_object_id;

    status = fbe_raid_send_usurper(fruts_p, 
                                   FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS,
                                   &fruts_p->logical_error,
                                   sizeof(fbe_block_transport_control_logical_error_t),
                                   /* Needed to make it to pdo.
                                    * If an object does not understand this packet (like pvd and LDO),  
                                    * it will send it to the next level.  
                                    */
                                   FBE_PACKET_FLAG_TRAVERSE );
    return status;
}
/**************************************
 * end fbe_raid_fruts_send_timeout_usurper()
 **************************************/
/*!**************************************************************
 * fbe_raid_send_usurper()
 ****************************************************************
 * @brief
 *  This function sends a usurper command to an edge
 *  asynchronously.
 *
 * @param fruts_p - fruts to send.
 * @param control_code  - control code to be sent.
 * @param buffer - the payload control buffer
 * @param buffer_length - payload control buffer length
 * @param attr - Packet attributes

 * @return fbe_status_t - FBE_STATUS_OK on success. 
 *                        FBE_STATUS_GENERIC_FAILURE on error.
 *
 * @author
 *  2/3/2011 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_send_usurper(fbe_raid_fruts_t *fruts_p,
                                          fbe_payload_control_operation_opcode_t control_code,
                                          fbe_payload_control_buffer_t buffer,
                                          fbe_payload_control_buffer_length_t buffer_length,
                                          fbe_packet_attr_t attr)
{
    fbe_packet_t *packet_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_raid_siots_t *siots_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_block_edge_t *block_edge_p = NULL;
    
    if (fruts_p == NULL) 
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING, "%s fruts is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    packet_p = fbe_raid_fruts_get_packet(fruts_p);
    siots_p = fbe_raid_fruts_get_siots(fruts_p);
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Raid always sends to the downstream edge, which is connected with a sep object,
     * so use a sep payload.
     */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    fbe_payload_control_build_operation(control_operation_p, control_code, buffer, buffer_length);
    fbe_transport_set_packet_attr(packet_p, attr);

    fbe_transport_set_completion_function(packet_p,
                                          fbe_raid_fruts_usurper_packet_completion,
                                          fruts_p);

    fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, fruts_p->position);

    /* Send packet.  Our callback is always called.
     */
    fbe_block_transport_send_control_packet(block_edge_p, packet_p);

    return FBE_STATUS_OK;
}
/********************************************************************
 * end fbe_raid_send_usurper()
 ********************************************************************/

/*!***************************************************************
 * fbe_raid_fruts_usurper_packet_completion()
 ****************************************************************
 * 
 * @brief   This is the completion for a usurper sent to the edge.
 *          We assume the caller set the wait count in the siots
 *          before launching this packet.
 * 
 * @param packet_p - The packet being completed.
 * @param context - The fruts ptr.
 *
 * @return fbe_status_t FBE_STATUS_MORE_PROCESSING_REQUIRED
 *                      Since this is the last completion function
 *                      in this packet we need to return this status.
 *
 * @author
 *  2/4/2011 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_fruts_usurper_packet_completion(fbe_packet_t * packet_p, 
                                                             fbe_packet_completion_context_t context)
{
    fbe_raid_fruts_t   *fruts_p = NULL;
    fbe_raid_siots_t   *siots_p = NULL;
    fbe_raid_geometry_t   *raid_geometry_p = NULL;
    fbe_object_id_t rg_object_id;
    fbe_block_edge_t *block_edge_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_status_t pkt_status;
    fbe_payload_control_status_t control_status;

    /* First get and validate the fruts associated with this packet.
     * Although we could use the packet itself we use the completion context.
     */
    fruts_p = (fbe_raid_fruts_t *)context;
    if (RAID_FRUTS_COND((fruts_p == NULL) ||
                        !(fbe_raid_common_get_flags(&fruts_p->common) & FBE_RAID_COMMON_FLAG_TYPE_FRU_TS) ) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: %s errored: fruts_p is NULL or flags 0x%x is wrong or evaluate state NULL\n",
                               __FUNCTION__, 
                               fruts_p->common.flags);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Now get the raid group associated with the request.
     */
    siots_p = fbe_raid_fruts_get_siots(fruts_p);
    if (RAID_FRUTS_COND((siots_p == NULL) ||
                        (siots_p->common.state == (fbe_raid_common_state_t)fbe_raid_siots_freed) ||
                        (siots_p->wait_count == 0) ) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: %s errored: siots_p is null or common state 0x%p is wrong or wait count 0x%llx == 0\n",
                               __FUNCTION__, 
                               siots_p->common.state, 
                               siots_p->wait_count);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    if (RAID_FRUTS_COND(raid_geometry_p == NULL) )
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                               "raid: %s failed to get raid geometry: raid_geometry_p is null : siots_p = 0x%p\n",
                               __FUNCTION__,
                               siots_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Make sure we still have something outstanding.
     * We do a lower and upper range check.  The upper range check is fairly arbitrary, 
     * but it is unlikely that we will have more than twice the nubmer of drives worth of 
     * operations outstanding at this time. 
     */
    if ((siots_p->wait_count == 0) || (siots_p->wait_count > (fbe_raid_siots_get_width(siots_p) * 2)))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "%s siots_p %p has invalid wait count: %lld\n", __FUNCTION__,
                             siots_p, siots_p->wait_count);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* If the siots state is freed, then something is very wrong.
     */
    if (siots_p->common.state == (fbe_raid_common_state_t)fbe_raid_siots_freed)
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "%s siots_p %p state is freed\n", __FUNCTION__, siots_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Copy the packet status to the fruts.
     */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    if (sep_payload_p == NULL)
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "%s fruts %p packet %p has null sep_payload\n",
                             __FUNCTION__, fruts_p, fbe_raid_fruts_get_packet(fruts_p));
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    if (control_operation_p == NULL)
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "%s fruts %p packet %p has a null control operation\n",
                             __FUNCTION__, fruts_p, fbe_raid_fruts_get_packet(fruts_p));
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    rg_object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, fruts_p->position);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    pkt_status = fbe_transport_get_status_code(&fruts_p->io_packet);
    
    if ((pkt_status != FBE_STATUS_OK) ||
        (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {

        fbe_raid_library_object_trace(fruts_p->position, 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                      "set I/O err failed pdo_obj: 0x%x pos: %d pkt_st: 0x%x cnt_st: 0x%x\n",
                                     fruts_p->pvd_object_id,
                                     fruts_p->position,
                                     pkt_status, control_status);

    }

    /* We always release the control operation since 
     * there is likely still an active block operation that we will 
     * want to look at to interpret status. 
     */
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);

    /* Now check if all requests we sent are done.
     */
    /* Lockless siots */
    //fbe_raid_siots_lock(siots_p);
    if (fbe_atomic_decrement(&siots_p->wait_count) == 0)
    {
        fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
        /* Lockless siots */
        //fbe_raid_siots_unlock(siots_p);

        if (fbe_raid_siots_is_monitor_initiated_op(siots_p))
        {
            /* Siots needs to be restarted to complete this siots as failed. 
             * We cannot wait as a monitor initated operation. 
             */
            fbe_raid_common_state((fbe_raid_common_t *)siots_p);
        }
        else
        {
            /* All requests completed.
             * Since sending the requests cause the drive to go away and for the 
             * monitor to get an event, just set waiting shutdown continue, so that 
             * the monitor will know to eventually kick us off. 
             */
            fbe_raid_iots_lock(iots_p);
            fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE);
            fbe_raid_iots_unlock(iots_p);
        }
    }
    else
    {
        /* Lockless siots */
        //fbe_raid_siots_unlock(siots_p);
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_fruts_usurper_packet_completion
 **************************************/

/*!***************************************************************************
 *          fbe_raid_fruts_check_degrade_corrupt_data()
 *****************************************************************************
 *
 * @brief   If the corrupt crc op writes to a degraded position, we would fail
 *          with a media error. The corrupt crc is an engineer testing only operation.
 *          A media error won't trigger ill effect in upper layer drivers.
 *  
 * @param   fruts_p - The head of the fruts chain. 
 * @param   siots_p - The sub request.
 *
 * @return  status - FBE_TRUE - we are writing to degraded position.
 *                   FBE_FALSE - no
 * 
 * @author
 *  12/15/2010 - Created. Ruomu Gao
 *
 ****************************************************************/
fbe_bool_t fbe_raid_fruts_check_degraded_corrupt_data(fbe_raid_fruts_t *fruts_p,
                                                     fbe_raid_siots_t *siots_p)
{
    fbe_raid_fruts_t *cur_fruts_p = fruts_p;
    fbe_bool_t b_on_degraded = FBE_FALSE;
    fbe_u32_t first_dead_pos = fbe_raid_siots_dead_pos_get( siots_p, FBE_RAID_FIRST_DEAD_POS );
    fbe_u32_t second_dead_pos = fbe_raid_siots_dead_pos_get( siots_p, FBE_RAID_SECOND_DEAD_POS );

    if (first_dead_pos == FBE_RAID_INVALID_DEAD_POS)
    {
        /* There is no dead drive. The check is not needed. */
        return b_on_degraded;
    }

    /* Loop through all the write fruts to see if we hit the degraded position.
     * We will transition to media error if we do.
     */
    while ( cur_fruts_p )
    {
        if ( (cur_fruts_p->position == siots_p->parity_pos) ||
             (fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) &&
             cur_fruts_p->position == siots_p->parity_pos + 1) )
        {
            /* For corrupt data, we don't write to parity, so it is ok. */
        }
        else if ( cur_fruts_p->position == first_dead_pos ||
                  cur_fruts_p->position == second_dead_pos )
        {
            /* The purposed of corrupt data is to modify only the data position.
             * If the user attempted to modify a degraded/removed position and
             * we are going to fail the request, trace some information.
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                 "raid: write pos: %d lba: 0x%llx blks: 0x%llx parity pos: %d dead: %d dead2: %d \n",
                                 cur_fruts_p->position, cur_fruts_p->lba, cur_fruts_p->blocks, 
                                 siots_p->parity_pos, first_dead_pos, second_dead_pos);
            b_on_degraded = FBE_TRUE;
            break;
        }

        cur_fruts_p = fbe_raid_fruts_get_next( cur_fruts_p );
    }

    return b_on_degraded;
}
/********************************************************************
 * end fbe_raid_fruts_check_degraded_corrupt_data()
 ********************************************************************/

/*!***************************************************************************
 *          fbe_raid_fruts_to_master_packet()
 *****************************************************************************
 *
 * @brief   This function returns pointer to master packet for given fruts
 *  
 * @param   fruts_p - The head of the fruts chain. 
 *
 * @return  Pointer to master packet
 * 
 * @author
 *  08/04/2011 - Created. Peter Puhov
 *
 ****************************************************************/
static fbe_packet_t * fbe_raid_fruts_to_master_packet(fbe_raid_fruts_t *fruts_p)
{
    fbe_raid_siots_t   *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_raid_iots_t *iots_p =  fbe_raid_siots_get_iots(siots_p);
    fbe_packet_t *master_p = fbe_raid_iots_get_packet(iots_p);

    return master_p;
}
/********************************************************************
 * end fbe_raid_fruts_to_master_packet()
 ********************************************************************/

/*!***************************************************************************
 *          fbe_raid_fruts_increment_eboard_counters()
 *****************************************************************************
 *
 * @brief   Increment the fruts counters based on the eboard counts
 *  
 * @param   fru_eboard - pointer to eboard counter to look at.
 * @param   fruts_p - furts to increment counters in. 
 *
 * @return  Pointer to master packet
 * 
 * @author
 *  01/13/2012 - Created. Shay Harel
 *
 ****************************************************************/
fbe_status_t fbe_raid_fruts_increment_eboard_counters(fbe_raid_fru_eboard_t *fru_eboard, fbe_raid_fruts_t *fruts_p)
{
	/*error couts needs to be updated*/
	if (fru_eboard->hard_media_err_count > 0) {
		fbe_raid_fruts_inc_error_count(fruts_p, FBE_RAID_FRU_ERROR_TYPE_HARD_MEDIA);
	}

	if (fru_eboard->soft_media_err_count > 0) {
		fbe_raid_fruts_inc_error_count(fruts_p, FBE_RAID_FRU_ERROR_TYPE_SOFT_MEDIA);
	}

	return FBE_STATUS_OK;

}
/********************************************************************
 * end fbe_raid_fruts_increment_eboard_counters()
 ********************************************************************/

/*!**************************************************************
 * fbe_raid_fruts_chain_get_errors_injected()
 ****************************************************************
 * @brief
 *  It will return true if for any of the fruts in a siots, have
 *  errors injected via error injection service.
 * 
 *  In this case, we should not send usurper to PDO since
 *  errors are not real one, but the injected one
 *
 * @param fruts_p - Chain of fruts to check into.
 *
 * @return fbe_status_t  True or False
 *
 * @author
 *  2/27/2012 - Created. Swati Fursule
 *
 ****************************************************************/

fbe_bool_t fbe_raid_fruts_chain_get_errors_injected(fbe_raid_fruts_t *fruts_p)
{
    fbe_raid_fruts_t *cur_fruts_p = NULL;
    fbe_raid_fruts_t *next_fruts_p = NULL;


    cur_fruts_p = fruts_p;
    /* Keep looping until we either hit the end of the chain 
     */
    while(cur_fruts_p != NULL)
    {
        next_fruts_p = fbe_raid_fruts_get_next(cur_fruts_p);
        
        if (fbe_raid_fruts_is_flag_set(cur_fruts_p, FBE_RAID_FRUTS_FLAG_ERROR_INJECTED)) 
        {
             return FBE_TRUE;
        }
        cur_fruts_p = next_fruts_p;
    }
    return FBE_FALSE;
 }  
/******************************************
 * end fbe_raid_fruts_chain_get_errors_injected()
 ******************************************/

/*!**************************************************************
 * fbe_raid_siots_send_crc_usurper()
 ****************************************************************
 * @brief
 *  Send out the CRC usurper for the single bit or multi bit crc errors
 *  in the current chain. This CRC usurper is sent directly to pdo so 
 *  that pdo can take action e.g. killing drive etc
 * 
 *
 * @param siots_p - Current I/O.
 * @param fruts_p - Chain of fruts to send usurpers for.               
 *
 * @return fbe_status_t FBE_STATUS_OK if usurpers were sent
 *                      FBE_STATUS_GENERIC_FAILURE if no
 *                      crc errors found in the chain
 *                      and thus no usurpers need to be sent.
 *
 * @author
 *  1/27/2012 - Created. Swati Fursule
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_send_crc_usurper(fbe_raid_siots_t *siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_position_bitmask_t degraded_bitmask = 0;
    fbe_raid_position_bitmask_t crc_bitmask = 0;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_fruts_t * fruts_p = NULL;
    fbe_raid_position_bitmask_t multi_bitmap;

    fbe_raid_siots_get_multi_bit_crc_bitmap(siots_p, &multi_bitmap);
    fbe_raid_siots_get_read_fruts(siots_p, &fruts_p);

    /* If we have CRC errors then inform the lower level of the CRC errors.
     */
    crc_bitmask = siots_p->vrts_ptr->eboard.crc_single_bitmap | multi_bitmap;
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);

    /*NO need to send CRC usurper for dead position */
    crc_bitmask &= ~(degraded_bitmask);
    
    /* Task TA893
     * We do not need to send usurper in case errors were injected.
     * In some cases, we need to send usurper although error were injected, if asked by user.
     */
    if( (fbe_raid_fruts_chain_get_errors_injected(fruts_p)) && 
        (!fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_SEND_CRC_ERR_TO_PDO))
       )
    {
        /* No CRC usurpers to send.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
           "no CRC usurper send to pdo, rg: 0x%x since errors are injected and debug flag not set to SEND_CRC_ERR_TO_PDO\n",
           fbe_raid_geometry_get_object_id(raid_geometry_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* If this is a BVA I/O or the LUN is scheduled for Background Verify,
     * do not inform PDO, as the CRC error might be due to an interrupted write,
     * rather than a drive problem.*/
    if ( (crc_bitmask != 0) && (fruts_p != NULL) && !(fbe_raid_siots_is_bva_verify(siots_p)))
    {

        /* The siots wait count is set for the fruts we will wait for.
         */
        siots_p->wait_count = fbe_raid_get_bitcount(crc_bitmask);

        /* Kick off the usurper packets to inform the lower level. 
         * When the packets complete, this siots will go waiting. 
         */
        status = fbe_raid_fruts_chain_send_crc_usurper(fruts_p, crc_bitmask);
        /*status = FBE_STATUS_OK;*/
    }
    else
    {
        /* No CRC usurpers to send.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "no CRC errors rg: 0x%x\n",
                                    fbe_raid_geometry_get_object_id(raid_geometry_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_raid_siots_send_crc_usurper 
 **************************************/


/*!**************************************************************
 * fbe_raid_fruts_chain_send_crc_usurper()
 ****************************************************************
 * @brief
 *  Send CRC usurpers for positions that have single bit or 
 * multi bit CRC errors
 * 
 *  Raid will send this when it believes that the drive is
 *  taking physical errors and is taking too long to process an I/O.
 *
 * @param fruts_p - Chain of fruts to retry.
 * @param pos_bitmask - bitmask of positions to send on.
 *
 * @return fbe_status_t  FBE_STATUS_OK if usurpers were sent.
 *                       other status implies failure sending usurpers.
 *
 * @author
 *  1/27/2012 - Created. Swati Fursule
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_chain_send_crc_usurper(fbe_raid_fruts_t *fruts_p,
                                                   fbe_raid_position_bitmask_t pos_bitmask)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_fruts_t *cur_fruts_p = NULL;
    fbe_raid_fruts_t *next_fruts_p = NULL;
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_block_transport_error_type_t error_type;

    if (pos_bitmask == 0)
    {
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        fbe_object_id_t rg_object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "crc bitmap is 0 obj: 0x%x fruts: %p siots: %p\n",
                                      rg_object_id, fruts_p, siots_p);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    cur_fruts_p = fruts_p;
    /* Keep looping until we either hit the end of the chain or
     * have no more to do. 
     */
    while((pos_bitmask != 0) && (cur_fruts_p != NULL))
    {
        next_fruts_p = fbe_raid_fruts_get_next(cur_fruts_p);
        if ((1 << cur_fruts_p->position) & pos_bitmask)
        {
            if(siots_p->vrts_ptr->eboard.crc_single_bitmap & pos_bitmask)
            {
                error_type = FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC_SINGLE_BIT;
            }
            else 
            {
                error_type = FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC_MULTI_BITS;
            }
            /* Found a fruts with a position indicated
             * by the bitmap.
             * Invoke fn on this fruts.
             */
            pos_bitmask &= ~(1 << cur_fruts_p->position);

            status = fbe_raid_fruts_send_crc_usurper(cur_fruts_p, error_type);

        }
        cur_fruts_p = next_fruts_p;
    }

    return FBE_STATUS_OK;
 }  
/******************************************
 * end fbe_raid_fruts_chain_send_crc_usurper()
 ******************************************/
/*!**************************************************************
 * fbe_raid_fruts_send_crc_usurper()
 ****************************************************************
 * @brief
 *  This function sends a CRC usurper to a given position.
 *
 * @param  fruts_p - Fruts to send command for.
 *
 * @return fbe_status_t  FBE_STATUS_OK if usurpers were sent.
 *                       other status implies failure sending usurpers.
 *
 * @author
 *  1/27/2012 - Created. Swati Fursule
 *
 ****************************************************************/
static fbe_status_t fbe_raid_fruts_send_crc_usurper(fbe_raid_fruts_t * fruts_p,
                                                    fbe_block_transport_error_type_t error_type)
{
    fbe_status_t status;
    fbe_packet_t* packet_p = NULL;
    /* Initialize the packet within the fruts.
     */
    if (!fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_PACKET_INITIALIZED))
    {
        status = fbe_raid_fruts_initialize_transport_packet(fruts_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s packet initialize status %d\n",
                                   __FUNCTION__, status);
            return status;
        }
    }

    /* We setup the logical error structure with this timeout flavor of error. 
     * We also setup with the object id of the PDO which took this error. 
     * This ensures that we do not send this usurper to a PDO which might have 
     * been swapped in by the lower level. 
     */
    fruts_p->logical_error.error_type = error_type;
    fruts_p->logical_error.pvd_object_id = fruts_p->pvd_object_id;
    packet_p = fbe_raid_fruts_get_packet(fruts_p);
    status = fbe_raid_fruts_usurper_send_control_packet_to_pdo(packet_p,
                                                               FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS,
                                                               &fruts_p->logical_error,
                                                               sizeof(fbe_block_transport_control_logical_error_t),
                                                               fruts_p->pvd_object_id, 
                                                               FBE_PACKET_FLAG_TRAVERSE,
                                                               fruts_p,
                                                               FBE_PACKAGE_ID_SEP_0);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                     "raid: send to pdo fruts_p: %p pos: %d failed update stats to pvd obj: 0x%x status: 0x%x \n",
                                     fruts_p, fruts_p->position, fruts_p->pvd_object_id, status);
    }

    return status;
}
/**************************************
 * end fbe_raid_fruts_send_crc_usurper()
 **************************************/

/*!**************************************************************
 * fbe_raid_fruts_block_edge_has_timeout_errors()
 ****************************************************************
 * @brief
 *  Return TRUE if the edge has the timeout errors attribute set.
 *
 * @param fruts_p - The current I/O. 
 *
 * @return FBE_TRUE if the attribute is set, FBE_FALSE otherwise.
 *
 * @author
 *  5/25/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_bool_t fbe_raid_fruts_block_edge_has_timeout_errors( fbe_raid_fruts_t *fruts_p)
{
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_block_edge_t *block_edge_p = NULL;
    fbe_raid_siots_t *siots_p = NULL;
    fbe_path_attr_t path_attr;
    
    if (fruts_p == NULL) 
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING, "%s fruts is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    siots_p = fbe_raid_fruts_get_siots(fruts_p);
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, fruts_p->position);
    fbe_block_transport_get_path_attributes(block_edge_p, &path_attr);

    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS)
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}
/******************************************
 * end fbe_raid_fruts_block_edge_has_timeout_errors()
 ******************************************/


/*!***************************************************************
 * fbe_raid_reaffine_waited_siots()
 ****************************************************************
 * @brief
 *  This function is called after a waiting siots is sent to the proper cpu queue, and then
 *  is woken up on that cpu to be run.
 *  
 * @param packet_p - the packet that was put on the cpu run queue
 *        context - ptr to the current fruts being resumed
 * 
 * @return void
 *
 * @author
 *  6/12/12 - Created. Dave Agans
 *
 ****************************************************************/
fbe_status_t fbe_raid_reaffine_waited_siots(fbe_packet_t *packet_p, 
                                            fbe_packet_completion_context_t context)
{
    fbe_raid_fruts_t *fruts_p = (fbe_raid_fruts_t *)context;
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);

    /* Execute Siots next state */
    fbe_raid_common_state((fbe_raid_common_t *)siots_p);

    return(FBE_STATUS_MORE_PROCESSING_REQUIRED);
}



/*!***************************************************************
 * fbe_raid_fruts_init_cpu_affinity_packet()
 ****************************************************************
 * @brief
 *  This function initializes a fruts packet to allow the siots to be re-queued on the proper cpu.
 *  It only initializes what is needed, since there is no IO associated with it.  The fruts can be
 *  reused for its intended io after the siots is executing on the proper cpu.
 *  
 * @param context - ptr to the fruts to be initialized
 * 
 * @return fbe_status_t - FBE_STATUS_OK if worked, error if not
 *
 * @author
 *  6/12/12 - Created. Dave Agans
 *
 ****************************************************************/
fbe_status_t fbe_raid_fruts_init_cpu_affinity_packet(fbe_raid_fruts_t *fruts_p)
{
    fbe_status_t   fbe_status = FBE_STATUS_OK;
    fbe_packet_t   *packet_p = NULL;
    fbe_packet_t   *master_packet_p = NULL;

    master_packet_p = fbe_raid_fruts_to_master_packet(fruts_p);

    /* Initialize the packet within the fruts.
     */
    fbe_status = fbe_raid_fruts_initialize_transport_packet(fruts_p);
    if (fbe_status != FBE_STATUS_OK)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: %s cpu affinity packet initialize status %d\n",
                               __FUNCTION__, fbe_status);
        return fbe_status;
    }

    /* Get the packet associated with this fruts.
     */
    packet_p = fbe_raid_fruts_get_packet(fruts_p);

    /* Pass packet priority
     */
    fbe_transport_get_packet_priority(master_packet_p, &packet_p->packet_priority);

    /* Now set callback.
     */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_raid_reaffine_waited_siots,
                                          fruts_p);

    return fbe_status;
}

/*!**************************************************************
 * fbe_raid_fruts_get_stats()
 ****************************************************************
 * @brief
 *  Fetch the statistics for this chain of fruts.
 *
 * @param None.       
 *
 * @param fruts_p
 * @param stats_p
 *
 * @author
 *  4/29/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_fruts_get_stats(fbe_raid_fruts_t *fruts_p,
                              fbe_raid_fru_statistics_t *stats_p)
{
    fbe_raid_fruts_t *current_fruts_p = NULL;
    for (current_fruts_p = fruts_p;
         current_fruts_p != NULL;
         current_fruts_p = fbe_raid_fruts_get_next(current_fruts_p))
    {
        /* Skip fruts that are not initialized or not outstanding.
         */
        if ((current_fruts_p->position >= FBE_RAID_MAX_DISK_ARRAY_WIDTH) ||
            ((current_fruts_p->flags & FBE_RAID_FRUTS_FLAG_REQUEST_OUTSTANDING) == 0) ||
            (current_fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID)) {
            continue;
        }
        switch (current_fruts_p->opcode)
        {
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
                stats_p[current_fruts_p->position].read_count++;
                break;
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
                stats_p[current_fruts_p->position].write_count++;
                break;
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
                stats_p[current_fruts_p->position].write_verify_count++;
                break;
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
                stats_p[current_fruts_p->position].zero_count++;
                break;
            default:
                stats_p[current_fruts_p->position].unknown_count++;
                stats_p[current_fruts_p->position].unknown_opcode = current_fruts_p->opcode;
                break;
        };
    }
}
/******************************************
 * end fbe_raid_fruts_get_stats()
 ******************************************/

/*!**************************************************************
 * fbe_raid_fruts_get_stats_filtered()
 ****************************************************************
 * @brief
 *  Fetch the statistics for this chain of fruts.
 *
 * @param None.       
 *
 * @param fruts_p
 * @param stats_p
 *
 * @author
 *  4/29/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_fruts_get_stats_filtered(fbe_raid_fruts_t *fruts_p,
                                       fbe_raid_library_statistics_t *stats_p)
{
    fbe_raid_fruts_t *current_fruts_p = NULL;
    fbe_raid_fru_statistics_t *fru_stats_p = &stats_p->user_fru_stats[0];

    for (current_fruts_p = fruts_p;
         current_fruts_p != NULL;
         current_fruts_p = fbe_raid_fruts_get_next(current_fruts_p))
    {
        fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);
        fbe_lba_t   journal_log_start_lba = FBE_LBA_INVALID;
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_log_start_lba);

        /* Skip fruts that are not initialized or not outstanding.
         */
        if ((current_fruts_p->position >= FBE_RAID_MAX_DISK_ARRAY_WIDTH) ||
            ((current_fruts_p->flags & FBE_RAID_FRUTS_FLAG_REQUEST_OUTSTANDING) == 0) ||
            (current_fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID)) {
            continue;
        }
        /* Increment counts for specific lba area.
         */
        if (fbe_raid_siots_is_metadata_operation(siots_p)){
            fru_stats_p = &stats_p->metadata_fru_stats[0];
        }
        else if (current_fruts_p->lba >= journal_log_start_lba){
            fru_stats_p = &stats_p->journal_fru_stats[0];
        }
        fbe_raid_fruts_get_stats(fruts_p, fru_stats_p);
        if (siots_p->parity_pos < FBE_RAID_MAX_DISK_ARRAY_WIDTH){
            /* Increment the position of parity that we hit with this request. 
             * First increment the "area" specific stat. 
             * Than increment the totals stat. 
             */
            fru_stats_p[siots_p->parity_pos].parity_count++;
            stats_p->total_fru_stats[siots_p->parity_pos].parity_count++;
        }
        return;
    }
}
/**************************************
 * end fbe_raid_fruts_get_stats_filtered
 **************************************/

/*************************
 * end file fbe_raid_fruts.c
 *************************/
