 /***************************************************************************
  * Copyright (C) EMC Corporation 2009-2010
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/
 
 /*!**************************************************************************
  * @file fbe_extent_pool_executor.c
  ***************************************************************************
  *
  * @brief
  *  This file contains the executer functions for the extent_pool.
  * 
  *  This includes the fbe_extent_pool_io_entry() function which is our entry
  *  point for functional packets.
  * 
  * @ingroup extent_pool_class_files
  * 
  * @version
 *   06/06/2014:  Created.
  *
  ***************************************************************************/
 
 /*************************
  *   INCLUDE FILES
  *************************/
//#include "fbe/fbe_provision_drive.h"
#include "fbe_extent_pool_private.h"
#include "fbe_raid_library.h"
#include "fbe_traffic_trace.h"
#include "fbe_service_manager.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_sector.h"

 /*************************
  *   FUNCTION DEFINITIONS
  *************************/
static fbe_status_t fbe_extent_pool_handle_io(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_extent_pool_start_new_iots(fbe_extent_pool_t *extent_pool_p, 
                                                   fbe_packet_t *packet_p);
static fbe_status_t fbe_extent_pool_start_iots(fbe_extent_pool_t *extent_pool_p,
                                               fbe_raid_iots_t *iots_p);
static fbe_status_t fbe_extent_pool_iots_completion(void * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_extent_pool_iots_cleanup(void * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_extent_pool_iots_finished(fbe_extent_pool_t *extent_pool_p, fbe_raid_iots_t *iots_p);
static fbe_status_t fbe_extent_pool_handle_get_capacity(fbe_extent_pool_t * const extent_pool_p, 
                                                        fbe_packet_t * const packet_p);

static fbe_status_t fbe_extent_pool_get_stripe_lock(fbe_extent_pool_t *extent_pool_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_extent_pool_stripe_lock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

static fbe_status_t fbe_extent_pool_iots_release_stripe_lock(fbe_extent_pool_t *extent_pool_p, fbe_raid_iots_t *iots_p);
static fbe_status_t fbe_extent_pool_stripe_unlock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

/*!****************************************************************************
 * fbe_extent_pool_io_entry()
 ******************************************************************************
 * @brief
 *  This function is the entry point for functional transport operations to the
 *  extent_pool object. The FBE infrastructure will call this function for
 *  our object.
 *
 * @param object_handle - The extent_pool handle.
 * @param packet - The io packet that is arriving.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if transport is unknown,
 *                 otherwise we return the status of the transport handler.
 *
 * @author
 *  06/06/2014 - Created.
 *
 ******************************************************************************/
fbe_status_t 
fbe_extent_pool_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_extent_pool_t * extent_pool = NULL;

    extent_pool = (fbe_extent_pool_t *)fbe_base_handle_to_pointer(object_handle);

    status = fbe_base_config_bouncer_entry((fbe_base_config_t *)extent_pool, packet);
    if(status == FBE_STATUS_OK){ /* Small stack */
        fbe_transport_set_completion_function(packet, fbe_extent_pool_handle_io, extent_pool);
        status = fbe_transport_complete_packet(packet);		
    }
    return status;
}
/******************************************************************************
 * end fbe_extent_pool_io_entry()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_extent_pool_block_transport_entry()
 ******************************************************************************
 * @brief
 *   This is the entry function that the block transport will
 *   call to pass a packet to the provision drive object.
 *  
 * @param context       - Pointer to the provision drive object. 
 * @param packet      - The packet to process.
 * 
 * @return fbe_status_t
 *
 * @author
 *   06/06/2014:  Created.
 *
 ******************************************************************************/
fbe_status_t
fbe_extent_pool_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_extent_pool_t * extent_pool = (fbe_extent_pool_t *)context;

    /* hand off to the provision drive block transport handler. */
    fbe_transport_set_completion_function(packet, fbe_extent_pool_handle_io, extent_pool);
    status = fbe_transport_complete_packet(packet);

    return status;
}

/*!****************************************************************************
 * @fn fbe_extent_pool_handle_io()
 ******************************************************************************
 * @brief
 *  This function is the entry point for block transport operations to the
 *  provision drive object, it is called only from the provision drive block
 *  transport entry.
 *
 * @param extent_pool - Pointer to the provision drive object.
 * @param packet          - Pointer the the I/O packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *   06/06/2014:  Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_extent_pool_handle_io(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_extent_pool_t                      *extent_pool = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_payload_ex_t				       *payload = NULL;
    fbe_payload_block_operation_t          *block_operation_p = NULL;

    extent_pool = (fbe_extent_pool_t *)context;
    payload = fbe_transport_get_payload_ex(packet);
    block_operation_p = fbe_payload_ex_get_block_operation(payload); /* It was get_present here. Why? */
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);


    if (block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE) {
        fbe_extent_pool_start_new_iots(extent_pool, packet);
    } else {
        return fbe_extent_pool_handle_get_capacity(extent_pool, packet);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_extent_pool_handle_io()
 ******************************************************************************/

fbe_status_t fbe_extent_pool_get_slice(fbe_extent_pool_t *extent_pool_p,
                                       fbe_u32_t lun,
                                       fbe_lba_t lba,
                                       fbe_extent_pool_slice_t **slice_p)
{
    fbe_extent_pool_hash_table_t *hash_table_p = NULL;
    fbe_extent_pool_get_hash_table(extent_pool_p, &hash_table_p);

    /* We lookup the slice in the hash table via the lookup.
     */
    *slice_p = hash_table_p[fbe_extent_pool_hash(extent_pool_p, lba)].slice_ptr;
    return FBE_STATUS_OK;
}
                                       
/*!***************************************************************************
 * fbe_extent_pool_start_new_iots()
 *****************************************************************************
 * @brief
 *  Start a new iots that has just arrived in the object.
 * 
 * @param extent_pool_p - the extent pool object.
 * @param packet_p - packet to start the iots for.
 *
 * @return fbe_status_t.
 *
 * @author
 *  6/11/2014 - Created. Rob Foley
 *
 *****************************************************************************/

static fbe_status_t fbe_extent_pool_start_new_iots(fbe_extent_pool_t *extent_pool_p, 
                                                   fbe_packet_t *packet_p)
{
    fbe_raid_iots_t     *iots_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_payload_ex_t    *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t       *block_operation_p = NULL;
    fbe_lba_t                            lba;
    fbe_block_count_t                    blocks;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_chunk_size_t                     chunk_size = FBE_EXTENT_POOL_METADATA_CHUNK_SIZE;
    fbe_extent_pool_slice_t             *slice_p = NULL;
    fbe_edge_index_t                     server_index;

    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    if (block_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p,
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Raid group: %s block operation is null.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_PENDING;
    }

    /* Initialize and kick off iots.
     * The iots is part of the payload. 
     */
    fbe_payload_ex_get_iots(payload_p, &iots_p);
    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_block_count(block_operation_p, &blocks);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    /* All background ops should be a multiple of the chunk size.
     */
    if (fbe_raid_library_is_opcode_lba_disk_based(opcode) && (blocks % chunk_size))
    {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "invalid background op size lba: 0x%llx block_count: 0x%llx\n", lba, blocks);
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_COUNT);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_PENDING;
    }
    raid_geometry_p = &extent_pool_p->geometry_p[0];
    fbe_raid_iots_init(iots_p, packet_p, block_operation_p, raid_geometry_p, lba, blocks);
    fbe_raid_iots_set_chunk_size(iots_p, chunk_size);
    fbe_raid_iots_set_callback(iots_p, fbe_extent_pool_iots_completion, extent_pool_p);

    fbe_transport_get_server_index(packet_p, &server_index);
    fbe_extent_pool_get_slice(extent_pool_p, server_index, /* LUN */ lba, &slice_p);
    fbe_raid_iots_set_extent(iots_p, (void*)slice_p, (void*)extent_pool_p);

    /*! @todo eventually we will allocate the slice on the fly, but for now just error since 
     * everything is mapped ahead of time. 
     */
    if (slice_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "Slice not found for lba: 0x%llx block_count: 0x%llx\n", lba, blocks);
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_COUNT);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_PENDING;
    }
    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                          "extent pool: lun: %u op: %u lba: 0x%llx block_count: 0x%llx slba: 0x%llx\n", 
                          server_index, block_operation_p->block_opcode, lba, blocks, slice_p->address);
    fbe_transport_set_completion_function(packet_p, fbe_extent_pool_stripe_lock_completion, extent_pool_p);
    fbe_extent_pool_get_stripe_lock(extent_pool_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_extent_pool_start_new_iots()
 **************************************/
fbe_status_t fbe_extent_pool_iots_get_slice(fbe_raid_iots_t *iots_p, fbe_extent_pool_slice_t **slice_p)
{
    *slice_p = (fbe_extent_pool_slice_t*)iots_p->extent_p;
    return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_extent_pool_get_stripe_lock()
 ****************************************************************
 * @brief
 *  Get an iots stripe lock for this request.
 * 
 * @param extent_pool_p - the extent pool object.
 * @param packet_p - Packet ptr.
 * 
 * @notes Assumes we are NOT on the terminator queue.
 * 
 * @return fbe_status_t.
 *
 * @author
 *  6/18/2014 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_extent_pool_get_stripe_lock(fbe_extent_pool_t *extent_pool_p,
                                                    fbe_packet_t *packet_p)
{
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_lba_t sl_lba;
    fbe_block_count_t sl_blocks;
    fbe_u64_t stripe_number;
    fbe_u64_t stripe_count;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_bool_t b_is_lba_disk_based;
    fbe_bool_t b_allow_hold;
    fbe_element_size_t                  element_size;
    fbe_payload_block_operation_flags_t block_flags;
    fbe_block_count_t                   blocks_per_slice;
    fbe_u16_t                           lun;
    fbe_extent_pool_slice_t            *slice_p = NULL;
    fbe_raid_iots_t                    *iots_p = NULL;

    fbe_payload_ex_get_iots(payload_p, &iots_p);
    fbe_extent_pool_iots_get_slice(iots_p, &slice_p);
    lun = fbe_extent_pool_slice_address_get_lun(slice_p->address);

    fbe_extent_pool_class_get_blocks_per_slice(&blocks_per_slice);

    fbe_extent_pool_get_user_pool_geometry(extent_pool_p, &raid_geometry_p);
    fbe_raid_geometry_get_element_size(raid_geometry_p, &element_size);

    /* Get the block operation information
     */
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_block_count(block_operation_p, &blocks);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);
    fbe_payload_block_get_flags(block_operation_p, &block_flags);

    b_is_lba_disk_based = fbe_raid_library_is_opcode_lba_disk_based(opcode);

#if 0
    /* Because we are locking within a slice, we need to translate the lba/blocks to within the slice.
     */
    lba = lba % blocks_per_slice;
    blocks = blocks % blocks_per_slice;
#endif

    /* Determine what we should be locking.
     */
    if (b_is_lba_disk_based) {
        /* These use a physical address.
         */
        fbe_raid_geometry_calculate_lock_range_physical(raid_geometry_p,
                                                        lba,
                                                        blocks,
                                                        &stripe_number,
                                                        &stripe_count);
    }
    else {
        /* These use a logical address.
         */
        fbe_raid_geometry_calculate_lock_range(raid_geometry_p,
                                               lba,
                                               blocks,
                                               &stripe_number,
                                               &stripe_count);
    }

    /* We might need to align the request to a different multiple if we are 4k.
     */
    if (fbe_raid_geometry_needs_alignment(raid_geometry_p)) {
        fbe_raid_geometry_align_io(raid_geometry_p, &stripe_number, &stripe_count);
    }
    stripe_lock_operation_p = fbe_payload_ex_allocate_stripe_lock_operation(payload_p);

    /* Determine the setting for `allow hold'
     */
    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY) ||       
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA) ||  
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_MARK_FOR_REBUILD) ||
        (fbe_raid_geometry_is_metadata_io(raid_geometry_p, lba))) {
        /* Any metadata operation or monitor operation cannot 
         * wait for stripe locks when we are quiescing or we will deadlock. 
         */
        b_allow_hold = FBE_FALSE;
    } else {
        b_allow_hold = FBE_TRUE;
    }

    sl_lba = stripe_number;
    sl_blocks = stripe_count;
    //fbe_extent_pool_slice_address_set(&sl_lba, sl_lba, lun);

    /* Build the stripe lock operation and start it.
     */
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) {
        fbe_payload_stripe_lock_build_read_lock(stripe_lock_operation_p, 
                                                &extent_pool_p->base_config.metadata_element, 
                                                sl_lba, sl_blocks); 
    } else {
        fbe_payload_stripe_lock_build_write_lock(stripe_lock_operation_p, 
                                                 &extent_pool_p->base_config.metadata_element, 
                                                 sl_lba, sl_blocks);
    }

    if (block_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_NO_SL_THIS_LEVEL) {
        /* In this case we want to skip the stripe lock.
         */
        stripe_lock_operation_p->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_DO_NOT_LOCK;
    }

	if(b_allow_hold){ /* TODO Deprecated nedd to be removed */
		stripe_lock_operation_p->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_ALLOW_HOLD;
	}


    fbe_payload_ex_increment_stripe_lock_operation_level(payload_p);

    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                          "SL get pkt: %x/%x get lba: 0x%llx bl: 0x%llx l: %u opcode: 0x%x s_n: 0x%llx s_c: 0x%llx\n", 
                          (fbe_u32_t)packet_p, (fbe_u32_t)stripe_lock_operation_p, lba, blocks, lun, opcode,
                          stripe_lock_operation_p->stripe.first, 
						  stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);
