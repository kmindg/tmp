/******************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ******************************************************************************/

/*!****************************************************************************
 * @file    fbe_virtual_drive_monitor.c
 ******************************************************************************
 *
 * @brief   This file contains the virtual_drive object lifecycle code.  The
 *          virtual drive object has (2) primary responsibilities:
 *              1) Handle drive failures.  Wait a predetermined period of time
 *                 ('permanenent spare trigger time') and request a replacement
 *                 drive (permanent spare).
 *              2) Handle copy operations which consists of (3) phases:
 *                  2a) Swap-In - Request a `destination drive' to be swapped-in
 *                      to the unused virtual drive position.  This process
 *                      includes changing the virtual drive configuraiton mode
 *                      from `pass-thru' to `mirror'.
 *                  2b) Copy operation - Once the virtual drive is in mirror it
 *                      will continually drive the copy operation to completion.
 *                  2c) Swap-Out - Once the copy operation is complete (either
 *                      successfully or failure) the source drive will be
 *                      swapped-out.  First the configuration mode is changed to
 *                      `pass-thru' from `mirror' then the source position is
 *                      swapped-out.
 *
 * 
 *  This includes the
 *  @ref    fbe_virtual_drive_monitor_entry "virtual_drive monitor entry point", 
 *          as well as all the lifecycle defines such as rotaries and conditions, 
 *          along with the actual condition functions.
 * 
 * @ingroup virtual_drive_class_files
 *
 * @note    Since each of the virtual drive operations (swap-in, copy, swap-out)
 *          consist of multiple phases and conditions there must be a single
 *          method that can determine the next phase/condition that must be
 *          generated.  This is required since the SP could be rebooted at any
 *          time and the copy operation must continue on the alternate SP.
 *          Currently the method that ensures the copy operation proceeds is:
 *              o Evaluate downstream health.
 * 
 * @version
 *   05/20/2009:    Rob Foley   - Created.
 *
 ******************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_winddk.h"
#include "fbe_spare.h"
#include "fbe_transport_memory.h"
#include "fbe_virtual_drive_private.h"
#include "fbe_raid_group_needs_rebuild.h"
#include "fbe_raid_group_rebuild.h"
#include "fbe/fbe_event_log_utils.h"                    //  for message codes
#include "fbe_virtual_drive_event_logging.h"
#include "fbe_notification.h"

#define VD_EDGE_ATTACH_DEBOUNCE_TIME_MS    30000

         
/*!****************************************************************************
 * fbe_virtual_drive_monitor_entry()
 ******************************************************************************
 * @brief
 *  Entry routine for the virtual_drive's monitor.
 *
 * @param object_handle - This is the object handle, or in our case the pdo.
 * @param packet_p - The packet arriving from the monitor scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.             
 *
 * @author
 *  5/20/2009 - Created. RPF
 *
 ******************************************************************************/

fbe_status_t 
fbe_virtual_drive_monitor_entry(fbe_object_handle_t object_handle, 
                                fbe_packet_t * packet_p)
{
    fbe_status_t            status;
    fbe_virtual_drive_t *   virtual_drive_p = NULL;

    virtual_drive_p = (fbe_virtual_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_base_config_monitor_crank_object(&fbe_virtual_drive_lifecycle_const, 
                                        (fbe_base_object_t*)virtual_drive_p, packet_p);
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_monitor_entry()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_monitor_load_verify()
 ******************************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data that is 
 *  associated with the virtual_drive.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for the 
 *                        virtual_drive's constant lifecycle data.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ******************************************************************************/

fbe_status_t fbe_virtual_drive_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(virtual_drive));
}
/******************************************************************************
 * end fbe_virtual_drive_monitor_load_verify()
 ******************************************************************************/

/*--- local function prototypes --------------------------------------------------------*/

/* Downstream health broken condition related function. */
static fbe_lifecycle_status_t fbe_virtual_drive_downstream_health_broken_cond_function(fbe_base_object_t * object_p,
                                                                                       fbe_packet_t * packet_p);

/* Downstream health verification condition function. */
static fbe_lifecycle_status_t fbe_virtual_drive_fail_check_downstream_health_cond_function(fbe_base_object_t * object_p,
                                                                                       fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_virtual_drive_downstream_health_not_optimal_cond_function(fbe_base_object_t * object_p,
                                                                                            fbe_packet_t * packet_p);

/* Establish edge states condition function. */
static fbe_lifecycle_status_t fbe_virtual_drive_establish_edge_states_cond_function(fbe_base_object_t * in_object_p,
                                                                                fbe_packet_t * packet_p);

/* Downstream health disabled condition related function. */
static fbe_lifecycle_status_t fbe_virtual_drive_downstream_health_disabled_cond_function(fbe_base_object_t * object_p,
                                                                                         fbe_packet_t * packet_p);

/* Need permanent spare condition function. */
static fbe_lifecycle_status_t fbe_virtual_drive_need_replacement_drive_cond_function(fbe_base_object_t * object_p,
                                                                                   fbe_packet_t * packet_p);

/* Proactive spare swap-in related function. */
static fbe_lifecycle_status_t fbe_virtual_drive_need_proactive_spare_cond_function(fbe_base_object_t * object_p,
                                                                                   fbe_packet_t * packet_p);

/* Start user copy condition function. */
static fbe_lifecycle_status_t fbe_virtual_drive_start_user_copy_cond_function(fbe_base_object_t * object_p,
                                                                              fbe_packet_t * packet_p);

/* Need to re-evaluate the downstream health. */
static fbe_lifecycle_status_t fbe_virtual_drive_evaluate_downstream_health_cond_function(fbe_base_object_t * object_p,
                                                                                         fbe_packet_t * packet_p);

/* If required, `setup for the paged metadata verify */
static fbe_lifecycle_status_t fbe_virtual_drive_setup_for_verify_paged_metadata_cond_function(fbe_base_object_t *object_p, 
                                                                                              fbe_packet_t *packet_p);

/* If required verify the paged metadata */
static fbe_lifecycle_status_t fbe_virtual_drive_verify_paged_metadata_cond_function(fbe_base_object_t *object_p, 
                                                                                    fbe_packet_t * packet_p);

/* If required verify the paged metadata */
static fbe_status_t fbe_virtual_drive_verify_paged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                 fbe_packet_completion_context_t context);

/* If required verify the paged metadata */
static fbe_status_t fbe_virtual_drive_verify_paged_metadata(fbe_virtual_drive_t *virtual_drive_p,
                                                            fbe_packet_t *packet_p);

/* Copy completed successfully */
static fbe_lifecycle_status_t fbe_virtual_drive_copy_complete_cond_function(fbe_base_object_t *object_p,
                                                                            fbe_packet_t *packet_p);

/* Abort the copy operation. */
static fbe_lifecycle_status_t fbe_virtual_drive_abort_copy_cond_function(fbe_base_object_t *object_p,
                                                                         fbe_packet_t *packet_p);

/* The condition is set when any swap operation (start user copy, proactive copy, 
 * complete copy etc) has been completed by the job service.  The virtual drive
 * cleanup must be done in the monitor context.
 */
static fbe_lifecycle_status_t fbe_virtual_drive_swap_operation_complete_cond_function(fbe_base_object_t *object_p,
                                                                                      fbe_packet_t *packet_p);


/* It changes configuration mode to Mirror. */
static fbe_lifecycle_status_t fbe_virtual_drive_change_configuration_mode_cond_function(fbe_base_object_t * object_p,
                                                                                        fbe_packet_t * packet_p);

/* Checks if the copy operation is complete or not.*/
static fbe_lifecycle_status_t fbe_virtual_drive_is_copy_complete_cond_function(fbe_base_object_t * object_p,
                                                                               fbe_packet_t * packet_p);

/* It quiesce the virtual drive object's i/o operation. */
static fbe_lifecycle_status_t fbe_virtual_drive_quiesce_cond_function(fbe_base_object_t * object_p, 
                                                                      fbe_packet_t * packet_p);

/* It unquiesce the virtual drive object's i/o operation. */
static fbe_lifecycle_status_t fbe_virtual_drive_unquiesce_cond_function(fbe_base_object_t * object_p, 
                                                                        fbe_packet_t * packet_p);

/* the background operation condition for the virtual drive object (always set) */
static fbe_lifecycle_status_t fbe_virtual_drive_background_monitor_operation_cond_function(fbe_base_object_t * object_p,
                                                                      fbe_packet_t * packet_p);

/* Set the rebuild (copy) checkpoint to the end marker */
static fbe_lifecycle_status_t fbe_virtual_drive_copy_complete_set_rebuild_checkpoint_to_end_marker_cond_function(
                                fbe_base_object_t*      in_object_p,
                                fbe_packet_t*           in_packet_p);

/* Destination drive failed (copy was aborted) but source drive is still enabled. */
static fbe_lifecycle_status_t fbe_virtual_drive_dest_drive_failed_set_rebuild_checkpoint_to_end_marker_cond_function(
                                fbe_base_object_t*      in_object_p,
                                fbe_packet_t*           in_packet_p);

/* Set the virtual drive checkpoints as required  when in the failed state.*/
static fbe_lifecycle_status_t fbe_virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker_cond_function(
                                fbe_base_object_t*      in_object_p,
                                fbe_packet_t*           in_packet_p);

/* Swap in of the appropriate edge condition function. */
static fbe_lifecycle_status_t fbe_virtual_drive_swap_in_edge_cond_function(fbe_base_object_t * object_p,
                                                                            fbe_packet_t * packet_p);

/* Swap out of the appropriate edge condition function. */
static fbe_lifecycle_status_t fbe_virtual_drive_swap_out_edge_cond_function(fbe_base_object_t * object_p,
                                                                            fbe_packet_t * packet_p);

/* Need a callback for the monitor packet when we initiate a job */
static fbe_status_t fbe_virtual_drive_initiate_copy_complete_or_abort_completion(fbe_packet_t * packet_p,
                                                                                 fbe_packet_completion_context_t context);

/* It sends ask for permanent spare permission request to all of its upstream object. */
static fbe_status_t fbe_virtual_drive_ask_permanent_spare_permission(fbe_virtual_drive_t * virtual_drive,
                                                                     fbe_packet_t * packet);

/* It sends proactive spare permission request to all of its upstream object. */
static fbe_status_t fbe_virtual_drive_ask_proactive_spare_permission(fbe_virtual_drive_t * virtual_drive,
                                                                     fbe_packet_t * packet);

/* completion function for the need permanent spare condition. */
static fbe_status_t fbe_virtual_drive_need_permanent_spare_cond_completion(fbe_packet_t * packet_p,
                                                                           fbe_packet_completion_context_t context);

/* completion function for the need proactive spare condition. */
static fbe_status_t fbe_virtual_drive_need_proactive_spare_cond_completion(fbe_packet_t * packet_p,
                                                                           fbe_packet_completion_context_t context);

/* Completion function assocliated with ask for permanent spare permission event. */
static fbe_status_t fbe_virtual_drive_ask_permanent_spare_permission_completion(fbe_event_t * event_p,
                                                                                fbe_event_completion_context_t context);

/* Completion function assocliated with ask for proactive spare permission event. */
static fbe_status_t fbe_virtual_drive_ask_proactive_spare_permission_completion(fbe_event_t * event_p,
                                                                                fbe_event_completion_context_t context);

/* if needed (i.e. copy back) we may need to set NR for the new drive */
static fbe_lifecycle_status_t fbe_virtual_drive_eval_mark_nr_if_needed_cond_function(fbe_base_object_t * object_p, 
                                                                                     fbe_packet_t * packet_p);

/* used to clear the `no spares reported non-paged flag */
static fbe_lifecycle_status_t fbe_virtual_drive_clear_no_spare_cond_function(fbe_base_object_t * object_p,
                                                                             fbe_packet_t * packet_p);

/* Simply report that the copy request was denied. */
static fbe_lifecycle_status_t fbe_virtual_drive_report_copy_denied_cond_function(fbe_base_object_t * object_p,
                                                                                 fbe_packet_t * packet_p);

/* determines if we are ready to start the copy operation or not. */
static fbe_status_t fbe_virtual_drive_is_rebuild_logging(fbe_virtual_drive_t *virtual_drive_p,
                                                         fbe_bool_t *b_is_rebuild_logging_p);

/* VD zero consumed area condition zero out the metadata area before it writes paged metadata. */
static fbe_lifecycle_status_t fbe_virtual_drive_zero_paged_metadata_cond_function(fbe_base_object_t * object_p,
                                                                              fbe_packet_t * packet_p);

/* Re-zero the paged metadata after a swap */
static fbe_lifecycle_status_t fbe_virtual_drive_swapped_zero_paged_metadata_cond_function(fbe_base_object_t * object_p,
                                                                                          fbe_packet_t * packet_p);

/* Write default paged metadata after a swap */
static fbe_lifecycle_status_t fbe_virtual_drive_swapped_write_default_paged_metadata_cond_function(fbe_base_object_t * object_p,
                                                                                                   fbe_packet_t * packet_p);

/* Ask permission to swap-out after a failed copy*/
static fbe_status_t fbe_virtual_drive_copy_failed_ask_swap_out_permission(fbe_virtual_drive_t *virtual_drive_p,
                                                                          fbe_packet_t * packet_p);

/* Ask permission to swap-out after a failed copy completion. */
static fbe_status_t fbe_virtual_drive_copy_failed_ask_swap_out_permission_completion(fbe_packet_t * packet_p,
                                                                                     fbe_packet_completion_context_t context);

/* Finished writting the default paged metadata */
static fbe_status_t fbe_virtual_drive_write_default_paged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                                                   fbe_packet_completion_context_t context);

/* Extern prototypes */
extern fbe_status_t fbe_virtual_drive_copy_initialize_rebuild_context(fbe_virtual_drive_t *virtual_drive_p,
                                        fbe_raid_group_rebuild_context_t *in_rebuild_context_p, 
                                        fbe_raid_position_bitmask_t       in_positions_to_be_rebuilt,
                                        fbe_lba_t                         in_rebuild_lba);
/* override background monitor condition function */
static fbe_lifecycle_status_t fbe_virtual_drive_raid_group_background_monitor_operation_cond_function(fbe_base_object_t * object_p,
                                                                                           fbe_packet_t * packet_p);
/* Eval download request in background monitor operation */
static void fbe_virtual_drive_eval_download_request(fbe_virtual_drive_t *virtual_drive_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_virtual_drive_swap_out_check_encryption_state(fbe_virtual_drive_t *virtual_drive_p);
static fbe_status_t fbe_virtual_drive_swap_in_check_encryption_state(fbe_virtual_drive_t *virtual_drive_p);


/*--- lifecycle callback functions -----------------------------------------------------*/

FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(virtual_drive);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(virtual_drive);

/*  virtual_drive_lifecycle_callbacks
 *  This variable has all the callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(virtual_drive) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        virtual_drive,
        FBE_LIFECYCLE_NULL_FUNC,          /* online function */
        FBE_LIFECYCLE_NULL_FUNC)          /* pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/

static fbe_lifecycle_const_cond_t virtual_drive_downstream_health_disabled_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED,
        fbe_virtual_drive_downstream_health_disabled_cond_function)
};


static fbe_lifecycle_const_cond_t virtual_drive_downstream_health_broken_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN,
        fbe_virtual_drive_downstream_health_broken_cond_function)
};

static fbe_lifecycle_const_cond_t virtual_drive_downstream_health_not_optimal_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_NOT_OPTIMAL,
        fbe_virtual_drive_downstream_health_not_optimal_cond_function)
};

static fbe_lifecycle_const_cond_t virtual_drive_establish_edge_states_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_ESTABLISH_EDGE_STATES,
        fbe_virtual_drive_establish_edge_states_cond_function)
};


/* raid group metadata metadata element init condition function. */
static fbe_lifecycle_const_cond_t virtual_drive_swapped_zero_paged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_ZERO_CONSUMED_AREA,
        fbe_virtual_drive_swapped_zero_paged_metadata_cond_function)
};

/* raid group metadata metadata element init condition function. */
static fbe_lifecycle_const_cond_t virtual_drive_swapped_write_default_paged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_PAGED_METADATA,
        fbe_virtual_drive_swapped_write_default_paged_metadata_cond_function)
};


/* VD zero consumed space condition function. */
static fbe_lifecycle_const_cond_t virtual_drive_zero_paged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_ZERO_CONSUMED_AREA,
        fbe_virtual_drive_zero_paged_metadata_cond_function)
};

/* raid group common background monitor operation condition function. */
static fbe_lifecycle_const_cond_t virtual_drive_raid_group_background_monitor_operation_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_BACKGROUND_MONITOR_OPERATION,
        fbe_virtual_drive_raid_group_background_monitor_operation_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/

static fbe_lifecycle_const_base_timer_cond_t virtual_drive_need_replacement_drive_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "virtual_drive_need_replacement_drive_cond",
            FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE,
            fbe_virtual_drive_need_replacement_drive_cond_function),
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
    1000 /* fires every 10 seconds */
};

static fbe_lifecycle_const_base_cond_t virtual_drive_evaluate_downstream_health_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_evaluate_downstream_health_cond",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_EVALUATE_DOWNSTREAM_HEALTH,
        fbe_virtual_drive_evaluate_downstream_health_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SETUP_FOR_VERIFY_PAGED_METADATA -
 *
 * The purpose of this condition is to setup for verification of the paged metadata.
 */
static fbe_lifecycle_const_base_cond_t virtual_drive_setup_for_verify_paged_metadata_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_setup_for_verify_paged_metadata",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SETUP_FOR_VERIFY_PAGED_METADATA,
        fbe_virtual_drive_setup_for_verify_paged_metadata_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_VERIFY_PAGED_METADATA -
 *
 * The purpose of this condition is to verify the paged metadata.
 * The condition function is implemented in the derived class and not in this base class. 
 */
static fbe_lifecycle_const_base_cond_t virtual_drive_verify_paged_metadata_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_verify_paged_metadata",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_VERIFY_PAGED_METADATA,
        fbe_virtual_drive_verify_paged_metadata_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t virtual_drive_need_proactive_spare_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_need_proactive_spare_cond",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_PROACTIVE_SPARE,
        fbe_virtual_drive_need_proactive_spare_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t virtual_drive_start_user_copy_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_start_user_copy_cond",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_START_USER_COPY,
        fbe_virtual_drive_start_user_copy_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t virtual_drive_swap_in_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_swap_in_edge_cond",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SWAP_IN_EDGE,
        fbe_virtual_drive_swap_in_edge_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t virtual_drive_swap_out_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_swap_out_edge_cond",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SWAP_OUT_EDGE,
        fbe_virtual_drive_swap_out_edge_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t virtual_drive_copy_complete_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_copy_complete_cond",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_COPY_COMPLETE,
        fbe_virtual_drive_copy_complete_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t virtual_drive_abort_copy_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_abort_copy_cond",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_ABORT_COPY,
        fbe_virtual_drive_abort_copy_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t virtual_drive_swap_operation_complete_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_swap_operation_complete_cond",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SWAP_OPERATION_COMPLETE,
        fbe_virtual_drive_swap_operation_complete_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker_cond",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_COPY_FAILED_SET_REBUILD_CHECKPOINT_TO_END_MARKER,
        fbe_virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t virtual_drive_eval_mark_nr_if_needed_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_eval_mark_nr_if_needed_cond",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_EVAL_MARK_NR_IF_NEEDED_COND,
        fbe_virtual_drive_eval_mark_nr_if_needed_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t virtual_drive_clear_no_spare_reported_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_clear_no_spare_reported_cond",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_CLEAR_NO_SPARE_REPORTED,
        fbe_virtual_drive_clear_no_spare_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t virtual_drive_report_copy_denied_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_report_copy_denied_cond",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_REPORT_COPY_DENIED,
        fbe_virtual_drive_report_copy_denied_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/*! @note virtual drive common background monitor operation condition function. 
 *        This condition CANNOT be derived since the base raid group class
 *        will always run prior to any of the virtual drive conditions and thus
 *        no other virtual drive conditions will run because this condition
 *        returns with `done'.
 */
static fbe_lifecycle_const_base_cond_t virtual_drive_background_monitor_operation_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_background_monitor_operation_cond",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_BACKGROUND_MONITOR_OPERATION,
        fbe_virtual_drive_background_monitor_operation_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,       /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,          /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,      /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,        /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,           /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t virtual_drive_is_copy_complete_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_is_copy_complete_cond",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_IS_COPY_COMPLETE,
        fbe_virtual_drive_is_copy_complete_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t virtual_drive_fail_check_downstream_health_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "virtual_drive_fail_check_downstream_health_cond",
        FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_FAIL_CHECK_DOWNSTREAM_HEALTH,
        fbe_virtual_drive_fail_check_downstream_health_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


/*!*************************************************** 
 * @note FBE_RAID_GROUP_LIFECYCLE_COND over-rides
 *
 * ==> Start:
 ******************************************************/

static fbe_lifecycle_const_cond_t virtual_drive_quiesce_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE,
        fbe_virtual_drive_quiesce_cond_function)
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_SET_REBUILD_CHECKPOINT_TO_END_MARKER -
 * 
 * The purpose of this condition is move the rebuild checkpoint for a disk(s) to the logical end marker.  I/O
 * needs to be quiesced when doing so, which is why this is in a condition. 
 * 
 */
static fbe_lifecycle_const_base_cond_t virtual_drive_copy_complete_set_rebuild_checkpoint_to_end_marker_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_RAID_GROUP_LIFECYCLE_COND_SET_REBUILD_CHECKPOINT_TO_END_MARKER,
        fbe_virtual_drive_copy_complete_set_rebuild_checkpoint_to_end_marker_cond_function)
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_DEST_DRIVE_FAILED_SET_REBUILD_CHECKPOINT_TO_END_MARKER -
 * 
 * The purpose of this condition is move the rebuild checkpoint for a disk(s) to the logical end marker.  I/O
 * needs to be quiesced when doing so, which is why this is in a condition. 
 * 
 */
static fbe_lifecycle_const_base_cond_t virtual_drive_dest_drive_failed_set_rebuild_checkpoint_to_end_marker_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_RAID_GROUP_LIFECYCLE_COND_DEST_DRIVE_FAILED_SET_REBUILD_CHECKPOINT_TO_END_MARKER,
        fbe_virtual_drive_dest_drive_failed_set_rebuild_checkpoint_to_end_marker_cond_function)
};

/* FBE_RAID_GROUP_LIFECYCLE_COND_CONFIGURATION_CHANGE -
 * 
 * The purpose of this condition is to take the necessary action when the virtual drive
 * changes configuration mode.  For instance changing to or from the mirror configuration.
 * 
 */
static fbe_lifecycle_const_base_cond_t virtual_drive_change_configuration_mode_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_RAID_GROUP_LIFECYCLE_COND_CONFIGURATION_CHANGE,
        fbe_virtual_drive_change_configuration_mode_cond_function)
};

static fbe_lifecycle_const_cond_t virtual_drive_unquiesce_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE,
        fbe_virtual_drive_unquiesce_cond_function)
};

/*!*************************************************** 
 * @note FBE_RAID_GROUP_LIFECYCLE_COND over-rides
 *
 * <== End:
 ******************************************************/


/* virtual_drive_lifecycle_base_cond_array
 * This is our static list of base conditions.
 */
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(virtual_drive)[] = {
    (fbe_lifecycle_const_base_cond_t *)&virtual_drive_need_replacement_drive_cond,
    &virtual_drive_evaluate_downstream_health_cond,
    &virtual_drive_setup_for_verify_paged_metadata_cond,
    &virtual_drive_verify_paged_metadata_cond,
    &virtual_drive_need_proactive_spare_cond,
    &virtual_drive_start_user_copy_cond,
    &virtual_drive_swap_in_edge_cond,
    &virtual_drive_swap_out_edge_cond,
    &virtual_drive_copy_complete_cond,
    &virtual_drive_abort_copy_cond,
    &virtual_drive_swap_operation_complete_cond,
    &virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker_cond,
    &virtual_drive_eval_mark_nr_if_needed_cond,
    &virtual_drive_background_monitor_operation_cond,
    &virtual_drive_is_copy_complete_cond,    
    &virtual_drive_fail_check_downstream_health_cond,
    &virtual_drive_clear_no_spare_reported_cond,
    &virtual_drive_report_copy_denied_cond,
};

/* virtual_drive_lifecycle_base_conditions 
 *  This is the list of base default base conditions for the logical
 *  drive.
 */
FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(virtual_drive);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t virtual_drive_specialize_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_downstream_health_not_optimal_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_establish_edge_states_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_zero_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /* Base conditions */
};

static fbe_lifecycle_const_rotary_cond_t virtual_drive_activate_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_downstream_health_disabled_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_swapped_zero_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_swapped_write_default_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_setup_for_verify_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_verify_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t virtual_drive_ready_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_raid_group_background_monitor_operation_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_swap_in_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_quiesce_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /*!@note All the condition which needs quiesce operation in virtual drive object needs to be added below. */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_change_configuration_mode_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_copy_complete_set_rebuild_checkpoint_to_end_marker_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_dest_drive_failed_set_rebuild_checkpoint_to_end_marker_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /*!@note All the condition which needs quiesce operation in virtual drive object needs to be added above. */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_unquiesce_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_evaluate_downstream_health_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_need_proactive_spare_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_start_user_copy_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_clear_no_spare_reported_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_report_copy_denied_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_copy_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_abort_copy_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_swap_out_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_eval_mark_nr_if_needed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_is_copy_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_swap_operation_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_need_replacement_drive_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* This is always the last condition in the ready rotary.  This condition is never cleared. 
     * Please, do put any condition after this one !!
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_background_monitor_operation_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    
};

static fbe_lifecycle_const_rotary_cond_t virtual_drive_fail_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_downstream_health_broken_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_quiesce_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /*!@note All the condition which needs quiesce operation in virtual drive object needs to be added below. */

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_change_configuration_mode_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /*!@note All the condition which needs quiesce operation in virtual drive object needs to be added above. */
    //FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_unquiesce_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_start_user_copy_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_abort_copy_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_swap_out_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_swap_in_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_copy_complete_set_rebuild_checkpoint_to_end_marker_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_is_copy_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_swap_operation_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_need_replacement_drive_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(virtual_drive_fail_check_downstream_health_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(virtual_drive)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, virtual_drive_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, virtual_drive_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_FAIL, virtual_drive_fail_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, virtual_drive_ready_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(virtual_drive);

/*--- global virtual_drive lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(virtual_drive) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        virtual_drive,
        FBE_CLASS_ID_VIRTUAL_DRIVE,                   /* This class */
        FBE_LIFECYCLE_CONST_DATA(mirror))  /* The super class */
};
/*--- Local Functions ------------------------------------------------------------------*/

/*--- Condition Functions --------------------------------------------------------------*/

