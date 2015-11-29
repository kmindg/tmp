#ifndef SEP_TEST_IO_PRIVATE_H
#define SEP_TEST_IO_PRIVATE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file sep_test_io_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the prototypes for private Test SEP I/O methods.
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "sep_utils.h"
#include "fbe/fbe_api_rdgen_interface.h"


/************************************************************
 *  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS
 ************************************************************/
#define FBE_TEST_SEP_BACKGROUND_OPS_ELEMENT_SIZE            128
#define FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT    16

/*!*******************************************************************
 * @enum fbe_test_sep_io_block_operation_test_status_e
 *********************************************************************
 * @brief
 *  This structure contains the status of block operation tesitng info.
 *
 *********************************************************************/
typedef enum fbe_test_sep_io_block_operation_test_status_e
{
    FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_TESTED   = 0,    /*!< MBZ. The block operation test status is invalid (i.e. initialized). */
    FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_CANNOT_TEST  = 1,    /*!< Cannot excersize this block operation from the lun object. */
    FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE = 2,  /*!< This test combination is not applicable to SEP */
    FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_CURRENTLY_UNUSED = 3, /*!< Currently this operation code is not used */ 
    FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO        = 4,    /*!< @todo We currently don't test this but it should be tested. */
    FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TESTED       = 5,    /*!< We have confirmed that this block operation has been tested. */
    FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NEXT         = 6,    /*!< Next available block operation test status */
} fbe_test_sep_io_block_operation_test_status_t;

/*!*******************************************************************
 * @struct fbe_test_sep_io_block_operations_info_t
 *********************************************************************
 * @brief
 *  This structure contains the status of block operation tesitng info.
 *
 *********************************************************************/
typedef struct fbe_test_sep_io_block_operations_info_s
{
    /*! Array of block operation test status without the `check checksum' flag set. 
     */
    fbe_test_sep_io_block_operation_test_status_t   block_operation_status[FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_LAST];

    /*! Array of block operation test status with the `check checksum' flag set. 
     */
    fbe_test_sep_io_block_operation_test_status_t   check_checksum_status[FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_LAST];
} fbe_test_sep_io_block_operations_info_t;
    

/*!*************************************************
 * @typedef fbe_test_sep_background_ops_event_type_t
 ***************************************************
 * @brief This is the event type enumeration.
 ***************************************************/
typedef enum fbe_test_sep_background_ops_event_type_e
{
    FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_INVALID                  = 0,
    FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SOURCE_EDGE_EOL          = 1,
    FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_IN                  = 2,
    FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_MIRROR_MODE              = 3,
    FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_METADATA_REBUILD_START   = 4,
    FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_REBUILD_HOOK             = 5,
    FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_START               = 6,
    FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_COMPLETE            = 7,
    FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_COMPLETE_INITIATED  = 8,
    FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_OUT                 = 9,
    FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_OUT_COMPLETE        = 10,
    FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SOURCE_MARKED_NR         = 11,
    FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_REINSERT_FAILED_DRIVES   = 12,

    FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_LAST

} fbe_test_sep_background_ops_event_type_t;

/*!***************************************************************
 * @struct  fbe_test_sep_background_ops_event_t 
 * 
 * @brief   This structure contains information about the type of
 *          event that we are waiting for.  
 ******************************************************************/
typedef struct fbe_test_sep_background_ops_event_info_s
{
    fbe_test_sep_background_ops_event_type_t  event_type;
    fbe_char_t                               *event_name;
} fbe_test_sep_background_ops_event_info_t;


/***********************
 * FUNCTION PROTOTYPES
 ***********************/

/*********************************
 * fbe_test_sep_io.c
 *********************************/
fbe_test_random_abort_msec_t fbe_test_sep_io_get_configured_rdgen_abort_msecs(void);
fbe_api_rdgen_peer_options_t fbe_test_sep_io_get_configured_rdgen_peer_options(void);
fbe_test_sep_io_sequence_state_t fbe_test_sep_io_get_sequence_io_state(fbe_test_sep_io_sequence_config_t *sequence_config_p);
fbe_status_t fbe_test_sep_io_run_rdgen_tests(fbe_api_rdgen_context_t *context_p,
                                             fbe_package_id_t package_id,
                                             fbe_u32_t num_tests,
                                             fbe_test_rg_configuration_t *rg_config_p);
fbe_u32_t fbe_test_sep_io_get_small_io_count(fbe_u32_t default_io_count);
fbe_u32_t fbe_test_sep_io_get_large_io_count(fbe_u32_t default_io_count);

/**************************************
 * fbe_test_sep_background_operations.c
 **************************************/
fbe_status_t fbe_test_sep_background_ops_get_source_destination_edge_index(fbe_object_id_t vd_object_id,
                                                                           fbe_edge_index_t *source_edge_index_p,
                                                                           fbe_edge_index_t *dest_edge_index_p);
