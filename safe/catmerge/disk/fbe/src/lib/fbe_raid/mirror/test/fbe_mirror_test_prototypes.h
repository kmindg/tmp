#ifndef __FBE_MIRROR_TEST_PROTOTYPES_H__
#define __FBE_MIRROR_TEST_PROTOTYPES_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_mirror_test_prototypes.h
 ***************************************************************************
 *
 * @brief
 *  This file contains prototypes for the raid mirror unit test.
 *
 * @version
 *  9/11/2009   Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/**************************************** 
 * fbe_mirror_test.c
 ****************************************/
extern fbe_raid_group_test_io_range_t fbe_mirror_test_common_io_tasks[];
fbe_status_t fbe_mirror_test_initialize_mirror_object(fbe_raid_geometry_t *geometry_p,
                                                      fbe_block_edge_t *block_edge_p, 
                                                      fbe_u32_t width,
                                                      fbe_lba_t capacity);
void fbe_mirror_unit_tests_teardown(void);
void fbe_mirror_unit_tests_setup(void);

/**************************************** 
 * fbe_mirror_test_generate.c
 ****************************************/
void fbe_mirror_test_generate_add_unit_tests(mut_testsuite_t * const suite_p);

/**************************************** 
 * fbe_mirror_test_read.c
 ****************************************/
void fbe_mirror_test_read_add_unit_tests(mut_testsuite_t * const suite_p);

/**************************************** 
 * fbe_mirror_test_write.c
 ****************************************/
void fbe_mirror_test_write_add_unit_tests(mut_testsuite_t * const suite_p);


/***************************************
 * end file fbe_mirror_test_prototypes.h
 ***************************************/

#endif /* end __FBE_MIRROR_TEST_PROTOTYPES_H__ */