/*!****************************************************************************
 * fbe_virtual_drive_downstream_health_broken_cond_function()
 ******************************************************************************
 * @brief
 *  This is the derived condition function for virtual drive 
 *  object where it will clear this condition when it finds tries
 *  to detach all the edges with downstream object which are in 
 *  GONE state.
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is be based on parent
 *                                  function call.
 *
 * @note    This condition is typically a `one shot' (i.e. it is cleared after
 *          the first occurance).
 *
 * @author
 *  05/21/2009 - Created. Dhaval Patel.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_virtual_drive_downstream_health_broken_cond_function(fbe_base_object_t * object_p,
                                                         fbe_packet_t * packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_scheduler_hook_status_t                 hook_status;
    fbe_virtual_drive_t                        *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_base_config_downstream_health_state_t   downstream_health_state;
    fbe_lifecycle_state_t                       my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_bool_t                                  b_is_active = FBE_TRUE;
    fbe_lifecycle_state_t                       peer_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_virtual_drive_flags_t                   flags;
    fbe_virtual_drive_configuration_mode_t      configuration_mode;
    fbe_bool_t                                  b_is_mirror_mode = FBE_FALSE;
    fbe_bool_t                                  b_is_source_failed_set = FBE_FALSE;
    fbe_bool_t                                  b_is_source_edge_broken = FBE_FALSE;

    fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Wait for peer to see broken*/
    fbe_virtual_drive_lock(virtual_drive_p);
    if(!fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)virtual_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN))
    {
        fbe_base_config_set_clustered_flag((fbe_base_config_t*)virtual_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN);
        fbe_virtual_drive_unlock(virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    fbe_virtual_drive_unlock(virtual_drive_p);

    /* Passive side should not come out of FAILED state if the active side is still
     * in FAILED state. This is similar to PVD.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    b_is_mirror_mode = fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p);
    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)virtual_drive_p);
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t*)virtual_drive_p, &peer_state);

    /* Get the lifecycle state and downstream health.
     */
    downstream_health_state = fbe_virtual_drive_verify_downstream_health(virtual_drive_p);
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);

    /* Set the `need replacement drive' start timer if not already set.
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE))
    {
        /* Set the need replacement start time.
         */
        fbe_virtual_drive_swap_set_need_replacement_drive_start_time(virtual_drive_p);

        /*! @note There is at least one issue where timer conditions need to be 
         *        set (even if it is a preset).  Set the condition now.
         */
        fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                               (fbe_base_object_t *)virtual_drive_p,
                               FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE);
    }

    /* Since we may have a debug hook set, only trace if flag is enabled.
     */
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_HEALTH_TRACING,
                            "virtual_drive: health broken mode: %d life: %d peer: %d active: %d flags: 0x%x health: %d \n",
                            configuration_mode, my_state, peer_state, b_is_active, flags, downstream_health_state);

    /* Clear any conditions remaining from the transition from Ready to Fail.
     */
    fbe_lifecycle_force_clear_cond(&fbe_virtual_drive_lifecycle_const, 
                                   (fbe_base_object_t*)virtual_drive_p,
                                   FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING);

    /* If the `failed' debug hook is set wait until it is cleared.
     */
    fbe_virtual_drive_check_hook(virtual_drive_p,
                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED, 
                              FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED, 
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Since we have cleared this condition we can trace an information message.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: health broken mode: %d life: %d peer: %d active: %d flags: 0x%x health: %d \n",
                          configuration_mode, my_state, peer_state, b_is_active, flags, downstream_health_state);

    /* Determine if we need to set the `source failed' non-paged flag.
     */
    fbe_virtual_drive_metadata_is_source_failed(virtual_drive_p, &b_is_source_failed_set); 
    b_is_source_edge_broken = fbe_virtual_drive_swap_is_primary_edge_broken(virtual_drive_p);  
    if ((b_is_active == FBE_TRUE)             &&
        (b_is_mirror_mode == FBE_TRUE)        &&
        (b_is_source_edge_broken == FBE_TRUE) &&
        (b_is_source_failed_set == FBE_FALSE)    )
    {
        /* Set the `source failed' flag in the non-paged metadata.
         */
        status = fbe_virtual_drive_metadata_set_source_failed(virtual_drive_p, packet_p);
        if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
        {
            /* Trace the error and clear this condition
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s set source failed - status: 0x%x\n",
                                  __FUNCTION__, status);

            /* clear this condition to allow to run other condition in rotary. */
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)virtual_drive_p);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s can't clear current condition, status: 0x%X\n",
                                        __FUNCTION__, status);
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            /* Return `done'
             */
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        else
        {
            /* Else the lifecycle status is `pending'
             */
            return FBE_LIFECYCLE_STATUS_PENDING;
        }

    } /* end if we need to set the `source failed' flag in the non-paged */

    /* clear this condition to allow to run other condition in rotary. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)virtual_drive_p);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    /* Always return done*/
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_virtual_drive_downstream_health_broken_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_fail_check_downstream_health_cond_function()
 ******************************************************************************
 * @brief
 *  This condition function is used to hold the virtual drive object in fail
 *  state until it finds downstream health state gets changed to allow the 
 *  virtual drive object to transition to activate state.
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - lifecycle status.
 *
 * @note    This condition is never cleared since we always want to check if it
 *          is possible to exit the failed state.
 *
 * @author
 *  05/21/2009 - Created. Dhaval Patel.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_fail_check_downstream_health_cond_function(fbe_base_object_t *object_p,
                                                                                           fbe_packet_t *packet_p)
{
    fbe_virtual_drive_t                        *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_base_config_downstream_health_state_t   downstream_health_state;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_scheduler_hook_status_t                 hook_status;
    fbe_bool_t                                  b_is_active = FBE_TRUE;
    fbe_lifecycle_state_t                       my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_lifecycle_state_t                       peer_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_virtual_drive_flags_t                   flags;
    fbe_virtual_drive_configuration_mode_t      configuration_mode;
    fbe_lifecycle_cond_id_t                     cond_id = 0;
    fbe_bool_t                                  b_is_mirror_mode = FBE_FALSE;
    fbe_bool_t                                  b_is_source_failed_set = FBE_FALSE;
    fbe_bool_t                                  b_is_source_edge_broken = FBE_FALSE;

    fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Passive side should not come out of FAILED state if the active side is still
     * in FAILED state. This is similar to PVD.
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)virtual_drive_p);
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t*)virtual_drive_p, &peer_state);

    /* Get the downstream health.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    b_is_mirror_mode = fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p);
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    downstream_health_state = fbe_virtual_drive_verify_downstream_health(virtual_drive_p);

    /* Since we may have a debug hook set, only trace if flag is enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_HEALTH_TRACING,
                            "virtual_drive: fail check health mode: %d life: %d peer: %d act: %d flags: 0x%x health: %d \n",
                            configuration_mode, my_state, peer_state, b_is_active, flags, downstream_health_state);

    /* Job is still outstanding.  To prevent race between removal of edge and 
     * insertion of edge, we wait for the job to complete.
     */ 
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE)                                      ||
        fbe_raid_group_is_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS)      ||
        fbe_raid_group_is_peer_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS) ||
        fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS)                                       )
    {
        /* We also don't want to take action until the job finishes because the 
         * configuration may be in the process of committing the edge changes. 
         */
        fbe_virtual_drive_trace(virtual_drive_p, 
                                FBE_TRACE_LEVEL_INFO, 
                                FBE_VIRTUAL_DRIVE_DEBUG_FLAG_HEALTH_TRACING,
                                "virtual_drive: fail check health active: %d mode: %d health: %d flags: 0x%x job outstanding\n",
                                b_is_active, configuration_mode, downstream_health_state, flags);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If the `hold in fail' flag is set we don't want to exit the failed state.
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_HOLD_IN_FAIL))
    {
        /* Wait until the swap-out is complete.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Check if we need to clear the source drive failed flag.
     */
    fbe_virtual_drive_metadata_is_source_failed(virtual_drive_p, &b_is_source_failed_set); 
    b_is_source_edge_broken = fbe_virtual_drive_swap_is_primary_edge_broken(virtual_drive_p);
    if ((b_is_active == FBE_TRUE)              &&
        (b_is_mirror_mode == FBE_TRUE)         &&
        (b_is_source_edge_broken == FBE_FALSE) &&
        (b_is_source_failed_set == FBE_TRUE)      )
    {
        /* Clear the `source failed' flag in the non-paged metadata.
         */
        status = fbe_virtual_drive_metadata_clear_source_failed(virtual_drive_p, packet_p);
        if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
        {
            /* Trace the error and clear this condition
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s clear source failed - status: 0x%x\n",
                                  __FUNCTION__, status);

            /* clear this condition to allow to run other condition in rotary. */
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)virtual_drive_p);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s can't clear current condition, status: 0x%X\n",
                                        __FUNCTION__, status);
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            /* Return `done'
             */
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        else
        {
            /* Else the lifecycle status is `pending'
             */
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
        
        /* end if we need to clear source drive failed */
    }

    /* Look for all the edges with downstream objects and verify its current
     * downstream health.
     */
    switch (downstream_health_state)
    {
        case FBE_DOWNSTREAM_HEALTH_OPTIMAL:
        case FBE_DOWNSTREAM_HEALTH_DEGRADED:
        case FBE_DOWNSTREAM_HEALTH_DISABLED:
            /* If the `clear failed' debug hook is set wait until it is cleared.
             */
            fbe_virtual_drive_check_hook(virtual_drive_p,
                                      SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                      FBE_VIRTUAL_DRIVE_SUBSTATE_EXIT_FAILED_STATE, 
                                      0, &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE) 
            {
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            /* Passive side should not come out of FAILED state if the active side is still
             * in FAILED state. This is similar to PVD.
             */
            if (!b_is_active)
            {
                if (peer_state == FBE_LIFECYCLE_STATE_FAIL)
                {
                    /* not clearing current condition so that we will revisit after 3 second.
                     * if peer goes away, this side can then take the lead.
                     */
                    return FBE_LIFECYCLE_STATUS_DONE;
                }
            
            }

            /* If it had failed to attach edge than some conditions in specialize
             * rotary were skipped. Transition to specialize.
             */
            if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_ATTACH_EDGE_TIMEDOUT))
            {
                fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_ATTACH_EDGE_TIMEDOUT);

                fbe_base_config_set_flag((fbe_base_config_t *) virtual_drive_p, FBE_BASE_CONFIG_FLAG_RESPECIALIZE);
                
                /* set specialize condition. */
                status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                                (fbe_base_object_t *)virtual_drive_p,
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_SPECIALIZE);
                cond_id = FBE_BASE_OBJECT_LIFECYCLE_COND_SPECIALIZE;
            }
            else
            {
                /* set activate condition. */
                status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                                (fbe_base_object_t *)virtual_drive_p,
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
                cond_id = FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE;
            }

            /* If setting the condition failed stay in the failed state and trace an error
             */
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*) virtual_drive_p, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s set condition failed, status: 0x%X\n", 
                                      __FUNCTION__, status);
                return FBE_LIFECYCLE_STATUS_DONE;
            }
          
            /* If we find that one of the edge with downstream object has changed its current
             * state to ENABLED/DISABLED then clear the health verify condition and set the 
             * activate condition.
             */
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*) virtual_drive_p);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t*) virtual_drive_p, 
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s can't clear current condition, status: 0x%X\n",
                                        __FUNCTION__, status);
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            /* If we are exiting the failed state and not in mirror mode
             * generate the swap notification.
             */
            if (b_is_mirror_mode == FBE_FALSE)
            {
                fbe_virtual_drive_check_and_send_notification(virtual_drive_p);
            }
            break;

        case FBE_DOWNSTREAM_HEALTH_BROKEN: 
            /* Stay in broken state.
             */      
            break;

        default:
            fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Invalid downstream health state: 0x%x!!\n", 
                                  __FUNCTION__,
                                  downstream_health_state);
            return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If we have set any condition trace the information.
     */
    if (cond_id != 0)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: fail check health mode: %d life: %d peer: %d act: %d flags: 0x%x health: %d cond: 0x%x \n",
                              configuration_mode, my_state, peer_state, b_is_active, flags, downstream_health_state, cond_id);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_virtual_drive_fail_check_downstream_health_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_downstream_health_disabled_cond_function()
 ******************************************************************************
 * @brief
 *  This is the derived condition function for virtual drive 
 *  object where it will hold the object in activate state until
 *  it finds enough edge with downstream objects.
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is be based on parent
 *                                  function call.
 *
 * @author
 *  10/16/2009 - Created. Dhaval Patel.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_virtual_drive_downstream_health_disabled_cond_function(fbe_base_object_t * object_p,
                                                           fbe_packet_t * packet_p)
{
    fbe_virtual_drive_t *                       virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_virtual_drive_configuration_mode_t      configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
    fbe_base_config_downstream_health_state_t   downstream_health_state;
    fbe_status_t                                status = FBE_STATUS_OK;    
    fbe_scheduler_hook_status_t                 hook_status;

    fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Store the `original' provision_drive object id so that when a swap 
     * occurs we remember it.
     */
    fbe_virtual_drive_swap_save_original_pvd_object_id(virtual_drive_p);

    /* Look for all the edges with downstream objects and verify its current 
     * downstream health. 
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    downstream_health_state = fbe_virtual_drive_verify_downstream_health(virtual_drive_p);

    /* If there is a job request in progress transition to broken.  The Failed
     * rotary will handle the job completion.
     */
    if (fbe_raid_group_is_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS)      ||
        fbe_raid_group_is_peer_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS) ||
        fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS)                                       )
    {
        /* This condition can occur when a virtual drive edge transitions thru
         * disabled.  We enter Activate to quiesce I/O but we need to complete
         * the job before continuing (we don't want to re-iniatialize the paged
         * metadata etc).
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: downstream health disabled cond mode: %d health: %d job in-progress go broken\n",
                              configuration_mode, downstream_health_state);

        /* If the `activate - job inprogress' hook is set stop here.
         */
        fbe_virtual_drive_check_hook(virtual_drive_p,
                                     SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE, 
                                     FBE_VIRTUAL_DRIVE_SUBSTATE_JOB_IN_PROGRESS, 
                                     0, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) 
        {
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* Transition to Failed and return done.
         */
        fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                               (fbe_base_object_t *)virtual_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
        
        /* Return done
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Set the `degraded needs rebuild' path attribute on the upstream edge if
     * the virtual drive is in pass-thru mode and not fully rebuilt.
     */
    status = fbe_virtual_drive_swap_set_degraded_needs_rebuild_if_needed(virtual_drive_p,
                                                                         packet_p,
                                                                         downstream_health_state);
    if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        /* Updating non-paged return pending
         */
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* Now set any conditions based on downstream health.
     */
    switch (downstream_health_state)
    {
        case FBE_DOWNSTREAM_HEALTH_OPTIMAL:
        case FBE_DOWNSTREAM_HEALTH_DEGRADED:
            /* If the `activate - enabled' hook is set stop here.
             */
            fbe_virtual_drive_check_hook(virtual_drive_p,
                                      SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE, 
                                      FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY, 
                                      0, &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE) 
            {
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            /* Clear the condition if we are in good state. 
             */
            fbe_lifecycle_clear_current_cond((fbe_base_object_t*)virtual_drive_p);

            /* Set the specific lifecycle condition based on health of the 
             * path with downstream objects. 
             */
            status = fbe_virtual_drive_set_condition_based_on_downstream_health(virtual_drive_p, downstream_health_state, __FUNCTION__);
            break;

        case FBE_DOWNSTREAM_HEALTH_BROKEN:
            /* If the `activate - broken' hook is set stop here.
             */
            fbe_virtual_drive_check_hook(virtual_drive_p,
                                      SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE, 
                                      FBE_VIRTUAL_DRIVE_SUBSTATE_BROKEN, 
                                      0, &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE) 
            {
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            /* set downstream health broken condition.
             * There are races during the setting of conditions in event context. 
             * Thus we could get here and have a broken path state. 
             */
            fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                   (fbe_base_object_t *)virtual_drive_p,
                                   FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
            break;

        default:
            break;
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_virtual_drive_downstream_health_disabled_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swapped_zero_paged_metadata_cond_function()
 ******************************************************************************
 * @brief
 *  This is the derived condition function for virtual drive 
 *  object where it will zero the paged metadata for VD if it's A 
 *  permanent spare.The edge swapped bit will indicate it is a permanent spare 
 *  or a proactive spare. For proactive spare we should not write out 
 *  the paged MD. The proactive copy will take care of that.
 *  Mirror condfiguration mode indicates it is a proactive spare
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t 
 *
 * @author
 *  12/07/2011  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_swapped_zero_paged_metadata_cond_function(fbe_base_object_t * object_p,
                                                                                          fbe_packet_t * packet_p)
{
    fbe_virtual_drive_t                    *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_bool_t                              b_is_edge_swapped;
    fbe_bool_t                              b_is_source_failed;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_virtual_drive_configuration_mode_t  new_configuration_mode;
    fbe_bool_t                              is_active;

    fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    is_active = fbe_base_config_is_active((fbe_base_config_t*)virtual_drive_p);
    if(!is_active)
    {
        fbe_lifecycle_clear_current_cond(object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Get the information to determine if we need to re-zero the paged metadata.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_get_new_configuration_mode(virtual_drive_p, &new_configuration_mode);
    fbe_virtual_drive_metadata_is_edge_swapped(virtual_drive_p, &b_is_edge_swapped);
    fbe_virtual_drive_metadata_is_source_failed(virtual_drive_p, &b_is_source_failed);
    
    /* If a permanent spare swapped in or the source drive failed zero the 
     * paged metadata.
     */
    if (!fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p) &&
         ((b_is_edge_swapped == FBE_TRUE)  ||
          (b_is_source_failed == FBE_TRUE)    )                                  )
    {
        /* First validate that a copy command wasn't issued while not in the
         * Ready state.
         */
        if (new_configuration_mode != FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN)
        {
            /* Trace an error and clear this condition.
             */
            fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s mode: %d swapped: %d source failed: %d new mode: %d unexpected\n",
                                  __FUNCTION__, configuration_mode, b_is_edge_swapped, b_is_source_failed, new_configuration_mode);

            /* Clear this condition and let the Ready state handle it.
             */
            fbe_lifecycle_clear_current_cond(object_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* Trace some information.
         */
        fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: re-zero paged metadata mode: %d swapped: %d source failed: %d\n",
                              configuration_mode, b_is_edge_swapped, b_is_source_failed);

        /*! @note We must zero the paged metadata when a drive is swapped in
         *        and we are in pass-thru mode since the paged metadata must
         *        be properly set for the virtual drive object's use.
         */
        fbe_virtual_drive_zero_paged_metadata_cond_function(object_p, packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* Clear the condition.
     */
    fbe_lifecycle_clear_current_cond(object_p);

    return FBE_LIFECYCLE_STATUS_DONE;
    
}
/******************************************************************************
 * end fbe_virtual_drive_swapped_zero_paged_metadata_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_write_default_paged_metadata_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to write the default paged metadata for the virtual
 *  drive object.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  10/12/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_virtual_drive_write_default_paged_metadata_cond_function(fbe_base_object_t *object_p,
                                                                                    fbe_packet_t *packet_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_virtual_drive_t    *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_raid_group_t       *raid_group_p = NULL;
    fbe_raid_geometry_t    *raid_geometry_p = NULL;;
    
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Get the raid geometry.
     */
    raid_group_p = (fbe_raid_group_t *)virtual_drive_p;
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    /* Validate that the object does have paged metadata.
     */
    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Set the completion function after BOTH the non-paged and paged metadata 
     * have been set to the default values.
     */
    fbe_transport_set_completion_function(packet_p, fbe_virtual_drive_write_default_paged_metadata_cond_completion, virtual_drive_p);

    /* initialize signature for the raid group object. */
    status = fbe_virtual_drive_metadata_write_default_nonpaged_metadata(virtual_drive_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_virtual_drive_paged_metadata_write_default_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_write_default_paged_metadata_cond_completion()
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
 *  10/12/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_write_default_paged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                            fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_virtual_drive_t                *virtual_drive_p = NULL;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_bool_t                          b_is_source_failed = FBE_FALSE;
    
    /* Get the virtual drive object and determine if the `source failed' 
     * flag is set.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)context;
    fbe_virtual_drive_metadata_is_source_failed(virtual_drive_p, &b_is_source_failed);

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if ((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* clear paged metadata write default condition if status is good. */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);

        /* If the source failed flag is set we need to clear it now.
         */
        if (b_is_source_failed == FBE_TRUE)
        {
            status = fbe_virtual_drive_metadata_clear_source_failed(virtual_drive_p, packet_p);
            if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
            {
                /* Trace the error
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s set source failed - status: 0x%x\n",
                                      __FUNCTION__, status);
            }

            /* Return status
             */
            return status;
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: write default comp failed - status: 0x%x control status: 0x%x\n",
                              status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_write_default_paged_metadata_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swapped_write_default_paged_metadata_cond_function()
 ******************************************************************************
 * @brief
 *  This is the derived condition function for virtual drive 
 *  object where it will initialize the paged metadata for VD if it's A 
 *  permanent spare.The edge swapped bit will indicate it is a permanent spare 
 *  or a proactive spare. For proactive spare we should not write out 
 *  the paged MD. The proactive copy will take care of that.
 *  Mirror condfiguration mode indicates it is a proactive spare
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t 
 *
 * @author
 *  11/11/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_virtual_drive_swapped_write_default_paged_metadata_cond_function(fbe_base_object_t * object_p,
                                                                     fbe_packet_t * packet_p)
{
    fbe_virtual_drive_t    *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_bool_t              b_is_edge_swapped = FBE_FALSE;
    fbe_bool_t              b_is_source_failed = FBE_FALSE;
    fbe_bool_t              is_active;

    fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    is_active = fbe_base_config_is_active((fbe_base_config_t*)virtual_drive_p);
    if(!is_active)
    {
        fbe_lifecycle_clear_current_cond(object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Get the status of the `edge swapped' and `source failed' metadata bits.
     */
    fbe_virtual_drive_metadata_is_edge_swapped(virtual_drive_p, &b_is_edge_swapped);
    fbe_virtual_drive_metadata_is_source_failed(virtual_drive_p, &b_is_source_failed);
    
    /* If we are not in mirror mode and either the `edge swapped' or `source failed'
     * flag is set, then we need to set/reset the paged metadata to the default values.
     */
    if (!fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p) &&
         ((b_is_edge_swapped == FBE_TRUE)  ||
          (b_is_source_failed == FBE_TRUE)    )                                 )
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: swapped write default paged - swapped: %d source failed: %d \n",
                              b_is_edge_swapped, b_is_source_failed);

        fbe_virtual_drive_write_default_paged_metadata_cond_function(object_p, packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    else
    {
        fbe_lifecycle_clear_current_cond(object_p);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
    
}
/******************************************************************************
 * end fbe_virtual_drive_swapped_write_default_paged_metadata_cond_function()
 ******************************************************************************/

/*!**************************************************************
 * fbe_virtual_drive_setup_for_verify_paged_metadata_cond_function()
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
static fbe_lifecycle_status_t fbe_virtual_drive_setup_for_verify_paged_metadata_cond_function(fbe_base_object_t *object_p, 
                                                                                              fbe_packet_t *packet_p)
{
    fbe_status_t                            status;
    fbe_virtual_drive_t                    *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_bool_t                              b_is_active;
    fbe_bool_t                              b_is_mirror_mode;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_chunk_count_t                       chunk_count;
    fbe_u32_t                               chunk_index;
    fbe_raid_group_paged_metadata_t        *paged_md_md_p;
    fbe_raid_group_nonpaged_metadata_t     *nonpaged_metadata_p = NULL;
    fbe_lba_t                               paged_md_start_lba = FBE_LBA_INVALID;
    fbe_raid_geometry_t                    *raid_geometry_p = NULL;
    fbe_u16_t                               data_disks = 0;
      
    /* Determine if we are active or not.
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)virtual_drive_p);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    b_is_mirror_mode = fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p);

    /* Debug trace the function entry.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s mode: %d active: %d\n", 
                          __FUNCTION__, configuration_mode, b_is_active);

    /* Check if the RG is active on this SP or not.  If not, then we don't 
     * need to do anything more - return here and the monitor will reschedule 
     * on its normal cycle.  If the virtual drive is not in mirror mode 
     * (typical case), clear this condition and the `issue' metadata verify 
     * condition.
     */
    if ((b_is_active == FBE_FALSE)      ||
        (b_is_mirror_mode == FBE_FALSE)    )
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }

        /* Clear any conditions remaining
         */
        fbe_lifecycle_force_clear_cond(&fbe_virtual_drive_lifecycle_const, 
                                       (fbe_base_object_t*)virtual_drive_p,
                                       FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_VERIFY_PAGED_METADATA);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Turn on the "read-write verify" bit in the nonpaged metadata of metadata.
     * This will allow us to kick off a verify of the paged metadata when that 
     * condition runs.
     */

    /*  First determine how many chunks of paged metadata we have for this RG.
     */
    fbe_raid_group_bitmap_get_md_of_md_count((fbe_raid_group_t *)virtual_drive_p, &chunk_count);

    /*  Get a pointer to the paged metadata of metadata.
     */
    paged_md_md_p = fbe_raid_group_get_paged_metadata_metadata_ptr((fbe_raid_group_t *)virtual_drive_p);

    /*  Set the paged metadata of metadata verify bits in memory only.
     */
    for (chunk_index = 0; chunk_index < chunk_count; chunk_index++)
    {
        paged_md_md_p[chunk_index].verify_bits |= FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE;
    }

    // Get the paged metadata start lba.
    fbe_base_config_metadata_get_paged_record_start_lba(&virtual_drive_p->spare.raid_group.base_config,
                                                        &paged_md_start_lba);

    // Get the number of data disks in this raid object
    raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t *)virtual_drive_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);    

    /* Set the verify checkpoint to start of paged metadata. 
     * We do not persist checkpoint for activate state verify.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)virtual_drive_p, (void **)&nonpaged_metadata_p);

    /* Set the incomplete write start lba to the per-disk metadata start 
     */
    nonpaged_metadata_p->incomplete_write_verify_checkpoint = paged_md_start_lba / data_disks;

    /* Clear the condition.
     */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }
    else
    {
        /* Always trace when we have completed the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: mode: %d setup for verify of paged metadata complete\n", 
                              configuration_mode);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/***********************************************************************
 * end fbe_virtual_drive_setup_for_verify_paged_metadata_cond_function()
 ***********************************************************************/

/*!**************************************************************
 * fbe_virtual_drive_verify_paged_metadata_cond_function()
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
static fbe_lifecycle_status_t fbe_virtual_drive_verify_paged_metadata_cond_function(fbe_base_object_t *object_p, 
                                                                                    fbe_packet_t * packet_p)
{
    fbe_status_t                            status;
    fbe_virtual_drive_t                    *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_bool_t                              b_is_active;
    fbe_bool_t                              b_is_mirror_mode;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_scheduler_hook_status_t             hook_status = FBE_SCHED_STATUS_OK;
      
    /* Determine if we are active or not.
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)virtual_drive_p);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    b_is_mirror_mode = fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p);

    /* Debug trace the function entry.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s mode: %d active: %d\n", 
                          __FUNCTION__, configuration_mode, b_is_active);

    /* If a debug hook is set, we need to execute the specified action.
     */
    fbe_virtual_drive_check_hook(virtual_drive_p, 
                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                 FBE_VIRTUAL_DRIVE_SUBSTATE_PAGED_METADATA_VERIFY_START, 
                                 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /*! @todo we should have code that evaluates signature and nr before we get here,  
     *        but until we get that code we need to do this since we will hold stripe locks
     *        if any edge state events occur.
     */
    status = fbe_metadata_element_restart_io(&virtual_drive_p->spare.raid_group.base_config.metadata_element);
    fbe_base_config_clear_clustered_flag((fbe_base_config_t *)virtual_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED);
    fbe_base_config_clear_clustered_flag((fbe_base_config_t *)virtual_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING);

    /* Peter Puhov. We should not do it on Passive side */      
    /* Check if the RG is active on this SP or not.  If not, then we don't 
     * need to do anything more - return here and the monitor will reschedule 
     * on its normal cycle.  If the virtual drive is not in mirror mode 
     * (typical case), clear this condition and the `issue' metadata verify 
     * condition.
     */
    if ((b_is_active == FBE_FALSE)      ||
        (b_is_mirror_mode == FBE_FALSE)    )
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)virtual_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* See if metadata verify needs to run. 
     */
    if (fbe_raid_group_background_op_is_metadata_verify_need_to_run((fbe_raid_group_t *)virtual_drive_p, 
           (FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE | FBE_RAID_BITMAP_VERIFY_ERROR)))
    {
        /* Run verify operation. 
         */
        fbe_transport_set_completion_function(packet_p, fbe_virtual_drive_verify_paged_metadata_cond_completion, virtual_drive_p);
        fbe_virtual_drive_verify_paged_metadata(virtual_drive_p, packet_p);        
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    // If a debug hook is set, we need to execute the specified action...
    fbe_virtual_drive_check_hook(virtual_drive_p, 
                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                              FBE_VIRTUAL_DRIVE_SUBSTATE_PAGED_METADATA_VERIFY_COMPLETE, 
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }
    else
    {
        /* Always trace when we have completed the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: mode: %d verify of paged metadata complete\n", 
                              configuration_mode);
    }

    return FBE_LIFECYCLE_STATUS_DONE;

}
/*************************************************************
 * end fbe_virtual_drive_verify_paged_metadata_cond_function()
 *************************************************************/

/*!**************************************************************
 * fbe_virtual_drive_verify_paged_metadata()
 ****************************************************************
 * @brief
 *  This function performs raid group paged metadata verify operations.
 *
 * @param virtual_drive_p - Pointer to virtual drive object
 * @param packet_p - monitor packet
 *
 * @return fbe_status_t 
 *
 * @author
 *  01/2012 - Created.  Susan Rundbaken
 *  04/2012 - Modified  Prahlad Purohit
 *
 ****************************************************************/
static fbe_status_t fbe_virtual_drive_verify_paged_metadata(fbe_virtual_drive_t *virtual_drive_p,
                                                            fbe_packet_t *packet_p)
{
    fbe_raid_group_t                    *raid_group_p = (fbe_raid_group_t *)virtual_drive_p;
    fbe_lba_t                            metadata_verify_lba;
    fbe_traffic_priority_t               io_priority; 
    fbe_payload_block_operation_opcode_t op_code = FBE_RAID_GROUP_BACKGROUND_OP_NONE;
    fbe_chunk_size_t                     chunk_size;
    fbe_block_count_t                    block_count;
    fbe_u8_t                             verify_flags;

    /* Trace the function entry
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,          
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                          "%s entry\n", __FUNCTION__);         

    /*  Set the verify priority in the packet.
     */
    io_priority = fbe_raid_group_get_verify_priority(raid_group_p);
    fbe_transport_set_traffic_priority(packet_p, io_priority);    

    /*  Get the lba and type for the metadata chunk to verify.
     */
    fbe_raid_group_verify_get_next_verify_type(raid_group_p, 
                                               TRUE,
                                               &verify_flags);

    fbe_raid_group_verify_get_next_metadata_verify_lba(raid_group_p,
                                                       verify_flags,
                                                       &metadata_verify_lba);
    if(metadata_verify_lba == FBE_LBA_INVALID)
    {
        if (verify_flags != 0)
        {
            /* There are no valid verify flags set, but we still have a checkpoint,
             * it is possible the flags were cleared in a rebuild. 
             * Set the checkpoint back to 0 so we can continue with the user region. 
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: no chunks marked, but checkpoint present for vr flags: 0x%x\n", 
                                  __FUNCTION__, verify_flags);
            fbe_raid_group_set_verify_checkpoint(raid_group_p, packet_p, verify_flags, 0);
        }
        else
        {
            /*  No valid flag bits are set.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
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
    op_code = (fbe_payload_block_operation_opcode_t)fbe_raid_group_verify_get_opcode_for_verify(raid_group_p, verify_flags);
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
    block_count = chunk_size;

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: send verify op: %d lba: 0x%llx blks: 0x%llx\n",
                          op_code, (unsigned long long)metadata_verify_lba, (unsigned long long)block_count);

    /* Send the verify packet to the raid executor.
     */
    fbe_raid_group_executor_send_monitor_packet(raid_group_p, packet_p, 
                                                op_code, metadata_verify_lba, block_count);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/***********************************************
 * end fbe_virtual_drive_verify_paged_metadata()
 ***********************************************/

/*!****************************************************************************
 * fbe_virtual_drive_verify_paged_metadata_cond_completion()
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
static fbe_status_t fbe_virtual_drive_verify_paged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                 fbe_packet_completion_context_t context)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_virtual_drive_t            *virtual_drive_p = (fbe_virtual_drive_t *)context;
    fbe_raid_group_t               *raid_group_p = (fbe_raid_group_t *)virtual_drive_p;
    fbe_payload_ex_t               *sep_payload_p = NULL;
    fbe_payload_block_operation_t  *block_operation_p = NULL;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qual;
    fbe_raid_verify_error_counts_t *verify_errors_p;
    fbe_lba_t                       lba;
    fbe_block_count_t               block_count;

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

    /* Get the raid object's verify error counters.
     */
    verify_errors_p = fbe_raid_group_get_verify_error_counters_ptr(raid_group_p);

    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry\n", __FUNCTION__);

    /* Check status and for now only clear the condition if status is good.
     */
    if ((status != FBE_STATUS_OK) || 
        (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: paged metadata verify failed - status: 0x%x sts/qual: 0x%x/0x%x\n",
                              status, block_status, block_qual);

        if ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT) ||
            (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED)   )
        {
            /* We do not clear the condition since we need to wait for the object to be ready.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s block_status: 0x%x\n", __FUNCTION__, block_status);
        }
        else if (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
        {
            /*! We need to add handling for uncorrectables/media errors.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s block_status: 0x%x\n", __FUNCTION__, block_status);
        }
        else
        {
            /*! @todo This is an unexpected case, add some handling.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s block_status: 0x%x\n", __FUNCTION__, block_status);
        }
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Check for uncorrectable errors in the verify report.
     * This means the I/O succeeded; however, the data is not correct.
     * Will try to reconstruct the paged metadata in this case.
     * !@TODO: use a utility function for the uncorrectables check; need appropriate home for it
     */
    if (verify_errors_p->u_coh_count ||
        verify_errors_p->u_crc_count ||
        verify_errors_p->u_crc_multi_count ||
        verify_errors_p->u_crc_single_count ||
        verify_errors_p->u_media_count ||
        verify_errors_p->u_ss_count ||
        verify_errors_p->u_ts_count ||
        verify_errors_p->u_ws_count ||
        verify_errors_p->invalidate_count)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: paged MD verify coh: %d crc: %d media: %d stamp: %d inv: %d. So reconstruct required\n",
                              verify_errors_p->u_coh_count,
                              (verify_errors_p->u_crc_count + verify_errors_p->u_crc_multi_count + verify_errors_p->u_crc_single_count),
                              verify_errors_p->u_media_count,
                              (verify_errors_p->u_ss_count + verify_errors_p->u_ts_count + verify_errors_p->u_ws_count),
                              verify_errors_p->invalidate_count);

        fbe_payload_block_get_lba(block_operation_p, &lba);
        fbe_payload_block_get_block_count(block_operation_p, &block_count);

        fbe_raid_group_bitmap_reconstruct_paged_metadata((fbe_raid_group_t *)virtual_drive_p, 
            packet_p, 
            lba,
            block_count, 
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/***************************************************************
 * end fbe_virtual_drive_verify_paged_metadata_cond_completion()
 ***************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_downstream_health_not_optimal_cond_function()
 ******************************************************************************
 * @brief
 *  This is a condition of specialize where we are waiting for the
 *  downstream health to be reasonable enough for us to continue with specialize.
 * 
 *  For the VD, it overrides this function since it does not need to wait at all
 *  before proceeding with specialize.
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is be based on parent
 *                                  function call.
 *
 * @author
 *  5/20/2011 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_virtual_drive_downstream_health_not_optimal_cond_function(fbe_base_object_t * object_p,
                                                              fbe_packet_t * packet_p)
{
    fbe_virtual_drive_t *                       virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_base_config_path_state_counters_t       path_state_counters;

    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Determine health of the downstream edges.
     * Just make sure there are no invalid edges since 
     * the VD is allowed to proceed through specialize even though 
     * the downstream health is not good, since it needs to be able to request 
     * a spare in this situation. 
     */
    fbe_base_config_get_path_state_counters((fbe_base_config_t*)virtual_drive_p, &path_state_counters);

    /* One invalid edge is OK since we're not swapped.
     */
    if (path_state_counters.invalid_counter < 2)
    {
        /* Clear the condition if we are in good state. */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)virtual_drive_p);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_virtual_drive_downstream_health_not_optimal_cond_function()
 ******************************************************************************/


/*!****************************************************************************
 *          fbe_virtual_drive_establish_edge_states_cond_function()
 ******************************************************************************
 * @brief
 *  This condition function will be invoked only in the Specialize state.  It
 *  checks the edge states to make sure that edges have finished coming up.
 * 
 *  For the VD, it overrides this function since it needs to transition to 
 *  fail if edges fail to come up. In fail state it will then trigger sparing.
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is be based on parent
 *                                  function call.
 *
 * @author
 *  9/25/2012 - Created. Prahlad Purohit
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_establish_edge_states_cond_function(fbe_base_object_t * in_object_p,
                                                  fbe_packet_t * packet_p)
{
    fbe_virtual_drive_t *virtual_drive_p = (fbe_virtual_drive_t *)in_object_p; 
    fbe_bool_t                              primary_edge_is_broken = FBE_TRUE;
    fbe_path_state_t                        first_edge_path_state;
    fbe_path_state_t                        second_edge_path_state;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
 
    /* Trace function entry.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Get some information
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);  

    /* Get the path counters, which contain the number of edges in each of the 
     * possible states.
     */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);

    /* If the primary edge in the enabled state we are done.
     */
    primary_edge_is_broken = fbe_virtual_drive_swap_is_primary_edge_broken(virtual_drive_p);
    if (primary_edge_is_broken == FBE_FALSE)
    {
        /* If the need replacemnet flag is set clear it.
         */
        if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE))
        {
            fbe_virtual_drive_swap_clear_need_replacement_drive_start_time(virtual_drive_p);
        }

        /* We are ready to transition to activate.  Clear this condition and 
         * continue.
         */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        fbe_base_config_clear_flag((fbe_base_config_t *)virtual_drive_p, FBE_BASE_CONFIG_FLAG_RESPECIALIZE); 
        return FBE_LIFECYCLE_STATUS_DONE; 
    }

    /* If this first time we need to wait set the `need replacement flag'.
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE))
    {
        fbe_virtual_drive_swap_set_need_replacement_drive_start_time(virtual_drive_p);
    }
    else
    {
        fbe_time_t  replacement_start_time;

        fbe_virtual_drive_swap_get_need_replacement_drive_start_time(virtual_drive_p, &replacement_start_time);            

        /* Check if it is time to transition to fail.
         */
        if ((fbe_get_time() - replacement_start_time) > VD_EDGE_ATTACH_DEBOUNCE_TIME_MS)
        {
            /* Transition to fail.
             */
            fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const, 
                                   in_object_p,
                                   FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

            fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_ATTACH_EDGE_TIMEDOUT);

            /* Trace some information.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: establish edge states mode: %d life: %d path state:[%d-%d] tmo secs: %d goto fail\n",
                                  configuration_mode, my_state, first_edge_path_state, second_edge_path_state,
                                  (VD_EDGE_ATTACH_DEBOUNCE_TIME_MS / 1000));

            fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 0);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    //  If the primary edge is not there, then we want to remain in the specialize state -
    //  return here and wait before rescheduling 
    if (primary_edge_is_broken == FBE_TRUE)
    {
        fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 100);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  The edges have finished initializing.  Clear the condition and reschedule the monitor. 
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);

    // Clear  need replacement time.
    fbe_virtual_drive_swap_clear_need_replacement_drive_start_time(virtual_drive_p);
    fbe_base_config_clear_flag((fbe_base_config_t *)virtual_drive_p, FBE_BASE_CONFIG_FLAG_RESPECIALIZE);        

    return FBE_LIFECYCLE_STATUS_RESCHEDULE;
}
/******************************************************************************
 * end fbe_virtual_drive_establish_edge_states_cond_function()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_need_replacement_drive_cond_function()
 *****************************************************************************
 *
 * @brief   This is a timer based condition that is periodically invoked.  We
 *          need to check the virtual drive configuration mode, downstream health
 *          and whether or not sufficient time has transpired to determine what if
 *          any actions to take.
 *          o Determine if we are in mirror mode or not.
 *              + If we are in mirror mode exit.
 *              + Else we are in pass-thru mode.  Determine if we need a permanent
 *                replacement drive or not.
 * 
 * @param   object_p - The ptr to the base object of the base config.
 * @param   packet_p - The packet sent to us from the scheduler.
 *
 * @return  fbe_lifecycle_status_t - Return is based on parent function call.
 *
 * @author
 *  09/25/2009 - Created. Dhaval Patel.
 *
 *****************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_need_replacement_drive_cond_function(fbe_base_object_t * object_p,
                                                                                     fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_t                    *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_bool_t                              b_is_mirror_mode = FBE_FALSE;
    fbe_bool_t                              b_is_copy_complete = FBE_FALSE;
    fbe_bool_t                              b_is_active_sp = FBE_TRUE;
    fbe_virtual_drive_flags_t               flags;
    fbe_bool_t                              b_no_spare_reported = FBE_FALSE;
    fbe_bool_t                              b_can_initiate = FBE_TRUE;
    fbe_bool_t                              b_is_permanent_sparing_required = FBE_FALSE;
    fbe_bool_t                              b_can_permanent_sparing_start = FBE_FALSE;
    fbe_bool_t                              b_can_proactive_copy_start = FBE_FALSE;
    fbe_bool_t                              b_can_swap_out_start = FBE_FALSE;
    fbe_bool_t                              b_is_proactive_spare_needed = FBE_FALSE;
    fbe_edge_index_t                        proactive_spare_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_base_config_downstream_health_state_t downstream_health;
    fbe_edge_index_t                        mirror_mode_edge_index_to_swap_out = FBE_EDGE_INDEX_INVALID;
    fbe_time_t                              need_replacement_drive_start_time = 0;
    fbe_base_config_path_state_counters_t   edge_state_counters;
    fbe_u32_t                               width = 0;

    /*! @note For debug purposes we don't use the standard method to get
     *        this value.
     */
    need_replacement_drive_start_time = virtual_drive_p->need_replacement_drive_start_time;

    /*! @note We let the `need replacement timer' run on both the passive and
     *        active SPs.  In case the passive becomes active the timer is
     *        not reset.
     */
    b_is_active_sp = fbe_base_config_is_active((fbe_base_config_t *) virtual_drive_p);

    /* Get the virtual drive flags.
     */
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);

    /* Determine our configuration mode.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    b_is_mirror_mode = fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p);
    b_is_copy_complete = fbe_virtual_drive_is_copy_complete(virtual_drive_p);
   
    /* Look for all the edges with downstream objects and verify its current
     * downstream health.
     */
    downstream_health = fbe_virtual_drive_verify_downstream_health(virtual_drive_p);  

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: need replacement active: %d life: %d mode: %d health: %d flags: 0x%x time: 0x%llx\n",
                            b_is_active_sp, my_state, configuration_mode, downstream_health, flags,
                            (unsigned long long)need_replacement_drive_start_time);
 
    /* Check for the case where we need to set the `no spare found reported' flag.
     */   
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SET_NO_SPARE_REPORTED))
    {
        /* Check the `no spare reported' in the metadata.
         */
        fbe_virtual_drive_metadata_has_no_spare_been_reported(virtual_drive_p, &b_no_spare_reported);

        /* If we need to set the no spare reported in the metadata.
         */
        if ((b_is_active_sp == FBE_TRUE)       &&
            (b_no_spare_reported == FBE_FALSE)    )
        {
            /* Now set the non-paged bit that indicates we have already reported
             * `no spare found'.
             */
            fbe_virtual_drive_metadata_set_no_spare_been_reported(virtual_drive_p, packet_p);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
    }


    /* If a proactive copy request was denied, check if we need to retry the
     * request (by setting the need proactive spare condition).
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SET_NO_SPARE_REPORTED)        ||
        fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_PROACTIVE_REQUEST_DENIED)     ||
        fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_BROKEN_REPORTED)   ||
        fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DENIED_REPORTED)   ||
        fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DEGRADED_REPORTED) ||
        fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_IN_PROGRESS_REPORTED)    ||
        fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_SOURCE_DRIVE_DEGRADED)      )
    {
        /* Check if proactive copy is required.
         */
        fbe_virtual_drive_swap_check_if_proactive_spare_needed(virtual_drive_p, &b_is_proactive_spare_needed, &proactive_spare_edge_index);
        if ( (b_is_active_sp == FBE_TRUE)                                                                    &&
             (b_is_proactive_spare_needed == FBE_TRUE)                                                       &&
             (my_state == FBE_LIFECYCLE_STATE_READY)                                                         &&
            !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE)             &&
            !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS)    )
        {
            /* determine if we can start proactive copy now */
            status = fbe_virtual_drive_swap_check_if_proactive_copy_can_start(virtual_drive_p,
                                                                              &b_can_proactive_copy_start);
            if (!b_can_proactive_copy_start)
            {
                /* Wait the minimum interval (1 second) an check again.
                 */
                fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 1000);
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            /* Else validate that we can start the swap-out.
             */
            fbe_virtual_drive_can_we_initiate_proactive_copy_request(virtual_drive_p, 
                                                                     &b_can_initiate);
            if (!b_can_initiate)
            {
                /* Cannot initiate right now.
                 */
                fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 1000);
                return FBE_LIFECYCLE_STATUS_DONE;
            }
            else
            {
                /* Trace a message and retry the proactive spare request.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: need replacement mode: %d flags: 0x%x health: %d set proactive copy cond\n",
                                      configuration_mode, flags, downstream_health);

                /* Set the proactive copy condition.
                 */
                fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const, 
                                       (fbe_base_object_t *)virtual_drive_p, 
                                       FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_PROACTIVE_SPARE);

                /* Although this is a timer we clear the condition.
                 */
                fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
                return FBE_LIFECYCLE_STATUS_DONE;
            }

        } /* end if proactive copy is required */
    }

    /* If either the local or peer job in-progress flag is set we cannot either
     * clear or set needs replacement and we cannot initiate a swap job.  Since
     * the job in-progress is set in the monitor context we don't need to lock.
     */
    if (fbe_raid_group_is_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS)      ||
        fbe_raid_group_is_peer_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS) ||
        fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS)                                       )
    {
        /* Wait the minimum interval (1 second) an check again.
         */
        fbe_virtual_drive_trace(virtual_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                                "virtual_drive: need replacement active: %d life: %d mode: %d flags: 0x%x jobinprg local: %d peer: %d\n",
                                b_is_active_sp, my_state, configuration_mode, flags,
                                fbe_raid_group_is_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS),
                                fbe_raid_group_is_peer_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS));
        fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 1000);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Get the path counters and width. */
    fbe_base_config_get_path_state_counters((fbe_base_config_t *)virtual_drive_p, &edge_state_counters);
    fbe_base_config_get_width((fbe_base_config_t *) virtual_drive_p, &width);

    /* If the virtual drive is in mirror mode and the downstream health is 
     * optimal and the `need replacement drive' flag is set, clear it now.
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE) &&
        (b_is_mirror_mode == FBE_TRUE)                                                                &&
        ((downstream_health == FBE_DOWNSTREAM_HEALTH_OPTIMAL) || 
         (edge_state_counters.enabled_counter == width))                                                 )                               
    {
        fbe_virtual_drive_swap_clear_need_replacement_drive_start_time(virtual_drive_p);
    }

    /* If the virtual drive is in pass-thru mode and the downstream health is 
     * optimal or degraded and the `need replacement drive' flag is set, clear it now.
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE) &&
        (b_is_mirror_mode == FBE_FALSE)                                                               &&
        ((downstream_health == FBE_DOWNSTREAM_HEALTH_OPTIMAL)  ||
         (downstream_health == FBE_DOWNSTREAM_HEALTH_DEGRADED)    )                                      )
    {
        fbe_virtual_drive_swap_clear_need_replacement_drive_start_time(virtual_drive_p);
    }

    /* If the virtual drive is in mirror mode and the downstream health is not
     * optimal and the `need replacement drive' flag is not set, set it now.
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE) && 
         (b_is_mirror_mode == FBE_TRUE)                                                                &&
         (downstream_health != FBE_DOWNSTREAM_HEALTH_OPTIMAL)                                          &&
         (edge_state_counters.enabled_counter != width)                                                &&
         (b_is_copy_complete == FBE_FALSE)                                                                )
    {
        fbe_virtual_drive_swap_set_need_replacement_drive_start_time(virtual_drive_p);
    }

    /*! @note This condition can be invoked without the need to replace the 
     *        failed drive. If the `need replacement drive' flag is not set
     *        simply clear the condition and exit.
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE))
    {
        /* Although this is a timer we clear the condition.
         */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If this is the passive SP just wait until we no longer need a 
     * replacement drive.
     */
    if (b_is_active_sp == FBE_FALSE)
    {
        /* Wait the minimum interval (1 second) an check again.
         */
        fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 1000);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /*! @note Based on the configuration mode and downstream health detemine if
     *        a swap request is required.
     */
    if (b_is_mirror_mode == FBE_TRUE)
    {
        /* Check if a swap-out is required.
         */
        status = fbe_virtual_drive_swap_get_swap_out_edge_index(virtual_drive_p, &mirror_mode_edge_index_to_swap_out);
        if (status != FBE_STATUS_OK)
        {
            /* If there is no position that needs a replacement drive clear
             * this condition.
             */
            fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s find swap out index failed. status :0x%x\n", __FUNCTION__, status);
            fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* If the swap-out index is valid it means that we need to change to
         * pass-thru mode.  But we need to wait a sufficent period before
         * we change mode to give the drive some time to be re-inserted.
         */
        if (mirror_mode_edge_index_to_swap_out != FBE_EDGE_INDEX_INVALID)
        {
            /* Determine if we can swap-out the edge or not.
             */
            status = fbe_virtual_drive_swap_check_if_swap_out_can_start(virtual_drive_p, &b_can_swap_out_start);
            if (!b_can_swap_out_start)
            {
                /* Wait the minimum interval (1 second) an check again.
                 */
                fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 1000);
                return FBE_LIFECYCLE_STATUS_DONE;
            }
            else
            {
                /* Else validate that we can start the swap-out.
                 */
                fbe_virtual_drive_can_we_initiate_replacement_request(virtual_drive_p, 
                                                                      &b_can_initiate);
                if (!b_can_initiate)
                {
                    /* Cannot initiate right now.
                     */
                    fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 1000);
                    return FBE_LIFECYCLE_STATUS_DONE;
                }
                else
                {
                    /* Else ask for permission
                     */
                    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_REQUEST_ABORT_COPY)       &&
                        !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS)    )
                    {
                        /* set the completion function before we ask for the spare permission. */
                        fbe_transport_set_completion_function(packet_p,
                                                              fbe_virtual_drive_copy_failed_ask_swap_out_permission_completion,
                                                              virtual_drive_p);
    
                        /* set the ask swap-in permission flag before we ask for permission. */
                        fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_REQUEST_ABORT_COPY);
    
                        /* Send event request to upstream edges to request for the hot sparing operation. */
                        fbe_virtual_drive_copy_failed_ask_swap_out_permission(virtual_drive_p, packet_p);
    
                        /* Return pending status since we are waiting for the permission.
                         * Clear this condition since we have started the `request spare'
                         * process. 
                         */
                        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
                        return FBE_LIFECYCLE_STATUS_PENDING;
                    }
            
                    /* We will come back.
                     */
                    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
                    return FBE_LIFECYCLE_STATUS_DONE;
                }
            }
        }
        else
        {
            /* Else there is no edge that requires swapping out.  Transiiton 
             * to activate.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: need replacement mode: %d flags: 0x%x health: %d swap-out not required.\n",
                                  configuration_mode, flags, downstream_health);

            /* Clear the `needs replacement start time'.
             */
            if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_REQUEST_ABORT_COPY)       &&
                !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS) &&
                 fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE)      )
            {
                status = fbe_virtual_drive_swap_clear_need_replacement_drive_start_time(virtual_drive_p);
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s clear need replacement failed. status: 0x%X\n", __FUNCTION__, status);
                }
            }

            /* Clear the condition
             */
            fbe_lifecycle_clear_current_cond((fbe_base_object_t *) virtual_drive_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }
    else
    {
        /* Else Check if a permanent spare is required.
         */
        status = fbe_virtual_drive_swap_check_if_permanent_spare_needed(virtual_drive_p,
                                                                        &b_is_permanent_sparing_required);
        if (status != FBE_STATUS_OK)
        {
             fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                                   FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s failed to check if permanent spare needed, status:0x%x\n",
                                   __FUNCTION__, status);
             return FBE_LIFECYCLE_STATUS_DONE;
        }
     
        /* If permanent sparing is required.
         */
        if (b_is_permanent_sparing_required == FBE_TRUE)
        {
            /* determine if we can start permanent sparing now? */
            status = fbe_virtual_drive_swap_check_if_permanent_sparing_can_start(virtual_drive_p,
                                                                                 &b_can_permanent_sparing_start);
            if (!b_can_permanent_sparing_start)
            {
                /* Wait the minimum interval (1 second) an check again.
                 */
                fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 1000);
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            /* If we are passive SP we cannot initiate sparing */
            fbe_virtual_drive_can_we_initiate_replacement_request(virtual_drive_p, 
                                                                  &b_can_initiate);
            if (!b_can_initiate)
            {
                /* Cannot initiate right now.
                 */
                fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 1000);
                return FBE_LIFECYCLE_STATUS_DONE;
            }
    
            /* ask upstream edge to get the permission to swap-in spare. */
            if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE)             &&
                !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS)    )
            {
                /* set the completion function before we ask for the spare permission. */
                fbe_transport_set_completion_function(packet_p,
                                                  fbe_virtual_drive_need_permanent_spare_cond_completion,
                                                  virtual_drive_p);
    
                /* set the ask swap-in permission flag before we ask for permission. */
                fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE);
    
                /* Send event request to upstream edges to request for the hot sparing operation. */
                fbe_virtual_drive_ask_permanent_spare_permission(virtual_drive_p, packet_p);
    
                /* Return pending status since we are waiting for the permission.
                 * Clear this condition since we have started the `request spare'
                 * process. 
                 */
                fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
                return FBE_LIFECYCLE_STATUS_PENDING;
            }
            
            /* We will come back.
             */
            fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        else
        {
            /* If permanent sparing is no longer required clear the needs replacement
             * start time.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: need replacement mode: %d flags: 0x%x health: %d permanent spare not required.\n",
                                  configuration_mode, flags, downstream_health);
     
            /* If we haven't started the swap-in job clear the needs replacement flag.
             */
            if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE)             &&
                !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS) &&
                 fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE)      )
            {
                status = fbe_virtual_drive_swap_clear_need_replacement_drive_start_time(virtual_drive_p);
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s clear need replacement failed. status: 0x%X\n", __FUNCTION__, status);
                }
            }

            /* Clear the condition
             */
            fbe_lifecycle_clear_current_cond((fbe_base_object_t *) virtual_drive_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    /* If permanent spare is not needed then clear the condition. */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_virtual_drive_need_replacement_drive_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_need_permanent_spare_cond_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for the condition function
 *  where we are trying to swap-in the spare edge.
 *
 * @param packet_p - The packet sent to us from the scheduler.
 * @param context - The pointer to the object.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  10/21/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_need_permanent_spare_cond_completion(fbe_packet_t * packet_p,
                                                       fbe_packet_completion_context_t context)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_virtual_drive_t *   virtual_drive_p = (fbe_virtual_drive_t *)context;

    /* Only clear the condition if the status is good and the update succeeded. */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed,status:0x%x\n", __FUNCTION__, status);

        /* clear the swap-in progress flag if job service returns error. */
        fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE);
    }

    /* By returning OK, the monitor packet will be completed to the next level. */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_need_permanent_spare_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_ask_permanent_spare_permission()
 ******************************************************************************
 * @brief
 *  This function is used to allocate the event and send it to RAID object to 
 *  ask for the spare operation which virtual drive object want to perform.
 * 
 * @param virtual_drive_p - Virtual Drive object.
 * @param packet_p        - FBE packet pointer.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 * @author
 * 10/19/2009 - Created. Peter Puhov.
 * 
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_ask_permanent_spare_permission(fbe_virtual_drive_t * virtual_drive_p,
                                                 fbe_packet_t * packet_p)
{
    fbe_event_t *           event_p = NULL;
    fbe_event_stack_t *     event_stack = NULL;
    fbe_lba_t               capacity;

    event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));
    if(event_p == NULL){ /* Do not worry we will send it later */
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_event_init(event_p);
    fbe_event_set_master_packet(event_p, packet_p);
    event_stack = fbe_event_allocate_stack(event_p);

    /* Fill the LBA range. */
    fbe_base_config_get_capacity((fbe_base_config_t *)virtual_drive_p, &capacity);
    fbe_event_stack_set_extent(event_stack, 0, (fbe_block_count_t) capacity);

    /* Set the event completion function. */
    fbe_event_stack_set_completion_function(event_stack,
                                            fbe_virtual_drive_ask_permanent_spare_permission_completion,
                                            virtual_drive_p);

    fbe_base_config_send_event((fbe_base_config_t *)virtual_drive_p, FBE_EVENT_TYPE_SPARING_REQUEST, event_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_ask_permanent_spare_permission()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_ask_permanent_spare_permission_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the event to ask for the 
 *  spare permission to RAID object.
 * 
 * @param event_p   - Event pointer.
 * @param context_p - Context which was passed with event.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 * @author
 * 10/19/2009 - Created. Peter Puhov.
 * 
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_ask_permanent_spare_permission_completion(fbe_event_t * event_p,
                                                            fbe_event_completion_context_t context)
{
    fbe_event_stack_t *         event_stack = NULL;
    fbe_packet_t *              packet_p = NULL;
    fbe_virtual_drive_t *       virtual_drive_p = NULL;
    fbe_u32_t                   event_flags = 0;
    fbe_event_status_t          event_status = FBE_EVENT_STATUS_INVALID;
    fbe_status_t                status;
    fbe_edge_index_t            permanent_spare_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t            original_drive_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_bool_t                  b_is_permanent_spare_required = FBE_TRUE;

    event_stack = fbe_event_get_current_stack(event_p);
    fbe_event_get_master_packet(event_p, &packet_p);

    virtual_drive_p = (fbe_virtual_drive_t *) context;

    /* Get flag and status of the event. */
    fbe_event_get_flags (event_p, &event_flags);
    fbe_event_get_status(event_p, &event_status);

    /* Release the event.  */
    fbe_event_release_stack(event_p, event_stack);
    fbe_event_destroy(event_p);
    fbe_memory_ex_release(event_p);

    if (event_status == FBE_EVENT_STATUS_BUSY)
    {
        /* Our upstream client is busy, we will need to try again later. */
        fbe_transport_set_status(packet_p, FBE_STATUS_BUSY, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_BUSY;
    }
    else if (event_status == FBE_EVENT_STATUS_NO_USER_DATA)
    {
        /*! @note Policy is that if the raid group is unconsumed we will NOT
         *        swap-in a permament spare.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_DRIVE_STATUS_CHANGE,
                              "%s Parent raid group unconsumed. event_status: %d\n",
                              __FUNCTION__, event_status);

        fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE);

        /* If parent is unconsumed we will not swap-in
         */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    else if (event_status != FBE_EVENT_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Unexpected event_status: %d\n",
                              __FUNCTION__, event_status);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet (packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* send the permanent spare swap-in request to job service if even flag is not set to deny. */
    if (!(event_flags & FBE_EVENT_FLAG_DENY))
    {
        /* It was not `denied' clear the `denied' flag.
         */
        fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SPARE_REQUEST_DENIED);

        /* Verify the current state of the edge before we start swap-in permanent spare. 
         */
        fbe_virtual_drive_swap_check_if_permanent_spare_needed(virtual_drive_p,
                                                               &b_is_permanent_spare_required);

        /* Find the edge index where we need to swap-in permanent spare. 
         */
        fbe_virtual_drive_swap_get_permanent_spare_edge_index(virtual_drive_p, 
                                                              &permanent_spare_edge_index);

        /* Validate that we still need a permanent spare.
         */
        if ((b_is_permanent_spare_required == FBE_FALSE)           ||
            (permanent_spare_edge_index == FBE_EDGE_INDEX_INVALID)    )
        {
            /* The drive came back after the need replacement drive request was
             * initiated.  Here we simply clear the condition.
             */
            if (b_is_permanent_spare_required == FBE_FALSE)
            {
                /* When the drive comes back we automatically clear the
                 * need replacement drive timer.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_DRIVE_STATUS_CHANGE,
                                      "%s Permanent sparing no longer needed.\n",
                                      __FUNCTION__);
            }
            else
            {
                /* Else permanent sparing is required but we failed to locate the
                 * edge that requires a spare.  Generate an error trace.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Failed to locate edge to swap in.\n",
                                      __FUNCTION__);
            }

            fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE);

            /* If downstream edge state is changed in between then clear the condition. 
             */
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_OK;
        }

        /* Now determine the `mirror' (a.k.a. original) edge index.
         */
        original_drive_edge_index = (permanent_spare_edge_index == FIRST_EDGE_INDEX) ? SECOND_EDGE_INDEX : FIRST_EDGE_INDEX;

        /* send the permanent spare swap-in request to job service. 
         */
        status = fbe_virtual_drive_initiate_swap_operation(virtual_drive_p,
                                                           packet_p,
                                                           permanent_spare_edge_index,
                                                           original_drive_edge_index,
                                                           FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND);

        /* Clear the flag that indicates there is a swap-in in progress.
         */
        fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE);

        /* If the request failed complete the packet.
         */
        if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
        {
            fbe_transport_set_status(packet_p, status, 0);
            fbe_transport_complete_packet(packet_p);
            return status;
        }
    }
    else
    {
        /* If RAID sends negative response then complete the packet with error status. 
         */
        if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SPARE_REQUEST_DENIED))
        {
            /* We have not reported the `denied' status.  Reset the timer to 
             * the full period, log the event and set the flag.
             */
            fbe_lifecycle_clear_current_cond((fbe_base_object_t *) virtual_drive_p);

            /* Log the event.
             */
            fbe_virtual_drive_swap_log_permanent_spare_denied(virtual_drive_p, packet_p);

            /* Set the flag.
             */
            fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SPARE_REQUEST_DENIED);
        }
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_ask_permanent_spare_permission_completion()
*******************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_copy_failed_ask_swap_out_permission()
 ******************************************************************************
 *
 * @brief   We `may' need to ask the upstream raid group if we are allowed to
 *          swap-out a failed copy position(s).  If the destination drive failed
 *          we do not need to ask for permission.
 *          
 to swap-out
 *          (i.e. abort the copy operation) for a copy operation where either the
 *          source or destination drive has failed.
 *
 * @param   packet_p - The packet sent to us from the scheduler.
 * @param   context - The pointer to the object.
 *
 * @return  fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  09/17/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_copy_failed_ask_swap_out_permission(fbe_virtual_drive_t *virtual_drive_p,
                                                                          fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_edge_index_t                        mirror_mode_edge_index_to_swap_out = FBE_EDGE_INDEX_INVALID;
    fbe_bool_t                              b_source_drive_failed = FBE_FALSE; 
    fbe_bool_t                              b_destination_drive_failed = FBE_FALSE; 
    fbe_object_id_t                         original_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         spare_pvd_object_id = FBE_OBJECT_ID_INVALID;

    /* Get the configuration mode.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Get the edge index to swap out.
     */
    status = fbe_virtual_drive_swap_get_swap_out_edge_index(virtual_drive_p, &mirror_mode_edge_index_to_swap_out);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy failed ask swap-out permission mode: %d swap-out index: %d status: 0x%x \n",
                          configuration_mode, mirror_mode_edge_index_to_swap_out, status);

    /* If the we are not in mirror mode something is wrong.
     */
    if ((configuration_mode != FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)  &&
        (configuration_mode != FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)    )
    {
        /* Trace the error and fail the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy failed ask swap-out permission mode: %d is not mirror.\n",
                          configuration_mode);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* If we could not which edge to swap out report an error and complete.
     */
    if ((status != FBE_STATUS_OK)                                      ||
        (mirror_mode_edge_index_to_swap_out == FBE_EDGE_INDEX_INVALID)    )
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy failed ask swap-out permission mode: %d index: %d failed to location edge.\n",
                          configuration_mode, mirror_mode_edge_index_to_swap_out);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* Determine if the source drive failed (need to handle the case where both 
     * the source and destination drive have failed).
     */
    b_source_drive_failed = fbe_virtual_drive_swap_is_primary_edge_broken(virtual_drive_p);

    /* Based on the configuration mode determine if the source, destination or both failed.
     */
    if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)
    {
        if (mirror_mode_edge_index_to_swap_out == FIRST_EDGE_INDEX)
        {
            b_source_drive_failed = FBE_TRUE;
        }
        else
        {
            b_destination_drive_failed = FBE_TRUE;
        }
    }
    else
    {
        /* Else mirror second edge.
         */
        if (mirror_mode_edge_index_to_swap_out == SECOND_EDGE_INDEX)
        {
            b_source_drive_failed = FBE_TRUE;
        }
        else
        {
            b_destination_drive_failed = FBE_TRUE;
        }
    }

    /* Log the event indicating that the copy operation has completed. */
    fbe_virtual_drive_event_logging_get_original_pvd_object_id(virtual_drive_p,
                                                               &original_pvd_object_id,
                                                               FBE_TRUE /* Copy is in-progress */);
    fbe_virtual_drive_event_logging_get_spare_pvd_object_id(virtual_drive_p,
                                                            &spare_pvd_object_id,
                                                            FBE_TRUE /* Copy is in-progress */);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                  FBE_TRACE_LEVEL_INFO,
                  FBE_TRACE_MESSAGE_ID_INFO,
                  "virtual_drive: copy failed ask prms source failed: %d dest failed: %d orig pvd: 0x%x dest pvd: 0x%x\n",
                  b_source_drive_failed, b_destination_drive_failed, original_pvd_object_id, spare_pvd_object_id);

    /* Need to ask for permission before we can proceed.
     */
    status = fbe_virtual_drive_swap_ask_copy_abort_go_degraded_permission(virtual_drive_p,
                                                                          packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy failed ask swap-out permission mode: %d index: %d failed. status: 0x%x\n",
                          configuration_mode, mirror_mode_edge_index_to_swap_out, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_copy_failed_ask_swap_out_permission()
 *******************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_copy_failed_ask_swap_out_permission_completion()
 ******************************************************************************
 *
 * @brief   This is the completion after we have asked for permission to swap-out
 *          (i.e. abort the copy operation) for a copy operation where either the
 *          source or destination drive has failed.
 *
 * @param   packet_p - The packet sent to us from the scheduler.
 * @param   context - The pointer to the object.
 *
 * @return  fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  09/17/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_copy_failed_ask_swap_out_permission_completion(fbe_packet_t * packet_p,
                                                                                     fbe_packet_completion_context_t context)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_scheduler_hook_status_t hook_status;
    fbe_virtual_drive_t        *virtual_drive_p = (fbe_virtual_drive_t *)context;
    fbe_virtual_drive_flags_t   flags;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_lifecycle_state_t       my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_base_config_downstream_health_state_t downstream_health;
    fbe_edge_index_t            mirror_mode_edge_index_to_swap_out = FBE_EDGE_INDEX_INVALID;

    /* Determine our configuration mode.
     */
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
   
    /* Look for all the edges with downstream objects and verify its current
     * downstream health.
     */
    downstream_health = fbe_virtual_drive_verify_downstream_health(virtual_drive_p); 
    fbe_virtual_drive_swap_get_swap_out_edge_index(virtual_drive_p, &mirror_mode_edge_index_to_swap_out);
    
    /* Get the status of the permission request.
     */
    status = fbe_transport_get_status_code(packet_p);

    /* Always trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "virtual_drive: ask swap-out permission comp life: %d mode: %d flags: 0x%x health: %d index: %d status: 0x%x\n",
                           my_state, configuration_mode, flags, downstream_health,
                           mirror_mode_edge_index_to_swap_out, status);

    /* Clear the flag indicating that an abort copy request is in progress
     */
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_REQUEST_ABORT_COPY);

    /* Only clear the condition if the status is good and the update succeeded. */
    if (status == FBE_STATUS_OK)
    {
        /* Ok to proceed with swap-out.
         */
        fbe_virtual_drive_check_hook(virtual_drive_p,
                                     SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY, 
                                     FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_GRANTED, 
                                     0, &hook_status);

        /*! @note Only when the swap-out completes can we modify the checkpoints
         *        etc.  Set the condition that will start the abort copy job.
         */
        fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                               (fbe_base_object_t *)virtual_drive_p,
                               FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_IS_COPY_COMPLETE);

        /* If we are not in the Ready state set the `hold in fail' flag.
         */
        if (my_state != FBE_LIFECYCLE_STATE_READY)
        {
            /* Set the flag to hold the virtual drive in the failed state until 
             * the `abort copy' is complete.
             */
            fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_HOLD_IN_FAIL);
        }
     
        /* Clear the `needs replacement start time' and this condition.
         */
        status = fbe_virtual_drive_swap_clear_need_replacement_drive_start_time(virtual_drive_p);
    }
    else
    {
        /* Else based on the status we determine how to proceed.
         */
        if (status == FBE_STATUS_BUSY)
        {
            /* The upstream raid group denied the swap-out.  Reset the 
             * `need replacement' timer.
             */
            fbe_virtual_drive_check_hook(virtual_drive_p,
                                         SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY, 
                                         FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_DENIED, 
                                         0, &hook_status);
            status = fbe_virtual_drive_swap_reset_need_replacement_drive_start_time(virtual_drive_p);
        }
        else
        {
            /* Else trace an error and clear the `needs replacement' timer.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                           "virtual_drive: ask swap-out permission comp mode: %d flags: 0x%x health: %d swap-out index: %d status: 0x%x\n",
                           configuration_mode, flags, downstream_health,
                           mirror_mode_edge_index_to_swap_out, status);

            /* Clear the `needs replacement start time' and this condition.
             */
            status = fbe_virtual_drive_swap_clear_need_replacement_drive_start_time(virtual_drive_p);
        }
    }

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_copy_failed_ask_swap_out_permission_completion()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_evaluate_downstream_health_cond_function()
 ******************************************************************************
 *
 * @brief   This condition executes when a downstream `event' occurred but the
 *          virtual drive was not yet `Ready'.  This condition runs in the
 *          Ready state and re-evaluates the downstream health and then clears
 *          the condition.
 * 
 * @param   object_p - The ptr to the base object of the base config.
 * @param   packet_p - The packet sent to us from the scheduler.
 *
 * @return  fbe_lifecycle_status_t - Return is based on parent function call.
 *
 * @author
 *  07/03/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_evaluate_downstream_health_cond_function(fbe_base_object_t * object_p,
                                                                                         fbe_packet_t * packet_p)
{
    fbe_virtual_drive_t                        *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_base_config_downstream_health_state_t   downstream_health_state;

    /* Determine the health of the path with downstream objects. 
     */
    downstream_health_state = fbe_virtual_drive_verify_downstream_health(virtual_drive_p);

    /* Set the specific lifecycle condition based on health of the path with 
     * downstream objects. 
     */
    fbe_virtual_drive_set_condition_based_on_downstream_health(virtual_drive_p, downstream_health_state, __FUNCTION__);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: evaluate downstream health: %d\n",
                          downstream_health_state);

    /* Clear the condition since this is a `one-shot'*/
    fbe_lifecycle_clear_current_cond((fbe_base_object_t*)virtual_drive_p);

    /* We are done with this condition.
     */
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_virtual_drive_evaluate_downstream_health_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_start_user_copy_cond_function()
 ******************************************************************************
 *
 * @brief   This condition is set when a user copy request is initiated from
 *          the job service.  Control of all virtual drive operations needs to
 *          be done thru the virtual drive monitor.  This condition is signalled
 *          to start the user copy process.
 * 
 * @param   object_p - The ptr to the base object of the base config.
 * @param   packet_p - The packet sent to us from the scheduler.
 *
 * @return  fbe_lifecycle_status_t - Return is based on parent function call.
 *
 * @author
 *  04/26/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_start_user_copy_cond_function(fbe_base_object_t * object_p,
                                                                              fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_t                    *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;  
    fbe_virtual_drive_flags_t               flags;
    fbe_spare_swap_command_t                copy_request_type;

    /* Get the virtual drive flags.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &copy_request_type);

    /* Trace some information
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: start user copy cond mode: %d cmd: %d flags: 0x%x \n",
                          configuration_mode, copy_request_type, flags);

    /* Invoke the method to start the user copy
     */
    status = fbe_virtual_drive_start_user_copy(virtual_drive_p);
    if (status != FBE_STATUS_OK)
    {
        /* Trace an error and continue
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: start user copy cond mode: %d cmd: %d failed - status: 0x%x\n",
                              configuration_mode, copy_request_type, status);
    }

    /* Clear the condition since we do not need proactive spare anymore. 
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *) virtual_drive_p);

    /* reschedule monitor immediately. */
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_virtual_drive_start_user_copy_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_need_proactive_spare_cond_function()
 ******************************************************************************
 *  This is the base condition function for virtual drive, this condition will
 *  be set whenever we need to swap-in the proactive spare, spare for the 
 *  equalize operation or external copy operation.
 *  
 *  It determines current state of the downstream health and send the specific
 *  request to job service to perform the swap operation.
 * 
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is based on parent function call.
 *
 * @author
 *  12/24/2009 - Created. Dhaval Patel.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_need_proactive_spare_cond_function(fbe_base_object_t * object_p,
                                                     fbe_packet_t * packet_p)
{
    fbe_virtual_drive_t                    *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_bool_t                              b_is_active_sp = FBE_TRUE;
    fbe_bool_t                              b_is_proactive_spare_needed = FBE_FALSE;
    fbe_bool_t                              b_is_proactive_spare_already_swapped_in = FBE_FALSE;
    fbe_edge_index_t                        proactive_spare_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;  
    fbe_virtual_drive_flags_t               flags;
    fbe_scheduler_hook_status_t             hook_status;

    /* Get the virtual drive flags.
     */
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);

    /* determine if we are passive or active? */
    b_is_active_sp = fbe_base_config_is_active((fbe_base_config_t *)virtual_drive_p);
    if (!b_is_active_sp)
    {
        /* passive sp does not initiate the proactive sparing.
         * clear the condition.
         */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* check that do we need to start the proactive sparing operation. */
    fbe_virtual_drive_swap_check_if_proactive_spare_needed(virtual_drive_p, &b_is_proactive_spare_needed, &proactive_spare_edge_index);

    /* verify whether proactive spare is already swapped-in. */
    fbe_virtual_drive_swap_is_proactive_spare_swapped_in(virtual_drive_p, &b_is_proactive_spare_already_swapped_in);

    /* Get the configuration mode.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    
    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_PROACTIVE_TRACING,
                            "virtual_drive: proactive spare needed: %d mode: %d flags: 0x%x edge index: %d already swapped: %d \n",
                            b_is_proactive_spare_needed, configuration_mode, flags, proactive_spare_edge_index, 
                            b_is_proactive_spare_already_swapped_in);

    /* If the `need proactive spare' hook is set then process it
     */
    fbe_virtual_drive_check_hook(virtual_drive_p,
                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                 FBE_VIRTUAL_DRIVE_SUBSTATE_NEED_PROACTIVE_SPARE, 
                                 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Send the swap-in spare edge command to job service if we can start the
     * proactive spare swap-in.  The `swap-in-edge' flag indicates that we have already
     * started the proactive copy.
     */
    if ( (b_is_proactive_spare_needed == FBE_TRUE)                                           &&
        !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE)    )
    {
        /* If this is the first time we have detected the `need proactive copy' 
         * condition set the timer.
         */
        if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_PROACTIVE_COPY))
        {
            /* Set the `need proactive copy' start time.
             */
            fbe_virtual_drive_swap_set_need_proactive_copy_start_time(virtual_drive_p);
        }

        /* If there is already a job-in-progress mark the request as `denied'
         */
        if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS))
        {
            /* Mark the request as denied and clear this condition.
             */
            fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_PROACTIVE_REQUEST_DENIED);

            /* Set the condition to retry the proactive copy operation.
             */
            fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                   (fbe_base_object_t *)virtual_drive_p,
                                   FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE);

            /* Clear this condition since either the proactive copy is already
             * in progress or needs replacement will recognize the denied flag.
             */
            fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        else
        {
            /* Else if we haven't already started the proactive copy.  Set the swap-in progress
             * flag before we ask for permission. 
             */
            fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE);
    
            /* Set the completion function before we ask for the spare permission. */
            fbe_transport_set_completion_function(packet_p,
                                                  fbe_virtual_drive_need_proactive_spare_cond_completion,
                                                  virtual_drive_p);
    
            /* Send event request to upstream edges to request for the proactive sparing operation. */
            fbe_virtual_drive_ask_proactive_spare_permission(virtual_drive_p, packet_p);
    
            /* Return pending status since we are waiting for the permission.
             * Clear this condition since we have started the `proactive spare'
             * process. 
             */
            fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
    }

    /* Clear the condition since we do not need proactive spare anymore. 
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *) virtual_drive_p);

    /* reschedule monitor immediately. */
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_virtual_drive_need_proactive_spare_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_need_proactive_spare_cond_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for the condition function where we are trying to 
 *  swap-in the proactive spare edge.
 *
 * @param packet_p  - The packet sent to us from the scheduler.
 * @param context   - The pointer to the object.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  10/21/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_need_proactive_spare_cond_completion(fbe_packet_t * packet_p,
                                                       fbe_packet_completion_context_t context)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_virtual_drive_t *   virtual_drive_p = (fbe_virtual_drive_t *)context;

    /* Only clear the condition if the status is good and the update succeeded. */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed - status: 0x%x\n",
                              __FUNCTION__, status);

        /* Clear the swap-in progress flag if job service returns error. */
        fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE);
    }

    /* By returning OK, the monitor packet will be completed to the next level. */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_need_proactive_spare_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_clear_no_spare_cond_function()
 ******************************************************************************
 *  This is the base condition function for virtual drive, this condition will
 *  be clear the `no spare reported' non-paged metadata flag.
 *  
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is based on parent function call.
 *
 * @author
 *  04/26/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_clear_no_spare_cond_function(fbe_base_object_t * object_p,
                                                                             fbe_packet_t * packet_p)
{
    fbe_virtual_drive_t *                     virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_bool_t                                b_no_spare_reported = FBE_FALSE;

    /* Always clear this condition.
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *) virtual_drive_p);

    /* Validate that we need to the clear the `no spare reported' non-paged
     * flag.
     */
    fbe_virtual_drive_metadata_has_no_spare_been_reported(virtual_drive_p, &b_no_spare_reported);
    if (b_no_spare_reported == FBE_FALSE)
    {
        /* Simply return done.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Invoke method to clear the `no spare reported' non-paged flag.
     */
    fbe_virtual_drive_metadata_clear_no_spare_been_reported(virtual_drive_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_virtual_drive_clear_no_spare_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_report_copy_denied_cond_function()
 ******************************************************************************
 *  The purpose of this condition is to simply report that the user copy request
 *  has been denied.
 *  
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is based on parent function call.
 *
 * @author
 *  05/24/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_report_copy_denied_cond_function(fbe_base_object_t * object_p,
                                                                                 fbe_packet_t * packet_p)
{
    fbe_virtual_drive_t *                     virtual_drive_p = (fbe_virtual_drive_t *)object_p;

    /* Always clear this condition.
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *) virtual_drive_p);

    /* Simply invoke the method to report the fact that the user copy was
     * denied.
     */
    fbe_virtual_drive_swap_log_user_copy_denied(virtual_drive_p, packet_p);

    /* Clear the flag indicating that we need to log this event.
     */
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_USER_COPY_DENIED);

    /* Done.
     */
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_virtual_drive_report_copy_denied_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_ask_proactive_spare_permission()
 ******************************************************************************
 * @brief
 *  This function is used to allocate the event and send it to RAID object to
 *  ask for the spare operation which virtual drive object want to perform.
 * 
 * @param virtual_drive_p - Virtual Drive object.
 * @param packet_p        - FBE packet pointer.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 * @author
 * 10/19/2009 - Created. Peter Puhov.
 * 
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_ask_proactive_spare_permission(fbe_virtual_drive_t * virtual_drive_p,
                                                 fbe_packet_t * packet_p)
{
    fbe_event_t *           event_p = NULL;
    fbe_event_stack_t *     event_stack = NULL;
    fbe_lba_t               capacity;

    event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));
    if(event_p == NULL){ /* Do not worry we will send it later */
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_event_init(event_p);
    fbe_event_set_master_packet(event_p, packet_p);
    event_stack = fbe_event_allocate_stack(event_p);

    /* Fill LBA data. */
    fbe_base_config_get_capacity((fbe_base_config_t *)virtual_drive_p, &capacity);
    fbe_event_stack_set_extent(event_stack, 0, (fbe_block_count_t) capacity);

    /* Set the event completion function. */
    fbe_event_stack_set_completion_function(event_stack,
                                            fbe_virtual_drive_ask_proactive_spare_permission_completion,
                                            virtual_drive_p);

    fbe_base_config_send_event((fbe_base_config_t *)virtual_drive_p, FBE_EVENT_TYPE_COPY_REQUEST, event_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_ask_proactive_spare_permission()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_ask_proactive_spare_permission_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the event to ask for the
 *  permission to RAID object.
 * 
 * @param event_p   - Event pointer.
 * @param context_p - Context which was passed with event.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 * @author
 * 10/19/2009 - Created. Peter Puhov.
 * 
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_ask_proactive_spare_permission_completion(fbe_event_t * event_p,
                                                            fbe_event_completion_context_t context)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_event_stack_t      *event_stack = NULL;
    fbe_packet_t           *packet_p = NULL;
    fbe_virtual_drive_t    *virtual_drive_p = NULL;
    fbe_u32_t               event_flags = 0;
    fbe_event_status_t      event_status = FBE_EVENT_STATUS_INVALID;
    fbe_path_state_t        edge_path_state;
    fbe_block_edge_t       *block_edge_p = NULL;
    fbe_edge_index_t        proactive_spare_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t        original_drive_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_bool_t              b_is_proactive_copy_required = FBE_FALSE;

    event_stack = fbe_event_get_current_stack(event_p);
    fbe_event_get_master_packet(event_p, &packet_p);

    virtual_drive_p = (fbe_virtual_drive_t *) context;

    /* Get flag and status of the event. */
    fbe_event_get_flags (event_p, &event_flags);
    fbe_event_get_status(event_p, &event_status);

    /* Release the event. */
    fbe_event_release_stack(event_p, event_stack);
    fbe_event_destroy(event_p);
    fbe_memory_ex_release(event_p);

    /* Trace some information if the status wasn't success.
     */
    if ((event_status != FBE_EVENT_STATUS_OK)      ||
        ((event_flags & FBE_EVENT_FLAG_DENY) != 0)    )
    {
        /* Trace some information.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: ask proactive copy perm comp event status: %d flags: 0x%x failed \n",
                              event_status, event_flags);
    }

    /* If busy return busy.
     */
    if (event_status == FBE_EVENT_STATUS_BUSY)
    {
        /* Our upstream client is busy, we will need to try again later, 
         * we cannot start the proactive sparing operation now.
         */
        status = FBE_STATUS_BUSY;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet (packet_p);
        return status;
    }
    else if (event_status == FBE_EVENT_STATUS_NO_USER_DATA)
    {
        /*! @note Policy is that if the raid group is unconsumed we will NOT
         *        swap-in a permament spare.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_DRIVE_STATUS_CHANGE,
                              "%s Parent raid group unconsumed. event_status: %d\n",
                              __FUNCTION__, event_status);

        fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE);

        /* If parent is unconsumed we will not swap-in
         */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    else if (event_status != FBE_EVENT_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s unexpected event status: 0x%x\n",
                              __FUNCTION__, event_status);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet (packet_p);
        return status;
    }

    /* If we find that deny flag is not set then send the proactive spare
     * swap-in request to job service.
     */
    if(!(event_flags & FBE_EVENT_FLAG_DENY))
    {
        /* Clear the `denied' flag.
         */
        fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_PROACTIVE_REQUEST_DENIED);

        /* if we find we do not need proactive spare swap-in any more then complete the request.*/
        fbe_virtual_drive_swap_check_if_proactive_spare_needed(virtual_drive_p, &b_is_proactive_copy_required, &proactive_spare_edge_index);
        if((!b_is_proactive_copy_required) ||
           (proactive_spare_edge_index == FBE_EDGE_INDEX_INVALID))
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_set_status(packet_p, status, 0);
            fbe_transport_complete_packet (packet_p);
            return status;
        }

        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: ask permission complete swap-in proactive spare edge index: %d\n",
                              proactive_spare_edge_index);

        /* verify the current state of the edge before we start swap-in proactive spare. */
        fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge_p, proactive_spare_edge_index);
        fbe_block_transport_get_path_state(block_edge_p, &edge_path_state);
        if(edge_path_state != FBE_PATH_STATE_INVALID)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_set_status(packet_p, status, 0);
            fbe_transport_complete_packet(packet_p);
            return status;
        }

        /* Now determine the `mirror' (a.k.a. original) edge index.
         */
        original_drive_edge_index = (proactive_spare_edge_index == FIRST_EDGE_INDEX) ? SECOND_EDGE_INDEX : FIRST_EDGE_INDEX;

        /* send the proactive spare swap-in request to job serive. */
        status = fbe_virtual_drive_initiate_swap_operation(virtual_drive_p,
                                                           packet_p,
                                                           proactive_spare_edge_index,
                                                           original_drive_edge_index,
                                                           FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND);

        /* Clear the flag that indicates there is a swap-in in progress.
         */
        fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE);

        /* If the request failed complete the packet.
         */
        if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
        {
            fbe_transport_set_status(packet_p, status, 0);
            fbe_transport_complete_packet(packet_p);
            return status;
        }
    }
    else
    {
        /* If RAID sends negative response then complete the packet with error status. */
        fbe_virtual_drive_swap_reset_need_proactive_copy_start_time(virtual_drive_p);
        if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_PROACTIVE_REQUEST_DENIED))
        {
            /* We have not reported the `denied' status log the event.
             */

            /* Log the event.
             */
            fbe_virtual_drive_swap_log_proactive_spare_denied(virtual_drive_p, packet_p);

            /* Set the flag.
             */
            fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_PROACTIVE_REQUEST_DENIED);
        }

        /* Set the condition to retry the proactive copy operation.
         */
        fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                               (fbe_base_object_t *)virtual_drive_p,
                               FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE);

        /* Faile the request.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_ask_proactive_spare_permission_completion()
*******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_in_edge_cond_function()
 *****************************************************************************
 *
 * @brief   This monitor condition is signalled when a new edge has been
 *          created on a virtual drive.
 *   
 * @param   object_p - The ptr to the base object of the base config.
 * @param   packet_p - The packet sent to us from the scheduler.
 *
 * @return  fbe_lifecycle_status_t
 *
 * @author
 *  11/01/2012  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_swap_in_edge_cond_function(fbe_base_object_t *object_p,
                                                                           fbe_packet_t *packet_p)
{
    fbe_virtual_drive_t                    *virtual_drive_p = NULL;
    fbe_scheduler_hook_status_t             hook_status;
    fbe_edge_index_t                        swap_in_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_bool_t                              b_is_active = FBE_TRUE;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_virtual_drive_flags_t               flags;
    fbe_path_state_t                        swap_in_edge_path_state = FBE_PATH_STATE_INVALID;
    fbe_spare_swap_command_t                copy_request_type = FBE_SPARE_SWAP_INVALID_COMMAND;
    fbe_object_id_t                         orig_pvd_object_id;
    fbe_status_t                            status;

    /* cast the base object to a virtual drive object */
    virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)virtual_drive_p);
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &copy_request_type);
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    swap_in_edge_index = fbe_virtual_drive_get_swap_in_edge_index(virtual_drive_p);
    fbe_virtual_drive_get_orig_pvd_object_id(virtual_drive_p, &orig_pvd_object_id);

    /* Get the current path state of the edges that was just swapped-in.
     */
    if ((swap_in_edge_index == FIRST_EDGE_INDEX)  ||
        (swap_in_edge_index == SECOND_EDGE_INDEX)    )
    {
        fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[swap_in_edge_index],
                                           &swap_in_edge_path_state);
    }

    /* This condition is called for the initial edge creation as well as
     * the swap in of a destination edge for a copy operation.  If it is
     * a copy operation trace it.
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS))
    {
        /* Trace swap-in for job request.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: swap in cond mode: %d life: %d active: %d cmd: %d flags: 0x%x swap: %d state: %d\n",
                              configuration_mode, my_state, b_is_active, 
                              copy_request_type, flags, swap_in_edge_index, swap_in_edge_path_state);
    }
    else
    {
        /* Else only trace if enabled
         */
        fbe_virtual_drive_trace(virtual_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                                "virtual_drive: swap in cond mode: %d life: %d active: %d cmd: %d flags: 0x%x swap: %d state: %d\n",
                                configuration_mode, my_state, b_is_active, 
                                copy_request_type, flags, swap_in_edge_index, swap_in_edge_path_state);
    }

    /* Validate the swap-in edge index.
     */
    if ((swap_in_edge_index != FIRST_EDGE_INDEX)  &&
        (swap_in_edge_index != SECOND_EDGE_INDEX)    )
    {
        /* Trace the error and clear this condition.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s invalid swap-in index: %d \n",
                              __FUNCTION__, swap_in_edge_index);
        
        /* Clear the condition and return done.
         */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_virtual_drive_swap_in_check_encryption_state(virtual_drive_p);
    if (status != FBE_STATUS_CONTINUE)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s waiting new keys to be pushed \n",
                              __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If the lifecycle is failed and we are swapping in an edge we may need to
     * update the `original' pvd id since the swap-in occurred after a reboot.
     */
    if (orig_pvd_object_id == FBE_OBJECT_ID_INVALID)
    {
        fbe_virtual_drive_swap_save_original_pvd_object_id(virtual_drive_p);
    }

    /*! @note Do not wait for an edge state of `Enabled' since the edge state
     *        can change at any time. The edge has been created, if the edge
     *        state changes after it is created we will get an edge state
     *        change notification.
     */

    /* If the swap-in debug hook is set process it.
     */
    fbe_virtual_drive_check_hook(virtual_drive_p,
                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                 FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED, 
                                 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Set the flag indicating that the swap-in edge is complete.  Even if
     * confirmation is disabled we set the swap-in edge complete.
     */
    if ( fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS))
    {
        fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE_COMPLETE);
    }
    
    /* Clear the condition. We are done.
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);

    /* We are done.
     */
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_in_edge_cond_function()
 ******************************************************************************/

