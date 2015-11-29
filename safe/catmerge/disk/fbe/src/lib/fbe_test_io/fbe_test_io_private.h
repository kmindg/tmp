#ifndef FBE_TEST_IO_PRIVATE_H
#define FBE_TEST_IO_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_io_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private definitions of the fbe test io library.
 *
 * HISTORY
 *   8/8/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_topology.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_logical_drive.h"
#include "mut.h"
#include "fbe/fbe_api_rdgen_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/* fbe_test_io.c
 */
fbe_status_t fbe_test_io_issue_verify( fbe_object_id_t object_id,
                                       fbe_u32_t pass_count,
                                       fbe_lba_t lba,
                                       fbe_block_count_t blocks,
                                       fbe_block_size_t exported_block_size,
                                       fbe_block_size_t exported_opt_block_size,
                                       fbe_u32_t *const qualifier_p );

fbe_sg_element_t * fbe_test_io_alloc_memory_with_sg_offset( fbe_u64_t bytes,
                                                            fbe_u64_t sg_offset,
                                                            fbe_u64_t sg_extra_length,
                                                            fbe_u64_t max_bytes_per_sg_entry);

void fbe_test_io_free_memory_with_sg( fbe_sg_element_t **sg_p );

fbe_u32_t fbe_test_io_issue_write_same( fbe_object_id_t object_id,
                                        fbe_lba_t lba,
                                        fbe_block_count_t blocks,
                                        fbe_block_size_t exported_block_size,
                                        fbe_block_size_t exported_opt_block_size,
                                        fbe_sg_element_t * const sg_p,
                                        fbe_u32_t * const qualifier_p );

fbe_status_t 
fbe_test_io_issue_write( fbe_object_id_t object_id,
                         fbe_payload_block_operation_opcode_t opcode,
                         fbe_lba_t lba,
                         fbe_block_count_t blocks,
                         fbe_block_size_t exported_block_size,
                         fbe_block_size_t exported_opt_block_size,
                         fbe_block_size_t imported_block_size,
                         fbe_sg_element_t * const sg_p,
                         fbe_api_block_status_values_t * const block_status_p,
                         fbe_u32_t *const qualifier_p);

fbe_status_t fbe_test_io_write_same_pattern( fbe_object_id_t object_id,
                                             fbe_test_io_pattern_t pattern,
                                             fbe_u32_t pass_count,
                                             fbe_payload_block_operation_opcode_t opcode,
                                             fbe_lba_t lba,
                                             fbe_block_count_t blocks,
                                             fbe_u32_t exported_block_size,
                                             fbe_block_size_t imported_block_size,
                                             fbe_block_size_t exported_opt_block_size,
                                             fbe_u32_t * const qualifier_p );

fbe_status_t fbe_test_check_buffer_for_pattern(fbe_u32_t tag,
                                               fbe_test_io_pattern_t pattern,
                                               fbe_u32_t pass_count,
                                               fbe_lba_t lba, 
                                               fbe_block_count_t block_count, 
                                               fbe_block_count_t operation_block_count,
                                               fbe_block_count_t block_offset,
                                               fbe_block_size_t block_size,
                                               fbe_u8_t *const data_p);

void fbe_test_fill_seeded_pattern(fbe_u32_t tag,
                                  fbe_test_io_pattern_t pattern,
                                  fbe_u32_t pass_count,
                                  fbe_lba_t lba, 
                                  fbe_block_count_t block_count, 
                                  fbe_block_size_t block_size, 
                                  fbe_u8_t *const data_p);

fbe_u64_t fbe_test_get_word_pattern(fbe_lba_t lba, 
                                    fbe_u32_t word_index,
                                    fbe_u32_t tag,
                                    fbe_test_io_pattern_t pattern_type,
                                    fbe_u32_t pass_count,
                                    fbe_block_size_t block_size, 
                                    fbe_block_count_t block_count,
                                    fbe_block_count_t block_offset);
fbe_status_t fbe_test_io_get_block_size(const fbe_object_id_t object_id,
                                        fbe_block_transport_negotiate_t *const negotiate_p,
                                        fbe_payload_block_operation_status_t *const block_status_p,
                                        fbe_payload_block_operation_qualifier_t *const block_qualifier);

#endif /* FBE_TEST_IO_PRIVATE_H */
/*************************
 * end file fbe_test_io_private.h
 *************************/
