/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_map.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive object's lba
 *  mapping functionality.
 *
 * HISTORY
 *   05/27/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"
#include "fbe_logical_drive_test_private.h"
#include "fbe_logical_drive_test_state_object.h"
#include "fbe_logical_drive_test_alloc_mem_mock_object.h"
#include "fbe_logical_drive_test_send_io_packet_mock_object.h"
#include "mut.h"

/* This is an LBA that we add on to some test cases to 
 * get an LBA that is beyond 32 bits. 
 */
#define FBE_LDO_LARGE_LBA_OFFSET 0x1000000000000000

/* This structure represents a test case for calculating edge sizes..
 * All of the below represent parameters or return values from 
 * fbe_ldo_get_edge_sizes(). 
 */
typedef struct fbe_ldo_test_get_edge_sizes_task_s
{

    /* The logical block address and block count.
     */
    fbe_lba_t exported_lba;
    fbe_block_count_t exported_blocks;

    /* The block sizes of this command and this device
     */
    fbe_block_size_t exported_block_size;
    fbe_block_size_t exported_opt_block_size;
    fbe_block_size_t imported_block_size;
    fbe_block_size_t imported_opt_block_size;

    /* These are the edge sizes we expect to receive back from 
     *  fbe_ldo_get_edge_sizes().
     */
    fbe_u32_t edge_0;
    fbe_u32_t edge_1;
}
fbe_ldo_test_get_edge_sizes_task_t;

/* This is the global set of test cases.
 */