#if 0
    /* If we attempted a stripe lock request that is beyond the configured
     * capacity fail the request.
     */
    disk_capacity = fbe_extent_pool_get_disk_capacity(extent_pool_p);
    if (stripe_lock_operation_p->stripe.last > disk_capacity) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RG SL last 0x%llx > exported disk cap 0x%llx\n", 
                              stripe_lock_operation_p->stripe.last, disk_capacity);

        /*! @note Mark the stripe lock as failed and invoke the callback. 
         */
        fbe_payload_stripe_lock_set_status(stripe_lock_operation_p, FBE_PAYLOAD_STRIPE_LOCK_STATUS_ILLEGAL_REQUEST);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
#endif

#if 0
    /*! @todo eventually we will submit to md svc, but for now just complete the packet.
     */
    stripe_lock_operation_p->status = FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK;
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
#else
    fbe_ext_pool_lock_operation_entry(packet_p);
#endif

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_get_stripe_lock()
 ******************************************/
/*!**************************************************************
 * fbe_extent_pool_stripe_lock_completion()
 ****************************************************************
 * @brief
 *  Handle allocation of the lock.
 *
 * @param packet_p - Packet that is completing.
 * @param context - the raid group. 
 *
 * @return fbe_status_t.
 *
 * @author
 *  6/18/2014 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_extent_pool_stripe_lock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_extent_pool_t *extent_pool_p = (fbe_extent_pool_t *)context;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_payload_stripe_lock_status_t stripe_lock_status;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_status_t packet_status;
    fbe_trace_level_t trace_level = FBE_TRACE_LEVEL_DEBUG_LOW;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    packet_status = fbe_transport_get_status_code(packet_p);
    fbe_raid_iots_get_opcode(iots_p, &opcode);
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(sep_payload_p);

    /* Validate that a stripe lock operaiton was requested
     */
    if (stripe_lock_operation_p == NULL) {
        /* Something went wrong
         */
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "SL no stripe lock opt: packet_status: 0x%x op: %d\n", 
                              packet_status, opcode);
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        status = fbe_extent_pool_iots_finished(extent_pool_p, iots_p);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "rg_sl_compl, SL error finishing iots status: 0x%x\n", status);
        }
        /* Return FBE_STATUS_OK so that the packet gets completed up to the next level.
         */
        return FBE_STATUS_OK;
    }

    /* Get the stripe lock operation
     */
    fbe_payload_stripe_lock_get_status(stripe_lock_operation_p, &stripe_lock_status);
    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                          "SL done iots_p:0x%x lba:0x%llx bl:0x%llx opcode:0x%x s_n:0x%llx s_c:0x%llx\n", 
                          (fbe_u32_t)iots_p, iots_p->lba, iots_p->blocks, opcode,
                          stripe_lock_operation_p->stripe.first, 
						  stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);

    if ( (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK) && (packet_status == FBE_STATUS_OK) ) {
        /* Terminate the I/O since we will be sending this to the library.
         */
        //fbe_extent_pool_add_to_terminator_queue(extent_pool_p, packet_p);

        fbe_extent_pool_start_iots(extent_pool_p, iots_p);
    } else if ( (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK) && (packet_status != FBE_STATUS_OK) ) {
        /* We have our lock, but the packet was cancelled on the way back.
         * We only expect cancelled status here, so other status values trace with error.
         */
        if (packet_status != FBE_STATUS_CANCELED) {
            trace_level = FBE_TRACE_LEVEL_ERROR;
        }
        /* In this case we need to release the lock and fail the operation.
         */
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                              "SL bad status/lock held sl_status: 0x%x packet_status: 0x%x op: %d\n", 
                              stripe_lock_status, packet_status, stripe_lock_operation_p->opcode);
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED);

        /* Below code assumes we are on the queue.
         */
        //fbe_extent_pool_add_to_terminator_queue(extent_pool_p, packet_p);
        /* After we release the lock, the iots is freed, and packet completed.
         */
        fbe_extent_pool_iots_release_stripe_lock(extent_pool_p, iots_p);
    } else {
        /* For some reason the stripe lock was not available.
         * We will fail the request. 
         */
        if (((stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_DROPPED) ||
             (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED)) &&
            ((stripe_lock_operation_p->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_ALLOW_HOLD) != 0)) {
            /* For dropped requests we will enqueue to the terminator queue to allow this request to
             * get processed later.  We were dropped because of a drive going away or a quiesce. 
             * Once the monitor resumes I/O, this request will get restarted. 
             *  
             * Monitor or metadata requests do not get quiesced to the terminator queue, 
             * thus we will fail those requests back instead of queuing.
             */
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "SL get failed queuing sl_status: 0x%x pckt_status: 0x%x op: %d\n", 
                                  stripe_lock_status, packet_status, stripe_lock_operation_p->opcode);

            //fbe_raid_iots_transition_quiesced(iots_p, fbe_extent_pool_iots_get_sl, extent_pool_p);
            fbe_payload_ex_release_stripe_lock_operation(sep_payload_p, stripe_lock_operation_p);
            //fbe_extent_pool_add_to_terminator_queue(extent_pool_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;

        }
        if (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED) {
            /* We intentionally aborted this lock due to shutdown.
             */
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "SL aborted sl_status: 0x%x packet_status: 0x%x op: %d\n", 
                                  stripe_lock_status, packet_status, stripe_lock_operation_p->opcode);
            fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
            fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
        } else if ((packet_status == FBE_STATUS_CANCELED) || 
                   (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_CANCELLED)) {
            /* We only check for cancelled.  Only cancelled means that the MDS has returned this as cancelled. 
             */
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "SL cancelled iots: 0x%x sl_status: 0x%x pkt_st: 0x%x op: %d\n", 
                                  stripe_lock_status, (fbe_u32_t) iots_p, packet_status, stripe_lock_operation_p->opcode);
            fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED);
            fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED);
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p,
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "rg_sl_compl:iots_p:%p cancelled lba:0x%llx bl:0x%llx opcode:0x%x s_n:0x%llx s_c:0x%llx\n", 
                                  iots_p, iots_p->lba, iots_p->blocks, opcode,
                                  stripe_lock_operation_p->stripe.first,
                                  stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);
        } else {
            /* Other errors should only get reported on metadata ops, which will specifically handle this error.
             */
            fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
            fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_LOCK_FAILED);

            if (stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_DROPPED) {
                /* We expect dropped/aborted/cancelled, but no other status values.
                 */
                trace_level = FBE_TRACE_LEVEL_ERROR;

                /* Change the status if it was an illegal request.
                 */
                if (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_ILLEGAL_REQUEST) {
                    fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
                }
            }
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                                  "SL failed sl_status: 0x%x packet_status: 0x%x op: %d\n", 
                                  stripe_lock_status, packet_status, stripe_lock_operation_p->opcode);
        }
        fbe_payload_ex_release_stripe_lock_operation(sep_payload_p, stripe_lock_operation_p);
        status = fbe_extent_pool_iots_finished(extent_pool_p, iots_p);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "rg_sl_compl, SL error finishing iots status: 0x%x\n",  status);
        }
        /* Return FBE_STATUS_OK so that the packet gets completed up to the next level.
         */
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_extent_pool_stripe_lock_completion()
 ******************************************/