fbe_status_t fbe_test_sep_background_ops_save_source_info(fbe_test_rg_configuration_t *current_rg_config_p,
                                                          fbe_u32_t sparing_position,
                                                          fbe_edge_index_t source_edge_index,
                                                          fbe_edge_index_t dest_edge_index);
fbe_status_t fbe_test_sep_background_ops_populate_io_error_injection_record(fbe_test_rg_configuration_t *rg_config_p,
                                                                         fbe_u32_t drive_pos,
                                                                         fbe_u8_t requested_scsi_command_to_trigger,
                                                                         fbe_protocol_error_injection_error_record_t *record_p,
                                                                         fbe_protocol_error_injection_record_handle_t *record_handle_p);
fbe_test_sep_background_ops_event_info_t *fbe_test_sep_background_ops_get_event_info(fbe_test_sep_background_ops_event_type_t event_type);
fbe_status_t fbe_test_sep_background_ops_run_io_to_start_copy(fbe_test_rg_configuration_t *rg_config_p,
                                                              fbe_u32_t raid_group_count,
                                                              fbe_u32_t sparing_position,
                                                              fbe_u8_t requested_scsi_opcode_to_inject_on,
                                                              fbe_block_count_t io_size);
fbe_status_t fbe_test_sep_background_ops_wait_for_event(fbe_test_rg_configuration_t *rg_config_p, 
                                                        fbe_u32_t raid_group_count,
                                                        fbe_u32_t sparing_position,
                                                        fbe_test_sep_background_ops_event_info_t *event_info_p,
                                                        fbe_edge_index_t original_source_edge_index,
                                                        fbe_edge_index_t original_dest_edge_index);
fbe_status_t fbe_test_sep_background_ops_copy_cleanup_disks(fbe_test_rg_configuration_t *rg_config_p,
                                                            fbe_u32_t raid_group_count,
                                                            fbe_u32_t sparing_position,
                                                            fbe_bool_t b_clear_eol);
fbe_status_t fbe_test_sep_start_background_ops(fbe_test_sep_io_sequence_config_t  *sequence_config_p,
                                               fbe_api_rdgen_context_t *context_p);
fbe_status_t fbe_test_sep_wait_for_background_ops_complete(fbe_test_sep_io_sequence_config_t  *sequence_config_p,
                                                           fbe_u32_t wait_timeout_ms,
                                                           fbe_api_rdgen_context_t *context_p);
fbe_status_t fbe_test_sep_wait_for_background_ops_cleanup(fbe_test_sep_io_sequence_config_t  *sequence_config_p,
                                                          fbe_u32_t wait_timeout_ms,
                                                          fbe_api_rdgen_context_t *context_p);
fbe_status_t fbe_test_sep_background_ops_refresh_raid_group_disk_info(fbe_test_rg_configuration_t *rg_config_p,
                                                                      fbe_u32_t raid_group_count);

/**************************************
 * fbe_test_sep_background_pause.c
 **************************************/


/************************************
 * fbe_test_sep_io_block_operations.c
 ************************************/
fbe_status_t fbe_test_sep_background_ops_cleanup(void);
fbe_status_t fbe_test_sep_io_run_block_operations_sequence(fbe_test_sep_io_sequence_config_t *sequence_config_p);

/*********************************
 * fbe_test_sep_io_caterpillar.c
 *********************************/
fbe_u32_t fbe_test_sep_io_caterpillar_get_small_io_count(void);
fbe_u32_t fbe_test_sep_io_caterpillar_get_large_io_count(void);
fbe_status_t fbe_test_sep_io_run_caterpillar_sequence(fbe_test_sep_io_sequence_config_t *sequence_config_p);

/*********************************
 * fbe_test_sep_io_standard.c
 *********************************/
fbe_status_t fbe_test_sep_io_run_write_pattern(fbe_test_sep_io_sequence_config_t *sequence_config_p,
                                               fbe_api_rdgen_context_t *context_p);
fbe_status_t fbe_test_sep_io_run_read_pattern(fbe_test_sep_io_sequence_config_t *sequence_config_p,
                                              fbe_api_rdgen_context_t *context_p);
fbe_status_t fbe_test_sep_io_run_verify(fbe_test_sep_io_sequence_config_t *sequence_config_p);
fbe_status_t fbe_test_sep_io_run_prefill(fbe_test_sep_io_sequence_config_t *sequence_config_p,
                                         fbe_api_rdgen_context_t *context_p);
fbe_status_t fbe_test_sep_io_run_standard_sequence(fbe_test_sep_io_sequence_config_t *sequence_config_p,
                                                   fbe_api_rdgen_context_t *context_p);
fbe_status_t fbe_test_sep_io_run_standard_abort_sequence(fbe_test_sep_io_sequence_config_t *sequence_config_p,
                                                         fbe_api_rdgen_context_t *context_p);

#endif /* SEP_TEST_IO_PRIVATE_H */
