/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_rdgen_object.c
 ***************************************************************************
 *
 * @brief
 *  This file contains rdgen object related functions.
 *
 * @version
 *   5/28/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_rdgen.h"
#include "fbe_rdgen_private_inlines.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_bvd_interface.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_lun.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t fbe_rdgen_object_get_edge(fbe_rdgen_object_t * object_p, fbe_packet_t * packet_p, fbe_semaphore_t *sem);

/*************************
 *   GLOBAL DEFINITIONS
 *************************/
static fbe_rdgen_notification_context_t fbe_rdgen_object_destroy_notification_context = { 0 };

/*!**************************************************************
 * fbe_rdgen_object_init()
 ****************************************************************
 * @brief
 *  This function initializes an rdgen object.
 *
 * @param object_p - Object to initialize.
 * @param object_id - The object id to init.
 * @param package_id - The package this object is in.
 * @param name_p - Name is ptr to possible device string.
 *
 * @return None.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_object_init(fbe_rdgen_object_t *object_p,
                           fbe_object_id_t object_id,
                           fbe_package_id_t package_id,
                           fbe_char_t *name_p,
                           fbe_rdgen_control_start_io_t *start_p)
{
    fbe_zero_memory(object_p, sizeof(fbe_rdgen_object_t));
    fbe_spinlock_init(&object_p->lock);
    fbe_queue_init(&object_p->active_ts_head);
    fbe_queue_init(&object_p->active_request_head);
    fbe_queue_init(&object_p->valid_record_queue);
    fbe_queue_init(&object_p->free_record_queue);

    object_p->object_id = object_id;
    object_p->package_id = package_id;
    object_p->capacity = FBE_LBA_INVALID;
    object_p->destroy_edge_ptr = FBE_TRUE;/*edges to all objects but LUN are fake so we have to destroy them*/
    object_p->edge_ptr = NULL;
    fbe_queue_element_init(&object_p->queue_element);
    fbe_rdgen_object_set_magic(object_p);
    object_p->topology_object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    object_p->b_is_object_under_test_destroyed = FBE_FALSE;
    csx_p_snprintf(&object_p->device_name[0], FBE_RDGEN_DEVICE_NAME_CHARS, "%s", name_p);

    if (start_p->specification.options & FBE_RDGEN_OPTIONS_FIXED_RANDOM_NUMBERS)
    {
        /* We seed based on the value passed in for the seed base, and object id.
         * This ensures that this object will always start with the same base for a given passed in seed.
         */
        fbe_rdgen_object_set_random_context(object_p, object_p->object_id + start_p->specification.random_seed_base);
    }
    else 
    {
        /* Initialize the random seed from a random number.
         */
        fbe_rdgen_object_set_random_context(object_p, fbe_rdgen_get_random_seed());
    }
    return;
}
/******************************************
 * end fbe_rdgen_object_init()
 ******************************************/