/*!**************************************************************
 * fbe_extent_pool_start_iots()
 ****************************************************************
 * @brief
 *  Handle an iots completion from the raid library.
 * 
 * @param extent_pool_p - the extent pool object.
 * @param packet_p - packet to start the iots for.
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_extent_pool_start_iots(fbe_extent_pool_t *extent_pool_p,
                                               fbe_raid_iots_t *iots_p)
{
    //fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_chunk_size_t                       chunk_size = FBE_EXTENT_POOL_METADATA_CHUNK_SIZE;
    fbe_lba_t                              lba;   
    fbe_block_count_t                      blocks;    
    fbe_chunk_count_t                      start_bitmap_chunk_index;
    fbe_chunk_count_t                      num_chunks;

    //chunk_size = fbe_extent_pool_get_chunk_size(extent_pool_p);

    fbe_raid_iots_set_callback(iots_p, fbe_extent_pool_iots_completion, extent_pool_p);


    //  Remove the packet from the terminator queue.  We have to do this before any paged metadata access.
    //fbe_extent_pool_remove_from_terminator_queue(extent_pool_p, packet_p);

    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_NON_DEGRADED);
    /*  Get the LBA and blocks from the iots 
    */
    fbe_raid_iots_get_current_op_lba(iots_p, &lba);
    fbe_raid_iots_get_current_op_blocks(iots_p, &blocks);

    /*  Get the range of chunks affected by this iots.
    */
    fbe_raid_iots_get_chunk_range(iots_p, lba, blocks, chunk_size, &start_bitmap_chunk_index, &num_chunks);

    if (num_chunks > FBE_RAID_IOTS_MAX_CHUNKS) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "number of chunks %d is beyond the max for an iots %d\n",
                              num_chunks, FBE_RAID_IOTS_MAX_CHUNKS);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize the chunk info in the IOTS to 0, ie. the chunks are not marked for rebuild or verify 
    */
    fbe_set_memory(&iots_p->chunk_info[0], 0, num_chunks * sizeof(fbe_raid_group_paged_metadata_t));

    fbe_raid_iots_start(iots_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_extent_pool_start_iots()
 ******************************************/

/*!**************************************************************
 * fbe_extent_pool_iots_release_stripe_lock()
 ****************************************************************
 * @brief
 *  Release this iots lock.
 *
 * @param extent_pool_p - current object.
 * @param iots_p - Current iots.
 *
 * @return fbe_status_t.
 *
 * @author
 *  6/18/2014 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_extent_pool_iots_release_stripe_lock(fbe_extent_pool_t *extent_pool_p, fbe_raid_iots_t *iots_p)
{
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_status_t status;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_payload_stripe_lock_status_t stripe_lock_status;

    fbe_raid_iots_get_opcode(iots_p, &opcode);

    /* Pull the packet off the termination queue since it causes issues when the stripe
     * lock service tries to complete a packet. 
     */
#if 0
    status = fbe_extent_pool_remove_from_terminator_queue(extent_pool_p, fbe_raid_iots_get_packet(iots_p));
    if (status != FBE_STATUS_OK) {
        /* Some thing is wrong mark the iots with unexpected error but continue
         * since we want to free the stripe lock.
         */
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: error removing from terminator queue status: 0x%x\n", 
                              __FUNCTION__, status);

        /* Fail iots
         */
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
    }
