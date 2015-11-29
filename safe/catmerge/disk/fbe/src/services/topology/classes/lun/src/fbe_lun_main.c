/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_lun_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the lun object.
 *  This includes the create and destroy methods for the lun, as
 *  well as the event entry point, load and unload methods.
 * 
 * @ingroup lun_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_lun_private.h"
#include "base_object_private.h"
#include "fbe_perfstats.h"

/* Class methods forward declaration.
 */
fbe_status_t fbe_lun_load(void);
fbe_status_t fbe_lun_unload(void);
fbe_status_t fbe_lun_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_lun_destroy( fbe_object_handle_t object_handle);
fbe_status_t fbe_lun_destroy_interface( fbe_object_handle_t object_handle);
static fbe_status_t fbe_lun_process_block_transport_event(fbe_block_transport_event_type_t event_type, 
                                                          fbe_block_trasnport_event_context_t context);

/* Export class methods.
 */
fbe_class_methods_t fbe_lun_class_methods = {FBE_CLASS_ID_LUN,
                                             fbe_lun_load,
                                             fbe_lun_unload,
                                             fbe_lun_create_object,
                                             fbe_lun_destroy_interface,
                                             fbe_lun_control_entry,
                                             fbe_lun_event_entry,
                                             fbe_lun_io_entry,
                                             fbe_lun_monitor_entry};

/* This is our global transport constant, which we use to setup a portion of our block 
 * transport server. This is global since it is the same for all lun objects
 */
fbe_block_transport_const_t fbe_lun_block_transport_const = {fbe_lun_block_transport_entry, 
															fbe_lun_process_block_transport_event,
															fbe_lun_io_entry,
                                                            NULL, NULL};

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

static void fbe_lun_initialize_verify_report (fbe_lun_t*  in_lun_p);


/*!***************************************************************
 * fbe_lun_load()
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
fbe_lun_load(void)
{
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_lun_t) <= FBE_MEMORY_CHUNK_SIZE); 
    /* Make sure the lun's non-paged metadata has not exceeded its limit.  
     * The calls to read/write the non-paged MD check the value FBE_PAYLOAD_METADATA_MAX_DATA_SIZE instead of 
     * FBE_METADATA_NONPAGED_MAX_SIZE.
     */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_lun_nonpaged_metadata_t) <= FBE_METADATA_NONPAGED_MAX_SIZE);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_lun_nonpaged_metadata_t) <= FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);

    fbe_lun_monitor_load_verify ();
    return FBE_STATUS_OK;
}
/* end fbe_lun_load() */

/*!***************************************************************
 * fbe_lun_unload()
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
fbe_lun_unload(void)
{
    /* Destroy any global data.
     */
    return FBE_STATUS_OK;
}
/* end fbe_lun_unload() */

/*!***************************************************************
 * fbe_lun_create_object()
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
fbe_lun_create_object(fbe_packet_t * packet_p, fbe_object_handle_t * object_handle)
{
    fbe_lun_t *lun_p;
    fbe_status_t status;

    /* Call the base class create function.
     */
    status = fbe_base_config_create_object(packet_p, object_handle);
    if(status != FBE_STATUS_OK) {
        return status;
    }

    /* Set class id.
     */
    lun_p = (fbe_lun_t *)fbe_base_handle_to_pointer(*object_handle);

    fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "lun create: %s Object: 0x%x Ptr: 0x%p\n", 
                          __FUNCTION__,
                          lun_p->base_config.base_object.object_id,
                          lun_p);

    /* Set class id.
     */
    fbe_base_object_set_class_id((fbe_base_object_t *) lun_p, FBE_CLASS_ID_LUN);

	fbe_base_config_send_specialize_notification((fbe_base_config_t *) lun_p);

    /* Simply call our static init function to init members that have no
     * dependencies. 
     */
	status = fbe_lun_init(lun_p);

    /* Force verify to make sure the object is properly initialized before scheduling it.
     * This avoids race condition between scheduler thread and usurper command trying to 
     * initialize the object rotary preset at the same time.
     */
    fbe_lifecycle_verify((fbe_lifecycle_const_t *)&fbe_lun_lifecycle_const, (fbe_base_object_t *) lun_p);

	/* Enable lifecycle tracing if requested */
	status = fbe_base_object_enable_lifecycle_debug_trace((fbe_base_object_t *)lun_p);

    return status;
}
/* end fbe_lun_create_object() */

