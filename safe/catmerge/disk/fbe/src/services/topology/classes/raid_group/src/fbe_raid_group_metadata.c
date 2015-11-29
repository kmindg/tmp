/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_metadata.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the raid group object's s metadata related functionality.
 * 
 *
 * @ingroup raid_group_class_files
 * 
 * @version
 *   02/04/2010:  Created. Dhaval Patel
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_raid_group_object.h"
#include "fbe_raid_group_bitmap.h"
#include "fbe/fbe_raid_group_metadata.h"                  //  this file's .h file 

/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/

static fbe_status_t fbe_raid_group_metadata_write_default_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                                    fbe_packet_completion_context_t context);

static fbe_status_t fbe_raid_group_metadata_zero_metadata_completion(fbe_packet_t * packet_p,
                                                                     fbe_packet_completion_context_t context);


/***************
 * FUNCTIONS
 ***************/

/*!****************************************************************************
 * fbe_raid_group_initialize_metadata_positions_with_default_values()
 ******************************************************************************
 * @brief
 *  This function is used to initialize metadata positions with default 
 *  values.
 * 
 * @param raid_group_metadata_positions_p  - Pointer to the metadata position.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  3/24/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_raid_group_initialize_metadata_positions_with_default_values(fbe_raid_group_metadata_positions_t * raid_group_metadata_positions_p)
{
    /* If input paramenter is null then return error. */
    if(raid_group_metadata_positions_p == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize with default values. */
    raid_group_metadata_positions_p->paged_metadata_capacity = FBE_LBA_INVALID;
    raid_group_metadata_positions_p->paged_metadata_block_count = 0;
    raid_group_metadata_positions_p->paged_metadata_lba = FBE_LBA_INVALID;
    raid_group_metadata_positions_p->paged_mirror_metadata_offset = FBE_LBA_INVALID;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_metadata_init_records()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_get_metadata_positions()
 ******************************************************************************
 * @brief
 *  This function is used to get the metadata position from the object.
 *
 * @param raid_group_p                     - raid group object.  
 * @param raid_group_metadata_positions_p  - Pointer to the metadata position.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  3/24/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t  
fbe_raid_group_get_metadata_positions(fbe_raid_group_t *  raid_group_p,
                                      fbe_raid_group_metadata_positions_t * raid_group_metadata_positions_p)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lba_t               exported_capacity = FBE_LBA_INVALID;  
    fbe_lba_t               user_blocks_per_chunk = FBE_LBA_INVALID;  
    fbe_u16_t               data_disks = 0;
    fbe_u32_t               width = 0;
    fbe_raid_group_type_t   raid_type = FBE_RAID_GROUP_TYPE_UNKNOWN;
    fbe_raid_geometry_t *   raid_geometry_p = NULL;
    
    /* Get the exported capacity of the raid group object. */
    fbe_base_config_get_capacity((fbe_base_config_t *) raid_group_p, &exported_capacity);

    /* Get the geometry of the raid group object. */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    /* Get the raid type of the object. */
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    fbe_raid_type_get_data_disks(raid_type, width, &data_disks);


    /* Round the exported capacity up to a chunk before calculating metadata capacity. */
    user_blocks_per_chunk = FBE_RAID_DEFAULT_CHUNK_SIZE * data_disks;
    if (exported_capacity % user_blocks_per_chunk) {
        exported_capacity += user_blocks_per_chunk - (exported_capacity % user_blocks_per_chunk);
    }

    /* Get the metadata position of the raid object. */
    status = fbe_raid_group_class_get_metadata_positions(exported_capacity,
                                                         FBE_LBA_INVALID,
                                                         data_disks,
                                                         raid_group_metadata_positions_p,
                                                         raid_type);
    return status;
}
/******************************************************************************
 * end fbe_raid_group_get_metadata_positions()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_metadata_initialize_metadata_element()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the metadata record for the RAID
 *   group object.
 *
 * @param raid_group_p         - pointer to the raid group
 * @param packet_p             - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   12/16/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_raid_group_metadata_initialize_metadata_element(fbe_raid_group_t * raid_group_p,
                                                    fbe_packet_t * packet_p)
{
    fbe_raid_group_metadata_positions_t    metadata_positions;
    fbe_status_t                           status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                        payload_p = NULL;
    fbe_payload_control_operation_t *      control_operation_p = NULL;
    fbe_raid_geometry_t *                  raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u64_t                              stripe_number = 0;
    fbe_u64_t                              number_of_stripes = 0;
    fbe_u64_t                              number_of_user_stripes = 0;
    fbe_u64_t                              stripe_count = 0;
    fbe_u64_t                              user_stripe_count = 0;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Get the metadata position for the raid object. */
    status = fbe_raid_group_get_metadata_positions(raid_group_p, &metadata_positions);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s metadata position calculation failed, status:0x%x.\n",
                              __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* debug trace to track the paged metadata and signature data positions. */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s metadata position, paged_lba:0x%llx, mir_off:0x%llx\n",
                          __FUNCTION__,
                          (unsigned long long)metadata_positions.paged_metadata_lba,
                          (unsigned long long)metadata_positions.paged_mirror_metadata_offset);

    /* initialize metadata element with metadata position information. */
    fbe_base_config_metadata_set_paged_record_start_lba((fbe_base_config_t *) raid_group_p,
                                                        metadata_positions.paged_metadata_lba);

    fbe_base_config_metadata_set_paged_mirror_metadata_offset((fbe_base_config_t *) raid_group_p,
                                                              metadata_positions.paged_mirror_metadata_offset);

    fbe_base_config_metadata_set_paged_metadata_capacity((fbe_base_config_t *) raid_group_p,
                                                        metadata_positions.paged_metadata_capacity);

    /* The total number of stripes is determined by the total raid group capacity.
     * We use the last accessible lba and then add one to get the total stripe count.
     */
    fbe_raid_geometry_calculate_lock_range(raid_geometry_p,
                                           (metadata_positions.raid_capacity - 1),
                                           1,
                                           &stripe_number,
                                           &stripe_count);
    number_of_stripes = stripe_number + 1;
    fbe_base_config_metadata_set_number_of_stripes((fbe_base_config_t *) raid_group_p,
                                                   number_of_stripes);
    /* The total number of stripes is determined by the total usable raid group capacity.
     * We use the last accessible lba and then add one to get the total stripe count.
     */
    fbe_raid_geometry_calculate_lock_range(raid_geometry_p,
                                           (metadata_positions.paged_metadata_lba - 1),
                                           1,
                                           &number_of_user_stripes,
                                           &user_stripe_count);
    number_of_user_stripes ++;
    fbe_base_config_metadata_set_number_of_private_stripes((fbe_base_config_t *) raid_group_p,
                                                            ( number_of_stripes - number_of_user_stripes));

    /* complete the packet with good status here. */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_metadata_initialize_metadata_element()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_nonpaged_metadata_init()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the nonpaged metadata memory of the
 *  raid group object.
 *
 * @param raid_group_p         - pointer to the raid group
 * @param packet_p             - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/01/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_nonpaged_metadata_init(fbe_raid_group_t * raid_group_p,
                                      fbe_packet_t *packet_p)
{
    fbe_status_t status;

    /* call the base config nonpaged metadata init to initialize the metadata. */
    status = fbe_base_config_metadata_nonpaged_init((fbe_base_config_t *) raid_group_p,
                                                    sizeof(fbe_raid_group_nonpaged_metadata_t),
                                                    packet_p);
    return status;
}
/******************************************************************************
 * end fbe_raid_group_nonpaged_metadata_init()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_metadata_zero_metadata()
 ******************************************************************************
 * @brief
 *  This function is used to start the zeroing for the consumed area of the 
 *  raid group object, raid object consumes metadata area and export rest of 
 *  the capacity for the block transport server.
 *
 * @param lun_p         - pointer to the LUN object.
 * @param packet_p      - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   4/15/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_raid_group_metadata_zero_metadata(fbe_raid_group_t * raid_group_p,
                                      fbe_packet_t * packet_p)
{
    fbe_status_t                        status;
    fbe_raid_group_metadata_positions_t raid_group_metadata_positions;
    fbe_payload_block_operation_t      *block_operation_p;  // block operation
    fbe_optimum_block_size_t            optimum_block_size; // optimium block size
    fbe_payload_ex_t *                 sep_payload_p;      // sep payload
    fbe_raid_geometry_t *               raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_lba_t                           metadata_capacity = 0;
    fbe_u16_t data_disks;

    /* Get the metadata positions.
     */
    fbe_raid_group_get_metadata_positions(raid_group_p, &raid_group_metadata_positions);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    /* The metadata capacity is the total of the metadata and any journal capacity.
     */
    metadata_capacity = raid_group_metadata_positions.paged_metadata_capacity + 
                            raid_group_metadata_positions.journal_write_log_physical_capacity * data_disks;

    /* get optimum block size for this i/o request */
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &optimum_block_size);

    /* get the sep payload */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    /*set-up block operation in sep payload */
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);

    /* Send the zero request which covers the metadata area for the raid group object. 
     * The zero request is a logical operation.
     */
    fbe_payload_block_build_operation(block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO,
                                      raid_group_metadata_positions.paged_metadata_lba,
                                      (fbe_block_count_t) metadata_capacity,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      optimum_block_size,
                                      NULL);
    fbe_payload_block_set_flag(block_operation_p, (FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_METADATA_OP | 
                                                 FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_NO_SL_THIS_LEVEL));

    /* Set the completion function for the zeroing block operation. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_raid_group_metadata_zero_metadata_completion,
                                          raid_group_p);

    /* now, activate this block operation */
    fbe_payload_ex_increment_block_operation_level(sep_payload_p);

    /* invoke bouncer to forward i/o request to downstream object */
    status = fbe_base_config_bouncer_entry((fbe_base_config_t *)raid_group_p, packet_p);
    return status;
}
/******************************************************************************
 * end fbe_raid_group_metadata_zero_metadata()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_metadata_zero_metadata_completion()
 ******************************************************************************
 * @brief
 *  This function is used as a completion function for the zeroing of the 
 *  consumed area for the metadata.
 *
 * @param packet_p - Pointer to the packet.
 * @param context  - Packet completion context.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/15/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_metadata_zero_metadata_completion(fbe_packet_t * packet_p,
                                                 fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_raid_group_t *                      raid_group_p = NULL;
    fbe_object_id_t                         raid_group_object_id;
    fbe_payload_block_operation_status_t    block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fbe_payload_block_operation_qualifier_t block_qual = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_payload_control_status_t            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    /* Cast the context to raid_group object pointer. */
    raid_group_p = (fbe_raid_group_t *)context;

    /* Get the status of the raid_group zeroing block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    /* Get the control operation. */
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);
    control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
    
    /* Get the status of the operation. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qual);

    /* Release this block payload since we are done with it. */
    fbe_payload_ex_release_block_operation(sep_payload_p, block_operation_p);

    /* Copy the status from block operation to control operation. */
    if((status != FBE_STATUS_OK) || 
       (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_base_object_get_object_id((fbe_base_object_t *) raid_group_p, &raid_group_object_id);
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s zero paged request failed pkt_status: 0x%x, bl_status: 0x%x bl_qual: 0x%x \n",
                              __FUNCTION__, status, block_status, block_qual);
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_payload_control_set_status(control_operation_p, control_status);
    return status; 
}
/******************************************************************************
 * end fbe_raid_group_metadata_zero_metadata_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_metadata_write_default_paged_metadata()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the metadata with default values for 
 *   the non paged and paged metadata.
 *
 * @param raid_group_p         - pointer to the raid group
 * @param packet_p             - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/01/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_metadata_write_default_paged_metadata(fbe_raid_group_t * raid_group_p,
                                                     fbe_packet_t *packet_p)
{

    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u64_t                                       repeat_count = 0;
    fbe_raid_group_paged_metadata_t                 paged_set_bits;
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_metadata_operation_t *              metadata_operation_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_lba_t                                       paged_metadata_capacity = 0;

    /* get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Set the repeat count as number of records in paged metadata capacity.
     * This is the number of chunks of user data + paddings for alignment.
     */
    fbe_base_config_metadata_get_paged_metadata_capacity((fbe_base_config_t *)raid_group_p,
                                                       &paged_metadata_capacity);

    repeat_count = (paged_metadata_capacity*FBE_METADATA_BLOCK_DATA_SIZE)/sizeof(fbe_raid_group_paged_metadata_t);

    /* Initialize the paged bit with default values. */
    fbe_raid_group_metadata_init_paged_metadata(&paged_set_bits);

    /* Allocate the metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p);

    /* Build the paged paged metadata write request. */
    status = fbe_payload_metadata_build_paged_write(metadata_operation_p, 
                                                    &(((fbe_base_config_t *) raid_group_p)->metadata_element),
                                                    0, /* Paged metadata offset is zero for the default paged metadata write. 
                                                          paint the whole paged metadata with the default value */
                                                    (fbe_u8_t *) &paged_set_bits,
                                                    sizeof(fbe_raid_group_paged_metadata_t),
                                                    repeat_count);
    if(status != FBE_STATUS_OK)
    {
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);

        return status;
    }

    /* Now populate the metadata strip lock information */
	/*
    status = fbe_raid_group_bitmap_metadata_populate_stripe_lock(raid_group_p,
                                                                 metadata_operation_p,
                                                                 number_of_chunks_in_paged_metadata);
	*/

    /* set the completion function for the default paged metadata write. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_metadata_write_default_paged_metadata_completion, raid_group_p);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_CANCEL);

    /* Issue the metadata operation. */
    status =  fbe_metadata_operation_entry(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_raid_group_metadata_write_default_paged_metadata()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_metadata_write_default_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the metadata with default values for 
 *   the non paged and paged metadata.
 *
 * @param raid_group_p         - pointer to the raid group
 * @param packet_p             - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/01/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_metadata_write_default_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                  raid_group_p = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_status_t                        status;
    fbe_payload_metadata_status_t       metadata_status;
    
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
        
    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_mdata_wt_def_paged_mdata_compl write def paged metadata failed,stat:0x%x, mdata_stat:0x%x.\n",
                              status, metadata_status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
    }
    else
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_metadata_write_default_paged_metadata_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_metadata_write_default_nonpaged_metadata()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the metadata with default values for 
 *   the non paged metadata.
 *
 * @param raid_group_p         - pointer to the provision drive
 * @param packet_p             - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/01/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_raid_group_metadata_write_default_nonpaged_metadata(fbe_raid_group_t * raid_group_p,
                                                        fbe_packet_t * packet_p)
{
    fbe_status_t                        status;
    fbe_bool_t                          b_is_active;
    fbe_raid_group_nonpaged_metadata_t  raid_group_nonpaged_metadata;
    fbe_u32_t                           position_index;
    fbe_u8_t                            chunk_index;

    /*! @note We should not be here if this is not the active SP.
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)raid_group_p);
    if (b_is_active == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Unexpected passive set.\n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set up the defaults for the non-paged metadata.   First zero out the 
     * structure to write to it.  We'll explicitly set only those fields whose 
     * default values are non-zero.
     */
    fbe_zero_memory(&raid_group_nonpaged_metadata, sizeof(fbe_raid_group_nonpaged_metadata_t));

    /* Now set the default non-paged metadata.
     */
    status = fbe_base_config_set_default_nonpaged_metadata((fbe_base_config_t *)raid_group_p,
                                                           (fbe_base_config_nonpaged_metadata_t *)&raid_group_nonpaged_metadata);
    if (status != FBE_STATUS_OK)
    {
        /* Trace an error and return.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Set default base config nonpaged failed status: 0x%x\n",
                              __FUNCTION__, status);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return status;
    }	

    /* Initialize the rebuild checkpoints and positions to indicate that all 
     * the disks are fully rebuilt.
     */
    for (position_index = 0; position_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; position_index++)
    {
        raid_group_nonpaged_metadata.rebuild_info.rebuild_checkpoint_info[position_index].checkpoint = FBE_LBA_INVALID; 
        raid_group_nonpaged_metadata.rebuild_info.rebuild_checkpoint_info[position_index].position = FBE_RAID_INVALID_DISK_POSITION;
    }

    //  Initialize the verify checkpoints to indicate that no verifies are needed 
    raid_group_nonpaged_metadata.ro_verify_checkpoint    = FBE_LBA_INVALID;
    raid_group_nonpaged_metadata.rw_verify_checkpoint    = FBE_LBA_INVALID;
    raid_group_nonpaged_metadata.error_verify_checkpoint = FBE_LBA_INVALID;
    raid_group_nonpaged_metadata.incomplete_write_verify_checkpoint = FBE_LBA_INVALID;
    raid_group_nonpaged_metadata.system_verify_checkpoint = FBE_LBA_INVALID;    
    raid_group_nonpaged_metadata.journal_verify_checkpoint = FBE_LBA_INVALID;
    raid_group_nonpaged_metadata.encryption.rekey_checkpoint = FBE_LBA_INVALID;
    raid_group_nonpaged_metadata.encryption.unused = 0;
    raid_group_nonpaged_metadata.encryption.flags = 0;
    raid_group_nonpaged_metadata.raid_group_np_md_flags = FBE_RAID_GROUP_NP_FLAGS_DEFAULT;
    raid_group_nonpaged_metadata.raid_group_np_md_extended_flags = 
        fbe_database_is_4k_drive_committed() ? FBE_RAID_GROUP_NP_EXTENDED_FLAGS_4K_COMMITTED : FBE_RAID_GROUP_NP_EXTENDED_FLAGS_DEFAULT;
    
    // Set all background operations by default as enabled in raid group
    raid_group_nonpaged_metadata.base_config_nonpaged_metadata.operation_bitmask = 0;

    for(chunk_index = 0; chunk_index < FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS; chunk_index++)
    {
        raid_group_nonpaged_metadata.paged_metadata_metadata[chunk_index].valid_bit = 1;
    }

    //  Send the nonpaged metadata write request to metadata service. 
    status = fbe_base_config_metadata_nonpaged_write((fbe_base_config_t *) raid_group_p,
                                                 packet_p,
                                                 0, // offset is zero for the write of entire non paged metadata record. 
                                                 (fbe_u8_t *) &raid_group_nonpaged_metadata, // non paged metadata memory. 
                                                 sizeof(fbe_raid_group_nonpaged_metadata_t)); // size of the non paged metadata. 
    return status;
}
/******************************************************************************
 * end fbe_raid_group_metadata_write_default_nonpaged_metadata()
 ******************************************************************************/

fbe_status_t
fbe_raid_group_metadata_get_paged_metadata_capacity(fbe_raid_group_t * raid_group_p, 
                                                    fbe_lba_t * paged_metadata_capacity_p)
{
    fbe_lba_t   paged_mirror_metadata_offset;

    fbe_base_config_metadata_get_paged_mirror_metadata_offset((fbe_base_config_t *) raid_group_p, &paged_mirror_metadata_offset);
    *paged_metadata_capacity_p = paged_mirror_metadata_offset * FBE_RAID_GROUP_NUMBER_OF_METADATA_COPIES;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_raid_group_get_NP_lock()
 ****************************************************************
 * @brief
 *  This function gets the Non paged clustered lock
 *
 * @param provision_drive_p
 * @param packet_p
 * @param completion_function              
 *
 * @return fbe_status_t
 *
 * @author
 *  10/26/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_get_NP_lock(fbe_raid_group_t*  raid_group_p,
                                        fbe_packet_t*    packet_p,
                                        fbe_packet_completion_function_t  completion_function)
{

    fbe_transport_set_completion_function(packet_p, completion_function, raid_group_p);
    fbe_base_config_get_np_distributed_lock((fbe_base_config_t*)raid_group_p,
                                             packet_p);     

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    
}
/******************************************************************************
 * end fbe_raid_group_get_NP_lock()
 ******************************************************************************/


/*!**************************************************************
 * fbe_raid_group_release_NP_lock()
 ****************************************************************
 * @brief
 *  This function releases the Non paged clustered lock
 *
 * @param packet_p
 * @param context
 *
 * @return fbe_status_t
 *
 * @author
 *  10/26/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_release_NP_lock(fbe_packet_t*    packet_p,
                                            fbe_packet_completion_context_t context)
{
    fbe_raid_group_t*       raid_group_p = NULL;

    raid_group_p = (fbe_raid_group_t*)context;

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_NP_LOCK_TRACING,
                          "%s: Releasing NP lock. pkt: %p\n",
                          __FUNCTION__, packet_p);
        
    fbe_base_config_release_np_distributed_lock((fbe_base_config_t*)raid_group_p, packet_p);    

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    
}
/******************************************************************************
 * end fbe_raid_group_release_NP_lock()
 ******************************************************************************/

/*!**************************************************************
 * fbe_raid_group_metadata_init_paged_metadata_bits()
 ****************************************************************
 * @brief
 *  This function initializes the paged metadata to default values.
 *
 * @param paged_set_bits_p  - pointer to the paged metadata bits to initialize 
 *
 * @return fbe_status_t
 *
 * @author
 *  1/2012 - Created. Susan Rundbaken
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_metadata_init_paged_metadata(fbe_raid_group_paged_metadata_t* paged_set_bits_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    paged_set_bits_p->valid_bit = 1;
    paged_set_bits_p->verify_bits = 0x00;
    paged_set_bits_p->needs_rebuild_bits = 0x00;
    paged_set_bits_p->rekey = 0x0;
    paged_set_bits_p->reserved_bits = 0x00;

    return status;
}
/******************************************************************************
 * end fbe_raid_group_metadata_init_paged_metadata()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_metadata_set_default_nonpaged_metadata()
 ******************************************************************************
 * @brief
 *   This function is used to set and persist the metadata with default values for 
 *   the non paged metadata.
 *
 * @param raid_group_p         - pointer to the provision drive
 * @param packet_p             - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   05/15/2012 - Created. zhangy26
 *
 ******************************************************************************/
fbe_status_t
fbe_raid_group_metadata_set_default_nonpaged_metadata(fbe_raid_group_t * raid_group_p,
                                                        fbe_packet_t * packet_p)
{
    fbe_status_t                        status;
    fbe_raid_group_nonpaged_metadata_t  raid_group_nonpaged_metadata;
    fbe_u32_t                           position_index;
    fbe_u8_t                            chunk_index;


    /* Set up the defaults for the non-paged metadata.   First zero out the 
     * structure to write to it.  We'll explicitly set only those fields whose 
     * default values are non-zero.
     */
    fbe_zero_memory(&raid_group_nonpaged_metadata, sizeof(fbe_raid_group_nonpaged_metadata_t));

    /* Now set the default non-paged metadata.
     */
    status = fbe_base_config_set_default_nonpaged_metadata((fbe_base_config_t *)raid_group_p,
                                                           (fbe_base_config_nonpaged_metadata_t *)&raid_group_nonpaged_metadata);

    fbe_base_config_nonpaged_metadata_set_np_state((fbe_base_config_nonpaged_metadata_t *)&raid_group_nonpaged_metadata, 
                                                   FBE_NONPAGED_METADATA_INITIALIZED);
    
    if (status != FBE_STATUS_OK)
    {
        /* Trace an error and return.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Set default base config nonpaged failed status: 0x%x\n",
                              __FUNCTION__, status);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return status;
    }	

    /* Initialize the rebuild checkpoints and positions to indicate that all 
     * the disks are fully rebuilt.
     */
    for (position_index = 0; position_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; position_index++)
    {
        raid_group_nonpaged_metadata.rebuild_info.rebuild_checkpoint_info[position_index].checkpoint = FBE_LBA_INVALID; 
        raid_group_nonpaged_metadata.rebuild_info.rebuild_checkpoint_info[position_index].position = FBE_RAID_INVALID_DISK_POSITION;
    }

    //  Initialize the verify checkpoints to indicate that no verifies are needed 
    raid_group_nonpaged_metadata.ro_verify_checkpoint    = FBE_LBA_INVALID;
    raid_group_nonpaged_metadata.rw_verify_checkpoint    = FBE_LBA_INVALID;
    raid_group_nonpaged_metadata.error_verify_checkpoint = FBE_LBA_INVALID;
    raid_group_nonpaged_metadata.incomplete_write_verify_checkpoint = FBE_LBA_INVALID;
    raid_group_nonpaged_metadata.system_verify_checkpoint = FBE_LBA_INVALID;    
    raid_group_nonpaged_metadata.journal_verify_checkpoint = FBE_LBA_INVALID;

    // Initialize the non paged metadata flags
    raid_group_nonpaged_metadata.raid_group_np_md_flags = FBE_RAID_GROUP_NP_FLAGS_DEFAULT;
    raid_group_nonpaged_metadata.raid_group_np_md_extended_flags = 
        fbe_database_is_4k_drive_committed() ? FBE_RAID_GROUP_NP_EXTENDED_FLAGS_4K_COMMITTED : FBE_RAID_GROUP_NP_EXTENDED_FLAGS_DEFAULT;

    // Set all background operations by default as enabled in raid group
    raid_group_nonpaged_metadata.base_config_nonpaged_metadata.operation_bitmask = 0;

    for(chunk_index = 0; chunk_index < FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS; chunk_index++)
    {
        raid_group_nonpaged_metadata.paged_metadata_metadata[chunk_index].valid_bit = 1;
    }

    //  Send the nonpaged metadata write request to metadata service. 
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) raid_group_p,
                                                              packet_p,
                                                              0, // offset is zero for the write of entire non paged metadata record. 
                                                              (fbe_u8_t *) &raid_group_nonpaged_metadata, // non paged metadata memory. 
                                                              sizeof(fbe_raid_group_nonpaged_metadata_t)); // size of the non paged metadata. 
    return status;
}
/******************************************************************************
 * end fbe_raid_group_metadata_set_default_nonpaged_metadata()
 ******************************************************************************/
/*!**************************************************************
 * fbe_raid_group_get_metadata_lba()
 ****************************************************************
 * @brief
 *  Return the bitmap's lba for a given chunk index.
 *
 * @param raid_group_p -
 * @param chunk_index -
 * @param metadata_lba_p - The lba of the bitmap block which contains this chunk info.
 *
 * @return fbe_status_t 
 *
 * @author
 *  6/1/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_get_metadata_lba(fbe_raid_group_t *raid_group_p,
                                             fbe_chunk_index_t chunk_index,
                                             fbe_lba_t *metadata_lba_p)
{
    fbe_lba_t lba;
    lba = (chunk_index * sizeof(fbe_raid_group_paged_metadata_t)) / FBE_METADATA_BLOCK_DATA_SIZE;
    *metadata_lba_p = lba;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_get_metadata_lba()
 ******************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_write_mark_nr_required()
 ****************************************************************************** 
 * 
 * @brief   This function will either set or clear the `mark NR required' 
 *          non-paged metadata flag in the virtual drive object. The boolean is 
 *          set to True when a copy operation starts and it is clear when the
 *          mark NR condition is complete (i.e. it is cleared when we finish 
 *          marking Needs Rebuild for the destination drive).
 *
 * @param   raid_group_p - Pointer to virtual drive object
 * @param   packet_p - pointer to a control packet from the scheduler 
 * @param   b_mark_nr_required - FBE_TRUE - Set the mark NR required flag in the metadata
 *                           FBE_FALSE - Clear the mark NR required flag in the matadata
 * @param   
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  02/15/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_metadata_write_mark_nr_required(fbe_raid_group_t *raid_group_p,
                                                                      fbe_packet_t *packet_p,
                                                                      fbe_bool_t b_mark_nr_required)
{
    fbe_status_t                        status;             
    fbe_u64_t                           metadata_offset; 
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_raid_group_np_metadata_flags_t  raid_group_np_md_flags;              
    
    /* Get the current value for the non-paged flags.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);
    raid_group_np_md_flags = nonpaged_metadata_p->raid_group_np_md_flags;  
     
    /* Set the value as requested.
     */
    if (b_mark_nr_required == FBE_TRUE)
    {
        raid_group_np_md_flags |= FBE_RAID_GROUP_NP_FLAGS_MARK_NR_REQUIRED;
    }
    else
    {
        raid_group_np_md_flags &= ~FBE_RAID_GROUP_NP_FLAGS_MARK_NR_REQUIRED;
    }

    /*! @note The metadata operation will copy the value passed into the 
     *        metadata operation before it returns.
     */
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->raid_group_np_md_flags);       
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)raid_group_p, packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t*)&raid_group_np_md_flags,
                                                             sizeof(fbe_raid_group_np_metadata_flags_t));
    
    //  Check the status of the call and trace if error.  The called function is completing the packet on error
    //  so we can't complete it here.
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: write mark NR required to: %d failed with status: 0x%x\n", 
                              __FUNCTION__, b_mark_nr_required, status);        
        return FBE_STATUS_GENERIC_FAILURE;         
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

}
/*********************************************************
 * end fbe_raid_group_metadata_write_mark_nr_required()
**********************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_clear_mark_nr_required()
 ****************************************************************************** 
 * 
 * @brief   This function will clear the `mark NR required' flag in the non-paged
 *          metadata.
 *
 * @param   raid_group_p - Pointer to virtual drive to clear bit for
 * @param   packet_p - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  02/15/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_metadata_clear_mark_nr_required(fbe_raid_group_t *raid_group_p,
                                                            fbe_packet_t *packet_p)
{
    fbe_status_t    status;             
    
    status = fbe_raid_group_metadata_write_mark_nr_required(raid_group_p, packet_p,
                                                            FBE_FALSE /* Clear the flag */);

    /* Return the execution status.
     */
    return status; 

}
/**********************************************************
 * end fbe_raid_group_metadata_clear_mark_nr_required()
***********************************************************/

