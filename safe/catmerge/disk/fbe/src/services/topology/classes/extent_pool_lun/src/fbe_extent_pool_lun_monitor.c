/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_lun_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the lun object lifecycle code.
 * 
 *  This includes the
 *  @ref fbe_ext_pool_lun_monitor_entry "lun monitor entry point", as well as all
 *  the lifecycle defines such as rotaries and conditions, along with the actual condition
 *  functions.
 * 
 * @ingroup lun_class_files
 * 
 * @version
 *   05/22/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_ext_pool_lun_private.h"
#include "fbe/fbe_extent_pool.h"
#include "VolumeAttributes.h"
#include "fbe_database.h"
#include "fbe/fbe_service_manager_interface.h"
#include "../../../../../lib/lifecycle/fbe_lifecycle_private.h"

#include "EmcPAL_Misc.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_event_log_api.h"                      /*  for fbe_event_log_write */
#include "fbe/fbe_event_log_utils.h"                    /*  for message codes */
#include "fbe_transport_memory.h"

static fbe_atomic_t fbe_lun_monitor_io_counter;
#define FBE_MAX_LUN_CONCURRENT_REQUESTS 100

/*!***************************************************************
 * fbe_extent_pool_lun_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the lun's monitor.
 *
 * @param object_handle - This is the object handle, or in our case the pdo.
 * @param packet_p - The packet arriving from the monitor scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.             
 *
 * @author
 *  5/22/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_ext_pool_lun_monitor_entry(fbe_object_handle_t object_handle, 
                              fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_ext_pool_lun_t * lun_p = NULL;

    lun_p = (fbe_ext_pool_lun_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_base_config_monitor_crank_object(&fbe_ext_pool_lun_lifecycle_const, 
                                        (fbe_base_object_t*)lun_p, packet_p);
    return status;
}
/******************************************
 * end fbe_ext_pool_lun_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_ext_pool_lun_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the lun.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the lun's constant
 *                        lifecycle data.
 *
 * @author
 *  05/22/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_ext_pool_lun_monitor_load_verify(void)
{
    fbe_lun_monitor_io_counter = 0;
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(ext_pool_lun));
}
/******************************************
 * end fbe_ext_pool_lun_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t fbe_ext_pool_lun_downstream_health_broken_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_ext_pool_lun_negotiate_block_size_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_ext_pool_lun_downstream_health_disabled_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_status_t lun_check_local_dirty_flag_completion(fbe_packet_t * in_packet_p,
                                                          fbe_packet_completion_context_t in_context);

static fbe_status_t lun_check_peer_dirty_flag_completion(fbe_packet_t * in_packet_p,
                                                         fbe_packet_completion_context_t in_context);

static fbe_lifecycle_status_t fbe_ext_pool_lun_monitor_check_flush_for_destroy_cond_function(fbe_base_object_t* object_p, fbe_packet_t* packet_p);
static fbe_status_t fbe_ext_pool_lun_check_power_saving_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_ext_pool_lun_wait_before_hibernation_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_ext_pool_lun_get_power_save_info_from_raid_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_ext_pool_lun_monitor_initiate_verify_for_newly_bound_lun_cond_function(fbe_base_object_t * object_p, 
                                                                                                fbe_packet_t * packet_p);
static fbe_status_t fbe_lun_start_initial_verify_completion(fbe_packet_t * in_packet_p,
                                                            fbe_packet_completion_context_t in_context);

/* Initialize the metadata memory for the provision drive object. */
static fbe_lifecycle_status_t fbe_ext_pool_lun_metadata_memory_init_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* It updates nonpaged metadata with default values. */
static fbe_lifecycle_status_t fbe_ext_pool_lun_write_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* Persist the default nonpaged metadata */
static fbe_lifecycle_status_t fbe_ext_pool_lun_persist_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* Verify the signature data for the provision drive object. */
static fbe_lifecycle_status_t fbe_ext_pool_lun_metadata_verify_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* Retrieve the metadata using metadata service and initialize the provision drive object with non paged metadata. */
static fbe_lifecycle_status_t fbe_ext_pool_lun_metadata_element_init_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* It starts lun zeroing if it finds object is initialized first time and ndb flag is not set. */
static fbe_lifecycle_status_t fbe_ext_pool_lun_zero_consumed_area_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* Initialize the object's nonpaged metadata memory. */
static fbe_lifecycle_status_t fbe_ext_pool_lun_nonpaged_metadata_init_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* It updates zero checkpoint for the lu object. */
static fbe_lifecycle_status_t fbe_ext_pool_lun_update_zero_checkpoint_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);


static fbe_status_t fbe_lun_metadata_memory_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_ext_pool_lun_nonpaged_metadata_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_lun_metadata_element_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_lun_write_default_nonpaged_metadata_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_ext_pool_lun_zero_consumed_area_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_lun_update_zero_checkpoint_completion(fbe_packet_t * packet_p,  fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t fbe_ext_pool_lun_signal_first_write_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_ext_pool_lun_signal_first_write_cond_function_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t fbe_ext_pool_lun_written_update_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_lifecycle_status_t fbe_ext_pool_lun_write_default_paged_metadata_cond_function(
    fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_lifecycle_status_t fbe_ext_pool_lun_check_local_dirty_flag_cond_function(
    fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_lifecycle_status_t fbe_ext_pool_lun_check_peer_dirty_flag_cond_function(
    fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_lifecycle_status_t fbe_ext_pool_lun_mark_local_clean_cond_function(
    fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_lifecycle_status_t fbe_ext_pool_lun_mark_peer_clean_cond_function(
    fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_lifecycle_status_t fbe_ext_pool_lun_lazy_clean_cond_function(
    fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_lun_write_default_paged_metadata_cond_completion(
    fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

static fbe_status_t lun_start_dirty_verify_completion(
    fbe_packet_t * in_packet_p, fbe_packet_completion_context_t in_context);

static fbe_status_t fbe_lun_mark_local_clean_completion(
    fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

static fbe_status_t fbe_lun_mark_peer_clean_completion(
    fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t 
fbe_ext_pool_lun_passive_request_join_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_lifecycle_status_t 
fbe_ext_pool_lun_active_allow_join_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_status_t fbe_lun_persist_initial_verify_flag_completion(fbe_packet_t * in_packet_p,
                                                                   fbe_packet_completion_context_t in_context);

static fbe_status_t fbe_ext_pool_lun_check_background_operations_progress_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_status_t lun_get_rebuild_status(fbe_packet_t * packet, fbe_ext_pool_lun_t *lun_p);
static fbe_status_t lun_get_rebuild_status_completion(fbe_packet_t * packet_p,
                                                      fbe_packet_completion_context_t context);

static void lun_process_rebuild_progress(fbe_ext_pool_lun_t *lun_p, fbe_u32_t current_rebuilt_percentage);
static void lun_process_zero_progress(fbe_ext_pool_lun_t *lun_p);

static fbe_lifecycle_status_t fbe_ext_pool_lun_background_monitor_operation_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_lifecycle_status_t fbe_ext_pool_lun_unexpected_error_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_status_t fbe_lun_init_lun_extent_metadata_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_lun_mark_all_clean(fbe_ext_pool_lun_t * lun_p, 
                                      fbe_packet_t *packet_p);
static fbe_status_t 
fbe_lun_mark_all_clean_local_completion(fbe_packet_t * packet_p,
                                        fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t fbe_ext_pool_lun_packet_cancelled_cond_function(fbe_base_object_t * object_p, 
                                                                 fbe_packet_t * packet_p);


static fbe_lifecycle_status_t fbe_ext_pool_lun_verify_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_status_t fbe_lun_initiate_verify_completion(fbe_packet_t * in_packet_p,
                                                                   fbe_packet_completion_context_t in_context);
static fbe_bool_t fbe_lun_update_zero_checkpoint_need_to_run(fbe_ext_pool_lun_t *lun_p, fbe_packet_t * packet_p);

/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(ext_pool_lun);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(ext_pool_lun);

/*  lun_lifecycle_callbacks
 *  This variable has all the callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(ext_pool_lun) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        ext_pool_lun,
        FBE_LIFECYCLE_NULL_FUNC,          /* online function */
        fbe_ext_pool_lun_pending_func)/* pending function */        
};

/*--- constant derived condition entries -----------------------------------------------*/

static fbe_lifecycle_const_cond_t lun_downstream_health_broken_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN,
        fbe_ext_pool_lun_downstream_health_broken_cond_function)
};

static fbe_lifecycle_const_cond_t lun_downstream_health_disabled_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED,
        fbe_ext_pool_lun_downstream_health_disabled_cond_function)
};

/* This condition is overloaded by the LUN, but 
 * the condition to handle it is the same functionality as for the 
 * health disabled condition. 
 */
static fbe_lifecycle_const_cond_t lun_downstream_health_not_optimal_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_NOT_OPTIMAL,
        fbe_ext_pool_lun_downstream_health_disabled_cond_function)
};


static fbe_lifecycle_const_cond_t lun_negotiate_block_size_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_NEGOTIATE_BLOCK_SIZE,
        fbe_ext_pool_lun_negotiate_block_size_cond_function)
};

/* lun metadata memory initialization condition function. */
static fbe_lifecycle_const_cond_t lun_metadata_memory_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_MEMORY_INIT,
        fbe_ext_pool_lun_metadata_memory_init_cond_function)
};

/* lun nonpaged metadata initialization condition function. */
static fbe_lifecycle_const_cond_t lun_nonpaged_metadata_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_NONPAGED_METADATA_INIT,
        fbe_ext_pool_lun_nonpaged_metadata_init_cond_function)         
};

/* lun metadata metadata record init condition function. */
static fbe_lifecycle_const_cond_t lun_metadata_element_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_ELEMENT_INIT,
        fbe_ext_pool_lun_metadata_element_init_cond_function)
};

/* lun metadata signature verification condition function. */
static fbe_lifecycle_const_cond_t lun_metadata_verify_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_VERIFY,
        fbe_ext_pool_lun_metadata_verify_cond_function)
};

/* lun metadata default nonpaged write condition function. */
static fbe_lifecycle_const_cond_t lun_write_default_nonpaged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_NONPAGED_METADATA,
        fbe_ext_pool_lun_write_default_nonpaged_metadata_cond_function)
};

/* lun metadata default nonpaged persist condition function. */
static fbe_lifecycle_const_cond_t lun_persist_default_nonpaged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_PERSIST_DEFAULT_NONPAGED_METADATA,
        fbe_ext_pool_lun_persist_default_nonpaged_metadata_cond_function)
};

/* lun zero consumed space condition function. */
static fbe_lifecycle_const_cond_t lun_zero_consumed_area_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_ZERO_CONSUMED_AREA,
        fbe_ext_pool_lun_zero_consumed_area_cond_function)
};

/* lun metadata default paged write condition function. */
static fbe_lifecycle_const_cond_t lun_write_default_paged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_PAGED_METADATA,
        fbe_ext_pool_lun_write_default_paged_metadata_cond_function)
};

static fbe_lifecycle_const_cond_t lun_passive_request_join_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_PASSIVE_REQUEST_JOIN,
        fbe_ext_pool_lun_passive_request_join_cond_function)
};

static fbe_lifecycle_const_cond_t lun_active_allow_join_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_ACTIVE_ALLOW_JOIN,
        fbe_ext_pool_lun_active_allow_join_cond_function)
};

/* raid group common background monitor operation condition function. */
static fbe_lifecycle_const_cond_t lun_background_monitor_operation_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_BACKGROUND_MONITOR_OPERATION,
        fbe_ext_pool_lun_background_monitor_operation_cond_function)
};

static fbe_lifecycle_const_cond_t lun_packet_cancelled_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_OBJECT_LIFECYCLE_COND_PACKET_CANCELED,
        fbe_ext_pool_lun_packet_cancelled_cond_function)
};


/*--- constant base condition entries --------------------------------------------------*/

/* lun_check_flush_for_destroy
 * FBE_LUN_LIFECYCLE_COND_LUN_FLUSH_FOR_DESTROY
 * The purpose of this condition is to check whether the lun is flushing
 * I/O in preparation for LUN destroy.  If no longer flushing, the
 * corresponding usurper packet is found on usurper queue and completed 
 * so LUN destroy can continue.
 */
static fbe_lifecycle_const_base_cond_t lun_check_flush_for_destroy = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "lun_check_flush_for_destroy",
        FBE_LUN_LIFECYCLE_COND_LUN_CHECK_FLUSH_FOR_DESTROY,
        fbe_ext_pool_lun_monitor_check_flush_for_destroy_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,            /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,            /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_timer_cond_t lun_wait_before_hibernation = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "lun_wait_before_hibernation",
            FBE_LUN_LIFECYCLE_COND_LUN_WAIT_BEFORE_HIBERNATION,
            fbe_ext_pool_lun_wait_before_hibernation_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,           /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,       /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    300 /* fires every 3 seconds */
};


static fbe_lifecycle_const_base_cond_t lun_get_power_save_info_from_raid = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "lun_get_power_save_info_from_raid",
        FBE_LUN_LIFECYCLE_COND_LUN_GET_POWER_SAVE_INFO_FROM_RAID,
        fbe_ext_pool_lun_get_power_save_info_from_raid_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,            /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,            /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* verify_new_lun
 * FBE_LUN_LIFECYCLE_COND_VERIFY_NEW_LUN
 * The purpose of this condition is to initate a 
 *  verify operation when the lun is newly bound.
 */
static fbe_lifecycle_const_base_cond_t verify_new_lun_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "verify_new_lun",
        FBE_LUN_LIFECYCLE_COND_VERIFY_NEW_LUN,
        fbe_ext_pool_lun_monitor_initiate_verify_for_newly_bound_lun_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,       /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,          /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,      /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,        /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,           /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};

/* FBE_LUN_LIFECYCLE_COND_LUN_SIGNAL_FIRST_WRITE
 * The purpose of this condition is to notify upper layeres the first write had been made to the LUN
 */
static fbe_lifecycle_const_base_cond_t lu_signal_first_write_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "lu_signal_first_write_cond",
        FBE_LUN_LIFECYCLE_COND_LUN_SIGNAL_FIRST_WRITE,
        fbe_ext_pool_lun_signal_first_write_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,             /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,            /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_LUN_LIFECYCLE_COND_LUN_MARK_LUN_WRITTEN
 * The purpose of this condition is to mark the edge on startup if this lun has user data
 */
static fbe_lifecycle_const_base_cond_t lu_written_update_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "lu_written_update_cond",
        FBE_LUN_LIFECYCLE_COND_LUN_MARK_LUN_WRITTEN,
        fbe_ext_pool_lun_written_update_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* fbe_lun_lazy_clean_cond
 * FBE_LUN_LIFECYCLE_COND_LUN_MARK_LUN_CLEAN
 * The purpose of this condition is to mark the LUN as clean if it has not received
 * non-cached writes within the past few seconds.
 */
static fbe_lifecycle_const_base_cond_t lun_lazy_clean_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_lun_lazy_clean_cond",
        FBE_LUN_LIFECYCLE_COND_LAZY_CLEAN,
        fbe_ext_pool_lun_lazy_clean_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};


/* FBE_LUN_LIFECYCLE_COND_CHECK_LOCAL_DIRTY_FLAG
 * 
 */
static fbe_lifecycle_const_base_cond_t lun_check_local_dirty_flag_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "lun_check_local_dirty_flag_cond",
        FBE_LUN_LIFECYCLE_COND_CHECK_LOCAL_DIRTY_FLAG,
        fbe_ext_pool_lun_check_local_dirty_flag_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};


/* FBE_LUN_LIFECYCLE_COND_CHECK_PEER_DIRTY_FLAG
 * 
 */
static fbe_lifecycle_const_base_cond_t lun_check_peer_dirty_flag_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "lun_check_peer_dirty_flag_cond",
        FBE_LUN_LIFECYCLE_COND_CHECK_PEER_DIRTY_FLAG,
        fbe_ext_pool_lun_check_peer_dirty_flag_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_LUN_LIFECYCLE_COND_MARK_LOCAL_CLEAN
 * 
 */
static fbe_lifecycle_const_base_cond_t lun_mark_local_clean_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_lun_mark_local_clean_cond",
        FBE_LUN_LIFECYCLE_COND_MARK_LOCAL_CLEAN,
        fbe_ext_pool_lun_mark_local_clean_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_LUN_LIFECYCLE_COND_MARK_PEER_CLEAN
 * 
 */
static fbe_lifecycle_const_base_cond_t lun_mark_peer_clean_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_lun_mark_peer_clean_cond",
        FBE_LUN_LIFECYCLE_COND_MARK_PEER_CLEAN,
        fbe_ext_pool_lun_mark_peer_clean_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* lun_unexpected_error_cond
 * FBE_LUN_LIFECYCLE_COND_UNEXPECTED_ERROR
 * The purpose of this condition is to send an edge state transition up 
 * to make the cache happy.  It panics if it does not see an edge state 
 * transition on I/O failures. 
 * We also check to see if we hit the threshold to FAIL the LUN. 
 */
static fbe_lifecycle_const_base_cond_t lun_unexpected_error_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "lun_unexpected_error_cond",
        FBE_LUN_LIFECYCLE_COND_UNEXPECTED_ERROR,
        fbe_ext_pool_lun_unexpected_error_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/*LUN Verify*/
static fbe_lifecycle_const_base_cond_t lun_verify_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_lun_verify_cond",
        FBE_LUN_LIFECYCLE_COND_VERIFY_LUN,
        fbe_ext_pool_lun_verify_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};


