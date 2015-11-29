/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the provision_drive_p object lifecycle code.
 * 
 *  This includes the
 *  @ref fbe_provision_drive_monitor_entry "provision_drive_p monitor entry point", as well as all
 *  the lifecycle defines such as rotaries and conditions, along with the actual condition
 *  functions.
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
#include "fbe_raid_library.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe_provision_drive_private.h"
#include "fbe_cmi.h"
#include "fbe_job_service.h" 
#include "fbe_transport_memory.h"
#include "fbe_database.h"
#include "fbe_notification.h"
#include "fbe_base_config_private.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_event_log_utils.h"

/* the following two Macros is used for IO to clear DDMI region */
#define CHUNK_NUMBER_PER_OPERATION_FOR_CLEAR_DDMI 0X20
#define CHUNK_SIZE_PER_OPERATION_FOR_CLEAR_DDMI 0X40

/*!***************************************************************
 * fbe_provision_drive_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the provision_drive_p's monitor.
 *
 * @param object_handle - This is the object handle, or in our case the pdo.
 * @param packet_p - The packet_p arriving from the monitor scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.             
 *
 * @author
 *  5/20/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_provision_drive_monitor_entry(fbe_object_handle_t object_handle, 
                                  fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_provision_drive_t * provision_drive_p = NULL;

    provision_drive_p = (fbe_provision_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    status = fbe_base_config_monitor_crank_object(&fbe_provision_drive_lifecycle_const,
                                        (fbe_base_object_t*)provision_drive_p, packet_p);
    return status;
}
/******************************************
 * end fbe_provision_drive_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the provision_drive_p.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the provision_drive_p's constant
 *                        lifecycle data.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_provision_drive_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(provision_drive));
}
/******************************************
 * end fbe_provision_drive_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t fbe_provision_drive_edges_detached_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_provision_drive_downstream_health_broken_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_provision_drive_downstream_health_not_optimal_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_provision_drive_downstream_health_disabled_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_status_t           pvd_fw_download_reject_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_downstream_health_broken_cond_process_slf(fbe_provision_drive_t * provision_drive_p, fbe_path_state_t path_state);

static fbe_status_t 
fbe_provision_drive_set_remove_timestamp_completion(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context);
/* Initialize the metadata memory for the provision drive object. */
static fbe_lifecycle_status_t fbe_provision_drive_metadata_memory_init_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* Initialize the object's nonpaged metadata memory. */
static fbe_lifecycle_status_t fbe_provision_drive_nonpaged_metadata_init_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* write the default paged metadata for the provision drive object during object initialization. */
static fbe_lifecycle_status_t fbe_provision_drive_write_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* Persist the default nonpaged metadata */
static fbe_lifecycle_status_t fbe_provision_drive_persist_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* write the default nopaged metadata for the provision drive object during object initialization*/
static fbe_lifecycle_status_t fbe_provision_drive_write_default_paged_metadata_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);



/* Verify the signature data for the provision drive object. */
static fbe_lifecycle_status_t fbe_provision_drive_metadata_verify_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);


/* Retrieve the metadata using metadata service and initialize the provision drive object with non paged metadata. */
static fbe_lifecycle_status_t fbe_provision_drive_metadata_element_init_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* it is used to start the background zero operation for the pvd object. */
static fbe_lifecycle_status_t fbe_provision_drive_run_background_zeroing(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* it quiesce the i/o operation for the pvd object. */
static fbe_lifecycle_status_t fbe_provision_drive_quiesce_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* set zero checkpoint to logical marker end. */
static fbe_lifecycle_status_t fbe_provision_drive_set_zero_checkpoint_to_end_marker_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* it unquiesce the i/o operation for the pvd object. */
static fbe_lifecycle_status_t fbe_provision_drive_unquiesce_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* it is used to set the end of life state in nonpaged metadata. */
static fbe_lifecycle_status_t fbe_provision_drive_set_end_of_life_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* clear the end of life state in the nonpaged metadata */
static fbe_lifecycle_status_t fbe_provision_drive_clear_end_of_life_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* it verifies current end of life state and take appropriate action. */
static fbe_lifecycle_status_t fbe_provision_drive_verify_end_of_life_state_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* it is used to update the physical drive information. */
static fbe_lifecycle_status_t fbe_provision_drive_update_physical_drive_info_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_provision_drive_reset_checkpoint_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_reset_checkpoint_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_metadata_memory_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_metadata_element_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_signature_verify_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_signature_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_nonpaged_metadata_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_write_default_paged_metadata_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_write_default_nonpaged_metadata_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);


static fbe_status_t fbe_provision_drive_set_zero_checkpoint_to_end_marker_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_run_background_zeroing_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_set_end_of_life_cond_function_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_update_physical_drive_info_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t fbe_provision_drive_run_verify( fbe_base_object_t* in_object_p,
                                                                           fbe_packet_t*      in_packet_p  );

static fbe_status_t fbe_provision_drive_run_verify_completion( fbe_packet_t*                   in_packet_p, 
                                                                   fbe_packet_completion_context_t in_context   );

static fbe_lifecycle_status_t fbe_provision_drive_do_remap_cond_function( fbe_base_object_t* in_object_p,
                                                                          fbe_packet_t*      in_packet_p  );

static fbe_lifecycle_status_t fbe_provision_drive_metadata_verify_invalidate_cond_function( fbe_base_object_t* in_object_p,
                                                                                            fbe_packet_t*      in_packet_p  );

static fbe_status_t fbe_provision_drive_do_remap_cond_completion( fbe_packet_t*                   in_packet_p, 
                                                                  fbe_packet_completion_context_t in_context   );

static fbe_status_t fbe_provision_drive_set_upstream_edge_priority( fbe_provision_drive_t *in_provision_drive_p,
                                                                    fbe_medic_action_t     in_medic_action);

static fbe_bool_t fbe_provision_drive_check_downstream_edge_priority( fbe_provision_drive_t*      in_provision_drive_p,
                                                                      fbe_medic_action_priority_t in_medic_action_priority );

static fbe_lifecycle_status_t provision_drive_hibernation_sniff_wakeup_cond_function( fbe_base_object_t* in_object_p,
                                                                                      fbe_packet_t*      in_packet_p  );


static fbe_status_t fbe_provision_drive_stripe_lock_start_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_raid_state_status_t fbe_provision_drive_iots_resend_lock_start(fbe_raid_iots_t *iots_p);

fbe_lifecycle_status_t fbe_provision_drive_pending_func(fbe_base_object_t *base_object_p, fbe_packet_t * packet);

static fbe_lifecycle_status_t fbe_provision_drive_download_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* If needed, zero the paged metadata area before we write to it*/
static fbe_lifecycle_status_t fbe_provision_drive_zero_paged_metadata_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_lifecycle_status_t 
fbe_provision_drive_background_monitor_operation_cond_function(fbe_base_object_t * object_p,
                                                               fbe_packet_t * packet_p);
static fbe_lifecycle_status_t provision_drive_passive_request_join_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t provision_drive_active_allow_join_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t provision_drive_join_sync_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_set_zero_checkpoint_to_end_marker(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);                                                      
static fbe_status_t fbe_provision_drive_set_zero_checkpoint_to_end_marker_verify_flags(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);                                                      


static fbe_status_t fbe_provision_drive_set_end_of_life(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t provision_drive_wait_for_config_commit_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);                                                                       
static fbe_status_t 
fbe_provision_drive_send_destroy_request(fbe_provision_drive_t *provision_drive_p,
                                     fbe_job_service_destroy_provision_drive_t * pvd_destroy_request);
static fbe_status_t fbe_provision_drive_run_metadata_verify_invalidate_completion(fbe_packet_t * packet_p,
                                                                                  fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t
fbe_provision_drive_run_metadata_verify_invalidate(fbe_base_object_t * object_p, 
                                                   fbe_packet_t * packet_p);


static fbe_lifecycle_status_t fbe_provision_drive_remap_done_cond_function( fbe_base_object_t*  object_p, fbe_packet_t* packet_p);
                                         
static fbe_status_t fbe_provision_drive_remap_done_cond_completion( fbe_packet_t*  packet_p, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t fbe_provision_drive_send_remap_event_cond_function(fbe_base_object_t*  in_object_p, fbe_packet_t* in_packet_p);

static fbe_lifecycle_status_t fbe_provision_drive_clear_event_flag_cond_function(fbe_base_object_t*  in_object_p, fbe_packet_t*       in_packet_p);
static fbe_status_t fbe_provision_drive_set_invalidate_checkpoint_clear_condition_completion(fbe_packet_t * packet_p,
                                                                                             fbe_packet_completion_context_t context);
                                           
static fbe_lifecycle_status_t provision_drive_zero_started_cond_function( fbe_base_object_t* in_object_p, fbe_packet_t*      in_packet_p  );
static fbe_lifecycle_status_t provision_drive_zero_ended_cond_function( fbe_base_object_t* in_object_p, fbe_packet_t*      in_packet_p  );
static fbe_status_t 
fbe_provision_drive_set_verify_invalidate_checkpoint_to_zero_with_np_lock(fbe_packet_t * packet_p,
                                                                          fbe_packet_completion_context_t context);
static fbe_status_t 
fbe_provision_drive_reset_verify_invalidate_checkpoint_with_np_lock(fbe_packet_t * packet_p,
                                                                    fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t fbe_provision_drive_verify_invalidate_remap_request_cond( fbe_base_object_t* in_object_p, 
                                                                                        fbe_packet_t*      in_packet_p  );
static fbe_lifecycle_status_t fbe_provision_drive_eval_slf_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_clear_nonpaged_completion(fbe_packet_t* packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_pvd_metadata_reinit_memory_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t  provision_drive_clear_np_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t  fbe_provision_drive_notify_server_logically_online_cond(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_provision_drive_set_drive_fault_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_set_drive_fault_persist_completion(fbe_packet_t * packet_p,
                                                                           fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t fbe_provision_drive_clear_drive_fault_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_clear_drive_fault_persist_completion(fbe_packet_t * packet_p,
                                                                             fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_clear_drive_fault_inform_pdo_completion(fbe_packet_t * packet_p,
                                                                                fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t fbe_provision_drive_verify_drive_fault_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_provision_drive_health_check_passive_side_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_provision_drive_paged_metadata_needs_zero_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_provision_drive_paged_metadata_reconstruct_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_provision_drive_clear_DDMI_region_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_provision_drive_peer_death_release_sl_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_provision_drive_reinit_np_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
    
static fbe_status_t fbe_provision_drive_clear_DDMI_region_allocation_completion(fbe_memory_request_t *memory_request, fbe_memory_completion_context_t context);
static fbe_status_t fbe_provision_drive_clear_DDMI_region_io_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_set_drive_fault(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_clear_drive_fault(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t fbe_provision_drive_initiate_health_check(fbe_base_object_t* in_object_p, fbe_packet_t* in_packet_p);
static fbe_lifecycle_status_t fbe_provision_drive_stripe_lock_start_cond_function(fbe_base_object_t * object,
                                                                                  fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_inform_logical_state(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_inform_logical_state_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_bool_t fbe_provision_drive_is_encryption_key_needed(fbe_provision_drive_t * provision_drive,
                                                               fbe_edge_index_t client_index);
static fbe_status_t fbe_provision_drive_background_monitor_handle_encryption_operations(fbe_provision_drive_t * provision_drive_p);
static fbe_lifecycle_status_t fbe_provision_drive_monitor_unregister_keys_if_needed(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_provision_drive_monitor_destroy_unregister_keys(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
static fbe_bool_t fbe_provision_drive_is_md_memory_initialized(fbe_provision_drive_t * provision_drive_p);
static fbe_lifecycle_status_t fbe_provision_drive_encryption_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_get_port_object_id(fbe_provision_drive_t *provision_drive_p, fbe_packet_t * packet_p, fbe_object_id_t *port_object_id);
static fbe_lifecycle_status_t fbe_provision_drive_verify_drive_offline_clear_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_can_exit_failed_state(fbe_provision_drive_t *provision_drive_p, fbe_bool_t *b_can_exit_failed_p);
static fbe_lifecycle_status_t fbe_provision_drive_get_wear_leveling(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_get_wear_level_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(provision_drive);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(provision_drive);



/*  provision_drive_lifecycle_callbacks
 *  This variable has all the callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(provision_drive) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        provision_drive,
        FBE_LIFECYCLE_NULL_FUNC,             /* online function */
        fbe_provision_drive_pending_func)    /* pending function */
};


/*--- constant derived condition entries -----------------------------------------------*/
static fbe_lifecycle_const_cond_t provision_drive_downstream_health_broken_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN,
        fbe_provision_drive_downstream_health_broken_cond_function)
};

static fbe_lifecycle_const_cond_t provision_drive_downstream_health_disabled_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED,
        fbe_provision_drive_downstream_health_disabled_cond_function)
};

static fbe_lifecycle_const_cond_t provision_drive_downstream_health_not_optimal_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_NOT_OPTIMAL,
        fbe_provision_drive_downstream_health_not_optimal_cond_function)
};

/* provision drive metadata memory initialization condition function. */
static fbe_lifecycle_const_cond_t provision_drive_metadata_memory_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_MEMORY_INIT,
        fbe_provision_drive_metadata_memory_init_cond_function)
};

/* provision drive nonpaged metadata initialization condition function. */
static fbe_lifecycle_const_cond_t provision_drive_nonpaged_metadata_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_NONPAGED_METADATA_INIT,
        fbe_provision_drive_nonpaged_metadata_init_cond_function)         
};

/* provision drive metadata metadata element init condition function. */
static fbe_lifecycle_const_cond_t provision_drive_metadata_element_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_ELEMENT_INIT,
        fbe_provision_drive_metadata_element_init_cond_function)         
};

/* provision drive metadata signature verification condition function. */
static fbe_lifecycle_const_cond_t provision_drive_metadata_verify_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_VERIFY,
        fbe_provision_drive_metadata_verify_cond_function)
};


/* provision drive metadata element init condition function. */
static fbe_lifecycle_const_cond_t provision_drive_write_default_paged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_PAGED_METADATA,
        fbe_provision_drive_write_default_paged_metadata_cond_function)
};

/* provision drive metadata metadata element init condition function. */
static fbe_lifecycle_const_cond_t provision_drive_write_default_nonpaged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_NONPAGED_METADATA,
        fbe_provision_drive_write_default_nonpaged_metadata_cond_function)
};

/* provision drive metadata default nonpaged persist condition function. */
static fbe_lifecycle_const_cond_t provision_drive_persist_default_nonpaged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_PERSIST_DEFAULT_NONPAGED_METADATA,
        fbe_provision_drive_persist_default_nonpaged_metadata_cond_function)
};

static fbe_lifecycle_const_cond_t provision_drive_hibernation_sniff_wakeup_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_HIBERNATION_SNIFF_WAKEUP_COND,
        provision_drive_hibernation_sniff_wakeup_cond_function)
};

/* raid group zero consumed space condition function. */
static fbe_lifecycle_const_cond_t provision_drive_zero_paged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_ZERO_CONSUMED_AREA,
        fbe_provision_drive_zero_paged_metadata_cond_function)
};

static fbe_lifecycle_const_cond_t provision_drive_quiesce_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE,
        fbe_provision_drive_quiesce_cond_function)
};

static fbe_lifecycle_const_cond_t provision_drive_unquiesce_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE,
        fbe_provision_drive_unquiesce_cond_function)
};

/* provision drive common background monitor operation condition function. */
static fbe_lifecycle_const_cond_t provision_drive_background_monitor_operation_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_BACKGROUND_MONITOR_OPERATION,
        fbe_provision_drive_background_monitor_operation_cond_function)
};

/* provision drive metadata verify condition function. */
static fbe_lifecycle_const_cond_t provision_drive_metadata_verify_operation_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_BACKGROUND_MONITOR_OPERATION,
        fbe_provision_drive_background_monitor_operation_cond_function)
};

static fbe_lifecycle_const_cond_t provision_drive_passive_request_join_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_PASSIVE_REQUEST_JOIN,
        provision_drive_passive_request_join_cond_function)
};

static fbe_lifecycle_const_cond_t provision_drive_active_allow_join_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_ACTIVE_ALLOW_JOIN,
        provision_drive_active_allow_join_cond_function)
};

static fbe_lifecycle_const_cond_t provision_drive_wait_for_config_commit_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_WAIT_FOR_CONFIG_COMMIT,
        provision_drive_wait_for_config_commit_function)
};

static fbe_lifecycle_const_cond_t provision_drive_join_sync_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(FBE_BASE_CONFIG_LIFECYCLE_COND_JOIN_SYNC,
                                         provision_drive_join_sync_cond_function)
};

static fbe_lifecycle_const_cond_t provision_drive_clear_np_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(FBE_BASE_CONFIG_LIFECYCLE_COND_CLEAR_NP,
                                         provision_drive_clear_np_cond_function)
};

static fbe_lifecycle_const_cond_t provision_drive_set_drive_fault_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(FBE_BASE_CONFIG_LIFECYCLE_COND_SET_DRIVE_FAULT,
                                         fbe_provision_drive_set_drive_fault_cond_function)
};

static fbe_lifecycle_const_cond_t provision_drive_clear_drive_fault_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(FBE_BASE_CONFIG_LIFECYCLE_COND_CLEAR_DRIVE_FAULT,
                                         fbe_provision_drive_clear_drive_fault_cond_function)
};

static fbe_lifecycle_const_cond_t provision_drive_peer_death_release_sl_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(FBE_BASE_CONFIG_LIFECYCLE_COND_PEER_DEATH_RELEASE_SL,
                                         fbe_provision_drive_peer_death_release_sl_function)
};

static fbe_lifecycle_const_cond_t provision_drive_reinit_np_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(FBE_BASE_CONFIG_LIFECYCLE_COND_REINIT_NP,
                                         fbe_provision_drive_reinit_np_cond_function)
};

/* Purpose of this condition is to start stripe locking with the peer. */
static fbe_lifecycle_const_cond_t provision_drive_stripe_lock_start_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_STRIPE_LOCK_START,
       fbe_provision_drive_stripe_lock_start_cond_function)
};

static fbe_lifecycle_const_cond_t provision_drive_encryption_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(FBE_BASE_CONFIG_LIFECYCLE_COND_ENCRYPTION,
                                         fbe_provision_drive_encryption_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/


/*
 * remap condition entry
 */
static fbe_lifecycle_const_base_cond_t provision_drive_do_remap_cond =
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_do_remap_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_DO_REMAP,
        fbe_provision_drive_do_remap_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_READY,               /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY    */
};

/*
 * Indicates that upstream object have marked the chunk for verify
 */
static fbe_lifecycle_const_base_cond_t provision_drive_remap_done_cond =
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_remap_done_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_REMAP_DONE,
        fbe_provision_drive_remap_done_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_READY,               /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY    */
};

/*
 * Indicates that upstream object have marked the chunk for verify
 */
static fbe_lifecycle_const_base_cond_t provision_drive_send_remap_event_cond =
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_send_remap_event_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_SEND_REMAP_EVENT,
        fbe_provision_drive_send_remap_event_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_READY,               /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY    */
};

/*
 * Indicates that upstream object have marked the chunk for verify
 */
static fbe_lifecycle_const_base_cond_t provision_drive_clear_event_flag_cond =
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_clear_event_flag_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_CLEAR_EVENT_FLAG,
        fbe_provision_drive_clear_event_flag_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_READY,               /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY    */
};

/* 
 * provision drive quiesce i/o condition entry.
 */
static fbe_lifecycle_const_base_cond_t provision_drive_set_zero_checkpoint_to_end_marker_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_set_zero_checkpoint_to_end_marker_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_SET_ZERO_CHECKPOINT_TO_END_MARKER,
        fbe_provision_drive_set_zero_checkpoint_to_end_marker_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* 
 * provision drive persist eol bit in nonpaged metadata condition entry.
 */
static fbe_lifecycle_const_base_cond_t provision_drive_set_end_of_life_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_set_end_of_life_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_SET_END_OF_LIFE,
        fbe_provision_drive_set_end_of_life_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* 
 * provision drive persist eol bit in nonpaged metadata condition entry.
 */
static fbe_lifecycle_const_base_cond_t provision_drive_clear_end_of_life_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_clear_end_of_life_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_CLEAR_END_OF_LIFE,
        fbe_provision_drive_clear_end_of_life_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* 
 * provision drive verify end of life status of the object.
 */
static fbe_lifecycle_const_base_cond_t provision_drive_verify_end_of_life_state_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_verify_end_of_life_state_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_VERIFY_END_OF_LIFE_STATE,
        fbe_provision_drive_verify_end_of_life_state_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* 
 * provision drive update physical drive information condition, it is used to update the
 * nonpaged metadata with physical drive information.
 */
static fbe_lifecycle_const_base_cond_t provision_drive_update_physical_drive_info_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_update_physical_drive_info_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_UPDATE_PHYSICAL_DRIVE_INFO,
        fbe_provision_drive_update_physical_drive_info_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* 
 * When the PVD starts up, if zod is set, then reset the checkpoint to 0. 
 * We do this since the checkpoint does not get persisted at runtime. 
 * If any zero request came to move the checkpoint back it does not get persisted. 
 */
static fbe_lifecycle_const_base_cond_t provision_drive_reset_checkpoint_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_reset_checkpoint_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_RESET_CHECKPOINT,
        fbe_provision_drive_reset_checkpoint_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};



/* 
 * provision drive download condition entry.
 */
static fbe_lifecycle_const_base_cond_t provision_drive_download_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_download_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_DOWNLOAD,
        fbe_provision_drive_download_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/*
 * Metadata verify condition entry
 */
static fbe_lifecycle_const_base_cond_t provision_drive_metadata_verify_invalidate_cond =
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_metadata_verify_invalidate_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_METADATA_VERIFY_INVALIDATE,
        fbe_provision_drive_metadata_verify_invalidate_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_READY,               /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY    */
};


static fbe_lifecycle_const_base_cond_t provision_drive_zero_started_cond =
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_zero_started_or_ended_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_ZERO_STARTED,
        provision_drive_zero_started_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,            /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_READY,              /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,            /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,            /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY    */
};
static fbe_lifecycle_const_base_cond_t provision_drive_zero_ended_cond =
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_zero_ended_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_ZERO_ENDED,
        provision_drive_zero_ended_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,            /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_READY,              /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,            /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,            /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY    */
};

/*
 * single loop failure condition entry
 */
static fbe_lifecycle_const_base_cond_t provision_drive_eval_slf_cond =
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_eval_slf_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_EVAL_SLF,
        fbe_provision_drive_eval_slf_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
}; 

/*
 * Indicates to send a request to upstream object so that they can mark for verify
 */
static fbe_lifecycle_const_base_cond_t provision_drive_verify_invalidate_remap_request_cond =
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_provision_drive_verify_invalidate_remap_request_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_VERIFY_INVALIDATE_REMAP_REQUEST,
        fbe_provision_drive_verify_invalidate_remap_request_cond),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_READY,               /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY    */
};

/*
 * Notifiy PDO that drive is logically online
*/
static fbe_lifecycle_const_base_cond_t provision_drive_notify_server_logically_online_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_provision_drive_notify_server_logically_online_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_NOTIFY_SERVER_LOGICALLY_ONLINE,
        fbe_provision_drive_notify_server_logically_online_cond),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_READY,               /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY    */
};
/* 
 * provision drive verify drive fault status of the object.
 */
static fbe_lifecycle_const_base_cond_t provision_drive_verify_drive_fault_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_verify_drive_fault_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_VERIFY_DRIVE_FAULT,
        fbe_provision_drive_verify_drive_fault_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/*
 * Condition is used to put the non-originator side into activate state in order
 * to hold IO off and then allow originator to issue a health check to the drive.
 */
static fbe_lifecycle_const_base_cond_t provision_drive_health_check_passive_side_cond =
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_health_check_passive_side_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_HEALTH_CHECK_PASSIVE_SIDE,
        fbe_provision_drive_health_check_passive_side_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY    */
};

/* FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_METADATA_NEEDS_ZERO
 *
 * The purpose of this condition is to zero the paged after a encrypted drive is swapped
 * out.
 */
static fbe_lifecycle_const_base_cond_t provision_drive_paged_metadata_needs_zero_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_paged_metadata_needs_zero_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_METADATA_NEEDS_ZERO,
        fbe_provision_drive_paged_metadata_needs_zero_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};
/* FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_METADATA_RECONSTRUCT
 *
 * The purpose of this condition is reconstruct the paged metadata using default values
 */
static fbe_lifecycle_const_base_cond_t provision_drive_paged_metadata_reconstruct_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_paged_metadata_reconstruct_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_METADATA_RECONSTRUCT,
        fbe_provision_drive_paged_metadata_reconstruct_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};
/* FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_RECONSTRUCT_READY
 *
 * The purpose of this condition is reconstruct the paged metadata using default values
 */
static fbe_lifecycle_const_base_cond_t provision_drive_paged_reconstruct_ready_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_paged_reconstruct_ready_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_RECONSTRUCT_READY,
        fbe_provision_drive_paged_metadata_reconstruct_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* 
 * provision drive clear the DDMI region drive.
 */
static fbe_lifecycle_const_base_cond_t provision_drive_clear_DDMI_region_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "provision_drive_clear_DDMI_region_cond",
        FBE_PROVISION_DRIVE_LIFECYCLE_COND_CLEAR_DDMI_REGION,
        fbe_provision_drive_clear_DDMI_region_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};


/* provision_drive_lifecycle_base_cond_array
 * This is our static list of base conditions.
 */
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(provision_drive)[] =
{
    &provision_drive_do_remap_cond,
    &provision_drive_remap_done_cond,
    &provision_drive_send_remap_event_cond,
    &provision_drive_clear_event_flag_cond,
    &provision_drive_set_zero_checkpoint_to_end_marker_cond,    
    &provision_drive_set_end_of_life_cond,
    &provision_drive_verify_end_of_life_state_cond,
    &provision_drive_update_physical_drive_info_cond,
    &provision_drive_download_cond,
    &provision_drive_metadata_verify_invalidate_cond,
    &provision_drive_clear_end_of_life_cond,
    &provision_drive_zero_started_cond,
    &provision_drive_zero_ended_cond,
    &provision_drive_eval_slf_cond,
    &provision_drive_verify_invalidate_remap_request_cond,
    &provision_drive_notify_server_logically_online_cond,
    &provision_drive_verify_drive_fault_cond,
    &provision_drive_health_check_passive_side_cond,
    &provision_drive_paged_metadata_needs_zero_cond,
    &provision_drive_paged_metadata_reconstruct_cond,
    &provision_drive_clear_DDMI_region_cond,
    &provision_drive_reset_checkpoint_cond,
    &provision_drive_paged_reconstruct_ready_cond,
};

/* provision_drive_lifecycle_base_conditions 
 *  This is the list of base default base conditions for the provision
 *  drive object.
 */
FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(provision_drive);



/*--- constant rotaries ----------------------------------------------------------------*/
static fbe_lifecycle_const_rotary_cond_t provision_drive_specialize_rotary[] = {
    /* Derived conditions */    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_wait_for_config_commit_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_clear_np_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_metadata_memory_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_nonpaged_metadata_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_downstream_health_not_optimal_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), 
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_metadata_element_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_stripe_lock_start_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_reinit_np_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /*!@note following five condition needs to be in order, do not change the order without valid reason */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_clear_DDMI_region_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_write_default_nonpaged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_zero_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_write_default_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_persist_default_nonpaged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /*!@note previous five condition needs to be in order, do not change the order without valid reason */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_metadata_verify_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    /* Base conditions */
};


static fbe_lifecycle_const_rotary_cond_t provision_drive_activate_rotary[] = {
    /* Derived conditions */

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_encryption_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_downstream_health_disabled_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_update_physical_drive_info_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_reset_checkpoint_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_clear_end_of_life_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_verify_drive_fault_cond,   FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_verify_end_of_life_state_cond,   FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_peer_death_release_sl_cond,             FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    
    /* Base conditions */    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_health_check_passive_side_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_paged_metadata_needs_zero_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_paged_metadata_reconstruct_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* This condition is intentionally at the end of the activate rotary since we are coordinating the transition of a
     * passive object to ready.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_passive_request_join_cond,       FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t provision_drive_ready_rotary[] =
{
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_join_sync_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_encryption_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_peer_death_release_sl_cond,             FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_update_physical_drive_info_cond,        FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_set_end_of_life_cond,                   FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_verify_end_of_life_state_cond,          FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_quiesce_cond,                           FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_eval_slf_cond,                          FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_paged_reconstruct_ready_cond,           FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_set_zero_checkpoint_to_end_marker_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_download_cond,                          FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),  
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_zero_started_cond,                      FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_zero_ended_cond,                        FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /*!@note Condition which requires quiesce i/o will be added here. */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_unquiesce_cond,                         FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_clear_event_flag_cond,                  FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_remap_done_cond,                        FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_do_remap_cond,                          FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_send_remap_event_cond,                  FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_verify_invalidate_remap_request_cond,   FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /* This condition is intentionally after all conditions that require peer to peer coordination.
     * This allows the peer to join us without interfering with any of the above conditions. 
     */
         
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_active_allow_join_cond,                 FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_metadata_verify_invalidate_cond,        FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_notify_server_logically_online_cond,    FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_reset_checkpoint_cond,                  FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_background_monitor_operation_cond,      FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t provision_drive_fail_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_encryption_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_peer_death_release_sl_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_clear_np_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_set_drive_fault_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_clear_drive_fault_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_downstream_health_broken_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /* Base conditions */
};

static fbe_lifecycle_const_rotary_cond_t provision_drive_hibernate_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_peer_death_release_sl_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_hibernation_sniff_wakeup_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
        
};

static fbe_lifecycle_const_rotary_cond_t provision_drive_destroy_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(provision_drive_encryption_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
};


static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(provision_drive)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, provision_drive_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, provision_drive_activate_rotary), 
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, provision_drive_ready_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_FAIL, provision_drive_fail_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_HIBERNATE, provision_drive_hibernate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, provision_drive_destroy_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(provision_drive);


/*--- global provision_drive_p lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(provision_drive) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        provision_drive,
        FBE_CLASS_ID_PROVISION_DRIVE,            /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_config))  /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/

/*--- Condition Functions --------------------------------------------------------------*/