/*!****************************************************************************
 * fbe_raid_group_is_paged_reconstruct_needed()
 ****************************************************************************** 
 * @brief
 *  Determines if the flag to reconstruc paged. is set in the non-paged metadata.
 *
 * @param   raid_group_p - Pointer to RG object.
 *
 * @return  fbe_bool_t  
 *
 * @author
 *  11/22/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_bool_t fbe_raid_group_is_paged_reconstruct_needed(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    
    /* Should not be here unless this is a virtual drive.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);
        
    /* Set the output based on the edge swapped flag.
     */
    if (nonpaged_metadata_p->raid_group_np_md_flags & FBE_RAID_GROUP_NP_FLAGS_RECONSTRUCT_REQUIRED) {
        return FBE_TRUE;
    }
    else {
        return FBE_FALSE;
    }
}
/*******************************************************
 * end fbe_raid_group_is_paged_reconstruct_needed()
********************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_is_mark_nr_required()
 ****************************************************************************** 
 * 
 * @brief   Determines if the `mark NR required' flag is set in the non-paged
 *          metadata.
 *
 * @param   raid_group_p - Pointer to virtual drive object
 * @param   b_is_mark_nr_required_p - Pointer to bool to update:
 *              FBE_TRUE - Mark NR is set and thus we must run the eval mark NR
 *                  condition.
 *              FBE_FALSE - No special action required.
 *
 * @return  fbe_status_t        
 *
 * @author
 *  02/15/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_metadata_is_mark_nr_required(fbe_raid_group_t *raid_group_p,
                                                         fbe_bool_t *b_is_mark_nr_required_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    
    /* Should not be here unless this is a virtual drive.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);
        
    /* Set the output based on the edge swapped flag.
     */
    if (nonpaged_metadata_p->raid_group_np_md_flags & FBE_RAID_GROUP_NP_FLAGS_MARK_NR_REQUIRED)
    {
        *b_is_mark_nr_required_p = FBE_TRUE;
    }
    else
    {
        *b_is_mark_nr_required_p = FBE_FALSE;
    }

    //  Return success 
    return FBE_STATUS_OK;

}
/*******************************************************
 * end fbe_raid_group_metadata_is_mark_nr_required()
********************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_write_reconstruct_required()
 ****************************************************************************** 
 * 
 * @brief   This function will either set or clear the `paged metadata 
 *          reconstruct required' non-paged metadata flag in the raid groip 
 *          object.
 *
 * @param   raid_group_p - Pointer to virtual drive object
 * @param   packet_p - pointer to a control packet from the scheduler 
 * @param   b_reconstruct_required - FBE_TRUE - Set the reconstruct required flag in the metadata
 *                           FBE_FALSE - Clear the reconstruct required flag in the matadata
 * @param   
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  03/08/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_metadata_write_reconstruct_required(fbe_raid_group_t *raid_group_p,
                                                                      fbe_packet_t *packet_p,
                                                                      fbe_bool_t b_reconstruct_required)
{
    fbe_status_t                        status;             
    fbe_u64_t                           metadata_offset; 
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_raid_group_np_metadata_flags_t  raid_group_np_md_flags;              
    
    /* Get the current value for the non-paged flags.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);
    raid_group_np_md_flags = nonpaged_metadata_p->raid_group_np_md_flags;  
     
    /* Set the value as requested.
     */
    if (b_reconstruct_required == FBE_TRUE)
    {
        raid_group_np_md_flags |= FBE_RAID_GROUP_NP_FLAGS_RECONSTRUCT_REQUIRED;
    }
    else
    {
        raid_group_np_md_flags &= ~FBE_RAID_GROUP_NP_FLAGS_RECONSTRUCT_REQUIRED;
    }

    /*! @note The metadata operation will copy the value passed into the 
     *        metadata operation before it returns.
     */
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->raid_group_np_md_flags);       
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)raid_group_p, packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t*)&raid_group_np_md_flags,
                                                             sizeof(fbe_raid_group_np_metadata_flags_t));
    
    //  Check the status of the call and trace if error.  The called function is completing the packet on error
    //  so we can't complete it here.
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: write reconstruct required to: %d failed with status: 0x%x\n", 
                              __FUNCTION__, b_reconstruct_required, status);        
        return FBE_STATUS_GENERIC_FAILURE;         
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

}
/*********************************************************
 * end fbe_raid_group_metadata_write_reconstruct_required()
**********************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_clear_paged_metadata_reconstruct_required()
 ****************************************************************************** 
 * 
 * @brief   This function will clear the `paged metadata reconstruct required' 
 *          flag in the non-paged metadata.
 *
 * @param   raid_group_p - Pointer to virtual drive to clear bit for
 * @param   packet_p - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  03/08/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_metadata_clear_paged_metadata_reconstruct_required(fbe_raid_group_t *raid_group_p,
                                                            fbe_packet_t *packet_p)
{
    fbe_status_t    status;             
    
    status = fbe_raid_group_metadata_write_reconstruct_required(raid_group_p, packet_p,
                                                            FBE_FALSE /* Clear the flag */);

    /* Return the execution status.
     */
    return status; 

}
/*************************************************************************
 * end fbe_raid_group_metadata_clear_paged_metadata_reconstruct_required()
**************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_set_paged_metadata_reconstruct_required()
 ****************************************************************************** 
 * 
 * @brief   This function will set the `paged metadata reconstruct required' 
 *          flag in the non-paged metadata.
 *
 * @param   raid_group_p - Pointer to virtual drive to set bit for
 * @param   packet_p - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  03/08/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_metadata_set_paged_metadata_reconstruct_required(fbe_raid_group_t *raid_group_p,
                                                             fbe_packet_t *packet_p)
{
    fbe_status_t    status;             
    
    status = fbe_raid_group_metadata_write_reconstruct_required(raid_group_p, packet_p,
                                                               FBE_TRUE /* Set the flag */);

    /* Return the execution status.
     */
    return status; 

}
/***********************************************************************
 * end fbe_raid_group_metadata_set_paged_metadata_reconstruct_required()
************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_is_paged_metadata_reconstruct_required()
 ****************************************************************************** 
 * 
 * @brief   Determines if the `paged metadata reconstruct required' flag is set
 *          in the non-paged metadata.
 *
 * @param   raid_group_p - Pointer to virtual drive object
 * @param   b_is_reconstruct_required_p - Pointer to bool to update:
 *              FBE_TRUE - Reconstruct required is set and thus we must run the
 *                  paged metadata reconstruct condition.
 *              FBE_FALSE - The paged metadata reconstruct required flag is not
 *                  set in the non-paged metadata.
 *
 * @return  fbe_status_t        
 *
 * @author
 *  03/08/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_metadata_is_paged_metadata_reconstruct_required(fbe_raid_group_t *raid_group_p,
                                                         fbe_bool_t *b_is_reconstruct_required_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    
    /* Should not be here unless this is a virtual drive.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);
        
    /* Set the output based on the reconstruct required flag.
     */
    if (nonpaged_metadata_p->raid_group_np_md_flags & FBE_RAID_GROUP_NP_FLAGS_RECONSTRUCT_REQUIRED)
    {
        *b_is_reconstruct_required_p = FBE_TRUE;
    }
    else
    {
        *b_is_reconstruct_required_p = FBE_FALSE;
    }

    //  Return success 
    return FBE_STATUS_OK;

}
/**********************************************************************
 * end fbe_raid_group_metadata_is_paged_metadata_reconstruct_required()
***********************************************************************/
/*!**************************************************************
 * fbe_raid_group_reconstruct_all_md_encryption()
 ****************************************************************
 * @brief
 *  Kick off our reconstruct state machine to reconstruct
 *  the entire paged metadata during encryption.
 *
 * @param raid_group_p - current object.
 * @param packet_p - current packet.               
 *
 * @return fbe_status_t
 *
 * @author
 *  7/25/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_reconstruct_all_md_encryption(fbe_raid_group_t *raid_group_p,
                                                          fbe_packet_t *packet_p)
{
    fbe_lba_t metadata_start_lba;
    fbe_block_count_t metadata_capacity;
    fbe_lba_t verify_start_lba;
    fbe_block_count_t verify_blocks;
    fbe_u16_t data_disks;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, &metadata_start_lba);
    fbe_raid_geometry_get_metadata_capacity(raid_geometry_p, &metadata_capacity);

    verify_start_lba = metadata_start_lba / data_disks;
    verify_blocks = metadata_capacity / data_disks;

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "reconstruct all metadata for enc start: 0x%llx end: 0x%llx\n", 
                          verify_start_lba, verify_blocks);
    
    fbe_raid_group_encrypt_reconstruct_sm_start(raid_group_p, packet_p, verify_start_lba, verify_blocks);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_reconstruct_all_md_encryption()
 ******************************************/
