#ifndef SEP_TEST_BACKGROUND_OPS_H
#define SEP_TEST_BACKGROUND_OPS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file sep_test_background_ops.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for background (copy operations, etc)
 *  operations test utilities.
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "sep_utils.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_sep.h"


/************************************************************
 *  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS
 ************************************************************/

/*!***************************************************************************
 * @typedef     fbe_test_sep_background_copy_state_t
 *****************************************************************************
 * @brief       This is an enumeration of all the possible `states' that a
 *              copy operation can be in.
 *****************************************************************************/
typedef enum fbe_test_sep_background_copy_state_e
{
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_INVALID                      =  0,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_NOT_APPLICABLE               =  1,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED          =  2,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_STARTED            =  3,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL            =  4,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_START    =  5,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_COMPLETE =  6,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_MIRROR           =  7,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_START       =  8,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_COMPLETE    =  9,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_START           = 10,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT   = 11,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_COMPLETE        = 12,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_INITIATE_COPY_COMPLETE_JOB   = 13,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_PASS_THRU        = 14,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_SWAP_OUT_START        = 15,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_SWAP_OUT_COMPLETE     = 16,
    FBE_TEST_SEP_BACKGROUND_COPY_STATE_COPY_OPERATION_COMPLETE      = 17,

    FBE_TEST_SEP_BACKGROUND_COPY_STATE_LAST

} fbe_test_sep_background_copy_state_t;

/*!***************************************************************************
 * @typedef     fbe_test_sep_background_copy_event_t
 *****************************************************************************
 * @brief       This is an enumeration of all the possible `events' that can occur
 *              during a copy operaiton.
 * 
 * @note        Although these are similar to the copy state, it is not a
 *              one-to-one mapping.
 *****************************************************************************/
typedef enum fbe_test_sep_background_copy_event_e
{
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_INVALID                      =  0,
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE                         =  1,
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_OPERATION_COMPLETE           =  2,
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_OPERATION_FAILED             =  3,
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_SOURCE_MARKED_EOL            =  4,
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_DESTINATION_SWAP_IN_START    =  5,
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_DESTINATION_SWAP_IN_COMPLETE =  6,
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_MODE_SET_TO_MIRROR           =  7,
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_METADATA_REBUILD_START       =  8,
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_METADATA_REBUILD_COMPLETE    =  9,
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_USER_REBUILD_START           = 10,
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_DESIRED_PERCENTAGE_REBUILT   = 11,
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_USER_REBUILD_COMPLETE        = 12,
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_SOURCE_SWAP_OUT_START        = 13,
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_SOURCE_SWAP_OUT_COMPLETE     = 14,
    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_MODE_SET_TO_PASS_THRU        = 15,

    FBE_TEST_SEP_BACKGROUND_COPY_EVENT_LAST

} fbe_test_sep_background_copy_event_t;

/***********************
 * FUNCTION PROTOTYPES
 ***********************/

/***************************************
 * fbe_test_sep_background_operations.c
 ***************************************/
fbe_status_t fbe_test_sep_background_ops_cleanup(void);

fbe_status_t fbe_test_sep_background_ops_start_copy_operation(fbe_test_rg_configuration_t *rg_config_p,
                                                              fbe_u32_t raid_group_count,
                                                              fbe_u32_t position_to_spare,
                                                              fbe_spare_swap_command_t copy_type,
                                                              fbe_test_raid_group_disk_set_t *dest_array_p);

fbe_status_t fbe_test_sep_background_ops_wait_for_copy_operation_complete(fbe_test_rg_configuration_t *rg_config_p, 
                                                                          fbe_u32_t raid_group_count,
                                                                          fbe_u32_t copy_position,
                                                                          fbe_spare_swap_command_t copy_type,
                                                                          fbe_bool_t b_wait_for_swap_out,
                                                                          fbe_bool_t b_skip_copy_operation_cleanup);

fbe_status_t fbe_test_sep_background_ops_copy_operation_cleanup(fbe_test_rg_configuration_t *rg_config_p, 
                                                                fbe_u32_t raid_group_count,
                                                                fbe_u32_t position_to_cleanup,
                                                                fbe_spare_swap_command_t copy_type,
                                                                fbe_bool_t b_has_cleanup_been_performed);

