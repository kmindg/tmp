/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_library.c
 ***************************************************************************
 *
 * @brief
 *  This file contains tests for the functions of the logical drive library
 *  module.
 *
 * HISTORY
 *   6/17/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe_trace.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_test_private.h"
#include "fbe_logical_drive_test_prototypes.h"
#include "mut.h"


/******************************
 ** LOCAL DEFINITIONS
 ******************************/

/* This is the definition of the test case record 
 * for testing the pre-read function. 
 */
typedef struct fbe_ldo_pre_read_lba_case_s
{
    fbe_block_size_t exported_block_size;
    fbe_block_size_t exported_opt_block_size;
    fbe_block_size_t imported_block_size;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_lba_t expected_lba;
    fbe_block_count_t expected_blocks;
}
fbe_ldo_pre_read_lba_case_t;

/* These are the test cases for testing the calculate pre-read function.
 * 520 cases. 
 */
fbe_ldo_pre_read_lba_case_t fbe_ldo_pre_read_lba_520_cases[] = 
{
    /* 520 - exported
     * 520 - imported 
     * test cases.
     *  
     * exported exported imported   lba  blocks expected  expected
     * block    optimum  block                  lba       blocks
     * size     bl size  size     
     */
    {520,        1,       520,      0,   1,      0,         1},
    {520,        1,       520,      1,   1,      1,         1},
    {520,        1,       520,      2,   1,      2,         1},
    {520,        1,       520,      18,  1,      18,        1},
    {520,        1,       520,      61,  1,      61,        1},
    {520,        1,       520,      62,  1,      62,        1},
    {520,        1,       520,      63,  1,      63,        1},
    {520,        1,       520,      64,  1,      64,        1},
    {520,        1,       520,      65,  1,      65,        1},

    /* Insert new records here.
     */

    /* This is the terminator record.
     */
    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0}
    
};
/* end fbe_ldo_pre_read_lba_520_cases global definition */

/* These are the test cases for testing the calculate pre-read function.
 * 520/512 cases. 
 */
fbe_ldo_pre_read_lba_case_t fbe_ldo_pre_read_lba_512_cases[] = 
{
    /* 520/512 cases.
     * exported exported imported imported   lba  blocks expected  expected
     * block    optimum  block    opt block              lba       blocks
     * size     bl size  size     size
     */
    {520,       64,      512,     0,   1,      0,         2},
    {520,       64,      512,     1,   1,      0,         3},
    {520,       64,      512,     2,   1,      1,         3},
    {520,       64,      512,     3,   1,      2,         3},
    {520,       64,      512,     4,   1,      3,         3},
    {520,       64,      512,     5,   1,      4,         3},
    {520,       64,      512,     6,   1,      5,         3},
    {520,       64,      512,     7,   1,      6,         3},
    {520,       64,      512,     8,   1,      7,         3},
    {520,       64,      512,     9,   1,      8,         3},
    {520,       64,      512,     10,   1,      9,         3},
    {520,       64,      512,     11,   1,     10,         3},
    {520,       64,      512,     12,   1,     11,         3},
    {520,       64,      512,     13,   1,     12,         3},
    {520,       64,      512,     14,   1,     13,         3},
    {520,       64,      512,     15,   1,     14,         3},
    {520,       64,      512,     16,   1,     15,         3},
    {520,       64,      512,     17,   1,     16,         3},
    {520,       64,      512,     18,   1,     17,         3},
    {520,       64,      512,     19,   1,     18,         3},
    {520,       64,      512,     20,   1,     19,         3},
    {520,       64,      512,     21,   1,     20,         3},
    {520,       64,      512,     22,   1,     21,         3},
    {520,       64,      512,     23,   1,     22,         3},
    {520,       64,      512,     24,   1,     23,         3},
    {520,       64,      512,     25,   1,     24,         3},
    {520,       64,      512,     26,   1,     25,         3},
    {520,       64,      512,     27,   1,     26,         3},
    {520,       64,      512,     28,   1,     27,         3},
    {520,       64,      512,     29,   1,     28,         3},
    {520,       64,      512,     30,   1,     29,         3},
    {520,       64,      512,     31,   1,     30,         3},
    {520,       64,      512,     32,   1,     31,         3},
    {520,       64,      512,     33,   1,     32,         3},
    {520,       64,      512,     34,   1,     33,         3},
    {520,       64,      512,     35,   1,     34,         3},
    {520,       64,      512,     36,   1,     35,         3},
    {520,       64,      512,     37,   1,     36,         3},
    {520,       64,      512,     38,   1,     37,         3},
    {520,       64,      512,     39,   1,     38,         3},
    {520,       64,      512,     40,   1,     39,         3},
    {520,       64,      512,     41,   1,     40,         3},
    {520,       64,      512,     42,   1,     41,         3},
    {520,       64,      512,     43,   1,     42,         3},
    {520,       64,      512,     44,   1,     43,         3},
    {520,       64,      512,     45,   1,     44,         3},
    {520,       64,      512,     46,   1,     45,         3},
    {520,       64,      512,     47,   1,     46,         3},
    {520,       64,      512,     48,   1,     47,         3},
    {520,       64,      512,     49,   1,     48,         3},
    {520,       64,      512,     50,   1,     49,         3},
    {520,       64,      512,     51,   1,     50,         3},
    {520,       64,      512,     52,   1,     51,         3},
    {520,       64,      512,     53,   1,     52,         3},
    {520,       64,      512,     54,   1,     53,         3},
    {520,       64,      512,     55,   1,     54,         3},
    {520,       64,      512,     56,   1,     55,         3},
    {520,       64,      512,     57,   1,     56,         3},
    {520,       64,      512,     58,   1,     57,         3},
    {520,       64,      512,     59,   1,     58,         3},
    {520,       64,      512,     60,   1,     59,         3},
    {520,       64,      512,     61,  1,      60,         3},
    {520,       64,      512,     62,  1,      61,         3},
    {520,       64,      512,     63,  1,      62,         2},
    {520,       64,      512,     64,  1,      64,         2},
    {520,       64,      512,     65,  1,      64,         3},
    {520,       64,      512,     66,  1,      65,         3},
    {520,       64,      512,     67,  1,      66,         3},

    /* Insert new records here.
     */

    /* This is the terminator record.
     */
    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0}
    
};
/* end fbe_ldo_pre_read_lba_512_cases global definition */

