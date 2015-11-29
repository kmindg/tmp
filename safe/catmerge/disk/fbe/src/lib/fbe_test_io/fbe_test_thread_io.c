 /***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_thread_io.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_test_io library's I/O testing functions for
 *  testing with multiple threads.
 *
 * @version
 *  10/27/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"

#include "fbe_trace.h"

#include "fbe/fbe_logical_drive.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_test_io_api.h"
#include "mut.h"
#include "fbe/fbe_api_common_transport.h"

/************************************************** 
 * LOCAL STRUCTURE DEFINITIONS
 **************************************************/

 /*! @def FBE_TEST_IO_THREAD_MAX_DISPLAY_SECONDS
  *  @brief max seconds in between displaying something during test. 
  */
#define FBE_TEST_IO_THREAD_MAX_DISPLAY_SECONDS 2

/*! @def FBE_TEST_THREAD_PACKET_TIMEOUT 
 *  
 *  @brief This is the number of seconds we will wait on a
 *         semaphore for a packet.  If it does not complete
 *         in 20 seconds, then something is wrong.
 */
#define FBE_TEST_THREAD_PACKET_TIMEOUT  (-20 * (10000000L))

/*! @def FBE_TEST_THREAD_MSEC 
 *  
 *  @brief This is the number of milliseconds we will wait on a
 *         semaphore for a packet.
 *         If it does not complete in 60 seconds, then something is wrong.
 */
#define FBE_TEST_THREAD_MSEC 60000

struct fbe_test_io_thread_db_s;

/*! @enum fbe_test_io_state_status_enum_t
 *  
 * @brief This is the set of states returned by functions of the test io state 
 *        machines.
 */
typedef enum fbe_test_io_state_status_enum_e
{
    FBE_TEST_IO_STATE_STATUS_INVALID = 0, /*!< This should always be first. */

    /*! This means we will go execute the next state now. 
     */
    FBE_TEST_IO_STATE_STATUS_EXECUTING,

    /*! Waiting for a response of some kind, not executing the next state now. 
     */
    FBE_TEST_IO_STATE_STATUS_WAITING,

    /*! Done, but not waiting for a response. 
     */
    FBE_TEST_IO_STATE_STATUS_DONE,

    /*! This should always be last. 
     */
	FBE_TEST_IO_STATE_STATUS_LAST
}
fbe_test_io_state_status_enum_t;

/*! @typedef fbe_test_io_thread_packet_state_t
 *  
 * @brief 
 *   This defines the state field of the io thread packet db.
 */
struct fbe_test_io_thread_packet_ts_s;
typedef fbe_test_io_state_status_enum_t(*fbe_test_io_thread_packet_state_t) (struct fbe_test_io_thread_packet_ts_s *const);

/*! @struct fbe_test_io_thread_packet_ts_t
 *  @brief  Each thread has multiple packets associated with it.
 *          Each packet has a database structure that
 *          associates it with a thread db.
 */
typedef struct fbe_test_io_thread_packet_ts_s 
{
    struct fbe_test_io_thread_db_s *parent_thread_db_p;
    fbe_test_io_thread_packet_state_t state;
    fbe_packet_t *packet_p;
    fbe_sg_element_t *sg_p;
    fbe_bool_t b_outstanding;
    fbe_u32_t pass_count;
    fbe_lba_t current_lba;
    fbe_block_count_t current_blocks;
}   
fbe_test_io_thread_packet_ts_t;

struct fbe_test_io_threads_db_s;

/*! @struct fbe_test_io_thread_db_t
 *  @brief  This is the database for a single thread.
 *          This thread has within it threads that will
 *          execute asyncnronously.
 *          This structure contains the needed info to synchronize those
 *          threads.
 */
typedef struct fbe_test_io_thread_db_s 
{
    /*! The pointer to the parent io threads db structure.
     */
    struct fbe_test_io_threads_db_s *parent_threads_db_p;

    fbe_object_id_t object_id;/*!< Object id to issue to. */

    /*! Semaphore to synchronize the outstanding packets.
     */
    fbe_semaphore_t outstanding_packet_semaphore;

    /*! Semaphore to synchronize thread termination.
     */
    fbe_semaphore_t thread_semaphore;

    /*! Count of number of packets outstanding
     */
    fbe_u32_t outstanding_count;
    
    fbe_bool_t b_running;/*!< True if running, false if being terminated. */

    /*! Lock to protect changing the fields in this structure.
     */
    fbe_spinlock_t spinlock;

    /*! This is the FBE thread structure that our thread 
     *  uses to execute. 
     */
    fbe_thread_t thread;

    /*! This is the index 0..N of this thread within the parent's 
     *  threads db structure. 
     */
    fbe_u32_t thread_number;

    /*! This is the array of packet ts structures.
     */
    fbe_test_io_thread_packet_ts_t *packet_ts_array;
}   
fbe_test_io_thread_db_t;

