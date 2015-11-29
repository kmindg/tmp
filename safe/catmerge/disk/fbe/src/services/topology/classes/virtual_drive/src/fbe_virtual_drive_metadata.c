/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_virtual_drive_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the virtual drive object metadata iniitialization,
 *  read, write, verification code.
 * 
 * @version
 *   12/21/2009:  Created. Dhaval Patel
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_virtual_drive_private.h"
#include "fbe_transport_memory.h"

/*--- local function prototypes --------------------------------------------------------*/
static fbe_status_t fbe_virtual_drive_metadata_write_default_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                                       fbe_packet_completion_context_t context);

/*!**************************************************************
 * fbe_virtual_drive_get_metadata_positions()
 ****************************************************************
 * @brief
 *  This function is used to get the metadata position for the
 *  virtual drive object.
 * 
 * @param virtual_drive_p - Virtual Drive object.
 * @param virtual_drive_metadata_position_p - Pointe to virtual
 *        drive metadata position.
 *                           
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  18/22/2009 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_status_t fbe_virtual_drive_get_metadata_positions(fbe_virtual_drive_t *  virtual_drive_p,    
                                                      fbe_virtual_drive_metadata_positions_t *virtual_drive_metadata_position_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_raid_group_metadata_positions_t raid_group_metadata_positions;

    /*! @note The virtual drive is a leaf class of the raid group.
     */
    status = fbe_raid_group_get_metadata_positions((fbe_raid_group_t *)virtual_drive_p, &raid_group_metadata_positions);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: Failed to get raid group positions. status: 0x%x\n", 
                              __FUNCTION__, status);
        return status;
    }

    /* Use the defined imported capacity to get the virtual drive metadata
     * positions.
     */
    status = fbe_class_virtual_drive_get_metadata_positions(raid_group_metadata_positions.imported_capacity,
                                                            virtual_drive_metadata_position_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: Failed to get virtual drive class positions. status: 0x%x\n", 
                              __FUNCTION__, status);
    }

    return status;
}
/**************************************************
 * end fbe_virtual_drive_get_metadata_positions()
***************************************************/


