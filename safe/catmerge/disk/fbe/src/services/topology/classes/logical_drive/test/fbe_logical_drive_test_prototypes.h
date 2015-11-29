#ifndef FBE_LOGICAL_DRIVE_TEST_PROTOTYPES_H
#define FBE_LOGICAL_DRIVE_TEST_PROTOTYPES_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_logical_drive_test_prototypes.h
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains testing prototypes for the logical drive
 *  object.
 *
 * HISTORY
 *   10/29/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe_topology.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"
#include "fbe_logical_drive_test_private.h"
#include "fbe_logical_drive_test_state_object.h"

/* fbe_logical_drive_test.c
 */
fbe_u32_t fbe_logical_drive_setup(void);
fbe_u32_t fbe_logical_drive_teardown(void);
void fbe_logical_drive_unit_tests_setup(void);
void fbe_logical_drive_unit_tests_teardown(void);
fbe_object_id_t fbe_ldo_test_get_object_id_for_drive_block_size(fbe_block_size_t block_size);
fbe_block_size_t fbe_ldo_test_get_block_size_for_drive_index(fbe_u32_t index);
fbe_u32_t fbe_ldo_get_drive_error_count(fbe_block_size_t block_size);

/* fbe_logical_drive_test_map.c
 */
void fbe_ldo_test_get_edge_sizes(void);

/* fbe_logical_drive_test_negotiate_block_size.c
 */
void fbe_ldo_test_add_negotiate_block_size_tests(mut_testsuite_t * const suite_p);
fbe_status_t 
fbe_ldo_test_send_and_validate_negotiate_block_size(fbe_object_id_t object_id,
                                                    fbe_block_size_t requested_block_size,
                                                    fbe_block_size_t requested_opt_block_size,
                                                    fbe_block_transport_negotiate_t *expected_negotiate_p,
                                                    fbe_payload_block_operation_status_t *block_status_p);

/* fbe_logical_drive_test_read.c
 */
void fbe_ldo_test_add_read_unit_tests(mut_testsuite_t * const suite_p);
fbe_u32_t fbe_ldo_test_get_payload_bytes(fbe_payload_ex_t *const payload_p);

/* fbe_logical_drive_test_state_machine.c
 */
fbe_status_t fbe_logical_drive_state_machine_tests(fbe_test_io_task_t *const table_p,
                                                   fbe_ldo_test_sm_test_case_fn const test_function_p);
fbe_status_t fbe_ldo_test_sg_cases(fbe_test_io_task_t *const task_p,
                                   fbe_ldo_test_count_setup_sgs_fn test_fn);
fbe_status_t fbe_ldo_test_execute_state_machine(
    fbe_ldo_test_state_t * const test_state_p,
    fbe_ldo_test_state_setup_fn_t state_setup_fn_p,
    fbe_ldo_test_state_execute_fn_t state_fn_p,
    fbe_ldo_test_state_machine_task_t *const task_p);
void fbe_ldo_test_increment_stack_level(fbe_packet_t *const packet_p);
void fbe_ldo_test_allocate_and_increment_stack_level(fbe_packet_t *const packet_p);

/* fbe_logical_drive_test_write.c
 */
void fbe_ldo_test_add_write_unit_tests(mut_testsuite_t * const suite_p);

/* fbe_logical_drive_test_verify.c
 */
fbe_status_t fbe_ldo_verify_state_machine_tests(void);
fbe_status_t fbe_ldo_test_verify_io(void);
fbe_status_t fbe_ldo_test_write_verify_io(void);

/* fbe_logical_drive_test_io.c
 */
fbe_status_t fbe_ldo_test_read_io(void);
fbe_status_t fbe_ldo_test_write_io(void);
fbe_status_t fbe_ldo_test_write_only_io(void);
fbe_status_t fbe_ldo_test_write_same_io(void);
void fbe_ldo_test_setup_sg_list(fbe_sg_element_t *const sg_p,
                                fbe_u8_t *const memory_p,
                                fbe_u32_t bytes,
                                fbe_u32_t max_bytes_per_sg);
fbe_status_t fbe_ldo_test_io_cmd_sg(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_status_t fbe_ldo_test_thread_ios(void);
fbe_status_t fbe_ldo_test_pass_through_io(void);
fbe_status_t fbe_ldo_test_beyond_capacity_error(void);
fbe_status_t fbe_ldo_test_empty_sg_error(void);
fbe_u32_t fbe_ldo_test_get_first_state(void);
fbe_u32_t fbe_ldo_test_get_attributes_cases(void);
fbe_u32_t fbe_ldo_test_pdo_failure(void);
fbe_status_t fbe_ldo_test_single_write_io(void);
fbe_status_t fbe_ldo_test_zero_block_sizes(void);
fbe_status_t fbe_ldo_test_block_size_too_large(void);

/* fbe_ldo_test_send_packet.c
 */
fbe_status_t fbe_ldo_test_set_block_size(fbe_lba_t capacity,
                                         fbe_block_size_t imported_block_size,
                                         fbe_object_id_t object_id);
/* fbe_logical_drive_test_library.c
 */
void fbe_ldo_test_add_library_tests(mut_testsuite_t * const suite_p);

/* fbe_logical_drive_test_sg_element
 */
void fbe_ldo_test_add_sg_element_tests(mut_testsuite_t * const suite_p);

/* fbe_logical_drive_test_bitbucket.c
 */
void fbe_ldo_test_add_bitbucket_tests(mut_testsuite_t * const suite_p);

/* fbe_logical_drive_test_write_same.c
 */
void fbe_ldo_test_add_write_same_tests(mut_testsuite_t * const suite_p);

/* fbe_logical_drive_test_validate_pre_read_descriptor.c
 */
void fbe_ldo_add_pre_read_descriptor_tests(mut_testsuite_t * const suite_p);

/* fbe_logical_drive_test_io_cmd_pool.c
 */
fbe_status_t fbe_ldo_test_io_cmd_pool(void);

/* fbe_logical_drive_test_monitor.c
 */
void fbe_ldo_test_add_monitor_tests(mut_testsuite_t * const suite_p);

#endif /* FBE_LOGICAL_DRIVE_TEST_PROTOTYPES_H */
/****************************************
 * end fbe_logical_drive_test_private.h
 ****************************************/