/*!***************************************************************************
 * fbe_virtual_drive_initiate_copy_complete_or_abort_completion()
 *****************************************************************************
 *
 * @brief   Due to the fact that we need to complete the monitor packet, this
 *          method simply returns `success' when we initiate a job request
 *          from the monitor context.
 *
 * @param   packet_p - The packet sent to us from the scheduler.
 * @param   context - The pointer to the object.
 *
 * @return  fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  10/21/2009 - Created. Dhaval Patel
 *
 *****************************************************************************/
static fbe_status_t fbe_virtual_drive_initiate_copy_complete_or_abort_completion(fbe_packet_t * packet_p,
                                                                                 fbe_packet_completion_context_t context)
{
    fbe_virtual_drive_t                    *virtual_drive_p = (fbe_virtual_drive_t *)context;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_virtual_drive_flags_t               flags;
    fbe_spare_swap_command_t                swap_command;

    /* If enabled trace the `initiate' job acknowledgement.  This only means
     * the job has started.  It does not mean the job is complete.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &swap_command);
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: initiate copy complete or abort mode: %d flags: 0x%x cmd: %d - complete \n",
                            configuration_mode, flags, swap_command);

    /* By returning OK, the monitor packet will be completed to the next level. 
     */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_initiate_copy_complete_or_abort_completion()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_swap_out_edge_cond_function()
 ******************************************************************************
 * @brief
 *  This condition function is invoked when the job service has acknowledge the
 *  swap-out has completed.  This method waits for the swapped out edge to
 *  transition to `invalid'.
 *   
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is based on parent function call.
 *
 * @author
 *  04/02/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_swap_out_edge_cond_function(fbe_base_object_t * object_p,
                                                                            fbe_packet_t * packet_p)
{
    fbe_virtual_drive_t                    *virtual_drive_p = NULL;
    fbe_scheduler_hook_status_t             hook_status;
    fbe_edge_index_t                        swap_out_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_bool_t                              b_is_active = FBE_TRUE;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_virtual_drive_flags_t               flags;
    fbe_path_state_t                        swap_out_edge_path_state = FBE_PATH_STATE_INVALID;
    fbe_spare_swap_command_t                copy_request_type = FBE_SPARE_SWAP_INVALID_COMMAND;
    fbe_status_t                            status;

    /* cast the base object to a virtual drive object */
    virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)virtual_drive_p);
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &copy_request_type);
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    swap_out_edge_index = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);

    /* Get the current path state of the edges that was just swapped-out.
     */
    if ((swap_out_edge_index == FIRST_EDGE_INDEX)  ||
        (swap_out_edge_index == SECOND_EDGE_INDEX)    )
    {
        fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[swap_out_edge_index],
                                           &swap_out_edge_path_state);
    }

    /* This condition is called when the object is destroyed as well as
     * the swap in of a destination edge for a copy operation.  If it is
     * a copy operation trace it.
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS))
    {
        /* Trace swap-in for job request.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: swap out cond mode: %d life: %d active: %d cmd: %d flags: 0x%x swap: %d state: %d\n",
                              configuration_mode, my_state, b_is_active, 
                              copy_request_type, flags, swap_out_edge_index, swap_out_edge_path_state);
    }
    else
    {
        /* Else only trace if enabled
         */
        fbe_virtual_drive_trace(virtual_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                                "virtual_drive: swap out cond mode: %d life: %d active: %d cmd: %d flags: 0x%x swap: %d state: %d\n",
                                configuration_mode, my_state, b_is_active, 
                                copy_request_type, flags, swap_out_edge_index, swap_out_edge_path_state);
    }

    /* Validate the swap-out edge index.
     */
    if ((swap_out_edge_index != FIRST_EDGE_INDEX)  &&
        (swap_out_edge_index != SECOND_EDGE_INDEX)    )
    {
        /* Trace the error and clear this condition.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s invalid swap-out index: %d \n",
                              __FUNCTION__, swap_out_edge_index);
        
        /* Clear the condition and return done.
         */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_virtual_drive_swap_out_check_encryption_state(virtual_drive_p);
    if (status != FBE_STATUS_CONTINUE)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s waiting encryption keys to be freed\n",
                              __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Always clear the `hold in fail' flag.
     */
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_HOLD_IN_FAIL);

    /*! @note Wait for the swapped-out edge state to be:
     *  o Invalid (swap-out complete)
     *  o Enabled (drive came back or replacement drive inserted)
     * Currently `set condition based on downstream health' doesn't have 
     * knowledge of the transaction history.  Therefore is should not be invoked 
     * to process transitory states (for instance Enabled->Broken->Invalid).
     * Since we know that we have just swapped-out an edge wait for the edge 
     * to transition to `Invalid' or `Enabled' (drive came back or replacement
     * drive inserted)
     */
    if ((swap_out_edge_path_state != FBE_PATH_STATE_INVALID) &&
        (swap_out_edge_path_state != FBE_PATH_STATE_ENABLED)    )
    {
        /* Wait a short period and check again.
         */
        fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 100);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Check the debug hook.*/
    fbe_virtual_drive_check_hook(virtual_drive_p,
                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT, 
                              FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE, 
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Set the flag indicating that the swap-out edge is complete.
     */
    if ( fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS) &&
        !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CONFIRMATION_DISABLED)       )
    {
        fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_OUT_COMPLETE);
    }

    /* Clear this condition.
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);

    /* This condition is complete*/
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_out_edge_cond_function()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_copy_complete_cond_function()
 *****************************************************************************
 *
 * @brief   This condition is signalled when the copy operation has completed
 *          successfully and now we need to:
 *              o Change the virtual drive configuration mode to pass-thru
 *              o Swap-out the source drive
 *
 * @param   object_p - The ptr to the base object of the base config.
 * @param   packet_p - The packet sent to us from the scheduler.
 *
