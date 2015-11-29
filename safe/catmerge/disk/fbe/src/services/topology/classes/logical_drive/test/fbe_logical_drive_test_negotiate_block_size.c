/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_negotiate_block_size.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive's negotiate block
 *  size functionality.
 *
 * HISTORY
 *   10/29/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_test_private.h"
#include "fbe_logical_drive_test_prototypes.h"
#include "fbe/fbe_api_logical_drive_interface.h"

/* This structure contains the negotiate block size test cases.
 * These cases are used when we are sending real negotiate packets to 
 * the logical drive.
 */
fbe_ldo_test_negotiate_block_size_case_t fbe_ldo_test_negotiate_block_size_data[] = 
{
    /* No matter what optimum block size we negotiate for with 520, we always 
     * should get 1.
     *  
     * Imported Imported Req    Req    Failure   Expected Negotiate Data
     * Capacity Bl size. Bl Sz  Opt Sz Expected  Block      Block Opt    Block Imported 
     *                                           Count      Size  Block  Size  Size
     *                                                            Size
     */
    {0x10b5c730, 520,     520,   1,     0,       0x10b5c730, 520, 1,  0, 520, 1, 520, 1},
    {0x10b5c730, 520,     520,   2,     0,       0x10b5c730, 520, 1,  0, 520, 1, 520, 1},
    {0x10b5c730, 520,     520,   3,     0,       0x10b5c730, 520, 1,  0, 520, 1, 520, 1},
    {0x10b5c730, 520,     520,   4,     0,       0x10b5c730, 520, 1,  0, 520, 1, 520, 1},
    {0x10b5c730, 520,     520,   5,     0,       0x10b5c730, 520, 1,  0, 520, 1, 520, 1},
    {0x10b5c730, 520,     520,   6,     0,       0x10b5c730, 520, 1,  0, 520, 1, 520, 1},
    {0x10b5c730, 520,     520,   30,    0,       0x10b5c730, 520, 1,  0, 520, 1, 520, 1},

    /* For 520/512, we should be able to negotiate with either 1 or 64 and end 
     * up with the same optimal block size.
     * We no longer support 512 byte/sector block sizes
     */
    
#ifdef BLOCKSIZE_512_SUPPORTED    
    {0x10b5c730, 512,     520,   1,     0,       0x1073F740, 520, 64, 0, 512, 65, 520, 1},
    {0x10b5c730, 512,     520,  64,     0,       0x1073F740, 520, 64, 0, 512, 65, 520, 64},
#endif    

    /* For 520/4096, we should be able to negotiate with either 1 or 512 and end
     * up with the same optimal block size. 
     */
    {0x10b5c730, 4096,    520,   1,     0,       0x839FBA00, 520, 512, 0, 4096, 65, 520, 1},
    {0x10b5c730, 4096,    520,  512,    0,       0x839FBA00, 520, 512, 0, 4096, 65, 520, 512},

    /* For 520/4160, we should be able to negotiate with either 1 or 8 and end
     * up with the same optimal block size. 
     */
    {0x10b5c730, 4160,    520,   1,     0,       0x85AE3980, 520, 8,   0, 4160, 1, 520, 1},
    {0x10b5c730, 4160,    520,   8,     0,       0x85AE3980, 520, 8,   0, 4160, 1, 520, 8},

    /* Next, negotiate for some 520/512 lossy cases.
     * We no longer support 512 byte/sector block sizes
     *
     * Imported Imported Req    Req    Failure   Expected Negotiate Data
     * Capacity Bl size. Bl Sz  Opt Sz Expected
     */
    
#ifdef BLOCKSIZE_512_SUPPORTED       
    {0x10b5c730, 512,     520,   2,     0,       0xB23DA20, 520, 2, 0, 512, 3, 520, 2},
    {0x10b5c730, 512,     520,   3,     0,       0xC885564, 520, 3, 0, 512, 4, 520, 3},
    {0x10b5c730, 512,     520,   4,     0,       0xD5E38F0, 520, 4, 0, 512, 5, 520, 4},
    {0x10b5c730, 512,     520,   5,     0,       0xDECD0A8, 520, 5, 0, 512, 6, 520, 5},
    {0x10b5c730, 512,     520,   6,     0,       0xE52AAB8, 520, 6, 0, 512, 7, 520, 6},
    {0x10b5c730, 512,     520,   7,     0,       0xE9F0E4A, 520, 7, 0, 512, 8, 520, 7},
    {0x10b5c730, 512,     520,   8,     0,       0xEDA7828, 520, 8, 0, 512, 9, 520, 8},
    {0x10b5c730, 512,     520,  16,     0,       0xFBA24E0, 520, 16, 0, 512, 17, 520, 16},
    {0x10b5c730, 512,     520,  32,     0,       0x10342600, 520, 32, 0, 512, 33, 520, 32},
#endif     

    /* 520/4096 lossy cases.
     * For these cases note how when we negotiate for an optimum block size that 
     * has more than an exported block size worth of "loss", the LDO will round 
     * up the optimum block size to minimize the waste. 
     *  
     * Imported Imported Req    Req    Failure   Expected Negotiate Data
     * Capacity Bl size. Bl Sz  Opt Sz Expected 
     */
    {0x10b5c730, 4096,    520,   7,     0,       0x74F87250, 520, 7,  0, 4096, 1, 520, 1},
    {0x10b5c730, 4096,    520,   8,     0,       0x7D5355E8, 520, 15, 0, 4096, 2, 520, 1},
    {0x10b5c730, 4096,    520,   9,     0,       0x7D5355E8, 520, 15, 0, 4096, 2, 520, 1},
    {0x10b5c730, 4096,    520,   10,    0,       0x7D5355E8, 520, 15, 0, 4096, 2, 520, 1},
    {0x10b5c730, 4096,    520,   11,    0,       0x7D5355E8, 520, 15, 0, 4096, 2, 520, 1},
    {0x10b5c730, 4096,    520,   12,    0,       0x7D5355E8, 520, 15, 0, 4096, 2, 520, 1},
    {0x10b5c730, 4096,    520,   13,    0,       0x7D5355E8, 520, 15, 0, 4096, 2, 520, 1},
    {0x10b5c730, 4096,    520,   14,    0,       0x7D5355E8, 520, 15, 0, 4096, 2, 520, 1},
    {0x10b5c730, 4096,    520,   15,    0,       0x7D5355E8, 520, 15, 0, 4096, 2, 520, 1},
    {0x10b5c730, 4096,    520,   16,    0,       0x801C4C70, 520, 23, 0, 4096, 3, 520, 1},
    {0x10b5c730, 4096,    520,   17,    0,       0x801C4C70, 520, 23, 0, 4096, 3, 520, 1},
    {0x10b5c730, 4096,    520,   23,    0,       0x801C4C70, 520, 23, 0, 4096, 3, 520, 1},
    {0x10b5c730, 4096,    520,   24,    0,       0x8180C7B4, 520, 31, 0, 4096, 4, 520, 1},
    {0x10b5c730, 4096,    520,   25,    0,       0x8180C7B4, 520, 31, 0, 4096, 4, 520, 1},
    {0x10b5c730, 4096,    520,   30,    0,       0x8180C7B4, 520, 31, 0, 4096, 4, 520, 1},
    {0x10b5c730, 4096,    520,   31,    0,       0x8180C7B4, 520, 31, 0, 4096, 4, 520, 1},

    /* Test cases where the negotiate fails.
     * We simply choose to negotiate for a block size that is not supported. 
     *  
     * Imported Imported Req    Req    Failure   Expected Negotiate Data
     * Capacity Bl size. Bl Sz  Opt Sz Expected 
     */
    {0x10b5c730, 520,    1024,   1,     1,       0x10b5c730, 520, 1,  0, 520, 1, 520, 1},
    {0x10b5c730, 520,     512,   1,     1,       0x10b5c730, 520, 1,  0, 520, 1, 520, 1},
    {0x10b5c730, 512,     521,   1,     1,       0x1073F740, 520, 64, 0, 512, 65, 520, 1},
    {0x10b5c730, 512,    1024,   1,     1,       0x1073F740, 520, 64, 0, 512, 65, 520, 64},
    {0x10b5c730, 4096,    521,   1,     1,       0x839FBA00, 520, 512, 0, 4096, 65, 520, 1},
    {0x10b5c730, 4096,   1024,   1,     1,       0x839FBA00, 520, 512, 0, 4096, 65, 520, 512},
    {0x10b5c730, 4160,    128,   1,     1,       0x85AE3980, 520, 8,   0, 4160, 1, 520, 1},
    {0x10b5c730, 4160,    521,   8,     1,       0x85AE3980, 520, 8,   0, 4160, 1, 520, 8},

    /* Insert any new test cases above this point.
     */
    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}      /* zero means end. */
};