/*!**************************************************************
 * fbe_virtual_drive_metadata_init_paged_metadata_bits()
 ****************************************************************
 * @brief
 *  This function initializes the paged metadata to default values.
 *
 * @param paged_set_bits_p  - pointer to the paged metadata bits to initialize 
 *
 * @return fbe_status_t
 *
 * @author
 *  10/09/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_virtual_drive_metadata_init_paged_metadata(fbe_virtual_drive_t *virtual_drive_p,
                                                                   fbe_raid_group_paged_metadata_t *paged_set_bits_p)
{
    fbe_raid_position_bitmask_t             need_rebuild_bits = 0;
    fbe_virtual_drive_configuration_mode_t  configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;

    /* Get the virtual drive configuration mode. */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Only pass-thru configuration modes are supported.
     */
    switch (configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            /* Pass-thru first edge.  Second edge should be marked NR.
             */
            need_rebuild_bits |= (1 << SECOND_EDGE_INDEX);
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* Pass-thru second edge.  First edge should be marked NR.
             */
            need_rebuild_bits |= (1 << FIRST_EDGE_INDEX);
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
        default:
            /* Unsupported configuration modes.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unexpected configuration mode: %d \n",
                                  __FUNCTION__, configuration_mode);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set default paged metadata values.
     */
    paged_set_bits_p->valid_bit = 1;
    paged_set_bits_p->verify_bits = 0x00;
    paged_set_bits_p->needs_rebuild_bits = 0;   /*! @note If we update non-paged to proper value.*/
    paged_set_bits_p->reserved_bits = 0x00;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_metadata_init_paged_metadata()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_metadata_write_default_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the metadata with default values for 
 *   the non paged and paged metadata.
 *
 * @param virtual_drive_p         - pointer to the raid group
 * @param packet_p             - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  10/09/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_metadata_write_default_paged_metadata_completion(fbe_packet_t *packet_p,
                                                                                       fbe_packet_completion_context_t context)
{
    fbe_virtual_drive_t                *virtual_drive_p = NULL;
    fbe_payload_metadata_operation_t   *metadata_operation_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_status_t                        status;
    fbe_payload_metadata_status_t       metadata_status;
    
    virtual_drive_p = (fbe_virtual_drive_t *)context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
        
    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* If the request failed fail the packet.
     */
    if ((status != FBE_STATUS_OK)                           ||
        (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK)    )
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s failed - status: 0x%x metadata status: 0x%x\n",
                              __FUNCTION__, status, metadata_status);
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
 * end fbe_virtual_drive_metadata_write_default_paged_metadata_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_metadata_write_default_nonpaged_metadata_completion()
 ******************************************************************************
 *
 * @brief   This is the completion for writing (re-writing) the default
 *          non-paged metadata for the virtual drive.
 *
 * @param   packet_p  - pointer to a monitor packet.
 * @param   context - Pointer to virtual drive object
 *
 * @return  fbe_status_t  
 *
 * @author
 *  07/03/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_metadata_write_default_nonpaged_metadata_completion(fbe_packet_t *packet_p,
                                                                          fbe_packet_completion_context_t context)
{

    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_virtual_drive_t                *virtual_drive_p = NULL;
    fbe_status_t                        packet_status;
    fbe_u64_t                           repeat_count = 0;
    fbe_raid_group_paged_metadata_t     paged_set_bits;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_metadata_operation_t   *metadata_operation_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_lba_t                           paged_metadata_capacity = 0;
    fbe_virtual_drive_configuration_mode_t configuration_mode;

    /* Get the payload and control operation.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)context;
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p); 

    /* Check the packet status.  If the write had an error, then we'll leave 
     * the condition set so that we can try again.  Complete the packet but
     * return `more processing' since the completion will cleanup.
     */ 
    packet_status = fbe_transport_get_status_code(packet_p);
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: error %d on nonpaged write call\n", __FUNCTION__, packet_status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Always mark the control request as success.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* Set the repeat count as number of records in paged metadata capacity.
     * This is the number of chunks of user data + paddings for alignment.
     */
    fbe_base_config_metadata_get_paged_metadata_capacity((fbe_base_config_t *)virtual_drive_p,
                                                       &paged_metadata_capacity);

    repeat_count = (paged_metadata_capacity*FBE_METADATA_BLOCK_DATA_SIZE)/sizeof(fbe_raid_group_paged_metadata_t);

    /* Initialize the paged bit with default values. If it fails set the failure
     * code in the control operation, complete the packet and return
     * `more processing required'.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    status = fbe_virtual_drive_metadata_init_paged_metadata(virtual_drive_p, &paged_set_bits);
    if (status != FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Allocate the metadata operation. */
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p);

    /* Trace this operation.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: write default paged mode: %d nr mask: 0x%x repeat count: 0x%llx \n",
                          configuration_mode, paged_set_bits.needs_rebuild_bits, (unsigned long long)repeat_count);

    /* Build the paged paged metadata write request. */
    status = fbe_payload_metadata_build_paged_write(metadata_operation_p, 
                                                    &(((fbe_base_config_t *)virtual_drive_p)->metadata_element),
                                                    0, /* Paged metadata offset is zero for the default paged metadata write. 
                                                          paint the whole paged metadata with the default value */
                                                    (fbe_u8_t *) &paged_set_bits,
                                                    sizeof(fbe_raid_group_paged_metadata_t),
                                                    repeat_count);
    if (status != FBE_STATUS_OK)
    {
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Now populate the metadata strip lock information */
	/*
    status = fbe_raid_group_bitmap_metadata_populate_stripe_lock(raid_group_p,
                                                                 metadata_operation_p,
                                                                 number_of_chunks_in_paged_metadata);
	*/

    /* set the completion function for the default paged metadata write. */
    fbe_transport_set_completion_function(packet_p, fbe_virtual_drive_metadata_write_default_paged_metadata_completion, virtual_drive_p);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_CANCEL);

    /* Issue the metadata operation. */
    status =  fbe_metadata_operation_entry(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_metadata_write_default_nonpaged_metadata_completion()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_metadata_write_default_nonpaged_metadata()
 *****************************************************************************
 *
 * @brief   This function initializes the non-paged metadata to default values.
 *          it is called in the Activate rotary eiher:
 *              o After a permanent spare has swapped in for a failed drive
 *                                  OR
 *              o After the source drive of a copy operation has failed
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   packet_p - Pointer to packet
 *
 * @return fbe_status_t
 *
 * @author
 *  07/03/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_write_default_nonpaged_metadata(fbe_virtual_drive_t *virtual_drive_p,
                                                                        fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_raid_group_nonpaged_metadata_t  default_nonpaged_metadata;
    fbe_raid_group_nonpaged_metadata_t *default_nonpaged_metadata_p;
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_chunk_index_t                   chunk_index;
    
    /* Get the current value for the non-paged flags.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)virtual_drive_p, (void **)&nonpaged_metadata_p);

    /* Copy the existing data to the structure we will write.
     */
    default_nonpaged_metadata_p = &default_nonpaged_metadata;
    *default_nonpaged_metadata_p = *nonpaged_metadata_p;

    /*! @note Don't modify the base config or the rebuild information.
     */

    /* Set all the verify checkpoints to the end marker.
     */
    default_nonpaged_metadata_p->ro_verify_checkpoint = FBE_LBA_INVALID;
    default_nonpaged_metadata_p->rw_verify_checkpoint = FBE_LBA_INVALID;
    default_nonpaged_metadata_p->error_verify_checkpoint = FBE_LBA_INVALID;
    default_nonpaged_metadata_p->journal_verify_checkpoint = FBE_LBA_INVALID; 
    default_nonpaged_metadata_p->incomplete_write_verify_checkpoint = FBE_LBA_INVALID;    
    default_nonpaged_metadata_p->system_verify_checkpoint = FBE_LBA_INVALID; 

    /* Reset the Needs Rebuild in the non-paged to 0.
     */
    for (chunk_index = 0; chunk_index < FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS; chunk_index++)
    {
        default_nonpaged_metadata_p->paged_metadata_metadata[chunk_index].needs_rebuild_bits = 0;
        default_nonpaged_metadata_p->paged_metadata_metadata[chunk_index].valid_bit = 1;
    }

    /*! @note Don't modify the raid group non-paged flags.
     */

    /* Clear the glitching bitmask.
     */
    default_nonpaged_metadata_p->glitching_disks_bitmask = 0;

    /* set the completion function for the default non-paged metadata write. 
     */
    fbe_transport_set_completion_function(packet_p, fbe_virtual_drive_metadata_write_default_nonpaged_metadata_completion, virtual_drive_p);

    /* Send the nonpaged metadata write request to metadata service. 
     */
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *)virtual_drive_p,
                                                              packet_p,
                                                              0, // offset is zero for the write of entire non paged metadata record. 
                                                              (fbe_u8_t *)default_nonpaged_metadata_p, // non paged metadata memory. 
                                                              sizeof(fbe_raid_group_nonpaged_metadata_t)); // size of the non paged metadata.

    /* Return more processing since a packet is outstanding.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************
 * end fbe_virtual_drive_metadata_write_default_nonpaged_metadata()
 ******************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_is_mark_nr_required()
 ****************************************************************************** 
 * 
 * @brief   Determines if the `mark NR required' flag is set in the non-paged
 *          metadata.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   b_is_mark_nr_required_p - Pointer to bool to update:
 *              FBE_TRUE - Mark NR is set and thus we must run the eval mark NR
 *                  condition.
 *              FBE_FALSE - No special action required.
 *
 * @return  fbe_status_t        
 *
 * @author
 *  08/06/2015  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_is_mark_nr_required(fbe_virtual_drive_t *virtual_drive_p,
                                                            fbe_bool_t *b_is_mark_nr_required_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    
    /* Should not be here unless this is a virtual drive.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)virtual_drive_p, (void **)&nonpaged_metadata_p);
        
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
 * end fbe_virtual_drive_metadata_is_mark_nr_required()
********************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_determine_nonpaged_flags()
 ****************************************************************************** 
 * 
 * @brief   This function will either perform one of the following (3) actions
 *          on the following non-paged metadata flags:
 *              o Edge Swapped - Indicates that this is a new drive
 *              o Source Failed - Indicates that source drive failed during a
 *                  copy operation
 *          Actions:
 *          1. Don't modify
 *          2. Set the bit
 *          3. Clear the bit
 *              
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   set_np_flags_p - Pointer to non-paged flags structure to populate
 * @param   b_modify_edge_swapped - FBE_TRUE - Set the edge swapped bit as requested
 * @param   b_set_clear_edged_swapped - If `b_modify_edge_swapped' is FBE_TRUE, this
 *              is the value to set it to.
 * @param   b_modify_mark_nr_required - FBE_TRUE - Set the mark nr required bit as requested
 * @param   b_set_clear_mark_nr_required - If `b_modify_mark_nr_required' is FBE_TRUE, this
 *              is the value to set it to.
 * @param   b_modify_source_failed - FBE_TRUE - Set the source failed bit as requested
 * @param   b_set_clear_source_failed - If `b_modify_source_failed' is FBE_TRUE, this
 *              is the value to set it to.
 * @param   b_modify_degraded_needs_rebuild - FBE_TRUE - Set degraded needs rebuild as
 *              requested
 * @param   b_set_clear_degraded_needs_rebuild - If `b_modify_degraded_needs_rebuild'
 *              is FBE_TRUE, this is the value to set the degraded needs rebuild flag to.
 * 
 * @return  fbe_status_t  - Always FBE_STATUS_OK
 *
 * @author
 *  10/08/2012  Ron Proulx  - Created.  
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_determine_nonpaged_flags(fbe_virtual_drive_t *virtual_drive_p,
                                                                 fbe_raid_group_np_metadata_flags_t *set_np_flags_p,
                                                                 fbe_bool_t b_modify_edge_swapped,
                                                                 fbe_bool_t b_set_clear_edged_swapped,
                                                                 fbe_bool_t b_modify_mark_nr_required,
                                                                 fbe_bool_t b_set_clear_mark_nr_required,
                                                                 fbe_bool_t b_modify_source_failed,
                                                                 fbe_bool_t b_set_clear_source_failed,
                                                                 fbe_bool_t b_modify_degraded_needs_rebuild,
                                                                 fbe_bool_t b_set_clear_degraded_needs_rebuild)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_raid_group_np_metadata_flags_t  raid_group_np_md_flags;              
    
    /* Get the current value for the non-paged flags.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)virtual_drive_p, (void **)&nonpaged_metadata_p);
    raid_group_np_md_flags = nonpaged_metadata_p->raid_group_np_md_flags;

    /* If the `modify edge swapped' is set, set the flag according.
     */
    if (b_modify_edge_swapped == FBE_TRUE)
    {
        if (b_set_clear_edged_swapped == FBE_TRUE)
        {
            raid_group_np_md_flags |= FBE_RAID_GROUP_NP_FLAGS_SWAPPED_IN;
        }
        else
        {
            raid_group_np_md_flags &= ~FBE_RAID_GROUP_NP_FLAGS_SWAPPED_IN;
        }
    }

    /* If the `modify mark nr required' is set, set the flag according.
     */
    if (b_modify_mark_nr_required == FBE_TRUE)
    {
        if (b_set_clear_mark_nr_required == FBE_TRUE)
        {
            raid_group_np_md_flags |= FBE_RAID_GROUP_NP_FLAGS_MARK_NR_REQUIRED;
        }
        else
        {
            raid_group_np_md_flags &= ~FBE_RAID_GROUP_NP_FLAGS_MARK_NR_REQUIRED;
        }
    }

    /* If the `modify source failed' is set, set the flag according.
     */
    if (b_modify_source_failed == FBE_TRUE)
    {
        if (b_set_clear_source_failed == FBE_TRUE)
        {
            raid_group_np_md_flags |= FBE_RAID_GROUP_NP_FLAGS_SOURCE_DRIVE_FAILED;
        }
        else
        {
            raid_group_np_md_flags &= ~FBE_RAID_GROUP_NP_FLAGS_SOURCE_DRIVE_FAILED;
        }
    }

    /* If the `modify degraded needs rebuild' is set, set the flag according.
     */
    if (b_modify_degraded_needs_rebuild == FBE_TRUE)
    {
        if (b_set_clear_degraded_needs_rebuild == FBE_TRUE)
        {
            raid_group_np_md_flags |= FBE_RAID_GROUP_NP_FLAGS_DEGRADED_NEEDS_REBUILD;
        }
        else
        {
            raid_group_np_md_flags &= ~FBE_RAID_GROUP_NP_FLAGS_DEGRADED_NEEDS_REBUILD;
        }
    }

    /* Copy the local to the output.
     */
    *set_np_flags_p = raid_group_np_md_flags;

    /* Always succeeds
     */
    return FBE_STATUS_OK;
}
/***********************************************************
 * end fbe_virtual_drive_metadata_determine_nonpaged_flags()
 ***********************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_write_nonpaged_flags()
 ****************************************************************************** 
 * 
 * @brief   This function will either set or clear the `edge swapped' non-paged
 *          metadata flag in the virtual drive object. The boolean is set to
 *          true when a permanent spare swaps in for a failed drive.  The boolean
 *          is cleared when RAID has marked NR on the entire disk.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   packet_p - pointer to a control packet from the scheduler 
 * @param   b_edge_swapped - FBE_TRUE - Set the edge swapped flag in the metadata
 *                           FBE_FALSE - Clear the edge swapped flag in the matadata
 * @param   
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   08/31/2011 - Created. Ashwin Tamilarasan. 
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_write_nonpaged_flags(fbe_virtual_drive_t *virtual_drive_p,
                                                             fbe_packet_t *packet_p,
                                                             fbe_raid_group_np_metadata_flags_t md_flags_to_write)
{
    fbe_status_t                        status;   
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_u64_t                           metadata_offset; 
    fbe_raid_group_np_metadata_flags_t  raid_group_np_md_flags = md_flags_to_write;          
    
    /* Trace some information.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)virtual_drive_p, (void **)&nonpaged_metadata_p);          
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: metadata write nonpaged flags from: 0x%x to: 0x%x \n",
                          nonpaged_metadata_p->raid_group_np_md_flags, raid_group_np_md_flags);

    /*! @note The metadata operation will copy the value passed into the 
     *        metadata operation before it returns.
     */
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->raid_group_np_md_flags);       
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)virtual_drive_p, packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t*)&raid_group_np_md_flags,
                                                             sizeof(fbe_raid_group_np_metadata_flags_t));
    
    //  Check the status of the call and trace if error.  The called function is completing the packet on error
    //  so we can't complete it here.
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: write of np flags to: 0x%x failed with status: 0x%x\n", 
                              __FUNCTION__, raid_group_np_md_flags, status);        
        return FBE_STATUS_GENERIC_FAILURE;         
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

}
/********************************************************
 * end fbe_virtual_drive_metadata_write_nonpaged_flags()
********************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_metadata_set_checkpoint_to_end_marker_completion()
 ******************************************************************************
 * @brief
 *   This function is called when the non-paged metadata write to set the 
 *   rebuild checkpoint(s) to the logical end marker has completed.
 * 
 * @param packet_p   - pointer to a control packet from the usurper
 * @param context_p    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  04/13/2013  Ron Proulx  - Updated.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_set_checkpoint_to_end_marker_completion(
                                    fbe_packet_t*                   packet_p,
                                    fbe_packet_completion_context_t context_p)

{
    fbe_status_t                        packet_status;                  // status from packet 
    fbe_virtual_drive_t                *virtual_drive_p;                // pointer to virtual drive object
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;

    /* Get the payload and control operation.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p); 

    /* Always mark the control request as success.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* Get the virtual drive pointer from the input context
     */ 
    virtual_drive_p = (fbe_virtual_drive_t *)context_p; 

    /* Check the packet status.  If the write had an error, then we'll leave 
     * the condition set so that we can try again.  Return success so that 
     * the next completion function on the stack gets called.
     */ 
    packet_status = fbe_transport_get_status_code(packet_p);
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: error %d on nonpaged write call\n", __FUNCTION__, packet_status);
    }

    /* Return success
     */ 
    return FBE_STATUS_OK;
}
/**********************************************************************************
 * end fbe_virtual_drive_metadata_set_checkpoint_to_end_marker_completion()
 *********************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_write_edge_swapped()
 ****************************************************************************** 
 * 
 * @brief   This function will either set or clear the `edge swapped' non-paged
 *          metadata flag in the virtual drive object. The boolean is set to
 *          true when a permanent spare swaps in for a failed drive.  The boolean
 *          is cleared when RAID has marked NR on the entire disk.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   packet_p - pointer to a control packet from the scheduler 
 * @param   b_edge_swapped - FBE_TRUE - Set the edge swapped flag in the metadata
 *                           FBE_FALSE - Clear the edge swapped flag in the matadata
 * @param   
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   08/31/2011 - Created. Ashwin Tamilarasan. 
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_metadata_write_edge_swapped(fbe_virtual_drive_t *virtual_drive_p,
                                                                  fbe_packet_t *packet_p,
                                                                  fbe_bool_t b_edge_swapped)
{
    fbe_status_t                        status;             
    fbe_u64_t                           metadata_offset; 
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_raid_group_np_metadata_flags_t  raid_group_np_md_flags;              
    
    /* Get the current value for the non-paged flags.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)virtual_drive_p, (void **)&nonpaged_metadata_p);
    raid_group_np_md_flags = nonpaged_metadata_p->raid_group_np_md_flags;  
     
    /* Set the value as requested.
     */
    if (b_edge_swapped == FBE_TRUE)
    {
        raid_group_np_md_flags |= FBE_RAID_GROUP_NP_FLAGS_SWAPPED_IN;
    }
    else
    {
        raid_group_np_md_flags &= ~FBE_RAID_GROUP_NP_FLAGS_SWAPPED_IN;
    }

    /*! @note The metadata operation will copy the value passed into the 
     *        metadata operation before it returns.
     */
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->raid_group_np_md_flags);       
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)virtual_drive_p, packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t*)&raid_group_np_md_flags,
                                                             sizeof(fbe_raid_group_np_metadata_flags_t));
    
    //  Check the status of the call and trace if error.  The called function is completing the packet on error
    //  so we can't complete it here.
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: write of edge swapped to: %d failed with status: 0x%x\n", 
                              __FUNCTION__, b_edge_swapped, status);        
        return FBE_STATUS_GENERIC_FAILURE;         
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

}
/*****************************************************
 * end fbe_virtual_drive_metadata_write_edge_swapped()
*****************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_clear_edge_swapped()
 ****************************************************************************** 
 * 
 * @brief   This function will clear the edge swapped flag in the non-paged
 *          metadata.
 *
 * @param   virtual_drive_p - Pointer to virtual drive to clear bit for
 * @param   packet_p - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   08/31/2011 - Created. Ashwin Tamilarasan. 
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_clear_edge_swapped(fbe_virtual_drive_t *virtual_drive_p,
                                                           fbe_packet_t *packet_p)
{
    fbe_status_t    status;             
    
    status = fbe_virtual_drive_metadata_write_edge_swapped(virtual_drive_p, packet_p,
                                                           FBE_FALSE /* Clear the flag */);

    /* Return the execution status.
     */
    return status; 

}
/*****************************************************
 * end fbe_virtual_drive_metadata_clear_edge_swapped()
*****************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_set_edge_swapped()
 ****************************************************************************** 
 * 
 * @brief   This function will set the edge swapped flag in the non-paged
 *          metadata.
 *
 * @param   virtual_drive_p - Pointer to virtual drive to clear bit for
 * @param   packet_p - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   08/31/2011 - Created. Ashwin Tamilarasan. 
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_set_edge_swapped(fbe_virtual_drive_t *virtual_drive_p,
                                                         fbe_packet_t *packet_p)
{
    fbe_status_t    status;             
    
    status = fbe_virtual_drive_metadata_write_edge_swapped(virtual_drive_p, packet_p,
                                                           FBE_TRUE /* Set the flag */);

    /* Return the execution status.
     */
    return status; 

}
/*****************************************************
 * end fbe_virtual_drive_metadata_set_edge_swapped()
*****************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_is_edge_swapped()
 ****************************************************************************** 
 * 
 * @brief   Determines if (1) (does not determine which edge) edge has been
 *          swapped on a virtual drive.
 *
 * @param virtual_drive_p - Pointer to virtual drive object
 * @param b_edge_swapped_p - Pointer to bool to update:
 *                           FBE_TRUE - A spare has swapped into this virtual drive
 *                           FBE_FALSE - The virtual drive does not have an edge
 *                              swapped in.
 *
 * @return fbe_status_t        
 *
 * @author
 *   08/31/2011 - Created. Ashwin Tamilarasan.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_is_edge_swapped(fbe_virtual_drive_t *virtual_drive_p,
                                                        fbe_bool_t *b_edge_swapped_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    
    /* Should not be here unless this is a virtual drive.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)virtual_drive_p, (void **)&nonpaged_metadata_p);
        
    /* Set the output based on the edge swapped flag.
     */
    if (nonpaged_metadata_p->raid_group_np_md_flags & FBE_RAID_GROUP_NP_FLAGS_SWAPPED_IN)
    {
        *b_edge_swapped_p = FBE_TRUE;
    }
    else
    {
        *b_edge_swapped_p = FBE_FALSE;
    }

    //  Return success 
    return FBE_STATUS_OK;

}
/**************************************************
 * end fbe_virtual_drive_metadata_is_edge_swapped()
***************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_write_no_spare_been_reported()
 ****************************************************************************** 
 * 
 * @brief   This function will either set or clear the `no spare reported' 
 *          non-paged metadata flag in the virtual drive object. The boolean is
 *          set to True the first time the virtual drive cannot locate a spare.
 *          We persist this information so that we don't constantly report
 *          `no spare found'.  We clear this flag when the spare request is
 *          successful.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   packet_p - pointer to a control packet from the scheduler 
 * @param   b_set_no_spare_reported - FBE_TRUE - Set the `no spare reported' bit
 *                           FBE_FALSE - Clear the `no spare reported' bit
 * @param   
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   04/25/2012 Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_metadata_write_no_spare_been_reported(fbe_virtual_drive_t *virtual_drive_p,
                                                                            fbe_packet_t *packet_p,
                                                                            fbe_bool_t b_set_no_spare_reported)
{
    fbe_status_t                        status;             
    fbe_u64_t                           metadata_offset; 
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_raid_group_np_metadata_flags_t  raid_group_np_md_flags;              
    
    /* Get the current value for the non-paged flags.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)virtual_drive_p, (void **)&nonpaged_metadata_p);
    raid_group_np_md_flags = nonpaged_metadata_p->raid_group_np_md_flags;  
     
    /* Set the value as requested.
     */
    if (b_set_no_spare_reported == FBE_TRUE)
    {
        raid_group_np_md_flags |= FBE_RAID_GROUP_NP_FLAGS_NO_SPARE_REPORTED;
    }
    else
    {
        raid_group_np_md_flags &= ~FBE_RAID_GROUP_NP_FLAGS_NO_SPARE_REPORTED;
    }

    /*! @note The metadata operation will copy the value passed into the 
     *        metadata operation before it returns.
     */
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->raid_group_np_md_flags);       
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)virtual_drive_p, packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t*)&raid_group_np_md_flags,
                                                             sizeof(fbe_raid_group_np_metadata_flags_t));
    
    //  Check the status of the call and trace if error.  The called function is completing the packet on error
    //  so we can't complete it here.
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: write of no spare reported to: %d failed with status: 0x%x\n", 
                              __FUNCTION__, b_set_no_spare_reported, status);        
        return FBE_STATUS_GENERIC_FAILURE;         
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/***************************************************************
 * end fbe_virtual_drive_metadata_write_no_spare_been_reported()
****************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_clear_no_spare_been_reported()
 ****************************************************************************** 
 * 
 * @brief   This function will clear the no spare reported flag in the non-paged
 *          metadata.
 *
 * @param   virtual_drive_p - Pointer to virtual drive to clear bit for
 * @param   packet_p - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  04/25/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_clear_no_spare_been_reported(fbe_virtual_drive_t *virtual_drive_p,
                                                                     fbe_packet_t *packet_p)
{
    fbe_status_t    status;             
    
    status = fbe_virtual_drive_metadata_write_no_spare_been_reported(virtual_drive_p, packet_p,
                                                                     FBE_FALSE /* Clear the flag */);

    /* Return the execution status.
     */
    return status; 

}
/***************************************************************
 * end fbe_virtual_drive_metadata_clear_no_spare_been_reported()
****************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_set_no_spare_been_reported()
 ****************************************************************************** 
 * 
 * @brief   This function will set the no spare reported flag in the non-paged
 *          metadata.
 *
 * @param   virtual_drive_p - Pointer to virtual drive to clear bit for
 * @param   packet_p - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  04/25/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_set_no_spare_been_reported(fbe_virtual_drive_t *virtual_drive_p,
                                                                   fbe_packet_t *packet_p)
{
    fbe_status_t    status;             
    
    status = fbe_virtual_drive_metadata_write_no_spare_been_reported(virtual_drive_p, packet_p,
                                                                     FBE_TRUE /* Set the flag */);

    /* Return the execution status.
     */
    return status; 
}
/*************************************************************
 * end fbe_virtual_drive_metadata_set_no_spare_been_reported()
**************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_has_no_spare_been_reported()
 ****************************************************************************** 
 * 
 * @brief   Determines if we have already reported that there was not suitable
 *          spare or not.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   b_has_been_reported_p - Pointer to bool:
 *              FBE_TRUE - We have already reported that there was no spare
 *                         available.
 *              FBE_FALSE - We have already reported that there was no suitable
 *                         spare.
 *
 * @return fbe_status_t        
 *
 * @author
 *  04/25/2012  - Ron Proulx    - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_has_no_spare_been_reported(fbe_virtual_drive_t *virtual_drive_p,
                                                                   fbe_bool_t *b_has_been_reported_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    
    /* Should not be here unless this is a virtual drive.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)virtual_drive_p, (void **)&nonpaged_metadata_p);
        
    /* Set the output based on if we have reported that there
     * was no suitable spare or not.
     */
    if (nonpaged_metadata_p->raid_group_np_md_flags & FBE_RAID_GROUP_NP_FLAGS_NO_SPARE_REPORTED)
    {
        *b_has_been_reported_p = FBE_TRUE;
    }
    else
    {
        *b_has_been_reported_p = FBE_FALSE;
    }

    //  Return success 
    return FBE_STATUS_OK;

}
/*************************************************************
 * end fbe_virtual_drive_metadata_has_no_spare_been_reported()
**************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_write_source_failed()
 ****************************************************************************** 
 * 
 * @brief   This function will either set or clear the `source failed' non-paged
 *          metadata flag in the virtual drive object. The boolean is set to
 *          True when the source drive fails during a copy operation and we swap
 *          the source drive and use the destination drive.  This leave the NR
 *          flag set in the paged metadata for the area that was not copied.
 *          This flag indicate that we need to `reset' (a.k.a `write default')
 *          the paged metadata to clear NR for the now source drive (this also
 *          sets the NR bit for the `old' source drive).
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   packet_p - pointer to a control packet from the scheduler 
 * @param   b_source_failed - FBE_TRUE - Set the source failed flag in the metadata
 *                           FBE_FALSE - Clear the source flag in the matadata
 * @param   
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  10/08/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_metadata_write_source_failed(fbe_virtual_drive_t *virtual_drive_p,
                                                                   fbe_packet_t *packet_p,
                                                                   fbe_bool_t b_source_failed)
{
    fbe_status_t                        status;             
    fbe_u64_t                           metadata_offset; 
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_raid_group_np_metadata_flags_t  raid_group_np_md_flags;              
    
    /* Get the current value for the non-paged flags.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)virtual_drive_p, (void **)&nonpaged_metadata_p);
    raid_group_np_md_flags = nonpaged_metadata_p->raid_group_np_md_flags;  
     
    /* Set the value as requested.
     */
    if (b_source_failed == FBE_TRUE)
    {
        raid_group_np_md_flags |= FBE_RAID_GROUP_NP_FLAGS_SOURCE_DRIVE_FAILED;
    }
    else
    {
        raid_group_np_md_flags &= ~FBE_RAID_GROUP_NP_FLAGS_SOURCE_DRIVE_FAILED;
    }

    /*! @note The metadata operation will copy the value passed into the 
     *        metadata operation before it returns.
     */
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->raid_group_np_md_flags);       
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)virtual_drive_p, packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t*)&raid_group_np_md_flags,
                                                             sizeof(fbe_raid_group_np_metadata_flags_t));
    
    //  Check the status of the call and trace if error.  The called function is completing the packet on error
    //  so we can't complete it here.
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: write of source failed to: %d failed with status: 0x%x\n", 
                              __FUNCTION__, b_source_failed, status);        
        return FBE_STATUS_GENERIC_FAILURE;         
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

}
/*******************************************************
 * end fbe_virtual_drive_metadata_write_source_failed()
********************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_clear_source_failed()
 ****************************************************************************** 
 * 
 * @brief   This function will clear the `source failed' flag in the non-paged
 *          metadata.
 *
 * @param   virtual_drive_p - Pointer to virtual drive to clear bit for
 * @param   packet_p - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  10/08/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_clear_source_failed(fbe_virtual_drive_t *virtual_drive_p,
                                                            fbe_packet_t *packet_p)
{
    fbe_status_t    status;             
    
    status = fbe_virtual_drive_metadata_write_source_failed(virtual_drive_p, packet_p,
                                                            FBE_FALSE /* Clear the flag */);

    /* Return the execution status.
     */
    return status; 

}
/*******************************************************
 * end fbe_virtual_drive_metadata_clear_source_failed()
*******************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_set_source_failed()
 ****************************************************************************** 
 * 
 * @brief   This function will set the `source failed' flag in the non-paged
 *          metadata.
 *
 * @param   virtual_drive_p - Pointer to virtual drive to clear bit for
 * @param   packet_p - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  10/08/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_set_source_failed(fbe_virtual_drive_t *virtual_drive_p,
                                                          fbe_packet_t *packet_p)
{
    fbe_status_t    status;             
    
    status = fbe_virtual_drive_metadata_write_source_failed(virtual_drive_p, packet_p,
                                                            FBE_TRUE /* Set the flag */);

    /* Return the execution status.
     */
    return status; 

}
/*****************************************************
 * end fbe_virtual_drive_metadata_set_source_failed()
*****************************************************/