/*!***************************************************************
 * fbe_lun_init()
 ****************************************************************
 * @brief
 *  This function initializes the lun object.
 *
 *  Note that everything that is created here must be
 *  destroyed within fbe_lun_destroy().
 *
 * @param lun_p - The lun object.
 *
 * @return
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_lun_init(fbe_lun_t * const lun_p)
{
    fbe_status_t status;
    /* fbe_base_config_init((fbe_base_config_t *)lun_p); */
    // Lun is being initialized, so zero out the block edge memory and it will be
    // populated when the edge gets created between the lun and raid group.
    fbe_zero_memory((void*)&lun_p->block_edge, sizeof(lun_p->block_edge));
    fbe_base_config_set_block_edge_ptr((fbe_base_config_t *)lun_p, &lun_p->block_edge);
    status = fbe_base_config_set_block_transport_const((fbe_base_config_t *)lun_p, &fbe_lun_block_transport_const);
	status = fbe_base_config_set_stack_limit((fbe_base_config_t *) lun_p);

    /* We want the LUN to catch any invalid request errors, retry them and also take the LUN 
     * offline if we cannot resolve some threshold of errors. 
     */
    fbe_block_transport_server_set_handle_sw_errors(&lun_p->base_config.block_transport_server);

    /*! @todo: Exported Capacity should be set here? */
    fbe_lun_set_capacity(lun_p, 0);

    /* We only have a single edge.
     */
    fbe_base_config_set_width(&lun_p->base_config, 1);    
    
    // Initialize the LUN verify report data.
    fbe_lun_initialize_verify_report(lun_p);
    lun_p->lun_verify.verify_type = FBE_LUN_VERIFY_TYPE_NONE;

    /* Set the default offset */
    status = fbe_base_config_set_default_offset((fbe_base_config_t *) lun_p);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    fbe_lun_init_flags(lun_p);
    lun_p->prev_path_attr = 0;
	lun_p->power_save_io_drain_delay_in_sec = FBE_LUN_POWER_SAVE_DELAY_DEFAULT;
	lun_p->max_lun_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    lun_p->wait_for_power_save = 0;
	fbe_base_config_set_power_saving_idle_time((fbe_base_config_t *)lun_p, FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME);

    fbe_spinlock_init(&lun_p->io_counter_lock);

    lun_p->io_counter = 0;
    lun_p->dirty_pending = FBE_FALSE;
    lun_p->clean_pending = FBE_FALSE;
    lun_p->clean_time = 0;
    lun_p->marked_dirty = FBE_FALSE;

	lun_p->lun_attributes = 0;

	lun_p->write_bypass_mode = FBE_FALSE;
	lun_p->last_rebuild_percent = 100;
	lun_p->last_zero_checkpoint = 0;

    lun_p->zero_update_interval_count = FBE_LUN_MIN_ZEROING_UPDATE_INTERVAL;
    lun_p->zero_update_count_to_update = lun_p->zero_update_interval_count;

    //initialize performance stats
    lun_p->b_perf_stats_enabled = FBE_FALSE; //default is false.
    status = fbe_perfstats_get_system_memory_for_object_id(lun_p->base_config.base_object.object_id,
                                                           FBE_PACKAGE_ID_SEP_0,
                                                           (void**)&lun_p->performance_stats.counter_ptr.lun_counters);

    if (status == FBE_STATUS_OK && lun_p->performance_stats.counter_ptr.lun_counters) {
        lun_p->performance_stats.counter_ptr.lun_counters->object_id = lun_p->base_config.base_object.object_id;
        lun_p->performance_stats.counter_ptr.lun_counters->lun_number = FBE_U32_MAX;
        lun_p->performance_stats.counter_ptr.lun_counters->timestamp = fbe_get_time();
        fbe_perfstats_is_collection_enabled_for_package(FBE_PACKAGE_ID_SEP_0, //we only check package-level stats collection if we have a valid stat pointer
                                                       &lun_p->b_perf_stats_enabled);
    }
    
    return status;
}
/* end fbe_lun_init() */

