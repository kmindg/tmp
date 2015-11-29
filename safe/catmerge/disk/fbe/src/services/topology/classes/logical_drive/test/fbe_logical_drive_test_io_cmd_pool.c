 /***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_io_cmd_pool.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive object's 
 *  io cmd pool functionality.
 *
 * @version
 *  09/15/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"

#include "fbe_trace.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_test_private.h"
#include "fbe_logical_drive_test_prototypes.h"
#include "mut.h"
#include "fbe/fbe_time.h"
#include "fbe_test_progress_object.h"

/************************************************** 
 * LOCAL STRUCTURE DEFINITIONS
 **************************************************/

/*! @def FBE_LDO_TEST_IO_CMD_POOL_MAX_THREAD 
 *  
 *  @brief Max number of threads to test when testing the io cmd pools.
 */
#define FBE_LDO_TEST_IO_CMD_POOL_MAX_THREAD 5

/*! @def FBE_LDO_TEST_IO_CMD_POOL_MAX_SECONDS 
 *  @brief Max number of seconds to run the io_cmd pool test.
 */
#define FBE_LDO_TEST_IO_CMD_POOL_MAX_SECONDS 15

/*! @def FBE_LDO_TEST_IO_CMD_POOL_DISPLAY_SECONDS 
 *  
 *  @brief Display progress of the io cmd pool test after this many seconds. 
 */
#define FBE_LDO_TEST_IO_CMD_POOL_DISPLAY_SECONDS 3

/*!*******************************************************************
 * @def FBE_LDO_THREAD_WAIT_MSEC
 *********************************************************************
 * @brief Timeout to wait for threads.
 *
 *********************************************************************/
 #define FBE_LDO_THREAD_WAIT_MSEC 60000
/*! @struct fbe_ldo_test_io_cmd_pool_thread_db_t 
 *  
 *  @brief This is a per thread structure for testing io command pools.
 */
typedef struct fbe_ldo_test_io_cmd_pool_thread_db_s 
{
    /*! How many sgs this thread will allocate.
     */
    fbe_u32_t num_sgs_to_allocate;

    /*! max number of seconds of alloc/free we will run.
     */
    fbe_u32_t max_seconds;

    /*! This is the thread number for display purposes.
     */
    fbe_u32_t thread_number;

    /*! Thread instance.
     */
    fbe_thread_t thread;

    /*! Synchronization for thread termination.
     */
    fbe_semaphore_t thread_semaphore;
}
fbe_ldo_test_io_cmd_pool_thread_db_t;

/*! @struct fbe_ldo_test_io_cmd_pool_threads_db_t 
 *  
 *  @brief This structure defines an overall database for keeping track of sg
 *         pool tests. 
 */
typedef struct fbe_ldo_test_io_cmd_pool_threads_db_s 
{
    /*! Total number of threads in use.
     */
    fbe_u32_t threads_running;

    /*! Semaphore for coordinating start of threads. 
     */
    fbe_semaphore_t semaphore;

    /*! Array of thread database structures we allocate.
     */
    fbe_ldo_test_io_cmd_pool_thread_db_t *thread_db_array;
}
fbe_ldo_test_io_cmd_pool_threads_db_t;

/*! @var fbe_ldo_test_io_cmd_pool_threads_db 
 *  @brief This is the global instance of the database structure used
 *         for keeping track of io cmd pool tests.
 */
static fbe_ldo_test_io_cmd_pool_threads_db_t fbe_ldo_test_io_cmd_pool_threads_db;

/************************************************** 
 * LOCAL FUNCTION DEFINITIONS
 **************************************************/

/*!**************************************************************
 * fbe_ldo_test_io_cmd_pool_threads_db_init()
 ****************************************************************
 * @brief
 *  Initialize the threads db structure and all the embedded 
 *  structures including the db for each thread.
 *
 * @param threads_db_p - The threads db structure to init.
 * @param total_threads - Number of threads to use.
 * @param num_sgs_to_allocate - Number of sgs first thread should 
 *                              allocate.
 * @param num_seconds - Number of iterations first thread should 
 *                         execute.
 *
 * @return None.
 *
 * HISTORY:
 *  9/15/2008 - Created. RPF
 *
 ****************************************************************/