/*!****************************************************************************
 * fbe_provision_drive_downstream_heath_broken_cond_function()
 ******************************************************************************
 * @brief
 *  This is the derived condition function for provision drive object where it 
 *  tries to detach the edge with downstream object if edge change its state 
 *  to BROKEN/GONE, it will clear this condition if edge state changes to 
 *  ENABLED.
 *
 * @param object_p                - The ptr to the base object.
 * @param packet_p                - The packet_p sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is be based on parent
 *                                  function call.
 * call.
 *
 * @author
 *  05/21/2009 - Created. Dhaval Patel.
 *  01/16/2012 - Modified by zhangy. Add time stamp process for pvd garbage collection.
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_downstream_health_broken_cond_function(fbe_base_object_t * object_p, 
                                                           fbe_packet_t * packet_p)
{
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t*)object_p;    
    fbe_block_edge_t *      edge_p = NULL;
    fbe_path_state_t        path_state;
    fbe_path_attr_t         path_attr;
    fbe_bool_t              should_removed = FBE_FALSE;
    fbe_job_service_destroy_provision_drive_t * pvd_destroy_request;
    fbe_system_time_t       old_time_stamp;
    fbe_status_t            status; 
    fbe_bool_t              b_is_active;
    fbe_bool_t              is_metadata_initialized = FBE_FALSE;
    fbe_object_id_t         object_id;
    fbe_provision_drive_local_state_t local_state = 0;
    fbe_provision_drive_nonpaged_metadata_t *   provision_drive_nonpaged_metadata_p = NULL;
    fbe_lifecycle_status_t key_status;
    fbe_scheduler_hook_status_t                 hook_status;
    fbe_bool_t              b_can_exit_failed = FBE_FALSE;

    fbe_provision_drive_check_hook(provision_drive_p, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_FAIL,
                                   FBE_PROVISION_DRIVE_SUBSTATE_DS_HEALTH_BROKEN, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_zero_memory(&old_time_stamp,sizeof(fbe_system_time_t));
    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    fbe_base_object_get_object_id(object_p, &object_id);

    /* Check the path state of the edge */
    fbe_base_config_get_block_edge( (fbe_base_config_t *) provision_drive_p, &edge_p, 0);
    fbe_block_transport_get_path_state(edge_p, &path_state);
    fbe_block_transport_get_path_attributes(edge_p, &path_attr);

    fbe_base_config_metadata_nonpaged_is_initialized((fbe_base_config_t*)provision_drive_p, &is_metadata_initialized);
    
    /*****************************************************************************************************
     * If downstream is enabled now but the metadata has not been initialized or it needs to be reinitialized, we transits from FAIL
     * to SPEC state, where it can do the metadata init, including init metadata element, write default nonpaged and paged MD, etc.
     * 
     * This happens under system PVD reinitialization story, where a system drive is replaced by a new one. Before here, the nonpaged
     * MD of this PVD has been cleared.
     *
     * There are two cases:
     * (1) The PVD (active or passive) nonpaged MD is cleared when we arrive here, so is_metadata_initialized = FALSE
     * (2) In the case when the drive becomes online slower on passive side (it originally was passive side, or it originally was active 
     *      side, but peer side's drive comes online first, so it requests to become passive object), the PVD is connected to
     *      downstream drive after peer PVD finishes all the reinit work in SPEC state but before peer PVD transits to READY state. Then
     *      is_metadata_initialized = FALSE because when peer write default nonpaged MD, this PVD's nonpaged MD has also been 
     *      updated. In this case, we would have to depend on the FBE_METADATA_ELEMENT_ATTRIBUTES_ELEMENT_REINITED attrib.
     *
     *      - AR 567642
     *      - Zhipeng Hu  06/23/2013
     *****************************************************************************************************/
    if ((path_state == FBE_PATH_STATE_ENABLED) && 
        (is_metadata_initialized == FBE_FALSE ||
        fbe_provision_drive_metadata_need_reinitialize(provision_drive_p)))
    {
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *)provision_drive_p, FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_VERIFY);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_provision_drive_lock(provision_drive_p);

    fbe_provision_drive_get_local_state(provision_drive_p, &local_state);


    /* If drive fw download was requested and we went to FAIL then reject the request.
    */
    if (FBE_PVD_LOCAL_STATE_DOWNLOAD_MASK & local_state)
    {

        fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "PVD downstream_health_broken_cond_function, fwdl_reject object_id:0x%x local_state:0x%llx\n", object_id, local_state);

        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_MASK);
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_MASK);

        fbe_provision_drive_unlock(provision_drive_p);

        return fbe_provision_drive_send_download_command_to_pdo(provision_drive_p, packet_p,
                                                                FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_REJECT,
                                                                pvd_fw_download_reject_completion);        
    }

    /* If drive health check was request and we went to FAIL then reject the request. */
    if (fbe_provision_drive_is_health_check_needed(path_state, path_attr))
    {
        fbe_provision_drive_unlock(provision_drive_p);

        return fbe_provision_drive_health_check_abort_req_originator(provision_drive_p, packet_p);
    }

    if(!fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN))
    {
        fbe_base_config_set_clustered_flag((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    fbe_provision_drive_unlock(provision_drive_p);

    /* Process SLF in the condition function. */
    fbe_provision_drive_downstream_health_broken_cond_process_slf(provision_drive_p, path_state);

    /* We may need to inform PDO about the logical state change. */            
    if (fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_CHECK_LOGICAL_STATE) &&
        (path_state!= FBE_PATH_STATE_INVALID) && (path_state!= FBE_PATH_STATE_GONE))
    {
        status = fbe_provision_drive_inform_logical_state(provision_drive_p, packet_p);
        if (status == FBE_STATUS_PENDING)
        {
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
    }

    /* Unregister keys first. */
    key_status = fbe_provision_drive_monitor_unregister_keys_if_needed(provision_drive_p, packet_p);
    if (key_status != FBE_LIFECYCLE_STATUS_CONTINUE)
    {
        return key_status;
    }

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);
    /* Check path state of the configure edge */
    switch(path_state)
    {
            case FBE_PATH_STATE_ENABLED:
                /* Check whether the PVD failed because of both sides have different drive info */
                if (!fbe_provision_drive_check_drive_location(provision_drive_p))
                {
                    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s drive info not match\n", __FUNCTION__);
                    return FBE_LIFECYCLE_STATUS_DONE;
                }

                /* Invoke the routine to detemine if we can exit the failed state or not.
                 */
                status = fbe_provision_drive_can_exit_failed_state(provision_drive_p, &b_can_exit_failed);
                if (status != FBE_STATUS_OK) {
                    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s - can exit failed status: 0x%x \n",
                                          __FUNCTION__, status);
                    return FBE_LIFECYCLE_STATUS_DONE;
                }

                /* If we are allowed to exit failed do it now.
                 */
                if (b_can_exit_failed) {
                   /* clear the downstream health broken condition. */
                    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)provision_drive_p);
                    /* Jump to Activate state */            
                    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                           (fbe_base_object_t *)provision_drive_p, 
                                           FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);

                    if (fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_EAS_COMPLETE)) {
                        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                               (fbe_base_object_t *) provision_drive_p,
                                               FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_METADATA_RECONSTRUCT);
                    }
                }

                return FBE_LIFECYCLE_STATUS_DONE;
                break;
            case FBE_PATH_STATE_BROKEN:
                fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s path state broken\n", __FUNCTION__); 
                break;
            case FBE_PATH_STATE_GONE: /* we need to detach the edge */
                fbe_base_config_detach_edge((fbe_base_config_t *) provision_drive_p, packet_p, edge_p);

                /*get drive  time stamp */
                if(b_is_active) {
                    status = fbe_provision_drive_metadata_get_time_stamp(provision_drive_p,&old_time_stamp);
                    if(status != FBE_STATUS_OK) {
                        fbe_provision_drive_utils_trace( provision_drive_p,
                                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                      FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                      "%s failed to get remove_timestamp from PVD nonpaged metadata.\n", __FUNCTION__);
                    }else{
                        if (fbe_provision_drive_system_time_is_zero(&old_time_stamp) && !(fbe_private_space_layout_object_id_is_system_pvd(object_id))) {
                            status = fbe_provision_drive_metadata_set_time_stamp(provision_drive_p,packet_p,FBE_FALSE);
                            if(status != FBE_STATUS_OK) {
                                fbe_provision_drive_utils_trace( provision_drive_p,
                                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                                  "%s failed to set remove_timestamp in PVD nonpaged metadata.\n", __FUNCTION__);
                            }
                            fbe_provision_drive_utils_trace( provision_drive_p,
                                                             FBE_TRACE_LEVEL_INFO,
                                                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                             FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                                             "%s: Path gone. set remove_timestamp for this PVD.\n", __FUNCTION__);
                            return FBE_LIFECYCLE_STATUS_PENDING;
                        }
                    }
                }
                break;
            case FBE_PATH_STATE_INVALID: /* Do nothing for now */
                break;
            case FBE_PATH_STATE_DISABLED: /* Do nothing since not enabled yet.*/
                break;
            default:
                fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s Invalid path state %d\n", __FUNCTION__, path_state);                
    }

    /* If downstream health is not optimal, we set the start time*/
    if (path_state != FBE_PATH_STATE_ENABLED) {
        if (fbe_provision_drive_get_ds_health_not_optimal_start_time(provision_drive_p) == 0)
        {
            fbe_provision_drive_set_ds_health_not_optimal_start_time(provision_drive_p, fbe_get_time());
        }
    }
    /* Only active side can destroy PVD */
    if((path_state != FBE_PATH_STATE_ENABLED)&&(b_is_active)) {
        /* check whether an PVD should be destroyed or not */
        status = fbe_provision_drive_should_permanently_removed(provision_drive_p,&should_removed);
        if(status != FBE_STATUS_OK) {
            fbe_provision_drive_utils_trace(provision_drive_p,
                                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                            FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                            "%s failed to check PVD is permanently removed. \n", __FUNCTION__);
        }
        /* if it is, send the destroy request to job service */
       if(should_removed) {
           fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                 FBE_TRACE_LEVEL_INFO,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s next to remove this PVD \n", __FUNCTION__);                
            pvd_destroy_request = (fbe_job_service_destroy_provision_drive_t *)fbe_transport_allocate_buffer();
            pvd_destroy_request->object_id = provision_drive_p->base_config.base_object.object_id;
            status = fbe_provision_drive_send_destroy_request(provision_drive_p,pvd_destroy_request);
            if(status != FBE_STATUS_OK) {
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                                "%s failed to destroy PVD  \n", __FUNCTION__);

                fbe_transport_release_buffer(pvd_destroy_request);   
                }else{
                fbe_lifecycle_clear_current_cond((fbe_base_object_t *)provision_drive_p);
                fbe_transport_release_buffer(pvd_destroy_request);   
            }
        }
    }

    if (path_state == FBE_PATH_STATE_GONE || path_state == FBE_PATH_STATE_INVALID)
    {
        fbe_u32_t port_num, encl_num, slot_num;
        fbe_provision_drive_get_drive_location(provision_drive_p, &port_num, &encl_num, &slot_num);

        if (!b_is_active) {
            if(port_num != FBE_PORT_NUMBER_INVALID || encl_num != FBE_ENCLOSURE_NUMBER_INVALID || slot_num != FBE_SLOT_NUMBER_INVALID) {
                fbe_provision_drive_set_drive_location(provision_drive_p, FBE_PORT_NUMBER_INVALID, FBE_ENCLOSURE_NUMBER_INVALID, FBE_SLOT_NUMBER_INVALID);
            }
        } else {
            if(port_num == FBE_PORT_NUMBER_INVALID || encl_num == FBE_ENCLOSURE_NUMBER_INVALID || slot_num == FBE_SLOT_NUMBER_INVALID) {
                /* we set location info for peer */
                fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)provision_drive_p, (void **)&provision_drive_nonpaged_metadata_p);
                fbe_provision_drive_set_drive_location(provision_drive_p,
                                                       provision_drive_nonpaged_metadata_p->nonpaged_drive_info.port_number,
                                                       provision_drive_nonpaged_metadata_p->nonpaged_drive_info.enclosure_number,
                                                       provision_drive_nonpaged_metadata_p->nonpaged_drive_info.slot_number);
            }
        }
    }
    fbe_provision_drive_check_hook(provision_drive_p, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_FAIL,
                                   FBE_PROVISION_DRIVE_SUBSTATE_DS_HEALTH_BROKEN_MD_UPDATED, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_provision_drive_downstream_health_broken_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_downstream_health_broken_cond_process_slf()
 ******************************************************************************
 * @brief
 *  This function is used to process single loop failure when the PVD is in 
 *  health_broken condition.  
 * 
 * @param provision_drive_p   - Pointer to the provision drive.
 * @param path_state          - path state.
 *
 * @return fbe_status_t
 *
 * @author
 *  06/11/2013  Lili Chen - Created. 
 * 
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_downstream_health_broken_cond_process_slf(fbe_provision_drive_t * provision_drive_p, fbe_path_state_t path_state)
{
    /* Set/clear SLF clustered flag */
    fbe_provision_drive_set_slf_flag_based_on_downstream_edge(provision_drive_p);
    if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF))
    {
        /* Clear block attribute */
        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(
                &((fbe_base_config_t *)provision_drive_p)->block_transport_server, 0,
                FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED);
        /* Clear SLF local state */
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF);
    }

    /* If path is enabled, we are done here. */
    if (path_state == FBE_PATH_STATE_ENABLED)
    {		
        return FBE_STATUS_OK;
    }

    /* Reschedule a short time. */
    if (fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF))
    {
        fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                                 (fbe_base_object_t *) provision_drive_p,
                                 (fbe_lifecycle_timer_msec_t) 500);

    }

    /* If we are active and in FAIL state, and peer clears SLF flag,
     * we need to send passive request. 
     */
    if (fbe_provision_drive_slf_need_to_send_passive_request(provision_drive_p))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s passive request sent\n", __FUNCTION__);
    }
    /* If we are passive and in FAIL state, and peer goes to READY state,
     * we need to go to activate state to join. 
     */
    else if (fbe_provision_drive_slf_passive_need_to_join(provision_drive_p) && 
             !fbe_provision_drive_metadata_need_reinitialize(provision_drive_p))
    {		
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s peer ready, start SLF. Go to ACT\n", __FUNCTION__);
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                           (fbe_base_object_t *)provision_drive_p, FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_downstream_health_broken_cond_process_slf()
 ******************************************************************************/

/*!****************************************************************************
 * pvd_fw_download_reject_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for sending usurper commands
 *  to PDO for fw download rejection.  
 * 
 * @param packet_p   - Pointer to the packet.
 * @param context    - context, which is a pointer to the base object.
 *
 * @return fbe_status_t
 *
 * @author
 *  02/05/2013  Wayne Garrett - Created. 
 * 
 ******************************************************************************/
static fbe_status_t 
pvd_fw_download_reject_completion(fbe_packet_t * packet_p,
                                  fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = (fbe_provision_drive_t*)context;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        pkt_status = FBE_STATUS_GENERIC_FAILURE;

    /* Get the packet status. */
    pkt_status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    if (pkt_status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed. pkt_status:%d control_status:%d\n", __FUNCTION__, pkt_status, control_status);    
    }    

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_provision_drive_downstream_health_disabled_cond_function()
 ******************************************************************************
 * @brief
 *  This is the derived condition function for virtual drive object where it 
 *  will hold the object in activate state until it finds downstream health 
 *  is optimal.
 *
 * @param object_p                  - The ptr to the base object.
 * @param packet_p                  - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is be based on parent
 *                                  function call.
 *
 * @author
 *  10/16/2009 - Created. Dhaval Patel.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_downstream_health_disabled_cond_function(fbe_base_object_t * object_p,
                                                             fbe_packet_t * packet_p)
{
    fbe_provision_drive_t *                     provision_drive_p = (fbe_provision_drive_t*)object_p;
    fbe_base_config_downstream_health_state_t   downstream_health_state;
    fbe_lifecycle_status_t                      lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;
    fbe_provision_drive_local_state_t           local_state = 0;
    fbe_lifecycle_state_t                       peer_state= FBE_LIFECYCLE_STATE_INVALID;  
    fbe_object_id_t                             object_id;
    fbe_scheduler_hook_status_t                 hook_status;
    fbe_lifecycle_status_t                      key_status = FBE_LIFECYCLE_STATUS_DONE;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    fbe_base_object_get_object_id(object_p, &object_id);

    fbe_provision_drive_check_hook(provision_drive_p,  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                   FBE_PROVISION_DRIVE_SUBSTATE_DS_HEALTH_DISABLED, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) { return FBE_LIFECYCLE_STATUS_DONE; }
    
    fbe_provision_drive_lock(provision_drive_p);

    fbe_provision_drive_get_local_state(provision_drive_p, &local_state);

    /* If drive fw download was requested and we went to ACTIVATE then reject the request.
    */
    if (FBE_PVD_LOCAL_STATE_DOWNLOAD_MASK & local_state)
    {
        fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "PVD downstream_health_disabled_cond_function, fwdl_reject object_id:0x%x local_state:0x%llx\n", object_id, local_state);

        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_MASK);
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_MASK);

        fbe_provision_drive_unlock(provision_drive_p);

        return fbe_provision_drive_send_download_command_to_pdo(provision_drive_p, packet_p,
                                                                FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_REJECT,
                                                                pvd_fw_download_reject_completion);        
    }

    fbe_provision_drive_unlock(provision_drive_p);

    /* If we are doing a Health Check then process the next state.  Only CONTINUE to next condition check
       after HC state machines is finished.
    */
    if ((local_state & FBE_PVD_LOCAL_STATE_HEALTH_CHECK_MASK) ||
        (local_state & FBE_PVD_LOCAL_STATE_HEALTH_CHECK_ABORT_REQ))
    {
        lifecycle_status = fbe_provision_drive_initiate_health_check(object_p, packet_p);

        if (lifecycle_status != FBE_LIFECYCLE_STATUS_CONTINUE)
        {
            return lifecycle_status;
        }
    }

    /* Set/clear SLF clustered flag */
    fbe_provision_drive_set_slf_flag_based_on_downstream_edge(provision_drive_p);
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t *)provision_drive_p, &peer_state);

    /* Check downstream health 
    */
    lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;

    downstream_health_state = fbe_provision_drive_verify_downstream_health(provision_drive_p);

    /* Check key state first. */
    {
        key_status = fbe_provision_drive_monitor_unregister_keys_if_needed(provision_drive_p, packet_p);
        if (key_status != FBE_LIFECYCLE_STATUS_CONTINUE)
        {
            return key_status;
        }
	
        key_status = fbe_provision_drive_check_key_info(provision_drive_p, packet_p);
        if (key_status != FBE_LIFECYCLE_STATUS_CONTINUE)
        {
            return key_status;
        }
    }

    switch (downstream_health_state)
    {
        case FBE_DOWNSTREAM_HEALTH_OPTIMAL:
            /* Clear the condition only if we are in good state. */
            fbe_lifecycle_clear_current_cond((fbe_base_object_t *)provision_drive_p);
            break;
        case FBE_DOWNSTREAM_HEALTH_BROKEN:
            if (fbe_provision_drive_is_slf_enabled() &&
                fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF) &&
                fbe_base_config_is_peer_present((fbe_base_config_t *)provision_drive_p) &&
                ((peer_state == FBE_LIFECYCLE_STATE_READY) || (peer_state == FBE_LIFECYCLE_STATE_ACTIVATE)))
            {
                /* We should let SLF to decide. */
                fbe_lifecycle_clear_current_cond((fbe_base_object_t *)provision_drive_p);
                break;
            }
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "*** %s *** Found broken in disabled ***\n", __FUNCTION__);
            /* set downstream health broken condition.
             * There are races during the setting of conditions in event context. 
             * Thus we could get here and have a broken path state. 
             */
            fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                   (fbe_base_object_t*)provision_drive_p,
                                   FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
            break;

        default:
            /* Return from the lifecycle condition.
             */
            lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;
            break;
    }

    return lifecycle_status;
}
/******************************************************************************
 * end fbe_provision_drive_downstream_health_disabled_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_downstream_health_not_optimal_cond_function()
 ******************************************************************************
 * @brief
 *  This is the derived condition function for provision drive object where it 
 *  will hold the object in specialize state until it finds downstream health 
 *  is optimal.
 * 
 *  We need this in order to hold the object until its path is in the
 *  correct state to proceed with the rest of specialize.
 * 
 *  We can't transition to ds health broken or ds health disabled when we are
 *  in specialize since the object has not initialized far enough.
 *
 * @param object_p                  - The ptr to the base object.
 * @param packet_p                  - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is be based on parent
 *                                  function call.
 *
 * @author
 *  5/20/2011 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_downstream_health_not_optimal_cond_function(fbe_base_object_t * object_p,
                                                                fbe_packet_t * packet_p)
{
    fbe_provision_drive_t *                     provision_drive_p = (fbe_provision_drive_t*)object_p;
    fbe_base_config_downstream_health_state_t   downstream_health_state;
    fbe_lifecycle_status_t                      lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t                                  should_removed = FBE_FALSE;
    fbe_system_time_t                           time_stamp;
    fbe_bool_t                                  b_is_active;
    fbe_bool_t              is_metadata_initialized = FBE_FALSE;
    fbe_base_config_nonpaged_metadata_state_t   np_state = FBE_NONPAGED_METADATA_INVALID;
    fbe_block_edge_t *                          edge_p = NULL;
    fbe_path_state_t                            path_state;
    fbe_object_id_t                             object_id;

    fbe_base_object_get_object_id(object_p, &object_id);

    fbe_zero_memory(&time_stamp,sizeof(fbe_system_time_t));
    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    /* Set/clear SLF clustered flag */
    fbe_provision_drive_set_slf_flag_based_on_downstream_edge(provision_drive_p);

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);
    fbe_base_config_metadata_nonpaged_is_initialized((fbe_base_config_t*)provision_drive_p, &is_metadata_initialized);
    status = fbe_base_config_get_nonpaged_metadata_state((fbe_base_config_t*)provision_drive_p,&np_state);
    downstream_health_state = fbe_provision_drive_verify_downstream_health(provision_drive_p);
    switch (downstream_health_state)
    {
        case FBE_DOWNSTREAM_HEALTH_OPTIMAL:
            /** We clear the PVD's remove_timestamp here before. But we may miss the SLF case.
             *  Now the remove_timestamp will be clear at the passive_join_request condition 
             *  in activate rotary.
             *  Here is the case: 
             *  - PVD on peer is active but failed state. 
             *  - Current SP reboot. Let's suppost the PVD on this SP has the good state.
             *  - PVD on this SP is passive at current point. It will become active later 
             *  - PVD on peer SP is failed.
             *
             *  In the above case, if we clear the remove_timestamp, we may miss the change to clear this
             *  PVD's remove_timestamp
             *
             *  Do Nothing here. Just clear the condition
             */

                /* Clear the condition only if we are in good state. */
                fbe_lifecycle_clear_current_cond((fbe_base_object_t *)provision_drive_p);
                break;
        default:
            /* Check the path state of the edge */
            fbe_base_config_get_block_edge((fbe_base_config_t *) provision_drive_p, &edge_p, 0);
            fbe_block_transport_get_path_state(edge_p, &path_state);
            if (path_state == FBE_PATH_STATE_GONE)
            {
                fbe_base_config_detach_edge((fbe_base_config_t *) provision_drive_p, packet_p, edge_p);
            }
            /* Return from the lifecycle condition.
             */
            lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;

            if (fbe_provision_drive_slf_need_to_send_passive_request(provision_drive_p))
            {
                fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s passive request sent\n", __FUNCTION__);
            }
            else if (fbe_provision_drive_slf_passive_need_to_join(provision_drive_p))
            {
                fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s passive join slf\n", __FUNCTION__);
                fbe_lifecycle_clear_current_cond((fbe_base_object_t *)provision_drive_p);
                return lifecycle_status;
            }

            /* If downstream health is not optimal, we set the start time*/
            if (fbe_provision_drive_get_ds_health_not_optimal_start_time(provision_drive_p) == 0)
            {
                fbe_provision_drive_set_ds_health_not_optimal_start_time(provision_drive_p, fbe_get_time());
            }

            /* check whether we should destroy this pvd or not */
            if(b_is_active 
               && (status == FBE_STATUS_OK && np_state != FBE_NONPAGED_METADATA_INVALID)
               && !(fbe_private_space_layout_object_id_is_system_pvd(object_id))){
                /* If nonpaged metadata has been initialized, read the time_stamp,
                 * It time_stamp is zero, set the new time_stamp to non-paged metadata */
                if (is_metadata_initialized) {
                    status = fbe_provision_drive_metadata_get_time_stamp(provision_drive_p,&time_stamp);
                    if(status != FBE_STATUS_OK) {
                        fbe_provision_drive_utils_trace( provision_drive_p,
                                                         FBE_TRACE_LEVEL_DEBUG_HIGH,
                                                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                         FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                                         "%s failed to get time stamp from PVD nonpaged metadata.\n", __FUNCTION__);
                    }else{
                        if (fbe_provision_drive_system_time_is_zero(&time_stamp) ) {
                            status = fbe_provision_drive_metadata_set_time_stamp(provision_drive_p,packet_p,FBE_FALSE);
                            if(status != FBE_STATUS_OK) {
                                fbe_provision_drive_utils_trace( provision_drive_p,
                                                                 FBE_TRACE_LEVEL_DEBUG_HIGH,
                                                                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                                 FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                                                 "%s failed to set time stamp in PVD nonpaged metadata.\n", __FUNCTION__);

                            }
                            fbe_provision_drive_utils_trace( provision_drive_p,
                                                             FBE_TRACE_LEVEL_INFO,
                                                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                             FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                                             "%s edge not optimal. set remove_timestamp for PVD.\n", __FUNCTION__);
                            return FBE_LIFECYCLE_STATUS_PENDING;
                        }
                    }
                }

                /* When path state is not enable, there maybe a delay for the edge connection.
                 * So wait 120 seconds for that edge be ready  
                 *
                 * There is a case we need to resolve. If we pull a drive, the PVD will become 
                 * Failed. Then we shut down the SPs. We return back the drive during system is down.
                 * After that, reboot the system a long time later. 
                 * The following logic may judge if removing the PVD or not before the drive is 
                 * connected to the LDO. So we let it wait for 120 seconds before removing the PVD.
                 * If we can connect the drive, don't remove the PVD again.
                 ********************************************************************************/
                if (fbe_provision_drive_wait_for_downstream_health_optimal_timeout(provision_drive_p))
                {
                    status = fbe_provision_drive_should_permanently_removed(provision_drive_p,&should_removed);
                    if(status != FBE_STATUS_OK) {
                        fbe_provision_drive_utils_trace(provision_drive_p,
                                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                                        "%s failed to check PVD is permanently removed. \n", __FUNCTION__);
                    }
                    if(should_removed) {
                        /* this will crank the object into disable_cond_function, 
                           then go into broken_cond_fun */
                        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                              FBE_TRACE_LEVEL_INFO,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "%s next to remove this PVD \n", __FUNCTION__);                
                        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)provision_drive_p);
                        /* downstream_health_broken condition won't transfer to fail rotoary */
                        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                               (fbe_base_object_t*)provision_drive_p,
                                               FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
                        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                               (fbe_base_object_t *)provision_drive_p,
                                               FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

                    }
                }
                /* Jump out the switch*/
                break;
            }else{
                /* If this is passive side, or it is a system PVD, don't relaim it*/
                break;
            }
    }
    return lifecycle_status;
}
/******************************************************************************
 * end fbe_provision_drive_downstream_health_not_optimal_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_set_remove_timestamp_completion()
 ******************************************************************************
 * @brief
 *  This function is used as set time stamp completion function 
 *  
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 * 01/16/2012 - zhangy
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_set_remove_timestamp_completion(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* Clear the condition only if we are in good state. */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)provision_drive_p);
    
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s set remove timestamp failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_set_remove_timestamp_completion 
 ******************************************************************************/
/*!**************************************************************
 * fbe_provision_drive_clear_peer_state()
 ****************************************************************
 * @brief
 *   Clear out the peer state as part of transitioning
 *   from one state to another.
 *  
 * @param provision_drive_p - The raid group object.
 * 
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING
 *                                    - if we are not done yet.
 *                                  FBE_LIFECYCLE_STATUS_DONE
 *                                    - if we are done.
 *
 * @author
 *  10/06/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_lifecycle_status_t fbe_provision_drive_clear_peer_state(fbe_provision_drive_t *provision_drive_p)
{
    fbe_lifecycle_state_t lifecycle_state;
    fbe_status_t status;
    fbe_provision_drive_clustered_flags_t flags = 0;
    /* When we change states it is important to clear out our masks for things
     * related to the peer. 
     * For the activate -> ready transition we do not want to clear the flags. 
     * At the point of activate we have initted these peer to peer flags and 
     * have joined the peer. 
     */
    fbe_provision_drive_lock(provision_drive_p);
    fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_ALL_MASKS);
    fbe_provision_drive_set_ds_disabled_start_time(provision_drive_p, 0);
    fbe_provision_drive_get_clustered_flags(provision_drive_p, &flags);

    status = fbe_lifecycle_get_state(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*)provision_drive_p, &lifecycle_state);
    if ( (lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_READY) &&         
         (fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t*)provision_drive_p, 
                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_ALL_MASKS) ||
          (flags & FBE_PROVISION_DRIVE_CLUSTERED_FLAG_ALL_MASKS) != 0 ))
    {
        fbe_base_config_clear_clustered_flag((fbe_base_config_t*)provision_drive_p, 
                                             FBE_BASE_CONFIG_CLUSTERED_FLAG_ALL_MASKS);                                
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, 
                                                 FBE_PROVISION_DRIVE_CLUSTERED_FLAG_ALL_MASKS);           
        
        if (lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_FAIL)
        {
            /* Health Check should only be cleared when transitioning to fail.  Intentionally not in ALL_MASKS */    
            fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_MASK);
            fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_MASK);
        }

        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "pvd clearing clustered flags obj_state: 0x%x l_bc_flags: 0x%x peer: 0x%x local state: 0x%x\n", 
                              provision_drive_p->base_config.base_object.lifecycle.state,
                              (unsigned int)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned int)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned int)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    fbe_provision_drive_unlock(provision_drive_p);
    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************
 * end fbe_provision_drive_clear_peer_state()
 **************************************/

