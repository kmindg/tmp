#ifndef FBE_SPARE_LIB_PRIVATE_H
#define FBE_SPARE_LIB_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_spare_lib_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local defination for the drive swap service component
 *  in drive swap library.   
 *
 * @version
  *  8/26/2009:  Created. Dhaval Patel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe_block_transport.h"
#include "fbe_job_service.h"

/************************************************************
 *  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS
 ************************************************************/

/* Macro to trace the entry of a function in the RAID group object or in one of its derived classes */
#define FBE_SPARE_LIB_TRACE_FUNC_ENTRY()                           \
    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,           \
                            FBE_TRACE_LEVEL_DEBUG_HIGH,            \
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,   \
                            "%s entry.\n",                           \
                            __FUNCTION__);                         \

// Return error status if pointer is NULL
#define FBE_SPARE_LIB_TRACE_RETURN_ON_NULL(m_arg_p)                    \
    if (m_arg_p == NULL)                                               \
    {                                                                  \
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,           \
                               FBE_TRACE_LEVEL_WARNING,                \
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,   \
                               "%s FBE NULL pointer\n", __FUNCTION__);   \
        return FBE_STATUS_GENERIC_FAILURE;                             \
    }                                                                  \

/*!*******************************************************************
 * @struct fbe_virtual_drive_spare_info_s
 *********************************************************************
 * @brief
 *  It defines spare information returned from the virtual drive
 *  object, it associates virtual drive object id with drive spare
 *  information return from the specific virtual drive object.
 *********************************************************************/
typedef struct fbe_virtual_drive_spare_info_s
{
    /*! Virtual drive object-id of the specific drive. 
     */
    fbe_object_id_t                         vd_object_id;

    /*! Spare drive information returned from associated virtual drive 
     *  object. 
     */
    fbe_spare_drive_info_t                  spare_drive_info;
} fbe_virtual_drive_spare_info_t;

static __forceinline void
fbe_spare_lib_virtual_drive_spare_info_set_object_id(fbe_virtual_drive_spare_info_t * vd_spare_info_p,
                                                     fbe_object_id_t const vd_object_id)
{
    vd_spare_info_p->vd_object_id = vd_object_id;
}

static __forceinline void
fbe_spare_lib_virtual_drive_spare_info_get_object_id(fbe_virtual_drive_spare_info_t * const  vd_spare_info_p,
                                                     fbe_object_id_t * vd_object_id_p)
{
    *vd_object_id_p = vd_spare_info_p->vd_object_id;
}

/*!****************************************************************************
 * @struct fbe_virtual_drive_spare_info_list_s
 ******************************************************************************
 * @brief
 *  It defines an array of the virtual drive spare information.
 ******************************************************************************/
typedef struct fbe_virtual_drive_spare_info_list_s
{
    /*! Array of virtual drive spare information.
     */
    fbe_virtual_drive_spare_info_t     vd_spare_drive_info[128];
} fbe_virtual_drive_spare_info_list_t;


/*!****************************************************************************
 * fbe_spare_lib_virtual_drive_spare_info_list_get_spare_info()
 ******************************************************************************
 * @brief 
 *   Get the virtual drive spare information from the list of virtual drive
 *   spare information.
 *
 * @param vd_spare_info_list_p  - VD spare inforamtion list.
 * @param vd_spare_index        - VD spare index.
 * @param vd_spare_drive_info_p - VD spare drive information pointer.
 *
 * @return swap_status_p    - Pointer to the drive swap request status.
 *
 ******************************************************************************/
static __forceinline void
fbe_spare_lib_virtual_drive_spare_info_list_get_spare_info (fbe_virtual_drive_spare_info_list_t * const vd_spare_info_list_p,
                                                            fbe_u32_t  vd_spare_index,
                                                            fbe_virtual_drive_spare_info_t ** vd_spare_drive_info_p)
{
    *vd_spare_drive_info_p = &(vd_spare_info_list_p->vd_spare_drive_info[vd_spare_index]);
}

/*!****************************************************************************
 * @enum fbe_spare_selection_priority_t
 ******************************************************************************
 * @brief
 *  Enumeration the spare selection priorities.
 ******************************************************************************/
typedef enum fbe_spare_selection_priority_e
{   
    FBE_SPARE_SELECTION_PRIORITY_INVALID        = -1,   /*!< Selection priority is invalid.  */
    FBE_SPARE_SELECTION_PRIORITY_ZERO           =  0,   /*!< Lowest selection priority */
    FBE_SPARE_SELECTION_PRIORITY_NUM_OF_RULES   =  6,   /*!< There are currently this many rules. */
} fbe_spare_selection_priority_t;