static void 
fbe_ldo_test_io_cmd_pool_threads_db_init(fbe_ldo_test_io_cmd_pool_threads_db_t *const threads_db_p,
                                         fbe_u32_t total_threads,
                                         fbe_u32_t num_sgs_to_allocate,
                                         fbe_u32_t num_seconds) 
{
    fbe_u32_t thread_index;
    fbe_ldo_test_io_cmd_pool_threads_db.threads_running = total_threads;

    fbe_semaphore_init(&threads_db_p->semaphore, 0, total_threads);

    /* Allocate memory for our threads.
     */
    threads_db_p->thread_db_array = 
        (fbe_ldo_test_io_cmd_pool_thread_db_t*) malloc( total_threads * sizeof(fbe_ldo_test_io_cmd_pool_thread_db_t) );

    MUT_ASSERT_TRUE(threads_db_p->thread_db_array != NULL);

    /* The first thread always tries to get num_sgs_to_allocate.  The remainder 
     * of the threads only try to get 1 sg in order to avoid deadlock. 
     */
    threads_db_p->thread_db_array[0].thread_number = 0;
    threads_db_p->thread_db_array[0].max_seconds = num_seconds;
    threads_db_p->thread_db_array[0].num_sgs_to_allocate = num_sgs_to_allocate;

    /* Initialize the remainder of the threads.
     */
    for (thread_index = 1; thread_index < total_threads; thread_index++)
    {
        threads_db_p->thread_db_array[thread_index].thread_number = thread_index;
        threads_db_p->thread_db_array[thread_index].max_seconds = num_seconds;
        /* The remainder of threads only get one sg in order to interfere with
         * the first thread. 
         */
        threads_db_p->thread_db_array[thread_index].num_sgs_to_allocate = 1;

        fbe_semaphore_init(&threads_db_p->thread_db_array[thread_index].thread_semaphore, 0, 1);
    }
    return;
}
/******************************************
 * end fbe_ldo_test_io_cmd_pool_threads_db_init()
 ******************************************/

/*!**************************************************************
 * fbe_ldo_test_io_cmd_pool_threads_db_destroy()
 ****************************************************************
 * @brief
 *  Destroy the input threads db structure.
 *
 * @param threads_db_p - the structure to free memory for.     
 *
 * @return None.
 * 
 * HISTORY:
 *  9/15/2008 - Created. RPF
 *
 ****************************************************************/

static void 
fbe_ldo_test_io_cmd_pool_threads_db_destroy(fbe_ldo_test_io_cmd_pool_threads_db_t *const threads_db_p) 
{
    fbe_u32_t total_threads;
    total_threads = threads_db_p->threads_running;

    fbe_semaphore_destroy(&threads_db_p->semaphore);

    /* Free the memory we allocated for all the threads.
     */
    free(threads_db_p->thread_db_array); 
    threads_db_p->thread_db_array = NULL;
    return;
}
/******************************************
 * end fbe_ldo_test_io_cmd_pool_threads_db_destroy()
 ******************************************/

/*!***************************************************************
 * fbe_ldo_test_io_cmd_pool_allocate_completion()
 ****************************************************************
 * @brief
 *  This is the completion function for when an sg
 *  gets allocated.
 *
 * @param memory_request_p - Memory request being called back for.
 * @param context - The context structure, currently the
 *                 io command object.
 *
 * @return
 *  NONE
 *
 * HISTORY:
 *  09/15/08 - Created. RPF
 *
 ****************************************************************/
void
fbe_ldo_test_io_cmd_pool_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                         fbe_memory_completion_context_t context)
{
    /* Simply release the sempahore the caller is waiting on.
     */
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return;
}
/* end fbe_ldo_test_io_cmd_pool_allocate_completion() */

/*!**************************************************************
 * fbe_ldo_io_cmd_pool_execute_test()
 ****************************************************************
 * @brief
 *  Execute the test for a given thread database.
 *  This will allocate and then free io command pool structures for the
 *  given iterations.
 *
 * @param thread_db_p - Thread database to use. This has our total
 *                      number of sgs to allocate and our total
 *                      number of iterations.
 *
 * @return None.   
 *
 * HISTORY:
 *  9/15/2008 - Created. RPF
 *
 ****************************************************************/