/*!****************************************************************************
 * fbe_raid_group_metadata_reconstruct_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the metadata with default values for 
 *   the non paged and paged metadata.
 *
 * @param raid_group_p         - pointer to the raid group
 * @param packet_p             - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  03/08/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_metadata_reconstruct_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                  raid_group_p = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_status_t                        status;
    fbe_payload_metadata_status_t       metadata_status;
    
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
        
    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_reconstruct_paged_mdata_compl write def paged metadata failed,stat:0x%x, mdata_stat:0x%x.\n",
                              status, metadata_status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
    }
    else
    {
        /* Else set the status to success and complete the packet*/
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_metadata_reconstruct_paged_metadata_completion()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_reconstruct_update_nonpaged_completion()
 ******************************************************************************
 *
 * @brief   This is the completion function for the first step ofa  `reconstruct
 *          paged metadata' request which is to first reconstuct the non-paged
 *          metadata.  If the non-paged was updated successfully we proceed
 *          to updating the non-paged.
 *
 * @param   packet_p      - pointer to a control packet from the scheduler.
 * @param   context       - context, which is a pointer to the base object.
 * 
 * @return  fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  03/10/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_metadata_reconstruct_update_nonpaged_completion(fbe_packet_t *packet_p,
                                                          fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_raid_group_t                   *raid_group_p = NULL;
    fbe_raid_geometry_t                *raid_geometry_p = NULL;
    fbe_u32_t                           width;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p = NULL;
    fbe_chunk_index_t                   paged_md_chunk_index;
    fbe_chunk_count_t                   paged_md_chunk_count;
    fbe_u32_t                           position_index;
    fbe_u32_t                           mark_nr_bitmask = 0;
    fbe_raid_group_paged_metadata_t     paged_set_bits;
    fbe_u64_t                           repeat_count = 0;
    fbe_payload_metadata_operation_t   *metadata_operation_p = NULL;
    fbe_lba_t                           paged_metadata_capacity = 0;
    fbe_u16_t                           data_disks;
    fbe_lba_t                           exported_capacity = 0;
    
    /* Get some information about the raid group.
     */
    raid_group_p = (fbe_raid_group_t*)context;
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_raid_group_get_width(raid_group_p, &width);

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    /* If we updated the non-paged successfully, proceed to reconstruc the paged
     * metadata.
     */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        fbe_base_config_encryption_mode_t encryption_mode;
        fbe_base_config_get_encryption_mode((fbe_base_config_t*)raid_group_p, &encryption_mode);
        if ((encryption_mode > FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED) &&
            (encryption_mode < FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED)) {
            fbe_raid_group_reconstruct_all_md_encryption(raid_group_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        /* Get the non-paged metadata pointer.
         */
        fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);
        
        /* Build the bitmask of positions that needs to be rebuilt
         */
        for (position_index = 0; position_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; position_index++)
        {
            if ((nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[position_index].position != FBE_RAID_INVALID_DISK_POSITION) &&
                (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[position_index].position <  width)                             )
            {
                mark_nr_bitmask |= (1 << nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[position_index].position);
            }
        }

        /* Set the repeat count as number of records in paged metadata capacity.
         * This is the number of chunks of user data + paddings for alignment.
         */
        fbe_base_config_metadata_get_paged_metadata_capacity((fbe_base_config_t *)raid_group_p,
                                                             &paged_metadata_capacity);

        repeat_count = (paged_metadata_capacity*FBE_METADATA_BLOCK_DATA_SIZE)/sizeof(fbe_raid_group_paged_metadata_t);

        /* Get the user capacity in physical blocks.
         */
        fbe_base_config_get_capacity((fbe_base_config_t *)raid_group_p, &exported_capacity);
        exported_capacity /= data_disks;
        
        /* Initialize the paged bit with default values. Then use the non-paged to
         * populate the paged
         */
        fbe_raid_group_metadata_init_paged_metadata(&paged_set_bits);

        /* Determine the user data chunk range for entire user area.
        */
        fbe_raid_group_bitmap_get_chunk_range(raid_group_p, 0, exported_capacity, &paged_md_chunk_index, &paged_md_chunk_count);

        /* Set the paged metadata bits from the nonpaged metadata for the given RG user data chunk range.
         */
        fbe_raid_group_bitmap_set_paged_bits_from_nonpaged(raid_group_p, 
                                                           paged_md_chunk_index, 
                                                           paged_md_chunk_count,
                                                           &paged_set_bits);

        /* Trace some information.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: paged reconstruct initialize: 0x%llx NR chunk infos with: 0x%x NR mask \n",
                              (unsigned long long)repeat_count, paged_set_bits.needs_rebuild_bits);

        /* For debug validate that the local value matches the determined
         */
        if (mark_nr_bitmask != paged_set_bits.needs_rebuild_bits)
        {
            /* Trace an error but continue.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: paged reconstruct calculated NR mask: 0x%x doesn't agree with determined: 0x%x \n",
                                  mark_nr_bitmask, paged_set_bits.needs_rebuild_bits);
        }

        /* Allocate the metadata operation. */
        metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(payload_p);

        /* Build the paged paged metadata write request. */
        status = fbe_payload_metadata_build_paged_write(metadata_operation_p, 
                                                        &(((fbe_base_config_t *) raid_group_p)->metadata_element),
                                                        0, /* Paged metadata offset is zero for the default paged metadata write. 
                                                          paint the whole paged metadata with the default value */
                                                        (fbe_u8_t *) &paged_set_bits,
                                                        sizeof(fbe_raid_group_paged_metadata_t),
                                                        repeat_count);
        if (status != FBE_STATUS_OK)
        {
            /* Failed, report the error and tray again on the next monitor cycle.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: paged reconstruct - failed: 0x%llx NR chunks info with: 0%x NR mask - status: 0x%x \n",
                                  (unsigned long long)repeat_count, mark_nr_bitmask, status);
            fbe_payload_ex_release_metadata_operation(payload_p, metadata_operation_p);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, status, 0);
            return status;
        }

        /* set the completion function for the default paged metadata write. */
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_metadata_reconstruct_paged_metadata_completion, raid_group_p);

        fbe_payload_ex_increment_metadata_operation_level(payload_p);

        fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_CANCEL);

        /* Issue the metadata operation to write the paged metadata chunk.
         */
        status = fbe_metadata_operation_entry(packet_p);

        /* because metadata_operation guaranteed to call completion, we need to stop the completion here */
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_reconstruct_nonpaged_mdata_cond_compl fail,stat:0x%X,ctrl_stat:0x%X\n",
                              status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_metadata_reconstruct_update_nonpaged_completion()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_reconstruct_update_nonpaged()
 ****************************************************************************** 
 * 
 * @brief   This function updates the non-paged for a `reconstruct paged
 *          metadata' request.
 *
 * @param   raid_group_p - pointer to the raid group
 * @param   packet_p - pointer to a monitor packet
 * @param   updated_nonpaged_p - Pointer to updated raid group non-paged
 *              metadata.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  03/08/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_metadata_reconstruct_update_nonpaged(fbe_raid_group_t *raid_group_p,
                                                                        fbe_packet_t *packet_p,
                                                                        fbe_raid_group_nonpaged_metadata_t *updated_nonpaged_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /*! @note The metadata operation will copy the values passed into the 
     *        metadata operation before it returns.
     */
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)raid_group_p, packet_p, 
                                                             0, 
                                                             (fbe_u8_t*)updated_nonpaged_p,
                                                             sizeof(fbe_raid_group_nonpaged_metadata_t));
    
    /* Check the status of the call and trace if error.  The called function 
     * is completing the packet on error so we can't complete it here.
     */
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: update nonpaged failed with status: 0x%x\n", 
                              __FUNCTION__, status);        
        return FBE_STATUS_GENERIC_FAILURE;         
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/************************************************************
 * end fbe_raid_group_metadata_reconstruct_update_nonpaged()
 ************************************************************/