/* lun_lifecycle_base_cond_array
 * This is our static list of base conditions.
 */
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(ext_pool_lun)[] = {
    &lun_check_flush_for_destroy,
    (fbe_lifecycle_const_base_cond_t *)&lun_wait_before_hibernation,
    &lun_get_power_save_info_from_raid,
    &verify_new_lun_cond,
    &lu_signal_first_write_cond,
    &lu_written_update_cond,
    (fbe_lifecycle_const_base_cond_t *)&lun_lazy_clean_cond,
    &lun_check_local_dirty_flag_cond,
    &lun_check_peer_dirty_flag_cond,
    &lun_mark_local_clean_cond,
    &lun_mark_peer_clean_cond,
    &lun_unexpected_error_cond,
    &lun_verify_cond,
};



/* lun_lifecycle_base_conditions 
 *  This is the list of base default base conditions for the lun
 */
FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(ext_pool_lun);


/*--- constant rotaries ----------------------------------------------------------------*/
static fbe_lifecycle_const_rotary_cond_t lun_specialize_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_downstream_health_not_optimal_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_negotiate_block_size_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_metadata_memory_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_nonpaged_metadata_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_metadata_element_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    /*!@note the following four conditions needs to be in order, do not change the order without a valid reason */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_write_default_nonpaged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_zero_consumed_area_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_persist_default_nonpaged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_write_default_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_mark_local_clean_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_mark_peer_clean_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_metadata_verify_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_get_power_save_info_from_raid, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    /* Base conditions */
};

static fbe_lifecycle_const_rotary_cond_t lun_activate_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_downstream_health_disabled_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
                                        
    /* Base conditions */
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_check_local_dirty_flag_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_check_peer_dirty_flag_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_mark_local_clean_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_mark_peer_clean_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lu_written_update_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_get_power_save_info_from_raid, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_passive_request_join_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t lun_ready_rotary[] = {
    /* Derived conditions */
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_get_power_save_info_from_raid, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_check_power_saving_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_wait_before_hibernation, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_packet_cancelled_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    
    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(verify_new_lun_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_check_flush_for_destroy, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lu_signal_first_write_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_check_peer_dirty_flag_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_mark_peer_clean_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_unexpected_error_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_active_allow_join_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_lazy_clean_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_verify_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

     /* This is always the last condition in the ready rotary.  This condition is never cleared. 
     * Please, do put any condition after this one !!
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_background_monitor_operation_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};
static fbe_lifecycle_const_rotary_cond_t lun_hibernate_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_get_power_save_info_from_raid, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
  
};

static fbe_lifecycle_const_rotary_cond_t lun_fail_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(lun_downstream_health_broken_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Base conditions */
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(ext_pool_lun)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, lun_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, lun_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, lun_ready_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_HIBERNATE, lun_hibernate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_FAIL, lun_fail_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(ext_pool_lun);

/*--- global lun lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(ext_pool_lun) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        ext_pool_lun,
        FBE_CLASS_ID_EXTENT_POOL_LUN,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_config))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/

/*--- Condition Functions --------------------------------------------------------------*/

/*!**************************************************************
 * fbe_ext_pool_lun_downstream_health_broken_cond_function()
 ****************************************************************
 * @brief
 *  This is the derived condition function for LUN object where 
 *  it will clear this condition when it finds health of the 
 *  downstream object is in ENABLE state.
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is be based on parent
 *                                  function call.
 * call.
 *
 * @author
 *  8/18/2009 - Created. Dhaval Patel.
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_ext_pool_lun_downstream_health_broken_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_ext_pool_lun_t * lun_p = (fbe_ext_pool_lun_t *)object_p;
    fbe_status_t status;
    fbe_base_config_downstream_health_state_t downstream_health_state;

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Look for all the edges with downstream objects.
       */
    downstream_health_state = fbe_ext_pool_lun_verify_downstream_health(lun_p);

    switch (downstream_health_state)
    {
        case FBE_DOWNSTREAM_HEALTH_OPTIMAL:
        case FBE_DOWNSTREAM_HEALTH_DISABLED:

            if (fbe_ext_pool_lun_is_unexpected_error_limit_hit(lun_p)){

                fbe_u32_t num_errors;
                fbe_lun_get_num_unexpected_errors(lun_p, &num_errors);

                /* Stay in failed until someone clears this.
                 */
                fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "lun unexpected error limit is not cleared num_errors %d\n",
                                      num_errors);
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            status = fbe_ext_pool_lun_clear_clustered_flag(lun_p, FBE_LUN_CLUSTERED_FLAG_THRESHOLD_REACHED);
            if (status != FBE_STATUS_OK) {
                fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "lun unable to clear clustered flag\n");
                return FBE_LIFECYCLE_STATUS_DONE;
            }/* Currently clear the condition in both the cases, later logic
             * will be changed to integrate different use cases.
             */
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);
            if (status != FBE_STATUS_OK) {
                fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s can't clear current condition, status: 0x%X\n",
                                        __FUNCTION__, status);
            }
            fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const, (fbe_base_object_t *)lun_p, FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
            break;
        case FBE_DOWNSTREAM_HEALTH_BROKEN:
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Invalid downstream health state: 0x%x!!\n", 
                                  __FUNCTION__,
                                  downstream_health_state);
            break;
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/*************************************************
 * end fbe_ext_pool_lun_downstream_health_broken_cond_function()
 *************************************************/

/*!**************************************************************
 * fbe_ext_pool_lun_negotiate_block_size_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function for lun to kick off
 *  the fetching of negotiate block size information.
 *  This is a derived condition from the config object, which
 *  must be implemented.
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING   
 *
 * @author
 *  9/23/2009 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_ext_pool_lun_negotiate_block_size_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_ext_pool_lun_t * lun_p = (fbe_ext_pool_lun_t *)object_p;
    fbe_payload_ex_t                * payload_p = NULL;
    fbe_block_edge_t                * block_edge_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_raid_group_get_info_t           get_info;

    payload_p = fbe_transport_get_payload_ex(packet_p);

    lun_p->alignment_boundary = 0;

    fbe_lun_get_block_edge(lun_p, &block_edge_p);

    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_RAID_GROUP_CONTROL_CODE_GET_INFO,
                                        &get_info,
                                        sizeof(fbe_raid_group_get_info_t));

    fbe_payload_ex_increment_control_operation_level(payload_p); 
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /*send it*/
    fbe_block_transport_send_control_packet(block_edge_p, packet_p);

    fbe_transport_wait_completion(packet_p);

    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

    /*when we get here the data is there for us to use(in case the packet completed nicely*/
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to get raid group info\n", __FUNCTION__);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Break I/O on the parity stripe.
     */
    if (get_info.elements_per_parity_stripe == 0)
    {
        /* Non parity will break on our largest parity stripe size.
         */
        lun_p->alignment_boundary = get_info.num_data_disk * FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH * FBE_RAID_ELEMENTS_PER_PARITY_BANDWIDTH;
    }
    else
    {
        lun_p->alignment_boundary = get_info.num_data_disk * get_info.element_size * get_info.elements_per_parity_stripe;
    }

    fbe_base_object_trace((fbe_base_object_t *)lun_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "LUN alignment_boundary: 0x%llX\n", lun_p->alignment_boundary);

    status = fbe_lifecycle_clear_current_cond(object_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(object_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%x",
                              __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************
 * end fbe_ext_pool_lun_negotiate_block_size_cond_function()
 **************************************/

/*!**************************************************************
 * fbe_extent_pool_attach_edge()
 ****************************************************************
 * @brief
 *  This function sets up the basic edge configuration info
 *  for this extent pool object.
 *  We also attempt to attach the edge.
 *
 * @param extent_pool_p - Pointer to extent pool object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/06/2014 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t
fbe_ext_pool_lun_attach_edge(fbe_ext_pool_lun_t * ext_pool_lun_p, 
                             fbe_u32_t server_index, 
                             fbe_object_id_t server_id, 
                             fbe_lba_t capacity, 
                             fbe_lba_t offset, 
                             fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_block_edge_t *edge_p = NULL;
    fbe_object_id_t my_object_id;
    fbe_block_transport_control_attach_edge_t attach_edge;
    fbe_package_id_t my_package_id;

    fbe_base_object_trace( (fbe_base_object_t *)ext_pool_lun_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Now attach the edge. */
    fbe_base_object_get_object_id((fbe_base_object_t *)ext_pool_lun_p, &my_object_id);

    fbe_base_config_get_block_edge((fbe_base_config_t *)ext_pool_lun_p, &edge_p, 0);
    fbe_block_transport_set_transport_id(edge_p);
    fbe_block_transport_set_server_id(edge_p, server_id);
    fbe_block_transport_set_client_id(edge_p, my_object_id);
    fbe_block_transport_set_client_index(edge_p, 0);
    fbe_block_transport_set_server_index(edge_p, server_index);
    fbe_block_transport_edge_set_capacity(edge_p, capacity); /* This needs to be changed!!! */
    fbe_block_transport_edge_set_offset(edge_p, offset);

    /*first of all we need to check what is the lowest latency time on the edges above us, the lowest one wins*/
    //fbe_block_transport_edge_set_time_to_become_ready(edge_p, lowest_latency_time_in_seconds);

    attach_edge.block_edge = edge_p;

    fbe_get_package_id(&my_package_id);
    fbe_block_transport_edge_set_client_package_id(edge_p, my_package_id);

    status = fbe_base_config_send_attach_edge((fbe_base_config_t *)ext_pool_lun_p, packet_p, &attach_edge) ;
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace(  (fbe_base_object_t *)ext_pool_lun_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_base_config_attach_downstream_block_edge failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_extent_pool_attach_edge()
 ******************************************************************************/

static fbe_lifecycle_status_t 
fbe_ext_pool_lun_downstream_health_disabled_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_ext_pool_lun_t * lun = (fbe_ext_pool_lun_t *)object_p;
    fbe_base_config_downstream_health_state_t downstream_health_state;
    fbe_path_state_t	path_state;
    fbe_object_id_t     pool_object_id;
    fbe_object_id_t     lun_object_id;
    fbe_u32_t           server_index;
    fbe_lba_t           capacity, offset;
    fbe_status_t        status;

    fbe_base_object_trace((fbe_base_object_t*)lun,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    fbe_block_transport_get_path_state(&((fbe_base_config_t *) object_p)->block_edge_p[0], &path_state);
    if (path_state == FBE_PATH_STATE_INVALID) {
        fbe_base_object_get_object_id(object_p, &lun_object_id);
        status = fbe_database_get_ext_pool_lun_config_info(lun_object_id, &server_index, &capacity, &offset, &pool_object_id);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)lun, 
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s DS objects doesn't exist yet\n", __FUNCTION__);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        fbe_base_config_set_capacity(&lun->base_config, capacity);
        fbe_extent_pool_class_get_blocks_per_slice(&lun->alignment_boundary);
        /* For now we will assume 4+1, so split on multiple of 4 * disk slice size.
         */
        lun->alignment_boundary *= 4;
        fbe_base_object_trace((fbe_base_object_t*)lun, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Ext Pool LUN: Attach DS object server index: %u offset: 0x%llx\n", server_index, offset);
        status = fbe_ext_pool_lun_attach_edge(lun, server_index, pool_object_id, capacity, offset, packet_p);
        if (status != FBE_STATUS_OK) 
        {
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    downstream_health_state = fbe_ext_pool_lun_verify_downstream_health(lun);
    if(downstream_health_state == FBE_DOWNSTREAM_HEALTH_OPTIMAL){

        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun);
    }

    

    return FBE_LIFECYCLE_STATUS_DONE;
}


/*!**************************************************************
 * fbe_ext_pool_lun_monitor_initiate_verify_for_newly_bound_lun_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function to mark the lun for error verify
 *  when the lun is bound the first time
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING   
 *
 * @author
 *  10/11/2010 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/

static fbe_lifecycle_status_t 
fbe_ext_pool_lun_monitor_initiate_verify_for_newly_bound_lun_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t                   status; 
    fbe_ext_pool_lun_t*                     lun_p;
    fbe_lun_initiate_verify_t      initiate_verify;
    fbe_lba_t                      block_count;
    
    lun_p  = (fbe_ext_pool_lun_t *)object_p;

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    // This is an imported capacity of the lun
    fbe_lun_get_imported_capacity(lun_p, &block_count);

    initiate_verify.start_lba = 0;
    initiate_verify.block_count = block_count;
    initiate_verify.verify_type = FBE_LUN_VERIFY_TYPE_SYSTEM;

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s - start\n", 
                          __FUNCTION__);

    status = fbe_transport_set_completion_function(packet_p, 
                                                   fbe_lun_start_initial_verify_completion, 
                                                   lun_p);

   // Tell the RAID object to initialize its verify data for this lun.
    status = fbe_ext_pool_lun_usurper_send_initiate_verify_to_raid(lun_p, packet_p, &initiate_verify);

    return FBE_LIFECYCLE_STATUS_PENDING;

}
/**************************************
 * end fbe_ext_pool_lun_monitor_initiate_verify_for_newly_bound_lun_cond_function()
 **************************************/


/*!**************************************************************
 * fbe_lun_start_initial_verify_completion()
 ****************************************************************
 * @brief
 *  This is the condition function to mark the lun for error verify
 *  when the lun is bound the first time.
 * 
 * @param packet_p - monitor packet
 * @param context - Completion context.
 *
 * @return fbe_lifecycle_status_t 
 *
 * @author
 *  10/11/2010 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/

static fbe_status_t 
fbe_lun_start_initial_verify_completion(fbe_packet_t * packet_p,
                                        fbe_packet_completion_context_t context)
{
    fbe_status_t                   status; 
    fbe_ext_pool_lun_t*                     lun_p;
    
    
    lun_p  = (fbe_ext_pool_lun_t *)context;

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_transport_get_status_code(packet_p);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s : Error in initiating initial verify 0x%x\n", __FUNCTION__, status);
        return FBE_STATUS_OK;
    }
    else
    {
        /* If the mark succeeded, kick off the nonpaged persist of the 
         * flag that we performed the initial verify. 
         */
        fbe_u8_t flags = FBE_LUN_NONPAGED_FLAGS_INITIAL_VERIFY_RAN;
        status = fbe_transport_set_completion_function(packet_p, 
                                                       fbe_lun_persist_initial_verify_flag_completion, 
                                                       lun_p);
        status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) lun_p,
                                                                 packet_p,
                                                                 (fbe_u64_t)(&((fbe_lun_nonpaged_metadata_t*)0)->flags),
                                                                 (fbe_u8_t *)&flags,
                                                                 sizeof(fbe_lun_nonpaged_flags_t));  
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
}
/**************************************
 * end fbe_lun_start_initial_verify_completion()
 **************************************/
/*!**************************************************************
 * fbe_lun_persist_initial_verify_flag_completion()
 ****************************************************************
 * @brief
 *  This is the completion for persisting the initial verify was run flag.
 * 
 * @param packet_p - monitor packet
 * @param context - Completion context.
 *
 * @return fbe_status_t 
 *
 * @author
 *  3/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t 
fbe_lun_persist_initial_verify_flag_completion(fbe_packet_t * packet_p,
                                               fbe_packet_completion_context_t context)
{
    fbe_status_t                   status; 
    fbe_ext_pool_lun_t*                     lun_p = (fbe_ext_pool_lun_t *)context;

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_transport_get_status_code(packet_p);

   /*! On a metadata failure, cause the condition to run again by returning the packet without clearing the condition.
    */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s : Error in initiating initial verify 0x%x\n", __FUNCTION__, status);
    }
    else
    {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);    
    }
    
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_lun_persist_initial_verify_flag_completion()
 **************************************/

/*!**************************************************************
 * fbe_ext_pool_lun_monitor_check_flush_for_destroy_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function for lun to determine if
 *  I/O to it is being flushed in preparation for destroying
 *  the LUN. If so, there is nothing to do; otherwise, if I/O 
 *  flushing has completed, the usurper packet that initiated the 
 *  flush must be found and completed.
 *
 * @param object_p - current object
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING   
 *
 * @author
 *  03/2010 - Created. rundbs
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_ext_pool_lun_monitor_check_flush_for_destroy_cond_function(fbe_base_object_t* object_p, 
                                                      fbe_packet_t* packet_p)
{
    fbe_status_t                                status;
    fbe_ext_pool_lun_t*                                  lun_p = (fbe_ext_pool_lun_t *)object_p;
    fbe_packet_t*                               usurper_packet;
    fbe_payload_control_operation_opcode_t      control_code;
    fbe_queue_element_t*                        queue_element = NULL;
    fbe_queue_head_t*                           queue_head = fbe_base_object_get_usurper_queue_head(object_p);


    /* If I/O is still in progress on the LUN, return immediately; flushing
     * is not complete.
     */
    if (lun_p->base_config.block_transport_server.outstanding_io_count)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If got this far, I/O to the LUN has been flushed.  The usurper packet 
     * that initiated the flush can now be completed.  The usurper packet is 
     * waiting on the usurper queue.
     */
    fbe_base_object_usurper_queue_lock(object_p);

    queue_element = fbe_queue_front(queue_head);
    while (queue_element)
    {
        usurper_packet = fbe_transport_queue_element_to_packet(queue_element);
        control_code = fbe_transport_get_control_code(usurper_packet);

        if (control_code == FBE_LUN_CONTROL_CODE_PREPARE_TO_DESTROY_LUN)
        {
            /* Remove element from usurper queue.
             * Note that the remove function takes out the usurper queue lock
             * and releases it when the object has been removed.
             */
            fbe_base_object_usurper_queue_unlock(object_p);
            fbe_base_object_remove_from_usurper_queue(object_p, usurper_packet);

            /* Complete usurper packet */
            fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(usurper_packet);

            /* Clear condition and return */
            status = fbe_lifecycle_clear_current_cond(object_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)object_p, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%x\n",
                                    __FUNCTION__, status);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)object_p, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                                    "%s flushing complete.\n",
                                    __FUNCTION__);
            }

            return FBE_LIFECYCLE_STATUS_DONE;
        }
        else
        {
            queue_element = fbe_queue_next(queue_head, queue_element);
        }
    }

    /* If got this far, the usurper packet was not on the queue */

    fbe_base_object_usurper_queue_unlock(object_p);

    fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                             "%s Critical error; cannot find usurper command on queue for LUN destroy\n", __FUNCTION__);

    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************
 * end fbe_ext_pool_lun_monitor_check_flush_for_destroy_cond_function()
 **************************************/