fbe_status_t 
fbe_rdgen_object_init_completion(fbe_packet_t * packet,
                                 fbe_packet_completion_context_t context)
{   
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*!**************************************************************
 * fbe_rdgen_object_initialize_capacity()
 ****************************************************************
 * @brief
 *  This function initializes an rdgen object's capacity..
 *
 * @param object_p - Object to initialize.
 *
 * @return FBE_STATUS_OK if the object was initted OK.
 *
 * @author
 *  9/28/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_object_initialize_capacity(fbe_rdgen_object_t *object_p)
{
    fbe_status_t                            status;
    fbe_packet_t                           *packet_p = NULL;
    fbe_sg_element_t                        sg;
    fbe_block_transport_negotiate_t         negotiate_block_size;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_block_operation_t          *block_operation_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_semaphore_t                         sem;
    fbe_base_config_control_get_capacity_t  base_config_control_get_capacity;
    fbe_cpu_id_t                            cpu_id;
    fbe_block_count_t                       cache_zero_bit_map_size = 0;

    /* Check if we already know about this object and it hasn't been destroyed.
     */
    if ((object_p->capacity != FBE_LBA_INVALID)                   &&
        (object_p->b_is_object_under_test_destroyed == FBE_FALSE)    )
    {
        /* Else we have already initialized this object and it has not changed.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "fbe_rdgen_object_initialize_capacity already initialized obj: 0x%x pkg: 0x%x\n",
                                object_p->object_id, object_p->package_id);
        return FBE_STATUS_OK;
    }

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "fbe_rdgen_object_initialize_capacity first init obj: 0x%x pkg: 0x%x\n",
                                object_p->object_id, object_p->package_id);
    object_p->topology_object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;

    /* When we size a device object we need to size using IRPs.
     */
    if (object_p->package_id == FBE_PACKAGE_ID_INVALID)
    {
        /* We are not using the edge ptr, so don't destroy it.
         */
        object_p->destroy_edge_ptr = FBE_FALSE;
        return fbe_rdgen_object_size_disk_object(object_p);
    }

    fbe_semaphore_init_named(&sem, "rdgen object init cap", 0, 1);
    /* initialize topology object type
     */
    packet_p = fbe_transport_allocate_packet();

    if (packet_p == NULL)
    {
        fbe_semaphore_destroy(&sem);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "fbe_rdgen_object_initialize_capacity cannot allocate packet\n");
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    fbe_sg_element_init(&sg, sizeof(fbe_block_transport_negotiate_t), &negotiate_block_size);
    negotiate_block_size.requested_block_size = FBE_BE_BYTES_PER_BLOCK;
    negotiate_block_size.block_size = FBE_BE_BYTES_PER_BLOCK;   /* Client block size */
    negotiate_block_size.requested_optimum_block_size = 1;
    negotiate_block_size.physical_block_size = 0;

    /* Setup the block operation in the packet.
     */
    if (object_p->package_id == FBE_PACKAGE_ID_PHYSICAL)
    {
        fbe_transport_initialize_packet(packet_p);
    }
    else
    {
        fbe_transport_initialize_sep_packet(packet_p);
    }

    payload_p = fbe_transport_get_payload_ex(packet_p);

    if (object_p->package_id == FBE_PACKAGE_ID_SEP_0)
    {
        /*try to get it (the function we are here now 'fbe_rdgen_object_initialize_capacity', may be called twice so we have to be careful
        and we can't just rely on the pointer to know if we got a vlaid one back because it might be initialized from before)*/
        status  = fbe_rdgen_object_get_edge(object_p, packet_p, &sem);
        if (status != FBE_STATUS_OK && FBE_STATUS_NO_OBJECT != status)
        {
            fbe_semaphore_destroy(&sem);
            fbe_transport_release_packet(packet_p);
            return status;
        }
        object_p->destroy_edge_ptr = FBE_FALSE;/*don't touch it because it is a real and not fake edge*/
    }

    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);
    fbe_payload_ex_set_sg_list(payload_p, &sg, 0);

    /* We want to complete in a different context since we are calling this syncronously. 
     * We would like to have this be asyncronous, but until that time, force it to complete on a different core. 
     * This only matters for cases where we hit an error on the edge and it gets scheduled 
     * to the transport thread on the same core where we are waiting. 
     */
    cpu_id = (fbe_get_cpu_id() == 0) ? 1 : 0;
    fbe_transport_set_cpu_id(packet_p, cpu_id);

    fbe_payload_block_build_negotiate_block_size(block_operation_p);

    /* Init the sg and negotiate structure.
     */
    fbe_transport_set_completion_function(packet_p, fbe_rdgen_object_init_completion, &sem);
    fbe_transport_set_address(packet_p,
                              object_p->package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_p->object_id);

    if ((object_p->package_id == FBE_PACKAGE_ID_SEP_0) &&
        (object_p->edge_ptr != NULL))
    {
        status = fbe_block_transport_send_functional_packet(object_p->edge_ptr, packet_p);
    }
    else
    {
        /* Increment the level since we are not sending on an edge.
         */
        status = fbe_payload_ex_increment_block_operation_level(payload_p);

        if (status != FBE_STATUS_OK)
        {
            fbe_semaphore_destroy(&sem);
            fbe_transport_release_packet(packet_p);
            return status;
        }
        status = fbe_topology_send_io_packet(packet_p);
    }
    fbe_semaphore_wait(&sem, NULL);

    status = fbe_transport_get_status_code(packet_p);

    if (status == FBE_STATUS_OK)
    {
        if (negotiate_block_size.optimum_block_alignment == 0)
        {
            object_p->stripe_size = FBE_RDGEN_DEFAULT_STRIPE_SIZE;
        }
        else
        {
            object_p->stripe_size = negotiate_block_size.optimum_block_alignment;
        }
        object_p->physical_block_size = negotiate_block_size.physical_block_size;
        object_p->optimum_block_size = negotiate_block_size.optimum_block_size;
        object_p->block_size = negotiate_block_size.block_size;

        /*if this is on SEP, and this is a LUN, we want to use the LUN edge directly.*/
        if (object_p->package_id == FBE_PACKAGE_ID_SEP_0)
        {
            /*try to get it (the function we are here now 'fbe_rdgen_object_initialize_capacity', may be called twice so we have to be careful
            and we can't just rely on the pointer to know if we got a vlaid one back because it might be initialized from before)*/
            status  = fbe_rdgen_object_get_edge(object_p, packet_p, &sem);
            if (status != FBE_STATUS_OK && FBE_STATUS_NO_OBJECT != status)
            {
                fbe_semaphore_destroy(&sem);
                fbe_transport_release_packet(packet_p);
                return status;
            }
            else 
            {
                /* if it is a lun or a raid group , always use 520 */
                if(object_p->topology_object_type == FBE_TOPOLOGY_OBJECT_TYPE_LUN || 
                   object_p->topology_object_type == FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP )
                {
                    object_p->physical_block_size = FBE_BE_BYTES_PER_BLOCK;
                    object_p->optimum_block_size = 1 ;
                    object_p->block_size = FBE_BE_BYTES_PER_BLOCK;
                    negotiate_block_size.physical_block_size = FBE_BE_BYTES_PER_BLOCK;
                }

                /*did we get some block edge data here? if so, we can use it for capacity and just go out, we are all set here*/
                if (FBE_STATUS_NO_OBJECT != status)
                {
                    object_p->capacity = object_p->edge_ptr->capacity;

                    /*sice this is a user LUN, we need to reduce the capacity a little bit so we don't go over the 
                    area at the end where sp cache stores it's zero bit maps*/
                    if (object_p->topology_object_type == FBE_TOPOLOGY_OBJECT_TYPE_LUN)
                    {
                        if (fbe_rdgen_object_is_lun_destroy_registered() == FBE_FALSE)
                        {
                            fbe_rdgen_object_register_lun_destroy_notifications();
                        }
                        fbe_lun_calculate_cache_zero_bit_map_size_to_remove(object_p->capacity, &cache_zero_bit_map_size);
                    }
                    object_p->capacity -= cache_zero_bit_map_size;
                    object_p->destroy_edge_ptr = FBE_FALSE;/*don't touch it because it is a real and not fake edge*/
                    fbe_semaphore_destroy(&sem);
                    fbe_transport_release_packet(packet_p);

                    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "fbe_rdgen_object_initialize_capacity: obj %p use edge %p BVD->LUN 0x%X\n", 
                                            object_p, object_p->edge_ptr, object_p->edge_ptr->base_edge.server_id);

                    return status;
                }
            }
        }
        
        /*what if the RDGEN object (who's ID does not change) used to run IO to a LUN directly via the BVD edge and now
        the topology is using this object ID to a different object ? In this case, let's just forget the lun edge and 
        re-create a fake edge which we use for all the other objects*/
        if ((object_p->edge_ptr != NULL) && (object_p->edge_ptr->base_edge.server_id != object_p->object_id))
        {
            if (object_p->destroy_edge_ptr) {
                fbe_memory_ex_release(object_p->edge_ptr);
            }

            object_p->edge_ptr = NULL;

        }

        /*let's see if we got called before or this is the first time we are called*/
        if (object_p->edge_ptr == NULL)
        {
            object_p->edge_ptr = (fbe_block_edge_t *)fbe_memory_ex_allocate(sizeof(fbe_block_edge_t));
            fbe_zero_memory(object_p->edge_ptr, sizeof (fbe_block_edge_t));
            object_p->destroy_edge_ptr = FBE_TRUE;
        }

        fbe_block_transport_set_transport_id(object_p->edge_ptr);
        fbe_block_transport_set_server_id(object_p->edge_ptr, object_p->object_id);
        fbe_block_transport_set_client_id(object_p->edge_ptr, 0);
        fbe_block_transport_set_client_index(object_p->edge_ptr, 0);
        fbe_block_transport_edge_set_capacity(object_p->edge_ptr, object_p->capacity);
        fbe_block_transport_edge_set_offset(object_p->edge_ptr, 0);
        fbe_block_transport_edge_set_server_package_id(object_p->edge_ptr, object_p->package_id);
        object_p->edge_ptr->base_edge.path_state = FBE_PATH_STATE_ENABLED;
        
        switch (negotiate_block_size.physical_block_size)
        {
            /*! @note Currently only 520 and 4160 block sizes are supported.
             */
            case 520:
                fbe_block_transport_edge_set_geometry(object_p->edge_ptr, FBE_BLOCK_EDGE_GEOMETRY_520_520);
                break;
            case 4160:
                fbe_block_transport_edge_set_geometry(object_p->edge_ptr, FBE_BLOCK_EDGE_GEOMETRY_4160_4160);
                break;
            default:
                fbe_block_transport_edge_set_geometry(object_p->edge_ptr, FBE_BLOCK_EDGE_GEOMETRY_520_520);

                object_p->block_size = object_p->physical_block_size = 520;
                object_p->optimum_block_size = 1;
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "fbe_rdgen_object_initialize_capacity: init of capacity for objid: 0x%x fail with status 0x%x\n", 
                                object_p->object_id, status);
                break;
        };

        /* Send get capacity control code to LUN object. 
         */
        fbe_transport_destroy_packet(packet_p);
        fbe_transport_initialize_packet(packet_p);

        if (object_p->package_id != FBE_PACKAGE_ID_PHYSICAL)
        {
            payload_p = fbe_transport_get_payload_ex(packet_p);
            control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
            base_config_control_get_capacity.capacity = FBE_LBA_INVALID;
            fbe_payload_control_build_operation(control_operation_p,
                                                FBE_BASE_CONFIG_CONTROL_CODE_GET_CAPACITY,
                                                &base_config_control_get_capacity,
                                                sizeof(fbe_base_config_control_get_capacity_t));
            fbe_transport_set_completion_function(packet_p, fbe_rdgen_object_init_completion, &sem);
            fbe_transport_set_address(packet_p,
                                      object_p->package_id,
                                      FBE_SERVICE_ID_TOPOLOGY,
                                      FBE_CLASS_ID_INVALID,
                                      object_p->object_id);
            status = fbe_service_manager_send_control_packet(packet_p);
            fbe_semaphore_wait(&sem, NULL);
            if((status != FBE_STATUS_OK) || (base_config_control_get_capacity.capacity == FBE_LBA_INVALID)) {
                fbe_semaphore_destroy(&sem);
                fbe_transport_release_packet(packet_p);
                return status;
            }
    
            /* Save the capacity.
             */
            object_p->capacity = base_config_control_get_capacity.capacity;
        }
        else
        {
            /* Physical package uses negotiate for block count.
             */
            object_p->capacity = negotiate_block_size.block_count;
        }
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "obj: 0x%x %d/%d/%d capacity: 0x%llx\n", 
                                object_p->object_id, object_p->optimum_block_size, 
                                object_p->physical_block_size, object_p->block_size,
                                object_p->capacity);
        

    }
    else
    {
        /* Setup some default values
            Init the fake edge info.
            * At this time we do not want to introduce the complexity of attaching and 
            * detaching the edge.
            */

        if (object_p->edge_ptr == NULL) {
            object_p->edge_ptr = (fbe_block_edge_t *)fbe_memory_ex_allocate(sizeof(fbe_block_edge_t));
            object_p->destroy_edge_ptr = FBE_TRUE;
            fbe_zero_memory(object_p->edge_ptr, sizeof (fbe_block_edge_t));
            fbe_block_transport_set_transport_id(object_p->edge_ptr);
            fbe_block_transport_set_server_id(object_p->edge_ptr, object_p->object_id);
            fbe_block_transport_set_client_id(object_p->edge_ptr, 0);
            fbe_block_transport_set_client_index(object_p->edge_ptr, 0);
            fbe_block_transport_edge_set_capacity(object_p->edge_ptr, 0);
            fbe_block_transport_edge_set_offset(object_p->edge_ptr, 0);
            fbe_block_transport_edge_set_server_package_id(object_p->edge_ptr, object_p->package_id);
            object_p->edge_ptr->base_edge.path_state = FBE_PATH_STATE_ENABLED;
            fbe_block_transport_edge_set_geometry(object_p->edge_ptr, FBE_BLOCK_EDGE_GEOMETRY_520_520);
        }
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "fbe_rdgen_object_initialize_capacity: init of capacity for objid: 0x%x fail with stat 0x%x\n", 
                                object_p->object_id, status);
    }
    fbe_semaphore_destroy(&sem);
    fbe_transport_release_packet(packet_p);
    return status;
}
/******************************************
 * end fbe_rdgen_object_initialize_capacity()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_object_stop_io()
 ****************************************************************
 * @brief
 *  This function stops all I/O on an rdgen object.
 *
 * @param object_p - Object to stop I/O on.
 *
 * @return None.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_object_stop_io(fbe_rdgen_object_t *object_p)
{
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_rdgen_ts_t *ts_p = NULL;
    fbe_u32_t loop_count = 0;

    /* Prevent things from changing while we are processing the queue.
     */
    fbe_rdgen_object_lock(object_p);
    
    /* Simply mark each item aborted, and then wait for it to finish.
     */
    while (!fbe_queue_is_empty(&object_p->active_ts_head))
    {
        /* First mark everything
         */
        queue_element_p = fbe_queue_front(&object_p->active_ts_head);
        while (queue_element_p != NULL)
        {
            ts_p = (fbe_rdgen_ts_t *)queue_element_p;
            fbe_rdgen_ts_mark_aborted(ts_p);
            queue_element_p = fbe_queue_next(&object_p->active_ts_head, queue_element_p);
        } /* while the queue element is not null. */

        /* Unlock while we wait for things to halt.
         */
        fbe_rdgen_object_unlock(object_p);
        
        /* Re-lock so we can check the queue.
         */
        fbe_rdgen_object_lock(object_p);
        loop_count++;
    } /* while the queue is not empty.  */

    /* Unlock since we are done.
     */
    fbe_rdgen_object_unlock(object_p);
    return;
}
/******************************************
 * end fbe_rdgen_object_stop_io()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_object_destroy()
 ****************************************************************
 * @brief
 *  This function destroys an rdgen object.
 *
 * @param object_p - Object to initialize.
 *
 * @return None.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_rdgen_object_destroy(fbe_rdgen_object_t *object_p)
{
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s: destroy %p\n", 
                                __FUNCTION__, object_p);

    if (!fbe_rdgen_object_validate_magic(object_p))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: magic number is 0x%llx\n", __FUNCTION__, 
                                fbe_rdgen_object_get_magic(object_p));
    }
    fbe_rdgen_object_lock(object_p);
    /*! @todo theoretically we should abort all outstanding threads here.
     */
    fbe_rdgen_object_unlock(object_p);

    fbe_spinlock_destroy(&object_p->lock);
    fbe_queue_destroy(&object_p->active_ts_head);

    if (object_p->destroy_edge_ptr && object_p->edge_ptr)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                object_p->object_id,
                                "%s: object: %p edge: %p\n", 
                                __FUNCTION__, object_p, object_p->edge_ptr );

        fbe_memory_ex_release(object_p->edge_ptr);
    }
    else
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                object_p->object_id,
                                "%s: object: %p edge %p belongs to LUN\n", 
                                __FUNCTION__, object_p, object_p->edge_ptr);
    }
    if (object_p->playback_records_memory_p != NULL){
        fbe_memory_native_release(object_p->playback_records_memory_p);
        object_p->playback_records_memory_p = NULL;
        object_p->playback_records_p = NULL;
    }
    if (object_p->device_p != NULL)
    {
        fbe_rdgen_object_close(object_p);
    }

    fbe_rdgen_free_memory(object_p, sizeof(fbe_rdgen_object_t));
    fbe_rdgen_inc_obj_freed();

    
    return;
}
/******************************************
 * end fbe_rdgen_object_destroy()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_object_get_edge()
 ****************************************************************
 * @brief
 *  This function get's the edge data of the edge between LUN and BVD so we can use it directly
 *
 * @param object_p - the rdgen object we work on
 * @param packet_p - packet to send to get info.
 * @param sem - semaphore for synch.
 *
 * @return None.
 *
 * @author
 *  9/8/2011 - Created. Shay Harel
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_object_get_edge(fbe_rdgen_object_t * object_p, fbe_packet_t * packet_p, fbe_semaphore_t *sem)
{
    fbe_payload_ex_t *                      sep_payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_topology_mgmt_get_object_type_t         topology_mgmt_get_object_type;
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_control_get_bvd_id_t           get_bvd_id;
    fbe_bvd_interface_get_downstream_attr_t     get_bvd_edge;
    fbe_cpu_id_t cpu_id;

    fbe_transport_destroy_packet(packet_p);
    fbe_transport_initialize_packet(packet_p);

    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet_p, cpu_id);

    /*get the type of the object*/
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    topology_mgmt_get_object_type.object_id = object_p->object_id;
    fbe_payload_control_build_operation(control_operation, 
                                        FBE_TOPOLOGY_CONTROL_CODE_GET_OBJECT_TYPE,
                                        &topology_mgmt_get_object_type,
                                        sizeof(topology_mgmt_get_object_type));
    
    fbe_transport_set_completion_function(packet_p, fbe_rdgen_object_init_completion, sem);
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID);

    status = fbe_service_manager_send_control_packet(packet_p);
    fbe_semaphore_wait(sem, NULL);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation);
    if(status != FBE_STATUS_OK) {
        return status;
    }

    object_p->topology_object_type = topology_mgmt_get_object_type.topology_object_type;
    if (topology_mgmt_get_object_type.topology_object_type != FBE_TOPOLOGY_OBJECT_TYPE_LUN){
        return FBE_STATUS_NO_OBJECT;/*our work is done here, nothing to do and a fake edege should be created*/
    }

    /*let's get the edge information of the edge on top of this LUN, we'll do it via the BVD object
    since the LUN itself has the block transport server on top of it which can have in theory many edges,

    1) First, we'll get the object ID of the BD object
    2) Then we'll get the edge information of this LUN from BVD
    */

    fbe_transport_destroy_packet(packet_p);
    fbe_transport_initialize_packet(packet_p);

    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet_p, cpu_id);

    /*get the BVD ID*/
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_TOPOLOGY_CONTROL_CODE_GET_BVD_OBJECT_ID,
                                        &get_bvd_id,
                                        sizeof(fbe_topology_control_get_bvd_id_t));
    
    fbe_transport_set_completion_function(packet_p, fbe_rdgen_object_init_completion, sem);
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID);

    status = fbe_service_manager_send_control_packet(packet_p);
    fbe_semaphore_wait(sem, NULL);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation);
    if(status != FBE_STATUS_OK) {
        return status;
    }

    /*now use it to get the edge info*/
    get_bvd_edge.lun_object_id = object_p->object_id;
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    fbe_payload_control_build_operation(control_operation, 
                                        FBE_BVD_INTERFACE_CONTROL_CODE_GET_ATTR,
                                        &get_bvd_edge,
                                        sizeof(fbe_bvd_interface_get_downstream_attr_t));
    
    fbe_transport_set_completion_function(packet_p, fbe_rdgen_object_init_completion, sem);
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              get_bvd_id.bvd_object_id);

    status = fbe_service_manager_send_control_packet(packet_p);
    fbe_semaphore_wait(sem, NULL);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation);
    if(status != FBE_STATUS_OK) {
        return status;
    }

    /*was this edge previously allocated as a dummy edge and now this object points to a LUN ?
    If so, let's just delete the memory associated with this edge before using the BVD <-> LUN edge memory*/
    if (object_p->edge_ptr != NULL && object_p->destroy_edge_ptr) {
        fbe_memory_ex_release(object_p->edge_ptr);

    }
    /*and copy it for our use*/
    object_p->edge_ptr = (fbe_block_edge_t *)(fbe_ptrhld_t)get_bvd_edge.opaque_edge_ptr;
    object_p->destroy_edge_ptr = FBE_FALSE; /* Do not free this meemory since it is not ours */

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_object_get_edge()
 ******************************************/