static void 
fbe_ldo_io_cmd_pool_execute_test(fbe_ldo_test_io_cmd_pool_thread_db_t *thread_db_p)
{
    fbe_u32_t iterations = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_test_progress_object_t progress_object;

    /* Init our object for tracking progress.
     */
    fbe_test_progress_object_init(&progress_object, FBE_LDO_TEST_IO_CMD_POOL_DISPLAY_SECONDS);

    /* Loop until either we have reached max_seconds. 
     */
    while (fbe_test_progress_object_get_seconds_elapsed(&progress_object) < 
           thread_db_p->max_seconds)
    {
        fbe_u32_t sgs_to_allocate;
        fbe_logical_drive_io_cmd_t *io_cmd_p;
        fbe_queue_head_t allocated_sgs_head;
        fbe_packet_t packet;

        fbe_queue_init(&allocated_sgs_head);

        /* Display progress every N seconds.
         */
        if (fbe_test_progress_object_is_time_to_display(&progress_object))
        {
            mut_printf(MUT_LOG_HIGH, "%s thread %d iteration %d", 
                       __FUNCTION__, thread_db_p->thread_number, iterations);
        }
        /* First allocate the sgs, then free them.
         */
        for (sgs_to_allocate = 0; sgs_to_allocate < thread_db_p->num_sgs_to_allocate; sgs_to_allocate++)
        {
            fbe_semaphore_t sem;
            fbe_semaphore_init(&sem, 0, 1);

            /* Now allocate an sg.
             */
            status = fbe_ldo_allocate_io_cmd(&packet,
                                             fbe_ldo_test_io_cmd_pool_allocate_completion,
                                             &sem /* completion context */);
            MUT_ASSERT_FBE_STATUS_OK(status);

            /* Wait for our completion function to be called.
             */
            fbe_semaphore_wait(&sem, NULL);

            /* Make sure we have memory.
             */
            MUT_ASSERT_NOT_NULL(packet.memory_request.ptr);

            /* Put the memory on our temporary list.
             */
            io_cmd_p = packet.memory_request.ptr;

            MUT_ASSERT_TRUE(io_cmd_p->object_handle == 0);
            MUT_ASSERT_TRUE(io_cmd_p->imported_optimum_block_size == 0);
            MUT_ASSERT_TRUE(io_cmd_p->packet_p == 0);
            MUT_ASSERT_TRUE(io_cmd_p->sg_p == 0);
            MUT_ASSERT_TRUE(io_cmd_p->sg_list[0].address == 0);
            MUT_ASSERT_TRUE(io_cmd_p->sg_list[0].count == 0);
            MUT_ASSERT_TRUE(io_cmd_p->sg_list[FBE_LDO_SG_POOL_MAX_SG_LENGTH-1].address == 0);
            MUT_ASSERT_TRUE(io_cmd_p->sg_list[FBE_LDO_SG_POOL_MAX_SG_LENGTH-1].count == 0);

            fbe_queue_push(&allocated_sgs_head, &io_cmd_p->queue_element);

            /* There better be things on our queue.
             */
            MUT_ASSERT_TRUE(!fbe_queue_is_empty(&allocated_sgs_head));
            fbe_semaphore_destroy(&sem);

        }    /* End for all sgs to allocate */

        /* There better be things on our queue.
         */
        MUT_ASSERT_TRUE(!fbe_queue_is_empty(&allocated_sgs_head));

        /* For each item on the queue, pull it off our temporary list and
         * free it. 
         */
        sgs_to_allocate = 0;
        while (!fbe_queue_is_empty(&allocated_sgs_head))
        {
            fbe_queue_element_t *queue_element_p = fbe_queue_pop(&allocated_sgs_head);

            io_cmd_p = fbe_ldo_io_cmd_queue_element_to_pool_element(queue_element_p); 

            MUT_ASSERT_TRUE(io_cmd_p->object_handle == 0);
            MUT_ASSERT_TRUE(io_cmd_p->imported_optimum_block_size == 0);
            MUT_ASSERT_TRUE(io_cmd_p->packet_p == 0);
            MUT_ASSERT_TRUE(io_cmd_p->sg_p == 0);
            MUT_ASSERT_TRUE(io_cmd_p->sg_list[0].address == 0);
            MUT_ASSERT_TRUE(io_cmd_p->sg_list[0].count == 0);
            MUT_ASSERT_TRUE(io_cmd_p->sg_list[FBE_LDO_SG_POOL_MAX_SG_LENGTH-1].address == 0);
            MUT_ASSERT_TRUE(io_cmd_p->sg_list[FBE_LDO_SG_POOL_MAX_SG_LENGTH-1].count == 0);

            fbe_ldo_free_io_cmd(io_cmd_p);
            sgs_to_allocate++;

            /* Make sure we do not go beyond the number expected.
             */
            if (sgs_to_allocate > thread_db_p->num_sgs_to_allocate)
            {
                mut_printf(MUT_LOG_LOW, 
                           "Number of io commands processed %d is unexpected.  Expected: %d\n",
                           sgs_to_allocate, thread_db_p->num_sgs_to_allocate);
                MUT_FAIL_MSG("Number of io commands unexpected.");
            }
        } /* End while queue is not empty. */
        /* Make sure we allocated and freed the amount intended.
         */
        MUT_ASSERT_INT_EQUAL(sgs_to_allocate, thread_db_p->num_sgs_to_allocate);

        /* Tear down our queue.
         */
        fbe_queue_destroy(&allocated_sgs_head);

        iterations++;
    }    /* end for all iterations. */
    return;
}
/******************************************
 * end fbe_ldo_io_cmd_pool_execute_test()
 ******************************************/

