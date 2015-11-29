/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the provision_drive object.
 *  This includes the create and destroy methods for the provision_drive, as
 *  well as the event entry point, load and unload methods.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_provision_drive.h"
#include "base_object_private.h"
#include "fbe_provision_drive_private.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe_raid_library.h"
#include "fbe_service_manager.h"
#include "fbe_database.h" 
#include "fbe/fbe_time.h" 
#include "fbe_private_space_layout.h"

#define MAX_REMOVE_PVD_WAIT_FOR_DOWNSTREAM_EDGE_SECONDS 120

/* Export class methods.
 */
fbe_class_methods_t fbe_provision_drive_class_methods = {FBE_CLASS_ID_PROVISION_DRIVE,
                                                         fbe_provision_drive_load,
                                                         fbe_provision_drive_unload,
                                                         fbe_provision_drive_create_object,
                                                         fbe_provision_drive_destroy_object,
                                                         fbe_provision_drive_control_entry,
                                                         fbe_provision_drive_event_entry,
                                                         fbe_provision_drive_io_entry,
                                                         fbe_provision_drive_monitor_entry};

fbe_block_transport_const_t fbe_provision_drive_block_transport_const = {fbe_provision_drive_block_transport_entry,
																		 fbe_base_config_process_block_transport_event,
																		 fbe_provision_drive_io_entry,
                                                                         NULL, NULL};

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t fbe_provision_drive_init(fbe_provision_drive_t * const provision_drive_p);

static fbe_status_t fbe_provision_drive_set_priorities(fbe_provision_drive_t * provision_drive, fbe_provision_drive_set_priorities_t set_priorities);

static fbe_status_t fbe_provision_drive_get_physical_drive_port_number_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_get_physical_drive_encl_number_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_get_physical_drive_slot_number_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_bool_t   fbe_provision_drive_time_stamp_reached_threshold(fbe_provision_drive_t * provision_drive_p, fbe_system_time_t time_stamp,fbe_system_time_threshold_info_t *system_time_threshold);
static fbe_status_t fbe_provision_drive_init_pvd_invalid_pattern_bit_bucket(void);
static fbe_status_t fbe_provision_drive_process_drive_fault (fbe_provision_drive_t *provision_drive_p, fbe_path_attr_t path_attr, 
                                                             fbe_base_config_downstream_health_state_t downstream_health_state);


static fbe_u64_t fbe_provision_drive_zeroing_cpu_rate = 10;
static fbe_u64_t fbe_provision_drive_sniff_cpu_rate = 10;
static fbe_u64_t fbe_provision_drive_verify_invalidate_cpu_rate = 100;
static fbe_bool_t fbe_provision_drive_enable_slf = FBE_TRUE;
static fbe_u32_t fbe_provision_drive_zeroing_speed = 120;
static fbe_u32_t fbe_provision_drive_sniff_speed = 10;


/*!***************************************************************
 * fbe_provision_drive_load()
 ****************************************************************
 * @brief
 *  This function is called to construct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_provision_drive_load(void)
{
    fbe_status_t status;
    
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_provision_drive_t) < FBE_MEMORY_CHUNK_SIZE);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_provision_drive_nonpaged_metadata_t) < FBE_METADATA_NONPAGED_MAX_SIZE);
    /* Paged metadata MUST be aligned */ 
    FBE_ASSERT_AT_COMPILE_TIME((FBE_METADATA_BLOCK_DATA_SIZE % sizeof(fbe_provision_drive_paged_metadata_t)) == 0);

    status = fbe_provision_drive_init_pvd_invalid_pattern_bit_bucket();
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    status = fbe_provision_drive_monitor_load_verify();
    fbe_provision_drive_class_load_registry_keys();
    
    return status;
}
/* end fbe_provision_drive_load() */

/*!***************************************************************
 * fbe_provision_drive_unload()
 ****************************************************************
 * @brief
 *  This function is called to tear down any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_provision_drive_unload(void)
{
    /* Destroy any global data.
     */
    return FBE_STATUS_OK;
}
/* end fbe_provision_drive_unload() */
/*!***************************************************************
 * fbe_provision_drive_init_pvd_invalid_pattern_bit_bucket()
 ****************************************************************
 * @brief
 *  This function is called to initialize the bit bucket with
 *  invalid pattern.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  03/31/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t 
fbe_provision_drive_init_pvd_invalid_pattern_bit_bucket(void)
{
    fbe_status_t status;
    fbe_block_count_t       invalid_pattern_bit_bucket_size_in_blocks;
    fbe_u32_t       invalid_pattern_bucket_size_in_bytes;
    fbe_u8_t *      invalid_pattern_bit_bucket_p = NULL;
    fbe_sg_element_t sg_list[2];

    /* get the zero bit bucket address and its size before we plant the sgl */
    status = fbe_memory_get_invalid_pattern_bit_bucket(&invalid_pattern_bit_bucket_p, &invalid_pattern_bit_bucket_size_in_blocks);
    if((status != FBE_STATUS_OK) || (invalid_pattern_bit_bucket_p == NULL))
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* convert the zero bit bucket size in bytes. */
    invalid_pattern_bucket_size_in_bytes = (fbe_u32_t) invalid_pattern_bit_bucket_size_in_blocks * FBE_BE_BYTES_PER_BLOCK;

    fbe_sg_element_init( &sg_list[0], invalid_pattern_bucket_size_in_bytes, invalid_pattern_bit_bucket_p );
    fbe_sg_element_terminate( &sg_list[1] );

    // set-up invalid pattern in i/o buffer
    fbe_xor_lib_fill_invalid_sectors( &sg_list[0], 0, invalid_pattern_bit_bucket_size_in_blocks, 0, 
                                      XORLIB_SECTOR_INVALID_REASON_PVD_METADATA_INVALID,
                                      XORLIB_SECTOR_INVALID_WHO_PVD);
    return FBE_STATUS_OK;
}
/*!***************************************************************
 * fbe_provision_drive_create_object()
 ****************************************************************
 * @brief
 *  This function is called to create an instance of this class.
 *
 * @param packet_p - The packet used to construct the object.  
 * @param object_handle - The object being created.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_provision_drive_create_object(fbe_packet_t * packet_p, fbe_object_handle_t * object_handle)
{
    fbe_provision_drive_t * provision_drive_p = NULL;
    fbe_status_t status;

    /* Call the base class create function. */
    status = fbe_base_config_create_object(packet_p, object_handle);    
    if(status != FBE_STATUS_OK){
        return status;
    }

    provision_drive_p = (fbe_provision_drive_t *)fbe_base_handle_to_pointer(*object_handle);

    /* Set class id.*/
    fbe_base_object_set_class_id((fbe_base_object_t *) provision_drive_p, FBE_CLASS_ID_PROVISION_DRIVE);

	fbe_base_config_send_specialize_notification((fbe_base_config_t *) provision_drive_p);

    /* Simply call our static init function to init members that have no
     * dependencies. 
     */
    status = fbe_provision_drive_init(provision_drive_p);

    /* Force verify to make sure the object is properly initialized before scheduling it.
     * This avoids race condition between scheduler thread and usurper command trying to 
     * initialize the object rotary preset at the same time.
     */
    fbe_lifecycle_verify((fbe_lifecycle_const_t *)&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p);

	/* Enable lifecycle tracing if requested */
	status = fbe_base_object_enable_lifecycle_debug_trace((fbe_base_object_t *)provision_drive_p);

    return status;
}
/* end fbe_provision_drive_create_object() */

/*!***************************************************************
 * fbe_provision_drive_init()
 ****************************************************************
 * @brief
 *  This function initializes the provision_drive object.
 *
 *  Note that everything that is created here must be
 *  destroyed within fbe_provision_drive_destroy_object().
 *
 * @param provision_drive_p - The provision_drive object.
 *
 * @return
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  05/20/2009 - Created. Dhaval Patel
 *
 ****************************************************************/
