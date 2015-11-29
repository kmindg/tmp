#ifndef SEP_TEST_IO_H
#define SEP_TEST_IO_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file sep_test_io.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the defines for the SEP I/O utility functions.
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "sep_utils.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_job_service_interface.h"


/************************************************************
 *  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS
 ************************************************************/


 /*!**************************************************
 * @def FBE_TEST_HEADER_PATTERN
 ***************************************************
 * @brief This pattern appears at the head of all
 * sectors generated.
 ***************************************************/
#define FBE_TEST_HEADER_PATTERN 0x3CC3

/*!*******************************************************************
 * @def FBE_RDGEN_ABORT_TIME 
 *********************************************************************
 * @brief
 *  The time to cancel the IO requests in rdgen in millisecond
 *  FBE_TEST_ABORT_TIME_ZERO : zero sec 
 *  FBE_TEST_ABORT_TIME_ONE_TENTH_SEC : 1/10th of a second (100 msec)
 *  FBE_TEST_ABORT_TIME_TWO_SEC : two seconds (2000 msec)
 *********************************************************************/
#define FBE_TEST_RANDOM_ABORT_TIME_INVALID 0xFFFF
#define FBE_TEST_RANDOM_ABORT_TIME_ZERO_MSEC 0
#define FBE_TEST_RANDOM_ABORT_TIME_ONE_TENTH_MSEC 100
#define FBE_TEST_RANDOM_ABORT_TIME_TWO_SEC 2000



/* Definition of random aborts in milli secs to be set
 * for I/Os in rdgen
 */
typedef fbe_u32_t fbe_test_random_abort_msec_t;

/*!***************************************************************************
 * @enum fbe_test_sep_io_sequence_state_t
 *****************************************************************************
 * @brief
 *  Enumeration of the one and only sequence state machine.  Basically the
 *  purpose is to make sure the configuration modes have been initialized and
 *  that all the luns are ready (on both sps if dualsp mode is enabled).
 *****************************************************************************/
typedef enum fbe_test_sep_io_sequence_state_t
{
    FBE_TEST_SEP_IO_SEQUENCE_STATE_INVALID,
    FBE_TEST_SEP_IO_SEQUENCE_STATE_CONFIGURED,
    FBE_TEST_SEP_IO_SEQUENCE_STATE_WAITING_FOR_IO_READY,
    FBE_TEST_SEP_IO_SEQUENCE_STATE_FAILED,
    FBE_TEST_SEP_IO_SEQUENCE_STATE_IO_READY,
    FBE_TEST_SEP_IO_SEQUENCE_STATE_LAST,
} fbe_test_sep_io_sequence_state_t;

/*!***************************************************************************
 * @enum fbe_test_sep_io_sequence_type_t
 *****************************************************************************
 * @brief
 *  Enumeration of different I/O sequences used by the SEP test scenarios.
 *****************************************************************************/
typedef enum fbe_test_sep_io_sequence_type_e
{
    FBE_TEST_SEP_IO_SEQUENCE_TYPE_INVALID,
    FBE_TEST_SEP_IO_SEQUENCE_TYPE_WRITE_FIXED_PATTERN,
    FBE_TEST_SEP_IO_SEQUENCE_TYPE_READ_FIXED_PATTERN,
    FBE_TEST_SEP_IO_SEQUENCE_TYPE_VERIFY_DATA,
    FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD,
    FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_PREFILL,
    FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_ABORTS,
    FBE_TEST_SEP_IO_SEQUENCE_TYPE_CATERPILLAR,
    FBE_TEST_SEP_IO_SEQUENCE_TYPE_TEST_BLOCK_OPERATIONS,
    FBE_TEST_SEP_IO_SEQUENCE_TYPE_LAST,
} fbe_test_sep_io_sequence_type_t;

/*!***************************************************************************
 * @enum fbe_test_sep_io_dual_sp_mode_t
 *****************************************************************************
 * @brief
 *  Enumeration of different dual sp modes (they won't necessarily match rdgen)
 *****************************************************************************/