/* These are the test cases for testing the calculate pre-read function.
 * 520/512 lossy cases. 
 */
fbe_ldo_pre_read_lba_case_t fbe_ldo_pre_read_lba_512_lossy_cases[] = 
{
    /* 2 x 520 / 3 x 512 cases.
     *  
     * exported exported imported lba  blocks expected  expected
     * block    optimum  block                  lba       blocks
     * size     bl size  size     
     */
    {520,        2,       512,      0,   1,      0,         2},
    {520,        2,       512,      1,   1,      0,         2},
    {520,        2,       512,      2,   1,      2,         2},
    {520,        2,       512,      18,  1,      18,        2},
    {520,        2,       512,      61,  1,      60,        2},
    {520,        2,       512,      62,  1,      62,        2},
    {520,        2,       512,      63,  1,      62,        2},
    {520,        2,       512,      64,  1,      64,        2},
    {520,        2,       512,      65,  1,      64,        2},

    /* Insert new records here.
     */

    /* This is the terminator record.
     */
    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0}
    
};
/* end fbe_ldo_pre_read_lba_512_lossy_cases global definition */

/* These are the test cases for testing the calculate pre-read function.
 * 520/4096 cases. 
 */
fbe_ldo_pre_read_lba_case_t fbe_ldo_pre_read_lba_4096_cases[] = 
{
    /* 520/4096 cases.
     * exported exported imported lba  blocks expected  expected
     * block    optimum  block                lba       blocks
     * size     bl size  size     
     */
    {520,       512,     4096,    0,   1,      0,         8},
    {520,       512,     4096,    1,   1,      0,         8},
    {520,       512,     4096,    2,   1,      0,         8},
    {520,       512,     4096,    3,   1,      0,         8},
    {520,       512,     4096,    4,   1,      0,         8},
    {520,       512,     4096,    5,   1,      0,         8},
    {520,       512,     4096,    6,   1,      0,         8},
    {520,       512,     4096,    7,   1,      0,         16},
    {520,       512,     4096,    8,   1,      7,         9},
    {520,       512,     4096,    9,   1,      7,         9},
    {520,       512,     4096,    10,  1,      7,         9},
    {520,       512,     4096,    11,  1,      7,         9},
    {520,       512,     4096,    12,  1,      7,         9},
    {520,       512,     4096,    13,  1,      7,         9},
    {520,       512,     4096,    14,  1,      7,         9},
    {520,       512,     4096,    15,  1,      7,         17},
    {520,       512,     4096,    16,  1,      15,        9},
    {520,       512,     4096,    17,  1,      15,        9},
    {520,       512,     4096,    18,  1,      15,        9},
    {520,       512,     4096,    19,  1,      15,        9},
    {520,       512,     4096,    20,  1,      15,        9},
    {520,       512,     4096,    21,  1,      15,        9},
    {520,       512,     4096,    22,  1,      15,        9},
    {520,       512,     4096,    23,  1,      15,        17},
    {520,       512,     4096,    24,  1,      23,        9},
    {520,       512,     4096,    25,  1,      23,        9},
    {520,       512,     4096,    26,  1,      23,        9},
    {520,       512,     4096,    27,  1,      23,        9},
    {520,       512,     4096,    28,  1,      23,        9},
    {520,       512,     4096,    29,  1,      23,        9},
    {520,       512,     4096,    30,  1,      23,        9},
    {520,       512,     4096,    31,  1,      23,       17},
    {520,       512,     4096,    32,  1,      31,        9},
    {520,       512,     4096,    33,  1,      31,        9},
    {520,       512,     4096,    34,  1,      31,        9},
    {520,       512,     4096,    35,  1,      31,        9},
    {520,       512,     4096,    36,  1,      31,        9},
    {520,       512,     4096,    37,  1,      31,        9},
    {520,       512,     4096,    38,  1,      31,        9},
    {520,       512,     4096,    39,  1,      31,       17},
    {520,       512,     4096,    40,  1,      39,        9},
    {520,       512,     4096,    41,  1,      39,        9},
    {520,       512,     4096,    42,  1,      39,        9},
    {520,       512,     4096,    43,  1,      39,        9},
    {520,       512,     4096,    44,  1,      39,        9},
    {520,       512,     4096,    45,  1,      39,        9},
    {520,       512,     4096,    46,  1,      39,        9},
    {520,       512,     4096,    47,  1,      39,       17},
    {520,       512,     4096,    48,  1,      47,        9},
    {520,       512,     4096,    49,  1,      47,        9},
    {520,       512,     4096,    50,  1,      47,        9},
    {520,       512,     4096,    51,  1,      47,        9},
    {520,       512,     4096,    52,  1,      47,        9},
    {520,       512,     4096,    53,  1,      47,        9},

    /* More 520/4096 cases.
     * exported exported imported lba  blocks expected  expected
     * block    optimum  block                lba       blocks
     * size     bl size  size     
     */
    {520,       512,     4096,    54,  1,      47,        9},
    {520,       512,     4096,    55,  1,      47,        17},
    {520,       512,     4096,    56,  1,      55,        9},
    {520,       512,     4096,    57,  1,      55,        9},
    {520,       512,     4096,    58,  1,      55,        9},
    {520,       512,     4096,    59,  1,      55,        9},
    {520,       512,     4096,    60,  1,      55,        9},
    {520,       512,     4096,    61,  1,      55,        9},
    {520,       512,     4096,    62,  1,      55,        9},
    {520,       512,     4096,    63,  1,      55,        16},
    {520,       512,     4096,    64,  1,      63,        8},
    {520,       512,     4096,    65,  1,      63,        8},
    {520,       512,     4096,    66,  1,      63,        8},
    {520,       512,     4096,    67,  1,      63,        8},
    {520,       512,     4096,    68,  1,      63,        8},
    {520,       512,     4096,    69,  1,      63,        8},
    {520,       512,     4096,    70,  1,      63,        16},
    {520,       512,     4096,    71,  1,      70,        9},
    {520,       512,     4096,    72,  1,      70,        9},
    {520,       512,     4096,    73,  1,      70,        9},
    {520,       512,     4096,    74,  1,      70,        9},
    {520,       512,     4096,    75,  1,      70,        9},
    {520,       512,     4096,    76,  1,      70,        9},
    {520,       512,     4096,    77,  1,      70,        9},
    {520,       512,     4096,    78,  1,      70,        17},
    {520,       512,     4096,    79,  1,      78,        9},
    {520,       512,     4096,    80,  1,      78,        9},
    {520,       512,     4096,    81,  1,      78,        9},
    {520,       512,     4096,    82,  1,      78,        9},
    {520,       512,     4096,    83,  1,      78,        9},
    {520,       512,     4096,    84,  1,      78,        9},
    {520,       512,     4096,    85,  1,      78,        9},
    {520,       512,     4096,    86,  1,      78,       17},
    {520,       512,     4096,    87,  1,      86,        9},
    {520,       512,     4096,    88,  1,      86,        9},
    {520,       512,     4096,    89,  1,      86,        9},
    {520,       512,     4096,    90,  1,      86,        9},
    {520,       512,     4096,    91,  1,      86,        9},
    {520,       512,     4096,    92,  1,      86,        9},
    {520,       512,     4096,    93,  1,      86,        9},
    {520,       512,     4096,    94,  1,      86,       17},
    {520,       512,     4096,    95,  1,      94,        9},
    {520,       512,     4096,    96,  1,      94,        9},
    {520,       512,     4096,    97,  1,      94,        9},
    {520,       512,     4096,    98,  1,      94,        9},
    {520,       512,     4096,    99,  1,      94,        9},
    {520,       512,     4096,   100,  1,      94,        9},
    {520,       512,     4096,   101,  1,      94,        9},
    {520,       512,     4096,   102,  1,      94,       17},
    {520,       512,     4096,   103,  1,     102,        9},
    {520,       512,     4096,   104,  1,     102,        9},
    {520,       512,     4096,   105,  1,     102,        9},
    {520,       512,     4096,   106,  1,     102,        9},
    {520,       512,     4096,   107,  1,     102,        9},
    {520,       512,     4096,   108,  1,     102,        9},
    {520,       512,     4096,   109,  1,     102,        9},
    {520,       512,     4096,   110,  1,     102,       17},
    {520,       512,     4096,   111,  1,     110,        9},
    {520,       512,     4096,   112,  1,     110,        9},
    {520,       512,     4096,   113,  1,     110,        9},
    {520,       512,     4096,   114,  1,     110,        9},
    {520,       512,     4096,   115,  1,     110,        9},
    {520,       512,     4096,   116,  1,     110,        9},
    {520,       512,     4096,   117,  1,     110,        9},
    {520,       512,     4096,   118,  1,     110,       17},
    {520,       512,     4096,   119,  1,     118,        9},
    {520,       512,     4096,   120,  1,     118,        9},
    {520,       512,     4096,   121,  1,     118,        9},
    {520,       512,     4096,   122,  1,     118,        9},
    {520,       512,     4096,   123,  1,     118,        9},
    {520,       512,     4096,   124,  1,     118,        9},
    {520,       512,     4096,   125,  1,     118,        9},
    {520,       512,     4096,   126,  1,     118,       16},
    {520,       512,     4096,   127,  1,     126,        8},
    {520,       512,     4096,   128,  1,     126,        8},
    {520,       512,     4096,   129,  1,     126,        8},
    {520,       512,     4096,   130,  1,     126,        8},
    {520,       512,     4096,   131,  1,     126,        8},
    {520,       512,     4096,   132,  1,     126,        8},
    {520,       512,     4096,   133,  1,     126,       16},
    {520,       512,     4096,   134,  1,     133,        9},
    {520,       512,     4096,   135,  1,     133,        9},
    {520,       512,     4096,   136,  1,     133,        9},
    {520,       512,     4096,   137,  1,     133,        9},
    {520,       512,     4096,   138,  1,     133,        9},
    {520,       512,     4096,   139,  1,     133,        9},
    {520,       512,     4096,   140,  1,     133,        9},
    {520,       512,     4096,   141,  1,     133,       17},
    {520,       512,     4096,   142,  1,     141,        9},
    {520,       512,     4096,   143,  1,     141,        9},

    /* More 520/4096 cases.
     * exported exported imported lba  blocks expected  expected
     * block    optimum  block                  lba       blocks
     * size     bl size  size     
     */

    {520,       512,     4096,    500,  1,     496,        9},
    {520,       512,     4096,    501,  1,     496,        9},
    {520,       512,     4096,    502,  1,     496,        9},
    {520,       512,     4096,    503,  1,     496,        9},
    {520,       512,     4096,    504,  1,     496,        16},
    {520,       512,     4096,    505,  1,     504,        8},
    {520,       512,     4096,    506,  1,     504,        8},
    {520,       512,     4096,    507,  1,     504,        8},
    {520,       512,     4096,    508,  1,     504,        8},
    {520,       512,     4096,    509,  1,     504,        8},
    {520,       512,     4096,    510,  1,     504,        8},
    {520,       512,     4096,    511,  1,     504,        8},
    {520,       512,     4096,    512,  1,     512,        8},
    {520,       512,     4096,    513,  1,     512,        8},
    {520,       512,     4096,    514,  1,     512,        8},
    {520,       512,     4096,    515,  1,     512,        8},
    {520,       512,     4096,    516,  1,     512,        8},
    {520,       512,     4096,    517,  1,     512,        8},
    {520,       512,     4096,    518,  1,     512,        8},
    {520,       512,     4096,    519,  1,     512,        16},
    {520,       512,     4096,    520,  1,     519,        9},
    {520,       512,     4096,    521,  1,     519,        9},
    {520,       512,     4096,    522,  1,     519,        9},
    {520,       512,     4096,    523,  1,     519,        9},
    {520,       512,     4096,    524,  1,     519,        9},
    {520,       512,     4096,    525,  1,     519,        9},
    {520,       512,     4096,    526,  1,     519,        9},
    {520,       512,     4096,    527,  1,     519,        17},
    {520,       512,     4096,    528,  1,     527,        9},
    {520,       512,     4096,    529,  1,     527,        9},
    {520,       512,     4096,    530,  1,     527,        9},
    {520,       512,     4096,    531,  1,     527,        9},
    {520,       512,     4096,    532,  1,     527,        9},

    /* Insert new records here.
     */

    /* This is the terminator record.
     */
    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0}
    
};
/* end fbe_ldo_pre_read_lba_4096_cases global definition */