/*!****************************************************************************
 * fbe_ext_pool_lun_metadata_memory_init_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the metadata memory information for the
 *  LUN object.
 *
 * @param in_object_p               - pointer to a base object 
 * @param in_packet_p               - pointer to a control packet from the scheduler.
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when metadata
 *                                    memory init issued.
 *
 * @author
 *  3/24/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_ext_pool_lun_metadata_memory_init_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_ext_pool_lun_t * lun_p = NULL;

    lun_p = (fbe_ext_pool_lun_t *)object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);
    /* Set the completion function for the metadata memory initialization. */
    fbe_transport_set_completion_function(packet_p, fbe_lun_metadata_memory_init_cond_completion, lun_p);

    /* Initialize the metadata memory for the LUN object. */
    fbe_base_config_metadata_init_memory((fbe_base_config_t *) lun_p, 
                                            packet_p,
                                            &lun_p->lun_metadata_memory, 
                                            &lun_p->lun_metadata_memory_peer, 
                                            sizeof(fbe_lun_metadata_memory_t));
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_ext_pool_lun_metadata_memory_init_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_metadata_memory_init_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the metadata memory
 *  initialization.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  3/23/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_lun_metadata_memory_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_ext_pool_lun_t *                         lun_p = NULL;
    fbe_payload_ex_t *                     payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    lun_p = (fbe_ext_pool_lun_t *)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* If status is good then clear the metadata memory initialization
     * condition. 
     */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {    
        /* Clear the metadata memory initialization condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);
        if (status != FBE_STATUS_OK){
            fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }

    /* If it returns busy status then do not clear the condition and try to 
     * initialize the metadata memory in next monitor cycle.
     */
    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_BUSY) {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, payload status is FBE_PAYLOAD_CONTROL_STATUS_BUSY",
                                __FUNCTION__);
        
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_metadata_memory_init_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_nonpaged_metadata_init_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the lun object's nonpaged metadata
 *  memory.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  7/21/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_ext_pool_lun_nonpaged_metadata_init_cond_function(fbe_base_object_t * object_p,
                                             fbe_packet_t * packet_p)
{
    fbe_ext_pool_lun_t *         lun_p = NULL;
    fbe_status_t        status;

    lun_p = (fbe_ext_pool_lun_t *)object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* set the completion function for the nonpaged metadata init condition. */
    fbe_transport_set_completion_function(packet_p, fbe_ext_pool_lun_nonpaged_metadata_init_cond_completion, lun_p);

    /* initialize signature for the raid group object. */
    status = fbe_ext_pool_lun_nonpaged_metadata_init(lun_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_ext_pool_lun_nonpaged_metadata_init_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_nonpaged_metadata_init_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the nonpaged metadata init
 *  condition.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  7/21/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_ext_pool_lun_nonpaged_metadata_init_cond_completion(fbe_packet_t * packet_p,
                                               fbe_packet_completion_context_t context)
{
    fbe_ext_pool_lun_t *                         lun_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    lun_p = (fbe_ext_pool_lun_t*)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* clear nonpaged metadata init condition if status is good. */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) lun_p);

        /* reschedule immediately during object initialization. */
        fbe_lifecycle_reschedule(&fbe_ext_pool_lun_lifecycle_const,
                                 (fbe_base_object_t *) lun_p,
                                 (fbe_lifecycle_timer_msec_t) 0);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s nonpaged metadata init condition failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_ext_pool_lun_nonpaged_metadata_init_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_metadata_element_init_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the metadata element for the lun
 *  object.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when signature
 *                                    verification issued.
 *
 * @author
 *  7/21/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_ext_pool_lun_metadata_element_init_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_ext_pool_lun_t      *lun_p = NULL;
    fbe_object_id_t my_object_id;
    fbe_object_id_t np_lun_object_id = FBE_OBJECT_ID_INVALID;

    lun_p = (fbe_ext_pool_lun_t*)object_p;
    fbe_base_object_get_object_id((fbe_base_object_t *)lun_p, &my_object_id);
    
    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Check if this the non-paged lun
     */
    fbe_database_get_metadata_lun_id(&np_lun_object_id);
    if (my_object_id == np_lun_object_id) {
        fbe_lun_set_flag(lun_p, FBE_LUN_FLAGS_DISABLE_CLEAN_DIRTY);
        lun_p->marked_dirty;
    }

    /* Set the completion function for the metadata record initialization. */
    fbe_transport_set_completion_function(packet_p, fbe_lun_metadata_element_init_cond_completion, lun_p);

    /* Initialize the lun metadata record. */
    fbe_ext_pool_lun_metadata_initialize_metadata_element(lun_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_ext_pool_lun_metadata_element_init_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_metadata_element_init_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the metadata element 
 *  initialization condition.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  7/21/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_lun_metadata_element_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_ext_pool_lun_t *                  lun_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t status;
    
    lun_p = (fbe_ext_pool_lun_t*)context;

    /* Get the status of the packet. */
    status = fbe_transport_get_status_code(packet_p);

    /* Get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* Clear the condition if status is ok. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {    
        /* clear the metadata element init condition. */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*) lun_p);

        /* reschedule immediately during object initialization. */
        fbe_lifecycle_reschedule(&fbe_ext_pool_lun_lifecycle_const,
                                 (fbe_base_object_t *) lun_p,
                                 (fbe_lifecycle_timer_msec_t) 0);
        fbe_topology_clear_gate(lun_p->base_config.base_object.object_id);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s metadata element init condition failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_metadata_element_init_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_metadata_verify_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to verify the LUN's metadata initialization.
 *  It verify that if non paged metadata is not initialized than it set multiple 
 *  conditions to initialize LUN's metadata.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when metadata
 *                                    verification done.
 *
 * @author
 *  3/23/2010 - Created. Dhaval Patel
 *  5/16/2012 - Modified by Jingcheng Zhang for metadata versioning
 *  5/8/2012  - Modified Zhang Yang. Add NP magic munber check
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_ext_pool_lun_metadata_verify_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_ext_pool_lun_t * lun_p = NULL;
    fbe_bool_t is_metadata_initialized;
    fbe_bool_t is_active;
    fbe_u32_t  disk_version_size = 0;
    fbe_u32_t  memory_version_size = 0;    
    fbe_base_config_nonpaged_metadata_t    *base_config_nonpaged_metadata_p = NULL;

    fbe_bool_t is_magic_num_valid;
    
    lun_p = (fbe_ext_pool_lun_t *)object;
    /*check the NP state .
        If it is INVALID, the NP is not load correctly
        the object will stuck in this condition function until 
        its NP is set correctly by caller*/
    fbe_base_config_metadata_is_nonpaged_state_valid((fbe_base_config_t *) object, &is_magic_num_valid);
    if(is_magic_num_valid == FBE_FALSE)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    fbe_base_config_metadata_is_initialized((fbe_base_config_t *) object, &is_metadata_initialized);
    is_active = fbe_base_config_is_active((fbe_base_config_t *)object);

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s is_metdata_init: %d is_active: %d\n", 
                          __FUNCTION__, is_metadata_initialized, is_active);


    /*version check.  the non-paged metadata on disk may have different version with current
      non-paged metadata structure in memory. compare the size and set default value for the
      new version in memory*/
    if (is_metadata_initialized) {
        fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)lun_p, (void **)&base_config_nonpaged_metadata_p);
        fbe_base_config_get_nonpaged_metadata_size((fbe_base_config_t *)lun_p, &memory_version_size);
        fbe_base_config_nonpaged_metadata_get_version_size(base_config_nonpaged_metadata_p, &disk_version_size);
        if (disk_version_size != memory_version_size) {
            /*new software is operating old version data during upgrade, set default value for the new version*/
            /*currently these region already zerod, the new version developer should set correct value here*/
            fbe_base_object_trace((fbe_base_object_t*)lun_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "metatadata verify detect version un-match here, should set default value here. disk size %d, memory size %d\n",
                              disk_version_size, memory_version_size);            
        }
    }


    if (!is_active){ /* This will take care of passive side */
        if(is_metadata_initialized == FBE_TRUE)
        {
            fbe_lifecycle_clear_current_cond((fbe_base_object_t *) lun_p);
            fbe_base_config_clear_clustered_flag((fbe_base_config_t *) lun_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST);
        }
        else
        {
            /* Ask again for the nonpaged metadata update */
            if(!fbe_base_config_is_clustered_flag_set((fbe_base_config_t *) lun_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST))
            {
                fbe_base_config_set_clustered_flag((fbe_base_config_t *) lun_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST);
            }
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if (!is_metadata_initialized) /* We active and newly created */
    { 
        fbe_base_object_trace((fbe_base_object_t*)lun_p,
                      FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s We active and newly created\n", __FUNCTION__);

        /* Set all relevant conditions */

        /* We are initializing the object first time and so update the 
         * paged metadata, signature and then non paged metadata in order.
         */

        /* set the nonpaged metadata init condition. */
        fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const,
                               (fbe_base_object_t *)lun_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_NONPAGED_METADATA);

#if 0
        /* set the zero consumed area condition. */
        fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const,
                              (fbe_base_object_t *)lun_p,
                              FBE_BASE_CONFIG_LIFECYCLE_COND_ZERO_CONSUMED_AREA);

        /* set the paged metadata init condition. */
        /*! @note - Even though we dont have real paged metadata region, but still there is 
         * some private LUN information that needs to be initialized and so we will use this
         * condition to initialize the lun dirty flags
         */
        
        fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const,
                               (fbe_base_object_t *)lun_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_PAGED_METADATA);
#endif
        
        fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const,
                               (fbe_base_object_t *)lun_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_PERSIST_DEFAULT_NONPAGED_METADATA);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_config_clear_clustered_flag((fbe_base_config_t *) lun_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST);

    /* clear the signature verify condition */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *) lun_p);

    return FBE_LIFECYCLE_STATUS_DONE;

}
/******************************************************************************
 * end fbe_ext_pool_lun_metadata_verify_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_zero_consumed_area_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to zero out the consumed area for the LUN object.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  7/21/2010 - Created. Dhaval Patel
 *  3/25/2011 - Modified Sandeep Chaudhari
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_ext_pool_lun_zero_consumed_area_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t                   status; 
    fbe_ext_pool_lun_t*                     lun_p;
    fbe_lba_t                      block_count;
    fbe_payload_ex_t*                       payload_p = NULL;    
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_block_edge_t *                      block_edge_p = NULL;
    fbe_optimum_block_size_t                optimum_block_size;
    fbe_scheduler_hook_status_t hook_status;
    
    lun_p  = (fbe_ext_pool_lun_t *)object_p;

    fbe_ext_pool_lun_check_hook(lun_p, SCHEDULER_MONITOR_STATE_LUN_ZERO,
                       FBE_LUN_SUBSTATE_ZERO_START, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

    // This is an imported capacity of the lun
    fbe_lun_get_imported_capacity(lun_p, &block_count);

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Clear MD for this LUN. Block:0x%llx\n", 
                          __FUNCTION__, block_count);

    
   // Get the control op buffer data pointer from the packet payload.
    payload_p = fbe_transport_get_payload_ex(packet_p);
    if (payload_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE NULL payload pointer line %d", __FUNCTION__, __LINE__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    block_operation_p   = fbe_payload_ex_allocate_block_operation(payload_p);
    
    fbe_lun_get_block_edge(lun_p, &block_edge_p);

    /* get optimum block size for this i/o request */
    fbe_block_transport_edge_get_optimum_block_size(&lun_p->block_edge, &optimum_block_size);

    /* next, build block operation in sep payload */
    fbe_payload_block_build_operation(block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INIT_EXTENT_METADATA,
                                      0,
                                      block_count,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      optimum_block_size,
                                      NULL);

    fbe_payload_ex_increment_block_operation_level(payload_p);

    status = fbe_transport_set_completion_function(packet_p, 
                                                   fbe_lun_init_lun_extent_metadata_completion, 
                                                   lun_p);

    /* invoke bouncer to forward i/o request to downstream object */
    status = fbe_base_config_bouncer_entry((fbe_base_config_t *)lun_p, packet_p);
    if(status == FBE_STATUS_OK){ /* Small stack */
        status = fbe_ext_pool_lun_send_io_packet(lun_p, packet_p);
    }

    return FBE_LIFECYCLE_STATUS_PENDING;;
}
/******************************************************************************
 * end fbe_ext_pool_lun_zero_consumed_area_cond_function()
 ******************************************************************************/