/*!****************************************************************************
 *          fbe_raid_group_metadata_reconstruct_paged_metadata()
 ****************************************************************************** 
 * 
 * @brief   This function is used to reconstruct teh paged metadata from the
 *          non-paged values.  It uses any positions 
 *
 * @param   raid_group_p         - pointer to the raid group
 * @param   packet_p             - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  03/08/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_metadata_reconstruct_paged_metadata(fbe_raid_group_t *raid_group_p,
                                                                fbe_packet_t *packet_p)
{

    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t                           width;
    fbe_raid_geometry_t                *raid_geometry_p = NULL;
    fbe_u16_t                           num_parity_drives;
    fbe_raid_group_type_t               raid_type;
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p = NULL;
    fbe_raid_group_nonpaged_metadata_t  raid_group_nonpaged_metadata;
    fbe_u32_t                           position_index;
    fbe_u32_t                           mark_nr_bitmask = 0;
    fbe_u32_t                           num_of_nr_drives;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;

    /* Get control operation
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Get some information about the raid group.
     */
    fbe_raid_group_get_width(raid_group_p, &width);
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_parity_disks(raid_geometry_p, &num_parity_drives);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);

    /* First determine which positions need to be marked needs rebuild in
     * the metadata (both the non-paged and paged).
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&nonpaged_metadata_p);

    /* Copy the contents of the non-paged into a local (the in-memory copy will
     * be updated when teh non-paged update is complete).
     */
    raid_group_nonpaged_metadata = *nonpaged_metadata_p;

    /* Build the bitmask of positions that needs to be rebuilt
     */
    for (position_index = 0; position_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; position_index++)
    {
        if ((raid_group_nonpaged_metadata.rebuild_info.rebuild_checkpoint_info[position_index].position != FBE_RAID_INVALID_DISK_POSITION) &&
            (raid_group_nonpaged_metadata.rebuild_info.rebuild_checkpoint_info[position_index].position <  width)                             )
        {
            mark_nr_bitmask |= (1 << raid_group_nonpaged_metadata.rebuild_info.rebuild_checkpoint_info[position_index].position);
        }
    }
    num_of_nr_drives = fbe_raid_get_bitcount(mark_nr_bitmask);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: reconstruct paged metadata raid type: %d nr drives: %d parity drives: %d \n",
                          raid_type, num_of_nr_drives, num_parity_drives);


    /* We should not have been here if there is no paged metadata
     */
    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p) ||
         (raid_type == FBE_RAID_GROUP_TYPE_RAID10)                )        
    {
        /* Trace an error and complete the packet so that 
         * `paged metadata reconstruct required' gets cleared in the non-paged.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s unsupported raid type: %d\n", 
                              __FUNCTION__, raid_type);

        /* Return the packet with success so that we clear the `reconstruct
         * paged metadata' non-paged flag.
         */
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* If there are too many dead positions in the non-paged trace an error
     * and complete the packet with success.  This is clear this `reconstruct 
     * paged metadata flag and clear this condition.  The paged metadata verify 
     * will detect the issue and the raid group will be shutdown. 
     */
    if (num_of_nr_drives > num_parity_drives)
    {
        /* Trace the error and complete the packet will success.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: reconstruct paged metadata raid type: %d nr drives: %d parity drives: %d too many bad\n",
                          raid_type, num_of_nr_drives, num_parity_drives);

        /* Set the broken condition
         */
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *)raid_group_p, 
                               FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);

        /* Return the packet with success so that we clear the `reconstruct
         * paged metadata' non-paged flag.
         */
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* Now set the rebuild checkpoint for any position that needs to be rebuilt
     * to 0.                                                             .
     */
    for (position_index = 0; position_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; position_index++)
    {
        if ((raid_group_nonpaged_metadata.rebuild_info.rebuild_checkpoint_info[position_index].position != FBE_RAID_INVALID_DISK_POSITION) &&
            (raid_group_nonpaged_metadata.rebuild_info.rebuild_checkpoint_info[position_index].position <  width)                             )
        {
            raid_group_nonpaged_metadata.rebuild_info.rebuild_checkpoint_info[position_index].checkpoint = 0;
        }
    }

    /* Set the completion function.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_metadata_reconstruct_update_nonpaged_completion, raid_group_p);

    /* Update the non-paged before updating the paged.
     */
    status = fbe_raid_group_metadata_reconstruct_update_nonpaged(raid_group_p,
                                                                 packet_p,
                                                                 &raid_group_nonpaged_metadata);

    if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    /* Return the status of the operation (completion is always invoked).
     */
    return status;
}
/******************************************************************************
 * end fbe_raid_group_metadata_reconstruct_paged_metadata()
 ******************************************************************************/