/* These are the test cases for testing the calculate pre-read function.
 * 520/4096 lossy cases. 
 */
fbe_ldo_pre_read_lba_case_t fbe_ldo_pre_read_lba_4096_lossy_cases[] = 
{
    /* 7 x 520
     * 1 x 4096 cases.
     *  
     * exported exported imported lba  blocks expected  expected
     * block    optimum  block                  lba       blocks
     * size     bl size  size     
     */
    {520,         7,     4096,     0,  1,       0,        7},
    {520,         7,     4096,     0,  2,       0,        7},
    {520,         7,     4096,     0,  3,       0,        7},
    {520,         7,     4096,     0,  4,       0,        7},
    {520,         7,     4096,     0,  5,       0,        7},
    {520,         7,     4096,     0,  6,       0,        7},
    {520,         7,     4096,     0,  7,       0,        7},

    {520,         7,     4096,     0,  8,       7,        7},
    {520,         7,     4096,     0,  9,       7,        7},
    {520,         7,     4096,     0,  10,      7,        7},
    {520,         7,     4096,     0,  11,      7,        7},
    {520,         7,     4096,     0,  12,      7,        7},
    {520,         7,     4096,     0,  13,      7,        7},
    {520,         7,     4096,     0,  14,      0,       14},
    {520,         7,     4096,     1,  1,       0,        7},
    {520,         7,     4096,     1,  2,       0,        7},
    {520,         7,     4096,     1,  3,       0,        7},
    {520,         7,     4096,     1,  4,       0,        7},
    {520,         7,     4096,     1,  5,       0,        7},
    {520,         7,     4096,     1,  6,       0,        7},
    {520,         7,     4096,     1,  7,       0,       14},
    {520,         7,     4096,     1,  8,       0,       14},
    {520,         7,     4096,     1,  9,       0,       14},
    {520,         7,     4096,     1,  10,      0,       14},
    {520,         7,     4096,     0,  11,      7,       7},
    {520,         7,     4096,     1,  12,      0,       14},
    {520,         7,     4096,     1,  13,      0,       7},
    {520,         7,     4096,     1,  14,      0,       21},
    /* Insert new records here.
     */

    /* This is the terminator record.
     */
    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0}
    
};
/* end fbe_ldo_pre_read_lba_4096_lossy_cases global definition */