/*!****************************************************************************
 *         fbe_virtual_drive_metadata_is_source_failed()
 ****************************************************************************** 
 * 
 * @brief   Determines if the source drive failed during a copy operation.  If
 *          so we need to clear the NR for the current source position.  We do
 *          this by `writing the default' metadata condition.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   b_is_source_failed_p - Pointer to bool to update:
 *              FBE_TRUE - The source drive failed during a copy operation.
 *              FBE_FALSE - No special action required.
 *
 * @return  fbe_status_t        
 *
 * @author
 *  10/08/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_is_source_failed(fbe_virtual_drive_t *virtual_drive_p,
                                                         fbe_bool_t *b_is_source_failed_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    
    /* Should not be here unless this is a virtual drive.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)virtual_drive_p, (void **)&nonpaged_metadata_p);
        
    /* Set the output based on the edge swapped flag.
     */
    if (nonpaged_metadata_p->raid_group_np_md_flags & FBE_RAID_GROUP_NP_FLAGS_SOURCE_DRIVE_FAILED)
    {
        *b_is_source_failed_p = FBE_TRUE;
    }
    else
    {
        *b_is_source_failed_p = FBE_FALSE;
    }

    //  Return success 
    return FBE_STATUS_OK;

}
/***************************************************
 * end fbe_virtual_drive_metadata_is_source_failed()
****************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_write_degraded_needs_rebuild()
 ****************************************************************************** 
 * 
 * @brief   This function will either set or clear the `degraded needs rebuild' 
 *          non-paged metadata flag in the virtual drive object. The boolean is 
 *          set to True when the virtual drive is in pass-thru mode but the
 *          rebuild checkpoint is not at the end marker.  It indicates that the
 *          parent raid group must mark Needs Rebuild before this position is 
 *          ready.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   packet_p - pointer to a control packet from the scheduler 
 * @param   b_degraded_needs_rebuild - FBE_TRUE - Set the degraded needs rebuild 
 *                                      flag in the metadata
 *                           FBE_FALSE - Clear the degraded needs rebuild flag 
 *                                      in the matadata
 * @param   
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  04/17/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_metadata_write_degraded_needs_rebuild(fbe_virtual_drive_t *virtual_drive_p,
                                                                   fbe_packet_t *packet_p,
                                                                   fbe_bool_t b_degraded_needs_rebuild)
{
    fbe_status_t                        status;             
    fbe_u64_t                           metadata_offset; 
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_raid_group_np_metadata_flags_t  raid_group_np_md_flags;              
    
    /* Get the current value for the non-paged flags.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)virtual_drive_p, (void **)&nonpaged_metadata_p);
    raid_group_np_md_flags = nonpaged_metadata_p->raid_group_np_md_flags;  
     
    /* Set the value as requested.
     */
    if (b_degraded_needs_rebuild == FBE_TRUE)
    {
        raid_group_np_md_flags |= FBE_RAID_GROUP_NP_FLAGS_DEGRADED_NEEDS_REBUILD;
    }
    else
    {
        raid_group_np_md_flags &= ~FBE_RAID_GROUP_NP_FLAGS_DEGRADED_NEEDS_REBUILD;
    }

    /*! @note The metadata operation will copy the value passed into the 
     *        metadata operation before it returns.
     */
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->raid_group_np_md_flags);       
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)virtual_drive_p, packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t*)&raid_group_np_md_flags,
                                                             sizeof(fbe_raid_group_np_metadata_flags_t));
    
    /* Check the status of the call and trace if error.  The called function 
     * is completing the packet on error so we can't complete it here.
     */
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: write of degraded needs rebuild to: %d failed with status: 0x%x\n", 
                              __FUNCTION__, b_degraded_needs_rebuild, status);        
        return FBE_STATUS_GENERIC_FAILURE;         
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

}
/***************************************************************
 * end fbe_virtual_drive_metadata_write_degraded_needs_rebuild()
****************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_clear_degraded_needs_rebuild()
 ****************************************************************************** 
 * 
 * @brief   This function will clear the `degraded needs rebuild' flag in the 
 *          non-paged metadata.
 *
 * @param   virtual_drive_p - Pointer to virtual drive to clear bit for
 * @param   packet_p - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  04/17/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_clear_degraded_needs_rebuild(fbe_virtual_drive_t *virtual_drive_p,
                                                            fbe_packet_t *packet_p)
{
    fbe_status_t    status;             
    
    status = fbe_virtual_drive_metadata_write_degraded_needs_rebuild(virtual_drive_p, packet_p,
                                                            FBE_FALSE /* Clear the flag */);

    /* Return the execution status.
     */
    return status; 

}
/***************************************************************
 * end fbe_virtual_drive_metadata_clear_degraded_needs_rebuild()
****************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_set_degraded_needs_rebuild()
 ****************************************************************************** 
 * 
 * @brief   This function will set the `degraded needs rebuild' flag in the 
 *          non-paged metadata.
 *
 * @param   virtual_drive_p - Pointer to virtual drive to set bit for
 * @param   packet_p - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  04/17/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_set_degraded_needs_rebuild(fbe_virtual_drive_t *virtual_drive_p,
                                                          fbe_packet_t *packet_p)
{
    fbe_status_t    status;             
    
    status = fbe_virtual_drive_metadata_write_degraded_needs_rebuild(virtual_drive_p, packet_p,
                                                            FBE_TRUE /* Set the flag */);

    /* Return the execution status.
     */
    return status; 

}
/*************************************************************
 * end fbe_virtual_drive_metadata_set_degraded_needs_rebuild()
**************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_metadata_is_degraded_needs_rebuild()
 ****************************************************************************** 
 * 
 * @brief   Determines if the `degraded needs rebuild' flag is set in the
 *          non-paged metadata.  This flag indicates that the virtual drive is
 *          in pass-thru mode but the rebuild checkpoint is not at the end
 *          marker.  The parent raid must:
 *              o Mark the extent that was not rebuild with Needs Rebuild
 *              o Rebuild the extent using redundancy
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   b_is_degraded_needs_rebuild_p - Pointer to bool to update:
 *              FBE_TRUE - The virtual drive rebuild checkpoint is not at the end
 *                  marker
 *              FBE_FALSE - No special action required.
 *
 * @return  fbe_status_t        
 *
 * @author
 *  04/17/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_metadata_is_degraded_needs_rebuild(fbe_virtual_drive_t *virtual_drive_p,
                                                                  fbe_bool_t *b_is_degraded_needs_rebuild_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    
    /* Should not be here unless this is a virtual drive.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)virtual_drive_p, (void **)&nonpaged_metadata_p);
        
    /* Set the output based on the `degraded needs rebuild' non-paged flag.
     */
    if (nonpaged_metadata_p->raid_group_np_md_flags & FBE_RAID_GROUP_NP_FLAGS_DEGRADED_NEEDS_REBUILD)
    {
        *b_is_degraded_needs_rebuild_p = FBE_TRUE;
    }
    else
    {
        *b_is_degraded_needs_rebuild_p = FBE_FALSE;
    }

    /* Return success
     */
    return FBE_STATUS_OK;

}
/************************************************************
 * end fbe_virtual_drive_metadata_is_degraded_needs_rebuild()
 ************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_set_swap_operation_start_nonpaged_flags()
 ******************************************************************************
 * @brief
 *   This function is called to set the non-paged flags when either a permanent
 *   spare request or a copy request is started.  It is invoked prior to the
 *   edge being swapped it.  The two possible non-paged flags set are:
 *      o Set permananet spare
 *      o Mark NR required (mark NR must run before clearing rl)
 * 
 * @param   virtual_drive_p - Pointer to virtual drive object to set checkpoint for
 * @param   packet_p               - pointer to a control packet from the scheduler
 * 
 * @return  fbe_status_t  
 *
 * @note    Completion - Return status
 *
 * @author
 *  08/06/2015  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_set_swap_operation_start_nonpaged_flags(fbe_virtual_drive_t *virtual_drive_p,
                                                                       fbe_packet_t *packet_p)

{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_spare_swap_command_t            swap_command;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_bool_t                          b_is_mark_nr_required = FBE_FALSE;
    fbe_raid_group_np_metadata_flags_t  raid_group_np_flags_to_set;

    /* Get the control operation
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Get the copy request type
     */
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &swap_command);

    /* Based on the swap operation (either permanent spare or copy) set the
     * permanent spare only or both the permanent spare and mark NR.
     */
    if (swap_command != FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND)
    {
        b_is_mark_nr_required = FBE_TRUE;
    }

    /* Determine if we need to clear the swapped bit or not.
     */
    fbe_virtual_drive_metadata_determine_nonpaged_flags(virtual_drive_p,
                                                        &raid_group_np_flags_to_set,
                                                        FBE_TRUE,           /* Always set the edged swapped bit */
                                                        FBE_TRUE,           /* Always set the edged swapped bit */
                                                        b_is_mark_nr_required,  /* If mark nr required is set we want to set it */
                                                        FBE_TRUE,           /* If mark nr required is set we want to setr it */
                                                        FBE_FALSE,          /* Don't modify the source drive failed flag */
                                                        FBE_FALSE,          /* Don't modify the source drive failed flag */
                                                        FBE_FALSE,          /* Don't modify the degraded needs rebuild flag */
                                                        FBE_FALSE           /* Don't modify the degraded needs rebuild flag */);

    /* Now write the updated non-paged flags.
     */
    status = fbe_virtual_drive_metadata_write_nonpaged_flags(virtual_drive_p,
                                                             packet_p,
                                                             raid_group_np_flags_to_set);
    if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s edge swapped: %d mark nr required: %d write failed - status: 0x%x\n",
                              __FUNCTION__, FBE_TRUE, b_is_mark_nr_required, status);
    
        /* Fail the request.
         */
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* Return more processing required.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*****************************************************************
 * end fbe_virtual_drive_set_swap_operation_start_nonpaged_flags()
 *****************************************************************/


/**********************************
 * end fbe_virtual_drive_metadata.c
 **********************************/