* @return   fbe_lifecycle_status_t - Lifecycle status.
 *
 * @author
 *  11/01/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_copy_complete_cond_function(fbe_base_object_t *object_p,
                                                                            fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_scheduler_hook_status_t             sched_status;
    fbe_virtual_drive_t                    *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_edge_index_t                        edge_index_to_swap_out = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                        remaining_edge_index = FBE_EDGE_INDEX_INVALID;

    /* Cast the base object to a virtual drive object */
    virtual_drive_p = (fbe_virtual_drive_t *)object_p;

    /* Update the configuration mode if it is not updated already. */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Get the appropriate edge index which needs a swap-out.
     */
    status = fbe_virtual_drive_swap_get_swap_out_edge_index(virtual_drive_p, &edge_index_to_swap_out);
    if (status != FBE_STATUS_OK)
    {
        /* Generate an error trace and clear this condition.
         */
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: find edge to swap-out failed with status: 0x%x\n", 
                              __FUNCTION__, status);

        /* Clear the condition and return done.
         */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy complete mode: %d life: %d swap-out edge index: %d\n",
                          configuration_mode, my_state, edge_index_to_swap_out);

    /* If the `set mode to pass-thru' hook is set pause.
     */
    fbe_virtual_drive_check_hook(virtual_drive_p,
                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                 FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE, 
                                 0, &sched_status);
    if (sched_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Invoke method to check if complete copy is allowed.
     */
    if (fbe_virtual_drive_swap_set_copy_complete_in_progress_if_allowed(virtual_drive_p) == FBE_FALSE)
    {
        /* Trace some information and wait for the next cycle.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: complete copy mode: %d life: %d swap-out edge index: %d denied\n",
                              configuration_mode, my_state, edge_index_to_swap_out);
        
        /* Leave the condition set and return done.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If we could not determine the edge index to swap-out we cannot complete
     * the copy request. Generate an error trace and clear the condition.
     */
    if (edge_index_to_swap_out == FBE_EDGE_INDEX_INVALID)
    {
        /* Generate an error trace.
         */
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: failed to locate swap-out edge\n", 
                              __FUNCTION__);

        /* Clear the condition and return done.
         */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Generate the `remaining' edge index.
     */
    remaining_edge_index = (edge_index_to_swap_out == FIRST_EDGE_INDEX) ? SECOND_EDGE_INDEX : FIRST_EDGE_INDEX;

    /* Generate an event log for the copy completion.
     */
    fbe_virtual_drive_logging_log_all_copies_complete(virtual_drive_p, packet_p, remaining_edge_index);

    /* Clear this condition since we will initiate the copy complete.
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);

    /* Set the master packet completion to the `initiate job' completion.
     */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_virtual_drive_initiate_copy_complete_or_abort_completion,
                                          virtual_drive_p);

    /* Initiate the copy completion job and use the monitor packet as the
     * master packet.
     */
    status = fbe_virtual_drive_initiate_swap_operation(virtual_drive_p,
                                                       packet_p,
                                                       edge_index_to_swap_out,
                                                       remaining_edge_index,
                                                       FBE_SPARE_COMPLETE_COPY_COMMAND);

    /* Clear the flag inidicating that the copy is complete.
     */
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_OPTIMAL_COMPLETE_COPY);

    /*! @note Currently we hold the monitor until the copy complete job is
     *        done (this is because we are use the monitor packet as the
     *        master to the job request packet).  But there is no need to
     *        hold the monitor packet since we place the job request packet
     *        on the terminator queue.
     */
    if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    else
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }
}
/******************************************************************************
 * end fbe_virtual_drive_copy_complete_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_abort_copy_cond_function()
 ******************************************************************************
 *
 * @brief   This condition is signalled when the copy operation needs to be
 *          aborted (due to either the source or destination failing).  We need
 *          to perform the following:
 *              o Change the virtual drive configuration mode to pass-thru
 *              o Swap-out the failed edge
 *
 * @param   object_p - The ptr to the base object of the base config.
 * @param   packet_p - The packet sent to us from the scheduler.
 *
 * @return  fbe_lifecycle_status_t - Lifecycle status.
 *
 * @author
 *  11/01/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_abort_copy_cond_function(fbe_base_object_t *object_p,
                                                                         fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_scheduler_hook_status_t             sched_status;
    fbe_virtual_drive_t                    *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_edge_index_t                        edge_index_to_swap_out = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                        remaining_edge_index = FBE_EDGE_INDEX_INVALID;

    /* Cast the base object to a virtual drive object */
    virtual_drive_p = (fbe_virtual_drive_t *)object_p;

    /* Update the configuration mode if it is not updated already. */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Get the appropriate edge index which needs a swap-out.
     */
    status = fbe_virtual_drive_swap_get_swap_out_edge_index(virtual_drive_p, &edge_index_to_swap_out);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: find edge to swap-out failed - status: 0x%x\n", 
                              __FUNCTION__, status);

        /* Clear the condition and return done.
         */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: abort copy mode: %d life: %d swap-out edge index: %d\n",
                          configuration_mode, my_state, edge_index_to_swap_out);

    /* If the `set mode to pass-thru' hook is set pause.
     */
    fbe_virtual_drive_check_hook(virtual_drive_p,
                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY, 
                                 FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE, 
                                 0, &sched_status);
    if (sched_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Invoke method to check if abort copy is allowed.
     */
    if (fbe_virtual_drive_swap_set_abort_copy_in_progress_if_allowed(virtual_drive_p) == FBE_FALSE)
    {
        /* Trace some information and wait for the next cycle.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: abort copy mode: %d life: %d swap-out edge index: %d denied\n",
                              configuration_mode, my_state, edge_index_to_swap_out);
        
        /* Leave the condition set and return done.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If we could not determine the edge index to swap-out we cannot complete
     * the copy request. Generate an error trace and clear the condition.
     */
    if (edge_index_to_swap_out == FBE_EDGE_INDEX_INVALID)
    {
        /* Generate an error trace.
         */
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: failed to locate swap-out edge\n", 
                              __FUNCTION__);

        /* Clear the condition and return done.
         */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Clear this condition since we will initiate the copy complete.
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);

    /* Set the master packet completion to the `initiate job' completion.
     */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_virtual_drive_initiate_copy_complete_or_abort_completion,
                                          virtual_drive_p);

    /* Generate the `remaining' edge index.
     */
    remaining_edge_index = (edge_index_to_swap_out == FIRST_EDGE_INDEX) ? SECOND_EDGE_INDEX : FIRST_EDGE_INDEX;
    status = fbe_virtual_drive_initiate_swap_operation(virtual_drive_p,
                                                       packet_p,
                                                       edge_index_to_swap_out,
                                                       remaining_edge_index,
                                                       FBE_SPARE_ABORT_COPY_COMMAND);

    /* Clear the flag inidicating that an abort copy is in progress.
     */
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_REQUEST_ABORT_COPY);

    /*! @note Currently we hold the monitor until the copy complete job is
     *        done (this is because we are use the monitor packet as the
     *        master to the job request packet).  But there is not need to
     *        hold the monitor packet since we place the job request packet
     *        on the terminator queue.
     */
    if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    else
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }
}
/***************************************************
 * end fbe_virtual_drive_abort_copy_cond_function()
 ***************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_swap_operation_complete_cond_function()
 ******************************************************************************
 *
 * @brief   This condition is signalled when the job service has completed
 *          any swap operation:
 *              o Permanent spare
 *              o Start Proactive copy
 *              o Start user copy
 *              o Complete copy operation
 *              o Abort copy operation
 *          The virtual drive must perform any cleanup and this must be done in
 *          the monitor context.
 *
 * @param   object_p - The ptr to the base object of the base config.
 * @param   packet_p - The packet sent to us from the scheduler.
 *
 * @return  fbe_lifecycle_status_t - Lifecycle status.
 *
 * @author
 *  04/26/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_swap_operation_complete_cond_function(fbe_base_object_t *object_p,
                                                                                      fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_t                    *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_spare_swap_command_t                swap_command;
    fbe_u32_t                               job_status;
    fbe_job_service_error_type_t            status_code;
    fbe_scheduler_hook_status_t             sched_status;

    /* Cast the base object to a virtual drive object */
    virtual_drive_p = (fbe_virtual_drive_t *)object_p;

    /* Get some debug information
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &swap_command);
    fbe_virtual_drive_get_job_status(virtual_drive_p, &job_status);
    status_code = (fbe_job_service_error_type_t)job_status;

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: swap operation complete cond mode: %d life: %d cmd: %d job status: %d\n",
                          configuration_mode, my_state, swap_command, status_code);

    /* If the `swap operation complete' hook is set stop here.
     */
    fbe_virtual_drive_check_hook(virtual_drive_p,
                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                 FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE, 
                                 0, &sched_status);
    if (sched_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If this is the first time invoke the method to cleanup.
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CLEANUP_SWAP_REQUEST))
    {
        status = fbe_virtual_drive_swap_completion_cleanup(virtual_drive_p, packet_p);
        if (status != FBE_STATUS_OK)
        {
            /* Trace and error message
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: swap operation complete cond mode: %d life: %d cmd: %d job status: %d failed - status: 0x%x\n",
                                  configuration_mode, my_state, swap_command, status_code, status);
        }
    }

    /* Wait for both the local and peer to clear the job-in-progress flag before
     * clearing the request in progress flag and this condition.
     */
    if (fbe_raid_group_is_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS)      ||
        fbe_raid_group_is_peer_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS)    )
    {
        /* Either the local and/or the peer still has the clustered 
         * job-in-progress flag set.  Wait for the flag to be cleared.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Clear the cleanup swap request flag.
     */
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CLEANUP_SWAP_REQUEST);

    /*! @note Do not clear the FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS
     *        flag here.  It can only be cleared when the job is totally done.
     */

    /* Clear this condition since we will have completed the swap request.
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
    return FBE_LIFECYCLE_STATUS_DONE;
}
/***************************************************************
 * end fbe_virtual_drive_swap_operation_complete_cond_function()
 ***************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_change_configuration_mode_cond_function()
 ******************************************************************************
 * @brief
 *  This is the condition function which is used to change the configuration
 *  mode of the virtual drive object when configuration service commit the
 *  transaction.
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  03/28/2013  Ron Proulx  - Updated.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t
fbe_virtual_drive_change_configuration_mode_cond_function(fbe_base_object_t * object_p,
                                                          fbe_packet_t * packet_p)
{
    fbe_status_t                            status;
    fbe_scheduler_hook_status_t             hook_status;
    fbe_virtual_drive_t                    *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_virtual_drive_configuration_mode_t  current_configuration_mode;
    fbe_virtual_drive_configuration_mode_t  new_configuration_mode;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_path_state_t                        first_edge_path_state;
    fbe_path_state_t                        second_edge_path_state;
    fbe_virtual_drive_flags_t               flags;
    fbe_bool_t                              b_is_active = FBE_TRUE;
 
    /* Cast the base object to a virtual drive object */
    virtual_drive_p = (fbe_virtual_drive_t*) object_p;

    /*! @note Most of the conditions below are only valid when in the Ready state.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *) virtual_drive_p, &my_state);

    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);

    /* Get the virtual drive flags
     */
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)virtual_drive_p);

    /* Get the current configuration mode */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &current_configuration_mode);

    /* Change the configuration mode to mirror. */
    fbe_virtual_drive_get_new_configuration_mode(virtual_drive_p, &new_configuration_mode);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: change config mode from: %d to: %d act: %d path state:[%d-%d] flags: 0x%x life: %d\n",
                          current_configuration_mode, new_configuration_mode, 
                          b_is_active, first_edge_path_state, second_edge_path_state, flags, my_state);

    /* If we hit the debug hook set the `update config mode' flag.
     */
    fbe_virtual_drive_check_hook(virtual_drive_p,
                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                 FBE_VIRTUAL_DRIVE_SUBSTATE_SET_CONFIG_MODE, 
                                 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Validate the new configuration mode. */
    if (new_configuration_mode != FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN)
    {  
        /* Base on the current and new configuration mode we need to set and/or
         * clear some conditions.  We should be quiesced by the time we are here.
         */
        if ((new_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)  ||
            (new_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)    )
        {
            /* Invoke the method that handle outstanding (quiesced) I/O etc.
             */
            status = fbe_virtual_drive_handle_config_change(virtual_drive_p);
            if (status == FBE_STATUS_PENDING)
            {
                /* Trace a message and re-schedule
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: change config mode from: %d to: %d wait for outstanding I/Os\n",
                                      current_configuration_mode, new_configuration_mode);

                /* Wait a short period
                 */
                fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 10);
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            /* Generate the rebuild end notification.
             */
            fbe_virtual_drive_copy_generate_notifications(virtual_drive_p, packet_p);

            /* Now we can change the configuration mode.
             */
            fbe_virtual_drive_set_configuration_mode(virtual_drive_p, new_configuration_mode);

            /* The handle case should have performed the cleanup.  If it failed
             * we proceed with the mode change since we have already acknowledged it.
             * 
            /* If the previous configuration mode was mirror we must:
             *  o Clear the copy condition
             *  o Clear the check copy complete condition
             */
            if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_UPDATE_CONFIG_MODE) ||
                (current_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)           ||
                (current_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)             )
            {
                /* If we hit the debug hook set the `update config mode' flag.
                 */
                fbe_virtual_drive_check_hook(virtual_drive_p,
                                             SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                             FBE_VIRTUAL_DRIVE_SUBSTATE_PASS_THRU_MODE_SET, 
                                             0, &hook_status);
                if (hook_status == FBE_SCHED_STATUS_DONE) 
                {
                    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_UPDATE_CONFIG_MODE);
                    return FBE_LIFECYCLE_STATUS_DONE;
                }

                /* clear the update configuration mode flag. */
                fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_UPDATE_CONFIG_MODE);

                /* Set the new configuration mode to `unknown'.
                 */
                fbe_virtual_drive_set_new_configuration_mode(virtual_drive_p, FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN);

                /* Clear any conditions that cannot or should not run.
                 */
                fbe_lifecycle_force_clear_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
                                               FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_EVAL_MARK_NR_IF_NEEDED_COND);
                fbe_lifecycle_force_clear_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
                                               FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_IS_COPY_COMPLETE);

                /* First send a swap-out notification to indicate that we have
                 * changed which drive represents this raid group position.
                 */
                fbe_virtual_drive_send_swap_notification(virtual_drive_p, new_configuration_mode);

                /* Now update the `original' pvd object id using the new 
                 * configuration mode.
                 */
                fbe_virtual_drive_swap_save_original_pvd_object_id(virtual_drive_p);

                /* Flag the `change config mode' as complete
                 */
                if ( fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS) &&
                    !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CONFIRMATION_DISABLED)       )
                {
                    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CHANGE_CONFIG_MODE_COMPLETE);
                }
            }
            else
            {
                /* Else we attempted to change the configuration mode multiple
                 * times.  This is unexpected.
                 */
                fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s new config: %d matches current: %d\n", 
                                      __FUNCTION__, new_configuration_mode, current_configuration_mode);
            }
        }
        else if ((new_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)  ||
                 (new_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)    )
        {
            /* Now we can change the configuration mode.
             */
            fbe_virtual_drive_set_configuration_mode(virtual_drive_p, new_configuration_mode);

            /* If the previous configuration mode was pas-thru we must:
             *  o Set the condition to eval mark needs rebuild
             */
            if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_UPDATE_CONFIG_MODE) ||
                (current_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)        ||
                (current_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)          )
            {
                /* Else if we are changing to a mirror mode trigger the rebuild 
                 * operation. 
                */
                fbe_virtual_drive_check_hook(virtual_drive_p,
                                  SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                  FBE_VIRTUAL_DRIVE_SUBSTATE_MIRROR_MODE_SET, 
                                  0, &hook_status);
                if (hook_status == FBE_SCHED_STATUS_DONE) 
                {
                    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_UPDATE_CONFIG_MODE);
                    return FBE_LIFECYCLE_STATUS_DONE;
                }

                /* clear the update configuration mode flag. */
                fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_UPDATE_CONFIG_MODE);

                /* Set the new configuration mode to `unknown'.
                 */
                fbe_virtual_drive_set_new_configuration_mode(virtual_drive_p, FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN);

                /* This will send a notification to the registered components(Admin-shim) about the swap */
                fbe_virtual_drive_send_swap_notification(virtual_drive_p, new_configuration_mode);              

                /* It is possible that a drive failed in between the time that the edge arrived and 
                 * we changed the config mode.  In this case the event for the drive lost would have 
                 * been ignored.  Evaluate health now and if needed set a condition to handle this. 
                 */
                if ((first_edge_path_state  != FBE_PATH_STATE_ENABLED) ||
                    (second_edge_path_state != FBE_PATH_STATE_ENABLED)    )
                {
                    /* Trace an informational message.
                     */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "virtual_drive: change config cond mode: %d path state:[%d-%d] not optimal evaluate health\n",
                                          new_configuration_mode, first_edge_path_state, second_edge_path_state);
    
                    /* Set the condition to evaluate downstream health which 
                     * follows this condition.
                     */
                    status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                                    (fbe_base_object_t *)virtual_drive_p,
                                                    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_EVALUATE_DOWNSTREAM_HEALTH);
                }

                /* If this is the active we must set the condition to evaluate
                 * mark NR since we will not clear rl until the mode has been
                 * changed to mirror.
                 */
                if (b_is_active)
                {
                    /* Flag the `change config mode' as complete
                     */
                    if ( fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS) &&
                        !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CONFIRMATION_DISABLED)       )
                    {
                        fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CHANGE_CONFIG_MODE_COMPLETE);
                    }
    
                    /* We have set the `mark NR required' non-paged flag to prevent
                     * the clearing of rl until we have set the configuration mode
                     * to mirror.  Now that the mode has been changed, set the
                     * condition to make sure the mark NR condition runs (if needed).
                     */
                    fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                           (fbe_base_object_t *)virtual_drive_p,
                                           FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_EVAL_MARK_NR_IF_NEEDED_COND);
                }
            }
            else
            {
                /* Else we attempted to change the configuration mode multiple
                 * times.  This is unexpected.
                 */
                fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s new config: %d matches current: %d\n", 
                                      __FUNCTION__, new_configuration_mode, current_configuration_mode);
            }
        }
        else
        {
            /* Else unsupported new configuration mode.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s new mode: %d not supported. current mode: %d\n", 
                                  __FUNCTION__, new_configuration_mode, current_configuration_mode);
        }
    }
    else
    {
        /* We never set this condition until configuration service has updated the
         * configuration mode.
         */
        fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s new configuration mode is unknown.\n", __FUNCTION__);
    }

    /* Success or failure we need to clear this condition.
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);

    /* We are done.
     */
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_virtual_drive_change_configuration_mode_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_is_copy_complete_cond_function()
 ******************************************************************************
 * @brief
 *  This is the condition function which will verify the current rebuild
 *  checkpoint to find out whether we have completed proactive copy
 *  operation or not.
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  10/21/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_is_copy_complete_cond_function(fbe_base_object_t * object_p,
                                                                               fbe_packet_t *packet_p)
{
    fbe_scheduler_hook_status_t                 hook_status;
    fbe_virtual_drive_t                        *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_virtual_drive_configuration_mode_t      configuration_mode;
    fbe_base_config_downstream_health_state_t   downstream_health;
    fbe_path_state_t                            first_edge_path_state;
    fbe_path_state_t                            second_edge_path_state;
    fbe_lba_t                                   first_edge_checkpoint;
    fbe_lba_t                                   second_edge_checkpoint;
    fbe_object_id_t                             original_pvd_object_id;
    fbe_object_id_t                             spare_pvd_object_id;
    fbe_bool_t                                  b_is_copy_complete;
    fbe_virtual_drive_flags_t                   flags;
    fbe_bool_t                                  b_is_mirror_mode;

    /* Get the configuration mode downstream path states and rebuild checkpoints.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p,  &configuration_mode);
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    downstream_health = fbe_virtual_drive_verify_downstream_health(virtual_drive_p);
    b_is_copy_complete = fbe_virtual_drive_is_copy_complete(virtual_drive_p);
    b_is_mirror_mode = fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p);

    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);

    /* Get the rebuild checkpoints.
     */
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t*)virtual_drive_p, FIRST_EDGE_INDEX, &first_edge_checkpoint);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t*)virtual_drive_p, SECOND_EDGE_INDEX, &second_edge_checkpoint); 

    /* Trace the information
     */
    if (first_edge_checkpoint == FBE_LBA_INVALID)
    {
        first_edge_checkpoint = FBE_U32_MAX;
    }
    if (second_edge_checkpoint == FBE_LBA_INVALID)
    {
        second_edge_checkpoint = FBE_U32_MAX;
    }
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: is copy complete: %d cond mode: %d flags: 0x%x path:[%d-%d] chkpt:[0x%llx-0x%llx]\n",
                          b_is_copy_complete, configuration_mode, flags,
                          first_edge_path_state, second_edge_path_state,
                          first_edge_checkpoint, second_edge_checkpoint);

    /*! @note The following condition must be meet to declare `copy complete':
     *          o The request must not be in progress
     *          o The copy must be complete
     *          o The downstream health must be optimal
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS))
    {
        /* There is a case where the downstream health checks if we need to 
         * complete the copy request (for instance reboot).  Always clear the
         * `copy complete' flag.
         */
        fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_OPTIMAL_COMPLETE_COPY);
    }
    else if ((b_is_copy_complete == FBE_TRUE)                     &&
             (downstream_health == FBE_DOWNSTREAM_HEALTH_OPTIMAL)    )
    {
        /* Check the debug hook. */
        fbe_virtual_drive_check_hook(virtual_drive_p,
                                  SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                  FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE, 
                                  0, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) 
        {
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* Log the event indicating that the copy operation has completed.
         */
        fbe_virtual_drive_event_logging_get_original_pvd_object_id(virtual_drive_p,
                                                                   &original_pvd_object_id,
                                                                   FBE_TRUE /* Copy is in-progress */);
        fbe_virtual_drive_event_logging_get_spare_pvd_object_id(virtual_drive_p,
                                                                &spare_pvd_object_id,
                                                                FBE_TRUE /* Copy is in-progress */);
        fbe_virtual_drive_write_event_log(virtual_drive_p,
                                          SEP_INFO_SPARE_COPY_OPERATION_COMPLETED,
                                          FBE_SPARE_INTERNAL_ERROR_NONE,
                                          original_pvd_object_id,
                                          spare_pvd_object_id,
                                          packet_p);
 
        /* Set the condition to complete the copy operation.
         */
        fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                               (fbe_base_object_t *)virtual_drive_p,
                               FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_COPY_COMPLETE);

    }   /* end if copy completed successfully */
    else if ((b_is_mirror_mode == FBE_TRUE)                                 &&
             fbe_virtual_drive_swap_is_primary_edge_broken(virtual_drive_p)    )
    {
        /* Else if the source drive failed set the condition to abort the copy
         * operation.
         */
        fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                               (fbe_base_object_t *)virtual_drive_p,
                               FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_ABORT_COPY);
    }
    else if ( (b_is_mirror_mode == FBE_TRUE)                                          &&
             ( fbe_virtual_drive_swap_is_secondary_edge_broken(virtual_drive_p)  ||
              !fbe_virtual_drive_swap_is_secondary_edge_healthy(virtual_drive_p)    )    )
    {
        /* Else if the destination drive failed set the condition to abort the
         * copy operation.
         */
        fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                               (fbe_base_object_t *)virtual_drive_p,
                               FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_ABORT_COPY);
    }

    /* Clear this condition.*/
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);

    /* return lifecycle status. */ 
    return FBE_LIFECYCLE_STATUS_DONE;
}
/********************************************************
 * end fbe_virtual_drive_is_copy_complete_cond_function()
 ********************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_quiesce_cond_function()
 ******************************************************************************
 * @brief
 *  This is the condition function which is used to quiesce the i/o for the
 *  virtual drive object.
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - lifecycle status.
 *
 * @author
 *  04/28/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t
fbe_virtual_drive_quiesce_cond_function(fbe_base_object_t * object_p, 
                                        fbe_packet_t * packet_p)
{
    fbe_virtual_drive_t    *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_lifecycle_status_t  lifecycle_status;
    fbe_bool_t              b_is_pass_thru_mode = FBE_TRUE; 
    
    b_is_pass_thru_mode = fbe_virtual_drive_is_pass_thru_configuration_mode_set(virtual_drive_p);
    lifecycle_status = fbe_raid_group_quiesce_cond_function((fbe_raid_group_t *)virtual_drive_p, packet_p,
                                                            b_is_pass_thru_mode /* Need to indicate if in pass-thru or not.*/);

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_QUIESCE_TRACING,
                            "%s - is pass-thru: %d raid group quiesce lifecycle_status: %d \n",
                            __FUNCTION__, b_is_pass_thru_mode, lifecycle_status);

    return lifecycle_status;
}
/******************************************************************************
 * end fbe_virtual_drive_quiesce_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_unquiesce_cond_function()
 ******************************************************************************
 * @brief
 *  This is the condition function which is used to unquiesce the i/o for the
 *  virtual drive object.
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - lifecycle status.  
 *
 * @author
 *  04/28/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_virtual_drive_unquiesce_cond_function(fbe_base_object_t * object_p, 
                                          fbe_packet_t * packet_p)
{
    fbe_virtual_drive_t * virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_lifecycle_status_t lifecycle_status;
    
    lifecycle_status = fbe_raid_group_unquiesce_cond_function((fbe_raid_group_t *)virtual_drive_p, packet_p);

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_QUIESCE_TRACING,
                            "%s - raid group unquiesce lifecycle_status: %d \n",
                            __FUNCTION__, lifecycle_status);

    return lifecycle_status;
}
/***************************************************
 * end fbe_virtual_drive_unquiesce_cond_function()
 ***************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_monitor_run_rebuild_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function for the do_rebuild condition.  It is invoked
 *   when a rebuild I/O operation has completed.  It reschedules the monitor to
 *   immediately run again in order to perform the next rebuild operation.
 *
 * @param packet_p               - pointer to a control packet from the scheduler
 * @param context                - context, which is a pointer to the base object 
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
static fbe_status_t fbe_virtual_drive_monitor_run_rebuild_completion(fbe_packet_t *packet_p,
                                                                     fbe_packet_completion_context_t in_context)
{
    fbe_virtual_drive_t                *virtual_drive_p;
    fbe_status_t                        status;                     //  fbe status
    fbe_payload_ex_t                   *sep_payload_p;              //  pointer to the payload
    fbe_payload_control_operation_t    *control_operation_p;        //  pointer to the control operation
    fbe_raid_group_rebuild_context_t   *needs_rebuild_context_p;    //  rebuild monitor's context information
    fbe_payload_control_status_t       control_status;
    fbe_time_t time_stamp;
    fbe_u32_t service_time_ms;

    //  Cast the context to a pointer to a virtual drive object 
    virtual_drive_p = (fbe_virtual_drive_t *)in_context;

    //  Get the sep payload and control operation
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    status = fbe_transport_get_status_code(packet_p);

    //  Release the payload buffer
    fbe_payload_control_get_buffer(control_operation_p, &needs_rebuild_context_p);

    /* If enabled trace some information.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_REBUILD_TRACING,
                            "virtual_drive: rebuild complete lba: 0x%llx blocks: 0x%llx state: %d status: 0x%x \n",
                            (unsigned long long)needs_rebuild_context_p->start_lba,
                            (unsigned long long)needs_rebuild_context_p->block_count,
                            needs_rebuild_context_p->current_state, status);

    fbe_payload_control_get_status(control_operation_p, &control_status);

    //  Release the context and control operation
    fbe_raid_group_rebuild_release_rebuild_context(packet_p);

    //  Check for a failure on the I/O.  If one occurred, trace and return here so the monitor is scheduled on its
    //  normal cycle.
    if ((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s rebuild failed, status: 0x%x\n", __FUNCTION__, status);

        //  Reschedule the next rebuild operation after a short delay
        fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p, 1000);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We keep the start time in the pdo time stamp since it is not used by raid.
     * The right thing to do is to have a "generic" time stamp. 
     */
    fbe_transport_get_physical_drive_io_stamp(packet_p, &time_stamp);
    service_time_ms = fbe_get_elapsed_milliseconds(time_stamp);
    if (service_time_ms > FBE_RAID_GROUP_MAX_IO_SERVICE_TIME)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Copy Completion: service time: %d Reschedule for: %d ms\n",
                              service_time_ms, FBE_RAID_GROUP_SLOW_IO_RESCHEDULE_TIME);
        /* Simply reschedule for a greater time if the I/O took longer than our threshold..
         */
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, 
                                 (fbe_base_object_t*) virtual_drive_p, 
                                 FBE_RAID_GROUP_SLOW_IO_RESCHEDULE_TIME);
        return FBE_STATUS_OK;
    }

    //  Reschedule the next rebuild operation after a short delay
    fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p, 100);

    //  Return success in all cases so the packet is completed to the next level 
    return FBE_STATUS_OK;

}
/********************************************************
 * end fbe_virtual_drive_monitor_run_rebuild_completion()
 ********************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_monitor_run_metadata_rebuild()
 ******************************************************************************
 * @brief
 *   This is the condition function to determine if a rebuild is able to be 
 *   performed.  If so, it will send a command to the executor to perform a 
 *   rebuild I/O and return a pending status.  The condition will remain set in
 *   that case.
 *   
 *   If a rebuild cannot be performed at this time, the condition will be cleared.  
 *
 * @param virtual_drive_p - Pointer to virtual drive object
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
static fbe_lifecycle_status_t fbe_virtual_drive_monitor_run_metadata_rebuild(fbe_virtual_drive_t *virtual_drive_p,
                                                                             fbe_packet_t *packet_p)
{
    fbe_scheduler_hook_status_t         status;
    fbe_bool_t                          can_rebuild_start_b;        //  true if rebuild can start
    fbe_raid_group_rebuild_context_t   *rebuild_context_p;          //  rebuild monitor's context information
    fbe_lba_t                           rebuild_metadata_lba;       //  current checkpoint
    fbe_u32_t                           lowest_rebuild_position;
    fbe_bool_t                          b_metadata_rebuild_enabled;
    
    //  Trace function entry 
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,    
                          FBE_TRACE_LEVEL_DEBUG_HIGH,         
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);        

    // See if metadata rebuild background operations is enabled to run
    fbe_base_config_is_background_operation_enabled((fbe_base_config_t *)virtual_drive_p, 
                                                    FBE_VIRTUAL_DRIVE_BACKGROUND_OP_METADATA_REBUILD, 
                                                    &b_metadata_rebuild_enabled);
    if( b_metadata_rebuild_enabled == FBE_FALSE ) 
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: run metadata rebuild: rebuild operation: 0x%x is disabled. \n",
                              FBE_VIRTUAL_DRIVE_BACKGROUND_OP_METADATA_REBUILD);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  See if we are allowed to do a rebuild I/O based on the current load and active/passive state.  If not,
    //  just return here and the monitor will reschedule on its normal cycle.
    fbe_raid_group_rebuild_determine_if_rebuild_can_start((fbe_raid_group_t *)virtual_drive_p, &can_rebuild_start_b);
    if(can_rebuild_start_b == FBE_FALSE)
    {  
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Check of the copy start hook
     */
    fbe_virtual_drive_check_hook(virtual_drive_p,  
                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                 FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START, 
                                 0, &status);
    if (status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  Get the next metadata lba which we need to rebuid, if all the chunks rebuild then clear the condition
    fbe_raid_group_rebuild_get_next_metadata_lba_to_rebuild((fbe_raid_group_t *)virtual_drive_p, 
                                                            &rebuild_metadata_lba, 
                                                            &lowest_rebuild_position);
    if (rebuild_metadata_lba == FBE_LBA_INVALID)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    /* Check of the metadata rebuild hook is set.
     */
    fbe_virtual_drive_check_hook(virtual_drive_p,  
                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD, 
                                 FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY, 
                                 rebuild_metadata_lba, &status);
    if (status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  Allocates control operation to track the rebuild operation
    fbe_raid_group_rebuild_allocate_rebuild_context((fbe_raid_group_t *)virtual_drive_p, packet_p, &rebuild_context_p);
    if (rebuild_context_p == NULL)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_REBUILD_TRACING,
                            "virtual_drive: run metadata rebuild: lowest rebuild pos: %d metadata lba: 0x%llx \n",
                            lowest_rebuild_position,
                (unsigned long long)rebuild_metadata_lba);

    //  Initialize rebuild context before we start the rebuild opreation
    fbe_virtual_drive_copy_initialize_rebuild_context(virtual_drive_p,
                                                      rebuild_context_p,
                                                      FBE_RAID_POSITION_BITMASK_ALL, /*! @note Position is irrelevant for metadata rebuild. */         
                                                      rebuild_metadata_lba);

    /* Set time stamp so we can use it to detect slow I/O.
     */
    fbe_transport_set_physical_drive_io_stamp(packet_p, fbe_get_time());

    //  Set the completion function for the rebuild condition
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_virtual_drive_monitor_run_rebuild_completion,
                                          virtual_drive_p);

    /* If enabled trace some information.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_REBUILD_TRACING,
                            "virtual_drive: rebuild metadata start lba: 0x%llx blocks: 0x%llx state: %d \n",
                            (unsigned long long)rebuild_context_p->start_lba,
                (unsigned long long)rebuild_context_p->block_count,
                            rebuild_context_p->current_state);

    // Send the rebuild I/O .  Its completion function will execute when it finishes. 
    fbe_virtual_drive_copy_send_io_request(virtual_drive_p, packet_p, 
                                           FBE_FALSE, /* Don't need to break the context. */
                                           rebuild_context_p->start_lba, 
                                           rebuild_context_p->block_count);

    /* We have send a metadata rebuild request.
     */
    return FBE_LIFECYCLE_STATUS_PENDING;

}
/********************************************************
 * end fbe_virtual_drive_monitor_run_metadata_rebuild()
 ********************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_monitor_run_rebuild()
 ******************************************************************************
 * @brief
 *   This function determines if a rebuild is able to be 
 *   performed.  If so, it will send a command to the executor to perform a 
 *   rebuild I/O and return a pending status.
 *   
 *
 * @param virtual_drive_p - Pointer to virtual drive object
 * @param packet_p  - Pointer to a control packet from the scheduler
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
static fbe_lifecycle_status_t fbe_virtual_drive_monitor_run_rebuild(fbe_virtual_drive_t *virtual_drive_p,
                                                                    fbe_packet_t *packet_p)
{

    fbe_raid_position_bitmask_t         complete_rebuild_positions = 0; //  positions which needs its rebuild "completed"
    fbe_raid_position_bitmask_t         positions_to_rebuild = 0;   //  bitmask of positions thre will be rebuilt (now) 
    fbe_raid_position_bitmask_t         all_degraded_positions = 0; //  bitmask of all degraded positions
    fbe_bool_t                          can_rebuild_start_b;
    fbe_raid_group_rebuild_context_t   *rebuild_context_p = NULL;
    fbe_lba_t                           checkpoint;
    fbe_scheduler_hook_status_t         status = FBE_SCHED_STATUS_OK;
    fbe_bool_t                          b_rebuild_enabled;
    fbe_bool_t                          b_checkpoint_reset_needed;
    fbe_medic_action_priority_t         ds_medic_priority;
    fbe_medic_action_priority_t         rg_medic_priority;

    /* Trace the function entry.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,    
                          FBE_TRACE_LEVEL_DEBUG_HIGH,         
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);        

    /* If if copy operations are enabled or not.
     */
    fbe_base_config_is_background_operation_enabled((fbe_base_config_t *)virtual_drive_p, 
                                                    FBE_VIRTUAL_DRIVE_BACKGROUND_OP_COPY, 
                                                    &b_rebuild_enabled);
    if( b_rebuild_enabled == FBE_FALSE )
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: run rebuild: copy operation: 0x%x is disabled \n",
                              FBE_VIRTUAL_DRIVE_BACKGROUND_OP_COPY);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    ds_medic_priority = fbe_base_config_get_downstream_edge_priority((fbe_base_config_t*)virtual_drive_p);
    fbe_virtual_drive_get_medic_action_priority(virtual_drive_p, &rg_medic_priority);

    if (ds_medic_priority > rg_medic_priority)
    {
        fbe_virtual_drive_check_hook(virtual_drive_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, 
                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_DOWNSTREAM_PERMISSION, 0,  &status);
        /* No hook status is handled since we can only log here.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "vd_monitor_run_rebuild downstream medic priority %d > rg medic priority %d\n",
                              ds_medic_priority, rg_medic_priority);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /*! @note Get the bitmasks for all degraded positions, positions that need
     *        `rebuild complete' and positions that we are going to rebuild in
     *        this request (must have the same rebuild checkpoint).
     */
    fbe_raid_group_rebuild_find_disk_needing_action((fbe_raid_group_t *)virtual_drive_p,
                                                    &all_degraded_positions,
                                                    &positions_to_rebuild,
                                                    &complete_rebuild_positions,
                                                    &checkpoint,
                                                    &b_checkpoint_reset_needed);

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_REBUILD_TRACING,
                            "virtual_drive: run rebuild: degraded: 0x%x rebuild: 0x%x complete: 0x%x checkpoint: 0x%llx \n",
                            all_degraded_positions, positions_to_rebuild, complete_rebuild_positions, (unsigned long long)checkpoint);
    
    /* Put a hook before we reset the checkpoint
     */
    fbe_virtual_drive_check_hook(virtual_drive_p,
                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                 FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_USER_DATA_START, 
                                 0, &status);
    if (status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if (b_checkpoint_reset_needed == FBE_TRUE )
    {
        /* Trace the transition from metadata to user data.
         */
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: reset rebuild checkpoint to position 0x%x\n" , 
                              positions_to_rebuild);
        fbe_raid_group_set_rebuild_checkpoint_to_zero((fbe_raid_group_t *)virtual_drive_p, 
                                                      packet_p, 
                                                      positions_to_rebuild);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    //  Check if a disk was found that needs to complete its rebuild by moving the checkpoint.  Set the condition 
    //  to do so and return. 
    if (complete_rebuild_positions != 0) 
    {
        /* Trace this transition.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: run rebuild checkpoint 0x%llx complete rebuild positions: 0x%x\n" , 
                              (unsigned long long)checkpoint,
                              complete_rebuild_positions);

        /*! @note We never clear the background operation condition.
         */
     
        /* Set the condition to determine if the copy is complete
         */
        fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 
                               FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_IS_COPY_COMPLETE);
        return FBE_LIFECYCLE_STATUS_DONE; 
    }

    /*! @todo If no drives are ready to be rebuilt, then clear the do_rebuild 
     *        rotary condition and return reschedule.
     *
     *  @note A drive may still have chunks marked NR, but if the edge is 
     *        unavailable, it cannot rebuild now.  The condition is cleared in 
     *        this case.  It will be set again when the disk becomes enabled.
     */
    if (positions_to_rebuild == 0)
    {
        /*! @note We never clear the background operation condition.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* See if we are allowed to do a rebuild I/O based on the current load 
     * and active/passive state.  If we can't start return `done'
     * without clearing the condition.
     */
    fbe_raid_group_rebuild_determine_if_rebuild_can_start((fbe_raid_group_t *)virtual_drive_p, &can_rebuild_start_b);
    if (can_rebuild_start_b == FBE_FALSE)
    {
        /* Check the debug hook.*/
        fbe_virtual_drive_check_hook(virtual_drive_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, 
                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_PERMISSION, 
                                  0, &status);
        if (status == FBE_SCHED_STATUS_DONE) 
        {
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* We were not granted permission.  Wait the normal monitor cycle to see 
         * if the load changes.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Else rebuild is able to run.  Check the debug hooks.
     */
    fbe_virtual_drive_check_hook(virtual_drive_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY, checkpoint, &status);
    if (status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

    if (fbe_raid_group_is_singly_degraded((fbe_raid_group_t *)virtual_drive_p))
    {
        fbe_virtual_drive_check_hook(virtual_drive_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, FBE_RAID_GROUP_SUBSTATE_REBUILD_SINGLY_DEGRADED, checkpoint, &status);
        if (status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}
    }

    /* Allocate the rebuild context.
     */
    fbe_raid_group_rebuild_allocate_rebuild_context((fbe_raid_group_t *)virtual_drive_p, packet_p, &rebuild_context_p);
    if (rebuild_context_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: run rebuild: failed to allocate rebuild context \n");
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Initialize the rebuild context.
     */
    fbe_virtual_drive_copy_initialize_rebuild_context(virtual_drive_p, 
                                                      rebuild_context_p,
                                                      positions_to_rebuild,
                                                      checkpoint);
    /* Set time stamp so we can use it to detect slow I/O.
     */
    fbe_transport_set_physical_drive_io_stamp(packet_p, fbe_get_time());

    //  Set the completion function for the rebuild condition
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_virtual_drive_monitor_run_rebuild_completion,
                                          virtual_drive_p);

    /* If enabled trace some information.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_REBUILD_TRACING,
                            "virtual_drive: rebuild start lba: 0x%llx blocks: 0x%llx state: %d \n",
                            (unsigned long long)rebuild_context_p->start_lba,
                (unsigned long long)rebuild_context_p->block_count,
                            rebuild_context_p->current_state);

    /* Send the event to check if this space is consumed or not.
     */
    fbe_virtual_drive_copy_send_event_to_check_lba(virtual_drive_p,
                                                   rebuild_context_p,
                                                   packet_p);

    return FBE_LIFECYCLE_STATUS_PENDING;

}
/*********************************************
 * end fbe_virtual_drive_monitor_run_rebuild()
 *********************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_generate_error_trace()
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
static fbe_status_t fbe_virtual_drive_generate_error_trace(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_raid_group_t               *raid_group_p = (fbe_raid_group_t *)virtual_drive_p;
    fbe_object_id_t                 vd_object_id = fbe_raid_group_get_object_id(raid_group_p);
    fbe_raid_group_debug_flags_t    debug_flags = 0;

    /* Get the current debug flags
     */
    fbe_raid_group_get_debug_flags(raid_group_p, &debug_flags);

    /* Now generate the error trace.
     */
    fbe_virtual_drive_trace(virtual_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_VIRTUAL_DRIVE_DEBUG_FLAG_NONE,
                            "virtual_drive: Generate error trace requested for vd obj: 0x%x (%d)\n",
                            vd_object_id, vd_object_id);

    /* This is a `one shot'.  Clear the flag.
     */
    debug_flags &= ~FBE_RAID_GROUP_DEBUG_FLAGS_GENERATE_ERROR_TRACE;
    fbe_raid_group_set_debug_flags(raid_group_p, debug_flags);

    return FBE_STATUS_OK;

}
/**************************************************
 * end fbe_virtual_drive_generate_error_trace()
 **************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_background_monitor_operation_cond_function()
 *****************************************************************************
 *
 * @brief   This is the background operation condition for the virtual drive.
 *          This condition is never cleared and runs after all other conditions
 *          in the Ready rotary.  In most cases the virtual drive object will
 *          be in pass-thru mode and therefore will have no background operation
 *          to execute.  Currently the only background operation the virtual
 *          drive object supports is the copy operation.
 *
 * @param   object_p - The ptr to the base object of the base config.
 * @param   packet_p - The packet sent to us from the scheduler.
 *
 * @return  fbe_lifecycle_status_t - In most cases this method returns
 *              FBE_LIFECYCLE_STATUS_DONE.
 *
 * @note    This condition should never be cleared.
 *
 * @author
 *  03/7/2011 - Created. Ashwin Tamilarasan.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_background_monitor_operation_cond_function(fbe_base_object_t * object_p,
                                                                      fbe_packet_t * packet_p)
{
    fbe_status_t                                status;
    fbe_virtual_drive_t                        *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_lifecycle_status_t                      lifecycle_status;           
    fbe_lifecycle_state_t                       lifecycle_state;
    fbe_power_save_state_t                      power_save_state = FBE_POWER_SAVE_STATE_INVALID;
    fbe_bool_t                                  b_is_active;
    fbe_virtual_drive_configuration_mode_t      configuration_mode;
    fbe_virtual_drive_flags_t                   flags;
    fbe_base_config_downstream_health_state_t   downstream_health;
    fbe_medic_action_priority_t                 rg_medic_priority;    
    fbe_medic_action_priority_t                 ds_medic_priority;   

    /*! @note 01-Oct-12: 
     *   Remove this temporary until the interaction between VD and job services is fixed. See Ron P. before putting 
     *   this back again. 
     *
    if (fbe_base_config_is_active((fbe_base_config_t*)virtual_drive_p) &&
        fbe_raid_group_is_peer_present((fbe_raid_group_t *)virtual_drive_p) &&
        fbe_raid_group_is_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_REQ) &&
        !fbe_raid_group_is_peer_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_REQ))
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s send passive request\n", 
                              __FUNCTION__);
        fbe_base_config_passive_request((fbe_base_config_t *) virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    */

    /* The code below is structured similar to the raid group class but copy
     * operations have different handling.
     */

    /* If the `generate error trace' flag is set generate an error trace
     */
    if (fbe_raid_group_is_debug_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_DEBUG_FLAGS_GENERATE_ERROR_TRACE))
    {
        fbe_virtual_drive_generate_error_trace(virtual_drive_p);
    }

    /* Currently the only background operation(s) for the virtual drive are:
     *      o copy
     * Independent of the type of background operation, there is no need to
     * run a background operation if the virtual drive is in `pass-thru' mode.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    downstream_health = fbe_virtual_drive_verify_downstream_health(virtual_drive_p);
    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)virtual_drive_p);
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);

    /* Get the virtual drive config mode, state and downstream health.
     */
    status = fbe_lifecycle_get_state(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, &lifecycle_state);
    if (status != FBE_STATUS_OK) 
    {
        /* We cannot modify the checkpoint etc. unless we are in the ready state.
         */
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s Critical error fbe_lifecycle_get_state failed, status = 0x%x", __FUNCTION__, 
                                 status);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Invoke the method to generate the proper notifications if:
     *  o Virtual drive is in mirror mode (checked above)
     *                  AND
     *  o Virtual drive is Ready
     *                  AND
     *      + Downstream health is optimal
     *                  OR
     *      + Copy is complete
     */
    if (lifecycle_state == FBE_LIFECYCLE_STATE_READY)
    {
        if ((downstream_health == FBE_DOWNSTREAM_HEALTH_OPTIMAL)                                         ||
            (virtual_drive_p->spare.raid_group.rebuilt_pvds[FIRST_EDGE_INDEX] != FBE_OBJECT_ID_INVALID)  ||
            (virtual_drive_p->spare.raid_group.rebuilt_pvds[SECOND_EDGE_INDEX] != FBE_OBJECT_ID_INVALID)    )
        {
            /* We cannot allow notification if a swap request is in progress.
             */ 
            if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS))
            {
                /*! @note We need to be here BEFORE any rebuild activity so we catch 
                 *        the fact the previous rebuild is done and we send notification 
                 *        for it. Otherwise, or cached PVDs we rebuilt will not be 
                 *        correct.
                 */
                fbe_virtual_drive_copy_generate_notifications(virtual_drive_p, packet_p);
            }
        }
    }

    /* We should not run the background condition if we are quiesced or quiescing. 
     * The Block Transport Server is held in this case so issuing a 
     * monitor operation will cause the monitor operation to be held, and since 
     * we only unhold in the monitor context, we will never be able to unhold. 
     */
    if (fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t *)virtual_drive_p, 
                    (FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED|FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING))) 
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: background op - quiescing.\n");

        /* Just return done.  Since we haven't cleared the condition we will be re-scheduled.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* We must Ready to execute a background operaiton.
     */
    if (lifecycle_state != FBE_LIFECYCLE_STATE_READY)
    {
        /* Report a warning but continue.
         */
        fbe_base_object_trace(object_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s lifecycle_state: %d is not ready: %d\n", 
                              __FUNCTION__, lifecycle_state, FBE_LIFECYCLE_STATE_READY);
    }

    /* If there is a swap request in progress reschedule
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS))
    {
        /* If enabled trace.
         */
        fbe_virtual_drive_trace(virtual_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                                "virtual_drive: background op mode: %d flags: 0x%x health: %d swap in progress - wait\n",
                                configuration_mode, flags, downstream_health);

        /* Wait a short period and check again.
         */
        fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 100);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else if (downstream_health != FBE_DOWNSTREAM_HEALTH_OPTIMAL)
    {
        /* Trace some information and wait until we are optimal.
         */
        fbe_virtual_drive_trace(virtual_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_VIRTUAL_DRIVE_DEBUG_FLAG_HEALTH_TRACING,
                                "virtual_drive: background op mode: %d flags: 0x%x health: %d is not optimal\n",
                                configuration_mode, flags, downstream_health);
    }

    /* Handle download request.
     */
    fbe_virtual_drive_eval_download_request(virtual_drive_p, packet_p);

    /*! @note Rebuild metadata first.  Currently we can use the raid group
     *        method for checking the metadata.
     */
    if (fbe_raid_group_background_op_is_metadata_rebuild_need_to_run((fbe_raid_group_t *)virtual_drive_p))
    {
        /* Copy is running update the upsteam edge priority.
         */
        status = fbe_base_config_set_upstream_edge_priority((fbe_base_config_t *)virtual_drive_p, FBE_MEDIC_ACTION_COPY);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: set upstream action to copy failed. status: 0x%x\n",
                                  __FUNCTION__, status);
        }

        /* If we are healthy, optimal and active run the copy request.
         */
        if ((lifecycle_state == FBE_LIFECYCLE_STATE_READY)       && 
            (downstream_health == FBE_DOWNSTREAM_HEALTH_OPTIMAL) &&
            (b_is_active == FBE_TRUE)                               )
        {
            /* run rebuild background operation */
            lifecycle_status = fbe_virtual_drive_monitor_run_metadata_rebuild(virtual_drive_p, packet_p);
    
            /* Trace some information if enabled.
             */
            fbe_virtual_drive_trace(virtual_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_REBUILD_TRACING,
                                    "virtual_drive: background op - metadata rebuild mode: %d health: %d life status: %d \n",
                                    configuration_mode, downstream_health, lifecycle_status);
            return lifecycle_status;
        }
    }

    /* check if rebuild background operation needs to run */
    if (fbe_raid_group_background_op_is_rebuild_need_to_run((fbe_raid_group_t *)virtual_drive_p))
    {
        /* Copy is running update the upsteam edge priority.
         */
        status = fbe_base_config_set_upstream_edge_priority((fbe_base_config_t *)virtual_drive_p, FBE_MEDIC_ACTION_COPY);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: set upstream action to copy failed. status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        /* If we are healthy, optimal and active run the copy request.
         */
        if ((lifecycle_state == FBE_LIFECYCLE_STATE_READY)       && 
            (downstream_health == FBE_DOWNSTREAM_HEALTH_OPTIMAL) &&
            (b_is_active == FBE_TRUE)                               )
        {
            /* run rebuild background operation */
            lifecycle_status = fbe_virtual_drive_monitor_run_rebuild(virtual_drive_p, packet_p);
    
            /* Trace some information if enabled.
             */
            fbe_virtual_drive_trace(virtual_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_REBUILD_TRACING,
                                    "virtual_drive: background op - rebuild mode: %d health: %d life status: %d \n",
                                    configuration_mode, downstream_health, lifecycle_status);
            return lifecycle_status;
        }
    }

    /* Update to the downstream priority
     */
    ds_medic_priority = fbe_base_config_get_downstream_edge_priority((fbe_base_config_t*)virtual_drive_p);
    fbe_virtual_drive_get_medic_action_priority(virtual_drive_p, &rg_medic_priority);
    if (ds_medic_priority > rg_medic_priority)
    {
        status = fbe_base_config_set_upstream_edge_priority((fbe_base_config_t*)virtual_drive_p, ds_medic_priority);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s set upstream from ds: %d failed - status: 0x%x\n",
                              __FUNCTION__, ds_medic_priority, status);
        }
    }
    else
    {
        status = fbe_base_config_set_upstream_edge_priority((fbe_base_config_t*)virtual_drive_p, rg_medic_priority);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s set upstream from rg: %d failed - status: 0x%x\n",
                              __FUNCTION__, rg_medic_priority, status);
        }
    }

    /* If we are in pass-thry handle power-save
     */
    if (fbe_virtual_drive_is_pass_thru_configuration_mode_set(virtual_drive_p))
    {
        /* Lock base config before getting power saving state */
        fbe_base_config_lock((fbe_base_config_t *)virtual_drive_p);

        /*! @note We only allow power savings if we are in pass-thru mode.
         */
        fbe_base_config_metadata_get_power_save_state((fbe_base_config_t *)virtual_drive_p, &power_save_state);

        /* Unlock base config before getting power saving state */
        fbe_base_config_unlock((fbe_base_config_t *)virtual_drive_p);

        if ((!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS)) &&
            (power_save_state != FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE))
        {
            base_config_check_power_save_cond_function(object_p, packet_p);
        }

        /*! @note We no longer exit the condition since we must change the
         *        medic priority, generate notifications etc below. 
         */
    }

    /* We are done with this condition.
     */
    return FBE_LIFECYCLE_STATUS_DONE;
}
/*********************************************************************
 * end fbe_virtual_drive_background_monitor_operation_cond_function()
 *********************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_quiesce_io_if_needed()
 ******************************************************************************
 * @brief
 *   This function will check if I/O has been quiesced on the raid group.  If it
 *   has not, then it will set the quiesce I/O condition and return a status to 
 *   indicate that it must wait for the quiesce.  It will also set the unquiesce
 *   condition.  The unquiesce will only run after all of the other higher 
 *   priority conditions such as eval RL have completed.  
 * 
 * @param   virtual_drive_p - Pointer to virtual drive object
 *
 * @return  fbe_status_t  
 *
 * @author
 *   02/24/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_quiesce_io_if_needed(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_bool_t              peer_alive_b;
    fbe_bool_t              local_quiesced;
    fbe_bool_t              peer_quiesced;
    fbe_lifecycle_state_t   my_state;

    //  Lock the virtual drive object before checking the quiesced flag
    fbe_virtual_drive_lock(virtual_drive_p);

    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    peer_alive_b = fbe_base_config_has_peer_joined((fbe_base_config_t *)virtual_drive_p);
    local_quiesced = fbe_base_config_is_clustered_flag_set((fbe_base_config_t *)virtual_drive_p, 
                                                           FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED);
    peer_quiesced = fbe_base_config_is_any_peer_clustered_flag_set((fbe_base_config_t *)virtual_drive_p, 
                                                               (FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED|
                                                                FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_COMPLETE));

    //  If the RAID group is already quiesced, we don't need to do anything more - return with normal status
    //  after unlocking the raid group
    //  We also need to check peer side to make sure both sides are quiesced, if peer is alive
    //  Quiesce only occurs in the Ready state !!
    if ((my_state != FBE_LIFECYCLE_STATE_READY) ||
        ((local_quiesced == FBE_TRUE)     &&
         ((peer_alive_b != FBE_TRUE)  || 
          (peer_quiesced == FBE_TRUE)    )   )      )
    {
        fbe_virtual_drive_unlock(virtual_drive_p);
        return FBE_STATUS_OK; 
    }

    //  Unlock the virtual drive 
    fbe_virtual_drive_unlock(virtual_drive_p);

    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s set quiesce condition\n", __FUNCTION__);

    /* Set a condition to quiese.
     */
    fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
                           FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE);


    /*! @note The quiesce condition now automatically sets the unquiesce 
     *        condition !
     */
    //fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
    //                       FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE);
    
    /*! @todo Not clear if this is needed for the virtual drive copy. 
     * Next we clear out an error attributes marked on downstream edges, which are holding our view of the drive state 
     * as bad. 
     */
    //fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) in_raid_group_p,
    //              FBE_RAID_GROUP_LIFECYCLE_COND_CLEAR_ERRORS);

    /* Return pending.  This signifies that the caller needs to wait for the 
     * quiesce to complete before proceeding.
     */ 
    return FBE_STATUS_PENDING; 

}
/***********************************************
 * end fbe_virtual_drive_quiesce_io_if_needed()
 ***********************************************/