#endif
    /* Get the stripe lock operation so that we can release the stripe lock
    */
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(sep_payload_p);
    if (stripe_lock_operation_p == NULL) {
        /* We expected a stripe lock. Fail the iots with `unexpected error'.
         * Just finish the iots.
         * We always procedute
         * Pull off the terminator queue since we are done with the I/O. 
         */
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: lba: 0x%llx blks: 0x%llx no stripe lock operation\n", 
                              __FUNCTION__, (unsigned long long)iots_p->lba,
			      (unsigned long long)iots_p->blocks);

        /* Fail iots and finish the request.  Since we completed the iots
         * return success.
         */
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_extent_pool_iots_finished(extent_pool_p, iots_p);

        status = fbe_transport_complete_packet(packet_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: error completing packet status: 0x%x\n", __FUNCTION__, status);
        }
        return FBE_STATUS_OK;
    }

    /* Trace some basic information
     */
    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                          "SL Unlock iots/pkt: %p/%p Release lba: 0x%llx bl: 0x%llx opcode: 0x%x s_n: 0x%llx s_c: 0x%llx\n", 
                          iots_p, packet_p, iots_p->lba, iots_p->blocks, opcode,
                          stripe_lock_operation_p->stripe.first, 
						  stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);

    fbe_payload_stripe_lock_get_status(stripe_lock_operation_p, &stripe_lock_status);
    if (stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: unexpected stripe_lock_status: 0x%x \n", 
                              __FUNCTION__, stripe_lock_status);
    }

    /* Generate the proper unlock operation
     */
    if (stripe_lock_operation_p->opcode == FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK) {
        fbe_payload_stripe_lock_build_read_unlock(stripe_lock_operation_p);
    } else if (stripe_lock_operation_p->opcode == FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK) {
        fbe_payload_stripe_lock_build_write_unlock(stripe_lock_operation_p);
    } else {
        /* Else this stripe lock operation isn't supported
         */
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: unexpected opcode: 0x%x not supported\n", 
                              __FUNCTION__, opcode);
        /* Fail iots and finish the request.  Since we completed the iots
         * return success.
         */
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_extent_pool_iots_finished(extent_pool_p, iots_p);

        status = fbe_transport_complete_packet(packet_p);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: error completing packet status: 0x%x\n", __FUNCTION__, status);
        }
        return FBE_STATUS_OK;
    }

    /* Release the stripe lock
     */
    fbe_transport_set_completion_function(packet_p, fbe_extent_pool_stripe_unlock_completion, extent_pool_p);