/* These are the test cases for testing the calculate pre-read function.
 * 520/4160 cases. 
 */
fbe_ldo_pre_read_lba_case_t fbe_ldo_pre_read_lba_4160_cases[] = 
{
    /* 520/4160 (520x8) cases.
     * exported exported imported lba  blocks expected  expected
     * block    optimum  block                  lba       blocks
     * size     bl size  size     
     */
    {520,       8,       4160,    0,   1,      0,         8},
    {520,       8,       4160,    1,   1,      0,         8},
    {520,       8,       4160,    2,   1,      0,         8},
    {520,       8,       4160,    3,   1,      0,         8},
    {520,       8,       4160,    4,   1,      0,         8},
    {520,       8,       4160,    5,   1,      0,         8},
    {520,       8,       4160,    6,   1,      0,         8},
    {520,       8,       4160,    7,   1,      0,         8},
    {520,       8,       4160,    8,   1,      8,         8},
    {520,       8,       4160,    9,   1,      8,         8},
    {520,       8,       4160,    11,  1,      8,         8},
    {520,       8,       4160,    12,  1,      8,         8},
    {520,       8,       4160,    13,  1,      8,         8},
    {520,       8,       4160,    14,  1,      8,         8},
    {520,       8,       4160,    15,  1,      8,         8},
    {520,       8,       4160,    16,  1,      16,        8},
    {520,       8,       4160,    17,  1,      16,        8},
    {520,       8,       4160,    18,  1,      16,        8},
    {520,       8,       4160,    19,  1,      16,        8},

    /* Insert new records here.
     */

    /* This is the terminator record.
     */
    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0}
    
};
/* end fbe_ldo_pre_read_lba_4160_cases global definition */

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/****************************************************************
 * @fn fbe_ldo_test_get_pr_lba_count_case(
 *             fbe_lba_t exported_block_size,
 *             fbe_lba_t exported_opt_block_size,
 *             fbe_lba_t imported_block_size,
 *             fbe_lba_t lba,
 *             fbe_block_count_t blocks,
 *             fbe_lba_t expected_lba,
 *             fbe_block_count_t expected_blocks)
 ****************************************************************
 * @brief
 *  This function validates a single test case for
 *  the get pre read lba count function.
 *
 * @param exported_block_size - Exported block size to use in write.
 *  exported_opt_block_size - Exported optimal block size 
 *                                 to use in write.
 *  imported_block_size - Imported block size to use in write.
 *  imported_opt_block_size - Imported optimal block size 
 *                                 to use in write.
 *  lba - Start logical block address of write.
 *  blocks - Total blocks to write.
 *  expected_lba - The lba we expect to receive back
 *                      on our call to get pre-read lba.
 *  expected_blocks - The blocks we expect to receive back
 *                         on our call to get pre-read blocks.
 *
 * RETURNS:
 *  The status of the test case.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * HISTORY:
 *  11/27/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_get_pr_lba_count_case( fbe_lba_t exported_block_size,
                                                 fbe_lba_t exported_opt_block_size,
                                                 fbe_lba_t imported_block_size,
                                                 fbe_lba_t lba,
                                                 fbe_block_count_t blocks,
                                                 fbe_lba_t expected_lba,
                                                 fbe_block_count_t expected_blocks )
{
    fbe_status_t status;
    fbe_lba_t output_lba = lba;
    fbe_block_count_t output_blocks = blocks;

    /* Call the function under test. 
     * The output_lba and output_blocks are returned.
     */
    fbe_logical_drive_get_pre_read_lba_blocks_for_write(exported_block_size,
                                              exported_opt_block_size,
                                              imported_block_size,
                                              &output_lba,
                                              &output_blocks);

    /* Validate the results are expected.
     */
    if (output_lba == expected_lba && output_blocks == expected_blocks)
    {
        status = FBE_STATUS_OK;
    }
    else
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/* end fbe_ldo_test_get_pr_lba_count_case */