/*!****************************************************************************
 * fbe_provision_drive_pending_func()
 ******************************************************************************
 * @brief
 *  This gets called when we are going to transition from one lifecycle to 
 *  another. The purpose here is to drain all I/O before we transition state.
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
 *  01/14/2011 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_pending_func(fbe_base_object_t *base_object_p, fbe_packet_t * packet)
{
    fbe_lifecycle_status_t  lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;
    fbe_base_config_t *     base_config_p = NULL;
    fbe_provision_drive_t * provision_drive_p = NULL;
    fbe_status_t            status;
    fbe_trace_level_t       trace_level = FBE_TRACE_LEVEL_DEBUG_HIGH;
    fbe_lifecycle_state_t   lifecycle_state;
    fbe_u32_t outstanding_io = 0;
    fbe_bool_t              b_is_active;
    fbe_block_path_attr_flags_t attribute_exception = FBE_BLOCK_PATH_ATTR_FLAGS_NONE;
	fbe_object_id_t								object_id;
	fbe_bool_t									is_system_drive;

    base_config_p = (fbe_base_config_t *)base_object_p;
    provision_drive_p = (fbe_provision_drive_t*) base_object_p;

    lifecycle_status = fbe_provision_drive_clear_peer_state(provision_drive_p);
    if (lifecycle_status != FBE_LIFECYCLE_STATUS_DONE)
    {
        return lifecycle_status;
    }

    /* Clear the event outstanding flag */
    fbe_provision_drive_clear_event_outstanding_flag(provision_drive_p);

    /* PVD needs to make sure that if quiesce/unquiesce condition set in ready state then needs to clear it before
     *  transition to not ready state. When PVD again come to the ready state, it does not have way to clear these 
     *  condition so needs to cleanup it here explicitly.
     */

    /* clear the quiesce conditon if it is set. This function internally have check if it is set or not */
    fbe_lifecycle_force_clear_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                           FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE);
    /* clear the unquiesce conditon if it is set. This function internally have check if it is set or not */
    fbe_lifecycle_force_clear_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                           FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE);

    /* If there are I/Os outstanding raise the trace level to info
     */    
    fbe_block_transport_server_get_outstanding_io_count(&base_config_p->block_transport_server, &outstanding_io);

    if (outstanding_io > 0)
    {
        trace_level = FBE_TRACE_LEVEL_INFO;
    }

    status = fbe_lifecycle_get_state(&fbe_base_config_lifecycle_const, base_object_p, &lifecycle_state);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s fbe_lifecycle_get_state failed, status = %X", __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                          trace_level,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry state: %d outstanding I/O: %d attributes: 0x%llx\n", 
                          __FUNCTION__, lifecycle_state,
                          outstanding_io, 
                          (unsigned long long)base_config_p->block_transport_server.attributes);
    
    /* Clear the hold bit in case it was set previously.
     * This could occur if we were doing a quiesce and the raid group goes shutdown.
     */
    fbe_block_transport_server_resume(&base_config_p->block_transport_server);

    /* We need to completely drain our block transport server.
     * If we have anything on our block transport server queue from MDS, 
     * then we need to drain it with a method that unconditionally drains the queue.
     * block_transport_server_pending below only drains the queues after the outstanding I/O count is zero. 
     */
    fbe_block_transport_server_drain_all_queues(&base_config_p->block_transport_server,
                                                &fbe_base_config_lifecycle_const,
                                                (fbe_base_object_t *) base_config_p);

    /* Abort all queued I/Os.
     * Don't do it if we're pending ready since there are no I/Os in flight. 
     * Also the block trans server changes the path state to enabled immediately after we drain, so 
     * to prevent aborted metadata I/Os don't set the bit now, clear it. 
     */
    if (lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_READY)
    {
        fbe_metadata_element_abort_io(&provision_drive_p->base_config.metadata_element);
    }
    else
    {
        fbe_metadata_element_clear_abort(&base_config_p->metadata_element);
        /* we are moving into ready state, after peer sets JOIN_SYNCING */
        b_is_active = fbe_base_config_is_active(base_config_p);
        if (!b_is_active)
        {
            fbe_base_config_set_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING); 
        }
    }

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    is_system_drive = fbe_database_is_object_system_pvd(object_id);

    if (is_system_drive) 
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "provision_drive: Sys Drive request key \n");
        attribute_exception = FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED;
    }

    /* Call the below function to fail out any I/O that might have received an 
     * error and needs to get restarted. 
     */
    status = fbe_provision_drive_handle_shutdown(provision_drive_p);

    if (status == FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                              trace_level,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s draining block transport server attributes: 0x%llx\n", __FUNCTION__, 
                              (unsigned long long)base_config_p->block_transport_server.attributes);
        /* We simply call to the block transport server to handle pending.
         */
        lifecycle_status = fbe_block_transport_server_pending_with_exception(&base_config_p->block_transport_server,
                                                                             &fbe_base_config_lifecycle_const,
                                                                             (fbe_base_object_t *) base_config_p,
                                                                             attribute_exception);
    }
    else
    {
        /* We are not done yet.  Wait for I/Os to finish.
         * We still drain the block transport server so any waiting requests in the bts can get freed 
         * before we drain all requests to the raid library. 
         */
        lifecycle_status = fbe_block_transport_server_pending_with_exception(&base_config_p->block_transport_server,
                                                                             &fbe_base_config_lifecycle_const,
                                                                             (fbe_base_object_t *) base_config_p,
                                                                             attribute_exception);
        lifecycle_status = FBE_LIFECYCLE_STATUS_PENDING;
    }

    if ((lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_DESTROY) &&
        !fbe_topology_is_reference_count_zero(base_config_p->base_object.object_id))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD pending saw non-zero reference count.\n");
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    if (lifecycle_status == FBE_LIFECYCLE_STATUS_DONE)
    {
        /* Clear the cache
         */
        fbe_base_config_metadata_paged_clear_cache(base_config_p);

        fbe_metadata_element_clear_abort(&base_config_p->metadata_element);
    }
    fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                          trace_level,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                          "%s exit status: %d outstanding I/O: %d attributes: 0x%llx\n", 
                          __FUNCTION__, lifecycle_status,
                          outstanding_io, 
                          (unsigned long long)base_config_p->block_transport_server.attributes);

    return lifecycle_status;
}
/******************************************************************************
 * end fbe_provision_drive_pending_func()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_memory_init_cond_function()
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
fbe_provision_drive_metadata_memory_init_cond_function(fbe_base_object_t * object_p,
                                                       fbe_packet_t * packet_p)
{
    fbe_provision_drive_t * provision_drive_p = NULL;

    provision_drive_p = (fbe_provision_drive_t*)object_p;
    
    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING,
                                    "%s entry\n", __FUNCTION__);

#if 0
    if (!fbe_provision_drive_metadata_need_reinitialize(provision_drive_p) &&
        fbe_provision_drive_is_md_memory_initialized(provision_drive_p))
    {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        fbe_lifecycle_force_clear_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_NONPAGED_METADATA_INIT);
        fbe_lifecycle_force_clear_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_ELEMENT_INIT);
        fbe_lifecycle_force_clear_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_STRIPE_LOCK_START);

        return FBE_LIFECYCLE_STATUS_DONE;
    }
#endif

    /* Set the completion function before we initialize the metadata memory. */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_metadata_memory_init_cond_completion, provision_drive_p);

    /* Initialize metadata memory for the provision drive object. */
    fbe_base_config_metadata_init_memory((fbe_base_config_t *) provision_drive_p, 
                                         packet_p,
                                         &provision_drive_p->provision_drive_metadata_memory,
                                         &provision_drive_p->provision_drive_metadata_memory_peer, 
                                         sizeof(fbe_provision_drive_metadata_memory_t));


    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_memory_init_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_memory_init_cond_completion()
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
fbe_provision_drive_metadata_memory_init_cond_completion(fbe_packet_t * packet_p,
                                                         fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* Get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* Clear the metadata memory init condition if status is ok. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {    
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        if (status != FBE_STATUS_OK){
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }

        /* Init drive location. */
        fbe_provision_drive_set_drive_location(provision_drive_p, FBE_PORT_NUMBER_INVALID, FBE_ENCLOSURE_NUMBER_INVALID, FBE_SLOT_NUMBER_INVALID);
    }

    /* If it returns busy status then do not clear the condition and try to 
     * initialize the metadata memory in next monitor cycle.
     */
    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_BUSY)
    {
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING,
                                        "%s can't clear current condition, payload status is FBE_PAYLOAD_CONTROL_STATUS_BUSY\n",
                                        __FUNCTION__);
        
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_memory_init_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_nonpaged_metadata_init_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the provision drive object's nonpaged
 *  metadata memory.
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
fbe_provision_drive_nonpaged_metadata_init_cond_function(fbe_base_object_t * object_p,
                                                         fbe_packet_t * packet_p)
{
    fbe_provision_drive_t *         provision_drive_p = NULL;
    fbe_status_t                    status;
    fbe_scheduler_hook_status_t	hook_status;

    provision_drive_p = (fbe_provision_drive_t *)object_p;
    
    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING,
                                    "%s entry\n", __FUNCTION__);

    fbe_provision_drive_check_hook(provision_drive_p,
                                                                        SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_NON_PAGED_INIT,
                                 FBE_PROVISION_DRIVE_SUBSTATE_NONPAGED_INIT, 
                                0,
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* set the completion function for the nonpaged metadata init condition. */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_nonpaged_metadata_init_cond_completion, provision_drive_p);

    /* initialize signature for the provision drive object. */
    status = fbe_provision_drive_nonpaged_metadata_init(provision_drive_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_provision_drive_nonpaged_metadata_init_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_nonpaged_metadata_init_cond_completion()
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
fbe_provision_drive_nonpaged_metadata_init_cond_completion(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t                          is_metadata_initialized;
    fbe_provision_drive_nonpaged_metadata_t *   provision_drive_nonpaged_metadata_p = NULL;
    
    provision_drive_p = (fbe_provision_drive_t*)context;

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
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s nonpaged metadata init condition failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }

    /* set drive location if nonpaged metadata is already initialized. */
    fbe_base_config_metadata_is_initialized((fbe_base_config_t *)provision_drive_p, &is_metadata_initialized);
    if (is_metadata_initialized)
    {
        fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)provision_drive_p, (void **)&provision_drive_nonpaged_metadata_p);
        fbe_provision_drive_set_drive_location(provision_drive_p, 
                                               provision_drive_nonpaged_metadata_p->nonpaged_drive_info.port_number,
                                               provision_drive_nonpaged_metadata_p->nonpaged_drive_info.enclosure_number,
                                               provision_drive_nonpaged_metadata_p->nonpaged_drive_info.slot_number);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_nonpaged_metadata_init_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_element_init_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the metadata element for the provision
 *  drive object.
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
fbe_provision_drive_metadata_element_init_cond_function(fbe_base_object_t * object_p,
                                                        fbe_packet_t * packet_p)
{
    fbe_provision_drive_t * provision_drive_p = NULL;

    provision_drive_p = (fbe_provision_drive_t*)object_p;
    
    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING,
                                    "%s entry\n", __FUNCTION__);

    /* Set the completion function for the metadata element initialization. */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_metadata_element_init_cond_completion, provision_drive_p);

    /* Initialize the metadata element for the provision drive object. */
    fbe_provision_drive_metadata_initialize_metadata_element(provision_drive_p, packet_p);

    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_element_init_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_element_init_cond_completion()
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
fbe_provision_drive_metadata_element_init_cond_completion(fbe_packet_t * packet_p,
                                                          fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t * provision_drive_p = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* If it returns good status then clear the metadata element init condition. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {    
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
        fbe_topology_clear_gate(provision_drive_p->base_config.base_object.object_id);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s metadata element init condition failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_element_init_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_stripe_lock_start_cond_function()
 ******************************************************************************
 * @brief
 *  Start locking with the peer.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_DONE when metadata
 *                                    verification done.
 *
 * @author
 *  10/26/2012 - Created. Rob Foley
 ******************************************************************************/

static fbe_lifecycle_status_t 
fbe_provision_drive_stripe_lock_start_cond_function(fbe_base_object_t * object,
                                                    fbe_packet_t * packet_p)
{
    fbe_provision_drive_t * provision_drive_p = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_stripe_lock_operation_t * stripe_lock_operation = NULL;

    provision_drive_p = (fbe_provision_drive_t*)object;

    if(!fbe_base_config_stripe_lock_is_started((fbe_base_config_t *) provision_drive_p)){
        payload = fbe_transport_get_payload_ex(packet_p);
        stripe_lock_operation = fbe_payload_ex_allocate_stripe_lock_operation(payload);
        fbe_payload_stripe_lock_build_start(stripe_lock_operation, &((fbe_base_config_t *)provision_drive_p)->metadata_element);
        
        /* We may choose to allocate the memory here, but for now ... */
        ((fbe_base_config_t *)object)->metadata_element.stripe_lock_blob = &provision_drive_p->stripe_lock_blob;
        provision_drive_p->stripe_lock_blob.size = METADATA_STRIPE_LOCK_SLOTS;

        fbe_payload_ex_increment_stripe_lock_operation_level(payload);
        fbe_transport_set_completion_function(packet_p, fbe_provision_drive_stripe_lock_start_completion, provision_drive_p);
        fbe_stripe_lock_operation_entry(packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    /* clear the metadata verify condition if non paged metadata is good. */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);

    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************
 * end fbe_provision_drive_stripe_lock_start_cond_function()
 **************************************/
/*!****************************************************************************
 * fbe_provision_drive_metadata_verify_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to verify the provision drive's metadata initialization.
 *  It verify that if non paged metadata is not initialized than it set multiple 
 *  conditions to initialize provision drive's metadata.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_DONE when metadata
 *                                    verification done.
 *
 * @author
 *  3/23/2010 - Created. Dhaval Patel
 *  09/14/2011 - Modified Amit Dhaduk
 *  05/21/2012 - Modified by Jingzheng Zhang for metadata versioning check
 *  5/8/2012   - Modified Zhang Yang. Add NP state check
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_metadata_verify_cond_function(fbe_base_object_t * object,
                                                   fbe_packet_t * packet_p)
{
    fbe_provision_drive_t * provision_drive_p = NULL;
    fbe_bool_t is_metadata_initialized;
    fbe_bool_t is_active;
    fbe_u32_t  disk_version_size = 0;
    fbe_u32_t  memory_version_size = 0;    
    fbe_base_config_nonpaged_metadata_t    *base_config_nonpaged_metadata_p = NULL;
    fbe_scheduler_hook_status_t	hook_status;
    fbe_bool_t is_magic_num_valid;	
    //fbe_lifecycle_status_t lifecycle_status;
    provision_drive_p = (fbe_provision_drive_t*)object;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);
    
    /*check the NP magic number .
        If it is INVALID, the NP is not load correctly
        the object will stuck in this condition function until 
        its NP is set correctly by caller*/
    fbe_base_config_metadata_is_nonpaged_state_valid((fbe_base_config_t *) object, &is_magic_num_valid);
    if(is_magic_num_valid == FBE_FALSE)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    is_active = fbe_base_config_is_active((fbe_base_config_t *)provision_drive_p);
    fbe_base_config_metadata_is_initialized((fbe_base_config_t *)provision_drive_p, &is_metadata_initialized);

#if 0 /* We should avoid getting NP locks when the metadata is still in initialize */
    lifecycle_status = fbe_provision_drive_monitor_unregister_keys_if_needed(provision_drive_p, packet_p);
    if (lifecycle_status != FBE_LIFECYCLE_STATUS_CONTINUE)
    {
        return lifecycle_status;
    }
	
    lifecycle_status = fbe_provision_drive_check_key_info(provision_drive_p, packet_p);
    if (lifecycle_status != FBE_LIFECYCLE_STATUS_CONTINUE)
    {
        return lifecycle_status;
    }
#endif

    /*version check.  the non-paged metadata on disk may have different version with current
      non-paged metadata structure in memory. compare the size and set default value for the
      new version in memory*/
    if (is_metadata_initialized) {
        fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)provision_drive_p, (void **)&base_config_nonpaged_metadata_p);
        fbe_base_config_get_nonpaged_metadata_size((fbe_base_config_t *)provision_drive_p, &memory_version_size);
        fbe_base_config_nonpaged_metadata_get_version_size(base_config_nonpaged_metadata_p, &disk_version_size);
        if (disk_version_size != memory_version_size) {
            /*new software is operating old version data during upgrade, set default value for the new version*/
            /*currently these region already zerod, the new version developer should set correct value here*/
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "pvd nonpaged verify - on-disk size: %d in-memory size: %d update new values here.\n",
                              disk_version_size, memory_version_size);            
        }

        /* Set flag nonpaged metadata is initialized */
        fbe_base_config_set_clustered_flag((fbe_base_config_t *)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_NONPAGED_INITIALIZED);
    }

    if (!is_active){ /* This will take care of passive side */
        if(is_metadata_initialized == FBE_TRUE){
            fbe_base_config_clear_clustered_flag((fbe_base_config_t *) provision_drive_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST);
            
            fbe_provision_drive_check_hook(provision_drive_p,
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT,
                                           FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_NOT_FIRST_CREATE, 
                                           0,
                                           &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE) {
                return FBE_LIFECYCLE_STATUS_DONE;
            }
            
            fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);            
        } else {
        
            fbe_provision_drive_check_hook(provision_drive_p,
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT,
                                           FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_FIRST_CREATE, 
                                           0,
                                           &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE) {
                return FBE_LIFECYCLE_STATUS_DONE;
            }

        
            /* Ask again for the nonpaged metadata update */
            if(!fbe_base_config_is_clustered_flag_set((fbe_base_config_t *) provision_drive_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST)){
                fbe_base_config_set_clustered_flag((fbe_base_config_t *) provision_drive_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST);
            }
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    if (!is_metadata_initialized) { /* We active and newly created */
        fbe_base_config_nonpaged_metadata_t *md_p = NULL;
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "%s: We active and newly created\n", __FUNCTION__);

        fbe_provision_drive_check_hook(provision_drive_p,
                                       SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT,
                                       FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_FIRST_CREATE, 
                                       0,
                                       &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) {
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)provision_drive_p, (void **)&md_p);
        
        fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "generation #: 0x%llx/0x%llx md_state: 0x%x object_id: 0x%x\n", 
                                        md_p->generation_number, 
                                         provision_drive_p->base_config.generation_number,
                                        md_p->nonpaged_metadata_state, md_p->object_id);
        /* Set all relevant conditions */
        /* We are initializing the object first time and so update the 
         * non paged metadata, paged metadata and nonpaged metadata signature in order.
         */

        /* set the clear DDMI region condition first. The DDMi region is compitable with Flare drive
        *   The aim of clearing that region is avoid the following issue:
        *   1. Insert a flare drive into MCR array.
        *   2. After the drive consumed by MCR, reinsert it to a Flare array.
        *   3. If the DDMI region is not cleared, the Flare will consider it's a clean drive
        *       It won't zero the disk and will read wrong data.
        */
        
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                (fbe_base_object_t *)provision_drive_p,
                                FBE_PROVISION_DRIVE_LIFECYCLE_COND_CLEAR_DDMI_REGION);

        /* set the nonpaged metadata init condition. */
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_NONPAGED_METADATA);

        /* set the zero consumed area condition. */
       fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                              (fbe_base_object_t *)provision_drive_p,
                              FBE_BASE_CONFIG_LIFECYCLE_COND_ZERO_CONSUMED_AREA);

        /* set the paged metadata init condition. */
       fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                              (fbe_base_object_t *)provision_drive_p,
                              FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_PAGED_METADATA);

        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_PERSIST_DEFAULT_NONPAGED_METADATA);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_config_clear_clustered_flag((fbe_base_config_t *) provision_drive_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST);

    /* make sure we send the nonpaged to peer. */
    if(fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t *) provision_drive_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST)){
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    fbe_provision_drive_check_hook(provision_drive_p,
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT,
                                   FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_NOT_FIRST_CREATE, 
                                   0,
                                   &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* clear the metadata verify condition if non paged metadata is good. */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);

    return FBE_LIFECYCLE_STATUS_DONE;

}
/******************************************************************************
 * end fbe_provision_drive_signature_verify_cond_function()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_write_default_paged_metadata_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to write the default paged metadata for the provision
 *  drive object.
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
fbe_provision_drive_write_default_paged_metadata_cond_function(fbe_base_object_t * object_p,
                                                               fbe_packet_t * packet_p)
{
    fbe_provision_drive_t *         provision_drive_p = NULL;
    fbe_status_t                    status;

    provision_drive_p = (fbe_provision_drive_t*)object_p;
    
    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING,
                                    "%s entry\n", __FUNCTION__);

    /* set the completion function for the signature init condition. */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_write_default_paged_metadata_cond_completion, provision_drive_p);

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_write_default_paged_metadata_set_np_flag, provision_drive_p);
    /* initialize signature for the provision drive object. */
    status = fbe_provision_drive_metadata_write_default_paged_metadata(provision_drive_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_provision_drive_paged_metadata_write_default_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_write_default_paged_metadata_cond_completion()
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
fbe_provision_drive_write_default_paged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                                 fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* clear paged metadata write default condition if status is good. */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "provision drive write default paged metadata: metadata condition failed, status:0x%X, control_status: 0x%X\n",
                              status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_write_default_paged_metadata_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_write_default_nonpaged_metadata_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to write the default nonpaged metadata for the 
 *  provision drive object during object initialization.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  7/11/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_write_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p,
                                                                  fbe_packet_t * packet_p)
{
    fbe_provision_drive_t *         provision_drive_p = NULL;
    fbe_status_t                    status;

    provision_drive_p = (fbe_provision_drive_t*)object_p;
    
    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING,
                                    "%s entry\n", __FUNCTION__);


    /* set the completion function for the paged metadata init condition. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_write_default_nonpaged_metadata_cond_completion,
                                          provision_drive_p);

    /* initialize signature for the provision drive object. */
    status = fbe_provision_drive_metadata_write_default_nonpaged_metadata(provision_drive_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_provision_drive_write_default_nonpaged_metadata_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_write_default_nonpaged_metadata_cond_completion()
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
 *  3/23/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_write_default_nonpaged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                                    fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    provision_drive_p = (fbe_provision_drive_t*)context;

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
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);

    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s write default nonpaged metadata condition failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_write_default_nonpaged_metadata_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_persist_default_nonpaged_metadata_cond_function()
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
static fbe_lifecycle_status_t fbe_provision_drive_persist_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p, 
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
 * end fbe_provision_drive_persist_default_nonpaged_metadata_cond_function()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_run_background_zeroing()
 ******************************************************************************
 * @brief
 *   This functions runs background zeroing operation. It does second level sanity 
 *   check and if the operation is not completed, it sends either permission request 
 *   to upstream edge or directly sends zeroing IO request to executor.
 *
 *  @param object_p                 - pointer to a base object 
 *  @param packet_p                 - pointer to a control packet_p from the scheduler
 *
 *  @return fbe_lifecycle_status_t  - FBE_LIFECYCLE_STATUS_RESCHEDULE on success/
 *                                    condition was cleared
 *                                  - FBE_LIFECYCLE_STATUS_PENDING when I/O is
 *                                    being issued 
 * @author
 *   01/08/2010 - Created. Amit Dhaduk.
 *
 ******************************************************************************/
fbe_lifecycle_status_t
fbe_provision_drive_run_background_zeroing(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{

    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_provision_drive_t *     provision_drive_p = NULL;    
    fbe_lba_t                   zero_checkpoint = FBE_LBA_INVALID;
    fbe_bool_t                  b_is_enabled = FBE_TRUE;
    fbe_bool_t                  b_can_zero_start = FBE_TRUE;
    fbe_bool_t                  b_is_zero_complete;
    fbe_bool_t                  b_is_consumed = FBE_TRUE;
    fbe_bool_t                  b_is_system_drive = FBE_FALSE;
    fbe_lba_t                   default_offset = FBE_LBA_INVALID;    
    fbe_scheduler_hook_status_t hook_status;
    fbe_object_id_t             object_id;
    fbe_lba_t                   next_consumed_lba = FBE_LBA_INVALID;

    /*  Cast the base object to a provision drive object. */     
    provision_drive_p = (fbe_provision_drive_t*) object_p;

    /* get pvd object id. */
    fbe_base_object_get_object_id((fbe_base_object_t *) provision_drive_p, &object_id);

    /* If background zeroing has been disabled we are done. */
    fbe_base_config_is_background_operation_enabled((fbe_base_config_t *)provision_drive_p ,
                                                    FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, 
                                                    &b_is_enabled);
    if (b_is_enabled == FBE_FALSE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Check the downstream status and load before starting. */
    fbe_provision_drive_zero_utils_can_zeroing_start(provision_drive_p, &b_can_zero_start);
    if(!b_can_zero_start) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Get the zero checkpoint. */
    status = fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive_p, &zero_checkpoint);
    if (status != FBE_STATUS_OK) {
        fbe_provision_drive_utils_trace( provision_drive_p,
                     FBE_TRACE_LEVEL_WARNING,
                     FBE_TRACE_MESSAGE_ID_INFO, 
                     FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                     "pvd_run_bgz: Failed to get checkpoint. Status: 0x%X\n", status);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If a debug hook is set, we need to execute the specified action. */
    fbe_provision_drive_check_hook(provision_drive_p,  
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO, 
                                   FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY, 
                                   zero_checkpoint, 
                                   &hook_status);

    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Determine if zeroing completed for the pvd object. */
    fbe_provision_drive_zero_utils_determine_if_zeroing_complete(provision_drive_p, zero_checkpoint, &b_is_zero_complete);
    if (b_is_zero_complete == FBE_TRUE) {
        /* If a debug hook is set, we need to execute the specified action. */
        fbe_provision_drive_check_hook(provision_drive_p,  
                                       SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO, 
                                       FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION, 
                                       0, 
                                       &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) {
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* set the condition to mark the zero checkpoint as logical end marker for the drive. */
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *) provision_drive_p,
                               FBE_PROVISION_DRIVE_LIFECYCLE_COND_SET_ZERO_CHECKPOINT_TO_END_MARKER);


        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Set the completion function for the disk zeroing condition. */
    /* It is too early */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_run_background_zeroing_completion,
                                          provision_drive_p);
    


    /* Check for system drive */
    b_is_system_drive = fbe_database_is_object_system_pvd(object_id);

    /* Find out whether next zeroing range is consumed by the upstream object.
     * If it is not consumed permission is automatically granted.  This is
     * only applicable for non-system drives
     */
    fbe_block_transport_server_is_lba_range_consumed(&((fbe_base_config_t *) provision_drive_p)->block_transport_server,
                                                     zero_checkpoint,
                                                     FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                     &b_is_consumed);

    if(b_is_consumed == FBE_TRUE) { /* We MUST to ask permition */
        fbe_provision_drive_zero_utils_ask_zero_permision(provision_drive_p,
                                                          zero_checkpoint,
                                                          FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                          packet_p);

        /* return pending status since we are waiting for the permission.*/
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* Get the default offset. */
    fbe_base_config_get_default_offset((fbe_base_config_t *) provision_drive_p, &default_offset);

    /* The chunk is NOT consumed */
    if(b_is_system_drive == FBE_FALSE){ /* Regular drive */
        /* If the current background zero checkpoint is below the default
         * (not clear how that occurred), log a warning and set the background
         * zeroing checkpoint to the default_offset.
         */ 
   
        if (zero_checkpoint < default_offset) {
            /* Trace an error, set the completion function (done above) and 
             * invoke method to update the zero checkpoint.
             */
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "BG_ZERO: zero checkpoint: 0x%llx below default_offset: 0x%llx unexpected. \n",
                                  (unsigned long long)zero_checkpoint, (unsigned long long)default_offset);

            /* Ignore the status. */
            fbe_provision_drive_metadata_set_bz_chkpt_without_persist(provision_drive_p, packet_p, default_offset);
            return FBE_LIFECYCLE_STATUS_PENDING;
        } else {
            fbe_scheduler_hook_status_t hook_status;
            fbe_provision_drive_check_hook(provision_drive_p,  
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO, 
                                           FBE_PROVISION_DRIVE_SUBSTATE_UNCONSUMED_ZERO_SEND, 0,  &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE)
            {
                fbe_transport_set_status( packet_p, FBE_STATUS_OK, 0 );
                fbe_transport_complete_packet( packet_p );
                return FBE_STATUS_OK;
            }

            /* log the start of background zeroing */
            if (!provision_drive_p->bg_zeroing_log_status)
            {
                fbe_provision_drive_event_log_disk_zero_start_or_complete(provision_drive_p, FBE_TRUE, packet_p);
            }

            /* The chunk is not consumed and we are above the default_offset, so let's issue the zero request. */
            status = fbe_provision_drive_send_monitor_packet(provision_drive_p,
                                                             packet_p,
                                                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_BACKGROUND_ZERO,
                                                             zero_checkpoint,
                                                             FBE_PROVISION_DRIVE_CHUNK_SIZE);

            return FBE_LIFECYCLE_STATUS_PENDING;
        }
    } /* end if there is no need to ask permission */
    else
    {
        /* log the start of background zeroing for system drives*/
        if (zero_checkpoint == 0)
        {
            fbe_provision_drive_event_log_disk_zero_start_or_complete(provision_drive_p, FBE_TRUE, packet_p);
        }

    }
    /* Handle unconsumed chunk in system object */
    /* Find the next consumed area */
    fbe_block_transport_server_find_next_consumed_lba(&((fbe_base_config_t *) provision_drive_p)->block_transport_server,
                                                        zero_checkpoint,
                                                        &next_consumed_lba);


    if(next_consumed_lba == FBE_LBA_INVALID){ /* Nothing is consumed */
        if(zero_checkpoint >= default_offset){ /* Issue zero command */
            /* The chunk is not consumed and we are above the default_offset, so let's issue the zero request. */
            status = fbe_provision_drive_send_monitor_packet(provision_drive_p,
                                                             packet_p,
                                                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_BACKGROUND_ZERO,
                                                             zero_checkpoint,
                                                             FBE_PROVISION_DRIVE_CHUNK_SIZE);

            return FBE_LIFECYCLE_STATUS_PENDING;
        } else { /* Advance checkpoint to default offset */
            fbe_provision_drive_metadata_set_bz_chkpt_without_persist(provision_drive_p, packet_p, default_offset);
            return FBE_LIFECYCLE_STATUS_PENDING;            
        }
    } else { /* Advance checkpoint to next consumed area */
        next_consumed_lba = (next_consumed_lba / FBE_PROVISION_DRIVE_CHUNK_SIZE) * FBE_PROVISION_DRIVE_CHUNK_SIZE;
            fbe_provision_drive_metadata_set_bz_chkpt_without_persist(provision_drive_p, packet_p, next_consumed_lba);
            return FBE_LIFECYCLE_STATUS_PENDING;            
    }

    /* return pending status since we are waiting for the permission.*/
    return FBE_LIFECYCLE_STATUS_PENDING;
} 
/******************************************************************************
 * end fbe_provision_drive_run_background_zeroing()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_run_background_zeroing_completion()
 ******************************************************************************
 *
 * @brief
 *    This is the completion function of disk zeroing operation. It is invoked when a 
 *    disk zeroing i/o operation has completed.  It reschedules the  monitor to
 *    run again in order to perform the next disk zeroing i/o operation.
 *
 *    Note: This function must always return FBE_STATUS_OK so that the  packet
 *          gets completed to the next level. If any other status is returned,
 *          the packet_p will get stuck.
 *
 * @param   in_packet_p   -  pointer to control packet from the scheduler
 * @param   in_context    -  context, which is a pointer to the base object
 * 
 * @return  fbe_status_t  -  always FBE_STATUS_OK
 *
 * @version
 *    02/12/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/

static fbe_status_t fbe_provision_drive_run_background_zeroing_completion(fbe_packet_t * packet_p,
                                                                        fbe_packet_completion_context_t context)
{

    fbe_provision_drive_t*          provision_drive_p;  // pointer to provision drive
    fbe_status_t                    status;             // fbe status

    /* cast the context to provision drive object */
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* get disk zero i/o completion status */
    status = fbe_transport_get_status_code(packet_p);

    /* trace error if zeroing i/o failed */
    if(status != FBE_STATUS_OK)
    {
        /* trace error in zero operation */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s disk zeroing i/o failed, status: 0x%x\n",
                              __FUNCTION__, status);

        /* If we do not have permision we should not rescedule */
        return FBE_STATUS_OK;
    }

#if 0 
/* 01-09: Per Architect's request, comment this out for now. */
    /* Trace if zeroing IO is slow */
    if(packet_p->physical_drive_service_time_ms > 200)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s PDO service time: %lld\n",
                              __FUNCTION__, packet_p->physical_drive_service_time_ms);

        /* If the IO is slow we should not rescedule */
        return FBE_STATUS_OK;
    }
#endif

    /* reschedule to continue disk zeroing, but don't do it too often because we still other drives' cpu time */
    if(provision_drive_p->base_config.background_operation_count < fbe_provision_drive_get_disk_zeroing_speed()){
        provision_drive_p->base_config.background_operation_count++;
        fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                                (fbe_base_object_t *) provision_drive_p,
                                (fbe_lifecycle_timer_msec_t) 0);                        
    } else {
        provision_drive_p->base_config.background_operation_count = 0;
        fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                                (fbe_base_object_t *) provision_drive_p,
                                (fbe_lifecycle_timer_msec_t) 200);                        
    }

    /* return success in all cases so the packet is completed to the next level */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_run_background_zeroing_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_run_verify()
 ******************************************************************************
 *
 * @brief
 *    This function performs sniff verify operation. It sends a permission request
 *    to upstream.
 *
 * @param   object_p  -  pointer to provision drive
 * @param   packet_p  -  pointer to control packet from the scheduler
 *
 * @return  fbe_lifecycle_status_t  -  FBE_LIFECYCLE_STATUS_DONE
 *                                     FBE_LIFECYCLE_STATUS_PENDING
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *    07/28/2011 - Modified - Amit Dhaduk
 *    03/02/2012 - Ashok Tamilarasan - Added Metadata Verify 
 *
 ******************************************************************************/

fbe_lifecycle_status_t fbe_provision_drive_run_verify(fbe_base_object_t*  object_p,
                                                      fbe_packet_t*       packet_p)
{
    fbe_provision_drive_t  *provision_drive_p;  // pointer to provision drive
    fbe_bool_t              b_sniff_enabled;    // sniff verify enable
    fbe_lba_t               start_lba = 0;          // starting lba - checkpoint
    fbe_lba_t               paged_metadata_start_lba;
    fbe_bool_t              is_event_outstanding;


    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( object_p );

    // cast the base object to provision drive object
    provision_drive_p = (fbe_provision_drive_t *) object_p;

    fbe_base_config_is_background_operation_enabled((fbe_base_config_t *) provision_drive_p, 
                                        FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF, &b_sniff_enabled);
    if(b_sniff_enabled == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
            FBE_TRACE_LEVEL_DEBUG_HIGH,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s sniff is disabled \n", __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_provision_drive_get_event_outstanding_flag(provision_drive_p, &is_event_outstanding);
    if(is_event_outstanding) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

     // set completion function
    fbe_transport_set_completion_function( packet_p,
                                           fbe_provision_drive_run_verify_completion,
                                           provision_drive_p);

    // get starting lba for next verify i/o
    fbe_provision_drive_metadata_get_sniff_verify_checkpoint( provision_drive_p, &start_lba );

    /* get the paged metadata start lba of the pvd object. */
    fbe_base_config_metadata_get_paged_record_start_lba((fbe_base_config_t *) provision_drive_p,
                                                        &paged_metadata_start_lba);

    if(start_lba >= paged_metadata_start_lba)
    {
        /* This is sniff verify of the paged metadata region which only PVD knows about
         * and so we dont need to get permission, just kick off the verify
         */
        fbe_provision_drive_start_next_verify_io( provision_drive_p, packet_p);
    }
    else
    {
        //ask upstream object for permission to do verify
        fbe_provision_drive_ask_verify_permission( object_p, packet_p  );
    }

    /* return pending status since we are waiting for the permission.*/
    return FBE_LIFECYCLE_STATUS_PENDING;

}   // end fbe_provision_drive_run_verify()



/*!****************************************************************************
 * fbe_provision_drive_run_verify_completion
 ******************************************************************************
 *
 * @brief
 *    This is the completion function of sniff operation. It is invoked when a 
 *    sniff verify i/o operation has completed.  It reschedules the  monitor to
 *    run again in order to perform the next sniff verify i/o operation.
 *
 *    Note: This function must always return FBE_STATUS_OK so that the  packet
 *          gets completed to the next level. If any other status is returned,
 *          the packet_p will get stuck.
 *
 * @param   packet_p   -  pointer to control packet from the scheduler
 * @param   context    -  context, which is a pointer to the base object
 * 
 * @return  fbe_status_t  -  always FBE_STATUS_OK
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_run_verify_completion(
                                               fbe_packet_t*  packet_p,
                                               fbe_packet_completion_context_t context
                                             )
{
    fbe_provision_drive_t*          provision_drive_p;  // pointer to provision drive
    fbe_status_t                    status;             // fbe status

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( context );

    // cast the context to provision drive object
    provision_drive_p = (fbe_provision_drive_t *) context;

    // get verify i/o completion status
    status = fbe_transport_get_status_code( packet_p );

    if ( status == FBE_STATUS_OK )
    {
#if 0 
/* 01-09: Per Architect's request, comment this out for now. */
        /* Trace if verify IO is slow */
        if(packet_p->physical_drive_service_time_ms > 200)
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s PDO service time: %lld\n",
                                  __FUNCTION__, packet_p->physical_drive_service_time_ms);

            /* If the IO is slow we should not rescedule */
            return FBE_STATUS_OK;
        }
#endif 
        // reschedule to continue verify of this disk
        fbe_lifecycle_reschedule( &fbe_provision_drive_lifecycle_const,
                                  (fbe_base_object_t*)provision_drive_p,
                                  (fbe_lifecycle_timer_msec_t) 1000
                                );
    }


    // return success in all cases so the packet is completed to the next level 
    return FBE_STATUS_OK;

}   // end fbe_provision_drive_run_verify_completion()