/*!***************************************************************************
 *          fbe_rdgen_object_is_lun_destroy_registered() 
 *****************************************************************************
 *
 *  @brief  Determine if we need to register or not.
 *
 *  @return bool
 * 
 *  @author
 *  01/31/2013  Ron Proulx  - Created.
 *    
 *****************************************************************************/
fbe_bool_t fbe_rdgen_object_is_lun_destroy_registered(void)
{
    if (fbe_rdgen_object_destroy_notification_context.b_is_initialized == FBE_TRUE)
    {
        return FBE_TRUE;
    }
    
    return FBE_FALSE;
}
/***************************************************
 * end fbe_rdgen_object_is_lun_destroy_registered()
 ***************************************************/

/*!***************************************************************************
 *          fbe_rdgen_object_lun_destroy_notification_callback
 *****************************************************************************
 *
 *  @brief  This routine is the callback when a SEP lun is destroyed.  This
 *          routine will walk the object queue and if any objects under test
 *          are now destroyed, we will set the `b_is_object_destroyed' flag
 *          in that object.
 *
 *  @param  
 *
 *  @return status
 *
 *  @author
 *  01/31/2013  Ron Proulx  - Created.
 *    
 *****************************************************************************/
static fbe_status_t fbe_rdgen_object_lun_destroy_notification_callback(fbe_object_id_t object_id, 
                                                                       fbe_notification_info_t notification_info,
                                                                       fbe_notification_context_t context)
{
    fbe_char_t                          name = 0;
    fbe_rdgen_object_t                 *object_p = NULL;

    /* Prevent things from changing while we are processing the queue.
     */
    fbe_rdgen_lock();
    
    /* Simply drain the queue of objects, destroying each item Until there are no more
     * items. 
     */
    object_p = fbe_rdgen_find_object(object_id, &name);
    if (object_p != NULL)
    {
        /* While the lock is held update the `b_is_object_destroyed'
         */
        fbe_rdgen_object_lock(object_p);
        object_p->b_is_object_under_test_destroyed = FBE_TRUE;
        fbe_rdgen_object_unlock(object_p);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                object_id,
                                "rdgen: lun: 0x%x (%s) has been destroyed\n",
                                object_id, object_p->device_name);
    }

    /* Unlock since we are done.
     */
    fbe_rdgen_unlock();

    /* Always success
     */
    return FBE_STATUS_OK;
}
/*********************************************************** 
 * end fbe_rdgen_object_lun_destroy_notification_callback()
 ***********************************************************/