#if 0
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    stripe_lock_operation_p->status = FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK;
    fbe_transport_complete_packet(packet_p);
#else
    fbe_ext_pool_lock_operation_entry(packet_p);
#endif

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/***********************************************
 * end fbe_extent_pool_iots_release_stripe_lock()
 ***********************************************/

/*!**************************************************************
 * fbe_extent_pool_stripe_unlock_completion()
 ****************************************************************
 * @brief
 *  Handle allocation of the lock.
 *
 * @param packet_p - Packet that is completing.
 * @param iots_p - The iots that is completing.  This has
 *                 Status in it to tell us if it is done.
 *
 * @return fbe_status_t.
 *
 * @author
 *  6/18/2014 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t 
fbe_extent_pool_stripe_unlock_completion(fbe_packet_t * packet_p, 
                                         fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_extent_pool_t *extent_pool_p = (fbe_extent_pool_t *)context;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_payload_stripe_lock_status_t stripe_lock_status;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(sep_payload_p);

    fbe_payload_stripe_lock_get_status(stripe_lock_operation_p, &stripe_lock_status);

    /* Since we are done with the stripe lock, release the operation.
     */
    fbe_payload_ex_release_stripe_lock_operation(sep_payload_p, stripe_lock_operation_p);

    if (stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK) {
        /* For some reason the stripe lock could not be released.
         * We will display an error.
         */
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: stripe lock failed status: 0x%x\n", __FUNCTION__, stripe_lock_status);
    }
    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                          "SL Unlock done i/sl:%x/%x l/b: 0x%llx/0x%llx op: 0x%x s_n: 0x%llx s_c: 0x%llx\n", 
                          (fbe_u32_t)iots_p, (fbe_u32_t) stripe_lock_operation_p,
                          iots_p->lba, iots_p->blocks, iots_p->block_operation_p->block_opcode,
                          stripe_lock_operation_p->stripe.first, 
						  stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);

    /* Continue to cleanup the iots.
     */
    status = fbe_extent_pool_iots_finished(extent_pool_p, iots_p);
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: error finishing iots status: 0x%x\n", __FUNCTION__, status);
    }
    /* Return FBE_STATUS_OK so that the packet gets completed up to the next level.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_stripe_unlock_completion()
 ******************************************/