/*!****************************************************************************
 * fbe_virtual_drive_copy_complete_set_rebuild_checkpoint_to_end_marker_completion()
 ******************************************************************************
 * @brief
 *   This function is called when the non-paged metadata write to set the 
 *   rebuild checkpoint(s) to the logical end marker has completed.  It will 
 *   check the packet status and clear the condition. 
 * 
 * @param   packet_p   - pointer to a control packet from the scheduler
 * @param   in_context - completion context, which is a pointer to the virtual
 *              drive object.
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  04/13/2013  Ron Proulx  - Updated.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_copy_complete_set_rebuild_checkpoint_to_end_marker_completion(
                                                                    fbe_packet_t *packet_p,
                                                                    fbe_packet_completion_context_t in_context)

{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_virtual_drive_t                *virtual_drive_p;
    fbe_status_t                        packet_status;
    fbe_edge_index_t                    swapped_out_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_bool_t                          b_is_edge_swapped = FBE_TRUE;
    fbe_bool_t                          b_is_mark_nr_required = FBE_FALSE;
    fbe_bool_t                          b_is_source_failed = FBE_FALSE;
    fbe_bool_t                          b_is_degraded_needs_rebuild = FBE_FALSE;
    fbe_raid_group_np_metadata_flags_t  raid_group_np_flags_to_clear;
    fbe_base_config_downstream_health_state_t downstream_health_state;

    /* Get the virtual drive object.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)in_context;
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    swapped_out_edge_index = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);
    fbe_virtual_drive_metadata_is_edge_swapped(virtual_drive_p, &b_is_edge_swapped);
    fbe_virtual_drive_metadata_is_mark_nr_required(virtual_drive_p, &b_is_mark_nr_required);
    fbe_virtual_drive_metadata_is_source_failed(virtual_drive_p, &b_is_source_failed);
    fbe_virtual_drive_metadata_is_degraded_needs_rebuild(virtual_drive_p, &b_is_degraded_needs_rebuild);

    /* Trace some information
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy complete set chkpt to end mode: %d swapped: %d source failed: %d\n",
                          configuration_mode, b_is_edge_swapped, b_is_source_failed);

    /* Check the packet status.  If the write had an error, then we'll leave 
     * the condition set so that we can try again.  Return success so that the 
     * next completion function on the stack gets called.
     */ 
    packet_status = fbe_transport_get_status_code(packet_p);
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s failed - status: 0x%x\n", 
                              __FUNCTION__, packet_status);
        return FBE_STATUS_OK;
    }

    /* If we have successfully set the checkpoints, validate them.
     */
    status = fbe_virtual_drive_copy_swap_out_validate_nonpaged_metadata(virtual_drive_p,
                                                                        swapped_out_edge_index);
    if (status != FBE_STATUS_OK)
    {
        /* Trace the error and return.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s validate nonpaged failed - status: 0x%x \n",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* Determine if we need to clear the swapped bit or not.
     */
    fbe_virtual_drive_metadata_determine_nonpaged_flags(virtual_drive_p,
                                                        &raid_group_np_flags_to_clear,
                                                        b_is_edge_swapped,  /* If edge swapped is set we want to clear it */
                                                        FBE_FALSE,          /* If edge swpped is set we want to clear it */
                                                        b_is_mark_nr_required,  /* If mark nr required is set we want to clear it */
                                                        FBE_FALSE,          /* If mark nr required is set we want to clear it */
                                                        b_is_source_failed, /* If source failed is set we want to clear it */
                                                        FBE_FALSE,          /* If source failed is set we want to clear it */
                                                        b_is_degraded_needs_rebuild, /* If degraded needs rebuild is set clear it */
                                                        FBE_FALSE           /* If degraded needs rebuild is set clear it */ );  

    /* Now write the updated non-paged flags.
     */
    status = fbe_virtual_drive_metadata_write_nonpaged_flags(virtual_drive_p,
                                                             packet_p,
                                                             raid_group_np_flags_to_clear);
    if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s edge swapped: %d source failed: %d write failed - status: 0x%x\n",
                              __FUNCTION__, b_is_edge_swapped, b_is_source_failed, status);
    
        /* Fail the request.
         */
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* Only clear this condition if the checkpoints were properly set.
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t*)virtual_drive_p);

    /* Mark the request as complete
     */
    if ( fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS) &&
        !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CONFIRMATION_DISABLED)       )
    {
        fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_COMPLETE_SET_CHECKPOINT);
    }

    /* Determine the health of the path with downstream objects. */
    downstream_health_state = fbe_virtual_drive_verify_downstream_health(virtual_drive_p);

    /* Set the specific lifecycle condition based on health of the path with downstream objects. */
    fbe_virtual_drive_set_condition_based_on_downstream_health(virtual_drive_p, downstream_health_state, __FUNCTION__);

    /* Return more processing required.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/***************************************************************************************
 * end fbe_virtual_drive_copy_complete_set_rebuild_checkpoint_to_end_marker_completion()
 ***************************************************************************************/ 

