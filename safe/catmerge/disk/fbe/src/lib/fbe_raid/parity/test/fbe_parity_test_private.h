#ifndef PARITY_TEST_LOCAL_H
#define PARITY_TEST_LOCAL_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_parity_test_private.h
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains local definitions for the raid unit test library.
 *
 * HISTORY
 *   10/6/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_raid_library.h"

/*************************
 *   IMPORTED DEFINITIONS
 *************************/

char * _cdecl getenv(const char *);

/* Globals used for displaying progress in our loops. 
 */
extern fbe_u64_t fbe_raid_test_iteration_count;
extern fbe_u64_t fbe_raid_test_last_display_seconds;
extern fbe_u64_t fbe_raid_test_current_seconds;

/* We display progress every RAID_TEST_DISPLAY_SECONDS seconds. 
 */
#define MILLISECONDS_PER_SECOND 1000
#define FBE_RAID_TEST_DISPLAY_SECONDS 10

/*!*******************************************************************
 * @enum fbe_raid_test_option_flags_t
 *********************************************************************
 * @brief
 *  This defines all the possible option flags for the RAID unit tests
 *
 *********************************************************************/
typedef enum fbe_raid_test_option_flags_s
{
    FBE_RAID_TEST_OPTION_FLAG_NONE      = 0x00000000,
    FBE_RAID_TEST_OPTION_FLAG_RL_ENABLE = 0x00000001,   /*< Enable rebuild logging during this test */
}
fbe_raid_test_option_flags_t;

/*!*******************************************************************
 * @def FBE_RAID_TEST_MAX_NR_EXTENT
 *********************************************************************
 * @brief Max number of extents we need to use.
 *
 *********************************************************************/
#define FBE_RAID_TEST_MAX_NR_EXTENT 2                                                    

/*!*******************************************************************
 * @struct fbe_raid_test_generate_parameters_t
 *********************************************************************
 * @brief Set of parameters generate needs to generate iots/siots.
 *
 *********************************************************************/
typedef struct fbe_raid_test_generate_parameters_s
{
    fbe_raid_group_type_t unit_type; 
    fbe_payload_block_operation_opcode_t opcode; 
    fbe_u32_t width; 
    fbe_u32_t cache_page_size; 
    fbe_element_size_t element_size;
    fbe_elements_per_parity_t elements_per_parity;
    fbe_optimum_block_size_t optimal_size;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
}
fbe_raid_test_generate_parameters_t;

/*!*******************************************************************
 * @struct fbe_raid_test_degraded_info_t
 *********************************************************************
 * @brief This is the global info we use to determine the
 *        degraded information. 
 *
 *********************************************************************/
typedef struct fbe_raid_test_degraded_info_s
{
    fbe_raid_test_option_flags_t  test_flags;
    fbe_u32_t                     dead_pos[2];
    fbe_lba_t                     degraded_chkpt[2];
    fbe_raid_nr_extent_t          *nr_extent_p[2];
}
fbe_raid_test_degraded_info_t;

/* This defines a structure that describes an offset from 
 * either the beginning or ending of an element. 
 */
typedef struct fbe_raid_test_element_offset_s
{
    /* Positive offsets are an offset from the start. 
     * Otherwise it is a negative offset, which is an 
     * offset from the end. 
     */
    fbe_bool_t b_positive;

    fbe_u32_t offset; /* offset in blocks */
}
fbe_raid_test_element_offset_t;

/* Functions declare for visibility across files.
 */
void fbe_parity_test_generate_add_tests(mut_testsuite_t *suite_p);
void fbe_parity_unit_tests_setup(void);
void fbe_parity_unit_tests_teardown(void);
/*************************
 * end file fbe_parity_test_private.h
 *************************/

#endif /* PARITY_TEST_LOCAL_H */