/*!****************************************************************************
 * fbe_spare_hot_spare_info_set_object_id()
 ******************************************************************************
 * @brief 
 *   Set the hot spare object-id in hot spare drive information.
 *
 * @param hot_spare_info_p        - Pointer to the hot spare drive info.
 * @param spare_object_id     - Hot spare PVD object-id.
 *
 ******************************************************************************/
static __forceinline void
fbe_spare_lib_hot_spare_info_set_object_id(fbe_spare_selection_info_t *  hot_spare_info_p,
                                           fbe_object_id_t const   spare_object_id)
{
    hot_spare_info_p->spare_object_id = spare_object_id;
}


/*!****************************************************************************
 * fbe_spare_lib_hot_spare_info_get_object_id()
 ******************************************************************************
 * @brief 
 *   Get the object-id associated with hot spare drive information.
 *
 * @param hot_spare_info_p        - Pointer to the hot spare drive info.
 * 
 * @return spare_object_id_p  - Pointer to the hot spare PVD object-id.
 *
 ******************************************************************************/
static __forceinline void
fbe_spare_lib_hot_spare_info_get_object_id(fbe_spare_selection_info_t * const    hot_spare_info_p,
                                           fbe_object_id_t *               spare_object_id_p)
{
    *spare_object_id_p = hot_spare_info_p->spare_object_id;
}

/*!****************************************************************************
 * fbe_spare_selection_init_spare_selection_priority()
 ******************************************************************************
 * @brief 
 *   Initialize the spare selection priority in spare drive information.
 *
 * @param spare_info_p        - Pointer to the hot spare drive info.
 * @param spare_object_id     - Hot spare PVD object-id.
 *
 ******************************************************************************/
static __forceinline void
fbe_spare_selection_init_spare_selection_priority(fbe_spare_selection_info_t *spare_info_p)
{
    spare_info_p->spare_selection_bitmask = 0;
}

/*!****************************************************************************
 * fbe_spare_selection_add_spare_selection_priority()
 ****************************************************************************** 
 * 
 * @brief   Add the spare selection priority to the spare selection bitmask. 
 *
 * @param   spare_info_p - Pointer to the hot spare drive info.
 * @param   spare_selection_priority - Spare selection priority.
 *
 ******************************************************************************/
static __forceinline void
fbe_spare_selection_add_spare_selection_priority(fbe_spare_selection_info_t *spare_info_p,
                                                fbe_spare_selection_priority_t const spare_selection_priority)
{
    spare_info_p->spare_selection_bitmask |= (1 << spare_selection_priority);
}

/*!****************************************************************************
 * fbe_spare_selection_remove_spare_selection_priority()
 ****************************************************************************** 
 * 
 * @brief   Remove the spare selection priority from the spare selection bitmask. 
 *
 * @param   spare_info_p - Pointer to the hot spare drive info.
 * @param   spare_selection_priority - Spare selection priority.
 *
 ******************************************************************************/
static __forceinline void
fbe_spare_selection_remove_spare_selection_priority(fbe_spare_selection_info_t *spare_info_p,
                                                fbe_spare_selection_priority_t const spare_selection_priority)
{
    spare_info_p->spare_selection_bitmask &= ~(1 << spare_selection_priority);
}

/*!****************************************************************************
 * fbe_spare_selection_set_spare_selection_priority()
 ****************************************************************************** 
 * 
 * @brief   Add the spare selection priority to to the spare selection bitmask. 
 *
 * @param   spare_info_p - Pointer to the hot spare drive info.
 * @param   spare_selection_priority_bitmask - Spare selection priority bitmask.
 *
 ******************************************************************************/
static __forceinline void
fbe_spare_selection_set_spare_selection_priority(fbe_spare_selection_info_t *spare_info_p,
                                                 fbe_spare_selection_priority_bitmask_t const spare_selection_priority_bitmask)
{
    spare_info_p->spare_selection_bitmask = spare_selection_priority_bitmask;
}

/*!****************************************************************************
 * fbe_spare_selection_is_spare_selection_priority_set()
 ****************************************************************************** 
 * 
 * @brief   Determines if the priority passed is set in the spare selection
 *          priority bitmask.
 *
 * @param   spare_info_p - Pointer to the hot spare drive info.
 * @param   spare_selection_priority - Spare selection priority.
 * 
 * @return  bool - FBE_TRUE - The specified spare selection priority is set.
 *                 FBE_FALSE - The specified spare selection priority is not set.
 ******************************************************************************/
