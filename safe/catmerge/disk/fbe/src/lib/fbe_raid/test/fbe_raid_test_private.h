#ifndef UTEST_RG_static_H
#define UTEST_RG_static_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2001
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*******************
 * INCLUDE FILES
 ******************/
#include "fbe/fbe_types.h"
#include "fbe_raid_library.h"


/********************************************************************
 * MACRO DECLARATION(S):
 * ---------------------
 *
 * This section of file contains MACROS used by various unit test
 * cases of RAID driver as well as GLOBAL DATA defined in various
 * files.
 *******************************************************************/
#define   MAX_RAID_CQ_ELEMENT       10
#define   INVALID_VAL               (-1)
#define   MAX_BM_MEMORY_ID          10
#define   MAX_TRANSFER_BLOCKS       0x1000
#define   MAX_TYPE_OF_ELSZ          7
#define   MAX_OFFSET_POSITION       6
#define   MAX_UINT32_VALUE          ((fbe_u32_t)-1)

#define   SRC_SGL_INDEX         0 
#define   BACKFILL_SGL_INDEX    1 
#define   DEST_SGL_INDEX        2
#define   PR_SGL_INDEX          3

#define PARTIAL_TOKEN_LIST_COUNT    6
#define MAX_ELEMENT_PER_TOKEN_LIST  4

#define   TOKEN                     0xffffffff


/********************************************************************
 * DATA TYPE DECLARATION(S):
 * -------------------------
 *
 * This section of file contains enum declaration, macro 
 * declaration related to automated unit test case meant for SG 
 * lists utility functions.
 *******************************************************************/



/********************************************************************
 * IMPORTED DATA STRUCTURE(S):
 * ---------------------------
 *
 * This section of file imports the global data defined in various
 * other files. Global data is used by unit test cases of 
 * RAID DRIVER.
 *******************************************************************/
extern fbe_u32_t utest_elsz_type[];
extern fbe_u32_t utest_std_offset_pos[];
extern fbe_raid_common_t utest_rg_common_queue_elements[];
extern fbe_raid_iots_t utest_iots_pool[]; 
extern fbe_raid_fruts_t utest_fruts_pool[];
extern fbe_sg_element_t utest_sg_pool[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH][FBE_RAID_MAX_SG_ELEMENTS];
extern fbe_raid_memory_header_t utest_mv_pool[];
extern fbe_sector_t utest_sector_pool[];
extern fbe_u32_t utest_token_list[PARTIAL_TOKEN_LIST_COUNT][MAX_ELEMENT_PER_TOKEN_LIST];

    
void fbe_raid_library_test_add_sg_util_tests(mut_testsuite_t * const suite_p);   
void fbe_raid_memory_test_add_tests(mut_testsuite_t *suite_p);
#endif /*  UTEST_RG_static_H */