/*!***************************************************************************
 *          fbe_rdgen_object_register_lun_destroy_notifications() 
 *****************************************************************************
 *
 *  @brief  Register for LUN destroy notifications so that we refresh our edge
 *          information if a LUN is destroyed.
 *
 *  @return status
 * 
 *  @note   Do NOT call this method before SEP is loaded.  This method should
 *          only be called when an LUN rdgen object is created.  Thus SEP will
 *          be loaded at that time.
 * 
 *  @author
 *  01/31/2013  Ron Proulx  - Created.
 *    
 *****************************************************************************/
fbe_status_t fbe_rdgen_object_register_lun_destroy_notifications(void)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_notification_element_t *notification_element_p = NULL;
    fbe_packet_t               *packet_p = NULL;
    fbe_payload_ex_t           *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    
    /* Only regiser (1) time
     */
    fbe_rdgen_lock();
    if (fbe_rdgen_object_destroy_notification_context.b_is_initialized == FBE_FALSE)
    {
        /* Register for LUN destroy notifications.
         */
        fbe_rdgen_object_destroy_notification_context.b_is_initialized = FBE_TRUE;
        fbe_rdgen_unlock();

        notification_element_p = &fbe_rdgen_object_destroy_notification_context.notification_element;
        notification_element_p->notification_function = fbe_rdgen_object_lun_destroy_notification_callback;
        notification_element_p->notification_context = &fbe_rdgen_object_destroy_notification_context;
        notification_element_p->notification_type = FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY;
        notification_element_p->object_type = FBE_TOPOLOGY_OBJECT_TYPE_LUN;
    
        /* Allocate a packet and send it.
         */
        packet_p = fbe_transport_allocate_packet();
        if (packet_p == NULL) 
        {
            fbe_rdgen_object_destroy_notification_context.b_is_initialized = FBE_FALSE;
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_transport_initialize_packet(packet_p);
        payload_p = fbe_transport_get_payload_ex(packet_p);
        control_p = fbe_payload_ex_allocate_control_operation(payload_p);
        fbe_payload_control_build_operation(control_p,
                                            FBE_NOTIFICATION_CONTROL_CODE_REGISTER,
                                            notification_element_p,
                                            sizeof(fbe_notification_element_t));

        /* Set packet address.
         */
        fbe_transport_set_address(packet_p,
                                  FBE_PACKAGE_ID_SEP_0,
                                  FBE_SERVICE_ID_NOTIFICATION,
                                  FBE_CLASS_ID_INVALID,
                                  FBE_OBJECT_ID_INVALID);
        fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
        fbe_service_manager_send_control_packet(packet_p);

        /* Wait for completion.
         * The packet is always completed so the status above need not be checked.
         */
        fbe_transport_wait_completion(packet_p);

        /* Save status before we release packet.
         */
        status = fbe_transport_get_status_code(packet_p);
        fbe_transport_release_packet(packet_p);

        /* If this failed trace an error and mark `not registered'
         */
        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_object_destroy_notification_context.b_is_initialized = FBE_FALSE;
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: failed - status: 0x%x\n", __FUNCTION__, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Trace the registration
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: success\n", __FUNCTION__);
    }
    else
    {
        /* Else already registered*/
        fbe_rdgen_unlock();
    }

    /* Return the completion status.
     */
    return status;
}
/****************************************************************
 * end fbe_rdgen_object_register_lun_destroy_notifications()
 ****************************************************************/