/* This structure contains the negotiate block size test cases,
 * which are used for testing the negotiate calculation functions of the logical
 * drive. 
 */
fbe_ldo_test_negotiate_block_size_case_t fbe_ldo_test_calculate_negotiate_data[] = 
{
    /* 520 cases.  We vary the capacity, but there are only aligned capacities
     *             since the exported is a equal to the imported.
     
     * Imported Imported Req    Req    Failure   Expected Negotiate Data
     * Capacity Bl size. Bl Sz  Opt Sz Expected 
     */
    {0x52000,    520,     520,   1,     0,       0x52000, 520, 1,  0, 520, 1, 520, 1},  /* aligned. */
    {0x52001,    520,     520,   1,     0,       0x52001, 520, 1,  0, 520, 1, 520, 1},  /* aligned. */

    /* 520 cases around 2 TB boundary.
     *  
     * Imported Imported Req    Req    Failure   Expected Negotiate Data
     * Capacity Bl size. Bl Sz  Opt Sz Expected 
     */
    {0xFFFFFFF0, 520,     520,   1,     0,     0xFFFFFFF0, 520, 1,  0, 520, 1, 520, 1},  /* aligned. */
    {0xFFFFFFFF, 520,     520,   1,     0,     0xFFFFFFFF, 520, 1,  0, 520, 1, 520, 1},  /* aligned. */
    {0x100000000,520,     520,   1,     0,    0x100000000, 520, 1,  0, 520, 1, 520, 1},  /* aligned. */
    {0x100000001,520,     520,   1,     0,    0x100000001, 520, 1,  0, 520, 1, 520, 1},  /* aligned. */
    {0x200000002,520,     520,   1,     0,    0x200000002, 520, 1,  0, 520, 1, 520, 1},  /* aligned. */
    {0x400000005,520,     520,   1,     0,    0x400000005, 520, 1,  0, 520, 1, 520, 1},  /* aligned. */

    /* 512 cases.  We vary the capacity to test with aligned and unaligned.
     */
    {0x51200,   512,     520,   2,     0,        0x36154, 520, 2, 0, 512, 3, 520, 2},/* aligned. */
    {0x51201,   512,     520,   2,     0,        0x36156, 520, 2, 0, 512, 3, 520, 2},/* unaligned. */
    {0x51202,   512,     520,   2,     0,        0x36156, 520, 2, 0, 512, 3, 520, 2},/* unaligned. */
    {0x51203,   512,     520,   2,     0,        0x36156, 520, 2, 0, 512, 3, 520, 2},/* aligned. */
    {0x51204,   512,     520,   2,     0,        0x36158, 520, 2, 0, 512, 3, 520, 2},/* aligned. */

    /* 520/512 cases.  We vary the capacity to test aligned and
     * unaligned capacity values.  This allows us to test the code 
     * that aligns the capacity exported to the optimum block size. 
     *  
     * Imported Imported Req    Req    Failure   Expected Negotiate Data
     * Capacity Bl size. Bl Sz  Opt Sz Expected 
     */
    {0x511f8,   512,     520,   1,     0,        0x4fe00, 520, 64, 0, 512, 65, 520, 1}, /* aligned. */
    {0x51239,   512,     520,   1,     0,        0x4fe40, 520, 64, 0, 512, 65, 520, 1}, /* aligned. */
    {0x5127A,   512,     520,   1,     0,        0x4fe80, 520, 64, 0, 512, 65, 520, 1}, /* aligned. */
    {0x512BB,   512,     520,   1,     0,        0x4feC0, 520, 64, 0, 512, 65, 520, 1}, /* aligned. */
    {0x512FC,   512,     520,   1,     0,        0x4fF00, 520, 64, 0, 512, 65, 520, 1}, /* aligned. */
    {0x5133D,   512,     520,   1,     0,        0x4fF40, 520, 64, 0, 512, 65, 520, 1}, /* aligned. */
    {0x5137E,   512,     520,   1,     0,        0x4fF80, 520, 64, 0, 512, 65, 520, 1}, /* aligned. */
    {0x513BF,   512,     520,   1,     0,        0x4fFC0, 520, 64, 0, 512, 65, 520, 1}, /* aligned. */
    {0x51400,   512,     520,   1,     0,        0x50000, 520, 64, 0, 512, 65, 520, 1}, /* aligned. */

    {0x51200,   512,     520,   1,     0,        0x4fe00, 520, 64, 0, 512, 65, 520, 1},/* unaligned. */
    {0x51201,   512,     520,   1,     0,        0x4fe00, 520, 64, 0, 512, 65, 520, 1}, /* unaligned. */
    {0x51202,   512,     520,   1,     0,        0x4fe00, 520, 64, 0, 512, 65, 520, 1}, /* unaligned. */
    {0x51203,   512,     520,   1,     0,        0x4fe00, 520, 64, 0, 512, 65, 520, 1}, /* unaligned. */
    {0x51204,   512,     520,   1,     0,        0x4fe00, 520, 64, 0, 512, 65, 520, 1}, /* unaligned. */
    {0x51205,   512,     520,   1,     0,        0x4fe00, 520, 64, 0, 512, 65, 520, 1}, /* unaligned. */
    {0x51206,   512,     520,   1,     0,        0x4fe00, 520, 64, 0, 512, 65, 520, 1}, /* unaligned. */
    {0x51207,   512,     520,   1,     0,        0x4fe00, 520, 64, 0, 512, 65, 520, 1}, /* unaligned. */
    {0x51208,   512,     520,   1,     0,        0x4fe00, 520, 64, 0, 512, 65, 520, 1}, /* unaligned. */
    {0x51209,   512,     520,   1,     0,        0x4fe00, 520, 64, 0, 512, 65, 520, 1}, /* unaligned. */

    /* 520/4096 cases. We vary the capacity to test aligned and unaligned.
     *  
     * Imported Imported Req    Req    Failure   Expected Negotiate Data
     * Capacity Bl size. Bl Sz  Opt Sz Expected 
     */
    {0x511f8,   4096,    520,   1,     0,        0x27f000, 520, 512, 0, 4096, 65, 520, 1}, /* aligned. */
    {0x51239,   4096,    520,   1,     0,        0x27f200, 520, 512, 0, 4096, 65, 520, 1}, /* aligned. */
    {0x5127A,   4096,    520,   1,     0,        0x27f400, 520, 512, 0, 4096, 65, 520, 1}, /* aligned. */
    {0x512BB,   4096,    520,   1,     0,        0x27f600, 520, 512, 0, 4096, 65, 520, 1}, /* aligned. */
    {0x512FC,   4096,    520,   1,     0,        0x27f800, 520, 512, 0, 4096, 65, 520, 1}, /* aligned. */
    {0x51200,   4096,    520,   1,     0,        0x27f000, 520, 512, 0, 4096, 65, 520, 1}, /* unaligned. */
    {0x51201,   4096,    520,   1,     0,        0x27f000, 520, 512, 0, 4096, 65, 520, 1}, /* unaligned. */
    {0x51202,   4096,    520,   1,     0,        0x27f000, 520, 512, 0, 4096, 65, 520, 1}, /* unaligned. */
    {0x51203,   4096,    520,   1,     0,        0x27f000, 520, 512, 0, 4096, 65, 520, 1}, /* unaligned. */
    {0x51204,   4096,    520,   1,     0,        0x27f000, 520, 512, 0, 4096, 65, 520, 1}, /* unaligned. */
    {0x51205,   4096,    520,   1,     0,        0x27f000, 520, 512, 0, 4096, 65, 520, 1}, /* unaligned. */
    {0x51206,   4096,    520,   1,     0,        0x27f000, 520, 512, 0, 4096, 65, 520, 1}, /* unaligned. */

    /* 520/4104 cases. We vary the capacity to test aligned and unaligned.
     *  
     * Imported Imported Req    Req    Failure   Expected Negotiate Data
     * Capacity Bl size. Bl Sz  Opt Sz Expected 
     */
    {0x511F8,   4104,    520,   1,     0,        0x2803F8, 520, 513, 0, 4104, 65, 520, 1}, /* aligned. */
    {0x513F9,   4104,    520,   1,     0,        0x2813C8, 520, 513, 0, 4104, 65, 520, 1}, /* aligned. */
    {0x515FA,   4104,    520,   1,     0,        0x282399, 520, 513, 0, 4104, 65, 520, 1}, /* aligned. */
    {0x517FB,   4104,    520,   1,     0,        0x28336A, 520, 513, 0, 4104, 65, 520, 1}, /* aligned. */
    {0x519FC,   4104,    520,   1,     0,        0x28433B, 520, 513, 0, 4104, 65, 520, 1}, /* aligned. */
    {0x51BFD,   4104,    520,   1,     0,        0x28530B, 520, 513, 0, 4104, 65, 520, 1}, /* aligned. */
    {0x51200,   4104,    520,   1,     0,        0x2803F8, 520, 513, 0, 4104, 65, 520, 1}, /* unaligned. */
    {0x51201,   4104,    520,   1,     0,        0x2803F8, 520, 513, 0, 4104, 65, 520, 1}, /* unaligned. */
    {0x51202,   4104,    520,   1,     0,        0x2803F8, 520, 513, 0, 4104, 65, 520, 1}, /* unaligned. */
    {0x51202,   4104,    520,   1,     0,        0x2803F8, 520, 513, 0, 4104, 65, 520, 1}, /* unaligned. */
    {0x51202,   4104,    520,   1,     0,        0x2803F8, 520, 513, 0, 4104, 65, 520, 1}, /* unaligned. */
    {0x51202,   4104,    520,   1,     0,        0x2803F8, 520, 513, 0, 4104, 65, 520, 1}, /* unaligned. */

    /* 520/4160 cases. We vary the capacity to test aligned and unaligned.
     *  
     * Imported Imported Req    Req    Failure   Expected Negotiate Data
     * Capacity Bl size. Bl Sz  Opt Sz Expected 
     */
    {0x51200,   4160,    520,   1,     0,        0x289000, 520, 8,   0, 4160, 1, 520, 1},  /* aligned. */
    {0x51201,   4160,    520,   1,     0,        0x289008, 520, 8,   0, 4160, 1, 520, 1},  /* aligned. */
    {0x51202,   4160,    520,   1,     0,        0x289010, 520, 8,   0, 4160, 1, 520, 1},  /* aligned. */
    {0x51203,   4160,    520,   1,     0,        0x289018, 520, 8,   0, 4160, 1, 520, 1},  /* aligned. */
    {0x51204,   4160,    520,   1,     0,        0x289020, 520, 8,   0, 4160, 1, 520, 1},  /* aligned. */
    {0x51205,   4160,    520,   1,     0,        0x289028, 520, 8,   0, 4160, 1, 520, 1},  /* aligned. */
    {0x51206,   4160,    520,   1,     0,        0x289030, 520, 8,   0, 4160, 1, 520, 1},  /* aligned. */
    {0x51207,   4160,    520,   1,     0,        0x289038, 520, 8,   0, 4160, 1, 520, 1},  /* aligned. */

    /* If we try to negotiate for a block size other than 520, we might receive 
     * an error.
     *  
     * Imported Imported Req    Req    Failure   Expected Negotiate Data
     * Capacity Bl size. Bl Sz  Opt Sz Expected 
     */
    {0x10b5c730, 520,     512,   1,     1,       0x10b5c730, 520, 1,  0, 520, 1, 512, 1},
    {0x10b5c730, 512,     528,   1,     1,       0x1073F752, 520, 64, 0, 512, 65, 528, 1},
    {0x10b5c730, 4096,    512,   1,     1,       0x839FBA00, 520, 512, 0, 4096, 65, 512, 1},
    {0x10b5c730, 4160,    512,   1,     1,       0x85AE3980, 520, 8,   0, 4160, 1, 512, 1},

    /* We should always be able to negotiate for the physical block size.
     */
    {0x10b5c730, 512,     512,   1,     0,       0x10b5c730, 512, 1, 0, 512, 1, 512, 1},
    {0x10b5c730, 4096,    4096,  1,     0,       0x10b5c730, 4096, 1, 0, 4096, 1, 4096, 1},
    {0x10b5c730, 4160,    4160,  1,     0,       0x10b5c730, 4160, 1, 0, 4160, 1, 4160, 1},

    /* For future block sizes we should also be able to negotiate the physical 
     * block size. 
     */
    {0x10b5c730, 4000,    4000,  1,     0,       0x10b5c730, 1, 0, 4000, 1, 4000, 1},
    {0x10b5c730, 5000,    5000,  1,     0,       0x10b5c730, 5000, 1, 0, 5000, 1, 5000, 1},

    /* Insert any new test cases above this point.
     */
    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0, 0, 0, 0}      /* INVALID means end. */
};