/*!**************************************************************
 * fbe_raid_group_paged_metadata_iots_completion()
 ****************************************************************
 * @brief
 *   This is the completion function that gets called 
 *   when we find that the chunk need not be verified.
 *   This will clean up and complete the iots.
 * 
 * @param packet_p   - packet that is completing
 * @param in_context    - completion context, which is a pointer to 
 *                        the raid group object
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/27/2010 - Created Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_paged_metadata_iots_completion(fbe_packet_t *packet_p,
                                                           fbe_packet_completion_context_t in_context)
{
    fbe_status_t                            packet_status;
    fbe_u32_t                               packet_qualifier;
    fbe_raid_group_t                       *raid_group_p;
    fbe_payload_ex_t                       *sep_payload_p; 
    fbe_raid_iots_t                        *iots_p;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;  
    fbe_payload_block_operation_opcode_t    opcode; 
    fbe_raid_verify_error_counts_t          local_error_counters;
    fbe_raid_verify_error_counts_t         *error_counters_p = NULL;                           

    raid_group_p = (fbe_raid_group_t*)in_context;

    /* Put the packet back on the termination queue since we had to take it
     * off before reading the chunk info.
     */
    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);

    /* Get the iots.
     */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    /* Get the packet status.
     */
    packet_status = fbe_transport_get_status_code(packet_p);
    packet_qualifier = fbe_transport_get_status_qualifier(packet_p);

    /* Get the current block status and opcode.
     */
    fbe_raid_iots_get_block_status(iots_p, &block_status);
    fbe_raid_iots_get_block_qualifier(iots_p, &block_qualifier);
    fbe_raid_iots_get_opcode(iots_p, &opcode);

    /* Since the packet has made it this far iots cleanup will always mark
     * the packet status as success.  Therefore we must translate the
     * packet status into a block status (unless the block status was
     * not success).
     */
    if (packet_status != FBE_STATUS_OK)
    {
        /* If the current block status is either invalid or success, update it.
         */
        if ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID) ||
            (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)    )
        {
            /*! @note Although this waste some stack space we need to validate the 
             *        I/O status below and this make the processing simpler.
             */
            error_counters_p = &local_error_counters;
            fbe_zero_memory(error_counters_p, sizeof(*error_counters_p));

            /* Translate the packet status to a block status.
             */
            fbe_raid_group_handle_transport_error(raid_group_p,
                                                  packet_status,
                                                  packet_qualifier,
                                                  &block_status,
                                                  &block_qualifier,
                                                  error_counters_p);

            /* Trace some information and set the packet status
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s op: %d sts: 0x%x block sts/qual: %d/%d failed\n",
                              __FUNCTION__, opcode, packet_status, block_status, block_qualifier);

            /* Now set the block status and qualifier.
             */
            fbe_raid_iots_set_block_status(iots_p, block_status);
            fbe_raid_iots_set_block_qualifier(iots_p, block_qualifier);
        }
    }
    
    /* If the block status is not invalid or success return now.
     */
    if ((packet_status != FBE_STATUS_OK)                                   ||
        ((block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID) &&
         (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)    )    )
    {
        /* Complete the iots.
         */
        fbe_raid_group_iots_cleanup(packet_p, in_context);
    }
    else
    {
        /* Else start the next piece.
        */
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        fbe_raid_iots_set_status(iots_p, FBE_RAID_IOTS_STATUS_COMPLETE);
        fbe_raid_group_iots_start_next_piece(packet_p, raid_group_p);
    }

    /* Always return more processing required.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}
/*****************************************************
 * end fbe_raid_group_paged_metadata_iots_completion()
 *****************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_write_error_verify_required()
 ****************************************************************************** 
 * 
 * @brief   This function will either set or clear the `error_verify_required' 
 *          non-paged metadata flag in the virtual drive object. The boolean is 
 *          set to True when an error is marked for verify below the current error
 *          verify checkpoint (always if this is passive) and it is cleared when the
 *          current error_verify pass is complete and we start the new pass.
 *
 * @param   raid_group_p - Pointer to raid group object
 * @param   packet_p - pointer to a control packet from the scheduler 
 * @param   b_error_verify_required - FBE_TRUE - Set the error_verify_required flag in the metadata
 *                                    FBE_FALSE - Clear the error_verify_required flag in the matadata
 * @param   
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  04/09/2013  Dave Agans - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_metadata_write_error_verify_required(fbe_raid_group_t *raid_group_p,
                                                                        fbe_packet_t *packet_p,
                                                                        fbe_bool_t b_error_verify_required)
{
    fbe_status_t                        status;             
    fbe_u64_t                           metadata_offset; 
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_raid_group_np_metadata_flags_t  raid_group_np_md_flags;              
    
    /* Get the current value for the non-paged flags.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);
    raid_group_np_md_flags = nonpaged_metadata_p->raid_group_np_md_flags;  
     
    /* Set the value as requested.
     */
    if (b_error_verify_required == FBE_TRUE)
    {
        raid_group_np_md_flags |= FBE_RAID_GROUP_NP_FLAGS_ERROR_VERIFY_REQUIRED;
    }
    else
    {
        raid_group_np_md_flags &= ~FBE_RAID_GROUP_NP_FLAGS_ERROR_VERIFY_REQUIRED;
    }

    /*! @note The metadata operation will copy the value passed into the 
     *        metadata operation before it returns.
     */
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->raid_group_np_md_flags);       
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)raid_group_p,
                                                             packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t*)&raid_group_np_md_flags,
                                                             sizeof(fbe_raid_group_np_metadata_flags_t));
    
    //  Check the status of the call and trace if error.  The called function is completing the packet on error
    //  so we can't complete it here.
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: write error_verify_required to: %d failed with status: 0x%x\n", 
                              __FUNCTION__, b_error_verify_required, status);        
        return FBE_STATUS_GENERIC_FAILURE;         
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

}
/*********************************************************
 * end fbe_raid_group_metadata_write_error_verify_required()
**********************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_clear_error_verify_required()
 ****************************************************************************** 
 * 
 * @brief   This completion function will clear the `error_verify_required' flag in the non-paged
 *          metadata.
 *
 * @param   raid_group_p - Pointer to virtual drive to clear bit for
 * @param   packet_p - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  04/09/2013  Dave Agans - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_metadata_clear_error_verify_required(fbe_packet_t * packet_p, 
                                                                 fbe_packet_completion_context_t context)
{
    fbe_status_t    status;             
    fbe_raid_group_t * raid_group_p = (fbe_raid_group_t *)context;

    status = fbe_raid_group_metadata_write_error_verify_required(raid_group_p, packet_p,
                                                                 FBE_FALSE /* Clear the flag */);

    /* Return the execution status.
     */
    return status; 

}
/**********************************************************
 * end fbe_raid_group_metadata_clear_error_verify_required()
***********************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_set_error_verify_required()
 ****************************************************************************** 
 * 
 * @brief   This function will set the `error_verify_required' flag in the non-paged
 *          metadata.
 *
 * @param   raid_group_p - Pointer to virtual drive to set bit for
 * @param   packet_p - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  04/09/2013  Dave Agans - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_metadata_set_error_verify_required(fbe_raid_group_t *raid_group_p,
                                                               fbe_packet_t *packet_p)
{
    fbe_status_t    status;             
    
    status = fbe_raid_group_metadata_write_error_verify_required(raid_group_p, packet_p,
                                                                 FBE_TRUE /* Set the flag */);

    /* Return the execution status.
     */
    return status; 

}
/********************************************************
 * end fbe_raid_group_metadata_set_error_verify_required()
*********************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_is_error_verify_required()
 ****************************************************************************** 
 * 
 * @brief   Determines if the `error_verify_required' flag is set in the non-paged
 *          metadata.
 *
 * @param   raid_group_p - Pointer to virtual drive object
 *
 * @return  fbe_bool_t - FBE_TRUE - error_verify_required is set and thus
 *                                  another verify pass is needed.
 *                       FBE_FALSE - No special action required.
 *
 * @author
 *  04/09/2013  Dave Agans - Created.
 *
 ******************************************************************************/