/*!**************************************************************
 * fbe_lun_init_lun_extent_metadata_completion()
 ****************************************************************
 * @brief
 *  This is the completion for init the lun extent metadata.
 * 
 * @param packet_p - monitor packet
 * @param context - Completion context.
 *
 * @return fbe_status_t 
 *
 * @author
 *  09/14/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t 
fbe_lun_init_lun_extent_metadata_completion(fbe_packet_t * packet_p,
                                            fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_status_t pkt_status;
    fbe_payload_ex_t *                      payload_p = NULL;
    fbe_payload_block_operation_t*          block_operation_p = NULL;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qual;
    fbe_ext_pool_lun_t*                     lun_p = (fbe_ext_pool_lun_t *)context;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p); 
     
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qual);
    pkt_status = fbe_transport_get_status_code(packet_p);

    fbe_base_object_trace((fbe_base_object_t*) context, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          " %s pk_st: 0x%x blk_st:0x%x bl_q:0x%x\n",
                          __FUNCTION__, pkt_status, block_status, block_qual);    

    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

    if(FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS == block_status)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    }
    else
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, block_status);
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s : NDB flag: %u, CZ flag: %u\n", __FUNCTION__, lun_p->ndb_b, lun_p->clear_need_zero_b);
        
    /* clear_need_zero_b=1, ndb_b=1: send command to unmark zero.
     * clear_need_zero_b=1, ndb_b=0: not a valid combination.
     * clear_need_zero_b=0, ndb_b=1: do nothing.
     * clear_need_zero_b=0, ndb_b=0: send command to user zero.
     */
    if((lun_p->clear_need_zero_b == FBE_FALSE) && (lun_p->ndb_b == FBE_TRUE))
    {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);
        return FBE_STATUS_OK;
    }

    /* Set the completion function before we initialize the lun signature. */
    fbe_transport_set_completion_function(packet_p, fbe_ext_pool_lun_zero_consumed_area_cond_completion, lun_p);

    /* initialize lun signature. */ 
    status = fbe_ext_pool_lun_zero_start_lun_zeroing_packet_allocation(lun_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_lun_persist_initial_verify_flag_completion()
 **************************************/
/*!****************************************************************************
 * fbe_ext_pool_lun_zero_consumed_area_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the signature init
 *  condition.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  7/21/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_ext_pool_lun_zero_consumed_area_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_ext_pool_lun_t *                         lun_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    lun_p = (fbe_ext_pool_lun_t *)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code(packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* clear zero consumed area condition if status is good. */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) lun_p);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s zero consumed area condition failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_ext_pool_lun_zero_consumed_area_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_write_default_nonpaged_metadata_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to write the default nonpaged metadata for the 
 *  raid group object during object initialization.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  7/21/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_ext_pool_lun_write_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p,
                                                      fbe_packet_t * packet_p)
{
    fbe_ext_pool_lun_t *         lun_p = NULL;
    fbe_status_t        status;

    lun_p = (fbe_ext_pool_lun_t*)object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    if (fbe_base_config_is_active((fbe_base_config_t *)object_p))
    {
        /* set the completion function for the paged metadata init condition. */
        fbe_transport_set_completion_function(packet_p, fbe_lun_write_default_nonpaged_metadata_cond_completion, lun_p);

        /* initialize signature for the raid group object. */
        status = fbe_ext_pool_lun_metadata_write_default_nonpaged_metadata(lun_p, packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* we ar passive, just clear the condition and move on */
    /* clear nonpaged metadata write default condition. */
    fbe_lifecycle_clear_current_cond(object_p);

    /* reschedule immediately during object initialization. */
    fbe_lifecycle_reschedule(&fbe_ext_pool_lun_lifecycle_const,
                             object_p,
                             (fbe_lifecycle_timer_msec_t) 0);
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_ext_pool_lun_write_default_nonpaged_metadata_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_write_default_nonpaged_metadata_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the signature init
 *  condition.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  7/21/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_lun_write_default_nonpaged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                        fbe_packet_completion_context_t context)
{
    fbe_ext_pool_lun_t *                         lun_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    lun_p = (fbe_ext_pool_lun_t *)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* clear nonpaged metadata write default condition if status is good. */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) lun_p);

        /* reschedule immediately during object initialization. */
        fbe_lifecycle_reschedule(&fbe_ext_pool_lun_lifecycle_const,
                                 (fbe_base_object_t *) lun_p,
                                 (fbe_lifecycle_timer_msec_t) 0);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s write default nonpaged metadata condition failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_write_default_nonpaged_metadata_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_ext_pool_lun_persist_default_nonpaged_metadata_cond_function()
 ******************************************************************************
 *
 * @brief   This function is used to persist the default nonpaged metadata for the 
 *          lun object during object initialization.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  02/15/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_ext_pool_lun_persist_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p, 
                                                                                      fbe_packet_t * packet_p)
{
    fbe_status_t status;

    fbe_base_object_trace((fbe_base_object_t*)object_p,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    if (!fbe_base_config_is_active((fbe_base_config_t *)object_p)) {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* At this point, we just go ahead and write the fact that we are now initialized */
    status = fbe_base_config_set_nonpaged_metadata_state((fbe_base_config_t *)object_p, packet_p, FBE_NONPAGED_METADATA_INITIALIZED);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(object_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s set np metadata state failed, status: 0x%x\n",
                              __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_ext_pool_lun_persist_default_nonpaged_metadata_cond_function()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_lun_update_zero_checkpoint_need_to_run()
 ******************************************************************************
 * @brief
 *  Determines if update zero checkpoint for LU needs to run.
 *
 * @param lun_p    - pointer to a LUN object.
 * @param packet_p - pointer to a monitor packet.
 * 
 * @return fbe_bool_t   - TRUE indicates update zero checkpoint need to run.
 *
 * @author
 *  07/02/2012 - Created Prahlad Purohit
 *
 ******************************************************************************/
static fbe_bool_t
fbe_lun_update_zero_checkpoint_need_to_run(fbe_ext_pool_lun_t *lun_p, fbe_packet_t * packet_p)
{
    fbe_status_t    status;
    fbe_lba_t       zero_checkpoint = FBE_LBA_INVALID;
    fbe_cpu_id_t    cpu_id;

    /* get the current zero checkpoint. If LU zeroing already completed no need
     * to update checkpoint.
     */
    status = fbe_ext_pool_lun_metadata_get_zero_checkpoint(lun_p, &zero_checkpoint);
    if ((FBE_STATUS_OK == status) && (FBE_LBA_INVALID == zero_checkpoint))
    {
        return FBE_FALSE;
    }

    /* decremenet count_to_update. If count to update expired we need to run
     * zero checkpoint update.
     */
    lun_p->zero_update_count_to_update--;
    if(0 == lun_p->zero_update_count_to_update)
    {
        lun_p->zero_update_count_to_update = lun_p->zero_update_interval_count;

        fbe_transport_get_cpu_id(packet_p, &cpu_id);

        status = fbe_memory_check_state(cpu_id, FBE_FALSE); /* Just control memory */

        if (status != FBE_STATUS_OK)
        {
            fbe_base_config_increment_deny_count((fbe_base_config_t *)lun_p);
            /* we wait up to 10 times for memory */
            if (fbe_base_config_get_deny_count((fbe_base_config_t *)lun_p) < 10)
            {
                return FBE_FALSE;
            }
        }
        fbe_base_config_reset_deny_count((fbe_base_config_t *)lun_p);

        return FBE_TRUE;
    }

    return FBE_FALSE;
}


/*!****************************************************************************
 * fbe_ext_pool_lun_update_zero_checkpoint_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to collect the zeroing status for the raid group
 *  extent and update the LU zero checkpoint based on that, once zeroing gets
 *  completed it will clear the condition.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  10/13/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t
fbe_ext_pool_lun_update_zero_checkpoint_cond_function(fbe_base_object_t * object_p,
                                             fbe_packet_t * packet_p)
{
    fbe_ext_pool_lun_t *         lun_p = NULL;
    fbe_status_t        status;
    fbe_lba_t           zero_checkpoint;
    fbe_bool_t          is_active_sp = FBE_FALSE;

    lun_p = (fbe_ext_pool_lun_t *)object_p;
    
    fbe_base_object_trace((fbe_base_object_t *)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* get the current zero checkpoint.  If the status is not okay, just return done. */
    status = fbe_ext_pool_lun_metadata_get_zero_checkpoint(lun_p, &zero_checkpoint);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(object_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Failed to get checkpoint. Status: 0x%X\n", __FUNCTION__, status);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if(zero_checkpoint == FBE_LBA_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: zeroing gets completed\n", __FUNCTION__);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* only active sp update the zero checkpoint. */
    is_active_sp = fbe_base_config_is_active((fbe_base_config_t *) lun_p);
    if(is_active_sp == FBE_FALSE)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* set the completion function before we get raid extent zero checkpoint. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_lun_update_zero_checkpoint_completion,
                                          lun_p);

    /* get the raid extent zero checkpoint to calculate lu checkpoint. */
    status = fbe_ext_pool_lun_zero_get_raid_extent_zero_checkpoint(lun_p, packet_p);

    if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_ext_pool_lun_update_zero_checkpoint_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_update_zero_checkpoint_completion()
 ******************************************************************************
 * @brief
 *  This function is used handle the completion for the zero checkpoint update
 *  condition, it clears the condition if zeroing gets completed and log a
 *  message for the lu zeroing completion.
 *
 * @param packet_p                  - pointer to a  monitor packet.
 * @param context                   - pointer to lun object.
 * 
 * @return fbe_status_t             - status of the operation.
 *
 * @author
 *  10/13/2010 - Created. Dhaval Patel
 *  05/26/2011 - Modified Sandeep Chaudhari
 *  07/02/2012 - Modified Prahlad Purohit
 ******************************************************************************/
static fbe_status_t
fbe_lun_update_zero_checkpoint_completion(fbe_packet_t * packet_p,
                                          fbe_packet_completion_context_t context)
{
    fbe_ext_pool_lun_t *                         lun_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_scheduler_hook_status_t hook_status;
    
    lun_p = (fbe_ext_pool_lun_t *)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /*! @note Since there will always be the case of an I/O in-flight during
     *        destroy, generate a `warning' not an `error' trace.
     */
    if ((status != FBE_STATUS_OK)                         ||
        (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)    )
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s update zero checkpoint cond failed, status:0x%X, control_status:0x%X\n",
                              __FUNCTION__, status, control_status);
    }

    /* Make note of the fact that zero checkpoint was updated. */
    fbe_ext_pool_lun_check_hook(lun_p,
                       SCHEDULER_MONITOR_STATE_LUN_ZERO,
                       FBE_LUN_SUBSTATE_ZERO_CHECKPOINT_UPDATED,
                       0,
                       &hook_status);    

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_update_zero_checkpoint_completion()
 ******************************************************************************/

static fbe_status_t fbe_ext_pool_lun_check_power_saving_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t                  need_to_power_save = FBE_FALSE;
    fbe_ext_pool_lun_t *                 lun_p = (fbe_ext_pool_lun_t *)object_p;
    fbe_base_config_t *         base_config_p = (fbe_base_config_t *)object_p;
    fbe_object_id_t             my_object_id;
    fbe_lifecycle_state_t       peer_state;
    fbe_power_save_state_t      power_save_state = FBE_POWER_SAVE_STATE_INVALID;

    /*this function will do some LU specific tasks for power saving and override the base config stuff*/
    fbe_base_object_get_object_id((fbe_base_object_t *)lun_p, &my_object_id);

    /*check if this is either the system lun or a private lun and these should never be spun down*/
    if (fbe_private_space_layout_object_id_is_system_lun(my_object_id)) {
         return FBE_STATUS_OK;/*no need to power save*/

    }

    /* Lock base config before getting the power saving state */
    fbe_base_config_lock(base_config_p);    

    /* if we are exiting power saving mode, we should not try to get back in right away,
     * we need a complete exit first.
     */
    fbe_base_config_metadata_get_power_save_state(base_config_p, &power_save_state);

    /* Unlock base config after getting the power saving state */
    fbe_base_config_unlock(base_config_p);

    if (power_save_state == FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE)
    {
        return FBE_STATUS_OK;/*no need to power save*/
    }

    if (!fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY)) 
    {
        /*see if we even need to go to power save*/
        status = fbe_base_config_check_if_need_to_power_save((fbe_base_config_t *)object_p, &need_to_power_save);
        if (status != FBE_STATUS_OK) {
             fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s failed to check if need to power save\n",__FUNCTION__);
             return status;
        }

        if (!need_to_power_save) {
            return FBE_STATUS_OK;/*no need to power save*/
        }

        status = base_config_send_spindown_qualified_usurper_to_servers(base_config_p, packet_p);

        if (status == FBE_STATUS_INSUFFICIENT_RESOURCES) {
            /*let's also disbale power saving on this object*/
            fbe_base_config_disable_object_power_save(base_config_p);
            return FBE_STATUS_OK;
        }else if (status == FBE_STATUS_BUSY){
            /*some edges were not able to respound, so let's try again in the same cycle*/
            return FBE_STATUS_OK;
        }else if (status != FBE_STATUS_OK){
            fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't send usurper to servers.\n",__FUNCTION__);
            return FBE_STATUS_OK;/*we will need to try agin so we don't clear the condition*/
        }
        
       /*before we go to slumer, we mark the path state as FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_READY_TO_HIBERNATE. This will generate an event at the BVD level
        which will send a state change to the SEP clients (e.g. cache). this event will tell cache we are about to go
        to hibernate. LUN also needs to make sure it waits enough time as described in fbe_ext_pool_lun_t.power_save_io_drain_delay*/
        status = fbe_block_transport_server_set_path_attr(&base_config_p->block_transport_server, 0, FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_READY_TO_HIBERNATE);
        if (status != FBE_STATUS_OK) {
             fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s failed to set path attribute\n",__FUNCTION__);
             return status;
        }

        fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_INFO ,FBE_TRACE_MESSAGE_ID_INFO,"LU hibernate, syncing with peer\n");
        fbe_base_config_set_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY);
    }
    /* now the two side need to sync up */
    if (fbe_base_config_is_peer_present(base_config_p)) 
    {
        fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t*)lun_p, &peer_state);
        if ((peer_state != FBE_LIFECYCLE_STATE_HIBERNATE) &&
            (!fbe_base_config_is_peer_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY)))
        {
            /* both sides will start the timer independently.  
             * And one could get out after this point.  When this happens, one will see peer in hibernate state,
             * it can then proceed directly.
             */
            return FBE_STATUS_OK;
        }
    }

    /*since we will use the base config portion in HIBERNATE state, we need to mark out edges with the expected latency time
    we got from the upper layers as part of the power saving policy */
    status = fbe_block_transport_server_set_time_to_become_ready(&base_config_p->block_transport_server, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_BVD_INTERFACE, lun_p->max_lun_latency_time_is_sec);
    if (status != FBE_STATUS_OK) {
         fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s failed to set latency time\n",__FUNCTION__);
         fbe_block_transport_server_clear_path_attr(&base_config_p->block_transport_server, 0, FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_READY_TO_HIBERNATE);
         return status;
    }

    lun_p->wait_for_power_save = fbe_get_time();/*let's start counting until we get to power_save_io_drain_delay_in_sec*/

    /*start the timer that will bring us to the next condition in which we wait before we start the real hibernation
    this is used to let any in flight IOs drain from cache*/
    fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const, object_p, FBE_LUN_LIFECYCLE_COND_LUN_WAIT_BEFORE_HIBERNATION);
    
    fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_INFO ,FBE_TRACE_MESSAGE_ID_INFO,"LU will hibernate in %llu seconds\n",
                          (unsigned long long)lun_p->power_save_io_drain_delay_in_sec);

    /* Lock base config for updating */
    fbe_base_config_lock(base_config_p);

    fbe_base_config_metadata_set_power_save_state(base_config_p, FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE);

    /* Unlock base config after updating */
    fbe_base_config_unlock(base_config_p);   

    return FBE_STATUS_OK;

}

static fbe_lifecycle_status_t fbe_ext_pool_lun_wait_before_hibernation_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_base_config_t *         base_config_p = (fbe_base_config_t *)object_p;
    fbe_ext_pool_lun_t *                 lun_p = (fbe_ext_pool_lun_t *)object_p;
    fbe_time_t                  time_since_wait = 0;
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   outstanding_io;
    fbe_system_power_saving_info_t  get_info;
    fbe_power_save_state_t                  power_save_state = FBE_POWER_SAVE_STATE_INVALID;
    fbe_lifecycle_state_t       peer_state;

    /*before we do any other check, let's see we did not get an IO that was in flight from the time we set this condition
    the least expensive thing is to check the outstanding io count. We can certainly have a case where IOs came and went by the 
    time the monitor kicked, but the othe option is for each IO to do some expensive checks.
    we also have to check if by any chance the power saving was disabled along the way*/
    fbe_block_transport_server_get_outstanding_io_count(&base_config_p->block_transport_server, &outstanding_io);
    fbe_base_config_get_system_power_saving_info(&get_info);
    fbe_base_config_metadata_get_peer_lifecycle_state(base_config_p, &peer_state);

    /* Lock base config before getting the power saving state */
    fbe_base_config_lock(base_config_p);
    /* Get the power saving state */
    fbe_base_config_metadata_get_power_save_state(base_config_p, &power_save_state);
    /* Unlock base config after updating */
    fbe_base_config_unlock(base_config_p);

    if (outstanding_io != 0 ||
        !fbe_base_config_is_flag_set(base_config_p, FBE_BASE_CONFIG_FLAG_POWER_SAVING_ENABLED) ||
        !get_info.enabled ||
        power_save_state != FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE ||
        (fbe_base_config_is_peer_present(base_config_p) &&
         (peer_state != FBE_LIFECYCLE_STATE_HIBERNATE) &&
         !fbe_base_config_is_peer_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY))) {

        fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "Got IO or powersave disabled while waiting to save power, will NOT save power\n");

        /*we have IO, let's cancel this timer and not get into hibernation*/
        /*PAY ATTENTION  - fbe_lifecycle_stop_timer will not work if you used fbe_lifecycle_clear_current_cond before or after it*/
        status = fbe_lifecycle_stop_timer(&fbe_ext_pool_lun_lifecycle_const, object_p, FBE_LUN_LIFECYCLE_COND_LUN_WAIT_BEFORE_HIBERNATION);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,"%s can't stop timer, status: 0x%X\n",
                                  __FUNCTION__, status);
    
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* Lock base config before getting the power saving state */
        fbe_base_config_lock(base_config_p);
        /* Set new power saving state */
        fbe_base_config_metadata_set_power_save_state(base_config_p, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);
        /* Unlock base config after updating */
        fbe_base_config_unlock(base_config_p);

        if (fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY))
        {
            fbe_base_config_clear_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY);
        }

        /*say we are not ready to sleep anymore*/
        status = fbe_block_transport_server_clear_path_attr(&base_config_p->block_transport_server, 0, FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_READY_TO_HIBERNATE);
        if (status != FBE_STATUS_OK) {
             fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s failed to set path attribute\n",__FUNCTION__);
             return FBE_LIFECYCLE_STATUS_DONE;
        }

        return FBE_LIFECYCLE_STATUS_DONE;

    }

    time_since_wait = fbe_get_elapsed_seconds(lun_p->wait_for_power_save);
    if (time_since_wait >= lun_p->power_save_io_drain_delay_in_sec) {
        lun_p->wait_for_power_save = 0;/*reset for next time*/

        /*PAY ATTENTION  - fbe_lifecycle_stop_timer will not work if you used fbe_lifecycle_clear_current_cond before or after it*/
        status = fbe_lifecycle_stop_timer(&fbe_ext_pool_lun_lifecycle_const, object_p, FBE_LUN_LIFECYCLE_COND_LUN_WAIT_BEFORE_HIBERNATION);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,"%s can't stop timer, status: 0x%X\n",
                                  __FUNCTION__, status);
    
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        status = fbe_block_transport_server_clear_path_attr(&base_config_p->block_transport_server, 0, FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_READY_TO_HIBERNATE);

        status = fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const, 
                                        object_p, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_HIBERNATE);

        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s can't set object to HIBERNATE\n",__FUNCTION__);
        }
        
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_lifecycle_clear_current_cond(object_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,"%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;

}