/******************************
 ** LOCAL FUNCTION PROTOTYPES *
 ******************************/
static fbe_status_t 
fbe_ldo_test_validate_negotiate_block_size_data( fbe_block_transport_negotiate_t *received_capacity_p,
                                                 fbe_block_transport_negotiate_t *expected_capacity_p );


static fbe_status_t fbe_ldo_test_send_negotiate_block_size_cases(void);
static fbe_status_t 
fbe_ldo_test_validate_negotiate_block_size_data( fbe_block_transport_negotiate_t *received_negotiate_p,
                                                 fbe_block_transport_negotiate_t *expected_negotiate_p );
static fbe_u32_t fbe_ldo_test_negotiate_block_size_invalid_sg_case(void);
static fbe_u32_t fbe_ldo_test_negotiate_block_size_invalid_buffer_case(void);
static fbe_status_t fbe_ldo_test_negotiate_invalid_data_case(fbe_object_id_t object_id);
static fbe_u32_t fbe_ldo_test_negotiate_block_size_invalid_data_cases(void);
static fbe_status_t fbe_ldo_test_state_setup_negotiate(fbe_ldo_test_state_t * const test_state_p);
static fbe_status_t fbe_ldo_test_negotiate_case(fbe_ldo_test_negotiate_block_size_case_t * const case_p);
static fbe_status_t fbe_ldo_test_calculate_negotiate_cases(void);