/*!****************************************************************************
 * fbe_virtual_drive_copy_complete_set_rebuild_checkpoint_to_end_marker_cond_function()
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
 *
 * @note    This method will clear the swapped or checkpoint from either the
 *          passive or active side.
 *
 * @author
 *  09/27/2012  Ron Proulx  - Updated.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_copy_complete_set_rebuild_checkpoint_to_end_marker_cond_function(
                                fbe_base_object_t*      in_object_p,
                                fbe_packet_t*           in_packet_p)
{
    fbe_status_t                            status;
    fbe_virtual_drive_t                    *virtual_drive_p; 
    fbe_edge_index_t                        edge_index_swapped_out = FBE_EDGE_INDEX_INVALID;           
    fbe_bool_t                              b_is_active; 
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_scheduler_hook_status_t             hook_status;

    /* Get the virtual drive object.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)in_object_p;
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)virtual_drive_p);

    /* Trace the function entry
     */
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,     
                          FBE_TRACE_LEVEL_DEBUG_HIGH,          
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                          "%s entry\n", __FUNCTION__);         

    /* Get the configuration mode.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Get the edge that was just swapped out.
     */
    edge_index_swapped_out = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy complete set chkpt mode: %d active: %d swapped out index: %d \n",
                          configuration_mode, b_is_active, edge_index_swapped_out);

    /* If this is not the active clear the condition.
     */
    if (b_is_active == FBE_FALSE)
    {
        /* This is unexpected.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s condition should only be set on active\n",
                              __FUNCTION__);

        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Validate the swapped out edge based on current configuration mode.
     */
    status = fbe_virtual_drive_copy_validate_swapped_out_edge_index(virtual_drive_p);
    if (status != FBE_STATUS_OK)
    {
        /* Unexpected swapped out edge index or mode.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s validate swapped index: %d failed - status: 0x%x\n",
                              __FUNCTION__, edge_index_swapped_out, status);

        /* Perform any neccessary cleanup
         */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    
    /* Check and handle the `copy complete set checkpoint to end' hook.
     */
    fbe_virtual_drive_check_hook(virtual_drive_p,
                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                 FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE_SET_CHECKPOINT, 
                                 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If I/O is not already quiesed, set a condition to quiese I/O and return 
     */
    status = fbe_virtual_drive_quiesce_io_if_needed(virtual_drive_p);
    if (status == FBE_STATUS_PENDING)
    {
        /* Reschedule so that quiesce runs.*/
        return FBE_LIFECYCLE_STATUS_RESCHEDULE; 
    }

    /* Set the completion to cleanup.
     */
    fbe_transport_set_completion_function(in_packet_p, 
                                          fbe_virtual_drive_copy_complete_set_rebuild_checkpoint_to_end_marker_completion, 
                                          virtual_drive_p);    

    /* Invoke the virtual drive specific method to set the checkpoint to the 
     * end marker.
     */
    status = fbe_virtual_drive_copy_complete_set_checkpoint_to_end_marker(virtual_drive_p,
                                                                          in_packet_p);
    /* Always return pending.
     */
    return FBE_LIFECYCLE_STATUS_PENDING;

}
/*****************************************************************************************
 * end fbe_virtual_drive_copy_complete_set_rebuild_checkpoint_to_end_marker_cond_function()
 *****************************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_dest_drive_failed_set_rebuild_checkpoint_to_end_marker_completion()
 ******************************************************************************
 * @brief
 *   This function is called when the non-paged metadata write to set the 
 *   rebuild checkpoint(s) to the logical end marker has completed.  It will 
 *   check the packet status and clear the condition. 
 * 
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  04/13/2013  Ron Proulx  - Updated. 
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_dest_drive_failed_set_rebuild_checkpoint_to_end_marker_completion(
                                                                    fbe_packet_t *packet_p,
                                                                    fbe_packet_completion_context_t in_context)

{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_virtual_drive_t                *virtual_drive_p;
    fbe_status_t                        packet_status;
    fbe_edge_index_t                    swapped_out_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_bool_t                          b_is_edge_swapped = FBE_TRUE;
    fbe_bool_t                          b_is_mark_nr_required = FBE_FALSE;
    fbe_bool_t                          b_is_source_failed = FBE_FALSE;
    fbe_bool_t                          b_is_degraded_needs_rebuild = FBE_FALSE;
    fbe_raid_group_np_metadata_flags_t  raid_group_np_flags_to_clear;
    fbe_base_config_downstream_health_state_t downstream_health_state;

    /* Get the virtual drive object.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)in_context;
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    swapped_out_edge_index = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);
    fbe_virtual_drive_metadata_is_edge_swapped(virtual_drive_p, &b_is_edge_swapped);
    fbe_virtual_drive_metadata_is_mark_nr_required(virtual_drive_p, &b_is_mark_nr_required);
    fbe_virtual_drive_metadata_is_source_failed(virtual_drive_p, &b_is_source_failed);
    fbe_virtual_drive_metadata_is_degraded_needs_rebuild(virtual_drive_p, &b_is_degraded_needs_rebuild);

    /* Trace some information
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: dest failed set chkpt to end mode: %d swapped: %d source failed: %d\n",
                          configuration_mode, b_is_edge_swapped, b_is_source_failed);

    /* Check the packet status.  If the write had an error, then we'll leave 
     * the condition set so that we can try again.  Return success so that the 
     * next completion function on the stack gets called.
     */ 
    packet_status = fbe_transport_get_status_code(packet_p);
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s failed - status: 0x%x\n", 
                              __FUNCTION__, packet_status);
        return FBE_STATUS_OK;
    }

    /* If we have successfully set the checkpoints, validate them.
     */
    status = fbe_virtual_drive_copy_swap_out_validate_nonpaged_metadata(virtual_drive_p,
                                                                        swapped_out_edge_index);
    if (status != FBE_STATUS_OK)
    {
        /* Trace the error and return.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s validate nonpaged failed - status: 0x%x \n",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* Determine if we need to clear the swapped bit or not.
     */
    fbe_virtual_drive_metadata_determine_nonpaged_flags(virtual_drive_p,
                                                        &raid_group_np_flags_to_clear,
                                                        b_is_edge_swapped,  /* If edge swapped is set we want to clear it */
                                                        FBE_FALSE,          /* If edge swapped is set we want to clear it */
                                                        b_is_mark_nr_required,  /* If mark nr required is set we want to clear it */
                                                        FBE_FALSE,          /* If mark nr required is set we want to clear it */
                                                        b_is_source_failed, /* If source failed is set we want to clear it */
                                                        FBE_FALSE,          /* If source failed is set we want to clear it */
                                                        b_is_degraded_needs_rebuild, /* If degraded needs rebuild is set clear it */
                                                        FBE_FALSE           /* If degraded needs rebuild is set clear it */ ); 

    /* Now write the updated non-paged flags.
     */
    status = fbe_virtual_drive_metadata_write_nonpaged_flags(virtual_drive_p,
                                                             packet_p,
                                                             raid_group_np_flags_to_clear);
    if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s edge swapped: %d source failed: %d write failed - status: 0x%x\n",
                              __FUNCTION__, b_is_edge_swapped, b_is_source_failed, status);
    
        /* Fail the request.
         */
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* Only clear this condition if the checkpoints were properly set.
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t*)virtual_drive_p);

    /* Mark this request as complete
     */
    if ( fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS) &&
        !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CONFIRMATION_DISABLED)       )
    {
        fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_FAILED_SET_CHECKPOINT);
    }

    /* Determine the health of the path with downstream objects. */
    downstream_health_state = fbe_virtual_drive_verify_downstream_health(virtual_drive_p);

    /* Set the specific lifecycle condition based on health of the path with downstream objects. */
    fbe_virtual_drive_set_condition_based_on_downstream_health(virtual_drive_p, downstream_health_state, __FUNCTION__);

    /* Return more processing required.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*******************************************************************************************
 * end fbe_virtual_drive_dest_drive_failed_set_rebuild_checkpoint_to_end_marker_completion()
 *******************************************************************************************/ 