static fbe_status_t 
fbe_provision_drive_init(fbe_provision_drive_t * const provision_drive_p)
{
    fbe_status_t status = FBE_STATUS_OK;

	fbe_provision_drive_set_priorities_t	set_priorities;

	/*init base*/
   /* fbe_base_config_init((fbe_base_config_t *)provision_drive_p); */

    /* Initialize our members. */

    /* For now we just hard code the width to 1.
     * We also immediately set the configured bit. 
     */
    fbe_base_config_set_width((fbe_base_config_t *)provision_drive_p, 1);
    fbe_base_config_set_block_transport_const((fbe_base_config_t *) provision_drive_p,
                                              &fbe_provision_drive_block_transport_const);
    fbe_base_edge_init(&provision_drive_p->block_edge.base_edge);
    fbe_base_config_set_block_edge_ptr((fbe_base_config_t *)provision_drive_p, &provision_drive_p->block_edge);
    fbe_base_config_set_outstanding_io_max((fbe_base_config_t *) provision_drive_p, 0);
	fbe_base_config_set_stack_limit((fbe_base_config_t *) provision_drive_p);

	/*set the power saving to be enabled by default. Once the system power saving is enabled,
	 we will go into power save in FBE_BASE_CONFIG_DEFUALT_IDLE_TIME_IN_SECONDS minutes*/
	base_config_enable_object_power_save((fbe_base_config_t *) provision_drive_p);

    /* Set the provision drive configuration type as unknown by default. */
    fbe_provision_drive_set_config_type(provision_drive_p, FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID);

    /* Set the default pvd debug flag */
    fbe_provision_drive_set_debug_flag(provision_drive_p, FBE_PROVISION_DRIVE_DEBUG_FLAG_NONE);

    /* Set the default pvd flags */
    fbe_provision_drive_init_flags(provision_drive_p);

    /* Set the configured physical block size */
    fbe_provision_drive_set_configured_physical_block_size (provision_drive_p, 
            FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID);

    /* Set the default offset */
    status = fbe_base_config_set_default_offset((fbe_base_config_t *) provision_drive_p);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

	/*set the priorities for operations*/
	set_priorities.zero_priority = FBE_TRAFFIC_PRIORITY_LOW;
	set_priorities.verify_priority = FBE_TRAFFIC_PRIORITY_LOW;
    set_priorities.verify_invalidate_priority = FBE_TRAFFIC_PRIORITY_URGENT;

	fbe_provision_drive_set_priorities(provision_drive_p, set_priorities);
    fbe_provision_drive_update_last_checkpoint_time(provision_drive_p);
	provision_drive_p->monitor_counter = 0;

	provision_drive_p->last_zero_percent_notification = 0;

    /* Set percent rebuilt to initial value */
    fbe_provision_drive_set_percent_rebuilt(provision_drive_p, 0);

    fbe_provision_drive_set_ds_health_not_optimal_start_time(provision_drive_p, 0);

	/* Invalidate encryption keys */
	fbe_provision_drive_invalidate_keys(provision_drive_p);

    fbe_provision_drive_set_user_capacity(provision_drive_p, 0);

    /* Initialize paged metadata cache */
    fbe_provision_drive_metadata_cache_init(provision_drive_p);

    /* Initialize unmap bitmap */
    fbe_zero_memory(&provision_drive_p->unmap_bitmap, sizeof(fbe_provision_drive_unmap_bitmap_t));

    return FBE_STATUS_OK;
}
/* end fbe_provision_drive_init() */

/*!***************************************************************
 * fbe_provision_drive_destroy_object()
 ****************************************************************
 * @brief
 *  This function is the interface function, which is called
 *  by the topology to destroy an instance of this class.
 *
 * @param object_handle - The object being destroyed.
 *
 * @return fbe_status_t
 *  STATUS of the destroy operation.
 *
 * @author
 *  05/21/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_provision_drive_destroy_object(fbe_object_handle_t object_handle)
{
    fbe_status_t status;

    fbe_provision_drive_utils_trace((fbe_provision_drive_t *) fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_DESTROY_OBJECT,
                          FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                          "%s entry\n", __FUNCTION__);

    fbe_provision_drive_unmap_bitmap_destroy((fbe_provision_drive_t *)fbe_base_handle_to_pointer(object_handle));

    /* Call parent destructor. */
    status = fbe_base_config_destroy_object(object_handle);

    return status;
}
/* end fbe_provision_drive_destroy_object */

/*!**************************************************************
 * fbe_provision_drive_verify_downstream_health()
 ****************************************************************
 * @brief
 *  This function is used to verify the health of the downstream 
 *  objects, it will return any of the three return type based 
 *  on state of the edge with its downstream objects.
 *
 * @param virtual_drive_p - Virtual Drive object.
 *
 * @return fbe_status_t
 *  It will return any of the two values based on current state
 *  of the edge with downstream logical drive object.
 * 
 *  1) FBE_DOWNSTREAM_HEALTH_OPTIMAL: Communication with downsteam
 *                                    object is fully optimal.
 *  3) FBE_DOWNSTREAM_HEALTH_BROKEN:  Communication with downstream
 *                                    object is broken.
 *
 * @author
 *  8/14/2009 - Created. Dhaval Patel
 *  10/05/2009 - Peter Puhov. Minor cleanup
 ****************************************************************/
fbe_base_config_downstream_health_state_t
fbe_provision_drive_verify_downstream_health (fbe_provision_drive_t * provision_drive_p)
{
    fbe_base_config_downstream_health_state_t downstream_health_state;
    fbe_base_config_path_state_counters_t path_state_counters;

    fbe_provision_drive_utils_trace( provision_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                          "%s entry\n", __FUNCTION__);

    fbe_base_config_get_path_state_counters((fbe_base_config_t *) provision_drive_p, &path_state_counters);

    if(path_state_counters.enabled_counter > 0){ /* We do have our edge perfectly enabled */
        downstream_health_state = FBE_DOWNSTREAM_HEALTH_OPTIMAL;
    } else if(path_state_counters.disabled_counter > 0){ /* Our edge is disabled for some reason */
        downstream_health_state = FBE_DOWNSTREAM_HEALTH_DISABLED;
    } else if(path_state_counters.broken_counter > 0){ /* Our edge is gone or broken */
        downstream_health_state = FBE_DOWNSTREAM_HEALTH_BROKEN;
    } else {
        downstream_health_state = FBE_DOWNSTREAM_HEALTH_BROKEN;
    }

    return downstream_health_state;
}
/*****************************************************
 * end fbe_provision_drive_verify_downstream_health()
 *****************************************************/

/*!***************************************************************
 * fbe_provision_drive_set_condition_based_on_downstream_health()
 ****************************************************************
 * @brief
 *  This function is called to create an instance of this class.
 *
 * @param packet_p - The packet used to construct the object.  
 * @param object_handle - The object being created.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  8/14/2009 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_status_t
fbe_provision_drive_set_condition_based_on_downstream_health (fbe_provision_drive_t *provision_drive_p, 
                                                             fbe_path_attr_t path_attr,
                                                              fbe_base_config_downstream_health_state_t downstream_health_state)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t      end_of_life_state;
    fbe_lifecycle_state_t       lifecycle_state;

    fbe_provision_drive_metadata_get_end_of_life_state(provision_drive_p, &end_of_life_state);

    switch (downstream_health_state)
    {
        case FBE_DOWNSTREAM_HEALTH_OPTIMAL:
            /* First check for timeout errors.
             */
            if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS)
            {
                /* Update the timeout attributes on the edge with upstream object.
                 */
                fbe_block_transport_server_set_path_attr_all_servers(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                                         FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS);
            }

            /* Now check for end-of-life (EOL)
             */
            if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE)
            {
                if (end_of_life_state == FBE_FALSE)
                {
                    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO, 
                                          "provision drive end of life bit seen\n");
                    /* if end of life attribute is set then set cond to update metadata with eol. */
                    status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                                    (fbe_base_object_t *)provision_drive_p,
                                                    FBE_PROVISION_DRIVE_LIFECYCLE_COND_SET_END_OF_LIFE);
                }
            }
            break;
        case FBE_DOWNSTREAM_HEALTH_BROKEN:
            /* get the lifecycly state */
            status = fbe_lifecycle_get_state(&fbe_provision_drive_lifecycle_const, 
                                            (fbe_base_object_t *)provision_drive_p, &lifecycle_state);
            if(status != FBE_STATUS_OK){
                fbe_base_object_trace((fbe_base_object_t *) provision_drive_p, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Critical error fbe_lifecycle_get_state failed, status = %X", __FUNCTION__, status);
                break;
            }

            /* let's break out if the object needs to be destroyed */
            if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
            {
                /* use condition valid in specialize state. */
                status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                                (fbe_base_object_t*)provision_drive_p,
                                                FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_NOT_OPTIMAL);
            }
            else
            {
                /* Downstream health broken condition will be set. */
                status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                                (fbe_base_object_t*)provision_drive_p,
                                                FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
            }
            break;
        case FBE_DOWNSTREAM_HEALTH_DISABLED:
            /* Now check for end-of-life (EOL).   There are cases where PDO will set EOL when its in the
               activate state.   Even if PDO waits until it goes to Ready before setting the EOL there
               is a race condition between PVD path state changing to optimal.  This needs to be handled
               in when state is disabled.
             */
            if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE)
            {
                if (end_of_life_state == FBE_FALSE)
                {
                    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO, 
                                          "provision drive end of life bit seen\n");
                    /* if end of life attribute is set then set cond to update metadata with eol. */
                    status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                                    (fbe_base_object_t *)provision_drive_p,
                                                    FBE_PROVISION_DRIVE_LIFECYCLE_COND_SET_END_OF_LIFE);
                }
            }
            /* Downstream health disabled condition will be set. */
            status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                            (fbe_base_object_t*)provision_drive_p,
                                            FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED);

            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Invalid downstream health state: 0x%x!!\n", 
                                  __FUNCTION__,
                                  downstream_health_state);

            break;
    }

    return status;
}
/*********************************************************************
 * end fbe_provision_drive_set_condition_based_on_downstream_health()
 *********************************************************************/