/*!**************************************************************
 * fbe_extent_pool_iots_completion()
 ****************************************************************
 * @brief
 *  Handle an iots completion from the raid library.
 *
 * @param packet_p - Packet that is completing.
 * @param iots_p - The iots that is completing.  This has
 *                 Status in it to tell us if it is done.
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_extent_pool_iots_completion(void * packet_p, fbe_packet_completion_context_t context)
{
    //fbe_extent_pool_t *extent_pool_p = (fbe_extent_pool_t *)context;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_payload_block_operation_opcode_t current_opcode;
    fbe_payload_block_operation_status_t block_status;
    fbe_lba_t lba;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    fbe_raid_iots_get_opcode(iots_p, &opcode);
    fbe_raid_iots_get_current_opcode(iots_p, &current_opcode);
    fbe_raid_iots_get_block_status(iots_p, &block_status);
    fbe_raid_iots_get_lba(iots_p, &lba);

    fbe_extent_pool_iots_cleanup(packet_p, context);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_extent_pool_iots_completion()
 **************************************/
/*!**************************************************************
 * fbe_extent_pool_iots_cleanup()
 ****************************************************************
 * @brief
 *  Iots is done, cleanup and send status.
 *
 * @param packet_p - Packet that is completing.
 * @param context - raid group ptr.
 *
 * @return fbe_status_t.
 *
 * @author
 *  8/3/2010 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_extent_pool_iots_cleanup(void * packet_p, fbe_packet_completion_context_t context)
{
    fbe_extent_pool_t *extent_pool_p = (fbe_extent_pool_t *)context;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    /* If debug is enabled trace the error.
     */
    if (iots_p->error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        fbe_payload_block_operation_opcode_t opcode;

        fbe_raid_iots_get_opcode(iots_p, &opcode);
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s iots: %p failed status: 0x%x qual: 0x%x opcode: 0x%x lba: 0x%llx bl: 0x%llx\n",
                              __FUNCTION__, iots_p, iots_p->error,
                              iots_p->qualifier, opcode,
                              (unsigned long long)iots_p->packet_lba,
                              (unsigned long long)iots_p->packet_blocks);
    }
    //fbe_extent_pool_remove_from_terminator_queue(extent_pool_p, fbe_raid_iots_get_packet(iots_p));
    fbe_extent_pool_iots_release_stripe_lock(extent_pool_p, iots_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_extent_pool_iots_cleanup()
 **************************************/

/*!**************************************************************
 * fbe_extent_pool_iots_finished()
 ****************************************************************
 * @brief
 *  Handle an iots completion from the raid library.
 *
 * @param extent_pool_p - current raid group.
 * @param iots_p - The iots that is completing. This has
 *                 Status in it to tell us if it is done.
 * 
 * @notes It is the responsibility of the caller to call
 *        fbe_transport_complete_packet()
 * 
 * @return fbe_status_t.
 *
 * @author
 *  6/11/2014 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_extent_pool_iots_finished(fbe_extent_pool_t *extent_pool_p, fbe_raid_iots_t *iots_p)
{
    fbe_status_t status;

    /* We are done.  Set the status from the iots in the packet.
     */
    status = fbe_raid_iots_set_packet_status(iots_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error setting packet status: 0x%x\n", 
                              __FUNCTION__, status);
    }
    /* Destroy the iots now that it is no longer on a queue.
     */
    status = fbe_raid_iots_destroy(iots_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error destroying iots status: 0x%x\n", 
                              __FUNCTION__, status);
    }
    return status;
}
/******************************************
 * end fbe_extent_pool_iots_finished()
 ******************************************/

