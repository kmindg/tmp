/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_expansion.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the raid group object code for handling the expansion functionality.
 *  This functionality only increases the capacity of a raid group into unused space,
 *  relocating the paged to the end of this free space.
 * 
 * @ingroup raid_group_class_files
 * 
 * @version
 *   10/04/2013:  Created. Shay Harel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_object.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_group_rebuild.h"
#include "fbe_raid_library.h"
#include "fbe_raid_verify.h"
#include "fbe_raid_group_bitmap.h"
#include "fbe/fbe_winddk.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_cmi.h"



/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

void fbe_raid_group_get_quiesced_io(fbe_raid_group_t *raid_group_p, fbe_packet_t **packet_pp);
static fbe_status_t fbe_raid_group_expansion_usurper_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_change_capacity(fbe_raid_group_t * raid_group_p, fbe_lba_t new_capacity);

static fbe_status_t fbe_raid_group_get_update_capacity(fbe_raid_group_t *raid_group_p, 
                                                fbe_packet_t *packet_p,
                                                fbe_lba_t *cap_p);
static void fbe_raid_group_round_capacity(fbe_raid_group_t *raid_group_p, 
                                          fbe_lba_t requested_capacity, 
                                          fbe_lba_t *new_capacity_p);

/*!**************************************************************
 * fbe_raid_group_usurper_start_expansion()
 ****************************************************************
 * @brief
 *  This function starts the expansion of the RG
 *
 * @param raid_group_p - The raid group.
 * @param packet_p - The usurper packet.
 *
 * @return status - The status of the operation.
 *
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_usurper_start_expansion(fbe_raid_group_t * raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_lba_t rounded_exported_capacity  = FBE_LBA_INVALID;
    fbe_lba_t current_exported_capacity = FBE_LBA_INVALID;
    fbe_lba_t new_capacity;

    /* Add a block operation since everything on the terminator queue has one.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);

    /* Fetch the new capacity for the raid group.
     */
    status = fbe_raid_group_get_update_capacity(raid_group_p, packet_p, &new_capacity);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "change capacity failed to get capacity status: %d\n", status);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    /* Convert the new capacity into a properly rounded capacity.
     */
    fbe_raid_group_round_capacity(raid_group_p, new_capacity, &rounded_exported_capacity);

    fbe_base_config_get_capacity(&raid_group_p->base_config, &current_exported_capacity);

    /* If the capacity is already changed, then just return success.
     */
    if (current_exported_capacity == rounded_exported_capacity) {
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "Expansion already changed capacity, return success.\n");
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);
    fbe_payload_ex_get_iots(payload_p, &iots_p);
    fbe_payload_block_build_operation(block_operation_p, 
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID,
                                      0,
                                      0,
                                      FBE_BE_BYTES_PER_BLOCK, 1,    /* optimal block size */ 
                                      NULL);
    status = fbe_payload_ex_increment_block_operation_level(payload_p);

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_expansion_usurper_completion, raid_group_p);

    /* Add this packet to the usurper queue. If a shutdown occurs then this gets completed with error.
     */
    fbe_base_object_add_to_usurper_queue((fbe_base_object_t*)raid_group_p, packet_p);

    fbe_raid_group_lock(raid_group_p);

    /* Set condition so that we will process this usurper in monitor context
     */
    fbe_raid_group_metadata_memory_set_capacity(raid_group_p, new_capacity);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_REQUEST);
    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                           (fbe_base_object_t *) raid_group_p,
                           FBE_RAID_GROUP_LIFECYCLE_COND_CONFIGURATION_CHANGE);
    fbe_raid_group_unlock(raid_group_p);

    status = fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const,
                                      (fbe_base_object_t *) raid_group_p,
                                      (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_usurper_start_expansion()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_round_capacity()
 ****************************************************************
 * @brief
 *  Convert an input new requested capacity and convert it into
 *  a properly rounded capacity.
 *
 * @param raid_group_p - current RG.
 * @param requested_capacity - Capacity we want to change to.
 * @param new_capacity_p - The properly rounded capacity output.
 * 
 * @return None.   
 *
 * @author
 *  10/23/2013 - Created. Rob Foley
 *
 ****************************************************************/

static void fbe_raid_group_round_capacity(fbe_raid_group_t *raid_group_p, 
                                          fbe_lba_t requested_capacity, 
                                          fbe_lba_t *new_capacity_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_lba_t rounded_exported_capacity  = FBE_LBA_INVALID;
    fbe_u16_t data_disks;
    fbe_lba_t user_blocks_per_chunk = FBE_LBA_INVALID;

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    user_blocks_per_chunk = FBE_RAID_DEFAULT_CHUNK_SIZE * data_disks;
    if (requested_capacity % user_blocks_per_chunk) {
        rounded_exported_capacity = requested_capacity + user_blocks_per_chunk - (requested_capacity % user_blocks_per_chunk);
    }
    else {
        rounded_exported_capacity = requested_capacity;
    }
    *new_capacity_p = rounded_exported_capacity;
}
/******************************************
 * end fbe_raid_group_round_capacity()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_get_update_capacity()
 ****************************************************************
 * @brief
 *   Get capacity to use to change the capacity.
 *
 * @param raid_group_p -
 * @param packet_p - 
 * @param cap_p - output capacity to update to.
 *
 * @return FBE_STATUS_OK - Success
 *         FBE_STATUS_GENERIC_FAILURE - On Error.
 *
 * @author
 *  10/18/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_get_update_capacity(fbe_raid_group_t *raid_group_p, 
                                                       fbe_packet_t *packet_p,
                                                       fbe_lba_t *cap_p)
{
    fbe_raid_group_configuration_t *set_config_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL; 
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_present_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &set_config_p);
    if (set_config_p == NULL) {
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "change capacity fbe_payload_control_get_buffer failed\n");

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *cap_p = set_config_p->capacity;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_get_update_capacity()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_change_capacity()
 ****************************************************************
 * @brief
 *  Modify the capacity of the raid group either up or down.
 * 
 *  Assumes that the paged is already moved and that the journal is not in use.
 *
 * @param raid_group_p -
 * @param packet_p - 
 * @param new_capacity - The new capacity.
 *
 * @return FBE_STATUS_OK - Success
 *         FBE_STATUS_GENERIC_FAILURE - On Error.
 *
 * @author
 *  10/18/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_change_capacity(fbe_raid_group_t * raid_group_p, fbe_lba_t new_capacity)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t data_disks;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_group_metadata_positions_t raid_group_metadata_positions;
    fbe_lba_t rounded_exported_capacity  = FBE_LBA_INVALID;
    fbe_lba_t configured_total_capacity = FBE_LBA_INVALID;
    fbe_u64_t stripe_number = 0;
    fbe_u64_t number_of_stripes = 0;
    fbe_u64_t number_of_user_stripes = 0;
    fbe_u64_t stripe_count = 0;
    fbe_u64_t user_stripe_count = 0;
    
    fbe_raid_group_lock(raid_group_p);
    
    fbe_raid_group_round_capacity(raid_group_p, new_capacity, &rounded_exported_capacity);

    /* Set the exported capacity on block transport server. */
    fbe_base_config_set_capacity((fbe_base_config_t*)raid_group_p, rounded_exported_capacity);

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    /* Calculate the metadata positions based on the exported capacity. */
    status = fbe_raid_group_class_get_metadata_positions(rounded_exported_capacity,
                                                         FBE_LBA_INVALID,
                                                         data_disks,
                                                         &raid_group_metadata_positions,
                                                         raid_geometry_p->raid_type);
    if(status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: calculation of imported capacity failed.\n", __FUNCTION__);
        fbe_raid_group_unlock(raid_group_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Calculate the configured total capacity (exported capacity + metadata capacity). */
    configured_total_capacity = rounded_exported_capacity + raid_group_metadata_positions.paged_metadata_capacity;    
    
    /* Configuring the raid geometry could fail if the configuration isn't
     * supported for this type of raid group etc.
     */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    status = fbe_raid_geometry_set_configuration(raid_geometry_p,
                                                 raid_geometry_p->width,
                                                 raid_geometry_p->raid_type,
                                                 raid_geometry_p->element_size,
                                                 raid_geometry_p->elements_per_parity,
                                                 configured_total_capacity,
                                                 raid_geometry_p->max_blocks_per_drive);
    if (status != FBE_STATUS_OK) { 
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s set configuration failed, status:0x%x.\n",
                              __FUNCTION__, status);
        return status; 
    }

    /* Set the metadata start lba in the raid geometry
     */
    fbe_raid_geometry_set_metadata_configuration(raid_geometry_p, 
                                                 raid_group_metadata_positions.paged_metadata_lba,
                                                 raid_group_metadata_positions.paged_metadata_capacity,
                                                 FBE_LBA_INVALID,
                                                 raid_group_metadata_positions.journal_write_log_pba);

    status = fbe_raid_group_get_metadata_positions(raid_group_p, &raid_group_metadata_positions);
    if(status != FBE_STATUS_OK) {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s metadata position calculation failed, status:0x%x.\n",
                              __FUNCTION__, status);
        return status;
    }

    /* initialize metadata element with metadata position information. */
    fbe_base_config_metadata_set_paged_record_start_lba((fbe_base_config_t *) raid_group_p,
                                                        raid_group_metadata_positions.paged_metadata_lba);
    fbe_base_config_metadata_set_paged_metadata_capacity((fbe_base_config_t *) raid_group_p,
                                                         raid_group_metadata_positions.paged_metadata_capacity);

    /* The total number of stripes is determined by the total raid group capacity.
     * We use the last accessible lba and then add one to get the total stripe count.
     */
    fbe_raid_geometry_calculate_lock_range(raid_geometry_p,
                                           (raid_group_metadata_positions.raid_capacity - 1),
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
                                           (raid_group_metadata_positions.paged_metadata_lba - 1),
                                           1,
                                           &number_of_user_stripes,
                                           &user_stripe_count);
    number_of_user_stripes ++;
    fbe_base_config_metadata_set_number_of_private_stripes((fbe_base_config_t *) raid_group_p,
                                                            ( number_of_stripes - number_of_user_stripes));

    /* Tell the metadata stripe locks to recalculate based on the new parameters in the metadata element.
     */
    metadata_stripe_lock_update(&raid_group_p->base_config.metadata_element);

    fbe_raid_group_unlock(raid_group_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_change_capacity()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_expansion_usurper_completion()
 ****************************************************************
 * @brief
 *   This completion function handles a usurper that is completing
 *   from the raid group.
 *   Since the usurper had a block operation added to it, we will
 *   remove that block operation now.
 *
 * @param packet_p - Current usurper packet.
 * @param context - Raid group, not used.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/23/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_expansion_usurper_completion(fbe_packet_t * packet_p, 
                                                                fbe_packet_completion_context_t context)
{
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p); 
    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_expansion_usurper_completion()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_expansion_paged_write_completion()
 ****************************************************************
 * @brief
 *   This completion function handles a completion
 *   of a paged reconstruct.
 *
 * @param packet_p - Current usurper packet.
 * @param context - Raid group, not used.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/23/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_expansion_paged_write_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t * raid_group_p = (fbe_raid_group_t*)context;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_lba_t paged_md_start_lba;
    fbe_block_count_t paged_md_blocks;
    fbe_u16_t data_disks;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    status = fbe_transport_get_status_code(packet_p);

    if (status != FBE_STATUS_OK) {
        /* We failed, return the packet with this bad status.
         */
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: status: 0x%x\n", __FUNCTION__, status);

        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_ERROR);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_DONE);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_STARTED);
        fbe_raid_group_unlock(raid_group_p);
        fbe_payload_ex_release_block_operation(payload_p, block_operation_p);
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                          FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "Exp paged write complete lba: 0x%llx bl: 0x%llx.\n", block_operation_p->lba, block_operation_p->block_count);

    fbe_base_config_metadata_get_paged_record_start_lba(&(raid_group_p->base_config), &paged_md_start_lba);
    fbe_base_config_metadata_get_paged_metadata_capacity(&(raid_group_p->base_config), &paged_md_blocks);

    /* Determine the start lba for the next rebuild of paged.
     */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    paged_md_start_lba /= data_disks;
    paged_md_blocks /= data_disks;
   
    /* Goto the next chunk.
     */ 
    block_operation_p->lba += FBE_RAID_DEFAULT_CHUNK_SIZE;
    if (block_operation_p->lba < paged_md_start_lba + paged_md_blocks) {
        /* More to do.
         */
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "Exp paged write lba: 0x%llx bl: 0x%llx.\n", block_operation_p->lba, block_operation_p->block_count);
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_expansion_paged_write_completion, raid_group_p);
        fbe_raid_group_bitmap_reconstruct_paged_metadata(raid_group_p, 
                                                         packet_p, block_operation_p->lba, FBE_RAID_DEFAULT_CHUNK_SIZE, 
                                                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else {
        /* All done
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Exp paged write done rebuilding paged.\n");

        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_DONE);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_STARTED);
        fbe_raid_group_unlock(raid_group_p);
        return FBE_STATUS_OK;
    }
}
/**************************************
 * end fbe_raid_group_expansion_paged_write_completion
 **************************************/

/*!**************************************************************
 * fbe_raid_group_expansion_zero_paged_metadata_completion()
 ****************************************************************
 *
 * @brief   This is he completion after we zero the expanded paged 
 *          metadata (before writing it). This is similar to when the 
 *          raid group is first created.
 * 
 * @param   packet_p      - Current packet.
 * @param   context - Pointer to raid group object
 * 
 * @return fbe_status_t
 *
 * @author
 *  01/16/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_expansion_zero_paged_metadata_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t                        status;
    fbe_raid_group_t                   *raid_group_p = (fbe_raid_group_t*)context;
    fbe_payload_block_operation_t      *block_operation_p = NULL;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_lba_t                           paged_md_start_lba;
    fbe_u16_t                           data_disks;
    fbe_raid_geometry_t                *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    status = fbe_transport_get_status_code(packet_p);

    if (status != FBE_STATUS_OK) {
        /* We failed, return the packet with this bad status.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: status: 0x%x \n", 
                              __FUNCTION__, status);

        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_ERROR);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_DONE);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_STARTED);
        fbe_raid_group_unlock(raid_group_p);
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                          FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "Exp zero paged complete \n");

    fbe_base_config_metadata_get_paged_record_start_lba(&(raid_group_p->base_config),
                                                            &paged_md_start_lba);
    /* Determine the start lba for the rebuild of paged.
     */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    paged_md_start_lba /= data_disks;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);
    fbe_payload_block_build_operation(block_operation_p, 
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID,
                                      paged_md_start_lba,
                                      FBE_RAID_DEFAULT_CHUNK_SIZE,
                                      FBE_BE_BYTES_PER_BLOCK, 1,    /* optimal block size */ 
                                      NULL);
    status = fbe_payload_ex_increment_block_operation_level(payload_p);

    fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "Exp paged write start lba: 0x%llx bl: 0x%llx.\n", block_operation_p->lba, block_operation_p->block_count);
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_expansion_paged_write_completion, raid_group_p);
    fbe_raid_group_bitmap_reconstruct_paged_metadata(raid_group_p, 
                                                     packet_p, paged_md_start_lba, FBE_RAID_DEFAULT_CHUNK_SIZE, 
                                                     FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************************************
 * end fbe_raid_group_expansion_zero_paged_metadata_completion()
 **************************************************************/

/*!**************************************************************
 * fbe_raid_group_start_expansion()
 ****************************************************************
 * @brief
 *  Start the expansion operation.
 *  We will change the in memory capacity and then
 *  reconstruct the paged in the new location.
 * 
 * @param raid_group_p  - Current rg
 * @param packet_p      - Current packet.
 * @param new_capacity  - New capacity to expand to.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/23/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_start_expansion(fbe_raid_group_t * raid_group_p, 
                                                   fbe_packet_t *packet_p,
                                                   fbe_lba_t new_capacity)
{
    fbe_status_t status;
    fbe_scheduler_hook_status_t hook_status;
    fbe_lba_t orig_capacity = FBE_LBA_INVALID;

    /* Save the old value of capacity so we can set back to this on error.
     */
    fbe_base_config_get_capacity((fbe_base_config_t*)raid_group_p, &orig_capacity);
    fbe_raid_group_set_last_capacity(raid_group_p, orig_capacity);

    /* Before we relocate the paged, change our capacity.
     */
    status = fbe_raid_group_change_capacity(raid_group_p, new_capacity);

    /* When the hook is set, fail the change capacity operation. */
    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                               FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_ERROR_ACTIVE, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) { status = FBE_STATUS_FAILED; }

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "Change capacity failed with status: 0x%x  Unquiescing.\n", status);
        /* Note the error and transition to done to revert the capacity change.
         */
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_ERROR);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_DONE);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_STARTED);
        fbe_raid_group_unlock(raid_group_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        /* Schedule monitor.
         */
        status = fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const,
                                          (fbe_base_object_t *) raid_group_p,
                                          (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */
    }
    else {
        /* Before we reconstruct the paged we must first zero it.  This is
         * similar to when the raid group is first created.
         */
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_expansion_zero_paged_metadata_completion, raid_group_p);
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_zero_journal_completion, raid_group_p);

        /* Zero the metadata area for this raid group
         */
        status = fbe_raid_group_metadata_zero_metadata(raid_group_p, packet_p);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_start_expansion()
 ******************************************/

