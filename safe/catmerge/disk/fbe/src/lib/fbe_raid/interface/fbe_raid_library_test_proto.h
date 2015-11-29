#ifndef __FBE_RAID_GROUP_TEST_PROTOTYPES_H__
#define __FBE_RAID_GROUP_TEST_PROTOTYPES_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_group_test_prototypes.h
 ***************************************************************************
 *
 * @brief
 *  This file contains prototypes for the raid_group unit test.
 *
 * @version
 *   7/30/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe_raid_library_test.h"
#include "fbe_raid_geometry.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/* fbe_raid_group_test_read.c
 */
void fbe_raid_group_test_read_add_unit_tests(mut_testsuite_t * const suite_p);

/* fbe_raid_group_test.c
 */
fbe_status_t fbe_raid_library_test_initialize_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                                       fbe_block_edge_t *block_edge_p,
                                                       fbe_raid_group_type_t raid_type,
                                                       fbe_u32_t width,
                                                       fbe_element_size_t element_size,
                                                       fbe_elements_per_parity_t elements_per_parity,
                                                       fbe_lba_t capacity,
                                                       fbe_raid_common_state_t generate_state);
void fbe_raid_group_test_init_memory(void);
void fbe_raid_group_test_destroy_memory(void);
void fbe_raid_group_unit_tests_teardown(void);
void fbe_raid_group_unit_tests_setup(void);
void fbe_raid_group_test_build_packet(fbe_packet_t * const packet_p,
                                      fbe_payload_block_operation_opcode_t opcode,
                                      fbe_block_edge_t *edge_p,
                                      fbe_lba_t lba,
                                      fbe_block_count_t blocks,
                                      fbe_block_size_t block_size,
                                      fbe_block_size_t optimal_block_size,
                                      fbe_sg_element_t * const sg_p,
                                      fbe_packet_completion_context_t context);
void fbe_raid_group_test_increment_stack_level(fbe_packet_t *const packet_p);
void fbe_raid_group_test_setup_sg_list(fbe_sg_element_t *const sg_p,
                                       fbe_u8_t *const memory_p,
                                       fbe_u32_t bytes,
                                       fbe_u32_t max_bytes_per_sg);

/* fbe_raid_group_test_state_machine.c
 */
fbe_status_t 
fbe_raid_group_state_machine_tests(fbe_raid_group_test_io_range_t *const table_p,
                                   fbe_raid_group_test_sm_test_case_fn const test_function_p);

/* fbe_raid_group_memory_test.c
 */

/*************************
 * end file fbe_raid_group_test_prototypes.h
 *************************/

#endif /* end __FBE_RAID_GROUP_TEST_PROTOTYPES_H__ */