/*the original design was to LUN to get power saving info from cache but it is not implemented at layered drivers
level yet so we get it from RAID (which gets it from NAVI)*/
static fbe_lifecycle_status_t   fbe_ext_pool_lun_get_power_save_info_from_raid_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet)
{
    fbe_ext_pool_lun_t *                                     lun_p = (fbe_ext_pool_lun_t *)object_p;
    fbe_block_edge_t *                              edge = NULL;
    fbe_payload_control_operation_t *               control_operation = NULL;
    fbe_payload_ex_t *                                 payload = NULL;
    fbe_payload_control_status_t                    control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_group_get_power_saving_info_t          get_rg_power_save_info;
    fbe_u32_t                                       client_count = 0;

    
    fbe_base_config_get_block_edge((fbe_base_config_t *)lun_p, &edge, 0);
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload); 
    if(control_operation == NULL) {    
        fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Failed to allocate control operation\n",__FUNCTION__);

        
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_payload_control_build_operation(control_operation,  
                                        FBE_RAID_GROUP_CONTROL_CODE_GET_POWER_SAVING_PARAMETERS,  
                                        &get_rg_power_save_info, 
                                        sizeof(fbe_raid_group_get_power_saving_info_t)); 

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    
    status  = fbe_block_transport_send_control_packet(edge, packet);  
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) {    
        fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Failed to send get power save info packet\n",__FUNCTION__);

        fbe_payload_ex_release_control_operation(payload, control_operation);
        return FBE_LIFECYCLE_STATUS_DONE;  
    }

    fbe_transport_wait_completion(packet);

    status = fbe_transport_get_status_code(packet);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_ex_release_control_operation(payload, control_operation);

    if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || status != FBE_STATUS_OK) {
         fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Error is getting power save info from RG\n",__FUNCTION__);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_lun_set_power_save_info(lun_p, lun_p->power_save_io_drain_delay_in_sec, get_rg_power_save_info.max_raid_latency_time_is_sec);
    fbe_base_config_set_power_saving_idle_time((fbe_base_config_t *)lun_p, get_rg_power_save_info.idle_time_in_sec);

    fbe_block_transport_server_get_number_of_clients(&lun_p->base_config.block_transport_server, &client_count);

    /*now that we got the information, let's enable/disbale if needed
    this needs to be the last step*/
      if (get_rg_power_save_info.power_saving_enabled) {

        base_config_enable_object_power_save((fbe_base_config_t *)lun_p);
        lun_p->lun_attributes |= VOL_ATTR_STANDBY_POSSIBLE;/*we have to mark it on the lun as well since by the time we execute this we may not have the edge up*/

        if (client_count != 0) {
            fbe_block_transport_server_set_path_attr(&lun_p->base_config.block_transport_server, 0, FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_CAN_SAVE_POWER);
        }       

    }else{
    
        base_config_disable_object_power_save((fbe_base_config_t *)lun_p);
        lun_p->lun_attributes &= ~VOL_ATTR_STANDBY_POSSIBLE;/*we have to mark it on the lun as well since by the time we execute this we may not have the edge up*/

        if (client_count != 0) {
            fbe_block_transport_server_clear_path_attr(&lun_p->base_config.block_transport_server, 0, FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_CAN_SAVE_POWER);
        }
        

    }
    
    status = fbe_lifecycle_clear_current_cond(object_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,"%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);

    }

    return FBE_LIFECYCLE_STATUS_DONE; 
}

/*!**************************************************************
 * fbe_lun_clear_peer_state()
 ****************************************************************
 * @brief
 *   Clear out the peer state as part of transitioning
 *   from one state to another.
 *  
 * @param lun_p - The lun object.
 * 
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING
 *                                    - if we are not done yet.
 *                                  FBE_LIFECYCLE_STATUS_DONE
 *                                    - if we are done.
 *
 * @author
 *  03/27/2012 - Created. MFerson
 *
 ****************************************************************/
static fbe_lifecycle_status_t fbe_lun_clear_peer_state(fbe_ext_pool_lun_t *lun_p)
{
    fbe_lifecycle_state_t lifecycle_state;
    fbe_status_t status;
    fbe_bool_t            b_is_active;

    /* When we change states it is important to clear out our masks for things
     * related to the peer. 
     * For the activate -> ready transition we do not want to clear the flags. 
     * At the point of activate we have initted these peer to peer flags and 
     * have joined the peer. 
     */

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    fbe_ext_pool_lun_lock(lun_p);

    //fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_ALL_MASKS);

    status = fbe_lifecycle_get_state(&fbe_ext_pool_lun_lifecycle_const, (fbe_base_object_t*)lun_p, &lifecycle_state);

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                      FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s lifecycle state is %d, local state is 0x%X\n", 
                      __FUNCTION__, lifecycle_state, (unsigned int)lun_p->local_state);

    fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_ALL_MASKS);

    if ((lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_READY) && (lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_ACTIVATE))
    {

        //fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_ALL_MASKS);
    }

    if (lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_READY) 
    {
        if (fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t*)lun_p, 
                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_ALL_MASKS) )
        {
            fbe_base_config_clear_clustered_flag((fbe_base_config_t*)lun_p, 
                                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_ALL_MASKS);                                
            fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "lun clearing clustered flags obj_state: 0x%x l_bc_flags: 0x%x peer: 0x%x local state: 0x%x\n", 
                                  lun_p->base_config.base_object.lifecycle.state,
                                  (unsigned int)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                                  (unsigned int)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                                  (unsigned int)lun_p->local_state);
            fbe_ext_pool_lun_unlock(lun_p);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
    }
    else
    {
        /* we are moving into ready state, after peer sets JOIN_SYNCING */
        b_is_active = fbe_base_config_is_active((fbe_base_config_t*)lun_p);
        if (!b_is_active)
        {
            fbe_base_config_set_clustered_flag((fbe_base_config_t*)lun_p, 
                                               FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING); 
        }
    }
    fbe_ext_pool_lun_unlock(lun_p);
    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************
 * end fbe_lun_clear_peer_state()
 **************************************/

/*!**************************************************************
 * fbe_ext_pool_lun_pending_func()
 ****************************************************************
 * @brief
 *   This gets called when we are going to transition from one
 *   lifecycle state to another. The purpose here is to drain
 *   all I/O before we transition state.
 *  
 * @param base_config - Object that is pending.
 * @param packet - The monitor paacket.
 * 
 * @return fbe_lifecycle_status_t - We return the status
 *         from fbe_block_transport_server_pending() since
 *         that is the function which deals with determining
 *         when I/Os are drained.
 *
 * @author
 *  09/20/2010 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/

fbe_lifecycle_status_t 
fbe_ext_pool_lun_pending_func(fbe_base_object_t *base_object_p, fbe_packet_t * packet)
{
    fbe_lifecycle_status_t lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;
    fbe_base_config_t * base_config_p = NULL;
    fbe_ext_pool_lun_t  *lun_p = NULL;
    fbe_status_t status;
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    base_config_p = (fbe_base_config_t *)base_object_p;
    lun_p = (fbe_ext_pool_lun_t*) base_object_p;

    fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "LUN pending statel/p:%u/%u mde_st: %x %c lcf:%x/%x bcf:%x/%x\n", 
                          base_object_p->lifecycle.state,
                          lun_p->lun_metadata_memory_peer.base_config_metadata_memory.lifecycle_state,
                          base_config_p->metadata_element.metadata_element_state,
                          ((base_config_p->metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE) ? 'A' : 'P'),
                          (unsigned int)lun_p->lun_metadata_memory.flags, 
                          (unsigned int)lun_p->lun_metadata_memory_peer.flags,
                          (unsigned int)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                          (unsigned int)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags);

    lifecycle_status = fbe_lun_clear_peer_state(lun_p);
    if (lifecycle_status != FBE_LIFECYCLE_STATUS_DONE)
    {
        return lifecycle_status;
    }

    /* We must update the path state before any operations are allowed to fail back.
     * The upper levels expect the path state to change before I/Os get bad status. 
     */
    fbe_block_transport_server_update_path_state(&base_config_p->block_transport_server,
                                                 &fbe_base_config_lifecycle_const,
                                                 (fbe_base_object_t*)lun_p,
                                                 FBE_BLOCK_PATH_ATTR_FLAGS_NONE);
    
    /* Call the below function to fail out any IOs that was put in the terminator queue
     * error and needs to get restarted. 
     */
    status = fbe_ext_pool_lun_handle_shutdown(lun_p);
    
    fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s draining block transport server attributes: 0x%llx\n", __FUNCTION__, 
                          (unsigned long long)base_config_p->block_transport_server.attributes);

    /* We simply call to the block transport server to handle pending.
     */
    lifecycle_status = fbe_block_transport_server_pending(&base_config_p->block_transport_server,
                                                          &fbe_base_config_lifecycle_const,
                                                          (fbe_base_object_t *) base_config_p);
    
    status = fbe_base_object_get_lifecycle_state(base_object_p, &lifecycle_state);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "Unable to get lifecycle state status: %d\n", status);
    }
    if ((lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_DESTROY) &&
        !fbe_topology_is_reference_count_zero(base_config_p->base_object.object_id))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN pending saw non-zero reference count.\n");
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    return lifecycle_status;
}
/**************************************
 * end fbe_ext_pool_lun_pending_func()
 **************************************/

/****************************************************************
 *  fbe_ext_pool_lun_handle_shutdown
 ****************************************************************
 * @brief
 *  This function is the handler for a shutdown. If the lun is
 *  going broken, empty the terminator queue and complete the
 *  packet with io failed status
 *
 * @param lun_p - a pointer to lun
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  09/20/2010 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/

fbe_status_t fbe_ext_pool_lun_handle_shutdown(fbe_ext_pool_lun_t * const lun_p)
{
    
    fbe_packet_t                          *new_packet_p = NULL;    
    fbe_payload_ex_t*                     sep_payload_p;
    fbe_payload_block_operation_t*         block_operation_p;         
    fbe_queue_head_t temp_queue;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_queue_element_t *next_element_p = NULL;
    fbe_queue_head_t *termination_queue_p = &((fbe_base_object_t*)lun_p)->terminator_queue_head;
    fbe_queue_init(&temp_queue);

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s \n", 
                          __FUNCTION__);

    fbe_spinlock_lock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);

    // If the queue is empty just return ok
    if(fbe_queue_is_empty(termination_queue_p))
    {
        fbe_spinlock_unlock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);
        return FBE_STATUS_OK;    
        
    }
    queue_element_p = fbe_queue_front(termination_queue_p);

    while (queue_element_p != NULL)
    {
        new_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        next_element_p = fbe_queue_next(termination_queue_p, &new_packet_p->queue_element);

        /* If there is something on this subpacket queue, this is still in flight.
         */
        if (fbe_queue_is_empty(&new_packet_p->subpacket_queue_head))
        {
            fbe_transport_remove_packet_from_queue(new_packet_p);
            fbe_queue_push(&temp_queue, queue_element_p);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)lun_p,
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Not draining packet %p due to subpacket\n", __FUNCTION__, new_packet_p );
        }
        queue_element_p = next_element_p;
    }
    // Dequeue all the packets from the terminator queue and set the 
    // block status to io failed and qualifier to retry not possible
    // and complete the packet    
    while(!fbe_queue_is_empty(&temp_queue)) 
    {
        queue_element_p = fbe_queue_pop(&temp_queue);
        new_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        if(new_packet_p != NULL)
        {
            // Get the payload and block operation for this I/O operation
            sep_payload_p = fbe_transport_get_payload_ex(new_packet_p);
            block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

            // Set the status to io failed and complete the packet 
            fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);

            fbe_spinlock_unlock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);

            fbe_transport_set_status(new_packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(new_packet_p);

            fbe_spinlock_lock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);
            
        }
    }
    fbe_spinlock_unlock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);
    fbe_queue_destroy(&temp_queue);

    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_ext_pool_lun_handle_shutdown()
 *****************************************/

/*let BVD know the first IO has been written to the LUN*/
static fbe_lifecycle_status_t fbe_ext_pool_lun_signal_first_write_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t            status;
    fbe_ext_pool_lun_t  *            lun_p = (fbe_ext_pool_lun_t*)object_p;
    fbe_u32_t               client_count = 0;
    fbe_lun_nonpaged_metadata_t *       lun_nonpaged_metadata = NULL;

    /*do we have any clients ? We might be a system LUN, not conntected to BVD*/
    fbe_block_transport_server_get_number_of_clients(&lun_p->base_config.block_transport_server, &client_count);
    
    if ((client_count != 0)) {
        /*let the client know*/
        fbe_block_transport_server_set_path_attr(&lun_p->base_config.block_transport_server,
                                                0,
                                                FBE_BLOCK_PATH_ATTR_FLAGS_HAS_BEEN_WRITTEN);
    }

    status = fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) lun_p, (void **)&lun_nonpaged_metadata);
    if (lun_nonpaged_metadata == NULL) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to get non paged metadata", __FUNCTION__);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* if the bit has already being written, should */
    if (!FBE_IS_TRUE(lun_nonpaged_metadata->has_been_written))
    {
        fbe_transport_set_completion_function(packet_p, fbe_ext_pool_lun_signal_first_write_cond_function_cond_completion, lun_p);
        status = fbe_ext_pool_lun_metadata_set_lun_has_been_written(lun_p, packet_p);
        if ((status != FBE_STATUS_PENDING) && (status != FBE_STATUS_OK)) {
             fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s can't update non paged metadata", __FUNCTION__);
        }

        return  FBE_LIFECYCLE_STATUS_PENDING;
    }

    status = fbe_lifecycle_clear_current_cond(object_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,"%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);

    }

    return FBE_LIFECYCLE_STATUS_DONE; 
}

static fbe_status_t 
fbe_ext_pool_lun_signal_first_write_cond_function_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    
    fbe_ext_pool_lun_t *                         lun_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    lun_p = (fbe_ext_pool_lun_t *)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK)){
        /* we are done with updating the fact the lun is written to
        now let's force it to be persisted on the disk and not just in memory*/
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) lun_p);
        fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const, (fbe_base_object_t *)lun_p, FBE_BASE_CONFIG_LIFECYCLE_COND_PERSIST_NON_PAGED_METADATA);
             
    }else{
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s write nonpaged metadata condition failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }
    return FBE_STATUS_OK;
    
}

/*make sure the attribute is set correctly on the edge when we boot so upper layers know about it*/
static fbe_lifecycle_status_t fbe_ext_pool_lun_written_update_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t                        status;
    fbe_ext_pool_lun_t *                         lun_p = (fbe_ext_pool_lun_t *)object_p;
    fbe_lun_nonpaged_metadata_t *       lun_nonpaged_metadata = NULL;
    fbe_u32_t                           client_count = 0;
    
    status = fbe_lifecycle_clear_current_cond(object_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,"%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);

    }

    status = fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) lun_p, (void **)&lun_nonpaged_metadata);
    if (lun_nonpaged_metadata == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s failed to get metadata ptr", __FUNCTION__);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /*we want to mark the lun with the correct attribute*/
    if (FBE_IS_TRUE(lun_nonpaged_metadata->has_been_written)) {

        /*this is for us when we get IO to the LUN*/
        fbe_lun_set_flag(lun_p, FBE_LUN_FLAGS_HAS_BEEN_WRITTEN);/*mark it*/

        /*and let the BVD know about it*/
        /*do we have any clients ? We might be a system LUN, not conntected to BVD*/
        fbe_block_transport_server_get_number_of_clients(&lun_p->base_config.block_transport_server, &client_count);
    
        if ((client_count != 0)) {
            /*let the client know*/
            fbe_block_transport_server_set_path_attr(&lun_p->base_config.block_transport_server,
                                                    0,
                                                    FBE_BLOCK_PATH_ATTR_FLAGS_HAS_BEEN_WRITTEN);
        }

    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!****************************************************************************
 * fbe_ext_pool_lun_write_default_paged_metadata_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to write the default paged metadata for the 
 *  LUN object during object initialization. The LUN object does not have real
 * paged MD per se like other objects, but this function is used to clear the
 * dirty flags that is LUN private data. So we will use the flow is similar to
 * other objects
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  2/22/2012 Created. MFerson
 *  09/30/2012 - Ashok Tamilarasan - Modofied to clear the dirty flags
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_ext_pool_lun_write_default_paged_metadata_cond_function(fbe_base_object_t * object_p,
                                                   fbe_packet_t * packet_p)
{
    fbe_ext_pool_lun_t *         lun_p = NULL;
    fbe_status_t        status;

    lun_p = (fbe_ext_pool_lun_t*)object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    if (fbe_base_config_is_active((fbe_base_config_t *)object_p))
    {
        /* initialize paged metadata for the lun object. */
        status = fbe_lun_mark_all_clean(lun_p, packet_p);

        if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
        {
            fbe_base_object_trace((fbe_base_object_t *)lun_p,
                      FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s write default paged metadata condition failed, status:0x%X\n",
                      __FUNCTION__, status);

            return FBE_LIFECYCLE_STATUS_DONE;
        }

        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    else
    {
        /* we are passive, just clear the condition and move on */
        fbe_lifecycle_clear_current_cond(object_p);
    
        return FBE_LIFECYCLE_STATUS_DONE;
    }
}

/******************************************************************************
 * end fbe_ext_pool_lun_write_default_paged_metadata_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_mark_all_clean()
 ******************************************************************************
 * @brief
 *  This function marks the local and peer dirty flag as clean
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_status_t             - fbe status
 *
 * @author
 *  09/30/2012 - Created from fbe_lun_mark_local_clean_cond . Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t fbe_lun_mark_all_clean(fbe_ext_pool_lun_t * lun_p, 
                                           fbe_packet_t *packet_p)
{
    fbe_status_t status;

    /* first mark the local as clean */
    status = fbe_ext_pool_lun_mark_local_clean(lun_p, packet_p,
                                      fbe_lun_mark_all_clean_local_completion);
    return status;
}

/*!****************************************************************************
 * fbe_lun_mark_all_clean_local_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function that gets called after the local dirty flag
 *  is cleared
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t             - fbe status
 *
 * @author
 *  09/30/2012 - Created from fbe_lun_mark_local_clean_cond . Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t 
fbe_lun_mark_all_clean_local_completion(fbe_packet_t * packet_p,
                                        fbe_packet_completion_context_t context)
{
    fbe_ext_pool_lun_t *                         lun_p = NULL;
    fbe_lun_clean_dirty_context_t       *clean_dirty_context;

    lun_p = (fbe_ext_pool_lun_t *)context;

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    clean_dirty_context = &lun_p->clean_dirty_monitor_context;

    if(clean_dirty_context->status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) lun_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: write failed, status %d\n", __FUNCTION__, clean_dirty_context->status);

        return clean_dirty_context->status;        
    }

    /* Go ahead an mark the peer as also clean */
    fbe_ext_pool_lun_mark_peer_clean(lun_p, packet_p,
                             fbe_lun_write_default_paged_metadata_cond_completion);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}

/*!****************************************************************************
 * fbe_lun_write_default_paged_metadata_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the paged (aka. the dirty
 * flags)
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  2/22/2012 Created. MFerson
 *
 ******************************************************************************/
static fbe_status_t 
fbe_lun_write_default_paged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                        fbe_packet_completion_context_t context)
{
    fbe_ext_pool_lun_t *                         lun_p = NULL;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lun_clean_dirty_context_t       *clean_dirty_context;

    lun_p = (fbe_ext_pool_lun_t *)context;

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    clean_dirty_context = &lun_p->clean_dirty_monitor_context;

    if(clean_dirty_context->status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) lun_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: write failed, status %d\n", __FUNCTION__, clean_dirty_context->status);

        return clean_dirty_context->status;        
    }

    fbe_spinlock_lock(&lun_p->io_counter_lock);
    lun_p->clean_pending = FBE_FALSE;
    fbe_spinlock_unlock(&lun_p->io_counter_lock);

    fbe_base_object_trace((fbe_base_object_t*) lun_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: Local and peer dirty flag has been cleared\n", __FUNCTION__);

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*) lun_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_write_default_paged_metadata_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_mark_local_clean_cond_function()
 ******************************************************************************
 * @brief
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  3/6/2012 - Created. MFerson
 *
 ******************************************************************************/