/*! @struct fbe_test_io_threads_db_t  
 *  @brief
 *     This is the structure that manages and coordinates a set of threads
 *     running I/O tests.
 */
typedef struct fbe_test_io_threads_db_s 
{
    fbe_u32_t threads_running; /*!< Number of threads. */
    fbe_test_io_thread_task_t *task_p; /*!< Test task to execute for each thread. */
    fbe_semaphore_t start_io_semaphore; /*! Coordinates the starting of the threads. */

    /*! Array of threads associated with this test.
     */
    fbe_test_io_thread_db_t *thread_db_array;
}
fbe_test_io_threads_db_t;


/************************************************** 
 * LOCAL FUNCTION DEFINITIONS
 **************************************************/
void fbe_test_io_thread_db_packet_test(fbe_test_io_thread_db_t *thread_db_p);
fbe_test_io_state_status_enum_t fbe_test_io_thread_packet_rd_state1(fbe_test_io_thread_packet_ts_t *const packet_ts_p);
static fbe_status_t fbe_test_io_thread_packet_ts_send(fbe_test_io_thread_packet_ts_t *packet_ts_p, 
                                                      fbe_object_id_t object_id);


/*!**************************************************************
 * @fn fbe_test_io_thread_task_init(fbe_test_io_thread_task_t *const task_p,
 *                                  fbe_test_io_task_type_t test_type,
 *                                  fbe_block_size_t exported_block_size,
 *                                  fbe_block_size_t exported_opt_block_size,
 *                                  fbe_block_size_t imported_block_size,
 *                                  fbe_block_size_t imported_opt_block_size,
 *                                  fbe_u32_t packets_per_thread,
 *                                  fbe_block_count_t max_blocks,
 *                                  fbe_u32_t seconds_to_test)
 ****************************************************************
 * @brief
 *  Fills out all the information in the task structure.
 *  Everything we need to initialize it is passed in.
 *  
 * @param task_p - The task object that contains information about
 *                 the type of I/O test to run such as max blocks
 *                 and max threads.
 * @param test_type - Type of test, read only, write only,
 *                    or write/read/check, etc.
 * @param exported_block_size - Size in bytes of the exported block
 * @param exported_opt_block_size - Number of exported blocks in
 *                                  the optimal size.
 * @param imported_block_size - Size in bytes of the imported block.
 * @param imported_opt_block_size - Number of imported blocks per
 *                                  the optimal block.
 * @param packets_per_thread - Number of packets we will have outstanding
 *                             per thread.
 * @param max_blocks - Max number of blocks to issue.
 * @praam test_seconds - Seconds to run test for.
 *
 * @return None.   
 *
 * HISTORY:
 *  10/28/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_test_io_thread_task_init(fbe_test_io_thread_task_t *const task_p,
                                  fbe_test_io_task_type_t test_type,
                                  fbe_block_size_t exported_block_size,
                                  fbe_block_size_t exported_opt_block_size,
                                  fbe_block_size_t imported_block_size,
                                  fbe_block_size_t imported_opt_block_size,
                                  fbe_u32_t packets_per_thread,
                                  fbe_block_count_t max_blocks,
                                  fbe_u32_t seconds_to_test)
{
    task_p->exported_block_size = exported_block_size;
    task_p->exported_opt_block_size = exported_opt_block_size;
    task_p->imported_block_size = imported_block_size;
    task_p->imported_opt_block_size = imported_opt_block_size;
    task_p->packets_per_thread = packets_per_thread;
    task_p->blocks = max_blocks;
    task_p->test_type = test_type;
    task_p->seconds_to_test = seconds_to_test;
    return;
}
/**************************************
 * end fbe_test_io_thread_task_init()
 **************************************/