/*!****************************************************************************
 * fbe_raid_group_change_config_request()
 ******************************************************************************
 * @brief
 *  Setup the change config operation across both SPs.
 *  Once both SPs are ready the SPs will move to the started state.
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  10/18/2013 - Created. Rob Foley
 *  
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_change_config_request(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p)
{
    fbe_bool_t           b_is_active;
    fbe_bool_t           b_peer_flag_set = FBE_FALSE;
    fbe_scheduler_hook_status_t        hook_status = FBE_SCHED_STATUS_OK;
    
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    fbe_raid_group_lock(raid_group_p);

    if (!fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_REQUEST)){
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_REQUEST);
    }
    /* Either the active or passive side can initiate this. 
     * Start the process by setting the change configuration request to get both sides running this condition.
     */
    if (!fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_REQ)) {
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_REQ);

        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s marking CHANGE_CONFIG_REQ state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                               FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_REQUEST, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Now make sure the peer also has this flag set.
     */
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_REQ);

    if (!b_peer_flag_set) {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "CCfg %s, wait peer request local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    } else {

        /* Save what the peer might have sent to us in our metadata memory.
         */
        if ((raid_group_p->raid_group_metadata_memory.capacity_expansion_blocks == 0) &&
            (raid_group_p->raid_group_metadata_memory_peer.capacity_expansion_blocks != 0)){

            fbe_raid_group_metadata_memory_set_capacity(raid_group_p, 
                                                        raid_group_p->raid_group_metadata_memory_peer.capacity_expansion_blocks);
        }
        /* The peer has set the flag, we are ready to do the next set of checks.
         */
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_STARTED);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_REQUEST);
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "CCfg %s peer req set local: 0x%llx peer: 0x%llx state: 0x%llx 0x%x\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state,
                              raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }            
}
/**************************************************
 * end fbe_raid_group_change_config_request()
 **************************************************/