/*!***************************************************************************
 *          fbe_rdgen_object_unregister_lun_destroy_notifications() 
 *****************************************************************************
 *
 *  @brief  Un-register for LUN destroy notifications so that we refresh our edge
 *          information if a LUN is destroyed.
 *
 *  @return status
 * 
 *  @note   Do NOT call this method before SEP is loaded.  This method should
 *          only be called when an LUN rdgen object is created.  Thus SEP will
 *          be loaded at that time.
 * 
 *  @note   This method is currently called holding the rdgen lock.
 * 
 *  @author
 *  01/31/2013  Ron Proulx  - Created.
 *    
 *****************************************************************************/
fbe_status_t fbe_rdgen_object_unregister_lun_destroy_notifications(void)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_notification_element_t *notification_element_p = NULL;
    fbe_packet_t               *packet_p = NULL;
    fbe_payload_ex_t           *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    
    /* Only register (1) time
     */
    if (fbe_rdgen_object_destroy_notification_context.b_is_initialized == FBE_TRUE)
    {
        /* Unregister for LUN destroy notifications.
         */
        fbe_rdgen_object_destroy_notification_context.b_is_initialized = FBE_FALSE;
        notification_element_p = &fbe_rdgen_object_destroy_notification_context.notification_element;

        /* If we didn't register properly something is wrong.
         */
        if ((notification_element_p->notification_context != (fbe_notification_context_t)&fbe_rdgen_object_destroy_notification_context) ||
            (notification_element_p->notification_type != FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY)                                 ||
            (notification_element_p->object_type != FBE_TOPOLOGY_OBJECT_TYPE_LUN)                                                           )
        {
            /* We can't un-register, that's bad
             */
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: notification element is bad\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Allocate a packet and send it.
         */
        packet_p = fbe_transport_allocate_packet();
        if (packet_p == NULL) 
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_transport_initialize_packet(packet_p);
        payload_p = fbe_transport_get_payload_ex(packet_p);
        control_p = fbe_payload_ex_allocate_control_operation(payload_p);
        fbe_payload_control_build_operation(control_p,
                                            FBE_NOTIFICATION_CONTROL_CODE_UNREGISTER,
                                            notification_element_p,
                                            sizeof(fbe_notification_element_t));

        /* Set packet address.
         */
        fbe_transport_set_address(packet_p,
                                  FBE_PACKAGE_ID_SEP_0,
                                  FBE_SERVICE_ID_NOTIFICATION,
                                  FBE_CLASS_ID_INVALID,
                                  FBE_OBJECT_ID_INVALID);
        fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
        fbe_service_manager_send_control_packet(packet_p);

        /* Wait for completion.
         * The packet is always completed so the status above need not be checked.
         */
        fbe_transport_wait_completion(packet_p);

        /* Save status before we release packet.
         */
        status = fbe_transport_get_status_code(packet_p);
        fbe_transport_release_packet(packet_p);

        /* If this failed trace an error and mark `not registered'
         */
        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: failed - status: 0x%x\n", __FUNCTION__, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Trace the un-registration
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: success\n", __FUNCTION__);
    }

    /* Return the completion status.
     */
    return status;
}
/****************************************************************
 * end fbe_rdgen_object_unregister_lun_destroy_notifications()
 ****************************************************************/