static __forceinline fbe_bool_t
fbe_spare_selection_is_spare_selection_priority_set(fbe_spare_selection_info_t *spare_info_p,
                                                    fbe_spare_selection_priority_t const spare_selection_priority)
{
    return ((spare_info_p->spare_selection_bitmask & (1 << spare_selection_priority)) == (1 << spare_selection_priority));
}

/*!****************************************************************************
 * fbe_spare_selection_is_proposed_drive_higher_priority()
 ****************************************************************************** 
 * 
 * @brief   Determines if the priority of the proposed spare is higher (i.e. a
 *          better fir) than the currently selected spare.
 *
 * @param   proposed_spare_info_p - Pointer to proposed spare info
 * @param   selected_spare_info_p - Pointer to selected spare info
 * 
 * @return  bool - FBE_TRUE - The proposed spare is a better fit.
 *                 FBE_FALSE - The proposed drive is not a better fit.
 ******************************************************************************/
static __forceinline fbe_bool_t
fbe_spare_selection_is_proposed_drive_higher_priority(fbe_spare_selection_info_t *proposed_spare_info_p,
                                                      fbe_spare_selection_info_t *selected_spare_info_p)
{
    return (((fbe_u32_t)proposed_spare_info_p->spare_selection_bitmask > (fbe_u32_t)selected_spare_info_p->spare_selection_bitmask) ? FBE_TRUE : FBE_FALSE);
}

/*!****************************************************************************
 * fbe_spare_lib_hot_spare_info_get_selection_priority()
 ******************************************************************************
 * @brief 
 *   Get the selection priority bitmask associated with hot spare drive information.
 *
 * @param spare_info_p    - Pointer to the hot spare drive info.
 * @param spare_selection_priority_bitmask_p - Address of bitmask to update
 *
 ******************************************************************************/
static __forceinline void
fbe_spare_lib_hot_spare_info_get_selection_priority(fbe_spare_selection_info_t *const spare_info_p,
                                           fbe_spare_selection_priority_bitmask_t *spare_selection_priority_bitmask_p)
{
    *spare_selection_priority_bitmask_p = spare_info_p->spare_selection_bitmask;
}

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/* fbe_spare_lib_validation.c */
fbe_status_t fbe_spare_lib_validation_validate_user_request_swap_edge_index(fbe_job_service_drive_swap_request_t *js_swap_request_p);

fbe_status_t fbe_spare_lib_validation_swap_in_request(fbe_job_service_drive_swap_request_t *job_service_swap_request_p);

fbe_status_t fbe_spare_lib_validation_swap_out_request(fbe_job_service_drive_swap_request_t *job_service_swap_request_p);

fbe_status_t fbe_spare_lib_validation_validate_virtual_drive_object_id(fbe_job_service_drive_swap_request_t *js_swap_request_p);

/* fbe_spare_lib_selection.c */
fbe_status_t fbe_spare_lib_selection_find_best_suitable_spare(fbe_packet_t *packet_p,
                                                              fbe_job_service_drive_swap_request_t * job_service_swap_request_p);
// Finds the drive spare information associated with VD object.
fbe_status_t fbe_spare_lib_selection_get_virtual_drive_spare_info(fbe_object_id_t vd_object_id,
                                                                  fbe_spare_drive_info_t *spare_drive_info_p);

// Finds the drive spare information associated with hot spare PVD object.
fbe_status_t fbe_spare_lib_selection_get_hot_spare_info(fbe_object_id_t hs_pvd_object_id,
                                                        fbe_spare_drive_info_t *in_spare_drive_info_p);

// Get the upstream object information
fbe_status_t fbe_spare_lib_selection_get_virtual_drive_upstream_object_info(fbe_object_id_t in_vd_object_id,
                                                                            fbe_spare_get_upsteam_object_info_t *in_upstream_object_info_p);
/* fbe_spare_main.c */
fbe_u32_t fbe_spare_main_get_operation_timeout_secs(void);
fbe_bool_t fbe_spare_main_is_operation_confirmation_enabled(void);

/* fbe_spare_swap.c */
fbe_status_t fbe_spare_lib_swap_in_permanent_spare(fbe_packet_t *packet_p,
                                                   fbe_job_service_drive_swap_request_t * job_service_swap_request_p);
fbe_status_t fbe_spare_lib_initiate_copy_operation(fbe_packet_t *packet_p,
                                                   fbe_job_service_drive_swap_request_t * job_service_swap_request_p);
fbe_status_t fbe_spare_lib_complete_copy_operation(fbe_packet_t *packet_p,
                                                   fbe_job_service_drive_swap_request_t * job_service_swap_request_p);