/*!****************************************************************************
 * fbe_raid_group_change_config_passive()
 ******************************************************************************
 * @brief
 *  Passive side updates the in memory information with the new capacity.
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  10/18/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_change_config_passive(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_bool_t       b_peer_flag_set = FBE_FALSE;
    fbe_bool_t       b_local_flag_set = FBE_FALSE;
    fbe_bool_t           b_is_active;
    fbe_scheduler_hook_status_t        hook_status = FBE_SCHED_STATUS_OK;

    fbe_raid_group_lock(raid_group_p);

    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_STARTED);
    if (fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_STARTED)) {
        b_local_flag_set = FBE_TRUE;
    }    

    /* We first wait for the peer to start the eval. 
     * This is needed since either active or passive can initiate the eval. 
     * Once we see this set, we will set our flag, which the peer waits for before it completes the eval.  
     */
    if (!b_peer_flag_set && !b_local_flag_set) {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive waiting peer CHANGE_CONFIG_STARTED local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                               FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_STARTED, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* At this point the peer started flag is set.
     * If we have not set this yet locally, set it and wait for the condition to run to send it to the peer.
     */
    if (!b_local_flag_set) { 
        fbe_lba_t orig_capacity = FBE_LBA_INVALID;
        fbe_scheduler_hook_status_t hook_status;
        fbe_raid_group_unlock(raid_group_p);
        fbe_metadata_element_clear_abort(&raid_group_p->base_config.metadata_element);

        /* Save the old value of capacity so we can set back to this on error.
         */
        fbe_base_config_get_capacity((fbe_base_config_t*)raid_group_p, &orig_capacity);
        fbe_raid_group_set_last_capacity(raid_group_p, orig_capacity);

        status = fbe_raid_group_change_capacity(raid_group_p, 
                                                raid_group_p->raid_group_metadata_memory.capacity_expansion_blocks);

        fbe_check_rg_monitor_hooks(raid_group_p,
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                                   FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_ERROR_PASSIVE, 
                                   &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE){
            /* When the hook is set, fail the change capacity operation.
             */
            status = FBE_STATUS_FAILED;
        }
        fbe_raid_group_lock(raid_group_p);
        if (status != FBE_STATUS_OK){
            fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                                   FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "Passive change capacity failed with status: 0x%x.\n", status);

            /*! need to fail the overall operation.  We will let it fall through to done eventually where we clean up.
             */
            fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_DONE_ERROR);
        }

        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_STARTED);
        fbe_raid_group_unlock(raid_group_p);

        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive marking CHANGE_CONFIG_STARTED loc fl: 0x%llx peer fl: 0x%x>\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned int)raid_group_p->raid_group_metadata_memory_peer.flags);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If we become active, just return out.
     * We need to handle the active's work ourself. 
     * When we come back into the condition we'll automatically execute the active code.
     */
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
    if (b_is_active) {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive CHANGE_CONFIG became active local: 0x%llx peer: 0x%llx lstate: 0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                               FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_STARTED2, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Once we have set the started flag, we simply wait for the active side to complete. 
     * We know Active is done when it clears the flags. 
     */
    if (!b_peer_flag_set) {
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_DONE);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_STARTED);
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "CCfg Passive start cleanup local: 0x%llx peer: 0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    } else {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "CCfg Passive wait peer complete loc: 0x%llx peer: 0x%llx lstate: 0x%llx bcf: 0x%llx pp: %d\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.flags,
                              (raid_group_p->base_config.metadata_element.peer_metadata_element_ptr != 0));
    }

    fbe_raid_group_unlock(raid_group_p);
    return FBE_LIFECYCLE_STATUS_DONE;    
}
/**************************************************
 * end fbe_raid_group_change_config_passive()
 **************************************************/