/*!**************************************************************
 * @fn fbe_io_test_packet_ts_state_machine(fbe_test_io_thread_packet_ts_t *packet_ts_p)
 ****************************************************************
 * @brief
 *  This function executes the state machine of the io thread packet
 *  structure.  We simply execute the state until something other
 *  than executing is returned.
 *  
 * @param packet_ts_p - The io thread packet structure.
 *
 * @return  - None.  
 *
 * HISTORY:
 *  10/27/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_io_test_packet_ts_state_machine(fbe_test_io_thread_packet_ts_t *packet_ts_p)
{
    fbe_test_io_thread_packet_state_t state;
    fbe_test_io_state_status_enum_t state_status;

    /* Execute the state of this object until it does not return
     * the executing state.
     */
    do
    {
        /* Get the state for the next pass of the loop.
         */
        state = packet_ts_p->state;

        /* Execute the state, which returns the status we will check below.
         */
        state_status = state(packet_ts_p);
    }
    while ( FBE_TEST_IO_STATE_STATUS_EXECUTING == state_status );
    
    return;
}
/**************************************
 * end fbe_io_test_packet_ts_state_machine()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_thread_packet_get_next_lba_blocks(
 *              fbe_test_io_thread_packet_ts_t *const packet_ts_p)
 ****************************************************************
 * @brief
 *  Fill in the packet database structure with the next
 *  lba and block count for this test.
 *  
 * @param packet_ts_p - The packet database to get the next lba and
 *                      block count for.
 *
 * @return - None.   
 *
 * HISTORY:
 *  10/28/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_test_io_thread_packet_get_next_lba_blocks(fbe_test_io_thread_packet_ts_t *const packet_ts_p)
{
    packet_ts_p->current_lba = 0;
    packet_ts_p->current_blocks = 1;
    return;
}
/**************************************
 * end fbe_test_io_thread_packet_get_next_lba_blocks()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_thread_packet_rd_state0(fbe_test_io_thread_packet_ts_t *const packet_ts_p)
 ****************************************************************
 * @brief
 *  This is the first state of the io thread packet state machine.
 *  Here we will simply allocate resources, fill out our packet
 *  and send the packet.
 * 
 * @param packet_ts_p - The io thread packet structure.
 * 
 * @return fbe_test_io_state_status_enum_t
 *
 * @return Always FBE_TEST_IO_STATE_STATUS_WAITING   
 *
 * HISTORY:
 *  10/28/2008 - Created. RPF
 *
 ****************************************************************/