/*!**************************************************************
 * fbe_ldo_io_cmd_pool_test_func()
 ****************************************************************
 * @brief
 *  This is the function to start the threads that are testing the io command pool.
 *
 * @param context - This is the thread db structure for this thread.            
 *
 * @return None.   
 *
 * HISTORY:
 *  9/15/2008 - Created. RPF
 *
 ****************************************************************/

static void fbe_ldo_io_cmd_pool_test_func(void * context) 
{
    fbe_ldo_test_io_cmd_pool_thread_db_t *thread_db_p = (fbe_ldo_test_io_cmd_pool_thread_db_t*) context;
    fbe_s32_t sem_status;

    mut_printf(MUT_LOG_LOW, "LDO: 0x%p sg thread %d starting", 
               thread_db_p, thread_db_p->thread_number);

    /* Wait for the semaphore to get signaled which kicks off testing.
     */
    sem_status = fbe_semaphore_wait(&fbe_ldo_test_io_cmd_pool_threads_db.semaphore, NULL);

    /* Make sure the status was expected.
     */
    MUT_ASSERT_INT_EQUAL(sem_status, EMCPAL_STATUS_SUCCESS);

    /* Run test.
     */
    fbe_ldo_io_cmd_pool_execute_test(thread_db_p);

    fbe_semaphore_release(&thread_db_p->thread_semaphore, 0, 1, FBE_FALSE);

    mut_printf(MUT_LOG_LOW, "LDO: 0x%p sg thread %d finished", thread_db_p, thread_db_p->thread_number);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
    return;
}
/******************************************
 * end fbe_ldo_io_cmd_pool_test_func()
 ******************************************/

/*!**************************************************************
 * fbe_ldo_test_io_cmd_pool_threads_db_execute()
 ****************************************************************
 * @brief
 *  Kick off all threads and then wait for them to finish testing.
 *
 * @param threads_db_p - The threads database to test with.               
 *
 * @return None.   
 *
 * HISTORY:
 *  9/15/2008 - Created. RPF
 *
 ****************************************************************/

static void 
fbe_ldo_test_io_cmd_pool_threads_db_execute(fbe_ldo_test_io_cmd_pool_threads_db_t *const threads_db_p)
{
    fbe_u32_t current_thread;

    /* Setup all the threads to test with.
     */
    mut_printf(MUT_LOG_LOW, "LDO: %s starting %d threads",
               __FUNCTION__, threads_db_p->threads_running);
    for (current_thread = 0; current_thread < threads_db_p->threads_running; current_thread++)
    {
        EMCPAL_STATUS thread_status;

        thread_status = fbe_thread_init(&threads_db_p->thread_db_array[current_thread].thread,
                                        "fbe_ldo_io", 
                                        fbe_ldo_io_cmd_pool_test_func, 
                                        &threads_db_p->thread_db_array[current_thread]);
        MUT_ASSERT_TRUE_MSG(thread_status == EMCPAL_STATUS_SUCCESS, "Failed to init thread.");
        mut_printf(MUT_LOG_LOW, "LDO: Thread %d started", current_thread);
    }

    /* Kick off all the threads which should be waiting on the semaphore.
     * This allows all the threads to get kicked off more or less at once. 
     */
    fbe_semaphore_release(&threads_db_p->semaphore, 0, threads_db_p->threads_running, FALSE);

    mut_printf(MUT_LOG_LOW, "LDO: Semaphore released to initiate io command pool test with %d threads", 
               threads_db_p->threads_running);
    /* Wait for threads to finish.
     */
    for (current_thread = 0; current_thread < threads_db_p->threads_running; current_thread++)
    {
        fbe_s32_t sem_status;
        fbe_ldo_test_io_cmd_pool_thread_db_t *thread_info_p;

        thread_info_p = &threads_db_p->thread_db_array[current_thread];
        /* Wait for either the test to finish or our timeout to expire.
         */
        sem_status = fbe_semaphore_wait_ms(&thread_info_p->thread_semaphore,
                                           FBE_LDO_THREAD_WAIT_MSEC);

        if (EMCPAL_STATUS_SUCCESS == sem_status) {
            fbe_thread_wait(&thread_info_p->thread);
            mut_printf(MUT_LOG_LOW, "FBE TEST IO THREAD: Thread %d finished",
                       current_thread);
        } else {
            mut_printf(MUT_LOG_LOW,
                       "FBE TEST IO THREAD: Thread %d did not exit!",
                       current_thread);
        }
        MUT_ASSERT_TRUE(sem_status == EMCPAL_STATUS_SUCCESS);

        fbe_semaphore_destroy(&thread_info_p->thread_semaphore);
        fbe_thread_destroy(&thread_info_p->thread);

        mut_printf(MUT_LOG_LOW, "LDO: Thread %d finished", current_thread);
    }
    return;
}
/******************************************
 * end fbe_ldo_test_io_cmd_pool_threads_db_execute()
 ******************************************/