static fbe_lifecycle_status_t
fbe_ext_pool_lun_mark_local_clean_cond_function(fbe_base_object_t * object_p,
                                       fbe_packet_t * packet_p)
{
    fbe_ext_pool_lun_t *         lun_p = NULL;
    fbe_status_t        status;
    fbe_atomic_t        counter;

    lun_p = (fbe_ext_pool_lun_t *)object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* we don't need to do flow control to non-ready state object, but we do need to
     * keep trace of the outstanding IO, because this completion routine will decrement it.
     */
    counter = fbe_atomic_increment(&fbe_lun_monitor_io_counter);

    status = fbe_ext_pool_lun_mark_local_clean(lun_p, packet_p, fbe_lun_mark_local_clean_completion);

    if(status == FBE_STATUS_OK) {
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_ext_pool_lun_mark_local_clean_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_mark_peer_clean_cond_function()
 ******************************************************************************
 * @brief
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  3/6/2012 - Created. MFerson
 *
 ******************************************************************************/
static fbe_lifecycle_status_t
fbe_ext_pool_lun_mark_peer_clean_cond_function(fbe_base_object_t * object_p,
                                       fbe_packet_t * packet_p)
{
    fbe_ext_pool_lun_t *         lun_p = NULL;
    fbe_status_t        status;
    fbe_atomic_t        counter;

    lun_p = (fbe_ext_pool_lun_t *)object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* we need to control total number of LUN monitor packets outstanding */
    counter = fbe_atomic_increment(&fbe_lun_monitor_io_counter);
    if (counter > FBE_MAX_LUN_CONCURRENT_REQUESTS) 
    {
        fbe_atomic_decrement(&fbe_lun_monitor_io_counter);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_ext_pool_lun_mark_peer_clean(lun_p, packet_p, fbe_lun_mark_peer_clean_completion);
    if(status == FBE_STATUS_OK) {
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_ext_pool_lun_mark_peer_clean_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_mark_peer_clean_completion()
 ******************************************************************************
 * @brief
 *
 * @param packet_p                  - pointer to a  monitor packet.
 * @param context                   - pointer to lun object.
 * 
 * @return fbe_status_t             - status of the operation.
 *
 * @author
 *  3/15/2012 - Created. MFerson
 ******************************************************************************/
static fbe_status_t
fbe_lun_mark_peer_clean_completion(fbe_packet_t * packet_p,
                                   fbe_packet_completion_context_t context)
{
    fbe_ext_pool_lun_t*                              lun_p = NULL;
    fbe_status_t                            status;
    fbe_lun_clean_dirty_context_t *         clean_dirty_context;

    lun_p = (fbe_ext_pool_lun_t*)context;

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* update the counter */
    fbe_atomic_decrement(&fbe_lun_monitor_io_counter);

    clean_dirty_context = &lun_p->clean_dirty_monitor_context;

    if(clean_dirty_context->status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) lun_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: write failed pkt_status: 0x%x\n", 
                              __FUNCTION__, clean_dirty_context->status);

        return clean_dirty_context->status;        
    }

    fbe_base_object_trace((fbe_base_object_t*) lun_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: Peer's dirty flag has been cleared\n", __FUNCTION__);


    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*) lun_p);

    return status;
}
/******************************************************************************
 * end fbe_lun_mark_peer_clean_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_lazy_clean_cond_function()
 ******************************************************************************
 * @brief
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  3/6/2012 - Created. MFerson
 *
 ******************************************************************************/
static fbe_lifecycle_status_t
fbe_ext_pool_lun_lazy_clean_cond_function(fbe_base_object_t * object_p,
                                 fbe_packet_t * packet_p)
{
    fbe_ext_pool_lun_t                  *lun_p = NULL;
    fbe_status_t                status;
    fbe_bool_t                  mark_clean = FALSE;
    fbe_time_t                  idle_time;
    fbe_atomic_t                counter;
    fbe_scheduler_hook_status_t hook_status;

    fbe_lun_clean_dirty_context_t * context;

    lun_p = (fbe_ext_pool_lun_t *)object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* we need to control total number of LUN monitor packets outstanding */
    counter = fbe_atomic_increment(&fbe_lun_monitor_io_counter);
    if (counter > FBE_MAX_LUN_CONCURRENT_REQUESTS) 
    {
        fbe_atomic_decrement(&fbe_lun_monitor_io_counter);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_spinlock_lock(&lun_p->io_counter_lock);

    if((lun_p->clean_time == 0) || (lun_p->io_counter > 0))
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s LUN is dirty again clean_time 0x%llX io_counter %d\n", 
                              __FUNCTION__,
                              lun_p->clean_time,
                              lun_p->io_counter
                              );

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);

        fbe_spinlock_unlock(&lun_p->io_counter_lock);

        fbe_atomic_decrement(&fbe_lun_monitor_io_counter);

        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        idle_time = fbe_get_elapsed_milliseconds(lun_p->clean_time);

        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s LUN has been clean for %lld milliseconds.\n", 
                              __FUNCTION__,
                              (long long)idle_time);
    
    
        if(idle_time >= FBE_LUN_LAZY_CLEAN_TIME_MS)
        {
            lun_p->clean_pending    = TRUE;

            fbe_ext_pool_lun_check_hook(lun_p,
                               SCHEDULER_MONITOR_STATE_LUN_CLEAN_DIRTY,
                               FBE_LUN_SUBSTATE_LAZY_CLEAN_START,
                               0,
                               &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE) {
                fbe_spinlock_unlock(&lun_p->io_counter_lock);
                return FBE_LIFECYCLE_STATUS_DONE;
            } 

            lun_p->clean_time       = 0;
            lun_p->marked_dirty     = FALSE;
            mark_clean              = TRUE;
        }
        fbe_spinlock_unlock(&lun_p->io_counter_lock);
    }

    if(mark_clean)
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Marking LUN Clean\n", 
                              __FUNCTION__);

        context = &lun_p->clean_dirty_monitor_context;
        context->dirty      = FBE_FALSE;
        context->is_read    = FBE_FALSE;
        context->lun_p      = lun_p;
        context->sp_id      = FBE_CMI_SP_ID_INVALID;

        status = fbe_ext_pool_lun_update_dirty_flag(context, packet_p,
                                           fbe_lun_mark_local_clean_completion);   

        if(status == FBE_STATUS_OK) {
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
        else {
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }
    else
    {
        fbe_atomic_decrement(&fbe_lun_monitor_io_counter);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    
}
/******************************************************************************
 * end fbe_ext_pool_lun_lazy_clean_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_mark_local_clean_completion()
 ******************************************************************************
 * @brief
 *  This function is used handle the completion for the zero checkpoint update
 *  condition, it clears the condition if zeroing gets completed and log a
 *  message for the lu zeroing completion.
 *
 * @param packet_p                  - pointer to a  monitor packet.
 * @param context                   - pointer to lun object.
 * 
 * @return fbe_status_t             - status of the operation.
 *
 * @author
 *  3/6/2012 - Created. MFerson
 ******************************************************************************/
static fbe_status_t
fbe_lun_mark_local_clean_completion(fbe_packet_t * packet_p,
                                    fbe_packet_completion_context_t context)
{
    fbe_ext_pool_lun_t*                              lun_p = NULL;
    fbe_status_t                            status;
    fbe_lun_clean_dirty_context_t *         clean_dirty_context;

    lun_p = (fbe_ext_pool_lun_t*)context;

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* update the counter */
    fbe_atomic_decrement(&fbe_lun_monitor_io_counter);

    clean_dirty_context = &lun_p->clean_dirty_monitor_context;

    /* If lower level return error, we need to set clean_time again
     * so that lazy_clean condition will go through the same code path. 
     *
     * Like marking dirty scenario, we intentionally do not try to differentiate 
     * between status values. In the case where we are going failed, the monitor
     * will drain the queue. 
     */
    if(clean_dirty_context->status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) lun_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: write failed, set clean_time again.\n",
                              __FUNCTION__);

        fbe_spinlock_lock(&lun_p->io_counter_lock);
        lun_p->clean_time = fbe_get_time();
        lun_p->clean_pending = FBE_FALSE;
        fbe_spinlock_unlock(&lun_p->io_counter_lock);

        return FBE_STATUS_OK;
    }

    fbe_spinlock_lock(&lun_p->io_counter_lock);
    lun_p->clean_pending = FBE_FALSE;
    fbe_spinlock_unlock(&lun_p->io_counter_lock);

    fbe_base_object_trace((fbe_base_object_t*) lun_p,
                         FBE_TRACE_LEVEL_DEBUG_LOW,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: Local dirty flag has been cleared\n", __FUNCTION__);

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*) lun_p);

    fbe_lifecycle_force_clear_cond(&fbe_ext_pool_lun_lifecycle_const,
                                   (fbe_base_object_t*)lun_p,
                                   FBE_LUN_LIFECYCLE_COND_LAZY_CLEAN);

    fbe_ext_pool_lun_handle_queued_packets_waiting_for_clear_dirty_flag(lun_p);

    return status;
}
/******************************************************************************
 * end fbe_lun_mark_local_clean_completion()
 ******************************************************************************/

/*!**************************************************************
 * fbe_ext_pool_lun_check_local_dirty_flag_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function for lun to check whether
 *  the lun is marked dirty. If it is, initiate a background verify
 *  to the lun
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING   
 *
 * @author
 *  03/12/2012 - Created. MFerson
 *
 ****************************************************************/

static fbe_lifecycle_status_t 
fbe_ext_pool_lun_check_local_dirty_flag_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t                    status; 
    fbe_ext_pool_lun_t*                      lun_p   = NULL;
    fbe_lun_clean_dirty_context_t * context = NULL;
    fbe_base_config_downstream_health_state_t downstream_health_state;
    
    lun_p  = (fbe_ext_pool_lun_t *)object_p;

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We definitely don't have any I/O in progress, so clear the counters */
    fbe_spinlock_lock(&lun_p->io_counter_lock);

    lun_p->io_counter      = 0;
    lun_p->dirty_pending   = FALSE;
    lun_p->clean_pending   = FALSE;
    lun_p->clean_time      = 0;
    lun_p->marked_dirty    = FALSE;

    fbe_spinlock_unlock(&lun_p->io_counter_lock);


    /* No need to do a lazy clean, if the condition was previously set*/
    fbe_lifecycle_force_clear_cond(&fbe_ext_pool_lun_lifecycle_const,
                                   (fbe_base_object_t*)lun_p,
                                   FBE_LUN_LIFECYCLE_COND_LAZY_CLEAN);

    /* If we're not the active SP, just clear the condition and
     * reschedule the monitor. The peer has already processed our
     * clean/dirty flag in either ACTIVATE or READY.
     */
    if(! fbe_base_config_is_active((fbe_base_config_t *)lun_p))
    {
        /* Clear the current condition */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*) lun_p);

        /* reschedule immediately */
        status = fbe_lifecycle_reschedule(
            &fbe_ext_pool_lun_lifecycle_const,
            (fbe_base_object_t *) lun_p,
            (fbe_lifecycle_timer_msec_t) 0);

        return(FBE_LIFECYCLE_STATUS_DONE);
    }

    downstream_health_state = fbe_ext_pool_lun_verify_downstream_health(lun_p);

    if(downstream_health_state == FBE_DOWNSTREAM_HEALTH_OPTIMAL)
    {
        context = &lun_p->clean_dirty_monitor_context;

        context->sp_id = FBE_CMI_SP_ID_INVALID;
        context->lun_p = lun_p;

        status = fbe_ext_pool_lun_get_dirty_flag(
            context,
            packet_p,
            lun_check_local_dirty_flag_completion);

        fbe_base_object_trace(
                              (fbe_base_object_t*)lun_p,
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Status is 0x%X\n", __FUNCTION__, status);

        if(status == FBE_STATUS_OK) {
            return(FBE_LIFECYCLE_STATUS_PENDING);
        }
    }
    else {
        fbe_base_object_trace(
                              (fbe_base_object_t*)lun_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Downstream health is not optimal\n", __FUNCTION__);
    }

    return(FBE_LIFECYCLE_STATUS_DONE); 
}
/**************************************
 * end fbe_ext_pool_lun_check_local_dirty_flag_cond_function()
 **************************************/

/*!**************************************************************
 * fbe_ext_pool_lun_check_peer_dirty_flag_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function for lun to check whether
 *  the lun is marked dirty. If it is, initiate a background verify
 *  to the lun
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING   
 *
 * @author
 *  03/12/2012 - Created. MFerson
 *
 ****************************************************************/

static fbe_lifecycle_status_t 
fbe_ext_pool_lun_check_peer_dirty_flag_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t                    status; 
    fbe_ext_pool_lun_t*                      lun_p   = NULL;
    fbe_lun_clean_dirty_context_t * context = NULL;
    fbe_cmi_sp_id_t                 cmi_sp_id;
    fbe_cmi_sp_id_t                 sp_id;
    fbe_atomic_t                    counter;

    fbe_base_config_downstream_health_state_t downstream_health_state;
    
    lun_p  = (fbe_ext_pool_lun_t *)object_p;

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* we need to control total number of LUN monitor packets outstanding */
    counter = fbe_atomic_increment(&fbe_lun_monitor_io_counter);
    if (counter > FBE_MAX_LUN_CONCURRENT_REQUESTS) 
    {
        fbe_atomic_decrement(&fbe_lun_monitor_io_counter);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    downstream_health_state = fbe_ext_pool_lun_verify_downstream_health(lun_p);

    if(downstream_health_state == FBE_DOWNSTREAM_HEALTH_OPTIMAL)
    {
        status = fbe_cmi_get_sp_id(&cmi_sp_id);
    
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*) lun_p,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: failed to get SP ID\n", __FUNCTION__);

            fbe_atomic_decrement(&fbe_lun_monitor_io_counter);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    
        if(cmi_sp_id == FBE_CMI_SP_ID_A)
        {
            sp_id = FBE_CMI_SP_ID_B;
        }
        else
        {
            sp_id = FBE_CMI_SP_ID_A;
        }

        context = &lun_p->clean_dirty_monitor_context;

        context->sp_id = sp_id;
        context->lun_p = lun_p;

        status = fbe_ext_pool_lun_get_dirty_flag(
             context,
             packet_p,
             lun_check_peer_dirty_flag_completion);

        if(status == FBE_STATUS_OK) {
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
    }
    else {
        fbe_atomic_decrement(&fbe_lun_monitor_io_counter);
    }


    return FBE_LIFECYCLE_STATUS_DONE;
}


/*!****************************************************************************
 * lun_check_common_dirty_flag_completion()
 ******************************************************************************
 * @brief
 *
 * @param packet_p                  - pointer to a monitor packet.
 * @param context                   - pointer to lun object.
 * @param peer_sp
 * 
 * @return fbe_status_t             - status of the operation.
 *
 * @author
 *  3/12/2012 - Created. MFerson
 ******************************************************************************/
static fbe_status_t lun_check_common_dirty_flag_completion(fbe_packet_t * in_packet_p,
                                                    fbe_packet_completion_context_t in_context,
                                                    fbe_bool_t peer_sp)
{
    fbe_status_t                    status; 
    fbe_ext_pool_lun_t*                      lun_p           = NULL;
    fbe_lun_clean_dirty_context_t * context         = NULL;
    fbe_lba_t                       block_count;
    fbe_lun_initiate_verify_t       initiate_verify;
    fbe_scheduler_hook_status_t     hook_status;

    fbe_base_object_trace((fbe_base_object_t*)in_context,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    // Get the lun pointer from the input context 
    lun_p = (fbe_ext_pool_lun_t*) in_context;

    context = &lun_p->clean_dirty_monitor_context;

    /* A status of quiesced is a transitory status so reschedule 
     * quickly (1 second).
     */
    if (context->status == FBE_STATUS_QUIESCED)
    {
        fbe_base_object_trace((fbe_base_object_t*) lun_p, FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s read failed with quiesced status 0x%x.\n", __FUNCTION__, context->status);

        /* Reschedule after a short delay.
         */
        fbe_lifecycle_reschedule(&fbe_ext_pool_lun_lifecycle_const,
                                 (fbe_base_object_t *) lun_p,
                                 (fbe_lifecycle_timer_msec_t) 1000);
        return FBE_STATUS_OK;
    }

    /* There are some error status values that we want to retry since 
     * they indicate that the object is merely not available now. 
     *  
     * We do not want to mark the LUN dirty below simply because we have seen 
     * the object go away at the time of the dirty read. 
     *  
     * Instead we will simply retry the read on the next monitor cycle. 
     */
    if ((context->status == FBE_STATUS_FAILED) ||
        (context->status == FBE_STATUS_BUSY) ||
        (context->status == FBE_STATUS_EDGE_NOT_ENABLED) ||
        (context->status == FBE_STATUS_IO_FAILED_RETRYABLE))
    {
        fbe_base_object_trace((fbe_base_object_t*) lun_p, FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s read failed with retryable status 0x%x.\n", __FUNCTION__, context->status);
        return FBE_STATUS_OK;
    }

    // If the packet status was not a success, return the packet status as is 
    if(context->status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) lun_p, FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "%s read failed, status %d\n",
            __FUNCTION__, context->status);

        context->dirty = FBE_TRUE;
    }

    if(context->dirty)
    {
        fbe_lun_get_imported_capacity(lun_p, &block_count);


        initiate_verify.start_lba           = 0;
        initiate_verify.block_count         = block_count;
        initiate_verify.verify_type         = FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE;

        fbe_base_object_trace(
            (fbe_base_object_t*)lun_p,
            FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s:Starting dirty verify for LUN 0x%x, offset 0x%llX count 0x%llX\n", 
            __FUNCTION__, 
            lun_p->base_config.base_object.object_id,
            initiate_verify.start_lba,
            initiate_verify.block_count);

        /* Make note of the fact that incomplete write verify request was started */
        fbe_ext_pool_lun_check_hook(lun_p,
                           SCHEDULER_MONITOR_STATE_LUN_INCOMPLETE_WRITE_VERIFY,
                           FBE_LUN_SUBSTATE_INCOMPLETE_WRITE_VERIFY_REQUESTED,
                           0,
                           &hook_status);

        status = fbe_transport_set_completion_function(
            in_packet_p, 
            lun_start_dirty_verify_completion, 
            lun_p);

        status = fbe_ext_pool_lun_usurper_send_initiate_verify_to_raid(
            lun_p, in_packet_p, &initiate_verify);

        return(status);
    }
    else
    {
        if(! peer_sp) {
            /* Time to check the peer's clean/dirty flag */
            status = fbe_lifecycle_set_cond(
                &fbe_ext_pool_lun_lifecycle_const,
                (fbe_base_object_t*)lun_p,
                FBE_LUN_LIFECYCLE_COND_CHECK_PEER_DIRTY_FLAG);
        }

        /* Clear the current condition */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*) lun_p);
    }

    return context->status;
}
/******************************************************************************
 * end lun_check_common_dirty_flag_completion()
 ******************************************************************************/