/*!****************************************************************************
 * fbe_provision_drive_do_remap_cond_function
 ******************************************************************************
 *
 * @brief
 *    This is the "do_remap" condition function which determines if  a  remap
 *    i/o operation needs to be performed. Calls an event to check if the bad block is consumed.
 *    If consumed - handled by upstream. If not - do remap. 
 *
 * @param   object_p  -  pointer to provision drive
 * @param   packet_p  -  pointer to control packet from the scheduler
 *
 * @return  fbe_lifecycle_status_t  -  FBE_LIFECYCLE_STATUS_DONE
 *                                     FBE_LIFECYCLE_STATUS_PENDING
 *
 * @version
 *    03/19/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_lifecycle_status_t
fbe_provision_drive_do_remap_cond_function( fbe_base_object_t*  object_p, fbe_packet_t*       packet_p )
{
    fbe_provision_drive_t * provision_drive_p;  // pointer to provision drive
    fbe_status_t            status;             // fbe status
    fbe_bool_t				remap_permission;
    fbe_lba_t				zero_chkpt;

    // cast the base object to provision drive object
    provision_drive_p = (fbe_provision_drive_t *) object_p;

    // terminate current monitor cycle if not active
    if ( !(fbe_base_config_is_active((fbe_base_config_t *)object_p)) ) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

     // ask remap permission
    status = fbe_provision_drive_get_verify_permission_by_priority(provision_drive_p,&remap_permission);
                                
    // terminate current monitor cycle if remap i/o permission not granted
    if ( (status != FBE_STATUS_OK) || (!remap_permission) ) {
        // set success status in packet and complete packet
        fbe_transport_set_status( packet_p, FBE_STATUS_OK, 0 );

        return FBE_LIFECYCLE_STATUS_DONE;        
    }

    // clear the condition if PVD is in ZOD mode    
    fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive_p, &zero_chkpt);

    if(zero_chkpt != FBE_LBA_INVALID)
    {
        // clear "do_remap" condition for this drive as PVD is in ZOD mode and we want to let zero finish first.
        // Once PVD will out of ZOD mode, sniff will start from same checkpoint and if it hit the error on unconsumed
        // area, PVD will do remap at that time. As per the priority, if PVD is in ZOD mode, Zeroing should run
        // first than REMAP.

        // This normally occur with following sequence
        // 1. Zeroing get completed and PVD is out of ZOD mode
        // 2. sniff ran and found media error on unconsumed area
        // 3. Sniff has initiated remap to run
        // 4. Before remap start, PVD get USER ZERO request and get transition to again ZOD mode
        // 5. In PVD ready rotary, REMAP is above background zeroing condition and will run first.

        status = fbe_lifecycle_clear_current_cond( (fbe_base_object_t *)provision_drive_p );
        
        // trace to know that why we clear "do_remap" condition
        if(status == FBE_STATUS_OK)
        {
            fbe_provision_drive_utils_trace(provision_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            FBE_PROVISION_DRIVE_DEBUG_FLAG_VERIFY_TRACING,                
                            "_do_remap_cond_function: PVD is in ZOD, cleared the condition\n");

        }
        else
        {
            // trace error in clearing "do_remap" condition
            fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "_do_remap_cond_function: PVD is in ZOD, unable to clear remap condition, status: 0x%x\n",status);
        }

        return FBE_LIFECYCLE_STATUS_DONE;
    }
   

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_do_remap_cond_completion, provision_drive_p);
    // now, start next remap i/o on this disk
    status = fbe_provision_drive_start_next_remap_io( provision_drive_p, packet_p );
    
    // pend current monitor cycle until this remap i/o completes
    return FBE_LIFECYCLE_STATUS_PENDING;

}   // end fbe_provision_drive_do_remap_cond_function()


/*!****************************************************************************
 * fbe_provision_drive_do_remap_cond_completion
 ******************************************************************************
 *
 * @brief
 *    This is the completion function for  the  "do_remap"  condition.   It is
 *    invoked  when  a  remap i/o operation has completed.  It reschedules the
 *    monitor to run again in order to perform the next i/o operation.
 *
 *    Note: This function must always return FBE_STATUS_OK so that the  packet
 *          gets completed to the next level. If any other status is returned,
 *          the packet_p will get stuck.
 *
 * @param   packet_p   -  pointer to control packet from the scheduler
 * @param   context    -  context, which is a pointer to the base object
 * 
 * @return  fbe_status_t  -  always FBE_STATUS_OK
 *
 * @version
 *    03/30/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_do_remap_cond_completion(
                                              fbe_packet_t*  packet_p,
                                              fbe_packet_completion_context_t context
                                            )
{
    fbe_provision_drive_t*          provision_drive_p;  // pointer to provision drive
    fbe_status_t                    status;             // fbe status

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( context );

    // cast the context to provision drive object
    provision_drive_p = (fbe_provision_drive_t *) context;

    // get remap i/o completion status
    status = fbe_transport_get_status_code( packet_p );

    // clear the "do_remap" condition for this drive if
    // last remap i/o operation completed successfully
    if ( status == FBE_STATUS_OK )
    {
        // clear "do_remap" condition for this drive
        status = fbe_lifecycle_clear_current_cond( (fbe_base_object_t *)provision_drive_p );
        
        // trace error if unable to clear "do_remap" condition
        if ( status != FBE_STATUS_OK )
        {
            // trace error in clearing "do_remap" condition
            fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s unable to clear current condition, status: 0x%x\n",
                                   __FUNCTION__, status
                                 );
        }
    }

    // return success in all cases so the packet is completed to the next level 
    return FBE_STATUS_OK;

}   // end fbe_provision_drive_do_remap_cond_completion()


/*!****************************************************************************
 * fbe_provision_drive_send_remap_event_cond_function
 ******************************************************************************
 *
 * @brief
 *    This condition function will just send the remap event to 
 *    upstream objects.
 *
 * @param   object_p  -  pointer to provision drive
 * @param   packet_p  -  pointer to control packet from the scheduler
 *
 * @return  fbe_lifecycle_status_t  
 *                                  
 *
 * @version
 *    03/02/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/

fbe_lifecycle_status_t
fbe_provision_drive_send_remap_event_cond_function(
                                            fbe_base_object_t*  object_p,
                                            fbe_packet_t*       packet_p
                                          )
{
    fbe_provision_drive_t*                     provision_drive_p;  
    fbe_status_t                               status;             
    fbe_lba_t                                  media_error_lba;
    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( object_p );

    // cast the base object to provision drive object
    provision_drive_p = (fbe_provision_drive_t *) object_p;

    // terminate current monitor cycle if not active
    if ( !(fbe_base_config_is_active((fbe_base_config_t *)object_p)) )
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    // set media error lba and block count
    fbe_provision_drive_metadata_get_sniff_media_error_lba( provision_drive_p, &media_error_lba );

    // send event up
    status = fbe_provision_drive_ask_remap_action(provision_drive_p,
                                                  packet_p,
                                                  media_error_lba,
                                                  1,
                                                  fbe_provision_drive_ask_remap_action_completion);

    fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: clearing the condition\n",
                       __FUNCTION__);
    fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);

    /* If the status is not ok, that means the called function completed the packet */
    if(status != FBE_STATUS_OK) 
    {
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;

}   // end fbe_provision_drive_send_remap_event_cond_function()


/*!****************************************************************************
 * fbe_provision_drive_clear_event_flag_cond_function
 ******************************************************************************
 *
 * @brief
 *    This condition function will just clear the event flag since 
 *    the data event that was outstanding was completed
 *
 * @param   object_p  -  pointer to provision drive
 * @param   packet_p  -  pointer to control packet from the scheduler
 *
 * @return  fbe_lifecycle_status_t  
 *                                  
 *
 * @version
 *    03/07/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/

fbe_lifecycle_status_t
fbe_provision_drive_clear_event_flag_cond_function(
                                            fbe_base_object_t*  object_p,
                                            fbe_packet_t*       packet_p
                                          )
{
    fbe_provision_drive_t*                     provision_drive_p;

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( object_p );

    // cast the base object to provision drive object
    provision_drive_p = (fbe_provision_drive_t *) object_p;

    // terminate current monitor cycle if not active
    if ( !(fbe_base_config_is_active((fbe_base_config_t *)object_p)) )
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    
    
    fbe_provision_drive_clear_event_outstanding_flag(provision_drive_p);

    fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: clearing the condition\n",
                       __FUNCTION__);

    fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);

    
    return FBE_LIFECYCLE_STATUS_DONE;

}   // end fbe_provision_drive_clear_event_flag_cond_function()



/*!****************************************************************************
 * fbe_provision_drive_remap_done_cond_function
 ******************************************************************************
 *
 * @brief
 *    This condition gets set after RAID has marked the chunk for error verify.
 *    This function will advance the checkpoint and set the media error lba to 
 *    invalid.
 *
 * @param   object_p  -  pointer to provision drive
 * @param   packet_p  -  pointer to control packet from the scheduler
 *
 * @return  fbe_lifecycle_status_t  
 *                                  
 *
 * @version
 *    03/1/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/

fbe_lifecycle_status_t
fbe_provision_drive_remap_done_cond_function(
                                            fbe_base_object_t*  object_p,
                                            fbe_packet_t*       packet_p
                                          )
{
    fbe_provision_drive_t*                     provision_drive_p;  
    fbe_status_t                               status;             


    provision_drive_p = (fbe_provision_drive_t *) object_p;

    if ( !(fbe_base_config_is_active((fbe_base_config_t *)object_p)) )
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s chunk marked for verfiy. Move the chkpt\n",
                           __FUNCTION__);

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_remap_done_cond_completion, provision_drive_p);
    status = fbe_provision_drive_advance_verify_checkpoint(provision_drive_p, packet_p);

    return FBE_LIFECYCLE_STATUS_PENDING;

}   // end fbe_provision_drive_remap_done_cond_function()


/*!****************************************************************************
 * fbe_provision_drive_remap_done_cond_completion
 ******************************************************************************
 *
 * @brief
 *    This is the completion function which will just clear the condition
 *
 * @param   packet_p   -  pointer to control packet from the scheduler
 * @param   context    -  context, which is a pointer to the base object
 * 
 * @return  fbe_status_t  -  always FBE_STATUS_OK
 *
 * @version
 *    03/01/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_remap_done_cond_completion(
                                              fbe_packet_t*  packet_p,
                                              fbe_packet_completion_context_t context
                                            )
{
    fbe_provision_drive_t*          provision_drive_p;  
    fbe_status_t                    status;             

    
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( context );

    provision_drive_p = (fbe_provision_drive_t *) context;

    status = fbe_transport_get_status_code(packet_p);
    if ( status == FBE_STATUS_OK )
    {
        status = fbe_lifecycle_clear_current_cond( (fbe_base_object_t *)provision_drive_p );
        // trace error in clearing "do_remap" condition
        fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_DEBUG_HIGH,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s clearing the cond\n",
                               __FUNCTION__);
                                
        if ( status != FBE_STATUS_OK )
        {
            // trace error in clearing "do_remap" condition
            fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s unable to clear current condition, status: 0x%x\n",
                                   __FUNCTION__, status
                                 );
        }
    }

    // reschedule to continue remap of this disk
    fbe_lifecycle_reschedule( &fbe_provision_drive_lifecycle_const,
                              (fbe_base_object_t*)provision_drive_p,
                              (fbe_lifecycle_timer_msec_t) 100
                            );

    return FBE_STATUS_OK;

}   // end fbe_provision_drive_remap_done_cond_completion()


/*!****************************************************************************
 * fbe_provision_drive_set_upstream_edge_priority
 ******************************************************************************
 *
 * @brief
 *    This function sets a new medic action priority on the upstream edge of
 *    the specified provision drive. The new priority is derived solely from
 *    the supplied medic action.
 *
 * @param   provision_drive_p  -  pointer to provision drive
 * @param   medic_action       -  medic action
 *
 * @return  fbe_status_t          -  status of this operation  
 *
 * @version
 *    03/04/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t fbe_provision_drive_set_upstream_edge_priority(fbe_provision_drive_t* provision_drive_p,
                                                            fbe_medic_action_t     medic_action)
{
    fbe_medic_action_priority_t     edge_priority;      // edge's new action priority
    fbe_status_t                    status;             // fbe status

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( provision_drive_p );

    // get action priority corresponding to supplied medic action
    status = fbe_medic_get_action_priority( medic_action, &edge_priority );

    if ( status != FBE_STATUS_OK )
    {
        return status;
    }

    // now, set new upstream edge priority
    status = fbe_base_config_set_upstream_edge_priority( (fbe_base_config_t *)provision_drive_p, edge_priority );

    return status;

}   // end fbe_provision_drive_set_upstream_edge_priority()


/*!****************************************************************************
 * provision_drive_hibernation_sniff_wakeup_cond_function
 ******************************************************************************
 *
 * @brief
 *    This function wakes up once a while and checks if we are sleeping
 *    for a time specified in base config. If so, we wake up which will also
 *    cause us to sniff. After a while the system will switch back to power saving
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   in_medic_action       -  medic action
 *
 * @return  fbe_status_t          -  status of this operation  
 *
 ******************************************************************************/
static fbe_lifecycle_status_t provision_drive_hibernation_sniff_wakeup_cond_function( fbe_base_object_t* in_object_p,
                                                                                      fbe_packet_t*      in_packet_p  )
{
    fbe_provision_drive_t *             provision_drive_p = (fbe_provision_drive_t* )in_object_p;
    fbe_base_config_t *                 base_config = (fbe_base_config_t *)in_object_p;
    fbe_u32_t                           seconds_sleeping = 0;
    fbe_system_power_saving_info_t      power_saving_info;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           total_edges = 0;
    fbe_path_state_t                    path_state;

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to clear condition %d\n", __FUNCTION__, status);                

    }

    /*check if we have any edges above us. If so, the RAID above us will take care of waking us up to sniff*/
    fbe_block_transport_server_get_number_of_clients(&base_config->block_transport_server, &total_edges);
    if (total_edges != 0) {
        return FBE_LIFECYCLE_STATUS_DONE;/*no need to do anything*/
    }

    fbe_block_transport_get_path_state(&(((fbe_base_config_t *)provision_drive_p)->block_edge_p[0]), &path_state);
    /*if spare pvd enter hibernate during slf, when the edge is attached again and path state become enabled,
      wake up pvd to eval slf*/
    if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF) &&
        (path_state == FBE_PATH_STATE_ENABLED))
    {
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_VERIFY_TRACING ,
                                        "%s Waking up from power saving to EVAL SLF\n", __FUNCTION__); 

        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                (fbe_base_object_t *)provision_drive_p,
                                FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);

        return FBE_LIFECYCLE_STATUS_DONE;        
    }    

    /*how long have we been sleeping?*/
    seconds_sleeping = fbe_get_elapsed_seconds(base_config->hibernation_time);

    /*do we need to wake up ?*/
    fbe_base_config_get_system_power_saving_info(&power_saving_info);
    if ((seconds_sleeping / 60) >= power_saving_info.hibernation_wake_up_time_in_minutes)
    {
        /*yes, we do. Let's just go to activeate and this will make sure our logical and physical drives 
        will wake up too as well so they can serve us*/
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_VERIFY_TRACING ,
                                        "%s Waking up from power saving to do a sniff\n", __FUNCTION__); 

        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                (fbe_base_object_t *)provision_drive_p,
                                FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);



    }

    return FBE_LIFECYCLE_STATUS_DONE;


}   // end provision_drive_hibernation_sniff_wakeup_cond_function()


/*!****************************************************************************
 * fbe_provision_drive_quiesce_cond_function()
 ******************************************************************************
 * @brief
 *  This condition function is used to quiesce the i/o operation.
 *
 * @param object_p               - pointer to a base object.
 * @param packet_p              - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when signature
 *                                    verification issued.
 *
 * @author
 *  06/25/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_quiesce_cond_function(fbe_base_object_t * object_p,
                                          fbe_packet_t * packet_p)
{

    fbe_status_t                status;
    fbe_provision_drive_t *     provision_drive_p = (fbe_provision_drive_t *) object_p;
    fbe_bool_t                  is_io_request_quiesce_b = FBE_FALSE;
    
    fbe_scheduler_hook_status_t hook_status;

    

    /* Cast the base object to a virtual drive object */
    provision_drive_p = (fbe_provision_drive_t *) object_p;

    // If a debug hook is set, we need to execute the specified action...
    fbe_provision_drive_check_hook(provision_drive_p,  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_QUIESCE, FBE_PROVISION_DRIVE_SUBSTATE_QUIESCE_ENTRY, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

    /* Quiesce all the i/o requests which are in progress and held the new
     * incoming request to block transport server.
     */
    fbe_base_config_quiesce_io_requests((fbe_base_config_t *)provision_drive_p, packet_p, 
                                        FBE_FALSE, /* The provision drive is not a pass-thru object*/
                                        &is_io_request_quiesce_b);
    if(is_io_request_quiesce_b)
    {
        /* Since we already quiesce i/o operation clear the quiesce condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)provision_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
    }
    else
    {
        /* Reschedule soon since I/Os are completing. */
        fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                                 (fbe_base_object_t *)provision_drive_p,
                                 50);
    }

    /* We are always returning DONE since we are not initiating packets. */
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_provision_drive_quiesce_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_unquiesce_cond_function()
 ******************************************************************************
 * @brief
 *  This condition function is used to unquiesce the i/o operation.
 *
 * @param object_p               - pointer to a base object.
 * @param packet_p              - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when signature
 *                                    verification issued.
 *
 * @author
 *  06/25/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_unquiesce_cond_function(fbe_base_object_t * object_p,
                                            fbe_packet_t * packet_p)
{
    fbe_status_t            status;
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t *)object_p;
    fbe_bool_t              is_io_request_unquiesce_ready_b = FBE_FALSE;


    /* Unquiesce all the request which is waiting for the restart. */
    status = fbe_base_config_unquiesce_io_requests((fbe_base_config_t *)provision_drive_p, packet_p, &is_io_request_unquiesce_ready_b);
    if (is_io_request_unquiesce_ready_b) 
    {
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed while doing unquiesce of pending i/o operation, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
    
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)provision_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        else
        {
            fbe_provision_drive_utils_trace(provision_drive_p, 
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                            FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                            "%s Unquiesced.\n",
                                            __FUNCTION__);
        }
    } else {
        /* Reschedule soon since peer may be getting in sync */
        fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                                 (fbe_base_object_t *)provision_drive_p,
                                 50);
    }

    /* We are always returning DONE since we are not initiating packets. */
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_provision_drive_unquiesce_cond_function()
 ******************************************************************************/





/*!****************************************************************************
 * fbe_provision_drive_set_zero_checkpoint_to_end_marker_cond_function()
 ******************************************************************************
 * @brief
 *  This condition function is used to clear the zero on demand flag, it first
 *  quiesce the i/o if needded and then update the non paged metadata to clear
 *  the zero on demand flag.
 *
 * @param object_p                  - pointer to a base object.
 * @param packet_p                  - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when signature
 *                                    verification issued.
 *
 * @author
 *  06/25/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_set_zero_checkpoint_to_end_marker_cond_function(fbe_base_object_t * object_p,
                                                                    fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_provision_drive_t *                 provision_drive_p = (fbe_provision_drive_t *) object_p;
    fbe_lba_t                               exported_capacity = FBE_LBA_INVALID;
    fbe_lba_t                               zero_checkpoint = FBE_LBA_INVALID;    
    fbe_scheduler_hook_status_t             hook_status;

    

    /* Cast the base object to a virtual drive object */
    provision_drive_p = (fbe_provision_drive_t *) object_p;

    // If a debug hook is set, we need to execute the specified action...
    fbe_provision_drive_check_hook(provision_drive_p,  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO, FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

    /* get the curent zero checkpoint, if it is already set to LBA invalid then clear the condition. */
    status = fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive_p, &zero_checkpoint);
    if (status != FBE_STATUS_OK) 
    {
        fbe_provision_drive_utils_trace( provision_drive_p,
                     FBE_TRACE_LEVEL_WARNING,
                     FBE_TRACE_MESSAGE_ID_INFO, 
                     FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                     "pvd_set_zero_ckpt_to_end_marker_cond_func: Failed to get checkpoint. Status: 0x%X\n", status);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if(zero_checkpoint == FBE_LBA_INVALID)
    {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* if current zero checkpoint is below exported capacity then then clear the condition. */
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &exported_capacity);
    if(zero_checkpoint < exported_capacity)
    {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* zeroing gets completed for the provision drive object, update checkpoint as invalid. */
    fbe_provision_drive_utils_trace( provision_drive_p,
                                     FBE_TRACE_LEVEL_INFO,
                                     FBE_TRACE_MESSAGE_ID_INFO, 
                                     FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                     "BG_ZERO:zeroing gets completed, zero_chk:0x%llx, export_cap:0x%llx\n",
                                     (unsigned long long)zero_checkpoint,
                     (unsigned long long)exported_capacity);

#if 0
    /* Verify whether we have already quiesce i/o or not, if we find i/o is not then quiesce
     * the I/O before we update zero zero checkpoint as lba invalid.
     */
    fbe_provision_drive_utils_quiesce_io_if_needed(provision_drive_p, &is_io_already_quiesce_b);
    if(is_io_already_quiesce_b == FBE_FALSE)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }
#endif
    /* log disk zero complete event */
    fbe_provision_drive_event_log_disk_zero_start_or_complete(provision_drive_p, FBE_FALSE, packet_p);

    /* Update the nonpaged metadata with checkpoint as invalid. */  
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_set_zero_checkpoint_to_end_marker_cond_completion, provision_drive_p);    
    status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_set_zero_checkpoint_to_end_marker_verify_flags);    
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_provision_drive_set_zero_checkpoint_to_end_marker_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_set_zero_checkpoint_to_end_marker_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for updating the zero checkpoint
 *  as logical disk end.
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
fbe_provision_drive_set_zero_checkpoint_to_end_marker_cond_completion(fbe_packet_t * packet_p,
                                                                      fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* Get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* Clear the set logical marker end condition if status is ok. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {    
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                    FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X", __FUNCTION__, status);
        }
    }
    else
    {   
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                        "%s update zero checkpoint to end marker cond failed, payload_status:%d, packet_status:%d\n",
                                        __FUNCTION__, control_status, status);
    }

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_set_zero_checkpoint_to_end_marker_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_set_zero_checkpoint_to_end_marker()
 ******************************************************************************
 * @brief
 *  This function is used to set the checkpoint to the logical end marker
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t 
 *
 * @author
 *  10/18/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_set_zero_checkpoint_to_end_marker(fbe_packet_t * packet_p,
                                                      fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_lba_t                           exported_capacity = FBE_LBA_INVALID;
    fbe_lba_t                           zero_checkpoint = FBE_LBA_INVALID;
    fbe_status_t                        status;
    
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just complete the packet
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
        fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);
        return FBE_STATUS_CONTINUE;
    }


    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);

    /* get the curent zero checkpoint, if it is already set to LBA invalid then clear the condition. */
    status = fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive_p, &zero_checkpoint);
    if (status != FBE_STATUS_OK) 
    {
        fbe_provision_drive_utils_trace( provision_drive_p,
                     FBE_TRACE_LEVEL_WARNING,
                     FBE_TRACE_MESSAGE_ID_INFO, 
                     FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                     "pvd_set_zero_ckpt_to_end_marker: Failed to get checkpoint. Status: 0x%X\n", status);

        return FBE_STATUS_OK;
    }

    if(zero_checkpoint == FBE_LBA_INVALID)
    {
        fbe_transport_complete_packet(packet_p);        
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* if current zero checkpoint is below exported capacity then then clear the condition. */
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &exported_capacity);
    if(zero_checkpoint < exported_capacity)
    {        
        fbe_transport_complete_packet(packet_p);        
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    
    fbe_provision_drive_metadata_set_background_zero_checkpoint(provision_drive_p, packet_p, FBE_LBA_INVALID);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_set_zero_checkpoint_to_end_marker()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_set_zero_checkpoint_to_end_marker_verify_flags()
 ******************************************************************************
 * @brief
 *  This function is used to set proper NP flags before setting
 * the checkpoint to the logical end marker.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t 
 *
 * @author
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_set_zero_checkpoint_to_end_marker_verify_flags(fbe_packet_t * packet_p,
                                                      fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_status_t                        status;
    fbe_provision_drive_np_flags_t      np_flags;
	fbe_system_encryption_mode_t		system_encryption_mode;
    
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just complete the packet
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    status = fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &np_flags);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s failed to get non-paged flags, status %d\n",
                              __FUNCTION__, status);
        return FBE_STATUS_OK;
    }

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_set_zero_checkpoint_to_end_marker, provision_drive_p);

    fbe_database_get_system_encryption_mode(&system_encryption_mode);

    if ((system_encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) ||
        ((np_flags & FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED) != FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED))
    {
        /* if we don't need to update np flags, just move on to set zero checkpoint */
        return FBE_STATUS_CONTINUE;
    }

    np_flags &= ~FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED;
    np_flags |= FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_COMPLETE;

    status = fbe_provision_drive_metadata_np_flag_set(provision_drive_p, packet_p, np_flags);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_set_zero_checkpoint_to_end_marker_verify_flags()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_set_end_of_life_cond_function()
 ******************************************************************************
 * @brief
 *  This condition function is used to set the end of life attributes in non
 *  paged metadata for this drive.
 *
 * @param object_p                  - pointer to a base object.
 * @param packet_p                  - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - lifecycle status.
 *
 * @author
 *  08/20/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_set_end_of_life_cond_function(fbe_base_object_t * object_p,
                                                  fbe_packet_t * packet_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_provision_drive_t      *provision_drive_p = (fbe_provision_drive_t *) object_p;
    fbe_bool_t                  end_of_life_state = FBE_FALSE;

    /* Cast the base object to a virtual drive object */
    provision_drive_p = (fbe_provision_drive_t *) object_p;

    /* get the curent zero checkpoint, if it is already set to LBA invalid then clear the condition. */
    fbe_provision_drive_metadata_get_end_of_life_state(provision_drive_p, &end_of_life_state);
    if(end_of_life_state == FBE_TRUE)
    {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_QUIESCE);

    /* Update the nonpaged metadata with checkpoint as invalid. */  
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_set_end_of_life_cond_function_completion, provision_drive_p);
    status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_set_end_of_life);   
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_provision_drive_set_end_of_life_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_set_end_of_life_cond_function_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for persisting the eol bit
 *  in nonpaged metadata
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  08/20/2010 - Created. Dhaval Patel
 *  10/31/2012 - Updated by adding user disk EOL event log. Wenxuan Yin
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_set_end_of_life_cond_function_completion(fbe_packet_t * packet_p,
                                                             fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t                     object_id = FBE_OBJECT_ID_INVALID;

    provision_drive_p = (fbe_provision_drive_t*)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* clear the the set end of life condition if status is ok. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* clear the set end of life bit condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                    FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X", __FUNCTION__, status);
            return status;
        }

        /* set the verify end of life state condition to update the upstream edge if needed. */
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *)provision_drive_p,
                               FBE_PROVISION_DRIVE_LIFECYCLE_COND_VERIFY_END_OF_LIFE_STATE);

        /* log an event for system drives. */
        fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
        if (fbe_database_is_object_system_pvd(object_id))
        {
            fbe_provision_drive_write_event_log(provision_drive_p, SEP_WARN_SYSTEM_DISK_NEED_REPLACEMENT);
        }
        else /* log an event for user drives. */
        {
            fbe_provision_drive_write_event_log(provision_drive_p, SEP_WARN_USER_DISK_END_OF_LIFE);
        }
    }
    else
    {   
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s end of life state persist failed, payload_status:%d, packet_status:%d\n",
                              __FUNCTION__, control_status, status);
    }

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_set_end_of_life_cond_function_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_set_end_of_life()
 ******************************************************************************
 * @brief
 *  This function is used to set the end of life in the NP and persist it
 *  as logical disk end.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  10/18/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_set_end_of_life(fbe_packet_t * packet_p,
                                    fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;    
    fbe_status_t                        status;
    
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return, we are already in the completion routine
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }
    

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);
    fbe_provision_drive_metadata_set_end_of_life_state(provision_drive_p, packet_p, FBE_TRUE);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_set_end_of_life()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_verify_eol_usurper_completion()
 ******************************************************************************
 * @brief
 *  This function is the completion function for sending usurper commands
 *  to PDO for eol state for unconsumed PVD.
 * 
 * @param packet_p   - Pointer to the packet.
 * @param context    - context, which is a pointer to the base object.
 *
 * @return fbe_status_t
 *
 * @author
 *  08/01/2012 - Created. Lili Chen
 * 
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_verify_eol_usurper_completion(fbe_packet_t * packet_p,
                                                  fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = (fbe_provision_drive_t*)context;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;

    /* Get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {    
        /* set the downstream health broken condition to change the state to fail. */
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear condition, status: 0x%x", __FUNCTION__, status);
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_verify_eol_usurper_completion()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_verify_end_of_life_state_cond_function()
 ******************************************************************************
 * @brief
 *  This condition function is used to verify the end of life state, it sets
 *  end of life on upstream edge if it has upstream edge connected else it will
 *  transition to fail state and remain to fail state.
 *
 * @param object_p                  - pointer to a base object.
 * @param packet_p                  - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - lifecycle status.
 *
 * @author
 *  08/20/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_verify_end_of_life_state_cond_function(fbe_base_object_t * object_p,
                                                           fbe_packet_t * packet_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_provision_drive_t      *provision_drive_p = (fbe_provision_drive_t *) object_p;
    fbe_bool_t                  end_of_life_state = FBE_FALSE;
    fbe_u32_t                   number_of_clients;
    fbe_scheduler_hook_status_t hook_status;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_block_transport_logical_state_t state;
    fbe_block_edge_t                   *edge_p = NULL;
    fbe_path_state_t                    path_state;

    /* cast the base object to a virtual drive object */
    provision_drive_p = (fbe_provision_drive_t *)object_p;


    /* If a debug hook is set, we need to execute the specified action.
     */
    fbe_provision_drive_check_hook(provision_drive_p,  
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EOL, 
                                   FBE_PROVISION_DRIVE_SUBSTATE_SET_EOL, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* get the curent zero checkpoint, if it is already set to LBA invalid then clear the condition. */
    fbe_provision_drive_metadata_get_end_of_life_state(provision_drive_p, &end_of_life_state);
    if (end_of_life_state == FBE_TRUE)
    {
        /* update the upstream edge with end of life bit attributes if pvd has upstream edge. */
        fbe_block_transport_server_get_number_of_clients(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                         &number_of_clients);
        if (number_of_clients != 0)
        {
            /* If EOL is not already set, set it now and generate the attribute 
             * change event upstream.
             */
            fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(
                &((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE,  /* Path attribute to set */
                FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE   /* Mask of attributes to not set if already set*/);
        }
        else
        {
            fbe_base_config_get_block_edge( (fbe_base_config_t *) provision_drive_p, &edge_p, 0);
            fbe_block_transport_get_path_state(edge_p, &path_state);
            if (path_state != FBE_PATH_STATE_INVALID)
            {
                /* Send usurper command to PDO */
                payload_p = fbe_transport_get_payload_ex(packet_p);
                control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
                state = FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_EOL;
                fbe_payload_control_build_operation(control_operation_p,
                                                    FBE_BLOCK_TRANSPORT_CONTROL_CODE_LOGICAL_DRIVE_STATE_CHANGED,
                                                    &state,
                                                    sizeof(fbe_block_transport_logical_state_t));
                fbe_transport_set_completion_function(packet_p, 
                                                      fbe_provision_drive_verify_eol_usurper_completion,
                                                      provision_drive_p);
                fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
                status = fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);
                return FBE_LIFECYCLE_STATUS_PENDING;
            }
            else
            {
                /* set the downstream health broken condition to change the state to fail. */
                fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                       (fbe_base_object_t *)provision_drive_p,
                                       FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
            }
        }
    }

    /* clear the condition after updating upstream edge. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X", __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_provision_drive_verify_end_of_life_state_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_update_physical_drive_info_cond_function()
 ******************************************************************************
 * @brief
 *  This condition function is used to update the enclosure drive slot
 *  information in nonpaged metadata.
 *
 * @param object_p                  - pointer to a base object.
 * @param packet_p                  - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - lifecycle status.
 *
 * @author
 *  12/10/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_update_physical_drive_info_cond_function(fbe_base_object_t * object_p,
                                                             fbe_packet_t * packet_p)
{
    fbe_status_t                        status;
    fbe_provision_drive_t *             provision_drive_p = (fbe_provision_drive_t *) object_p;
    fbe_base_config_downstream_health_state_t   downstream_health_state;

    /* cast the base object to a virtual drive object */
    provision_drive_p = (fbe_provision_drive_t *) object_p;

    /* we might be in single loop failre */
    downstream_health_state = fbe_provision_drive_verify_downstream_health(provision_drive_p);
    if ((downstream_health_state == FBE_DOWNSTREAM_HEALTH_BROKEN) &&
        fbe_provision_drive_is_slf_enabled() &&
        fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s condition skipped for SLF", __FUNCTION__);
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* set the condition completion function before issuing get bus, encl, slot control operation. */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_provision_drive_update_physical_drive_info_cond_completion,
                                          provision_drive_p);

    /* get the bus, encl and slot info */
    status = fbe_provision_drive_metadata_get_physical_drive_info(provision_drive_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_provision_drive_update_physical_drive_info_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_update_physical_drive_info_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for updating physical drive
 *  information.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  12/10/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_update_physical_drive_info_cond_completion(fbe_packet_t * packet_p,
                                                               fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_config_downstream_health_state_t downstream_health_state;

    provision_drive_p = (fbe_provision_drive_t*)context;
    downstream_health_state = fbe_provision_drive_verify_downstream_health(provision_drive_p);

    /* Get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* Clear the set logical marker end condition if status is ok. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && 
       (status == FBE_STATUS_OK))
    {    
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear condition, status: 0x%x", __FUNCTION__, status);
        }
    }
    else
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                        "%s update drive location failed, ctl_stat:%d, stat:%d\n",
                                        __FUNCTION__, control_status, status);
    }

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_update_physical_drive_info_cond_completion()
 ******************************************************************************/


static fbe_status_t 
fbe_provision_drive_stripe_lock_start_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_base_config_t * base_config = NULL;
    fbe_provision_drive_t * provision_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_stripe_lock_operation_t * stripe_lock_operation = NULL;
    fbe_payload_stripe_lock_status_t stripe_lock_status;
    fbe_status_t packet_status;

    base_config = (fbe_base_config_t *)context;
    provision_drive = (fbe_provision_drive_t *)base_config;

    packet_status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(payload);


    /* Get the stripe lock operation
     */
    fbe_payload_stripe_lock_get_status(stripe_lock_operation, &stripe_lock_status);
#if 0
    if (((stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_DROPPED) ||
         (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED)) &&
        ((stripe_lock_operation->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_ALLOW_HOLD) != 0))
    {
        /* For dropped requests we will enqueue to the terminator queue to allow this request to
         * get processed later.  We were dropped because of a drive going away or a quiesce. 
         * Once the monitor resumes I/O, this request will get restarted. 
         */
        fbe_base_object_trace((fbe_base_object_t*)provision_drive, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s failed. queuing req. sl_status: 0x%x pckt_status: 0x%x op: %d\n", 
                              __FUNCTION__, stripe_lock_status, packet_status, stripe_lock_operation->opcode);
 
        /* Do not queue redirected IOs. */
        if (packet->packet_attr & FBE_PACKET_FLAG_REDIRECTED)
        {
            fbe_payload_ex_release_stripe_lock_operation(payload, stripe_lock_operation);
            fbe_transport_set_status(packet, FBE_STATUS_QUIESCED, 0);
            return FBE_STATUS_OK;
        }
        fbe_payload_ex_get_iots(payload, &iots_p);
        fbe_raid_iots_transition_quiesced(iots_p, fbe_provision_drive_iots_resend_lock_start, provision_drive);
        fbe_payload_ex_release_stripe_lock_operation(payload, stripe_lock_operation);
        fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)provision_drive, packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    /*  Any other failures will be failed back as usual */
#endif    

    fbe_payload_ex_release_stripe_lock_operation(payload, stripe_lock_operation);

    fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                             (fbe_base_object_t *) base_config,
                             (fbe_lifecycle_timer_msec_t) 0);
    
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_provision_drive_iots_resend_lock_start()
 ****************************************************************
 * @brief
 *  Restart the iots after it has been queued.
 *  It was queued just before getting a lock
 *
 * @param iots_p - I/O to restart.               
 *
 * @return fbe_raid_state_status_t - FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  8/19/2011 - Created. Wayne Garrett
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_provision_drive_iots_resend_lock_start(fbe_raid_iots_t *iots_p)
{
    fbe_packet_t * packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t*)iots_p->callback_context;
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_stripe_lock_operation_t * stripe_lock_operation_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_mark_unquiesced(iots_p);

    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: restarting iots %p\n", __FUNCTION__, iots_p);

    /* Remove from the terminator queue and get the stripe lock.
     */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *) provision_drive_p, packet_p);

    /* resend request */
    stripe_lock_operation_p = fbe_payload_ex_allocate_stripe_lock_operation(sep_payload_p);
    fbe_payload_stripe_lock_build_start(stripe_lock_operation_p, &((fbe_base_config_t *)provision_drive_p)->metadata_element);
    fbe_payload_ex_increment_stripe_lock_operation_level(sep_payload_p);
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_stripe_lock_start_completion, provision_drive_p);
    fbe_stripe_lock_operation_entry(packet_p);

    return FBE_RAID_STATE_STATUS_WAITING;
}
/******************************************************************************
 * end fbe_provision_drive_iots_resend_lock_start()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_download_cond_function()
 ******************************************************************************
 * @brief
 *  This condition function is used to prevent unquiesce from happening.
 *
 * @param object_p               - pointer to a base object.
 * @param packet_p              - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t 
 *
 * @author
 *  04/25/2011 - Created. Ruomu Gao
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_download_cond_function(fbe_base_object_t * object_p,
                                           fbe_packet_t * packet_p)
{
    fbe_provision_drive_t               *provision_drive_p = (fbe_provision_drive_t *)object_p;

    fbe_provision_drive_lock(provision_drive_p);

    /* The local state determines which function processes this condition.
     */
    if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_TRIAL_RUN))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl cond_function, DOWNLOAD_TRIAL_RUN  Lclst:0x%llx Pclst:0x%llx Lst:0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        return fbe_provision_drive_ask_download_trial_run_permission(provision_drive_p, packet_p);
    }
    if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_STARTED))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl cond_function, DOWNLOAD_STARTED    Lclst:0x%llx Pclst:0x%llx Lst:0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        return fbe_provision_drive_download_started_originator(provision_drive_p, packet_p);
    }
    else if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_PEER_START))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl cond_function, DOWNLOAD_PEER_START Lclst:0x%llx Pclst:0x%llx Lst:0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        return fbe_provision_drive_download_start_non_originator(provision_drive_p, packet_p);
    }
    else if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_PEER_STOP))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl cond_function, DOWNLOAD_PEER_STOP  Lclst:0x%llx Pclst:0x%llx Lst:0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        return fbe_provision_drive_download_stop_non_originator(provision_drive_p, packet_p);
    }
    else if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_PUP))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl cond_function, DOWNLOAD_PUP        Lclst:0x%llx Pclst:0x%llx Lst:0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        return fbe_provision_drive_download_pup(provision_drive_p, packet_p);
    }
    else if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_DONE))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl cond_function, DOWNLOAD_DONE       Lclst:0x%llx Pclst:0x%llx Lst:0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        return fbe_provision_drive_download_done(provision_drive_p, packet_p);
    }
    else // FBE_PVD_LOCAL_STATE_DOWNLOAD_REQUEST
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl cond_function, DOWNLOAD_REQUEST    Lclst:0x%llx Pclst:0x%llx Lst:0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        return fbe_provision_drive_download_request(provision_drive_p, packet_p);
    }
}