fbe_bool_t fbe_raid_group_metadata_is_error_verify_required(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_bool_t                         b_is_error_verify_required;
    
    /* Get the non-paged data.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);
        
    /* Set the output based on the edge swapped flag.
     */
    if (nonpaged_metadata_p->raid_group_np_md_flags & FBE_RAID_GROUP_NP_FLAGS_ERROR_VERIFY_REQUIRED)
    {
        b_is_error_verify_required = FBE_TRUE;
    }
    else
    {
        b_is_error_verify_required = FBE_FALSE;
    }

    //  Return success 
    return b_is_error_verify_required;

}
/*******************************************************
 * end fbe_raid_group_metadata_is_error_verify_required()
********************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_is_degraded_needs_rebuild()
 ****************************************************************************** 
 * 
 * @brief   Determines if the `degraded needs rebuild' flag is set
 *          in the non-paged metadata.
 *
 * @param   raid_group_p - Pointer to virtual drive object
 * @param   b_is_degraded_needs_rebuild - Pointer to bool to update:
 *              FBE_TRUE - Degraded needs rebuild is set and thus we must run the
 *                  paged metadata reconstruct condition.
 *              FBE_FALSE - The degraded needs rebuild flag is not
 *                  set in the non-paged metadata.
 *
 * @return  fbe_status_t        
 *
 * @author
 *  04/17/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_metadata_is_degraded_needs_rebuild(fbe_raid_group_t *raid_group_p,
                                                               fbe_bool_t *b_is_degraded_needs_rebuild)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    
    /* Should not be here unless this is a virtual drive.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);
        
    /* Set the output based on the degraded needs rebuild flag.
     */
    if (nonpaged_metadata_p->raid_group_np_md_flags & FBE_RAID_GROUP_NP_FLAGS_DEGRADED_NEEDS_REBUILD)
    {
        *b_is_degraded_needs_rebuild = FBE_TRUE;
    }
    else
    {
        *b_is_degraded_needs_rebuild = FBE_FALSE;
    }

    //  Return success 
    return FBE_STATUS_OK;

}
/**********************************************************************
 * end fbe_raid_group_metadata_is_degraded_needs_rebuild()
***********************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_write_iw_verify_required()
 ****************************************************************************** 
 * 
 * @brief   This function will either set or clear the `iw_verify_required' 
 *          non-paged metadata flag in the RG nonpaged metadata.
 *          This value is set to True when an area is marked for verify below
 *             the current iw verify checkpoint (always if this is passive)
 *          and it is cleared when the current iw_verify pass is complete
 *             and we start the new pass.
 *
 * @param   raid_group_p - Pointer to raid group object
 * @param   packet_p - pointer to a control packet from the scheduler 
 * @param   b_iw_verify_required - FBE_TRUE - Set the iw_verify_required flag in the metadata
 *                                    FBE_FALSE - Clear the iw_verify_required flag in the matadata
 * @param   
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  5/6/2013 -  Copied from fbe_raid_group_metadata_write_error_verify_required()
 *              to create incomplete write flavor. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_metadata_write_iw_verify_required(fbe_raid_group_t *raid_group_p,
                                                                     fbe_packet_t *packet_p,
                                                                     fbe_bool_t b_iw_verify_required)
{
    fbe_status_t                        status;             
    fbe_u64_t                           metadata_offset; 
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_raid_group_np_metadata_flags_t  raid_group_np_md_flags;              
    
    /* Get the current value for the non-paged flags.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);
    raid_group_np_md_flags = nonpaged_metadata_p->raid_group_np_md_flags;  
     
    /* Set the value as requested.
     */
    if (b_iw_verify_required == FBE_TRUE)
    {
        raid_group_np_md_flags |= FBE_RAID_GROUP_NP_FLAGS_IW_VERIFY_REQUIRED;
    }
    else
    {
        raid_group_np_md_flags &= ~FBE_RAID_GROUP_NP_FLAGS_IW_VERIFY_REQUIRED;
    }

    /*! @note The metadata operation will copy the value passed into the 
     *        metadata operation before it returns.
     */
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->raid_group_np_md_flags);       
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)raid_group_p,
                                                             packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t*)&raid_group_np_md_flags,
                                                             sizeof(fbe_raid_group_np_metadata_flags_t));
    
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: write iw_verify_required to: %d failed with status: 0x%x\n", 
                              __FUNCTION__, b_iw_verify_required, status);        
        return FBE_STATUS_GENERIC_FAILURE;         
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/*********************************************************
 * end fbe_raid_group_metadata_write_iw_verify_required()
**********************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_clear_iw_verify_required()
 ****************************************************************************** 
 * 
 * @brief   This completion function will clear the `iw_verify_required' flag in the non-paged
 *          metadata.
 *
 * @param   raid_group_p - Pointer to virtual drive to clear bit for
 * @param   packet_p - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  5/6/2013 - Copied from fbe_raid_group_metadata_clear_error_verify_required()
 *             to create incomplete write flavor Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_metadata_clear_iw_verify_required(fbe_packet_t * packet_p, 
                                                                 fbe_packet_completion_context_t context)
{
    fbe_status_t    status;             
    fbe_raid_group_t * raid_group_p = (fbe_raid_group_t *)context;

    status = fbe_raid_group_metadata_write_iw_verify_required(raid_group_p, packet_p,
                                                              FBE_FALSE /* Clear the flag */);
    return status; 
}
/**********************************************************
 * end fbe_raid_group_metadata_clear_iw_verify_required()
***********************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_set_iw_verify_required()
 ****************************************************************************** 
 * 
 * @brief   This function will set the `iw_verify_required' flag in the non-paged
 *          metadata.
 *
 * @param   raid_group_p - Pointer to virtual drive to set bit for
 * @param   packet_p - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  5/6/2013 - Copied from fbe_raid_group_metadata_set_error_verify_required()
 *             to create incomplete write flavor Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_metadata_set_iw_verify_required(fbe_raid_group_t *raid_group_p,
                                                            fbe_packet_t *packet_p)
{
    fbe_status_t    status;
    
    status = fbe_raid_group_metadata_write_iw_verify_required(raid_group_p, packet_p,
                                                              FBE_TRUE /* Set the flag */);
    return status; 
}
/********************************************************
 * end fbe_raid_group_metadata_set_iw_verify_required()
*********************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_is_iw_verify_required()
 ****************************************************************************** 
 * 
 * @brief   Determines if the `iw_verify_required' flag is set in the non-paged
 *          metadata.
 *
 * @param   raid_group_p - Pointer to virtual drive object
 *
 * @return  fbe_bool_t - FBE_TRUE - iw_verify_required is set and thus
 *                                  another verify pass is needed.
 *                       FBE_FALSE - No special action required.
 *
 * @author
 *  5/6/2013 - Copied from fbe_raid_group_metadata_is_error_verify_required().
 *             to create incomplete write flavor Rob Foley
 *
 ******************************************************************************/