/******************************************************************************
 *  fbe_provision_drive_handle_shutdown
 ******************************************************************************
 * @brief
 *  This function is the handler for a shutdown.
 *
 * @param provision_drive_p - a pointer to Provision drive
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  01/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/

fbe_status_t fbe_provision_drive_handle_shutdown(fbe_provision_drive_t * const provision_drive_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_queue_head_t               *termination_queue_p = &provision_drive_p->base_config.base_object.terminator_queue_head;
    fbe_packet_t                   *current_packet_p = NULL;
    fbe_raid_iots_t                *iots_p = NULL;
    fbe_queue_element_t            *next_element_p = NULL;
    fbe_queue_element_t            *queue_element_p = NULL;
    fbe_bool_t                      b_done = FBE_TRUE;
    fbe_payload_block_operation_t  *block_operation_p = NULL;
    fbe_queue_head_t                complete_queue;
    fbe_u32_t                       packet_fail_count = 0;

    /* Take both the provision drive (so that state doesn't change) and 
     * terminator queue lock.
     */
    fbe_provision_drive_lock(provision_drive_p);
    fbe_spinlock_lock(&provision_drive_p->base_config.base_object.terminator_queue_lock);

    /* Changing clustered flag will trigger more IO and CMI traffic, which prevents drive from shutting down.
     * On the other hand, when SEP is unload, peer should be able to pick up the changes.
     * Also, we need to abort IOs, not to restart them, as part of shutdown.
     */
    // fbe_base_config_clear_clustered_flag((fbe_base_config_t *) provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED);
    // fbe_base_config_clear_clustered_flag((fbe_base_config_t *) provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING);

    /* Loop over the termination queue and generate a list of iots that need to be
     * completed.
     */
    fbe_queue_init(&complete_queue);

    queue_element_p = fbe_queue_front(termination_queue_p);
    while (queue_element_p != NULL)
    {
        fbe_payload_ex_t *sep_payload_p = NULL;

        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload_p = fbe_transport_get_payload_ex(current_packet_p);

        next_element_p = fbe_queue_next(termination_queue_p, &current_packet_p->queue_element);
        fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

        /* If we found anything on the termination queue then we are not done.
         */
        b_done = FBE_FALSE;

        /* Add the siots for this iots that need to be restarted. 
         * We will kick them off below once we have finished iterating over 
         * the termination queue. 
         */
        if (iots_p->status == FBE_RAID_IOTS_STATUS_NOT_STARTED_TO_LIBRARY) 
        {
            /* Remove the packet from the terminator queue under lock.
             */
            status = fbe_transport_remove_packet_from_queue(current_packet_p);
            if (status == FBE_STATUS_OK)
            {
                /* Get the current block operation.
                 */
                block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    
                /* Mark the block operation status as failed, not retryable.
                 */
                fbe_payload_block_set_status(block_operation_p,
                                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
                
                /* Enqueue the packet onto the temporary queue.
                 */
                fbe_queue_push(&complete_queue, queue_element_p);
            }
            else
            {
                /* Else trace an error and break out of the loop.
                 */
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                      FBE_TRACE_LEVEL_ERROR, 
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s failed to remove packet: 0x%p status:0x%x\n", 
                                      __FUNCTION__, current_packet_p, status);
                break;
            }
        }
        else if(iots_p->status == FBE_RAID_IOTS_STATUS_NOT_USED)
        {
            /* If iots is not used then skip this terminated packet, eventually
             * packet will complete and it will handle the failure. 
             */
        }
        else
        {
            /* we do not expect any other state with pvd object. */
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s unexpected iots:%p status, status:0x%x\n", 
                                  __FUNCTION__, iots_p, iots_p->status);
        }

        /* Goto the next packet.
         */
        queue_element_p = next_element_p;
    }

    fbe_spinlock_unlock(&provision_drive_p->base_config.base_object.terminator_queue_lock);
    fbe_provision_drive_unlock(provision_drive_p);

    /* Restart all the items we found.
     */
    packet_fail_count = fbe_queue_length(&complete_queue);

    /* Trace out the number packets that we will complete.
     */
    if (packet_fail_count != 0)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s failing %d packets for shutdown\n", 
                              __FUNCTION__, packet_fail_count);
    }

    /* Complete all the failed packets. */
    while (!fbe_queue_is_empty(&complete_queue))
    {
        /* First get the queue element then the packet. */
        queue_element_p = fbe_queue_pop(&complete_queue);
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);

        /* Mark the packet status then complete it. */
        fbe_transport_set_status(current_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(current_packet_p);
    }

    /* If any packets where found return pending.
     */
    return (b_done) ? FBE_STATUS_OK : FBE_STATUS_PENDING;
}
/******************************************************************************
 * fbe_provision_drive_handle_shutdown()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_get_physical_drive_location()
 ******************************************************************************
 * @brief
 *  This function is use to get the provision drive location.
 *
 * @param provision_drive_p     - Provision drive object.
 * @param packet_p              - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/30/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_get_physical_drive_location(fbe_provision_drive_t * provision_drive_p,
                                                             fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                                     sep_payload_p = NULL;
    fbe_payload_control_operation_t *                       new_control_operation_p = NULL;
    fbe_payload_control_operation_t *                       control_operation_p = NULL;
    fbe_provision_drive_get_physical_drive_location_t *     get_physical_drive_location_p = NULL;
    fbe_base_port_mgmt_get_port_number_t *                  get_port_number_p = NULL;
    fbe_status_t                                            status = FBE_STATUS_OK;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p, 
                                                    sizeof(fbe_provision_drive_get_physical_drive_location_t),
                                                    (fbe_payload_control_buffer_t *)&get_physical_drive_location_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Allocate the control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    new_control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    if(new_control_operation_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate the memory for the physical drive management information. */
    get_port_number_p = (fbe_base_port_mgmt_get_port_number_t *) fbe_transport_allocate_buffer();
    if (get_port_number_p == NULL)
    {
        fbe_payload_ex_release_control_operation(sep_payload_p, new_control_operation_p);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize port number as invalid. */
    get_port_number_p->port_number = FBE_PORT_NUMBER_INVALID;

    /* Build the control packet to get the physical drive inforamtion. */
    fbe_payload_control_build_operation(new_control_operation_p,
                                        FBE_BASE_PORT_CONTROL_CODE_GET_PORT_NUMBER,
                                        get_port_number_p,
                                        sizeof(fbe_base_port_mgmt_get_port_number_t));

    /* Set our completion function. */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_provision_drive_get_physical_drive_port_number_completion,
                                          provision_drive_p);

    /* Set the traverse attribute which allows traverse to this packet
     * till port to get the required information.
     */
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
    status = fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_get_physical_drive_location()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_get_physical_drive_port_number_completion()
 ******************************************************************************
 * @brief
 *  This function is used as a completion function for the port number and it
 *  issues another operation to get the enclosure number info.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/10/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_get_physical_drive_port_number_completion(fbe_packet_t * packet_p,
                                                              fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                                 provision_drive_p = NULL;
    fbe_payload_ex_t *                                     sep_payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation_p = NULL;
    fbe_payload_control_operation_t *                       prev_control_operation_p = NULL;
    fbe_provision_drive_get_physical_drive_location_t *     get_physical_drive_location_p = NULL;
    fbe_status_t                                            status;
    fbe_base_port_mgmt_get_port_number_t *                  get_port_number_p = NULL;
    fbe_payload_control_status_t                            control_status;
    fbe_base_enclosure_mgmt_get_enclosure_number_t *        get_encl_number_p = NULL;
    fbe_u8_t *                                              buffer_p = NULL;

    provision_drive_p = (fbe_provision_drive_t *)context;

    /* get the current control operation for the port information. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the status of the control operation. */
    fbe_payload_control_get_status(control_operation_p, &control_status);
    status = fbe_transport_get_status_code(packet_p);

    /* get the previous control operation and associated buffer. */
    prev_control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(prev_control_operation_p, &get_physical_drive_location_p);

    /* get the buffer of the current control operation. */
    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    get_port_number_p = (fbe_base_port_mgmt_get_port_number_t *) buffer_p;

    /* If status is not good then complete the packet with error. */
    if((status != FBE_STATUS_OK) ||
       (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: get drive location failed, ctl_stat:0x%x, stat:0x%x, \n",
                               __FUNCTION__, control_status, status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
        control_status = (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ? control_status : FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
        fbe_transport_release_buffer(buffer_p);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_payload_control_set_status(prev_control_operation_p, control_status);
        return status;
    }

    /* store the port number in previous control operation. */
    get_physical_drive_location_p->port_number = get_port_number_p->port_number;

    /* get the enclosure description */
    get_encl_number_p = (fbe_base_enclosure_mgmt_get_enclosure_number_t *) buffer_p;

    /* initialize the physical drive information with zero. */
    get_encl_number_p->enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;

    /* build the control packet to get the physical drive inforamtion. */
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_NUMBER,
                                        get_encl_number_p,
                                        sizeof(fbe_base_enclosure_mgmt_get_enclosure_number_t));

    /* set our completion function */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_provision_drive_get_physical_drive_encl_number_completion,
                                          provision_drive_p);

    /* set the traverse attribute which allows traverse to this packet
     * till port to get the required information.
     */
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
    status = fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_get_physical_drive_port_number_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_get_physical_drive_encl_number_completion()
 ******************************************************************************
 * @brief
 *  This function is used as a completion function to get the enclosure number
 *  and it issues get slot number control operation.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/10/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_get_physical_drive_encl_number_completion(fbe_packet_t * packet_p,
                                                              fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                                 provision_drive_p = NULL;
    fbe_payload_ex_t *                                     sep_payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation_p = NULL;
    fbe_payload_control_operation_t *                       prev_control_operation_p = NULL;
    fbe_provision_drive_get_physical_drive_location_t *     get_physical_drive_location_p = NULL;
    fbe_base_enclosure_mgmt_get_enclosure_number_t *        get_encl_number_p = NULL;
    fbe_base_enclosure_mgmt_get_slot_number_t *             get_slot_number_p = NULL;
    fbe_status_t                                            status;
    fbe_payload_control_status_t                            control_status;
    fbe_u8_t *                                              buffer_p = NULL;

    provision_drive_p = (fbe_provision_drive_t *)context;

    /* get the current control operation for the port information. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the status of the control operation. */
    fbe_payload_control_get_status(control_operation_p, &control_status);
    status = fbe_transport_get_status_code(packet_p);

    /* get the previous control operation and associated buffer. */
    prev_control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(prev_control_operation_p, &get_physical_drive_location_p);

    /* get the buffer of the current control operation. */
    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    get_encl_number_p = (fbe_base_enclosure_mgmt_get_enclosure_number_t *) buffer_p;

    /* If status is not good then complete the packet with error. */
    if((status != FBE_STATUS_OK) ||
       (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: get drive location failed, ctl_stat:0x%x, stat:0x%x, \n",
                               __FUNCTION__, control_status, status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
        control_status = (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ? control_status : FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
        fbe_transport_release_buffer(buffer_p);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_payload_control_set_status(prev_control_operation_p, control_status);
        return status;
    }

    /* store the port number in previous control operation. */
    get_physical_drive_location_p->enclosure_number = get_encl_number_p->enclosure_number;

    /* get the enclosure description */
    get_slot_number_p = (fbe_base_enclosure_mgmt_get_slot_number_t *) buffer_p;

    /* initialize the physical drive information with zero. */
    get_slot_number_p->enclosure_slot_number = FBE_SLOT_NUMBER_INVALID;

    /* build the control packet to get the physical drive inforamtion. */
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_NUMBER,
                                        get_slot_number_p,
                                        sizeof(fbe_base_enclosure_mgmt_get_slot_number_t));

    /* set our completion function */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_provision_drive_get_physical_drive_slot_number_completion,
                                          provision_drive_p);

    /* set the traverse attribute which allows traverse to this packet
     * till port to get the required information.
     */
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
    status = fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_get_physical_drive_encl_number_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_get_physical_drive_slot_number_completion()
 ******************************************************************************
 * @brief
 *  This function is used as a completion function to get the slot number.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/10/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_get_physical_drive_slot_number_completion(fbe_packet_t * packet_p,
                                                              fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                                 provision_drive_p = NULL;
    fbe_payload_ex_t *                                     sep_payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation_p = NULL;
    fbe_payload_control_operation_t *                       prev_control_operation_p = NULL;
    fbe_provision_drive_get_physical_drive_location_t *     get_physical_drive_location_p = NULL;
    fbe_base_enclosure_mgmt_get_slot_number_t *             get_slot_number_p = NULL;
    fbe_status_t                                            status;
    fbe_payload_control_status_t                            control_status;

    provision_drive_p = (fbe_provision_drive_t *)context;

    /* get the current control operation for the port information. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the status of the control operation. */
    fbe_payload_control_get_status(control_operation_p, &control_status);
    status = fbe_transport_get_status_code(packet_p);

    /* get the previous control operation and associated buffer. */
    prev_control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(prev_control_operation_p, &get_physical_drive_location_p);

    /* get the buffer of the current control operation. */
    fbe_payload_control_get_buffer(control_operation_p, &get_slot_number_p);

    /* If status is not good then complete the packet with error. */
    if((status != FBE_STATUS_OK) ||
       (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: get drive location failed, ctl_stat:0x%x, stat:0x%x, \n",
                               __FUNCTION__, control_status, status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
        control_status = (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ? control_status : FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
        fbe_transport_release_buffer(get_slot_number_p);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_payload_control_set_status(prev_control_operation_p, control_status);
        return status;
    }

    /* store the port number in previous control operation. */
    get_physical_drive_location_p->slot_number = get_slot_number_p->enclosure_slot_number;

    /* get the enclosure description */
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
    fbe_transport_release_buffer(get_slot_number_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_get_physical_drive_slot_number_completion()
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_get_imported_offset(fbe_provision_drive_t * provision_drive_p, fbe_lba_t *offset)
{
    *offset = fbe_block_transport_edge_get_offset(&provision_drive_p->block_edge);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_provision_drive_set_config_type(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_config_type_t config_type)
{
    provision_drive_p->config_type = config_type;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_provision_drive_get_config_type(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_config_type_t *config_type)
{
    *config_type = provision_drive_p->config_type;
    return FBE_STATUS_OK;
}
fbe_status_t
fbe_provision_drive_update_last_checkpoint_time(fbe_provision_drive_t * provision_drive_p)
{
    provision_drive_p->last_checkpoint_time = fbe_get_time();
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_provision_drive_get_last_checkpoint_time(fbe_provision_drive_t * provision_drive_p, fbe_time_t *last_checkpoint_time)
{
    *last_checkpoint_time = provision_drive_p->last_checkpoint_time;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_provision_drive_get_configured_physical_block_size(fbe_provision_drive_t * provision_drive_p, 
        fbe_provision_drive_configured_physical_block_size_t *configured_physical_block_size)
{
    *configured_physical_block_size = provision_drive_p->configured_physical_block_size;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_provision_drive_set_configured_physical_block_size(fbe_provision_drive_t * provision_drive_p, 
        fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size)
{
    provision_drive_p->configured_physical_block_size = configured_physical_block_size;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_provision_drive_set_priorities(fbe_provision_drive_t * provision_drive, fbe_provision_drive_set_priorities_t set_priorities)
{
	provision_drive->zero_priority = set_priorities.zero_priority;
	provision_drive->verify_priority = set_priorities.verify_priority;
    provision_drive->verify_invalidate_priority = set_priorities.verify_invalidate_priority;
	return FBE_STATUS_OK;

}

fbe_status_t
fbe_provision_drive_set_max_drive_transfer_limit(fbe_provision_drive_t * provision_drive_p, fbe_block_count_t max_drive_xfer_limit)
{
    provision_drive_p->max_drive_xfer_limit  = max_drive_xfer_limit;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_provision_drive_get_max_drive_transfer_limit(fbe_provision_drive_t * provision_drive_p, fbe_block_count_t * max_drive_xfer_limit_p)
{
    *max_drive_xfer_limit_p = provision_drive_p->max_drive_xfer_limit;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_provision_drive_set_user_capacity(fbe_provision_drive_t * provision_drive_p, fbe_lba_t user_capacity)
{
    provision_drive_p->user_capacity  = user_capacity;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_provision_drive_get_user_capacity(fbe_provision_drive_t * provision_drive_p, fbe_lba_t *user_capacity)
{
    if (provision_drive_p->user_capacity) {
        *user_capacity = provision_drive_p->user_capacity;
    } else {
        fbe_base_config_get_capacity((fbe_base_config_t *)provision_drive_p, user_capacity);
    }
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_provision_drive_set_download_originator(fbe_provision_drive_t *provision_drive_p, fbe_bool_t is_originator)
{
    if (is_originator) {
        fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_DOWNLOAD_ORIGINATOR);
    } else {
        fbe_provision_drive_clear_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_DOWNLOAD_ORIGINATOR);
    }
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_provision_drive_set_health_check_originator(fbe_provision_drive_t *provision_drive_p, fbe_bool_t is_originator)
{
    if (is_originator) {
        fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_HC_ORIGINATOR);
    } else {
        fbe_provision_drive_clear_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_HC_ORIGINATOR);
    }
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_provision_drive_set_spin_down_qualified(fbe_provision_drive_t *provision_drive_p, fbe_bool_t is_qualified)
{
    if (is_qualified) {
        fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_SPIN_DOWN_QUALIFIED);
    } else {
        fbe_provision_drive_clear_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_SPIN_DOWN_QUALIFIED);
    }
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_provision_drive_set_unmap_supported(fbe_provision_drive_t *provision_drive_p, fbe_bool_t is_supported)
{
    if (is_supported) {
        fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_UNMAP_SUPPORTED);
        fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_UNMAP_ENABLED);
        fbe_provision_drive_unmap_bitmap_init(provision_drive_p);
    } else {
        fbe_provision_drive_clear_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_UNMAP_SUPPORTED);
        fbe_provision_drive_clear_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_UNMAP_ENABLED);
    }
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_provision_drive_set_unmap_enabled_disabled(fbe_provision_drive_t *provision_drive_p, fbe_bool_t is_enabled)
{
    if (is_enabled) {
        fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_UNMAP_ENABLED);
    } else {
        fbe_provision_drive_clear_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_UNMAP_ENABLED);
    }
    return FBE_STATUS_OK;
}

fbe_status_t  fbe_provision_drive_init_event_outstanding_flag(fbe_provision_drive_t  *provision_drive_p, fbe_bool_t  is_event_outstanding)
{
    if (is_event_outstanding) {
        fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_EVENT_OUTSTANDING);
    } else {
        fbe_provision_drive_clear_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_EVENT_OUTSTANDING);
    }
    return FBE_STATUS_OK;
}

fbe_status_t  fbe_provision_drive_set_event_outstanding_flag(fbe_provision_drive_t  *provision_drive_p)
{
    fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_EVENT_OUTSTANDING);
    return FBE_STATUS_OK;
}

fbe_status_t  fbe_provision_drive_get_event_outstanding_flag(fbe_provision_drive_t  *provision_drive_p, fbe_bool_t  *is_event_outstanding)
{
    *is_event_outstanding = fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_EVENT_OUTSTANDING);
    return FBE_STATUS_OK;
}

fbe_status_t  fbe_provision_drive_clear_event_outstanding_flag(fbe_provision_drive_t  *provision_drive_p)
{
    fbe_provision_drive_clear_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_EVENT_OUTSTANDING);
    return FBE_STATUS_OK;
}

fbe_u32_t fbe_provision_drive_get_disk_zeroing_speed(void)
{
    return fbe_provision_drive_zeroing_speed;
}
void fbe_provision_drive_set_disk_zeroing_speed(fbe_u32_t disk_zeroing_speed)
{
    fbe_provision_drive_zeroing_speed = disk_zeroing_speed; 
}

fbe_u32_t fbe_provision_drive_get_sniff_speed(void)
{
    return fbe_provision_drive_sniff_speed;
}
void fbe_provision_drive_set_sniff_speed(fbe_u32_t sniff_speed)
{
    fbe_provision_drive_sniff_speed = sniff_speed; 
}


/*!**************************************************************
 * fbe_provision_drive_set_clustered_flag()
 ****************************************************************
 * @brief
 *  This function sets the local clustered flags.
 *
 * @param provision_drive_p - The object.
 * @param flags - The flags to set.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/24/2011 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
fbe_provision_drive_set_clustered_flag(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_clustered_flags_t flags)
{
    fbe_provision_drive_metadata_memory_t * provision_drive_metadata_memory_p = NULL;

    if(provision_drive_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    provision_drive_metadata_memory_p = (fbe_provision_drive_metadata_memory_t *)provision_drive_p->base_config.metadata_element.metadata_memory.memory_ptr;
    if (provision_drive_metadata_memory_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!((provision_drive_metadata_memory_p->flags & flags) == flags))
    {
        provision_drive_metadata_memory_p->flags |= flags;
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*)provision_drive_p, FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_provision_drive_set_clustered_flag()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_clear_clustered_flag()
 ****************************************************************
 * @brief
 *  This function clears the local clustered flags.
 *
 * @param provision_drive_p - The object.
 * @param flags - The flags to clear.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/24/2011 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
fbe_provision_drive_clear_clustered_flag(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_clustered_flags_t flags)
{
    fbe_provision_drive_metadata_memory_t * provision_drive_metadata_memory_p = NULL;

    if(provision_drive_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    provision_drive_metadata_memory_p = (fbe_provision_drive_metadata_memory_t *)provision_drive_p->base_config.metadata_element.metadata_memory.memory_ptr;
    if (provision_drive_metadata_memory_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((provision_drive_metadata_memory_p->flags & flags) != 0)
    {
        provision_drive_metadata_memory_p->flags &= ~flags;
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*)provision_drive_p, FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_provision_drive_clear_clustered_flag()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_get_clustered_flags()
 ****************************************************************
 * @brief
 *  This function gets the local clustered flags.
 *
 * @param provision_drive_p - The object.
 * @param flags - The flags to get.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/24/2011 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
fbe_provision_drive_get_clustered_flags(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_clustered_flags_t * flags)
{
    fbe_provision_drive_metadata_memory_t * provision_drive_metadata_memory_p = NULL;

    if(provision_drive_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    provision_drive_metadata_memory_p = (fbe_provision_drive_metadata_memory_t *)provision_drive_p->base_config.metadata_element.metadata_memory.memory_ptr;
    if (provision_drive_metadata_memory_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s NULL pointer\n", __FUNCTION__);
        *flags = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *flags = provision_drive_metadata_memory_p->flags;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_provision_drive_get_clustered_flags()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_get_peer_clustered_flags()
 ****************************************************************
 * @brief
 *  This function gets the peer clustered flags.
 *
 * @param provision_drive_p - The object.
 * @param flags - The flags to get.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/24/2011 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
fbe_provision_drive_get_peer_clustered_flags(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_clustered_flags_t * flags)
{
    fbe_provision_drive_metadata_memory_t * provision_drive_metadata_memory_p = NULL;

    if(provision_drive_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    provision_drive_metadata_memory_p = (fbe_provision_drive_metadata_memory_t *)provision_drive_p->base_config.metadata_element.metadata_memory_peer.memory_ptr;
    if (provision_drive_metadata_memory_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s NULL pointer\n", __FUNCTION__);
        *flags = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *flags = provision_drive_metadata_memory_p->flags;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_provision_drive_get_peer_clustered_flags()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_is_clustered_flag_set()
 ****************************************************************
 * @brief
 *  This function checks whether the flags are set locally.
 *
 * @param provision_drive_p - The object.
 * @param flags - The flags to check.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/24/2011 - Created. chenl6
 *
 ****************************************************************/
fbe_bool_t 
fbe_provision_drive_is_clustered_flag_set(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_clustered_flags_t flags)
{
    fbe_bool_t is_flag_set = FBE_FALSE;
    fbe_provision_drive_metadata_memory_t * provision_drive_metadata_memory_p = NULL;

    if(provision_drive_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_FALSE;
    }

    provision_drive_metadata_memory_p = (fbe_provision_drive_metadata_memory_t *)provision_drive_p->base_config.metadata_element.metadata_memory.memory_ptr;
    if (provision_drive_metadata_memory_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s NULL pointer\n", __FUNCTION__);
        return FBE_FALSE;
    }

    if(((provision_drive_metadata_memory_p->flags & flags) == flags)) {
        is_flag_set = FBE_TRUE;
    }
    return is_flag_set;
}
/******************************************
 * end fbe_provision_drive_is_clustered_flag_set()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_is_peer_clustered_flag_set()
 ****************************************************************
 * @brief
 *  This function checks whether the flags are set in peer.
 *
 * @param provision_drive_p - The object.
 * @param flags - The flags to check.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/24/2011 - Created. chenl6
 *
 ****************************************************************/
fbe_bool_t 
fbe_provision_drive_is_peer_clustered_flag_set(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_clustered_flags_t flags)
{
    fbe_bool_t is_flag_set = FBE_FALSE;
    fbe_provision_drive_metadata_memory_t * provision_drive_metadata_memory_p = NULL;

    if(provision_drive_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_FALSE;
    }

    provision_drive_metadata_memory_p = (fbe_provision_drive_metadata_memory_t *)provision_drive_p->base_config.metadata_element.metadata_memory_peer.memory_ptr;
    if (provision_drive_metadata_memory_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s NULL pointer\n", __FUNCTION__);
        return FBE_FALSE;
    }

    if(((provision_drive_metadata_memory_p->flags & flags) == flags)) {
        is_flag_set = FBE_TRUE;
    }
    return is_flag_set;
}
/******************************************
 * end fbe_provision_drive_is_peer_clustered_flag_set()
 ******************************************/

/******************************************************************************
 *  fbe_provision_drive_abort_monitor_ops
 ******************************************************************************
 * @brief
 *  This function is the handler for aborting monitor IOs from the terminator queue.
 *
 * @param provision_drive_p - a pointer to Provision drive
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  11/22/2011 - Created. Amit Dhaduk
 *
 ******************************************************************************/

fbe_status_t fbe_provision_drive_abort_monitor_ops(fbe_provision_drive_t * const provision_drive_p)
{
    fbe_queue_head_t *                  termination_queue_p = &provision_drive_p->base_config.base_object.terminator_queue_head;
    fbe_base_object_t *                 base_object_p = &provision_drive_p->base_config.base_object;
    fbe_packet_t *                      current_packet_p = NULL;
    fbe_raid_iots_t *                   iots_p = NULL;
    fbe_queue_element_t *               next_element_p = NULL;
    fbe_queue_element_t *               queue_element_p = NULL;
    fbe_payload_ex_t *                  sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_payload_block_operation_opcode_t block_opcode; 


    fbe_provision_drive_lock(provision_drive_p);
	fbe_base_object_terminator_queue_lock((fbe_base_object_t *) provision_drive_p);

    //todo shutdown the door
     
    queue_element_p = fbe_queue_front(termination_queue_p);

    while (queue_element_p != NULL)
    {
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload_p = fbe_transport_get_payload_ex(current_packet_p);

        next_element_p = fbe_queue_next(termination_queue_p, &current_packet_p->queue_element);

        fbe_payload_ex_get_iots(sep_payload_p, &iots_p);


        /* Add the siots for this iots that need to be restarted. 
         * We will kick them off below once we have finished iterating over 
         * the termination queue. 
         */
        if (iots_p->status == FBE_RAID_IOTS_STATUS_NOT_STARTED_TO_LIBRARY) 
        {            
			fbe_base_object_terminator_queue_unlock((fbe_base_object_t *) provision_drive_p);

            /* complete the packet with failure status. */
            block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
            fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
              "%s  opcode %d, current packet 0x%p\n", __FUNCTION__, block_opcode, current_packet_p);

            if(fbe_provision_drive_is_background_request(block_opcode))
            {

                /* Not started to the library yet.
                 * Set this up so it can get completed through our standard completion path. 
                 */
                /* remove the packet from terminator queue and set the iots state to not used. */
                fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *) provision_drive_p, current_packet_p);
                fbe_raid_iots_set_as_not_used(iots_p);

                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                                      "%s  opcode %d, current packet 0x%x is going to abort\n", 
                                      __FUNCTION__, block_opcode, (fbe_u32_t)current_packet_p);


                /* set the faulure status and complete the packet. */
                fbe_payload_block_set_status(block_operation_p,
                                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
                fbe_transport_set_status(current_packet_p, FBE_STATUS_OK, 0);
				fbe_provision_drive_unlock(provision_drive_p);
                fbe_transport_complete_packet(current_packet_p);
				return FBE_STATUS_OK;

            }
            /* It is OK to return since there should only be a single monitor op in progress.
             */
            fbe_provision_drive_unlock(provision_drive_p);
            return FBE_STATUS_OK;
        }
        else if(iots_p->status == FBE_RAID_IOTS_STATUS_NOT_USED)
        {
            /* If iots is not used then skip this terminated packet, eventually
             * packet will complete and it will handle the failure. 
             */
            queue_element_p = next_element_p;
            continue;
        }
        else
        {
            /* we do not expect any other state with pvd object. */
            fbe_base_object_trace(base_object_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s unexpected iots:%p status, status:0x%x\n", __FUNCTION__, iots_p, iots_p->status);

            queue_element_p = next_element_p;
            continue;
    
        }
    }

	fbe_base_object_terminator_queue_unlock((fbe_base_object_t *) provision_drive_p);
	fbe_provision_drive_unlock(provision_drive_p);


    // todo open the door

    return FBE_STATUS_OK ;
}
/******************************************************************************
 * fbe_provision_drive_abort_monitor_ops()
 ******************************************************************************/

/******************************************************************************
 *  fbe_provision_drive_is_permanently_removed
 ******************************************************************************
 * @brief
 *  This function check whether a pvd is permanently removed.
 *  1. the PVD object has no downstream edge (LDO-PVD edge)
 *  2. the PVD object hasno upstream edge (PVD-VD edge)
 *  3. the PVD config type is "FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED"
 *  for a given time duration.
 * 
 * @param provision_drive_p - a pointer to Provision drive
 *        is_removed - FBE_TRUE/FALSE (OUT)
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  01/06/2012 - Created. zhangy
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_should_permanently_removed(fbe_provision_drive_t * provision_drive_p, fbe_bool_t * should_removed)
{
    fbe_status_t status;
    fbe_u32_t   downstream_edge_number = 0;
    fbe_u32_t   upstream_edges_number = 0;
    fbe_u32_t   width = 0;
    fbe_base_transport_server_t *               base_transport_server_p = NULL;
    fbe_base_edge_t *                           base_edge_p = NULL;
    fbe_block_edge_t *                          block_edge_p = NULL;
    fbe_provision_drive_config_type_t           config_type ;
    fbe_system_time_threshold_info_t            system_time_threshold;
    fbe_system_time_t                           time_stamp;
    fbe_bool_t                                  is_reached;
    fbe_path_state_t                            path_state;
    fbe_object_id_t                             my_object_id;
    fbe_bool_t                                  is_system_pvd;
    fbe_bool_t                                  is_metadata_initialized;
    fbe_time_t                                  ds_health_not_optimal_start_time;
    

    fbe_zero_memory(&time_stamp,sizeof(fbe_system_time_t));
    fbe_zero_memory(&system_time_threshold,sizeof(fbe_system_time_threshold_info_t));
    *should_removed = FBE_FALSE;

    /*system pvd shouldn't be destroyed, so we have to check whether it is a system pvd or not */
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &my_object_id);
    is_system_pvd = fbe_private_space_layout_object_id_is_system_pvd(my_object_id);
    /*if it is a system pvd, we should not destroy it*/
    if(is_system_pvd == FBE_TRUE)
    {
       return FBE_STATUS_OK;
    }

    fbe_base_config_get_width( (fbe_base_config_t *)&provision_drive_p->base_config, &width);

    /* Loop through the each downstream edges before we find either edge
     * pointer is NULL or path state is invalid.
     */
    while ((downstream_edge_number < width) &&
           (downstream_edge_number < FBE_MAX_DOWNSTREAM_OBJECTS))
    {
        /* Fetch the block edge ptr and send control packet on block edge.
         */
        fbe_base_config_get_block_edge((fbe_base_config_t *)&provision_drive_p->base_config, &block_edge_p, downstream_edge_number);
        if (block_edge_p == NULL)
        {   
            /* Break the loop if we find downstream edge is NULL.
             */
            break;
        }
        fbe_block_transport_get_path_state (block_edge_p, &path_state);
        if ((path_state == FBE_PATH_STATE_INVALID)||(path_state == FBE_PATH_STATE_BROKEN))// should there be broken?
        {
            /* Break the loop when we find the path state invalid.
             */
             break;
        }

        /* Increment the number of downstream edges.
         */
        downstream_edge_number++;
    }

    /* Get the base transport server pointer from the provision drive object.
     */
    base_transport_server_p = (fbe_base_transport_server_t *)
            &(((fbe_base_config_t *)&provision_drive_p->base_config)->block_transport_server);

    /* Get the edge list and traverse through to count the number of upstream
     * edge count.
     */
    base_edge_p = base_transport_server_p->client_list;
    while (base_edge_p != NULL)
    {
        upstream_edges_number++;
        base_edge_p = base_edge_p->next_edge;
    }

    // Get PVD config type
    fbe_provision_drive_get_config_type(provision_drive_p, &config_type);
    if(downstream_edge_number == 0 
       && upstream_edges_number == 0 
       && config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED) {
        status = database_get_time_threshold(&system_time_threshold);
        if(status != FBE_STATUS_OK) {
            fbe_provision_drive_utils_trace( provision_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                          "%s failed to get Global Info time_threshold from the DB service.\n", __FUNCTION__);
            return status;
        }

        /* If the non-paged metadata of PVD is still not valid yet, we check the downstream health not optimal start time_stamp*/
        fbe_base_config_metadata_is_initialized((fbe_base_config_t *)provision_drive_p, &is_metadata_initialized);
        if (!is_metadata_initialized) {
            ds_health_not_optimal_start_time = fbe_provision_drive_get_ds_health_not_optimal_start_time(provision_drive_p);
            if (fbe_get_elapsed_seconds(ds_health_not_optimal_start_time) >= (system_time_threshold.time_threshold_in_minutes * 60))
            {
                *should_removed = FBE_TRUE;
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "provision_drive: should remove - ds health has been not optimal for %d minutes,threshold minutes: %lld \n",
                                      fbe_get_elapsed_seconds(ds_health_not_optimal_start_time) / 60,
                                      system_time_threshold.time_threshold_in_minutes);
            }
            return FBE_STATUS_OK;
        }

        status = fbe_provision_drive_metadata_get_time_stamp(provision_drive_p,&time_stamp);
        if(status != FBE_STATUS_OK) {
            fbe_provision_drive_utils_trace( provision_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                          "%s failed to get time stamp from PVD nonpaged metadata.\n", __FUNCTION__);
            return status;
        }
        if(time_stamp.day != 0) {
            is_reached = fbe_provision_drive_time_stamp_reached_threshold(provision_drive_p, time_stamp,&system_time_threshold);
            if(is_reached){
                *should_removed = FBE_TRUE;
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "provision_drive: should remove - True time_stamp:%02d/%02d/%02d threshold minutes: %lld \n",
                                      time_stamp.year, time_stamp.month, time_stamp.day,
                                      system_time_threshold.time_threshold_in_minutes);
            }
        }
    }
    return FBE_STATUS_OK;

}
/******************************************************************************
 * fbe_provision_drive_is_permanently_removed()
 ******************************************************************************/