/*!****************************************************************************
 *          fbe_provision_drive_zero_paged_metadata_cond_function()
 ******************************************************************************
 *
 * @brief   This function is used to zero the paged metadata.  But since the 
 *          default values are NOT zero (for instance the Needs Zero (NZ) and 
 *          Valid (V) bits MUST be set to 1).  Therefore this method currently 
 *          doesn't do anything (just a placeholder).
 *
 * @param   provision_drive_p         - pointer to the provision drive
 * @param   packet_p                  - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @note    Currently there is no need to zero the paged metadata since we 
 *          must write default values (i.e. NZ: 1, V: 1).  Therefore simply
 *          complete this lifecycle.
 *
 * @author
 *   03/01/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_provision_drive_zero_paged_metadata_cond_function(fbe_base_object_t *object_p, fbe_packet_t *packet_p)
{
    fbe_provision_drive_t  *provision_drive_p = NULL;

    provision_drive_p = (fbe_provision_drive_t*)object_p;
    
    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING,
                                    "%s entry\n", __FUNCTION__);

    /*! @note Currently there is no need to zero the paged metadata since we 
     *        must write default values (i.e. NZ: 1, V: 1).  Therefore simply
     *        complete this lifecycle.
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)provision_drive_p);

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*********************************************************************
 * fbe_provision_drive_metadata_verify_invalidate_cond_function()
 ***********************************************************************
 * @brief
 *  This function checks if we need to verify the metadata region and invalidate
 *  the corresponding user region if applicable
 *  
 * @param in_object_p - The pointer to the provision drive.
 * @param in_packet_p - The control packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE or
 *                                  status returned from sub condition function
 *
 *
 * @author
 *  03/09/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_metadata_verify_invalidate_cond_function(fbe_base_object_t * object_p,
                                                               fbe_packet_t * packet_p)
{

    fbe_provision_drive_t *         provision_drive_p = NULL;
    fbe_status_t                    status;

    provision_drive_p = (fbe_provision_drive_t*)object_p;

    /* Set the completion function for the metadata verify invalidate condition. 
     */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_set_invalidate_checkpoint_clear_condition_completion,
                                          provision_drive_p);
    status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, 
                                             fbe_provision_drive_set_verify_invalidate_checkpoint_to_zero_with_np_lock);

    return FBE_LIFECYCLE_STATUS_PENDING;

}
/********************************************************************
 * end fbe_provision_drive_metadata_verify_invalidate_cond_function()
 ********************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_set_verify_invalidate_checkpoint_to_zero_with_np_lock()
 ******************************************************************************
 * @brief
 *   This function is completion that gets called after NP lock is obtained for
 *   the case where was metadata read error during IO. This moves the checkpoint
 *   to Zero to kick off metadata verify
 *
 * @param packet_p                  - pointer to a monitor packet.
 * @param context                   - pointer to the provision drive
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_set_verify_invalidate_checkpoint_to_zero_with_np_lock(fbe_packet_t * packet_p,
                                                                          fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t * provision_drive_p;
    fbe_status_t    status;
    fbe_payload_ex_t *                 sep_payload_p = NULL;

    /* get the payload and previous block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
  
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return. we are already in the completion function
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_OK;
    }
    
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_VERIFY_TRACING,
                                    "pvd_set_vr_inv_chk_with_lock. Set the metadata verify checkpoint to 0\n");

    fbe_provision_drive_metadata_set_verify_invalidate_checkpoint(provision_drive_p,
                                                                  packet_p,
                                                                  0);


    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 * fbe_provision_drive_reset_verify_invalidate_checkpoint_with_np_lock()
 ******************************************************************************
 * @brief
 *   This function is completion that gets called after NP lock is obtained to
 * reset the checkpoint to invalid on completion of verify invalidate operation 
 *
 * @param packet_p                  - pointer to a monitor packet.
 * @param context                   - pointer to the provision drive
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_reset_verify_invalidate_checkpoint_with_np_lock(fbe_packet_t * packet_p,
                                                                    fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t * provision_drive_p;
    fbe_status_t    status;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
   
    /* get the payload and previous block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
  
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return. we are already in the completion function
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_OK;
    }
    
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_VERIFY_TRACING,
                                    "pvd_verify_invalidate_checkpoint_with_np_lock Resetting checkpoint\n");

   fbe_provision_drive_metadata_set_verify_invalidate_checkpoint(provision_drive_p,
                                                                 packet_p,
                                                                 FBE_LBA_INVALID);


    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!*********************************************************************
 * fbe_provision_drive_background_monitor_operation_cond_function()
 ***********************************************************************
 * @brief
 *  This is the common condition function that checks Background monitor operation,
 *  including disk zeroing and sniff verify and run it if needed based on following priority 
 *  order. 
 *
 *  Background operation      priority
 *  --------------------      -------------
 *  Disk zeroing operation         High priority
 *  Sniff Verify operation         second priority(after disk zeroing)
 *
 *  This condition runs only one background operation at a time on active side.
 *  It will not evaluate lower priority background operation if higher priority 
 *  operation needs to run.
 *  
 * @param in_object_p - The pointer to the provision drive.
 * @param in_packet_p - The control packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE or
 *                                  status returned from sub condition function
 *
 *
 * @note - 
 *  Currently disk zeroing operation is disabled by default hence commented
 *  out in this condition and keep it as individual condition to start it from
 *  usurper command.
 *
 * @author
 *  07/28/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_background_monitor_operation_cond_function(fbe_base_object_t * object_p,
                                                               fbe_packet_t * packet_p)
{
    fbe_status_t            status;
    fbe_provision_drive_t  *provision_drive_p = NULL;
    fbe_lifecycle_status_t  lifecycle_status;
    fbe_power_save_state_t  power_save_state;
    fbe_bool_t              b_is_active;
    fbe_bool_t              b_need_to_redirect = FBE_FALSE;
    fbe_base_config_encryption_mode_t encryption_mode;
    fbe_u32_t               number_of_clients;
    fbe_bool_t              b_bg_encrypt_allowed = FBE_TRUE;
    fbe_scheduler_hook_status_t hook_status;

    /* Get the provision drive object and determine if we are active.
     */
    provision_drive_p = (fbe_provision_drive_t*)object_p;
    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);

    /* monitor the SLF changes in both SPs */
    if (fbe_provision_drive_is_slf_enabled())
    {
        fbe_path_attr_t path_attr;
        fbe_path_state_t path_state;
        fbe_block_transport_get_path_attributes(&(((fbe_base_config_t *)provision_drive_p)->block_edge_p[0]), &path_attr);        
        fbe_block_transport_get_path_state(&(((fbe_base_config_t *)provision_drive_p)->block_edge_p[0]), &path_state);
        fbe_provision_drive_initiate_eval_slf(provision_drive_p, path_state, path_attr);

        if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF) &&
            (path_state == FBE_PATH_STATE_GONE))
        {
            /* We need to detach the edge */
            fbe_base_config_detach_edge((fbe_base_config_t *) provision_drive_p, packet_p, &(((fbe_base_config_t *)provision_drive_p)->block_edge_p[0]));
            fbe_provision_drive_set_drive_location(provision_drive_p, FBE_PORT_NUMBER_INVALID, FBE_ENCLOSURE_NUMBER_INVALID, FBE_SLOT_NUMBER_INVALID);
        }
    }


    if (fbe_provision_drive_slf_need_redirect(provision_drive_p))
    {
        b_need_to_redirect = FBE_TRUE;
    }

    /* If EVAL SLF finished and we are active, check passive request. */
    if (fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_CHECK_PASSIVE_REQUEST) &&
        fbe_provision_drive_slf_check_passive_request_after_eval_slf(provision_drive_p))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s send passive request after EVAL SLF\n",
                              __FUNCTION__);
        fbe_base_config_passive_request((fbe_base_config_t *) provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    /* If we were asked to scrub, do it now */
    if (b_is_active && (fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_INTENT)))
    {
        /* If a debug hook is set, we need to execute the specified action. */
        fbe_provision_drive_check_hook(provision_drive_p,  
                                       SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO, 
                                       FBE_PROVISION_DRIVE_SUBSTATE_SCRUB_INTENT, 
                                       0, 
                                       &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) {
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        fbe_provision_drive_initiate_consumed_area_zeroing(provision_drive_p, packet_p);

        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* Currently "unbound" encrypted PVD should not run BG operations, except scrubbing. */
    fbe_base_config_get_encryption_mode((fbe_base_config_t *) provision_drive_p, &encryption_mode);
    fbe_block_transport_server_get_number_of_clients(&((fbe_base_config_t *)provision_drive_p)->block_transport_server, &number_of_clients);
    if ((encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) && 
        ((number_of_clients == 0) &&
         (!(fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED))))) {
        b_bg_encrypt_allowed = FBE_FALSE;
    }

    /* For both the active and passive check if we need to invalidate lost 
     * metadata.
     */
    if (b_bg_encrypt_allowed && fbe_provision_drive_metadata_verify_invalidate_is_need_to_run(provision_drive_p))
    {
        /* If we are active and not redirecting, send the verify invalidate 
         * request.
         */
        if ((b_is_active == FBE_TRUE) && (b_need_to_redirect == FBE_FALSE))
        {
            lifecycle_status = fbe_provision_drive_run_metadata_verify_invalidate(object_p, packet_p);
            return lifecycle_status;
        }

        /* If passive or we need to redirect we are done.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Now check if background zeroing needs to run.
     */
    if (b_bg_encrypt_allowed && fbe_base_config_is_system_bg_service_enabled(FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ZEROING) && 
        fbe_provision_drive_background_op_is_zeroing_need_to_run(provision_drive_p))
    {
        /* For both the passive and active set the upstream medic action priority.
         */
        status = fbe_provision_drive_set_upstream_edge_priority(provision_drive_p, FBE_MEDIC_ACTION_ZERO);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s disk zeroing set upstream priority failed, status: 0x%x\n",
                                  __FUNCTION__, status);
        }

        /* If we are active and not redirecting, send the background zero 
         * request.
         */
        if ((b_is_active == FBE_TRUE) && (b_need_to_redirect == FBE_FALSE))
        {
            lifecycle_status = fbe_provision_drive_run_background_zeroing(object_p, packet_p);
            return lifecycle_status;
        }

        /* If passive or we need to redirect we are done.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Lock base config before getting the power saving state */
    fbe_base_config_lock((fbe_base_config_t *)provision_drive_p);

    /* If we made it to here, we are ready to check if we need to save power
     * we need to do it before sniff because sniff is almost always enabled 
     * and will not let us check power save.
     */
    fbe_base_config_metadata_get_power_save_state((fbe_base_config_t *)provision_drive_p, &power_save_state);

    /* Unlock base config after getting the power saving state */
    fbe_base_config_unlock((fbe_base_config_t *)provision_drive_p);

    if (power_save_state != FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE && fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_SPIN_DOWN_QUALIFIED)) 
    {
        base_config_check_power_save_cond_function(object_p, packet_p);
    }

    /* Now check if we need to query the wear leveling for this drive 
     * Only query for low endurance ssd drives and if wear leveling timer is exceeded
     */
    if ((fbe_provision_drive_class_get_wear_leveling_timer() > 0) &&
        (b_is_active == FBE_TRUE) && (b_need_to_redirect == FBE_FALSE) &&
        fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_REPORT_WPD_WARNING) &&
        (fbe_get_elapsed_seconds(fbe_provision_drive_get_wear_leveling_query_time(provision_drive_p)) > fbe_provision_drive_class_get_wear_leveling_timer()))
    {
        lifecycle_status = fbe_provision_drive_get_wear_leveling(provision_drive_p, packet_p);
        return lifecycle_status;
    }

    
    /* Now check if sniff verify needs to run.
     */
    if (fbe_base_config_is_system_bg_service_enabled(FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_SNIFF_VERIFY) &&
        fbe_provision_drive_background_op_is_sniff_need_to_run(provision_drive_p))       
    {

        /* For both the passive and active set the upstream medic action priority.
         */
        status = fbe_provision_drive_set_upstream_edge_priority(provision_drive_p, FBE_MEDIC_ACTION_SNIFF);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s sniff set upstream priority failed, status: 0x%x\n",
                                  __FUNCTION__, status);
        }

        /* If we are active and not redirecting, send the sniff verify
         * request.
         */
        if ((b_is_active == FBE_TRUE) && (b_need_to_redirect == FBE_FALSE) )
        {
            lifecycle_status = fbe_provision_drive_run_verify(object_p, packet_p);
            return lifecycle_status;
        }

        /* If passive or we need to redirect we are done.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Nothing needs to run, be sure to set idle.
     */
    status = fbe_provision_drive_set_upstream_edge_priority(provision_drive_p, FBE_MEDIC_ACTION_IDLE);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s idle set upstream priority failed, status: 0x%x\n",
                              __FUNCTION__, status);
    }

    /* No background request needs to run, we are done.
     */
    return FBE_LIFECYCLE_STATUS_DONE;

}
/******************************************************************************
 * end fbe_provision_drive_background_monitor_operation_cond_function()
 ******************************************************************************/


/*!****************************************************************************
 * provision_drive_passive_request_join_cond_function()
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
 *  10/4/2011 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
provision_drive_passive_request_join_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_provision_drive_t *  provision_drive_p = NULL;
    fbe_bool_t          b_is_active;
    fbe_system_time_t   time_stamp;
    fbe_scheduler_hook_status_t hook_status;

    provision_drive_p = (fbe_provision_drive_t*)object_p;

    fbe_provision_drive_check_hook(provision_drive_p,
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_PASSIVE_REQUEST,
                                   FBE_PROVISION_DRIVE_SUBSTATE_JOIN_STARTED,
                                   0,
                                   &hook_status);

    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    /* This condition is only used by the passive side to join.
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);
    if (b_is_active)
    {
        /* If the peer hasn't clear joined flag yet, we need to wait.
         */
        if (fbe_base_config_has_peer_joined((fbe_base_config_t*)provision_drive_p))
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Active wait peer clear joined\n");
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* We move the taks for clearing remove_timestamp here 
         * At this point, we won't miss the SLF case (AR560481)
         */
        if (!fbe_private_space_layout_object_id_is_system_pvd(provision_drive_p->base_config.base_object.object_id)) {
            fbe_zero_memory(&time_stamp, sizeof(fbe_system_time_t));
            fbe_provision_drive_metadata_get_time_stamp(provision_drive_p,&time_stamp);
            if (!fbe_provision_drive_system_time_is_zero(&time_stamp)) {
                fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s: PVD remove_timestamp is not valid, reset it\n", __FUNCTION__);
                fbe_transport_set_completion_function(packet_p, fbe_provision_drive_set_remove_timestamp_completion, provision_drive_p);
                status = fbe_provision_drive_metadata_set_time_stamp(provision_drive_p,packet_p,FBE_TRUE);
                return FBE_LIFECYCLE_STATUS_PENDING;
            }
        }

        /* We are active and we do not need to run this condition in activate.
         */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to clear condition %d\n", __FUNCTION__, status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Active clear passive join condition\n");
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_provision_drive_lock(provision_drive_p);
    if(fbe_cmi_is_peer_alive() &&
       fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN))
    {
        /* If we decided we are broken, then we need to set the broken condition now.
             */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive trying to join. Active broken. Go broken.local: 0x%llx peer: 0x%llx\n",                               
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags);                              
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *)provision_drive_p, 
                               FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }    

    /* If the peer have not requested a join, it must be cleared for some reason.
     * The passive side should go back and start join from the beginning. 
     */
     if ((fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_STARTED) || 
          fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_DONE)) &&
         !fbe_base_config_is_any_peer_clustered_flag_set((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK))
     {
         fbe_base_config_clear_clustered_flag((fbe_base_config_t*)provision_drive_p, 
                                              FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK);
         fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_MASK);

         fbe_provision_drive_unlock(provision_drive_p);
         fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "Passive join peer REQ !set local: 0x%llx peer: 0x%llx/0x%x state:0x%llx\n", 
                               provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                               provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                               provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.lifecycle_state,
                               provision_drive_p->local_state);
         return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* The local state determines which function processes this condition.
     */
    if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_STARTED))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PASSIVE join started local state: 0x%llx\n",
                  (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return fbe_provision_drive_join_passive(provision_drive_p, packet_p);
    }
    else if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_DONE))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PASSIVE join done local state: 0x%llx\n",
                  (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return fbe_provision_drive_join_done(provision_drive_p, packet_p);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PASSIVE join request local state: 0x%llx\n",
                  (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return fbe_provision_drive_join_request(provision_drive_p, packet_p);
    }
}
/******************************************************************************
 * end provision_drive_passive_request_join_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * provision_drive_active_allow_join_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to verify the signature for the RAID group object.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when signature
 *                                    verification issued.
 *
 * @author
 *  10/6/2011 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
provision_drive_active_allow_join_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_provision_drive_t *  provision_drive_p = NULL;
    fbe_bool_t          b_is_active;

    provision_drive_p = (fbe_provision_drive_t*)object;

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Active join cond entry local: 0x%llx peer: 0x%llx state:0x%llx\n", 
                          (unsigned long long)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                          (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                          (unsigned long long)provision_drive_p->local_state);

    /* This condition only runs on the active side, so if we are passive, then clear it now.
     */
    if (!b_is_active)
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to clear condition %d\n", __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    fbe_provision_drive_lock(provision_drive_p);

    /* If the peer is not there or they have not requested a join, then clear this condition.
     * The active side only needs to try to sync when the peer is present. 
     */
    if (!fbe_cmi_is_peer_alive() ||
        !fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)provision_drive_p, 
                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ))
    {
        /* Only clear the clustered join in the case where the peer has gone away.
         * If the joined is set, the peer has successfully joined. 
         */
        if (!fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)provision_drive_p, 
                                                        FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED))
        {
            fbe_base_config_clear_clustered_flag((fbe_base_config_t*)provision_drive_p, 
                                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK);
        }
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_MASK);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to clear condition %d\n", __FUNCTION__, status);
        }
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Active join peer !alive or REQ !set local: 0x%llx peer: 0x%llx/0x%x state:0x%llx\n", 
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                              provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.lifecycle_state,
                              (unsigned long long)provision_drive_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Use the local state to determine which function processes this condition.
     */
    if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_STARTED))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "ACTIVE join started objectid: %d local state: 0x%llx\n",
                  object->object_id,
                  (unsigned long long)provision_drive_p->local_state);

        fbe_provision_drive_unlock(provision_drive_p);
        return fbe_provision_drive_join_active(provision_drive_p, packet_p);
    }
    else if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_DONE))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "ACTIVE join done objectid: %d local state: 0x%llx\n",
                  object->object_id,
                  (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return fbe_provision_drive_join_done(provision_drive_p, packet_p);
    }
    else
    {

        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "ACTIVE join request objectid: %d local state: 0x%llx\n",
                  object->object_id,
                  (unsigned long long)provision_drive_p->local_state);
        return fbe_provision_drive_join_request(provision_drive_p, packet_p);
    }
}
/******************************************************************************
 * end provision_drive_active_allow_join_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * provision_drive_join_sync_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to synchornize the final part of the join process.
 * It will clear itself when peer finishes its join process (clearing JOIN_START),
 * or peer drops out the process (not in ready state)
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_DONE 
 *
 * @author
 *  2/22/2012 - Create. Naizhong
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
provision_drive_join_sync_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_provision_drive_t *  provision_drive_p = NULL;
    fbe_bool_t              b_is_active;
    fbe_lifecycle_state_t   local_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_lifecycle_state_t   peer_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_base_config_clustered_flags_t local_flags = 0, peer_flags = 0;

    provision_drive_p = (fbe_provision_drive_t*)object;
    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t *)provision_drive_p, &peer_state);
    fbe_base_config_metadata_get_lifecycle_state((fbe_base_config_t *)provision_drive_p, &local_state);
    fbe_base_config_get_clustered_flags((fbe_base_config_t *) provision_drive_p, &local_flags);
    fbe_base_config_get_peer_clustered_flags((fbe_base_config_t *) provision_drive_p, &peer_flags);

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "PVD sync join cond %s entry local: 0x%llx peer: 0x%llx state:0x%x pState:0x%x\n", 
                          b_is_active ? "active" : "passive",
                          local_flags, peer_flags,
                          local_state, peer_state);

    /* If peer does not exists, or peer drops out of the process, we can move on.
     */
    if ((b_is_active) && (peer_state != FBE_LIFECYCLE_STATE_READY))
    {
        fbe_base_config_clear_clustered_flag((fbe_base_config_t*)provision_drive_p, 
                                             FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK);
        fbe_provision_drive_clear_local_state(provision_drive_p, (FBE_PVD_LOCAL_STATE_JOIN_STARTED |
                                                                  FBE_PVD_LOCAL_STATE_JOIN_DONE));

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    return fbe_provision_drive_join_sync_complete(provision_drive_p, packet_p);
}
/******************************************************************************
 * end provision_drive_join_sync_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * provision_drive_wait_for_config_commit_function()
 ******************************************************************************
 * @brief
 * This function checks if path state is gone. If it is- detaches edge.
 * Then it calls fbe_base_config_wait_for_configuration_commit_function.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   
 *
 * @author
 *  23/1/2012 - Created. 
 *
 ******************************************************************************/
static fbe_lifecycle_status_t provision_drive_wait_for_config_commit_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_provision_drive_t *				provision_drive_p = NULL;
    fbe_block_edge_t *					edge_p = NULL;
    fbe_path_state_t					path_state;
    fbe_status_t						status;
    fbe_lifecycle_status_t				lifecycle_status;

    provision_drive_p = (fbe_provision_drive_t*)object_p;

    
    fbe_base_config_get_block_edge( (fbe_base_config_t *) provision_drive_p, &edge_p, 0);
    fbe_block_transport_get_path_state(edge_p, &path_state);
    if(path_state == FBE_PATH_STATE_GONE)
    {
        status = fbe_base_config_detach_edge((fbe_base_config_t *) provision_drive_p, packet_p, edge_p);
        if (status != FBE_STATUS_OK)
        fbe_base_object_trace(object_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s detach edge returned with status %d\n", __FUNCTION__, status);
    }

    lifecycle_status = fbe_base_config_wait_for_configuration_commit_function(object_p, packet_p);

    return lifecycle_status;
}
/*!****************************************************************************
 * fbe_provision_drive_send_destroy_request() 
 ******************************************************************************
 * @brief
 *  This function send destroy request to job service to destroy a pvd.
 *
 * @param  provision_drive_p - The base config object.
 *         pvd_destroy_request - pvd destroy request.
 * 
 * @return status - The status of the operation.
 *
 * @author
 *  01/09/2012 - Created. zhangy
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_send_destroy_request(fbe_provision_drive_t *provision_drive_p,
                                     fbe_job_service_destroy_provision_drive_t * pvd_destroy_request) 
{   
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_packet_t *                      packet_p = NULL;
    fbe_payload_ex_t *                     payload_p = NULL;
    fbe_payload_control_operation_t *   control_p = NULL;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    packet_p = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(packet_p);

    /* Allocate control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_JOB_CONTROL_CODE_DESTROY_PROVISION_DRIVE,
                                        pvd_destroy_request,
                                        sizeof(fbe_job_service_destroy_provision_drive_t));

    /* We are sending control packet, so we have to form address manually. */
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_JOB_SERVICE,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID);

    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(packet_p);

    fbe_transport_wait_completion(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed status %X\n", 
                                __FUNCTION__, status);
        fbe_payload_ex_release_control_operation(payload_p, control_p);
        fbe_transport_release_packet(packet_p);
        return status;
    }

    /* Free the resources we allocated previously.*/
    fbe_payload_ex_release_control_operation(payload_p, control_p);
    fbe_transport_release_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_send_destroy_request()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_run_metadata_verify_invalidate()
 ******************************************************************************
 * @brief
 *   This functions runs metadata verify and invalidate operation. It does second level sanity 
 *   check and if the operation is not completed, it sends either permission request 
 *   to upstream edge or directly sends verify invalidate request to executor.
 *
 *  @param object_p                 - pointer to a base object 
 *  @param packet_p                 - pointer to a control packet_p from the scheduler
 *
 *  @return fbe_lifecycle_status_t  - FBE_LIFECYCLE_STATUS_RESCHEDULE on success/
 *                                    condition was cleared
 *                                  - FBE_LIFECYCLE_STATUS_PENDING when I/O is
 *                                    being issued 
 * @author
 *   03/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_lifecycle_status_t