/*!****************************************************************************
 * fbe_virtual_drive_dest_drive_failed_set_rebuild_checkpoint_to_end_marker_cond_function()
 ******************************************************************************
 * @brief
 *   This is the condition function to move a rebuild checkpoint to the logical
 *   end marker when the destination drive (ONLY) failed and the virtual drive is
 *   still in the Ready state.
 *
 * @param in_object_p               - pointer to a base object 
 * @param in_packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success/
 *                                    condition was cleared
 *                                  - FBE_LIFECYCLE_STATUS_DONE if waiting for 
 *                                    quiesce
 *
 * @note    This method will clear the swapped or checkpoint from either the
 *          passive or active side.
 *
 * @author
 *  04/12/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_dest_drive_failed_set_rebuild_checkpoint_to_end_marker_cond_function(
                                fbe_base_object_t*      in_object_p,
                                fbe_packet_t*           in_packet_p)
{
    fbe_status_t                            status;
    fbe_virtual_drive_t                    *virtual_drive_p; 
    fbe_edge_index_t                        edge_index_swapped_out = FBE_EDGE_INDEX_INVALID;           
    fbe_bool_t                              b_is_active; 
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_scheduler_hook_status_t             hook_status;

    /* Get the virtual drive object.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)in_object_p;
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)virtual_drive_p);

    /* Trace the function entry
     */
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,     
                          FBE_TRACE_LEVEL_DEBUG_HIGH,          
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                          "%s entry\n", __FUNCTION__);         

    /* Get the configuration mode.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Get the edge that was just swapped out.
     */
    edge_index_swapped_out = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: dest failed set chkpt mode: %d active: %d swapped out index: %d \n",
                          configuration_mode, b_is_active, edge_index_swapped_out);

    /* If this is not the active clear the condition.
     */
    if (b_is_active == FBE_FALSE)
    {
        /* This is unexpected.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s condition should only be set on active\n",
                              __FUNCTION__);

        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Validate the swapped out edge based on current configuration mode.
     */
    status = fbe_virtual_drive_copy_validate_swapped_out_edge_index(virtual_drive_p);
    if (status != FBE_STATUS_OK)
    {
        /* Unexpected swapped out edge index or mode.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s validate swapped index: %d failed - status: 0x%x\n",
                              __FUNCTION__, edge_index_swapped_out, status);

        /* Perform any neccessary cleanup
         */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    
    /* Check and handle the `abort complete set checkpoint to end' hook.
     */
    fbe_virtual_drive_check_hook(virtual_drive_p,
                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                 FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_SET_CHECKPOINT, 
                                 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If I/O is not already quiesed, set a condition to quiese I/O and return 
     */
    status = fbe_virtual_drive_quiesce_io_if_needed(virtual_drive_p);
    if (status == FBE_STATUS_PENDING)
    {
        /* Reschedule so that quiesce runs.*/
        return FBE_LIFECYCLE_STATUS_RESCHEDULE; 
    }
    
    /* Set the completion to cleanup.
     */
    fbe_transport_set_completion_function(in_packet_p, 
                                          fbe_virtual_drive_dest_drive_failed_set_rebuild_checkpoint_to_end_marker_completion, 
                                          virtual_drive_p);    

    /* Invoke the virtual drive specific method to set the checkpoint to the 
     * end marker.
     */
    status = fbe_virtual_drive_dest_drive_failed_set_checkpoint_to_end_marker(virtual_drive_p,
                                                                              in_packet_p);
    /* Always return pending.
     */
    return FBE_LIFECYCLE_STATUS_PENDING;

}
/****************************************************************************************
 * end fbe_virtual_drive_dest_drive_failed_set_rebuild_checkpoint_to_end_marker_cond_function()
 ****************************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker_completion()
 ******************************************************************************
 * @brief
 *   This function is called when the non-paged metadata write to set the 
 *   rebuild checkpoint(s) to the logical end marker has completed.  It will 
 *   check the packet status and clear the condition. 
 * 
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  04/13/2013  Ron Proulx  - Updated.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker_completion(
                                                                fbe_packet_t *packet_p,
                                                                fbe_packet_completion_context_t in_context)

{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_virtual_drive_t                *virtual_drive_p;
    fbe_status_t                        packet_status;
    fbe_edge_index_t                    swapped_out_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_bool_t                          b_is_edge_swapped = FBE_TRUE;
    fbe_bool_t                          b_is_mark_nr_required = FBE_FALSE;
    fbe_bool_t                          b_is_source_failed = FBE_FALSE;
    fbe_lba_t                           first_edge_checkpoint = 0;
    fbe_lba_t                           second_edge_checkpoint = 0;
    fbe_raid_group_np_metadata_flags_t  raid_group_np_flags_to_clear;
    fbe_base_config_downstream_health_state_t downstream_health_state;

    /* Get the virtual drive object.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)in_context;
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    swapped_out_edge_index = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);
    fbe_virtual_drive_metadata_is_edge_swapped(virtual_drive_p, &b_is_edge_swapped);
    fbe_virtual_drive_metadata_is_mark_nr_required(virtual_drive_p, &b_is_mark_nr_required);
    fbe_virtual_drive_metadata_is_source_failed(virtual_drive_p, &b_is_source_failed);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, FIRST_EDGE_INDEX, &first_edge_checkpoint);        
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, SECOND_EDGE_INDEX, &second_edge_checkpoint);

    /* Trace some information
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy failed set chkpt to end mode: %d swapped: %d source failed: %d\n",
                          configuration_mode, b_is_edge_swapped, b_is_source_failed);

    /* Check the packet status.  If the write had an error, then we'll leave 
     * the condition set so that we can try again.  Return success so that the 
     * next completion function on the stack gets called.
     */ 
    packet_status = fbe_transport_get_status_code(packet_p);
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s failed - status: 0x%x\n", 
                              __FUNCTION__, packet_status);
        return FBE_STATUS_OK;
    }

    /* If we have successfully set the checkpoints, validate them.
     */
    status = fbe_virtual_drive_copy_swap_out_validate_nonpaged_metadata(virtual_drive_p,
                                                                        swapped_out_edge_index);
    if (status != FBE_STATUS_OK)
    {
        /* Trace the error and return.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s validate nonpaged failed - status: 0x%x \n",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* Determine if we need to clear the swapped bit or not.
     */
    fbe_virtual_drive_metadata_determine_nonpaged_flags(virtual_drive_p,
                                                        &raid_group_np_flags_to_clear,
                                                        b_is_edge_swapped,  /* If edge swapped is set we want to clear it */
                                                        FBE_FALSE,          /* If edge swapped is set we want to clear it */
                                                        b_is_mark_nr_required,  /* If mark nr required is set we want to clear it */
                                                        FBE_FALSE,          /* If mark nr required is set we want to clear it */
                                                        FBE_TRUE,           /* Always set the source failed flag */
                                                        FBE_TRUE,           /* Always set the source failed flag */
                                                        FBE_TRUE,           /* Always set the degraded needs rebuild flag */
                                                        FBE_TRUE            /* Always set the degraded needs rebuild flag */);

    /* Now write the updated non-paged flags.
     */
    status = fbe_virtual_drive_metadata_write_nonpaged_flags(virtual_drive_p,
                                                             packet_p,
                                                             raid_group_np_flags_to_clear);
    if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s edge swapped: %d source failed: %d write failed - status: 0x%x\n",
                              __FUNCTION__, b_is_edge_swapped, b_is_source_failed, status);
    
        /* Fail the request.
         */
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* Only clear this condition if the checkpoints were properly set.
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t*)virtual_drive_p);

    /* Mark this request as complete
     */
    if ( fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS) &&
        !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CONFIRMATION_DISABLED)       )
    {
        fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_FAILED_SET_CHECKPOINT);
    }

    /* Determine the health of the path with downstream objects. */
    downstream_health_state = fbe_virtual_drive_verify_downstream_health(virtual_drive_p);

    /* Set the specific lifecycle condition based on health of the path with downstream objects. */
    fbe_virtual_drive_set_condition_based_on_downstream_health(virtual_drive_p, downstream_health_state, __FUNCTION__);

    /* Return more processing required.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/************************************************************************************
 * end fbe_virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker_completion()
 ************************************************************************************/ 