/*!***************************************************************
 * fbe_rdgen_object_thread_queue_element_to_ts_ptr()
 ****************************************************************
 * @brief
 *  Convert from the fbe_rdgen_object_t->thread_queue_element
 *  back to the fbe_rdgen_object_t *.
 * 
 *  struct fbe_rdgen_object_t { <-- We want to return the ptr to here.
 *  ...
 *    thread_queue_element   <-- Ptr to here is passed in.
 *  ...
 *  };
 *
 * @param thread_queue_element_p - This is the thread queue element that we wish to
 *                          get the ts ptr for.
 *
 * @return fbe_rdgen_ts_t - The ts that has the input thread queue element.
 *
 * @author
 *  7/30/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_object_t * 
fbe_rdgen_object_thread_queue_element_to_ts_ptr(fbe_queue_element_t * thread_queue_element_p)
{
    /* We're converting an address to a queue element into an address to the containing rdgen object.
     * In order to do this we need to subtract the offset to the queue element from the address of the queue element.
     */
    fbe_rdgen_object_t * ts_p;
    ts_p = (fbe_rdgen_object_t  *)((fbe_u8_t *)thread_queue_element_p - 
                               (fbe_u8_t *)(&((fbe_rdgen_object_t  *)0)->thread_queue_element));
    return ts_p;
}
/**************************************
 * end fbe_rdgen_object_thread_queue_element_to_ts_ptr()
 **************************************/