/*!****************************************************************************
 * lun_check_local_dirty_flag_completion()
 ******************************************************************************
 * @brief
 *
 * @param packet_p                  - pointer to a monitor packet.
 * @param context                   - pointer to lun object.
 * 
 * @return fbe_status_t             - status of the operation.
 *
 * @author
 *  3/13/2012 - Created. MFerson
 ******************************************************************************/
fbe_status_t lun_check_local_dirty_flag_completion(fbe_packet_t * in_packet_p,
                                                   fbe_packet_completion_context_t in_context)
{
    fbe_status_t status;

    fbe_base_object_trace((fbe_base_object_t*)in_context,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = lun_check_common_dirty_flag_completion(in_packet_p,
                                                   in_context,
                                                   FALSE);
    
    return status;
}
/*!****************************************************************************
 * end lun_check_local_dirty_flag_completion()
 *****************************************************************************/

/*!****************************************************************************
 * lun_check_peer_dirty_flag_completion()
 ******************************************************************************
 * @brief
 *
 * @param packet_p                  - pointer to a monitor packet.
 * @param context                   - pointer to lun object.
 * 
 * @return fbe_status_t             - status of the operation.
 *
 * @author
 *  3/13/2012 - Created. MFerson
 ******************************************************************************/
fbe_status_t lun_check_peer_dirty_flag_completion(fbe_packet_t * in_packet_p,
                                                   fbe_packet_completion_context_t in_context)
{
    fbe_status_t status;

    fbe_base_object_trace((fbe_base_object_t*)in_context,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* update the counter */
    fbe_atomic_decrement(&fbe_lun_monitor_io_counter);

    status = lun_check_common_dirty_flag_completion(in_packet_p,
                                                   in_context,
                                                   TRUE);
    
    return status;
}
/*!****************************************************************************
 * lun_check_peer_dirty_flag_completion()
 *****************************************************************************/

/*!****************************************************************************
 * lun_start_dirty_verify_completion()
 ******************************************************************************
 * @brief
 *
 * @param packet_p                  - pointer to a monitor packet.
 * @param context                   - pointer to lun object.
 * 
 * @return fbe_status_t             - status of the operation.
 *
 * @author
 *  3/13/2012 - Created. MFerson
 ******************************************************************************/
fbe_status_t lun_start_dirty_verify_completion(fbe_packet_t * in_packet_p,
                                               fbe_packet_completion_context_t in_context)
{
    fbe_ext_pool_lun_t*      lun_p = NULL;
    fbe_status_t    status;
    fbe_lifecycle_state_t lifecycle_state;

    fbe_base_object_trace((fbe_base_object_t*)in_context,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    // Get the lun pointer from the input context 
    lun_p = (fbe_ext_pool_lun_t*) in_context;

    status = fbe_transport_get_status_code(in_packet_p);

    if(status == FBE_STATUS_OK) {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);

        status = fbe_lifecycle_get_state(&fbe_ext_pool_lun_lifecycle_const, (fbe_base_object_t *)lun_p, &lifecycle_state);
    
        if(lifecycle_state == FBE_LIFECYCLE_STATE_ACTIVATE)
        {
            fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const,
                                   (fbe_base_object_t *)lun_p,
                                   FBE_LUN_LIFECYCLE_COND_MARK_LOCAL_CLEAN);
        }
    
        fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const,
                               (fbe_base_object_t *)lun_p,
                               FBE_LUN_LIFECYCLE_COND_MARK_PEER_CLEAN);
    }

    status = fbe_transport_get_status_code(in_packet_p);
    return status;
}

/******************************************************************************
 * end lun_start_dirty_verify_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_passive_request_join_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used by the passive side to request joining the active side.
 *  This is part of our coordination of the bringup of the passive SP.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  03/15/2012 - Created. MFerson
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_ext_pool_lun_passive_request_join_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_ext_pool_lun_t *  lun_p = NULL;
    fbe_bool_t          b_is_active;
    fbe_lifecycle_state_t peer_state;

    lun_p = (fbe_ext_pool_lun_t*)object_p;

    /* This condition is only used by the passive side to join.
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)lun_p);
    if (b_is_active)
    {
        /* We are active and we do not need to run this condition in activate.
         */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to clear condition %d\n", __FUNCTION__, status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Active clear passive join condition\n", __FUNCTION__);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_object_trace((fbe_base_object_t *)lun_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "MRF LUN Join Active cflags: 0x%llx Passive cflags: 0x%llx Passive state:0x%llx\n", 
                          (unsigned long long)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                          (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                          (unsigned long long)lun_p->local_state);

    fbe_ext_pool_lun_lock(lun_p);
    if(fbe_cmi_is_peer_alive() &&
       fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)lun_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN))
    {
       if (fbe_ext_pool_lun_is_peer_clustered_flag_set(lun_p, FBE_LUN_CLUSTERED_FLAG_THRESHOLD_REACHED))
       { 
           fbe_ext_pool_lun_unlock(lun_p);
           /* The peer is broken because of software errors.
            * The peer will recognize that we are in the activate state and will clear its 
            * threshold bit. 
            */
           fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN Active Failed due to sw errors. Wait peer to clear. local: 0x%llx peer: 0x%llx\n",                               
                              lun_p->lun_metadata_memory.flags,
                              lun_p->lun_metadata_memory_peer.flags); 
           return FBE_LIFECYCLE_STATUS_DONE; 
       }
        /* If we decided we are broken, then we need to set the broken condition now.
             */ 
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN Passive trying to join. Active broken. Go broken.local: 0x%llx peer: 0x%llx\n",                               
                              (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags);                              
        fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const, (fbe_base_object_t *)lun_p, 
                               FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
        fbe_ext_pool_lun_unlock(lun_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t*)lun_p, &peer_state);
    if(peer_state != FBE_LIFECYCLE_STATE_READY)
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN Passive trying to join, but peer is in %d\n",
                              peer_state);

        /* We need to restart join when peer goes away.
         */
        fbe_base_config_clear_clustered_flag((fbe_base_config_t *)lun_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK);
        fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_JOIN_MASK);
        fbe_ext_pool_lun_unlock(lun_p);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* The local state determines which function processes this condition.
     */

    if (fbe_lun_is_local_state_set(lun_p, FBE_LUN_LOCAL_STATE_JOIN_DONE))
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN PASSIVE join done objectid: %d local state: 0x%llx\n", object_p->object_id, (unsigned long long)lun_p->local_state);
        fbe_ext_pool_lun_unlock(lun_p);
        return fbe_ext_pool_lun_join_done(lun_p, packet_p);
    }
    else if (fbe_lun_is_local_state_set(lun_p, FBE_LUN_LOCAL_STATE_JOIN_STARTED))
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN PASSIVE join started objectid: %d local state: 0x%llx\n", object_p->object_id, (unsigned long long)lun_p->local_state);
        fbe_ext_pool_lun_unlock(lun_p);
        return fbe_ext_pool_lun_join_passive(lun_p, packet_p);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN PASSIVE join request objectid: %d local state: 0x%llx\n", object_p->object_id, (unsigned long long)lun_p->local_state);
        fbe_ext_pool_lun_unlock(lun_p);
        return fbe_ext_pool_lun_join_request(lun_p, packet_p);
    }
}
/******************************************************************************
 * end fbe_ext_pool_lun_passive_request_join_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_active_allow_join_cond_function()
 ******************************************************************************
 * @brief
 *  
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  10/6/2011 - Created. 03/15/2012 - Created. MFerson
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_ext_pool_lun_active_allow_join_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_ext_pool_lun_t *  lun_p = NULL;
    fbe_bool_t          b_is_active;

    lun_p = (fbe_ext_pool_lun_t*)object;

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)lun_p);

    fbe_base_object_trace((fbe_base_object_t *)lun_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "MRF LUN Join Active cflags: 0x%llx Passive cflags: 0x%llx Active state:0x%llx\n", 
                          (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                          (unsigned long long)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                          (unsigned long long)lun_p->local_state);

    /* This condition only runs on the active side, so if we are passive, then clear it now.
     */
    if (!b_is_active)
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to clear condition %d\n", __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }



    fbe_ext_pool_lun_lock(lun_p);

    /* If the peer is not there or they have not requested a join, then clear this condition.
     * The active side only needs to try to sync when the peer is present. 
     */
    if (!fbe_cmi_is_peer_alive() ||
        !fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)lun_p, 
                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ))
    {
        /* Only clear the clustered join in the case where the peer has gone away.
         * If the joined is set, the peer has successfully joined. 
         */
        if (!fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)lun_p, 
                                                        FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED))
        {
            fbe_base_config_clear_clustered_flag((fbe_base_config_t*)lun_p, 
                                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK);
        }
        fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_JOIN_MASK);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to clear condition %d\n", __FUNCTION__, status);
        }
        fbe_ext_pool_lun_unlock(lun_p);
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN Active join peer !alive or REQ !set local: 0x%llx peer: 0x%llx/0x%x state:0x%llx\n", 
                              (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                              lun_p->lun_metadata_memory_peer.base_config_metadata_memory.lifecycle_state,
                              (unsigned long long)lun_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Use the local state to determine which function processes this condition.
     */
    if (fbe_lun_is_local_state_set(lun_p, FBE_LUN_LOCAL_STATE_JOIN_STARTED))
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN ACTIVE join started objectid: %d local state: 0x%llx\n", object->object_id, (unsigned long long)lun_p->local_state);

        fbe_ext_pool_lun_unlock(lun_p);
        return fbe_ext_pool_lun_join_active(lun_p, packet_p);
    }
    else if (fbe_lun_is_local_state_set(lun_p, FBE_LUN_LOCAL_STATE_JOIN_DONE))
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN ACTIVE join done objectid: %d local state: 0x%llx\n", object->object_id, (unsigned long long)lun_p->local_state);
        fbe_ext_pool_lun_unlock(lun_p);
        return fbe_ext_pool_lun_join_done(lun_p, packet_p);
    }
    else
    {

        fbe_ext_pool_lun_unlock(lun_p);
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN ACTIVE join request objectid: %d local state: 0x%llx\n", object->object_id, (unsigned long long)lun_p->local_state);
        return fbe_ext_pool_lun_join_request(lun_p, packet_p);
    }
}
/******************************************************************************
 * end fbe_ext_pool_lun_active_allow_join_cond_function()
 ******************************************************************************/
/*!**************************************************************
 * fbe_ext_pool_lun_check_hook()
 ****************************************************************
 * @brief
 *  This is the function that calls the hook for the lun.
 *
 * @param lun_p - Pointer to the LUN Object.
 * @param state - Current state of the monitor for the caller.
 * @param substate - Sub state of this monitor op for the caller.
 * @param status - status of the hook to return.
 * 
 * @return fbe_status_t Overall status of the operation.
 *
 * @author
 *  04/26/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_ext_pool_lun_check_hook(fbe_ext_pool_lun_t *lun_p,
                                fbe_u32_t state,
                                fbe_u32_t substate,
                                fbe_u64_t val2,
                                fbe_scheduler_hook_status_t *status)
{

    fbe_status_t ret_status;
    ret_status = fbe_base_object_check_hook((fbe_base_object_t *)lun_p,
                                            state,
                                            substate,
                                            val2,
                                            status);
    return ret_status;
}
/******************************************
 * end fbe_ext_pool_lun_check_hook()
 ******************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_check_background_operations_progress_cond_function()
 ******************************************************************************
 * @brief
 *
 * Run on a timer based interval and check things that happend in the background and need
 * notifications to be sent.
 *  
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_status_t
 *
 *
 ******************************************************************************/
static fbe_status_t fbe_ext_pool_lun_check_background_operations_progress_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_ext_pool_lun_t *                             lun_p = (fbe_ext_pool_lun_t *)object_p;
    fbe_object_id_t                         my_object_id;
    fbe_block_edge_t *                      block_edge_p;
    fbe_path_attr_t                         attr;

    fbe_base_object_get_object_id((fbe_base_object_t *)lun_p, &my_object_id);
    
    /*send to RG a request to get the degraded drives (if any). 
    We have to keep a copy of the last state becuase once we get LBA_INVALID on all positions, it can mean two things:
        1) We are done rebuilding so we just need to generate a notification to say we are done
        2) We never rebuild(or already generated a notifiation), so in this case we do nothing.
    */

    /*we skip this stuff for system lun since the user don't care about them*/
    if (fbe_private_space_layout_object_id_is_system_lun(my_object_id)) {
        return FBE_STATUS_OK;
    }

    /*zero state check*/
    lun_process_zero_progress(lun_p);

    fbe_base_config_get_block_edge_ptr((fbe_base_config_t*)lun_p, &block_edge_p);
    if (block_edge_p != NULL)
    {
        status = fbe_block_transport_get_path_attributes(block_edge_p, &attr);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace(object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "lun backgroup op: Failed to get Attr.  Sts:0x%x\n", status);
            return FBE_STATUS_OK;
        }
        /* we don't need to get the rebuild status is not degraded, or previously rebuilding */
        if ((attr & FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED) || 
            (lun_p->last_rebuild_percent != 100))
        {
            /*let's get the latest rebuild status*/
            status = lun_get_rebuild_status(packet_p, lun_p);
            if ((status != FBE_STATUS_OK)&& (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)) {
                fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,"%s failed getting LUN rebuild status, status: 0x%X\n",
                                      __FUNCTION__, status);

                return FBE_STATUS_OK;
            }
        }
    }

    return status;

}

static fbe_status_t lun_get_rebuild_status(fbe_packet_t * packet, fbe_ext_pool_lun_t *lun_p)
{
    fbe_payload_control_operation_t            *control_operation = NULL;
    fbe_payload_control_operation_t            *new_control_operation = NULL;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_block_edge_t                           *edge_p = NULL; 
    /*! @note We don't wait for the completion so we cannot use the stack. */      
    fbe_raid_group_get_lun_percent_rebuilt_t   *lun_percent_rebuilt_p = &lun_p->rebuild_status;
    fbe_lba_t                                   offset;
    fbe_lba_t                                   capacity;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    new_control_operation = fbe_payload_ex_allocate_control_operation(payload); 
    if (new_control_operation == NULL) 
    {    
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Failed to allocate control operation\n",__FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        
        return FBE_STATUS_GENERIC_FAILURE;    
    }

    /* Populate the offset and capacity
     */
    fbe_lun_get_offset(lun_p, &offset);
    fbe_lun_get_imported_capacity(lun_p, &capacity);    /*! @note This should be exported */
    lun_percent_rebuilt_p->lun_capacity = capacity;
    lun_percent_rebuilt_p->lun_offset = offset;
    lun_percent_rebuilt_p->debug_flags = 0;

    fbe_payload_control_build_operation(new_control_operation,  
                                        FBE_RAID_GROUP_CONTROL_CODE_GET_LUN_PERCENT_REBUILT,  
                                        lun_percent_rebuilt_p, 
                                        sizeof(*lun_percent_rebuilt_p)); 

    fbe_payload_ex_increment_control_operation_level(payload);

    /* set the completion function. */
    fbe_transport_set_completion_function(packet, lun_get_rebuild_status_completion, lun_p);

    /* send the packet to raid object to get the checkpoint for raid extent. */
    fbe_lun_get_block_edge(lun_p, &edge_p);
    fbe_block_transport_send_control_packet(edge_p, packet);  

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}