typedef enum fbe_test_sep_io_dual_sp_mode_e
{
    FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID,
    FBE_TEST_SEP_IO_DUAL_SP_MODE_LOCAL_ONLY,
    FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE,
    FBE_TEST_SEP_IO_DUAL_SP_MODE_RANDOM,
    FBE_TEST_SEP_IO_DUAL_SP_MODE_PEER_ONLY,
    FBE_TEST_SEP_IO_DUAL_SP_MODE_LAST,
} fbe_test_sep_io_dual_sp_mode_t;

/*!***************************************************************************
 * @enum fbe_test_sep_io_sequence_state_t
 *****************************************************************************
 * @brief
 *  Enumeration of the one and only sequence state machine.  Basically the
 *  purpose is to make sure the configuration modes have been initialized and
 *  that all the luns are ready (on both sps if dualsp mode is enabled).
 *****************************************************************************/
typedef enum fbe_test_sep_io_background_op_enable_e
{
    FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_INVALID             = 0x00000000,
    FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_VERIFY              = 0x00000001,  /* Enable a verify during the I/O */
    FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_PROACTIVE_COPY      = 0x00000002,  /* Enable a proactive copy during the I/O */
    FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_USER_COPY           = 0x00000004,  /* Enable a user initiated copy during the I/O */
    FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_USER_PROACTIVE_COPY = 0x00000008,  /* Enable user initiated proactive copy during I/O */
    FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_DRIVE_GLITCH        = 0x00000010,  /* Enable a drive `glitch' during I/O */

    FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_LAST,
} fbe_test_sep_io_background_op_enable_t;

/*!*******************************************************************
 * @struct  fbe_test_sep_io_sequence_config_t
 ********************************************************************* 
 * 
 * @brief Definition of a configuration for a test sequence.
 *
 *********************************************************************/
typedef struct fbe_test_sep_io_sequence_config_s
{
    fbe_test_sep_io_sequence_type_t     sequence_type;
    fbe_test_sep_io_sequence_state_t    sequence_state;
    fbe_test_rg_configuration_t        *rg_config_p;
    fbe_block_count_t                   max_blocks_for_small_io;
    fbe_block_count_t                   max_blocks_for_large_io; 
    fbe_bool_t                          abort_enabled;
    fbe_test_random_abort_msec_t        abort_msecs;
    fbe_test_random_abort_msec_t        current_abort_msecs;
    fbe_bool_t                          dualsp_enabled;
    fbe_test_sep_io_dual_sp_mode_t      dualsp_mode;
    fbe_api_rdgen_peer_options_t        current_peer_options;
    fbe_u32_t                           io_seconds;
    fbe_u32_t                           io_thread_count;
    fbe_u32_t                           large_io_count;
    fbe_u32_t                           small_io_count;
    fbe_bool_t                          b_should_background_ops_run;
    fbe_test_sep_io_background_op_enable_t background_ops;
    fbe_bool_t                          b_have_background_ops_run;
} fbe_test_sep_io_sequence_config_t;


/***********************
 * FUNCTION PROTOTYPES
 ***********************/
fbe_u32_t fbe_test_sep_io_get_small_io_count(fbe_u32_t default_io_count);
fbe_u32_t fbe_test_sep_io_get_large_io_count(fbe_u32_t default_io_count);
fbe_status_t fbe_test_sep_io_set_threads(fbe_u32_t io_thread_count);
fbe_u32_t fbe_test_sep_io_get_threads(fbe_u32_t default_thread_count);
fbe_status_t fbe_test_sep_io_set_io_seconds(fbe_u32_t io_seconds);
fbe_u32_t fbe_test_sep_io_get_io_seconds(void);
fbe_status_t fbe_test_sep_io_configure_dualsp_io_mode(fbe_bool_t b_is_dualsp_enabled,
                                                      fbe_test_sep_io_dual_sp_mode_t requested_dualsp_mode);
fbe_status_t fbe_test_sep_io_configure_abort_mode(fbe_bool_t b_is_abort_enabled,
                                                  fbe_test_random_abort_msec_t rdgen_abort_msecs);