/*!***************************************************************
 * @fn fbe_ldo_test_get_pr_lba_count(fbe_ldo_pre_read_lba_case_t *const case_p)
 ****************************************************************
 * @brief
 *  This function validates all the test cases for
 *  the get pre read lba count function.
 *
 * @param case_p - Ptr to the table of test cases.
 *
 * @return
 *  The status of the Operation.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * HISTORY:
 *  11/27/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_get_pr_lba_count( fbe_ldo_pre_read_lba_case_t *const case_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index = 0;
    
    /* Loop over all of our test cases.
     */
    while (case_p[index].exported_block_size != FBE_LDO_TEST_INVALID_FIELD &&
           status == FBE_STATUS_OK)
    {
        /* For each test case, feed the validate case function
         * with the appropriate values from our
         * array of test cases.
         */
        status = fbe_ldo_test_get_pr_lba_count_case(case_p[index].exported_block_size,
                                                    case_p[index].exported_opt_block_size,
                                                    case_p[index].imported_block_size,
                                                    case_p[index].lba,
                                                    case_p[index].blocks,
                                                    case_p[index].expected_lba,
                                                    case_p[index].expected_blocks);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        index++;
    } /* End while fbe_ldo_pre_read_lba_cases[index].exported_block_size != FBE_LDO_TEST_INVALID_FIELD */
    return status;
}
/* end fbe_ldo_test_get_pr_lba_count */

