/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the raid group object lifecycle code.
 * 
 *  This includes the
 *  @ref fbe_raid_group_monitor_entry "raid_group monitor entry point", as well as all
 *  the lifecycle defines such as rotaries and conditions, along with the actual condition
 *  functions.
 * 
 * @ingroup raid_group_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/
 
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_raid_group.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_library.h"
#include "fbe_raid_verify.h"
#include "fbe_transport_memory.h"
#include "fbe_raid_group_needs_rebuild.h"       //  for fbe_raid_group_needs_rebuild_handle_mark_nr_
#include "fbe_raid_group_rebuild.h"             //  for fbe_raid_group_rebuild_get_drive_needing_rebuild, etc         
#include "fbe_cmi.h"
#include "fbe_raid_group_logging.h"
#include "fbe_raid_group_bitmap.h"
#include "fbe_parity_write_log.h"
#include "fbe_notification.h"
#include "fbe_private_space_layout.h"
#include "fbe_virtual_drive_private.h"
#include "fbe/fbe_event_log_api.h"                      //  for fbe_event_log_write
#include "fbe/fbe_event_log_utils.h"                    //  for message codes
#include "fbe_raid_group_expansion.h"

/*!***************************************************************
 * fbe_raid_group_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the raid group's monitor.
 *
 * @param object_handle - This is the object handle, or in our case the pdo.
 * @param packet_p - The packet arriving from the monitor scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.             
 *
 * @author
 *  5/23/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_raid_group_monitor_entry(fbe_object_handle_t object_handle, 
                              fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_raid_group_t * raid_group_p = NULL;

    raid_group_p = (fbe_raid_group_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_base_config_monitor_crank_object(&fbe_raid_group_lifecycle_const, 
                                        (fbe_base_object_t*)raid_group_p, packet_p);
    return status;
}
/******************************************
 * end fbe_raid_group_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the raid group.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the raid group's constant
 *                        lifecycle data.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(raid_group));
}
/******************************************
 * end fbe_raid_group_monitor_load_verify()
 ******************************************/


/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t raid_group_negotiate_block_size_cond_function(fbe_base_object_t * object_p, 
                                                                     fbe_packet_t * packet_p);

/* initialize the metadata memory for the raid group object. */
static fbe_lifecycle_status_t fbe_raid_group_metadata_memory_init_cond_function(fbe_base_object_t * object_p,
                                                                                fbe_packet_t * packet_p);

/* raid group object metadata memory initialization completion function. */
static fbe_status_t fbe_raid_group_metadata_memory_init_cond_completion(fbe_packet_t * packet_p,
                                                                        fbe_packet_completion_context_t context);

/* raid group object metadata write default completion function. */
static fbe_status_t fbe_raid_group_metadata_write_default_cond_completion(fbe_packet_t * packet_p,
                                                                          fbe_packet_completion_context_t context);

/* raid group metadata verify condition function. */
static fbe_lifecycle_status_t fbe_raid_group_metadata_verify_cond_function(fbe_base_object_t * object,
                                                                            fbe_packet_t * packet_p);
/* raid group metadata initialization condition function. */
static fbe_lifecycle_status_t fbe_raid_group_metadata_element_init_cond_function(fbe_base_object_t * object_p,
                                                                                 fbe_packet_t * packet_p);

/* raid group metadata eleme nt init condition completion function. */
static fbe_status_t fbe_raid_group_metadata_element_init_cond_completion(fbe_packet_t * packet_p,
                                                                         fbe_packet_completion_context_t context);

static fbe_status_t fbe_raid_group_journal_flush_completion(fbe_packet_t * packet_p,
                                                               fbe_packet_completion_context_t in_context);

/* Initialize the object's nonpaged metadata memory. */
static fbe_lifecycle_status_t fbe_raid_group_nonpaged_metadata_init_cond_function(fbe_base_object_t * object_p,
                                                                                       fbe_packet_t * packet_p);

/* raid group nonpaged metadata init condition completion function. */
static fbe_status_t fbe_raid_group_nonpaged_metadata_init_cond_completion(fbe_packet_t * packet_p,
                                                                               fbe_packet_completion_context_t context);

/* raid group zero consumed area condition zero out the metadata area before it writes paged metadata. */
static fbe_lifecycle_status_t fbe_raid_group_zero_metadata_cond_function(fbe_base_object_t * object_p,
                                                                              fbe_packet_t * packet_p);

/* raid group write default paged metadata condition completion function. */
static fbe_status_t fbe_raid_group_write_default_paged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                                                     fbe_packet_completion_context_t context);

/* write the default paged metadata for the raid group object during object initialization. */
static fbe_lifecycle_status_t fbe_raid_group_write_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p,
                                                                                           fbe_packet_t * packet_p);
/* Persist the default nonpaged metadata */
static fbe_lifecycle_status_t fbe_raid_group_persist_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* raid group write default nonpaged metadata condition completion function. */
static fbe_status_t fbe_raid_group_write_default_nonpaged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                                                   fbe_packet_completion_context_t context);

fbe_lifecycle_status_t fbe_raid_group_eval_mark_nr_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_raid_group_eval_rb_logging_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_eval_rl_activate_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_raid_group_passive_wait_for_ds_edges_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t raid_group_active_allow_join_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t raid_group_passive_request_join_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t raid_group_join_sync_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t raid_group_peer_death_release_sl_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p);

/* update rebuild checkpoint condition */
fbe_lifecycle_status_t fbe_raid_group_monitor_update_rebuild_checkpint_cond_function(fbe_base_object_t * in_object_p,
                                                                                     fbe_packet_t * in_packet_p);

/* process the event queue condition */
fbe_lifecycle_status_t fbe_raid_group_monitor_process_event_queue_cond_function(fbe_base_object_t * in_object_p,
                                                                                fbe_packet_t * in_packet_p);

/* Mark metadata region for reconstruction condition */
fbe_lifecycle_status_t raid_group_metadata_mark_incomplete_write_cond_function(
                                fbe_base_object_t * in_object_p,
                                fbe_packet_t * in_packet_p);

/* raid group checking if it needs to wake up and sniff after being in hibernation for a while*/
static fbe_lifecycle_status_t raid_group_hibernation_sniff_wakeup_cond_function(fbe_base_object_t* in_object_p,
                                                                                fbe_packet_t*      in_packet_p);

static fbe_lifecycle_status_t raid_group_quiesce_cond_function(fbe_base_object_t * object_p, 
                                                               fbe_packet_t * packet_p);

static fbe_lifecycle_status_t raid_group_unquiesce_cond_function(fbe_base_object_t * object_p, 
                                                                 fbe_packet_t * packet_p);

static fbe_lifecycle_status_t fbe_raid_group_monitor_journal_flush_cond_function(fbe_base_object_t *in_object_p,
                                                                                    fbe_packet_t * in_packet_p);

static fbe_lifecycle_status_t fbe_raid_group_monitor_journal_remap_cond_function(fbe_base_object_t *in_object_p,
                                                                                 fbe_packet_t * in_packet_p);
static fbe_status_t fbe_raid_group_monitor_journal_remap_completion(fbe_packet_t * in_packet_p, 
                                                                    fbe_packet_completion_context_t in_context);

static fbe_lifecycle_status_t raid_group_stripe_lock_start_cond_function(fbe_base_object_t * object_p, 
                                                                         fbe_packet_t * packet_p);

static fbe_status_t raid_group_stripe_lock_start_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

fbe_lifecycle_status_t fbe_raid_group_monitor_establish_edge_states_cond_function(
                                            fbe_base_object_t*      in_object_p,
                                            fbe_packet_t*           in_packet_p);
fbe_lifecycle_status_t fbe_raid_group_monitor_validate_edge_capacity_cond_function(
                                            fbe_base_object_t*      in_object_p,
                                            fbe_packet_t*           in_packet_p);
fbe_lifecycle_status_t fbe_raid_group_monitor_run_verify(fbe_base_object_t* in_object_p,
                                                                      fbe_packet_t* in_packet_p);
fbe_lifecycle_status_t fbe_raid_group_monitor_run_metadata_verify(fbe_base_object_t*  in_object_p,
                                                                  fbe_packet_t*       in_packet_p);

fbe_status_t fbe_raid_group_metadata_verify_np_lock_completion(fbe_packet_t * in_packet_p,                                    
                                                               fbe_packet_completion_context_t context);

fbe_lifecycle_status_t fbe_raid_group_monitor_mark_specific_nr_cond_function(
                                fbe_base_object_t*      in_object_p,
                                fbe_packet_t*           in_packet_p);
fbe_lifecycle_status_t fbe_raid_group_monitor_run_metadata_rebuild(
                                fbe_base_object_t*      in_object_p,
                                fbe_packet_t*           in_packet_p);
fbe_lifecycle_status_t fbe_raid_group_monitor_run_rebuild(fbe_base_object_t * in_object_p, fbe_packet_t * in_packet_p);
fbe_lifecycle_status_t fbe_raid_group_monitor_set_rebuild_checkpoint_to_end_marker_cond_function(
                                fbe_base_object_t*      in_object_p,
                                fbe_packet_t*           in_packet_p);
fbe_lifecycle_status_t fbe_raid_group_clear_rl_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_raid_group_setup_for_paged_metadata_reconstruct_cond_function(fbe_base_object_t * object_p, 
                                                         fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_raid_group_paged_metadata_reconstruct_cond_function(fbe_base_object_t * object_p, 
                                                         fbe_packet_t * packet_p);
static fbe_lifecycle_status_t raid_group_setup_for_verify_paged_metadata_cond_function(fbe_base_object_t * object_p, 
                                                                                       fbe_packet_t * packet_p);
static fbe_lifecycle_status_t raid_group_verify_paged_metadata_cond_function(fbe_base_object_t * object_p, 
                                                                             fbe_packet_t * packet_p);
fbe_status_t raid_group_verify_paged_metadata(fbe_raid_group_t *raid_group_p,
                                              fbe_packet_t * packet_p);
static fbe_status_t raid_group_verify_paged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                                     fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t raid_group_packet_cancelled_cond_function(fbe_base_object_t * object_p, 
                                                                        fbe_packet_t * packet_p);
fbe_lifecycle_status_t 
fbe_raid_group_pending_func(fbe_base_object_t *base_object_p, fbe_packet_t * packet);
static fbe_lifecycle_status_t raid_group_clear_errors_cond_function(fbe_base_object_t* in_object_p, fbe_packet_t* in_packet_p);
static fbe_lifecycle_status_t fbe_raid_group_background_monitor_operation_cond_function(fbe_base_object_t * object_p,
                                                                                        fbe_packet_t * packet_p);
static fbe_lifecycle_status_t raid_group_quiesced_background_operation_cond_function(fbe_base_object_t * object_p,
                                                                                     fbe_packet_t * packet_p);

static fbe_lifecycle_status_t raid_group_downstream_health_broken_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

//  Completion function for setting the rebuild checkpoint(s) to the logical marker
static fbe_status_t fbe_raid_group_rebuild_process_set_checkpoint_to_end_marker_completion(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_packet_completion_context_t in_context);

static fbe_lifecycle_status_t raid_group_zero_verify_chkpt_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_status_t raid_group_zero_verify_chkpt_cond_function_completion(
                                                        fbe_packet_t*    in_packet_p, 
                                                        fbe_packet_completion_context_t in_context);

static fbe_lifecycle_status_t raid_group_eval_download_request_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t raid_group_keys_requested_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_keys_requested_completion(fbe_packet_t * packet_p,
                                                             fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t raid_group_mark_nr_for_md_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_status_t raid_group_mark_nr_for_md_cond_function_completion(fbe_packet_t*    in_packet_p, 
                                                        fbe_packet_completion_context_t in_context);

static fbe_status_t fbe_raid_group_background_op_check_slf(fbe_raid_group_t * raid_group_p);


static fbe_bool_t fbe_raid_group_time_to_start_download(fbe_raid_group_t *raid_group_p);


static fbe_status_t fbe_raid_group_push_keys_downstream_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                              fbe_memory_completion_context_t context);
static fbe_status_t fbe_raid_group_push_keys_downstream_subpacket_completion(fbe_packet_t * packet_p,
                                                                             fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t raid_group_rekey_operation_cond_function(fbe_base_object_t * object_p,
                                                                       fbe_packet_t * packet_p);

static fbe_lifecycle_status_t fbe_raid_group_configuration_change_cond_function(fbe_base_object_t * object, 
                                                                                fbe_packet_t * packet_p);

static fbe_lifecycle_status_t raid_group_emeh_request_cond_function(fbe_base_object_t *object_p, fbe_packet_t *packet_p);

static fbe_lifecycle_status_t 
fbe_raid_group_background_handle_encryption_state_machine(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_raid_group_monitor_update_nonpaged_drive_tier_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_monitor_update_nonpaged_drive_tier_with_lock(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_bool_t fbe_raid_group_is_encryption_capable(fbe_raid_group_t *raid_group_p);
static fbe_lifecycle_status_t fbe_raid_group_wear_level_cond_function(fbe_base_object_t *object_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_raid_group_monitor_update_max_wear_level_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(raid_group);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(raid_group);


/*  raid_group_lifecycle_callbacks
 *  This variable has all the callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(raid_group) = 
{
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        raid_group,
        FBE_LIFECYCLE_NULL_FUNC,        /* online function */
        fbe_raid_group_pending_func)        /* pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/

/* Derived establish edge condition. */
static fbe_lifecycle_const_cond_t raid_group_establish_edge_states_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_ESTABLISH_EDGE_STATES,
        fbe_raid_group_monitor_establish_edge_states_cond_function)
};

/* FBE_BASE_CONFIG_LIFECYCLE_COND_VALIDATE_DOWNSTREAM_CAPACITY
 * The purpose of this condition is to validate the downstream edge capacity after
 * the edges have been established.
 */
static fbe_lifecycle_const_base_cond_t raid_group_validate_edge_capacity_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_VALIDATE_DOWNSTREAM_CAPACITY,
        fbe_raid_group_monitor_validate_edge_capacity_cond_function)
};

/* raid_group_negotiate_block_size_cond
 * FBE_RAID_GROUP_LIFECYCLE_COND_NEGOTIATE_BLOCK_SIZE condition. 
 * The purpose of this condition is for the raid group to send a negotiate block 
 * size packet on each of its edges and to calculate the optimal block size to be exported
 * by this raid group. 
 */
static fbe_lifecycle_const_cond_t raid_group_negotiate_block_size_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_NEGOTIATE_BLOCK_SIZE,
        raid_group_negotiate_block_size_cond_function)
};
/* raid group metadata memory initialization condition function. */
static fbe_lifecycle_const_cond_t raid_group_metadata_memory_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_MEMORY_INIT,
        fbe_raid_group_metadata_memory_init_cond_function)
};

/* raid group nonpaged metadata initialization condition function. */
static fbe_lifecycle_const_cond_t raid_group_nonpaged_metadata_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_NONPAGED_METADATA_INIT,
        fbe_raid_group_nonpaged_metadata_init_cond_function)         
};

/* raid group metadata metadata element init condition function. */
static fbe_lifecycle_const_cond_t raid_group_metadata_element_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_ELEMENT_INIT,
        fbe_raid_group_metadata_element_init_cond_function)         
};

/* raid group metadata metadata verification condition function. */
static fbe_lifecycle_const_cond_t raid_group_metadata_verify_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_VERIFY,
        fbe_raid_group_metadata_verify_cond_function)
};


/* raid group zero metadata condition.  This condition will zero any
 * non-user space for the raid group this includes:
 *  o non-paged metadata area
 *  o journal area (only allocated for parity raid groups)
 */
static fbe_lifecycle_const_cond_t raid_group_zero_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_ZERO_CONSUMED_AREA,
        fbe_raid_group_zero_metadata_cond_function)
};

/* raid group metadata metadata element init condition function. */
static fbe_lifecycle_const_cond_t raid_group_write_default_paged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_PAGED_METADATA,
        fbe_raid_group_write_default_paged_metadata_cond_function)
};


/* raid group metadata metadata element init condition function. */
static fbe_lifecycle_const_cond_t raid_group_write_default_nonpaged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_NONPAGED_METADATA,
        fbe_raid_group_write_default_nonpaged_metadata_cond_function)         
};

/* raid group metadata default nonpaged persist condition function. */
static fbe_lifecycle_const_cond_t raid_group_persist_default_nonpaged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_PERSIST_DEFAULT_NONPAGED_METADATA,
        fbe_raid_group_persist_default_nonpaged_metadata_cond_function)
};

/* The raid group will complete packets from the transport server queues and from the
 * termination queue. 
 */
static fbe_lifecycle_const_cond_t raid_group_packet_cancelled_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_OBJECT_LIFECYCLE_COND_PACKET_CANCELED,
        raid_group_packet_cancelled_cond_function)
};

static fbe_lifecycle_const_cond_t raid_group_hibernation_sniff_wakeup_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_HIBERNATION_SNIFF_WAKEUP_COND,
        raid_group_hibernation_sniff_wakeup_cond_function)
};

static fbe_lifecycle_const_cond_t raid_group_quiesce_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE,
        raid_group_quiesce_cond_function)
};

static fbe_lifecycle_const_cond_t raid_group_unquiesce_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE,
        raid_group_unquiesce_cond_function)
};

static fbe_lifecycle_const_cond_t raid_group_passive_request_join_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_PASSIVE_REQUEST_JOIN,
        raid_group_passive_request_join_cond_function)
};

static fbe_lifecycle_const_cond_t raid_group_active_allow_join_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_ACTIVE_ALLOW_JOIN,
        raid_group_active_allow_join_cond_function)
};

/* raid group common background monitor operation condition function. */
static fbe_lifecycle_const_cond_t raid_group_background_monitor_operation_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_BACKGROUND_MONITOR_OPERATION,
        fbe_raid_group_background_monitor_operation_cond_function)
};

static fbe_lifecycle_const_cond_t raid_group_downstream_health_broken_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN,
        raid_group_downstream_health_broken_cond_function)
};

static fbe_lifecycle_const_cond_t raid_group_downstream_health_disabled_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED,
        raid_group_downstream_health_disabled_cond_function)
};

static fbe_lifecycle_const_cond_t raid_group_join_sync_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_JOIN_SYNC,
        raid_group_join_sync_cond_function)
};

static fbe_lifecycle_const_cond_t raid_group_peer_death_release_sl_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_PEER_DEATH_RELEASE_SL,
        raid_group_peer_death_release_sl_function)
};


/*--- constant base condition entries --------------------------------------------------*/
/*
 * FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_FLUSH_JOUNRAL -
 * 
 * The purpose of this condition is to evaluate if write journal entries need to be
 * flushed before any rebuild can begin. 
 */
static fbe_lifecycle_const_base_cond_t raid_group_eval_journal_flush_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_eval_journal_flush_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_JOURNAL_FLUSH,
        FBE_LIFECYCLE_NULL_FUNC),
    
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_INVALID,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_INVALID,         /* READY */
        FBE_LIFECYCLE_STATE_INVALID,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_INVALID,         /* OFFLINE */       
        FBE_LIFECYCLE_STATE_INVALID,         /* FAIL */         
        FBE_LIFECYCLE_STATE_INVALID)         /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_DO_FLUSH_JOURNAL
 * The purpose of this condition is to flush journaled writes from 
 * a monitor thread.
 */
static fbe_lifecycle_const_base_cond_t raid_group_journal_flush_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_journal_flush_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_JOURNAL_FLUSH,
        fbe_raid_group_monitor_journal_flush_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,      /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,           /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,       /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_JOURNAL_REMAP
 * This condition is set when write_log flush sees media errors.
 * Purpose of this condition is to perform remaps of write_log area.
 */
static fbe_lifecycle_const_base_cond_t raid_group_journal_remap_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_journal_remap_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_JOURNAL_REMAP,
        fbe_raid_group_monitor_journal_remap_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,   /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,     /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,        /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,    /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,      /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)      /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING
 *
 * The purpose of this condition is to evaluate if rb logging needs to be initiated.
 */
static fbe_lifecycle_const_base_cond_t raid_group_eval_rb_logging_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_eval_rb_logging_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING,
        fbe_raid_group_eval_rb_logging_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING_ACTIVATE
 *
 * The purpose of this condition is to evaluate if rb logging needs to be initiated.
 * This is needed since the eval in activate only occurs on the active side,
 * and only takes signatures and downstream health into consideration.
 */
static fbe_lifecycle_const_base_cond_t raid_group_eval_rb_logging_activate_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_eval_rb_logging_activate_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING_ACTIVATE,
        fbe_raid_group_eval_rl_activate_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_UPDATE_REBUILD_CHECKPOINT -
 *
 * The purpose of this condition is to update the rebuild checkpoint if needed, this condition
 * is used to update the rebuild checkpoint when downmstream object sends an data event to mark
 * the certain chunks as needs rebuild
 */
static fbe_lifecycle_const_base_cond_t raid_group_update_rebuild_checkpoint_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_update_rebuild_checkpoint_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_UPDATE_REBUILD_CHECKPOINT,
        fbe_raid_group_monitor_update_rebuild_checkpint_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_MARK_NR
 *
 * The purpose of this condition is to evaluate the NR marking of the drive before we 
 * decide whether we need to clear rebuild logging or mark NR on the entire drive.
 */
static fbe_lifecycle_const_base_cond_t raid_group_eval_mark_nr = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_eval_mark_nr",
        FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_MARK_NR,
        fbe_raid_group_eval_mark_nr_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_CLEAR_RB_LOGGING
 *
 * The purpose of this condition is to clear rb logging.  This condition will be triggered when an
 * edge that is rebuild logging becomes enabled.
 *
 */
static fbe_lifecycle_const_base_cond_t raid_group_clear_rb_logging_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_clear_rb_logging_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_CLEAR_RB_LOGGING,
        fbe_raid_group_clear_rl_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_MARK_SPECIFIC_NR -
 * 
 * The purpose of this condition is to mark NR on an LBA range.  The condition will be triggered by an
 * event from the VD which will specify the LBA range.  The event is sent in the following situations:
 * 
 *   - when a media error has occurred: NR needs to be marked on the LBAs that had the error
 */
static fbe_lifecycle_const_base_cond_t raid_group_mark_specific_nr_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_mark_specific_nr_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_MARK_SPECIFIC_NR,
        fbe_raid_group_monitor_mark_specific_nr_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_SET_REBUILD_CHECKPOINT_TO_END_MARKER -
 * 
 * The purpose of this condition is move the rebuild checkpoint for a disk(s) to the logical end marker.  I/O
 * needs to be quiesced when doing so, which is why this is in a condition. 
 * 
 */
static fbe_lifecycle_const_base_cond_t raid_group_set_rebuild_checkpoint_to_end_marker_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_set_rebuild_checkpoint_to_end_marker_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_SET_REBUILD_CHECKPOINT_TO_END_MARKER,
        fbe_raid_group_monitor_set_rebuild_checkpoint_to_end_marker_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


/* FBE_RAID_GROUP_LIFECYCLE_COND_DEST_DRIVE_FAILED_SET_REBUILD_CHECKPOINT_TO_END_MARKER -
 * 
 * The purpose of this condition is to take the necessary action when the raid group
 * copy operation fails.  Currently there is nothing to do for the raid group and
 * thus is a NULL function
 * 
 */
static fbe_lifecycle_const_base_cond_t raid_group_dest_drive_failed_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_dest_drive_failed_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_DEST_DRIVE_FAILED_SET_REBUILD_CHECKPOINT_TO_END_MARKER,
        FBE_LIFECYCLE_NULL_FUNC),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* HIBERNATE --> ACTIVATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_CONFIGURATION_CHANGE -
 * 
 * The purpose of this condition is to take the necessary action when the raid group
 * changes configuration mode.  Currently there is nothing to do for the raid group and
 * thus is a NULL function
 * 
 */
static fbe_lifecycle_const_base_cond_t raid_group_configuration_change_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_configuration_change_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_CONFIGURATION_CHANGE,
        fbe_raid_group_configuration_change_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* HIBERNATE --> ACTIVATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* raid_group_clear_errors_cond
 * FBE_RAID_GROUP_LIFECYCLE_COND_clear_errors 
 * The purpose of this condition is to clear any errors marked on the downstream edge. 
 */
static fbe_lifecycle_const_base_cond_t raid_group_clear_errors_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_clear_errors_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_CLEAR_ERRORS,
        raid_group_clear_errors_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_VERIFY_PAGED_METADATA -
 *
 * The purpose of this condition is to verify the paged metadata.
 * The condition function is implemented in the derived class and not in this base class. 
 */
static fbe_lifecycle_const_base_cond_t raid_group_verify_paged_metadata_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_verify_paged_metadata",
        FBE_RAID_GROUP_LIFECYCLE_COND_VERIFY_PAGED_METADATA,
        raid_group_verify_paged_metadata_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_SETUP_FOR_PAGED_METADATA_RECONSTRUCT
 *
 * The purpose of this condition is to setup for verification of the paged metadata.
 */
static fbe_lifecycle_const_base_cond_t raid_group_setup_for_paged_metadata_reconstruct_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_setup_for_paged_metadata_reconstruct_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_SETUP_FOR_PAGED_METADATA_RECONSTRUCT,
        fbe_raid_group_setup_for_paged_metadata_reconstruct_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_PAGED_METADATA_RECONSTRUCT
 *
 * The purpose of this condition is reconstruct the paged metadata using the
 * non-paged.
 */
static fbe_lifecycle_const_base_cond_t raid_group_paged_metadata_reconstruct_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_paged_metadata_reconstruct_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_PAGED_METADATA_RECONSTRUCT,
        fbe_raid_group_paged_metadata_reconstruct_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_SETUP_FOR_VERIFY_PAGED_METADATA -
 *
 * The purpose of this condition is to setup for verification of the paged metadata.
 */
static fbe_lifecycle_const_base_cond_t raid_group_setup_for_verify_paged_metadata_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_setup_for_verify_paged_metadata",
        FBE_RAID_GROUP_LIFECYCLE_COND_SETUP_FOR_VERIFY_PAGED_METADATA,
        raid_group_setup_for_verify_paged_metadata_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* Purpose of this condition is to start stripe locking with the peer. */
static fbe_lifecycle_const_cond_t raid_group_stripe_lock_start_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_STRIPE_LOCK_START,
        raid_group_stripe_lock_start_cond_function)
};


/* FBE_RAID_GROUP_LIFECYCLE_COND_PASSIVE_WAIT_FOR_DS_EDGES -
 * 
 * The purpose of this condition is to wait for downstream edges to come up before starting join request 
 */
static fbe_lifecycle_const_base_cond_t raid_group_passive_wait_for_ds_edges_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_passive_wait_ds_edges_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_PASSIVE_WAIT_FOR_DS_EDGES,
        fbe_raid_group_passive_wait_for_ds_edges_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_PASSIVE_WAIT_FOR_DS_EDGES -
 * 
 * The purpose of this condition is to wait for downstream edges to come up before starting join request 
 */
static fbe_lifecycle_const_base_cond_t raid_group_zero_verify_chkpt_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_zero_verify_chkpt_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_ZERO_VERIFY_CHECKPOINT,
        raid_group_zero_verify_chkpt_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_PASSIVE_WAIT_FOR_DS_EDGES -
 * 
 * The purpose of this condition is to wait for downstream edges to come up before starting join request 
 */
static fbe_lifecycle_const_base_cond_t raid_group_mark_nr_for_md_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_mark_nr_for_md_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_MARK_NR_FOR_MD,
        raid_group_mark_nr_for_md_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_DOWNLOAD_REQUEST -
 * 
 * The purpose of this condition is to which drive could start download. 
 */
static fbe_lifecycle_const_base_cond_t raid_group_eval_download_request_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_eval_download_request_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_DOWNLOAD_REQUEST,
        raid_group_eval_download_request_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_KEYS_REQUESTED -
 * 
 * The purpose of this condition is push the keys down to the object that requested it
 */
static fbe_lifecycle_const_base_cond_t raid_group_keys_requested_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_keys_requested_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_KEYS_REQUESTED,
        raid_group_keys_requested_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_MD_MARK_INCOMPLETE_WRITE -
 *
 * The purpose of this condition is to mark metdata region for veify  if needed, 
 * this condition is used to mark MD for error verify when remap is requested
 * by IO on metdata region.
 */
static fbe_lifecycle_const_base_cond_t raid_group_metadata_mark_incomplete_write_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_metadata_mark_incomplete_write_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_MD_MARK_INCOMPLETE_WRITE,
        raid_group_metadata_mark_incomplete_write_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,      /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,           /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,            /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,            /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY    */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_QUIESCED_BACKGROUND_OPERATION -
 *
 * For some raid group types (like raw mirrors) we
 * perform the background operations while quiesced.
 * This is needed since there are some unlocked I/Os that are performed.
 */
static fbe_lifecycle_const_base_cond_t raid_group_quiesced_background_operation_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_quiesced_background_operation_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_QUIESCED_BACKGROUND_OPERATION,
        raid_group_quiesced_background_operation_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,      /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,           /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,       /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,            /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY    */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_REKEY_OPERATION -
 *
 * During the start and end of a rekey we need to perform some operations under quiesce.
 */
static fbe_lifecycle_const_base_cond_t raid_group_rekey_operation_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_rekey_operation_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_REKEY_OPERATION,
        raid_group_rekey_operation_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,      /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,           /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,       /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,            /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY    */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_EMEH_REQUEST -
 * 
 * The purpose of this condition is to process an extended media error handling (EMEH)
 * request.  The request can be to have all drives in the raid group enter `high
 * availability' mode due to the raid group becoming degraded.
 */
static fbe_lifecycle_const_base_cond_t raid_group_emeh_request_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "raid_group_emeh_request_cond",
        FBE_RAID_GROUP_LIFECYCLE_COND_EMEH_REQUEST,
        raid_group_emeh_request_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_WEAR_LEVEL -
 * 
 * The purpose of this condition is to process the max wear level
 * of the drives in the raid group
 *
 */
static fbe_lifecycle_const_base_timer_cond_t raid_group_wear_level_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "raid_group_wear_level_cond",
            FBE_RAID_GROUP_LIFECYCLE_COND_WEAR_LEVEL,
            fbe_raid_group_wear_level_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,         /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
     60 * 60 * 24 * 8 * 100 /* fires every 8 days */
};



/* raid_group_lifecycle_base_cond_array
 * This is our static list of base conditions.
 * This list is order-dependent - it must match the same order as the fbe_raid_group_lifecycle_cond_id_e.
 */
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(raid_group)[] = 
{
    &raid_group_eval_journal_flush_cond,
    &raid_group_journal_flush_cond,
    &raid_group_journal_remap_cond,
    &raid_group_eval_rb_logging_cond, 
    &raid_group_update_rebuild_checkpoint_cond,    
    &raid_group_mark_specific_nr_cond,
    &raid_group_clear_errors_cond,    
    &raid_group_clear_rb_logging_cond,
    &raid_group_eval_mark_nr,
    &raid_group_set_rebuild_checkpoint_to_end_marker_cond,
    &raid_group_dest_drive_failed_cond,
    &raid_group_configuration_change_cond,
    &raid_group_setup_for_paged_metadata_reconstruct_cond,
    &raid_group_paged_metadata_reconstruct_cond,
    &raid_group_setup_for_verify_paged_metadata_cond,
    &raid_group_verify_paged_metadata_cond,
    &raid_group_eval_rb_logging_activate_cond,
    &raid_group_passive_wait_for_ds_edges_cond,
    &raid_group_mark_nr_for_md_cond,
    &raid_group_zero_verify_chkpt_cond,
    &raid_group_eval_download_request_cond,
    &raid_group_metadata_mark_incomplete_write_cond,
    &raid_group_quiesced_background_operation_cond,
    &raid_group_rekey_operation_cond,
    &raid_group_keys_requested_cond,
    &raid_group_emeh_request_cond,
    (fbe_lifecycle_const_base_cond_t *)&raid_group_wear_level_cond,
};

/* raid_group_lifecycle_base_conditions 
 *  This is the list of base default base conditions for the raid group
 */
FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(raid_group);


/*--- constant rotaries ----------------------------------------------------------------*/


/*  Specialize rotary */
static fbe_lifecycle_const_rotary_cond_t raid_group_specialize_rotary[] = 
{
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_metadata_memory_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_nonpaged_metadata_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_metadata_element_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_stripe_lock_start_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_negotiate_block_size_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_establish_edge_states_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),  
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_validate_edge_capacity_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /*!@note following four condition needs to be in order, do not change the order without valid reason */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_write_default_nonpaged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_zero_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_write_default_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_persist_default_nonpaged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /*!@note previous four condition needs to be in order, do not change the order without valid reason */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_metadata_verify_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_mark_nr_for_md_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_zero_verify_chkpt_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    /* Base conditions. */
};

/*  Activate rotary */
static fbe_lifecycle_const_rotary_cond_t raid_group_activate_rotary[] = 
{
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_downstream_health_disabled_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_eval_rb_logging_activate_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_peer_death_release_sl_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_setup_for_paged_metadata_reconstruct_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_paged_metadata_reconstruct_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_setup_for_verify_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_verify_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_eval_journal_flush_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_journal_flush_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_journal_remap_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_passive_wait_for_ds_edges_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* This condition is intentionally at the end of the activate rotary since we are coordinating the transition of a
     * passive object to ready.
     * We are clearing FBE_RAID_GROUP_LOCAL_STATE_ACTIVATE inside this condition based on this assumption.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_passive_request_join_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

/*  Ready rotary */
static fbe_lifecycle_const_rotary_cond_t raid_group_ready_rotary[] = 
{
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_join_sync_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_packet_cancelled_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_quiesce_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_configuration_change_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_eval_rb_logging_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_update_rebuild_checkpoint_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_clear_rb_logging_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_set_rebuild_checkpoint_to_end_marker_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_dest_drive_failed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_quiesced_background_operation_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_rekey_operation_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Flushes should complete before we let peer join */
    /* The journal flush and the peer death release sl conditions need to run before eval mark nr.
     * Moreover the release sl needs to be before unquiesce so we don't have I/Os waiting on peer when we drop the peer 
     * locks. Unquiesce could find peer back and we could be waiting on peer when the drop occurs.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_journal_flush_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_peer_death_release_sl_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_unquiesce_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),


    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_clear_errors_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_eval_mark_nr, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_mark_specific_nr_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_journal_remap_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),    

    /* This condition is intentionally after all conditions that require peer to peer coordination.
     * This allows the peer to join us without interfering with any of the above conditions. 
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_active_allow_join_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_eval_download_request_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_keys_requested_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_metadata_mark_incomplete_write_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_emeh_request_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_wear_level_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    
    /* This is always the last condition in the ready rotary.  This condition is never cleared. 
     * Please, do put any condition after this one !!
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_background_monitor_operation_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    
    
};

static fbe_lifecycle_const_rotary_cond_t raid_group_hibernate_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_peer_death_release_sl_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_hibernation_sniff_wakeup_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
    
};

static fbe_lifecycle_const_rotary_cond_t raid_group_fail_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_peer_death_release_sl_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(raid_group_downstream_health_broken_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Base conditions */
    
};

/*  List of defined rotaries */
static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(raid_group)[] = 
{
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, raid_group_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, raid_group_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, raid_group_ready_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_HIBERNATE, raid_group_hibernate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_FAIL, raid_group_fail_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(raid_group);


/*--- global raid_group lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(raid_group) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        raid_group,
        FBE_CLASS_ID_RAID_GROUP,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_config))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/

/*--- Condition Functions --------------------------------------------------------------*/
fbe_status_t raid_group_check_downstream_edges_not_invalid(fbe_base_config_t * base_config,
                                                           fbe_raid_geometry_t* raid_geometry_p)
{
    fbe_u32_t num_found_edges = 0;
    fbe_u32_t edge_index = 0;
    fbe_block_edge_t *edge_p = NULL;
    fbe_u16_t   parity_disks;

    fbe_raid_geometry_get_parity_disks(raid_geometry_p, &parity_disks);
    for (edge_index = 0; edge_index < base_config->width; edge_index++) 
    {
        fbe_base_config_get_block_edge(base_config, &edge_p, edge_index);

        if (edge_p->base_edge.path_state != FBE_PATH_STATE_INVALID)
        {
            num_found_edges++;
        }
    }
    /*! @todo This is a hack that is needed until the raid group  
     *        knows how to 1) Wait for all the edges and 2) mark its metadata for rebuild.
     */
    if (base_config->base_object.class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        if (num_found_edges == 0)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    /* Checking whether the number of edges found is enough to bring the RG 
       online or not
    */
    /*!@todo this is just a workaround for now    
    */
    else if (!(num_found_edges >= (base_config->width - parity_disks)))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

static fbe_status_t raid_group_check_downstream_edges_enabled(fbe_raid_group_t *raid_group_p,
                                                       fbe_packet_t *packet_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           num_found_edges = 0;
    fbe_u32_t           num_of_edges_to_wait_for = 0;
    fbe_u32_t           edge_index = 0;
    fbe_block_edge_t   *edge_p = NULL;
    fbe_base_config_t  *base_config = (fbe_base_config_t *)raid_group_p;

    for (edge_index = 0; edge_index < base_config->width; edge_index++) 
    {
        fbe_base_config_get_block_edge(base_config, &edge_p, edge_index);

        if (edge_p->base_edge.path_state == FBE_PATH_STATE_ENABLED)
        {
            num_found_edges++;
        }
    }

    /* If this is the 
     */
    if (base_config->base_object.class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        /* Determine (based on configuration mode) how many virtual drive edges
         * we should wait for.
         */
        status = fbe_raid_group_get_number_of_virtual_drive_edges_to_wait_for(raid_group_p, packet_p,
                                                                              &num_of_edges_to_wait_for);
        if (status != FBE_STATUS_OK)
        {
            /* Just wait */
            return status;
        }
        else if (num_found_edges < num_of_edges_to_wait_for)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else if (num_found_edges != base_config->width)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * raid_group_negotiate_block_size_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function for raid group to calculate the
 *  block size info for the raid group from the edges.
 * 
 *  Note that we do not allow this condition to get cleared
 *  until all the edges have established themselves (are not invalid).
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING   
 *
 * @author
 *  9/18/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_lifecycle_status_t 
raid_group_negotiate_block_size_cond_function(fbe_base_object_t * object_p, 
                                              fbe_packet_t * packet_p)
{   
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_block_edge_geometry_t block_edge_geometry;
    fbe_block_size_t physical_block_size;
    fbe_block_size_t optimal_block_size;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    fbe_base_config_get_downstream_geometry((fbe_base_config_t *)raid_group_p, &block_edge_geometry);

    if (block_edge_geometry == FBE_BLOCK_EDGE_GEOMETRY_INVALID)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_raid_geometry_set_flag(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_NEGOTIATE_COMPLETE);
    fbe_block_transport_get_optimum_block_size(block_edge_geometry, &optimal_block_size);
    fbe_block_transport_get_physical_block_size(block_edge_geometry, &physical_block_size);
    
    status = fbe_raid_geometry_set_block_sizes(raid_geometry_p,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      physical_block_size, /* Physical block size */
                                      optimal_block_size);
    if (status == FBE_STATUS_OK)
    {
        status = raid_group_check_downstream_edges_not_invalid(&raid_group_p->base_config, raid_geometry_p);
        if (status == FBE_STATUS_OK)
        {
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%x\n",
                                      __FUNCTION__, status);
            }
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't set block sizes status: 0x%x\n",
                                __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************
 * end raid_group_negotiate_block_size_cond_function()
 ******************************************/

/*!**************************************************************
 * raid_group_quiesce_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function for raid group to kick off
 *  the fetching of negotiate block size information.
 *
 * @param raid_group_p - current object.
 * @param packet_p - monitor packet
 * @param b_is_pass_thru_object - FBE_FALSE (typical), this is a non-pass-thru
 *          object and thus all I/Os flow thru the terminator.
 *          FBE_TRUE - This is a pass-thur object, I/Os don't flow thru
 *          the terminator.
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING   
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_lifecycle_status_t fbe_raid_group_quiesce_cond_function(fbe_raid_group_t *raid_group_p, 
                                                            fbe_packet_t * packet_p,
                                                            fbe_bool_t b_is_pass_thru_object)
{   
    fbe_status_t                status;
    fbe_bool_t                  is_io_request_quiesce_b = FBE_FALSE;
    fbe_raid_geometry_t        *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    /*! @note In order to support usurper metadata operations we will postpone
     *        the quiesce request until the usurper request is complete.
     */
    fbe_raid_group_check_hook(raid_group_p,
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE,  
                              FBE_RAID_GROUP_SUBSTATE_QUIESCE_START, 
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If we are draining we will neither quiesce the write log nor the metadata operations. 
     * Note that we cannot use fbe_base_config_should_quiesce_drain() since 
     * at this point the peer might not have set hold. 
     * Thus we rely on our own hold flag for this check. 
     * If the peer starts drain and does not have the hold flag set we will clear it 
     * and then we will go through the below code to do a regular quiesce. 
     */
    if (!fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)raid_group_p, 
                                              FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD)) {
        /* if this is a parity object, we need to quiesce the write log */
        if (fbe_raid_geometry_is_raid3(raid_geometry_p)
            || fbe_raid_geometry_is_raid5(raid_geometry_p) 
            || fbe_raid_geometry_is_raid6(raid_geometry_p)) {
            /* Quiesce the write log -- this will delete the queue and lock the parity group log */
            fbe_parity_write_log_quiesce(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p);
        }

        /* Quiesce any metadata ops since they might be retrying.
         * If we are draining, then just let these finish. 
         */
        fbe_raid_group_quiesce_md_ops(raid_group_p);
    }

    /* Quiesce all the i/o requests which are in progress and held the new
     * incoming request to block transport server.
     */
    fbe_base_config_quiesce_io_requests((fbe_base_config_t *)raid_group_p, packet_p, b_is_pass_thru_object, &is_io_request_quiesce_b);
    if(is_io_request_quiesce_b)
    {
        /* Mark everything on termination queue.  We will wait for these to finish 
         * before we clear the error attributes on the edge. 
         * Save this count in the raid group object. 
         */
        fbe_raid_group_calc_quiesced_count(raid_group_p);

        /* Trace some information if enabled.
         */
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING,
                             "raid_group: quiesce updated quiesced count:%d\n",
                             raid_group_p->quiesced_count);

        fbe_raid_group_check_hook(raid_group_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE,  
                                  FBE_RAID_GROUP_SUBSTATE_QUIESCE_DONE, 
                                  0, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

        /* Since we already quiesce i/o operation clear the quiesce condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)raid_group_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
    }
    else
    {
        /* Reschedule soon since I/Os are completing. */
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const,
                                 (fbe_base_object_t*)raid_group_p,
                                 50);
    }

    /* We are always returning DONE since we are not initiating packets. */
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************
 * end fbe_raid_group_quiesce_cond_function()
 ******************************************/

/*!**************************************************************
 * raid_group_quiesce_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function for raid group to kick off
 *  the fetching of negotiate block size information.
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING   
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_lifecycle_status_t raid_group_quiesce_cond_function(fbe_base_object_t * object_p, 
                                                               fbe_packet_t * packet_p)
{ 
    /* Simply invoke the common quiesce method (used by both raid group and
     * virtual drive objects).
     */
    return (fbe_raid_group_quiesce_cond_function((fbe_raid_group_t *)object_p, packet_p,
                                                 FBE_FALSE /* Raid group is not a `pass-thru' object*/));
}
/******************************************
 * end raid_group_quiesce_cond_function()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_get_error_edge_position()
 ****************************************************************
 * @brief
 *  Fetch the first edge position that is marked with an error.
 *
 * @param raid_group_p - Raid group to scan edges for.
 * @param  position_p - output position with edge marked in error. 
 *
 * @return status fbe_status_t 
 *         FBE_STATUS_OK - When edge found.
 *         FBE_STATUS_ATTRIBUTE_NOT_FOUND - When edge with this attribute not found.  
 *
 * @author
 *  2/8/2011 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_get_error_edge_position(fbe_raid_group_t *raid_group_p,
                                                           fbe_raid_position_t *position_p)
{
    fbe_block_edge_t *block_edge_p = NULL;
    fbe_u32_t width;
    fbe_raid_position_t position;
    fbe_path_attr_t path_attrib;

    fbe_raid_group_get_width(raid_group_p, &width);

    /* Find the first position in error and return that.
     */
    for (position = 0; position < width; position++)
    {
        fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &block_edge_p, position);
        fbe_block_transport_get_path_attributes(block_edge_p, &path_attrib);

        if (path_attrib & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS)
        {
            *position_p = position;
            return FBE_STATUS_OK;
        }
    }
    return FBE_STATUS_ATTRIBUTE_NOT_FOUND;
}
/**************************************
 * end fbe_raid_group_get_error_edge_position()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_send_clear_errors_usurper()
 ****************************************************************
 * @brief
 *  This function sends a clear errors usurper to the
 *  passed in edge position.
 *
 * @param  packet_p - packet to use to send usurper.
 * @param  position - fru position to send for.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  2/8/2011 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_send_clear_errors_usurper(fbe_raid_group_t *raid_group_p,
                                                             fbe_packet_t *packet_p,
                                                             fbe_raid_position_t position)
{
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_block_edge_t *block_edge_p = NULL;

    /* Raid always sends to the downstream edge, which is connected with a sep object,
     * so use a sep payload.
     */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_BLOCK_TRANSPORT_CONTROL_CODE_CLEAR_LOGICAL_ERRORS,
                                        NULL,
                                        0);
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_NO_ATTRIB);
    /* We do not set a completion function since this is assumed to be a monitor packet. 
     * The monitor will scan for edges that need this sent and clear the condition if 
     * none are found. 
     */

    fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &block_edge_p, position);

    /* Send packet.  Our callback is always called.
     */
    fbe_block_transport_send_control_packet(block_edge_p, packet_p);
    
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_send_clear_errors_usurper()
 **************************************/
/*!**************************************************************
 * raid_group_clear_errors_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function for raid group to clear error
 *  flags that may have been set on the edge.
 *
 *  The flag gets set on the edge when we detect certain
 *  error cases.  e.g. timed out I/Os.  And raid will consider the
 *  edge with this flag broken until the flag is cleared.
 *  We always clear the flag after the drain of I/Os so that 
 *  in flight I/Os will rebuild log.  Clearing the flag
 *  allows the monitor to see the actual edge state.
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING   
 *
 * @author
 *  2/7/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_lifecycle_status_t 
raid_group_clear_errors_cond_function(fbe_base_object_t * object_p, 
                                      fbe_packet_t * packet_p)
{   
    fbe_raid_position_t position;
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    status = fbe_raid_group_get_error_edge_position(raid_group_p, &position);

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_ERRORS,
                               FBE_RAID_GROUP_SUBSTATE_CLEAR_ERRORS_ENTRY, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if (status == FBE_STATUS_ATTRIBUTE_NOT_FOUND) 
    { 
        /* An edge with this error attribute was not found. 
         * Clear the condition. 
         */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        fbe_u32_t count = 0;

        /* Mark everything on termination queue.  We will wait for these to finish 
         * before we clear the error attributes on the edge. 
         * Save this count in the raid group object. 
         */
        /*fbe_raid_group_calc_quiesced_count(raid_group_p, &count);
        fbe_raid_group_update_quiesced_count(raid_group_p, count);*/
        fbe_raid_group_get_quiesced_count(raid_group_p, &count);

        /* Trace some information if enabled.
         */
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING,
                             "raid_group: clear errors updated quiesced count:%d\n",
                             raid_group_p->quiesced_count);
        if (count != 0)
        {
            /* We only want to clear after the previously quiesced requests have finished.
             */
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        else
        {
            /* Send this usurper on the edge.
             * The next time we are rescheduled we will scan again. 
             */
            fbe_raid_group_send_clear_errors_usurper(raid_group_p, packet_p, position);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
    }
}
/******************************************
 * end raid_group_clear_errors_cond_function()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_unquiesce_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition that will restart I/Os
 *  that were previously quiesced.  
 *
 * @param raid_group_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING   
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_lifecycle_status_t fbe_raid_group_unquiesce_cond_function(fbe_raid_group_t *raid_group_p, 
                                                              fbe_packet_t * packet_p)
{   
    fbe_status_t         status;
    fbe_bool_t           is_io_request_unquiesce_ready_b = FBE_FALSE;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_base_config_metadata_memory_t * metadata_memory_ptr = NULL;
    fbe_base_config_metadata_memory_t * metadata_memory_peer_ptr = NULL;
    fbe_bool_t b_is_active;
    fbe_bool_t journal_committed;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    /*! @note In order to support usurper metadata operations we will postpone
     *        the quiesce request until the usurper request is complete.
     */
    fbe_raid_group_check_hook(raid_group_p,
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_UNQUIESCE,  
                              FBE_RAID_GROUP_SUBSTATE_UNQUIESCE_START, 
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_metadata_element_get_metadata_memory(&raid_group_p->base_config.metadata_element, (void **)&metadata_memory_ptr);
    fbe_metadata_element_get_peer_metadata_memory(&raid_group_p->base_config.metadata_element, (void **)&metadata_memory_peer_ptr);

    if (!fbe_raid_group_is_degraded(raid_group_p))
    {
        fbe_block_transport_server_clear_path_attr_all_servers(&((fbe_base_config_t *)raid_group_p)->block_transport_server,
                                                               FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED);

    }


    /* Must ensure that we are fully drained before updating the block size.
     * I/Os that are quiesced do not expect the block size to change. 
     * Note that we know that if we still have hold set, then the peer had it set also when 
     * quiesce finished, so we know both sides have drained. 
     */
    if (fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)raid_group_p, 
                                              FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD)) {
        /* As we unquiesce, refresh the bitmap of 4k drives in case we have different block sizes.
         */
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_geometry_refresh_block_sizes(raid_geometry_p);
        fbe_raid_group_unlock(raid_group_p);

        fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_BACKGROUND_CONDITION, 
                                  FBE_RAID_GROUP_SUBSTATE_BACKGROUND_COND_BL_REFRESHED, 0, &hook_status);
    }

    fbe_raid_group_metadata_is_journal_committed(raid_group_p, &journal_committed);

    /* If the journal is committed but we have not updated other information, updated it now..*/
    if ((FBE_TRUE == fbe_raid_geometry_is_parity_type(raid_geometry_p))&& journal_committed && 
        !fbe_raid_geometry_journal_is_flag_set(raid_geometry_p, FBE_PARITY_WRITE_LOG_FLAGS_4K_SLOT))
    {
        fbe_raid_geometry_init_journal_write_log(raid_geometry_p);
    }

    /* As we unquiesce, refresh the bitmap of 4k drives in case we have different block sizes.
     */
    fbe_raid_geometry_refresh_block_sizes(raid_geometry_p);
    
    /* Always trace the entry
     */    
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)raid_group_p);
    if (!b_is_active &&
        fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t *) raid_group_p, 
                                                   FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD)){
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "unquiesce hold passive wait peer 0x%llx/0x%llx\n",
                              (unsigned long long)((metadata_memory_ptr) ? metadata_memory_ptr->flags : 0),
                              (unsigned long long)((metadata_memory_peer_ptr) ? metadata_memory_peer_ptr->flags : 0));
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_raid_group_update_write_log_for_encryption(raid_group_p);

    /* if the raid group is degraded and not ssd, then set the attribute to bump up the queue ratio*/
    if (fbe_raid_group_is_degraded(raid_group_p) && 
        !fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_MIRROR_SEQUENTIAL_DISABLED))
    {
        block_transport_server_set_attributes(&raid_group_p->base_config.block_transport_server,
                                              FBE_BLOCK_TRANSPORT_ENABLE_DEGRADED_QUEUE_RATIO);
    } 
    else 
    {
        block_transport_server_clear_attributes(&raid_group_p->base_config.block_transport_server,
                                                FBE_BLOCK_TRANSPORT_ENABLE_DEGRADED_QUEUE_RATIO);
    }

    /* if this is a parity object, we need to unquiesce the write log */
    if (fbe_raid_geometry_is_raid3(raid_geometry_p)
        || fbe_raid_geometry_is_raid5(raid_geometry_p) 
        || fbe_raid_geometry_is_raid6(raid_geometry_p))
    {
        /* Quiesce the write log -- this will delete the queue and lock the parity group log */
        fbe_parity_write_log_unquiesce(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p);
    }

    /* Unquiesce all the request which is waiting for the restart. */
    status = fbe_base_config_unquiesce_io_requests((fbe_base_config_t *)raid_group_p, packet_p, &is_io_request_unquiesce_ready_b);
    if(is_io_request_unquiesce_ready_b)
    {
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed while doing unquiesce of pending i/o operation, status: 0x%x\n",
                                  __FUNCTION__, status);
        }

        fbe_raid_group_check_hook(raid_group_p,
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_UNQUIESCE,  
                              FBE_RAID_GROUP_SUBSTATE_UNQUIESCE_DONE, 
                              0, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) 
        {
            return FBE_LIFECYCLE_STATUS_DONE;
        } 

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unquiesced.\n",
                                  __FUNCTION__);
        }
    }
    else
    {
        /* Reschedule soon since peer may be getting in sync */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: %s reschedule unquiesce request\n",
                              __FUNCTION__);
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const,
                                 (fbe_base_object_t*)raid_group_p,
                                 50);
    }

    /* We are always returning DONE since we are not initiating packets. */
    return FBE_LIFECYCLE_STATUS_DONE;
}
/***********************************************
 * end fbe_raid_group_unquiesce_cond_function()
 ***********************************************/

/*!**************************************************************
 * raid_group_unquiesce_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition that will restart I/Os
 *  that were previously quiesced.  
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING   
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_lifecycle_status_t raid_group_unquiesce_cond_function(fbe_base_object_t * object_p, 
                                                                 fbe_packet_t * packet_p)
{
    /* Simply invoke the common raid group unquiesce.
     */   
    return (fbe_raid_group_unquiesce_cond_function((fbe_raid_group_t *)object_p, packet_p));
}
/******************************************
 * end raid_group_unquiesce_cond_function()
 ******************************************/

/*!***************************************************************************
 *          fbe_raid_group_reconstruct_required_completion()
 ***************************************************************************** 
 * 
 * @brief   This is the completion function after setting the `reconstruct required'
 *          in the non-page is complete.
 *
 * @param   packet_p - pointer to a control packet from the scheduler.
 * @param   context  - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  03/08/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_reconstruct_required_completion(fbe_packet_t *packet_p,
                                                            fbe_packet_completion_context_t context)
{
    fbe_raid_group_t                   *raid_group_p = NULL;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    raid_group_p = (fbe_raid_group_t *)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* Clear setup for paged metadata reconstruct.  The reconstruct paged
         * metadata condition is a preset which is the next condition in the
         * activate rotary.
         */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)raid_group_p);

        /* Set the condition to perform the paged metadata reconstruction
         */
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                               (fbe_base_object_t *)raid_group_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_PAGED_METADATA_RECONSTRUCT);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: set reconstruct required failed - status: 0x%x control status: 0x%x\n",
                              status, control_status);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_reconstruct_required_completion()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_setup_for_paged_metadata_reconstruct_cond_function()
 *****************************************************************************
 *
 * @brief   Setup to run a paged metadata reconstruct for the given raid group.
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE   
 *
 * @author
 *  03/08/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_raid_group_setup_for_paged_metadata_reconstruct_cond_function(fbe_base_object_t * object_p, 
                                                         fbe_packet_t * packet_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_group_t           *raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_raid_geometry_t        *raid_geometry_p = NULL;
    fbe_raid_group_type_t       raid_type;
    fbe_bool_t                  b_is_reconstruct_set;
    fbe_object_id_t             rg_object_id;
    fbe_raid_group_number_t     raid_group_id;
    fbe_scheduler_hook_status_t hook_status;

    /* Get geometry and raid type
     */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);

    /* Determine if this raid group has already been marked for paged metadata
     * reconstruction or not.
     */
    fbe_raid_group_metadata_is_paged_metadata_reconstruct_required(raid_group_p, &b_is_reconstruct_set);

    /* Get the raid group number and the raid group object ID
     * adjusted for RAID-10 if needed.
     */
    fbe_raid_group_logging_get_raid_group_number_and_object_id(raid_group_p,
                                                               &raid_group_id,
                                                               &rg_object_id);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: setup for paged metadata reconstruct cond raid id: %d type: %d is reconstruct set: %d\n",
                          raid_group_id, raid_type, b_is_reconstruct_set);

    /* If a debug hook is set, we need to execute the specified action.
     */
    fbe_raid_group_check_hook(raid_group_p,
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                              FBE_RAID_GROUP_SUBSTATE_PAGED_RECONSTRUCT_SETUP, 
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* A raid 10 does not have paged metadata.
     */
    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p) ||
         (raid_type == FBE_RAID_GROUP_TYPE_RAID10)                )
    {
        /* Currently no recovery for RAID-10
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s - unexpected raid type: %d\n",
                              __FUNCTION__, raid_type);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Determine if this raid group has already been marked for paged metadata
     * reconstruction or not.
     */
    if (b_is_reconstruct_set == FBE_FALSE)
    {
        /* Generate an event stating that we need to reconstruct the paged metadata.
         */
        fbe_event_log_write(SEP_ERROR_RAID_GROUP_METADATA_RECONSTRUCT_REQUIRED, NULL, 0, 
                            "%d %d %d", raid_group_id, rg_object_id, raid_type);

        /* Need to set the `reconstruct required' flag in the non-paged.
         */
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_reconstruct_required_completion, raid_group_p);

        /* Set the `reconstruct required' in the non-paged.
         */
        fbe_raid_group_metadata_set_paged_metadata_reconstruct_required(raid_group_p, packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* Clear setup for paged metadata reconstruct.  The reconstruct paged
     * metadata condition is a preset which is the next condition in the
     * activate rotary.
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)raid_group_p);

    /* Done.
     */
    return FBE_LIFECYCLE_STATUS_DONE;
}
/*************************************************************************
 * end fbe_raid_group_setup_for_paged_metadata_reconstruct_cond_function()
 *************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_reconstruct_paged_metadata_completion()
 ***************************************************************************** 
 * 
 * @brief   This is the completion function after setting the `reconstruct paged'
 *          condition is complete.
 *
 * @param   packet_p - pointer to a control packet from the scheduler.
 * @param   context  - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  03/08/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_reconstruct_paged_metadata_completion(fbe_packet_t *packet_p,
                                                            fbe_packet_completion_context_t context)
{
    fbe_raid_group_t                   *raid_group_p = NULL;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    raid_group_p = (fbe_raid_group_t *)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* Clear setup for paged metadata reconstruct.
         */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)raid_group_p);

    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: set reconstruct paged failed - status: 0x%x control status: 0x%x\n",
                              status, control_status);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_reconstruct_paged_metadata_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_reconstruct_paged_metadata_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the reconstruct paged
 *  metadata condition
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  03/08/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_reconstruct_paged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                          fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_group_t                   *raid_group_p = NULL;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_scheduler_hook_status_t         hook_status;
    
    raid_group_p = (fbe_raid_group_t*)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* If a debug hook is set, we need to execute the specified action.
     */
    fbe_raid_group_check_hook(raid_group_p,
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                              FBE_RAID_GROUP_SUBSTATE_PAGED_RECONSTRUCT_COMPLETE, 
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_STATUS_OK;
    }

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        fbe_base_config_encryption_mode_t encryption_mode;
        fbe_base_config_get_encryption_mode((fbe_base_config_t*)raid_group_p, &encryption_mode);
        if ((encryption_mode > FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED) &&
            fbe_raid_group_bg_op_is_flag_set(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_RECONSTRUCT_PAGED)){
            /* Do not clear anything until our bg op is cleared.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "rg_reconstruct_paged_mdata_cond_compl continue during enc stat:0x%X,ctrl_stat:0x%X\n",
                                  status, control_status);
            return FBE_STATUS_OK;
        }
        /* clear paged metadata write default condition if status is good. */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) raid_group_p);

        /* Now clear the `reconstruct paged required' flag in the non-paged.
         */
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_reconstruct_paged_metadata_completion, raid_group_p);

        /* Set the `reconstruct required' in the non-paged.
         */
        fbe_raid_group_metadata_clear_paged_metadata_reconstruct_required(raid_group_p, packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_reconstruct_paged_mdata_cond_compl write def paged metadata cond fail,stat:0x%X,ctrl_stat:0x%X\n",
                              status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_reconstruct_paged_metadata_cond_completion()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_paged_metadata_reconstruct_cond_function()
 *****************************************************************************
 *
 * @brief   If the `paged reconstruct required' bit in the non-paged is set
 *          it indicates that we need to reconstruct the paged metadata.
 *
 * @param   object_p - current object.
 * @param   packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE   
 *
 * @note    This is a preset condition.
 *
 * @author
 *  03/08/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_raid_group_paged_metadata_reconstruct_cond_function(fbe_base_object_t * object_p, 
                                                         fbe_packet_t * packet_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_group_t           *raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_bool_t                  b_is_active;
    fbe_bool_t                  b_is_paged_reconstruct_required = FBE_FALSE;
    fbe_raid_geometry_t        *raid_geometry_p = NULL;
    fbe_raid_group_type_t       raid_type;
    fbe_scheduler_hook_status_t hook_status;
    fbe_base_config_encryption_mode_t encryption_mode;
    
    /* Always trace the entry
     */    
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)raid_group_p);
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);
    /* When we go down while reconstructing the paged we need to pick up again where we left off 
     * before we can go ready. 
     */
    if (b_is_active &&
        fbe_raid_group_encryption_is_rekey_needed(raid_group_p) &&
        fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_NEEDS_RECONSTRUCT)) {
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_encrypt_paged_completion, raid_group_p);
        if (object_p->class_id == FBE_CLASS_ID_VIRTUAL_DRIVE) {
            fbe_raid_group_write_paged_metadata(packet_p, raid_group_p);
        } else {
            fbe_raid_group_zero_paged_metadata(packet_p, raid_group_p);
        }
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    /* Check if we need to reconstruct the paged metadata or not.
     */
    fbe_raid_group_metadata_is_paged_metadata_reconstruct_required(raid_group_p, &b_is_paged_reconstruct_required);

    /* If paged reconstruction is not required clear this condition and
     * return done.
     */
    if (b_is_paged_reconstruct_required == FBE_FALSE)
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: %s raid type: %d cond paged metadata reconstruct required set \n",
                          (b_is_active) ? "Active" : "Passive", raid_type);

    /* If a debug hook is set, we need to execute the specified action.
     */
    fbe_raid_group_check_hook(raid_group_p,
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                              FBE_RAID_GROUP_SUBSTATE_PAGED_RECONSTRUCT_START, 
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Only the active should run this condition.
     */
    if (b_is_active == FBE_FALSE)
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    /* set the completion function for the reconstruct paged condition.
     * If the bg op flag is set, then this completion just lets the monitor run again. 
     *  
     * Otherwise this completion will clear the np bit that indicates paged needs reconstruct. 
     * This completion also clears the condition.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_reconstruct_paged_metadata_cond_completion, raid_group_p);

    /* During encryption this condition kicks off a state machine that we might need to run.
     */
    fbe_base_config_get_encryption_mode((fbe_base_config_t*)raid_group_p, &encryption_mode);
    if ((encryption_mode > FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED) &&
        fbe_raid_group_bg_op_is_flag_set(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_RECONSTRUCT_PAGED)) {
        fbe_raid_group_encrypt_reconstruct_sm(raid_group_p, packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* Invoke the method to reconstruct the paged metadata using the non-paged.
     */
    status = fbe_raid_group_metadata_reconstruct_paged_metadata(raid_group_p, packet_p);

    return FBE_LIFECYCLE_STATUS_PENDING;
}
/***************************************************************
 * end fbe_raid_group_paged_metadata_reconstruct_cond_function()
 ***************************************************************/

/*!**************************************************************
 * raid_group_setup_for_verify_paged_metadata_cond_function()
 ****************************************************************
 * @brief
 *  Setup to run a verify of the paged metadata for the given raid group.
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE   
 *
 * @author
 *  01-2012 - Created. Susan Rundbaken
 *
 ****************************************************************/

static fbe_lifecycle_status_t 
raid_group_setup_for_verify_paged_metadata_cond_function(fbe_base_object_t * object_p, 
                                                         fbe_packet_t * packet_p)
{
    fbe_status_t                                status;
    fbe_raid_group_t *                          raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_raid_geometry_t*                        geo_p = NULL;
    fbe_bool_t                                  is_active_b;
    fbe_chunk_count_t                           chunk_count;
    fbe_u32_t                                   chunk_index;
    fbe_raid_group_paged_metadata_t *           paged_md_md_p;
    fbe_raid_group_nonpaged_metadata_t*         nonpaged_metadata_p = NULL;
    fbe_lba_t                                   paged_md_start_lba = FBE_LBA_INVALID;
    fbe_raid_geometry_t*                        raid_geometry_p = NULL;
    fbe_u16_t                                   data_disks = 0;

    geo_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    /*  Check if the RG is active on this SP or not.  If not, then we don't need to do anything more - return here
     *  and the monitor will reschedule on its normal cycle.
     */
    fbe_raid_group_monitor_check_active_state(raid_group_p, &is_active_b);
    if (is_active_b == FBE_FALSE)
        {
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s can't clear current condition, status: 0x%x\n",
                                      __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* A raid 10 does not have paged metadata, all the paged metadata is in the mirror objects. 
     * There is no reason at the moment for the virtual drive to perform this check either. 
     */
    if ((object_p->class_id == FBE_CLASS_ID_VIRTUAL_DRIVE) ||
        (geo_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10))
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry\n", __FUNCTION__);

    /* Turn on the "read-write verify" bit in the nonpaged metadata of metadata.
     * This will allow us to kick off a verify of the paged metadata when that 
     * condition runs.
     */

    /*  First determine how many chunks of paged metadata we have for this RG.
     */
    fbe_raid_group_bitmap_get_md_of_md_count(raid_group_p, &chunk_count);

    /*  Get a pointer to the paged metadata of metadata.
     */
    paged_md_md_p = fbe_raid_group_get_paged_metadata_metadata_ptr(raid_group_p);

    /*  Set the paged metadata of metadata verify bits in memory only.
     */
    for (chunk_index = 0; chunk_index < chunk_count; chunk_index++)
    {
        paged_md_md_p[chunk_index].verify_bits |= FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE;
    }

    // Get the paged metadata start lba.
    fbe_base_config_metadata_get_paged_record_start_lba(&(raid_group_p->base_config),
                                                        &paged_md_start_lba);

    // Get the number of data disks in this raid object
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);    

    /* Set the verify checkpoint to start of paged metadata. 
     * We do not persist checkpoint for activate state verify.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

    /* Raw mirrors always do a full verify of the entire capacity. 
     * Set the checkpoint to 0 now, the verify will occur when we get to ready. 
     */
    if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p))
    {
        nonpaged_metadata_p->incomplete_write_verify_checkpoint = 0;
    }
    else
    {
        nonpaged_metadata_p->incomplete_write_verify_checkpoint = paged_md_start_lba / data_disks;
    }

    /* Clear the condition.
     */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s chunks: 0x%x verify bits: 0x%x icw chkpt: 0x%llx \n",
                              __FUNCTION__, chunk_count, paged_md_md_p[0].verify_bits,
                              (unsigned long long)nonpaged_metadata_p->incomplete_write_verify_checkpoint);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/***************************************************************
 * end raid_group_setup_for_verify_paged_metadata_cond_function()
 ***************************************************************/
/*!**************************************************************
 * fbe_raid_group_setup_passive_encryption_state()
 ****************************************************************
 * @brief
 *  At init time, synchronize the encryption state.
 *
 * @param raid_group_p - Current RG.               
 *
 * @return fbe_status_t
 *
 * @author
 *  4/10/2014 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_setup_passive_encryption_state(fbe_raid_group_t *raid_group_p)
{
    fbe_lba_t rekey_checkpoint;
    fbe_lba_t exported_capacity;
    fbe_bool_t b_identical_keys;
    fbe_base_config_encryption_state_t encryption_state;
    fbe_base_config_encryption_mode_t encryption_mode;

    rekey_checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    exported_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);
    b_identical_keys = fbe_raid_group_has_identical_keys(raid_group_p);
    fbe_base_config_get_encryption_state((fbe_base_config_t *)raid_group_p, &encryption_state);
    fbe_base_config_get_encryption_mode((fbe_base_config_t *)raid_group_p, &encryption_mode);

    if (b_identical_keys && 
        (rekey_checkpoint == exported_capacity)) {
        fbe_base_config_set_encryption_state((fbe_base_config_t *)raid_group_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: passive setup enc state %d -> REKEY_COMPLETE_PUSH_DONE md: %d chkpt: %llx\n",
                              encryption_state, encryption_mode, rekey_checkpoint);
    } else if (b_identical_keys && 
               (rekey_checkpoint == FBE_LBA_INVALID)) {
        fbe_base_config_set_encryption_state((fbe_base_config_t *)raid_group_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_REMOVE_OLD_KEY);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: passive setup enc state %d -> REKEY_COMPLETE_REMOVE_OLD_KEY md: %d chkpt: %llx\n",
                              encryption_state, encryption_mode, rekey_checkpoint);
    } else if (rekey_checkpoint == exported_capacity) {
        fbe_base_config_set_encryption_state((fbe_base_config_t *)raid_group_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: passive setup enc state %d -> REKEY_COMPLETE md: %d chkpt: %llx\n",
                              encryption_state, encryption_mode, rekey_checkpoint);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_setup_passive_encryption_state()
 ******************************************/
/*!**************************************************************
 * raid_group_verify_paged_metadata_cond_function()
 ****************************************************************
 * @brief
 *  We kick off a verify on the paged metadata.
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING   
 *
 * @author
 *  8/4/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_lifecycle_status_t 
raid_group_verify_paged_metadata_cond_function(fbe_base_object_t * object_p, 
                                               fbe_packet_t * packet_p)
{
    fbe_status_t                                status;
    fbe_raid_group_t *                          raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_raid_geometry_t*                        geo_p = NULL;
    fbe_bool_t                                  is_active_b;
    fbe_scheduler_hook_status_t                 hook_status = FBE_SCHED_STATUS_OK;
    fbe_base_config_encryption_mode_t           encryption_mode;
    fbe_raid_position_bitmask_t                 rl_bitmask;
      
    // If a debug hook is set, we need to execute the specified action...
    fbe_raid_group_check_hook(raid_group_p, 
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                              FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START, 0, &hook_status);

    if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

    geo_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    /*! @todo we should have code that evaluates signature and nr before we get here,  
     *        but until we get that code we need to do this since we will hold stripe locks
     *        if any edge state events occur.
     */
    status = fbe_metadata_element_restart_io(&raid_group_p->base_config.metadata_element);
    fbe_base_config_clear_clustered_flag((fbe_base_config_t *) raid_group_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED);
    fbe_base_config_clear_clustered_flag((fbe_base_config_t *) raid_group_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING);


    /* Peter Puhov. We should not do it on Passive side */      
    //  Check if the RG is active on this SP or not.  If not, then we don't need to do anything more - return here
    //  and the monitor will reschedule on its normal cycle.
    fbe_raid_group_monitor_check_active_state(raid_group_p, &is_active_b);
    if (is_active_b == FBE_FALSE)
        {
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s can't clear current condition, status: 0x%x\n",
                                      __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    /* A raid 10 does not have paged metadata, all the paged metadata is in the mirror objects. 
     * There is no reason at the moment for the virtual drive to perform this check either. 
     */
    if ((object_p->class_id == FBE_CLASS_ID_VIRTUAL_DRIVE) ||
        (geo_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10))
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry\n", __FUNCTION__);
    fbe_base_config_get_encryption_mode((fbe_base_config_t*)raid_group_p, &encryption_mode);

    status = fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);

    /* If we go through activate with rebuild logging, make sure 
     * that we mark the paged to be rebuilt. 
     * If we are inconsistent then we must force the rebuild since 
     * the verify of paged will not happen in ready when the drive comes back. 
     */
    if ((encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) &&
        !fbe_raid_group_is_metadata_marked_NR(raid_group_p, rl_bitmask)){
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Mark NR on MD of MD Activate pos_mask: 0x%x, pkt: %p\n",
                              rl_bitmask, packet_p);
        fbe_raid_group_np_set_bits(packet_p, raid_group_p, rl_bitmask);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* If paged reconstruct sm is active, then run it.
     */
    if ((encryption_mode > FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED) &&
        fbe_raid_group_bg_op_is_flag_set(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_RECONSTRUCT_PAGED)) {
        fbe_raid_group_encrypt_reconstruct_sm(raid_group_p, packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    /* See if metadata verify needs to run. 
     */
    if(fbe_raid_group_background_op_is_metadata_verify_need_to_run(raid_group_p, 
           (FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE | FBE_RAID_BITMAP_VERIFY_ERROR)))
    {
        /* Run verify operation. 
         */
        fbe_transport_set_completion_function(packet_p, raid_group_verify_paged_metadata_cond_completion, raid_group_p);
        raid_group_verify_paged_metadata(raid_group_p, packet_p);        
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    // If a debug hook is set, we need to execute the specified action...
    fbe_raid_group_check_hook(raid_group_p, 
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                              FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED, 0, &hook_status);

    if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s verify of paged metadata complete.\n",
                              __FUNCTION__);
    }

    return FBE_LIFECYCLE_STATUS_DONE;

}
/******************************************
 * end raid_group_verify_paged_metadata_cond_function()
 ******************************************/

/*!**************************************************************
 * raid_group_verify_paged_metadata()
 ****************************************************************
 * @brief
 *  This function performs raid group paged metadata verify operations.
 *
 * @param raid_group_p - pointer to the raid group.
 * @param packet_p - monitor packet
 *
 * @return fbe_status_t 
 *
 * @author
 *  01/2012 - Created.  Susan Rundbaken
 *  04/2012 - Modified  Prahlad Purohit
 *
 ****************************************************************/

fbe_status_t raid_group_verify_paged_metadata(fbe_raid_group_t *raid_group_p,
                                              fbe_packet_t * packet_p)
{
    fbe_lba_t                            metadata_verify_lba;
    fbe_traffic_priority_t               io_priority; 
    fbe_payload_block_operation_opcode_t op_code = FBE_RAID_GROUP_BACKGROUND_OP_NONE;
    fbe_chunk_size_t                     chunk_size;
    fbe_u8_t                             verify_flags;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    /*  Set the verify priority in the packet.
     */
    io_priority = fbe_raid_group_get_verify_priority(raid_group_p);
    fbe_transport_set_traffic_priority(packet_p, io_priority);    

    /*  Get the lba and type for the metadata chunk to verify.
     */
    fbe_raid_group_verify_get_next_verify_type(raid_group_p, 
                                               FBE_TRUE,    /* This is only for the paged metadata*/
                                               &verify_flags);

    fbe_raid_group_verify_get_next_metadata_verify_lba(raid_group_p,
                                                       verify_flags,
                                                       &metadata_verify_lba);
    if (metadata_verify_lba == FBE_LBA_INVALID)
    {
        if (verify_flags != 0)
        {
            /* There are no valid verify flags set, but we still have a checkpoint,
             * it is possible the flags were cleared in a rebuild. 
             * Set the checkpoint back to 0 so we can continue with the user region. 
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: no chunks marked, but checkpoint present for vr flags: 0x%x\n", 
                                  __FUNCTION__, verify_flags);
            fbe_raid_group_set_verify_checkpoint(raid_group_p, packet_p, verify_flags, 0);
        }
        else
        {
            /*  No valid flag bits are set.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: No chunk marked for verify\n",
                                  __FUNCTION__);
            fbe_transport_complete_packet(packet_p);
        }
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Get the opcode for verify flags.
     */
    op_code = fbe_raid_group_verify_get_opcode_for_verify(raid_group_p, verify_flags);
    if (op_code == (fbe_payload_block_operation_opcode_t)FBE_RAID_GROUP_BACKGROUND_OP_NONE)
    {
        /*  Could not get opcode.
         */
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /*  Get the chunk size, which is the number of blocks to verify
     */
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "Send metadata verify. Op:%d, lba:0x%llx, size:%d\n",
              op_code, (unsigned long long)metadata_verify_lba,
              chunk_size);

    /* Send the verify packet to the raid executor.
     */
    fbe_raid_group_executor_send_monitor_packet(raid_group_p, packet_p, 
                                                op_code, metadata_verify_lba, (fbe_block_count_t)chunk_size);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end raid_group_verify_paged_metadata()
 ******************************************/

/*!****************************************************************************
 * raid_group_check_paged_metadata_verify_result()
 ******************************************************************************
 * @brief
 *  This function is used to test if verify result on paged metadata is
 *  uncorrectable.
 *
 * @param raid_group_p     - pointer to a raid group
 * @param out_is_uncorrectable  - return verify result
 *
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  12/12/2013 - Created. Jamin Kang
 *
 ******************************************************************************/
static fbe_status_t raid_group_check_paged_metadata_verify_result(fbe_raid_group_t *raid_group_p,
                                                                  fbe_bool_t *out_is_uncorrectable)
{
    fbe_raid_verify_error_counts_t *verify_errors_p;
    fbe_bool_t is_uncorrectable = FBE_FALSE;

    verify_errors_p = fbe_raid_group_get_verify_error_counters_ptr(raid_group_p);
    fbe_base_object_trace(
        (fbe_base_object_t*)raid_group_p,
        FBE_TRACE_LEVEL_DEBUG_LOW,
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: coh %u, crc %u, crc_m %u, crc_s %u, md %u, ss %u, ts %u, ws %u, inv %u\n",
        __FUNCTION__,
        verify_errors_p->u_coh_count,
        verify_errors_p->u_crc_count,
        verify_errors_p->u_crc_multi_count,
        verify_errors_p->u_crc_single_count,
        verify_errors_p->u_media_count,
        verify_errors_p->u_ss_count,
        verify_errors_p->u_ts_count,
        verify_errors_p->u_ws_count,
        verify_errors_p->invalidate_count);

    is_uncorrectable =
        verify_errors_p->u_coh_count ||
        verify_errors_p->u_crc_count ||
        verify_errors_p->u_crc_multi_count ||
        verify_errors_p->u_crc_single_count ||
        verify_errors_p->u_media_count ||
        verify_errors_p->u_ss_count ||
        verify_errors_p->u_ts_count ||
        verify_errors_p->u_ws_count ||
        verify_errors_p->invalidate_count;
    *out_is_uncorrectable = is_uncorrectable;

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * raid_group_verify_paged_metadata_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the paged metadata verify.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  8/4/2010 - Created. Rob Foley
 *
 ******************************************************************************/

static fbe_status_t 
raid_group_verify_paged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                 fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qual;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lba_t   lba;
    fbe_block_count_t block_count;
    fbe_bool_t is_uncorrectable = FBE_TRUE;
    fbe_scheduler_hook_status_t hook_status;
    
    raid_group_p = (fbe_raid_group_t*)context;

    status = fbe_transport_get_status_code(packet_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* It is possible that we didn't make it to the point of issuing the block operation.
     */
    if (block_operation_p == NULL)
    {
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qual);
    fbe_payload_ex_release_block_operation(sep_payload_p, block_operation_p);

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry\n", __FUNCTION__);

    /* Check status and for now only clear the condition if status is good.
     */
    if ((status != FBE_STATUS_OK) || 
        (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                      "rg_verify_paged_mdata_cond_compl stat:0x%x blk_stat:0x%x blk_qual:0x%x\n",
                                      status, block_status, block_qual);


        if ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT) ||
            (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED))
        {
            /* We do not clear the condition since we need to wait for the object to be ready.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s block_status: 0x%x\n", __FUNCTION__, block_status);
        }
        else if (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
        {
            /*! @todo We need to add handling for uncorrectables/media errors.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s block_status: 0x%x\n", __FUNCTION__, block_status);
        }
        else if (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST)
        {
            /*! @note We issued an invalid request.  Just re-issuing the request 
             *        will not solve the issue.  We expect the re-issue of the request
             *        to determine the problem.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s block_status: 0x%x\n", __FUNCTION__, block_status);
        }
        else
        {
            /*! @todo This is an unexpected case, add some handling.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s block_status: 0x%x\n", __FUNCTION__, block_status);
        }
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_raid_group_check_hook(raid_group_p, 
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                              FBE_RAID_GROUP_SUBSTATE_VERIFY_PAGED_COMPLETE, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        /* We need to re-run the verify since we did not take action on it.
         */
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *) raid_group_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_SETUP_FOR_VERIFY_PAGED_METADATA);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Check for uncorrectable errors in the verify report.
     * This means the I/O succeeded; however, the data is not correct.
     * Will try to reconstruct the paged metadata in this case.
     * !@TODO: use a utility function for the uncorrectables check; need appropriate home for it
     */
    raid_group_check_paged_metadata_verify_result(raid_group_p, &is_uncorrectable);

    if (is_uncorrectable)
    {
        fbe_base_config_encryption_mode_t encryption_mode;

        fbe_base_config_get_encryption_mode((fbe_base_config_t*)raid_group_p, &encryption_mode);

        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s need to reconstruct paged metadata!!!\n",
                              __FUNCTION__);

        fbe_payload_block_get_lba(block_operation_p, &lba);
        fbe_payload_block_get_block_count(block_operation_p, &block_count);
        if ((encryption_mode > FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED) &&
            (encryption_mode < FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED)) {
            fbe_raid_group_encrypt_reconstruct_sm_start(raid_group_p, packet_p, lba, block_count);
        } else {
            fbe_raid_group_bitmap_reconstruct_paged_metadata(raid_group_p, 
                                                             packet_p, 
                                                             lba,
                                                             block_count, 
                                                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY);
        }
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* if we have no errors, we can have the monitor rescheduled to kick off the next verify */
    fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *)raid_group_p, 100);

    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************
 * end raid_group_verify_paged_metadata_cond_completion()
 ********************************************************/

/*!**************************************************************
 * raid_group_packet_cancelled_cond_function()
 ****************************************************************
 * @brief
 *  This will attempt to complete packets or mark them
 *  as aborted.
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE   
 *
 * @author
 *  4/9/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_lifecycle_status_t raid_group_packet_cancelled_cond_function(fbe_base_object_t * object_p, 
                                                                        fbe_packet_t * packet_p)
{   
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)object_p;

    fbe_base_object_trace((fbe_base_object_t*)object_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_block_transport_server_process_canceled_packets(&raid_group_p->base_config.block_transport_server);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed processing cancelled packets, status: 0x%x\n",
                              __FUNCTION__, status);
    }

    /*! We also scan the termination queue and mark iots as aborted and 
     * abort fruts that are outstanding. 
     */
    status = fbe_raid_group_handle_aborted_packets(raid_group_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s error from handle aborted packets status: 0x%x\n",
                              __FUNCTION__, status);
    }

    // Start temporary code
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Clearing RG condition\n",
                          __FUNCTION__); 
    // End   temporary code
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }
    /* We are always returning DONE since we are not initiating packets.
     */
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************
 * end raid_group_packet_cancelled_cond_function()
 ******************************************/

/*!****************************************************************************
 * fbe_raid_group_monitor_establish_edge_states_cond_function()
 ******************************************************************************
 * @brief
 *   This condition function will be invoked only in the Specialize state.  It
 *   will check the edge states to make sure that the edges have finished 
 *   coming up.  Once they have, the object can move to the next state.  Otherwise
 *   if an edge is still in an initializing state, the condition will remain set 
 *   and the RAID object will stay in Specialize.
 * 
 * @param in_object_p               - pointer to a base object 
 * @param in_packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success/
 *                                    condition was cleared
 *                                  - FBE_LIFECYCLE_STATUS_DONE when condition is
 *                                    not cleared 
 * @author
 *   03/12/2010 - Created. Jean Montes.
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_monitor_establish_edge_states_cond_function(
                                            fbe_base_object_t*      in_object_p,
                                            fbe_packet_t*           in_packet_p)
{

    fbe_raid_group_t*                       raid_group_p;           // pointer to the raid group 
    fbe_base_config_path_state_counters_t   path_state_counters;    // summary of path states for all edges
    fbe_u16_t                               parity_disks;
    fbe_u32_t                               width; 
    fbe_raid_geometry_t*                    raid_geometry_p;
    fbe_raid_position_t                     position;
    fbe_status_t                            status = FBE_STATUS_OK;

    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_object_p);

    //  Cast the base object to a raid object
    raid_group_p = (fbe_raid_group_t*) in_object_p;

    //  Get the error edge position and clear any bits set on the edge
    status = fbe_raid_group_get_error_edge_position(raid_group_p, &position);
    if (status != FBE_STATUS_ATTRIBUTE_NOT_FOUND) 
    {
        // We have error bits on the edge to clear
        fbe_raid_group_send_clear_errors_usurper(raid_group_p, in_packet_p, position);

        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_parity_disks(raid_geometry_p, &parity_disks);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    /* Determine the downstream edge states.
     */
    fbe_raid_geometry_refresh_block_sizes(raid_geometry_p);

    //  Get the path counters, which contain the number of edges in each of the possible states
    fbe_base_config_get_path_state_counters((fbe_base_config_t*) raid_group_p, &path_state_counters);

    //  If enough edges are not there, then we want to remain in the specialize state -
    //  return here and wait before rescheduling 
    /*!@todo this is just a workaround for now    
    */
    if (!(path_state_counters.enabled_counter >= (width - parity_disks)))
    {
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, in_object_p, 100);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  The edges have finished initializing, but we may still be missing one or two.
    //  Note that when we get to activate we will give the missing drive(s) a few more seconds to come up
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_NEED_RL_DRIVE_WAIT);
    fbe_raid_group_unlock(raid_group_p);

    // Now clear the condition and reschedule the monitor.
    fbe_lifecycle_clear_current_cond(in_object_p);

    return FBE_LIFECYCLE_STATUS_RESCHEDULE;

} // End fbe_raid_group_monitor_establish_edge_states_cond_function()

/*!****************************************************************************
 *          fbe_raid_group_monitor_validate_edge_capacity_cond_function()
 ******************************************************************************
 *
 * @brief   This condition function will be invoked only in the Specialize state.  
 *          It will validate that the capacity of the edge is sufficient for the
 *          raid group.
 * 
 * @param   in_object_p               - pointer to a base object 
 * @param   in_packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_DONE when condition is
 *                                    not cleared 
 * @author
 *  12/01/2011  Ron Proulx - Created.
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_monitor_validate_edge_capacity_cond_function(
                                            fbe_base_object_t*      in_object_p,
                                            fbe_packet_t*           in_packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_raid_group_t                   *raid_group_p;
    fbe_raid_group_metadata_positions_t raid_group_metadata_positions;
    fbe_raid_geometry_t *raid_geometry_p = NULL;

    /* Trace function entry 
     */
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_object_p);

    /*  Cast the base object to a raid object
     */
    raid_group_p = (fbe_raid_group_t*)in_object_p;
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    /* Get the metadata position information. 
     */
    status = fbe_raid_group_get_metadata_positions(raid_group_p, 
                                                   &raid_group_metadata_positions);
    if (status != FBE_STATUS_OK)
    {
        /* Failed to get metadata position.  Stay in this state and trace the
         * error.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s failed to get metadata positions, status: 0x%x\n",
                              __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Validate the downstream capacity using the required blocks per disk.
     */
    status = fbe_raid_group_class_validate_downstream_capacity(raid_group_p,
                                                               raid_group_metadata_positions.imported_blocks_per_disk);
    if (status != FBE_STATUS_OK)
    {
        /* Validation of downstream capacity failed.  Trace an error
         * and remain in this state.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s validation of downstream capacity failed, status: 0x%x\n",
                              __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    /* We intentionally do not throttle for mirror under striper since for 
     * RAID-10 we only throttle at the striper. 
     */
    if ((raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
        (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1) ||
        (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) ||
        (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID5) ||
        (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID3) ||
        (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6))
    {
        /* This bandwidth throttle is disabled for now until we decide we need it. */
        //fbe_block_transport_server_set_io_throttle_max(&raid_group_p->base_config.block_transport_server,
        //                                               raid_geometry_p->width *
        //                                               FBE_RAID_GROUP_DEFAULT_THROTTLE_PER_DRIVE);
        if (fbe_raid_group_class_get_throttle_enabled()) {
            fbe_object_id_t object_id;
            fbe_u32_t queue_depth;
            /* Set this high enough so we will not throttle on io max.  This needs to be set for the weight to be evaluated.
             */
            fbe_block_transport_server_set_outstanding_io_max(&raid_group_p->base_config.block_transport_server,
                                                              raid_geometry_p->width * 500);
            fbe_base_object_get_object_id((fbe_base_object_t*)raid_group_p, &object_id);
            fbe_raid_group_class_get_queue_depth(object_id, raid_geometry_p->width, &queue_depth);

            if (fbe_raid_geometry_is_raid10(raid_geometry_p)) {
                /* The width at the striper is the number of mirrors. 
                 * We need to double the queue depth to consider the actual number of drives. 
                 */
                queue_depth = queue_depth * 2;
            }

            /* We want to have larger Q depth for SSD's */
            if(fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_MIRROR_SEQUENTIAL_DISABLED)){
                queue_depth = queue_depth * 2;
            }

            fbe_block_transport_server_set_io_credits_max(&raid_group_p->base_config.block_transport_server,
                                                          queue_depth);
        }
    }

    /* Else move on to next state.
     */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;

} // End fbe_raid_group_monitor_validate_edge_capacity_cond_function()

/*!**************************************************************
 * fbe_raid_group_monitor_run_metadata_verify()
 ****************************************************************
 * @brief
 *  This function initiate the raid group metadata verify operations.
 *
 * @param in_object_p - The pointer to the raid group.
 * @param in_packet_p - The control packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING 
 *
 * @author
 *  01/06/2012 - Modified Ashok Tamilarasan
 *
 ****************************************************************/
fbe_lifecycle_status_t fbe_raid_group_monitor_run_metadata_verify(fbe_base_object_t*  in_object_p,
                                                                  fbe_packet_t*       in_packet_p)
{
    fbe_raid_group_t*                    raid_group_p;   
    fbe_bool_t                           can_proceed = FBE_TRUE;
    fbe_scheduler_hook_status_t          status;
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_object_p);

    // Cast the base object to a raid object
    raid_group_p = (fbe_raid_group_t*)in_object_p;

    fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY, 
                              FBE_RAID_GROUP_SUBSTATE_START_METADATA_VERIFY, 0, &status);
    if (status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

    /* If we are quiesced/quiescing exit this condition. */
    if (fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t *)raid_group_p, 
                                (FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED|FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING))) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s quiesced or quiescing, defer MD verify\n", __FUNCTION__);

        /* Just return done.  Since we haven't cleared the condition we will be re-scheduled. */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  See if we are allowed to do a verify I/O based on the current load.  If not, just return here and the 
    //  monitor will reschedule on its normal cycle.
    fbe_raid_group_verify_check_permission_based_on_load(raid_group_p, &can_proceed);
    if (can_proceed == FBE_FALSE) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s The load on downstream edges is higher\n", __FUNCTION__);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Set time stamp so we can use it to detect slow I/O.
     */
    fbe_transport_set_physical_drive_io_stamp(in_packet_p, fbe_get_time());
    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_monitor_run_verify_completion, raid_group_p);
    fbe_raid_group_get_NP_lock(raid_group_p, in_packet_p, fbe_raid_group_metadata_verify_np_lock_completion);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
// End fbe_raid_group_monitor_run_metadata_verify()


/*!**************************************************************
 * fbe_raid_group_metadata_verify_np_lock_completion()
 ****************************************************************
 * @brief
 *  This function performs raid group metadata verify operations.
 *
 * @param in_object_p - The pointer to the raid group.
 * @param in_packet_p - The control packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING 
 *
 * @author
 *  01/06/2012 - Modified Ashok Tamilarasan
 *  04/21/2012 - Modified Prahlad Purohit
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_metadata_verify_np_lock_completion(fbe_packet_t * in_packet_p,                                    
                                                               fbe_packet_completion_context_t context)
{
    fbe_raid_group_t*                    raid_group_p = NULL;            
    fbe_traffic_priority_t               io_priority; 
    fbe_lba_t                            metadata_verify_lba;
    fbe_lba_t                            verify_checkpoint;
    fbe_lba_t                            paged_md_start_lba;
    fbe_payload_block_operation_opcode_t op_code = FBE_RAID_GROUP_BACKGROUND_OP_NONE;
    fbe_chunk_size_t                     chunk_size;
    fbe_u8_t                             verify_flags;
    fbe_status_t                         status;
    fbe_raid_geometry_t                  *raid_geometry_p;
    fbe_u16_t                             data_disks;
    fbe_u64_t                            metadata_offset;
    // Cast the base object to a raid object
    raid_group_p = (fbe_raid_group_t*)context;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    /* If the status is not ok, that means we didn't get the 
       lock. Just return. we are already in the completion routine
    */
    status = fbe_transport_get_status_code (in_packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    // Set the completion function to release the NP lock
    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_release_NP_lock, raid_group_p);    

    //  Set the verify priority in the packet This should be done inside verify
    io_priority = fbe_raid_group_get_verify_priority(raid_group_p);
    fbe_transport_set_traffic_priority(in_packet_p, io_priority);

    /* Get the LBA and type for metadata chunk to verify.
     */
    fbe_raid_group_verify_get_next_verify_type(raid_group_p, 
                                               TRUE,
                                               &verify_flags);

    // Get verify checkpoint for running verify operation.
    verify_checkpoint = fbe_raid_group_verify_get_checkpoint(raid_group_p, verify_flags);

    fbe_base_config_metadata_get_paged_record_start_lba(&(raid_group_p->base_config),
                                                        &paged_md_start_lba);

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    paged_md_start_lba /= data_disks;

    /* At this point we know MD verify needs to be done by the checkpoint is not where
     * it should start. So set this to start of the paged MD to kick start the verify
     */
    if(verify_checkpoint < paged_md_start_lba)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO,
                              "%s set checkpoint to paged start for vr:%d\n", __FUNCTION__, verify_flags); 
        status = fbe_raid_group_metadata_get_verify_chkpt_offset(raid_group_p, verify_flags, &metadata_offset);
        status = fbe_base_config_metadata_nonpaged_force_set_checkpoint((fbe_base_config_t *)raid_group_p,
                                                                    in_packet_p,
                                                                    metadata_offset,
                                                                    0, /* There is only (1) checkpoint for a verify */
                                                                    paged_md_start_lba);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_raid_group_verify_get_next_metadata_verify_lba(raid_group_p, 
                                                       verify_flags,
                                                       &metadata_verify_lba);
    if (metadata_verify_lba == FBE_LBA_INVALID)
    {
        // No valid flag bits are set.
        if (verify_flags != 0)
        {
            /* There are no valid verify flags set, but we still have a checkpoint,
             * it is possible the flags were cleared in a rebuild. 
             * Set the checkpoint back to 0 so we can continue with the user region. 
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: no chunks marked, but checkpoint present for vr flags: 0x%x\n", 
                                  __FUNCTION__, verify_flags);
            status = fbe_raid_group_set_verify_checkpoint(raid_group_p, in_packet_p, verify_flags, 0);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: No chunk marked for verify\n", __FUNCTION__);
            fbe_transport_complete_packet(in_packet_p);
        }
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Get the opcode for verify flags.
     */
    op_code = fbe_raid_group_verify_get_opcode_for_verify(raid_group_p, verify_flags);
    if (op_code == (fbe_payload_block_operation_opcode_t)FBE_RAID_GROUP_BACKGROUND_OP_NONE)
    {
        //  Could not get opcode
        fbe_transport_complete_packet(in_packet_p);

        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Background Opcode None\n",
                              __FUNCTION__);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    //  Get the chunk size, which is the number of blocks to verify
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "rg_md_verify_np_lock: Send MD VR Op:%d,lba:0x%llx,size:%d\n",
                          op_code, (unsigned long long)metadata_verify_lba, chunk_size);

    // Set the I/O completion function and start the verify I/O operation.
    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_process_and_release_block_operation, raid_group_p);

    // Send the verify packet to the raid executor
    fbe_raid_group_executor_send_monitor_packet(raid_group_p, in_packet_p, 
                                                op_code, metadata_verify_lba, (fbe_block_count_t)chunk_size);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    
}
// End fbe_raid_group_metadata_verify_np_lock_completion()

/*!**************************************************************
 * fbe_raid_group_monitor_run_verify()
 ****************************************************************
 * @brief
 *  This function performs raid group verify operations.
 *
 * @param in_object_p - The pointer to the raid group.
 * @param in_packet_p - The control packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING 
 *
 * @author
 *  09/15/2009 - Created. MEV
 *  07/16/2010 - Modified Ashwin Tamilarasan
 *  04/22/2012 - Modified Prahlad Purohit
 *
 ****************************************************************/
fbe_lifecycle_status_t fbe_raid_group_monitor_run_verify(fbe_base_object_t*  in_object_p,
                                                         fbe_packet_t*       in_packet_p)
{
    fbe_raid_group_t*                    raid_group_p = (fbe_raid_group_t*)in_object_p;            
    fbe_lba_t                            ro_verify_checkpoint;
    fbe_lba_t                            rw_verify_checkpoint;
    fbe_lba_t                            error_verify_checkpoint;
    fbe_lba_t                            incomplete_write_verify_checkpoint;
    fbe_lba_t                            system_verify_checkpoint;
    fbe_raid_group_nonpaged_metadata_t*  raid_group_non_paged_metadata_p;
    fbe_bool_t                           can_proceed = FBE_TRUE;
    fbe_traffic_priority_t               io_priority; 
    fbe_status_t                         status;
    fbe_scheduler_hook_status_t          hook_status;
    fbe_medic_action_priority_t          ds_medic_priority;
    fbe_medic_action_priority_t          rg_medic_priority;
    fbe_raid_verify_flags_t              verify_flags = FBE_RAID_BITMAP_VERIFY_NONE;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_object_p);
    
    if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p)) {

        /* in the case background verify is disabled, we don't have to quiesce RG */
        fbe_raid_group_verify_get_next_verify_type(raid_group_p, 
                                                   FALSE,
                                                   &verify_flags);
        if ((verify_flags & FBE_RAID_BITMAP_VERIFY_ALL) == 0)
        {
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        
        status = fbe_raid_group_quiesce_for_bg_op(raid_group_p);
        if (status == FBE_STATUS_PENDING)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: raw mirror verify quiescing. \n");
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: raw mirror verify quiesced. \n");
    }
    else if (fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t *)raid_group_p, 
             (FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED|FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING))) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_monitor_run_verify quiesced or quiescing, defer verify request\n");

        /* Just return done.  Since we haven't cleared the condition we will be re-scheduled. */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    ds_medic_priority = fbe_base_config_get_downstream_edge_priority((fbe_base_config_t*)raid_group_p);
    fbe_raid_group_get_medic_action_priority(raid_group_p, &rg_medic_priority);

    if (ds_medic_priority > rg_medic_priority)
    {
        fbe_raid_group_check_hook(raid_group_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY, 
                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_NO_PERMISSION, 0,  &hook_status);
        /* No hook status is handled since we can only log here.
         */
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO, 
                             FBE_RAID_GROUP_DEBUG_FLAG_MEDIC_ACTION_TRACING,
                             "rg_monitor_run_verify downstream medic priority %d > rg medic priority %d\n",
                             ds_medic_priority, rg_medic_priority);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    //  See if we are allowed to do a verify I/O based on the current load.  If not, just return here and the 
    //  monitor will reschedule on its normal cycle.
    fbe_raid_group_verify_check_permission_based_on_load(raid_group_p, &can_proceed);
    if (can_proceed == FBE_FALSE) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_monitor_run_verify The load on downstream edges is higher\n");

        return FBE_LIFECYCLE_STATUS_DONE; /* This is very BAD design */
    }
    
    fbe_raid_group_verify_get_next_verify_type(raid_group_p, 
                                               FALSE,
                                               &verify_flags);
    /* Set time stamp so we can use it to detect slow I/O.
     */
    fbe_transport_set_physical_drive_io_stamp(in_packet_p, fbe_get_time());

    //  Set the verify priority in the packet This should be done inside verify
    io_priority = fbe_raid_group_get_verify_priority(raid_group_p);
    fbe_transport_set_traffic_priority(in_packet_p, io_priority);    

    /* AR 573625 -- moved this code block from above fbe_raid_group_verify_get_next_verify_type,
     * because if other threads update the checkpoint we will set verify flags, but these checkpoint values will be stale.
     * If this happens, we can call fbe_raid_group_run_user_verify with an INVALID checkpoint and panic.
     */
    fbe_raid_group_lock(raid_group_p);
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&raid_group_non_paged_metadata_p);
    error_verify_checkpoint = raid_group_non_paged_metadata_p->error_verify_checkpoint;
    rw_verify_checkpoint = raid_group_non_paged_metadata_p->rw_verify_checkpoint;
    ro_verify_checkpoint = raid_group_non_paged_metadata_p->ro_verify_checkpoint;
    incomplete_write_verify_checkpoint = raid_group_non_paged_metadata_p->incomplete_write_verify_checkpoint;
    system_verify_checkpoint = raid_group_non_paged_metadata_p->system_verify_checkpoint;
    fbe_raid_group_unlock(raid_group_p);

    if(verify_flags & FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE) {
        fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY, FBE_RAID_GROUP_SUBSTATE_VERIFY_INCOMPLETE_WRITE_START, 
                                  incomplete_write_verify_checkpoint, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}
        fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_monitor_run_verify_completion, raid_group_p);
        status = fbe_raid_group_run_user_verify(raid_group_p, in_packet_p, incomplete_write_verify_checkpoint, FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    if(verify_flags & FBE_RAID_BITMAP_VERIFY_ERROR) {
        fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY, FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT, error_verify_checkpoint, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}
        fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_monitor_run_verify_completion, raid_group_p);
        status = fbe_raid_group_run_user_verify(raid_group_p, in_packet_p, error_verify_checkpoint, FBE_RAID_BITMAP_VERIFY_ERROR);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    if(verify_flags & FBE_RAID_BITMAP_VERIFY_SYSTEM) {
        fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY, FBE_RAID_GROUP_SUBSTATE_VERIFY_SYSTEM_START, 
                                  system_verify_checkpoint, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}
        fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_monitor_run_verify_completion, raid_group_p);
        status = fbe_raid_group_run_user_verify(raid_group_p, in_packet_p, system_verify_checkpoint, FBE_RAID_BITMAP_VERIFY_SYSTEM);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }    

    if(verify_flags & FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE) {
        fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY, FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RW_START, 
                                  rw_verify_checkpoint, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}
        fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_monitor_run_verify_completion, raid_group_p);
        status = fbe_raid_group_run_user_verify(raid_group_p, in_packet_p, rw_verify_checkpoint, FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    if(verify_flags & FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY) {
        fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY, FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RO_START, 
                                  ro_verify_checkpoint, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}
        fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_monitor_run_verify_completion, raid_group_p);
        status = fbe_raid_group_run_user_verify(raid_group_p, in_packet_p, ro_verify_checkpoint, FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    return FBE_LIFECYCLE_STATUS_DONE;    
}
// End fbe_raid_group_monitor_run_verify()

/*!**************************************************************
 * fbe_raid_group_monitor_run_verify_completion()
 ****************************************************************
 * @brief
 *  This is the completion function for the verify condition.
 *  It runs when a verify I/O operation has completed. It
 *  reschedules the monitor to immediately run again in order
 *  to continue the verify operation.
 *
 * @param in_packet_p - The packet sent to us from the scheduler.
 * @param in_context  - The pointer to the object.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  09/25/2009 - Created. MEV
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_monitor_run_verify_completion(fbe_packet_t *in_packet_p, 
                                                          fbe_packet_completion_context_t in_context)
{
    fbe_status_t                status;
    fbe_raid_group_t           *raid_group_p;
    fbe_time_t                  time_stamp;
    fbe_u32_t                   service_time_ms;
    fbe_raid_verify_flags_t     verify_flags = FBE_RAID_BITMAP_VERIFY_NONE;
    fbe_lba_t                   verify_checkpoint;
    fbe_scheduler_hook_status_t hook_status;


    // Cast the base object to a raid object
    raid_group_p = (fbe_raid_group_t*)in_context;
    fbe_raid_group_verify_get_next_verify_type(raid_group_p, 
                                               FBE_FALSE,
                                               &verify_flags);

/*    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: background verify completed, packet: %p\n",
                              __FUNCTION__, in_packet_p); */
 
    status = fbe_transport_get_status_code(in_packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s verify_flags: 0x%x failed - status: 0x%x\n",
                              __FUNCTION__, verify_flags, status);

        //  Reschedule the next rebuild operation after a short delay
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 100);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* For debug generate a log hook
     */
    verify_checkpoint = fbe_raid_group_verify_get_checkpoint(raid_group_p, verify_flags);
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING,
                         "verify completion: verify_flags: 0x%x checkpoint: 0x%llx \n",
                         verify_flags, verify_checkpoint);
    if (verify_checkpoint == FBE_LBA_INVALID) {
        fbe_raid_group_check_hook(raid_group_p, 
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY, 
                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_ALL_VERIFIES_COMPLETE, 
                                  verify_checkpoint, &hook_status);
    }

    /* We keep the start time in the pdo time stamp since it is not used by raid.
     * The right thing to do is to have a "generic" time stamp. 
     */
    fbe_transport_get_physical_drive_io_stamp(in_packet_p, &time_stamp);
    service_time_ms = fbe_get_elapsed_milliseconds(time_stamp);

    if (service_time_ms > FBE_RAID_GROUP_MAX_IO_SERVICE_TIME)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Verify Completion: verify_flags: 0x%x service time: %d Reschedule for: %d ms\n",
                              verify_flags, service_time_ms, FBE_RAID_GROUP_SLOW_IO_RESCHEDULE_TIME);
        /* Simply reschedule for a greater time if the I/O took longer than our threshold..
         */
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, 
                                 (fbe_base_object_t*) raid_group_p, 
                                 FBE_RAID_GROUP_SLOW_IO_RESCHEDULE_TIME);
        return FBE_STATUS_OK;
    }

    if(raid_group_p->base_config.background_operation_count < fbe_raid_group_get_verify_speed()){
        raid_group_p->base_config.background_operation_count++;
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p, 0);
    } else {
        raid_group_p->base_config.background_operation_count = 0;
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p, 200);
    }

    // By returning OK, the monitor packet will be completed to the next level.
    return FBE_STATUS_OK;
}
// End fbe_raid_group_monitor_run_verify_completion

/*!****************************************************************************
 * fbe_raid_group_monitor_mark_specific_nr_cond_function()
 ******************************************************************************
 * @brief
 *   This is the condition function to mark Needs Rebuild (NR) on a specific 
 *   LBA range.  The condition is triggered by an event from the VD.  
 *
 *   This function is not used yet and is untested. 
 *
 * @param in_object_p               - pointer to a base object 
 * @param in_packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   
 *
 * @author 
 *   05/03/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_monitor_mark_specific_nr_cond_function(
                                fbe_base_object_t*      in_object_p,
                                fbe_packet_t*           in_packet_p)
{

    fbe_raid_group_t *                          raid_group_p;                       // pointer to the raid group 
    fbe_u32_t                                   disk_position;                      // position in the RG of disk to mark NR 
    fbe_event_t *                               data_event_p;                       // needs rebuild data event
    fbe_lba_t                                   data_event_lba;                     // start lba of the event extent
    fbe_block_count_t                           block_count;                        // block count of the event extent
    fbe_event_stack_t*                          event_stack_p;                      // event stack
    fbe_raid_group_needs_rebuild_context_t*     needs_rebuild_context_p;            // pointer to the needs rebuild context
    fbe_lba_t                                   rebuild_checkpoint;                 // rebuild checkpoint of specific position
    fbe_lba_t                                   exported_capacity;

    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_object_p);

    //  Cast the base object to a raid object
    raid_group_p = (fbe_raid_group_t *) in_object_p;

    //  Get a pointer to the needs rebuild data event
    //  @todo We cannot just take the event from the front of queue, it should be verified.
    fbe_base_config_event_queue_front_event((fbe_base_config_t *)raid_group_p, &data_event_p);
    if(data_event_p == NULL)
    {
        //  Clear the condition if event pointer is null
        fbe_lifecycle_clear_current_cond(in_object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  Get the current stack of event
    event_stack_p = fbe_event_get_current_stack(data_event_p);

    //  Get the extent of the event stack
    fbe_event_stack_get_extent(event_stack_p, &data_event_lba, &block_count);

    //  Find edge which needs to mark needs rebuild from event
    fbe_raid_group_needs_rebuild_get_disk_position_to_mark_nr(raid_group_p, data_event_p, &disk_position);
    if(disk_position == FBE_EDGE_INDEX_INVALID)
    {
        //  Clear the condition
        fbe_lifecycle_clear_current_cond(in_object_p);

        //  Pop the the data event
        fbe_base_config_event_queue_pop_event((fbe_base_config_t *)raid_group_p, &data_event_p);

        //  Send deny response if raid does not find disk position to update checkpoint
        fbe_event_set_flags(data_event_p, FBE_EVENT_FLAG_DENY);
        fbe_event_complete(data_event_p);

        return FBE_LIFECYCLE_STATUS_DONE;
    }
  
    exported_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);  
    fbe_raid_group_get_rebuild_checkpoint(raid_group_p, disk_position, &rebuild_checkpoint);
  
    
    /*  Verify if checkpoint update is required before marking specific range for needs rebuild.
        If the lba is greater than exported capacity then we just have to update the paged metadata
     */ 
    if((rebuild_checkpoint > data_event_lba) && !(data_event_lba >= exported_capacity ))
    {
        //  Set the condition to update rebuild checkpoint only if required
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *) raid_group_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_UPDATE_REBUILD_CHECKPOINT);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  Allocate the needs rebuild control operation to manage the needs rebuild context
    fbe_raid_group_needs_rebuild_allocate_needs_rebuild_context(raid_group_p, in_packet_p, &needs_rebuild_context_p);
    if(needs_rebuild_context_p == NULL)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  Initialize rebuild context before we start the rebuild opreation
    fbe_raid_group_needs_rebuild_initialize_needs_rebuild_context(needs_rebuild_context_p, data_event_lba,
        block_count, disk_position);

    //  Set the condition completion function before we mark NR
    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_monitor_mark_specific_nr_cond_completion, 
            (fbe_packet_completion_context_t) raid_group_p);

    //  Call a function to mark NR for the RAID object
    fbe_raid_group_needs_rebuild_process_nr_request(raid_group_p, in_packet_p); 
                    
    //  Return pending in all cases.  In case of error in a called function, it will complete the packet.
    return FBE_LIFECYCLE_STATUS_PENDING;

} // End fbe_raid_group_monitor_mark_specific_nr_cond_function()

/*!****************************************************************************
 * fbe_raid_group_monitor_mark_specific_nr_cond_completion()
 ******************************************************************************
 * @brief
 *   This is the condition completion function to mark the needs rebuild (NR)
 *   on RAID extent. It will clear the condition if NR is marked successfully. 
 *   
 * @param in_packet_p       - Pointer to a control packet from the scheduler.
 * @param in_object_p       - Packet completion context.
 *
 * @return fbe_status_t     - Return FBE_STATUS_OK on success/condition will
 *                            be cleared.
 *
 * @author 
 *   23/04/2009 - Created. Dhaval Patel. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_monitor_mark_specific_nr_cond_completion(
                                fbe_packet_t * in_packet_p,
                                fbe_packet_completion_context_t context)
{

    fbe_raid_group_t *                          raid_group_p;
    fbe_payload_ex_t *                         sep_payload_p;
    fbe_payload_control_operation_t *           control_operation_p;
    fbe_raid_group_needs_rebuild_context_t*     needs_rebuild_context_p;
    fbe_status_t                                status;
    fbe_event_t *                               data_event_p;
    fbe_payload_control_status_t                control_status;


    //  Get pointer to raid group object
    raid_group_p = (fbe_raid_group_t *) context;

    //  Get the status of the packet
    status = fbe_transport_get_status_code(in_packet_p);

    //  Get control status
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    fbe_payload_control_get_status(control_operation_p, &control_status);

    //  Release the payload buffer
    fbe_payload_control_get_buffer(control_operation_p, &needs_rebuild_context_p);
    fbe_transport_release_buffer(needs_rebuild_context_p);

    //  Pop the event from queue after marking NR gets completed or failed
    fbe_base_config_event_queue_pop_event((fbe_base_config_t *)raid_group_p, &data_event_p);

    //  If mark needs rebuild failed then print the warning.
    if((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *) raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s mark needs rebuild failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);

        fbe_event_set_flags(data_event_p, FBE_EVENT_FLAG_DENY);
    }

    //  Clear the NR condition
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *) raid_group_p);

    //  Complete the event with appropriate sttaus
    fbe_event_complete(data_event_p);
    
    //  Return good status
    return FBE_STATUS_OK;

} // End fbe_raid_group_monitor_mark_specific_nr_cond_completion()

/*!****************************************************************************
 * fbe_raid_group_monitor_set_rebuild_checkpoint_to_end_marker_cond_function()
 ******************************************************************************
 * @brief
 *   This is the condition function to move a rebuild checkpoint to the logical
 *   end marker after the rebuild has completed.  
 *
 * @param in_object_p               - pointer to a base object 
 * @param in_packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success/
 *                                    condition was cleared
 *                                  - FBE_LIFECYCLE_STATUS_DONE if waiting for 
 *                                    quiesce
 * @author
 *   03/25/2010 - Created. Jean Montes.
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_monitor_set_rebuild_checkpoint_to_end_marker_cond_function(
                                fbe_base_object_t*      in_object_p,
                                fbe_packet_t*           in_packet_p)
{
    fbe_status_t                status; 
    fbe_u32_t                   disk_index;                 
    fbe_u32_t                   bus;                 
    fbe_u32_t                   enclosure;                 
    fbe_u32_t                   slot;                 
    fbe_bool_t                  is_active_b; 
    fbe_raid_group_t*           raid_group_p = NULL;  

   
    raid_group_p = (fbe_raid_group_t*)in_object_p;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);   

    /* Get some information to trace.
     */
    fbe_raid_group_rebuild_find_disk_to_set_checkpoint_to_end_marker(raid_group_p, &disk_index);
    fbe_raid_group_monitor_check_active_state(raid_group_p, &is_active_b);

    /* Trace some information if enabled.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,
                         "raid_group: set checkpoint to end disk_index: %d is_active: %d \n",
                         disk_index, is_active_b);

    /*! @note Not sure why we would be here if there is no disk completing 
     *        rebuild.
     */
    if (disk_index == FBE_RAID_INVALID_DISK_POSITION)
    {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*) raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE; 
    }
    
    /* We we are not active (alternate could have taken over?)
     */
    if (is_active_b == FBE_FALSE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  If I/O is not already quiesed, set a condition to quiese I/O and return 
    status = fbe_raid_group_needs_rebuild_quiesce_io_if_needed(raid_group_p);
    if (status == FBE_STATUS_PENDING)
    {
        return FBE_LIFECYCLE_STATUS_RESCHEDULE; 
    }
    
    fbe_raid_group_logging_get_disk_info_for_position(raid_group_p, in_packet_p, disk_index, &bus, &enclosure, &slot);
    
    fbe_raid_group_logging_log_all_complete(raid_group_p, in_packet_p, disk_index, bus, enclosure, slot);
    
    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_rebuild_process_set_checkpoint_to_end_marker_completion, raid_group_p);    
    status = fbe_raid_group_get_NP_lock(raid_group_p, in_packet_p, fbe_raid_group_rebuild_set_checkpoint_to_end_marker);
        
    return FBE_LIFECYCLE_STATUS_PENDING;

} // End fbe_raid_group_monitor_set_rebuild_checkpoint_to_end_marker_cond_function()


/*!****************************************************************************
 * fbe_raid_group_rebuild_process_set_checkpoint_to_end_marker_completion()
 ******************************************************************************
 * @brief
 *   This function is called when the non-paged metadata write to set the 
 *   rebuild checkpoint(s) to the logical end marker has completed.  It will 
 *   check the packet status and clear the condition. 
 * 
 * @param in_packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   04/23/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_rebuild_process_set_checkpoint_to_end_marker_completion(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_packet_completion_context_t in_context)

{
    fbe_status_t        packet_status;                  // status from packet 
    fbe_raid_group_t   *raid_group_p;                   // pointer to raid group object 
    fbe_u32_t           disk_index;                     // index/position of disk to clear NR for

    //  Get the raid group pointer from the input context 
    raid_group_p = (fbe_raid_group_t*) in_context; 

    //  Check the packet status.  If the write had an error, then we'll leave the condition set so that we 
    //  can try again.  Return success so that the next completion function on the stack gets called. 
    packet_status = fbe_transport_get_status_code(in_packet_p);
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO, "%s: error %d on nonpaged write call\n", __FUNCTION__, packet_status);
        return FBE_STATUS_OK;
    }

    //  See if there is another disk that needs its checkpoint moved.  If so, then we will return here without
    //  doing anything further.  
    fbe_raid_group_rebuild_find_disk_to_set_checkpoint_to_end_marker(raid_group_p, &disk_index);
    if (disk_index != FBE_RAID_INVALID_DISK_POSITION)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s: found disk %d to update\n", __FUNCTION__, disk_index); 
        return FBE_STATUS_OK;
    }

    //  Clear the condition 
    fbe_lifecycle_clear_current_cond((fbe_base_object_t*) raid_group_p);

    //  See if the "degraded" attribute should be cleared now
    if (!fbe_raid_group_is_degraded(raid_group_p))
    {
        fbe_block_transport_server_clear_path_attr_all_servers(&((fbe_base_config_t *)raid_group_p)->block_transport_server,
                                                               FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED);

        /* If we are no longer degraded send the restore EMEH usurper.
         * This is only done on the Active side.  The EMEH condition will
         * coordinate with the peer so that the condition is set on the
         * passive side also.
         */
        if (fbe_raid_group_is_extended_media_error_handling_capable(raid_group_p))
        {
            fbe_raid_emeh_command_t emeh_request;

            /* If enabled send the condition to send the EMEH (Extended Media Error 
             * Handling) usurper to remaining PDOs.
             */
            fbe_raid_group_lock(raid_group_p);
            fbe_raid_group_get_emeh_request(raid_group_p, &emeh_request);
            if (emeh_request != FBE_RAID_EMEH_COMMAND_INVALID)
            {
                /* Simply trace and don't set the condition
                 */
                fbe_raid_group_unlock(raid_group_p);
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "emeh set chkpt to end: %d already in progress \n",
                                      emeh_request);
            }
            else
            {
                /* Else set the request type and condition
                 */
                fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_RESTORE_NORMAL_MODE);
                fbe_raid_group_unlock(raid_group_p);
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "emeh set chkpt to end: Set %d emeh request\n",
                                      emeh_request);
                fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *)raid_group_p,
                                       FBE_RAID_GROUP_LIFECYCLE_COND_EMEH_REQUEST);
            }
        }
    }

    //  Return success 
    return FBE_STATUS_OK; 

} // End fbe_raid_group_rebuild_process_set_checkpoint_to_end_marker_completion()


/*!****************************************************************************
 * fbe_raid_group_monitor_update_rebuild_checkpint_cond_function()
 ******************************************************************************
 * @brief
 *  This is the condition function which is used to update the rebuild checkpoint
 *  while processing mark needs rebuild event from downstream object.
 *
 *
 * @param in_object_p               - pointer to a base object 
 * @param in_packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   
 *
 * @author 
 *   05/03/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_monitor_update_rebuild_checkpint_cond_function(
                                fbe_base_object_t * in_object_p,
                                fbe_packet_t * in_packet_p)
{

    fbe_raid_group_t *                          raid_group_p;                       // pointer to the raid group 
    fbe_event_t *                               data_event_p;                       // needs rebuild data event
    fbe_status_t                                status;                             // status of the operation
    fbe_event_data_request_t                    data_request;                       // pointer to data request
    fbe_data_event_type_t                       data_event_type;                    // data event type


    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_object_p);

    //  Cast the base object to a raid object
    raid_group_p = (fbe_raid_group_t *) in_object_p;

    //  Get a pointer to the needs rebuild data event
    //  @todo We cannot just take the event from the front of queue, it should be verified.
    fbe_base_config_event_queue_front_event((fbe_base_config_t *)raid_group_p, &data_event_p);
    if(data_event_p == NULL)
    {
        //  Clear the condition if there is no event left to process
        fbe_lifecycle_clear_current_cond(in_object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    // Get data event type from data request and verify it
    fbe_event_get_data_request_data(data_event_p, &data_request);
    data_event_type = data_request.data_event_type;

    //  Data event must be mark needs rebuild, this condition gets set only for the mark needs
    //  rebuild data event
    if(data_event_type != FBE_DATA_EVENT_TYPE_MARK_NEEDS_REBUILD)
    {
        //  Clear the condition
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: data event type:0x%x not expected\n",
                              __FUNCTION__, data_event_type);
        fbe_lifecycle_clear_current_cond(in_object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  Determine if we need to update the checkpoint or not, if required update the checkpoint
    status = fbe_raid_group_needs_rebuild_data_event_update_checkpoint_if_needed(raid_group_p,
                                                                                 data_event_p, in_packet_p);

    //  Check if setting the checkpoint has completed or not.  It will not complete if it is waiting to quiesce 
    //  I/O.  In that case, return here with the condition still set.  After the quiesce completes, the condition 
    //  function will be invoked again and we will continue processing it. 
    if (status == FBE_STATUS_PENDING)
    {
        /*!@todo we should not return reschedule, we shoud just return done. */
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    //  If we have a packet outstanding to write the non-paged metadata (which we should), return pending here
    if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED) 
    {
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    //  In all other cases, we are done.  The condition will have been cleared by the called function if needed.
    return FBE_LIFECYCLE_STATUS_DONE;
    
} // End fbe_raid_group_monitor_update_rebuild_checkpint_cond_function()

/*!****************************************************************************
 * fbe_raid_group_mark_mdd_for_incomplete_write_verify()
 ******************************************************************************
 * @brief
 *  This functions marks entire metdata region for incomplete write verify.
 *
 * @param in_packet_p               - pointer to a control packet from the scheduler
 * @param in_context                - Context carries raid_group_p
 *
 * @return fbe_status_t   
 *
 * @author 
 *   03/18/2012 - Created. Prahlad Purohit
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_mark_mdd_for_incomplete_write_verify(
                            fbe_packet_t * packet_p, 
                            fbe_packet_completion_context_t context)
{
    fbe_raid_group_t    *raid_group_p = (fbe_raid_group_t *)context;

    fbe_raid_group_bitmap_mark_metadata_of_metadata_verify(packet_p,
                                                           raid_group_p,
                                                           FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

} // End fbe_raid_group_mark_mdd_for_incomplete_write_verify

#if 0
/*!****************************************************************************
 * fbe_raid_group_mark_iw_verify_update_checkpoint()
 ******************************************************************************
 * @brief
 *  This functions updates the md reconstruction checkpoint to start of metadata 
 *  region It also triggers the marking of metdata region for error verify using
 *  a completion.
 *
 * @param in_packet_p               - pointer to a control packet from the scheduler
 * @param in_context                - Context carries raid_group_p
 *
 * @return fbe_status_t   
 *
 * @author 
 *   03/18/2012 - Created. Prahlad Purohit
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_mark_iw_verify_update_checkpoint(
                            fbe_packet_t * packet_p, 
                            fbe_packet_completion_context_t context)
{
    fbe_raid_group_t    *raid_group_p = (fbe_raid_group_t *)context;
    fbe_lba_t           paged_md_start_lba = FBE_LBA_INVALID;
    fbe_lba_t           paged_md_capacity  = FBE_LBA_INVALID;
    fbe_chunk_index_t   chunk_index = FBE_CHUNK_INDEX_INVALID;
    fbe_chunk_count_t   chunk_count = 0;
    fbe_u64_t           metadata_offset = 0;
    fbe_status_t        status;

    /* If the status is not ok, that means we didn't get the lock.
     */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Fetched NP lock.\n",
                          __FUNCTION__);

    // Push a completion on stack to release NP lock.
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);        

    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->incomplete_write_verify_checkpoint);      

    // Get paged metadata start lba and its capacity.
    fbe_base_config_metadata_get_paged_record_start_lba(&(raid_group_p->base_config),
                                                        &paged_md_start_lba);

    fbe_base_config_metadata_get_paged_metadata_capacity(&(raid_group_p->base_config),
                                                         &paged_md_capacity);


    // Convert the metadata start lba and block count into per disk chunk index and chunk count
    fbe_raid_group_bitmap_get_chunk_index_and_chunk_count(raid_group_p, 
                                                          paged_md_start_lba,
                                                          paged_md_capacity,
                                                          &chunk_index, 
                                                          &chunk_count);

    // Update the checkpoint in non paged metdata.
    fbe_raid_group_bitmap_set_non_paged_metadata_for_verify(packet_p,raid_group_p,
                                                            chunk_index, chunk_count,
                                                            fbe_raid_group_mark_mdd_for_incomplete_write_verify,
                                                            metadata_offset,
                                                            FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}// End fbe_raid_group_mark_iw_verify_update_checkpoint
#endif


/*!****************************************************************************
 * fbe_raid_group_mark_md_iw_or_err_verify_completion()
 ******************************************************************************
 * @brief
 *  This completion function is called when it has completed marking mdd region
 *  for error verify or incomplete write verify.
 *
 * @param in_packet_p               - pointer to a control packet from the scheduler
 * @param in_context                - Context carries raid_group_p
 *
 * @return fbe_status_t   
 *
 * @author 
 *   03/18/2012 - Created. Prahlad Purohit
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_mark_md_iw_or_err_verify_completion(
                            fbe_packet_t * packet_p, 
                            fbe_packet_completion_context_t in_context)
{

    fbe_raid_group_t*           raid_group_p;
    fbe_status_t                status;


    // Cast the base object to a raid object
    raid_group_p = (fbe_raid_group_t*)in_context;

    // Clear the condition.
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear, status:0x%x\n",
                              __FUNCTION__, status);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s complete.\n",
                              __FUNCTION__);
    }

    return FBE_STATUS_OK;
}// End fbe_raid_group_mark_md_iw_or_err_verify_completion


/*!****************************************************************************
 * raid_group_metadata_mark_incomplete_write_cond_function()
 ******************************************************************************
 * @brief
 *  This condition function is used to mark metdata region for reconstruction.
 *  This condition is set when an IOTS completion requests reconstruction in md
 *  region.
 *
 * @param in_object_p               - pointer to a base object 
 * @param in_packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   
 *
 * @author 
 *   03/30/2012 - Created. Prahlad Purohit
 *
 ******************************************************************************/
fbe_lifecycle_status_t raid_group_metadata_mark_incomplete_write_cond_function(
                                fbe_base_object_t * in_object_p,
                                fbe_packet_t * in_packet_p)
{
    fbe_raid_group_t *              raid_group_p = NULL;
    fbe_scheduler_hook_status_t     hook_status = FBE_SCHED_STATUS_OK;    
    fbe_raid_geometry_t *raid_geometry_p = NULL;

    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_object_p);

    //  Cast the base object to a raid object
    raid_group_p = (fbe_raid_group_t *) in_object_p;

    //  Raw mirrors do not use paged metadata.
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        fbe_lifecycle_clear_current_cond(in_object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    // Check for mark error verify hook.
    fbe_base_config_check_hook(&(raid_group_p->base_config),
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_VERIFY,
                               FBE_RAID_GROUP_SUBSTATE_METADATA_MARK_RECONSTRUCT,
                               0,
                               &hook_status);

    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_mark_md_iw_or_err_verify_completion, raid_group_p);

    // Get the NP lock as the first thing.
    //fbe_raid_group_get_NP_lock(raid_group_p, in_packet_p, fbe_raid_group_mark_iw_verify_update_checkpoint);
    // Start mark incomplete write verify.
    fbe_raid_group_verify_mark_iw_verify(in_packet_p, raid_group_p);

    return FBE_LIFECYCLE_STATUS_PENDING;

} // End raid_group_metadata_mark_incomplete_write_cond_function


/*!****************************************************************************
 * fbe_raid_group_monitor_process_event_queue_cond_function()
 ******************************************************************************
 * @brief
 *   This is the condition function which is used to process the event from the
 *   event queue and set the appropriate condition as needed
 *
 *
 * @param in_object_p               - pointer to a base object 
 * @param in_packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   
 *
 * @author 
 *   05/03/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_monitor_process_event_queue_cond_function(
                                fbe_base_object_t*      in_object_p,
                                fbe_packet_t*           in_packet_p)
{

    fbe_raid_group_t *                          raid_group_p;                       // pointer to the raid group 
    fbe_event_t *                               data_event_p;                       // needs rebuild data event
    fbe_status_t                                status;                             // status of the operation
    fbe_event_data_request_t                    data_request;                       // pointer to data request
    fbe_data_event_type_t                       data_event_type;                    // data event type


    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_object_p);

    //  Cast the base object to a raid object
    raid_group_p = (fbe_raid_group_t *) in_object_p;

    //  Get a pointer to the needs rebuild data event
    //  @todo We cannot just take the event from the front of queue, it should be verified.
    fbe_base_config_event_queue_front_event((fbe_base_config_t *)raid_group_p, &data_event_p);
    if(data_event_p == NULL)
    {
        //  Clear the condition if there is no event left to process
        fbe_lifecycle_clear_current_cond(in_object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  Get data event type from data request
    fbe_event_get_data_request_data(data_event_p, &data_request);
    data_event_type = data_request.data_event_type;

    switch(data_event_type)
    {
        case FBE_DATA_EVENT_TYPE_MARK_NEEDS_REBUILD:
             //  Process the data event to mark needs rebuild
             status = fbe_raid_group_needs_rebuild_process_needs_rebuild_event(raid_group_p, data_event_p, in_packet_p);
             break;
        case FBE_DATA_EVENT_TYPE_MARK_VERIFY:
        case FBE_DATA_EVENT_TYPE_MARK_IW_VERIFY:
             //  Process the data event to mark needs rebuild
             return (fbe_raid_group_verify_process_mark_verify_event(raid_group_p, data_event_p, in_packet_p));

        default:
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s: data event type:0x%x not supported\n",
                                  __FUNCTION__, data_event_type);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    //  Return lifecycle status done here
    return FBE_LIFECYCLE_STATUS_DONE;

} // End fbe_raid_group_monitor_process_event_queue_cond_function()


/*!****************************************************************************
 * fbe_raid_group_monitor_nonpaged_metadata_persist()
 ******************************************************************************
 * @brief
 *   We persist nonpaged metadata here.
 *
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param context    - completion context, which is a pointer to the raid
 *                        group object
 *
 * @return  fbe_status_t
 *
 * @author
 *   12/12/2013 - Created. Jamin Kang
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_monitor_nonpaged_metadata_persist(fbe_packet_t * packet_p,
                                                 fbe_packet_completion_context_t context)

{
    fbe_raid_group_t*                    raid_group_p = NULL;
    fbe_status_t                         status;

    raid_group_p = (fbe_raid_group_t*)context;

    /* If the status is not ok, that means we didn't get the
       lock. Just return. we are already in the completion routine
    */
    status = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status);
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Persist NP after md rebuild\n",
                          __FUNCTION__);
    /* Set the completion function to release the NP lock */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);

    fbe_base_config_metadata_nonpaged_persist_sync((fbe_base_config_t *)raid_group_p,
                                                   packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
} // End fbe_raid_group_nonpaged_metadata_persist

/*!****************************************************************************
 * fbe_raid_group_monitor_run_metadata_rebuild_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function of metadata rebuild.
 *   We persist nonpaged metadata after metadata rebuild done.
 *
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param context    - completion context, which is a pointer to the raid
 *                        group object
 *
 * @return  fbe_status_t
 *
 * @author
 *   12/12/2013 - Created. Jamin Kang
 ******************************************************************************/
static fbe_status_t fbe_raid_group_monitor_run_metadata_rebuild_completion(fbe_packet_t * packet_p,
                                                                           fbe_packet_completion_context_t context)
{

    fbe_status_t status;                     //  fbe status
    fbe_payload_ex_t  *sep_payload_p;              //  pointer to the payload
    fbe_payload_control_operation_t *control_operation_p;        //  pointer to the control operation
    fbe_payload_control_status_t control_status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *) context;
    fbe_bool_t is_metadata_need_rebuild;
    fbe_lba_t rebuild_metadata_lba;       //  current checkpoint
    fbe_u32_t lowest_rebuild_position;

    //  Get the sep payload and control operation
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    //  Check for a failure on the I/O.  If one occurred, dont' persist metadata
    if ((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)) {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO, "%s rebuild failed, status: 0x%x\n",
                              __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    is_metadata_need_rebuild = fbe_raid_group_background_op_is_metadata_rebuild_need_to_run(raid_group_p);
    fbe_raid_group_rebuild_get_next_metadata_lba_to_rebuild(raid_group_p, &rebuild_metadata_lba, &lowest_rebuild_position);
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: rb chkpt 0x%llx, rb_pos_b: 0x%x, nr: %u\n",
                          __FUNCTION__, (unsigned long long)rebuild_metadata_lba,
                          1 << lowest_rebuild_position, is_metadata_need_rebuild);

    /* We only persist nonpaged metadata when finish rebuilding on metadata */
    if (is_metadata_need_rebuild) {
        return FBE_STATUS_OK;
    } else {
        fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_monitor_nonpaged_metadata_persist);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
}

/*!****************************************************************************
 * fbe_raid_group_monitor_run_metadata_rebuild()
 ******************************************************************************
 * @brief
 *   This is the condition function to determine if a rebuild is able to be 
 *   performed.  If so, it will send a command to the executor to perform a 
 *   rebuild I/O and return a pending status.  The condition will remain set in
 *   that case.
 *   
 *   If a rebuild cannot be performed at this time, the condition will be cleared.  
 *
 * @param in_object_p               - pointer to a base object 
 * @param in_packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success/
 *                                    condition was cleared
 *                                  - FBE_LIFECYCLE_STATUS_PENDING when I/O is
 *                                    being issued 
 * @author
 *   12/07/2009 - Created. Jean Montes.
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_monitor_run_metadata_rebuild(
                                fbe_base_object_t*          in_object_p,
                                fbe_packet_t*               in_packet_p)
{

    fbe_raid_group_t*                   raid_group_p;               //  pointer to the raid group 
    fbe_bool_t                          can_rebuild_start_b;        //  true if rebuild can start
    fbe_raid_group_rebuild_context_t*   rebuild_context_p;          //  rebuild monitor's context information
    fbe_lba_t                           rebuild_metadata_lba;       //  current checkpoint
    fbe_u32_t                           lowest_rebuild_position;
    fbe_bool_t                          b_metadata_rebuild_enabled;
    fbe_scheduler_hook_status_t         status;
    fbe_lba_t                           imported_disk_capacity;

    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_object_p);

    //  Cast the base object to a raid object
    raid_group_p = (fbe_raid_group_t*) in_object_p;

    // See if metadata rebuild background operations is enabled to run
    fbe_base_config_is_background_operation_enabled((fbe_base_config_t *) raid_group_p, 
                            FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD, &b_metadata_rebuild_enabled);
    if( b_metadata_rebuild_enabled == FBE_FALSE ) 
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
            FBE_TRACE_LEVEL_DEBUG_HIGH,
            FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: run metadata rebuild: rebuild operation: 0x%x is disabled. \n",
                              FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  See if we are allowed to do a rebuild I/O based on the current load and active/passive state.  If not,
    //  just return here and the monitor will reschedule on its normal cycle.
    fbe_raid_group_rebuild_determine_if_rebuild_can_start(raid_group_p, &can_rebuild_start_b);
    if(can_rebuild_start_b == FBE_FALSE)
    {  
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  Get the next metadata lba which we need to rebuid, if all the chunks rebuild then clear the condition
    fbe_raid_group_rebuild_get_next_metadata_lba_to_rebuild(raid_group_p, &rebuild_metadata_lba, &lowest_rebuild_position);
    if(rebuild_metadata_lba == FBE_LBA_INVALID)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    imported_disk_capacity = fbe_raid_group_get_disk_capacity(raid_group_p);
    if (rebuild_metadata_lba >= imported_disk_capacity)
    {
        /* If we are here doing a metadata rebuild, it means we have marked mdd chunks that are below the checkpoint.
         * The only way for the rebuild to proceed is to set the checkpoint back to 0 to re-run 
         * the metadata lba for those chunks. 
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: rb checkpoint 0x%llx >= imported disk cap 0x%llx rb_pos: 0x%x\n",
                              (unsigned long long)rebuild_metadata_lba, (unsigned long long)imported_disk_capacity, lowest_rebuild_position);
        fbe_raid_group_set_rebuild_checkpoint_to_zero(raid_group_p, in_packet_p, (1 << lowest_rebuild_position) );
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
  
    fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY, rebuild_metadata_lba, &status);
    if (status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

    //  Allocates control operation to track the rebuild operation
    fbe_raid_group_rebuild_allocate_rebuild_context(raid_group_p, in_packet_p, &rebuild_context_p);
    if (rebuild_context_p == NULL)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Trace some information if enabled.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,
                         "raid_group: run metadata rebuild: lowest rebuild pos: %d metadata lba: 0x%llx \n",
                         lowest_rebuild_position,
             (unsigned long long)rebuild_metadata_lba);

    //  Initialize rebuild context before we start the rebuild opreation
    fbe_raid_group_rebuild_initialize_rebuild_context(raid_group_p,
                                                      rebuild_context_p,
                                                      FBE_RAID_POSITION_BITMASK_ALL, /*! @note Position is irrelevant for metadata rebuild. */         
                                                      rebuild_metadata_lba);
    /* Set time stamp so we can use it to detect slow I/O.
     */
    fbe_transport_set_physical_drive_io_stamp(in_packet_p, fbe_get_time());
    //  Set the completion function for the rebuild condition
    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_monitor_run_rebuild_completion, raid_group_p);
    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_monitor_run_metadata_rebuild_completion, raid_group_p);

    //  Check if LBA is within a LUN and continue accordingly
    fbe_raid_group_rebuild_send_event_to_check_lba(raid_group_p, rebuild_context_p, in_packet_p);

    return FBE_LIFECYCLE_STATUS_PENDING;

} // End fbe_raid_group_monitor_run_metadata_rebuild()

/*!****************************************************************************
 * fbe_raid_group_journal_flush_send_block_operation()
 ******************************************************************************
 * @brief
 * We have valid slot (or special slot) metadata in the payload of the packet.
 * This function will allocate a block operation to tell the raid object to
 * flush the data described by the slot metadata.
 *
 * @param in_packet_p               - pointer to a control packet from the scheduler
 * @param raid_group_p              - pointer to RG which we are flushing
 * @param slot_id                   - write_log slot that needs to be flushed 
 * 
 * @return fbe_status_t             - FBE_STATUS_MORE_PROCESSING_REQUIRED if the
 *                                    the operation was sent to the object. Otherwise
 *                                    a failure code is returned and the stack is to
 *                                    be unwound.
 *   
 * @author
 *   1/10/2011 - Created. Daniel Cummins.
 *   3/2/2012 - Vamsi V. Modified to use write_log info in parity object instead
 *                       of using metadata service.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_journal_flush_send_block_operation(fbe_packet_t * in_packet_p, 
                                                  fbe_raid_group_t * raid_group_p, 
                                                  fbe_u32_t slot_id)
{    
    fbe_raid_iots_t *       iots_p;
    fbe_lba_t               write_log_start_lba;
    fbe_u32_t               write_log_slot_size;
    fbe_payload_ex_t *      sep_payload_p        = fbe_transport_get_payload_ex(in_packet_p);
    fbe_raid_geometry_t     *raid_geometry_p     = &raid_group_p->geo;

    /* Get write_log slot information */
    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &write_log_start_lba); 
    fbe_raid_geometry_get_journal_log_slot_size(raid_geometry_p, &write_log_slot_size);

    /* Increment lba to slot we need to flush */
    write_log_start_lba = (write_log_start_lba + (slot_id * write_log_slot_size)); 

    /* We cannot lock the stripe that we are going to flush to as we do not have 
     * the live stripe info until Flush SM reads in the header */

    /* Set completion function */
    fbe_transport_set_completion_function(in_packet_p, 
                                          fbe_raid_group_journal_flush_completion, 
                                          (fbe_packet_completion_context_t) raid_group_p);

    /* initialize the journal slot id here so the raid object knows which slot entry
     * to fetch and validate.
     */
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_set_journal_slot_id(iots_p, slot_id);

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s iots_p 0x%p, slot 0x%x.\n",
                          __FUNCTION__, iots_p, slot_id);

    /* build the flush request - if successful the following function will return
     * FBE_STATUS_MORE_PROCESSING_REQUIRED
     */
    return fbe_raid_group_executor_send_monitor_packet(raid_group_p, 
                                                       in_packet_p, 
                                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH,
                                                       write_log_start_lba,
                                                       1 /* one block header read */);
} // End fbe_raid_group_journal_flush_send_block_operation()

/*!**************************************************************
 * fbe_raid_group_is_write_log_flush_required()
 ****************************************************************
 * @brief
 *  Utility function to determine if write_log Flush is required.
 *
 * @param raid_group_p - The ptr to raid object.
 *
 * @return fbe_bool_t - Return true if Flush is required
 *
 * @author
 *  5/16/2012 - Vamsi V
 *
 ****************************************************************/
fbe_bool_t fbe_raid_group_is_write_log_flush_required(fbe_raid_group_t * raid_group_p)
{
    fbe_bool_t is_flush_req = FBE_TRUE;
    fbe_raid_geometry_t * raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    /* this condition is only valid for raid356 -- this is a debug check to catch an occasional failure seen in fbe test */
    if (!fbe_raid_geometry_is_raid3(raid_geometry_p) &&
        !fbe_raid_geometry_is_raid5(raid_geometry_p) && 
        !fbe_raid_geometry_is_raid6(raid_geometry_p))
    {
        is_flush_req = FBE_FALSE;
    }

    /* Some RGs (vault) dont have write_logging enabled */
    if(!fbe_raid_geometry_is_write_logging_enabled(raid_geometry_p))
    {
        is_flush_req = FBE_FALSE;
    }

    /* Nothing in write_log slots for healthy RGs */
    if (!fbe_raid_group_is_degraded(raid_group_p))
    {
        is_flush_req = FBE_FALSE;       
    }

    return is_flush_req;
}
/*!****************************************************************************
 * fbe_raid_group_monitor_journal_flush_cond_function()
 ******************************************************************************
 * @brief
 *   This condition function will initiatate a journal flush.  This function will
 *   send a command to the executor to retreive a journal slot.  If a valid slot
 *   exists the data is retrieved from the journal and flushed.  This process
 *   continues until there are no more valid slots or a physical/logical error
 *   is encountered.  The condition is ultimately cleared by the condition
 *   completion function.
 *
 * @param object_p               - pointer to a base object 
 * @param packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING 
 *                                    (packet sent to executor)
 * @author
 *   1/05/2011 - Created. Daniel Cummins.
 *   3/2/2012 - Vamsi V. Modified to use write_log info in parity object instead
 *                       of using metadata service.
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_monitor_journal_flush_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_scheduler_hook_status_t hook_status;
    fbe_u32_t slot_id = FBE_PARITY_WRITE_LOG_INVALID_SLOT;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_raid_group_t * raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_raid_geometry_t * raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_bool_t b_flush_required;

    b_flush_required = fbe_raid_group_is_write_log_flush_required(raid_group_p);

    status = fbe_lifecycle_get_state(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p, &lifecycle_state);

    if (lifecycle_state == FBE_LIFECYCLE_STATE_READY)
    {
        status = fbe_raid_group_needs_rebuild_quiesce_io_if_needed(raid_group_p);
        if (status == FBE_STATUS_PENDING) {
            return FBE_LIFECYCLE_STATUS_RESCHEDULE; 
        }
        status = fbe_raid_group_abort_all_metadata_operations(raid_group_p);
        if (status == FBE_STATUS_PENDING) {
            return FBE_LIFECYCLE_STATUS_RESCHEDULE; 
        }
        /* The quiesce aborted stripe locks and paged.  Open the gates 
         * again so we can send the md ops. 
         */
        fbe_metadata_element_clear_abort(&raid_group_p->base_config.metadata_element);
    }

    /* clear the PEER_LOST flag if it is set so md request can get the stripe lock */
    fbe_metadata_element_clear_sl_peer_lost(raid_geometry_p->metadata_element_p);

    fbe_raid_group_check_hook(raid_group_p, SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH, 
                              FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_START, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) { return FBE_LIFECYCLE_STATUS_DONE; }

    if (b_flush_required) {
        /* Get slot ID that needs to be flushed */
        slot_id = fbe_parity_write_log_get_allocated_slot(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p);
    }

    if (slot_id == FBE_PARITY_WRITE_LOG_INVALID_SLOT) {
        /* Check if we are encrypting and need to handle incomplete write.
         * First check for the need to re-write the paged
         * Then check for the need to do an invalidate of a stripe 
         * RAID-10 does not need to check for invalidate since the mirror   
         * level will do this invalidate, not the striper.
         */
        if (fbe_raid_group_encryption_is_rekey_needed(raid_group_p) &&
            fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_NEEDS_RECONSTRUCT)) {
            fbe_raid_group_encryption_flags_t flags;
            fbe_raid_group_encryption_get_state(raid_group_p, &flags);
            fbe_raid_group_check_hook(raid_group_p, SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                                      FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PEER_FLUSH_PAGED, 0, &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE) { return FBE_LIFECYCLE_STATUS_DONE; }
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encryption: Peer restart reconstruct of paged. State: 0x%x\n",
                                  flags);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_set_completion_function(packet_p, fbe_raid_group_encrypt_paged_completion, raid_group_p);
            if (object_p->class_id == FBE_CLASS_ID_VIRTUAL_DRIVE) {
                fbe_raid_group_write_paged_metadata(packet_p, raid_group_p);
            } else {
                fbe_raid_group_zero_paged_metadata(packet_p, raid_group_p);
            }
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
        if (!fbe_raid_geometry_is_raid10(raid_geometry_p) &&
            fbe_raid_group_encryption_is_active(raid_group_p)) {
            fbe_raid_group_encryption_iw_check(raid_group_p, packet_p);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }

         /* Debug hook */
        fbe_raid_group_check_hook(raid_group_p, SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH, 
                                  FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_DONE, 0, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

        /* Clear Flush condition */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*) raid_group_p);

        if (status != FBE_STATUS_OK){
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear Write_log flush condition, status: 0x%X\n",
                                  __FUNCTION__, 
                                  status);
        }
        else {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "Write_log flush condition cleared.\n");
        }

        /* We are done with Flush operation */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* send the journal flush request to the raid object.
     * Returns FBE_STATUS_MORE_PROCESSING_REQUIRED or an error.
     */
    status = fbe_raid_group_journal_flush_send_block_operation(packet_p, raid_group_p, slot_id);

    if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED){
        /* We are done with this request. Complete the packet. */
        fbe_transport_complete_packet(packet_p);
    }

    return FBE_LIFECYCLE_STATUS_PENDING;

} // End fbe_raid_group_monitor_journal_flush_cond_function()

/*!****************************************************************************
 * fbe_raid_group_journal_flush_completion()
 ******************************************************************************
 * @brief
 * This transport completion function is called after the raid group has
 * processed a FLUSH_JOURNAL operation.  This function simply needs to undwind
 * the payload stack.  It does not clear the condition as this is handled by 
 * the monitor thread when it checks for remaining valid slot entries.
 *
 * @param in_packet_p               - pointer to a control packet from the scheduler
 * @param in_context                - context, which is a pointer to the base object 
 * 
 * @return FBE_STATUS_OK the operation succeeded and a release slot operation was
 *         sent to the metadata service.  If the block operation was not 
 *         successful or the packet failed the slot will not be released and
 *         the packet status is returned.
 *   
 * @author
 *   1/10/2011 - Created. Daniel Cummins.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_journal_flush_completion(
                                       fbe_packet_t*                   in_packet_p,
                                       fbe_packet_completion_context_t in_context)
{
    fbe_raid_group_t *                  raid_group_p         = (fbe_raid_group_t *) in_context;
    fbe_raid_geometry_t *               raid_geometry_p      = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_status_t                        status               = fbe_transport_get_status_code(in_packet_p);
    fbe_payload_ex_t *                  sep_payload_p        = fbe_transport_get_payload_ex(in_packet_p);
    fbe_raid_iots_t *                   iots_p;
    fbe_payload_block_operation_t    *  block_operation_p    = NULL;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qual;
    fbe_u32_t slot_index;

    /* unwind the payload
     */
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_get_journal_slot_id(iots_p, &slot_index);
    if (slot_index == FBE_PARITY_WRITE_LOG_INVALID_SLOT)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Write_log slot_id is invalid. iots_p 0x%p, slot 0x%x. \n",
                              __FUNCTION__, iots_p, slot_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    block_operation_p   = fbe_payload_ex_get_block_operation(sep_payload_p);
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qual);
    fbe_payload_ex_release_block_operation(sep_payload_p, block_operation_p);

#if 0
    /* If object is quiesced or if we get a retryable error, Flush needs to retry */
    if ((status == FBE_STATUS_QUIESCED) || 
        ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED) && 
         (block_qual == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE)))
    {
        /* Reset slot state from Flushing to Allocated so that we will retry Flush on this slot */
        fbe_parity_write_log_set_slot_allocated(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p, slot_index); 
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Write_log FLUSH needs to retry slot %d \n", slot_index);
        return FBE_STATUS_OK;
    }
#endif

    if ((status == FBE_STATUS_OK) && (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) && 
        (block_qual == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Write_log flush for slot 0x%x completed successfully. status 0x%x, block status 0x%x, block qual 0x%x \n",
                              slot_index, status, block_status, block_qual);

        /* Reschedule immediately so that we will initiate Flush of next write_log slot. */
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const,
                                 (fbe_base_object_t *)raid_group_p,
                                 0);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Write_log flush for slot 0x%x FAILED. status %d, block status 0x%x, block qual 0x%x \n",
                              slot_index, status, block_status, block_qual);

        /* Flush operation failed. Reset slot state to Allocated so that we will retry Flush on this slot */
        fbe_parity_write_log_set_slot_allocated(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p, slot_index);
    }

    return FBE_STATUS_OK;

} // End fbe_raid_group_journal_flush_completion()

/*!****************************************************************************
 * fbe_raid_group_monitor_journal_remap_cond_function()
 ******************************************************************************
 * @brief
 *   This condition function will initiatate write_verify on entire write_log slot
 * causing sectors with media errors to be remapped. If RG is in Activate state 
 * (no host IO) all write_log slots will be checked for media errors. If RG is in
 * Ready state (peer_contact_lost case), only peer's write_log slots will be checked
 * for media errors.     
 *
 * @param in_object_p               - pointer to a base object 
 * @param in_packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING 
 *                                    (packet sent to executor)
 * @author
 *   8/2/2012 - Created. Vamsi V.
 *   11/11/2013 - Add remap debug hook. Wenxuan Yin.
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_monitor_journal_remap_cond_function(fbe_base_object_t * in_object_p, 
                                                                          fbe_packet_t * in_packet_p)
{
    fbe_status_t status;
    fbe_scheduler_hook_status_t hook_status;
    fbe_u32_t slot_id;
    fbe_lifecycle_state_t               lifecycle_state;
    fbe_raid_group_t *                  raid_group_p = (fbe_raid_group_t *)in_object_p;
    fbe_raid_geometry_t *               raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_payload_ex_t *                  sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    fbe_raid_iots_t *                   iots_p;
    fbe_memory_request_t*               memory_request_p = NULL;
    fbe_packet_resource_priority_t      memory_request_priority;
    fbe_u32_t                           width;

    /* this condition is only valid for raid356 -- this is a debug check to catch an occasional failure seen in fbe test */
    if (!fbe_raid_geometry_is_raid3(raid_geometry_p) &&
        !fbe_raid_geometry_is_raid5(raid_geometry_p) && 
        !fbe_raid_geometry_is_raid6(raid_geometry_p))
    {
        fbe_base_object_trace(in_object_p, 
                              FBE_TRACE_LEVEL_CRITICAL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Write_Log FLUSH condition cleared, incorrect geometry.\n", __FUNCTION__);
        fbe_lifecycle_clear_current_cond(in_object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Get state of the object */
    status = fbe_lifecycle_get_state(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p, &lifecycle_state);

    /* Get slot ID that needs to be checked for media errors */
    if (lifecycle_state == FBE_LIFECYCLE_STATE_READY)
    {
        slot_id = fbe_parity_write_log_get_remap_slot(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p, FBE_FALSE);
    }
    else
    {
        slot_id = fbe_parity_write_log_get_remap_slot(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p, FBE_TRUE);
    }


    if (slot_id == FBE_PARITY_WRITE_LOG_INVALID_SLOT)
    {
        /* Clear write_log remap condition */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*) raid_group_p);

        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear Write_log Remap condition, status: 0x%X\n",
                                  __FUNCTION__, 
                                  status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Write_log Remap condition cleared.\n", __FUNCTION__);
        }

        /* Check the hook for pending remap condtion */
        fbe_raid_group_check_hook(raid_group_p, SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REMAP, 
                              FBE_RAID_GROUP_SUBSTATE_JOURNAL_REMAP_DONE, 0, &hook_status);

        /* We are done with Remap operation */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Check the hook for pending remap condtion */
    fbe_raid_group_check_hook(raid_group_p, SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REMAP, 
                              FBE_RAID_GROUP_SUBSTATE_JOURNAL_REMAP_SLOTS_ALLOCATED, 0, &hook_status);
    /* We need reset the needs_remap as TRUE first and then 
      simply return FBE_LIFECYCLE_STATUS_DONE,
     */
    if(hook_status == FBE_SCHED_STATUS_DONE)
    {
        fbe_parity_write_log_set_flag(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p, 
                                      FBE_PARITY_WRITE_LOG_FLAGS_NEEDS_REMAP);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Save the journal slot id that we are working on.
     */
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_set_journal_slot_id(iots_p, slot_id);

    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    memory_request_p = fbe_transport_get_memory_request(in_packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(in_packet_p);

    /* build the memory request for allocation. */
    status = fbe_memory_build_chunk_request(memory_request_p,
                                            FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                            width + 1,/* the +1 is for the sg elements*/
                                            memory_request_priority,
                                            fbe_transport_get_io_stamp(in_packet_p),
                                            (fbe_memory_completion_function_t)fbe_raid_group_journal_remap_memory_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                            raid_group_p);
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_base_object_trace(in_object_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "%s Write_log setup for memory allocation failed, status:0x%x \n", __FUNCTION__, status);        

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_transport_set_status(in_packet_p, FBE_STATUS_OK, 0);
    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_monitor_journal_remap_completion, raid_group_p); 

    fbe_transport_memory_request_set_io_master(memory_request_p, in_packet_p);

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace(in_object_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "%s Write_log memory allocation failed, status:0x%x \n", __FUNCTION__, status);        
        fbe_transport_set_status(in_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);        
        fbe_transport_complete_packet(in_packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    return FBE_LIFECYCLE_STATUS_PENDING;

} // End fbe_raid_group_monitor_journal_remap_cond_function()

/*!****************************************************************************
 * fbe_raid_group_monitor_journal_remap_completion()
 ******************************************************************************
 * @brief
 * This completion function is called after remap (write_verify) operation to a 
 * write_log slot is complete. Free up the allocated resources and if operation is 
 * not successful, mark the slot such that remap is retried. It does not clear the 
 * condition as this is handled by the monitor thread when it checks for remaining 
 * valid slot entries.
 *
 * @param in_packet_p               - pointer to a control packet from the scheduler
 * @param in_context                - context, which is a pointer to the base object 
 * 
 * @return                          - The status of the operation.
 *   
 * @author
 *   8/2/2012 - Created. Vamsi V.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_monitor_journal_remap_completion(
                                                                   fbe_packet_t*                   in_packet_p,
                                                                   fbe_packet_completion_context_t in_context)
{
    fbe_raid_group_t *                      raid_group_p         = (fbe_raid_group_t *) in_context;
    fbe_raid_geometry_t *                   raid_geometry_p      = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_status_t                            status               = fbe_transport_get_status_code(in_packet_p);
    fbe_payload_ex_t *                      sep_payload_p        = fbe_transport_get_payload_ex(in_packet_p);
    fbe_raid_iots_t *                       iots_p;
    fbe_u32_t                               slot_index;
    fbe_memory_request_t *                  memory_request_p = NULL;

    /* unwind the payload
     */
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_get_journal_slot_id(iots_p, &slot_index);
    if (slot_index == FBE_PARITY_WRITE_LOG_INVALID_SLOT)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Write_log remap slot_id is invalid. slot 0x%x. \n",
                              __FUNCTION__, slot_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Free up the memory that was allocated        
     */
    memory_request_p = fbe_transport_get_memory_request(in_packet_p);              
    fbe_memory_free_request_entry(memory_request_p);

    if (status == FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Write_log remap for slot 0x%x completed successfully. \n",
                              __FUNCTION__, slot_index);

        /* Remap is successful. Release the remap slot. */
        fbe_parity_write_log_release_remap_slot(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p, slot_index);

        /* Reschedule immediately so that we will initiate Remap on next write_log slot. */
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const,
                                 (fbe_base_object_t *)raid_group_p,
                                 0);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Write_log remap for slot: 0x%x failed, status:0x%x. Marking for retry. \n", 
                              __FUNCTION__,  slot_index, status);

        /* Remap is not successful. Reset slot state to Allocated so that we will retry Flush on this slot */
        fbe_parity_write_log_set_slot_needs_remap(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p, slot_index);
    }

    return FBE_STATUS_OK;
} // End fbe_raid_group_monitor_journal_remap_completion()


/*!****************************************************************************
 * fbe_raid_group_check_hook()
 ******************************************************************************
 * @brief
 *   This function checks the hook status and returns the appropriate lifecycle status.
 *
 *
 * @param raid_group_p              - pointer to the raid group
 * @param monitor_state             - the monitor state
 * @param monitor_substate          - the substate of the monitor
 * @param val2                      - a generic value that can be used (e.g. checkpoint)
 * @param status                    - pointer to the status
 *
 * @return fbe_status_t   - FBE_STATUS_OK
 * @author
 *   08/9/2011 - Created Jason White
 *   11/01/2011 - moved to base_object.  NCHIU
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_check_hook(fbe_raid_group_t *raid_group_p,
                                       fbe_u32_t monitor_state,
                                       fbe_u32_t monitor_substate,
                                       fbe_u64_t val2,
                                       fbe_scheduler_hook_status_t *status)
{
    fbe_status_t ret_status;
    ret_status = fbe_base_object_check_hook((fbe_base_object_t *)raid_group_p,
                                         monitor_state,
                                         monitor_substate,
                                         val2,
                                         status);
    return ret_status;
}

/*!****************************************************************************
 * fbe_raid_group_monitor_run_rebuild()
 ******************************************************************************
 * @brief
 *   This function determines if a rebuild is able to be 
 *   performed.  If so, it will send a command to the executor to perform a 
 *   rebuild I/O and return a pending status.
 *   
 *
 * @param in_object_p               - pointer to a base object 
 * @param in_packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success
 *                                  - FBE_LIFECYCLE_STATUS_PENDING when I/O is
 *                                    being issued 
 *                                  - FBE_LIFECYCLE_STATUS_DONE  when operation done
 *                                    without I/O is being issued
 * @author
 *   12/07/2009 - Created. Jean Montes.
 *   07/28/2011 - Modified Amit Dhaduk
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_monitor_run_rebuild(
                                fbe_base_object_t*          in_object_p,
                                fbe_packet_t*               in_packet_p)
{
    fbe_raid_group_t*                   raid_group_p = (fbe_raid_group_t*) in_object_p;
    fbe_raid_position_bitmask_t         complete_rebuild_positions = 0; //  positions which needs its rebuild "completed"
    fbe_raid_position_bitmask_t         positions_to_rebuild = 0;   //  bitmask of positions thre will be rebuilt (now) 
    fbe_raid_position_bitmask_t         all_degraded_positions = 0; //  bitmask of all degraded positions
    fbe_bool_t                          can_rebuild_start_b;
    fbe_raid_group_rebuild_context_t*   rebuild_context_p = NULL;
    fbe_lba_t                           checkpoint;
    fbe_scheduler_hook_status_t         hook_status = FBE_SCHED_STATUS_OK;
    fbe_status_t                        status;
    fbe_bool_t                          b_rebuild_enabled;
    fbe_bool_t                          b_checkpoint_reset_needed;
    fbe_medic_action_priority_t         ds_medic_priority;
    fbe_medic_action_priority_t         rg_medic_priority;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_object_p);
    
    if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p))
    {
        status = fbe_raid_group_quiesce_for_bg_op(raid_group_p);
        if (status == FBE_STATUS_PENDING)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: raw mirror rebuild quiescing. \n");
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: raw mirror rebuild quiesced. \n");
    }

    fbe_base_config_is_background_operation_enabled((fbe_base_config_t *) raid_group_p, 
         FBE_RAID_GROUP_BACKGROUND_OP_REBUILD, &b_rebuild_enabled);
    if( b_rebuild_enabled == FBE_FALSE )
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
            FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: run rebuild: rebuild operation: 0x%x is disabled \n",
                              FBE_RAID_GROUP_BACKGROUND_OP_REBUILD);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    ds_medic_priority = fbe_base_config_get_downstream_edge_priority((fbe_base_config_t*)raid_group_p);
    fbe_raid_group_get_medic_action_priority(raid_group_p, &rg_medic_priority);

    if (ds_medic_priority > rg_medic_priority)
    {
        fbe_raid_group_check_hook(raid_group_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, 
                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_DOWNSTREAM_HIGHER_PRIORITY, 0,  &hook_status);
        /* No hook status is handled since we can only log here.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_monitor_run_rebuild downstream medic priority %d > rg medic priority %d\n",
                              ds_medic_priority, rg_medic_priority);
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    /*! @note Get the bitmasks for all degraded positions, positions that need
     *        `rebuild complete' and positions that we are going to rebuild in
     *        this request (must have the same rebuild checkpoint).
     */
    b_checkpoint_reset_needed = FBE_FALSE;
    fbe_raid_group_rebuild_find_disk_needing_action(raid_group_p,
                                                    &all_degraded_positions,
                                                    &positions_to_rebuild,
                                                    &complete_rebuild_positions,
                                                    &checkpoint,
                                                    &b_checkpoint_reset_needed);

    /* Trace some information if enabled.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,
                         "raid_group: run rebuild: degraded: 0x%x rebuild: 0x%x complete: 0x%x checkpoint: 0x%llx \n",
                         all_degraded_positions, positions_to_rebuild, complete_rebuild_positions, (unsigned long long)checkpoint);

    if (b_checkpoint_reset_needed == FBE_TRUE)
    {
        /* Trace the transition from metadata to user data.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: reset rebuild checkpoint for positions: 0x%x\n" , 
                              positions_to_rebuild);
        fbe_raid_group_set_rebuild_checkpoint_to_zero(raid_group_p, in_packet_p, positions_to_rebuild);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
              
    if (checkpoint == 0)
    {
        raid_group_p->background_start_time = fbe_get_time();
    }

    /* Else rebuild is able to run.  Check the debug hooks.
     */
    fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY, checkpoint, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

    if (fbe_raid_group_is_singly_degraded(raid_group_p))
    {
        fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, FBE_RAID_GROUP_SUBSTATE_REBUILD_SINGLY_DEGRADED, checkpoint, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}
    }

    //  Check if a disk was found that needs to complete its rebuild by moving the checkpoint.  Set the condition 
    //  to do so and return. 
    if (complete_rebuild_positions != 0) 
    {
        /* Trace this transition.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: run rebuild checkpoint 0x%llx complete rebuild positions: 0x%x\n" , 
                              (unsigned long long)checkpoint,
                  complete_rebuild_positions);
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, in_object_p, 
            FBE_RAID_GROUP_LIFECYCLE_COND_SET_REBUILD_CHECKPOINT_TO_END_MARKER);

        return FBE_LIFECYCLE_STATUS_DONE; 
    }

    //  If no drives are ready to be rebuilt, then clear the do_rebuild rotary condition and return reschedule.
    //  Note: a drive may still have chunks marked NR, but if the edge is unavailable, it cannot rebuild now.
    //  The condition is cleared in this case.  It will be set again when the disk becomes enabled.
    if (positions_to_rebuild == 0)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  See if we are allowed to do a rebuild I/O based on the current load and active/passive state
    fbe_raid_group_rebuild_determine_if_rebuild_can_start(raid_group_p, &can_rebuild_start_b);
    if (can_rebuild_start_b == FBE_FALSE)
    {
        /* Check the debug hook.*/
        fbe_raid_group_check_hook(raid_group_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, 
                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_PERMISSION, 
                                  0, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) 
        {
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* We were not granted permission.  Wait the normal monitor cycle to see 
         * if the load changes.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Allocate the rebuild context.
     */
    fbe_raid_group_rebuild_allocate_rebuild_context(raid_group_p, in_packet_p, &rebuild_context_p);
    if (rebuild_context_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: run rebuild: failed to allocate rebuild context \n");

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Initialize the rebuild context.
     */
    fbe_raid_group_rebuild_initialize_rebuild_context(raid_group_p, 
                                                      rebuild_context_p,
                                                      positions_to_rebuild,
                                                      checkpoint);

    /* Set time stamp so we can use it to detect slow I/O.
     */
    fbe_transport_set_physical_drive_io_stamp(in_packet_p, fbe_get_time());
    //  Set the completion function for the rebuild condition
    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_monitor_run_rebuild_completion,
        raid_group_p);

    //  Check if LBA is within a LUN and continue accordingly
    fbe_raid_group_rebuild_send_event_to_check_lba(raid_group_p, rebuild_context_p, in_packet_p);

    return FBE_LIFECYCLE_STATUS_PENDING;

} // End fbe_raid_group_monitor_run_rebuild()


/*!****************************************************************************
 * fbe_raid_group_monitor_run_rebuild_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function for the do_rebuild condition.  It is invoked
 *   when a rebuild I/O operation has completed.  It reschedules the monitor to
 *   immediately run again in order to perform the next rebuild operation.
 *
 * @param in_packet_p               - pointer to a control packet from the scheduler
 * @param in_context                - context, which is a pointer to the base object 
 * 
 * @return fbe_status_t             - Always FBE_STATUS_OK
 *   
 * Note: This function must always return FBE_STATUS_OK so that the packet gets
 * completed to the next level.  (If any other status is returned, the packet will 
 * get stuck.)
 *
 * @author
 *   12/07/2009 - Created. Jean Montes.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_monitor_run_rebuild_completion(
                                fbe_packet_t*                   in_packet_p,
                                fbe_packet_completion_context_t in_context)
{

    fbe_status_t                        status;                     //  fbe status
    fbe_raid_group_t                   *raid_group_p;               //  pointer to the raid group 
    fbe_payload_ex_t                   *sep_payload_p;              //  pointer to the payload
    fbe_payload_control_operation_t    *control_operation_p;        //  pointer to the control operation
    fbe_raid_group_rebuild_context_t   *needs_rebuild_context_p;    //  rebuild monitor's context information
    fbe_payload_control_status_t        control_status;
    fbe_time_t time_stamp;
    fbe_u32_t service_time_ms;

    //  Cast the context to a pointer to a raid group object 
    raid_group_p = (fbe_raid_group_t*) in_context;

    //  Get the sep payload and control operation
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    status = fbe_transport_get_status_code(in_packet_p);

    //  Release the payload buffer
    fbe_payload_control_get_buffer(control_operation_p, &needs_rebuild_context_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    /* If enabled trace some information.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,
                         "raid_group: rebuild complete lba: 0x%llx blocks: 0x%llx state: %d status: 0x%x \n",
                         (unsigned long long)needs_rebuild_context_p->start_lba,
                         (unsigned long long)needs_rebuild_context_p->block_count,
                         needs_rebuild_context_p->current_state, status);

    //  Release the context and control operation
    fbe_raid_group_rebuild_release_rebuild_context(in_packet_p);

    //  Check for a failure on the I/O.  If one occurred, trace and return here so the monitor is scheduled on its
    //  normal cycle.
    if ((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, "%s rebuild failed, status: 0x%x\n", __FUNCTION__, status);

        //  Reschedule the next rebuild operation after a short delay
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 100);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We keep the start time in the pdo time stamp since it is not used by raid.
     * The right thing to do is to have a "generic" time stamp. 
     */
    fbe_transport_get_physical_drive_io_stamp(in_packet_p, &time_stamp);
    service_time_ms = 0; //fbe_get_elapsed_milliseconds(time_stamp);

    if (service_time_ms > FBE_RAID_GROUP_MAX_IO_SERVICE_TIME)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Rebuild Completion: service time: %d Reschedule for: %d ms\n",
                              service_time_ms, FBE_RAID_GROUP_SLOW_IO_RESCHEDULE_TIME);
        /* Simply reschedule for a greater time if the I/O took longer than our threshold..
         */
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, 
                                 (fbe_base_object_t*) raid_group_p, 
                                 FBE_RAID_GROUP_SLOW_IO_RESCHEDULE_TIME);
        return FBE_STATUS_OK;
    }

    if(raid_group_p->base_config.background_operation_count < fbe_raid_group_get_rebuild_speed()){
        raid_group_p->base_config.background_operation_count++;
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 0);
    } else {
        raid_group_p->base_config.background_operation_count = 0;
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 200);
    }

    //  Return success in all cases so the packet is completed to the next level 
    return FBE_STATUS_OK;

} // End fbe_raid_group_monitor_run_rebuild_completion()


/*!****************************************************************************
 * fbe_raid_group_metadata_memory_init_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the metadata memory information for the
 *  RAID object.
 *
 * @param in_object_p               - pointer to a base object 
 * @param in_packet_p               - pointer to a control packet from the scheduler.
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when metadata
 *                                    memory init issued.
 *
 * @author
 *  3/23/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_raid_group_metadata_memory_init_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_raid_group_t * raid_group_p = NULL;

    raid_group_p = (fbe_raid_group_t*) object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Do nothing if it is re-specializing.
     */
    if(fbe_base_config_is_flag_set((fbe_base_config_t *) object_p, FBE_BASE_CONFIG_FLAG_RESPECIALIZE))
    {
        fbe_lifecycle_clear_current_cond(object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Set the completion function for the metadata memory initialization. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_metadata_memory_init_cond_completion, raid_group_p);

    /* Initialize the metadata memory for the RAID group object. */
    fbe_base_config_metadata_init_memory((fbe_base_config_t *) raid_group_p, 
                                         packet_p,
                                         &raid_group_p->raid_group_metadata_memory,
                                         &raid_group_p->raid_group_metadata_memory_peer,
                                         sizeof(fbe_raid_group_metadata_memory_t));
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_raid_group_metadata_memory_init_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_metadata_memory_init_cond_completion()
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
fbe_raid_group_metadata_memory_init_cond_completion(fbe_packet_t * packet_p,
                                                    fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                  raid_group_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    raid_group_p = (fbe_raid_group_t*)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* If status is good then clear the metadata memory initialization
     * condition. 
     */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* Clear the metadata memory initialization condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X\n",
                                    __FUNCTION__, status);
        }
    }

    /* If it returns busy status then do not clear the condition and try to 
     * initialize the metadata memory in next monitor cycle.
     */
    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_BUSY)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload status is FBE_PAYLOAD_CONTROL_STATUS_BUSY\n",
                                __FUNCTION__);
        
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_metadata_memory_init_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_metadata_element_init_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the metadata element for the RAID group
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
fbe_raid_group_metadata_element_init_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_raid_group_t * raid_group_p = NULL;

    raid_group_p = (fbe_raid_group_t*)object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Do nothing if it is re-specializing.
     */
    if(fbe_base_config_is_flag_set((fbe_base_config_t *) object_p, FBE_BASE_CONFIG_FLAG_RESPECIALIZE))
    {
        fbe_lifecycle_clear_current_cond(object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Set the completion function for the metadata record initialization. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_metadata_element_init_cond_completion, raid_group_p);

    /* Initialiez the raid group metadata record. */
    fbe_raid_group_metadata_initialize_metadata_element(raid_group_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_raid_group_metadata_element_init_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_metadata_element_init_cond_completion()
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
fbe_raid_group_metadata_element_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                  raid_group_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t status;
    
    raid_group_p = (fbe_raid_group_t*)context;

    /* Get the status of the packet. */
    status = fbe_transport_get_status_code(packet_p);

    /* Get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* Clear the condition if status is ok. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {    
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*) raid_group_p);
        if (status != FBE_STATUS_OK){
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X\n",
                                    __FUNCTION__, status);
        }

        fbe_topology_clear_gate(raid_group_p->base_config.base_object.object_id);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_metadata_element_init_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_nonpaged_metadata_init_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the raid group object's nonpaged
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
fbe_raid_group_nonpaged_metadata_init_cond_function(fbe_base_object_t * object_p,
                                                    fbe_packet_t * packet_p)
{
    fbe_raid_group_t *         raid_group_p = NULL;
    fbe_status_t               status;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    raid_group_p = (fbe_raid_group_t *)object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Do nothing if it is re-specializing.
     */
    if(fbe_base_config_is_flag_set((fbe_base_config_t *) object_p, FBE_BASE_CONFIG_FLAG_RESPECIALIZE))
    {
        fbe_lifecycle_clear_current_cond(object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }    

    fbe_raid_group_check_hook(raid_group_p,
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_NON_PAGED_INIT,  
                              FBE_RAID_GROUP_SUBSTATE_NONPAGED_INIT, 
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* set the completion function for the nonpaged metadata init condition. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_nonpaged_metadata_init_cond_completion, raid_group_p);

    /* initialize non paged metadata for the raid group object. */
    status = fbe_raid_group_nonpaged_metadata_init(raid_group_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_raid_group_nonpaged_metadata_init_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_nonpaged_metadata_init_cond_completion()
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
fbe_raid_group_nonpaged_metadata_init_cond_completion(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *             raid_group_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    raid_group_p = (fbe_raid_group_t*)context;

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
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) raid_group_p);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s nonpaged metadata init condition failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_nonpaged_metadata_init_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_metadata_verify_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to verify the raid group's metadata initialization.
 *  It verify that if non paged metadata is not initialized than it set multiple 
 *  conditions to initialize raid group's metadata.

 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when metadata
 *                                    verification done.
 *
 * @author
 *  3/23/2010 - Created. Dhaval Patel
 *  9/17/2011 - Modified Amit Dhaduk
 *  5/8/2012   - Modified Zhang Yang. Add NP magic munber check
 *  5/16/2012 - Modified by Jingcheng Zhang for metadata version check
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_raid_group_metadata_verify_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_bool_t is_metadata_initialized;
    fbe_bool_t is_active;
    fbe_raid_group_t * raid_group_p = NULL;
    fbe_u32_t  disk_version_size = 0;
    fbe_u32_t  memory_version_size = 0;    
    fbe_base_config_nonpaged_metadata_t    *base_config_nonpaged_metadata_p = NULL;
    fbe_bool_t is_magic_num_valid;    
    fbe_lifecycle_status_t lifecycle_status;
    fbe_raid_group_nonpaged_metadata_t    *raid_group_nonpaged_metadata_p = NULL;
    fbe_scheduler_hook_status_t  hook_status = FBE_SCHED_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p;
    fbe_bool_t journal_committed;

    raid_group_p = (fbe_raid_group_t*)object;  
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry.\n", __FUNCTION__);

    /*check the NP magic number .
        If it is INVALID, the NP is not load correctly
        the object will stuck in this condition function until 
        its NP is set correctly by caller*/
    fbe_base_config_metadata_is_nonpaged_state_valid((fbe_base_config_t *) object, &is_magic_num_valid);
    if(is_magic_num_valid == FBE_FALSE)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    is_active = fbe_base_config_is_active((fbe_base_config_t *)raid_group_p);
    fbe_base_config_metadata_is_initialized((fbe_base_config_t *)raid_group_p, &is_metadata_initialized);

    lifecycle_status = fbe_raid_group_monitor_push_keys_if_needed(object, packet_p);
    if(lifecycle_status != FBE_LIFECYCLE_STATUS_CONTINUE){
        return lifecycle_status;
    }

    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&raid_group_nonpaged_metadata_p);
    if (is_metadata_initialized &&
        fbe_raid_group_is_drive_tier_query_needed(raid_group_p) && 
        raid_group_nonpaged_metadata_p->drive_tier == FBE_DRIVE_PERFORMANCE_TIER_TYPE_UNINITIALIZED) 
    {
        /* If a debug hook is set, we need to execute the specified action*/
        fbe_raid_group_check_hook(raid_group_p, 
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_VERIFY, 
                                  FBE_RAID_GROUP_SUBSTATE_METADATA_VERIFY_SPECIALIZE, 0, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE)
        { 
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        fbe_raid_group_monitor_update_nonpaged_drive_tier(object, packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }


    /*version check.  the non-paged metadata on disk may have different version with current
      non-paged metadata structure in memory. compare the size and set default value for the
      new version in memory*/
    if (is_metadata_initialized) {
        fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&base_config_nonpaged_metadata_p);
        fbe_base_config_get_nonpaged_metadata_size((fbe_base_config_t *)raid_group_p, &memory_version_size);
        fbe_base_config_nonpaged_metadata_get_version_size(base_config_nonpaged_metadata_p, &disk_version_size);
        if (disk_version_size != memory_version_size) {
            /*new software is operating old version data during upgrade, set default value for the new version*/
            /*currently these region already zerod, the new version developer should set correct value here*/
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid group nonpaged verify - on-disk size: %d in-memory size: %d update new values here.\n",
                              disk_version_size, memory_version_size);            
        }

        /* Set flag nonpaged metadata is initialized */
        fbe_base_config_set_clustered_flag((fbe_base_config_t *)raid_group_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_NONPAGED_INITIALIZED);
    }

    if (!is_active){ /* This will take care of passive side */
        if(is_metadata_initialized == FBE_TRUE){
            fbe_lifecycle_clear_current_cond((fbe_base_object_t *) raid_group_p);
            fbe_base_config_clear_clustered_flag((fbe_base_config_t *) raid_group_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST);
            fbe_raid_group_set_flag(raid_group_p, FBE_RAID_GROUP_FLAG_INITIALIZED);

            /* We need to init the journal write log for parity RGs ..*/
            if (FBE_TRUE == fbe_raid_geometry_is_parity_type(raid_geometry_p)){
                fbe_raid_group_metadata_is_journal_committed(raid_group_p, &journal_committed);
                fbe_raid_geometry_init_journal_write_log(raid_geometry_p);
               }
        } else {
            /* Ask again for the nonpaged metadata update */
            if(!fbe_base_config_is_clustered_flag_set((fbe_base_config_t *) raid_group_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST)){
                fbe_base_config_set_clustered_flag((fbe_base_config_t *) raid_group_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST);
            }
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    if (!is_metadata_initialized) { /* We active and newly created */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s We active and newly created\n", __FUNCTION__);

        /* Set all relevant conditions */
        /* We are initializing the object first time and so update the 
         * non paged metadata, paged metadata and non paged metadata signature in order.
         */

         /* set the nonpaged metadata init condition. */
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                               (fbe_base_object_t *)raid_group_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_NONPAGED_METADATA);

        /* set the zero consumed area condition. */
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                               (fbe_base_object_t *)raid_group_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_ZERO_CONSUMED_AREA);

        /* set the paged metadata init condition. */
       fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                              (fbe_base_object_t *)raid_group_p,
                              FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_PAGED_METADATA);

        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                               (fbe_base_object_t *)raid_group_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_PERSIST_DEFAULT_NONPAGED_METADATA);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if (fbe_raid_group_is_c4_mirror_raid_group(raid_group_p)) {
        fbe_bool_t is_loadded = FBE_FALSE;
        fbe_raid_group_c4_mirror_is_config_loadded(raid_group_p, &is_loadded);
        if (is_loadded == FBE_FALSE) {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: load c4mirror map info\n", __FUNCTION__);
            fbe_raid_group_c4_mirror_load_config(raid_group_p, packet_p);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
    }

    /* We need to init the journal write log for parity RGs ..*/
    if (FBE_TRUE == fbe_raid_geometry_is_parity_type(raid_geometry_p))
    {
        fbe_raid_group_metadata_is_journal_committed(raid_group_p, &journal_committed);
        fbe_raid_geometry_init_journal_write_log(raid_geometry_p);
    }
    /*! @todo, for now the location of this set is temporary.  
     * It needs to be after the point where we can start to process events. 
     */
    fbe_raid_group_set_flag(raid_group_p, FBE_RAID_GROUP_FLAG_INITIALIZED);

    fbe_base_config_clear_clustered_flag((fbe_base_config_t *) raid_group_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST);

    /* clear the metadata verify condition */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *) raid_group_p);

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_raid_group_metadata_verify_cond_function()
 ******************************************************************************/


/*!***************************************************************************
 * fbe_raid_group_zero_metadata_cond_function()
 *****************************************************************************
 *
 * @brief   This method will zero any non-user space for the raid group this 
 *          includes:
 *              o non-paged metadata area
 *              o journal area (only allocated for parity raid groups)
 *
 * @param   object_p               - pointer to a base object.
 * @param   packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  7/21/2010 - Created. Dhaval Patel
 *
 *****************************************************************************/
static fbe_lifecycle_status_t 
fbe_raid_group_zero_metadata_cond_function(fbe_base_object_t * object_p,
                                           fbe_packet_t * packet_p)
{
    fbe_raid_group_t *                 raid_group_p = NULL;
    fbe_status_t                status;
    fbe_base_config_downstream_health_state_t   downstream_health_state;    
    fbe_raid_position_t                     position;

    raid_group_p = (fbe_raid_group_t *)object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    //  Get the error edge position and clear any bits set on the edge
    status = fbe_raid_group_get_error_edge_position(raid_group_p, &position);
    if (status != FBE_STATUS_ATTRIBUTE_NOT_FOUND) 
    {
        // We have error bits on the edge to clear
        fbe_raid_group_send_clear_errors_usurper(raid_group_p, packet_p, position);

        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    downstream_health_state = fbe_raid_group_verify_downstream_health(raid_group_p);

    /* When we get here, the raid group is first created.  We expected all the downstream objects
     * to be available.
     */
    if (downstream_health_state != FBE_DOWNSTREAM_HEALTH_OPTIMAL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s not all drives are ready: 0x%x\n",
                                __FUNCTION__, downstream_health_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Set the completion function before we zero the paged metadata area
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_zero_metadata_cond_completion, raid_group_p);
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_zero_journal_completion, raid_group_p);

    /* Zero the metadata area for this raid group
     */
    status = fbe_raid_group_metadata_zero_metadata(raid_group_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_raid_group_zero_metadata_cond_function()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_group_zero_journal_completion()
 ******************************************************************************
 * @brief
 *  Kick off the write of zeros of the journal area.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  3/14/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_zero_journal_completion(fbe_packet_t * packet_p,
                                       fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                         raid_group_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    
    raid_group_p = (fbe_raid_group_t *)context;
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(payload, &iots_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* Status is good, so we can continue with init of journal area. 
         * Set our iots lba to the start of the journal to help track the operation. 
         */
        fbe_lba_t journal_log_start_lba;
        fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_log_start_lba);
        if (journal_log_start_lba != FBE_LBA_INVALID)
        {
            /* Only parity raid groups have a journal area.  For other raid groups this is invalid.
             */
            iots_p->lba = journal_log_start_lba;
            fbe_raid_group_monitor_initialize_journal((fbe_base_object_t*)raid_group_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s zero paged metadata area condition failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_zero_journal_completion()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_group_zero_metadata_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for zeroing the paged metadata
 *  area.
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
fbe_status_t 
fbe_raid_group_zero_metadata_cond_completion(fbe_packet_t * packet_p,
                                                  fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                         raid_group_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    raid_group_p = (fbe_raid_group_t *)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* clear zero consumed area condition if status is good. */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) raid_group_p);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s zero paged metadata area condition failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_zero_metadata_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_write_default_paged_metadata_cond_function()
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
fbe_lifecycle_status_t 
fbe_raid_group_write_default_paged_metadata_cond_function(fbe_base_object_t * object_p,
                                                          fbe_packet_t * packet_p)
{
    fbe_raid_group_t *         raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_status_t               status;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);
    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p)){
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);

        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* set the completion function for the signature init condition. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_write_default_paged_metadata_cond_completion, raid_group_p);

    /* initialize signature for the raid group object. */
    status = fbe_raid_group_metadata_write_default_paged_metadata(raid_group_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_raid_group_paged_metadata_write_default_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_write_default_paged_metadata_cond_completion()
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
fbe_raid_group_write_default_paged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                            fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *             raid_group_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    raid_group_p = (fbe_raid_group_t*)context;

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
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) raid_group_p);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_wt_def_paged_mdata_cond_compl write def paged metadata cond fail,stat:0x%X,ctrl_stat:0x%X\n",
                              status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_write_default_paged_metadata_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_write_default_nonpaged_metadata_cond_function()
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
fbe_raid_group_write_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p,
                                                             fbe_packet_t * packet_p)
{
    fbe_raid_group_t *         raid_group_p = NULL;
    fbe_status_t                    status;

    raid_group_p = (fbe_raid_group_t*)object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* set the completion function for the paged metadata init condition. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_write_default_nonpaged_metadata_cond_completion, raid_group_p);

    /* initialize signature for the raid group object. */
    status = fbe_raid_group_metadata_write_default_nonpaged_metadata(raid_group_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_raid_group_write_default_nonpaged_metadata_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_c4_mirror_write_default_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion for c4mirror map information write
 *  condition.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  04/07/2015 - Created. Jamin Kang
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_c4_mirror_write_default_completion(fbe_packet_t * packet_p,
                                                 fbe_packet_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_group_t *raid_group_p;

    raid_group_p = (fbe_raid_group_t*)context;
    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);
    /* verify the status and take appropriate action. */
    if(status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s write default map for c4mirror failed, status:0x%X\n",
                              __FUNCTION__, status);
    } else {
        /* clear nonpaged metadata write default condition if status is good. */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) raid_group_p);
    }

    return FBE_STATUS_OK;
}


/*!****************************************************************************
 * fbe_raid_group_write_default_nonpaged_metadata_cond_completion()
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
fbe_raid_group_write_default_nonpaged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                                    fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *             raid_group_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    raid_group_p = (fbe_raid_group_t*)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        if (fbe_raid_group_is_c4_mirror_raid_group(raid_group_p))
        {
            /* Write default map information for c4mirror */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s write default map for c4mirror\n",
                              __FUNCTION__);
            fbe_transport_set_completion_function(packet_p, fbe_raid_group_c4_mirror_write_default_completion, raid_group_p);
            fbe_raid_group_c4_mirror_write_default(raid_group_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            /* clear nonpaged metadata write default condition if status is good. */
            fbe_lifecycle_clear_current_cond((fbe_base_object_t *) raid_group_p);
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s write default nonpaged metadata condition failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_write_default_nonpaged_metadata_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_persist_default_nonpaged_metadata_cond_function()
 ******************************************************************************
 *
 * @brief   This function is used to persist the default nonpaged metadata for the 
 *          raid group object during object initialization.
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
static fbe_lifecycle_status_t fbe_raid_group_persist_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p, 
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
 * end fbe_raid_group_persist_default_nonpaged_metadata_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_eval_mark_nr_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to evaluate NR marking for the RAID group object.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when signature
 *                                    verification issued.
 *
 * @author
 *  6/20/2011 - Rewrote. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_eval_mark_nr_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_raid_group_t *  raid_group_p = NULL;
    fbe_bool_t          b_is_active;
    fbe_scheduler_hook_status_t  hook_status = FBE_SCHED_STATUS_OK;
    
    raid_group_p = (fbe_raid_group_t*)object;

    /* If a debug hook is set, we need to execute the specified action*/
    fbe_raid_group_check_hook(raid_group_p, 
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE)
    { 
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    fbe_raid_group_lock(raid_group_p);

    if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_STARTED))
    {
        fbe_raid_group_unlock(raid_group_p);

        if (b_is_active)
        {
            return fbe_raid_group_eval_mark_nr_active(raid_group_p, packet_p);
        }
        else
        {
            return fbe_raid_group_eval_mark_nr_passive(raid_group_p, packet_p);
        }
    }
    else if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_DONE))
    {
        fbe_raid_group_unlock(raid_group_p);
        return fbe_raid_group_eval_mark_nr_done(raid_group_p, packet_p);
    }
    else
    {
        fbe_raid_group_unlock(raid_group_p);
        return fbe_raid_group_eval_mark_nr_request(raid_group_p, packet_p);
    }
}
/******************************************************************************
 * end fbe_raid_group_eval_mark_nr_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_eval_rb_logging_cond_function()
 ******************************************************************************
 * @brief
 *  This fn determines if rebuild logging needs to be started.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when signature
 *                                    verification issued.
 *
 * @author
 *  6/27/2011 - Rewrote Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_eval_rb_logging_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_raid_group_t *         raid_group_p = NULL;
    fbe_bool_t                 b_is_active;
        
    raid_group_p = (fbe_raid_group_t*)object;       
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    fbe_raid_group_lock(raid_group_p);

    if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_STARTED))
    {
        fbe_raid_group_unlock(raid_group_p);

        if (b_is_active)
        {
            return fbe_raid_group_set_rb_logging_active(raid_group_p, packet_p);
        }
        else
        {
            return fbe_raid_group_set_rb_logging_passive(raid_group_p, packet_p);
        }
    }
    else if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_DONE))
    {

        /* we don't need this timer any more */
        fbe_raid_group_set_check_rg_broken_timestamp(raid_group_p, 0);

        fbe_raid_group_unlock(raid_group_p);
        return fbe_raid_group_eval_rb_logging_done(raid_group_p, packet_p);
    }
    else
    {
        /* we need to reset the degraded timer*/
        fbe_raid_group_set_check_rg_broken_timestamp(raid_group_p, 0);

        fbe_raid_group_unlock(raid_group_p);
        return fbe_raid_group_eval_rb_logging_request(raid_group_p, packet_p);
    }
}
/******************************************************************************
 * end fbe_raid_group_eval_rb_logging_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_clear_rl_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to stop rb logging for the RAID group object.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  7/13/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_clear_rl_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_raid_group_t *  raid_group_p = NULL;
    fbe_bool_t          b_is_active;
    
    raid_group_p = (fbe_raid_group_t*)object;
    
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    fbe_raid_group_lock(raid_group_p);

    if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_STARTED))
    {
        fbe_raid_group_unlock(raid_group_p);

        if (b_is_active)
        {
            return fbe_raid_group_clear_rl_active(raid_group_p, packet_p);
        }
        else
        {
            return fbe_raid_group_clear_rl_passive(raid_group_p, packet_p);
        }
    }
    else if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_DONE))
    {
        fbe_raid_group_unlock(raid_group_p);
        return fbe_raid_group_clear_rl_done(raid_group_p, packet_p);
    }
    else
    {
        fbe_raid_group_unlock(raid_group_p);
        return fbe_raid_group_clear_rl_request(raid_group_p, packet_p);
    }
}
/******************************************************************************
 * end fbe_raid_group_clear_rl_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_configuration_change_cond_function()
 ******************************************************************************
 * @brief
 *  This fn handles the details of changing configuration.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when signature
 *                                    verification issued.
 *
 * @author
 *  10/18/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_raid_group_configuration_change_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_raid_group_t *         raid_group_p = NULL;
    fbe_bool_t                 b_is_active;
    fbe_base_config_t * base_config_p = (fbe_base_config_t *)object;

    raid_group_p = (fbe_raid_group_t*)object;
    if (base_config_p->base_object.class_id == FBE_CLASS_ID_VIRTUAL_DRIVE){

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);

        if (status != FBE_STATUS_OK){
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
    }
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    fbe_raid_group_lock(raid_group_p);

    if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_STARTED))
    {
        fbe_raid_group_unlock(raid_group_p);

        if (b_is_active)
        {
            return fbe_raid_group_change_config_active(raid_group_p, packet_p);
        }
        else
        {
            return fbe_raid_group_change_config_passive(raid_group_p, packet_p);
        }
    }
    else if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_DONE))
    {
        fbe_raid_group_unlock(raid_group_p);
        return fbe_raid_group_change_config_done(raid_group_p, packet_p);
    }
    else
    {
        fbe_raid_group_unlock(raid_group_p);
        return fbe_raid_group_change_config_request(raid_group_p, packet_p);
    }
}
/******************************************************************************
 * end fbe_raid_group_configuration_change_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_passive_wait_for_ds_edges_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used by the passive side.
 *  We delay the join request, in case we have edges down 
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   FBE_LIFECYCLE_STATUS_DONE
 *
 * @author
 *  8/3/2011 - Created. Maya Dagon
 *
 ******************************************************************************/
fbe_lifecycle_status_t
fbe_raid_group_passive_wait_for_ds_edges_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t  status;
    fbe_bool_t    b_is_active;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_class_id_t  class_id;
    fbe_bool_t      raid_group_degraded;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
    if (!b_is_active)
    {
        status = raid_group_check_downstream_edges_enabled(raid_group_p, packet_p);
        if (status != FBE_STATUS_OK) 
        {
            fbe_time_t elapsed_time;
            fbe_bool_t b_is_timeout_expired;
            #define FBE_RAID_GROUP_PASSIVE_ACTIVATE_DEGRADED_WAIT_SEC 5
    
            /*! We wait 5 second to give a chance to the edges to come back
             */
            fbe_raid_group_needs_rebuild_determine_rg_broken_timeout(raid_group_p, FBE_RAID_GROUP_PASSIVE_ACTIVATE_DEGRADED_WAIT_SEC,
                                                                     &b_is_timeout_expired, &elapsed_time);
            if (b_is_timeout_expired)
            {
                fbe_raid_group_set_check_rg_broken_timestamp(raid_group_p, 0);
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s some edges not enabled\n",
                                      __FUNCTION__);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s Waiting for drives to come up .\n",
                                      __FUNCTION__);
                return FBE_LIFECYCLE_STATUS_DONE; 
            }           
        }
    }

    /* we need to clean up the timer before clearing the condition */
    fbe_raid_group_set_check_rg_broken_timestamp(raid_group_p, 0);

    /* set the DEGRADED attribute to upper objects if the RG is degraded*/
    class_id = fbe_raid_group_get_class_id(raid_group_p);
    raid_group_degraded = fbe_raid_group_is_degraded(raid_group_p);
    if(FBE_CLASS_ID_VIRTUAL_DRIVE != class_id && FBE_TRUE == raid_group_degraded)
    {
        fbe_block_transport_server_set_path_attr_all_servers(&((fbe_base_config_t *)raid_group_p)->block_transport_server,
                                                             FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED);
    }

    /* Below the striper level, raid groups need to advance checkpoint to check for incomplete writes.
     */
    if (!fbe_raid_geometry_is_raid10(raid_geometry_p) &&
        fbe_raid_group_encryption_is_active(raid_group_p)) {
        fbe_raid_group_encryption_iw_check(raid_group_p, packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;

}
/******************************************************************************
 * end fbe_raid_group_passive_wait_for_ds_edges_cond_function()
 ******************************************************************************/


/*!****************************************************************************
 * raid_group_passive_request_join_cond_function()
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
 *  7/21/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
raid_group_passive_request_join_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_raid_group_t *  raid_group_p = NULL;
    fbe_bool_t          b_is_active;

    raid_group_p = (fbe_raid_group_t*)object_p;

    /* This condition is only used by the passive side to join.
     */
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
    if (b_is_active)
    {
        /* If we were passive and we became active, then 
         * we might need to re-run some conditions in activate. 
         * We know this happened if we did not run those preset conditions as active. 
         */
        if (!fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_ACTIVATE_ACTIVE))
        {
            /* here set any conditions that we need to re-run in activate if we become active.
             * We need to re run these conditions since we are now active.
             */
            fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, object_p,
                                   FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING_ACTIVATE);
            fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, object_p,
                                   FBE_RAID_GROUP_LIFECYCLE_COND_SETUP_FOR_VERIFY_PAGED_METADATA);
            fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, object_p,
                                   FBE_RAID_GROUP_LIFECYCLE_COND_VERIFY_PAGED_METADATA); 
            fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, object_p,
                                   FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_JOURNAL_FLUSH);

            /* Also clear our local state since we will need to re-do this.
             */
            fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_ALL_MASKS);
        }
        else
        {
            /* We are active and we do not need to run this condition in activate.
             */
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                      FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s failed to clear condition %d\n", __FUNCTION__, status);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "Active clear passive join condition\n");
            }
            fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_ACTIVATE);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    fbe_raid_group_setup_passive_encryption_state(raid_group_p);


    fbe_raid_group_lock(raid_group_p);
    if(fbe_cmi_is_peer_alive() &&
       fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)raid_group_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN))
    {
        /* If we decided we are broken, then we need to set the broken condition now.
             */ 
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive trying to join. Active broken. Go broken.local: 0x%llx peer: 0x%llx\n",                               
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.flags);                              
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *)raid_group_p, 
                               FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If the peer have not requested a join, it must be cleared for some reason.
     * The passive side should go back and start join from the beginning. 
     */
     if ((fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_JOIN_STARTED) || 
          fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_JOIN_DONE)) &&
         !fbe_base_config_is_any_peer_clustered_flag_set((fbe_base_config_t*)raid_group_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK))
     {
         fbe_base_config_clear_clustered_flag((fbe_base_config_t*)raid_group_p, 
                                              FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK);
         fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_JOIN_MASK);

         fbe_raid_group_unlock(raid_group_p);
         fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "Passive join peer REQ !set local: 0x%llx peer: 0x%llx/0x%x state:0x%llx\n", 
                               raid_group_p->raid_group_metadata_memory.base_config_metadata_memory.flags,
                               raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.flags,
                               raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.lifecycle_state,
                               raid_group_p->local_state);
         return FBE_LIFECYCLE_STATUS_DONE;
    }

    if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_JOIN_STARTED))
    {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                               "PASSIVE join started objectid: %d local state: 0x%llx\n",
                   object_p->object_id,
                   (unsigned long long)raid_group_p->local_state);
        return fbe_raid_group_join_passive(raid_group_p, packet_p);
    }
    else if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_JOIN_DONE))
    {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "PASSIVE join done objectid: %d local state: 0x%llx\n",
                   object_p->object_id,
                   (unsigned long long)raid_group_p->local_state);
        return fbe_raid_group_join_done(raid_group_p, packet_p);
    }
    else
    {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "PASSIVE join request objectid: %d local state: 0x%llx\n",
                   object_p->object_id,
                   (unsigned long long)raid_group_p->local_state);
        return fbe_raid_group_join_request(raid_group_p, packet_p);
    }
}
/******************************************************************************
 * end raid_group_passive_request_join_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * raid_group_active_allow_join_cond_function()
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
 *  6/20/2011 - Rewrote. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
raid_group_active_allow_join_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_raid_group_t *  raid_group_p = NULL;
    fbe_bool_t          b_is_active;

    raid_group_p = (fbe_raid_group_t*)object;
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
    
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Active join cond entry local: 0x%llx peer: 0x%llx state:0x%llx\n", 
                          (unsigned long long)raid_group_p->raid_group_metadata_memory.base_config_metadata_memory.flags,
                          (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.flags,
                          (unsigned long long)raid_group_p->local_state);

    /* This condition only runs on the active side, so if we are passive, then clear it now.
     */
    if (!b_is_active)
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to clear condition %d\n", __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    fbe_raid_group_lock(raid_group_p);

    /* If the peer is not there or they have not requested a join, then clear this condition.
     * The active side only needs to try to sync when the peer is present. 
     */
     if (!fbe_cmi_is_peer_alive() ||
         !fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)raid_group_p, 
                                                     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ))
     {
         fbe_scheduler_hook_status_t  hook_status = FBE_SCHED_STATUS_OK;
         fbe_check_rg_monitor_hooks(raid_group_p,
                                    SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
                                    FBE_RAID_GROUP_SUBSTATE_JOIN_DONE, 
                                    &hook_status);
         if (hook_status == FBE_SCHED_STATUS_DONE)
         {
             fbe_raid_group_unlock(raid_group_p);
             return FBE_LIFECYCLE_STATUS_DONE;
         }
         /* Only clear the clustered join in the case where the peer has gone away.
          * If the joined is set, the peer has successfully joined. 
          */
         if (!fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)raid_group_p, 
                                                            FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED))
         {
             fbe_base_config_clear_clustered_flag((fbe_base_config_t*)raid_group_p, 
                                                     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK);
         }
         fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_JOIN_MASK);

         status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
         if (status != FBE_STATUS_OK)
         {
             fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                   FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s failed to clear condition %d\n", __FUNCTION__, status);
         }
         fbe_raid_group_unlock(raid_group_p);
         fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "Active join peer !alive or REQ !set local: 0x%llx peer: 0x%llx/0x%x state:0x%llx\n", 
                               raid_group_p->raid_group_metadata_memory.base_config_metadata_memory.flags,
                               raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.flags,
                               raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.lifecycle_state,
                               raid_group_p->local_state);
         return FBE_LIFECYCLE_STATUS_DONE;
    }

     /* If there is a `change in progress' wait until it completes.
      */
    if (fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS))
    {
        /* Trace a warning and clear the join condition.  When the `change' 
         * completes the join will be attempted again.
         */
         status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
         if (status != FBE_STATUS_OK)
         {
             fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                   FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s failed to clear condition %d\n", __FUNCTION__, status);
         }
         fbe_raid_group_unlock(raid_group_p);
         fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "ACTIVE join objectid: 0x%x local state: 0x%llx delay - swap request in progress\n", 
                              object->object_id, raid_group_p->local_state);
         return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If we have already started, continue.
     */
    if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_JOIN_STARTED))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "ACTIVE join started objectid: %d local state: 0x%llx\n",
                   object->object_id,
                   (unsigned long long)raid_group_p->local_state);

        fbe_raid_group_unlock(raid_group_p);
        return fbe_raid_group_join_active(raid_group_p, packet_p);
    }
    else if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_JOIN_DONE))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "ACTIVE join done objectid: %d local state: 0x%llx\n",
                   object->object_id,
                   (unsigned long long)raid_group_p->local_state);
        fbe_raid_group_unlock(raid_group_p);
        return fbe_raid_group_join_done(raid_group_p, packet_p);
    }
    else
    {
        
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "ACTIVE join request objectid: %d local state: 0x%llx\n",
                   object->object_id,
                   (unsigned long long)raid_group_p->local_state);
        return fbe_raid_group_join_request(raid_group_p, packet_p);
    }
}
/******************************************************************************
 * end raid_group_active_allow_join_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * raid_group_join_sync_cond_function()
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
 *  2/23/2012 - Create. Naizhong
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
raid_group_join_sync_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_raid_group_t *      raid_group_p = NULL;
    fbe_bool_t              b_is_active = FBE_FALSE;
    fbe_lifecycle_state_t   local_state= FBE_LIFECYCLE_STATE_INVALID, peer_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_base_config_clustered_flags_t local_flags = 0, peer_flags = 0;

    raid_group_p = (fbe_raid_group_t*)object;
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t *)raid_group_p, &peer_state);
    fbe_base_config_metadata_get_lifecycle_state((fbe_base_config_t *)raid_group_p, &local_state);
    fbe_base_config_get_clustered_flags((fbe_base_config_t *) raid_group_p, &local_flags);
    fbe_base_config_get_peer_clustered_flags((fbe_base_config_t *) raid_group_p, &peer_flags);

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Sync join cond %s entry local_bcf: 0x%llx peer_f: 0x%llx state:0x%x pState:0x%x\n", 
                          b_is_active ? "active" : "passive",
                          (unsigned long long)local_flags, (unsigned long long)peer_flags,
                          local_state, peer_state);

    /* If peer does not exists, or peer drops out of the process, we can move on.
     */
    if ((b_is_active) && (peer_state != FBE_LIFECYCLE_STATE_READY))
    {
        fbe_base_config_clear_clustered_flag((fbe_base_config_t*)raid_group_p, 
                                             FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK);
        fbe_raid_group_clear_local_state(raid_group_p, (FBE_RAID_GROUP_LOCAL_STATE_JOIN_STARTED |
                                                        FBE_RAID_GROUP_LOCAL_STATE_JOIN_DONE));

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to clear condition %d\n", __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    return fbe_raid_group_join_sync_complete(raid_group_p, packet_p);
}
/******************************************************************************
 * end raid_group_join_sync_cond_function()
 ******************************************************************************/
/*!****************************************************************************
 * raid_group_peer_death_release_sl_function()
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
raid_group_peer_death_release_sl_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_group_t *      raid_group_p = (fbe_raid_group_t *)base_object_p;
    

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Release all SL \n ", __FUNCTION__);
    fbe_stripe_lock_release_peer_data_stripe_locks(&raid_group_p->base_config.metadata_element);

    /* Clean up any background operations.
     */
    fbe_raid_group_background_op_peer_died_cleanup(raid_group_p);

    fbe_metadata_element_clear_abort_monitor_ops(&((fbe_base_config_t *)raid_group_p)->metadata_element);

    /* clear the condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *) raid_group_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X", __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end raid_group_peer_death_release_sl_function()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_group_eval_rl_activate_cond_function()
 ******************************************************************************
 * @brief
 *  This fn determines if rebuild logging needs to be started.
 *  This condition only runs on the active side at activate time.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when signature
 *                                    verification issued.
 *
 * @author
 *  7/27/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_eval_rl_activate_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_status_t                                  status;
    fbe_raid_group_t *                            raid_group_p = NULL;
    fbe_bool_t                                    b_is_active;
    fbe_base_config_downstream_health_state_t     downstream_health;
    fbe_lifecycle_status_t                        lifecycle_status;
    fbe_time_t                                    elapsed_time;
    fbe_bool_t                                    b_is_timeout_expired;
    fbe_raid_geometry_t*                          raid_geometry_p = NULL; 
    fbe_class_id_t                                class_id; 
    fbe_scheduler_hook_status_t  hook_status = FBE_SCHED_STATUS_OK;
    

    raid_group_p = (fbe_raid_group_t*)object;
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    
    /* If a debug hook is set, we need to execute the specified action*/
    fbe_raid_group_check_hook(raid_group_p, 
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE)
    { 
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    /* Make sure we have the current version of downstream block sizes.
     */
    fbe_raid_geometry_refresh_block_sizes(raid_geometry_p);

    /* We need to clear any flags we might have sent when we received
     * an event for drives that went away.
     */
    fbe_metadata_element_clear_abort(&raid_group_p->base_config.metadata_element);

    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    /* This condition only runs on the active side, so if we are passive, then clear it now.
     */
    if (!b_is_active)
    {
        /* we need to clean up the timer prior to clearing the condition */
        fbe_raid_group_set_check_rg_broken_timestamp(raid_group_p, 0);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to clear condition %d\n", __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /*! @todo Add activate eval code here. 
     */ 

    class_id = fbe_raid_geometry_get_class_id(raid_geometry_p);    
    if(class_id != FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        downstream_health = fbe_raid_group_verify_downstream_health(raid_group_p);
        if(downstream_health == FBE_DOWNSTREAM_HEALTH_BROKEN)
        {
            fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_NONE,
                                 "%s: Going broken\n", __FUNCTION__);        
            status = fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, object,
                                            FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                      FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s failed to set condition %d\n", __FUNCTION__, status);
            }
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        else if(downstream_health == FBE_DOWNSTREAM_HEALTH_DISABLED)
        {
            fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_NONE,
                                 "%s: Setting the disabled condition\n", __FUNCTION__);        
            status = fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, object,
                                            FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                      FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s failed to set condition %d\n", __FUNCTION__, status);
            }
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    
        else if(downstream_health == FBE_DOWNSTREAM_HEALTH_DEGRADED)
        { 
            fbe_bool_t b_need_rl_disk_wait;

            /* If all disks are coming up we may still be missing one or two
             * Check if we are coming from specialize or fail or hibernate and will have to wait
             */
            b_need_rl_disk_wait = fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_NEED_RL_DRIVE_WAIT);
            
            if (!b_need_rl_disk_wait)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s No need to wait for drives to come back before marking rl.\n",
                                      __FUNCTION__);
            }
                                   
            /* Rebuild logging will not be set until the timer expires giving time for the drives to powerup */
            fbe_raid_group_needs_rebuild_determine_rg_broken_timeout(raid_group_p, FBE_RAID_GROUP_ACTIVATE_DEGRADED_WAIT_SEC,
                                                                     &b_is_timeout_expired, &elapsed_time);

            /* If a debug hook is set, we need to execute the specified action*/
            fbe_raid_group_check_hook(raid_group_p, 
                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_START, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY, 0, &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE)
            { 
                return FBE_LIFECYCLE_STATUS_DONE;
            }
            
            /* If time is up or there's no need to wait since drives weren't just coming up, go ahead and eval mark rl
             * We clear the NEED_RL_DRIVE_WAIT flag now, so next time (if we get here from READY or we've already waited
             * and marked rl) we won't see it and won't wait.
             */            
            if (b_is_timeout_expired || !b_need_rl_disk_wait)
            {
                /* If a debug hook is set, we need to execute the specified action*/
                fbe_raid_group_check_hook(raid_group_p, 
                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_EXPIRE, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY, 0, &hook_status);
                if (hook_status == FBE_SCHED_STATUS_DONE)
                { 
                    return FBE_LIFECYCLE_STATUS_DONE;
                }
                
                fbe_raid_group_set_check_rg_broken_timestamp(raid_group_p, 0);

                /* Clear the flag that tells us we needed to wait
                 */
                fbe_raid_group_lock(raid_group_p);
                fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_NEED_RL_DRIVE_WAIT);
                fbe_raid_group_unlock(raid_group_p);

                lifecycle_status = fbe_raid_group_activate_set_rb_logging(raid_group_p, packet_p);

                if(lifecycle_status == FBE_LIFECYCLE_STATUS_PENDING)
                {
                    return lifecycle_status;
                }
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s Waiting for drives to come back before marking rl.\n",
                                      __FUNCTION__);
                return FBE_LIFECYCLE_STATUS_DONE; 
            }
                        
        } /* end if downstream health is degraded */

    }
    
             
    /* When the activate eval is done, we need to set this local state so we know that
     * we perfomed the eval as active. The passive side needs this if it becomes active.
     */
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_ACTIVATE_ACTIVE);
    fbe_raid_group_unlock(raid_group_p);

    /* we need to clean up the timer prior to clearing the condition */
    fbe_raid_group_set_check_rg_broken_timestamp(raid_group_p, 0);

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to clear condition %d\n", __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_raid_group_eval_rl_activate_cond_function()
 ******************************************************************************/
/*!**************************************************************
 * fbe_raid_group_clear_peer_state()
 ****************************************************************
 * @brief
 *   Clear out the peer state as part of transitioning
 *   from one state to another.
 *  
 * @param raid_group_p - The raid group object.
 * 
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING
 *                                    - if we are not done yet.
 *                                  FBE_LIFECYCLE_STATUS_DONE
 *                                    - if we are done.
 *
 * @author
 *  7/25/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_lifecycle_status_t fbe_raid_group_clear_peer_state(fbe_raid_group_t *raid_group_p)
{
    fbe_lifecycle_state_t lifecycle_state;
    fbe_status_t status;
    /* When we change states it is important to clear out our masks for things
     * related to the peer. 
     * For the activate -> ready transition we do not want to clear the flags. 
     * At the point of activate we have initted these peer to peer flags and 
     * have joined the peer. 
     */
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_ALL_MASKS);

    status = fbe_lifecycle_get_state(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p, &lifecycle_state);
    if ( (lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_READY) &&
         (fbe_raid_group_is_any_clustered_flag_set(raid_group_p, 
                                                   FBE_RAID_GROUP_CLUSTERED_FLAG_ALL_MASKS) ||
          fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t*)raid_group_p, 
                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_ALL_MASKS) ) )
    {
        fbe_base_config_clear_clustered_flag((fbe_base_config_t*)raid_group_p, 
                                             FBE_BASE_CONFIG_CLUSTERED_FLAG_ALL_MASKS);                        
        fbe_raid_group_clear_clustered_flag_and_bitmap(raid_group_p, 
                                                       FBE_RAID_GROUP_CLUSTERED_FLAG_ALL_MASKS);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "rg clearing clustered flags obj_state: 0x%x l_bc_flags: 0x%llx peer: 0x%llx local state: 0x%llx\n", 
                              raid_group_p->base_config.base_object.lifecycle.state,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)raid_group_p->local_state);
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    fbe_raid_group_unlock(raid_group_p);
    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************
 * end fbe_raid_group_clear_peer_state()
 **************************************/


fbe_status_t
fbe_raid_group_pending_destroy_disable_bg_completion(fbe_packet_t                    *packet, 
                                                     fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_raid_group_pending_func()
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
 *  10/27/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_lifecycle_status_t 
fbe_raid_group_pending_func(fbe_base_object_t *base_object_p, fbe_packet_t * packet)
{
    fbe_lifecycle_status_t lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;
    fbe_base_config_t * base_config_p = NULL;
    fbe_raid_group_t *raid_group_p = NULL;
    fbe_status_t status;
    fbe_trace_level_t trace_level = FBE_TRACE_LEVEL_DEBUG_HIGH;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_u32_t outstanding_io = 0;
    fbe_bool_t b_is_active;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    base_config_p = (fbe_base_config_t *)base_object_p;
    raid_group_p = (fbe_raid_group_t*) base_object_p;

    lifecycle_status = fbe_raid_group_clear_peer_state(raid_group_p);
    if (lifecycle_status != FBE_LIFECYCLE_STATUS_DONE)
    {
        return lifecycle_status;
    }

    /* Empty the event queue */
    fbe_raid_group_empty_event_queue(raid_group_p);

    /* If there are I/Os outstanding raise the trace level to info
     */
    fbe_block_transport_server_get_outstanding_io_count(&base_config_p->block_transport_server, &outstanding_io);

    /* If there are I/O outstanding raise the trace level.
     */
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
                          "%s entry lifecycle status: %d state: %d outstanding I/O: %d attr: 0x%llx\n", 
                          __FUNCTION__, lifecycle_status, lifecycle_state,
                          outstanding_io, 
                          (unsigned long long)base_config_p->block_transport_server.attributes);

    /* We must update the path state before any operations are allowed to fail back.
     * The upper levels expect the path state to change before I/Os get bad status. 
     */
    if (lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_READY) {
        fbe_block_transport_server_update_path_state(&base_config_p->block_transport_server,
                                                     &fbe_base_config_lifecycle_const,
                                                     (fbe_base_object_t*)raid_group_p,
                                                     FBE_BLOCK_PATH_ATTR_FLAGS_NONE);
    }

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

    status = fbe_lifecycle_get_state(&fbe_base_config_lifecycle_const, base_object_p, &lifecycle_state);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s fbe_lifecycle_get_state failed, status = %X", __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* Abort all queued I/Os.
     * Don't do it if we're pending ready since there are no I/Os in flight. 
     * Also the block trans server changes the path state to enabled immediately after we drain, so 
     * to prevent aborted metadata I/Os don't set the bit now, clear it. 
     */
    switch(lifecycle_state)
    {
    case FBE_LIFECYCLE_STATE_PENDING_READY:
        fbe_metadata_element_clear_abort(&raid_group_p->base_config.metadata_element);
        /* we are moving into ready state, after peer sets JOIN_SYNCING */
        fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
        if (!b_is_active)
        {
            fbe_base_config_set_clustered_flag((fbe_base_config_t*)raid_group_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING); 
        }
        break;

    case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
    {
#if 0
        fbe_semaphore_t             sem;

        // Initialize semaphore and set completion to trigger semaphore on completion of disable background operation.
        fbe_semaphore_init(&sem, 0, 1);
        fbe_transport_set_completion_function(packet, fbe_raid_group_pending_destroy_disable_bg_completion, &sem);

        // Disable all background operations and abort all IOs.        
        fbe_base_config_disable_background_operation(&raid_group_p->base_config, packet, FBE_RAID_GROUP_BACKGROUND_OP_ALL);

        fbe_semaphore_wait(&sem, NULL);
        fbe_semaphore_destroy(&sem); 
#endif        

        fbe_metadata_element_destroy_abort_io(&raid_group_p->base_config.metadata_element);
        if (!fbe_topology_is_reference_count_zero(base_config_p->base_object.object_id))
        {
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "RG pending saw non-zero reference count.\n");
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
        break;
    }
 
    default:
        fbe_metadata_element_abort_io(&raid_group_p->base_config.metadata_element);
        break;
    }

    fbe_raid_group_check_hook(raid_group_p,  
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_PENDING, 
                              FBE_RAID_GROUP_SUBSTATE_PENDING, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s skip drain of terminator queue due to hook 0x%x\n", __FUNCTION__, hook_status);
    }
    else
    {
        /* Call the below function to fail out any I/O that might have received an 
         * error and needs to get restarted. 
         */
        status = fbe_raid_group_handle_shutdown(raid_group_p);

        /* Drain any usurpers that are queued.
         */
        status = fbe_raid_group_drain_usurpers(raid_group_p);
    }

    if (status == FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                              trace_level,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s draining block transport server attributes: 0x%llx\n", __FUNCTION__, 
                              (unsigned long long)base_config_p->block_transport_server.attributes);
        /* We simply call to the block transport server to handle pending.
         */
        lifecycle_status = fbe_block_transport_server_pending(&base_config_p->block_transport_server,
                                                              &fbe_base_config_lifecycle_const,
                                                              (fbe_base_object_t *) base_config_p);
    }
    else
    {
        /* We are not done yet.  Wait for I/Os to finish.
         * We still drain the block transport server so any waiting requests in the bts can get freed 
         * before we drain all requests to the raid library. 
         */
        lifecycle_status = fbe_block_transport_server_pending(&base_config_p->block_transport_server,
                                                              &fbe_base_config_lifecycle_const,
                                                              (fbe_base_object_t *) base_config_p);
        lifecycle_status = FBE_LIFECYCLE_STATUS_PENDING;
    }
    if (lifecycle_status == FBE_LIFECYCLE_STATUS_DONE)
    {
        fbe_metadata_element_clear_abort(&base_config_p->metadata_element);
    }

    /* clean up the degraded timer */
    fbe_raid_group_set_check_rg_broken_timestamp(raid_group_p, 0);

    fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                          trace_level,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                          "%s exit status: %d outstanding I/O: %d attributes: 0x%llx\n", 
                          __FUNCTION__, lifecycle_status,
                          outstanding_io, 
                          (unsigned long long)base_config_p->block_transport_server.attributes);

    /* If we are done, then clear any bits set on the edge.
     */
    if (lifecycle_status == FBE_LIFECYCLE_STATUS_DONE)
    {
        fbe_raid_position_t position;

        /* Clear the cache
         */
        fbe_base_config_metadata_paged_clear_cache(base_config_p);

        status = fbe_raid_group_get_error_edge_position(raid_group_p, &position);
        if (status != FBE_STATUS_ATTRIBUTE_NOT_FOUND)
        {
            /* We have error bits on the edge to clear.
             */
            fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
            fbe_raid_group_send_clear_errors_usurper(raid_group_p, packet, position); 
            fbe_transport_wait_completion(packet);
            lifecycle_status = FBE_LIFECYCLE_STATUS_PENDING;
        }
    }
    if (fbe_base_config_is_rekey_mode((fbe_base_config_t *)raid_group_p) &&
        fbe_raid_group_bg_op_is_flag_set(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_OUTSTANDING)) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group:  bg op outstanding.  Wait for it to complete.\n");
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    return lifecycle_status;
}
/**************************************
 * end fbe_raid_group_pending_func()
 **************************************/

/*!****************************************************************************
 * raid_group_hibernation_sniff_wakeup_cond_function
 ******************************************************************************
 *
 * @brief
 *    This function wakes up every once a while and checks if we are sleeping
 *    for a time specified in base config. If so, we wake up which will also
 *    cause us to sniff. After a while the system will switch back to power saving
 *
 * @param   in_raid_group_p         -  pointer to raid group
 * @param   in_medic_action         -  medic action
 *
 * @return  fbe_status_t            -  status of this operation  
 *
 ******************************************************************************/
static fbe_lifecycle_status_t raid_group_hibernation_sniff_wakeup_cond_function( fbe_base_object_t* in_object_p,
                                                                                      fbe_packet_t*      in_packet_p  )
{
    fbe_raid_group_t *                  raid_group_p = (fbe_raid_group_t* )in_object_p;
    fbe_base_config_t *                 base_config = (fbe_base_config_t *)in_object_p;
    fbe_u32_t                           seconds_sleeping = 0;
    fbe_system_power_saving_info_t      power_saving_info;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_class_id_t                      my_class_id = FBE_CLASS_ID_INVALID;
    
    status = fbe_lifecycle_clear_current_cond(in_object_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to clear condition %d\n", __FUNCTION__, status);                

    }

    /*since VD is a type of a RAID group, we want to make sure VD will not do this check*/
    my_class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *) raid_group_p));  
    if (my_class_id == FBE_CLASS_ID_VIRTUAL_DRIVE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }
                                                                                                       
    /*how long have we been sleeping?*/
    seconds_sleeping = fbe_get_elapsed_seconds(base_config->hibernation_time);

    /*do we need to wake up ?*/
    fbe_base_config_get_system_power_saving_info(&power_saving_info);
    if ((seconds_sleeping / 60) >= power_saving_info.hibernation_wake_up_time_in_minutes) {
        /*yes, we do. Let's just go to activeate and this will make sure our logical and physical drives 
        will wake up too as well so they can serve us*/

        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Waking up from power saving to do a sniff\n", __FUNCTION__);   

        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                                in_object_p,
                                FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);

        /* When disks come back up we may still be missing one or two when we leave hibernate
         * Note that when we get to activate we will give the missing drive(s) a few more seconds to come up
         */
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_NEED_RL_DRIVE_WAIT);
        fbe_raid_group_unlock(raid_group_p);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}


/*!****************************************************************************
 * fbe_raid_group_monitor_check_active_state()
 ******************************************************************************
 * @brief
 *   This function will check if the RG object is active or passive on this SP.
 *   If the RG object is passive, it will perform any processing, such as clearing 
 *   the condition, if that is needed.  At this time, no special processing is
 *   required. 
 *
 * @param in_raid_group_p           - pointer to a raid group object
 * @param out_is_active_b_p         - pointer to output that gets set to FBE_TRUE
 *                                    if the RG object is active on this SP 
 *
 * @return  fbe_status_t 
 *
 * @author
 *   05/06/2010 - Created. Jean Montes.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_monitor_check_active_state(
                                fbe_raid_group_t*               in_raid_group_p,
                                fbe_bool_t*                     out_is_active_b_p)
{

    //  Determine if this is the active RG object or not 
    *out_is_active_b_p = fbe_base_config_is_active((fbe_base_config_t*) in_raid_group_p);

    //  Return success
    return FBE_STATUS_OK;

} // End fbe_raid_group_monitor_check_active_state()


/*!**************************************************************
 * raid_group_stripe_lock_start_cond_function()
 ****************************************************************
 * @brief
 *  Will start stripe locking.
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE    
 *
 * @author
 *  9/27/2010 - Created. Peter Puhov
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
raid_group_stripe_lock_start_cond_function(fbe_base_object_t * object_p, 
                                               fbe_packet_t * packet_p)
{   
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_stripe_lock_operation_t * stripe_lock_operation = NULL;
    fbe_raid_group_t * raid_group = (fbe_raid_group_t *)object_p;
    fbe_raid_geometry_t                    *raid_geometry_p;
    fbe_class_id_t                          class_id; 

    /* Do nothing if it is re-specializing.
     */
    if(fbe_base_config_is_flag_set((fbe_base_config_t *) object_p, FBE_BASE_CONFIG_FLAG_RESPECIALIZE))
    {
        fbe_lifecycle_clear_current_cond(object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }    

    if(fbe_base_config_stripe_lock_is_started((fbe_base_config_t *) object_p)){
        status = fbe_lifecycle_clear_current_cond(object_p);
        if (status != FBE_STATUS_OK){
            fbe_base_object_trace(object_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //if(fbe_base_config_is_active((fbe_base_config_t *)object_p)){
        payload = fbe_transport_get_payload_ex(packet_p);
        stripe_lock_operation = fbe_payload_ex_allocate_stripe_lock_operation(payload);
        fbe_payload_stripe_lock_build_start(stripe_lock_operation, &((fbe_base_config_t *)object_p)->metadata_element);

        /* We may choose to allocate the memory here, but for now ... */
        ((fbe_base_config_t *)object_p)->metadata_element.stripe_lock_blob = &raid_group->stripe_lock_blob;
        raid_group->stripe_lock_blob.size = METADATA_STRIPE_LOCK_SLOTS;

        /* This is just test code */
        /* For all raid groups except VD allocate memory for hash */
        raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group);
        class_id = fbe_raid_geometry_get_class_id(raid_geometry_p); 

        if(class_id != FBE_CLASS_ID_VIRTUAL_DRIVE){
            fbe_u8_t * ptr = fbe_transport_allocate_buffer();
            ((fbe_base_config_t *)object_p)->metadata_element.stripe_lock_hash = (fbe_metadata_stripe_lock_hash_t *)ptr;
            fbe_metadata_stripe_hash_init(ptr, FBE_MEMORY_CHUNK_SIZE, FBE_RAID_DEFAULT_CHUNK_SIZE);
        }

        fbe_payload_ex_increment_stripe_lock_operation_level(payload);
        fbe_transport_set_completion_function(packet_p, raid_group_stripe_lock_start_cond_completion, object_p);
        fbe_stripe_lock_operation_entry(packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    //} 

    //return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************
 * end raid_group_stripe_lock_start_cond_function()
 ******************************************/

static fbe_status_t 
raid_group_stripe_lock_start_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_base_config_t * base_config = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_stripe_lock_operation_t * stripe_lock_operation = NULL;

    base_config = (fbe_base_config_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(payload);
    fbe_payload_ex_release_stripe_lock_operation(payload, stripe_lock_operation);

    if(fbe_base_config_stripe_lock_is_started(base_config)){
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_config);
        if (status != FBE_STATUS_OK){
            fbe_base_object_trace((fbe_base_object_t *)base_config, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************************
 *          fbe_raid_group_generate_error_trace()
 *****************************************************************************
 *
 * @brief   This method simply generates a trace event of level `error'.  This
 *          has not affect unless the `stop on error' is set.
 *
 * @param   raid_group_p - The raid group.
 *
 * @return status - The status of the operation.
 *
 * @author
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_generate_error_trace(fbe_raid_group_t * raid_group_p)
{
    fbe_object_id_t                 rg_object_id = fbe_raid_group_get_object_id(raid_group_p);
    fbe_raid_group_debug_flags_t    debug_flags = 0;

    /* Get the current debug flags
     */
    fbe_raid_group_get_debug_flags(raid_group_p, &debug_flags);

    /* Now generate the error trace.
     */
    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_RAID_GROUP_DEBUG_FLAG_NONE,
                         "raid_group: Generate error trace requested for rg obj: 0x%x (%d)\n",
                         rg_object_id, rg_object_id);

    /* This is a `one shot'.  Clear the flag.
     */
    debug_flags &= ~FBE_RAID_GROUP_DEBUG_FLAGS_GENERATE_ERROR_TRACE;
    fbe_raid_group_set_debug_flags(raid_group_p, debug_flags);

    return FBE_STATUS_OK;

}
/**************************************************
 * end fbe_raid_group_generate_error_trace()
 **************************************************/
/*!**************************************************************
 * fbe_raid_group_check_outstanding_bg_ops()
 ****************************************************************
 * @brief
 *  Check to see if the pin or unpin has been outstanding for so 
 *  long that we need to trace it.
 *
 * @param raid_group_p - Current object.           
 *
 * @return None.
 *
 * @author
 *  3/18/2014 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_group_check_outstanding_bg_ops(fbe_raid_group_t *raid_group_p)
{
    fbe_u32_t elapsed_ms;
    fbe_bool_t b_is_pin;

    fbe_raid_group_lock(raid_group_p);
    b_is_pin = fbe_raid_group_bg_op_is_flag_set(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_PIN_OUTSTANDING);

    if (fbe_raid_group_bg_op_is_flag_set(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_PIN_OUTSTANDING) ||
        fbe_raid_group_bg_op_is_flag_set(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_UNPIN_OUTSTANDING)) {

        fbe_time_t pin_time = fbe_raid_group_bg_op_get_pin_time(raid_group_p);
        elapsed_ms = fbe_get_elapsed_milliseconds(pin_time);
        fbe_raid_group_unlock(raid_group_p);

        if (elapsed_ms > FBE_RAID_GROUP_PIN_TIME_LIMIT_MS) {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: %s has been outstanding for %d ms\n",
                                  (b_is_pin) ? "pin" : "unpin",
                                  elapsed_ms);
        }
    } else {
        fbe_raid_group_unlock(raid_group_p);
    }
}
/******************************************
 * end fbe_raid_group_check_outstanding_bg_ops()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_background_monitor_operation_cond_function()
 ****************************************************************
 * @brief
 *  This is the common condition function that checks Background monitor operation,
 *  including rebuild and verify and run it if needed based on following priority 
 *  order. 
 *
 *  Background operation      priority
 *  --------------------      -------------
 *  Rebuild operation         High priority
 *  Verify operation          second priority(after rebuild)
 *
 *  This condition runs only one background operation at a time on active side.
 *  It will not evaluate lower priority background operation if higher priority 
 *  operation needs to run.
 *  
 * @param in_object_p - The pointer to the raid group.
 * @param in_packet_p - The control packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE or
 *                                  status returned from sub condition function
 *
 * @author
 *  07/28/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_raid_group_background_monitor_operation_cond_function(fbe_base_object_t * object_p,
                                                               fbe_packet_t * packet_p)
{

    fbe_raid_group_t *          raid_group_p = (fbe_raid_group_t* )object_p;
    fbe_lifecycle_status_t      lifecycle_status;
    fbe_status_t                status;
    fbe_event_t *               data_event_p = NULL;
    fbe_event_data_request_t    data_request;
    fbe_data_event_type_t       data_event_type;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_power_save_state_t      power_save_state;
    fbe_bool_t                          b_is_active;
    fbe_system_encryption_mode_t		system_encryption_mode;
    fbe_raid_group_bg_op_info_t *bg_op_p = NULL;
    fbe_bool_t journal_committed;
    fbe_bool_t                   b_refresh_block_size;
    fbe_scheduler_hook_status_t hook_status;

    /*ATTENTION, we need to be here BEFORE any rebuild activity so we catch the fact the previous rebuild
    is done and we send notification for it. Otherwise, or cached PVDs we rebuilt will not be correct*/
    fbe_raid_group_data_reconstruction_progress_function(raid_group_p, packet_p);

    if (fbe_base_config_is_rekey_mode((fbe_base_config_t *)raid_group_p) &&
        fbe_raid_group_bg_op_is_flag_set(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_OUTSTANDING)) {

        /* Check to see if we need to trace when the packet is outstanding for too long.
         */
        fbe_raid_group_check_outstanding_bg_ops(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    /* If the `generate error trace' flag is set generate an error trace
     */
    if (fbe_raid_group_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAGS_GENERATE_ERROR_TRACE))
    {
        fbe_raid_group_generate_error_trace(raid_group_p);
    }

    /* Look at the first event in the queue*/
    if(!fbe_base_config_event_queue_is_empty((fbe_base_config_t *)raid_group_p))
    {
        /* To avoid starvation of RG background operation by flood of events, we alternate between
         * processing the events and the Raid Group Background operation
         */
        if(fbe_raid_group_is_flag_set(raid_group_p, FBE_RAID_GROUP_FLAG_PROCESS_EVENT))
        {
            fbe_raid_group_clear_flag(raid_group_p, FBE_RAID_GROUP_FLAG_PROCESS_EVENT);
            fbe_base_config_event_queue_front_event_in_use((fbe_base_config_t *)raid_group_p, &data_event_p);
            if(data_event_p != NULL)
            {
        
                //  Get data event type from data request
                fbe_event_get_data_request_data(data_event_p, &data_request);
                data_event_type = data_request.data_event_type;

                switch(data_event_type)
                {
                    case FBE_DATA_EVENT_TYPE_REMAP:
                         return fbe_raid_group_process_remap_request(raid_group_p, data_event_p, packet_p);
                         break;
        
                    case FBE_DATA_EVENT_TYPE_MARK_VERIFY:
                    case FBE_DATA_EVENT_TYPE_MARK_IW_VERIFY:
                         return fbe_raid_group_verify_process_mark_verify_event(raid_group_p, data_event_p, packet_p);
                         break;
                    default:
                        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "%s: data event type:0x%x not supported\n",
                                              __FUNCTION__, data_event_type);
                        status = FBE_STATUS_GENERIC_FAILURE;
                        break;
                }
              
            }
        }
        else
        {
            fbe_raid_group_set_flag(raid_group_p, FBE_RAID_GROUP_FLAG_PROCESS_EVENT);
        }
    
    }

    /* check if firmware download operation needs to run. 
     * Firmware download shall be granted on both Active and Passive sides.
     */
    if(fbe_raid_group_start_download_after_rebuild(raid_group_p) && 
       fbe_raid_group_time_to_start_download(raid_group_p) &&
        !fbe_raid_group_is_degraded(raid_group_p))
    {
        /* Set EVAL_DOWNLOAD condition to start download */
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, object_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_DOWNLOAD_REQUEST);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)raid_group_p);

    /* Check whether we should set NOT_PREFERRED attr for SLF */
    if (b_is_active && fbe_raid_group_background_op_check_slf(raid_group_p) == FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* We should not run the background condition if we are quiesced or quiescing. 
     * The Block Transport Server is held in this case so issuing a 
     * monitor operation will cause the monitor operation to be held, and since 
     * we only unhold in the monitor context, we will never be able to unhold. 
     */
    if (!fbe_raid_geometry_is_raw_mirror(raid_geometry_p) &&
        fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t *)raid_group_p, 
                    (FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED|FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING)))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "quiesced or quiescing do not run background monitor condition.\n");
        /* Just return done.  Since we haven't cleared the condition we will be re-scheduled.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Before allowing any background operation to run, 
     * check if we need to update our block sizes. 
     */
    fbe_raid_group_should_refresh_block_sizes(raid_group_p, &b_refresh_block_size);
    if (b_refresh_block_size) {
        fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_BACKGROUND_CONDITION, 
                                  FBE_RAID_GROUP_SUBSTATE_BACKGROUND_COND_BL_REFRESH_NEEDED, 0, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) { return FBE_LIFECYCLE_STATUS_DONE; }
        /* Set the conditions to start the drain.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "refresh bl/s needed start drain bc_l: 0x%llx bc_p: 0x%llx ls: 0x%llx\n", 
                              raid_group_p->raid_group_metadata_memory.base_config_metadata_memory.flags, 
                              raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.flags,
                              raid_group_p->local_state);
        fbe_base_config_set_clustered_flag((fbe_base_config_t *) raid_group_p, 
                                           FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD);
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE);
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Set the upstream edge priority based on rg and downstream medic priority
     */
    fbe_raid_group_set_upstream_medic_action_priority(raid_group_p);
    
    /* Order of rebuild is metadata then user data
     * Note that journal is no longer rebuilt here since it can collide
     *  with user IO which is enabled here.
     * Journal is now rebuilt as part of clear rebuild logging condition
     */
    if(fbe_raid_group_background_op_is_metadata_rebuild_need_to_run(raid_group_p) &&
       b_is_active == FBE_TRUE)
    {
        /* run rebuild background operation */
        lifecycle_status = fbe_raid_group_monitor_run_metadata_rebuild(object_p, packet_p);
        return lifecycle_status;
    }

    /* check if rebuild background operation needs to run */
    if(fbe_raid_group_background_op_is_rebuild_need_to_run(raid_group_p) &&
       b_is_active == FBE_TRUE)
    {
        /* run rebuild background operation */
        lifecycle_status = fbe_raid_group_monitor_run_rebuild(object_p, packet_p);
        return lifecycle_status;
    }

    fbe_raid_group_metadata_is_journal_committed(raid_group_p, &journal_committed);

    if((FBE_TRUE == fbe_raid_geometry_is_parity_type(raid_geometry_p)) && journal_committed && 
       !fbe_raid_geometry_journal_is_flag_set(raid_geometry_p, FBE_PARITY_WRITE_LOG_FLAGS_4K_SLOT)) {
        /* The journal is committed but we have not set the journal flag. We need to initiate the setting of this
         * flag which will be done as part of unqueise*/

        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Journal flag for 4K not set.\n",
                             __FUNCTION__);
        fbe_raid_group_start_drain(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;

    }
    if(fbe_raid_group_is_degraded(raid_group_p)) {
        fbe_raid_group_trace(raid_group_p, 
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING,
                             "%s RG is degraded. Cannot do verify\n",
                             __FUNCTION__);
    }
    else {
        if(b_is_active && fbe_database_is_4k_drive_committed() && !journal_committed) {
            /* we need to commit the Journal */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s Commit the 4K NP flags \n",
                                 __FUNCTION__);
            fbe_raid_group_metadata_write_4k_committed(raid_group_p,packet_p);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
        /* check if metadata verify background operation needs to run */
        if(fbe_raid_group_background_op_is_journal_verify_need_to_run(raid_group_p) &&
           b_is_active == FBE_TRUE)
        {
            /* run metadata verify background operation */
            lifecycle_status = fbe_raid_group_monitor_run_journal_verify(object_p, packet_p);
            return lifecycle_status;
        }

        /* check if metadata verify background operation needs to run */
        if(fbe_raid_group_background_op_is_metadata_verify_need_to_run(raid_group_p, FBE_RAID_BITMAP_VERIFY_ALL) &&
           b_is_active == FBE_TRUE)
        {
            /* run metadata verify background operation */
            lifecycle_status = fbe_raid_group_monitor_run_metadata_verify(object_p, packet_p);
            return lifecycle_status;
        }


        /* check if verify background operation needs to run */
        if(fbe_raid_group_background_op_is_verify_need_to_run(raid_group_p) && 
           b_is_active == FBE_TRUE)
        {           
            /* run verify background operation */
            lifecycle_status = fbe_raid_group_monitor_run_verify(object_p, packet_p);
            return lifecycle_status;
        }


        /* If system is in encryption mode, and we do not have keys we should requests them. */
        fbe_database_get_system_encryption_mode(&system_encryption_mode);
        if(system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED){
            lifecycle_status = fbe_raid_group_background_handle_encryption_state_machine(raid_group_p, packet_p);
            if(lifecycle_status != FBE_LIFECYCLE_STATUS_CONTINUE){
                return lifecycle_status;
            }			
        } /* if(system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) */
    }/* if(fbe_raid_group_is_degraded(raid_group_p)) */

    /* Make sure we free up any memory we might have allocated for a bg op.
     */
    bg_op_p = fbe_raid_group_get_bg_op_ptr(raid_group_p);

    if ((bg_op_p != NULL) &&
        (bg_op_p->flags == 0)) {
        fbe_memory_native_release(bg_op_p);
        fbe_raid_group_set_bg_op_ptr(raid_group_p, NULL);
    }

    /* Lock base config before getting power saving state */
    fbe_base_config_lock((fbe_base_config_t *)raid_group_p);
    
    /*if we made it all the way here, we can start looking at power savings*/
    fbe_base_config_metadata_get_power_save_state((fbe_base_config_t *)raid_group_p, &power_save_state);

    /* Unlock base config after getting power saving state */
    fbe_base_config_unlock((fbe_base_config_t *)raid_group_p);

    if (power_save_state != FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE) {
        base_config_check_power_save_cond_function(object_p, packet_p);
    }

    return FBE_LIFECYCLE_STATUS_DONE;

}
/******************************************************************************
 * end fbe_raid_group_background_monitor_operation_cond_function()
 ******************************************************************************/

/*!**************************************************************
 * raid_group_quiesced_background_operation_cond_function()
 ****************************************************************
 * @brief
 *  Some raid groups, like the raw mirror rg, only do background
 *  operations when they are quiesced.
 *  
 * @param object_p - The pointer to the raid group.
 * @param packet_p - The control packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE or
 *                                  status returned from sub condition function
 *
 * @author
 *  6/19/2012 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
raid_group_quiesced_background_operation_cond_function(fbe_base_object_t * object_p,
                                                       fbe_packet_t * packet_p)
{
    fbe_raid_group_t *          raid_group_p = (fbe_raid_group_t* )object_p;
    fbe_lifecycle_status_t      lifecycle_status;
    fbe_status_t                status;

    /*ATTENTION, we need to be here BEFORE any rebuild activity so we catch the fact the previous rebuild
    is done and we send notification for it. Otherwise, or cached PVDs we rebuilt will not be correct*/
    fbe_raid_group_data_reconstruction_progress_function(raid_group_p, packet_p);
    
    /* If the `generate error trace' flag is set generate an error trace
     */
    if (fbe_raid_group_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAGS_GENERATE_ERROR_TRACE))
    {
        fbe_raid_group_generate_error_trace(raid_group_p);
    }
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                         "raid_group: quiesced bg ops starting\n");

    if ( !(fbe_base_config_is_active((fbe_base_config_t*)raid_group_p)))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: quiesced bg ops not active clearing cond\n");
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n", __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* we are going to perform only unit IO from this condition as we can't hold RG in quiesced for longer time
     * so clearing the condition before start the background operation IO.
     */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n", __FUNCTION__, status);
    }
    
    if(fbe_raid_group_background_op_is_rebuild_need_to_run(raid_group_p))
    {
        lifecycle_status = fbe_raid_group_monitor_run_rebuild(object_p, packet_p);
        return lifecycle_status;
    }
    if (!fbe_raid_group_is_degraded(raid_group_p) &&
        fbe_raid_group_background_op_is_verify_need_to_run(raid_group_p))
    {

        lifecycle_status = fbe_raid_group_monitor_run_verify(object_p, packet_p);
        return lifecycle_status;
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                         "raid_group: quiesced bg ops done\n");
    
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end raid_group_quiesced_background_operation_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_rekey_operation_cond_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for the rekey operation.  We intentionally
 *  clear the condition so that the unquiesce can proceed.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/28/2013 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_rekey_operation_cond_completion(fbe_packet_t * packet_p, 
                                               fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_status_t status;
    /* we are going to perform only unit IO from this condition as we can't hold RG in quiesced for longer time
     * so clearing the condition before start the background operation IO.
     */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n", __FUNCTION__, status);
    }
    /* We always clear the condition so we can unquiesce immediately.
     */
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_rekey_operation_cond_completion()
 **************************************/
/*!**************************************************************
 * raid_group_rekey_operation_cond_function()
 ****************************************************************
 * @brief
 *  Some portions of the rekey are under quiesce such as the
 *  begin, end and transition from one drive to another.
 *  
 * @param object_p - The pointer to the raid group.
 * @param packet_p - The control packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE or
 *                                  status returned from sub condition function
 *
 * @author
 *  10/29/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
raid_group_rekey_operation_cond_function(fbe_base_object_t * object_p,
                                         fbe_packet_t * packet_p)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t* )object_p;
    fbe_status_t status;
    fbe_base_config_metadata_memory_t * metadata_memory_p = NULL;
    fbe_base_config_metadata_memory_t * metadata_memory_peer_p = NULL;

    /* Get a pointer to metadata memory */
    fbe_metadata_element_get_metadata_memory(&raid_group_p->base_config.metadata_element, (void **)&metadata_memory_p);
    fbe_metadata_element_get_peer_metadata_memory(&raid_group_p->base_config.metadata_element, (void **)&metadata_memory_peer_p);
    
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                         "raid_group: rekey ops starting attr: 0x%x cl_f: %llx/%llx\n",
                          raid_group_p->base_config.metadata_element.attributes,
                          (unsigned long long)((metadata_memory_p) ? metadata_memory_p->flags : 0),
                          (unsigned long long)((metadata_memory_peer_p) ? metadata_memory_peer_p->flags : 0));

    if ( !(fbe_base_config_is_active((fbe_base_config_t*)raid_group_p))){
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: rekey no longer active clearing cond\n");
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK){
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n", __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    /* We need to make sure no I/Os are in flight since we are rewriting the paged and 
     * paged locks will be needed. 
     */
    if (!fbe_base_config_should_quiesce_drain((fbe_base_config_t*)raid_group_p)) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: rekey not quiesce hold\n");
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK){
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n", __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_rekey_operation_cond_completion, raid_group_p);
    return fbe_raid_group_continue_encryption(raid_group_p, packet_p);
}
/******************************************************************************
 * end raid_group_rekey_operation_cond_function()
 ******************************************************************************/

/*!**************************************************************
 * raid_group_downstream_health_broken_cond_function()
 ****************************************************************
 * @brief
 *  This is the derived condition function for parity object 
 *  where it will clear this condition when it finds health with
 *  downstream object is either ENABLE or DEGRADED.
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
raid_group_downstream_health_broken_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_raid_group_t *                        raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_status_t                              status;
    fbe_base_config_downstream_health_state_t downstream_health_state;
    fbe_bool_t                                b_is_active;
    fbe_lifecycle_state_t                     peer_state;
    fbe_raid_position_t                       position;

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    //  Get the error edge position and clear any bits set on the edge
    status = fbe_raid_group_get_error_edge_position(raid_group_p, &position);
    if (status != FBE_STATUS_ATTRIBUTE_NOT_FOUND) 
    {
        // We have error bits on the edge to clear
        fbe_raid_group_send_clear_errors_usurper(raid_group_p, packet_p, position);

        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* When disks come back up we may still be missing one or two when we leave fail
     * Note that when we get to activate we will give the missing drive(s) a few more seconds to come up
     */
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_NEED_RL_DRIVE_WAIT);

    if(!fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)raid_group_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN))
    {
        fbe_base_config_set_clustered_flag((fbe_base_config_t*)raid_group_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN);
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    fbe_raid_group_unlock(raid_group_p);

    /* Look for all the edges with downstream objects.
       */
    downstream_health_state = fbe_raid_group_verify_downstream_health(raid_group_p);

    switch (downstream_health_state)
    {
    case FBE_DOWNSTREAM_HEALTH_OPTIMAL:
    case FBE_DOWNSTREAM_HEALTH_DEGRADED:
    case FBE_DOWNSTREAM_HEALTH_DISABLED:

        /* If we are passive, we need to keep this condition set, and check every 3 second,
         * unless peer is not in failed state.
         */
        b_is_active = fbe_base_config_is_active((fbe_base_config_t*)raid_group_p);
        fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t*)raid_group_p, &peer_state);
        if (!b_is_active && (peer_state == FBE_LIFECYCLE_STATE_FAIL))
        {
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        /* Currently clear the condition in all cases, later logic
         * will be changed to integrate different use cases.
         */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
        /* Jump to Activate state */
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *)raid_group_p, FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
        break;

    case FBE_DOWNSTREAM_HEALTH_BROKEN: 
        //  Since the downstream health is broken, clear the condition to evaluate NR on demand 
        fbe_lifecycle_force_clear_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                                       FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING);
        break;

    default:
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
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
 * end raid_group_downstream_health_broken_cond_function()
 *************************************************/

/*!**************************************************************
 * raid_group_downstream_health_disabled_cond_function()
 ****************************************************************
 * @brief
 *  This is the derived condition function for parity and mirror
 *  which handles the disabled condition   
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is be based on parent
 *                                  function call.
 *
 * @author
 *  8/18/2009 - Created. Dhaval Patel.
 *
 ****************************************************************/

fbe_lifecycle_status_t 
raid_group_downstream_health_disabled_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_raid_group_t *                              raid_group_p = (fbe_raid_group_t*)object_p;
    fbe_base_config_downstream_health_state_t   downstream_health_state;    

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_ACTIVATE);
    fbe_raid_group_unlock(raid_group_p);

    fbe_raid_group_update_write_log_for_encryption(raid_group_p);

    downstream_health_state = fbe_raid_group_verify_downstream_health(raid_group_p);

    /* Peter Puhov.
        This condition should "hold" specialize / activate states until health will become at least degraded.
        The concept was reviewed and approved by Rob Foley.
        Please do NOT modify this code unless you understand exactly what you are doing!!!
    */
    if (downstream_health_state == FBE_DOWNSTREAM_HEALTH_OPTIMAL || downstream_health_state == FBE_DOWNSTREAM_HEALTH_DEGRADED)
    {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    }
    else if (downstream_health_state == FBE_DOWNSTREAM_HEALTH_BROKEN)
    {        
        /* set downstream health broken condition. */
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                               (fbe_base_object_t *)raid_group_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
        
    }
    else
    {
        //  The downstream health is disabled, clear the evaluate NR on demand condition. 
        fbe_lifecycle_force_clear_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                                       FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

fbe_status_t  fbe_check_rg_monitor_hooks(fbe_raid_group_t *raid_group_p,
                                         fbe_u32_t state,
                                         fbe_u32_t substate,
                                         fbe_scheduler_hook_status_t *status) {

    *status = FBE_SCHED_STATUS_OK;
    return fbe_raid_group_check_hook(raid_group_p, 
                                     state,substate, 0, status);
}

/*!****************************************************************************
 * raid_group_zero_verify_chkpt_cond_function()
 ******************************************************************************
 * @brief
 *  This function checks whether the checkpoint needs to be moved 
 *  back to zero or not. If its needed it moves it back otherwise it clears the
 *  condition. If the checkpoint is a valid one, we will move it back to zero.
 *  The reason is we only persist the checkpoint if we are moving it to invalid
 *  or moving from invalid to a valid value. It could so happen that checkpoint was
 *  valid, and we moved it back to some other value and the system got rebooted.
 *  We need to verify from the last set checkpoint. Since it might not have been
 *  persisted, we move it to zero so that we do not miss any chunks that was marked
 *  for verify
 *
 * @param object_p               - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   FBE_LIFECYCLE_STATUS_DONE
 *
 * @author
 *  11/02/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_lifecycle_status_t
raid_group_zero_verify_chkpt_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t                         status;
    fbe_bool_t                           b_is_active;
    fbe_bool_t                           is_metadata_initialized;
    fbe_lba_t                            ro_verify_checkpoint;
    fbe_lba_t                            rw_verify_checkpoint;
    fbe_lba_t                            error_verify_checkpoint;
    fbe_lba_t                            incomplete_write_verify_checkpoint;
    fbe_lba_t                            system_verify_checkpoint;
    fbe_lba_t                            new_checkpoint;
    fbe_u64_t                            metadata_offset = FBE_LBA_INVALID;
    fbe_raid_group_t                     *raid_group_p = (fbe_raid_group_t *)object_p;
    
    /* If it is passive just clear the condition */
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
    if (!b_is_active)
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_config_metadata_nonpaged_is_initialized((fbe_base_config_t *)raid_group_p, &is_metadata_initialized);
    if(is_metadata_initialized == FBE_FALSE)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    else
    {
        error_verify_checkpoint = fbe_raid_group_verify_get_checkpoint(raid_group_p, FBE_RAID_BITMAP_VERIFY_ERROR);
        rw_verify_checkpoint = fbe_raid_group_verify_get_checkpoint(raid_group_p, FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE);
        ro_verify_checkpoint = fbe_raid_group_verify_get_checkpoint(raid_group_p, FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY);
        incomplete_write_verify_checkpoint = fbe_raid_group_verify_get_checkpoint(raid_group_p, FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE);
        system_verify_checkpoint = fbe_raid_group_verify_get_checkpoint(raid_group_p, FBE_RAID_BITMAP_VERIFY_SYSTEM);

        /* If the checkpoints are invalid or zero, then clear the condition */
        if((error_verify_checkpoint == FBE_LBA_INVALID || error_verify_checkpoint == 0) &&
           (rw_verify_checkpoint == FBE_LBA_INVALID || rw_verify_checkpoint == 0) &&
           (ro_verify_checkpoint == FBE_LBA_INVALID || ro_verify_checkpoint == 0) &&
           (system_verify_checkpoint == FBE_LBA_INVALID || system_verify_checkpoint == 0) &&
           (incomplete_write_verify_checkpoint == FBE_LBA_INVALID || incomplete_write_verify_checkpoint == 0) )
        {
            fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* If the checkpoint is valid then move the checkpoint back to zero */
        if(error_verify_checkpoint != FBE_LBA_INVALID && error_verify_checkpoint != 0)
        {
            new_checkpoint = 0;
            metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->error_verify_checkpoint); 
            fbe_transport_set_completion_function(packet_p, raid_group_zero_verify_chkpt_cond_function_completion, raid_group_p);
            fbe_base_config_metadata_nonpaged_set_checkpoint((fbe_base_config_t *)raid_group_p,
                                                              packet_p,
                                                              metadata_offset,
                                                              0, /* There is only (1) checkpoint for a verify */
                                                              new_checkpoint);

            fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "rg_zero_verify_chkpt_cond: err_ver_chkpt:0x%x; new_chkpt:0x%x\n", (unsigned int)error_verify_checkpoint, (unsigned int)new_checkpoint);

            return FBE_LIFECYCLE_STATUS_PENDING;

        }

        if(incomplete_write_verify_checkpoint != FBE_LBA_INVALID && incomplete_write_verify_checkpoint != 0)
        {
            new_checkpoint = 0;
            fbe_raid_group_metadata_get_verify_chkpt_offset(raid_group_p, FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE, &metadata_offset);
            fbe_transport_set_completion_function(packet_p, raid_group_zero_verify_chkpt_cond_function_completion, raid_group_p);
            fbe_base_config_metadata_nonpaged_set_checkpoint((fbe_base_config_t *)raid_group_p,
                                                              packet_p,
                                                              metadata_offset,
                                                              0, /* There is only (1) checkpoint for a verify */
                                                              new_checkpoint);
            fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "rg_zero_verify_chkpt_cond: incmplt_wrt_ver_chkpt:0x%x; new_chkpt:0x%x\n", (unsigned int)incomplete_write_verify_checkpoint, (unsigned int)new_checkpoint);

            return FBE_LIFECYCLE_STATUS_PENDING;

        }

        if(system_verify_checkpoint != FBE_LBA_INVALID && system_verify_checkpoint != 0)
        {
            new_checkpoint = 0;
            fbe_raid_group_metadata_get_verify_chkpt_offset(raid_group_p, FBE_RAID_BITMAP_VERIFY_SYSTEM, &metadata_offset);
            fbe_transport_set_completion_function(packet_p, raid_group_zero_verify_chkpt_cond_function_completion, raid_group_p);
            fbe_base_config_metadata_nonpaged_set_checkpoint((fbe_base_config_t *)raid_group_p,
                                                              packet_p,
                                                              metadata_offset,
                                                              0, /* There is only (1) checkpoint for a verify */
                                                              new_checkpoint);
            fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "rg_zero_verify_chkpt_cond: sys_ver_chkpt:0x%x; new_chkpt:0x%x\n", (unsigned int)system_verify_checkpoint, (unsigned int)new_checkpoint);

            return FBE_LIFECYCLE_STATUS_PENDING;

        }        

        if(rw_verify_checkpoint != FBE_LBA_INVALID && rw_verify_checkpoint != 0)
        {
            new_checkpoint = 0;
            metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->rw_verify_checkpoint); 
            fbe_transport_set_completion_function(packet_p, raid_group_zero_verify_chkpt_cond_function_completion, raid_group_p);
            fbe_base_config_metadata_nonpaged_set_checkpoint((fbe_base_config_t *)raid_group_p,
                                                              packet_p,
                                                              metadata_offset,
                                                              0, /* There is only (1) checkpoint for a verify */
                                                              new_checkpoint);
            fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "rg_zero_verify_chkpt_cond: rw_ver_chkpt:0x%x; new_chkpt:0x%x\n", (unsigned int)rw_verify_checkpoint, (unsigned int)new_checkpoint);

            return FBE_LIFECYCLE_STATUS_PENDING;

        }

        if(ro_verify_checkpoint != FBE_LBA_INVALID && ro_verify_checkpoint != 0)
        {
            new_checkpoint = 0;
            metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->ro_verify_checkpoint); 
            fbe_transport_set_completion_function(packet_p, raid_group_zero_verify_chkpt_cond_function_completion, raid_group_p);
            fbe_base_config_metadata_nonpaged_set_checkpoint((fbe_base_config_t *)raid_group_p,
                                                              packet_p,
                                                              metadata_offset,
                                                              0, /* There is only (1) checkpoint for a verify */
                                                              new_checkpoint);
            fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "rg_zero_verify_chkpt_cond: ro_ver_chkpt:0x%x; new_chkpt:0x%x\n", (unsigned int)ro_verify_checkpoint, (unsigned int)new_checkpoint);

            return FBE_LIFECYCLE_STATUS_PENDING;

        }

    }

    return FBE_LIFECYCLE_STATUS_DONE;

}
/******************************************************************************
 * end raid_group_zero_verify_chkpt_cond_function()
 ******************************************************************************/
/*!**************************************************************
 * raid_group_zero_verify_chkpt_cond_function_completion()
 ****************************************************************
 * @brief
 *  It just reschedules the condition after 100 ms
 *
 * @param in_packet_p - The packet sent to us from the scheduler.
 * @param in_context  - The pointer to the object.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  11/02/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
static fbe_status_t raid_group_zero_verify_chkpt_cond_function_completion(
                                                        fbe_packet_t*    in_packet_p, 
                                                        fbe_packet_completion_context_t in_context)
{
    fbe_raid_group_t*           raid_group_p;
    fbe_status_t                status;


    // Cast the base object to a raid object
    raid_group_p = (fbe_raid_group_t*)in_context;

    status = fbe_transport_get_status_code(in_packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Setting the checkpoint to zero failed, status: 0x%x\n",
                              __FUNCTION__, status);
    }

    // Reschedule immediately to continue the verify operation.
    fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p, 100);

    // By returning OK, the monitor packet will be completed to the next level.
    return FBE_STATUS_OK;
}
// End raid_group_zero_verify_chkpt_cond_function_completion


static fbe_bool_t raid_group_is_metadata_need_rebuild(fbe_raid_group_t *raid_group_p)
{
    /**
     * FIX for AR605580.
     * We can't simply mark all paged metadata NR,
     *
     * If we get incomplete write on paged metadata and it was mared NR, verify on
     * paged metadata may fail and cause paged metadata to reconstruct. After reconstruct,
     * some user chunks will be marked NR incorrectly, and may cause DATA LOST if they had
     * incomplete write.
     *
     * Since checkpoint is unreliable, we use paged_metadata_metadata to determine NR.
     * When rebuild on paged metadata done, we persist nonpaged to disks.
     * We need check rebuild logging bitmask too, because we don't persist nonpaged when
     * marking NR for performance reason.
     *
     * 1. We marked NR on metadata, and clear_rl cond didn't run before panic
     *     All chunks were in degraded mode. So write logging would take care of incomplete
     *     write. We are safe to rebuild metadata.
     *
     * 2. We marked NR on metadata, and clear_rl cond run before panic.
     *    clear_rl would persist all NR bits.
     *    If we panic while rebuilding metadata, we will rebuild all metadata here.
     *       We have a much smaller window (time of rebuilding 14 chunks).
     *
     *    If we finish rebuilding metadata, everything is ok.
     *
     * 3. We don't mark NR on metadata.
     *    Everything is ok.
     */

    fbe_raid_group_nonpaged_metadata_t*  nonpaged_metadata_p = NULL;
    fbe_u32_t i;
    fbe_bool_t is_rebuilding = FBE_FALSE;
    fbe_bool_t is_metadata_rebuilding = FBE_FALSE;
    fbe_bool_t need_rebuild = FBE_FALSE;
    fbe_raid_position_bitmask_t rl_bitmask;


    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

    for (i = 0; i < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; i++) {
        if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[i].position != FBE_RAID_INVALID_DISK_POSITION ) {
            is_rebuilding = FBE_TRUE;
            break;
        }
    }

    if (is_rebuilding) {
        /* We are rebuilding now.
         * Use page_metadata_metadata to check if we need rebuild paged metadata
         */
        fbe_u32_t width;
        fbe_raid_group_get_width(raid_group_p, &width);
        for (i = 0; i < width; i++) {
            fbe_raid_position_bitmask_t cur_position_mask;

            cur_position_mask = 1 << i;
            if (fbe_raid_group_is_any_metadata_marked_NR(raid_group_p, cur_position_mask)) {
                is_metadata_rebuilding = FBE_TRUE;
                break;
            }
        }
    }

    rl_bitmask = nonpaged_metadata_p->rebuild_info.rebuild_logging_bitmask;
    if((rl_bitmask != 0) || is_metadata_rebuilding) {
        need_rebuild = FBE_TRUE;
    } else {
        need_rebuild = FBE_FALSE;
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: need_rebuild %u, md rebuilding: %u, rl_b: 0x%x\n",
                          __FUNCTION__, need_rebuild, is_metadata_rebuilding, rl_bitmask);
    return need_rebuild;
}

/*!****************************************************************************
 * raid_group_mark_nr_for_md_cond_function()
 ******************************************************************************
 * @brief
 *  This function checks whether the rg is degraded
 *  If it is it marks NR for metadata in md of md. This is needed
 *  since NR bits in the MD of MD is not persisted and if both SPs
 * go down, next time when we come up we need to mark it again.
 *
 * @param object_p               - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   FBE_LIFECYCLE_STATUS_DONE
 *
 * @author
 *  03/30/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_lifecycle_status_t
raid_group_mark_nr_for_md_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t                         status;
    fbe_bool_t                           b_is_active;
    fbe_bool_t                           is_metadata_initialized;
    fbe_raid_position_bitmask_t          nr_bitmask = 0;
    fbe_raid_group_nonpaged_metadata_t*  nonpaged_metadata_p = NULL;
    fbe_raid_group_t                     *raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_u32_t                            i;
    
    /* If it is passive just clear the condition */
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
    if (!b_is_active)
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_config_metadata_nonpaged_is_initialized((fbe_base_config_t *)raid_group_p, &is_metadata_initialized);
    if(is_metadata_initialized == FBE_FALSE)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    else
    {
        fbe_bool_t is_metadata_need_rebuild;


        is_metadata_need_rebuild = raid_group_is_metadata_need_rebuild(raid_group_p);
        fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

        if (is_metadata_need_rebuild)
        {
            for(i=0;i<FBE_RAID_GROUP_MAX_REBUILD_POSITIONS ;i++)
            {
                if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[i].position != FBE_RAID_INVALID_DISK_POSITION )
                {
                    nr_bitmask = nr_bitmask |(1 << nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[i].position);
                }
            }

            fbe_transport_set_completion_function(packet_p, raid_group_mark_nr_for_md_cond_function_completion, (fbe_base_object_t*)raid_group_p);
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s set np nr bits: %x, rl: %x\n",
                                  __FUNCTION__, nr_bitmask,
                                  nonpaged_metadata_p->rebuild_info.rebuild_logging_bitmask);

            fbe_raid_group_np_set_bits(packet_p,raid_group_p, nr_bitmask);

            return FBE_LIFECYCLE_STATUS_PENDING;
        }

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);        

    }

    return FBE_LIFECYCLE_STATUS_DONE;

}
/******************************************************************************
 * end raid_group_mark_nr_for_md_cond_function()
 ******************************************************************************/
/*!**************************************************************
 * raid_group_mark_nr_for_md_cond_function_completion()
 ****************************************************************
 * @brief
 *  It just reschedules the condition after 100 ms
 *
 * @param in_packet_p - The packet sent to us from the scheduler.
 * @param in_context  - The pointer to the object.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  03/30/2012 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
static fbe_status_t raid_group_mark_nr_for_md_cond_function_completion(
                                                        fbe_packet_t*    in_packet_p, 
                                                        fbe_packet_completion_context_t in_context)
{
    fbe_raid_group_t*           raid_group_p;
    fbe_status_t                status;


    // Cast the base object to a raid object
    raid_group_p = (fbe_raid_group_t*)in_context;

    status = fbe_transport_get_status_code(in_packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Marking NR for MD failed. status: 0x%x\n",
                              __FUNCTION__, status);

        return status;
    }

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);

    // Reschedule immediately to continue the verify operation.
    fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p, 100);

    // By returning OK, the monitor packet will be completed to the next level.
    return FBE_STATUS_OK;
}
// End raid_group_mark_nr_for_md_cond_function

/*!****************************************************************************
 * raid_group_eval_download_request_cond_function()
 ******************************************************************************
 * @brief
 *  This function checks whether any of the object could start download. 
 *  If yes, it will send a permit to the downstream object.
 *
 * @param object_p               - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   FBE_LIFECYCLE_STATUS_DONE
 *
 * @author
 *  12/01/2011 - Created. chenl6
 *
 ******************************************************************************/
static fbe_lifecycle_status_t
raid_group_eval_download_request_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_raid_group_t                    *raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_base_config_downstream_health_state_t   downstream_health;
    fbe_u32_t                           width;
    fbe_raid_position_t                 position, selected_position = 0;
    fbe_path_attr_t                     path_attr;
    fbe_object_id_t                     pvd_object_id = FBE_OBJECT_ID_INVALID, selected_server_id = FBE_OBJECT_ID_INVALID;
    fbe_u16_t                           edge_bitmask;
    fbe_block_edge_t *                  block_edge_p = NULL;
    fbe_status_t                        status;

    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;
    fbe_raid_group_check_hook(raid_group_p,
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_DOWNLOAD,
                              FBE_RAID_GROUP_SUBSTATE_DOWNLOAD_START,
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Scan the block edges and select an edge */
    fbe_raid_group_get_width(raid_group_p, &width);
    for (position = 0; position < width; position++)
    {
        fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &block_edge_p, position);
        fbe_block_transport_get_path_attributes(block_edge_p, &path_attr);
        //fbe_block_transport_get_server_id(block_edge_p, &server_id);
        edge_bitmask = 1 << position;

        if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ)
        {
            raid_group_p->download_request_bitmask |= edge_bitmask;
            /* Get the pvd object id of the active edge of the virtual drive.
             */
            status = fbe_raid_group_get_id_of_pvd_being_worked_on(raid_group_p, packet_p, block_edge_p, &pvd_object_id);
            if (status != FBE_STATUS_OK)
            {
                /* If this fails it does not mean its an error.  
                 * That position could be removed.
                 */
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s get pvd id failed - status: 0x%x\n",
                                      __FUNCTION__, status); 
                continue; 
            }
            /* Select the edge with smallest pvd_object_id */
            if ((selected_server_id == FBE_OBJECT_ID_INVALID) || (selected_server_id > pvd_object_id))
            {
                selected_server_id = pvd_object_id;
                selected_position = position;
            }
        }
        else
        {
            raid_group_p->download_request_bitmask &= ~edge_bitmask;
        }

        if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_GRANT)
        {
            if (!(raid_group_p->download_granted_bitmask & edge_bitmask))
            {
                /* This should not happen */
                fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s granted_bitmask 0x%x edge 0x%x\n", 
                                      __FUNCTION__, raid_group_p->download_granted_bitmask, edge_bitmask);
            }
        }
        else
        {
            if (raid_group_p->download_granted_bitmask & edge_bitmask)
            {
                /* The grant bit is cleared by the VD/PVD. */
                raid_group_p->download_granted_bitmask &= ~edge_bitmask;
                fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s GRANT bit cleared\n", __FUNCTION__);
                /* We wait for 0.2 seconds for other edge change information. */
                fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, 
                                         (fbe_base_object_t*)raid_group_p, (fbe_lifecycle_timer_msec_t)200); 
                return FBE_LIFECYCLE_STATUS_DONE;
            }
        }
    }
    fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s grant 0x%x, request 0x%x\n", 
                          __FUNCTION__, raid_group_p->download_granted_bitmask, raid_group_p->download_request_bitmask);

    /* There are drives in download,
     * or no drive in download and no drive in request. 
     */
    if (raid_group_p->download_granted_bitmask || raid_group_p->download_request_bitmask == 0)
    {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Determine the overall health of the paths to the downstream objects */
    downstream_health = fbe_raid_group_verify_downstream_health(raid_group_p);
    if (downstream_health != FBE_DOWNSTREAM_HEALTH_OPTIMAL ||
        fbe_raid_group_is_degraded(raid_group_p))
    {
        fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s RG not healthy 0x%x\n", __FUNCTION__, downstream_health);
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if (fbe_raid_group_time_to_start_download(raid_group_p))
    {
        fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s selected position: 0x%x server: 0x%x\n",
                              __FUNCTION__, selected_position, selected_server_id);
        /* Send the ack to selected position */
        raid_group_p->download_granted_bitmask |= (1 << selected_position);
        fbe_raid_group_set_last_download_time(raid_group_p, fbe_get_time());
        raid_group_download_send_ack(raid_group_p, packet_p, selected_position, FBE_TRUE);
    }
    fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end raid_group_eval_download_request_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * raid_group_keys_requested_cond_function()
 ******************************************************************************
 * @brief
 *  This function checks whether if keys needs to be pushed to any of the 
 *  downstream objects 
 *
 * @param object_p               - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   FBE_LIFECYCLE_STATUS_DONE
 *
 * @author
 *  11/27/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_lifecycle_status_t
raid_group_keys_requested_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_raid_group_t                    *raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_u32_t                           width;
    fbe_raid_position_t                 position;
    fbe_path_attr_t                     path_attr;
    fbe_block_edge_t *                  block_edge_p = NULL;
    fbe_status_t                        status;
    fbe_provision_drive_control_register_keys_t *key_info_handle;
    fbe_encryption_setup_encryption_keys_t *key_handle;
    fbe_payload_ex_t *                                         sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_u32_t                           *position_buffer;

    fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Scan the block edges and select an edge */
    fbe_raid_group_get_width(raid_group_p, &width);
    for (position = 0; position < width; position++)
    {
        fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &block_edge_p, position);
        fbe_block_transport_get_path_attributes(block_edge_p, &path_attr);
       
        if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED)
        {

            fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Push Key down for position:%d\n",
                              __FUNCTION__, position);

            /* Push the keys down */
            sep_payload_p = fbe_transport_get_payload_ex(packet_p);

            status = fbe_base_config_get_key_handle((fbe_base_config_t *)raid_group_p, &key_handle);

            key_info_handle = (fbe_provision_drive_control_register_keys_t *) fbe_transport_allocate_buffer();
            position_buffer = (fbe_u32_t *)(key_info_handle + 1);

            *position_buffer = position;

            if (key_handle == NULL)
            {
                /* initialize the control buffer. */
                key_info_handle->key_handle = NULL;
                key_info_handle->key_handle_paco = NULL;
            }
            else
            {
                /* initialize the control buffer. */
                key_info_handle->key_handle = fbe_encryption_get_key_info_handle(key_handle, position);
                key_info_handle->key_handle_paco = fbe_encryption_get_key_info_handle(key_handle, FBE_RAID_MAX_DISK_ARRAY_WIDTH);
            }

            /* allocate and initialize the control operation. */
            control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
            fbe_payload_control_build_operation(control_operation_p,
                                                FBE_PROVISION_DRIVE_CONTROL_CODE_REGISTER_KEYS,
                                                key_info_handle,
                                                sizeof(fbe_provision_drive_control_register_keys_t));

            /* set completion functon. */
            fbe_transport_set_completion_function(packet_p,
                                                  fbe_raid_group_keys_requested_completion,
                                                  raid_group_p);

            fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
            fbe_block_transport_send_control_packet(block_edge_p, packet_p);

            return FBE_LIFECYCLE_STATUS_PENDING;
        }
    }

    /* All edges has been processed */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end raid_group_keys_requested_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 *          raid_group_emeh_request_cond_function()
 ******************************************************************************
 *
 * @brief   This condition is set when one of the following occurs:
 *                  Change thresholds when:
 *              o Raid group has become degraded
 *              o Proactive Copy has started
 *              o User Request to change thresholds
 *                  Restore Thresholds when:
 *              o Raid group is no longer degraded
 *              o Proactive copy has completed
 *              o User request to restore thresholds
 *
 * @param object_p               - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   FBE_LIFECYCLE_STATUS_DONE
 *
 * @author
 *  05/07/2015  Ron Proulx  - Created
 *
 ******************************************************************************/
static fbe_lifecycle_status_t raid_group_emeh_request_cond_function(fbe_base_object_t *object_p, fbe_packet_t *packet_p)
{
    fbe_status_t            status; 
    fbe_raid_group_t       *raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_raid_emeh_command_t emeh_command = FBE_RAID_EMEH_COMMAND_INVALID;

    fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Use raid group EMEH info to determine if enabled.
     */
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_get_emeh_request(raid_group_p, &emeh_command);
    if (fbe_raid_group_is_extended_media_error_handling_enabled(raid_group_p))
    {
        /* Determine which command has been requested.
         */
        fbe_raid_group_unlock(raid_group_p);
        if (emeh_command == FBE_RAID_EMEH_COMMAND_RAID_GROUP_DEGRADED)
        {
            /* Invoke method to process raid group degraded.
             */
            return fbe_raid_group_emeh_degraded_cond_function(raid_group_p, packet_p);
        }
        else if (emeh_command == FBE_RAID_EMEH_COMMAND_PACO_STARTED)
        {
            /* Invoke method to process proactive copy started.
             */
            return fbe_raid_group_emeh_paco_cond_function(raid_group_p, packet_p);
        }
        else if (emeh_command == FBE_RAID_EMEH_COMMAND_INCREASE_THRESHOLDS)
        {
            /* Invoke method to increase media error thresholds.
             */
            return fbe_raid_group_emeh_increase_cond_function(raid_group_p, packet_p);
        }
        else if (emeh_command == FBE_RAID_EMEH_COMMAND_RESTORE_NORMAL_MODE)
        {
            /* Invoke method to process restore defaults.
             */
            return fbe_raid_group_emeh_restore_default_cond_function(raid_group_p, packet_p);
        }
        else
        {
            /* Unsupported command.  Clear the condition.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s unexpected emeh command: %d \n",
                                  __FUNCTION__, emeh_command);
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s can't clear current condition, status: 0x%x\n",
                                      __FUNCTION__, status);
            }
            return FBE_LIFECYCLE_STATUS_DONE;
        }

    } /* end if EMEH is supported by this riad group. */

    /* Else EMEH is not enabled or this is a virtual drive etc.
     * Since this condition should only be set if enabled, we don't
     * expect this so trace an error.
     */
    fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_INVALID);
    fbe_raid_group_unlock(raid_group_p);
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s emeh not enabled for request: %d clear condition \n",
                          __FUNCTION__, emeh_command);

    fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end raid_group_emeh_request_cond_function()
 ******************************************************************************/

static fbe_status_t 
fbe_raid_group_keys_requested_completion(fbe_packet_t * packet_p,
                                         fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                                          raid_group_p = NULL;
    fbe_payload_ex_t *                                         sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_payload_control_status_t                                control_status;
    fbe_payload_control_status_qualifier_t                      control_qualifier;
    fbe_status_t                                                status;
    fbe_path_attr_t                     path_attr;
    fbe_block_edge_t *                  block_edge_p = NULL;
    fbe_u32_t                           *position_buffer;
    fbe_provision_drive_control_register_keys_t *key_info_handle;
    fbe_u8_t *                                              buffer_p = NULL;

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

     /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);

    /* Get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);

    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    key_info_handle = (fbe_provision_drive_control_register_keys_t *) buffer_p;
    position_buffer = (fbe_u32_t *) (key_info_handle + 1);

    fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &block_edge_p, *position_buffer);
    fbe_block_transport_get_path_attributes(block_edge_p, &path_attr);

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Pushed Key down for position:%d\n",
                              __FUNCTION__, *position_buffer);

    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED)
    {
        fbe_block_transport_clear_path_attributes(block_edge_p, FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED);
    }
    else
    {
        /* This flag may be cleared by the PVD.   */
    }


    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
    fbe_transport_release_buffer(buffer_p);
    fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, 
                              (fbe_base_object_t*)raid_group_p, (fbe_lifecycle_timer_msec_t)0); 
    return FBE_STATUS_OK; 
}

/*!****************************************************************************
 * raid_group_download_send_ack()
 ******************************************************************************
 * @brief
 *  This function sends download permit to PVD. 
 *
 * @param raid_group_p               - pointer to a raid group.
 * @param packet_p                   - pointer to a control packet.
 * @param selected_position          - selected position.
 * @param is_ack                     - ack or nack.
 * 
 * @return fbe_status_t 
 *
 * @author
 *  12/01/2011 - Created. chenl6
 *
 ******************************************************************************/
fbe_status_t
raid_group_download_send_ack(fbe_raid_group_t * raid_group_p, 
                             fbe_packet_t * packet_p, 
                             fbe_raid_position_t selected_position,
                             fbe_bool_t is_ack)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_status_t        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_block_edge_t *                  block_edge_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    if (is_ack)
    {
        fbe_payload_control_build_operation(control_operation_p,
                                            FBE_PROVISION_DRIVE_CONTROL_CODE_DOWNLOAD_ACK,
                                            NULL,
                                            0);
    } else {
        fbe_payload_control_build_operation(control_operation_p,
                                            FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_REJECT,
                                            NULL,
                                            0);
    }
    fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &block_edge_p, selected_position);

    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Send the packet and wait for its completion. */
    fbe_block_transport_send_control_packet(block_edge_p, packet_p);
    fbe_transport_wait_completion(packet_p);

    /* Get the control operation status and packet status */
    fbe_payload_control_get_status(control_operation_p, &control_status);
    status = fbe_transport_get_status_code(packet_p);

    /* Release the control operation */
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

    if (status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed status 0x%X control_status 0x%x\n", 
                                __FUNCTION__, status, control_status);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    
    return status;
}
/******************************************************************************
 * end raid_group_download_send_ack()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_background_op_check_slf()
 ******************************************************************************
 * @brief
 *   This function checks SLF of the PVDs and decides whether to set the block
 *   attributes on this or peer SP.
 * 
 * @param raid_group_p - pointer to the raid group
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   09/05/2012 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_background_op_check_slf(fbe_raid_group_t * raid_group_p)
{
    fbe_bool_t b_is_slf_set, b_peer_is_slf_set, b_is_peer_req_set;

    b_is_slf_set = fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_REQ);
    b_peer_is_slf_set = fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_REQ);
    b_is_peer_req_set = fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_PEER_REQ);

    if (!b_is_slf_set && 
        fbe_base_config_is_global_path_attr_set((fbe_base_config_t *)raid_group_p, FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED))
    { 
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s clear attr: %d peer %d peer_req %d\n", 
                              __FUNCTION__, b_is_slf_set, b_peer_is_slf_set, b_is_peer_req_set);
        /* Clear global_path_attr */
        fbe_base_config_clear_global_path_attr((fbe_base_config_t *)raid_group_p, FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED);

        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers( 
                    &((fbe_base_config_t *)raid_group_p)->block_transport_server, 
                    0, 
                    FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED);
    }

    if (!b_peer_is_slf_set && b_is_peer_req_set)
    { 
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s clear peer_req: %d peer %d peer_req %d\n", 
                              __FUNCTION__, b_is_slf_set, b_peer_is_slf_set, b_is_peer_req_set);
        fbe_raid_group_clear_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_PEER_REQ);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (b_is_slf_set && !b_peer_is_slf_set && !b_is_peer_req_set)
    {
        if (!fbe_base_config_is_global_path_attr_set((fbe_base_config_t *)raid_group_p, FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED))
        { 
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s set attr: %d peer %d peer_req %d\n", 
                                  __FUNCTION__, b_is_slf_set, b_peer_is_slf_set, b_is_peer_req_set);
            /* Set global_path_attr */
            fbe_base_config_set_global_path_attr((fbe_base_config_t *)raid_group_p, FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED);

            fbe_block_transport_server_propagate_path_attr_with_mask_all_servers( 
                    &((fbe_base_config_t *)raid_group_p)->block_transport_server, 
                    FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED, 
                    FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED);
        }

        if (fbe_raid_group_is_peer_present(raid_group_p))
        { 
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s send passive request\n", 
                                  __FUNCTION__);
            fbe_base_config_passive_request((fbe_base_config_t *) raid_group_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        return FBE_STATUS_OK;
    }

    if (!b_is_slf_set && b_peer_is_slf_set && !b_is_peer_req_set)
    { 
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s set peer_req: %d peer %d peer_req %d\n", 
                              __FUNCTION__, b_is_slf_set, b_peer_is_slf_set, b_is_peer_req_set);
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_PEER_REQ);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    return FBE_STATUS_OK;
}

/******************************************************************************
 * end fbe_raid_group_background_op_check_slf()
 ******************************************************************************/

/*!**************************************************************
 * fbe_raid_group_time_to_start_download()
 ****************************************************************
 * @brief
 *  Determine if we can start download now or if we need to
 *  wait for some time to pass.
 *
 * @param  raid_group_p - current raid group.               
 *
 * @return FBE_TRUE - time to start download
 *        FBE_FALSE - Wait to start download.
 *
 * @author
 *  6/5/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_bool_t fbe_raid_group_time_to_start_download(fbe_raid_group_t *raid_group_p)
{
    fbe_time_t                          last_download_time;
    fbe_u32_t                           seconds_since_download;
    
    fbe_raid_group_get_last_download_time(raid_group_p, &last_download_time);
    seconds_since_download = fbe_get_elapsed_seconds(last_download_time);

    /* If enough time has passed, then start the download.
     */
    if (seconds_since_download >= FBE_RAID_GROUP_SECONDS_BETWEEN_FW_DOWNLOAD){
        return FBE_TRUE;
    }
    else {
        return FBE_FALSE;
    }
}
/******************************************
 * end fbe_raid_group_time_to_start_download()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_is_encryption_key_needed()
 ****************************************************************
 * @brief
 *  Determine if RG needs to be wait for encryption key
 *
 * @param  raid_group_p - current raid group.               
 *
 * @return FBE_TRUE - Wait for encryption key to be provided
 *        FBE_FALSE - No need to wait
 *
 * @author
 *  10/17/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/

fbe_bool_t fbe_raid_group_is_encryption_key_needed(fbe_raid_group_t *raid_group_p)
{
    fbe_base_config_encryption_mode_t   encryption_mode;
    fbe_encryption_setup_encryption_keys_t *key_handle;
    fbe_raid_geometry_t        *raid_geometry_p = NULL;
    fbe_raid_group_type_t       raid_type;

    fbe_base_config_get_encryption_mode((fbe_base_config_t *)raid_group_p, &encryption_mode);
    fbe_base_config_get_key_handle((fbe_base_config_t *)raid_group_p, &key_handle);

    /* Get geometry and raid type
     */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);

    /* if we are encrypted mode and key handle is not there already then
     * need to wait for the keys to be provided..
     * Raid 0 Encryption is not supported, for RAID 10 it is handled by the mirror and
     * we dont care about VD also. So ignore those objects 
     */
    if (((encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) ||
         (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
         (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS)) &&
        (key_handle == NULL) &&
        (((fbe_base_config_t *)raid_group_p)->base_object.class_id != FBE_CLASS_ID_VIRTUAL_DRIVE) &&
        (raid_type != FBE_RAID_GROUP_TYPE_RAID10)/*&&
        (raid_type != FBE_RAID_GROUP_TYPE_RAID0)*/)
    {
        return FBE_TRUE;
    }
    else 
    {
        return FBE_FALSE;
    }
}
/************************************************
 * end fbe_raid_group_is_encryption_key_needed()
 ************************************************/
/*!**************************************************************
 * fbe_raid_group_encryption_notify_key_need()
 ****************************************************************
 * @brief
 *  The function updates the states so that whoever provides the key
 * will know what to do
 *
 * @param  raid_group_p - current raid group.               
 * @param  metadata_initialized - TRUE if MD is initialized(meaning 
 *                                this is an existing RG)
 * 
 * @return fbe status
 *
 * @author
 *  10/17/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/

fbe_status_t 
fbe_raid_group_encryption_notify_key_need(fbe_raid_group_t *raid_group_p, fbe_bool_t metadata_initialized)
{
    fbe_status_t status;
    fbe_object_id_t object_id;
    fbe_notification_info_t notification_info;	
    fbe_bool_t is_notification_requiered = FBE_FALSE;
    fbe_base_config_encryption_state_t encryption_state;
    fbe_class_id_t class_id;
    fbe_handle_t object_handle;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    fbe_raid_group_lock(raid_group_p);
    fbe_base_config_get_encryption_state((fbe_base_config_t *)raid_group_p, &encryption_state);

    if(encryption_state >= FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_PUSH_KEYS_DOWNSTREAM){
        /* We already have keys */
        fbe_raid_group_unlock(raid_group_p);
        return FBE_STATUS_OK;
    }

    if (metadata_initialized) {
        if(encryption_state != FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_CURRENT_KEYS_REQUIRED){
            is_notification_requiered = FBE_TRUE;
        }
        /* This is an existing RG and so we need "old" keys*/
        fbe_base_config_set_encryption_state((fbe_base_config_t *)raid_group_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_CURRENT_KEYS_REQUIRED);
    } else {
        if(encryption_state != FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_NEW_KEYS_REQUIRED){
            is_notification_requiered = FBE_TRUE;
        }

        /* This is a new RG and so new keys are required */
        fbe_base_config_set_encryption_state((fbe_base_config_t *)raid_group_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_NEW_KEYS_REQUIRED);
    }
    fbe_raid_group_unlock(raid_group_p);

    if(is_notification_requiered == FBE_FALSE){ /* Nothing else to do */
        return FBE_STATUS_OK;
    }

    /* Need to send notification */
    fbe_base_config_get_encryption_state((fbe_base_config_t *)raid_group_p, &encryption_state);


    fbe_base_object_get_object_id((fbe_base_object_t *)raid_group_p, &object_id);
    fbe_zero_memory(&notification_info, sizeof(fbe_notification_info_t));

    notification_info.notification_type = FBE_NOTIFICATION_TYPE_ENCRYPTION_STATE_CHANGED;    
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP;

    object_handle = fbe_base_pointer_to_handle((fbe_base_t *) raid_group_p);
    class_id = fbe_base_object_get_class_id(object_handle);
    notification_info.class_id = class_id;

    notification_info.notification_data.encryption_info.encryption_state = encryption_state;
    notification_info.notification_data.encryption_info.object_id = object_id;
    fbe_base_config_get_generation_num((fbe_base_config_t *)raid_group_p, &notification_info.notification_data.encryption_info.generation_number);
    fbe_database_lookup_raid_by_object_id(object_id, &notification_info.notification_data.encryption_info.control_number);
    if (fbe_raid_geometry_is_child_raid_group(raid_geometry_p))
    {
        fbe_base_transport_server_t * base_transport_server_p = NULL;
        fbe_base_edge_t * base_edge_p = NULL;
        base_transport_server_p = (fbe_base_transport_server_t *)&(((fbe_base_config_t *) raid_group_p)->block_transport_server);
        base_edge_p = base_transport_server_p->client_list;
        fbe_database_lookup_raid_by_object_id(base_edge_p->client_id, &notification_info.notification_data.encryption_info.control_number);
    }

    fbe_base_config_get_width((fbe_base_config_t*)raid_group_p, &notification_info.notification_data.encryption_info.width);

    status = fbe_notification_send(object_id, notification_info);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                          FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s Notification send Failed \n", __FUNCTION__);

        return status;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_raid_group_push_keys_downstream()
 ****************************************************************
 * @brief
 *  This function initialize the object Key handle and then registers
 *  the keys with the underlying PVD.
 *
 * @param raid_group_p - Current rg.
 * @param packet_p - Packet for this usurper.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t
fbe_raid_group_push_keys_downstream(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_u32_t width;
    fbe_base_config_memory_allocation_chunks_t mem_alloc_chunks;
    fbe_encryption_setup_encryption_keys_t *key_handle =  NULL;

    fbe_base_config_get_key_handle((fbe_base_config_t *)raid_group_p, &key_handle);
#if 0
    if (key_handle == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s key handle is NULL\n", __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
#endif
        /* get the width of the raid group object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* We need to send the request down to all the edges and so get the width and allocate
     * packet for every edge */
    /* Set the various counts and buffer size */
    mem_alloc_chunks.buffer_count = width;
    mem_alloc_chunks.number_of_packets = width;
    mem_alloc_chunks.sg_element_count = 0;
    mem_alloc_chunks.buffer_size = sizeof(fbe_provision_drive_control_register_keys_t);
    mem_alloc_chunks.alloc_pre_read_desc = FALSE;
    mem_alloc_chunks.pre_read_desc_count = 0;
    
    /* allocate the subpackets to collect the zero checkpoint for all the edges. */
    status = fbe_base_config_memory_allocate_chunks((fbe_base_config_t *) raid_group_p,
                                                    packet_p,
                                                    mem_alloc_chunks,
                                                    (fbe_memory_completion_function_t)fbe_raid_group_push_keys_downstream_allocation_completion); /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory allocation failed, status:0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
    return status;
}
/******************************************************************************
 * end fbe_raid_group_push_keys_downstream()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_push_keys_downstream_allocation_completion()
 ******************************************************************************
 * @brief
 *  This function gets called after memory has been allocated and sends the 
 *  register request down to the PVD
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  10/03/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_push_keys_downstream_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                             fbe_memory_completion_context_t context)
{
    fbe_packet_t *                                  packet_p = NULL;
    fbe_packet_t **                                 new_packet_p = NULL;
    fbe_u8_t **                                     buffer_p = NULL;
    fbe_payload_ex_t *                              new_sep_payload_p = NULL;
    fbe_payload_control_operation_t *               new_control_operation_p = NULL;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_raid_group_t *                              raid_group_p = NULL;
    fbe_u32_t                                       width = 0;
    fbe_u32_t                                       index = 0;
    fbe_block_edge_t *                              block_edge_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t   mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                               *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t              *pre_read_desc_p = NULL;
    fbe_packet_attr_t                               packet_attr;
    fbe_provision_drive_control_register_keys_t     *key_info_handle_p;
    fbe_encryption_setup_encryption_keys_t *key_handle = NULL;
    
    /* get the pointer to original packet. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* get the pointer to raid group object. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: memory request: 0x%p failed. request state: %d \n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_FAILED;
    }

    /* get the width of the base config object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* subpacket control buffer size. */
    mem_alloc_chunks_addr.buffer_size = sizeof(fbe_provision_drive_control_register_keys_t);
    mem_alloc_chunks_addr.number_of_packets = width;
    mem_alloc_chunks_addr.buffer_count = width;
    mem_alloc_chunks_addr.sg_element_count = 0;
    mem_alloc_chunks_addr.sg_list_p = NULL;
    mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
    mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
    mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;
    
    /* get subpacket pointer from the allocated memory. */
    status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                         memory_request_p,
                                                                         &mem_alloc_chunks_addr);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory distribution failed, status:0x%x, width:0x%x, buffer_size:0x%x\n",
                              __FUNCTION__, status, width, mem_alloc_chunks_addr.buffer_size);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    new_packet_p = mem_alloc_chunks_addr.packet_array_p;
    buffer_p = mem_alloc_chunks_addr.buffer_array_p;
    status = fbe_base_config_get_key_handle((fbe_base_config_t *)raid_group_p, &key_handle);

    while(index < width)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Push Key down for index:%d packet: %p\n", 
                              __FUNCTION__, index, packet_p);

        /* Initialize the sub packets. */
        fbe_transport_initialize_sep_packet(new_packet_p[index]);
        new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p[index]);

        /* initialize the control buffer. */
        key_info_handle_p = (fbe_provision_drive_control_register_keys_t *) buffer_p[index];
        if (key_handle == NULL) {
            key_info_handle_p->key_handle = NULL;
            key_info_handle_p->key_handle_paco = NULL;
        } else {
        key_info_handle_p->key_handle = fbe_encryption_get_key_info_handle(key_handle, index);
        key_info_handle_p->key_handle_paco = fbe_encryption_get_key_info_handle(key_handle, FBE_RAID_MAX_DISK_ARRAY_WIDTH);
        }

        /* allocate and initialize the control operation. */
        new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_sep_payload_p);
        fbe_payload_control_build_operation(new_control_operation_p,
                                            FBE_PROVISION_DRIVE_CONTROL_CODE_REGISTER_KEYS,
                                            key_info_handle_p,
                                            sizeof(fbe_provision_drive_control_register_keys_t));

        /* Initialize the new packet with FBE_PACKET_FLAG_EXTERNAL same as the master packet */
        fbe_transport_get_packet_attr(packet_p, &packet_attr);
        if (packet_attr & FBE_PACKET_FLAG_EXTERNAL) 
        {
            fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_EXTERNAL);            
        }

        /* set completion functon. */
        fbe_transport_set_completion_function(new_packet_p[index],
                                              fbe_raid_group_push_keys_downstream_subpacket_completion,
                                              raid_group_p);

        /* add the subpacket to master packet queue. */
        fbe_transport_add_subpacket(packet_p, new_packet_p[index]);
        index++;
    }

    /* send all the subpackets together. */
    for(index = 0; index < width; index++)
    {
        fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &block_edge_p, index);
        fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_TRAVERSE);
        fbe_block_transport_send_control_packet(block_edge_p, new_packet_p[index]);
    }

    /* send all the subpackets. */
     return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_push_keys_downstream_allocation_completion()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_group_push_keys_downstream_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the subpacket completion for the registration
 *  function for all the downstream objects.
 *
 * @param packet_p          - packet.
 * @param context           - context passed to request.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  10/03/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_push_keys_downstream_subpacket_completion(fbe_packet_t * packet_p,
                                                         fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                                          raid_group_p = NULL;
    fbe_packet_t *                                              master_packet_p = NULL;
    fbe_payload_ex_t *                                         master_sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           master_control_operation_p = NULL;
    fbe_payload_ex_t *                                         sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_payload_control_status_t                                control_status;
    fbe_payload_control_status_qualifier_t                      control_qualifier;
    fbe_bool_t                                                  is_empty;
    fbe_status_t                                                status;
    fbe_u32_t                           width;
    fbe_raid_position_t                 position;
    fbe_path_attr_t                     path_attr;
    fbe_block_edge_t *                  block_edge_p = NULL;

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the master packet, its payload and associated control operation. */
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    master_sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(master_sep_payload_p);

     /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);

    /* Get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);

    /* if any of the subpacket failed with bad status then update the master status. */
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(master_packet_p, status, 0);
        /* only control operation status is processed later */
        fbe_payload_control_set_status(master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    /* if any of the subpacket failed with failure status then update the master status. */
    /*!@todo we can use error predence here if required. */
    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_payload_control_set_status(master_control_operation_p, control_status);
        fbe_payload_control_set_status_qualifier(master_control_operation_p, control_qualifier);
    }
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Encryption: key pushed packet: %p status: %d control_status: %d\n",
                          packet_p, status, control_status);
    /* remove the sub packet from master queue. */ 
    //fbe_transport_remove_subpacket(packet_p);
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* destroy the subpacket. */
    //fbe_transport_destroy_packet(packet_p);

    /* when the queue is empty, all subpackets have completed. */

    if(is_empty) {
        fbe_transport_destroy_subpackets(master_packet_p);
        //fbe_memory_request_get_entry_mark_free(&master_packet_p->memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(&master_packet_p->memory_request);

        fbe_payload_control_get_status(master_control_operation_p, &control_status);

        if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
        {
            fbe_lba_t rekey_checkpoint;
            fbe_base_config_encryption_state_t encryption_state;
            fbe_base_config_encryption_mode_t encryption_mode;
            fbe_base_config_get_encryption_state((fbe_base_config_t *)raid_group_p, &encryption_state);
            fbe_base_config_get_encryption_mode((fbe_base_config_t *)raid_group_p, &encryption_mode);
            rekey_checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);

            fbe_raid_group_lock(raid_group_p);
            if (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_KEYS) {
                fbe_base_config_set_encryption_state((fbe_base_config_t *)raid_group_p, 
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE);
                fbe_raid_group_unlock(raid_group_p);
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "Encryption: keys pushed REKEY_COMPLETE_PUSH_KEYS -> REKEY_COMPLETE_PUSH_DONE\n");
            } else if ((encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) && 
                       (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE) &&
                       (rekey_checkpoint == FBE_LBA_INVALID)) {
                fbe_base_config_set_encryption_state((fbe_base_config_t *)raid_group_p, 
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED);
                fbe_raid_group_unlock(raid_group_p);
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "Encryption: keys pushed RK_CMPLTE_PUSH_DONE -> KEYS_PROVIDED cp: %llx md: %d\n",
                                      (unsigned long long)rekey_checkpoint, encryption_mode);
            } else if ((raid_group_p->base_config.key_handle != NULL) &&
                       fbe_encryption_key_mask_both_valid(&raid_group_p->base_config.key_handle->encryption_keys[0].key_mask)) {
                fbe_base_config_set_encryption_state((fbe_base_config_t *)raid_group_p, 
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED_FOR_REKEY);
                fbe_raid_group_unlock(raid_group_p);
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "Encryption: keys pushed %d -> KEYS_PROVIDED_FOR_REKEY md: %d chkpt: %llx\n",
                                      encryption_state, encryption_mode, rekey_checkpoint);
            } else {
                fbe_base_config_set_encryption_state((fbe_base_config_t *)raid_group_p, 
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED);
                fbe_raid_group_unlock(raid_group_p);
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "Encryption: keys pushed %d -> KEYS_PROVIDED md: %d chkpt: %llx\n",
                                      encryption_state, encryption_mode, rekey_checkpoint);
            }
            /* Now that we have pushed the keys, reset the path attribute */
            fbe_raid_group_get_width(raid_group_p, &width);
            for (position = 0; position < width; position++)
            {
                fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &block_edge_p, position);
                fbe_block_transport_get_path_attributes(block_edge_p, &path_attr);
                if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED)
                {
                    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                          "Clearing KEYS_REQUIRED path attr. Curr. Path Attr:0x%08x, position:%d\n",
                                          path_attr, position );
                    fbe_block_transport_clear_path_attributes(block_edge_p, FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED);
                }
            }
            
        }
        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet_p);
    }
    else
    {
        /* Not done with processing sub packets.
         */
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}



fbe_status_t fbe_raid_group_does_any_edge_need_keys(fbe_raid_group_t *raid_group_p,
                                                    fbe_bool_t *b_needed)
{
    fbe_u32_t                           width;
    fbe_raid_position_t                 position;
    fbe_path_attr_t                     path_attr;
    fbe_block_edge_t *                  block_edge_p = NULL;
    *b_needed = FBE_FALSE;

    /* Scan the block edges and select an edge */
    fbe_raid_group_get_width(raid_group_p, &width);
    for (position = 0; position < width; position++) {
        fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &block_edge_p, position);
        fbe_block_transport_get_path_attributes(block_edge_p, &path_attr);

        if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED) {
            *b_needed = FBE_TRUE;
            break;
        }
    }
    return FBE_STATUS_OK;
}
fbe_lifecycle_status_t 
fbe_raid_group_monitor_push_keys_if_needed(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_bool_t			is_metadata_initialized;
    fbe_bool_t			is_encryption_key_needed;
    fbe_bool_t			push_keys_down;
    fbe_status_t		status;
    fbe_raid_group_t *	raid_group_p;                  
    fbe_raid_geometry_t        *raid_geometry_p = NULL;
    fbe_raid_group_type_t       raid_type;

    raid_group_p = (fbe_raid_group_t *)object_p;

    /* If we are in encrypted mode PVD will need a keys to start up */
    fbe_base_config_metadata_is_initialized((fbe_base_config_t *)raid_group_p, &is_metadata_initialized);

    if(fbe_base_config_is_encrypted_mode((fbe_base_config_t *)raid_group_p) ||
       fbe_base_config_is_rekey_mode((fbe_base_config_t *)raid_group_p)){
        
        /* Check if a new Key is required and if so wait for the keys to be provided */
        is_encryption_key_needed = fbe_raid_group_is_encryption_key_needed(raid_group_p);
        push_keys_down = fbe_base_config_is_encryption_state_push_keys_downstream((fbe_base_config_t*)raid_group_p);

        if(is_encryption_key_needed)
        {
            /* let's see if database has what we need */
            status = fbe_raid_group_request_database_encryption_key(raid_group_p, packet_p);
            if (status != FBE_STATUS_OK)
            {
                /* Update some information so that keys will be provided to us.
                 */
                status = fbe_raid_group_encryption_notify_key_need(raid_group_p, is_metadata_initialized);
        
                /* At this point there is nothing we can do. So just return until we get the key
                 */
                return FBE_LIFECYCLE_STATUS_DONE;
            }
            /* check the encryption state again */
            push_keys_down = fbe_base_config_is_encryption_state_push_keys_downstream((fbe_base_config_t*)raid_group_p);
        }
        /* if we have the key, or get it from database, see if we need to push it down */
        if(push_keys_down)
        {
            status = fbe_raid_group_push_keys_downstream(raid_group_p, packet_p);

            return FBE_LIFECYCLE_STATUS_PENDING;
        }
    }else {
        /* System raid groups need to push keys to PVDs even if we are not encrypted. 
         * The PVDs will not bring an edge ready until we give a key. 
         */
        fbe_system_encryption_mode_t		system_encryption_mode;
        fbe_bool_t                          b_needed;
        fbe_database_get_system_encryption_mode(&system_encryption_mode);
        if(system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED){
            fbe_raid_group_does_any_edge_need_keys(raid_group_p, &b_needed);
            if (b_needed) {
                /* Check if this a RG that can be encrypted. If it is then we need a real key. If not
                 * we can just send a NULL one just to get PVD online */
                if(fbe_raid_group_is_encryption_capable(raid_group_p))
                {
                    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
                    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
                    if (((fbe_base_config_t *)raid_group_p)->base_object.class_id != FBE_CLASS_ID_VIRTUAL_DRIVE &&
                        (raid_type != FBE_RAID_GROUP_TYPE_RAID10))
                    {
                        /* we need to get a real key in this case. */
                        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                              "%s Keys needed \n",
                                              __FUNCTION__);
                        /* let's see if database has what we need */
                        status = fbe_raid_group_request_database_encryption_key(raid_group_p, packet_p);
                        if (status != FBE_STATUS_OK)
                        {
                            /* Update some information so that keys will be provided to us.
                             */
                            status = fbe_raid_group_encryption_notify_key_need(raid_group_p, is_metadata_initialized);
            
                            /* At this point there is nothing we can do. So just return until we get the key
                             */
                            return FBE_LIFECYCLE_STATUS_DONE;
                        }
                    } 
                    else 
                    {
                        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                              "%s Ignore Keys needed \n",
                                              __FUNCTION__);
                        return FBE_LIFECYCLE_STATUS_CONTINUE;
                    }
                }
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s Push Keys down \n",
                                          __FUNCTION__);
                fbe_raid_group_push_keys_downstream(raid_group_p, packet_p);
                return FBE_LIFECYCLE_STATUS_PENDING;
            }
        }
    }
    return FBE_LIFECYCLE_STATUS_CONTINUE;
}

/*!****************************************************************************
 * fbe_raid_group_request_database_encryption_key()
 ******************************************************************************
 * @brief
 *  This function is used generate request to database for encryption key handle
 *
 * @param raid_group_p  - raid group.
 * @param packet_p      - packet.
 *
 * @return status           - status of the operation.
 *
 * @author
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_request_database_encryption_key(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t                       status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                          width = 0;
    fbe_base_config_encryption_state_t encryption_state;
    fbe_encryption_setup_encryption_keys_t * key_handle;
    fbe_object_id_t object_id;

    object_id = fbe_raid_group_get_object_id(raid_group_p);

    status = fbe_database_control_get_object_encryption_key_handle(object_id, &key_handle);

    /* database doesn't have it */
    if (key_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s database doesn't have the key\n", 
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* get the width of the raid group object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);
#if 0
    if(width != key_handle->num_of_keys)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Number of keys unexpected Exp:%d actual:%d\n", 
                              __FUNCTION__, width, key_handle->num_of_keys);
        return FBE_STATUS_GENERIC_FAILURE; 

    }
#endif

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Retreive the key handle\n", 
                          __FUNCTION__);

    /* This sanity check will be removed */
    if(fbe_raid_geometry_is_raid10(fbe_raid_group_get_raid_geometry(raid_group_p)) && key_handle != NULL){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s Valid key handle for striper ? \n", __FUNCTION__);
    }	

    /* Save the key handle */
    status = fbe_base_config_set_key_handle((fbe_base_config_t *)raid_group_p, key_handle);

    fbe_base_config_get_encryption_state((fbe_base_config_t *)raid_group_p, &encryption_state);
    if (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE) {
        /* Change the state so that the keys will be pushed down from the monitor context */
        fbe_base_config_set_encryption_state((fbe_base_config_t *)raid_group_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_KEYS);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: REKEY_COMPLETE -> REKEY_COMPLETE_PUSH_KEYS\n");
    } else {
        /* Change the state so that the keys will be pushed down from the monitor context */
        fbe_base_config_set_encryption_state((fbe_base_config_t *)raid_group_p, 
                                              FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_PUSH_KEYS_DOWNSTREAM);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: %d -> LOCKED_PUSH_KEYS_DOWNSTREAM\n", encryption_state);
    }

    return FBE_STATUS_OK;
}
static fbe_lifecycle_status_t 
fbe_raid_group_handle_system_encryption_state_machine(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_base_config_encryption_state_t	encryption_state;
    fbe_base_config_encryption_mode_t   base_config_encryption_mode;
    fbe_object_id_t						object_id;
    fbe_raid_geometry_t * raid_geometry_p;
    fbe_status_t status;
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t   * control_operation;
    fbe_bool_t                          b_is_active;
    fbe_lifecycle_status_t				lifecycle_status;
    fbe_lba_t                           rekey_checkpoint;
    fbe_raid_group_encryption_flags_t   flags;
    fbe_scheduler_hook_status_t         hook_status;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    fbe_base_object_get_object_id((fbe_base_object_t *)raid_group_p, &object_id);

    /* We skipping system objects for now */
    if(!fbe_database_is_object_system_rg(object_id)){
        return FBE_LIFECYCLE_STATUS_CONTINUE;
    }
    if (!fbe_raid_geometry_is_attribute_set(raid_geometry_p, FBE_RAID_ATTRIBUTE_VAULT)){
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)raid_group_p);

    fbe_base_config_get_encryption_state((fbe_base_config_t *)raid_group_p, &encryption_state);

    /* Check if we have a keys first */
    if(encryption_state <= FBE_BASE_CONFIG_ENCRYPTION_STATE_UNENCRYPTED){ /* We do not have keys */
        fbe_raid_group_encryption_notify_key_need(raid_group_p, FBE_TRUE /* metadata_initialized */);
        return FBE_LIFECYCLE_STATUS_DONE; /* Key userper will rescedule monitor */
    }

    if (fbe_base_config_is_encryption_state_push_keys_downstream((fbe_base_config_t *)raid_group_p)) {
        status = fbe_raid_group_push_keys_downstream(raid_group_p, packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* We should wait untill keys are provided */
    if(encryption_state < FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED){ /* We do not have keys */
        return FBE_LIFECYCLE_STATUS_DONE; /* Key userper will rescedule monitor */
    }

    fbe_raid_group_encryption_get_state(raid_group_p, &flags);
    rekey_checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload); /* Monitor control operation */

    /* Check if raid is in encryption related mode */
    fbe_base_config_get_encryption_mode((fbe_base_config_t *)raid_group_p, &base_config_encryption_mode);

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                         "Encryption: sys sm %d cp: %llx fl: 0x%x md: %d\n",
                         encryption_state, (unsigned long long)fbe_raid_group_get_rekey_checkpoint(raid_group_p), 
                         flags, base_config_encryption_mode);
    if(base_config_encryption_mode <= FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED){
        
        if ((rekey_checkpoint != 0) || (flags != 0)) {
            /* We have values that need to be reset in NP before we start the encryption. 
             * This is required so that once we change the encryption mode we realize that we need 
             * to work through our state machine to get the flags set and then drive the checkpoint. 
             */
            fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_encryption_start_mark_np);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }

        /* If we here we do have keys and now it is time to mark monitor packet for attention */
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_UNIT_ATTENTION);
        fbe_payload_control_set_status_qualifier(control_operation, 
                                                    FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS_REQUEST);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_LIFECYCLE_STATUS_PENDING; /* Mode change from job will reschedule the monitor */
    }

    if ((base_config_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) &&
        (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED_FOR_REKEY)){

        if ((rekey_checkpoint != 0) || (flags != 0)) {
            /* We have values that need to be reset in NP before we start the encryption. 
             * This is required so that once we change the encryption mode we realize that we need 
             * to work through our state machine to get the flags set and then drive the checkpoint. 
             */
            fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_encryption_start_mark_np);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }

        /* If we here we do have keys and now it is time to mark monitor packet for attention */
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_UNIT_ATTENTION);
        fbe_payload_control_set_status_qualifier(control_operation, 
                                                    FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_ENCRYPTION_MODE_REKEY_IN_PROGRESS_REQUEST);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING; /* Mode change from job will reschedule the monitor */
    } 

    /* check if encryption rekey needs to run */
    if (fbe_raid_group_encryption_is_rekey_needed(raid_group_p)){

        if (b_is_active == FBE_TRUE) {
            /* run encryption background operation */
            lifecycle_status = fbe_raid_group_monitor_run_vault_encryption((fbe_base_object_t * )raid_group_p, packet_p);
            if(lifecycle_status != FBE_LIFECYCLE_STATUS_DONE){
                return lifecycle_status;
            }
            /* lifecycle_status == FBE_LIFECYCLE_STATUS_DONE here */
            /* Take care of sriper */
            if (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE){
                /* We should continue */
            } else if((encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE)){
                /* Regular raid - We should continue */
            } else { /* KMS should "update" keys */
                return lifecycle_status;
            }
        }
    }
    
    if (!b_is_active && 
        (fbe_raid_group_encryption_is_rekey_needed(raid_group_p) ||
         (fbe_base_config_is_encrypted_mode((fbe_base_config_t*)raid_group_p) &&
          !fbe_base_config_is_encrypted_state((fbe_base_config_t*)raid_group_p)))) {
        /* On the passive side we might need to change our state. */
        fbe_raid_group_encryption_change_passive_state(raid_group_p);
    }
    /* If we completed rekey process and keys removed we need to transition to FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED mode */
    if((base_config_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
        (base_config_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS)){

        if(encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE){ 
            fbe_raid_group_check_hook(raid_group_p,
                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                      FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETING,
                                      0, &hook_status);

            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_UNIT_ATTENTION);
            fbe_payload_control_set_status_qualifier(control_operation, 
                FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_ENCRYPTION_MODE_ENCRYPTED_REQUEST);

            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);

            fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                                 "Encryption request encrypted stpr state: %d mode: %d\n", encryption_state, base_config_encryption_mode);

            if (hook_status == FBE_SCHED_STATUS_DONE) 
            { 
                return FBE_LIFECYCLE_STATUS_DONE; 
            }

            return FBE_LIFECYCLE_STATUS_PENDING; /* Mode change from job will reschedule the monitor */
        }
    } /* encription or rekey is in progress */

    /* We are done with rekey.  Wait for rekey_checkpoint to be invalid  */
    fbe_raid_group_lock(raid_group_p);
    rekey_checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    if ((base_config_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) && 
        (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE) &&
        (rekey_checkpoint == FBE_LBA_INVALID)) {
        fbe_raid_group_check_hook(raid_group_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                  FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETE_REMOVE_OLD_KEY,
                                  0, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) {
            fbe_raid_group_unlock(raid_group_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: sm %d -> RK_CMPLT_RM_OLD_KEY cp: %llx fl: 0x%x md: %d\n",
                              encryption_state, (unsigned long long)fbe_raid_group_get_rekey_checkpoint(raid_group_p), 
                              flags, base_config_encryption_mode);
        
        fbe_base_config_set_encryption_state((fbe_base_config_t *) raid_group_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_REMOVE_OLD_KEY); 
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_config_encryption_state_notification((fbe_base_config_t *) raid_group_p);
    } else {
        fbe_raid_group_unlock(raid_group_p);
    }
    return FBE_LIFECYCLE_STATUS_CONTINUE;
}
/**************************************
 * end fbe_raid_group_handle_system_encryption_state_machine()
 **************************************/

/* Will be called from ... if system is in encrypted mode */
static fbe_lifecycle_status_t 
fbe_raid_group_background_handle_encryption_state_machine(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_base_config_encryption_state_t	encryption_state;
    fbe_base_config_encryption_mode_t   base_config_encryption_mode;
    fbe_object_id_t						object_id;
    fbe_raid_geometry_t * raid_geometry_p;
    fbe_status_t status;
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t   * control_operation;
    fbe_bool_t                          b_is_active;
    fbe_lifecycle_status_t				lifecycle_status;
    fbe_bool_t is_striper = FBE_FALSE;
    fbe_bool_t is_under_striper = FBE_FALSE;
    fbe_lba_t                           rekey_checkpoint;
    fbe_medic_action_priority_t         ds_medic_priority;
    fbe_scheduler_hook_status_t         hook_status = FBE_SCHED_STATUS_OK;
    fbe_raid_group_encryption_flags_t   flags;
    fbe_lifecycle_state_t               peer_lifecycle_state;
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    fbe_base_object_get_object_id((fbe_base_object_t *)raid_group_p, &object_id);

    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t*)raid_group_p, &peer_lifecycle_state);

    if (fbe_cmi_is_peer_alive() &&
        (peer_lifecycle_state != FBE_LIFECYCLE_STATE_READY)) {
        /* If cmi is up, but the peer is not ready yet, do not run encryption. 
         * We will wait for join so as not to confuse the peer if we decide to finish encryption. 
         */
        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                             "Encryption: sm do not run cp: %llx pstate: 0x%x\n",
                             (unsigned long long)fbe_raid_group_get_rekey_checkpoint(raid_group_p), 
                             peer_lifecycle_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    /* We skipping system objects for now */
    if(fbe_database_is_object_system_rg(object_id)){
        return fbe_raid_group_handle_system_encryption_state_machine(raid_group_p, packet_p);
    }

    fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                              FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_BEFORE_INITIAL_KEY, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    is_striper = fbe_raid_geometry_is_raid10(raid_geometry_p); /* For the striper of Raid10 we will switch mode right away */

    /* in the future we will put Raid50 and Raid60 here */
    is_under_striper = fbe_raid_geometry_is_mirror_under_striper(raid_geometry_p);

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)raid_group_p);

    fbe_base_config_get_encryption_state((fbe_base_config_t *)raid_group_p, &encryption_state);

    /* Check if we have a keys first */
    if(encryption_state <= FBE_BASE_CONFIG_ENCRYPTION_STATE_UNENCRYPTED && !is_striper){ /* We do not have keys */
        /* let's see if database has what we need */
        status = fbe_raid_group_request_database_encryption_key(raid_group_p, packet_p);
        if (status != FBE_STATUS_OK)
        {

            fbe_raid_group_encryption_notify_key_need(raid_group_p, FBE_TRUE /* metadata_initialized */);
            return FBE_LIFECYCLE_STATUS_DONE; /* Key userper will rescedule monitor */
        }
    }

    if (fbe_base_config_is_encryption_state_push_keys_downstream((fbe_base_config_t *)raid_group_p)) {
        status = fbe_raid_group_push_keys_downstream(raid_group_p, packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* We should wait untill keys are provided */
    if(encryption_state < FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED && !is_striper){ /* We do not have keys */
        return FBE_LIFECYCLE_STATUS_DONE; /* Key userper will rescedule monitor */
    }

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload); /* Monitor control operation */

    fbe_raid_group_encryption_get_state(raid_group_p, &flags);
    rekey_checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);

    /* Check if raid is in encryption related mode */
    fbe_base_config_get_encryption_mode((fbe_base_config_t *)raid_group_p, &base_config_encryption_mode);

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                         "Encryption: sm %d cp: %llx fl: 0x%x md: %d\n",
                         encryption_state, (unsigned long long)fbe_raid_group_get_rekey_checkpoint(raid_group_p), 
                         flags, base_config_encryption_mode);

    if(base_config_encryption_mode <= FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED && !is_under_striper){

        ds_medic_priority = fbe_base_config_get_downstream_edge_priority((fbe_base_config_t*)raid_group_p);
        if ((ds_medic_priority > FBE_MEDIC_ACTION_ENCRYPTION_REKEY) || (ds_medic_priority == FBE_MEDIC_ACTION_ZERO))
        {
             fbe_raid_group_check_hook(raid_group_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_DOWNSTREAM_HIGHER_PRIORITY, 0,  &hook_status);

             /* No hook status is handled since we can only log here.
              */
             fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_monitor_encryption_sm Zeroing or ds medic priority %d higher\n",
                              ds_medic_priority);

            /* We want to wait for the DS operations to complete we start. So just return which will reschedule the monitor */
            return FBE_LIFECYCLE_STATUS_DONE; /* Key userper will rescedule monitor */
        }
        if (!fbe_database_can_encryption_start(object_id)) {
            fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                                   "rg_monitor_encryption_sm cannot start encryption\n");

            return FBE_LIFECYCLE_STATUS_DONE;
        }
        if (is_striper) {

            /* If the striper is encrypted, detect the push to our mirrors and kick off encryption.
             */
            status = fbe_striper_check_ds_encryption_state(raid_group_p, 
                                                           FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED);
            if (status != FBE_STATUS_OK) {
                /* Need to wait for keys */
                return FBE_LIFECYCLE_STATUS_DONE;
            }
            /* Wait for mirror to reset its checkpoint.
             */
            status = fbe_striper_check_ds_encryption_chkpt(raid_group_p, 0);
            if (status != FBE_STATUS_OK) {
                /* Need to wait for mirror to reset chkpt */
                return FBE_LIFECYCLE_STATUS_DONE;
            }
        }

        if ((rekey_checkpoint != 0) || (flags != 0)) {
            /* We have values that need to be reset in NP before we start the encryption. 
             * This is required so that once we change the encryption mode we realize that we need 
             * to work through our state machine to get the flags set and then drive the checkpoint. 
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encryption: sm request encr: mode: %d chkpt: %llx flags: 0x%x state: %d\n",
                                  base_config_encryption_mode, rekey_checkpoint, flags, encryption_state);
            fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_encryption_start_mark_np);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
        /* If we here we do have keys and now it is time to mark monitor packet for attention */
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_UNIT_ATTENTION);
        fbe_payload_control_set_status_qualifier(control_operation, 
                                                    FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS_REQUEST);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_LIFECYCLE_STATUS_PENDING; /* Mode change from job will reschedule the monitor */
    }

    if ( is_under_striper && 
         ((base_config_encryption_mode <= FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED) ||

          ((base_config_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) &&
           (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED_FOR_REKEY)) ) ) {
        /* We are a mirror that has new keys.  Check if chckpoint needs to be adjusted before encryption starts.
         */
        if ((rekey_checkpoint != 0) || (flags != 0)) {
            /* We have values that need to be reset in NP before we start the encryption. 
             * This is required so that once we change the encryption mode we realize that we need 
             * to work through our state machine to get the flags set and then drive the checkpoint. 
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encryption: sm undr striper encr: mode: %d chkpt: %llx flags: 0x%x state: %d\n",
                                  base_config_encryption_mode, rekey_checkpoint, flags, encryption_state);
            fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_encryption_start_mark_np);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
    }

    if ((base_config_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) &&
        (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED_FOR_REKEY) && 
         !is_under_striper){

        if ((rekey_checkpoint != 0) || (flags != 0)) {
            /* We have values that need to be reset in NP before we start the encryption. 
             * This is required so that once we change the encryption mode we realize that we need 
             * to work through our state machine to get the flags set and then drive the checkpoint. 
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encryption sm request rekey: mode: %d chkpt: %llx flags: 0x%x state: %d\n",
                                  base_config_encryption_mode, rekey_checkpoint, flags, encryption_state);
            fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_encryption_start_mark_np);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }

        /* If we here we do have keys and now it is time to mark monitor packet for attention */
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_UNIT_ATTENTION);
        fbe_payload_control_set_status_qualifier(control_operation, 
                                                    FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_ENCRYPTION_MODE_REKEY_IN_PROGRESS_REQUEST);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_LIFECYCLE_STATUS_PENDING; /* Mode change from job will reschedule the monitor */
    } else if (is_striper &&
               (base_config_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED)){

        /* If the striper is encrypted, detect the push to our mirrors and kick off rekey.
         */
        status = fbe_striper_check_ds_encryption_state(raid_group_p, 
                                                       FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED_FOR_REKEY);

        if (status == FBE_STATUS_OK) {
            
            /* Wait for mirror to reset its checkpoint.
             */
            status = fbe_striper_check_ds_encryption_chkpt(raid_group_p, 0);
            if (status == FBE_STATUS_OK) {
                if ((rekey_checkpoint != 0) || (flags != 0)) {
                    /* We have values that need to be reset in NP before we start the encryption. 
                     * This is required so that once we change the encryption mode we realize that we need 
                     * to work through our state machine to get the flags set and then drive the checkpoint. 
                     */
                    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                          "Encryption sm request rekey1: mode: %d chkpt: %llx flags: 0x%x state: %d\n",
                                          base_config_encryption_mode, rekey_checkpoint, flags, encryption_state);
                    fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_encryption_start_mark_np);
                    return FBE_LIFECYCLE_STATUS_PENDING;
                }
                /* If we here we do have keys and now it is time to mark monitor packet for attention */
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_UNIT_ATTENTION);
                fbe_payload_control_set_status_qualifier(control_operation, 
                                                            FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_ENCRYPTION_MODE_REKEY_IN_PROGRESS_REQUEST);
                fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet_p);
        
                return FBE_LIFECYCLE_STATUS_PENDING; /* Mode change from job will reschedule the monitor */
            }
        }
    }

    /* check if encryption rekey needs to run */
    if (fbe_raid_group_encryption_is_rekey_needed(raid_group_p)){
        /* Encryption is running update the upsteam edge priority. */
        status = fbe_base_config_set_upstream_edge_priority((fbe_base_config_t *)raid_group_p, FBE_MEDIC_ACTION_ENCRYPTION_REKEY);
        if (status != FBE_STATUS_OK){
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: set upstream action to rekey failed. status: 0x%x\n",
                                  __FUNCTION__, status);
        }

        if (b_is_active == FBE_TRUE) {
            /* run encryption background operation */
            lifecycle_status = fbe_raid_group_monitor_run_encryption((fbe_base_object_t * )raid_group_p, packet_p);
            if(lifecycle_status != FBE_LIFECYCLE_STATUS_DONE){
                return lifecycle_status;
            }
            /* lifecycle_status == FBE_LIFECYCLE_STATUS_DONE here */
            /* Take care of sriper */
            if((encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE) && is_striper){
                /* We should continue */
            } else if((encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE)){
                /* Regular raid - We should continue */
            } else { /* KMS should "update" keys */
                return lifecycle_status;
            }
        }
    }
    if (!b_is_active && 
        (fbe_raid_group_encryption_is_rekey_needed(raid_group_p) ||
         (fbe_base_config_is_encrypted_mode((fbe_base_config_t*)raid_group_p) &&
          !fbe_base_config_is_encrypted_state((fbe_base_config_t*)raid_group_p)))) {
        /* On the passive side we might need to change our state. */
        fbe_raid_group_encryption_change_passive_state(raid_group_p);
    }
    

    /* If we completed rekey process and keys removed we need to transition to FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED mode */
    if((base_config_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
        (base_config_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS)){

        if(encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE && !is_striper && !is_under_striper){ 
            fbe_raid_group_check_hook(raid_group_p,
                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                      FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETING,
                                      0, &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE) { return FBE_LIFECYCLE_STATUS_DONE; }

            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_UNIT_ATTENTION);
            fbe_payload_control_set_status_qualifier(control_operation, 
                                                    FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_ENCRYPTION_MODE_ENCRYPTED_REQUEST);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);

            fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                                 "Encryption request encrypted state: %d mode: %d\n", encryption_state, base_config_encryption_mode);
            if (hook_status == FBE_SCHED_STATUS_DONE) 
            { 
                return FBE_LIFECYCLE_STATUS_DONE; 
            }

            return FBE_LIFECYCLE_STATUS_PENDING; /* Mode change from job will reschedule the monitor */
        }

        /* For the striper we will check rekey checkpoint and current mode */
        if(is_striper){
            if((base_config_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
                (base_config_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS)){
        
                    /* This will check all downstream objects of the striper if thes have expected state */
                status = fbe_striper_check_ds_encryption_state(raid_group_p, FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE);
                
                if(status == FBE_STATUS_OK){ /* We done with encryption or rekey */
                    fbe_raid_group_check_hook(raid_group_p,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                              FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETING,
                                              0, &hook_status);
                    if (hook_status == FBE_SCHED_STATUS_DONE) { return FBE_LIFECYCLE_STATUS_DONE; }

                    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_UNIT_ATTENTION);
                    fbe_payload_control_set_status_qualifier(control_operation, 
                                                            FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_ENCRYPTION_MODE_ENCRYPTED_REQUEST);
                    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                    fbe_transport_complete_packet(packet_p);

                    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                                         "Encryption request encrypted stpr state: %d mode: %d\n", encryption_state, base_config_encryption_mode);
                    if (hook_status == FBE_SCHED_STATUS_DONE) 
                    { 
                        return FBE_LIFECYCLE_STATUS_DONE; 
                    }

                    return FBE_LIFECYCLE_STATUS_PENDING; /* Mode change from job will reschedule the monitor */
                }
            }/* mode IN_PROGRESS */
        }/* if(is_striper) */
    } /* encription or rekey is in progress */

    /* We done with rekey */
    /* Wait for rekey_checkpoint to be invalid */
    if(!is_striper){
        fbe_raid_group_lock(raid_group_p);
        rekey_checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
        if((base_config_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) && 
           (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE) &&
           (rekey_checkpoint == FBE_LBA_INVALID)) {

            fbe_raid_group_check_hook(raid_group_p,
                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                      FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETE_REMOVE_OLD_KEY,
                                      0, &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE) {
                fbe_raid_group_unlock(raid_group_p);
                return FBE_LIFECYCLE_STATUS_DONE;
            }
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encryption: %d -> RK_CMPLT_RM_OLD_KEY cp: %llx fl: 0x%x md: %d\n",
                                  encryption_state,
                                  (unsigned long long)fbe_raid_group_get_rekey_checkpoint(raid_group_p), 
                                  flags, base_config_encryption_mode);
            fbe_base_config_set_encryption_state((fbe_base_config_t *) raid_group_p, 
                                                 FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_REMOVE_OLD_KEY);
            fbe_raid_group_unlock(raid_group_p);
            fbe_base_config_encryption_state_notification((fbe_base_config_t *) raid_group_p);
        } else {
            fbe_raid_group_unlock(raid_group_p);
        }
    } else { /* striper */

    }

    return FBE_LIFECYCLE_STATUS_CONTINUE;
}

/*!****************************************************************************
 * fbe_raid_group_is_drive_tier_query_needed()
 ******************************************************************************
 * @brief
 *  This function is used to determine whether we should query the drives downstream
 *  for the raidgroup's nonpaged drive tier
 *
 * @param raid_group_p  - raid group.
 *
 * @return boolean  
 *
 * @author 
 *   2/6/2014 - Created. Deanna Heng
 ******************************************************************************/
fbe_bool_t fbe_raid_group_is_drive_tier_query_needed(fbe_raid_group_t *raid_group_p) {
    fbe_raid_geometry_t                        *raid_geometry_p = NULL;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    if (fbe_raid_geometry_is_vault(raid_geometry_p)) 
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_raid_group_is_drive_tier_query_needed()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_monitor_update_nonpaged_drive_tier()
 ******************************************************************************
 * @brief
 *  Query downstream for the drive tier of each drive in the raid group.
 *
 * @param raid_group_p  - raid group.
 * @param packet_p - pointer to the packet
 *
 * @return lifecycle status 
 *
 * @author 
 *   2/6/2014 - Created. Deanna Heng
 ******************************************************************************/
fbe_status_t fbe_raid_group_monitor_update_nonpaged_drive_tier(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_monitor_update_nonpaged_drive_tier_with_lock);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_monitor_update_nonpaged_drive_tier()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_monitor_update_nonpaged_drive_tier_with_lock()
 ******************************************************************************
 * @brief
 *  Query downstream for the drive tier of each drive in the raid group with the lock.
 *
 * @param raid_group_p  - raid group.
 * @param packet_p - pointer to the packet
 *
 * @return lifecycle status 
 *
 * @author 
 *   2/6/2014 - Created. Deanna Heng
 ******************************************************************************/
fbe_status_t fbe_raid_group_monitor_update_nonpaged_drive_tier_with_lock(fbe_packet_t * packet_p, 
                                                                         fbe_packet_completion_context_t context)
{
    fbe_raid_group_t 	                       *raid_group_p = NULL;   
    fbe_raid_group_get_drive_tier_downstream_t *drive_tier_downstream_p = NULL;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_status_t                                status = FBE_STATUS_OK;

    raid_group_p = (fbe_raid_group_t *)context;

    /* If the status is not ok, that means we didn't get the lock.
     */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Fetched NP lock.\n",
                          __FUNCTION__);

    /* Push a completion on stack to release NP lock. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);    


    /* allocate the buffer to send the raid group zero percent raid extent usurpur. */
    drive_tier_downstream_p = 
            (fbe_raid_group_get_drive_tier_downstream_t *) fbe_transport_allocate_buffer();
    if (drive_tier_downstream_p == NULL)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* allocate the new control operation to build. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_RAID_GROUP_CONTROL_CODE_GET_LOWEST_DRIVE_TIER_DOWNSTREAM,
                                        drive_tier_downstream_p,
                                        sizeof(fbe_raid_group_get_drive_tier_downstream_t));
    
    fbe_payload_ex_increment_control_operation_level(sep_payload_p);

    fbe_transport_set_completion_function(packet_p,
                                          fbe_raid_group_monitor_update_nonpaged_drive_tier_completion,
                                          raid_group_p);

    /* call the function to get the drive tier of the raid group. */
    fbe_raid_group_query_for_lowest_drive_tier(raid_group_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_monitor_update_nonpaged_drive_tier_with_lock()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_monitor_update_nonpaged_drive_tier_completion()
 ******************************************************************************
 * @brief
 *  Analyze the data received from the subpackets and update the drive
 *  tier of the raid group as necessary
 *
 * @param packet_p - pointer to the packet
 * @param context  - raid group.
 *
 * @return status 
 *
 * @author 
 *   2/6/2014 - Created. Deanna Heng
 ******************************************************************************/
fbe_status_t fbe_raid_group_monitor_update_nonpaged_drive_tier_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t                                status;
    fbe_u32_t                                   width;
    fbe_u32_t                                   index;
    fbe_raid_group_get_drive_tier_downstream_t *drive_tier_downstream_p = NULL;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_payload_control_status_t                control_status;
    fbe_drive_performance_tier_type_np_t        rg_drive_tier;
    fbe_raid_group_t                           *raid_group_p = NULL; 
    fbe_raid_group_nonpaged_metadata_t         *nonpaged_metadata_p;
          
    

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the current value for the non-paged flags.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);

    /* get the sep payload and control operation */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* If the status is not ok release the control operation
       and return ok so that packet gets completed
     */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_buffer(control_operation_p, &drive_tier_downstream_p);
    if(status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        /* Release the control buffer and control operation */
        fbe_transport_release_buffer(drive_tier_downstream_p);
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);

        /* A drive is removed before we get here and drive tier hasn't been initialized */
        if (nonpaged_metadata_p->drive_tier == FBE_DRIVE_PERFORMANCE_TIER_TYPE_UNINITIALIZED) 
        {
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            /* update the drive tier only if the calculated drive tier has increased */
            fbe_raid_group_metadata_write_drive_performance_tier_type(raid_group_p,
                                                                      packet_p,
                                                                      FBE_DRIVE_PERFORMANCE_TIER_TYPE_INVALID);
    
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        } 
  
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0); 
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p, 
                                                       sizeof(fbe_raid_group_get_drive_tier_downstream_t),
                                                       (fbe_payload_control_buffer_t)&drive_tier_downstream_p);
    if (status != FBE_STATUS_OK) 
    { 
        /* Release the control buffer and control operation */
        fbe_transport_release_buffer(drive_tier_downstream_p);
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* The drive tier of the raid group is the lowest drive tier of the physical drives
     * in the raid group  */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);
    rg_drive_tier = drive_tier_downstream_p->drive_tier[0];
    for(index = 1; index < width; index++)
    {
        if (drive_tier_downstream_p->drive_tier[index] < rg_drive_tier) 
        {
            rg_drive_tier = drive_tier_downstream_p->drive_tier[index];
        }
    }

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                         "%s raid group calculated drive tier is 0x%x\n", 
                         __FUNCTION__, rg_drive_tier);

    /* Release the control buffer and control operation */
    fbe_transport_release_buffer(drive_tier_downstream_p);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);

    /* update the drive tier only if the calculated drive tier has increased */
    status = fbe_raid_group_metadata_write_drive_performance_tier_type(raid_group_p,
                                                                       packet_p,
                                                                       rg_drive_tier);
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}
/******************************************************************************
 * end fbe_raid_group_monitor_update_nonpaged_drive_tier_completion()
 ******************************************************************************/

static fbe_bool_t fbe_raid_group_is_encryption_capable(fbe_raid_group_t *raid_group_p)
{
    fbe_object_id_t object_id = ((fbe_base_object_t *)raid_group_p)->object_id;
    
    /* Currently we support encryption on Vault RG and User objects */
    if(
       (object_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG) ||
       !fbe_private_space_layout_object_id_is_system_raid_group(object_id))
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}


/*!****************************************************************************
 * fbe_raid_group_wear_level_cond_function()
 ******************************************************************************
 * @brief
 *  Get the max wear level of the drives in the raid group
 *
 * @param object_p               - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 *
 * @return status 
 *
 * @author 
 *   6/24/2015 - Created. Deanna Heng
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_raid_group_wear_level_cond_function(fbe_base_object_t *object_p, 
                                                                      fbe_packet_t *packet_p)
{
    fbe_raid_group_t 	                       *raid_group_p = NULL;   
    fbe_raid_group_get_wear_level_downstream_t *wear_level_downstream_p = NULL;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_raid_geometry_t                        *raid_geometry_p = NULL;
    fbe_scheduler_hook_status_t                 hook_status = FBE_SCHED_STATUS_OK;
    fbe_bool_t                                  is_under_striper = FBE_FALSE;
    fbe_bool_t                                  is_striper = FBE_FALSE;

    raid_group_p = (fbe_raid_group_t *)object_p;
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    is_under_striper = fbe_raid_geometry_is_mirror_under_striper(raid_geometry_p);
    is_striper = fbe_raid_geometry_is_raid10(raid_geometry_p); 

    /*! @note In order to support usurper metadata operations we will postpone
     *        the quiesce request until the usurper request is complete.
     */
    fbe_raid_group_check_hook(raid_group_p,
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY,  
                              FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START, 
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* We only want to collect the wear level for SSD drives 
     *
     * There is no need to perform the query for the mirrors under striper since the striper
     * will send the necessary usurpers down to the mirrors
     */
    if (!fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_MIRROR_SEQUENTIAL_DISABLED) ||
        is_under_striper)
    {
        raid_group_p->max_pe_cycle = 0;
        raid_group_p->current_pe_cycle = 0;
        raid_group_p->power_on_hours = 0;
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)raid_group_p);
    
        return FBE_LIFECYCLE_STATUS_DONE; 
    }
    
    /* allocate the buffer to send the raid group zero percent raid extent usurpur. */
    wear_level_downstream_p = (fbe_raid_group_get_wear_level_downstream_t *) fbe_transport_allocate_buffer();
    if (wear_level_downstream_p == NULL)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* allocate the new control operation to build. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_RAID_GROUP_CONTROL_CODE_GET_WEAR_LEVEL_DOWNSTREAM,
                                        wear_level_downstream_p,
                                        sizeof(fbe_raid_group_get_wear_level_downstream_t));
    
    fbe_payload_ex_increment_control_operation_level(sep_payload_p);

    fbe_transport_set_completion_function(packet_p,
                                          fbe_raid_group_monitor_update_max_wear_level_completion,
                                          raid_group_p);

    if (is_striper)
    {
        /* call the function to get the drive tier of the raid group for raid10. */
        fbe_striper_usurper_get_wear_level_downstream(raid_group_p, packet_p);
    } else
    {
        /* call the function to get the drive tier of the raid group. */
        fbe_raid_group_usurper_get_wear_level_downstream(raid_group_p, packet_p);
    }

    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_raid_group_wear_level_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_monitor_update_max_wear_level_completion()
 ******************************************************************************
 * @brief
 *  Update the raid group information received from the control packet
 *
 * @param packet_p - pointer to the packet
 * @param context  - raid group.
 *
 * @return status 
 *
 * @author 
 *   06/25/2015 - Created. Deanna Heng
 ******************************************************************************/
fbe_status_t fbe_raid_group_monitor_update_max_wear_level_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t                                status;
    fbe_raid_group_get_wear_level_downstream_t *wear_level_downstream_p = NULL;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_payload_control_status_t                control_status;
    fbe_raid_group_t                           *raid_group_p = NULL; 
    fbe_object_id_t                             rg_object_id;
    fbe_raid_group_number_t                     raid_group_id;  
    

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* get the sep payload and control operation */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* If the status is not ok release the control operation
       and return ok so that packet gets completed
     */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_buffer(control_operation_p, &wear_level_downstream_p);
    if(status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        /* Release the control buffer and control operation */
        fbe_transport_release_buffer(wear_level_downstream_p);
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
  
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0); 
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p, 
                                                       sizeof(fbe_raid_group_get_wear_level_downstream_t),
                                                       (fbe_payload_control_buffer_t)&wear_level_downstream_p);
    if (status != FBE_STATUS_OK) 
    { 
        /* Release the control buffer and control operation */
        fbe_transport_release_buffer(wear_level_downstream_p);
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
        fbe_transport_set_status(packet_p, status, 0);
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_raid_group_lock(raid_group_p);
    raid_group_p->max_pe_cycle = wear_level_downstream_p->eol_PE_cycles;
    raid_group_p->current_pe_cycle = wear_level_downstream_p->max_erase_count;
    raid_group_p->power_on_hours = wear_level_downstream_p->power_on_hours;
    fbe_raid_group_unlock(raid_group_p);
    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                         "%s raid group max wear level set to 0x%x\n", 
                         __FUNCTION__, wear_level_downstream_p->max_wear_level_percent);
    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                         "rg wear update cur: 0x%llx pec: 0x%llx poh: 0x%llx\n", 
                         raid_group_p->current_pe_cycle, raid_group_p->max_pe_cycle, raid_group_p->power_on_hours);

    /* Release the control buffer and control operation */
    fbe_transport_release_buffer(wear_level_downstream_p);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);

    /* Get the raid group number and the raid group object ID
     * adjusted for RAID-10 if needed.
     */
    fbe_raid_group_logging_get_raid_group_number_and_object_id(raid_group_p,
                                                               &raid_group_id,
                                                               &rg_object_id);

    /* Send the notification to clients
     */
    fbe_block_transport_server_set_path_attr_all_servers(&raid_group_p->base_config.block_transport_server,
                                                         FBE_BLOCK_PATH_ATTR_FLAGS_WEAR_LEVEL_REQUIRED);

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)raid_group_p);
    return FBE_LIFECYCLE_STATUS_DONE;

}
/******************************************************************************
 * end fbe_raid_group_monitor_update_max_wear_level_completion()
 ******************************************************************************/


/*******************************
 * end fbe_raid_group_monitor.c
 *******************************/



 
 