fbe_status_t fbe_spare_lib_complete_aborted_copy_operation(fbe_packet_t *packet_p,
                                                           fbe_job_service_drive_swap_request_t *js_swap_request_p);


/* fbe_spare_lib_utils.c */
void fbe_spare_lib_utils_complete_packet(fbe_packet_t *packet_p, 
                                         fbe_status_t packet_status);
fbe_status_t fbe_spare_lib_utils_check_capacity(fbe_lba_t desired_spare_capacity,
                                                fbe_lba_t curr_spare_gross_capacity,
                                                fbe_bool_t *hs_capacity_check_p);
fbe_status_t fbe_spare_lib_utils_send_control_packet(fbe_payload_control_operation_opcode_t control_code,
                                                     fbe_payload_control_buffer_t buffer,
                                                     fbe_payload_control_buffer_length_t buffer_length,
                                                     fbe_service_id_t service_id,
                                                     fbe_class_id_t class_id,
                                                     fbe_object_id_t object_id,
                                                     fbe_packet_attr_t attr);

fbe_status_t fbe_spare_lib_utils_send_swap_completion_status(fbe_job_service_drive_swap_request_t *job_service_drive_swap_request_p,
                                                             fbe_bool_t b_operation_confirmation);

fbe_status_t fbe_spare_lib_utils_send_job_complete(fbe_object_id_t vd_object_id);

fbe_status_t fbe_spare_lib_utils_send_swap_request_rollback(fbe_object_id_t vd_object_id,
                                                            fbe_job_service_drive_swap_request_t *js_swap_request_p);

fbe_status_t fbe_spare_lib_utils_database_service_mark_pvd_swap_pending(fbe_job_service_drive_swap_request_t *js_swap_request_p);
fbe_status_t fbe_spare_lib_utils_database_service_clear_pvd_swap_pending(fbe_job_service_drive_swap_request_t *js_swap_request_p);

fbe_status_t fbe_spare_lib_utils_abort_database_transaction(fbe_database_transaction_id_t transaction_id);
fbe_status_t fbe_spare_lib_utils_commit_database_transaction(fbe_database_transaction_id_t transaction_id);
fbe_status_t fbe_spare_lib_utils_start_database_transaction(fbe_database_transaction_id_t *transaction_id, fbe_u64_t job_number);

fbe_status_t fbe_spare_lib_utils_get_block_edge_info(fbe_object_id_t object_id,
                                                     fbe_edge_index_t edge_index,
                                                     fbe_block_transport_control_get_edge_info_t * block_edge_info_p);

fbe_status_t fbe_spare_lib_utils_database_service_create_edge(fbe_database_transaction_id_t transaction_id, 
                                                              fbe_object_id_t server_object_id,
                                                              fbe_object_id_t client_object_id,
                                                              fbe_u32_t client_index,
                                                              fbe_lba_t client_imported_capacity,
                                                              fbe_lba_t offset,
                                                              fbe_bool_t b_is_operation_confirmation_enabled);

fbe_status_t fbe_spare_lib_utils_database_service_destroy_edge(fbe_database_transaction_id_t transaction_id,
                                                               fbe_object_id_t object_id, 
                                                               fbe_edge_index_t client_index,
                                                               fbe_bool_t b_is_operation_confirmation_enabled);

fbe_status_t 
fbe_spare_lib_utils_database_service_swap_edge(fbe_database_transaction_id_t transaction_id,
                                                    fbe_object_id_t client_object_id, 
                                                    fbe_edge_index_t old_edge_client_index,
                                                    fbe_object_id_t new_edge_server_object_id,
                                                    fbe_u32_t new_edge_client_index,
                                                    fbe_lba_t new_edge_client_imported_capacity,
                                                    fbe_lba_t new_edge_offset);

fbe_status_t fbe_spare_lib_utils_database_service_update_pvd(fbe_database_control_update_pvd_t  *update_provision_drive);
fbe_status_t fbe_spare_lib_utils_take_object_offline(fbe_object_id_t object_id);

fbe_status_t fbe_spare_lib_utils_get_configuration_mode(fbe_object_id_t vd_object_id,
                                                        fbe_virtual_drive_configuration_mode_t * configuration_mode_p);

fbe_status_t fbe_spare_lib_utils_get_permanent_spare_swap_edge_index(fbe_object_id_t vd_object_id,
                                                                     fbe_edge_index_t * permanent_spare_swap_edge_index);