/*!**************************************************************
 * @fn fbe_ldo_test_get_pr_lba_count_520_cases(void)
 ****************************************************************
 * @brief
 *  This function tests the fbe_ldo_map_lba_blocks function with
 *  a set of test cases defined in fbe_ldo_test_lba_map_tasks[].
 *
 * @param  - none.
 * 
 * @return none.   
 *
 * HISTORY:
 *  8/18/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_get_pr_lba_count_520_cases(void)
{
    fbe_ldo_test_get_pr_lba_count(fbe_ldo_pre_read_lba_520_cases);
    return;
}
/******************************************
 * end fbe_ldo_test_get_pr_lba_count_520_cases() 
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_get_pr_lba_count_512_cases(void)
 ****************************************************************
 * @brief
 *  This function tests the fbe_ldo_map_lba_blocks function with
 *  a set of test cases defined in fbe_ldo_test_lba_map_tasks[].
 *
 * @param  - none.
 * 
 * @return none.   
 *
 * HISTORY:
 *  8/18/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_get_pr_lba_count_512_cases(void)
{
    fbe_ldo_test_get_pr_lba_count(fbe_ldo_pre_read_lba_512_cases);
    return;
}
/******************************************
 * end fbe_ldo_test_get_pr_lba_count_512_cases() 
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_get_pr_lba_count_512_lossy_cases(void)
 ****************************************************************
 * @brief
 *  This function tests the fbe_ldo_map_lba_blocks function with
 *  a set of test cases defined in fbe_ldo_test_lba_map_tasks[].
 *
 * @param - none.
 * 
 * @return none.   
 *
 * HISTORY:
 *  8/18/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_get_pr_lba_count_512_lossy_cases(void)
{
    fbe_ldo_test_get_pr_lba_count(fbe_ldo_pre_read_lba_512_lossy_cases);
    return;
}
/******************************************
 * end fbe_ldo_test_get_pr_lba_count_512_lossy_cases() 
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_get_pr_lba_count_4096_cases(void)
 ****************************************************************
 * @brief
 *  This function tests the fbe_ldo_map_lba_blocks function with
 *  a set of test cases defined in fbe_ldo_test_lba_map_tasks[].
 *
 * @param - none.
 * 
 * @return none.   
 *
 * HISTORY:
 *  8/18/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_get_pr_lba_count_4096_cases(void)
{
    fbe_ldo_test_get_pr_lba_count(fbe_ldo_pre_read_lba_4096_cases);
    return;
}
/******************************************
 * end fbe_ldo_test_get_pr_lba_count_4096_cases() 
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_get_pr_lba_count_4096_lossy_cases(void)
 ****************************************************************
 * @brief
 *  This function tests the fbe_ldo_map_lba_blocks function with
 *  a set of test cases defined in fbe_ldo_test_lba_map_tasks[].
 *
 * @param - none.
 * 
 * @return none.   
 *
 * HISTORY:
 *  8/18/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_get_pr_lba_count_4096_lossy_cases(void)
{
    fbe_ldo_test_get_pr_lba_count(fbe_ldo_pre_read_lba_4096_lossy_cases);
    return;
}
/******************************************
 * end fbe_ldo_test_get_pr_lba_count_4096_lossy_cases() 
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_get_pr_lba_count_4160_cases(void)
 ****************************************************************
 * @brief
 *  This function tests the fbe_ldo_map_lba_blocks function with
 *  a set of test cases defined in fbe_ldo_test_lba_map_tasks[].
 *
 * @param - none.
 * 
 * @return none.   
 *
 * HISTORY:
 *  8/18/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_get_pr_lba_count_4160_cases(void)
{
    fbe_ldo_test_get_pr_lba_count(fbe_ldo_pre_read_lba_4160_cases);
    return;
}
/******************************************
 * end fbe_ldo_test_get_pr_lba_count_4160_cases() 
 ******************************************/


/* Test cases for testing the 512/520 mapping functions.
 */
static fbe_ldo_test_lba_map_task_t fbe_ldo_test_lba_map_512_tasks[] = 
{
    /* 520 exported 
     * 512 imported tests. 
     */
    {0, 1, 520, 64, 512, 65, 0, 2},
    {0, 2, 520, 64, 512, 65, 0, 3},
    {0, 3, 520, 64, 512, 65, 0, 4},
    {0, 4, 520, 64, 512, 65, 0, 5},
    {0, 5, 520, 64, 512, 65, 0, 6},
    {0, 64, 520, 64, 512, 65, 0, 65},
    {0, 65, 520, 64, 512, 65, 0, 67},

    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0, 0},
};
/* end fbe_ldo_test_lba_map_512_tasks */

/* Test cases for testing the 4096/520 mapping functions.
 */
