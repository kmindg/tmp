/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_virtual_drive_class.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the virtual_drive class code .
 * 
 * @ingroup virtual_drive_class_files
 * 
 * @version
 *   12/18/2009:  Created. Dhaval Puhov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_virtual_drive_private.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_spare.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_raid_group_metadata.h"

static fbe_system_spare_config_info_t fbe_system_spare_config_info;
static fbe_spinlock_t system_spare_config_lock;

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t fbe_virtual_drive_calculate_capacity(fbe_packet_t * packet);
static fbe_status_t fbe_virtual_drive_update_spare_configuration(fbe_packet_t * packet);
static fbe_status_t fbe_virtual_drive_set_system_spare_config(fbe_system_spare_config_info_t *system_spare_config_info_p);
static fbe_status_t fbe_virtual_drive_set_permanent_spare_trigger_time(fbe_u64_t permanent_spare_trigger_time);
static fbe_status_t fbe_virtual_drive_get_spare_configuration(fbe_packet_t * packet);
static fbe_status_t fbe_virtual_drive_class_set_unused_as_spare_flag(fbe_packet_t *packet_p);
static fbe_status_t fbe_virtual_drive_class_set_raid_library_debug_flags(fbe_packet_t * packet_p);
static fbe_status_t fbe_virtual_drive_class_set_raid_group_debug_flags(fbe_packet_t * packet_p);
static fbe_status_t fbe_virtual_drive_class_get_debug_flags(fbe_packet_t * packet_p);
static fbe_status_t fbe_virtual_drive_class_set_debug_flags(fbe_packet_t * packet_p);
static fbe_status_t fbe_virtual_drive_class_get_performance_tier(fbe_packet_t *packet_p);

/*!***************************************************************
 * fbe_virtual_drive_class_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the virtual_drive class.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t - Return status of the operation.
 * 
 * @author
 *  12/18/2009 - Created. Dhaval Patel
 ****************************************************************/
