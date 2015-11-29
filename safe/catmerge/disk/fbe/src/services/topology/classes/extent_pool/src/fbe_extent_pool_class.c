/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_extent_pool_class.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the extent pool class code.
 * 
 * @version
 *   6/13/2014 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_extent_pool_private.h"
#include "fbe/fbe_extent_pool.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_spare.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe_database.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_library.h"
#include "fbe_cmi.h"
#include "fbe_private_space_layout.h"
#include "fbe_notification.h"



/*****************************************
 * LOCAL GLOBALS
 *****************************************/
/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t fbe_extent_pool_class_set_options(fbe_packet_t * packet_p);

static fbe_block_count_t fbe_extent_pool_blocks_per_slice = FBE_EXTENT_POOL_SLICE_BLOCKS;

fbe_extent_pool_slice_t *fbe_extent_pool_class_slice_pool_memory_p = NULL;
fbe_queue_head_t fbe_extent_pool_class_slice_pool_head;
fbe_u32_t fbe_extent_pool_class_free_slice;

fbe_status_t fbe_extent_pool_class_get_blocks_per_slice(fbe_block_count_t *blocks_p)
{
    *blocks_p = fbe_extent_pool_blocks_per_slice;
    return FBE_STATUS_OK;
}
fbe_status_t fbe_extent_pool_class_set_blocks_per_slice(fbe_block_count_t blocks)
{
    fbe_extent_pool_blocks_per_slice = blocks;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_extent_pool_class_get_blocks_per_disk_slice(fbe_block_count_t *blocks_p)
{
    *blocks_p = fbe_extent_pool_blocks_per_slice + FBE_EXTENT_POOL_SLICE_METADATA_BLOCKS;
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_extent_pool_class_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the raid group class.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t - Return status of the operation.
 * 
 * @author
 *  6/13/2014 - Created. Rob Foley
 ****************************************************************/
fbe_status_t 
fbe_extent_pool_class_control_entry(fbe_packet_t * packet)
{
    fbe_status_t                            status;
    fbe_payload_ex_t *                     sep_payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_payload_control_operation_opcode_t  opcode;

    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    fbe_payload_control_get_opcode(control_operation, &opcode);

    switch (opcode) {
        case FBE_EXTENT_POOL_CONTROL_CODE_CLASS_SET_OPTIONS:
            status  = fbe_extent_pool_class_set_options(packet);
            break;
        default:
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            status = fbe_transport_complete_packet(packet);
            break;
    }
    return status;
}
/********************************************
 * end fbe_extent_pool_class_control_entry
 ********************************************/

/*!**************************************************************
 * fbe_extent_pool_class_set_options()
 ****************************************************************
 * @brief Set some raid group class specific options.
 *
 * @param   packet_p - control packet.               
 *
 * @return  fbe_status_t   
 *
 * @author
 *  6/13/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_extent_pool_class_set_options(fbe_packet_t * packet_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_extent_pool_class_set_options_t *set_options_p = NULL;  /* INPUT */
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_buffer_length_t length;
    fbe_payload_control_operation_t *control_operation_p = NULL; 

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &set_options_p);
   
    /* Validate that a buffer was supplied
     */
    if (set_options_p == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_EXTENT_POOL, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d no buffer supplied \n",
                                 __FUNCTION__, __LINE__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate length
     */
    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(fbe_extent_pool_class_set_options_t))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_EXTENT_POOL, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d expected buffer length: 0x%llx actual: 0x%x\n",
                                 __FUNCTION__, __LINE__,
                                 (unsigned long long)sizeof(*set_options_p),
                                 length);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_extent_pool_blocks_per_slice = set_options_p->blocks_per_slice;
    fbe_topology_class_trace(FBE_CLASS_ID_EXTENT_POOL, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "extent pool: blocks per slice now %llu\n",
                                 (unsigned long long)set_options_p->blocks_per_slice);
    /* Success.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/******************************************************************************
 * end fbe_extent_pool_class_set_options()
 ******************************************************************************/

/*!**************************************************************
 * fbe_extent_pool_class_init_slice_memory()
 ****************************************************************
 * @brief
 *  Allocate and initialize the memory used by the extent pool class.
 *
 * @param extent_pool_p - Pool object.               
 *
 * @return fbe_status_t
 *
 * @author
 *  6/11/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_extent_pool_class_init_slice_memory(void)
{
    fbe_extent_pool_slice_t *slice_p = NULL;
    fbe_slice_index_t        slice_index;
    fbe_slice_count_t        slice_count = FBE_EXTENT_POOL_MAX_SLICES;

    slice_p = fbe_memory_allocate_required((fbe_u32_t)(sizeof(fbe_extent_pool_slice_t) * slice_count));
    if (slice_p == NULL) {fbe_topology_class_trace(FBE_CLASS_ID_EXTENT_POOL, 
                                                   FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                   "%s Cannot allocate memory for pool slices\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_extent_pool_class_slice_pool_memory_p = slice_p; 

    fbe_queue_init(&fbe_extent_pool_class_slice_pool_head);
    for (slice_index = 0; slice_index < slice_count; slice_index++) {
        slice_p->address = 0;
        slice_p->flags = 0;
        fbe_queue_push(&fbe_extent_pool_class_slice_pool_head, &slice_p->queue_element);
        slice_p++;
    }
    fbe_extent_pool_class_free_slice = (fbe_u32_t)slice_count;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_class_init_slice_memory()
 ******************************************/

/*!**************************************************************
 * fbe_extent_pool_allocate_slice()
 ****************************************************************
 * @brief
 *  Fetch a free slice from the queue.
 *
 * @param extent_pool_p - Pool object.
 * @param disk_info_p -
 * @param slice_p - output free slice.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/18/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_extent_pool_allocate_slice(fbe_extent_pool_t *extent_pool_p,
                                            fbe_extent_pool_slice_t **slice_p)
{
    *slice_p = (fbe_extent_pool_slice_t*)fbe_queue_pop(&fbe_extent_pool_class_slice_pool_head);
    fbe_extent_pool_class_free_slice--;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_extent_pool_allocate_slice()
 **************************************/

/*!**************************************************************
 * fbe_extent_pool_deallocate_slice()
 ****************************************************************
 * @brief
 *  Free a slice to the queue.
 *
 * @param extent_pool_p - Pool object.
 * @param slice_p -  slice.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/18/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_extent_pool_deallocate_slice(fbe_extent_pool_t *extent_pool_p,
                                            fbe_extent_pool_slice_t *slice_p)
{
    slice_p->address = 0;
    slice_p->flags = 0;
    fbe_queue_push(&fbe_extent_pool_class_slice_pool_head, &slice_p->queue_element);
    fbe_extent_pool_class_free_slice++;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_extent_pool_deallocate_slice()
 **************************************/

/*!**************************************************************
 * fbe_extent_pool_class_release_slice_memory()
 ****************************************************************
 * @brief
 *  Allocate and initialize the memory used by the extent pool class.
 *
 * @param extent_pool_p - Pool object.               
 *
 * @return fbe_status_t
 *
 * @author
 *  6/11/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_extent_pool_class_release_slice_memory(void)
{
    fbe_memory_release_required(fbe_extent_pool_class_slice_pool_memory_p);
    fbe_queue_init(&fbe_extent_pool_class_slice_pool_head);
    fbe_extent_pool_class_free_slice = 0;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_class_release_slice_memory()
 ******************************************/
/******************************
 * end fbe_extent_pool_class.c
 ******************************/