/******************************************************************************
 *  fbe_provision_drive_time_stamp_reached_threshold
 ******************************************************************************
 * @brief
 *  This function check whether a pvd removed time reaches the system time threshold.
 *  if so,the pvd should be destroyed
 * 
 * @param provision_drive_p - the pvd object
 *        time_stamp - a time stamp for pvd removed time
 *        system_time_threshold - time to remove the pvd
 *
 * @return
 *  fbe_bool_t
 *
 * @author
 *  01/09/2012 - Created. zhangy
 *
 ******************************************************************************/
static fbe_bool_t   fbe_provision_drive_time_stamp_reached_threshold(fbe_provision_drive_t * provision_drive_p, fbe_system_time_t time_stamp,fbe_system_time_threshold_info_t *system_time_threshold)
{
    fbe_bool_t  is_reached = FBE_FALSE;
    fbe_system_time_t current_time;
    fbe_status_t status;
    fbe_s64_t diff_in_day = 0;
    fbe_s64_t diff_in_hour = 0;
    fbe_s64_t diff_in_minute = 0;
    fbe_u32_t current_day_number = 0;
    fbe_u32_t timestamp_day_number = 0;
    fbe_u32_t month = 0;
    fbe_u32_t year = 0 ;
    status = fbe_get_system_time(&current_time);
    month = (current_time.month + 9)%12;
    year = current_time.year - month/10;
    current_day_number = 365*year + year/4 - year/100 + year/400 + (month*306 + 5)/10 + (current_time.day - 1);

    month = (time_stamp.month + 9)%12;
    year = time_stamp.year - month/10;
    timestamp_day_number = 365*year + year/4 - year/100 + year/400 + (month*306 + 5)/10 + (time_stamp.day - 1);
    
    if(current_day_number < timestamp_day_number)
    { //Consider the time sync, this could happen. In this situation, should not remove pvd.
        return FBE_FALSE;
    }

    /*The difference in days between two dates*/
    diff_in_day = current_day_number - timestamp_day_number;
    /*if the difference reaches the threshold will trigger the pvd destroy*/

    if(diff_in_day == 0)
    {
        /*If current_time < time_stamp, should not remove pvd*/
		if(current_time.hour < time_stamp.hour)
		{
			return is_reached;
		}
		diff_in_hour = current_time.hour - time_stamp.hour;
		if( current_time.minute < time_stamp.minute)
        { 
            diff_in_minute = time_stamp.minute - current_time.minute;
			diff_in_minute = diff_in_hour*60 - diff_in_minute;
        }else{            
			diff_in_minute = current_time.minute - time_stamp.minute;
            diff_in_minute = diff_in_hour*60 + diff_in_minute;
        }

        if(diff_in_minute < (fbe_s64_t)0)
        {
            return FBE_FALSE;
        }
        if(diff_in_minute >= system_time_threshold->time_threshold_in_minutes)
        {
            is_reached = FBE_TRUE;
        }
    }else{
        if( current_time.hour < time_stamp.hour)
        {
            
            diff_in_hour = time_stamp.hour - current_time.hour;
            diff_in_hour = diff_in_day *24 - diff_in_hour;
        }else{
        
		    diff_in_hour = current_time.hour - time_stamp.hour;
            diff_in_hour = diff_in_day*24 + diff_in_hour;
        }

        if(diff_in_hour < (fbe_s64_t)0)
        {
            return FBE_FALSE;
        }

        if( current_time.minute < time_stamp.minute)
        { 
            diff_in_minute = time_stamp.minute - current_time.minute;
			diff_in_minute = diff_in_hour*60 - diff_in_minute;
        }else{            
			diff_in_minute = current_time.minute - time_stamp.minute;
            diff_in_minute = diff_in_hour*60 + diff_in_minute;
        }
        
        if(diff_in_minute < (fbe_s64_t)0)
        {
            return FBE_FALSE;
        }
        if(diff_in_minute >= system_time_threshold->time_threshold_in_minutes)
        {
            is_reached = FBE_TRUE;
        }
    }

    if(is_reached == FBE_TRUE)
    {

        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "pvd should remove,cur_t:%02d/%02d/%02d<%02d:%02d>,stamp:%02d/%02d/%02d<%02d:%02d>,diff_min:%lld,threshold: %lld\n",
                              current_time.year, current_time.month, current_time.day,current_time.hour,current_time.minute,
                              time_stamp.year,time_stamp.month,time_stamp.day,time_stamp.hour,time_stamp.minute,
                              diff_in_minute, system_time_threshold->time_threshold_in_minutes);
    }

    return is_reached;
}