fbe_status_t fbe_spare_lib_utils_get_copy_swap_edge_index(fbe_object_id_t vd_object_id,
                                                          fbe_edge_index_t * proactive_spare_swap_edge_index);

fbe_status_t fbe_spare_lib_utils_initiate_copy_get_updated_virtual_drive_configuration_mode(fbe_object_id_t vd_object_id,
                                                                   fbe_virtual_drive_configuration_mode_t *updated_configuration_mode_p);

fbe_status_t fbe_spare_lib_utils_update_virtual_drive_configuration_mode(fbe_database_control_update_vd_t *update_virtual_drive_p,
                                                                         fbe_bool_t b_operation_confirmation);

fbe_status_t fbe_spare_lib_utils_get_server_object_id_for_edge(fbe_object_id_t object_id,
                                                               fbe_edge_index_t edge_index,
                                                               fbe_object_id_t * object_id_p);

fbe_status_t fbe_spare_lib_utils_get_provision_drive_info(fbe_object_id_t in_pvd_object_id,
                                                          fbe_provision_drive_info_t * in_provision_drive_info_p);

fbe_status_t fbe_spare_lib_utils_complete_copy_get_updated_virtual_drive_configuration_mode(fbe_object_id_t vd_object_id,
                                                                   fbe_virtual_drive_configuration_mode_t *updated_configuration_mode_p);

fbe_status_t fbe_spare_lib_utils_complete_aborted_copy_get_updated_virtual_drive_configuration_mode(fbe_object_id_t vd_object_id,
                                                                   fbe_edge_index_t swap_out_edge_index,
                                                                   fbe_virtual_drive_configuration_mode_t *updated_configuration_mode_p);

fbe_status_t fbe_spare_lib_utils_get_control_buffer(fbe_packet_t * packet_p,
                                                    fbe_payload_control_operation_t ** control_operation_p,
                                                    void ** payload_data_p,
                                                    fbe_u32_t payload_data_len,
                                                    const char * function);

fbe_status_t fbe_spare_lib_utils_send_notification(fbe_job_service_drive_swap_request_t *js_swap_request_p,
                                                   fbe_u64_t job_number,
                                                   fbe_status_t job_status,
                                                   fbe_job_action_state_t job_action_state);

fbe_status_t fbe_spare_lib_get_performance_table_info(fbe_u32_t performance_tier_number, 
                                                      fbe_performance_tier_drive_information_t *tier_drive_info_p);

fbe_status_t  fbe_spare_lib_utils_commit_complete_write_event_log(fbe_job_service_drive_swap_request_t *drive_swap_request_p);
                                                                
fbe_status_t fbe_spare_lib_utils_get_original_drive_info(fbe_packet_t *packet_p,
                                                 fbe_job_service_drive_swap_request_t *drive_swap_request_p,
                                                 fbe_spare_drive_info_t *desired_spare_drive_info_p);

fbe_status_t fbe_spare_lib_utils_enumerate_system_objects(fbe_class_id_t class_id,
                                             fbe_object_id_t *system_object_list_p,
                                             fbe_u32_t total_objects,
                                             fbe_u32_t *actual_num_objects_p);

fbe_status_t fbe_spare_lib_utils_get_unused_drive_as_spare_flag(fbe_object_id_t vd_object_id,
                                              fbe_bool_t * unused_drive_as_spare_p);

fbe_status_t fbe_spare_lib_utils_set_vd_nonpaged_metadata(fbe_packet_t* packet_p,
                                             fbe_job_service_drive_swap_request_t * js_swap_request_p);

fbe_status_t fbe_spare_lib_utils_switch_pool_ids(fbe_job_service_drive_swap_request_t *js_swap_request_p, 
                                                 fbe_object_id_t selected_spare_id);

fbe_status_t fbe_spare_lib_utils_rollback_write_event_log(fbe_job_service_drive_swap_request_t *drive_swap_request_p);

fbe_status_t fbe_spare_lib_utils_wait_for_object_to_be_ready(fbe_object_id_t object_id);

fbe_status_t fbe_spare_lib_utils_set_vd_checkpoint_to_end_marker(fbe_object_id_t vd_object_id,
                                                                 fbe_spare_swap_command_t swap_command,
                                                                 fbe_bool_t b_operation_confirmation);

fbe_status_t fbe_spare_lib_utils_start_user_copy_request(fbe_object_id_t vd_object_id,
                                                         fbe_spare_swap_command_t swap_command,
                                                         fbe_edge_index_t swap_edge_index,
                                                         fbe_bool_t b_operation_confirmation);

#endif /* FBE_SPARE_LIB_PRIVATE_H */