/*!***************************************************************
 * @fn fbe_ldo_test_send_negotiate_block_size_cases(void)
 ****************************************************************
 * @brief
 *  This function runs all of the test cases for negotiate block size.
 *  We loop over all the test cases from the
 *  fbe_ldo_test_negotiate_block_size_data[] test case array.
 *  For each case, an actual negotiate block size packet is sent to the logical
 *  drive.
 *
 * @param  - None.
 *
 * @return:
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * HISTORY:
 *  11/05/07 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t fbe_ldo_test_send_negotiate_block_size_cases(void)
{
    fbe_u32_t test_index; 
    fbe_status_t status = FBE_STATUS_OK;
    
    /* Simply loop over the test cases.  If we hit a zero test case, or if
     * we hit an error, then exit.
     */
    for (test_index = 0; 
         fbe_ldo_test_negotiate_block_size_data[test_index].imported_capacity != 
          FBE_LDO_TEST_INVALID_FIELD;
         test_index++)
    {
        fbe_payload_block_operation_status_t block_status;

        /* First get the needed values from this test_index position of the 
         * test case array.
         */
        fbe_block_size_t block_size =
            fbe_ldo_test_negotiate_block_size_data[test_index].imported_block_size;
        fbe_object_id_t object_id = 
            fbe_ldo_test_get_object_id_for_drive_block_size(block_size);
        fbe_block_transport_negotiate_t *expected_p = 
            &fbe_ldo_test_negotiate_block_size_data[test_index].negotiate_data_expected;
        fbe_block_size_t requested_block_size = 
            fbe_ldo_test_negotiate_block_size_data[test_index].requested_block_size;
        fbe_block_size_t requested_opt_block_size = 
            fbe_ldo_test_negotiate_block_size_data[test_index].requested_opt_block_size;

        /* Now negotiate the block size and make sure what we get back is expected.
         */
        status = fbe_ldo_test_send_and_validate_negotiate_block_size(object_id,
                                                                     requested_block_size,
                                                                     requested_opt_block_size,
                                                                     expected_p,
                                                                     &block_status);
        /* Make sure the status values are expected.
         */
        MUT_ASSERT_TRUE((status == FBE_STATUS_OK && 
                         block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) || 
                        fbe_ldo_test_negotiate_block_size_data[test_index].b_failure_expected);
    } /* end for all test cases. */

    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_send_negotiate_block_size_cases */

