 /***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_thread_io.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive object's
 *  io threads related functionality.
 *
 * @version
 *  06/10/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"

#include "fbe_trace.h"

#include "fbe/fbe_logical_drive.h"
#include "fbe_test_io_api.h"
#include "mut.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_test_private.h"

/************************************************** 
 * LOCAL STRUCTURE DEFINITIONS
 **************************************************/

/*!***************************************************************
 * @fn fbe_ldo_test_thread_ios(void)
 ****************************************************************
 * @brief
 *  This function performs a multi-thread io test.
 *
 * @param - None
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * HISTORY:
 *  02/12/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_thread_ios(void)
{
    fbe_u32_t threads;
    fbe_test_io_thread_task_t task[4];

    /* Test for each thread count over a range. 
     */
    for (threads = 1; threads < 4; threads++)
    {
        /* First run a read only test over a 520 byte per block drive.
         */
        fbe_test_io_thread_task_init(&task[threads - 1],
                                     FBE_IO_TEST_TASK_TYPE_READ_ONLY,    /* test type */
                                     520,   /* exported block size*/ 
                                     64,    /* exported optimum  block size*/
                                     512,   /* imported block size*/ 
                                     65,    /* imported optimum block size*/
                                     32,    /* Packets per thread*/
                                     1024,  /* max blocks*/ 
                                     10     /* seconds to test for. */ );

        fbe_test_io_threads(&task[threads - 1],
                            fbe_ldo_test_logical_drive_object_id_array[0],
                            threads);


        /* Then run a read only test over a 512 byte per block drive.
         */
        fbe_test_io_thread_task_init(&task[threads - 1],
                                     FBE_IO_TEST_TASK_TYPE_READ_ONLY,    /* test type */
                                     520,    /* exported block size*/ 
                                     64,    /* exported optimum  block size*/
                                     512,    /* imported block size*/ 
                                     65,    /* imported optimum block size*/
                                     32,    /* Packets per thread*/
                                     1024,  /* max blocks*/ 
                                     10     /* seconds to test for. */ );

        fbe_test_io_threads(&task[threads - 1],
                            fbe_ldo_test_logical_drive_object_id_array[1],
                            threads);
    }
    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_thread_ios() */

/*****************************************
 * end fbe_logical_drive_test_io.c
 *****************************************/