fbe_status_t fbe_test_sep_io_configure_background_operations(fbe_bool_t b_enable_background_op,
                                                             fbe_test_sep_io_background_op_enable_t background_enable_flags);
fbe_status_t fbe_test_sep_io_run_verify_on_lun(fbe_object_id_t lun_object_id,
                                               fbe_bool_t b_verify_entire_lun,
                                               fbe_lba_t start_lba,
                                               fbe_block_count_t blocks);
fbe_status_t fbe_test_sep_io_run_verify_on_all_luns(fbe_test_rg_configuration_t *rg_config_p,
                                                    fbe_bool_t b_verify_entire_lun,
                                                    fbe_lba_t start_lba,
                                                    fbe_block_count_t blocks);
void fbe_test_sep_io_setup_fill_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                             fbe_object_id_t object_id,
                                             fbe_class_id_t class_id,
                                             fbe_rdgen_operation_t rdgen_operation,
                                             fbe_lba_t max_lba,
                                             fbe_u64_t io_size_blocks);
fbe_status_t fbe_test_sep_io_get_num_luns(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t *num_luns_p);
fbe_status_t fbe_test_sep_io_allocate_rdgen_test_context_for_all_luns(fbe_api_rdgen_context_t **context_pp,
                                                                      fbe_u32_t num_tests_contexts);
fbe_status_t fbe_test_sep_io_setup_standard_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                fbe_object_id_t object_id,
                                                fbe_class_id_t class_id,
                                                fbe_rdgen_operation_t rdgen_operation,
                                                fbe_lba_t capacity,
                                                fbe_u32_t max_passes,
                                                fbe_u32_t threads,
                                                fbe_u64_t io_size_blocks,
                                                fbe_bool_t b_inject_random_aborts,
                                                fbe_bool_t b_dualsp_io);
void fbe_test_sep_io_validate_status(fbe_status_t status,
                                     fbe_api_rdgen_context_t *context_p,
                                     fbe_bool_t b_is_random_abort_enabled);
fbe_status_t fbe_test_sep_io_free_rdgen_test_context_for_all_luns(fbe_api_rdgen_context_t **context_pp,
                                                                  fbe_u32_t num_tests_contexts);
fbe_status_t fbe_test_sep_io_setup_caterpillar_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                fbe_object_id_t object_id,
                                                fbe_rdgen_operation_t rdgen_operation,
                                                fbe_lba_t capacity,
                                                fbe_u32_t max_passes,
                                                fbe_u32_t threads,
                                                fbe_u64_t io_size_blocks,
                                                fbe_bool_t b_random,
                                                fbe_bool_t b_increasing,
                                                fbe_bool_t b_inject_random_aborts,
                                                fbe_bool_t b_dualsp_io);
fbe_status_t fbe_test_sep_io_setup_rdgen_test_context_for_all_luns(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_api_rdgen_context_t *context_p,
                                             fbe_u32_t num_contexts,
                                             fbe_rdgen_operation_t rdgen_operation,
                                             fbe_u32_t max_passes,
                                             fbe_u32_t threads,
                                             fbe_u64_t io_size_blocks,
                                             fbe_bool_t b_random,
                                             fbe_bool_t b_increasing,
                                             fbe_bool_t b_inject_random_aborts,
                                             fbe_bool_t b_dualsp_io,
                                             fbe_status_t operation_setup_fn(fbe_api_rdgen_context_t *context_p,
                                                                             fbe_object_id_t object_id,
                                                                             fbe_rdgen_operation_t rdgen_operation,
                                                                             fbe_lba_t capacity,
                                                                             fbe_u32_t max_passes,
                                                                             fbe_u32_t threads,
                                                                             fbe_u64_t io_size_blocks,
                                                                             fbe_bool_t b_random,
                                                                             fbe_bool_t b_increasing,
                                                                             fbe_bool_t b_inject_random_aborts,
                                                                             fbe_bool_t b_dualsp_io) );