fbe_status_t 
fbe_virtual_drive_class_control_entry(fbe_packet_t * packet)
{
    fbe_status_t                            status;
    fbe_payload_ex_t *                     sep_payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_payload_control_operation_opcode_t  opcode;

    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    fbe_payload_control_get_opcode(control_operation, &opcode);

    switch (opcode)
    {
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_CALCULATE_CAPACITY:
            /* Calculate the posotion for the virtual drive metadata and
             * return the exported capacity to the caller.
             */
            status = fbe_virtual_drive_calculate_capacity(packet);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_UPDATE_SPARE_CONFIG:
            /* Configuration service sends spare cofiguration (it sends during
             * initialization or user changes any spare configuration.
             */
            status = fbe_virtual_drive_update_spare_configuration(packet);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_GET_SPARE_CONFIG:
            /* It returns spare current spare configuration for virtual drive class. */
            status = fbe_virtual_drive_get_spare_configuration(packet);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_SET_UNUSED_AS_SPARE_FLAG:
            status = fbe_virtual_drive_class_set_unused_as_spare_flag(packet);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_LIBRARY_DEBUG_FLAGS:
            status = fbe_virtual_drive_class_set_raid_library_debug_flags(packet);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_GROUP_DEBUG_FLAGS:
            status = fbe_virtual_drive_class_set_raid_group_debug_flags(packet);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_GET_DEBUG_FLAGS:
            status = fbe_virtual_drive_class_get_debug_flags(packet);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_SET_DEBUG_FLAGS:
            status = fbe_virtual_drive_class_set_debug_flags(packet);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_GET_PERFORMANCE_TIER:
            status = fbe_virtual_drive_class_get_performance_tier(packet);
            break;
        default:
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            status = fbe_transport_complete_packet(packet);
            break;
    }
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_class_control_entry
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_calculate_capacity()
 ******************************************************************************
 * @brief
 * This function Calculate the metadata posotion for the virtual
 * drive object and returns exported capacity to the caller of
 * this function.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t - Return status of the operation.
 * 
 * @author
 *  12/18/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t 
fbe_virtual_drive_calculate_capacity(fbe_packet_t * packet)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_virtual_drive_control_class_calculate_capacity_t *   calculate_capacity = NULL;    /* INPUT */
    fbe_payload_ex_t *                         sep_payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL; 
    fbe_u32_t                                   length = 0;  
    fbe_block_size_t                            block_size;
    fbe_virtual_drive_metadata_positions_t  virtual_drive_metadata_positions;


    /*! @todo Although we should always return `success' since we have 
     *        successfully receive and processed the packet, we MUST 
     *        return failure if it was an invalid request since the
     *        configuration service doesn't look at the status of the
     *        payload.
     */ 

    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);  

    fbe_payload_control_get_buffer(control_operation, &calculate_capacity);
    if (calculate_capacity == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_virtual_drive_control_class_calculate_capacity_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! The virtual drive object can only accept the 520 as block size and
     *  so we need to add check here to verify that block geometry does not
     *  end up giving 512 as block size.
     */
    fbe_block_transport_get_exported_block_size(calculate_capacity->block_edge_geometry, &block_size);
    if (block_size != FBE_BE_BYTES_PER_BLOCK)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If imported capacity is invalid then return error. */
    if(calculate_capacity->imported_capacity == FBE_LBA_INVALID)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Round the imported capacity up to a chunk before calculating metadata capacity. */
    if (calculate_capacity->imported_capacity % FBE_VIRTUAL_DRIVE_CHUNK_SIZE)
    {
        calculate_capacity->imported_capacity += FBE_VIRTUAL_DRIVE_CHUNK_SIZE - (calculate_capacity->imported_capacity % FBE_VIRTUAL_DRIVE_CHUNK_SIZE);
    }

    /* Get the metadata positions and capacity for the virtual drive object. */
    status = fbe_class_virtual_drive_get_metadata_positions(calculate_capacity->imported_capacity,
                                                            &virtual_drive_metadata_positions);
    if(status != FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Exported the capacity after consuming space for the metadata and 
     * signature for the virtual drive object.
     */
    calculate_capacity->exported_capacity = calculate_capacity->imported_capacity - virtual_drive_metadata_positions.paged_metadata_capacity;

    /* Set the good status. */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/*****************************************************************************
 * end fbe_virtual_drive_calculate_capacity
 *****************************************************************************/

/*!****************************************************************************
 * fbe_class_virtual_drive_get_metadata_positions()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the metadata capacity and positions
 *  based on the imported capacity provided by the caller.
 *
 * @param exported_capacity                 - Exported capacity for the LUN.
 *        virtual_drive_metadata_positions  - Pointer to the virtual drive
 *                                            metadata positions.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/24/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t  
fbe_class_virtual_drive_get_metadata_positions(fbe_lba_t imported_capacity,
                                               fbe_virtual_drive_metadata_positions_t * virtual_drive_metadata_positions_p)
{
    fbe_lba_t   overall_chunks = 0;
    fbe_lba_t   bitmap_entries_per_block = 0;
    fbe_lba_t   paged_bitmap_blocks = 0;
    fbe_lba_t   paged_bitmap_capacity = 0;
    fbe_lba_t   paged_metadata_start = FBE_LBA_INVALID;

    if(imported_capacity == FBE_LBA_INVALID)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_VIRTUAL_DRIVE,
                                 FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s imported capacity is invalid.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We need to round the edge capacity down to full virtual drive chunk size. */
    if (imported_capacity % FBE_VIRTUAL_DRIVE_CHUNK_SIZE)
    {
        /* Caller needs to provide rounded capacity always. */
        fbe_topology_class_trace(FBE_CLASS_ID_VIRTUAL_DRIVE,
                                 FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s imported capacity 0x%llx needs be rounded before calculation metadata position and size.\n", 
                                 __FUNCTION__,
				 (unsigned long long)imported_capacity);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Compute overall number of chunks */
    overall_chunks = imported_capacity / FBE_VIRTUAL_DRIVE_CHUNK_SIZE;

    /* Compute how many chunk entries can be saved in (1) logical block.
     * Currently we assume that a block can hold a whole multiple of entries.
     * Round up to the optimal block size.
     */
    bitmap_entries_per_block = FBE_METADATA_BLOCK_DATA_SIZE / FBE_RAID_GROUP_CHUNK_ENTRY_SIZE;
    paged_bitmap_blocks = (overall_chunks + (bitmap_entries_per_block - 1)) / bitmap_entries_per_block;

    /* Round the number of metadata blocks to virtual drive chunk size. */
    if(paged_bitmap_blocks % FBE_VIRTUAL_DRIVE_CHUNK_SIZE)
    {
        paged_bitmap_blocks += FBE_VIRTUAL_DRIVE_CHUNK_SIZE - (paged_bitmap_blocks % FBE_VIRTUAL_DRIVE_CHUNK_SIZE);
    }

    /* Now multiply by the number of metadata copies for the total paged bitmat capacity. */
    paged_bitmap_capacity = paged_bitmap_blocks * FBE_VIRTUAL_DRIVE_NUMBER_OF_METADATA_COPIES;


    /* Paged metadata starting position. */
    paged_metadata_start = imported_capacity - paged_bitmap_capacity ;

    virtual_drive_metadata_positions_p->paged_metadata_capacity = paged_bitmap_capacity;
    virtual_drive_metadata_positions_p->paged_metadata_lba = paged_metadata_start;
    virtual_drive_metadata_positions_p->paged_metadata_block_count = (fbe_block_count_t) paged_bitmap_blocks;
    virtual_drive_metadata_positions_p->paged_mirror_metadata_offset = paged_metadata_start + paged_bitmap_blocks;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_class_virtual_drive_get_metadata_positions()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_get_spare_configuration()
 ******************************************************************************
 * @brief
 * This function is used to get the spare configuration, configuration.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t - Return status of the operation.
 * 
 * @author
 *  12/18/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t 
fbe_virtual_drive_get_spare_configuration(fbe_packet_t * packet)
{
    fbe_status_t                                            status = FBE_STATUS_OK;
    fbe_virtual_drive_control_class_get_spare_config_t *    spare_config = NULL;    /* INPUT */
    fbe_payload_ex_t *                                     sep_payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL; 
    fbe_u32_t                                               length = 0; 

    /*! @todo Although we should always return `success' since we have 
     *        successfully receive and processed the packet, we MUST 
     *        return failure if it was an invalid request since the
     *        configuration service doesn't look at the status of the
     *        payload.
     */ 
    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);  

    fbe_payload_control_get_buffer(control_operation, &spare_config);
    if(spare_config == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_virtual_drive_control_class_get_spare_config_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the spare configuration from virtual drive object. */
    fbe_spinlock_lock(&system_spare_config_lock);
    fbe_copy_memory(&spare_config->system_spare_config_info, &fbe_system_spare_config_info, sizeof(fbe_system_spare_config_info_t));
    fbe_spinlock_unlock(&system_spare_config_lock);

    /* Set the good status. */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_get_spare_configuration()
 ******************************************************************************/

/*!**************************************************************
 *          fbe_virtual_drive_class_set_raid_library_debug_flags()
 ****************************************************************
 *
 * @brief   Set the default raid library default for all NEW raid
 *          groups to the value specified.  This method does NOT
 *          change the raid library debug flags for existing raid
 *          groups.
 *
 * @param   packet_p - control packet.               
 *
 * @return  fbe_status_t   
 *
 * @author
 *  03/15/2010  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t fbe_virtual_drive_class_set_raid_library_debug_flags(fbe_packet_t * packet_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_raid_group_raid_library_debug_payload_t *set_debug_flags_p = NULL;  /* INPUT */
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_buffer_length_t length;
    fbe_payload_control_operation_t *control_operation_p = NULL; 

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &set_debug_flags_p);
   
    /* Validate that a buffer was supplied
     */
    if (set_debug_flags_p == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_VIRTUAL_DRIVE, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "virtual_drive: %s line: %d no buffer supplied \n",
                                 __FUNCTION__, __LINE__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate length
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if (length != sizeof(*set_debug_flags_p))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_VIRTUAL_DRIVE, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "virtual_drive: %s line: %d expected buffer length: 0x%llx actual: 0x%x\n",
                                 __FUNCTION__, __LINE__,
				 (unsigned long long)sizeof(*set_debug_flags_p),
				 length);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now invoke raid geometry method that changes the default raid library
     * debug flags for all raid groups created after this point.
     */
    status = fbe_virtual_drive_set_default_raid_library_debug_flags(set_debug_flags_p->raid_library_debug_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_VIRTUAL_DRIVE, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "virtual_drive: %s line: %d set default raid library debug flags status: 0x%x \n",
                                 __FUNCTION__, __LINE__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Success.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/**********************************************************
 * end fbe_virtual_drive_class_set_raid_library_debug_flags()
 **********************************************************/

/*!**************************************************************
 *          fbe_virtual_drive_class_set_raid_group_debug_flags()
 ****************************************************************
 *
 * @brief   Set the default raid group default for all NEW raid
 *          groups to the value specified.  This method does NOT
 *          change the raid group debug flags for existing raid
 *          groups.
 *
 * @param   packet_p - control packet.               
 *
 * @return  fbe_status_t   
 *
 * @author
 *  03/15/2010  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t fbe_virtual_drive_class_set_raid_group_debug_flags(fbe_packet_t * packet_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_raid_group_raid_group_debug_payload_t *set_debug_flags_p = NULL;  /* INPUT */
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_buffer_length_t length;
    fbe_payload_control_operation_t *control_operation_p = NULL; 

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &set_debug_flags_p);
   
    /* Validate that a buffer was supplied
     */
    if (set_debug_flags_p == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_VIRTUAL_DRIVE, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "virtual_drive: %s line: %d no buffer supplied \n",
                                 __FUNCTION__, __LINE__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate length
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if (length != sizeof(*set_debug_flags_p))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_VIRTUAL_DRIVE, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "virtual_drive: %s line: %d expected buffer length: 0x%llx actual: 0x%x\n",
                                 __FUNCTION__, __LINE__,
				 (unsigned long long)sizeof(*set_debug_flags_p),
				 length);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now invoke raid group method that changes the default raid group
     * debug flags for all raid groups created after this point.
     */
    status = fbe_virtual_drive_set_default_raid_group_debug_flags(set_debug_flags_p->raid_group_debug_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_VIRTUAL_DRIVE, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "virtual_drive: %s line: %d set default raid group debug flags status: 0x%x \n",
                                 __FUNCTION__, __LINE__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Success.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/******************************************************************************
 * end fbe_virtual_drive_class_set_raid_group_debug_flags()
 ******************************************************************************/

/*!**************************************************************
 *          fbe_virtual_drive_class_get_debug_flags()
 ****************************************************************
 *
 * @brief   Retrieve the default raid group debug flags value 
 *          that is used for all newly created raid groups.
 *
 * @param   packet_p - control packet.               
 *
 * @return  fbe_status_t   
 *
 * @author
 *  03/17/2010  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t fbe_virtual_drive_class_get_debug_flags(fbe_packet_t * packet_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_virtual_drive_debug_payload_t *get_debug_flags_p = NULL;  /* INPUT */
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_buffer_length_t length;
    fbe_payload_control_operation_t *control_operation_p = NULL; 

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &get_debug_flags_p);
   
    /* Validate that a buffer was supplied
     */
    if (get_debug_flags_p == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_VIRTUAL_DRIVE, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "virtual_drive: %s line: %d no buffer supplied\n",
                                 __FUNCTION__, __LINE__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate length
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if (length != sizeof(*get_debug_flags_p))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_VIRTUAL_DRIVE, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "virtual_drive: %s line: %d expected buffer length: 0x%llx actual: 0x%x\n",
                                 __FUNCTION__, __LINE__,
				 (unsigned long long)sizeof(*get_debug_flags_p),
				 length);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now invoke virtual drive method that gets the default raid group
     * debug flags for all raid groups created after this point.
     */
    fbe_virtual_drive_get_default_debug_flags(&get_debug_flags_p->virtual_drive_debug_flags);
    
    /* Success.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/**********************************************************
 * end fbe_virtual_drive_class_get_debug_flags()
 **********************************************************/

/*!**************************************************************
 *          fbe_virtual_drive_class_set_debug_flags()
 ****************************************************************
 *
 * @brief   Set the default virtual drive default for all NEW raid
 *          groups to the value specified.  This method does NOT
 *          change the virtual drive debug flags for existing virtual
 *          drives.
 *
 * @param   packet_p - control packet.               
 *
 * @return  fbe_status_t   
 *
 * @author
 *  03/15/2010  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t fbe_virtual_drive_class_set_debug_flags(fbe_packet_t * packet_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_virtual_drive_debug_payload_t *set_debug_flags_p = NULL;  /* INPUT */
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_buffer_length_t length;
    fbe_payload_control_operation_t *control_operation_p = NULL; 

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &set_debug_flags_p);
   
    /* Validate that a buffer was supplied
     */
    if (set_debug_flags_p == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_VIRTUAL_DRIVE, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "virtual_drive: %s line: %d no buffer supplied \n",
                                 __FUNCTION__, __LINE__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate length
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if (length != sizeof(*set_debug_flags_p))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_VIRTUAL_DRIVE, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "virtual_drive: %s line: %d expected buffer length: 0x%llx actual: 0x%x\n",
                                 __FUNCTION__, __LINE__,
				 (unsigned long long)sizeof(*set_debug_flags_p),
				 length);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now invoke raid group method that changes the default raid group
     * debug flags for all raid groups created after this point.
     */
    status = fbe_virtual_drive_set_default_debug_flags(set_debug_flags_p->virtual_drive_debug_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_VIRTUAL_DRIVE, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "virtual_drive: %s line: %d set default raid group debug flags status: 0x%x \n",
                                 __FUNCTION__, __LINE__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Success.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/******************************************************************************
 * end fbe_virtual_drive_class_set_virtual_drive_debug_flags()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_update_spare_configuration()
 ******************************************************************************
 * @brief
 * This function is used to update the spare configuration, configuration 
 * service sends this update during initialization or during any configuration
 * update.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t - Return status of the operation.
 * 
 * @author
 *  12/18/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t 
fbe_virtual_drive_update_spare_configuration(fbe_packet_t * packet)
{
    fbe_status_t                                            status = FBE_STATUS_OK;
    fbe_virtual_drive_control_class_update_spare_config_t * update_spare_config = NULL;    /* INPUT */
    fbe_payload_ex_t *                                     sep_payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL; 
    fbe_u32_t                                               length = 0; 

    /*! @todo Although we should always return `success' since we have 
     *        successfully receive and processed the packet, we MUST 
     *        return failure if it was an invalid request since the
     *        configuration service doesn't look at the status of the
     *        payload.
     */ 
    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);  

    fbe_payload_control_get_buffer(control_operation, &update_spare_config);
    if(update_spare_config == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_virtual_drive_control_class_update_spare_config_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate the value requested (0 is not allowed).
     */
    if (update_spare_config->system_spare_config_info.permanent_spare_trigger_time == 0)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_VIRTUAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s invalid spare trigger timer: 0x%llx\n", 
                                 __FUNCTION__, (unsigned long long)update_spare_config->system_spare_config_info.permanent_spare_trigger_time);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* update the spare configuration. */
    fbe_virtual_drive_set_system_spare_config(&update_spare_config->system_spare_config_info);

    /* Set the good status. */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_update_spare_configuration()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_set_system_spare_config()
 ******************************************************************************
 * @brief
 * This function is used to set (update) the system spare configuration.
 *
 * @author
 *  09/10/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t 
fbe_virtual_drive_set_system_spare_config(fbe_system_spare_config_info_t *system_spare_config_info_p)
{
    if (system_spare_config_info_p == NULL)
    {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO, "vd_set_sys_spare_cfg NULL pointer\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* make sure we trace important information which was changed */
    if (fbe_system_spare_config_info.permanent_spare_trigger_time != 
        system_spare_config_info_p->permanent_spare_trigger_time)
    {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "vd_set_sys_spare_cfg: sys sparing timer changed, old:%llu,new:%llu\n",
                                    (unsigned long long)fbe_system_spare_config_info.permanent_spare_trigger_time,
                                    (unsigned long long)system_spare_config_info_p->permanent_spare_trigger_time);
    }

    fbe_spinlock_lock(&system_spare_config_lock);
    fbe_copy_memory(&fbe_system_spare_config_info, system_spare_config_info_p, sizeof(fbe_system_spare_config_info_t));
    fbe_spinlock_unlock(&system_spare_config_lock);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_set_system_spare_config()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_initialize_default_system_spare_config()
 ******************************************************************************
 * @brief
 * This function is used to initialize the default system spare config.
 *
 * @author
 *  09/10/2010 - Created. Dhaval Patel
 ******************************************************************************/
void
fbe_virtual_drive_initialize_default_system_spare_config(void)
{
    fbe_spinlock_init(&system_spare_config_lock);
    fbe_system_spare_config_info.permanent_spare_trigger_time = FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME;
    return;
}
/******************************************************************************
 * end fbe_virtual_drive_initialize_default_spare_config()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_teardown_default_system_spare_config()
 ******************************************************************************
 * @brief
 * This function is used to release global data used by the default system spare config.
 *
 * @author
 *  1/5/2012 - Created. Chris Gould
 ******************************************************************************/
void
fbe_virtual_drive_teardown_default_system_spare_config(void)
{
    fbe_spinlock_destroy(&system_spare_config_lock); /* SAFEBUG - needs destroy */
    return;
}

/******************************************************************************
 * end fbe_virtual_drive_initialize_default_spare_config()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_virtual_drive_get_permanent_spare_trigger_time()
 ******************************************************************************
 * @brief
 * This function is used to get the permanent spare trigger time in secs.
 *
 * @param permanent_spare_trigger_time_p    - swap-in trigger time.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/10/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_get_permanent_spare_trigger_time(fbe_u64_t * permanent_spare_trigger_time_p)
{
   *permanent_spare_trigger_time_p = fbe_system_spare_config_info.permanent_spare_trigger_time;
   return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_get_permanent_spare_trigger_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_set_permanent_spare_trigger_time()
 ******************************************************************************
 * @brief
 * This function is used to set the permanenet spare trigger time in secs.
 *
 * @param permanent_spare_trigger_time  - swap-in trigger time.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/10/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_virtual_drive_set_permanent_spare_trigger_time(fbe_u64_t permanent_spare_trigger_time)
{
   fbe_system_spare_config_info.permanent_spare_trigger_time = permanent_spare_trigger_time;
   return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_set_permanent_spare_trigger_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_class_set_resource_priority()
 ******************************************************************************
 * @brief
 * This function sets memory allocation priority for the packet.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/20/2011 - Created. Vamsi V
 *
 ******************************************************************************/
fbe_status_t  
fbe_virtual_drive_class_set_resource_priority(fbe_packet_t * packet_p)
{
    if((packet_p->resource_priority_boost & FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_VD) == 0)
    {
        /* If flag is not set, set it to true */
        packet_p->resource_priority_boost |= FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_VD;
    }
    else
    {
        /* Packet is being resent/reused without resetting the resource priroity. */ 
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                       "vd: resource priority is not reset for packet 0x%p \n",
                                       packet_p);
    }

    if(packet_p->resource_priority >= FBE_MEMORY_DPS_VD_BASE_PRIORITY)
    {
        /* Generally in IO path packets flow down the stack from LUN to RG to VD to PVD.
         * In this case, resource priority will be less than Objects base priority and so
         * we bump it up. 
         * But in some cases (such as monitor, control, event operations, etc.) packets do not 
         * always move down the stack. In these cases packets's resource priority will already be greater
         * than Object's base priority. But since we should never reduce priority, we dont
         * update it.    
         */
    }
    else
    {
        packet_p->resource_priority = FBE_MEMORY_DPS_VD_BASE_PRIORITY;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_class_set_resource_priority()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_class_set_unused_as_spare_flag()
 ******************************************************************************
 * @brief
 *  This function sets the control unused drive as spare flag to false
 *  for VD class.
 * 
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/11/2012 - Created. Vera Wang
 *
 ******************************************************************************/
static fbe_status_t
fbe_virtual_drive_class_set_unused_as_spare_flag(fbe_packet_t *packet)
{
	fbe_virtual_drive_control_class_set_unused_drive_as_spare_t *set_flag;
    fbe_payload_ex_t *                                     sep_payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL; 
    fbe_u32_t                                               length = 0; 

	sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);  

    fbe_payload_control_get_buffer(control_operation, &set_flag);
    if(set_flag == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_virtual_drive_control_class_set_unused_drive_as_spare_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the unused_as_spare_flag to false for VD class */
    fbe_virtual_drive_set_unused_as_spare_flag(set_flag->enable);

    fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Virtual drive class set unused drive as spare flag to %s.\n", set_flag->enable ? "Enable" : "Disable" );

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}// End fbe_virtual_drive_class_set_unused_as_spare_flag

/*!****************************************************************************
 *          fbe_virtual_drive_class_get_performance_tier()
 ****************************************************************************** 
 * 
 * @brief   Get the performance tier information using the passed parameters.
 *
 * @param   packet - The packet that is arriving.
 *
 * @return  fbe_status_t - Return status of the operation.
 * 
 * @author
 *  07/07/2015  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_class_get_performance_tier(fbe_packet_t *packet_p)
{
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_virtual_drive_control_get_performance_tier_t   *get_performance_tier_p = NULL;    /* INPUT */
    fbe_payload_ex_t                                   *sep_payload = NULL;
    fbe_payload_control_operation_t                    *control_operation = NULL; 
    fbe_u32_t                                           length = 0;
    fbe_u32_t                                           drive_type_index; 

    sep_payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);  

    fbe_payload_control_get_buffer(control_operation, &get_performance_tier_p);
    if(get_performance_tier_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length (control_operation, &length);
    if (length != sizeof(fbe_virtual_drive_control_get_performance_tier_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Populate the mapping
     */
    get_performance_tier_p->drive_type_last = FBE_DRIVE_TYPE_LAST;
    for (drive_type_index = 0; drive_type_index < FBE_DRIVE_TYPE_LAST; drive_type_index++)
    {
        /* There are unsupported drive types.
         */
        switch (drive_type_index)
        {
            case FBE_DRIVE_TYPE_INVALID:
            case FBE_DRIVE_TYPE_FIBRE:
            case FBE_DRIVE_TYPE_SATA:
            case FBE_DRIVE_TYPE_SAS_SED:
            case FBE_DRIVE_TYPE_LAST:
                /* Unsuppoted drive types. */
                get_performance_tier_p->performance_tier[drive_type_index] = FBE_PERFORMANCE_TIER_INVALID;
                break;
                
            default:
                /* All other drive types are supported. Use the spare library
                 * to get the lowest performance tier (i.e. the performance tier group).
                 */
                status = fbe_spare_lib_selection_get_drive_performance_tier_group(drive_type_index,
                                                                       &get_performance_tier_p->performance_tier[drive_type_index]);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_topology_class_trace(FBE_CLASS_ID_VIRTUAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s for drive type index: %d failed\n", 
                                 __FUNCTION__, drive_type_index);
                    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                    fbe_transport_complete_packet(packet_p);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                break;
        }
    }

    /* Set the good status. */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_class_get_performance_tier()
 ******************************************************************************/


/*******************************
 * end fbe_virtual_drive_class.c
 *******************************/