/*!**************************************************************
 * fbe_ldo_test_start_io_cmd_pool_threads()
 ****************************************************************
 * @brief
 *  Initialize and kick off the test for the io command pool.
 *
 * @param thread_count - Number of threads to start.
 * @param num_sgs_to_allocate - Number of sgs the first thread should allocate.
 * @param num_seconds - Number of iterations for first thread.
 *
 * @return None.
 *
 * HISTORY:
 *  9/15/2008 - Created. RPF
 *
 ****************************************************************/

static void fbe_ldo_test_start_io_cmd_pool_threads(fbe_u32_t thread_count,
                                                   fbe_u32_t num_sgs_to_allocate,
                                                   fbe_u32_t num_seconds) 
{
    /* Create the threads db.
     */
    fbe_ldo_test_io_cmd_pool_threads_db_init(&fbe_ldo_test_io_cmd_pool_threads_db, 
                                             thread_count, 
                                             num_sgs_to_allocate,
                                             num_seconds);

    /* Execute the threads db.
     */
    fbe_ldo_test_io_cmd_pool_threads_db_execute(&fbe_ldo_test_io_cmd_pool_threads_db);

    /* Destroy the threads db.
     */
    fbe_ldo_test_io_cmd_pool_threads_db_destroy(&fbe_ldo_test_io_cmd_pool_threads_db);
    return;
}
/******************************************
 * end fbe_ldo_test_start_io_cmd_pool_threads()
 ******************************************/

/*!***************************************************************
 * fbe_ldo_test_io_cmd_pool()
 ****************************************************************
 * @brief
 *  This function performs a write same io test.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * HISTORY:
 *  09/15/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_io_cmd_pool(void)
{
    fbe_u32_t threads;
    fbe_status_t status;

    /* Create the pool.
     */
    status = fbe_ldo_io_cmd_pool_init();
    MUT_ASSERT_FBE_STATUS_OK(status);

    /* With one thread, just allocate everything in the pool and free it.
     */
    fbe_ldo_test_start_io_cmd_pool_threads(1, /* Thread count */ 
                                           FBE_LDO_IO_CMD_POOL_MAX, 
                                           FBE_LDO_TEST_IO_CMD_POOL_MAX_SECONDS);

    /* For all the other thread counts, limit how much the first thread can
     * allocate.  This prevents deadlocks.
     */
    for (threads = 2; threads < FBE_LDO_TEST_IO_CMD_POOL_MAX_THREAD; threads++)
    {
        fbe_ldo_test_start_io_cmd_pool_threads(threads, 
                                           FBE_LDO_IO_CMD_POOL_MAX, 
                                           FBE_LDO_TEST_IO_CMD_POOL_MAX_SECONDS);

        /* Make sure the pool still has the same number of objects as when it was initialized.
         */
        MUT_ASSERT_INT_EQUAL_MSG(fbe_ldo_io_cmd_pool_get_free_count(), 
                                 FBE_LDO_IO_CMD_POOL_MAX,
                                 "Expected pool size to remain unchanged.");
    }

    /* Tear down the pool.
     */
    fbe_ldo_io_cmd_pool_destroy();
    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_io_cmd_pool_test() */

/*****************************************
 * end fbe_logical_drive_test_io_cmd_pool.c
 *****************************************/
