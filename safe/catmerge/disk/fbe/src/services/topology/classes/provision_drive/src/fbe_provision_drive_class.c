/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_class.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the provision_drive class code .
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   11/19/2009:  Created. Peter Puhov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_provision_drive_private.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_spare.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe_database.h"
#include "fbe/fbe_registry.h"


/*****************************************
 * LOCAL GLOBALS
 *****************************************/
static fbe_block_count_t    fbe_provision_drive_blocks_per_drive_request = 0;
static fbe_u64_t fbe_provision_drive_default_wear_leveling_timer_seconds = FBE_PROVISION_DRIVE_DEFAULT_WEAR_LEVEL_TIMER_SEC;
static fbe_u64_t fbe_provision_drive_warranty_period_hours = FBE_PROVISION_DRIVE_WEAR_LEVEL_5YR_WARRANTY_IN_HOURS;

/*************************
 *   GLOBAL DEFINITIONS
 *************************/
/*! @var    fbe_provision_drive_class_debug_flag
 *   @brief   This is the global variable to save provision drive debug trace falg value.
 */
fbe_u32_t fbe_provision_drive_class_debug_flag = FBE_PROVISION_DRIVE_DEBUG_FLAG_NONE;

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t fbe_provision_drive_calculate_capacity(fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_class_set_debug_flag(fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_usurper_set_slf_enabled(fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_usurper_is_slf_enabled(fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_class_set_bg_op_speed(fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_class_get_bg_op_speed(fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_get_dword_from_registry(fbe_u32_t *flag_p, 
                                                                fbe_u8_t* key_p,
                                                                fbe_u32_t default_input_value);
static fbe_status_t fbe_provision_drive_usurper_set_wear_leveling_timer(fbe_packet_t * packet_p);

/*!***************************************************************
 * fbe_provision_drive_class_is_max_drive_blocks_configured()
 ****************************************************************
 * @brief
 *  Simply determines if the maximum number of blocks that raid
 *  can send to a drive has been configured or not
 *
 * @param None
 *
 * @return bool - FBE_TRUE - We have configured the maximum number
 *                blocks we can send to a drive
 ****************************************************************/
static fbe_bool_t fbe_provision_drive_class_is_max_drive_blocks_configured(void)
{
    if (fbe_provision_drive_blocks_per_drive_request != 0)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

/*!***************************************************************
 * fbe_provision_drive_class_get_max_drive_blocks()
 ****************************************************************
 * @brief
 *  Simply return the maximum number of blocks that raid
 *  can send to a drive.
 *
 * @param None
 *
 * @return block count - The maximum number of block that raid can
 *          send to a drive in a single request.
 ****************************************************************/
static fbe_block_count_t fbe_provision_drive_class_get_max_drive_blocks(void)
{
    return(fbe_provision_drive_blocks_per_drive_request);
}

/*!***************************************************************
 * fbe_provision_drive_class_set_max_drive_blocks()
 ****************************************************************
 * @brief
 *  Simply set the maximum number of blocks that raid can send to a drive.
 *
 * @param max_blocks_per_drive - The configured maximum number of
 *          blocks that raid should send to a drive. 
 *
 * @return none
 ****************************************************************/
static void fbe_provision_drive_class_set_max_drive_blocks(fbe_block_count_t max_blocks_per_drive)
{
    fbe_provision_drive_blocks_per_drive_request = max_blocks_per_drive;
    return;
}

/*!***************************************************************
 * fbe_provision_drive_class_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the provision_drive class.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t 
fbe_provision_drive_class_control_entry(fbe_packet_t * packet)
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
        case FBE_PROVISION_DRIVE_CONTROL_CODE_CLASS_CALCULATE_CAPACITY:
            status = fbe_provision_drive_calculate_capacity(packet);
            break;
            
        case FBE_PROVISION_DRIVE_CONTROL_CODE_SET_CLASS_DEBUG_FLAGS:
            status = fbe_provision_drive_class_set_debug_flag( packet);
            break;
            
        case FBE_PROVISION_DRIVE_CONTROL_CODE_SET_SLF_ENABLED:
            status = fbe_provision_drive_usurper_set_slf_enabled(packet);
            break;
            
        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SLF_ENABLED:
            status = fbe_provision_drive_usurper_is_slf_enabled(packet);
            break;
            
        case FBE_PROVISION_DRIVE_CONTROL_CODE_SET_BG_OP_SPEED:
            status = fbe_provision_drive_class_set_bg_op_speed(packet);
            break;   
            
        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_BG_OP_SPEED:
            status = fbe_provision_drive_class_get_bg_op_speed(packet);
            break;   

        case FBE_PROVISION_DRIVE_CONTROL_CODE_SET_CLASS_WEAR_LEVELING_TIMER:
            status = fbe_provision_drive_usurper_set_wear_leveling_timer(packet); 
            break;

        default:
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            status = fbe_transport_complete_packet(packet);
            break;
    }
    return status;
}
/* end fbe_provision_drive_control_entry() */


/*!****************************************************************************
 * fbe_provision_drive_calculate_capacity()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the capacity.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t
 * 
 * @author
 * 11/24/2009 - Created. Peter Puhov
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_calculate_capacity(fbe_packet_t * packet)
{
    fbe_provision_drive_control_class_calculate_capacity_t *    calculate_capacity = NULL;    /* INPUT */
    fbe_status_t                                                status;
    fbe_payload_ex_t *                                         sep_payload = NULL;
    fbe_payload_control_operation_t *                           control_operation = NULL; 
    fbe_u32_t                                                   length = 0;  
    fbe_optimum_block_size_t                                    optimum_block_size;
    fbe_block_size_t                                            block_size;
    fbe_provision_drive_metadata_positions_t                    provision_drive_metadata_positions;
    fbe_lba_t                                                   imported_capacity = FBE_LBA_INVALID;


    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);  

    fbe_payload_control_get_buffer(control_operation, &calculate_capacity);
    if (calculate_capacity == NULL) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_provision_drive_control_class_calculate_capacity_t)) {
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

    /* not used by anything??*/
    fbe_block_transport_get_optimum_block_size(calculate_capacity->block_edge_geometry, &optimum_block_size);
    fbe_block_transport_get_exported_block_size(calculate_capacity->block_edge_geometry, &block_size);

    imported_capacity = calculate_capacity->imported_capacity;

    /* WARNING!!!!
       Some of the following code is duplicated in fbe_private_space_get_minimum_system_drive_size()
       Please update that function if you change the code here.
     */

    /* Round the imported capacity to down side, it is needed to make sure that
     * exported capacity and metadata is always aligned with chunk size. 
     */ 
    if(imported_capacity % FBE_PROVISION_DRIVE_CHUNK_SIZE)
    {
        imported_capacity = imported_capacity - (imported_capacity % FBE_PROVISION_DRIVE_CHUNK_SIZE);
    }

    status = fbe_class_provision_drive_get_metadata_positions(imported_capacity,
                                                              &provision_drive_metadata_positions);
    if(status != FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Exported the capacity after consuming space for the metadata 
     * for the provision drive object.
     */
    calculate_capacity->exported_capacity = imported_capacity - provision_drive_metadata_positions.paged_metadata_capacity;

    /* Complete the packet with good status.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_calculate_capacity()
 ******************************************************************************/
 
/*!****************************************************************************
 * fbe_class_provision_drive_get_metadata_positions()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the metadata capacity and positions
 *  based on the imported capacity provided by the caller.
 *
 * @param exported_capacity                    - Exported capacity for the LUN.
 *        provision_drive_metadata_positions   - Pointer to the provision drive
 *                                               metadata positions.
 *
 * @return fbe_status_t
 * @note WARNING!!!!
 *     Some of the following code is duplicated in fbe_private_space_get_minimum_system_drive_size()
 *     Please update that function if you change the code here.
 * @author
 *  3/24/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t  
fbe_class_provision_drive_get_metadata_positions(fbe_lba_t imported_capacity,
                                                 fbe_provision_drive_metadata_positions_t * provision_drive_metadata_positions_p)
{
    fbe_lba_t   overall_chunks = 0;
    fbe_lba_t   bitmap_entries_per_block = 0;
    fbe_lba_t   paged_bitmap_blocks = 0;
    fbe_lba_t   paged_bitmap_capacity = 0;
    fbe_lba_t   paged_metadata_start = FBE_LBA_INVALID;

    if(imported_capacity == FBE_LBA_INVALID)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_PROVISION_DRIVE,
                                 FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s imported capacity is invalid.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Sanity check: imported capacity should be multiple of full provision drive chunk size. */
    if (imported_capacity % FBE_PROVISION_DRIVE_CHUNK_SIZE)
    {
        /* Caller needs to provide rounded capacity always. */
        fbe_topology_class_trace(FBE_CLASS_ID_PROVISION_DRIVE,
                                 FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: imported capacity 0x%llx needs be rounded to multiple of 0x%x\n", 
                                 __FUNCTION__,
				 (unsigned long long)imported_capacity,
				 FBE_PROVISION_DRIVE_CHUNK_SIZE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* WARNING!!!!
       Some of the following code is duplicated in fbe_private_space_get_minimum_system_drive_size()
       Please update that function if you change the code here.
     */

    /* Compute overall number of chunks */
    overall_chunks = imported_capacity / FBE_PROVISION_DRIVE_CHUNK_SIZE;

    /* Compute how many chunk entries can be saved in (1) logical block.
     * Currently we assume that a block can hold a whole multiple of entries. 
     * This assumption is checked at compile time by fbe_provision_drive_load()                                                                      .
     */
    bitmap_entries_per_block = FBE_METADATA_BLOCK_DATA_SIZE /  sizeof(fbe_provision_drive_paged_metadata_t);
    /* Round up to the full block */
    paged_bitmap_blocks = (overall_chunks + (bitmap_entries_per_block - 1)) / bitmap_entries_per_block;

    /* Round the number of metadata blocks to provision drive chunk size. */
    if(paged_bitmap_blocks % FBE_PROVISION_DRIVE_CHUNK_SIZE)
    {
        paged_bitmap_blocks += FBE_PROVISION_DRIVE_CHUNK_SIZE - (paged_bitmap_blocks % FBE_PROVISION_DRIVE_CHUNK_SIZE);
    }

    /* Now multiply by the number of metadata copies for the total paged bitmat capacity. */
    paged_bitmap_capacity = paged_bitmap_blocks * FBE_PROVISION_DRIVE_NUMBER_OF_METADATA_COPIES;

    /* Paged metadata starting position. */
    /* paged_metadata_start needs to be aligned to the chunk size*/
    paged_metadata_start = (overall_chunks * FBE_PROVISION_DRIVE_CHUNK_SIZE) - paged_bitmap_capacity;

    /* Paged metadata start and paged metadata block count */
    provision_drive_metadata_positions_p->paged_metadata_lba = paged_metadata_start;
    provision_drive_metadata_positions_p->paged_metadata_block_count = (fbe_block_count_t )paged_bitmap_blocks;

    /* Paged bitmap mirror offset. */
    provision_drive_metadata_positions_p->paged_mirror_metadata_offset  = paged_bitmap_blocks; 

    /* Set the paged metadata capcity. */
    provision_drive_metadata_positions_p->paged_metadata_capacity = paged_bitmap_capacity;
        
    provision_drive_metadata_positions_p->number_of_private_stripes = paged_bitmap_blocks;// / FBE_PROVISION_DRIVE_CHUNK_SIZE;
    provision_drive_metadata_positions_p->number_of_stripes = paged_metadata_start + paged_bitmap_blocks;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_class_provision_drive_get_metadata_positions()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_class_provision_drive_get_metadata_positions_from_exported_capacity()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the metadata capacity and positions
 *  based on the exported capacity provided by the caller.
 *
 * @param exported_capacity                    - Exported capacity for the PVD.
 *        provision_drive_metadata_positions   - Pointer to the provision drive
 *                                               metadata positions.
 *
 * @return fbe_status_t
 * @author
 *  08/08/2012 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t  
fbe_class_provision_drive_get_metadata_positions_from_exported_capacity(fbe_lba_t exported_capacity,
                                                 fbe_provision_drive_metadata_positions_t * provision_drive_metadata_positions_p)
{
    fbe_lba_t   bitmap_entries_per_block = 0;
    fbe_lba_t   paged_bitmap_blocks = 0;
    fbe_lba_t   paged_bitmap_capacity = 0;
    fbe_lba_t   paged_metadata_start = FBE_LBA_INVALID;
    fbe_lba_t   paged_chunks = 0, exported_chunks;
    fbe_bool_t  b_found = FBE_FALSE;

    if(exported_capacity == FBE_LBA_INVALID)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_PROVISION_DRIVE,
                                 FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s exported capacity is invalid.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Sanity check: iexported capacity should be multiple of full provision drive chunk size. */
    if (exported_capacity % FBE_PROVISION_DRIVE_CHUNK_SIZE)
    {
        /* Caller needs to provide rounded capacity always. */
        fbe_topology_class_trace(FBE_CLASS_ID_PROVISION_DRIVE,
                                 FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: exported capacity 0x%x needs be rounded to multiple of 0x%x\n", 
                                 __FUNCTION__, (unsigned int)exported_capacity, FBE_PROVISION_DRIVE_CHUNK_SIZE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    exported_chunks = exported_capacity / FBE_PROVISION_DRIVE_CHUNK_SIZE;

    /* Compute how many chunk entries can be saved in (1) logical block.
     */
    bitmap_entries_per_block = FBE_METADATA_BLOCK_DATA_SIZE /  sizeof(fbe_provision_drive_paged_metadata_t);

    /* Calculate paged_bitmap_blocks */
    for (paged_chunks = 1; paged_chunks < exported_chunks; paged_chunks++)
    {
        paged_bitmap_blocks = (exported_chunks + paged_chunks + bitmap_entries_per_block - 1) / bitmap_entries_per_block;
        if (paged_chunks == ((paged_bitmap_blocks + FBE_PROVISION_DRIVE_CHUNK_SIZE - 1) / FBE_PROVISION_DRIVE_CHUNK_SIZE))
        {
            b_found = FBE_TRUE;
            break;
        }
    }

    if (!b_found)
    {
        /* Caller needs to provide rounded capacity always. */
        fbe_topology_class_trace(FBE_CLASS_ID_PROVISION_DRIVE,
                                 FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: failed\n", 
                                 __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* Round the number of metadata blocks to provision drive chunk size. */
    paged_bitmap_blocks = paged_chunks * FBE_PROVISION_DRIVE_CHUNK_SIZE;

    /* Now multiply by the number of metadata copies for the total paged bitmat capacity. */
    paged_bitmap_capacity = paged_bitmap_blocks * FBE_PROVISION_DRIVE_NUMBER_OF_METADATA_COPIES;

    /* Paged metadata starting position. */
    /* paged_metadata_start needs to be aligned to the chunk size*/
    paged_metadata_start = exported_capacity;

    /* Paged metadata start and paged metadata block count */
    provision_drive_metadata_positions_p->paged_metadata_lba = paged_metadata_start;
    provision_drive_metadata_positions_p->paged_metadata_block_count = (fbe_block_count_t )paged_bitmap_blocks;

    /* Paged bitmap mirror offset. */
    provision_drive_metadata_positions_p->paged_mirror_metadata_offset  = paged_bitmap_blocks; 

    /* Set the paged metadata capcity. */
    provision_drive_metadata_positions_p->paged_metadata_capacity = paged_bitmap_capacity;
        
    provision_drive_metadata_positions_p->number_of_private_stripes = paged_bitmap_blocks;
    provision_drive_metadata_positions_p->number_of_stripes = paged_metadata_start + paged_bitmap_blocks;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_class_provision_drive_get_metadata_positions_from_exported_capacity()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_class_send_control_request()
 ******************************************************************************
 * @brief
 *  This function is used to send a control request to the service specified.
 *  It was copied from send_config.
 *
 * @param provision_drive_p - Pointer to raid group that is requesting control request                 
 * @param control_buffer_p - Pointer to control buffer to populate                         
 * @param control_buffer_size - The number of bytes the control buffer can hold
 * @param opcode - The control operaiton code
 * @param package_id - The package id to send the request to
 * @param service_id - The serive id to send request to                    
 * @param class_id - The class id to send request to
 * @param object_id - The object id to send request to
 * 
 * @return - status - The status of the control request
 * 
 ******************************************************************************/
static
fbe_status_t fbe_provision_drive_class_send_control_request(fbe_provision_drive_t *provision_drive_p,
                                                            void *control_buffer_p,
                                                            fbe_u32_t control_buffer_size,
                                                            fbe_payload_control_operation_opcode_t opcode,
                                                            fbe_package_id_t package_id,
                                                            fbe_service_id_t service_id,
                                                            fbe_class_id_t class_id,
                                                            fbe_object_id_t object_id)
{
    fbe_packet_t * packet;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);
    sep_payload_p = fbe_transport_get_payload_ex(packet);

    control_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    fbe_payload_control_build_operation(control_p, opcode, control_buffer_p, control_buffer_size);

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Set packet address */
    fbe_transport_set_address(packet,
                              package_id,
                              service_id,
                              class_id,
                              object_id);

    /* Set traversal flag if set */
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);

    status = fbe_service_manager_send_control_packet(packet);

    fbe_transport_wait_completion(packet);

    /* Get the status of the control operation */
    if ( status == FBE_STATUS_OK )
    {
        fbe_payload_control_status_t control_status;
        
        fbe_payload_control_get_status(control_p, &control_status);

        if ( control_status != FBE_PAYLOAD_CONTROL_STATUS_OK )
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }
    fbe_payload_ex_release_control_operation(sep_payload_p, control_p);

    /* retrieve the status of the packet */
    if ( status == FBE_STATUS_OK || status == FBE_STATUS_PENDING )
    {
        status = fbe_transport_get_status_code(packet);
    }
    fbe_transport_release_packet(packet);

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_class_send_control_request()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_class_get_drive_transfer_limits()
 ******************************************************************************
 * 
 * @brief   This method retrieves the per-drive transfer limits.  The limits are
 *          used by the raid algorithms to prevent a request from exceeded the
 *          transfer size that a drive (or port or miniport) can accept.
 * 
 * @param   provision_drive_p - Pointer to the raid group object
 * @param   max_blocks_per_request_p - Address of maxiumum number of block per
 *              drive request to populate
 *
 * @return  fbe_status_t
 * 
 * @todo    Need to add support for maximum number of sg entries also
 *
 * @author
 *  07/14/2010  Ron Proulx  - Created
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_class_get_drive_transfer_limits(fbe_provision_drive_t * provision_drive_p,
                                                                 fbe_block_count_t * max_blocks_per_request_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /*! First determine if we need to generate the maximum blocks per drive
     *  or not.
     *  
     *  @note Current assumption is that the maximum number of blocks per drive
     *        only needs to be determined once per boot.
     */
    if (fbe_provision_drive_class_is_max_drive_blocks_configured() == FBE_FALSE)
    {
        fbe_u32_t max_bytes_per_drive_request;
        fbe_u32_t max_sg_entries;

        /* Need to issue control request to get maximum number of bytes
         * per drive request.
         */
        status = fbe_database_get_drive_transfer_limits(&max_bytes_per_drive_request, &max_sg_entries);

        /* If the request succeeds convert the maximum bytes per block to maximum blocks. */
        if (status == FBE_STATUS_OK)
        {
            fbe_block_count_t   max_blocks_per_drive;

            /* Trace some information and set, then return the maximum blocks per drive */
            max_blocks_per_drive = max_bytes_per_drive_request / FBE_BE_BYTES_PER_BLOCK;
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Max blocks per drive: %llu \n",
                                  __FUNCTION__,
				  (unsigned long long)max_blocks_per_drive);
            fbe_provision_drive_class_set_max_drive_blocks(max_blocks_per_drive);
            *max_blocks_per_request_p = fbe_provision_drive_class_get_max_drive_blocks();
        }
        else
        {
            /* Else report the error */
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Request to get transfer limits failed with status: 0x%x\n",
                                  __FUNCTION__, status);
        }
    }
    else
    {
        /* Else simply set the return value to the configured value */
        *max_blocks_per_request_p = fbe_provision_drive_class_get_max_drive_blocks();
    }

    /* Always return the execution status */
    return(status);
}
/******************************************************************************
 * end fbe_provision_drive_class_get_drive_transfer_limits()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_class_set_debug_flag()
 ******************************************************************************
 * @brief
 *  This function is used to set the provision drive debug  flag value.at class level which is applicalbe 
 *  to all PVD objects.
 * 
 * @param  provision_drive      - provision drive object.
 * @param  packet               - pointer to the packet.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  09/07/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_class_set_debug_flag(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_provision_drive_set_debug_trace_flag_t * set_trace_flag = NULL;
    fbe_payload_control_buffer_length_t         length = 0;


    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &set_trace_flag);
    if (set_trace_flag == NULL) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with set get provision drive 
     * information structure lenght then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_provision_drive_set_debug_trace_flag_t)) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* set the PVD debug trace flag at class level */
    fbe_provision_drive_class_set_class_debug_flag(set_trace_flag->trace_flag);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} 
/******************************************************************************
 * end fbe_provision_drive_class_set_debug_flag()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_provision_drive_class_get_debug_flag()
 *****************************************************************************
 *
 * @brief   Return the setting of the default pvd debug flags that
 *          will be used for all created provision drive objects.
 *
 * @param   debug_flags_p - Pointer to value of pvd debug flags 
 *
 * @return  None (always succeeds)
 *
 * @author
 *  09/21/2010  Amit Dhaduk - Created
 *
 *****************************************************************************/
void fbe_provision_drive_class_get_debug_flag(fbe_provision_drive_debug_flags_t *debug_flags_p)
{
    *debug_flags_p = fbe_provision_drive_class_debug_flag;
    return;
}
/******************************************************************************
 * end fbe_provision_drive_class_get_debug_flag()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_provision_drive_class_set_class_debug_flag()
 *****************************************************************************
 *
 * @brief   Set the value to be used for the pvd debug flags for any
 *          created provision driver object.
 *
 * @param   new_pvd_debug_flags - New pvd debug flags
 *
 * @return  status - Always FBE_STATUS_OK
 *
 * @author
 *  09/21/2010  Amit Dhaduk - Created
 *
 *****************************************************************************/
void fbe_provision_drive_class_set_class_debug_flag(fbe_provision_drive_debug_flags_t new_pvd_debug_flags)
{
    fbe_provision_drive_class_debug_flag = new_pvd_debug_flags;
    return;
}
/******************************************************************************
 * end fbe_provision_drive_class_set_class_debug_flag()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_provision_drive_class_is_debug_flag_set()
 *****************************************************************************
 *
 * @brief   check if given debug flag value is set at class level.
 *
 * @param   debug_flags - debug value to check
 *
 * @return  FBE_TRUE - if given debug flag is set
 *             FBE_FALSE    - if given debug flag is not set  
 *
 * @author
 *  09/21/2010  Amit Dhaduk - Created
 *
 *****************************************************************************/
fbe_bool_t fbe_provision_drive_class_is_debug_flag_set(fbe_provision_drive_debug_flags_t debug_flags)
{
    return ((fbe_provision_drive_class_debug_flag & debug_flags) == debug_flags);
}
/******************************************************************************
 * end fbe_provision_drive_class_is_debug_flag_set()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_class_set_resource_priority()
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
fbe_provision_drive_class_set_resource_priority(fbe_packet_t * packet_p)
{
    if((packet_p->resource_priority_boost & FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_PVD) == 0)
    {
        /* If flag is not set, set it to true */
        packet_p->resource_priority_boost |= FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_PVD;
    }
    else
    {
        /* Packet is being resent/reused without resetting the resource priroity. */ 
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                       "pvd: resource priority is not reset for packet 0x%p \n",
                                       packet_p);
    }

    if(packet_p->resource_priority >= FBE_MEMORY_DPS_PVD_BASE_PRIORITY)
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
        packet_p->resource_priority = FBE_MEMORY_DPS_PVD_BASE_PRIORITY;
    }  
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_class_set_resource_priority()
 ******************************************************************************/

/*!****************************************************************************
* fbe_provision_drive_usurper_set_slf_enabled()
******************************************************************************
* @brief
*	This function enables/disables single loop failure systemwide
* 
* @param  packet				- pointer to the packet.
*
* @return status				- status of the operation.
*
* @author
*	04/30/2012 - Created. Lili Chen
*
******************************************************************************/
static fbe_status_t
fbe_provision_drive_usurper_set_slf_enabled(fbe_packet_t * packet_p)
{   
    fbe_bool_t *                            slf_enabled_p = NULL;
    fbe_payload_ex_t * 	                    payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &slf_enabled_p);
    if (slf_enabled_p == NULL){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_bool_t)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_provision_drive_set_slf_enabled(*slf_enabled_p);
	
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_usurper_set_slf_enabled()
******************************************************************************/


/*!****************************************************************************
* fbe_provision_drive_usurper_is_slf_enabled()
******************************************************************************
* @brief
*	This function checks whether single loop failure systemwide
* 
* @param  packet				- pointer to the packet.
*
* @return status				- status of the operation.
*
* @author
*	10/01/2012 - Created. Lili Chen
*
******************************************************************************/
static fbe_status_t
fbe_provision_drive_usurper_is_slf_enabled(fbe_packet_t * packet_p)
{   
    fbe_bool_t *                            slf_enabled_p = NULL;
    fbe_payload_ex_t * 	                    payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &slf_enabled_p);
    if (slf_enabled_p == NULL){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_bool_t)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    *slf_enabled_p = fbe_provision_drive_is_slf_enabled();
	
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_usurper_is_slf_enabled()
******************************************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_class_set_bg_op_speed()
 ******************************************************************************
 * @brief
 *  This function is used to set Background operation speed.
 *
 * @param  packet               - pointer to the packet.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  08/06/2012 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_class_set_bg_op_speed(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_provision_drive_control_set_bg_op_speed_t * set_op_speed = NULL;
    fbe_payload_control_buffer_length_t         length = 0;


    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &set_op_speed);
    if (set_op_speed == NULL) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with set get provision drive 
     * information structure lenght then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_provision_drive_control_set_bg_op_speed_t)) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(set_op_speed->background_op == FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING)
    {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "pvd: set zeroing speed to %d \n",set_op_speed->background_op_speed);
    
        fbe_provision_drive_set_disk_zeroing_speed(set_op_speed->background_op_speed);
    }
    else if(set_op_speed->background_op == FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF) 
    {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "pvd: set sniff speed to %d \n",set_op_speed->background_op_speed);
    
        fbe_provision_drive_set_sniff_speed(set_op_speed->background_op_speed);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} 
/******************************************************************************
 * end fbe_provision_drive_class_set_bg_op_speed()
 ******************************************************************************/



/*!****************************************************************************
 *          fbe_provision_drive_class_get_bg_op_speed()
 ******************************************************************************
 * @brief
 *  This function is used to get Background operation speed.
 *
 * @param  packet               - pointer to the packet.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  08/06/2012 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_class_get_bg_op_speed(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_provision_drive_control_get_bg_op_speed_t * get_op_speed = NULL;
    fbe_payload_control_buffer_length_t         length = 0;


    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &get_op_speed);
    if (get_op_speed == NULL) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with set get provision drive 
     * information structure lenght then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_provision_drive_control_get_bg_op_speed_t)) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_op_speed->disk_zero_speed = fbe_provision_drive_get_disk_zeroing_speed();
    get_op_speed->sniff_speed = fbe_provision_drive_get_sniff_speed();

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} 
/******************************************************************************
 * end fbe_provision_drive_class_get_bg_op_speed()
 ******************************************************************************/

/*!**************************************************************
 * fbe_provision_drive_get_dword_from_registry()
 ****************************************************************
 * @brief
 *  Read this flag from the registry.
 *
 * @param flag_p
 * @param key_p             
 *
 * @return None.
 *
 * @author
 *  8/03/2015 - Created. Deanna Heng
 *
 ****************************************************************/

static fbe_status_t fbe_provision_drive_get_dword_from_registry(fbe_u32_t *flag_p, 
                                                                fbe_u8_t* key_p,
                                                                fbe_u32_t default_input_value)
{
    fbe_status_t status;
    fbe_u32_t flags;


    *flag_p = default_input_value;

    status = fbe_registry_read(NULL,
                               SEP_REG_PATH,
                               key_p,
                               &flags,
                               sizeof(fbe_u32_t),
                               FBE_REGISTRY_VALUE_DWORD,
                               &default_input_value,
                               sizeof(fbe_u32_t),
                               FALSE);

    if (status != FBE_STATUS_OK)
    {       
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    *flag_p = flags;
    return status;
}
/******************************************
 * end fbe_provision_drive_get_dword_from_registry()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_class_load_registry_keys()
 ****************************************************************
 * @brief
 *  Fetch class specific variables from registry.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  8/03/2015 - Created. Deanna Heng
 *
 ****************************************************************/

void fbe_provision_drive_class_load_registry_keys(void)
{
    fbe_status_t status;
    fbe_u32_t wear_level_timer;
    
    status = fbe_provision_drive_get_dword_from_registry(&wear_level_timer, 
                                                         FBE_PROVISION_DRIVE_REG_KEY_WEAR_LEVEL_TIMER_SEC,
                                                         FBE_PROVISION_DRIVE_DEFAULT_WEAR_LEVEL_TIMER_SEC);
    if (status == FBE_STATUS_OK) 
    { 
        fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "PVD load class registry entry wear leveling timer: 0x%x seconds\n", wear_level_timer);
        fbe_provision_drive_class_set_wear_leveling_timer(wear_level_timer);
    }

}
/******************************************
 * end fbe_provision_drive_class_load_registry_keys()
 ******************************************/

/*!***************************************************************************
 *          fbe_provision_drive_class_set_wear_leveling_timer()
 *****************************************************************************
 *
 * @brief   Set the default wear leveling timer (in seconds)
 *
 * @param   debug_flags - debug value to check
 *
 * @return  None 
 *
 * @author
 *  8/03/2015 - Created. Deanna Heng
 *
 *****************************************************************************/
void fbe_provision_drive_class_set_wear_leveling_timer(fbe_u64_t wear_level_timer)
{
    fbe_provision_drive_default_wear_leveling_timer_seconds = wear_level_timer;
}
/******************************************
 * end fbe_provision_drive_class_set_wear_leveling_timer()
 ******************************************/

/*!***************************************************************************
 *          fbe_provision_drive_class_get_wear_leveling_timer()
 *****************************************************************************
 *
 * @brief   get wear leveling timer (in seconds)
 *
 * @return  fbe_u64_t - wear leveling 
 *
 * @author
 *  08/03/2015  Deanna Heng - Created
 *
 *****************************************************************************/
fbe_u64_t fbe_provision_drive_class_get_wear_leveling_timer(void)
{
    return fbe_provision_drive_default_wear_leveling_timer_seconds;
}
/******************************************
 * end fbe_provision_drive_class_get_wear_leveling_timer()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_usurper_set_wear_leveling_timer()
 ****************************************************************
 * @brief
 *  set the wear leveling timer on the pvd
 *
 * @param   provision_drive_p - Pointer to provision drive object
 * @param   packet_p - Pointer to the packet
 *
 * @return  status   
 *
 * @author
 *  8/04/2015   Deanna Heng - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_provision_drive_usurper_set_wear_leveling_timer(fbe_packet_t * packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_payload_control_buffer_length_t length;
    fbe_u64_t                         *wear_leveling_timer_p = NULL;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &wear_leveling_timer_p);
    if (wear_leveling_timer_p == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with set get provision drive 
     * information structure length then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_u64_t)) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s fbe_payload_control_get_buffer length failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_provision_drive_class_set_wear_leveling_timer(*wear_leveling_timer_p);

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/******************************************
 * end fbe_provision_drive_usurper_set_wear_leveling_timer()
 ******************************************/

/*!***************************************************************************
 *          fbe_provision_drive_class_get_warranty_period()
 *****************************************************************************
 *
 * @brief   get warranty period in hours of the drive (default is 5 years)
 *
 * @return  fbe_u64_t - warranty period in hours
 *
 * @author
 *  08/04/2015  Deanna Heng - Created
 *
 *****************************************************************************/
fbe_u64_t fbe_provision_drive_class_get_warranty_period(void)
{
    return fbe_provision_drive_warranty_period_hours;
}
/******************************************
 * end fbe_provision_drive_class_get_warranty_period()
 ******************************************/

/******************************
 * end fbe_provision_drive_usurper.c
 ******************************/