fbe_test_io_state_status_enum_t 
fbe_test_io_thread_packet_rd_state0(fbe_test_io_thread_packet_ts_t *const packet_ts_p)
{
    fbe_test_io_thread_db_t *thread_db_p = packet_ts_p->parent_thread_db_p;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * payload_block_operation = NULL;
    fbe_test_io_thread_task_t *task_p = thread_db_p->parent_threads_db_p->task_p;

    /* Since we might have used the packet before, just reinitialize it.
     */
    fbe_transport_reuse_packet(packet_ts_p->packet_p);
    payload = fbe_transport_get_payload_ex(packet_ts_p->packet_p);
    payload_block_operation = fbe_payload_ex_allocate_block_operation(payload);

    fbe_test_io_thread_packet_get_next_lba_blocks(packet_ts_p);

    /* Get memory for this operation.  We assume it was previously freed.
     */
    MUT_ASSERT_NULL(packet_ts_p->sg_p);
    packet_ts_p->sg_p = fbe_test_io_alloc_memory_with_sg((fbe_u32_t)(packet_ts_p->current_blocks * task_p->exported_block_size));
    MUT_ASSERT_NOT_NULL(packet_ts_p->sg_p);

    status = fbe_payload_block_build_operation(payload_block_operation,
                                               FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                               packet_ts_p->current_lba,
                                               packet_ts_p->current_blocks,
                                               task_p->exported_block_size,
                                               task_p->exported_opt_block_size,
                                               NULL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_payload_ex_set_sg_list(payload, packet_ts_p->sg_p, 1);

    packet_ts_p->state = fbe_test_io_thread_packet_rd_state1;

    /* Send the packet.
     */
    status = fbe_test_io_thread_packet_ts_send(packet_ts_p, thread_db_p->object_id);

    return FBE_TEST_IO_STATE_STATUS_WAITING;
}
/**************************************
 * end fbe_test_io_thread_packet_rd_state0()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_thread_packet_rd_state1(fbe_test_io_thread_packet_ts_t *const packet_ts_p)
 ****************************************************************
 * @brief
 *  This is state one of the io thread packet state machine.
 *  Here we simply check the status of the operation and
 *  free memory before 
 * 
 * @param packet_ts_p - The io thread packet structure.
 * 
 * @return fbe_test_io_state_status_enum_t
 *
 * @return Always FBE_TEST_IO_STATE_STATUS_EXECUTING.
 *
 * HISTORY:
 *  10/28/2008 - Created. RPF
 *
 ****************************************************************/

fbe_test_io_state_status_enum_t fbe_test_io_thread_packet_rd_state1(fbe_test_io_thread_packet_ts_t *const packet_ts_p)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * payload_block_operation = NULL;
    fbe_status_t packet_status;
    fbe_payload_block_operation_status_t block_status;

    /* Make sure we did not get an error.
     */
    payload = fbe_transport_get_payload_ex(packet_ts_p->packet_p);
    MUT_ASSERT_NOT_NULL(payload);
    payload_block_operation = fbe_payload_ex_get_block_operation(payload);
    MUT_ASSERT_NOT_NULL(payload_block_operation);

    packet_status = fbe_transport_get_status_code(packet_ts_p->packet_p);
    MUT_ASSERT_INT_EQUAL(packet_status, FBE_STATUS_OK);

    fbe_payload_block_get_status(payload_block_operation, &block_status);
    MUT_ASSERT_INT_EQUAL(block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

    /* Increment the pass.
     */
    packet_ts_p->pass_count++;

    /* Free up the memory we allocated previously.
     */
    fbe_test_io_free_memory_with_sg(&packet_ts_p->sg_p);

    packet_ts_p->state = fbe_test_io_thread_packet_rd_state0;

    return FBE_TEST_IO_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_test_io_thread_packet_rd_state1()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_thread_packet_ts_init(
 *              fbe_test_io_thread_packet_ts_t *const packet_ts_p,
 *              fbe_test_io_thread_db_t *const thread_db_p)
 ****************************************************************
 * @brief
 *  Initialize the packet ts.
 *  
 * @param packet_ts_p - The io thread packet structure.
 * @param thread_db_p - The thread database for just this thread.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  10/27/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_status_t fbe_test_io_thread_packet_ts_init(fbe_test_io_thread_packet_ts_t *const packet_ts_p,
                                                      fbe_test_io_thread_db_t *const thread_db_p) 
{
    fbe_test_io_thread_task_t *const task_p = thread_db_p->parent_threads_db_p->task_p;
    fbe_test_io_thread_packet_state_t first_state = NULL;

    packet_ts_p->packet_p = (fbe_packet_t *) fbe_api_allocate_memory(sizeof (fbe_packet_t));

    /* No memory for this packet, assert and return failure.
     */
    MUT_ASSERT_NOT_NULL(packet_ts_p->packet_p);

    if (packet_ts_p->packet_p == NULL)
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_transport_initialize_packet(packet_ts_p->packet_p);
    packet_ts_p->sg_p = NULL;
    fbe_transport_set_cpu_id(packet_ts_p->packet_p, 0);

    packet_ts_p->b_outstanding = FBE_FALSE;
    packet_ts_p->parent_thread_db_p = thread_db_p;

    /* Setup our initial state based on the type of the test.
     */
    switch (task_p->test_type)
    {
        default:
        case FBE_IO_TEST_TASK_TYPE_READ_ONLY:
            first_state = fbe_test_io_thread_packet_rd_state0;
            break;
    };

    packet_ts_p->state = first_state;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_test_io_thread_packet_ts_init()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_packet_ts_destroy(fbe_test_io_thread_packet_ts_t *const packet_ts_p)
 ****************************************************************
 * @brief
 *  Destroy the packet tracking structure.
 *  
 * @param packet_ts_p - The io thread packet structure.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  10/27/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_status_t fbe_test_packet_ts_destroy(fbe_test_io_thread_packet_ts_t *const packet_ts_p) 
{
    fbe_transport_destroy_packet(packet_ts_p->packet_p);

    if (packet_ts_p->sg_p != NULL)
    {
        fbe_test_io_free_memory_with_sg(&packet_ts_p->sg_p);
    }

    fbe_api_free_memory(packet_ts_p->packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_test_packet_ts_destroy()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_thread_db_init(fbe_test_io_threads_db_t *const threads_db_p,
 *                                fbe_test_io_thread_db_t *const thread_db_p,
 *                                fbe_u32_t thread_number,
 *                                fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *   This initizlizes a thread db and it's associated structures.
 *  
 * @param threads_db_p - The io threads database for all threads.
 * @param thread_db_p - The thread database for just this thread.
 * @param thread_number - The index of this thread in the parent
 *                        thread_db array.
 * @param object_id - Object id to test.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  10/27/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_status_t fbe_test_io_thread_db_init(fbe_test_io_threads_db_t *const threads_db_p,
                                               fbe_test_io_thread_db_t *const thread_db_p,
                                               fbe_u32_t thread_number,
                                               fbe_object_id_t object_id) 
{
    fbe_u32_t index;
    fbe_status_t status = FBE_STATUS_INVALID;
    thread_db_p->parent_threads_db_p = threads_db_p;

    /* Init the semaphore that synchronizes packet completions. 
     */
    fbe_spinlock_init(&thread_db_p->spinlock);
    fbe_semaphore_init(&thread_db_p->outstanding_packet_semaphore, 0, threads_db_p->task_p->packets_per_thread);
    fbe_semaphore_init(&thread_db_p->thread_semaphore, 0, 1);
    thread_db_p->b_running = FBE_TRUE;
    thread_db_p->outstanding_count = 0;
    thread_db_p->thread_number = thread_number;
    thread_db_p->object_id = object_id;

    thread_db_p->packet_ts_array = malloc(sizeof(fbe_test_io_thread_packet_ts_t) * threads_db_p->task_p->packets_per_thread);
    MUT_ASSERT_NOT_NULL(thread_db_p->packet_ts_array);

    /* Initialize all the packets associated with this thread.
     */
    for(index = 0; index < threads_db_p->task_p->packets_per_thread; index++)
    {
        status = fbe_test_io_thread_packet_ts_init(&thread_db_p->packet_ts_array[index],
                                                   thread_db_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_test_io_thread_db_init()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_thread_db_destroy(fbe_test_io_thread_db_t *const thread_db_p)
 ****************************************************************
 * @brief
 *  Destroy the thread database structure.
 *  
 * @param thread_db_p - The thread database for just this thread.
 *
 * HISTORY:
 *  10/27/2008 - Created. RPF
 *
 ****************************************************************/

static void fbe_test_io_thread_db_destroy(fbe_test_io_thread_db_t *const thread_db_p) 
{
    fbe_u32_t index;
    /* Wait for all outstanding packets to complete.
     * We do not wait more than 20 seconds for the packets to complete. 
     * If they take longer than this, then something is wrong. 
     */
    for (index = 0 ; 
          index < thread_db_p->parent_threads_db_p->task_p->packets_per_thread; 
          index++)
    {
        EMCPAL_STATUS status;
        EMCPAL_LARGE_INTEGER timeout;
        timeout.QuadPart = FBE_TEST_THREAD_PACKET_TIMEOUT;

        status = fbe_semaphore_wait(&thread_db_p->outstanding_packet_semaphore, &timeout);

        /* Packets should not take more than 20 seconds to complete.
         */
        MUT_ASSERT_INT_EQUAL(EMCPAL_STATUS_SUCCESS, status);
    }

    /* Destroy all the packets associated with this thread.
     */
    for(index = 0; index < thread_db_p->parent_threads_db_p->task_p->packets_per_thread; index++)
    {
        fbe_test_packet_ts_destroy(&thread_db_p->packet_ts_array[index]);
    }

    /* Destroy remaining resources.
     */
    fbe_spinlock_destroy(&thread_db_p->spinlock);
    fbe_semaphore_destroy(&thread_db_p->outstanding_packet_semaphore);
    fbe_semaphore_destroy(&thread_db_p->thread_semaphore);
    free(thread_db_p->packet_ts_array);

    return;
}
/**************************************
 * end fbe_test_io_thread_db_destroy()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_threads_db_init(fbe_test_io_threads_db_t *const threads_db_p,
 *                                 const fbe_u32_t total_threads,
 *                                 fbe_test_io_thread_task_t *const task_p,
 *                                 fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *   This initializes the threads database structure.
 *  
 * @param threads_db_p - The io threads database for all threads.
 * @param total_threads - Number of threads to start.
 * @param task_p - The I/O task object with information about the test.
 * @param object_id - Object id to issue to.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  10/27/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_status_t fbe_test_io_threads_db_init(fbe_test_io_threads_db_t *const threads_db_p,
                                                const fbe_u32_t total_threads,
                                                fbe_test_io_thread_task_t *const task_p,
                                                fbe_object_id_t object_id) 
{
    fbe_u32_t index;
    threads_db_p->threads_running = total_threads;
    threads_db_p->task_p = task_p;

    fbe_semaphore_init(&threads_db_p->start_io_semaphore, 0, total_threads);
    threads_db_p->thread_db_array = (fbe_test_io_thread_db_t*) malloc( total_threads * sizeof(fbe_test_io_thread_db_t) );
    MUT_ASSERT_NOT_NULL(threads_db_p->thread_db_array);

    /* Initialize all the threads associated with this threads db.
     */
    for(index = 0 ; index < threads_db_p->threads_running; index++)
    {
        fbe_test_io_thread_db_init(threads_db_p,
                                    &threads_db_p->thread_db_array[index],
                                    index /* thread number. */,
                                    object_id);
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_test_io_threads_db_init()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_threads_db_destroy(fbe_test_io_threads_db_t *const threads_db_p)
 ****************************************************************
 * @brief
 *  This destroys the threads database structure.
 *  
 * @param threads_db_p - The io threads database for all threads.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  10/27/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_status_t fbe_test_io_threads_db_destroy(fbe_test_io_threads_db_t *const threads_db_p) 
{
    fbe_u32_t index;
    /* Destroy all the threads associated with this threads db.
     */
    for(index = 0 ; index < threads_db_p->threads_running; index++)
    {
        fbe_test_io_thread_db_destroy(&threads_db_p->thread_db_array[index]);
    }

    /* Free the memory we allocated.
     */
    free(threads_db_p->thread_db_array); 
    threads_db_p->thread_db_array = NULL;

    fbe_semaphore_destroy(&threads_db_p->start_io_semaphore);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_test_io_threads_db_destroy()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_thread_func(void * context)
 ****************************************************************
 * @brief
 *  This is the function that gets called when a new thread
 *  of I/O starts.
 * 
 *  We simply wait to be signaled to start, and then kick off
 *  I/O.
 *  
 * @param context - This is the thread db for our thread.
 *
 * @return  - None.   
 *
 * HISTORY:
 *  10/27/2008 - Created. RPF
 *
 ****************************************************************/

static void fbe_test_io_thread_func(void * context) 
{
    fbe_test_io_thread_db_t *thread_db_p = (fbe_test_io_thread_db_t*) context;
    fbe_s32_t sem_status;

    mut_printf(MUT_LOG_LOW, "FBE TEST IO THREAD: 0x%p I/O thread starting", thread_db_p);

    /* Wait for the semaphore to get signaled which kicks off testing.
     */
    sem_status = fbe_semaphore_wait(&thread_db_p->parent_threads_db_p->start_io_semaphore, NULL);

    /* Run test.
     */
    fbe_test_io_thread_db_packet_test(thread_db_p);

    fbe_semaphore_release(&thread_db_p->thread_semaphore, 0, 1, FALSE);
    mut_printf(MUT_LOG_LOW, "FBE TEST IO THREAD: 0x%p I/O thread finished", thread_db_p);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
    return;
}
/**************************************
 * end fbe_test_io_thread_func()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_threads_db_run(fbe_test_io_threads_db_t *const threads_db_p)
 ****************************************************************
 * @brief
 *  This kicks off all threads that are a part of our threads db structure, and
 *  then waits for them to finish.
 *  
 * @param threads_db_p - The io threads database for all threads.
 *
 * @return  - None. 
 *
 * HISTORY:
 *  10/27/2008 - Created. RPF
 *
 ****************************************************************/

static void fbe_test_io_threads_db_run(fbe_test_io_threads_db_t *const threads_db_p)
{
    fbe_u32_t current_thread;

    /* Setup all the threads to test with.
     */
    mut_printf(MUT_LOG_LOW, "FBE TEST IO THREAD: %s starting %d threads",
               __FUNCTION__, threads_db_p->threads_running);

    for (current_thread = 0; current_thread < threads_db_p->threads_running; current_thread++)
    {
        EMCPAL_STATUS thread_status;

        thread_status = fbe_thread_init(&threads_db_p->thread_db_array[current_thread].thread, "fbe_test_io", 
                                        fbe_test_io_thread_func, &threads_db_p->thread_db_array[current_thread]);

        MUT_ASSERT_INT_EQUAL_MSG(thread_status, EMCPAL_STATUS_SUCCESS, "Failed to init thread.");
        mut_printf(MUT_LOG_LOW, "FBE TEST IO THREAD: Thread %d started", current_thread);
    }

    /* Kick off all the threads which should be waiting on the start_io_semaphore.
     * This allows all the threads to get kicked off more or less at once. 
     */
    fbe_semaphore_release(&threads_db_p->start_io_semaphore, 0, threads_db_p->threads_running, FALSE);

    mut_printf(MUT_LOG_LOW, "FBE TEST IO THREAD: Semaphore released to initiate I/O");

    /* Wait for all threads to finish.
     */
    for (current_thread = 0; current_thread < threads_db_p->threads_running; current_thread++)
    {
        fbe_s32_t sem_status;

        sem_status = fbe_semaphore_wait_ms(&threads_db_p->thread_db_array[current_thread].thread_semaphore, FBE_TEST_THREAD_MSEC);

        if (EMCPAL_STATUS_SUCCESS == sem_status) {
            fbe_thread_wait(&threads_db_p->thread_db_array[current_thread].thread);
            mut_printf(MUT_LOG_LOW, "FBE TEST IO THREAD: Thread %d finished",
                       current_thread);
        } else {
            mut_printf(MUT_LOG_LOW,
                       "FBE TEST IO THREAD: Thread %d did not exit!",
                       current_thread);
        }
        MUT_ASSERT_INT_EQUAL(sem_status, EMCPAL_STATUS_SUCCESS);
        fbe_thread_destroy(&threads_db_p->thread_db_array[current_thread].thread);
    } /* end for all threads. */
    return;
}
/**************************************
 * end fbe_test_io_threads_db_run()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_thread_packet_ts_send_completion(
 *          fbe_packet_t * packet,
 *          fbe_packet_completion_context_t context)
 ****************************************************************
 * @brief
 *   This is the completion function for the packet.
 *  
 * @param packet_p - Packet that is completing.
 * @param context - Ptr to the io thread packet ts.
 * 
 * @return fbe_status_t - Always OK.
 *
 * HISTORY:
 *  10/27/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_status_t fbe_test_io_thread_packet_ts_send_completion(fbe_packet_t * packet_p, 
                                                                 fbe_packet_completion_context_t context)
{
    fbe_test_io_thread_packet_ts_t *packet_ts_p = (fbe_test_io_thread_packet_ts_t *)context;

    /* Simply update some stats and
     * release the sempahore the caller is waiting on.
     */
    fbe_spinlock_lock(&packet_ts_p->parent_thread_db_p->spinlock);
    MUT_ASSERT_INT_EQUAL(packet_ts_p->b_outstanding, FBE_TRUE);
    packet_ts_p->b_outstanding = FALSE;
    packet_ts_p->parent_thread_db_p->outstanding_count--;
    fbe_spinlock_unlock(&packet_ts_p->parent_thread_db_p->spinlock);

    fbe_semaphore_release(&packet_ts_p->parent_thread_db_p->outstanding_packet_semaphore, 0, 1, FALSE);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_test_io_thread_packet_ts_send_completion()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_thread_packet_ts_send(fbe_test_io_thread_packet_ts_t *packet_ts_p, 
 *                                       fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  Send out the packet for this packet_ts structure.
 *  
 * @param packet_ts_p - The io thread packet structure.
 * @param object_id - Object id to send to.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  10/27/2008 - Created. RPF
 *
 ************************************************************/

static fbe_status_t fbe_test_io_thread_packet_ts_send(fbe_test_io_thread_packet_ts_t *packet_ts_p, 
                                                      fbe_object_id_t object_id)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_packet_t * packet = NULL;

    packet = packet_ts_p->packet_p;

    fbe_transport_set_completion_function(packet, fbe_test_io_thread_packet_ts_send_completion, packet_ts_p);

    /* Setup the transport ID and the id of the object we are sending to.
     */
    fbe_transport_set_address( packet,
                               FBE_PACKAGE_ID_PHYSICAL,
                               FBE_SERVICE_ID_TOPOLOGY,
                               FBE_CLASS_ID_INVALID,
                               object_id );

    /* Increment the block operation level since we are not sending this through 
     * an edge. 
     */
    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_increment_block_operation_level(payload);

    /* This should not yet be outstanding since we are about to send it.
     */
    MUT_ASSERT_INT_EQUAL(packet_ts_p->b_outstanding, FBE_FALSE);

    /* Increment values under protection of the lock.
     */
    fbe_spinlock_lock(&packet_ts_p->parent_thread_db_p->spinlock);
    packet_ts_p->b_outstanding = FBE_TRUE;
    packet_ts_p->parent_thread_db_p->outstanding_count++;
    fbe_spinlock_unlock(&packet_ts_p->parent_thread_db_p->spinlock);

    /* Send out the io packet to the address we placed in the packet.
     */
    status = fbe_api_common_send_io_packet_to_driver(packet);
    return status;
}
/**************************************
 * end fbe_test_io_thread_packet_ts_send()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_thread_db_wait_and_resend_packet(fbe_test_io_thread_db_t *const thread_db_p)
 ****************************************************************
 * @brief
 *  This function waits for a single packet to complete and
 *  then will kick off the state machine to re-send the packet.
 *  
 * @param thread_db_p - The thread database for just this thread.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  10/27/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_test_io_thread_db_wait_and_resend_packet(fbe_test_io_thread_db_t *const thread_db_p)
{
    EMCPAL_STATUS nt_status;
    fbe_u32_t index;
    EMCPAL_LARGE_INTEGER timeout;
    timeout.QuadPart = FBE_TEST_THREAD_PACKET_TIMEOUT;  /* 20 sec (relative time) */

    nt_status = fbe_semaphore_wait(&thread_db_p->outstanding_packet_semaphore, &timeout);

    /* Packets should not take more than 20 seconds to complete.
     */
    MUT_ASSERT_INT_EQUAL(EMCPAL_STATUS_SUCCESS, nt_status);

    /* Get spinlock so we can look at the b_outstanding values.
     */
    fbe_spinlock_lock(&thread_db_p->spinlock);

    /* Find the first thing that needs to be re-sent.
     */
    index = 0;
    while (thread_db_p->packet_ts_array[index].b_outstanding == FBE_TRUE &&
           index < thread_db_p->parent_threads_db_p->task_p->packets_per_thread)
    {
        index++;
    }
    fbe_spinlock_unlock(&thread_db_p->spinlock);

    /* Make sure we did not go beyond the array.
     */
    MUT_ASSERT_TRUE(index < thread_db_p->parent_threads_db_p->task_p->packets_per_thread);

    /* The packet we found to kick off should have completed.
     */
    MUT_ASSERT_INT_EQUAL(thread_db_p->packet_ts_array[index].b_outstanding, FBE_FALSE);

    /* We will simply execute the state machine for this object.
     */
    fbe_io_test_packet_ts_state_machine(&thread_db_p->packet_ts_array[index]);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_test_io_thread_db_wait_and_resend_packet()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_thread_db_start_packet_ts(fbe_test_io_thread_db_t *const thread_db_p)
 ****************************************************************
 * @brief
 *   This function kick starts the packet databases in order
 *   to get them to send out their first I/Os.
 *  
 * @param thread_db_p - The thread database for just this thread.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  10/27/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_test_io_thread_db_start_packet_ts(fbe_test_io_thread_db_t *const thread_db_p)
{
    fbe_u32_t index;

    /* Simply send out all the packets for this thread db.
     */
    for(index = 0 ; index < thread_db_p->parent_threads_db_p->task_p->packets_per_thread; index++)
    {
        fbe_io_test_packet_ts_state_machine(&thread_db_p->packet_ts_array[index]);
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_test_io_thread_db_wait_and_resend_packet()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_thread_db_packet_test(fbe_test_io_thread_db_t *thread_db_p) 
 ****************************************************************
 * @brief
 *  This is the function that kicks off all the packets in a thread.
 *  It then goes into a loop for the test duration where it waits for
 *  a single packet to finish, and then re-issues this packet.
 *  
 * @param thread_db_p - The thread database for just this thread.
 *
 * @return  - None.  
 *
 * HISTORY:
 *  10/27/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_test_io_thread_db_packet_test(fbe_test_io_thread_db_t *thread_db_p)
{
    fbe_u32_t iterations;
    #define MILLISECONDS_PER_SECOND 1000 /* Used for displaying. */
    fbe_u64_t start_seconds;
    fbe_u64_t current_seconds;
    fbe_u64_t last_display_seconds;
    fbe_u32_t seconds_to_test = thread_db_p->parent_threads_db_p->task_p->seconds_to_test;

    /* Get the time we are starting at.  We'll compare to this over 
     * the test to determine when we are done.
     */
    start_seconds = (fbe_u32_t)(fbe_get_time() / MILLISECONDS_PER_SECOND);
    last_display_seconds = start_seconds;
    
    mut_printf(MUT_LOG_TEST_STATUS, "FBE TEST IO THREAD: Kicking off I/Os %p thread number %d for %d seconds.\n",
               thread_db_p, thread_db_p->thread_number,
               thread_db_p->parent_threads_db_p->task_p->seconds_to_test);

    /* Send all packets for the first time.
     */
    fbe_test_io_thread_db_start_packet_ts(thread_db_p);

    iterations = 0;
    current_seconds = (fbe_u32_t)(fbe_get_time() / MILLISECONDS_PER_SECOND);

    /* Continue running until the number of seconds to test is exhausted.
     */
    while ((current_seconds - start_seconds) < seconds_to_test)
    {
        /* Wait for one packet to complete, and resend it.
         */
        fbe_test_io_thread_db_wait_and_resend_packet(thread_db_p);
        iterations++;

        /* Re-calc seconds.
         */
        current_seconds = (fbe_u32_t)(fbe_get_time() / MILLISECONDS_PER_SECOND);

        if ((current_seconds - last_display_seconds) > FBE_TEST_IO_THREAD_MAX_DISPLAY_SECONDS)
        {
            /* At a fixed interval, display the number of elapsed seconds.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "FBE TEST IO THREAD: thread: %d total seconds: %llu iterations: %d \n",
                       thread_db_p->thread_number,
		       (unsigned long long)(current_seconds - start_seconds),
		       iterations );
            last_display_seconds = current_seconds;
        }
    } /* end while seconds to test */

    /* At this point we might still have packets outstanding. 
     * These packets will be waited for in the destroy for the thread db. 
     */
    return;
}
/**************************************
 * end fbe_test_io_thread_db_packet_test()
 **************************************/

/*!**************************************************************
 * @fn fbe_test_io_threads(fbe_test_io_thread_task_t *const task_p,
 *                         const fbe_object_id_t object_id,
 *                         const fbe_u32_t thread_count)
 ****************************************************************
 * @brief
 *  Starts testing of the I/O threads.
 *  
 * @param task_p - The test task to execute.
 * @param object_id - The object id to run to.
 * @param thread_count - The number of threads to start.
 *
 * @return  - None.  
 *
 * HISTORY:
 *  10/27/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_test_io_threads(fbe_test_io_thread_task_t *const task_p,
                         const fbe_object_id_t object_id,
                         const fbe_u32_t thread_count) 
{
    fbe_test_io_threads_db_t fbe_test_io_threads_db;

    /* Create the threads db.
     */
    fbe_test_io_threads_db_init(&fbe_test_io_threads_db, 
                                 thread_count,
                                 task_p,
                                 object_id);

    /* Execute the threads db.
     */
    fbe_test_io_threads_db_run(&fbe_test_io_threads_db);

    /* Destroy the threads db.
     */
    fbe_test_io_threads_db_destroy(&fbe_test_io_threads_db);
    return;
}
/******************************************
 * end fbe_test_io_threads()
 ******************************************/

/*****************************************
 * end fbe_logical_drive_test_io.c
 *****************************************/