/*!****************************************************************************
 * fbe_raid_group_change_config_active()
 ******************************************************************************
 * @brief
 *  This condition handles the active side of changing the configuration.
 * 
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  10/22/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_change_config_active(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{    
    fbe_bool_t                      b_peer_flag_set = FBE_FALSE;
    fbe_status_t                    status;
    fbe_raid_geometry_t            *raid_geometry_p = NULL;            
    fbe_scheduler_hook_status_t     hook_status = FBE_SCHED_STATUS_OK;
    fbe_bool_t b_quiesce_not_set = FBE_FALSE;
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    /* Quiesce the IO before proceeding */
    status = fbe_raid_group_start_drain(raid_group_p);
    if (status == FBE_STATUS_PENDING) {
        return FBE_LIFECYCLE_STATUS_RESCHEDULE; 
    }

    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                          FBE_TRACE_LEVEL_INFO,           
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,  
                          "CCfg active quiesced local state: 0x%llx \n", 
                          (unsigned long long)raid_group_p->local_state);

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                               FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_STARTED, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_raid_group_lock(raid_group_p);
    /* Now we set the bit that says we are starting. 
     * This bit always gets set first by the Active side. 
     * When passive sees the started peer bit, it will also set the started bit. 
     * The passive side will thereafter know we are done when this bit is cleared. 
     */
    if (!fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_STARTED)) {
        fbe_metadata_element_clear_abort(&raid_group_p->base_config.metadata_element);
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_STARTED);

        /*Split trace to two lines*/
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Active marking CHANGE_CONFIG loc fl: 0x%llx peer fl: 0x%x cap: %llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned int)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long  long)raid_group_p->raid_group_metadata_memory.capacity_expansion_blocks);

        /* Since we are active side, we don't have to wait, we can just go ahead */
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                               FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_STARTED2, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Now we need to wait for the peer to acknowledge that it saw us start.
     * By waiting for the passive side to get to this point, it allows the 
     * active side to clear the bits when it is complete. 
     * The passive side will also be able to know we are done when the bits are cleared.
     */
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set,
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_STARTED);
    if (!b_peer_flag_set){
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Active wait peer CHANGE_CONFIG_STARTED locfl:0x%llx peerfl:0x%llx cap: %llx>\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long  long)raid_group_p->raid_group_metadata_memory.capacity_expansion_blocks);

        return FBE_LIFECYCLE_STATUS_DONE;
    }
    /* If we could not use the quiesce we need, then fail this request. 
     * It is possible a normal quiesce occurred which freezes I/os inside the raid library. 
     * We cannot change config unless there are no I/Os in the raid library. 
     */
    if (!fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)raid_group_p, 
                                               FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD) ||
        (fbe_base_config_is_peer_present((fbe_base_config_t*)raid_group_p) &&
         !fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)raid_group_p, 
                                                     FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD) ) ){
        b_quiesce_not_set = FBE_TRUE;
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Active change config quiesce hold not set locfl:0x%llx peerfl:0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.flags);
    }

    /* When the hook is set, fail the change capacity operation. */
    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                               FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_ERROR_QUIESCE, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) { b_quiesce_not_set = FBE_TRUE; }

    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set,
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_DONE_ERROR);
    if ((b_peer_flag_set && fbe_raid_group_is_peer_present(raid_group_p)) || 
        b_quiesce_not_set){
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Active change config peer error locfl:0x%llx peerfl:0x%llx cap: %llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long  long)raid_group_p->raid_group_metadata_memory.capacity_expansion_blocks);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_ERROR);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_DONE);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_STARTED);
        fbe_raid_group_set_last_capacity(raid_group_p, 0); /* Never started so this should be 0. */
        fbe_raid_group_unlock(raid_group_p);
        fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                               FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_QUIESCE_ERROR, 
                               &hook_status);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    fbe_raid_group_unlock(raid_group_p);    
    /* Now we can launch the config change.
     */
    fbe_raid_group_start_expansion(raid_group_p, packet_p, 
                                            raid_group_p->raid_group_metadata_memory.capacity_expansion_blocks);

    return FBE_LIFECYCLE_STATUS_PENDING;
}
/**************************************************
 * end fbe_raid_group_change_config_active()
 **************************************************/