/*!***************************************************************
 * fbe_rdgen_object_process_queue_element_to_ts_ptr()
 ****************************************************************
 * @brief
 *  Convert from the fbe_rdgen_object_t->thread_queue_element
 *  back to the fbe_rdgen_object_t *.
 * 
 *  struct fbe_rdgen_object_t { <-- We want to return the ptr to here.
 *  ...
 *    thread_queue_element   <-- Ptr to here is passed in.
 *  ...
 *  };
 *
 * @param thread_queue_element_p - This is the thread queue element that we wish to
 *                          get the ts ptr for.
 *
 * @return fbe_rdgen_ts_t - The ts that has the input thread queue element.
 *
 * @author
 *  8/30/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_object_t * 
fbe_rdgen_object_process_queue_element_to_ts_ptr(fbe_queue_element_t * thread_queue_element_p)
{
    /* We're converting an address to a queue element into an address to the containing rdgen object.
     * In order to do this we need to subtract the offset to the queue element from the address of the queue element.
     */
    fbe_rdgen_object_t * ts_p;
    ts_p = (fbe_rdgen_object_t  *)((fbe_u8_t *)thread_queue_element_p - 
                               (fbe_u8_t *)(&((fbe_rdgen_object_t  *)0)->process_queue_element));
    return ts_p;
}
/**************************************
 * end fbe_rdgen_object_process_queue_element_to_ts_ptr()
 **************************************/