/******************************************************************************
 * fbe_provision_drive_wait_for_downstream_health_optimal_timeout
 ******************************************************************************
 * @brief
 * Before remove the PVD, we need to wait for a moment. Because the edge of the PVD
 * has not connected yet.
 *
 * @return
 *  fbe_bool_t
 *
 * @author
 *  05/24/2013 - Created. Hongpo Gao 
 *
 ******************************************************************************/
fbe_bool_t fbe_provision_drive_wait_for_downstream_health_optimal_timeout(fbe_provision_drive_t *provision_drive_p)
{
    fbe_time_t start_time = fbe_provision_drive_get_ds_health_not_optimal_start_time(provision_drive_p);

    if (fbe_get_elapsed_seconds(start_time) >= MAX_REMOVE_PVD_WAIT_FOR_DOWNSTREAM_EDGE_SECONDS) {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

fbe_u64_t fbe_provision_drive_class_get_zeroing_cpu_rate(void)
{
	return fbe_provision_drive_zeroing_cpu_rate;
}

void fbe_provision_drive_class_set_zeroing_cpu_rate(fbe_u64_t zeroing_cpu_rate)
{
	fbe_provision_drive_zeroing_cpu_rate = zeroing_cpu_rate;
}

fbe_u64_t fbe_provision_drive_class_get_verify_invalidate_cpu_rate(void)
{
	return fbe_provision_drive_verify_invalidate_cpu_rate;
}

void fbe_provision_drive_class_set_verify_invalidate_cpu_rate(fbe_u64_t verify_invalidate_cpu_rate)
{
	fbe_provision_drive_verify_invalidate_cpu_rate = verify_invalidate_cpu_rate;
}

fbe_u64_t fbe_provision_drive_class_get_sniff_cpu_rate(void)
{
	return fbe_provision_drive_sniff_cpu_rate;
}

void fbe_provision_drive_class_set_sniff_cpu_rate(fbe_u64_t sniff_cpu_rate)
{
	fbe_provision_drive_sniff_cpu_rate = sniff_cpu_rate;
}

fbe_bool_t fbe_provision_drive_is_slf_enabled(void)
{
	return fbe_provision_drive_enable_slf;
}

void fbe_provision_drive_set_slf_enabled(fbe_bool_t enabled)
{
	fbe_provision_drive_enable_slf = enabled;
}


/*!**************************************************************
 * fbe_provision_drive_is_peer_flag_set()
 ****************************************************************
 * @brief
 *  This function checks whether the peer clustered flag is set, 
 *  considering whether the peer is joined or not. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *   peer_flag  - the flag to check
 *
 * @return fbe_bool_t
 * 
 * @author
 *  05/02/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_bool_t fbe_provision_drive_is_peer_flag_set(fbe_provision_drive_t * provision_drive_p, 
                                                fbe_provision_drive_clustered_flags_t peer_flag)
{
    /* Check if the peer has joined. 
     */
    if (fbe_base_config_has_peer_joined((fbe_base_config_t *)provision_drive_p))
    {
        return (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, peer_flag));
    }
    else
    {
        /* If the peer is not there, we should not wait for it,
         * assume the bit is set. 
         */
        return FBE_TRUE;
    }
}
/****************************************************************
 * end fbe_provision_drive_is_peer_flag_set()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_process_drive_fault()
 ****************************************************************
 * @brief
 *  This function processes state changes caused by drive fault. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *   path_attr  - path attributes
 *   downstream_health_state  - downstream health
 *
 * @return fbe_status_t
 * 
 * @author
 *  07/11/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t
fbe_provision_drive_process_drive_fault (fbe_provision_drive_t *provision_drive_p, 
                                         fbe_path_attr_t path_attr,
                                         fbe_base_config_downstream_health_state_t downstream_health_state)
{
    fbe_status_t    status;
    fbe_bool_t      drive_fault_state;
    fbe_u32_t       number_of_clients = 0;

    fbe_provision_drive_metadata_get_drive_fault_state(provision_drive_p, &drive_fault_state);
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT)
    {
        /* Wait edge state change to disabled/broken/gone */
        if ((downstream_health_state == FBE_DOWNSTREAM_HEALTH_BROKEN) && !drive_fault_state)
        {
            /* update the upstream edge with drive fault bit attributes if pvd has upstream edge. */
            fbe_block_transport_server_get_number_of_clients(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                         &number_of_clients);
            if (number_of_clients != 0)
            {
                /* If `Drive Fault' is not already set, set it now and generate the 
                 * attribute change event upstream.
                 */
                fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(
                    &((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                    FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT,  /* Path attribute to set */
                    FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT   /* Mask of attributes to not set if already set*/);
            }

            /* Set the clustered flag if it is not already set */
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "provision drive DRIVE FAULT bit set\n");
            fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DRIVE_FAULT);
            status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                            (fbe_base_object_t *)provision_drive_p,
                                            FBE_BASE_CONFIG_LIFECYCLE_COND_SET_DRIVE_FAULT);
        }
    }
    else
    {
        /* Wait edge state change to enabled */
        if ((downstream_health_state == FBE_DOWNSTREAM_HEALTH_OPTIMAL) &&
            fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DRIVE_FAULT))
        {
            fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DRIVE_FAULT);
        }
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_provision_drive_process_drive_fault()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_check_drive_location()
 ****************************************************************
 * @brief
 *  This function checks whether both SPs have the same port, encl,
 *  and slot numbers. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *
 * @return FBE_TRUE if both sides have the same drive info
 * 
 * @author
 *  09/24/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_bool_t fbe_provision_drive_check_drive_location(fbe_provision_drive_t * provision_drive_p)
{
    fbe_u32_t port_num, encl_num, slot_num;
    fbe_u32_t peer_port_num, peer_encl_num, peer_slot_num;

    fbe_provision_drive_get_drive_location(provision_drive_p, &port_num, &encl_num, &slot_num);
    if(port_num == FBE_PORT_NUMBER_INVALID || encl_num == FBE_ENCLOSURE_NUMBER_INVALID || slot_num == FBE_SLOT_NUMBER_INVALID){
        return FBE_TRUE;
    }

    fbe_provision_drive_get_peer_drive_location(provision_drive_p, &peer_port_num, &peer_encl_num, &peer_slot_num);
    if(peer_port_num == FBE_PORT_NUMBER_INVALID || peer_encl_num == FBE_ENCLOSURE_NUMBER_INVALID || peer_slot_num == FBE_SLOT_NUMBER_INVALID){
        return FBE_TRUE;
    }

    if((port_num != peer_port_num) || (encl_num != peer_encl_num) || (slot_num != peer_slot_num)){
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s drive info not match B/E/S %x/%x/%x\n", __FUNCTION__, port_num, encl_num, slot_num);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s peer B/E/S %x/%x/%x\n", __FUNCTION__, peer_port_num, peer_encl_num, peer_slot_num);
        return FBE_FALSE;
    }

    return FBE_TRUE;
}
/****************************************************************
 * end fbe_provision_drive_check_drive_location()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_set_drive_location()
 ****************************************************************
 * @brief
 *  This function sets drive information in metadata memory. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *   port_num  - bus number
 *   encl_num  - enclosure number
 *   slot_num  - slot number
 *
 * @return fbe_status_t
 * 
 * @author
 *  09/24/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_set_drive_location(fbe_provision_drive_t * provision_drive_p,
                                                    fbe_u32_t               port_num,
                                                    fbe_u32_t               encl_num,
                                                    fbe_u32_t               slot_num)
{
    fbe_provision_drive_metadata_memory_t * provision_drive_metadata_memory_p = NULL;

    if(provision_drive_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    provision_drive_metadata_memory_p = (fbe_provision_drive_metadata_memory_t *)provision_drive_p->base_config.metadata_element.metadata_memory.memory_ptr;
    if (provision_drive_metadata_memory_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    provision_drive_metadata_memory_p->port_number = port_num;
    provision_drive_metadata_memory_p->enclosure_number = encl_num;
    provision_drive_metadata_memory_p->slot_number = slot_num;
    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*)provision_drive_p, FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_provision_drive_set_drive_location()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_get_drive_location()
 ****************************************************************
 * @brief
 *  This function sets drive information in metadata memory. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *   port_num_p   - pointer to bus number
 *   encl_num_p   - pointer to enclosure number
 *   slot_num_p   - pointer to slot number
 *
 * @return fbe_status_t
 * 
 * @author
 *  09/24/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_get_drive_location(fbe_provision_drive_t * provision_drive_p,
                                                     fbe_u32_t *        port_num_p,
                                                     fbe_u32_t *        encl_num_p,
                                                     fbe_u32_t *        slot_num_p)
{
    fbe_provision_drive_metadata_memory_t * provision_drive_metadata_memory_p = NULL;

    *port_num_p = FBE_PORT_NUMBER_INVALID;
    *encl_num_p = FBE_ENCLOSURE_NUMBER_INVALID;
    *slot_num_p = FBE_SLOT_NUMBER_INVALID;
    if(provision_drive_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    provision_drive_metadata_memory_p = (fbe_provision_drive_metadata_memory_t *)provision_drive_p->base_config.metadata_element.metadata_memory.memory_ptr;
    if (provision_drive_metadata_memory_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *port_num_p = provision_drive_metadata_memory_p->port_number;
    *encl_num_p = provision_drive_metadata_memory_p->enclosure_number;
    *slot_num_p = provision_drive_metadata_memory_p->slot_number;
    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_provision_drive_get_drive_location()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_get_peer_drive_location()
 ****************************************************************
 * @brief
 *  This function sets drive information in metadata memory. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *   peer_port_num_p   - pointer to bus number
 *   peer_encl_num_p   - pointer to enclosure number
 *   peer_slot_num_p   - pointer to slot number
 *
 * @return fbe_status_t
 * 
 * @author
 *  09/24/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_get_peer_drive_location(fbe_provision_drive_t * provision_drive_p,
                                                         fbe_u32_t *             peer_port_num_p,
                                                        fbe_u32_t *             peer_encl_num_p,
                                                         fbe_u32_t *             peer_slot_num_p)
{
    fbe_provision_drive_metadata_memory_t * provision_drive_metadata_memory_p = NULL;

    *peer_port_num_p = FBE_PORT_NUMBER_INVALID;
    *peer_encl_num_p = FBE_ENCLOSURE_NUMBER_INVALID;
    *peer_slot_num_p = FBE_SLOT_NUMBER_INVALID;
    if (!fbe_base_config_is_peer_present((fbe_base_config_t *)provision_drive_p)) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(provision_drive_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    provision_drive_metadata_memory_p = (fbe_provision_drive_metadata_memory_t *)provision_drive_p->base_config.metadata_element.metadata_memory_peer.memory_ptr;
    if (provision_drive_metadata_memory_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *peer_port_num_p = provision_drive_metadata_memory_p->port_number;
    *peer_encl_num_p = provision_drive_metadata_memory_p->enclosure_number;
    *peer_slot_num_p = provision_drive_metadata_memory_p->slot_number;
    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_provision_drive_get_peer_drive_location()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_process_state_change_for_faulted_drives()
 ****************************************************************
 * @brief
 *  This function processes state/attr changes caused by drive fault
 *  and link fault. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *   path_state  - path state
 *   path_attr  - path attributes
 *   downstream_health_state  - downstream health
 *
 * @return fbe_bool_t - TRUE if the caller should continue state handling.
 * 
 * @author
 *  10/30/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_bool_t
fbe_provision_drive_process_state_change_for_faulted_drives (fbe_provision_drive_t *provision_drive_p, 
                                                             fbe_path_state_t path_state,
                                                             fbe_path_attr_t path_attr,
                                                             fbe_base_config_downstream_health_state_t downstream_health_state)
{
    fbe_lifecycle_state_t       lifecycle_state;

    fbe_lifecycle_get_state(&fbe_provision_drive_lifecycle_const, 
                                    (fbe_base_object_t *)provision_drive_p, &lifecycle_state);
    if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
    {
        return FBE_TRUE;
    }

    /* Process state changes caused by drive fault. */
    fbe_provision_drive_process_drive_fault(provision_drive_p, path_attr, downstream_health_state);

    return (fbe_provision_drive_initiate_eval_slf(provision_drive_p, path_state, path_attr));
}
/****************************************************************
 * end fbe_provision_drive_process_state_change_for_faulted_drives()
 ****************************************************************/