/*!****************************************************************************
 * fbe_raid_group_change_config_done()
 ******************************************************************************
 * @brief
 *  This function is the last step where the clustered flags
 * and local flags will be cleared on both sides and the cond will be cleared.
 *  If there was an error, then we will back out the capacity change at this point.
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  10/22/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_change_config_done(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t status;
    fbe_bool_t b_is_active;
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t CSX_MAYBE_UNUSED b_peer_present = fbe_raid_group_is_peer_present(raid_group_p);
    fbe_scheduler_hook_status_t        hook_status = FBE_SCHED_STATUS_OK;

    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    fbe_raid_group_lock(raid_group_p);
    /* If we are done with error, set the clustered flag so the peer sees it too.
     */
    if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_ERROR) &&
        !fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_DONE_ERROR)) {
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_DONE_ERROR); 
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    } else {
        /* In order for the passive side to see we are broken, the
         * active side will set the broken clustered flag. 
         */
        fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set,
                                        FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_DONE_ERROR);
        if (fbe_raid_group_is_peer_present(raid_group_p) &&
            b_peer_flag_set)
        {
            /* Peer has said we are done with error.
             * Set the local state so that below we will handle the error.
             */
            fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_ERROR);
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "CCfg %s, peer done error is set local: 0x%llx peer: 0x%llx state:0x%llx\n", 
                                  b_is_active ? "Active" : "Passive",
                                  (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                                  (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                                  (unsigned long long)raid_group_p->local_state);
        }
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                               FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_DONE, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* We want to clear our local flags first.
     */
    if (fbe_raid_group_is_any_clustered_flag_set(raid_group_p, 
                            (FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_REQ|FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_STARTED))) {
        fbe_raid_group_clear_clustered_flag_and_bitmap(raid_group_p, 
                                                       (FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_REQ | 
                                                        FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_STARTED));
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    /* We then wait for the peer to indicate it is done by clearing the started flag.
     */
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set,
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_STARTED);
    if (fbe_raid_group_is_peer_present(raid_group_p) && 
        b_peer_flag_set) {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "CCfg cleanup %s, wait peer done local: 0x%llx peer: 0x%llx state:0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    } else {
        fbe_packet_t *usurper_packet_p = NULL;

        /* Dequeue and send back the usurper.
         */
        fbe_raid_group_get_quiesced_io(raid_group_p, &usurper_packet_p);
       
        if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_ERROR)){
            /* If we decided we are in error then back out this change by restoring the capacity we saved.
             */ 
            fbe_lba_t last_capacity;
            fbe_raid_group_get_last_capacity(raid_group_p, &last_capacity);
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "CCfg: %s, error is set local: 0x%llx peer: 0x%llx state:0x%llx\n", 
                                  b_is_active ? "Active" : "Passive",
                                  (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                                  (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                                  (unsigned long long)raid_group_p->local_state);
            fbe_raid_group_unlock(raid_group_p);
            if (last_capacity != 0) {
                /* Simply re-apply the old capacity which is still preserved in last_capacity.
                 * If we had not yet modified the capacity when we failed, it would be 0. 
                 */
                status = fbe_raid_group_change_capacity(raid_group_p, last_capacity);
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                                           FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                           "CCfg revert of capacity failed with status: 0x%x\n", status);
                }
            }
            fbe_raid_group_lock(raid_group_p);
        }
        /* If this was the originating side, then we have a usurper queued that needs completion.
         */
        if (usurper_packet_p != NULL){
            fbe_base_object_remove_from_usurper_queue((fbe_base_object_t*)raid_group_p, usurper_packet_p);
            if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_ERROR)){
                status = FBE_STATUS_FAILED;
            }
            else {
                status = FBE_STATUS_OK;
            }
            fbe_transport_set_status(usurper_packet_p, status, 0);
            fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                                   FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "CCfg: change of capacity Completed.  Return usurper with status: 0x%x\n",
                                   status);
            fbe_transport_complete_packet(usurper_packet_p);
        }
        /* The local flags tell us that we are waiting for the peer to clear their flags.
         * Since it seems they are complete, it is safe to clear our local flags. 
         */
        fbe_raid_group_clear_local_state(raid_group_p, 
                                         (FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_STARTED | 
                                          FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_DONE | 
                                          FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_ERROR));
        fbe_raid_group_clear_clustered_flag(raid_group_p, 
                                            FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_DONE_ERROR);
        fbe_raid_group_metadata_memory_set_capacity(raid_group_p, 0);
        /* It is possible that we received a request to eval either locally or from the peer.
         * If a request is not set, let's clear the condition. 
         * Otherwise we will run this condition again, and when we see that the started and done flags are not set, 
         * we'll re-evaluate rb logging. 
         */
        if (!fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_REQUEST)) {
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s can't clear current condition, status: 0x%x\n",
                                      __FUNCTION__, status);
            }
        }
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "CCfg %s cleanup complete local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }    
}
/**************************************************
 * end fbe_raid_group_change_config_done()
 **************************************************/