fbe_provision_drive_run_metadata_verify_invalidate(fbe_base_object_t * object_p, 
                                                   fbe_packet_t * packet_p)
{

    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_provision_drive_t *     provision_drive_p = NULL;    
    fbe_lba_t                   metadata_verify_checkpoint = FBE_LBA_INVALID;
    fbe_lba_t                   exported_capacity = FBE_LBA_INVALID;
    fbe_lba_t                   default_offset = FBE_LBA_INVALID;
    fbe_object_id_t             object_id;
    fbe_scheduler_hook_status_t hook_status;
    fbe_bool_t                  b_can_verify_invalidate_start;
    fbe_bool_t                  is_event_outstanding;

    /*  Cast the base object to a provision drive object.
     */     
    provision_drive_p = (fbe_provision_drive_t*) object_p;

    /* get pvd object id.
     */
    fbe_base_object_get_object_id((fbe_base_object_t *) provision_drive_p, &object_id);

    /* Get the metadata verify invalidate checkpoint. 
     */
    status = fbe_provision_drive_metadata_get_verify_invalidate_checkpoint (provision_drive_p, &metadata_verify_checkpoint);
    if (status != FBE_STATUS_OK) 
    {
        fbe_provision_drive_utils_trace( provision_drive_p,
                     FBE_TRACE_LEVEL_WARNING,
                     FBE_TRACE_MESSAGE_ID_INFO, 
                     FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                     "pvd_run_verify_invalidate: Failed to get checkpoint. Status: 0x%X\n", status);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_provision_drive_check_hook(provision_drive_p,  
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VERIFY_INVALIDATE, 
                                   FBE_PROVISION_DRIVE_SUBSTATE_VERIFY_INVALIDATE_CHECKPOINT, 
                                   metadata_verify_checkpoint, 
                                   &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE){return FBE_LIFECYCLE_STATUS_DONE;}

    fbe_provision_drive_get_event_outstanding_flag(provision_drive_p, &is_event_outstanding);
    if(is_event_outstanding) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Check the downstream status and load before starting. */
    fbe_provision_drive_utils_can_verify_invalidate_start(provision_drive_p, &b_can_verify_invalidate_start);
    if(!b_can_verify_invalidate_start) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    /* get the exported capacity of the provision drive object. */
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &exported_capacity);

    /* Set the completion function for the metadata verify invalidate condition. 
     */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_run_metadata_verify_invalidate_completion,
                                          provision_drive_p);
        
    if(metadata_verify_checkpoint >= exported_capacity)
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO, 
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                    "pvd_run_verify_invalidate: Complete. Chk:0x%llx,exp:0x%llx\n",
                                    metadata_verify_checkpoint,exported_capacity);

        status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, 
                                                 fbe_provision_drive_reset_verify_invalidate_checkpoint_with_np_lock);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* Get the default offset.
     */
    fbe_base_config_get_default_offset((fbe_base_config_t *) provision_drive_p, &default_offset);

    if (metadata_verify_checkpoint < default_offset)
    {
        /* For systemt PVDs, we need to ask permission first as this region might not be consumed by MCR controlled
         * region
         */
        if(fbe_database_is_object_system_pvd(object_id) == FBE_FALSE)
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "pvd_run_verify_invalidate: verify_invalidate checkpoint: 0x%llx below default:0x%llx\n",
                                  (unsigned long long)metadata_verify_checkpoint, (unsigned long long)default_offset);

            /* For non-system PVD just start from the default offset and so increment the checkpoint to default offset */
            fbe_provision_drive_metadata_incr_verify_invalidate_checkpoint(provision_drive_p, 
                                                                           packet_p, 
                                                                           metadata_verify_checkpoint,
                                                                           default_offset - metadata_verify_checkpoint);
        }
        else
        {
             /* If we are here we need to ask permission.  The completion will process
              * the permission result.
              */
            fbe_provision_drive_utils_ask_verify_invalidate_permission(provision_drive_p,
                                                                      metadata_verify_checkpoint,
                                                                      FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                                      packet_p);
        }
    }
    else
    {
        /* We are above the default_offset. For now we dont get permission from upstream object 
         * because need to complete the operation asap and so Issue the invalidate request.
         */
        status = fbe_provision_drive_send_monitor_packet(provision_drive_p,
                                                         packet_p,
                                                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE,
                                                         metadata_verify_checkpoint,
                                                         FBE_PROVISION_DRIVE_CHUNK_SIZE);
    }

    /* return pending status since we are waiting for the permission.*/
    return FBE_LIFECYCLE_STATUS_PENDING;
} 
/******************************************************************************
 * end fbe_provision_drive_run_metadata_verify_invalidate()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_run_metadata_verify_invalidate_completion()
 ******************************************************************************
 *
 * @brief
 *    This is the completion function of verify invalidate operation. It is invoked when a 
 *    i/o operation has completed.  It reschedules the  monitor to
 *    run again in order to perform the next i/o operation.
 *
 *    Note: This function must always return FBE_STATUS_OK so that the  packet
 *          gets completed to the next level. If any other status is returned,
 *          the packet_p will get stuck.
 *
 * @param   in_packet_p   -  pointer to control packet from the scheduler
 * @param   in_context    -  context, which is a pointer to the base object
 * 
 * @return  fbe_status_t  -  always FBE_STATUS_OK
 *
 * @version
 *    03/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/

static fbe_status_t fbe_provision_drive_run_metadata_verify_invalidate_completion(fbe_packet_t * packet_p,
                                                                                  fbe_packet_completion_context_t context)
{

    fbe_provision_drive_t*          provision_drive_p;  // pointer to provision drive
    fbe_status_t                    status;             // fbe status

    /* cast the context to provision drive object */
    provision_drive_p = (fbe_provision_drive_t *) context;

    status = fbe_transport_get_status_code(packet_p);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s i/o failed, status: 0x%x\n",
                              __FUNCTION__, status);
    }

    fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                             (fbe_base_object_t *) provision_drive_p,
                             (fbe_lifecycle_timer_msec_t) 100);

    /* return success in all cases so the packet is completed to the next level */
    return FBE_STATUS_OK;
}
/*********************************************************************
 * end fbe_provision_drive_run_metadata_verify_invalidate_completion()
 *********************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_set_invalidate_checkpoint_clear_condition_completion()
 ******************************************************************************
 *
 * @brief
 *    This is the completion function of verify invalidatate checkpoint update
 * function which will clear the condition. 
 *
 * @param   in_packet_p   -  pointer to control packet from the scheduler
 * @param   in_context    -  context, which is a pointer to the base object
 * 
 * @return  fbe_status_t  -  always FBE_STATUS_OK
 *
 * @version
 *    03/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/

static fbe_status_t fbe_provision_drive_set_invalidate_checkpoint_clear_condition_completion(fbe_packet_t * packet_p,
                                                                                             fbe_packet_completion_context_t context)
{

    fbe_provision_drive_t*          provision_drive_p;  // pointer to provision drive
    fbe_status_t                    status;             // fbe status

    /* cast the context to provision drive object */
    provision_drive_p = (fbe_provision_drive_t *) context;

    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                 FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s\n", __FUNCTION__);
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
    if (status != FBE_STATUS_OK)
    {
          fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                 FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s failed to clear condition %d\n", __FUNCTION__, status);
    }

    /* return success in all cases so the packet is completed to the next level */
    return FBE_LIFECYCLE_STATUS_DONE;
}
/********************************************************************************
 * end fbe_provision_drive_set_invalidate_checkpoint_clear_condition_completion()
 ********************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_clear_end_of_life_cond_function_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for persisting the eol bit
 *  in nonpaged metadata
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  08/20/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_clear_end_of_life_cond_function_completion(fbe_packet_t * packet_p,
                                                             fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_provision_drive_t              *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_bool_t                          b_end_of_life_state = FBE_TRUE;
    fbe_block_edge_t                   *edge_p = NULL;
    fbe_path_state_t                    path_state;
    fbe_path_attr_t                     path_attr;
    fbe_bool_t                          b_broken = FBE_TRUE;
    fbe_provision_drive_local_state_t   local_state;
    fbe_u32_t                           number_of_clients = 0;
	fbe_object_id_t                     object_id = FBE_OBJECT_ID_INVALID;

    /* Check the path state of the edge */
    fbe_base_config_get_block_edge( (fbe_base_config_t *) provision_drive_p, &edge_p, 0);
    fbe_block_transport_get_path_state(edge_p, &path_state);
    fbe_block_transport_get_path_attributes(edge_p, &path_attr);
    fbe_provision_drive_get_local_state(provision_drive_p, &local_state);

    fbe_provision_drive_lock(provision_drive_p);
    b_broken = fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)provision_drive_p, 
                                                     FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN);
    fbe_provision_drive_unlock(provision_drive_p);

    /* get the curent zero checkpoint, if it is already set to LBA invalid then clear the condition. */
    fbe_provision_drive_metadata_get_end_of_life_state(provision_drive_p, &b_end_of_life_state);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s path state: %d attr: 0x%x metadata eol: %d broken: %d eol local state: 0x%llx \n",
                          __FUNCTION__, path_state, path_attr, b_end_of_life_state, b_broken,
                          (unsigned long long)local_state);

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* clear the the set end of life condition if status is ok. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* Clear the broken flag */
        fbe_provision_drive_lock(provision_drive_p);
        fbe_base_config_clear_clustered_flag((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN);
        /* Clear the clear_end_of_life flag */
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_CLEAR_END_OF_LIFE);
        fbe_provision_drive_unlock(provision_drive_p);

        /* clear the set end of life bit condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                    FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X", __FUNCTION__, status);
            return status;
        }

        /* If there are any upstream objects, clear the upstream EOL.
         */
        fbe_block_transport_server_get_number_of_clients(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                         &number_of_clients);
        if (number_of_clients > 0)
        {
            /* Clear EOL on all upstream edges and generate the event.
             */
            fbe_block_transport_server_clear_path_attr_all_servers(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                                   FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);
        }

        /* Set the flag to indicate that we are done clearing EOL */
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_CLEAR_EOL_STARTED);
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_CLEAR_EOL_COMPLETE);
		/* log an event for system drives. */
        fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
        if (fbe_database_is_object_system_pvd(object_id))
        {
            fbe_provision_drive_write_event_log(provision_drive_p, SEP_INFO_SYSTEM_DISK_END_OF_LIFE_CLEAR);
        }
        else /* log an event for user drives. */
        {
            fbe_provision_drive_write_event_log(provision_drive_p, SEP_INFO_USER_DISK_END_OF_LIFE_CLEAR);
        }
    }
    else
    {   
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s end of life state persist failed, payload_status:%d, packet_status:%d\n",
                              __FUNCTION__, control_status, status);
    }

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_clear_end_of_life_cond_function_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_clear_end_of_life()
 ******************************************************************************
 * @brief
 *  This function is used to clear the end of life in the NP and persist it
 *  as logical disk end.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  10/18/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_clear_end_of_life(fbe_packet_t * packet_p,
                                    fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;    
    fbe_status_t                        status;
    
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return, we are already in the completion routine
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }
    

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);
    fbe_provision_drive_metadata_set_end_of_life_state(provision_drive_p, packet_p, FBE_FALSE);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_clear_end_of_life()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_set_end_of_life_cond_function()
 ******************************************************************************
 * @brief
 *  This condition function is used to set the end of life attributes in non
 *  paged metadata for this drive.
 *
 * @param object_p                  - pointer to a base object.
 * @param packet_p                  - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - lifecycle status.
 *
 * @author
 *  08/20/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_clear_end_of_life_cond_function(fbe_base_object_t * object_p,
                                                  fbe_packet_t * packet_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_provision_drive_t  *provision_drive_p = (fbe_provision_drive_t *)object_p;
    fbe_bool_t              b_end_of_life_state = FBE_TRUE;
    fbe_block_edge_t       *edge_p = NULL;
    fbe_path_state_t        path_state;
    fbe_path_attr_t         path_attr;
    fbe_bool_t              b_broken = FBE_TRUE;
    fbe_provision_drive_local_state_t local_state;
    fbe_base_config_downstream_health_state_t downstream_health_state;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t*       control_operation_p = NULL;

    /* Validate the downstream health */
    downstream_health_state = fbe_provision_drive_verify_downstream_health(provision_drive_p);

    /* Check the path state of the edge */
    fbe_base_config_get_block_edge( (fbe_base_config_t *) provision_drive_p, &edge_p, 0);
    fbe_block_transport_get_path_state(edge_p, &path_state);
    fbe_block_transport_get_path_attributes(edge_p, &path_attr);
    fbe_provision_drive_get_local_state(provision_drive_p, &local_state);

    fbe_provision_drive_lock(provision_drive_p);
    b_broken = fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)provision_drive_p, 
                                                     FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN);
    fbe_provision_drive_unlock(provision_drive_p);

    /* get the curent zero checkpoint, if it is already set to LBA invalid then clear the condition. */
    fbe_provision_drive_metadata_get_end_of_life_state(provision_drive_p, &b_end_of_life_state);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "pvd: clear EOL health: %d path state: %d attr: 0x%x meta eol: %d broken: %d eol local state: 0x%llx \n",
                          downstream_health_state, path_state, path_attr, b_end_of_life_state, b_broken,
                          (unsigned long long)local_state);

    /* Set the flag to indicate that we are clearing EOL */
    fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_CLEAR_EOL_REQUEST);
    fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_CLEAR_EOL_STARTED);

    /* Send usurper command to pdo to clear the attribute. */  
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE)
    {
        payload_p = fbe_transport_get_payload_ex(packet_p);
        control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
        fbe_payload_control_build_operation (control_operation_p,
                                             FBE_PHYSICAL_DRIVE_CONTROL_CODE_CLEAR_END_OF_LIFE,
                                             NULL,
                                             0);
        fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
        status = fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* If end of life and the broken are both cleared */
    if ((b_end_of_life_state == FBE_FALSE) &&
        (b_broken == FBE_FALSE)               )
    {
        /* Clear all the clear EOL related bits.*/
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_CLEAR_EOL_MASK);
        /* Clear the clear_end_of_life flag */
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_CLEAR_END_OF_LIFE);

        /* Clear the condition */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);

        /* If the downstream health is broken goto the failed state. */
        if (downstream_health_state == FBE_DOWNSTREAM_HEALTH_BROKEN)
        {
            fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                   (fbe_base_object_t*)provision_drive_p,
                                   FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Update the nonpaged metadata with checkpoint as invalid. */  
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_clear_end_of_life_cond_function_completion, provision_drive_p);
    status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_clear_end_of_life);   
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_provision_drive_clear_end_of_life_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * provision_drive_zero_started_cond_function()
 ******************************************************************************
 *
 * @brief
 *    Running to let the upper layers know even on the passive side that a zero operation started
 * 
 * @return  fbe_status_t  -  always FBE_LIFECYCLE_STATUS_DONE
 *
 *
 ******************************************************************************/
static fbe_lifecycle_status_t provision_drive_zero_started_cond_function( fbe_base_object_t* in_object_p, fbe_packet_t*      in_packet_p  )
{

    fbe_notification_info_t                 notify_info;
    fbe_provision_drive_t *                 provision_drive_p = (fbe_provision_drive_t *)in_object_p; 
    fbe_status_t                            status;
    fbe_object_id_t                         object_id;

    notify_info.notification_type = FBE_NOTIFICATION_TYPE_ZEROING;
    notify_info.class_id = FBE_CLASS_ID_PROVISION_DRIVE;
    notify_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE;
    notify_info.notification_data.object_zero_notification.zero_operation_status = FBE_OBJECT_ZERO_STARTED;

    fbe_base_object_get_object_id(in_object_p, &object_id);

    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s zero start \n", __FUNCTION__);

    status = fbe_notification_send(object_id, notify_info);
    if (status != FBE_STATUS_OK) { 
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s unexpected status: %d on notification send \n", __FUNCTION__, status);
    }

    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)provision_drive_p);
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to clear condition %d\n", __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*********************************************************************
 * end provision_drive_zero_started_cond_function()
 *********************************************************************/

/*!****************************************************************************
 * provision_drive_zero_ended_cond_function()
 ******************************************************************************
 *
 * @brief
 *    Running to let the upper layers know even on the passive side that a zero operation ended
 * 
 * @return  fbe_status_t  -  always FBE_LIFECYCLE_STATUS_DONE
 *
 *
 ******************************************************************************/
static fbe_lifecycle_status_t provision_drive_zero_ended_cond_function( fbe_base_object_t* in_object_p, fbe_packet_t*      in_packet_p  )
{

    fbe_notification_info_t                 notify_info;
    fbe_provision_drive_t *                 provision_drive_p = (fbe_provision_drive_t *)in_object_p; 
    fbe_status_t                            status;
    fbe_object_id_t                         object_id;

    notify_info.notification_type = FBE_NOTIFICATION_TYPE_ZEROING;
    notify_info.class_id = FBE_CLASS_ID_PROVISION_DRIVE;
    notify_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE;
    notify_info.notification_data.object_zero_notification.zero_operation_status = FBE_OBJECT_ZERO_ENDED;

    if (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SCRUB_ENDED)) {
        notify_info.notification_data.object_zero_notification.scrub_complete_to_be_set = FBE_TRUE;
    } else {
        notify_info.notification_data.object_zero_notification.scrub_complete_to_be_set = FBE_FALSE;
    }

    fbe_base_object_get_object_id(in_object_p, &object_id);

    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s zero end \n", __FUNCTION__);


    status = fbe_notification_send(object_id, notify_info);
    if (status != FBE_STATUS_OK) { 
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s unexpected status: %d on notification send \n", __FUNCTION__, status);
    }

    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)provision_drive_p);
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to clear condition %d\n", __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;


}

/*********************************************************************
 * end provision_drive_zero_started_or_ended_cond_function()
 *********************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_eval_slf_cond_function()
 ******************************************************************************
 * @brief
 *  This condition function is used to evaluate single loop failure situation.
 *
 * @param object_p              - pointer to a base object.
 * @param packet_p              - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t 
 *
 * @author
 *  04/30/2012 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_eval_slf_cond_function(fbe_base_object_t * object_p,
                                           fbe_packet_t * packet_p)
{
    fbe_provision_drive_t               *provision_drive_p = (fbe_provision_drive_t *)object_p;

    if (!fbe_provision_drive_is_slf_enabled())
    {
        fbe_lifecycle_clear_current_cond(object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_provision_drive_lock(provision_drive_p);

    /* The local state determines which function processes this condition.
     */
    if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_EVAL_SLF_STARTED))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        /* fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s EVAL SLF started local state: 0x%llx\n", __FUNCTION__, provision_drive_p->local_state);*/
        if (fbe_base_config_is_active((fbe_base_config_t *)provision_drive_p))
        {
            return fbe_provision_drive_eval_slf_started_active(provision_drive_p, packet_p);
        }
        else
        {
            return fbe_provision_drive_eval_slf_started_passive(provision_drive_p, packet_p);
        }
    }
    else if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_EVAL_SLF_DONE))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        /*fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s EVAL SLF done local state: 0x%llx\n", __FUNCTION__, provision_drive_p->local_state);*/
        return fbe_provision_drive_eval_slf_done(provision_drive_p, packet_p);
    }
    else
    {
        fbe_provision_drive_unlock(provision_drive_p);
        /*fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s EVAL SLF request local state: 0x%llx\n", __FUNCTION__, provision_drive_p->local_state);*/
        return fbe_provision_drive_eval_slf_request(provision_drive_p, packet_p);
    }

}
/*********************************************************************
 * end fbe_provision_drive_eval_slf_cond_function()
 *********************************************************************/
/*!****************************************************************************
 * provision_drive_verify_invalidate_remap_request_cond
 ******************************************************************************
 *
 * @brief
 *    This condition function will just send the remap event to 
 *    upstream objects.
 *
 * @param   object_p  -  pointer to provision drive
 * @param   packet_p  -  pointer to control packet from the scheduler
 *
 * @return  fbe_lifecycle_status_t  
 *                                  
 *
 * @version
 *    03/02/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/

static fbe_lifecycle_status_t
fbe_provision_drive_verify_invalidate_remap_request_cond(fbe_base_object_t*  object_p,
                                                         fbe_packet_t*       packet_p
                                                         )
{
    fbe_provision_drive_t*                     provision_drive_p;  
    fbe_status_t                               status;             
    fbe_lba_t                                  verify_invalidate_lba;

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( object_p );

    // cast the base object to provision drive object
    provision_drive_p = (fbe_provision_drive_t *) object_p;

    // terminate current monitor cycle if not active
    if ( !(fbe_base_config_is_active((fbe_base_config_t *)object_p)) )
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    verify_invalidate_lba = fbe_provision_drive_get_verify_invalidate_ts_lba(provision_drive_p);

    // send event up
    status = fbe_provision_drive_ask_remap_action(provision_drive_p, 
                                                  packet_p,
                                                  verify_invalidate_lba,
                                                  FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                  fbe_provision_drive_verify_invalidate_remap_completion);

    fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: clearing the condition\n",
                       __FUNCTION__);
    fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);

    /* If the status is not ok, that means the called function completed the packet */
    if(status != FBE_STATUS_OK) 
    {
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;

}   // end fbe_provision_drive_send_remap_event_cond_function()


/*!****************************************************************************
 * fbe_provision_drive_monitor_unregister_keys_completion
 ******************************************************************************
 *
 * @brief
 *    This is the completion function for unregistering keys.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return  fbe_lifecycle_status_t  
 *                                  
 *
 * @version
 *    01/17/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_monitor_unregister_keys_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t                      status;
    fbe_provision_drive_t*            provision_drive_p = NULL;

    provision_drive_p = (fbe_provision_drive_t*)context;

    status = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                            FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s unregister failed.\n", __FUNCTION__);		
    }

    fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                             (fbe_base_object_t*)provision_drive_p,
                             (fbe_lifecycle_timer_msec_t)0);

    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_provision_drive_monitor_unregister_keys_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_monitor_unregister_keys_if_needed
 ******************************************************************************
 *
 * @brief
 *    This function checks if unregistering keys is needed.
 *
 * @param provision_drive_p      - Pointer to the provision drive.
 * @param packet_p               - Packet requesting operation.
 *
 * @return  fbe_lifecycle_status_t  
 *                                  
 *
 * @version
 *    01/17/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_monitor_unregister_keys_if_needed(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_provision_drive_config_type_t config_type;
    fbe_u32_t i;
    fbe_system_encryption_mode_t system_encryption_mode;
    fbe_object_id_t port_object_id;

    fbe_database_get_system_encryption_mode(&system_encryption_mode);
    if (system_encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)
    {
        return FBE_LIFECYCLE_STATUS_CONTINUE;
    }

    fbe_provision_drive_get_config_type(provision_drive_p, &config_type);
#if 0
    if (config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID) 
    {
        return FBE_LIFECYCLE_STATUS_CONTINUE;
    }
#endif

    
    if (config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID) 
    {
        fbe_provision_drive_lock(provision_drive_p);
        for (i = 0; i < FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX; i++)
        {
            if ((provision_drive_p->key_info_table[i].key_handle == NULL) &&
                ((provision_drive_p->key_info_table[i].mp_key_handle_1 != FBE_INVALID_KEY_HANDLE) || 
                 (provision_drive_p->key_info_table[i].mp_key_handle_2 != FBE_INVALID_KEY_HANDLE)))
            {
                fbe_provision_drive_unlock(provision_drive_p);
                fbe_transport_set_completion_function(packet_p, 
                                              fbe_provision_drive_monitor_unregister_keys_completion,
                                              provision_drive_p);
                status = fbe_provision_drive_port_unregister_keys(provision_drive_p, packet_p, i);
                return FBE_LIFECYCLE_STATUS_PENDING;
            }
        }
        fbe_provision_drive_unlock(provision_drive_p);
    }
    else /* config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID */ 
    {
        fbe_block_edge_t *      edge_p = NULL;
        fbe_path_state_t        path_state;

        /* Check the path state of the edge */
        fbe_base_config_get_block_edge( (fbe_base_config_t *) provision_drive_p, &edge_p, 0);
        fbe_block_transport_get_path_state(edge_p, &path_state);
        if (path_state == FBE_PATH_STATE_INVALID)
        {
            /* If there is no edge or edge not ready, we should wait until it is ready */
            return FBE_LIFECYCLE_STATUS_CONTINUE;
        }

        status = fbe_provision_drive_get_port_object_id(provision_drive_p, packet_p, &port_object_id);
        if (status != FBE_STATUS_OK)
        {
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        fbe_provision_drive_lock(provision_drive_p);
        for (i = 0; i < FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX; i++)
        {
            if ((provision_drive_p->key_info_table[i].key_handle != NULL) &&
                ((provision_drive_p->key_info_table[i].mp_key_handle_1 != FBE_INVALID_KEY_HANDLE) || 
                 (provision_drive_p->key_info_table[i].mp_key_handle_2 != FBE_INVALID_KEY_HANDLE)) &&
                (provision_drive_p->key_info_table[i].port_object_id != port_object_id))
            {
                fbe_provision_drive_unlock(provision_drive_p);
                fbe_transport_set_completion_function(packet_p, 
                                              fbe_provision_drive_monitor_unregister_keys_completion,
                                              provision_drive_p);
                status = fbe_provision_drive_port_unregister_keys(provision_drive_p, packet_p, i);
                return FBE_LIFECYCLE_STATUS_PENDING;
            }
        }
        fbe_provision_drive_unlock(provision_drive_p);
    }

    return FBE_LIFECYCLE_STATUS_CONTINUE;
}
/******************************************************************************
 * end fbe_provision_drive_monitor_unregister_keys_if_needed()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_monitor_destroy_unregister_keys
 ******************************************************************************
 *
 * @brief
 *    This function unregisters keys in destroy state.
 *
 * @param provision_drive_p      - Pointer to the provision drive.
 * @param packet_p               - Packet requesting operation.
 *
 * @return  fbe_lifecycle_status_t  
 *                                  
 *
 * @version
 *    02/24/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_monitor_destroy_unregister_keys(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_u32_t i;

    for (i = 0; i < FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX; i++)
    {
        if ((provision_drive_p->key_info_table[i].mp_key_handle_1 != FBE_INVALID_KEY_HANDLE) || 
            (provision_drive_p->key_info_table[i].mp_key_handle_2 != FBE_INVALID_KEY_HANDLE))
        {
            fbe_transport_set_completion_function(packet_p, 
                                          fbe_provision_drive_monitor_unregister_keys_completion,
                                          provision_drive_p);
            status = fbe_provision_drive_port_unregister_keys(provision_drive_p, packet_p, i);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
    }

    fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_provision_drive_monitor_destroy_unregister_keys()
 ******************************************************************************/

/*!****************************************************************************
 * provision_drive_clear_np_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to clear the non paged of the pvd so that we can
 *  reinitialize the system PVD's. This condition should only be called
 *  when a system drive is pulled out and a new drive is inserted into that slot
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_DONE 
 *
 * @author
 *  06/06/2012 - Create. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
provision_drive_clear_np_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_provision_drive_t *  provision_drive_p = NULL;
    fbe_provision_drive_nonpaged_metadata_t provision_drive_nonpaged_metadata;
    //fbe_provision_drive_nonpaged_metadata_drive_info_t  drive_info;
    fbe_provision_drive_nonpaged_metadata_t * provision_drive_nonpaged_metadata_ptr;
    
    provision_drive_p = (fbe_provision_drive_t*)object;

    fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DRIVE_FAULT);
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_clear_nonpaged_completion, provision_drive_p);
    /* If it is a passive side, do not wipe out the nonpaged, just destroy the
       metadata element. The active side will clear the NP and it will be mirrored.
       */
    if ( !(fbe_base_config_is_active((fbe_base_config_t *)provision_drive_p)) )
    {
        provision_drive_nonpaged_metadata.base_config_nonpaged_metadata.generation_number = 0;
        provision_drive_nonpaged_metadata.base_config_nonpaged_metadata.nonpaged_metadata_state = FBE_NONPAGED_METADATA_UNINITIALIZED;

        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s passive side: destroy the metadata element.\n", __FUNCTION__);		

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_LIFECYCLE_STATUS_PENDING;
    }


    /* Copy original NP info on the stack */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)provision_drive_p, (void **)&provision_drive_nonpaged_metadata_ptr);
    fbe_copy_memory(&provision_drive_nonpaged_metadata, provision_drive_nonpaged_metadata_ptr, sizeof(fbe_provision_drive_nonpaged_metadata_t));

    //fbe_base_config_nonpaged_metadata_t base_config_nonpaged_metadata;  /* MUST be first */
    //fbe_sep_version_header_t    version_header; /* do not touch */
    provision_drive_nonpaged_metadata.base_config_nonpaged_metadata.generation_number = 0;
    //fbe_object_id_t         object_id;
    provision_drive_nonpaged_metadata.base_config_nonpaged_metadata.nonpaged_metadata_state = FBE_NONPAGED_METADATA_UNINITIALIZED;
    //fbe_base_config_background_operation_t operation_bitmask;


    provision_drive_nonpaged_metadata.sniff_verify_checkpoint = 0;
    provision_drive_nonpaged_metadata.zero_checkpoint = 0;
    provision_drive_nonpaged_metadata.zero_on_demand = FBE_TRUE;
    provision_drive_nonpaged_metadata.end_of_life_state = FBE_FALSE;
    provision_drive_nonpaged_metadata.drive_fault_state = FBE_FALSE;

    // fbe_provision_drive_nonpaged_metadata_drive_info_t nonpaged_drive_info; /* We will preserve it for now */

    provision_drive_nonpaged_metadata.media_error_lba = FBE_LBA_INVALID;
    provision_drive_nonpaged_metadata.verify_invalidate_checkpoint = FBE_LBA_INVALID;
    fbe_zero_memory(&provision_drive_nonpaged_metadata.remove_timestamp,sizeof(fbe_system_time_t));
    provision_drive_nonpaged_metadata.validate_area_bitmap = 0;

#if 0
    fbe_provision_drive_metadata_get_nonpaged_metadata_drive_info(provision_drive_p, &drive_info);    

    fbe_zero_memory(&provision_drive_nonpaged_metadata, sizeof(fbe_provision_drive_nonpaged_metadata_t));

    /*nonpaged drive info should not be wiped out as it will still be used as the original drive's info
     *when applying sparing policy next time*/
    fbe_copy_memory(&provision_drive_nonpaged_metadata.nonpaged_drive_info, &drive_info, sizeof(fbe_provision_drive_nonpaged_metadata_drive_info_t)); 

#endif

    status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                    (fbe_base_object_t*)provision_drive_p,
                                    FBE_PROVISION_DRIVE_LIFECYCLE_COND_UPDATE_PHYSICAL_DRIVE_INFO);


    /* Send the nonpaged metadata write request to metadata service. */
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                     packet_p,
                                                     0,
                                                     (fbe_u8_t *)&provision_drive_nonpaged_metadata, /* non paged metadata memory. */
                                                     sizeof(fbe_provision_drive_nonpaged_metadata_t)); /* size of the non paged metadata. */

    

    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end provision_drive_clear_np_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_clear_nonpaged_completion()
 ******************************************************************************
 * @brief
 *  This function will will send a metadata operation to destroy the metadata
 *  element since we will be reinitiazing it when we go through specialize
 * 
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/10/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_clear_nonpaged_completion(fbe_packet_t* packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t*     provision_drive_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_ex_t                       *sep_payload_p = NULL;
    fbe_payload_metadata_operation_t*       metadata_operation_p = NULL;
    fbe_payload_control_operation_t*       control_operation_p = NULL;
    fbe_status_t                           packet_status;
    fbe_status_t                           status;
    fbe_lifecycle_state_t                  lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_packet_t*                          reinit_packet_p;

    provision_drive_p = (fbe_provision_drive_t*)context;

    packet_status = fbe_transport_get_status_code(packet_p);
    if(packet_status == FBE_STATUS_QUIESCED)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                    FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s clear nonpaged failed with quiesced status:0x%x\n", __FUNCTION__,packet_status);
        return FBE_STATUS_OK;
    }

    if(packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_find_from_usurper_queue((fbe_base_object_t*)provision_drive_p, &reinit_packet_p);
        if(reinit_packet_p != NULL) 
        {
            status = fbe_base_object_remove_from_usurper_queue((fbe_base_object_t*)provision_drive_p, reinit_packet_p);
            if(status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s Couldn't remove the packet from usurper queue. status:0x%x\n", __FUNCTION__,status);
                return FBE_STATUS_OK;
            }
            fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);

            payload_p = fbe_transport_get_payload_ex(reinit_packet_p);
            control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(reinit_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(reinit_packet_p);
            return FBE_STATUS_OK;
        }
        return FBE_STATUS_OK;
    }

    /* Uncommit the configuration since the reinitialization of pvd will send a
       commit once it is persisted .
     */
    fbe_base_config_unset_configuration_committed((fbe_base_config_t*)provision_drive_p);
    status = fbe_base_object_get_lifecycle_state((fbe_base_object_t*)provision_drive_p, &lifecycle_state);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s Couldn't get lifecycle state. status:0x%x\n", __FUNCTION__,status);

        fbe_base_object_find_from_usurper_queue((fbe_base_object_t*)provision_drive_p, &reinit_packet_p);
        if(reinit_packet_p != NULL) 
        {
            status = fbe_base_object_remove_from_usurper_queue((fbe_base_object_t*)provision_drive_p, reinit_packet_p);
            if(status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s Couldn't remove the packet from usurper queue. status:0x%x\n", __FUNCTION__,status);
                return FBE_STATUS_OK;
            }
        }

        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);

        payload_p = fbe_transport_get_payload_ex(reinit_packet_p);
        control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(reinit_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(reinit_packet_p);
        return FBE_STATUS_OK;
    }

    /* We could be not committed in the use case where a system drive was pulled out.
       reboot the SP. Insert a new drive. we will reinit the PVD and we want to execute all the
       conditions in specialize rotary.
      */
    if(lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
    {
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_WAIT_FOR_CONFIG_COMMIT);
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_MEMORY_INIT);
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_ELEMENT_INIT);
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_NONPAGED_METADATA_INIT);
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_STRIPE_LOCK_START);
    }

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p = fbe_payload_ex_allocate_metadata_operation(sep_payload_p);
    
    fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s Destroying the metadata element\n", __FUNCTION__);

    fbe_payload_metadata_build_reinit_memory(metadata_operation_p, &((fbe_base_config_t*)provision_drive_p)->metadata_element);

    fbe_transport_set_completion_function(packet_p, fbe_pvd_metadata_reinit_memory_completion, provision_drive_p);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    status = fbe_metadata_operation_entry(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;


}// End fbe_provision_drive_clear_nonpaged_completion

/*!****************************************************************************
 * fbe_pvd_metadata_reinit_memory_completion()
 ******************************************************************************
 * @brief
 *  This function will clear the NP as part of reinitialization of system PVD
 *  when a new drive gets inserted in the system slot
 * 
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/10/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t 
fbe_pvd_metadata_reinit_memory_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_packet_t*               reinit_packet_p = NULL;
    fbe_payload_ex_t*           reinit_payload_p = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_metadata_status_t metadata_status;
    fbe_status_t                    status;
    fbe_provision_drive_t*          provision_drive_p = NULL;

    provision_drive_p = (fbe_provision_drive_t*)context;
    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation = fbe_payload_ex_get_metadata_operation(sep_payload);
    
    fbe_payload_metadata_get_status(metadata_operation, &metadata_status);

    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

    fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s metadata status: 0x%x\n", __FUNCTION__, metadata_status);

    fbe_base_object_find_from_usurper_queue((fbe_base_object_t*)provision_drive_p, &reinit_packet_p);
    if(reinit_packet_p == NULL) 
    {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: Reinit packet is not in usurper queue.\n", __FUNCTION__);
        return FBE_STATUS_OK;

    }
    status = fbe_base_object_remove_from_usurper_queue((fbe_base_object_t*)provision_drive_p, reinit_packet_p);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                    FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s Couldn't remove the packet from usurper queue. status:0x%x\n", __FUNCTION__,status);
        return FBE_STATUS_OK;
    }

    reinit_payload_p = fbe_transport_get_payload_ex(reinit_packet_p);
    control_operation = fbe_payload_ex_get_control_operation(reinit_payload_p);

    /* Clear the condition */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);

    /* Set control_status */
    //control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    if(metadata_status == FBE_PAYLOAD_METADATA_STATUS_OK){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    } else if (metadata_status == FBE_PAYLOAD_METADATA_STATUS_BUSY){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_BUSY);
    }else{
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    fbe_transport_set_status(reinit_packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(reinit_packet_p);

    return FBE_STATUS_OK;

}// End fbe_pvd_metadata_reinit_memory_completion

/*!****************************************************************************
 * fbe_provision_drive_notify_server_logically_online_cond()
 ******************************************************************************
 * @brief
 *  This function is used to notify PDO that drive has logically come online.
 *  PDO needs to know this so it can set edge attributes and guarantee they
 *  get propoated up to PVD.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_DONE 
 *
 * @author
 *  07/19/2012 :  Wayne Garrett - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_notify_server_logically_online_cond(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_p = NULL;
    fbe_block_transport_logical_state_t state;
    fbe_provision_drive_t *             provision_drive_p = (fbe_provision_drive_t *)object;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    
    
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)provision_drive_p);
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to clear condition %d\n", __FUNCTION__, status);
    }        
    
    /* There are cases where PVD will come Ready before it connects to LDO, such as 
       Single Loop Failure.   Verify edge is connected before sending notification.   If
       edge is not ready, simply clear this condition since this condition will be re-set 
       when edge becomes available.
     */
    if ( ((fbe_base_config_t *)provision_drive_p)->block_edge_p->base_edge.server_id != FBE_OBJECT_ID_INVALID)
    {   
        /* Allocate control operation. reusing monitor packet for control operation*/
        payload_p = fbe_transport_get_payload_ex(packet_p);        
        control_p = fbe_payload_ex_get_control_operation(payload_p);
    
        state = FBE_BLOCK_TRANSPORT_LOGICAL_STATE_ONLINE;
        fbe_payload_control_build_operation(control_p,
                                            FBE_BLOCK_TRANSPORT_CONTROL_CODE_LOGICAL_DRIVE_STATE_CHANGED,
                                            &state,
                                            sizeof(state));
           
        fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
    
        fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);

        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}