static fbe_ldo_test_get_edge_sizes_task_t fbe_ldo_test_get_edge_sizes_tasks[] = 
{
    /* 520 exported 
     * 512 imported tests. 
     */
    {0, 64, 520, 64, 512, 65, 0, 0},
    {0, 128, 520, 64, 512, 65, 0, 0},
    {0, 192, 520, 64, 512, 65, 0, 0},
    {64, 64, 520, 64, 512, 65, 0, 0},
    {64, 128, 520, 64, 512, 65, 0, 0},
    {64, 192, 520, 64, 512, 65, 0, 0},
    {192, 64, 520, 64, 512, 65, 0, 0},
    {192, 128, 520, 64, 512, 65, 0, 0},
    {192, 192, 520, 64, 512, 65, 0, 0},

    {0, 1, 520, 64, 512, 65, 0, 504},
    {1, 1, 520, 64, 512, 65, 8, 496},
    {2, 1, 520, 64, 512, 65, 16, 488},
    {3, 1, 520, 64, 512, 65, 24, 480},
    {4, 1, 520, 64, 512, 65, 32, 472},
    {5, 1, 520, 64, 512, 65, 40, 464},
    {6, 1, 520, 64, 512, 65, 48, 456},
    {7, 1, 520, 64, 512, 65, 56, 448},
    {8, 1, 520, 64, 512, 65, 64, 440},
    {59, 1, 520, 64, 512, 65, 472, 32},
    {60, 1, 520, 64, 512, 65, 480, 24},
    {61, 1, 520, 64, 512, 65, 488, 16},
    {62, 1, 520, 64, 512, 65, 496,  8},
    {63, 1, 520, 64, 512, 65, 504,  0},
    {64, 1, 520, 64, 512, 65, 0, 504},
    {65, 1, 520, 64, 512, 65, 8, 496},
    {66, 1, 520, 64, 512, 65, 16, 488},
    {67, 1, 520, 64, 512, 65, 24, 480},
    {68, 1, 520, 64, 512, 65, 32, 472},
    {69, 1, 520, 64, 512, 65, 40, 464},
    {70, 1, 520, 64, 512, 65, 48, 456},
    {71, 1, 520, 64, 512, 65, 56, 448},
    {72, 1, 520, 64, 512, 65, 64, 440},

    {0, 2, 520, 64, 512, 65, 0, 496},
    {1, 2, 520, 64, 512, 65, 8, 488},
    {2, 2, 520, 64, 512, 65, 16, 480},
    {3, 2, 520, 64, 512, 65, 24, 472},
    {4, 2, 520, 64, 512, 65, 32, 464},
    {5, 2, 520, 64, 512, 65, 40, 456},
    {6, 2, 520, 64, 512, 65, 48, 448},
    {7, 2, 520, 64, 512, 65, 56, 440},
    {8, 2, 520, 64, 512, 65, 64, 432},
    {59, 2, 520, 64, 512, 65, 472, 24},
    {60, 2, 520, 64, 512, 65, 480, 16},
    {61, 2, 520, 64, 512, 65, 488,  8},
    {62, 2, 520, 64, 512, 65, 496,  0},
    {63, 2, 520, 64, 512, 65, 504, 504},
    {64, 2, 520, 64, 512, 65, 0, 496},
    {65, 2, 520, 64, 512, 65, 8, 488},
    {66, 2, 520, 64, 512, 65, 16, 480},
    {67, 2, 520, 64, 512, 65, 24, 472},
    {68, 2, 520, 64, 512, 65, 32, 464},
    {69, 2, 520, 64, 512, 65, 40, 456},
    {70, 2, 520, 64, 512, 65, 48, 448},
    {71, 2, 520, 64, 512, 65, 56, 440},
    {72, 2, 520, 64, 512, 65, 64, 432},

    /* 520 exported 4096 imported tests.
     */
    {0, 1, 520, 512, 4096, 65, 0,    3576},
    {1, 1, 520, 512, 4096, 65, 520,  3056},
    {2, 1, 520, 512, 4096, 65, 1040, 2536},
    {3, 1, 520, 512, 4096, 65, 1560, 2016},
    {4, 1, 520, 512, 4096, 65, 2080, 1496},
    {5, 1, 520, 512, 4096, 65, 2600, 976},
    {6, 1, 520, 512, 4096, 65, 3120, 456},
    {7, 1, 520, 512, 4096, 65, 3640, 4032},
    {8, 1, 520, 512, 4096, 65, 64, 3512},
    {9, 1, 520, 512, 4096, 65, 584, 2992},
    {10, 1, 520, 512, 4096, 65, 1104, 2472},
    {11, 1, 520, 512, 4096, 65, 1624, 1952},
    {12, 1, 520, 512, 4096, 65, 2144, 1432},
    {13, 1, 520, 512, 4096, 65, 2664, 912},
    {14, 1, 520, 512, 4096, 65, 3184, 392},
    {15, 1, 520, 512, 4096, 65, 3704, 3968},
    {16, 1, 520, 512, 4096, 65, 128, 3448},
    {17, 1, 520, 512, 4096, 65, 648, 2928},

    {505, 1, 520, 512, 4096, 65, 456,  3120},
    {506, 1, 520, 512, 4096, 65, 976,  2600},
    {507, 1, 520, 512, 4096, 65, 1496, 2080},
    {508, 1, 520, 512, 4096, 65, 2016, 1560},
    {509, 1, 520, 512, 4096, 65, 2536, 1040},
    {510, 1, 520, 512, 4096, 65, 3056, 520},
    {511, 1, 520, 512, 4096, 65, 3576, 0},
    {512, 1, 520, 512, 4096, 65, 0,    3576},
    {513, 1, 520, 512, 4096, 65, 520,  3056},
    {514, 1, 520, 512, 4096, 65, 1040, 2536},
    {515, 1, 520, 512, 4096, 65, 1560, 2016},
    {516, 1, 520, 512, 4096, 65, 2080, 1496},
    {517, 1, 520, 512, 4096, 65, 2600, 976},
    {518, 1, 520, 512, 4096, 65, 3120, 456},
    {519, 1, 520, 512, 4096, 65, 3640, 4032},
    {520, 1, 520, 512, 4096, 65, 64, 3512},
    {521, 1, 520, 512, 4096, 65, 584, 2992},
    {522, 1, 520, 512, 4096, 65, 1104, 2472},
    {523, 1, 520, 512, 4096, 65, 1624, 1952},
    {524, 1, 520, 512, 4096, 65, 2144, 1432},
    {525, 1, 520, 512, 4096, 65, 2664, 912},
    {526, 1, 520, 512, 4096, 65, 3184, 392},
    {527, 1, 520, 512, 4096, 65, 3704, 3968},
    {528, 1, 520, 512, 4096, 65, 128, 3448},
    {529, 1, 520, 512, 4096, 65, 648, 2928},

    /* 520 exported 
     * 512 imported 
     * 2 x 520 exported optimal block size 
     * 3 x 512 imported optimal block size. 
     */
    {0, 1, 520, 2, 512, 3, 0, 504},
    {0, 2, 520, 2, 512, 3, 0, 496},
    {0, 3, 520, 2, 512, 3, 0, 504},
    {0, 4, 520, 2, 512, 3, 0, 496},
    {0, 5, 520, 2, 512, 3, 0, 504},
    {1, 1, 520, 2, 512, 3, 8, 496},
    {1, 2, 520, 2, 512, 3, 8, 504},
    {1, 3, 520, 2, 512, 3, 8, 496},
    {1, 4, 520, 2, 512, 3, 8, 504},

    {20, 1, 520, 2, 512, 3, 0, 504},
    {20, 2, 520, 2, 512, 3, 0, 496},
    {20, 3, 520, 2, 512, 3, 0, 504},
    {20, 4, 520, 2, 512, 3, 0, 496},
    {20, 5, 520, 2, 512, 3, 0, 504},
    {21, 1, 520, 2, 512, 3, 8, 496},
    {21, 2, 520, 2, 512, 3, 8, 504},
    {21, 3, 520, 2, 512, 3, 8, 496},
    {21, 4, 520, 2, 512, 3, 8, 504},

    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0, 0},
};
/* end fbe_ldo_test_get_edge_sizes_tasks */

