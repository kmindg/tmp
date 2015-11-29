#ifndef __FBE_STRIPER_TEST_PROTOTYPES_H__
#define __FBE_STRIPER_TEST_PROTOTYPES_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_striper_test_prototypes.h
 ***************************************************************************
 *
 * @brief
 *  This file contains prototypes for the striper unit test.
 *
 * @version
 *   7/30/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/* fbe_striper_test_read.c
 */
void fbe_striper_test_read_add_unit_tests(mut_testsuite_t * const suite_p);
fbe_status_t fbe_striper_test_initialize_striper_object(fbe_raid_geometry_t *raid_geometry_p,
                                                        fbe_block_edge_t *block_edge_p,
                                                        fbe_u32_t width,
                                                        fbe_lba_t capacity);
extern fbe_raid_group_test_io_range_t fbe_raid_group_test_io_tasks[];
fbe_status_t fbe_striper_test_state_destroy(fbe_raid_group_test_state_t * const state_p);

/* fbe_striper_test_generate.c
 */
void fbe_striper_test_generate_add_unit_tests(mut_testsuite_t * const suite_p);

/* fbe_striper_test.c
 */
void fbe_striper_unit_tests_teardown(void);
void fbe_striper_unit_tests_setup(void);

/*************************
 * end file fbe_striper_test_prototypes.h
 *************************/

#endif /* end __FBE_STRIPER_TEST_PROTOTYPES_H__ */