/*!**************************************************************
 * @fn fbe_ldo_test_send_and_validate_negotiate_block_size(
 *        fbe_object_id_t object_id,
 *        fbe_block_size_t requested_block_size,
 *        fbe_block_size_t requested_opt_block_size,
 *        fbe_block_transport_negotiate_t *expected_negotiate_p,
 *        fbe_payload_block_operation_status_t *block_status_p)
 ****************************************************************
 * @brief
 *  This function sends a negotiate block size,
 *  and validates that the data received is as expected.
 *
 * @param object_id - object to send command to.
 * @param requested_block_size - Size to negotiate for.
 * @param requested_opt_block_size - Size to negotiate for.
 * @param expected_negotiate_p - The data expected in return.
 * @param block_status_p - The block status to be returned.
 *
 * @return fbe_status_t - status of the operation.   
 *
 * HISTORY:
 *  8/20/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_ldo_test_send_and_validate_negotiate_block_size(fbe_object_id_t object_id,
                                                    fbe_block_size_t requested_block_size,
                                                    fbe_block_size_t requested_opt_block_size,
                                                    fbe_block_transport_negotiate_t *expected_negotiate_p,
                                                    fbe_payload_block_operation_status_t *block_status_p)
{
    fbe_status_t status;
    fbe_block_transport_negotiate_t received_negotiate;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;

    /* Set the negotiate sizes we are requesting.
     */  
    received_negotiate.requested_block_size = requested_block_size;
    received_negotiate.requested_optimum_block_size = requested_opt_block_size;

    /* Issue the negotiate.
     */
	status  = fbe_api_logical_drive_get_drive_block_size (object_id,
														  &received_negotiate,
														  &block_status,
														  &block_qualifier);

    /*status = fbe_api_send_negotiate_block_size( object_id, &received_negotiate,
                                                    &block_status, &block_qualifier);*/

    /* If both the overall packet status and the payload status are OK, 
     * then we will check the returned data.
     */
    if (status == FBE_STATUS_OK && block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        /* Validate the received values match the expected values.
         */
        status = fbe_ldo_test_validate_negotiate_block_size_data(&received_negotiate,
                                                                 expected_negotiate_p);
    }
    /* Save the block status for return.
     */
    *block_status_p = block_status;
    return status;
}
/******************************************
 * end fbe_ldo_test_send_and_validate_negotiate_block_size()
 ******************************************/