static fbe_ldo_test_lba_map_task_t fbe_ldo_test_lba_map_4096_tasks[] = 
{
    /* 520 exported 4096 imported tests.
     */
    {0, 1, 520, 512, 4096, 65, 0, 1},
    {0, 2, 520, 512, 4096, 65, 0, 1},
    {0, 3, 520, 512, 4096, 65, 0, 1},
    {0, 4, 520, 512, 4096, 65, 0, 1},
    {0, 5, 520, 512, 4096, 65, 0, 1},
    {0, 6, 520, 512, 4096, 65, 0, 1},
    {0, 7, 520, 512, 4096, 65, 0, 1},
    {0, 8, 520, 512, 4096, 65, 0, 2},
    {0, 9, 520, 512, 4096, 65, 0, 2},
    {0, 15, 520, 512, 4096, 65, 0, 2},
    {0, 16, 520, 512, 4096, 65, 0, 3},
    {0, 23, 520, 512, 4096, 65, 0, 3},
    {0, 24, 520, 512, 4096, 65, 0, 4},
    {0, 25, 520, 512, 4096, 65, 0, 4},
    {0, 31, 520, 512, 4096, 65, 0, 4},
    {0, 32, 520, 512, 4096, 65, 0, 5},
    {0, 39, 520, 512, 4096, 65, 0, 5},
    {0, 40, 520, 512, 4096, 65, 0, 6},
    {0, 41, 520, 512, 4096, 65, 0, 6},
    {0, 500, 520, 512, 4096, 65, 0, 64},
    {0, 501, 520, 512, 4096, 65, 0, 64},
    {0, 502, 520, 512, 4096, 65, 0, 64},
    {0, 503, 520, 512, 4096, 65, 0, 64},
    {0, 504, 520, 512, 4096, 65, 0, 64},
    {0, 505, 520, 512, 4096, 65, 0, 65},
    {0, 506, 520, 512, 4096, 65, 0, 65},
    {0, 507, 520, 512, 4096, 65, 0, 65},
    {0, 508, 520, 512, 4096, 65, 0, 65},
    {0, 509, 520, 512, 4096, 65, 0, 65},
    {0, 510, 520, 512, 4096, 65, 0, 65},
    {0, 511, 520, 512, 4096, 65, 0, 65},
    {0, 512, 520, 512, 4096, 65, 0, 65},
    {0, 513, 520, 512, 4096, 65, 0, 66},

    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0, 0},
};
/* end fbe_ldo_test_lba_map_4096_tasks */

/* Test cases for testing the 4160/520 mapping functions.
 */
static fbe_ldo_test_lba_map_task_t fbe_ldo_test_lba_map_4160_tasks[] = 
{
    /* 520 exported 4160 imported tests.
     */
    {0, 1, 520, 8, 4160, 1, 0, 1},
    {0, 2, 520, 8, 4160, 1, 0, 1},
    {0, 3, 520, 8, 4160, 1, 0, 1},
    {0, 4, 520, 8, 4160, 1, 0, 1},
    {0, 5, 520, 8, 4160, 1, 0, 1},
    {0, 6, 520, 8, 4160, 1, 0, 1},
    {0, 7, 520, 8, 4160, 1, 0, 1},
    {0, 8, 520, 8, 4160, 1, 0, 1},
    {0, 9, 520, 8, 4160, 1, 0, 2},
    {0, 10, 520, 8, 4160, 1, 0, 2},
    {0, 11, 520, 8, 4160, 1, 0, 2},
    {0, 12, 520, 8, 4160, 1, 0, 2},
    {0, 13, 520, 8, 4160, 1, 0, 2},
    {0, 14, 520, 8, 4160, 1, 0, 2},
    {0, 15, 520, 8, 4160, 1, 0, 2},
    {0, 16, 520, 8, 4160, 1, 0, 2},
    {0, 17, 520, 8, 4160, 1, 0, 3},
    {0, 18, 520, 8, 4160, 1, 0, 3},
    {0, 23, 520, 8, 4160, 1, 0, 3},
    {0, 24, 520, 8, 4160, 1, 0, 3},
    {0, 25, 520, 8, 4160, 1, 0, 4},
    {0, 26, 520, 8, 4160, 1, 0, 4},
    {0, 31, 520, 8, 4160, 1, 0, 4},
    {0, 32, 520, 8, 4160, 1, 0, 4},
    {0, 33, 520, 8, 4160, 1, 0, 5},
    {0, 34, 520, 8, 4160, 1, 0, 5},
    {0, 39, 520, 8, 4160, 1, 0, 5},
    {0, 40, 520, 8, 4160, 1, 0, 5},
    {0, 41, 520, 8, 4160, 1, 0, 6},
    {0, 42, 520, 8, 4160, 1, 0, 6},
    {0, 43, 520, 8, 4160, 1, 0, 6},
    {0, 44, 520, 8, 4160, 1, 0, 6},

    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0, 0},
};
/* end fbe_ldo_test_lba_map_4160_tasks */

/* Test cases for testing the 512/520 lossy mapping functions.
 */
static fbe_ldo_test_lba_map_task_t fbe_ldo_test_lba_map_512_lossy_tasks[] = 
{
    /* 520 exported 
     * 512 imported 
     * 2 x 520 exported optimal block size 
     * 3 x 512 imported optimal block size. 
     */
    {0, 1, 520, 2, 512, 3, 0, 2},
    {0, 2, 520, 2, 512, 3, 0, 3},
    {0, 3, 520, 2, 512, 3, 0, 5},
    {0, 4, 520, 2, 512, 3, 0, 6},
    {0, 5, 520, 2, 512, 3, 0, 8},
    {1, 1, 520, 2, 512, 3, 1, 2},
    {1, 2, 520, 2, 512, 3, 1, 4},
    {1, 3, 520, 2, 512, 3, 1, 5},
    {1, 4, 520, 2, 512, 3, 1, 7},

    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0, 0},
};
/* end fbe_ldo_test_lba_map_512_lossy_tasks */