/***************************************
 * fbe_test_sep_background_pause.c
 ***************************************/
fbe_status_t fbe_test_sep_background_pause_set_debug_hook(fbe_scheduler_debug_hook_t *debug_hook_p,
                                                          fbe_u32_t hook_index,
                                                          fbe_object_id_t object_id,
                                                          fbe_u32_t monitor_state,
                                                          fbe_u32_t monitor_substate,
                                                          fbe_u64_t val1,
                                                          fbe_u64_t val2,
                                                          fbe_u32_t check_type,
                                                          fbe_u32_t action,
                                                          fbe_bool_t b_is_reboot_test);

fbe_status_t fbe_test_sep_background_pause_check_debug_hook(fbe_scheduler_debug_hook_t *debug_hook_p,
                                                            fbe_bool_t *b_is_debug_hook_reached_p);

fbe_status_t fbe_test_sep_background_pause_remove_debug_hook(fbe_scheduler_debug_hook_t *debug_hook_p, 
                                                             fbe_u32_t hook_index,
                                                             fbe_bool_t b_is_reboot_test);

fbe_status_t fbe_test_sep_background_pause_get_rebuild_checkpoint_to_pause_at(fbe_object_id_t vd_object_id,
                                                                              fbe_u32_t percent_user_space_copied_before_pause,
                                                                              fbe_lba_t *checkpoint_to_pause_at_p);

fbe_status_t fbe_test_sep_background_copy_operation_with_pause(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position_to_spare,
                                                               fbe_spare_swap_command_t copy_type,
                                                               fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                               fbe_test_sep_background_copy_state_t copy_state_to_pause_at,
                                                               fbe_test_sep_background_copy_event_t validation_event,
                                                               fbe_sep_package_load_params_t *sep_params_p,
                                                               fbe_u32_t percent_copied_before_pause,
                                                               fbe_bool_t b_is_reboot_test,
                                                               fbe_test_raid_group_disk_set_t *dest_array_p);

fbe_status_t fbe_test_sep_background_pause_wait_for_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                 fbe_u32_t raid_group_count,
                                                                 fbe_u32_t position_to_copy,
                                                                 fbe_sep_package_load_params_t *hook_array_p,
                                                                 fbe_bool_t b_is_reboot_test);

fbe_status_t fbe_test_sep_background_pause_set_copy_debug_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position_to_copy,
                                                               fbe_sep_package_load_params_t *hook_array_p,
                                                               fbe_u32_t monitor_state,
                                                               fbe_u32_t monitor_substate,
                                                               fbe_u64_t val1,
                                                               fbe_u64_t val2,
                                                               fbe_u32_t check_type,
                                                               fbe_u32_t action,
                                                               fbe_bool_t b_is_reboot_test);

fbe_status_t fbe_test_sep_background_pause_remove_copy_debug_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                  fbe_u32_t raid_group_count,
                                                                  fbe_u32_t position_to_copy,
                                                                  fbe_sep_package_load_params_t *sep_params_p,
                                                                  fbe_bool_t b_is_reboot_test);

fbe_status_t fbe_test_sep_background_pause_check_remove_copy_debug_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position_to_copy,
                                                               fbe_sep_package_load_params_t *hook_array_p,
                                                               fbe_bool_t b_is_reboot_test);

fbe_status_t fbe_test_sep_background_pause_glitch_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                         fbe_u32_t raid_group_count,
                                                         fbe_u32_t position_to_copy,
                                                         fbe_bool_t b_glitch_source_drive,
                                                         fbe_bool_t b_glitch_dest_drive);

fbe_status_t fbe_test_sep_background_pause_wait_for_encryption_state(fbe_test_rg_configuration_t *rg_config_p,
                                                                     fbe_u32_t raid_group_count,
                                                                     fbe_u32_t position_to_copy,
                                                                     fbe_base_config_encryption_state_t expected_encryption_state);


#endif /* SEP_TEST_BACKGROUND_OPS_H */