void fbe_test_sep_io_run_test_sequence(fbe_test_rg_configuration_t *rg_config_p,
                                       fbe_test_sep_io_sequence_type_t sep_io_sequence_type,
                                       fbe_block_count_t max_blocks_for_small_io,
                                       fbe_block_count_t max_blocks_for_large_io, 
                                       fbe_bool_t b_inject_random_aborts, 
                                       fbe_test_random_abort_msec_t random_abort_time_msecs,
                                       fbe_bool_t b_run_dual_sp,
                                       fbe_test_sep_io_dual_sp_mode_t dual_sp_mode);

void fbe_test_sep_io_check_status(fbe_api_rdgen_context_t *context_p,
                                  fbe_u32_t num_tests,
                                  fbe_u32_t ran_abort_msecs);

void fbe_test_sep_io_validate_rdgen_ran(void);
void fbe_test_raid_group_get_info(fbe_test_rg_configuration_t *rg_config_p);
void fbe_test_sep_raid_group_find_mirror_obj_and_adj_pos_for_r10(
                                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                                    fbe_u32_t                       in_position,
                                                    fbe_object_id_t*                out_mirror_raid_group_object_id_p,
                                                    fbe_u32_t*                      out_position_p);
fbe_status_t fbe_test_rg_setup_rdgen_test_context(fbe_test_rg_configuration_t *const rg_config_p,
                                                  fbe_api_rdgen_context_t *context_p,
                                                  fbe_bool_t b_sequential_fixed,
                                                  fbe_rdgen_operation_t rdgen_operation,
                                                  fbe_rdgen_pattern_t pattern,
                                                  fbe_u32_t threads,
                                                  fbe_u64_t random_io_max_blocks,
                                                  fbe_bool_t b_inject_random_aborts,
                                                  fbe_bool_t b_run_peer_io,
                                                  fbe_api_rdgen_peer_options_t rdgen_peer_options,
                                                  fbe_lba_t lba,
                                                  fbe_block_count_t blocks);
fbe_status_t fbe_test_rg_run_sync_io(fbe_test_rg_configuration_t *const rg_config_p,
                                         fbe_rdgen_operation_t opcode,
                                         fbe_lba_t lba,
                                         fbe_block_count_t blocks,
                                         fbe_rdgen_pattern_t pattern);
fbe_status_t fbe_test_drain_rdgen_pins(fbe_u32_t wait_seconds);
fbe_status_t fbe_test_sep_raid_group_class_set_emeh_values(fbe_u32_t set_mode,
                                                           fbe_bool_t b_change_threshold,
                                                           fbe_u32_t percent_threshold_increase,
                                                           fbe_bool_t b_persist);
fbe_status_t fbe_test_sep_raid_group_class_get_emeh_values(fbe_raid_group_class_get_extended_media_error_handling_t *get_class_emeh_p);
fbe_status_t fbe_test_sep_raid_group_set_emeh_values(fbe_object_id_t rg_object_id,
                                                     fbe_u32_t set_control,
                                                     fbe_bool_t b_change_threshold,
                                                     fbe_u32_t percent_threshold_increase,
                                                     fbe_bool_t b_persist);
fbe_status_t fbe_test_sep_raid_group_get_emeh_values(fbe_object_id_t rg_object_id,
                                                     fbe_raid_group_get_extended_media_error_handling_t *get_emeh_p,
                                                     fbe_bool_t b_display_values,
                                                     fbe_bool_t b_send_to_both_sps);
fbe_status_t fbe_test_sep_raid_group_validate_emeh_values(fbe_test_rg_configuration_t * const rg_config_p,
                                                          fbe_u32_t raid_group_count,
                                                          fbe_u32_t position_to_evaluate,
                                                          fbe_bool_t b_expect_default,
                                                          fbe_bool_t b_expect_disabled,
                                                          fbe_bool_t b_expect_increase);
fbe_status_t fbe_test_sep_raid_group_set_debug_state(fbe_object_id_t rg_object_id,
                                                     fbe_u64_t set_local_state_mask,     
                                                     fbe_u64_t set_clustered_flags,      
                                                     fbe_u32_t set_raid_group_condition);

#endif /* SEP_TEST_IO_H */