/*!***************************************************************
 * @fn fbe_ldo_test_validate_negotiate_block_size_data(
 *  fbe_block_transport_negotiate_t *received_negotiate_p,
 *  fbe_block_transport_negotiate_t *expected_negotiate_p)
 ****************************************************************
 * @brief
 *  This function validates that the received negotiate block size
 *  data is the same as what was expected.
 *
 * @param received_negotiate_p - Received negotiate block size data.
 * @param expected_negotiate_p - The expected negotiate block size values. 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * HISTORY:
 *  11/02/07 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t 
fbe_ldo_test_validate_negotiate_block_size_data( fbe_block_transport_negotiate_t *received_negotiate_p,
                                                 fbe_block_transport_negotiate_t *expected_negotiate_p )
{
    /* Check to make sure that all fields are as expected.
     */
    MUT_ASSERT_TRUE(received_negotiate_p->block_size == expected_negotiate_p->block_size);
    MUT_ASSERT_TRUE(received_negotiate_p->block_count == expected_negotiate_p->block_count);

    MUT_ASSERT_TRUE(received_negotiate_p->optimum_block_size == expected_negotiate_p->optimum_block_size);
    MUT_ASSERT_TRUE(received_negotiate_p->optimum_block_alignment == expected_negotiate_p->optimum_block_alignment);

    MUT_ASSERT_TRUE(received_negotiate_p->physical_block_size == expected_negotiate_p->physical_block_size);
    MUT_ASSERT_TRUE(received_negotiate_p->physical_optimum_block_size == expected_negotiate_p->physical_optimum_block_size);
    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_validate_negotiate_block_size_data() */

/*!**************************************************************
 * @fn fbe_ldo_test_negotiate_block_size_invalid_sg_case(void)
 ****************************************************************
 * @brief
 *  This function tests the case where the sg is not provided
 *  on the negotiate block size.
 *
 * @return fbe_u32_t - status of the test. 
 *
 * HISTORY:
 *  8/26/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_u32_t 
fbe_ldo_test_negotiate_block_size_invalid_sg_case(void)
{
    fbe_status_t status;
    fbe_u32_t qualifier;
    fbe_api_block_status_values_t block_status;

    /* Test with the 520 bytes per block drive. This is arbitrary, we would
     * expect the same error regardless of the block size of the drive.
     */
    fbe_object_id_t object_id = fbe_ldo_test_get_object_id_for_drive_block_size(520);

    /* Try a normal negotiate block size. 
     * This should fail since the sg is null. 
     */
    status = fbe_api_logical_drive_interface_send_block_payload( object_id,
                                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE,
                                             0, /* lba not used */
                                             0, /* blocks not used*/
                                             0, /* requested block size not used */
                                             0, /* requested block size not used */
                                             NULL, /* Sg element null to cause an error */
                                             NULL, /* edge descriptor not used */
                                             &qualifier,
                                             &block_status );
    /* We expect the overall packet status and payload status to be bad.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE)

    /* The payload status is bad, since we sent a null sg.
     */
    MUT_ASSERT_INT_EQUAL(block_status.status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);

    /* The qualifier is null sg as expected.
     */
    MUT_ASSERT_INT_EQUAL(block_status.qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NULL_SG);
    return status;
}
/******************************************
 * end fbe_ldo_test_negotiate_block_size_invalid_sg_case()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_negotiate_block_size_invalid_buffer_case(void)
 ****************************************************************
 * @brief
 *  Test negotiate block size where the input sg is valid, but
 *  the data it points to (the negotiate structure) is null.
 *
 * @return fbe_u32_t - status of the test. 
 *
 * HISTORY:
 *  8/26/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_u32_t 
fbe_ldo_test_negotiate_block_size_invalid_buffer_case(void)
{
    fbe_status_t status;
    fbe_payload_block_operation_status_t block_status;
	fbe_payload_block_operation_qualifier_t block_qualifier;
    
    /* Test with the 520 bytes per block drive.  This is arbitrary, we would
     * expect the same error regardless of the block size of the drive.
     */
    fbe_object_id_t object_id = fbe_ldo_test_get_object_id_for_drive_block_size(520);
    
    /* Try a negotiate block size.
     * This should fail since the output buffer is too small.
     */