static fbe_status_t
lun_get_rebuild_status_completion(fbe_packet_t * packet_p,
                                  fbe_packet_completion_context_t context)
{
    fbe_ext_pool_lun_t                                  *lun_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_payload_control_operation_t            *prev_control_operation_p = NULL;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_payload_control_status_t                control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_raid_group_get_lun_percent_rebuilt_t   *lun_percent_rebuilt_p = NULL;
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;

    /* Cast the context to lun object pointer. */
    lun_p = (fbe_ext_pool_lun_t *) context;

    /* The result (buffer) was stored in the lun object since this was an
     * asyncronous callback
     */
    lun_percent_rebuilt_p  = &lun_p->rebuild_status;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    prev_control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
    
    /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_set_status(prev_control_operation_p, control_status);

    /* if any of the subpacket failed with bad status then update the master status. */
    if((status != FBE_STATUS_OK) ||
       (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Failed to get info, status 0x%x, control status 0x%x\n",
                                __FUNCTION__, status, control_status);
        /* release the allocated buffer and current control operation. */
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
        return status;
    }

    /*and process it*/
    if(lun_percent_rebuilt_p->is_rebuild_in_progress)
    {
        lun_process_rebuild_progress(lun_p, lun_percent_rebuilt_p->lun_percent_rebuilt);
    }

    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);

    return FBE_STATUS_OK;
}

static void lun_process_rebuild_progress(fbe_ext_pool_lun_t *lun_p, fbe_u32_t current_rebuilt_percentage)
{
    fbe_notification_info_t     notify_info;
    fbe_object_id_t             my_object_id;
    fbe_status_t                status;
    
    /* If the debug flags are set trace some information
     */
    if (lun_p->rebuild_status.debug_flags != 0)
    {
        /* Trace some information
         */
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "lun: get percent rebuilt cur prct: %d new prct: %d chkpt: 0x%llx \n",
                              lun_p->last_rebuild_percent, current_rebuilt_percentage,
                              (unsigned long long)lun_p->rebuild_status.lun_rebuild_checkpoint);
    }

    fbe_base_object_get_object_id((fbe_base_object_t *)lun_p, &my_object_id); 

    /* Check if there has been a change in the percent rebuilt since the last
     * poll. If it changed we generation a LUN percent rebuilt notification.
     */
    if (current_rebuilt_percentage != lun_p->last_rebuild_percent)
    {
        /*is this the first time we are here ?*/
        if (lun_p->last_rebuild_percent == 100) 
        {
            notify_info.notification_data.data_reconstruction.percent_complete = 0;
            notify_info.notification_data.data_reconstruction.state = DATA_RECONSTRUCTION_START;
        }
        /*let's say the lun is also rebuilding*/
        else if ((current_rebuilt_percentage < 100) && (current_rebuilt_percentage > 0)) 
        {
            notify_info.notification_data.data_reconstruction.percent_complete = current_rebuilt_percentage;
            notify_info.notification_data.data_reconstruction.state = DATA_RECONSTRUCTION_IN_PROGRESS;
        }
        /*any chance we are done ?*/
        else if (current_rebuilt_percentage == 100) 
        {
            notify_info.notification_data.data_reconstruction.percent_complete = 100;
            notify_info.notification_data.data_reconstruction.state = DATA_RECONSTRUCTION_END;
            lun_p->rebuild_status.is_rebuild_in_progress = FBE_FALSE;
        }

        notify_info.notification_type = FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION;
        notify_info.class_id = FBE_CLASS_ID_LUN;
        notify_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_LUN;
      
        status = fbe_notification_send(my_object_id, notify_info);
        if (status != FBE_STATUS_OK) 
        { 
            fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                  FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s unexpected status: %d on notification send \n", __FUNCTION__, status);
        }
    }

    lun_p->last_rebuild_percent = current_rebuilt_percentage;/*remember for next time*/   
}

static void lun_process_zero_progress(fbe_ext_pool_lun_t *lun_p)
{
    fbe_status_t                status;
    fbe_lba_t                   lu_zero_checkpoint;
    fbe_u16_t                   zero_percent;
    fbe_notification_info_t     notify_info;
    fbe_object_id_t             my_object_id;

    fbe_base_object_get_object_id((fbe_base_object_t *)lun_p, &my_object_id);

    status = fbe_ext_pool_lun_metadata_get_zero_checkpoint(lun_p, &lu_zero_checkpoint);
    /* we want to notify when we have an even change so we reduce notifications to user space which are costly*/
    if ((lu_zero_checkpoint != lun_p->last_zero_checkpoint) && (lu_zero_checkpoint % 2 == 0)) {

        fbe_ext_pool_lun_zero_calculate_zero_percent_for_lu_extent(lun_p,
                                                      lu_zero_checkpoint,
                                                      &zero_percent);

        if (lun_p->last_zero_checkpoint == 0) {
            notify_info.notification_type = FBE_NOTIFICATION_TYPE_ZEROING;
            notify_info.class_id = FBE_CLASS_ID_LUN;
            notify_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_LUN;
            notify_info.notification_data.object_zero_notification.zero_operation_status = FBE_OBJECT_ZERO_STARTED;

            status = fbe_notification_send(my_object_id, notify_info);
            if (status != FBE_STATUS_OK) { 
                fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                      FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                                      "%s unexpected status: %d on notification send \n", __FUNCTION__, status);
            }
            
        }

        notify_info.notification_type = FBE_NOTIFICATION_TYPE_ZEROING;
        notify_info.class_id = FBE_CLASS_ID_LUN;
        notify_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_LUN;
        notify_info.notification_data.object_zero_notification.zero_operation_status = FBE_OBJECT_ZERO_IN_PROGRESS;
        notify_info.notification_data.object_zero_notification.zero_operation_progress_percent = zero_percent;


        status = fbe_notification_send(my_object_id, notify_info);
        if (status != FBE_STATUS_OK) { 
            fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                  FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s unexpected status: %d on notification send \n", __FUNCTION__, status);
        }

        if (zero_percent == 100) {
            notify_info.notification_type = FBE_NOTIFICATION_TYPE_ZEROING;
            notify_info.class_id = FBE_CLASS_ID_LUN;
            notify_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_LUN;
            notify_info.notification_data.object_zero_notification.zero_operation_status = FBE_OBJECT_ZERO_ENDED;

            status = fbe_notification_send(my_object_id, notify_info);
            if (status != FBE_STATUS_OK) { 
                fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                      FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                                      "%s unexpected status: %d on notification send \n", __FUNCTION__, status);
            }
            
        }

        lun_p->last_zero_checkpoint = lu_zero_checkpoint;


    }

}

static fbe_lifecycle_status_t fbe_ext_pool_lun_background_monitor_operation_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
#if 0
    fbe_power_save_state_t      power_save_state;
    fbe_base_config_t *         base_config_p = (fbe_base_config_t *)object_p;
    fbe_status_t                status;
    
    /* Only execute the following if `update zero checkpoint' is enabled.
     */
    if (fbe_base_config_is_system_bg_service_enabled(FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ZERO_CHKPT_UPDATE) &&
        fbe_lun_update_zero_checkpoint_need_to_run((fbe_ext_pool_lun_t *)object_p, packet_p)                                                   )
    {
        /* update the zero checkpoint. */
        return fbe_ext_pool_lun_update_zero_checkpoint_cond_function(object_p, packet_p);
    }

    /*! @todo this seems a decent place to add the zero checkpoint update once we move it out of a timer and to here
        put here anything that will 
    /*

    /* Lock base config before getting the power saving state */
    fbe_base_config_lock(base_config_p);

    /*if we made it all the way here, we can start looking at power savings*/
    fbe_base_config_metadata_get_power_save_state(base_config_p, &power_save_state);

    /* Unlock base config after getting the power saving state */
    fbe_base_config_unlock(base_config_p);

    if (power_save_state != FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE) {
        fbe_ext_pool_lun_check_power_saving_cond_function(object_p, packet_p);
    }

    /* leave this first so any checkpoints that are somehow updated will be reported up to upper layers*/
    status = fbe_ext_pool_lun_check_background_operations_progress_cond_function(object_p, packet_p);

    if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
#endif
    return FBE_LIFECYCLE_STATUS_DONE;
}
/*!****************************************************************************
 * fbe_ext_pool_lun_unexpected_error_cond_function()
 ******************************************************************************
 * @brief
 *  Send up an edge state transition to keep the cache happy.
 *  The cache will panic if it sees an I/O failure without an edge state
 *  transition.
 * 
 *  Also check to see if we hit the threshold to take the object offline.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  6/26/2012 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_ext_pool_lun_unexpected_error_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_ext_pool_lun_t *  lun_p = (fbe_ext_pool_lun_t*)object;
    fbe_u32_t num_errors;
    fbe_object_id_t object_id;
    fbe_lun_number_t lun_number;
    fbe_bool_t b_is_active;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;
    fbe_lifecycle_state_t peer_state;

    b_is_active = fbe_cmi_is_active_sp();

    fbe_base_object_get_object_id(object, &object_id);
    fbe_database_get_lun_number(object_id, &lun_number);

    fbe_lun_get_num_unexpected_errors(lun_p, &num_errors);

    if (fbe_ext_pool_lun_is_unexpected_error_limit_hit(lun_p) &&
        !fbe_ext_pool_lun_is_clustered_flag_set(lun_p, FBE_LUN_CLUSTERED_FLAG_THRESHOLD_REACHED) )
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "lun unexpected error limit hit num_errors: 0x%x\n",
                              num_errors);
        status = fbe_event_log_write(SEP_ERROR_CODE_LUN_UNEXPECTED_ERRORS,
                                     NULL, 0, "%d %x", 
                                     lun_number, object_id);
        if (status != FBE_STATUS_OK) 
        { 
            fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "lun failed to set path attribute not preferred\n");
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        
        fbe_ext_pool_lun_lock(lun_p);
        fbe_ext_pool_lun_set_clustered_flag(lun_p, FBE_LUN_CLUSTERED_FLAG_THRESHOLD_REACHED);
        fbe_ext_pool_lun_unlock(lun_p);
    }

    fbe_ext_pool_lun_check_hook(lun_p, SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                       FBE_LUN_SUBSTATE_EVAL_ERR_ENTRY, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t*)lun_p, &peer_state);

    /* If the peer is gone (or not joined) and we did not exceed the threshold we should just clear the condition and
     * move on. 
     */
    if ((!fbe_cmi_is_peer_alive() ||
         (peer_state != FBE_LIFECYCLE_STATE_READY)) &&
        !fbe_ext_pool_lun_is_unexpected_error_limit_hit(lun_p))
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "lun unexpected error cond clear.  Limit not hit. errs: 0x%x\n", num_errors);
        fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_EVAL_ERR_MASK);
        fbe_ext_pool_lun_clear_clustered_flag(lun_p, FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_MASK);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to clear condition %d\n", __FUNCTION__, status);
        }
    }
    else if ((!fbe_cmi_is_peer_alive() ||
              (peer_state != FBE_LIFECYCLE_STATE_READY)) &&
             fbe_ext_pool_lun_is_unexpected_error_limit_hit(lun_p))
    {
        /* If the peer is not present, and we hit the limit, then we will simply fail, since
         * there is no point in staying ready.  The upper levels will just continue to retry 
         * this I/O forever and it is better to just fail rather than create a system DU. 
         */ 
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN err threshold reached go broken.local: 0x%llx peer: 0x%llx pr_alive: %d\n",                               
                              lun_p->lun_metadata_memory.flags,
                              lun_p->lun_metadata_memory_peer.flags,
                              fbe_cmi_is_peer_alive());
        fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_EVAL_ERR_MASK);
        fbe_ext_pool_lun_clear_clustered_flag(lun_p, FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_MASK);
        fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const, (fbe_base_object_t *)lun_p, 
                               FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else if (fbe_lun_is_local_state_set(lun_p, FBE_LUN_LOCAL_STATE_EVAL_ERR_STARTED))
    {
        if (b_is_active)
        {
            fbe_ext_pool_lun_eval_err_active(lun_p);
        }
        else
        {
            fbe_ext_pool_lun_eval_err_passive(lun_p);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        if (!fbe_lun_is_local_state_set(lun_p, FBE_LUN_LOCAL_STATE_EVAL_ERR_REQUEST))
        {
            fbe_ext_pool_lun_lock(lun_p);
            fbe_lun_set_local_state(lun_p, FBE_LUN_LOCAL_STATE_EVAL_ERR_REQUEST);
            fbe_ext_pool_lun_unlock(lun_p);
        }
        fbe_ext_pool_lun_eval_err_request(lun_p);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_ext_pool_lun_unexpected_error_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_initiate_verify_completion()
 ******************************************************************************
 * @brief
 *  Initiate verify
 *
 * @param context                         - context.
 * @param packet_p                      - pointer to a packet.
 *
 * @return status                       - status of the operation.
 *
 * @author
 *  04/01/2013 - Created. Preethi Poddatur
 *
 ******************************************************************************/

static fbe_status_t fbe_lun_initiate_verify_completion(fbe_packet_t * packet_p,fbe_packet_completion_context_t context){

   fbe_ext_pool_lun_t* lun_p = (fbe_ext_pool_lun_t *)context;
   fbe_status_t status = FBE_STATUS_OK;

   fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_transport_get_status_code(packet_p);

   /*! On a metadata failure, cause the condition to run again by returning the packet without clearing the condition.
    */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s : Error in initiating initial verify 0x%x\n", __FUNCTION__, status);
    }
    /* Clear out the verify type so that we know this has finished.
     */
    lun_p->lun_verify.verify_type = FBE_LUN_VERIFY_TYPE_NONE;
   fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);

   return FBE_STATUS_OK;
}

/******************************
 * end fbe_lun_initiate_verify_completion()
 ******************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_verify_cond_function()
 ******************************************************************************
 * @brief
 *
 * @param 
 *  
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_DONE
 * 
 * @author
 *  3/29/2013 - Created. Preethi Poddatur
 *
 ******************************************************************************/
static fbe_lifecycle_status_t
fbe_ext_pool_lun_verify_cond_function(fbe_base_object_t *object_p, fbe_packet_t * packet_p)
{
   fbe_lifecycle_state_t lifecycle_state;
   fbe_status_t status;
   fbe_ext_pool_lun_t * lun_object = (fbe_ext_pool_lun_t *)object_p;

   status = fbe_base_object_get_lifecycle_state(object_p, &lifecycle_state);
   if (status != FBE_STATUS_OK){
      fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "Unable to get lifecycle state status: %d\n", status);
   }
   if (lifecycle_state!= FBE_LIFECYCLE_STATE_READY){
      fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_object);
      return FBE_LIFECYCLE_STATUS_DONE;
   }

   fbe_transport_set_completion_function(packet_p, fbe_lun_initiate_verify_completion, lun_object);

   fbe_ext_pool_lun_usurper_send_initiate_verify_to_raid(lun_object, packet_p, &lun_object->lun_verify);
  
   return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_ext_pool_lun_verify_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_packet_cancelled_cond_function()
 ******************************************************************************
 * @brief Lun cancelled condition handler. Spin through all the packets, and if any
 * are cancelled, cancel their sub-packets.
 * 
 *
 * @param object_p               - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  3/29/2013 - Created - Kevin Schleicher
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_ext_pool_lun_packet_cancelled_cond_function(fbe_base_object_t * object_p, 
                                                                 fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_ext_pool_lun_t *lun_p = (fbe_ext_pool_lun_t *)object_p;
    fbe_queue_head_t *termination_queue_p = &lun_p->base_config.base_object.terminator_queue_head;
    fbe_queue_element_t *queue_element_p = NULL;
    

    fbe_base_object_trace((fbe_base_object_t*)object_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    fbe_spinlock_lock(&lun_p->base_config.base_object.terminator_queue_lock);

    /* Loop over the termination queue and break out when we find something that is not
     * quiesced. 
     */
    queue_element_p = fbe_queue_front(termination_queue_p);

    while (queue_element_p != NULL)
    {
        fbe_packet_t *current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        fbe_queue_element_t *packet_queue_element = NULL;
        
        /* See if this packet has been cancelled.
         */
        if (fbe_transport_is_packet_cancelled(current_packet_p) == FBE_TRUE)
        {
            packet_queue_element = fbe_queue_front(&current_packet_p->subpacket_queue_head);

            /* Cancel all sub-packets of the cancelled master packet.
             */
            while (packet_queue_element != NULL)
            {
                fbe_packet_t *current_sub_packet_p = NULL;

                current_sub_packet_p = fbe_transport_subpacket_queue_element_to_packet(packet_queue_element);

                /* Cancel the packet, if this was already cancelled in the past, this will do nothing.
                 */
                fbe_transport_cancel_packet(current_sub_packet_p);

                /* Advance to the next packet.
                 */
                packet_queue_element = fbe_queue_next(&current_packet_p->subpacket_queue_head, packet_queue_element);
            }
        }

        /* Move to the next packet in the q.
         */
        queue_element_p = fbe_queue_next(termination_queue_p, &current_packet_p->queue_element);
    }
    fbe_spinlock_unlock(&lun_p->base_config.base_object.terminator_queue_lock);

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_ext_pool_lun_packet_cancelled_cond_function()
 ******************************************************************************/

/*******************************
 * end fbe_extent_pool_lun_monitor.c
 *******************************/




 