/*!****************************************************************************
 * fbe_raid_group_start_drain()
 ******************************************************************************
 * @brief
 *  Quiesce the raid group if it is needed.
 * 
 * @param raid_group_p - pointer to a raid group object
 *
 * @return  fbe_status_t  
 *
 * @author
 *   10/18/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_start_drain(fbe_raid_group_t *raid_group_p)
{
    fbe_bool_t  peer_alive_b;
    fbe_bool_t  local_quiesced;
    fbe_bool_t  peer_quiesced;

    fbe_raid_group_lock(raid_group_p);
    peer_alive_b = fbe_base_config_has_peer_joined((fbe_base_config_t *) raid_group_p);
    local_quiesced = fbe_base_config_is_clustered_flag_set((fbe_base_config_t *) raid_group_p, 
                                                           FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED);
    peer_quiesced = fbe_base_config_is_any_peer_clustered_flag_set((fbe_base_config_t *) raid_group_p, 
                                                               (FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED|
                                                                FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_COMPLETE));

    /* Quiesce if needed, take peer into account.  If peer is there it also needs to be quiesced.
     */
    if ((local_quiesced == FBE_TRUE) &&
        ((peer_alive_b != FBE_TRUE) || (peer_quiesced == FBE_TRUE))) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_STATUS_OK; 
    }
    fbe_raid_group_unlock(raid_group_p);

    /* Set this bit so the quiesce will first quiesce the bts. 
     * This enables us to quiesce with nothing in flight on the terminator queue and holding 
     * stripe locks. 
     */
    fbe_base_config_set_clustered_flag((fbe_base_config_t *) raid_group_p, 
                                       FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD);

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "Quiesce Hold set q/unuiesce cond: bc_l: 0x%llx bc_p: 0x%llx ls: 0x%llx\n", 
                          raid_group_p->raid_group_metadata_memory.base_config_metadata_memory.flags, 
                          raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.flags,
                          raid_group_p->local_state);

    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                    FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE);

    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                    FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE);

    return FBE_STATUS_PENDING; 
}
/**************************************
 * end fbe_raid_group_start_drain()
 **************************************/

/*************************
 * end file fbe_raid_group_expansion.c
 *************************/