/* Accessors for the serial number */
fbe_status_t fbe_provision_drive_set_serial_num(fbe_provision_drive_t *provision_drive, fbe_u8_t *serial_num)
{
    fbe_object_id_t object_id;
    fbe_bool_t is_system_drive;

    fbe_copy_memory(provision_drive->serial_num, serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive, &object_id);
    is_system_drive = fbe_database_is_object_system_pvd(object_id);
    if (is_system_drive) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: update c4mirror map\n", __FUNCTION__);
        fbe_c4_mirror_update_pvd_by_object_id(object_id, provision_drive->serial_num,
                                             FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_provision_drive_get_serial_num(fbe_provision_drive_t *provision_drive, fbe_u8_t **serial_num)
{
    *serial_num = provision_drive->serial_num;
    return FBE_STATUS_OK;
}

fbe_bool_t fbe_provision_drive_is_write_same_enabled(fbe_provision_drive_t *provision_drive, fbe_lba_t lba)
{
	fbe_system_encryption_mode_t    system_encryption_mode;
    fbe_edge_index_t                server_index = 0;
    fbe_provision_drive_key_info_t  * key_info;
    fbe_object_id_t                 object_id;
    fbe_bool_t                      is_system_drive;
    fbe_status_t				    status;
    fbe_lba_t                       exported_capacity;

	if(fbe_provision_drive_class_is_debug_flag_set(FBE_PROVISION_DRIVE_DEBUG_FLAG_NO_WRITE_SAME)){
		return FBE_FALSE;
	}

	if(fbe_provision_drive_is_pvd_debug_flag_set(provision_drive, FBE_PROVISION_DRIVE_DEBUG_FLAG_NO_WRITE_SAME)){
		return FBE_FALSE;
	} 

    fbe_database_get_system_encryption_mode(&system_encryption_mode);
    if(system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) {
        /* We need to check for proper miniport key handle.  
         * We can't do write same if we have valid handle to miniport.
         */
        fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive, &object_id);
        is_system_drive = fbe_database_is_object_system_pvd(object_id);
        fbe_base_config_get_capacity((fbe_base_config_t *)provision_drive, &exported_capacity);
        if ((lba < exported_capacity) && (is_system_drive)) {
            /* If we have an edge that consumes this, then use that server index.
             */
            fbe_block_transport_server_get_server_index_for_lba(&provision_drive->base_config.block_transport_server,
                                                                lba, &server_index);
            if (server_index == FBE_EDGE_INDEX_INVALID) {
                server_index = 0;
            }
        } else {
            /* Non system drives just use server index 0 for monitor ops.
            */
            server_index = 0;
        }
        status = fbe_provision_drive_get_key_info(provision_drive, server_index, &key_info);
        if ((status == FBE_STATUS_OK) && 
            ((key_info->mp_key_handle_1 != FBE_INVALID_KEY_HANDLE) ||
             (key_info->mp_key_handle_2 != FBE_INVALID_KEY_HANDLE))){
            return FBE_FALSE;
        }
    }

	return FBE_TRUE;	
}


fbe_status_t 
fbe_provision_drive_get_key_info(fbe_provision_drive_t *provision_drive, 
								 fbe_edge_index_t client_index, 
								 fbe_provision_drive_key_info_t ** key_info)
{
	* key_info = NULL;

	if(client_index >= FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX){
		fbe_base_object_trace((fbe_base_object_t *)provision_drive, 
							  FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							  "PVD set encr handle Invalid client_index %d\n", client_index);
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
	* key_info = &provision_drive->key_info_table[client_index];

	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_provision_drive_invalidate_keys(fbe_provision_drive_t *provision_drive)
{
	fbe_u32_t i;

	for(i = 0; i < FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX; i++){
		provision_drive->key_info_table[i].key_handle = NULL;
		provision_drive->key_info_table[i].mp_key_handle_1 = FBE_INVALID_KEY_HANDLE;
		provision_drive->key_info_table[i].mp_key_handle_2 = FBE_INVALID_KEY_HANDLE;
		provision_drive->key_info_table[i].port_object_id = FBE_OBJECT_ID_INVALID;
        provision_drive->key_info_table[i].encryption_state = FBE_BASE_CONFIG_ENCRYPTION_STATE_INVALID;
	}

	return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_provision_drive_is_unmap_supported()
 ****************************************************************
 * @brief
 *  This function sets drive information in metadata memory. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/4/2015 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_bool_t fbe_provision_drive_is_unmap_supported(fbe_provision_drive_t *provision_drive)
{
    return (fbe_database_is_unmap_committed() &&
            fbe_provision_drive_is_flag_set(provision_drive, 
                                            FBE_PROVISION_DRIVE_FLAG_UNMAP_SUPPORTED|FBE_PROVISION_DRIVE_FLAG_UNMAP_ENABLED));
}

/*******************************
 * end fbe_provision_drive_main.c
 *******************************/