fbe_bool_t fbe_raid_group_metadata_is_iw_verify_required(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_bool_t                         b_is_iw_verify_required;
    
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);

    if (nonpaged_metadata_p->raid_group_np_md_flags & FBE_RAID_GROUP_NP_FLAGS_IW_VERIFY_REQUIRED)
    {
        b_is_iw_verify_required = FBE_TRUE;
    }
    else
    {
        b_is_iw_verify_required = FBE_FALSE;
    }

    return b_is_iw_verify_required;
}
/*******************************************************
 * end fbe_raid_group_metadata_is_iw_verify_required()
********************************************************/


/*!****************************************************************************
 *          fbe_raid_group_metadata_write_drive_performance_tier_type()
 ****************************************************************************** 
 * 
 * @brief   This function will set the lowest drive performance tier type of 
 *          the raid group in the nonpaged only if it has changed
 *
 * @param   raid_group_p - Pointer to the raid group
 * @param   packet_p - pointer to a control packet from the scheduler 
 * @param   lowest_drive_tier - the lowest drive tier that was calculated
 *    
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  02/06/2014  Deanna Heng  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_metadata_write_drive_performance_tier_type(fbe_raid_group_t *raid_group_p,
                                                                       fbe_packet_t *packet_p,
                                                                       fbe_drive_performance_tier_type_np_t lowest_drive_tier)
{
    fbe_status_t                        status = FBE_STATUS_OK;             
    fbe_u64_t                           metadata_offset; 
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_drive_performance_tier_type_np_t raid_group_np_lowest_drive_tier;              
    
    /* Get the current value for the non-paged flags.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);
    raid_group_np_lowest_drive_tier = nonpaged_metadata_p->drive_tier;  

    /* Set the value as requested.
     */
    if (lowest_drive_tier != raid_group_np_lowest_drive_tier)
    {
		metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->drive_tier);    
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_INFO, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
							   "%s: Update drive tier type from 0x%x to 0x%x offset: 0x%llx\n", 
                               __FUNCTION__, raid_group_np_lowest_drive_tier, lowest_drive_tier, metadata_offset);  

        /*! @note The metadata operation will copy the value passed into the 
         *        metadata operation before it returns.
         */
            
        status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)raid_group_p, packet_p, 
                                                                 metadata_offset, 
                                                                 (fbe_u8_t*)&lowest_drive_tier,
                                                                 sizeof(fbe_drive_performance_tier_type_np_t));
        
        /*  Check the status of the call and trace if error.  The called function is completing the packet on error
            so we can't complete it here.*/
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s: write drive performance tier type: 0x%x failed with status: 0x%x\n", 
                                  __FUNCTION__, lowest_drive_tier, status);        
        }

        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/*********************************************************
 * end fbe_raid_group_metadata_write_drive_performance_tier_type()
**********************************************************/
/*!****************************************************************************
 * fbe_raid_group_metadata_is_journal_committed()
 ****************************************************************************** 
 * @brief
 *  Determine if we have committed the journal layout yet.
 *
 * @param   raid_group_p - Pointer to virtual drive object
 * @param   b_committed_p - TRUE if committed, FALSE otherwise.
 *
 * @return  fbe_status_t        
 *
 * @author
 *  4/1/2014 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_metadata_is_journal_committed(fbe_raid_group_t *raid_group_p,
                                                          fbe_bool_t *b_committed_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    
    /* Should not be here unless this is a virtual drive.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);

    /* Set the output based on the degraded needs rebuild flag.
     */
    if (nonpaged_metadata_p->raid_group_np_md_extended_flags & FBE_RAID_GROUP_NP_EXTENDED_FLAGS_4K_COMMITTED) {
        *b_committed_p = FBE_TRUE;
    }
    else {
        *b_committed_p = FBE_FALSE;
    }
    return FBE_STATUS_OK;
}
/**********************************************************************
 * end fbe_raid_group_metadata_is_journal_committed()
***********************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_metadata_write_4k_committed()
 ****************************************************************************** 
 * 
 * @brief   This function will 4K Committed flags in the Non-paged area
 *
 * @param   raid_group_p - Pointer to raid group object
 * @param   packet_p - pointer to a control packet from the scheduler 
 * @param   
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  6/5/2014 -  Copied from fbe_raid_group_metadata_write_error_verify_required()
 *              to create 4K Committed . Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_metadata_write_4k_committed(fbe_raid_group_t *raid_group_p,
                                                        fbe_packet_t *packet_p)
{
    fbe_status_t                        status;             
    fbe_u64_t                           metadata_offset; 
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_raid_group_np_metadata_extended_flags_t  raid_group_np_md_extended_flags;              
    
    /* Get the current value for the non-paged flags.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);
    raid_group_np_md_extended_flags = nonpaged_metadata_p->raid_group_np_md_extended_flags;  
     
    raid_group_np_md_extended_flags |= FBE_RAID_GROUP_NP_EXTENDED_FLAGS_4K_COMMITTED;
    
    /*! @note The metadata operation will copy the value passed into the 
     *        metadata operation before it returns.
     */
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->raid_group_np_md_extended_flags);       
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)raid_group_p,
                                                             packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t*)&raid_group_np_md_extended_flags,
                                                             sizeof(fbe_raid_group_np_metadata_extended_flags_t));
    
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: failed with status: 0x%x\n", 
                              __FUNCTION__, status);        
        return FBE_STATUS_GENERIC_FAILURE;         
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/*********************************************************
 * end fbe_raid_group_metadata_write_4k_committed()
**********************************************************/

/***************************************** 
 * end of file fbe_raid_group_metadata.c
 *****************************************/