/*!***************************************************************
 * fbe_lun_destroy_interface()
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
fbe_lun_destroy_interface(fbe_object_handle_t object_handle)
{
    fbe_status_t status;

    fbe_base_object_trace((fbe_base_object_t*)fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "fbe_lun_main: %s entry\n", __FUNCTION__);

    /* Destroy this object.
     */
    fbe_lun_destroy(object_handle);
    
    /* Call parent destructor.
     */
    status = fbe_base_config_destroy_object(object_handle);

    return status;
}
/* end fbe_lun_destroy_interface */

/*!***************************************************************
 * fbe_lun_destroy()
 ****************************************************************
 * @brief
 *  This function is called to destroy the lun object.
 *
 *  This function tears down everything that was created in
 *  fbe_lun_init().
 *
 * @param object_handle - The object being destroyed.
 *
 * @return fbe_status_t
 *  STATUS of the destroy operation.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_lun_destroy(fbe_object_handle_t object_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lun_t * lun_p;

    fbe_base_object_trace((fbe_base_object_t*)fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, 
                          "fbe_lun_main: %s entry\n", __FUNCTION__);
    
    lun_p = (fbe_lun_t *)fbe_base_handle_to_pointer(object_handle);

    //zero counter memory then set object_id to invalid
    lun_p->b_perf_stats_enabled = FBE_FALSE;
    if (lun_p->performance_stats.counter_ptr.lun_counters != NULL) 
    {
        fbe_perfstats_delete_system_memory_for_object_id(lun_p->performance_stats.counter_ptr.lun_counters->object_id,
                                                         FBE_PACKAGE_ID_SEP_0);
    }

    fbe_spinlock_destroy(&lun_p->io_counter_lock);
    
    return status;
}
/* end fbe_lun_destroy */

/*!**************************************************************
 * fbe_lun_initialize_verify_report()
 ****************************************************************
 * @brief
 *  This function initializes the verify report data. The verify 
 *  report is a collection of RAID stripe error count information 
 *  organized in various ways along with other pertinent information.
 *
 * @param in_lun_p      - The lun.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/19/2009 - Created. mvalois
 *
 ****************************************************************/
static void fbe_lun_initialize_verify_report(fbe_lun_t*  in_lun_p)
{
    fbe_lun_verify_report_t*   verify_report_p;

    // Initialize the LUN verify report.
    verify_report_p = fbe_lun_get_verify_report_ptr(in_lun_p);
    fbe_zero_memory((void*)verify_report_p, sizeof(fbe_lun_verify_report_t));

    verify_report_p->revision = FBE_LUN_VERIFY_REPORT_CURRENT_REVISION;

    return;
} // End fbe_lun_initialize_verify_report()


/*!**************************************************************
 * fbe_lun_verify_downstream_health()
 ****************************************************************
 * @brief
 *  This function is used to verify the health of the downstream
 *  objects, it will return any of the three return type based on
 *  state of the edge with its downstream objects and specific 
 *  RAID type.
 *
 * @param fbe_striper_t - lun object.
 *
 * @return fbe_status_t
 *  It returns any of the two return type based on state of the
 *  edges with downstream objects.
 * 
 *  1) FBE_DOWNSTREAM_HEALTH_OPTIMAL: Communication with downstream
 *                                    object is fully optimal.
 *  2) FBE_DOWNSTREAM_HEALTH_BROKEN:  Communication with downstream
 *                                    object is broken.
 *
 * @author
 *  8/14/2009 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_base_config_downstream_health_state_t 
fbe_lun_verify_downstream_health (fbe_lun_t * lun_p)
{
    fbe_base_config_downstream_health_state_t downstream_health_state;
	fbe_base_config_path_state_counters_t path_state_counters;

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

	fbe_base_config_get_path_state_counters((fbe_base_config_t *) lun_p, &path_state_counters);

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
/**********************************************
 * end fbe_lun_verify_downstream_health()
 **********************************************/