/*!***************************************************************
 * @fn fbe_ldo_test_map_lba_blocks_case(fbe_ldo_test_lba_map_task_t * task_p)
 ****************************************************************
 * @brief
 *  Test the function fbe_ldo_map_lba_blocks() with
 *  the input task array.
 *
 * @param task_p - The task array to test with.               
 *
 * @return - None.
 *
 * HISTORY:
 *  5/27/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_map_lba_blocks_case(fbe_ldo_test_lba_map_task_t * task_p)
{
    fbe_lba_t lba;
    fbe_block_count_t blocks;

    /* Loop over all the test cases, feeding each 
     * test case to the function under test. 
     */
    while (task_p->exported_lba != FBE_LDO_TEST_INVALID_FIELD)
    {
        fbe_ldo_map_lba_blocks(task_p->exported_lba,
                               task_p->exported_blocks,
                               task_p->exported_block_size,
                               task_p->exported_opt_block_size,
                               task_p->imported_block_size,
                               task_p->imported_opt_block_size,
                               &lba,
                               &blocks);
        /* Make sure the result is what we expect.
         */
        MUT_ASSERT_TRUE(lba == task_p->imported_lba);
        MUT_ASSERT_INT_EQUAL((fbe_u32_t)blocks, (fbe_u32_t)(task_p->imported_blocks));

        /* Increment to the next task.
         */
        task_p++;
    } /* end while not at end of tasks */
    return;
}
/******************************************
 * end fbe_ldo_test_map_lba_blocks_case()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_map_lba_blocks_512_cases()
 ****************************************************************
 * @brief
 *  This function tests the fbe_ldo_map_lba_blocks function with
 *  a set of test cases defined in fbe_ldo_test_lba_map_tasks[].
 *
 * @param - none.
 * 
 * @return none.   
 *
 * HISTORY:
 *  8/18/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_map_lba_blocks_512_cases(void)
{
    fbe_ldo_test_map_lba_blocks_case(fbe_ldo_test_lba_map_512_tasks);
    return;
}
/******************************************
 * end fbe_ldo_test_map_lba_blocks_512_cases()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_map_lba_blocks_4096_cases()
 ****************************************************************
 * @brief
 *  This function tests the fbe_ldo_map_lba_blocks function with
 *  a set of test cases defined in fbe_ldo_test_lba_map_tasks[].
 *
 * @param - none.
 * 
 * @return none.   
 *
 * HISTORY:
 *  8/18/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_map_lba_blocks_4096_cases(void)
{
    fbe_ldo_test_map_lba_blocks_case(fbe_ldo_test_lba_map_4096_tasks);
    return;
}
/******************************************
 * end fbe_ldo_test_map_lba_blocks_4096_cases()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_map_lba_blocks_4160_cases()
 ****************************************************************
 * @brief
 *  This function tests the fbe_ldo_map_lba_blocks function with
 *  a set of test cases defined in fbe_ldo_test_lba_map_tasks[].
 *
 * @param - none.
 * 
 * @return none.   
 *
 * HISTORY:
 *  8/18/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_map_lba_blocks_4160_cases(void)
{
    fbe_ldo_test_map_lba_blocks_case(fbe_ldo_test_lba_map_4160_tasks);
    return;
}
/******************************************
 * end fbe_ldo_test_map_lba_blocks_4160_cases()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_map_lba_blocks_512_lossy_cases()
 ****************************************************************
 * @brief
 *  This function tests the fbe_ldo_map_lba_blocks function with
 *  a set of test cases defined in fbe_ldo_test_lba_map_tasks[].
 *
 * @param - none.
 * 
 * @return none.   
 *
 * HISTORY:
 *  8/18/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_map_lba_blocks_512_lossy_cases(void)
{
    fbe_ldo_test_map_lba_blocks_case(fbe_ldo_test_lba_map_512_lossy_tasks);
    return;
}
/******************************************
 * end fbe_ldo_test_map_lba_blocks_512_lossy_cases()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_add_library_tests(mut_testsuite_t * const suite_p)
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

void fbe_ldo_test_add_library_tests(mut_testsuite_t * const suite_p)
{
    /* Tests for testing the map lba blocks function.
     */
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_map_lba_blocks_512_cases, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_map_lba_blocks_4096_cases, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_map_lba_blocks_4160_cases, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_map_lba_blocks_512_lossy_cases, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);

    /* Tests for testing get pre-read lba count.
     */
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_get_pr_lba_count_520_cases,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_get_pr_lba_count_512_cases,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_get_pr_lba_count_512_lossy_cases,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_get_pr_lba_count_4096_cases,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_get_pr_lba_count_4096_lossy_cases,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_get_pr_lba_count_4160_cases,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    return;
}
/******************************************
 * end fbe_ldo_test_add_library_tests()
 ******************************************/
/*************************
 * end file fbe_logical_drive_test_library.c
 *************************/