/*!****************************************************************************
 * fbe_provision_drive_set_drive_fault_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to set the drive fault bit of the pvd.
 *
 * @param base_object_p          - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  07/13/2012 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_set_drive_fault_cond_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_provision_drive_t *             provision_drive_p = (fbe_provision_drive_t *)base_object_p;
    fbe_bool_t                          drive_fault_state = FBE_FALSE;
    fbe_u32_t                           number_of_clients = 0;   

    fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s Entry\n", 
                           __FUNCTION__);

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

    /* get drive fault state. */
    fbe_provision_drive_metadata_get_drive_fault_state(provision_drive_p, &drive_fault_state);
    if((drive_fault_state != FBE_TRUE) && fbe_base_config_is_active((fbe_base_config_t *)provision_drive_p)
       && !fbe_provision_drive_metadata_need_reinitialize(provision_drive_p))
    {
        /* Update the nonpaged metadata. */
        fbe_transport_set_completion_function(packet_p, fbe_provision_drive_set_drive_fault_persist_completion, provision_drive_p);
        status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_set_drive_fault);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* Set the flag for usurper command to PDO */
    fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_CHECK_LOGICAL_STATE);
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);
    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                           (fbe_base_object_t*)provision_drive_p,
                           FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_provision_drive_set_drive_fault_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_set_drive_fault_persist_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for setting the drive fault bit of the pvd.
 *
 * @param packet_p               - pointer to a control packet.
 * @param context                - pointer to an object.
 * 
 * @return fbe_status_t
 *
 * @author
 *  07/13/2012 - Create. Lili Chen
 *  10/31/2012 - Update by adding event logs. Wenxuan Yin
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_set_drive_fault_persist_completion(fbe_packet_t * packet_p,
                                                       fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t                     object_id = FBE_OBJECT_ID_INVALID;

    provision_drive_p = (fbe_provision_drive_t*)context;

    /* get status. */
    status = fbe_transport_get_status_code (packet_p);
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* check the status. */
    if((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (status != FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s drive fault persist failed, control:%d, packet:%d\n",
                              __FUNCTION__, control_status, status);
        return FBE_STATUS_OK;
    }

    fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                             (fbe_base_object_t*)provision_drive_p,
                             (fbe_lifecycle_timer_msec_t)0);
    /* log an event for system drives. */
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    if (fbe_database_is_object_system_pvd(object_id))
    {
        fbe_provision_drive_write_event_log(provision_drive_p, SEP_WARN_SYSTEM_DISK_DRIVE_FAULT);
    }
    else /* log an event for user drives. */
    {
        fbe_provision_drive_write_event_log(provision_drive_p, SEP_WARN_USER_DISK_DRIVE_FAULT);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_set_drive_fault_persist_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_clear_drive_fault_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to clear the drive fault bit of the pvd.
 *
 * @param base_object_p          - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  07/13/2012 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_clear_drive_fault_cond_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p)
{
    fbe_provision_drive_t              *provision_drive_p = (fbe_provision_drive_t *)base_object_p;
    fbe_status_t                        status;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_bool_t                          drive_fault_state = FBE_FALSE;

    fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s Entry\n", 
                           __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_build_operation (control_operation_p,
                                         FBE_PHYSICAL_DRIVE_CONTROL_CODE_UPDATE_DRIVE_FAULT,
                                         &drive_fault_state,
                                         sizeof(drive_fault_state));
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_clear_drive_fault_inform_pdo_completion, provision_drive_p);
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
    status = fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_provision_drive_clear_drive_fault_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_clear_drive_fault_inform_pdo_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for informing PDO about clear_drive_fault.
 *
 * @param packet_p               - pointer to a control packet.
 * @param context                - pointer to an object.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/29/2012 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_clear_drive_fault_inform_pdo_completion(fbe_packet_t * packet_p,
                                                            fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t                          drive_fault_state = FBE_FALSE;
    fbe_u32_t                           number_of_clients = 0;
	fbe_object_id_t                     object_id = FBE_OBJECT_ID_INVALID;

    provision_drive_p = (fbe_provision_drive_t*)context;

    /* get status. */
    status = fbe_transport_get_status_code (packet_p);
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* check the status. */
    if((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (status != FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s inform PDO failed, control:%d, packet:%d\n",
                              __FUNCTION__, control_status, status);
    }

    /* Second, set nonpaged metadata. */
    fbe_provision_drive_metadata_get_drive_fault_state(provision_drive_p, &drive_fault_state);
    if(drive_fault_state == FBE_TRUE)
    {
        /* If there are any upstream objects, clear the upstream EOL.
         */
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Drive Fault state true. \n",__FUNCTION__);
        fbe_block_transport_server_get_number_of_clients(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                         &number_of_clients);
        if (number_of_clients > 0)
        {
            /* Clear Drive Fault on all upstream edges and generate the event.
             */
            fbe_block_transport_server_clear_path_attr_all_servers(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                                   FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT);
        }

        /* Update the nonpaged metadata. */
        if (fbe_base_config_is_active((fbe_base_config_t *)provision_drive_p))
        {
            fbe_transport_set_completion_function(packet_p, fbe_provision_drive_clear_drive_fault_persist_completion, provision_drive_p);
            status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_clear_drive_fault);   
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
    fbe_provision_drive_clear_clustered_flag(provision_drive_p, 
        FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DRIVE_FAULT | FBE_PROVISION_DRIVE_CLUSTERED_FLAG_CLEAR_DRIVE_FAULT);
    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                           (fbe_base_object_t*)provision_drive_p,
                           FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
	fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Drive Fault flag clear, log an event. \n",__FUNCTION__);
    /* log an event for system drives. */
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    if (fbe_database_is_object_system_pvd(object_id))
    {
        fbe_provision_drive_write_event_log(provision_drive_p, SEP_INFO_SYSTEM_DISK_DRIVE_FAULT_CLEAR);
    }
    else /* log an event for user drives. */
    {
        fbe_provision_drive_write_event_log(provision_drive_p, SEP_INFO_USER_DISK_DRIVE_FAULT_CLEAR);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_clear_drive_fault_inform_pdo_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_clear_drive_fault_persist_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for clearing the drive fault bit of the pvd.
 *
 * @param packet_p               - pointer to a control packet.
 * @param context                - pointer to an object.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/29/2012 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_clear_drive_fault_persist_completion(fbe_packet_t * packet_p,
                                                       fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
	fbe_object_id_t 					object_id = FBE_OBJECT_ID_INVALID;


    provision_drive_p = (fbe_provision_drive_t*)context;

    /* get status. */
    status = fbe_transport_get_status_code (packet_p);
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* check the status. */
    if((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (status != FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s drive fault persist failed, control:%d, packet:%d\n",
                              __FUNCTION__, control_status, status);
        return FBE_STATUS_OK;
    }

    /* We are done with clear_drive_fault np metadata persist */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
    fbe_provision_drive_clear_clustered_flag(provision_drive_p, 
        FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DRIVE_FAULT | FBE_PROVISION_DRIVE_CLUSTERED_FLAG_CLEAR_DRIVE_FAULT);
    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                           (fbe_base_object_t*)provision_drive_p,
                           FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
    /* log an event for system drives. */
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    if (fbe_database_is_object_system_pvd(object_id))
    {
        fbe_provision_drive_write_event_log(provision_drive_p, SEP_INFO_SYSTEM_DISK_DRIVE_FAULT_CLEAR);
    }
    else /* log an event for user drives. */
    {
        fbe_provision_drive_write_event_log(provision_drive_p, SEP_INFO_USER_DISK_DRIVE_FAULT_CLEAR);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_clear_drive_fault_persist_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_verify_drive_fault_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to verify the drive fault bit of the pvd.
 *
 * @param base_object_p          - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  07/13/2012 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_verify_drive_fault_cond_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_provision_drive_t  *provision_drive_p = (fbe_provision_drive_t *)base_object_p;
    fbe_bool_t              drive_fault_state = FBE_FALSE;

    /* get the drive fault bit. */
    fbe_provision_drive_metadata_get_drive_fault_state(provision_drive_p, &drive_fault_state);
    if(drive_fault_state == FBE_TRUE)
    {
        /* set the drive fault condition to change the state to fail. */
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_SET_DRIVE_FAULT);
    }

    /* If `EAS' NP flags is set go broken.
     */
    if (fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p,
                                                    FBE_PROVISION_DRIVE_NP_FLAG_EAS_STARTED)) {
        /* Goto the Failed rotary.
         */
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
    }

    /* clear the condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X", __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_provision_drive_verify_drive_fault_cond_function()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_health_check_passive_side_cond_function()
 ******************************************************************************
 * @brief
 *  This condition function is used to hold the non-originator side of the HC
 *  in the Activate state.  This prevents IOs being sent while PDO does the 
 *  HC.
 *
 * @param base_object_p          - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  07/13/2012 - Created.  Wayne Garrett
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_health_check_passive_side_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t*)object_p;
    fbe_provision_drive_local_state_t local_state = 0;
    fbe_lifecycle_status_t lifecycle_status;

    fbe_provision_drive_get_local_state(provision_drive_p, &local_state);

    fbe_base_object_trace(object_p, 
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "PVD_HC passive_side_cond_func. local state: 0x%llx\n", local_state);

    /* Are we in the Health Check state machine.
    */
    if (local_state & FBE_PVD_LOCAL_STATE_HEALTH_CHECK_MASK)
    {
        lifecycle_status = fbe_provision_drive_initiate_health_check(object_p, packet_p);

        switch(lifecycle_status)
        {
            case FBE_LIFECYCLE_STATUS_CONTINUE:  /*HC state machine is done*/
                break;

            case FBE_LIFECYCLE_STATUS_DONE: /* still in HC state machine */
                /*reschedule 0.5 seconds since HC doesn't take very long*/
                fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const, 
                                         (fbe_base_object_t*)provision_drive_p, (fbe_lifecycle_timer_msec_t)500);
                return lifecycle_status;

            case FBE_LIFECYCLE_STATUS_RESCHEDULE:
            default:
                return lifecycle_status;	
        }
    }

    /* If we get here then HC state machine finished. clear condition so we go back to Ready. 
    */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)provision_drive_p);

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_provision_drive_health_check_passive_side_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_peer_death_release_sl_function()
 ******************************************************************************
 * @brief
 *  This function is used to verify the drive fault bit of the pvd.
 *
 * @param base_object_p          - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  07/13/2012 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_peer_death_release_sl_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_provision_drive_t *     provision_drive_p = (fbe_provision_drive_t *)base_object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Release all SL \n", __FUNCTION__);
    fbe_stripe_lock_release_peer_data_stripe_locks(&provision_drive_p->base_config.metadata_element);

    fbe_metadata_element_clear_abort_monitor_ops(&((fbe_base_config_t *)provision_drive_p)->metadata_element);

    /* clear the condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X\n", __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_provision_drive_peer_death_release_sl_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_set_drive_fault()
 ******************************************************************************
 * @brief
 *  This function is used to set the drive fault in the NP and persist it.
 *
 * @param packet_p      - pointer to a control packet from the scheduler.
 * @param context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  07/13/2012 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_set_drive_fault(fbe_packet_t * packet_p,
                                    fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;    
    fbe_status_t                        status;
    
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return, we are already in the completion routine
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);
    status = fbe_provision_drive_metadata_set_drive_fault_state(provision_drive_p, packet_p, FBE_TRUE);

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_set_drive_fault()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_clear_drive_fault()
 ******************************************************************************
 * @brief
 *  This function is used to clear the drive fault in the NP and persist it.
 *
 * @param packet_p      - pointer to a control packet from the scheduler.
 * @param context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  07/13/2012 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_clear_drive_fault(fbe_packet_t * packet_p,
                                      fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;    
    fbe_status_t                        status;
    
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return, we are already in the completion routine
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);
    status = fbe_provision_drive_metadata_set_drive_fault_state(provision_drive_p, packet_p, FBE_FALSE);

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_clear_drive_fault()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_reinit_np_cond_function()
 ******************************************************************************
 * @brief
 *  This condition function is used to re-initialize the nonpaged metadata.
 *
 * @param object_p              - pointer to a base object.
 * @param packet_p              - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t 
 *
 * @author
 *  
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_provision_drive_reinit_np_cond_function(
                                fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_provision_drive_t * provision_drive_p = NULL;
    fbe_bool_t is_active;
    fbe_base_config_nonpaged_metadata_t    *base_config_nonpaged_metadata_p;
 
    provision_drive_p = (fbe_provision_drive_t*)object;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);
           
    is_active = fbe_base_config_is_active((fbe_base_config_t *)provision_drive_p);
    if (!is_active){ /* This will take care of passive side */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Before write the default nonpaged metadata value, zero the non-paged metadata */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)provision_drive_p, (void **)&base_config_nonpaged_metadata_p);
    fbe_zero_memory(base_config_nonpaged_metadata_p, sizeof(fbe_base_config_nonpaged_metadata_t));
    
    /* set the nonpaged metadata init condition. */
    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                           (fbe_base_object_t *)provision_drive_p,
                           FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_NONPAGED_METADATA);

    /* Persist the non-page metadata */
    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                           (fbe_base_object_t *)provision_drive_p,
                           FBE_BASE_CONFIG_LIFECYCLE_COND_PERSIST_DEFAULT_NONPAGED_METADATA);

    //  Set the condition to start the verify invaldiate operation.
    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, 
                           (fbe_base_object_t *) provision_drive_p,
                           FBE_PROVISION_DRIVE_LIFECYCLE_COND_METADATA_VERIFY_INVALIDATE);

    fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);

    return FBE_LIFECYCLE_STATUS_DONE;

}
/******************************************************************************
 * end fbe_provision_drive_reinit_np_cond_function()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_initiate_health_check()
 ******************************************************************************
 * @brief
 *  This condition function is used to initiate health check for a drive.
 *
 * @param object_p              - pointer to a base object.
 * @param packet_p              - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t 
 *
 * @author  
 *   09/10/2012  Wayne Garrett - Reworked.
 *  
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_initiate_health_check(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_provision_drive_t               *provision_drive_p = (fbe_provision_drive_t *)object_p;

    fbe_provision_drive_lock(provision_drive_p);

    /* The local state determines which function processes this condition.
     */
    if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_ABORT_REQ))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC: HEALTH CHECK req aborted.     local_state:0x%llx local_clstr:0x%llx peer_clstr:0x%llx\n", 
                              provision_drive_p->local_state,
                              provision_drive_p->provision_drive_metadata_memory.flags,
                              provision_drive_p->provision_drive_metadata_memory_peer.flags);

        return fbe_provision_drive_health_check_abort_req_originator(provision_drive_p, packet_p);
    }
    else if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_STARTED))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC: HEALTH CHECK started.         local_state:0x%llx local_clstr:0x%llx peer_clstr:0x%llx\n", 
                              (unsigned long long)provision_drive_p->local_state,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);

        return fbe_provision_drive_health_check_started_originator(provision_drive_p, packet_p);
    }/* originator side waiting for PDO HC to complete */
    else if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_WAITING))  
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC: HEALTH CHECK waiting.         local_state:0x%llx local_clstr:0x%llx peer_clstr:0x%llx\n", 
                              (unsigned long long)provision_drive_p->local_state,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);

        return fbe_provision_drive_health_check_waiting_originator(provision_drive_p, packet_p);
    }
    else if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_DONE))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC: HEALTH CHECK done.            local_state:0x%llx local_clstr:0x%llx peer_clstr:0x%llx\n", 
                              (unsigned long long)provision_drive_p->local_state,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);

        return fbe_provision_drive_health_check_done(provision_drive_p, packet_p);
    }
    else if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_FAILED))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC:HEALTH CHECK failed.           local_state:0x%llx local_clstr:0x%llx peer_clstr:0x%llx\n",  
                              (unsigned long long)provision_drive_p->local_state,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);

        return fbe_provision_drive_health_check_failed(provision_drive_p, packet_p);
    }/* Secondary side. */
    else if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_PEER_START))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC: HEALTH CHECK peer start.      local_state:0x%llx local_clstr:0x%llx peer_clstr:0x%llx\n", 
                              (unsigned long long)provision_drive_p->local_state,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);

        return fbe_provision_drive_health_check_start_non_originator(provision_drive_p, packet_p);
    }/* Secondary side. */
    else if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_PEER_FAILED))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC: HEALTH CHECK peer failed.     local_state:0x%llx local_clstr:0x%llx peer_clstr:0x%llx\n",  
                              (unsigned long long)provision_drive_p->local_state,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);

        return fbe_provision_drive_health_check_peer_failed(provision_drive_p, packet_p);
    }/* handle HC Request for originator and secondary side*/
    else if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_REQUEST)) 
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC: HEALTH CHECK request.         local_state:0x%llx local_clstr:0x%llx peer_clstr:0x%llx\n", 
                              (unsigned long long)provision_drive_p->local_state,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);

        return fbe_provision_drive_health_check_request(provision_drive_p, packet_p);
    }/* clustered flag hasn't been set yet, so try again */    
    else
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC: HEALTH CHECK not started yet. local_state:0x%llx local_clstr:0x%llx peer_clstr:0x%llx\n", 
                              (unsigned long long)provision_drive_p->local_state,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

}
/*********************************************************************
 * end fbe_provision_drive_initiate_health_check_cond_function()
 *********************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_clear_DDMI_region_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to clear the DDMI region of the pvd.
 *  When this PVD is not initialized, this condition will be set.
 *  So only the new created PVD will clear the DDMI region.
 *
 * @param base_object_p          - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  07/19/2012 - Create. gaoh1
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_clear_DDMI_region_cond_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p)
{
    fbe_memory_request_t    *memory_request = NULL;
    fbe_memory_request_priority_t   memory_request_priority = 0;
    fbe_provision_drive_t   *provision_drive_p = (fbe_provision_drive_t *)base_object_p;
    fbe_status_t    status;

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: start. pvd's object id: 0x%x. server id 0x%x\n",
                           __FUNCTION__, provision_drive_p->block_edge.base_edge.client_id,
                           provision_drive_p->block_edge.base_edge.server_id);            

    /* Allocate memory for I/O */
    memory_request = fbe_transport_get_memory_request(packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(packet_p);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    fbe_memory_build_chunk_request(memory_request,
                        FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                        CHUNK_NUMBER_PER_OPERATION_FOR_CLEAR_DDMI + 1, /* number of chunks */
                        memory_request_priority,
                        fbe_transport_get_io_stamp(packet_p),
                        (fbe_memory_completion_function_t)fbe_provision_drive_clear_DDMI_region_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                        (fbe_base_object_t *)provision_drive_p);

    fbe_transport_memory_request_set_io_master(memory_request, packet_p);

    status = fbe_memory_request_entry(memory_request);                        
    if ((FBE_STATUS_OK != status) && (FBE_STATUS_PENDING != status)) {
        /*allocate memory failed, dont clear the condition and try again. */
        fbe_base_object_trace(base_object_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s allocate memory failed \n", 
                              __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }  else {
        /* The callback function will clear the condition */
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

}
/******************************************************************************
 * end fbe_provision_drive_clear_DDMI_region_cond_function()
 ******************************************************************************/

static fbe_status_t 
fbe_provision_drive_clear_DDMI_region_allocation_completion(fbe_memory_request_t *memory_request, fbe_memory_completion_context_t context)
{
    fbe_private_space_layout_region_t   psl_region_info;
    fbe_memory_header_t *       master_memory_header = NULL;
    fbe_memory_header_t *       next_memory_header_p = NULL;
    fbe_provision_drive_t   *provision_drive_p = NULL;
    fbe_packet_t    *packet = NULL;
    fbe_packet_t    *new_packet = NULL;
    fbe_payload_ex_t    *sep_payload = NULL;
    fbe_payload_block_operation_t   *block_operation = NULL;
    fbe_optimum_block_size_t    optimum_block_size;
    fbe_sg_element_t    *sg_list = NULL;
    fbe_u8_t            *buffer = NULL;
    fbe_block_count_t   blocks;
    fbe_u32_t   index;
    fbe_status_t    status;

    provision_drive_p = (fbe_provision_drive_t *)context;
    packet = fbe_transport_memory_request_to_packet(memory_request);

    if (FBE_FALSE == fbe_memory_request_is_allocation_complete(memory_request)){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s allocate memory failed \n", 
                              __FUNCTION__);
        /* if failed, didn't clear the condition and return */
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_DDMI, &psl_region_info);

    /* initialize the new packet and sg_list */
    master_memory_header = fbe_memory_get_first_memory_header(memory_request);
    buffer = (fbe_u8_t *)fbe_memory_header_to_data(master_memory_header);
    fbe_zero_memory(buffer, FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
    new_packet = (fbe_packet_t *)buffer;
    sg_list = (fbe_sg_element_t *)(buffer + sizeof(fbe_packet_t));

    fbe_transport_initialize_sep_packet(new_packet);
    fbe_transport_add_subpacket(packet, new_packet);
    sep_payload = fbe_transport_get_payload_ex(new_packet);
      
    for (index = 0; index < CHUNK_NUMBER_PER_OPERATION_FOR_CLEAR_DDMI; index++) {
        fbe_memory_get_next_memory_header(master_memory_header, &next_memory_header_p);
        master_memory_header = next_memory_header_p;
        buffer = (fbe_u8_t *)fbe_memory_header_to_data(next_memory_header_p);
        fbe_zero_memory(buffer, FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
        sg_list[index].address = buffer;
        sg_list[index].count = CHUNK_SIZE_PER_OPERATION_FOR_CLEAR_DDMI * FBE_BE_BYTES_PER_BLOCK;
    }
    sg_list[index].address = NULL;
    sg_list[index].count = 0;

    fbe_payload_ex_set_sg_list(sep_payload, sg_list, CHUNK_NUMBER_PER_OPERATION_FOR_CLEAR_DDMI);

    if (psl_region_info.size_in_blocks < CHUNK_SIZE_PER_OPERATION_FOR_CLEAR_DDMI * CHUNK_NUMBER_PER_OPERATION_FOR_CLEAR_DDMI) {
        blocks = psl_region_info.size_in_blocks;
    } else {
        blocks = CHUNK_SIZE_PER_OPERATION_FOR_CLEAR_DDMI * CHUNK_NUMBER_PER_OPERATION_FOR_CLEAR_DDMI;
    }

    fbe_block_transport_edge_get_optimum_block_size(&provision_drive_p->block_edge, &optimum_block_size);
    block_operation = fbe_payload_ex_allocate_block_operation(sep_payload);
    fbe_payload_block_build_operation(block_operation, 
                                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                psl_region_info.starting_block_address,
                                blocks,
                                FBE_BE_BYTES_PER_BLOCK,
                                optimum_block_size,
                                NULL);
    
    fbe_transport_set_completion_function(new_packet,
                            fbe_provision_drive_clear_DDMI_region_io_completion,
                            (fbe_base_object_t *)provision_drive_p);

    /* The packet will send to LDO directly. It shouldn't send to PVD. Because the edge
    *   between PVD and LDO may have offset, it will invalid the DDMI offset value defined in PSL.
    */
    fbe_transport_set_address(new_packet,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              provision_drive_p->block_edge.base_edge.server_id);
    fbe_payload_ex_increment_block_operation_level(sep_payload);

    new_packet->base_edge = NULL;
    status = fbe_topology_send_io_packet(new_packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_provision_drive_clear_DDMI_region_io_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t    *sep_payload = NULL;
    fbe_payload_block_operation_t   *block_operation = NULL;
    fbe_payload_block_operation_status_t    block_status;
    fbe_private_space_layout_region_t   psl_region_info;
    fbe_packet_t    *master_packet = NULL;
    fbe_base_object_t * base_object = (fbe_base_object_t *)context;
    fbe_provision_drive_t   *provision_drive_p = (fbe_provision_drive_t *)base_object;
    fbe_memory_request_t *memory_request = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_optimum_block_size_t    optimum_block_size;
    fbe_lba_t   lba;
    fbe_block_count_t   left_blocks;
    fbe_status_t    status;

    master_packet = fbe_transport_get_master_packet(packet);

    /*The block operation will be reused. */
    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);

    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation, &block_status);

    if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS){
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s block operation failed, block_status = 0x%x \n", 
                              __FUNCTION__, block_status);
        fbe_transport_remove_subpacket(packet);
        fbe_payload_ex_release_block_operation(sep_payload, block_operation);
        fbe_transport_destroy_packet(packet);
        /* free the memory */
        memory_request = fbe_transport_get_memory_request(master_packet);
        fbe_memory_free_request_entry(memory_request);

        /*If failed, don't clear the condition. */
        sep_payload = fbe_transport_get_payload_ex(master_packet);
        control_operation = fbe_payload_ex_get_control_operation(sep_payload);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet, status, 0);
        fbe_transport_complete_packet(master_packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_DDMI, &psl_region_info);

    left_blocks = (psl_region_info.starting_block_address + psl_region_info.size_in_blocks) - 
                (block_operation->lba + block_operation->block_count);
    
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: pvd's object id: 0x%x. server id 0x%x\n",
                           __FUNCTION__, provision_drive_p->block_edge.base_edge.client_id,
                           provision_drive_p->block_edge.base_edge.server_id);         

    /* Have we process all the blocks in DDMI region? If yes, we complete the packet and return */
    if ((block_operation->lba + block_operation->block_count) >= 
        (psl_region_info.starting_block_address + psl_region_info.size_in_blocks)) {
        fbe_transport_remove_subpacket(packet);
        fbe_payload_ex_release_block_operation(sep_payload, block_operation);
        fbe_transport_destroy_packet(packet);
        /* free the memory */
        memory_request = fbe_transport_get_memory_request(master_packet);
        fbe_memory_free_request_entry(memory_request);

        /* clear the condition. */
        status = fbe_lifecycle_clear_current_cond(base_object);
      
        sep_payload = fbe_transport_get_payload_ex(master_packet);
        control_operation = fbe_payload_ex_get_control_operation(sep_payload);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(master_packet, status, 0);
        fbe_transport_complete_packet(master_packet);
        return FBE_STATUS_OK;
    }
    
    /* we don't finish yet. continue... */
    if (left_blocks >= CHUNK_SIZE_PER_OPERATION_FOR_CLEAR_DDMI * CHUNK_NUMBER_PER_OPERATION_FOR_CLEAR_DDMI) {
        left_blocks = CHUNK_SIZE_PER_OPERATION_FOR_CLEAR_DDMI * CHUNK_NUMBER_PER_OPERATION_FOR_CLEAR_DDMI;
    }
    lba = block_operation->lba + block_operation->block_count;

    fbe_block_transport_edge_get_optimum_block_size(&provision_drive_p->block_edge, &optimum_block_size);
    fbe_payload_block_build_operation(block_operation, 
                                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                lba,
                                left_blocks,
                                FBE_BE_BYTES_PER_BLOCK,
                                optimum_block_size,
                                NULL);

    fbe_transport_set_completion_function(packet,
                            fbe_provision_drive_clear_DDMI_region_io_completion,
                            base_object);

    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              provision_drive_p->block_edge.base_edge.server_id);

    fbe_transport_packet_clear_callstack_depth(packet);/* reset this since we reuse the packet */

    packet->base_edge = NULL;
    status = fbe_topology_send_io_packet(packet);    

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 * fbe_provision_drive_reset_checkpoint_cond_function()
 ******************************************************************************
 * @brief
 *  If ZOD is set reset checkpoint to 0.  Needed since we do not persist
 *  the checkpoint at run time.
 *
 * @param object_p                  - pointer to a base object.
 * @param packet_p                  - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - lifecycle status.
 *
 * @author
 *  8/20/2012 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_reset_checkpoint_cond_function(fbe_base_object_t * object_p,
                                                             fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t *) object_p;
    fbe_lba_t zero_checkpoint;
    
    fbe_bool_t b_is_system_drive;
    fbe_lba_t default_offset;
    fbe_lba_t new_zero_checkpoint;
    fbe_object_id_t object_id;

    /* cast the base object to a virtual drive object */
    provision_drive_p = (fbe_provision_drive_t *) object_p;

    fbe_base_object_get_object_id(object_p, &object_id);
    b_is_system_drive = fbe_database_is_object_system_pvd(object_id);
    fbe_base_config_get_default_offset((fbe_base_config_t *)provision_drive_p, &default_offset);

    status = fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive_p, &zero_checkpoint);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't get zero checkpoint status: %d\n", __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Set the checkpoint to 0 if system drive since system drives are allowed to zero below their default offset.
     * We set checkpoint to default offset if non-system drive.
     */
    if (b_is_system_drive) {
        new_zero_checkpoint = 0;
    } else {
        new_zero_checkpoint = default_offset;
    }

    /* If we are zeroing and the checkpoint is not already zero,
     * then we should only do this condition if we are the active side. 
     */
    if ( zero_checkpoint == FBE_LBA_INVALID || (zero_checkpoint == 0) || !fbe_base_config_is_active((fbe_base_config_t *)provision_drive_p) ||
		(zero_checkpoint == new_zero_checkpoint))
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear condition, status: 0x%x\n", __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_provision_drive_reset_checkpoint_cond_completion,
                                          provision_drive_p);
    status = fbe_provision_drive_metadata_set_background_zero_checkpoint(provision_drive_p, packet_p, new_zero_checkpoint);

    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "pvd reset zero checkpoint to 0x%llx from 0x%llx\n", (unsigned long long)new_zero_checkpoint, (unsigned long long)zero_checkpoint);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_provision_drive_reset_checkpoint_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_reset_checkpoint_cond_completion()
 ******************************************************************************
 * @brief
 *  The completion function checks status and clears condition if needed.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  8/20/2012 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_reset_checkpoint_cond_completion(fbe_packet_t * packet_p,
                                                               fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_config_downstream_health_state_t downstream_health_state;

    provision_drive_p = (fbe_provision_drive_t*)context;
    downstream_health_state = fbe_provision_drive_verify_downstream_health(provision_drive_p);

    /* Get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && 
       (status == FBE_STATUS_OK))
    {    
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear condition, status: 0x%x", __FUNCTION__, status);
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s clear checkpoint failed, ctl_stat:%d, stat:%d\n",
                              __FUNCTION__, control_status, status);
    }
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_reset_checkpoint_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_inform_logical_state()
 ******************************************************************************
 * @brief
 *  This function is used to set the drive fault bit of the pvd.
 *
 * @param provision_drive_p      - pointer to a provision drive.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  03/29/2013 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_inform_logical_state(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;   
    fbe_block_transport_logical_state_t state = FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED;    
    fbe_bool_t                          end_of_life_state;
    fbe_bool_t                          drive_fault_state;

    fbe_provision_drive_metadata_get_drive_fault_state(provision_drive_p, &drive_fault_state);
    fbe_provision_drive_metadata_get_end_of_life_state(provision_drive_p, &end_of_life_state);
    if (drive_fault_state)
    {
        state = FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_DRIVE_FAULT;
    }
    else if (end_of_life_state)
    {
        state = FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_EOL;
    }
    else
    {
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                            FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s inform PDO, logical state:%d\n",
                          __FUNCTION__, state);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);    
    fbe_payload_control_build_operation (control_operation_p,
                                         FBE_BLOCK_TRANSPORT_CONTROL_CODE_LOGICAL_DRIVE_STATE_CHANGED,
                                         &state,
                                         sizeof(fbe_block_transport_logical_state_t));
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_inform_logical_state_completion, provision_drive_p);
    status = fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);

    return FBE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_provision_drive_inform_logical_state()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_inform_logical_state_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for sending usurper to PDO.
 *
 * @param packet_p               - pointer to a control packet.
 * @param context                - pointer to an object.
 * 
 * @return fbe_status_t
 *
 * @author
 *  03/29/2013 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_inform_logical_state_completion(fbe_packet_t * packet_p,
                                                    fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;

    provision_drive_p = (fbe_provision_drive_t*)context;

    /* get status. */
    status = fbe_transport_get_status_code (packet_p);
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* check the status. */
    if((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (status != FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s inform PDO failed, control:%d, packet:%d\n",
                              __FUNCTION__, control_status, status);
        return FBE_STATUS_OK;
    }

    fbe_provision_drive_clear_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_CHECK_LOGICAL_STATE);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_inform_logical_state_completion()
 ******************************************************************************/
fbe_lifecycle_status_t fbe_provision_drive_check_key_info(fbe_provision_drive_t *provision_drive_p,
                                                                 fbe_packet_t *packet_p)
{
    fbe_bool_t is_encryption_key_needed;
    fbe_path_attr_t path_attr = 0;
    fbe_u32_t index, number_of_clients;
    fbe_provision_drive_key_info_t      * key_info;
    fbe_provision_drive_config_type_t     config_type;
    fbe_encryption_key_mask_t mask0, mask1;
    fbe_base_config_encryption_state_t encryption_state;
    fbe_u32_t keys_needed;
    fbe_bool_t b_is_system_drive;
    fbe_object_id_t object_id;
    fbe_system_encryption_mode_t system_encryption_mode;
    fbe_bool_t key_found = FBE_FALSE;

	fbe_database_get_system_encryption_mode(&system_encryption_mode);
    fbe_base_object_get_object_id((fbe_base_object_t*)provision_drive_p, &object_id);
    b_is_system_drive = fbe_database_is_object_system_pvd(object_id);

    fbe_provision_drive_get_config_type(provision_drive_p, &config_type);
    fbe_provision_drive_get_key_info(provision_drive_p, 0, &key_info);
    fbe_block_transport_server_get_number_of_clients(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                     &number_of_clients);

    fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "check key info enc_mode: %d config_type: %d key_handle: %p sys_mode: %d c: %d\n", 
                                    provision_drive_p->base_config.encryption_mode,
                                    config_type, key_info->key_handle, system_encryption_mode, number_of_clients);

    for (index = 0; index < number_of_clients; index++) {
        fbe_provision_drive_get_key_info(provision_drive_p, index, &key_info);
        if (key_info->key_handle != NULL){
            key_found = FBE_TRUE;
            break;
        }
    }

    /* Need to figure out if we need to push the keys or not. If drives were zeroing,
     * mode would not have been changed but we still need to push the keys down and so
     * as long as there are keys, we need to verify if something needs to be pushed down or not
    */
    if (fbe_base_config_is_encrypted_mode((fbe_base_config_t *)provision_drive_p) ||
        fbe_base_config_is_rekey_mode((fbe_base_config_t *)provision_drive_p) ||
        (key_found == FBE_TRUE))
    {

        /* User drives need to wait for clients to get attached.
         */
        if ((number_of_clients == 0) && 
            !b_is_system_drive &&
            fbe_provision_drive_is_encryption_key_needed(provision_drive_p, 0)) {
            fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                            FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                            "%s: Waiting for keys.  Non sys drive Client: 0\n", __FUNCTION__);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        keys_needed = 0;
        for (index = 0; index < number_of_clients; index++) {
            /* Check if a new Key is required and if so wait for the keys to be provided */
            is_encryption_key_needed = fbe_provision_drive_is_encryption_key_needed(provision_drive_p, index);
    
            if (is_encryption_key_needed){
                /* At this request the key from the upstream objects  */
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_INFO,
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                                "%s: Waiting for keys.  Client: %d\n", __FUNCTION__, index);
    
                fbe_block_transport_server_get_path_attr(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                         index,
                                                         &path_attr);
                if (!(path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED)) {
                    fbe_provision_drive_utils_trace(provision_drive_p,
                                                    FBE_TRACE_LEVEL_INFO,
                                                    FBE_TRACE_MESSAGE_ID_INFO,
                                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                                    "%s: Set path Attr 0x%08x for Index: %d\n", __FUNCTION__, path_attr, index);
                    fbe_block_transport_server_set_path_attr(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                             index,
                                                             FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED);
                }
                keys_needed++;
            }
            else {
                fbe_block_transport_server_get_path_attr(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                         index,
                                                         &path_attr);
                if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED) {
                    fbe_provision_drive_utils_trace(provision_drive_p,
                                                    FBE_TRACE_LEVEL_INFO,
                                                    FBE_TRACE_MESSAGE_ID_INFO,
                                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                                    "%s: Clear path attr: 0x%08x for index:%d\n", __FUNCTION__, path_attr, index);
                    fbe_block_transport_server_clear_path_attr(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                               index,
                                                               FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED);
                }
            }
        }

        if (keys_needed && !b_is_system_drive) {
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* We previously registered the keys, but they appear to be incorrect. 
         * Do not let the caller proceed since we do not have valid keys. 
         */
        for (index = 0; index < number_of_clients; index++) {

            fbe_provision_drive_lock( provision_drive_p );
            fbe_provision_drive_get_encryption_state(provision_drive_p, index, &encryption_state);

            if ((encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEYS_INCORRECT) ||
                (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEY_ERROR)) {
                fbe_provision_drive_unlock( provision_drive_p );
                fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_ENCRYPTION,
                                                "provision_drive: do not push keys again, keys are incorrect.\n");
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            /* Keys are in progress now. 
             */
            if (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_PUSH_KEYS_DOWNSTREAM) {
                fbe_provision_drive_unlock( provision_drive_p );
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "provision_drive: keys are in progress. server_idx: %d\n", index);
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            fbe_provision_drive_unlock( provision_drive_p );
        }

        for (index = 0; index < number_of_clients; index++) {
            fbe_provision_drive_get_key_info(provision_drive_p, index, &key_info);
            /* If we have a key handle but we have not registered the key with the port, 
             * then kick off the register now. 
             */
            if (key_info->key_handle != NULL) {
                fbe_encryption_key_mask_get(&key_info->key_handle->key_mask, 0, &mask0);
                fbe_encryption_key_mask_get(&key_info->key_handle->key_mask, 1, &mask1);

                if ( (b_is_system_drive && fbe_provision_drive_validation_area_needs_init(provision_drive_p, index)) || 
                    ((mask0 & FBE_ENCRYPTION_KEY_MASK_VALID) && (key_info->mp_key_handle_1 == FBE_INVALID_KEY_HANDLE)) ||
                     ((mask1 & FBE_ENCRYPTION_KEY_MASK_VALID) && (key_info->mp_key_handle_2 == FBE_INVALID_KEY_HANDLE)) ) {
                    fbe_provision_drive_register_keys(provision_drive_p, packet_p, key_info->key_handle, index);
                    return FBE_LIFECYCLE_STATUS_PENDING;
                }
            }
        }
    }

    return FBE_LIFECYCLE_STATUS_CONTINUE;
}