/*!**************************************************************
 * fbe_lun_set_condition_based_on_downstream_health()
 ****************************************************************
 * @brief
 *  This function will set the different condition based on the 
 *  health of the communication path with its downstream obhects.
 *
 * @param fbe_striper_t - striper object.
 * @param downstream_health_state - Health state of the dowstream 
                                    objects communication path.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  08/14/2009 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_status_t
fbe_lun_set_condition_based_on_downstream_health (fbe_lun_t *lun_p, 
                                                  fbe_base_config_downstream_health_state_t downstream_health_state)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch (downstream_health_state)
    {
        case FBE_DOWNSTREAM_HEALTH_OPTIMAL:
            break;
        case FBE_DOWNSTREAM_HEALTH_DISABLED:

            /* Downstream health broken condition will be set. */
            status = fbe_lifecycle_set_cond(&fbe_lun_lifecycle_const,
                                            (fbe_base_object_t*)lun_p,
                                            FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED);
            /* We must update the path state before any operations are allowed to fail back.
             * The upper levels expect the path state to change before I/Os get bad status. 
             */
            fbe_block_transport_server_update_path_state(&lun_p->base_config.block_transport_server,
                                                         &fbe_base_config_lifecycle_const,
                                                         (fbe_base_object_t*)lun_p,
                                                         FBE_BLOCK_PATH_ATTR_FLAGS_NONE);
            break;
        case FBE_DOWNSTREAM_HEALTH_BROKEN:
            /* Downstream health broken condition will be set. */
            status = fbe_lifecycle_set_cond(&fbe_lun_lifecycle_const,
                                            (fbe_base_object_t*)lun_p,
                                            FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
            /* We must update the path state before any operations are allowed to fail back.
             * The upper levels expect the path state to change before I/Os get bad status. 
             */
            fbe_block_transport_server_update_path_state(&lun_p->base_config.block_transport_server,
                                                         &fbe_base_config_lifecycle_const,
                                                         (fbe_base_object_t*)lun_p,
                                                         FBE_BLOCK_PATH_ATTR_FLAGS_NONE);
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Invalid base config path state: 0x%x!!\n", 
                                  __FUNCTION__,
                                  downstream_health_state);

            break;
    }

    return status;
}
/**************************************************************
 * end fbe_lun_set_condition_based_on_downstream_health()
 **************************************************************/

/*!**************************************************************
 * fbe_lun_process_block_transport_event()
 ****************************************************************
 * @brief
 *  Entry point for events from the block transport server.
 *
 * @param event_type
 * @param context
 *
 * @return 
 *
 * @author
 *  6/29/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_lun_process_block_transport_event(fbe_block_transport_event_type_t event_type, 
                                                          fbe_block_trasnport_event_context_t context)
{
    fbe_status_t status;
    fbe_lun_t *lun_p = (fbe_lun_t *)context;

    switch (event_type)
    {
        case FBE_BLOCK_TRASPORT_EVENT_TYPE_UNEXPECTED_ERROR:
            /* Trigger the condition to handle this event.
             */
            fbe_lun_inc_num_unexpected_errors(lun_p);
            fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "lun unexpected error received num_errors: 0x%x limit_hit: %d\n",
                                  lun_p->num_unexpected_errors, 
                                  fbe_lun_is_unexpected_error_limit_hit(lun_p));
            /* We need to monitor to evaluate where we should go next.
             */
            status = fbe_lifecycle_set_cond(&fbe_lun_lifecycle_const,
                                            (fbe_base_object_t*)lun_p,
                                            FBE_LUN_LIFECYCLE_COND_UNEXPECTED_ERROR);
            if (status != FBE_STATUS_OK) 
            { 
                fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s failed to set condition for unexpected errors.\n", __FUNCTION__);
                return status; 
            }
            break;
        default:
            status = fbe_base_config_process_block_transport_event(event_type, context);
            break;
    };
	return status;
}
/******************************************
 * end fbe_lun_process_block_transport_event()
 ******************************************/

/*!****************************************************************************
 * fbe_lun_reset_unexpected_errors()
 ******************************************************************************
 * @brief
 *  Reset the unexpected information of the LUN.
 *
 * @param lun_p - pointer to lun object.
 *
 * @return status - status of the operation.
 *
 * @author
 *  9/7/2012 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t
fbe_lun_reset_unexpected_errors(fbe_lun_t* lun_p)
{
    fbe_u32_t num_errors;
    fbe_lun_get_num_unexpected_errors(lun_p, &num_errors);

    fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "lun reset unexpected error limits errors: %d \n", num_errors);

    fbe_lun_reset_num_unexpected_errors(lun_p);
    return FBE_STATUS_OK;
}
/******************************
 * end fbe_lun_reset_unexpected_errors()
 ******************************/
/*******************************
 * end fbe_lun_main.c
 *******************************/