/*!***************************************************************
 * @fn fbe_ldo_test_get_edge_sizes_case(
 *                     fbe_ldo_test_get_edge_sizes_task_t * task_p)
 ****************************************************************
 * @brief
 *  Test the function fbe_ldo_get_edge_sizes() with
 *  the input task array.
 *
 * @param task_p - The task array to test with.
 *
 * @return None.
 *
 * HISTORY:
 *  5/27/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_get_edge_sizes_case(fbe_ldo_test_get_edge_sizes_task_t * task_p)
{
    fbe_u32_t edge_0;
    fbe_u32_t edge_1;

    /* Loop over all the test cases, feeding each 
     * test case to the function under test. 
     */
    while (task_p->exported_lba != FBE_LDO_TEST_INVALID_FIELD)
    {
        fbe_lba_t exported_lba = task_p->exported_lba;

        fbe_ldo_get_edge_sizes(exported_lba,
                               task_p->exported_blocks,
                               task_p->exported_block_size,
                               task_p->exported_opt_block_size,
                               task_p->imported_block_size,
                               task_p->imported_opt_block_size,
                               &edge_0,
                               &edge_1);
        /* Make sure the result is what we expect.
         */
        MUT_ASSERT_INT_EQUAL(edge_0, task_p->edge_0);
        MUT_ASSERT_INT_EQUAL(edge_1, task_p->edge_1);

        /* Now test cases with a very large lba. 
         * We add on a very large lba to the current test lba. 
         * We assert that it does not wrap. 
         */
        MUT_ASSERT_TRUE(exported_lba + FBE_LDO_LARGE_LBA_OFFSET > exported_lba);

        /* Create the large lba.
         */
        exported_lba += FBE_LDO_LARGE_LBA_OFFSET;

        fbe_ldo_get_edge_sizes(exported_lba,
                               task_p->exported_blocks,
                               task_p->exported_block_size,
                               task_p->exported_opt_block_size,
                               task_p->imported_block_size,
                               task_p->imported_opt_block_size,
                               &edge_0,
                               &edge_1);
        /* Make sure the result is what we expect.
         */
        MUT_ASSERT_INT_EQUAL(edge_0, task_p->edge_0);
        MUT_ASSERT_INT_EQUAL(edge_1, task_p->edge_1);

        /* Increment to the next task.
         */
        task_p++;
    } /* end while not at end of tasks */
    return;
}
/******************************************
 * end fbe_ldo_test_get_edge_sizes_case()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_get_edge_sizes(void)
 ****************************************************************
 * @brief
 *  Executes the get edge sizes case with the input test case array.
 *
 * @param - None.               
 *
 * @return None.   
 *
 * HISTORY:
 *  9/10/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_get_edge_sizes(void)
{
    fbe_ldo_test_get_edge_sizes_case(fbe_ldo_test_get_edge_sizes_tasks);
    return;
}
/******************************************
 * end fbe_ldo_test_get_edge_sizes()
 ******************************************/
/*****************************************
 * end fbe_logical_drive_test_map.c
 *****************************************/