#if 0
    status = fbe_api_send_negotiate_block_size(object_id,
                                                   NULL, /* set to null to cause an error */
                                                   &block_status,
                                                   &block_qualifier);
#endif
	status  = fbe_api_logical_drive_get_drive_block_size (object_id,
														  NULL,
														  &block_status,
														  &block_qualifier);
    /* But we expect the payload and packet status to be bad. 
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
    MUT_ASSERT_INT_EQUAL(block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    /* TO-DO: once negotiate passes in an sg we can uncomment the below.
     */
    //MUT_ASSERT_INT_EQUAL(block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_SG);
    return status;
}
/******************************************
 * end fbe_ldo_test_negotiate_block_size_invalid_buffer_case()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_negotiate_invalid_data_case(
 *                             fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  Test negotiate block size where the input data is not valid.
 * 
 * @param object_id - The object id to test.
 * 
 * @return fbe_status_t - FBE_STATUS_OK.
 * 
 * HISTORY:
 *  8/26/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_status_t 
fbe_ldo_test_negotiate_invalid_data_case(fbe_object_id_t object_id)
{
    fbe_status_t status;
    fbe_block_transport_negotiate_t negotiate;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;

    /* Fill with 0xFFs.  This is arbitrary, but this should cause an error on
     * the negotiate. 
     */
    memset(&negotiate, 0xFF, sizeof(fbe_block_transport_negotiate_t));
    
    status = fbe_api_logical_drive_get_drive_block_size(object_id, &negotiate, &block_status, &block_qualifier);

    /* Packet and Payload status are bad since the payload was not well formed. 
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
    MUT_ASSERT_INT_EQUAL(block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    MUT_ASSERT_INT_EQUAL(block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);

    /* We expect it to fail since the input data is not valid.
     */

    /* Now test for the case of zeros.  We use zeros since this is also a
     * possible failure cases where someone forgets to fill out the negotiate. 
     */
    memset(&negotiate, 0, sizeof(fbe_block_transport_negotiate_t));
    status = fbe_api_logical_drive_get_drive_block_size(object_id, &negotiate, &block_status, &block_qualifier);

    /* Packet and Payload status are bad since the payload was not well formed.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
    MUT_ASSERT_INT_EQUAL(block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    MUT_ASSERT_INT_EQUAL(block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);

    /* We expect it to fail since the input data is not valid.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_ldo_test_negotiate_invalid_data_case()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_negotiate_block_size_invalid_data_cases(void)
 ****************************************************************
 * @brief
 *  Test negotiate block size where the input data is not valid.
 * 
 *  We test all the possible block sizes we support.
 *
 * @return fbe_u32_t - status of the test. 
 *
 * HISTORY:
 *  8/26/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_u32_t fbe_ldo_test_negotiate_block_size_invalid_data_cases(void)
{
    fbe_status_t status;
    fbe_u32_t drive_index;

    /* For every block size we support, test the negotiate with invalid data.
     */
    for (drive_index = 0; drive_index < FBE_LDO_TEST_SUPPORTED_BLOCK_SIZE_MAX; drive_index++)
    {
        /* Get the block size for this supported drive.
         */
        fbe_block_size_t block_size = fbe_ldo_test_get_block_size_for_drive_index(drive_index);
        fbe_object_id_t object_id = fbe_ldo_test_get_object_id_for_drive_block_size(block_size);

        /* Issue the negotiate.
         */
        status = fbe_ldo_test_negotiate_invalid_data_case(object_id);

        /* We expect all these cases to succeed.
         */
        MUT_ASSERT_FBE_STATUS_OK(status);
    }
    return status;
}
/******************************************
 * end fbe_ldo_test_negotiate_block_size_invalid_data_cases()
 ******************************************/