/*!****************************************************************************
 * fbe_virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker_cond_function()
 ******************************************************************************
 * @brief
 *   This is the condition function to move a rebuild checkpoint to the logical
 *   end marker when the virtual drive is in the failed state and we are aborting
 *   the copy operation.
 *
 * @param in_object_p               - pointer to a base object 
 * @param in_packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success/
 *                                    condition was cleared
 *                                  - FBE_LIFECYCLE_STATUS_DONE if waiting for 
 *                                    quiesce
 *
 * @note    This method will clear the swapped or checkpoint from either the
 *          passive or active side.
 *
 * @author
 *  09/28/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker_cond_function(
                                fbe_base_object_t*      in_object_p,
                                fbe_packet_t*           in_packet_p)
{
    fbe_status_t                            status;
    fbe_virtual_drive_t                    *virtual_drive_p; 
    fbe_edge_index_t                        edge_index_swapped_out = FBE_EDGE_INDEX_INVALID;           
    fbe_bool_t                              b_is_active; 
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_scheduler_hook_status_t             hook_status;

    /* Get the virtual drive object.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)in_object_p;
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)virtual_drive_p);

    /* Trace the function entry
     */
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,     
                          FBE_TRACE_LEVEL_DEBUG_HIGH,          
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                          "%s entry\n", __FUNCTION__);         

    /* Get the configuration mode.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Get the edge that was just swapped out.
     */
    edge_index_swapped_out = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy failed set chkpt mode: %d active: %d swapped out index: %d \n",
                          configuration_mode, b_is_active, edge_index_swapped_out);

    /* If this is not the active clear the condition.
     */
    if (b_is_active == FBE_FALSE)
    {
        /* This is unexpected.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s condition should only be set on active\n",
                              __FUNCTION__);

        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Validate the swapped out edge based on current configuration mode.
     */
    status = fbe_virtual_drive_copy_validate_swapped_out_edge_index(virtual_drive_p);
    if (status != FBE_STATUS_OK)
    {
        /* Unexpected swapped out edge index or mode.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s validate swapped index: %d failed - status: 0x%x\n",
                              __FUNCTION__, edge_index_swapped_out, status);

        /* Perform any neccessary cleanup
         */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    
    /* Check and handle the `abort complete set checkpoint to end' hook.
     */
    fbe_virtual_drive_check_hook(virtual_drive_p,
                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                 FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_SET_CHECKPOINT, 
                                 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Set the completion to cleanup.
     */
    fbe_transport_set_completion_function(in_packet_p, 
                                          fbe_virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker_completion, 
                                          virtual_drive_p);    

    /* Invoke the virtual drive specific method to set the checkpoint to the 
     * end marker.
     */
    status = fbe_virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker(virtual_drive_p,
                                                                                in_packet_p);
    /* Always return pending.
     */
    return FBE_LIFECYCLE_STATUS_PENDING;

}
/****************************************************************************************
 * end fbe_virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker_cond_function()
 ****************************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_eval_mark_nr_if_needed_cond_function()
 ******************************************************************************
 *
 * @brief   This is the condition function is used to evalulate the state of the
 *          virtual drive mirror raid group.  It looks all all edges and determines
 *          if any need to have set NR set and the checkpoint adjusted.
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - lifecycle status.
 *
 * @author
 *  03/05/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_lifecycle_status_t
fbe_virtual_drive_eval_mark_nr_if_needed_cond_function(fbe_base_object_t * object_p, 
                                                       fbe_packet_t * packet_p)
{
    fbe_virtual_drive_t    *virtual_drive_p = (fbe_virtual_drive_t *)object_p;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_bool_t              b_is_mirror_mode = FBE_TRUE;
    fbe_bool_t              b_is_rebuild_logging = FBE_FALSE;
    fbe_bool_t b_is_active;         
    fbe_lifecycle_state_t   lifecycle_state;
    /* Trace the function entry.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", __FUNCTION__);

    /* Clear the current condition. 
     */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *)virtual_drive_p);
    /* Get the current configuration mode.  If we are no longer in mirror
     * simply exit.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    
    fbe_raid_group_monitor_check_active_state((fbe_raid_group_t *)virtual_drive_p, &b_is_active);

    if (!b_is_active)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s not active mode: %d\n", __FUNCTION__, configuration_mode);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    b_is_mirror_mode = fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p);
    if (b_is_mirror_mode == FBE_FALSE) 
    {
        /* Trace and informational message and complete.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s mode: %d is no longer mirror \n",
                              __FUNCTION__, configuration_mode);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Determine if we need to run mark NR or not.
     */
    /*! @note It is now up to the raid group super class to handle the setting 
     *        of NR and the clearing of rebuild logging for the new position.
     */
    status = fbe_virtual_drive_is_rebuild_logging(virtual_drive_p, &b_is_rebuild_logging);       
    if(status != FBE_STATUS_OK)
    {
        /* Trace an error. */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: Is rebuild logging failed with status: 0x%x \n",
                              status);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_MARK_NR_TRACING,
                            "virtual_drive: eval mark nr mode: %d mark nr needed: %d\n",
                            configuration_mode, b_is_rebuild_logging);

    /* If there is any position that is rebuild logging set the `clear rl'
     * condition and run again after a short delay. 
     */
    if (b_is_rebuild_logging == FBE_TRUE)
    {
        status = fbe_lifecycle_get_state(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, &lifecycle_state);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error fbe_lifecycle_get_state failed, status = 0x%x", __FUNCTION__, status);

            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* Set the condition to `mark needs rebuild'.
         */
        status = fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
                                            FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_MARK_NR);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*) virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s set condition failed, status: 0x%X\n", __FUNCTION__, status);
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************************************
 * end fbe_virtual_drive_eval_mark_nr_if_needed_cond_function()
 **************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_is_rebuild_logging()
 ******************************************************************************
 *
 * @brief   This function goes through the non-paged metadata and determines if
 *          any if the downstream virtual drive edges are still in the `rebuild
 *          logging' (i.e. either disabled or NR hasn't been marked yet) state.
 *          If there are any edges still marked rebuild logging we cannot start
 *          the proactive copy so we wait.
 *
 * @param virtual_drive_p  - Poiner to virtual drive to check for rebuild logging
 *          for.
 * @param b_is_rebuild_logging_p - Pointer to bool that states whether there is
 *          (1) or more edge that is still marked `rebuild logging'.
 *
 * @return  fbe_status_t        
 *
 * @author
 *   04/27/2009 - Created. Dhaval Patel.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_is_rebuild_logging(fbe_virtual_drive_t *virtual_drive_p,
                                                         fbe_bool_t *b_is_rebuild_logging_p)
{

    fbe_u32_t           width;
    fbe_edge_index_t    edge_index = 0;
    fbe_bool_t          b_is_rb_logging = FBE_FALSE;

    /* By default we are not rebuild logging.
     */
    *b_is_rebuild_logging_p = FBE_FALSE;

    /* Get width of this virtual drive object. */
    fbe_base_config_get_width((fbe_base_config_t *) virtual_drive_p, &width);

    /* Go through all the downstream edges to find whether we have any drive which has rb logging,
     * if we find then set the condition to evaluate mark NR - the drive which has just become enabled
     * is a new drive then it it will mark the drive as needs rebuild and trigger the rebuild
     * operation else it will resume the rebuild (proactive copy) where it left.
     */
    for(edge_index = 0; edge_index < width; edge_index++)
    {
        fbe_raid_group_get_rb_logging((fbe_raid_group_t *) virtual_drive_p, edge_index, &b_is_rb_logging);
        if (b_is_rb_logging == FBE_TRUE)
        {
            /*! @note Trace some information.  It is up to the raid group super
             *        class to handle the condition:
             *          o FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_MARK_NR
             */
             fbe_virtual_drive_trace(virtual_drive_p,
                                     FBE_TRACE_LEVEL_INFO,
                                     FBE_VIRTUAL_DRIVE_DEBUG_FLAG_RL_TRACING,
                                     "virtual_drive: rb logging set for pos: %d (mask: 0x%x) \n",
                                     edge_index, (1 << edge_index));

            /* Flag the fact that we are waiting for rebuild logging to get 
             * cleared.
             */
            *b_is_rebuild_logging_p = FBE_TRUE;
            break;
        }
    }
    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_virtual_drive_is_rebuild_logging()
 ********************************************/

/*!****************************************************************************
 * fbe_virtual_drive_zero_paged_metadata_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to zero out the paged metadata area for the VD 
 *  object.
 * We have to create this override to skip the checks for downstream health.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  11/15/2011 - nchiu, copied from fbe_raid_group_zero_paged_metadata_cond_function
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_virtual_drive_zero_paged_metadata_cond_function(fbe_base_object_t * object_p,
                                                fbe_packet_t * packet_p)
{
    fbe_raid_group_t *raid_group_p = NULL;
    fbe_status_t      status;

    raid_group_p = (fbe_raid_group_t *)object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Set the completion function before we zero the paged metadata area
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_zero_metadata_cond_completion, raid_group_p);

    /* Zero the paged metadata area for this raid group
     */
    status = fbe_raid_group_metadata_zero_metadata(raid_group_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_virtual_drive_zero_paged_metadata_cond_function()
 ******************************************************************************/

/*!**************************************************************
 * fbe_virtual_drive_raid_group_background_monitor_operation_cond_function()
 ****************************************************************
 * @brief
 *  This is the common condition function for super class.
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
 *  10/03/2012 - Created. NCHIU
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_virtual_drive_raid_group_background_monitor_operation_cond_function(fbe_base_object_t * object_p,
                                                               fbe_packet_t * packet_p)
{
    fbe_status_t                status;

    status = fbe_lifecycle_clear_current_cond(object_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(object_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!****************************************************************************
 * fbe_virtual_drive_eval_download_request()
 ******************************************************************************
 * @brief
 *  This function checks whether we need to reject download request. 
 *
 * @param virtual_drive_p        - pointer to a virtual drive object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return NONE
 *
 * @author
 *  12/12/2011 - Created. Lili Chen
 *
 ******************************************************************************/
void 
fbe_virtual_drive_eval_download_request(fbe_virtual_drive_t *virtual_drive_p, fbe_packet_t * packet_p)
{
    fbe_raid_group_t *                  raid_group_p = (fbe_raid_group_t *)virtual_drive_p;
    fbe_u32_t                           width;
    fbe_raid_position_t                 position;
    fbe_path_attr_t                     path_attr;
    fbe_block_edge_t *                  block_edge_p = NULL;
    fbe_block_transport_server_t *      block_transport_server_p;

    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    if (fbe_virtual_drive_is_pass_thru_configuration_mode_set(virtual_drive_p))
    {
        return;
    }

    /* Scan the block edges and select an edge */
    fbe_raid_group_get_width(raid_group_p, &width);
    for (position = 0; position < width; position++)
    {
        fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &block_edge_p, position);
        fbe_block_transport_get_path_attributes(block_edge_p, &path_attr);

        if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ)
        {
            /* Send a NACK to PDO */
            raid_group_download_send_ack(raid_group_p, packet_p, position, FBE_FALSE);
            /* Clear path attr */
            block_transport_server_p = &((fbe_base_config_t *)virtual_drive_p)->block_transport_server;
            fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(block_transport_server_p, 
                0, 
                FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);
        }
    }

    return;
}
/******************************************************************************
 * end fbe_virtual_drive_eval_download_request()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_encryption_send_notification()
 ***************************************************************************** 
 * 
 * @brief   This function checks encryption state during drive swap out.
 *          VD should wait until the keys for swap-out PVD are removed.
 *
 * @param   virtual_drive_p - pointer to a virtual drive.
 * @param   encryption_state - encryption state.
 * 
 * @return fbe_status_t
 *
 * @author
 *  01/03/2014  Lili Chen  - Created.
 *
 *****************************************************************************/
static fbe_status_t
fbe_virtual_drive_encryption_send_notification(fbe_virtual_drive_t *virtual_drive_p, 
                                               fbe_base_config_encryption_state_t encryption_state,
                                               fbe_spare_swap_command_t swap_command_type, 
                                               fbe_edge_index_t edge_index)
{
    fbe_notification_info_t notification_info;
    fbe_object_id_t object_id;
    fbe_class_id_t class_id;
    fbe_handle_t object_handle;
    fbe_status_t status;
    fbe_base_transport_server_t * base_transport_server_p = (fbe_base_transport_server_t *)&(((fbe_base_config_t *)virtual_drive_p)->block_transport_server);
    fbe_base_edge_t * base_edge_p = base_transport_server_p->client_list;
    fbe_object_id_t rg_id = base_edge_p->client_id;

    fbe_zero_memory(&notification_info, sizeof(fbe_notification_info_t));
    fbe_base_object_get_object_id((fbe_base_object_t *)virtual_drive_p, &object_id);
    object_handle = fbe_base_pointer_to_handle((fbe_base_t *)virtual_drive_p);
    class_id = fbe_base_object_get_class_id(object_handle);

    notification_info.notification_type = FBE_NOTIFICATION_TYPE_ENCRYPTION_STATE_CHANGED;    
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    notification_info.class_id = class_id;

    notification_info.notification_data.encryption_info.encryption_state = encryption_state;
    notification_info.notification_data.encryption_info.object_id = object_id;
    fbe_base_config_get_generation_num((fbe_base_config_t *)virtual_drive_p, &notification_info.notification_data.encryption_info.generation_number);
    fbe_base_config_get_width((fbe_base_config_t*)virtual_drive_p, &notification_info.notification_data.encryption_info.width);
    notification_info.notification_data.encryption_info.width = base_edge_p->client_index; /* We cache VD position in RG here */
    notification_info.notification_data.encryption_info.u.vd.swap_command_type = swap_command_type;
    notification_info.notification_data.encryption_info.u.vd.edge_index = edge_index;
    notification_info.notification_data.encryption_info.u.vd.upstream_object_id = rg_id;

    fbe_database_lookup_raid_by_object_id(rg_id, &notification_info.notification_data.encryption_info.control_number);
    if (notification_info.notification_data.encryption_info.control_number == FBE_RAID_ID_INVALID)
    {
        fbe_database_lookup_raid_for_internal_rg(rg_id, &notification_info.notification_data.encryption_info.control_number);
    }

    status = fbe_notification_send(object_id, notification_info);
    if (status != FBE_STATUS_OK) {
	    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
					          FBE_TRACE_LEVEL_ERROR, 
					          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
					          "%s Notification send Failed \n", __FUNCTION__);

    }

    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_encryption_send_notification()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_out_check_encryption_state()
 ***************************************************************************** 
 * 
 * @brief   This function checks encryption state during drive swap out.
 *          VD should wait until the keys for swap-out PVD are removed.
 *
 * @param   virtual_drive_p - pointer to a virtual drive.
 * 
 * @return fbe_status_t
 *
 * @author
 *  01/03/2014  Lili Chen  - Created.
 *
 *****************************************************************************/
static fbe_status_t
fbe_virtual_drive_swap_out_check_encryption_state(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_system_encryption_mode_t system_encryption_mode;
    fbe_base_config_encryption_state_t state;
    fbe_spare_swap_command_t swap_command_type;

    fbe_database_get_system_encryption_mode(&system_encryption_mode);
    if (system_encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)
    { 
        /* The system is NOT in encrypted mode */
        return FBE_STATUS_CONTINUE;
    }

    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS) &&
        !fbe_raid_group_is_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS))
    {
        /* The VD is not in swap */
        return FBE_STATUS_CONTINUE;
    }

    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &swap_command_type);
    if (swap_command_type == FBE_SPARE_SWAP_INVALID_COMMAND &&
		!fbe_base_config_is_active((fbe_base_config_t*)virtual_drive_p))
    {
        swap_command_type = FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND;
    }
#if 0
    if (swap_command_type != FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND)
    {
        /* This is needed for permanent spare only */
        return FBE_STATUS_CONTINUE;
    }
#endif

    fbe_virtual_drive_lock(virtual_drive_p);
    fbe_base_config_get_encryption_state((fbe_base_config_t *)virtual_drive_p, &state);
    if (state == FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_REMOVED)
    {
        /* Old keys in PVD are removed, continue */
        fbe_virtual_drive_unlock(virtual_drive_p);
        return FBE_STATUS_CONTINUE;
    }

    if (state != FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_NEED_REMOVED)
    {
        fbe_base_config_set_encryption_state((fbe_base_config_t *)virtual_drive_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_NEED_REMOVED);
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: SWAP_OUT_OLD_KEY_NEED_REMOVED, orig state %d\n", state);
    }
    fbe_virtual_drive_unlock(virtual_drive_p);

    /* We should inform KMS to remove the key now */
    fbe_virtual_drive_encryption_send_notification(virtual_drive_p, 
                                                   FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_NEED_REMOVED,
                                                   swap_command_type, 
                                                   fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p));
  
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_out_check_encryption_state()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_in_check_encryption_state()
 ***************************************************************************** 
 * 
 * @brief   This function checks encryption state during drive swap in.
 *          VD should wait until the keys for swap-in PVD are provided.
 *
 * @param   virtual_drive_p - pointer to a virtual drive.
 * 
 * @return fbe_status_t
 *
 * @author
 *  01/03/2014  Lili Chen  - Created.
 *
 *****************************************************************************/
static fbe_status_t
fbe_virtual_drive_swap_in_check_encryption_state(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_system_encryption_mode_t system_encryption_mode;
    fbe_base_config_encryption_state_t state;
    fbe_spare_swap_command_t swap_command_type;

    fbe_database_get_system_encryption_mode(&system_encryption_mode);
    if (system_encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)
    { 
        /* The system is NOT in encrypted mode */
        return FBE_STATUS_CONTINUE;
    }

    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS) &&
        !fbe_raid_group_is_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS))
    {
        /* The VD is not in swap */
        return FBE_STATUS_CONTINUE;
    }

    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &swap_command_type);
    if (swap_command_type == FBE_SPARE_SWAP_INVALID_COMMAND &&
		!fbe_base_config_is_active((fbe_base_config_t*)virtual_drive_p))
    {
        swap_command_type = FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND;
    }
#if 0
    if (swap_command_type != FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND)
    {
        /* This is needed for permanent spare only */
        return FBE_STATUS_CONTINUE;
    }
#endif

    fbe_virtual_drive_lock(virtual_drive_p);
    fbe_base_config_get_encryption_state((fbe_base_config_t *)virtual_drive_p, &state);
    if (state == FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_PROVIDED)
    {
        /* New keys in PVD now, continue */
        fbe_virtual_drive_unlock(virtual_drive_p);
        return FBE_STATUS_CONTINUE;
    }

    if (state != FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_REQUIRED)
    {
        fbe_base_config_set_encryption_state((fbe_base_config_t *)virtual_drive_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_REQUIRED);
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: SWAP_IN_NEW_KEY_REQUIRED, orig state %d\n", state);
    }
    fbe_virtual_drive_unlock(virtual_drive_p);

    /* We should inform KMS to push new key now */
    fbe_virtual_drive_encryption_send_notification(virtual_drive_p, 
                                                   FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_REQUIRED,
                                                   swap_command_type, 
                                                   fbe_virtual_drive_get_swap_in_edge_index(virtual_drive_p));
  
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_in_check_encryption_state()
 ******************************************************************************/


/**********************************
 * end fbe_virtual_drive_monitor.c
 **********************************/
