#ifndef FBE_TEST_IO_H
#define FBE_TEST_IO_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_io_api.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the API for the FBE io testing library.
 *
 * HISTORY
 *   8/8/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_test_io_object.h"
#include "fbe_block_transport.h"

/* Used to mark the end of an array of structures.
 */
#define FBE_TEST_IO_INVALID_FIELD FBE_U32_MAX

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

fbe_status_t fbe_test_io_write_read_check( fbe_object_id_t object_id,
                                           fbe_lba_t lba,
                                           fbe_block_count_t blocks,
                                           fbe_block_size_t exported_block_size,
                                           fbe_block_size_t exported_opt_block_size,
                                           fbe_block_size_t imported_block_size,
                                           fbe_payload_block_operation_status_t *const write_status_p,
                                           fbe_payload_block_operation_qualifier_t * const write_qualifier_p,
                                           fbe_payload_block_operation_status_t *const read_status_p,
                                           fbe_payload_block_operation_qualifier_t * const read_qualifier_p );

fbe_status_t fbe_test_io_run_write_same_test(fbe_test_io_case_t *const cases_p,
                                             fbe_object_id_t object_id,
                                             fbe_u32_t max_case_index);

fbe_status_t fbe_test_io_run_write_read_check_test(fbe_test_io_case_t *const cases_p,
                                                   fbe_object_id_t object_id,
                                                   fbe_u32_t max_case_index);
fbe_status_t fbe_test_io_run_write_verify_read_check_test(fbe_test_io_case_t *const cases_p,
                                                          fbe_object_id_t object_id,
                                                          fbe_u32_t max_case_index);
fbe_status_t fbe_test_io_run_write_only_test(fbe_test_io_case_t *const cases_p,
                                             fbe_object_id_t object_id,
                                             fbe_u32_t max_case_index);
fbe_status_t fbe_test_io_run_verify_test(fbe_test_io_case_t *const cases_p,
                                         fbe_object_id_t object_id,
                                         fbe_u32_t max_case_index);
fbe_status_t fbe_test_io_run_read_only_test(fbe_test_io_case_t *const cases_p,
                                            fbe_object_id_t object_id,
                                            fbe_u32_t max_case_index);


void fbe_test_io_free_memory_with_sg( fbe_sg_element_t **sg_p );
fbe_sg_element_t * fbe_test_io_alloc_memory_with_sg( fbe_u32_t bytes );
fbe_status_t fbe_test_io_negotiate_and_validate_block_size(fbe_object_id_t object_id,
                                 fbe_block_size_t exported_block_size,
                                 fbe_block_size_t exported_optimal_block_size,
                                 fbe_block_size_t expected_imported_block_size,
                                 fbe_block_size_t expected_imported_optimal_block_size);

void fbe_test_io_thread_task_init(fbe_test_io_thread_task_t *const task_p,
                                  fbe_test_io_task_type_t test_type,
                                  fbe_block_size_t exported_block_size,
                                  fbe_block_size_t exported_opt_block_size,
                                  fbe_block_size_t imported_block_size,
                                  fbe_block_size_t imported_opt_block_size,
                                  fbe_u32_t packets_per_thread,
                                  fbe_block_count_t max_blocks,
                                  fbe_u32_t seconds_to_test);
void fbe_test_io_threads(fbe_test_io_thread_task_t *const task_p,
                         const fbe_object_id_t object_id,
                         const fbe_u32_t thread_count);
fbe_bool_t fbe_test_io_get_use_sep(void);
void fbe_test_io_set_use_sep(fbe_bool_t b_use_sep);
#endif /* FBE_TEST_IO_H */
/*************************
 * end file fbe_test_io_api.h
 *************************/