/*!***************************************************************
 * @fn fbe_ldo_test_state_setup_negotiate(fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function sets up for testing the negotiate calculations in isolation.
 *  The main pieces that we leverage here are the packet and the logical drive
 *  being setup for testing.
 *  The packet and the logical drive are needed by the negotiate calcuations
 *  code.
 *
 * @param test_state_p - state ptr.
 *
 * @return fbe_status_t - FBE_STATUS_OK.
 *
 * HISTORY:
 *  09/09/08 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t 
fbe_ldo_test_state_setup_negotiate(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;
    fbe_payload_ex_t *payload_p;
    fbe_payload_block_operation_t *block_operation_p;
    fbe_status_t status;
    fbe_sg_element_t *sg_p = NULL;
    fbe_block_transport_negotiate_t *negotiate_p = NULL;
    /* Init the state object.
     */
    fbe_ldo_test_state_init(test_state_p);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);

    MUT_ASSERT_NOT_NULL_MSG( logical_drive_p, "Logical drive is NULL");
    MUT_ASSERT_NOT_NULL_MSG( io_packet_p, "io packet is NULL");
    
    /* Setup io packet for the negotiate.
     */
    payload_p = fbe_transport_get_payload_ex(io_packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

    negotiate_p = fbe_ldo_test_state_get_negotiate(test_state_p);

    /* Setup the sg using the internal packet sgs.
     * This creates an sg list using the negotiate information passed in. 
     */
    fbe_payload_ex_get_pre_sg_list(payload_p, &sg_p);
    fbe_payload_ex_set_sg_list(payload_p, sg_p, 0);
    fbe_sg_element_init(sg_p, sizeof(fbe_block_transport_negotiate_t), negotiate_p);
    fbe_sg_element_increment(&sg_p);
    fbe_sg_element_terminate(sg_p);

    status =  fbe_payload_block_build_negotiate_block_size(block_operation_p);
    fbe_payload_ex_set_sg_list(payload_p, sg_p, 0);
    MUT_ASSERT_FBE_STATUS_OK(status);

    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_state_setup_negotiate() */

/*!**************************************************************
 * @fn fbe_ldo_test_negotiate_case(
 *       fbe_ldo_test_negotiate_block_size_case_t * const case_p)
 ****************************************************************
 * @brief
 *  This function tests a single case of
 *  the fbe_ldo_calculate_negotiate_info() function.
 *
 * @param case_p - The test case to execute.               
 *
 * @return fbe_status_t - FBE_STATUS_OK.
 * 
 * HISTORY:
 *  9/9/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_status_t 
fbe_ldo_test_negotiate_case(fbe_ldo_test_negotiate_block_size_case_t * const case_p)
{
    fbe_status_t status;
    fbe_logical_drive_t *logical_drive_p;
    fbe_block_transport_negotiate_t *negotiate_p;
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    negotiate_p = fbe_ldo_test_state_get_negotiate(test_state_p);

    /* Get our state object setup.  This inits the logical drive and packet.
     */
    status = fbe_ldo_test_state_setup_negotiate(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Also init the logical drive's imported values.
     */
    fbe_ldo_set_imported_capacity(logical_drive_p, case_p->imported_capacity);
    fbe_ldo_setup_block_sizes(logical_drive_p, case_p->imported_block_size);

    /* Needed since capacity is only returned by the accessor if the path state is good.
     */
    logical_drive_p->block_edge.base_edge.path_state = FBE_PATH_STATE_ENABLED;

    /* Copy the requested block size and requested optimal block size from the 
     * expected data. 
     */
    negotiate_p->requested_block_size = 
        case_p->negotiate_data_expected.requested_block_size;
    negotiate_p->requested_optimum_block_size = 
        case_p->negotiate_data_expected.requested_optimum_block_size;

    /* Next, call the logical drive function to handle a negotiate.
     */
    status = fbe_ldo_calculate_negotiate_info(logical_drive_p, negotiate_p);

    if (status == FBE_STATUS_OK)
    {
        /* Validate the received values match the expected values.
         */
        status = fbe_ldo_test_validate_negotiate_block_size_data(negotiate_p,
                                                                 &case_p->negotiate_data_expected);
    }

    /* Make sure the result was as expected.
     * We either succeeded or a failure was expected here.
     */
    if (case_p->b_failure_expected)
    {
        MUT_ASSERT_FBE_STATUS_FAILED(status);
    }
    else
    {
        MUT_ASSERT_FBE_STATUS_OK(status);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_ldo_test_negotiate_case()
 ******************************************/

/*!***************************************************************
 * @fn fbe_ldo_test_calculate_negotiate_cases(void)
 ****************************************************************
 * @brief
 *  This function runs all of the test cases for testing the
 *  calculate negotiate block size functionality.
 *
 * @param  - None.
 *
 * @return:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  9/9/2008 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t fbe_ldo_test_calculate_negotiate_cases(void)
{
    fbe_u32_t test_index; 
    fbe_status_t status = FBE_STATUS_OK;
    
    /* Simply loop over the test cases.  If we hit a zero test case, or if
     * we hit an error, then exit.
     */
    for (test_index = 0; 
         fbe_ldo_test_calculate_negotiate_data[test_index].imported_capacity != 
          FBE_LDO_TEST_INVALID_FIELD;
         test_index++)
    {
        /* Test the "test_index" case.
         */
        status = fbe_ldo_test_negotiate_case( &fbe_ldo_test_calculate_negotiate_data[test_index]);
        MUT_ASSERT_FBE_STATUS_OK(status);

    } /* end for all test cases. */

    return status;
}
/* end fbe_ldo_test_calculate_negotiate_cases */

/*!**************************************************************
 * @fn fbe_ldo_test_add_negotiate_block_size_tests(
 *                               mut_testsuite_t * const suite_p)
 ****************************************************************
 * @brief
 *  Add the negotiate block size unit tests to the input suite.
 *
 * @param suite_p - suite to add tests to.               
 *
 * @return none.
 *
 * HISTORY:
 *  8/11/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_add_negotiate_block_size_tests(mut_testsuite_t * const suite_p)
{
    /* Just add the tests we want to run to the suite.
     */
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_send_negotiate_block_size_cases, 
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_negotiate_block_size_invalid_sg_case, 
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_negotiate_block_size_invalid_buffer_case, 
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_negotiate_block_size_invalid_data_cases, 
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);

    /* This test uses a different setup/teardown function since it 
     * is more of an unit test that tests in isolation, whereas the prior 
     * test functions are more of an integration test with the whole physical package.
     */
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_calculate_negotiate_cases, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);

    return;
}
/******************************************
 * end fbe_ldo_test_add_negotiate_block_size_tests() 
 ******************************************/

/*****************************************
 * end fbe_logical_drive_test_negotiate_block_size.c
 *****************************************/