/*!**************************************************************
 * fbe_rdgen_object_enqueue_to_thread()
 ****************************************************************
 * @brief
 *  Cause this rdgen ts to get scheduled on a thread.
 *
 * @param object_p - Current object.
 * 
 * @notes Expects to be called with the object lock held.
 *
 * @return fbe_status_t - True if enqueue worked, False otherwise.
 *
 * @author
 *  7/30/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_object_enqueue_to_thread(fbe_rdgen_object_t *object_p)
{
    fbe_status_t status;

    status = rdgen_service_send_rdgen_object_to_thread(object_p);

    /* If enqueuing to a thread failed, something is very wrong. It is possible we are being destroyed.
     */
    if (status != FBE_STATUS_OK){
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: cannot enqueue to thread, status: 0x%x\n", __FUNCTION__, status);
    }
    return status;
}
/******************************************
 * end fbe_rdgen_object_enqueue_to_thread()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_object_set_file()
 ****************************************************************
 * @brief
 *  Set the file name
 *
 * @param object_p - Current object.
 *
 * @return fbe_status_t - True if enqueue worked, False otherwise.
 *
 * @author
 *  7/30/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_object_set_file(fbe_rdgen_object_t *object_p, fbe_char_t *file_p)
{
    fbe_u32_t length = (fbe_u32_t)strlen(file_p);
    csx_p_strncpy(&object_p->file_name[0], file_p, FBE_MIN(length-4, FBE_RDGEN_DEVICE_NAME_CHARS));
    sprintf(&object_p->file_name[0], "%s_%d.rdg", &object_p->file_name[0], object_p->object_id); 
}
/******************************************
 * end fbe_rdgen_object_set_file()
 ******************************************/
/******************************
 * end file fbe_rdgen_object.c
 ******************************/