static fbe_bool_t 
fbe_provision_drive_is_encryption_key_needed(fbe_provision_drive_t * provision_drive,
                                             fbe_edge_index_t client_index)
{
    fbe_provision_drive_key_info_t      * key_info;
    fbe_provision_drive_config_type_t     config_type;

    fbe_provision_drive_get_config_type(provision_drive, &config_type);

    /* Currently "unbound" PVD will not need keys */
    if(config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID){
        return FBE_FALSE;
    }

    fbe_provision_drive_get_key_info(provision_drive, client_index, &key_info);

    if (key_info->key_handle == NULL) {
        return FBE_TRUE;
    } else if (!fbe_encryption_key_mask_has_valid_key(&key_info->key_handle->key_mask)) {
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }
}

/*!****************************************************************************
 *          fbe_provision_drive_encrypt_paged_cond_completion()
 ******************************************************************************
 *
 * @brief   If we successfully reconstructed the paged metadata then clear the
 *          reconstruct condition.  Other wait for a short period and retry.
 *
 * @param   packet_p    - pointer to a control packet from the scheduler
 * @param   context     - pointer to a base object 
 *
 * @return  status - Always FBE_STATUS_OK
 *
 * @author
 *  01/13/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_encrypt_paged_cond_completion(fbe_packet_t *packet_p,
                                        fbe_packet_completion_context_t context)
{
    fbe_status_t                status;
    fbe_provision_drive_t      *provision_drive_p = (fbe_provision_drive_t*)context;
    fbe_bool_t                  b_needs_reconstruct;
    fbe_bool_t                  b_paged_valid;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    /* If the request failed or the reconstruct flag is still set then do not 
     * clear the condition.
     */
    b_needs_reconstruct = fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, 
                                                                      FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_CONSUMED);
    b_paged_valid = fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, 
                                                      FBE_PROVISION_DRIVE_NP_FLAG_PAGED_VALID);
    status = fbe_transport_get_status_code(packet_p);
    if ((status != FBE_STATUS_OK)                                                  ||
        (b_needs_reconstruct == FBE_TRUE)                                          || 
        ((b_paged_valid == FBE_FALSE)                                          &&
         fbe_base_config_is_rekey_mode((fbe_base_config_t *)provision_drive_p)    )    ) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s encrypt paged failed - needs reconstruct: %d valid: %d status: 0x%x\n", 
                              __FUNCTION__, b_needs_reconstruct, b_paged_valid, status); 

        /* Wait a second before re-trying
         */
        fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*)provision_drive_p,
                                 (fbe_lifecycle_timer_msec_t)1000);
        return FBE_STATUS_OK;
    }

    /* No hook status is handled since we can only log here.
     */
    fbe_provision_drive_check_hook(provision_drive_p,
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                   FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_COMPLETE,
                                   0, &hook_status);

    fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*)provision_drive_p,
                             (fbe_lifecycle_timer_msec_t)0);

    return FBE_STATUS_OK;
}
/********************************************************
 * end fbe_provision_drive_encrypt_paged_cond_completion
 ********************************************************/

/*!***************************************************************************
 *          fbe_provision_drive_paged_metadata_needs_zero_cond_function()
 *****************************************************************************
 *
 * @brief   After a encrypted drive is swapped-out (either permanent spare or
 *          copy), the paged needs to be zeroed without a key and the consumed
 *          area zeroed.  Typically the drive will not be alive but if it is
 *          the data needs to be zeroed.
 *
 * @param   object_p - current object.
 * @param   packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE   
 *
 * @note    This is a preset condition.
 *
 * @author
 *  11/11/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_provision_drive_paged_metadata_needs_zero_cond_function(fbe_base_object_t * object_p, 
                                                         fbe_packet_t * packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_provision_drive_t              *provision_drive_p = (fbe_provision_drive_t *)object_p;
    fbe_bool_t                          b_is_active;
    fbe_provision_drive_nonpaged_metadata_t *nonpaged_metadata_p = NULL;
	fbe_base_config_encryption_mode_t   encryption_mode;
	fbe_base_config_encryption_state_t  encryption_state;
    fbe_scheduler_hook_status_t         hook_status;
    fbe_bool_t b_io_already_quiesced;
    fbe_lifecycle_state_t lifecycle_state;

    /* Determine if we are active
     */    
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)provision_drive_p);
    fbe_base_config_get_encryption_mode((fbe_base_config_t *)provision_drive_p, &encryption_mode);
    fbe_provision_drive_get_encryption_state(provision_drive_p,
                                             0, /* Client 0 for paged. */
                                             &encryption_state);

    /* This is a preset so if needs zero is not set just exit.
     */
    if (!fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, 
                                                     FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_ZERO)) {
        /* If needs zero is not required clear this condition and
         * return done.
         */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If a debug hook is set, we need to execute the specified action.
     */
    fbe_provision_drive_check_hook(provision_drive_p,
                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                              FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START, 
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Trace some information.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) provision_drive_p, (void **)&nonpaged_metadata_p);

    fbe_base_object_get_lifecycle_state((fbe_base_object_t*)provision_drive_p, &lifecycle_state);
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "provision_drive: %s paged needs zero NP flags: 0x%x enc mode: 0x%x state: 0x%x\n",
                          (b_is_active) ? "Active" : "Passive",
                          nonpaged_metadata_p->flags, encryption_mode, lifecycle_state);

    fbe_provision_drive_utils_quiesce_io_if_needed(provision_drive_p, &b_io_already_quiesced);
    if ((lifecycle_state == FBE_LIFECYCLE_STATE_READY) && !b_io_already_quiesced){
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* The quiesce aborted stripe locks and paged.  Open the gates 
     * again so we can send the md ops. 
     */
    fbe_metadata_element_clear_abort(&provision_drive_p->base_config.metadata_element);

    /* We need to write default paged metadata if NEEDS_ZERO flag is set. 
     */
    if (fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_ZERO))
    {
        /* Since needs zero replicates the paged reconstruct clear that condition now.
         */
        fbe_lifecycle_force_clear_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*)provision_drive_p,
                                       FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_METADATA_RECONSTRUCT);

        /* Clear the cache on both sides.
         */
        fbe_base_config_metadata_paged_clear_cache((fbe_base_config_t *)provision_drive_p);
        if (b_is_active) {
            fbe_object_id_t	object_id;
            fbe_bool_t is_system_drive;

            fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
            is_system_drive = fbe_database_is_object_system_pvd(object_id);
            if (!is_system_drive) {
                fbe_transport_set_completion_function(packet_p, fbe_provision_drive_write_default_paged_metadata_set_np_flag, provision_drive_p);
                /* initialize signature for the provision drive object. */
                status = fbe_provision_drive_metadata_write_default_paged_metadata(provision_drive_p, packet_p);
                return FBE_LIFECYCLE_STATUS_PENDING;
            } else {
                fbe_transport_set_completion_function(packet_p, fbe_provision_drive_write_default_paged_metadata_set_np_flag, provision_drive_p);
                status = fbe_provision_drive_write_default_system_paged(provision_drive_p, packet_p);
                return FBE_LIFECYCLE_STATUS_PENDING;
            }
        }
    }

    /* If paged reconstruction is not required clear this condition and
     * return done.
     */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/*******************************************************************
 * end fbe_provision_drive_paged_metadata_needs_zero_cond_function()
 *******************************************************************/

/*!***************************************************************************
 *          fbe_provision_drive_paged_metadata_reconstruct_cond_function()
 *****************************************************************************
 *
 * @brief   If we were in the middle of encrption, continue the re-key.
 *
 * @param   object_p - current object.
 * @param   packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE   
 *
 * @note    This is a preset condition.
 *
 * @author
 *  01/09/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_provision_drive_paged_metadata_reconstruct_cond_function(fbe_base_object_t * object_p, 
                                                         fbe_packet_t * packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_provision_drive_t              *provision_drive_p = (fbe_provision_drive_t *)object_p;
    fbe_bool_t                          b_is_active;
    fbe_provision_drive_nonpaged_metadata_t *nonpaged_metadata_p = NULL;
	fbe_base_config_encryption_mode_t   encryption_mode;
	fbe_base_config_encryption_state_t  encryption_state;
    fbe_scheduler_hook_status_t         hook_status;
    fbe_bool_t b_io_already_quiesced;
    fbe_lifecycle_state_t lifecycle_state;

    /* Determine if we are active
     */    
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)provision_drive_p);


    /* If a debug hook is set, we need to execute the specified action.
     */
    fbe_provision_drive_check_hook(provision_drive_p,
                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                              FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START, 
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* We tried to reconstruct paged, but may have received an unwrap error from the miniport.
     * If so, we will not retry now until we get good keys. 
     */
    fbe_provision_drive_get_encryption_state(provision_drive_p,
                                             0, /* Client 0 for paged. */
                                             &encryption_state);

    if (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEY_ERROR) {
        
        fbe_provision_drive_check_hook(provision_drive_p,
                                       SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                       FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_KEY_WRAP_ERROR, 
                                       0, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) { return FBE_LIFECYCLE_STATUS_DONE; }

        fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ENCRYPTION,
                                        "provision_drive: Keys are incorrect do not continue activate reconstruct.\n");
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Trace some information.
     */
    fbe_base_config_get_encryption_mode((fbe_base_config_t *)provision_drive_p, &encryption_mode);
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) provision_drive_p, (void **)&nonpaged_metadata_p);

    fbe_base_object_get_lifecycle_state((fbe_base_object_t*)provision_drive_p, &lifecycle_state);
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "provision_drive: %s paged reconst NP flags: 0x%x enc mode: 0x%x state: 0x%x\n",
                          (b_is_active) ? "Active" : "Passive",
                          nonpaged_metadata_p->flags, encryption_mode, lifecycle_state);

    fbe_provision_drive_utils_quiesce_io_if_needed(provision_drive_p, &b_io_already_quiesced);
    if ((lifecycle_state == FBE_LIFECYCLE_STATE_READY) && !b_io_already_quiesced){
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* The quiesce aborted stripe locks and paged.  Open the gates 
     * again so we can send the md ops. 
     */
    fbe_metadata_element_clear_abort(&provision_drive_p->base_config.metadata_element);

    /* We need to write default paged metadata if NEEDS_ZERO flag is set. 
     */
    if (fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_ZERO))
    {
        fbe_base_config_metadata_paged_clear_cache((fbe_base_config_t *)provision_drive_p);
        if (b_is_active) {
            fbe_object_id_t	object_id;
            fbe_bool_t is_system_drive;
            fbe_provision_drive_config_type_t config_type;

            fbe_provision_drive_get_config_type(provision_drive_p, &config_type);
            if (config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_EXT_POOL) {
                fbe_transport_set_completion_function(packet_p, fbe_provision_drive_metadata_write_default_paged_for_ext_pool_clear_np_flag, provision_drive_p);
                status = fbe_provision_drive_metadata_write_default_paged_for_ext_pool(provision_drive_p, packet_p);
                return FBE_LIFECYCLE_STATUS_PENDING;
            }

            fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
            is_system_drive = fbe_database_is_object_system_pvd(object_id);
            if (!is_system_drive) {
                fbe_transport_set_completion_function(packet_p, fbe_provision_drive_write_default_paged_metadata_set_np_flag, provision_drive_p);
                /* initialize signature for the provision drive object. */
                status = fbe_provision_drive_metadata_write_default_paged_metadata(provision_drive_p, packet_p);
                return FBE_LIFECYCLE_STATUS_PENDING;
            } else {
                fbe_transport_set_completion_function(packet_p, fbe_provision_drive_write_default_paged_metadata_set_np_flag, provision_drive_p);
                status = fbe_provision_drive_write_default_system_paged(provision_drive_p, packet_p);
                return FBE_LIFECYCLE_STATUS_PENDING;
            }
        }
    }

    /* When we go down while reconstructing the paged we need to pick up again where we left off 
     * before we can go ready. 
     */
    if ( b_is_active                                                                                            &&
        (fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, 
                                                     FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_CONSUMED) ||
         (!fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, 
                                                      FBE_PROVISION_DRIVE_NP_FLAG_PAGED_VALID) &&
           fbe_base_config_is_rekey_mode((fbe_base_config_t *)provision_drive_p)                        )     )     ) {

        fbe_provision_drive_check_hook(provision_drive_p,
                                       SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                       FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_INITIATE, 
                                       0, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) { return FBE_LIFECYCLE_STATUS_DONE; }

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_set_completion_function(packet_p, (fbe_packet_completion_function_t)fbe_provision_drive_encrypt_paged_cond_completion, provision_drive_p);
        fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_mark_paged_write_start);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* If paged reconstruction is not required clear this condition and
     * return done.
     */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/*******************************************************************
 * end fbe_provision_drive_paged_metadata_reconstruct_cond_function()
 *******************************************************************/

/*!***************************************************************************
 *          fbe_provision_drive_background_monitor_handle_encryption_operations()
 *****************************************************************************
 *
 * @brief   This function removes usurper packet from the queue and unregisters the key.
 *
 * @param   provision_drive_p - Pointer to provision drive object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/09/2014  Lili Chen  - Created.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_background_monitor_handle_encryption_operations(fbe_provision_drive_t * provision_drive_p)
{
    fbe_packet_t * usurper_packet_p = NULL;
    fbe_status_t status;
    fbe_base_config_encryption_mode_t encryption_mode;

    fbe_base_config_get_encryption_mode((fbe_base_config_t *) provision_drive_p, &encryption_mode);
    if (encryption_mode != FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) 
    {
        return FBE_STATUS_OK;
    }

    status = fbe_base_object_find_control_op_from_usurper_queue((fbe_base_object_t*)provision_drive_p, &usurper_packet_p, FBE_PROVISION_DRIVE_CONTROL_CODE_UNREGISTER_KEYS);
    if(usurper_packet_p != NULL) 
    {
        status = fbe_base_object_remove_from_usurper_queue((fbe_base_object_t*)provision_drive_p, usurper_packet_p);
        fbe_provision_drive_unregister_keys(provision_drive_p, usurper_packet_p);
    }
    return FBE_STATUS_OK;
}
/*******************************************************************
 * end fbe_provision_drive_background_monitor_handle_encryption_operations()
 *******************************************************************/


/*!***************************************************************************
 *          fbe_provision_drive_is_md_memory_initialized()
 *****************************************************************************
 *
 * @brief 
 *  This function checks whether the metadata memory is already initialized.
 *  It detects multiple re-entry into specialize state.
 *
 * @param   provision_drive_p - Pointer to provision drive object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/17/2014  Lili Chen  - Created.
 *
 ******************************************************************************/
static fbe_bool_t
fbe_provision_drive_is_md_memory_initialized(fbe_provision_drive_t * provision_drive_p)
{
    fbe_base_config_t * base_config = (fbe_base_config_t *)provision_drive_p;

    if (base_config->metadata_element.metadata_memory.memory_ptr == (void *)&provision_drive_p->provision_drive_metadata_memory &&
        base_config->metadata_element.metadata_memory_peer.memory_ptr == (void *)&provision_drive_p->provision_drive_metadata_memory_peer){
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }
}
/*******************************************************************
 * end fbe_provision_drive_is_md_memory_initialized()
 *******************************************************************/

/*!***************************************************************************
 *          fbe_provision_drive_write_default_paged_metadata_set_np_flag()
 *****************************************************************************
 *
 * @brief 
 *  This is the completion for writing default paged metadata.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/17/2014  Lili Chen  - Created.
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_write_default_paged_metadata_set_np_flag(fbe_packet_t * packet_p,
                                                             fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_system_encryption_mode_t		system_encryption_mode;

    provision_drive_p = (fbe_provision_drive_t*)context;
    
    fbe_database_get_system_encryption_mode(&system_encryption_mode);
    if (system_encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED &&
        !fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_ZERO)) {
        return FBE_STATUS_OK;
    }

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        //if (fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_ZERO))
        //{
            /* Clear need zero if status is good. */
            fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_metadata_clear_np_flag_need_zero_paged);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        //}
    }

    return FBE_STATUS_OK;
}
/*******************************************************************
 * end fbe_provision_drive_write_default_paged_metadata_set_np_flag()
 *******************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_encryption_cond_function()
 ******************************************************************************
 * @brief
 *  Handle encryption register/unregister.
 *
 * @param object_p   - pointer to a base object.
 * @param packet_p  - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_DONE when metadata
 *                                    verification done.
 *
 * @author
 *  2/11/2014 - Created. Rob Foley
 ******************************************************************************/

static fbe_lifecycle_status_t 
fbe_provision_drive_encryption_cond_function(fbe_base_object_t * object_p,
                                             fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t*)object_p;
    fbe_system_encryption_mode_t system_encryption_mode;
    fbe_base_config_encryption_mode_t encryption_mode;
    fbe_bool_t b_is_system_drive;
    fbe_object_id_t object_id;
    fbe_lifecycle_status_t lifecycle_status;
    fbe_lifecycle_state_t lifecycle_state;	

    fbe_database_get_system_encryption_mode(&system_encryption_mode);
    if (system_encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED){
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)provision_drive_p, &lifecycle_state);
    if (lifecycle_state == FBE_LIFECYCLE_STATE_DESTROY) {
        return fbe_provision_drive_monitor_destroy_unregister_keys(provision_drive_p, packet_p);
    }

    lifecycle_status = fbe_provision_drive_monitor_unregister_keys_if_needed(provision_drive_p, packet_p);
    if (lifecycle_status != FBE_LIFECYCLE_STATUS_CONTINUE) {
        return lifecycle_status;
    }

    fbe_base_config_get_encryption_mode((fbe_base_config_t *) provision_drive_p, &encryption_mode);
    if (encryption_mode <= FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED) {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_object_get_object_id(object_p, &object_id);
    b_is_system_drive = fbe_database_is_object_system_pvd(object_id);

    /* Non-system drives go through activate when config mode is changed. 
     * System drives need to stay ready and this code handles this in any condition. 
     */
    if (b_is_system_drive) {
        fbe_u32_t i;

        for (i = 0; i < FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX; i++) {
            if ((provision_drive_p->key_info_table[i].key_handle == NULL) &&
                ((provision_drive_p->key_info_table[i].mp_key_handle_1 != FBE_INVALID_KEY_HANDLE) || 
                 (provision_drive_p->key_info_table[i].mp_key_handle_2 != FBE_INVALID_KEY_HANDLE))) {
                fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_ENCRYPTION,
                                                "provision_drive: unregister keys for client %d mp_key1: 0x%llx mp_key2: 0x%llx\n", 
                                                i, provision_drive_p->key_info_table[i].mp_key_handle_1,
                                                provision_drive_p->key_info_table[i].mp_key_handle_2);
                fbe_transport_set_completion_function(packet_p, 
                                                      fbe_provision_drive_monitor_unregister_keys_completion,
                                                      provision_drive_p);
                status = fbe_provision_drive_port_unregister_keys(provision_drive_p, packet_p, i);
                return FBE_LIFECYCLE_STATUS_PENDING;
            }
        }
    }
    /* clear the metadata verify condition if non paged metadata is good. */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *) provision_drive_p);

    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************
 * end fbe_provision_drive_encryption_cond_function()
 **************************************/

/*!****************************************************************************
 * fbe_provision_drive_get_port_object_id()
 ******************************************************************************
 * @brief
 *  This function sends a usurper request to PDO to get port object id.
 *
 * @param provision_drive_p   - pointer to the provision drive.
 * @param packet_p            - pointer to a control packet.
 * 
 * @return fbe_status_t 
 *
 * @author
 *  2/19/2014 - Created. Lili Chen
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_get_port_object_id(fbe_provision_drive_t *provision_drive_p,
                                       fbe_packet_t * packet_p,
                                       fbe_object_id_t *port_object_id) 
{   
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_p = NULL;
    fbe_physical_drive_get_port_info_t  get_port_info_p;

    /* Allocate control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_PORT_INFO,
                                        &get_port_info_p,
                                        sizeof(fbe_physical_drive_get_port_info_t));

    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    /* Control packets sent to pdo. */
    status = fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);

    fbe_transport_wait_completion(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed status %X\n", 
                                __FUNCTION__, status);
        fbe_payload_ex_release_control_operation(payload_p, control_p);
        return status;
    }

    /* Free the resources we allocated previously.*/
    fbe_payload_ex_release_control_operation(payload_p, control_p);
    *port_object_id = get_port_info_p.port_object_id;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_provision_drive_get_port_object_id()
 **************************************/

/*!****************************************************************************
 *          fbe_provision_drive_can_exit_failed_state()
 ******************************************************************************
 * @brief
 *  Based on the downstream health and other conditions, determine if the this
 *  provision drive should exit the Failed state or not.
 *
 * @param   provision_drive_p - Pointer to virutal drive object
 * @param   b_can_exit_failed_p - Pointer to bool to set True or False.  
 * 
 * @return  status - Status of this request.
 *
 * @author
 *  06/05/2014  Ron Proulx  - Created from health broken condition.
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_can_exit_failed_state(fbe_provision_drive_t *provision_drive_p,
                                                       fbe_bool_t *b_can_exit_failed_p)
{
    fbe_bool_t                  b_is_active; 
    fbe_base_config_downstream_health_state_t downstream_health_state;
    fbe_bool_t                  b_is_end_of_life; 
    fbe_bool_t                  b_is_drive_fault;
    fbe_provision_drive_np_flags_t np_flags;
    fbe_lifecycle_state_t       peer_lifecycle_state;
    fbe_u32_t                   number_of_clients;

    /* Get other conditions that would prevent the provision drive from becoming ready.
     */
    *b_can_exit_failed_p = FBE_FALSE;
    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);
    downstream_health_state = fbe_provision_drive_verify_downstream_health(provision_drive_p);
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t*)provision_drive_p, &peer_lifecycle_state);
    fbe_provision_drive_metadata_get_end_of_life_state(provision_drive_p, &b_is_end_of_life);
    fbe_provision_drive_metadata_get_drive_fault_state(provision_drive_p, &b_is_drive_fault);
    fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &np_flags);

    /* Trace some information if enabled.
     */
    fbe_provision_drive_utils_trace(provision_drive_p, 
                                    FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_HEALTH_TRACING,
                                    "pvd can exit failed - health: %d EOL: %d DRIVE_FAULT: %d np flags: 0x%x \n",
                                    downstream_health_state, b_is_end_of_life, b_is_drive_fault, np_flags);

    /* If EOL is set BUT it has upstream clients `logically clear it'.
     */
    fbe_block_transport_server_get_number_of_clients(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                     &number_of_clients);
    if (number_of_clients > 0) {
        b_is_end_of_life = FBE_FALSE;
    }

    /* If none of the `marked drive offline' NP flags are set AND no other conditions
     * exists that would prevent the provision drive form becoming Ready goto Activate.
     */
    if (!fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p,
                                                     FBE_PROVISION_DRIVE_NP_FLAG_EAS_STARTED) &&
         (downstream_health_state == FBE_DOWNSTREAM_HEALTH_OPTIMAL)                           &&
         (b_is_end_of_life == FBE_FALSE)                                                      &&
         (b_is_drive_fault == FBE_FALSE)                                                         ) {
        /*  If we are active or the peer is no longer in the failed state allow
         *  the provision drive to goto activate.
         */ 
        if ((b_is_active == FBE_TRUE)                          ||
            (peer_lifecycle_state != FBE_LIFECYCLE_STATE_FAIL)    ) {
            /* Allow the provision drive to goto activate.
             */
            *b_can_exit_failed_p = FBE_TRUE;
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_can_exit_failed_state()
 ******************************************************************************/
/*!****************************************************************************
 *          fbe_provision_drive_get_wear_leveling()
 ******************************************************************************
 * @brief
 *  Go and retrieve the wear leveling info on the drive
 *
 * @param   provision_drive_p - Pointer to virutal drive object
 * @param   packet_p - Pointer to the packet
 * 
 * @return  fbe_lifecycle_status_t - Status of this request.
 *
 * @author
 *  08/03/2015  Deanna Heng
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_provision_drive_get_wear_leveling(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_provision_drive_get_ssd_statistics_t   *drive_ssd_stats_p = NULL;

    /* allocate the buffer  
     */
    drive_ssd_stats_p = (fbe_provision_drive_get_ssd_statistics_t  *) fbe_transport_allocate_buffer();
    if (drive_ssd_stats_p == NULL)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* allocate the new control operation to build. 
     */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SSD_STATISTICS,
                                        drive_ssd_stats_p,
                                        sizeof(fbe_provision_drive_get_ssd_statistics_t));
    
    fbe_payload_ex_increment_control_operation_level(sep_payload_p);

    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_get_wear_level_completion,
                                          provision_drive_p);

    /* call the function to get the drive ssd statistics 
     */
    fbe_provision_drive_usurper_get_ssd_statitics(provision_drive_p, packet_p);

    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_provision_drive_get_wear_leveling()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_get_wear_level_completion()
 ******************************************************************************
 * @brief
 *  Update the provision drive information received from the control packet
 *
 * @param packet_p - pointer to the packet
 * @param context  - provision drive.
 *
 * @return status 
 *
 * @author 
 *   08/03/2015 - Created. Deanna Heng
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_get_wear_level_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_provision_drive_get_ssd_statistics_t   *drive_ssd_stats_p;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_payload_control_status_t                control_status;
    fbe_provision_drive_t                      *provision_drive_p = NULL; 
    fbe_u64_t                                   projected_lifetime;

    provision_drive_p = (fbe_provision_drive_t *) context;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* If the status is not ok release the control operation
     * and return ok so that packet gets completed
     */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_buffer(control_operation_p, &drive_ssd_stats_p);
    if(status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        /* Release the control buffer and control operation */
        fbe_transport_release_buffer(drive_ssd_stats_p);
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0); 
        return status;
    }

    /* get the control buffer pointer from the packet payload.
     */
    status = fbe_provision_drive_get_control_buffer(provision_drive_p,
                                                    packet_p, 
                                                    sizeof(fbe_provision_drive_get_ssd_statistics_t),
                                                    (fbe_payload_control_buffer_t)&drive_ssd_stats_p);
    if (status != FBE_STATUS_OK) 
    { 
        /* Release the control buffer and control operation
         */
        fbe_transport_release_buffer(drive_ssd_stats_p);
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    projected_lifetime = (drive_ssd_stats_p->eol_PE_cycles * drive_ssd_stats_p->power_on_hours) / drive_ssd_stats_p->max_erase_count;
    if (projected_lifetime < fbe_provision_drive_class_get_warranty_period())
    {

#ifdef FBE_SEND_LE_WEAR_LEVELING_EVENT_LOG_MESSAGE
        fbe_provision_drive_write_event_log(provision_drive_p,
                                            SEP_ERROR_CODE_PROVISION_DRIVE_WEAR_LEVELING_THRESHOLD);
#endif
        /* Trace some information if enabled.
         */
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "WPD projected lifetime: 0x%llx hours mec: 0x%llx poh: 0x%llx epc: 0x%llx warranty hrs: 0x%llx\n",
                                projected_lifetime,  
                                drive_ssd_stats_p->max_erase_count,
                                drive_ssd_stats_p->power_on_hours, 
                                drive_ssd_stats_p->eol_PE_cycles, 
                                fbe_provision_drive_class_get_warranty_period());
    }

    /* reset the timer
     */
    fbe_provision_drive_lock(provision_drive_p);
    fbe_provision_drive_set_wear_leveling_query_time(provision_drive_p, fbe_get_time());
    fbe_provision_drive_unlock(provision_drive_p);
 
    /* Release the control buffer and control operation
     */
    fbe_transport_release_buffer(drive_ssd_stats_p);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);

    return status;
}
/******************************************************************************
 * end fbe_raid_group_monitor_update_max_wear_level_completion()
 ******************************************************************************/

/***********************************
 * end fbe_provision_drive_monitor.c
 ***********************************/