/*!***************************************************************
 * fbe_extent_pool_handle_get_capacity()
 ****************************************************************
 * @brief
 *  This function handles the negotiate block size packet.
 * 
 *  This allows the caller to determine which block size and optimal block size
 *  they can use when sending I/O requests.
 *
 * @param extent_pool_p - The raid group handle.
 * @param packet_p - The mgmt packet that is arriving.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/16/2014 - Copied from fbe_raid_group_negotiate_block_size. Rob Foley
 * 
 ****************************************************************/
static fbe_status_t
fbe_extent_pool_handle_get_capacity(fbe_extent_pool_t * const extent_pool_p, 
                                    fbe_packet_t * const packet_p)
{
    fbe_block_transport_negotiate_t* negotiate_block_size_p = NULL;

    /* First get a pointer to the block command.
     */
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_sg_element_t *sg_p;
    fbe_status_t status;
    fbe_lba_t capacity;
    fbe_block_edge_t *edge_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = &extent_pool_p->geometry_p[0];
    fbe_u32_t extent_pool_optimal_block_size;

    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);

    if (sg_p == NULL)
    {
        /* We always expect to receive a buffer.
         */
        fbe_base_object_trace((fbe_base_object_t *)extent_pool_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "raid group: negotiate sg null. 0x%p %s\n", 
                              extent_pool_p, __FUNCTION__);
        /* Transport status on the packet is OK since it was received OK. 
         * The payload status is set to bad since this command is not formed 
         * correctly. 
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NULL_SG);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Next, get a ptr to the negotiate info. 
     */
    negotiate_block_size_p = (fbe_block_transport_negotiate_t*)fbe_sg_element_address(sg_p);

    if (negotiate_block_size_p == NULL)
    {
        /* We always expect to receive a buffer.
         */
        fbe_base_object_trace((fbe_base_object_t *)extent_pool_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "raid group: negotiate buffer null. 0x%p %s \n", 
                              extent_pool_p, __FUNCTION__);
        /* Transport status on the packet is OK since it was received OK. 
         * The payload status is set to bad since this command is not formed 
         * correctly. 
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_SG);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Generate the negotiate info.  
     * Raid always exports 520 (FBE_BE_BYTES_PER_BLOCK), with an optimal block size 
     * that is set to the raid group's optimal size, which gets set by the 
     * monitor's "get block size" condition. 
     */
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &extent_pool_optimal_block_size); 
    status = FBE_STATUS_OK;
    fbe_base_config_get_capacity(&extent_pool_p->base_config, &capacity);
    edge_p = (fbe_block_edge_t*)packet_p->base_edge;
    if (edge_p != NULL)
    {
        fbe_u16_t data_disks;
        fbe_u32_t stripe_size;
        fbe_element_size_t element_size;
        fbe_raid_geometry_get_element_size(raid_geometry_p, 
                                           &element_size);
        /* We have an edge, so fill in the optimal size and block count from the edge.
         */
        fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
        stripe_size = data_disks * element_size;

        /* Optimal block alignment is the max of either the optimum block size or the
         * stripe size. 
         */
        negotiate_block_size_p->optimum_block_alignment = 
        FBE_MAX(extent_pool_optimal_block_size, stripe_size);

        negotiate_block_size_p->block_count = edge_p->capacity;
    }
    else
    {
        negotiate_block_size_p->block_count = capacity;
        negotiate_block_size_p->optimum_block_alignment = 0;
    }
    negotiate_block_size_p->optimum_block_size = extent_pool_optimal_block_size;
    negotiate_block_size_p->block_size = FBE_BE_BYTES_PER_BLOCK;
    negotiate_block_size_p->physical_block_size = FBE_BE_BYTES_PER_BLOCK;
    negotiate_block_size_p->physical_optimum_block_size = 1;

    /* Transport status is set based on the status returned from calculating.
     */
    fbe_transport_set_status(packet_p, status, 0);

    if (status != FBE_STATUS_OK)
    {
        fbe_block_edge_t *edge_p = NULL;

        fbe_base_config_get_block_edge(&extent_pool_p->base_config,
                                       &edge_p,
                                       0);

        /* The only way this can fail is if the combination of block sizes are
         * not supported. 
         */
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);

        fbe_base_object_trace((fbe_base_object_t *)extent_pool_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "extent pool: negotiate invalid block size. Geometry:0x%x req:0x%x "
                              "req opt size:0x%x\n",
                              edge_p->block_edge_geometry, 
                              negotiate_block_size_p->requested_block_size,
                              negotiate_block_size_p->requested_optimum_block_size);
    }
    else
    {
        /* Negotiate was successful.
         */
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    }

    return FBE_STATUS_OK;
}
/* end fbe_extent_pool_handle_get_capacity */
/*******************************************
 * end file fbe_extent_pool_executor.c
 *******************************************/
 
 
 
