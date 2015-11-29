/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry point for the rdgen service.
 *
 * @version
 *   5/28/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_rdgen.h"
#include "fbe_database.h"
#include "fbe_rdgen_private_inlines.h"
#include "fbe_transport_memory.h"
#include "csx_ext.h"
#include "fbe_memory_private.h" /*for dps init*/

/* Declare our service methods */
fbe_status_t fbe_rdgen_service_control_entry(fbe_packet_t * packet); 
fbe_service_methods_t fbe_rdgen_service_methods = {FBE_SERVICE_ID_RDGEN, fbe_rdgen_service_control_entry};

/* globals */
static void (*fbe_rdgen_external_queue) (fbe_queue_element_t *, fbe_cpu_id_t ) = NULL;
static void (*fbe_rdgen_external_queue_location) (fbe_queue_element_t *, fbe_cpu_id_t ) = NULL;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t fbe_rdgen_service_start_io(fbe_packet_t * packet);
static fbe_status_t fbe_rdgen_service_stop_io(fbe_packet_t * packet);
static fbe_status_t fbe_rdgen_service_get_stats(fbe_packet_t * packet);
static fbe_status_t fbe_rdgen_service_reset_stats(fbe_packet_t * packet);
static fbe_status_t fbe_rdgen_service_get_object_info(fbe_packet_t * packet_p);
static fbe_status_t fbe_rdgen_service_get_request_info(fbe_packet_t * packet_p);
static fbe_status_t fbe_rdgen_service_get_thread_info(fbe_packet_t * packet_p);
static fbe_rdgen_service_t *fbe_rdgen_service_p = NULL;
static fbe_status_t fbe_rdgen_start_io_on_object_allocate(fbe_packet_t *packet_p, 
                                                          fbe_rdgen_control_start_io_t * start_io_p,
                                                          fbe_package_id_t package_id,
                                                          fbe_object_id_t object_id);
static fbe_status_t fbe_rdgen_start_class_io(fbe_packet_t * packet_p, 
                                             fbe_rdgen_control_start_io_t *start_p,
                                             fbe_package_id_t package_id,
                                             fbe_class_id_t class_id);
static void fbe_rdgen_check_destroy_object(fbe_object_id_t object_id,
                                           fbe_char_t *name_p);
static fbe_status_t fbe_rdgen_service_alloc_peer_memory(fbe_packet_t * packet_p);
static fbe_status_t fbe_rdgen_service_set_timeout(fbe_packet_t * packet_p);
static fbe_status_t fbe_rdgen_enable_system_threads(fbe_packet_t * packet_p);
static fbe_status_t fbe_rdgen_disable_system_threads(fbe_packet_t * packet_p);

static fbe_semaphore_t rdgen_start_io_semaphore;


static fbe_status_t fbe_rdgen_service_init_dps_memory(fbe_packet_t *packet_p);
static fbe_status_t fbe_rdgen_service_release_dps_memory(fbe_packet_t *packet_p);
static fbe_status_t fbe_rdgen_get_total_objects_of_rg(fbe_object_id_t object_id, 
                                                      fbe_package_id_t package_id,
                                                      fbe_u32_t *total_p);

static fbe_status_t fbe_rdgen_start_rg_io_allocate(fbe_packet_t * packet_p, 
                                                   fbe_rdgen_control_start_io_t *start_p,
                                                   fbe_package_id_t package_id,
                                                   fbe_object_id_t object_id);
static fbe_status_t fbe_rdgen_start_rg_io(fbe_packet_t * packet_p, 
                                          fbe_rdgen_control_start_io_t *start_p,
                                          fbe_package_id_t package_id,
                                          fbe_object_id_t object_id);
static fbe_status_t fbe_rdgen_start_playback_io(fbe_packet_t * packet_p, 
                                                fbe_rdgen_control_start_io_t *start_p);
static fbe_status_t fbe_rdgen_control_unlock(fbe_packet_t *packet_p);
static fbe_status_t fbe_rdgen_control_set_svc_options(fbe_packet_t * packet_p);
static fbe_status_t fbe_rdgen_destroy_unpins(void);

/*!**************************************************************
 * fbe_get_rdgen_service()
 ****************************************************************
 * @brief
 *  Returns the global rdgen service ptr.
 *
 * @param None             
 *
 * @return - The global rdgen service.  
 *
 ****************************************************************/

fbe_rdgen_service_t *fbe_get_rdgen_service(void)
{
    return fbe_rdgen_service_p;
}
/******************************************
 * end fbe_get_rdgen_service()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_init_peer_request_pool()
 ****************************************************************
 * @brief
 *  Initialize our pool of requests used to process peer messages.
 *
 * @param None.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  9/23/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_init_peer_request_pool(void)
{
    fbe_rdgen_peer_request_t *request_p = NULL;
    fbe_queue_head_t *queue_p = NULL;
    fbe_u32_t index;
    fbe_rdgen_get_peer_request_pool(&request_p);
    fbe_rdgen_get_peer_request_queue(&queue_p);

    for ( index = 0; index < FBE_RDGEN_PEER_REQUEST_POOL_COUNT; index++)
    {
        fbe_queue_push(queue_p, &request_p->pool_queue_element);
        request_p++;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_init_peer_request_pool()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_init_thread_pool()
 ****************************************************************
 * @brief
 *  Allocate the memory for our thread pool.
 *  This allocates memory for one thread per core.
 *
 * @param None.
 *
 * @return fbe_status_t - FBE_STATUS_OK - Success.
 *           FBE_STATUS_INSUFFICIENT_RESOURCES - No memory available.
 *
 * @author
 *  4/8/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_init_thread_pool(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t cores;
    void *thread_pool_p = NULL;
	
	cores = fbe_get_cpu_count();

    thread_pool_p = fbe_memory_native_allocate(sizeof(fbe_rdgen_thread_t) * cores);

    if (thread_pool_p != NULL)
    {
        fbe_rdgen_setup_thread_pool(thread_pool_p);
    }
    else
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: cannot allocate thread pool\n",__FUNCTION__);
        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    return status;
}
/******************************************
 * end fbe_rdgen_init_thread_pool()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_destroy_thread_pool()
 ****************************************************************
 * @brief
 *  Destroy the thread pool memory.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  4/8/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_destroy_thread_pool(void)
{
    fbe_rdgen_thread_t *thread_p = NULL;
    fbe_rdgen_get_io_thread(&thread_p, 0);
    fbe_memory_native_release(thread_p);

    fbe_rdgen_setup_thread_pool(NULL);
    return;
}
/******************************************
 * end fbe_rdgen_destroy_thread_pool()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_service_init()
 ****************************************************************
 * @brief
 *  Initialize the rdgen service.
 *
 * @param packet_p - The packet sent with the init request.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_service_init(fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_queue_head_t *queue_p = NULL;

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: entry\n", __FUNCTION__);

    fbe_rdgen_cmm_initialize();

    fbe_rdgen_service_p = fbe_rdgen_cmm_memory_allocate(sizeof(fbe_rdgen_service_t));
    if(fbe_rdgen_service_p == NULL) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Failed to alloc memory for rdgen service\n", __FUNCTION__); 

        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_INSUFFICIENT_RESOURCES; 
    }
    fbe_zero_memory(fbe_rdgen_service_p, sizeof(fbe_rdgen_service_t));

    status = fbe_rdgen_peer_init();
    if (status != FBE_STATUS_OK) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s error initing peer.  Status: 0x%x \n", __FUNCTION__, status); 

        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    status = fbe_rdgen_init_thread_pool();
    if (status != FBE_STATUS_OK) 
    { 
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: error initializing thread pool status: 0x%x\n", 
                                __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    fbe_base_service_set_service_id((fbe_base_service_t *) fbe_rdgen_service_p, FBE_SERVICE_ID_RDGEN);
    fbe_base_service_set_trace_level((fbe_base_service_t *) fbe_rdgen_service_p, fbe_trace_get_default_trace_level());

    fbe_base_service_init((fbe_base_service_t *) fbe_rdgen_service_p);

    fbe_rdgen_init_num_threads();
    fbe_queue_init(&fbe_rdgen_service_p->active_object_head);
    fbe_queue_init(&fbe_rdgen_service_p->active_request_head);
    fbe_rdgen_get_peer_packet_queue(&queue_p);
    fbe_queue_init(queue_p);
    fbe_rdgen_get_wait_peer_ts_queue(&queue_p);
    fbe_queue_init(queue_p);

    fbe_rdgen_get_peer_request_queue(&queue_p);
    fbe_queue_init(queue_p);
    fbe_rdgen_get_peer_request_active_queue(&queue_p);
    fbe_queue_init(queue_p);
    fbe_rdgen_service_init_flags();
    fbe_spinlock_init(&fbe_rdgen_service_p->lock);
    fbe_rdgen_memory_initialize();
    fbe_rdgen_set_seconds_per_trace(0);
    fbe_rdgen_set_last_trace_time(0);
    fbe_rdgen_set_trace_per_pass_count(0);
    fbe_rdgen_set_trace_per_io_count(1000);
    fbe_rdgen_set_seconds_per_trace(5);
    fbe_rdgen_set_max_io_msecs(FBE_RDGEN_MAX_IO_MSECS);
    fbe_rdgen_update_last_scan_time();
    fbe_rdgen_thread_init_flags(&fbe_rdgen_service_p->abort_thread);
    fbe_rdgen_thread_init_flags(&fbe_rdgen_service_p->scan_thread);
    fbe_rdgen_thread_init_flags(&fbe_rdgen_service_p->peer_thread);
    fbe_rdgen_thread_init_flags(&fbe_rdgen_service_p->object_thread);
    fbe_rdgen_set_random_seed((fbe_u32_t)fbe_get_time());
    fbe_rdgen_init_peer_request_pool();

    /* Validate that rdgen's io_stamp is the same size as the packet
     */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_rdgen_io_stamp_t) == sizeof(fbe_packet_io_stamp_t));

    /* We need to make sure the peer thread is always running.
     */
    status = fbe_rdgen_thread_init(&fbe_rdgen_service_p->peer_thread,
                                   "fbe_rdgen_peer",
                                   FBE_RDGEN_THREAD_TYPE_PEER,
                                   FBE_U32_MAX /* thread num not used since just one. */);

    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: error initializing peer thread status: 0x%x\n", 
                                __FUNCTION__, status);
    }

    status = fbe_rdgen_cmi_init();

    if (status != FBE_STATUS_OK) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: error initializing CMI status: 0x%x\n", 
                                __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

	fbe_semaphore_init(&rdgen_start_io_semaphore, 1, 1);

    /* Always complete this request with good status.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_service_init()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_service_allow_trace()
 ****************************************************************
 * @brief
 *  Determine if we are allowed to trace based on the current level.
 *
 * @param trace_level - trace level of this message.
 * @param message_id - generic identifier.
 * @param fmt... - variable argument list starting with format.
 *
 * @return None.  
 *
 * @author
 *  4/18/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_rdgen_service_allow_trace(fbe_trace_level_t trace_level)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    /* Assume we are using the default trace level.
     */
    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();

    /* The global default trace level overrides the service trace level.
     */
    if((fbe_rdgen_service_p != NULL) && fbe_base_service_is_initialized(&fbe_rdgen_service_p->base_service))
    {
        service_trace_level = fbe_base_service_get_trace_level(&fbe_rdgen_service_p->base_service);
        if (default_trace_level > service_trace_level) 
        {
            /* Global trace level overrides the current service trace level.
             */
            service_trace_level = default_trace_level;
        }
    }

    /* If the service trace level passed in is greater than the
     * current chosen trace level then just return.
     */
    if (trace_level > service_trace_level) 
    {
        return FBE_FALSE;
    }
    return FBE_TRUE;
}
/**************************************
 * end fbe_rdgen_service_allow_trace()
 **************************************/
/*!**************************************************************
 * fbe_rdgen_service_trace()
 ****************************************************************
 * @brief
 *  Trace function for use by the rdgen service.
 *  Takes into account the current global trace level and
 *  the locally set trace level.
 *
 * @param trace_level - trace level of this message.
 * @param message_id - generic identifier.
 * @param fmt... - variable argument list starting with format.
 *
 * @return None.  
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_service_trace(fbe_trace_level_t trace_level,
                         fbe_trace_message_id_t message_id,
                         const char * fmt, ...)
{
    va_list args;

    if (!fbe_rdgen_service_allow_trace(trace_level))
    {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_RDGEN,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
    return;
}
/******************************************
 * end fbe_rdgen_service_trace()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_initialize_all_threads()
 ****************************************************************
 * @brief
 *  Create all the threads for use by rdgen service.
 *
 * @param None.
 *
 * @return fbe_status_t - Status of creating threads.   
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_initialize_all_threads(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t thread_num;
    fbe_u32_t max_threads;
    fbe_rdgen_thread_t *thread_p = NULL;

    /* We kick off one thread per CPU.
     */
    
	max_threads = fbe_get_cpu_count();
    for (thread_num = 0; thread_num < max_threads; thread_num++)
    {
        fbe_rdgen_get_io_thread(&thread_p, thread_num);
        status = fbe_rdgen_thread_init(thread_p, "fbe_rdgen_io", FBE_RDGEN_THREAD_TYPE_IO, thread_num);

        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: error initializing thread number: %d status: 0x%x\n", 
                                __FUNCTION__, thread_num, status );
            break;
        }
        else
        {
            fbe_rdgen_inc_num_threads();
        }
    }
    status = fbe_rdgen_thread_init(&fbe_rdgen_service_p->abort_thread,
                                   "fbe_rdgen_abort",
                                   FBE_RDGEN_THREAD_TYPE_ABORT_IOS,
                                   FBE_U32_MAX /* thread num not used since just one. */);

    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: error initializing abort thread status: 0x%x\n", 
                                __FUNCTION__, status);
    }

    status = fbe_rdgen_thread_init(&fbe_rdgen_service_p->object_thread,
                                   "fbe_rdgen_object playback",
                                   FBE_RDGEN_THREAD_TYPE_OBJECT_PLAYBACK,
                                   FBE_U32_MAX /* thread num not used since just one. */);

    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: error initializing abort thread status: 0x%x\n", 
                                __FUNCTION__, status);
    }
/* We likely do not need this thread since we have the abort thread.
 */
#if 0
    status = fbe_rdgen_thread_init(fbe_rdgen_service_p->scan_thread,
                                   "fbe_rdgen_scan",
                                   FBE_RDGEN_THREAD_TYPE_SCAN_QUEUES,
                                   FBE_U32_MAX /* thread num not used since just one. */);

    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: error initializing scan thread status: 0x%x\n", 
                                __FUNCTION__, status);
    }
#endif
    return status;
}
/******************************************
 * end fbe_rdgen_initialize_all_threads()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_initialize_threads()
 ****************************************************************
 * @brief
 *  Decide which threads to initialize if any.
 *
 * @param None.
 *
 * @return fbe_status_t - Status of initting.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_initialize_threads(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_initializing = FBE_TRUE;
    fbe_u32_t num_threads;

    fbe_rdgen_lock();
    b_initializing = fbe_rdgen_get_threads_initializing();
    num_threads = fbe_rdgen_get_num_threads();

    /* If there are no threads, and no-one else is initializing the threads then
     * initialize them ourselves. 
     */
    if ((num_threads == 0) &&
        (b_initializing == FALSE))
    {
        /* Under protection of the lock, set b_initializing to prevent
         * anyone else from initializing the threads. 
         */
        fbe_rdgen_set_threads_initializing(FBE_TRUE);
        fbe_rdgen_unlock();

        /* Init all our threads.
         */
        status = fbe_rdgen_initialize_all_threads();

        /* Clear b_initializing.
         */
        fbe_rdgen_lock();
        fbe_rdgen_set_threads_initializing(FBE_FALSE);
        fbe_rdgen_unlock();
    }
    else if (num_threads > 0)
    {
        /* Someone else initialized the threads, all done.
         */
        fbe_rdgen_unlock();
    }
    else if (b_initializing)
    {
        fbe_rdgen_unlock();

        /* Just fail it back if we are in the middle of initializing.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/******************************************
 * end fbe_rdgen_initialize_threads()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_destroy_threads()
 ****************************************************************
 * @brief
 *  Destroy the threads rdgen uses.
 *
 * @param None.               
 *
 * @return fbe_status_t - Status of destroy.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_destroy_threads(void)
{
    fbe_u32_t num_ts;
    fbe_u32_t thread_num;
    fbe_bool_t b_initializing;
    fbe_u32_t orig_num_threads = fbe_rdgen_get_num_threads();
    fbe_status_t status = FBE_STATUS_OK;

    fbe_rdgen_lock();

    /* Make sure nothing is running.  If it is then we fail the destroy.
     */
    num_ts = fbe_rdgen_get_num_ts();
    b_initializing = fbe_rdgen_get_threads_initializing();

    if (num_ts > 0)
    {
        /* Can't destroy the threads if something is running. 
         */
        fbe_rdgen_unlock();
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: unable to destroy, %d threads running.\n", 
                            __FUNCTION__, num_ts);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else if (b_initializing)
    {
        /* Can't destroy the threads if something is already trying to initialize. 
         */
        fbe_rdgen_unlock();
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: unable to destroy. Initialize already in progress.\n", 
                            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* This prevents any new ts from being enqueued.
         */
        fbe_rdgen_set_threads_initializing(FBE_TRUE);
        fbe_rdgen_unlock();
    }

    /* Remove all the threads.
     */
    for (thread_num = 0; thread_num < orig_num_threads; thread_num++)
    {
        fbe_rdgen_thread_t *thread_p = NULL;
        fbe_rdgen_get_io_thread(&thread_p, thread_num);

        status = fbe_rdgen_thread_destroy(thread_p);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        fbe_rdgen_dec_num_threads();
    }
    /* If we had threads to destroy then the scan and abort threads were started. 
     * Try to stop these now. 
     */
    if (orig_num_threads)
    {
        status = fbe_rdgen_thread_destroy(&fbe_rdgen_service_p->scan_thread);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        status = fbe_rdgen_thread_destroy(&fbe_rdgen_service_p->abort_thread);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        status = fbe_rdgen_thread_destroy(&fbe_rdgen_service_p->object_thread);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }
    /* Clear initializing to indicate we are done.
     */
    fbe_rdgen_lock();
    fbe_rdgen_set_threads_initializing(FBE_FALSE);
    fbe_rdgen_unlock();

    return status;
}
/******************************************
 * end fbe_rdgen_destroy_threads()
 ******************************************/

/*!**************************************************************
 * rdgen_service_send_rdgen_ts_to_thread()
 ****************************************************************
 * @brief
 *  Cause an rdgen ts to get scheduled on a thread.
 * 
 *  If no threads are running, we will synchronously kick the
 *  threads off.
 *
 * @param queue_element_p - The queue element to put on the thread.
 *                          This is assumed to be an rdgen ts.
 * @param thread_number - The thread number to enqueue to.
 *
 * @return fbe_status_t - Status of the enqueue.
 *                        This can fail if we fail to init
 *                        threads.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t rdgen_service_send_rdgen_ts_to_thread(fbe_queue_element_t *queue_element_p,
                                                   fbe_u32_t thread_number)
{
    fbe_status_t status;
    fbe_rdgen_thread_t *thread_p = NULL;
    fbe_rdgen_service_t *service_p = NULL;

    /* If we are not yet initialized, threads are not created, and 
     * thus there is no way to send something to the thread. 
     * This might occur if we are being destroyed with I/Os pending. 
     */
    service_p = fbe_get_rdgen_service();
    if (!service_p->base_service.initialized)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: we are not initialized.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fbe_rdgen_get_num_threads() == 0)
    {
        /* No threads are running.  Kick them off now.
         */
        status = fbe_rdgen_initialize_threads();

        if (status != FBE_STATUS_OK)
        {
             fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: failed to initialize threads\n", __FUNCTION__);
             return status;
        }
    }

    if (RDGEN_COND(fbe_rdgen_get_num_threads() == 0))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_rdgen_get_num_threads == 0. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* If the thread number seems too big, just scale it down to the number 
     * of threads we have. 
     * We only allow the range of 0..FBE_RDGEN_MAX_THREADS-1 
     */
    if (thread_number >= fbe_rdgen_get_num_threads())
    {
        thread_number = thread_number % fbe_rdgen_get_num_threads();
    }
    /* Add it to the queue.
     */
    // is using sytem threds queue here, else do existing code
    if (fbe_rdgen_external_queue != NULL)
    {
        (*fbe_rdgen_external_queue)(queue_element_p, thread_number);
    }
    else
    {
        status = fbe_rdgen_get_io_thread(&thread_p, thread_number);

        if (status != FBE_STATUS_OK) 
        { 
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: fbe_rdgen_get_io_thread() returned %d\n", __FUNCTION__, status);
            return status; 
        }
        
        fbe_rdgen_thread_enqueue(thread_p, queue_element_p);
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end rdgen_service_send_rdgen_ts_to_thread()
 ******************************************/

/*!**************************************************************
 * rdgen_service_send_rdgen_object_to_thread()
 ****************************************************************
 * @brief
 *  Cause an rdgen object to get scheduled on a thread.
 * 
 *  If no threads are running, we will synchronously kick the
 *  threads off.
 *
 * @param queue_element_p - The queue element to put on the thread.
 *                          This is assumed to be an rdgen ts.
 *
 * @return fbe_status_t - Status of the enqueue.
 *                        This can fail if we fail to init
 *                        threads.
 *
 * @author
 *  7/30/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t rdgen_service_send_rdgen_object_to_thread(fbe_rdgen_object_t *object_p)
{
    fbe_status_t status;
    fbe_rdgen_thread_t *thread_p = NULL;
    fbe_rdgen_service_t *service_p = NULL;
    fbe_queue_element_t *queue_element_p = &object_p->thread_queue_element;

    /* If we are not yet initialized, threads are not created, and 
     * thus there is no way to send something to the thread. 
     * This might occur if we are being destroyed with I/Os pending. 
     */
    service_p = fbe_get_rdgen_service();
    if (!service_p->base_service.initialized)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: we are not initialized.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fbe_rdgen_get_num_threads() == 0)
    {

        /* No threads are running.  Kick them off now.
         */
        status = fbe_rdgen_initialize_threads();

        if (status != FBE_STATUS_OK)
        {
             fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: failed to initialize threads\n", __FUNCTION__);
             return status;
        }
    }

    if (RDGEN_COND(fbe_rdgen_get_num_threads() == 0))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_rdgen_get_num_threads == 0. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Add it to the queue.
     */
    status = fbe_rdgen_get_object_thread(&thread_p);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: get_object_thread() returned %d\n", __FUNCTION__, status);
        return status; 
    }
    
    fbe_rdgen_thread_lock(thread_p);

    if (fbe_rdgen_object_is_flag_set(object_p, FBE_RDGEN_OBJECT_FLAGS_QUEUED)){
        fbe_rdgen_thread_unlock(thread_p);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, object_p->object_id,
                                "enqueue to object thread: Already queued 0x%x\n", object_p->flags);
        /* Already enqeued, do not enqueue it again, but signal thread since
         * we might be enqueued, but waiting. 
         */
        fbe_rdgen_object_thread_signal();
        return FBE_STATUS_OK;
    }
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, object_p->object_id,
                            "enqueue to object thread fl: 0x%x\n", object_p->flags);
    fbe_rdgen_object_set_flag(object_p, FBE_RDGEN_OBJECT_FLAGS_QUEUED);
    fbe_queue_push(&thread_p->ts_queue_head, queue_element_p);
    fbe_rendezvous_event_set(&thread_p->event);
    fbe_rdgen_thread_unlock(thread_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end rdgen_service_send_rdgen_object_to_thread()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_drain_io()
 ****************************************************************
 * @brief
 *  Destroy all the rdgen objects.
 *
 * @param None.             
 *
 * @return fbe_status_t - Status of the destroy.
 *
 * @author
 *  10/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_drain_io(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t wait_msec = 0;
    #define FBE_RDGEN_DRAIN_DELAY_MSEC 500
    #define FBE_RDGEN_DRAIN_MAX_WAIT_MSEC 120000

    /* We expect the drain will normally be quick.  This is arbitrary.
     */
    #define FBE_RDGEN_DRAIN_EXPECTED_MSEC 5000

    /* Set the bit that says we are draining I/O.
     */
    fbe_rdgen_lock();
    fbe_rdgen_service_set_flag(FBE_RDGEN_SERVICE_FLAGS_DRAIN_IO);

    /* Continue to wait until we have drained ourselves of TS and drained ourselves of 
     * any peer thread requests. 
     */
    while ((fbe_rdgen_get_num_ts() > 0) ||
           !fbe_rdgen_peer_thread_is_empty())
    {
        fbe_rdgen_unlock();
        fbe_thread_delay(FBE_RDGEN_DRAIN_DELAY_MSEC);
        wait_msec += FBE_RDGEN_DRAIN_DELAY_MSEC;
        fbe_rdgen_destroy_unpins();

        if (wait_msec > FBE_RDGEN_DRAIN_MAX_WAIT_MSEC)
        {
            fbe_rdgen_lock();
            break;
        }
        fbe_rdgen_lock();
    }
    /* Unlock since we are done.
     */
    fbe_rdgen_unlock();

    if (wait_msec > FBE_RDGEN_DRAIN_MAX_WAIT_MSEC)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: drain of service failed since %d I/Os are still in progress after %d msec. \n", 
                                __FUNCTION__, fbe_rdgen_get_num_ts(), wait_msec);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else if (wait_msec > FBE_RDGEN_DRAIN_EXPECTED_MSEC)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: drain of service took %d msec.\n", __FUNCTION__, wait_msec);
    }

    return status;
}
/******************************************
 * end fbe_rdgen_drain_io()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_destroy_objects()
 ****************************************************************
 * @brief
 *  Destroy all the rdgen objects.
 *
 * @param None.             
 *
 * @return fbe_status_t - Status of the destroy.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_destroy_objects(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_rdgen_object_t *object_p = NULL;

    
    /* Unregister the destroy callback
     */
    fbe_rdgen_object_unregister_lun_destroy_notifications();

    /* Prevent things from changing while we are processing the queue.
     */
    fbe_rdgen_lock();

    /* Simply drain the queue of objects, destroying each item Until there are no more
     * items. 
     */
    while (fbe_queue_is_empty(&fbe_rdgen_service_p->active_object_head) == FBE_FALSE)
    {
        queue_element_p = fbe_queue_front(&fbe_rdgen_service_p->active_object_head);
        object_p = (fbe_rdgen_object_t *)queue_element_p;
        fbe_rdgen_dequeue_object(object_p);
        fbe_rdgen_dec_num_objects();

        /* Unlock while we process this object.
         */
        fbe_rdgen_unlock();
        fbe_rdgen_object_destroy(object_p);

        /* Re-lock so we can check the queue.
         */
        fbe_rdgen_lock();
    }

    /* Unlock since we are done.
     */
    fbe_rdgen_unlock();

    return status;
}
/******************************************
 * end fbe_rdgen_destroy_objects()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_service_destroy()
 ****************************************************************
 * @brief
 *  Destroy the rdgen service.
 *  This will fail if I/Os are getting generated.
 *
 * @param packet_p - Packet for the destroy operation.          
 *
 * @return FBE_STATUS_OK - Destroy successful.
 *         FBE_STATUS_GENERIC_FAILURE - Destroy Failed.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_service_destroy(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: entry\n", __FUNCTION__);

    fbe_rdgen_service_set_flag(FBE_RDGEN_SERVICE_FLAGS_DESTROY);

    /*! Try to drain the I/Os.
     */
    fbe_rdgen_drain_io();
    fbe_rdgen_lock();
    if (fbe_rdgen_get_num_ts() > 0)
    {
        /* We are running I/O for now we fail the destroy.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_rdgen_unlock();
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: destroy of service failed since %d I/Os are still in progress. \n", 
                                __FUNCTION__, fbe_rdgen_get_num_ts());
    }
    if (status == FBE_STATUS_OK)
    {
        fbe_rdgen_unlock();
    
        /* Destroy the objects.
         * Currently this cannot fail. 
         */
        fbe_rdgen_destroy_objects();
    
        /* Destroy the threads.
         */
        status = fbe_rdgen_destroy_threads();

        if (status == FBE_STATUS_OK)
        {
            status = fbe_rdgen_memory_destroy();
        }
    
        /* The peer thread is always initted and always needs to get destroyed.
         */
        status = fbe_rdgen_thread_destroy(&fbe_rdgen_service_p->peer_thread);
        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s error destroying peer thread status: 0x%x\n", 
                                    __FUNCTION__, status);
        }
    
        fbe_base_service_destroy((fbe_base_service_t *)fbe_rdgen_service_p);
        fbe_spinlock_destroy(&fbe_rdgen_service_p->lock);
    }
    status = fbe_rdgen_cmi_destroy();
    if (status != FBE_STATUS_OK) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s error destrying cmi.  Status: 0x%x \n", __FUNCTION__, status); 
    }

    fbe_rdgen_destroy_thread_pool();

	fbe_semaphore_destroy(&rdgen_start_io_semaphore);

	if (fbe_rdgen_service_is_flag_set(FBE_RDGEN_SERVICE_FLAGS_DPS_MEM_INITTED)) 
    {
		fbe_memory_dps_destroy();
	}
    status = fbe_rdgen_peer_destroy();
    if (status != FBE_STATUS_OK) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s error destroying peer.  Status: 0x%x \n", __FUNCTION__, status); 
    }

    fbe_rdgen_cmm_memory_release(fbe_rdgen_service_p);

    fbe_rdgen_cmm_destroy();     

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_service_destroy()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_service_control_entry()
 ****************************************************************
 * @brief
 *  This is the main entry point for the rdgen service.
 *
 * @param packet_p - Packet of the control operation.
 *
 * @return fbe_status_t.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_service_control_entry(fbe_packet_t * packet_p)
{    
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;

    control_code = fbe_transport_get_control_code(packet_p);

    /* Handle the init control code first.
     */
    if (control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) 
    {
        status = fbe_rdgen_service_init(packet_p);
        return status;
    }

    /* If we are not initialized we do not handle any other control packets.
     */
    if ((fbe_rdgen_service_p == NULL) || (!fbe_base_service_is_initialized((fbe_base_service_t *)fbe_rdgen_service_p)))
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    /* We do not process any requests when we are destroying.
     */
    fbe_rdgen_lock();
    if (fbe_rdgen_service_is_flag_set(FBE_RDGEN_SERVICE_FLAGS_DESTROY))
    {
        fbe_rdgen_unlock();
        fbe_transport_set_status(packet_p, FBE_STATUS_DEAD, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_DEAD;
    }
    fbe_rdgen_unlock();
    /* Handle the remainder of the control packets.
     */
    switch(control_code) 
    {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = fbe_rdgen_service_destroy(packet_p);
            break;
        case FBE_RDGEN_CONTROL_CODE_START_IO:
            status = fbe_rdgen_service_start_io(packet_p);
            break;
        case FBE_RDGEN_CONTROL_CODE_STOP_IO:
            status = fbe_rdgen_service_stop_io(packet_p);
            break;
        case FBE_RDGEN_CONTROL_CODE_GET_STATS:
            status = fbe_rdgen_service_get_stats(packet_p);
            break;
        case FBE_RDGEN_CONTROL_CODE_RESET_STATS:
            status = fbe_rdgen_service_reset_stats(packet_p);
            break;
        case FBE_RDGEN_CONTROL_CODE_GET_OBJECT_INFO:
            status = fbe_rdgen_service_get_object_info(packet_p);
            break;
        case FBE_RDGEN_CONTROL_CODE_GET_REQUEST_INFO:
            status = fbe_rdgen_service_get_request_info(packet_p);
            break;
        case FBE_RDGEN_CONTROL_CODE_GET_THREAD_INFO:
            status = fbe_rdgen_service_get_thread_info(packet_p);
            break;
		case FBE_RDGEN_CONTROL_CODE_TEST_CMI_PERFORMANCE:
            status = fbe_rdgen_service_test_cmi_performance(packet_p);
            break;
		case FBE_RDGEN_CONTROL_CODE_INIT_DPS_MEMORY:
			status = fbe_rdgen_service_init_dps_memory(packet_p);
			break;
		case FBE_RDGEN_CONTROL_CODE_RELEASE_DPS_MEMORY:
			status = fbe_rdgen_service_release_dps_memory(packet_p);
			break;
        case FBE_RDGEN_CONTROL_CODE_ALLOC_PEER_MEMORY:
            status = fbe_rdgen_service_alloc_peer_memory(packet_p);
            break;
        case FBE_RDGEN_CONTROL_CODE_SET_TIMEOUT:
            status = fbe_rdgen_service_set_timeout(packet_p);
            break;
            /* By default pass down to our base class to handle the 
             * operations it is aware of such as setting the trace 
             * level, etc. 
             */
        case FBE_RDGEN_CONTROL_CODE_ENABLE_SYSTEM_THREADS:
            status = fbe_rdgen_enable_system_threads(packet_p);
            break;
        case FBE_RDGEN_CONTROL_CODE_DISABLE_SYSTEM_THREADS:
            status = fbe_rdgen_disable_system_threads(packet_p);
            break;
        case FBE_RDGEN_CONTROL_CODE_UNLOCK:
            status = fbe_rdgen_control_unlock(packet_p);
            break;
        case FBE_RDGEN_CONTROL_CODE_SET_SVC_OPTIONS:
            status = fbe_rdgen_control_set_svc_options(packet_p);
            break;
        default:
            return fbe_base_service_control_entry((fbe_base_service_t *)fbe_rdgen_service_p, packet_p);
            break;
    };

    return status;
}
/******************************************
 * end fbe_rdgen_service_control_entry()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_find_object()
 ****************************************************************
 * @brief
 *  Find the given object that matches the input object id on
 *  the list of objects in the rdgen service.
 * 
 *  Assumes rdgen lock is held by the caller.
 *
 * @param object_id - Object to search for.
 * @param name_p - Name of object to search for (If using irps).
 *
 * @return fbe_rdgen_object_t * -  Ptr to matching object id
 *                                 or NULL if not found.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_object_t *fbe_rdgen_find_object(fbe_object_id_t object_id,
                                          fbe_char_t *name_p)
{
    fbe_rdgen_object_t *object_p = NULL;
    fbe_queue_head_t *head_p = NULL;
    fbe_queue_element_t *element_p = NULL;

    head_p = &fbe_rdgen_service_p->active_object_head;
    element_p = fbe_queue_front(head_p);

    while (element_p != NULL)
    {
        /* We can cast the element to the object since the 
         * element happens to be defined at the top of the object. 
         */
        object_p = (fbe_rdgen_object_t*) element_p;
        if ((object_p->object_id == object_id) &&
            ((name_p[0] == 0) || !csx_p_strncmp(&object_p->device_name[0],
                                               &name_p[0], FBE_RDGEN_DEVICE_NAME_CHARS)))
        {
            /* Found what we were looking for.
             */
            break;
        }
        element_p = fbe_queue_next(head_p, element_p);
        object_p = NULL;
    }
    return object_p;
}
/******************************************
 * end fbe_rdgen_find_object()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_get_total_objects_of_class()
 ****************************************************************
 * @brief
 *  Get the total number of objects of a given class.
 *
 * @param class_id - class id to get count for.
 * @parram total_p - The total number being returned.               
 *
 * @return fbe_status_t
 *
 * @author
 *  1/8/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_get_total_objects_of_class(fbe_class_id_t class_id, 
                                                         fbe_package_id_t package_id,
                                                         fbe_u32_t *total_p)
{
    fbe_status_t status;
    fbe_packet_t *                              packet_p = NULL;
    fbe_topology_control_get_total_objects_of_class_t get_total;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;

    /* Allocate packet.
     */
    packet_p = fbe_transport_allocate_packet();

    if (packet_p == NULL) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (package_id == FBE_PACKAGE_ID_PHYSICAL)
    {
        fbe_transport_initialize_packet(packet_p);
    }
    else
    {
        fbe_transport_initialize_sep_packet(packet_p);
    }
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);

    get_total.class_id = class_id;
    fbe_payload_control_build_operation(control_p,
                                        FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS_OF_CLASS,
                                        &get_total,
                                        sizeof(fbe_topology_control_get_total_objects_of_class_t));
    /* Set packet address.
     */
    fbe_transport_set_address(packet_p,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID);
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    fbe_service_manager_send_control_packet(packet_p);

    /* Wait for completion.
     * The packet is always completed so the status above need not be checked.
     */
    fbe_transport_wait_completion(packet_p);

    /* Save status before we release packet.
     */
    status = fbe_transport_get_status_code(packet_p);

    *total_p = get_total.total_objects;

    fbe_transport_release_packet(packet_p);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_get_total_objects_of_class()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_get_total_objects_of_rg()
 ****************************************************************
 * @brief
 *  Get the total number of objects of a given RG.
 *
 * @param object_id - object id to get count for.
 * @parram total_p - The total number being returned.               
 *
 * @return fbe_status_t
 *
 * @author
 *  3/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_get_total_objects_of_rg(fbe_object_id_t object_id, 
                                                      fbe_package_id_t package_id,
                                                      fbe_u32_t *total_p)
{
    fbe_status_t status;
    fbe_packet_t *                              packet_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_base_config_upstream_edge_count_t   upstream_object;

    upstream_object.number_of_upstream_edges = 0;

    /* Allocate packet.
     */
    packet_p = fbe_transport_allocate_packet();

    if (packet_p == NULL) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (package_id == FBE_PACKAGE_ID_PHYSICAL)
    {
        fbe_transport_initialize_packet(packet_p);
    }
    else
    {
        fbe_transport_initialize_sep_packet(packet_p);
    }
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_EDGE_COUNT,
                                        &upstream_object,
                                        sizeof(fbe_base_config_upstream_edge_count_t));
    /* Set packet address.
     */
    fbe_transport_set_address(packet_p,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id);
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    fbe_service_manager_send_control_packet(packet_p);

    /* Wait for completion.
     * The packet is always completed so the status above need not be checked.
     */
    fbe_transport_wait_completion(packet_p);

    /* Save status before we release packet.
     */
    status = fbe_transport_get_status_code(packet_p);

    *total_p = upstream_object.number_of_upstream_edges;

    fbe_transport_release_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_get_total_objects_of_rg()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_enumerate_rg_objects()
 ****************************************************************
 * @brief
 *  Enumerate all the objects of a given RG.
 *
 * @param object_id - RG to enumerate.
 * @param package_id - Package to enumerate.
 * @param enumerate_class_p - ptr to structure for enumerating
 *                            a class.               
 *
 * @return - fbe_status_t
 *
 * @author
 *  10/26/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_enumerate_rg_objects(fbe_object_id_t object_id,
                                            fbe_package_id_t package_id,
                                            fbe_base_config_upstream_object_list_t *object_list_p)
{   
    fbe_status_t status;
    fbe_packet_t * packet_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    /* Allocate packet.
     */
    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fbe_transport_allocate_packet() failed\n", __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (package_id == FBE_PACKAGE_ID_PHYSICAL)
    {
        fbe_transport_initialize_packet(packet_p);
    }
    else
    {
        fbe_transport_initialize_sep_packet(packet_p);
    }

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                        object_list_p,
                                        sizeof(fbe_base_config_upstream_object_list_t));
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Set packet address */
    fbe_transport_set_address(packet_p,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id);

    /* Our callback is always called, no need to check status here.
     */
    fbe_service_manager_send_control_packet(packet_p);
    fbe_transport_wait_completion(packet_p);

    /* Save status before we release packet.
     */
    status = fbe_transport_get_status_code(packet_p);
    fbe_transport_release_packet(packet_p);
    return status;
}
/******************************************
 * end fbe_rdgen_enumerate_rg_objects()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_enumerate_class_objects()
 ****************************************************************
 * @brief
 *  Enumerate all the objects of a given class.
 *
 * @param class_id - class to enumerate.
 * @param package_id - Package to enumerate.
 * @param enumerate_class_p - ptr to structure for enumerating
 *                            a class.               
 *
 * @return - fbe_status_t
 *
 * @author
 *  10/26/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_enumerate_class_objects(fbe_class_id_t class_id,
                                       fbe_package_id_t package_id,
                                       fbe_object_id_t *object_list_p,
                                       fbe_u32_t total_objects,
                                       fbe_u32_t *actual_num_objects_p)
{   
    fbe_status_t status;
    fbe_packet_t * packet_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_topology_control_enumerate_class_t enumerate_class;
    fbe_sg_element_t sg[FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT];
    /* Allocate packet.
     */
    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fbe_transport_allocate_packet() failed\n", __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (package_id == FBE_PACKAGE_ID_PHYSICAL)
    {
        fbe_transport_initialize_packet(packet_p);
    }
    else
    {
        fbe_transport_initialize_sep_packet(packet_p);
    }

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);
    fbe_payload_ex_set_sg_list(payload_p, &sg[0], 2);

    sg[0].address = (fbe_u8_t *)object_list_p;
    sg[0].count = total_objects * sizeof(fbe_object_id_t);
    sg[1].address = NULL;
    sg[1].count = 0;

    enumerate_class.number_of_objects = total_objects;
    enumerate_class.class_id = class_id;

    fbe_payload_control_build_operation(control_p,
                                        FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_CLASS,
                                        &enumerate_class,
                                        sizeof(fbe_topology_control_enumerate_class_t));
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Set packet address */
    fbe_transport_set_address(packet_p,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID);

    /* Our callback is always called, no need to check status here.
     */
    fbe_service_manager_send_control_packet(packet_p);
    fbe_transport_wait_completion(packet_p);

    /* Save status before we release packet.
     */
    status = fbe_transport_get_status_code(packet_p);
    fbe_transport_release_packet(packet_p);

    *actual_num_objects_p = enumerate_class.number_of_objects_copied;
    return status;
}
/******************************************
 * end fbe_rdgen_enumerate_class_objects()
 ******************************************/

/*!***************************************************************************
 *          fbe_rdgen_enumerate_system_objects()
 *****************************************************************************
 *
 * @brief   Enumerate all the system objects
 *
 * @param   class_id - class to enumerate.
 * @param   system_object_list_p - ptr to structure for enumerating a class.
 * @param   total_objects - Max objects that can be enumerated
 * @param   actual_num_objects_p - Pointer to actual number of objects in list               
 *
 * @return - fbe_status_t
 *
 * @author
 *  04/07/2010  Ron Proulx  - Created
 *
 *****************************************************************************/

fbe_status_t fbe_rdgen_enumerate_system_objects(fbe_class_id_t class_id,
                                                fbe_object_id_t *system_object_list_p,
                                                fbe_u32_t total_objects,
                                                fbe_u32_t *actual_num_objects_p)
{   
    fbe_status_t status;
    fbe_packet_t * packet_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_database_control_enumerate_system_objects_t enumerate_system_objects;
    fbe_sg_element_t sg[FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT];

    /*! @todo Currently enumerating by class id isn't supported.
     */
    if (class_id != FBE_CLASS_ID_INVALID)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: class_id: 0x%x currently not supported.\n", __FUNCTION__, class_id);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    /* Allocate packet.
     */
    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_transport_allocate_packet() failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);
    fbe_payload_ex_set_sg_list(payload_p, &sg[0], 2);

    sg[0].address = (fbe_u8_t *)system_object_list_p;
    sg[0].count = total_objects * sizeof(fbe_object_id_t);
    sg[1].address = NULL;
    sg[1].count = 0;

    enumerate_system_objects.class_id = class_id;
    enumerate_system_objects.number_of_objects = total_objects;
    enumerate_system_objects.number_of_objects_copied = 0;

    fbe_payload_control_build_operation(control_p,
                                        FBE_DATABASE_CONTROL_CODE_ENUMERATE_SYSTEM_OBJECTS,
                                        &enumerate_system_objects,
                                        sizeof(fbe_database_control_enumerate_system_objects_t));
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Set packet address (configuration service lives in SEP_0)*/
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_DATABASE,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID);

    /* Our callback is always called, no need to check status here.
     */
    fbe_service_manager_send_control_packet(packet_p);
    fbe_transport_wait_completion(packet_p);

    /* Save status before we release packet.
     */
    status = fbe_transport_get_status_code(packet_p);
    fbe_transport_release_packet(packet_p);

    *actual_num_objects_p = enumerate_system_objects.number_of_objects_copied;
    return status;
}
/********************************************************
 * end fbe_rdgen_enumerate_system_objects()
 ********************************************************/

/*!***************************************************************************
 *          fbe_rdgen_is_system_object()
 *****************************************************************************
 *
 * @brief   Check if the object id is system object by walk the list of
 *          system objects passed.
 *
 * @param   object_id_to_check - Object id to check if system object
 * @param   system_object_list_p - ptr to list of system objects
 * @param   num_system_objects - Number of system objects in list passed
 *
 * @return - fbe_status_t
 *
 * @author
 *  04/09/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_bool_t fbe_rdgen_is_system_object(fbe_object_id_t object_id_to_check,
                                      fbe_object_id_t *system_object_list_p,
                                      fbe_u32_t num_system_objects)
{
    fbe_bool_t  b_found = FBE_FALSE;
    fbe_u32_t   object_index;

    /* Simply walk the system object list and set found.
     */
    for (object_index = 0; 
         ((system_object_list_p != NULL) && (object_index < num_system_objects)); 
         object_index++)
    {
        if (object_id_to_check == system_object_list_p[object_index])
        {
            b_found = FBE_TRUE;
            break;
        }
    }

    return(b_found);
}
/********************************************************
 * end fbe_rdgen_is_system_object()
 ********************************************************/

/*!***************************************************************************
 *          fbe_rdgen_enumerate_class_exclude_system_objects()
 *****************************************************************************
 *
 * @brief   Enumerate all the objects of a given class but exclude any system
 *          objects
 *
 * @param   class_id - class to enumerate.
 * @param   package_id - Package to enumerate.
 * @param   non_system_object_list_p - ptr to structure for enumerating a class.
 * @param   max_class_objects - Max objects that can be enumerated
 * @param   actual_num_objects_p - Pointer to actual number of objects in list               
 *
 * @return - fbe_status_t
 *
 * @author
 *  04/07/2010  Ron Proulx  - Created
 *
 *****************************************************************************/

fbe_status_t fbe_rdgen_enumerate_class_exclude_system_objects(fbe_class_id_t class_id,
                                                              fbe_package_id_t package_id,
                                                              fbe_object_id_t *non_system_object_list_p,
                                                              fbe_u32_t max_class_objects,
                                                              fbe_u32_t *actual_num_objects_p)
{   
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       object_index;
    fbe_u32_t       total_class_objects = 0;
    fbe_u32_t       max_system_objects = 512 / sizeof(fbe_object_id_t);
    fbe_u32_t       total_system_objects = 0;
    fbe_u32_t       non_system_objects = 0;
    fbe_object_id_t *class_obj_list_p = NULL;
    fbe_object_id_t *system_obj_list_p = NULL;
        
    /* Allocate a list so that we can remove the system objects.  The size
     * is simply the maximum class objects allowed
     */
    class_obj_list_p = (fbe_object_id_t *)fbe_memory_native_allocate(sizeof(fbe_object_id_t) * max_class_objects);
    if (class_obj_list_p == NULL) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s: cannot allocate memory for enumerate of class id: 0x%x package: %d objs: %d\n", 
                                                  __FUNCTION__, class_id, package_id, max_class_objects);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    fbe_set_memory(class_obj_list_p, (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * max_class_objects);

    /* Now enumerate all the objects for the class specified into the list
     * that we have allocated.
     */
    status = fbe_rdgen_enumerate_class_objects(class_id, 
                                               package_id, 
                                               class_obj_list_p, 
                                               max_class_objects, 
                                               &total_class_objects);
    if (status != FBE_STATUS_OK) 
    {
        fbe_memory_native_release(class_obj_list_p);
        return(status);
    }

    /* Now enumerate all the system objects.
     */
    system_obj_list_p = (fbe_object_id_t *)fbe_memory_native_allocate(sizeof(fbe_object_id_t) * max_system_objects);
    if (system_obj_list_p == NULL) 
    {
        fbe_memory_native_release(class_obj_list_p);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s: cannot allocate memory for enumerate of class id: 0x%x package: %d objs: %d\n", 
                                                  __FUNCTION__, class_id, package_id, max_system_objects);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    fbe_set_memory(system_obj_list_p, (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * max_system_objects);
    
    /*! @todo Currently cannot enumerate system objects by class.
     */
    if (package_id == FBE_PACKAGE_ID_SEP_0)
    {
        status = fbe_rdgen_enumerate_system_objects(FBE_CLASS_ID_INVALID, 
                                                system_obj_list_p, 
                                                max_system_objects, 
                                                &total_system_objects);

        if (status != FBE_STATUS_OK)
        {
            fbe_memory_native_release(class_obj_list_p);
            fbe_memory_native_release(system_obj_list_p);
            return(status);
        }
    }

    /* Now walk thru the list of object ids for the requested class and only
     * copy the non-system objects into the passed buffer.
     */
    for (object_index = 0; object_index < total_class_objects; object_index++)
    {
        /* Invoke method that will check if the current object is a system
         * object or not.
         */
        if ((package_id == FBE_PACKAGE_ID_SEP_0)                           &&
            (fbe_rdgen_is_system_object(class_obj_list_p[object_index],
                                        system_obj_list_p,
                                        total_system_objects) == FBE_TRUE)     )
        {
            /* Don't copy this object id.
             */
        }
        else
        {
            /* Copy the object id to the list passed.
             */
            non_system_object_list_p[non_system_objects] = class_obj_list_p[object_index];
            non_system_objects++;
        }
    }

    /* Now update the actual number of objects in the list and free the 
     * memory we allocated.
     */
    *actual_num_objects_p = non_system_objects;
    fbe_memory_native_release(class_obj_list_p);
    fbe_memory_native_release(system_obj_list_p);

    /* Always return the status.
     */
    return(status);
}
/********************************************************
 * end fbe_rdgen_enumerate_class_exclude_system_objects()
 ********************************************************/

/*!**************************************************************
 * fbe_rdgen_object_start_ts()
 ****************************************************************
 * @brief
 *  Start a new rdgen ts for a request on a given object.
 *
 * @param object_p - The object to start on.
 * @param request_p - The request to start.
 * @param info_p - Memory allocation structure.
 *
 * @return fbe_status_t -
 *   FBE_STATUS_INSUFFICIENT_RESOURCES if we cannot allocate a new ts.
 *   FBE_STATUS_OK if it was started successfully.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_object_start_ts(fbe_rdgen_object_t *object_p,
                                       fbe_rdgen_request_t *request_p,
                                       fbe_rdgen_memory_info_t *info_p)
                               
{
    fbe_rdgen_ts_t     *ts_p = NULL;
    fbe_u32_t           bytes_allocated = 0;
    fbe_block_count_t   max_blocks = (fbe_block_count_t)object_p->capacity;

    /* If the user specified FBE_LBA_INVALID for max blocks, then they want to use 
     * the capacity of the obeject as the area to run over. 
     */
    if (request_p->specification.min_blocks == FBE_LBA_INVALID)
    {
        request_p->specification.min_blocks = max_blocks;
    }
    if (request_p->specification.max_blocks == FBE_LBA_INVALID)
    {
        request_p->specification.max_blocks = max_blocks;
    }

    /* Validate that the request is valid with this object.
     */
    if (((request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING)  ||
         (request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_SEQUENTIAL_DECREASING)  ||
         (request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_CATERPILLAR_INCREASING) ||
         (request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_CATERPILLAR_DECREASING)    ) &&
        (request_p->specification.block_spec == FBE_RDGEN_BLOCK_SPEC_CONSTANT)                 &&
        ((request_p->specification.min_blocks > max_blocks) ||
         (request_p->specification.max_blocks > max_blocks)    )                                   )
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: min_blocks: 0x%llx or max_blocks: 0x%llx too big for ts object_id: 0x%x\n", 
                                __FUNCTION__,
				(unsigned long long)request_p->specification.min_blocks,
				(unsigned long long)request_p->specification.max_blocks,
				object_p->object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now allocate the tracking structure.
     */
    ts_p = fbe_rdgen_memory_allocate_next_buffer(info_p, 
                                                 sizeof(fbe_rdgen_ts_t), 
                                                 &bytes_allocated,
                                                 FBE_TRUE /* Yes memory contiguous */);

    if ((ts_p == NULL) || (bytes_allocated != sizeof(fbe_rdgen_ts_t)))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: %d bytes allocated expected %llu cannot allocate mem for ts object_id: 0x%x\n", 
                                __FUNCTION__, bytes_allocated, (unsigned long long)sizeof(fbe_rdgen_ts_t), object_p->object_id);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    fbe_rdgen_inc_ts_allocated();

    /* Initialize this rdgen ts.
     */
    fbe_rdgen_ts_init(ts_p, object_p, request_p);

    if (request_p->specification.b_from_peer)
    {
        fbe_rdgen_inc_requests_from_peer();
    }

    /* Get locks in order of outer most to inner most objects.
     */
    fbe_rdgen_lock();
    fbe_rdgen_request_lock(request_p);
    fbe_rdgen_object_lock(object_p);

    /* Inc counters.
     */
    fbe_rdgen_inc_num_ts();
    fbe_rdgen_request_inc_num_threads(ts_p->request_p);
    fbe_rdgen_object_inc_num_threads(ts_p->object_p);
    fbe_rdgen_object_inc_current_threads(object_p);

    /* Enqueue.
     */
    fbe_rdgen_request_enqueue_ts(request_p, ts_p);
    fbe_rdgen_object_enqueue_ts(object_p, ts_p);

    /* We need to set this while we have the object locked so the num threads does not 
     * change. 
     */
    fbe_rdgen_ts_set_thread_num(ts_p, fbe_rdgen_object_get_num_threads(object_p));

    fbe_rdgen_ts_init_random(ts_p);
    /* Release locks in opposite order.
     */
    fbe_rdgen_object_unlock(object_p);
    fbe_rdgen_request_unlock(request_p);
    fbe_rdgen_unlock();

    /* Send this ts to a thread to execute.
     */
    fbe_rdgen_ts_enqueue_to_thread(ts_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_object_start_ts()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_object_start_io()
 ****************************************************************
 * @brief
 *  Start I/O for a newly arrived fbe_rdgen_control_start_io_t.
 *
 * @param object_p - Object to start on. 
 * @param start_p - ptr to new rdgen specification.
 * @param request_p - new request to start.
 * @param memory_info_p - Memory information structure.
 * @param threads - Number of threads to start.
 *
 * @return - fbe_status_t
 *  FBE_STATUS_INSUFFICIENT_RESOURCES - if we cannot allocate a request.
 *  FBE_STATUS_OK - If at least one request got started.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_object_start_io(fbe_rdgen_object_t *object_p,
                                       fbe_rdgen_control_start_io_t *start_p,
                                       fbe_rdgen_request_t *request_p,
                                       fbe_rdgen_memory_info_t *memory_info_p,
                                       fbe_u32_t threads)
                               
{
    fbe_u32_t thread;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* If alignment is set and less than optimal it is an error
     */
    if ((object_p->optimum_block_size > 1)                                               &&
        ((request_p->specification.alignment_blocks != 0)                           &&
         (request_p->specification.alignment_blocks < object_p->optimum_block_size)    )    )
    {
        /* Cannot allow alignment less than optimal.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: object_id: 0x%x alignment: 0x%x less than optimal: 0x%x\n",
                                __FUNCTION__, object_p->object_id,
                                request_p->specification.alignment_blocks, object_p->optimum_block_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Kick off all the threads for this request. 
     * Stop if we hit an error. 
     */
    for (thread = 0; thread < threads; thread++)
    {
        status = fbe_rdgen_object_start_ts(object_p, request_p, memory_info_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Unable to start ts on object_id: 0x%x status: 0x%x\n", 
                                    __FUNCTION__, object_p->object_id, status);
            break;
        }
    }
    /* We report success if we were able to kick off at least one thread.
     */
    if (thread > 0)
    {
        status = FBE_STATUS_OK;
    }
    return status;
}
/******************************************
 * end fbe_rdgen_object_start_io()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_object_allocate_request()
 ****************************************************************
 * @brief
 *  Alloc a request for a newly arrived fbe_rdgen_control_start_io_t.
 * 
 *  Assumes rdgen lock is held.
 * 
 * @param object_p - Object to start on. 
 * @param packet_p - packet for the new control request.
 * @param start_p - ptr to new rdgen specification.
 * @param request_pp - the output ptr to the new request.
 *
 * @return - fbe_status_t
 *  FBE_STATUS_INSUFFICIENT_RESOURCES - if we cannot allocate a request.
 *  FBE_STATUS_OK - If request got started.
 *
 * @author
 *  9/25/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_object_allocate_request(fbe_rdgen_memory_info_t *memory_info_p,
                                               fbe_packet_t * packet_p, 
                                               fbe_rdgen_control_start_io_t *start_p,
                                               fbe_rdgen_request_t **request_pp)
                               
{
    fbe_rdgen_request_t *request_p = NULL;
    fbe_u32_t bytes_allocated = 0;

    request_p = fbe_rdgen_memory_allocate_next_buffer(memory_info_p,
                                                      sizeof(fbe_rdgen_request_t), 
                                                      &bytes_allocated,
                                                      FBE_TRUE /* Yes memory contiguous */);
    if (request_p == NULL)
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    } 

    /* Initialize this rdgen request.
     */
    fbe_rdgen_request_init(request_p, packet_p, start_p);

    /* Add to the request queue.
     */
    fbe_rdgen_inc_num_requests();
    fbe_rdgen_enqueue_request(request_p);

    /* Return the object we allocated.
     */
    *request_pp = request_p;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_object_allocate_request()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_get_object()
 ****************************************************************
 * @brief
 *  Get a ptr to an object.
 *  This function will look through the global rdgen list
 *  first.  If the object is not found, it will attempt to
 *  allocate one.

 *  Assumes rdgen lock is held by the caller since we are traversing
 *  the list of all objects.
 *
 * @param object_id - id of object to find.
 * @param package_id - package this object is in.
 * @param name_p - Name is ptr to possible device string.
 * @param object_pp - ptr ptr of object to return.
 *
 * @return FBE_STATUS_OK if object was either found or created.
 *        FBE_STATUS_INSUFFICIENT_RESOURCES if object id not found
 *                                          and could not be allocated.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_get_object(fbe_object_id_t object_id,
                                         fbe_package_id_t package_id,
                                         fbe_char_t *name_p,
                                         fbe_rdgen_object_t **object_pp,
                                         fbe_rdgen_control_start_io_t *start_p)
{
    fbe_rdgen_object_t *object_p = NULL;

    if (object_id >= FBE_OBJECT_ID_INVALID)
    {
        return FBE_STATUS_NO_OBJECT;
    }

    /* Next get the ptr to the object.
     */
    object_p = fbe_rdgen_find_object(object_id, name_p);

    if (object_p == NULL)
    {
        /* Object does not exist yet, simply allocate and init it.
         */
        object_p = fbe_rdgen_allocate_memory(sizeof(fbe_rdgen_object_t));

        if (object_p == NULL)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to allocate object for object_id: 0x%x\n", __FUNCTION__, object_id);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        fbe_rdgen_inc_obj_allocated();

        fbe_rdgen_object_init(object_p, object_id, package_id, name_p, start_p);

        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: allocate object %p\n", 
                                __FUNCTION__, object_p);
        fbe_rdgen_inc_num_objects();
        fbe_rdgen_enqueue_object(object_p);
    }
    *object_pp = object_p;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_get_object()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_complete_packet()
 ****************************************************************
 * @brief
 *  Complete the control packet with a given status.
 *
 * @param packet_p - packet to complete.
 * @param control_payload_status - Status to put in control payload.
 * @parram packet_status - Status to put in packet.
 *
 * @return None.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_complete_packet(fbe_packet_t *packet_p,
                               fbe_payload_control_status_t control_payload_status,
                               fbe_status_t packet_status)
{
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_set_status(control_operation_p, control_payload_status);
    fbe_transport_set_status(packet_p, packet_status, 0);
    fbe_transport_complete_packet(packet_p);
    return;
}
/******************************************
 * end fbe_rdgen_complete_packet()
 ******************************************/

/*!***************************************************************
 * fbe_rdgen_io_completion_free_memory
 ****************************************************************
 * @brief
 *  This is the completion for freeing memory rdgen allocated
 *  for the request and all the ts.
 *
 * @param packet_p - The packet being completed.
 * @param context - The tracking structure being completed.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_MORE_PROCESSING_REQUIRED.
 *
 * @author
 *  10/18/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_rdgen_io_completion_free_memory(fbe_packet_t * packet_p, 
                                    fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_memory_request_t *memory_req_p = NULL;
//    fbe_memory_ptr_t memory_entry = NULL;
    memory_req_p = fbe_transport_get_memory_request(packet_p);
    /* Increment our free count.  We will try to match this with our allocation count.
     */
    fbe_rdgen_inc_req_freed();

    //fbe_memory_request_get_entry_mark_free(memory_req_p, &memory_entry);
    //status = fbe_memory_free_entry(memory_entry);
	status = fbe_memory_free_request_entry(memory_req_p);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: unexpected status 0x%x from fbe_memory_free_entry packet: %p\n", 
                                __FUNCTION__, status, packet_p);
        return status; 
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_rdgen_io_completion_free_memory()
 **************************************/

/*!**************************************************************
 * fbe_rdgen_start_io_on_object()
 ****************************************************************
 * @brief
 *  Try to start I/O on a given object.
 *
 * @param packet_p - packet for the new control request.
 * @param start_io_p - ptr to new rdgen specification.
 * @param object_id - Object id to start on. 
 *
 * @return -
 *  FBE_STATUS_INSUFFICIENT_RESOURCES - Not enough resources.
 *  FBE_STATUS_OK - Successful.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_rdgen_start_io_on_object(fbe_packet_t *packet_p, 
                                                 fbe_rdgen_control_start_io_t * start_io_p,
                                                 fbe_package_id_t package_id,
                                                 fbe_object_id_t object_id)
{
    fbe_status_t status;
    fbe_u32_t threads;
    fbe_memory_request_t *memory_req_p = NULL;
    fbe_rdgen_memory_info_t mem_info;
    fbe_rdgen_request_t *request_p = NULL;
    fbe_rdgen_object_t *object_p = NULL;
    fbe_u32_t num_threads = 0;

    fbe_semaphore_wait(&rdgen_start_io_semaphore, NULL);
    if (0 == fbe_rdgen_get_num_threads())
    {
        status = fbe_rdgen_initialize_threads();
        if (status != FBE_STATUS_OK)
        {
            fbe_semaphore_release(&rdgen_start_io_semaphore, 0, 1 ,FALSE);

            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: failed to initialize threads\n",
                                    __FUNCTION__);
            fbe_rdgen_complete_packet(packet_p,
                                      FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                                      status);
            return status;
        }
    }


    fbe_rdgen_lock();
    status = fbe_rdgen_get_object(object_id, package_id, &start_io_p->filter.device_name[0], &object_p, start_io_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_unlock();
		fbe_semaphore_release(&rdgen_start_io_semaphore, 0, 1 ,FALSE);
        /* Out of resources, cannot allocate object.
         */
        start_io_p->status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_OBJECT_DOES_NOT_EXIST;
        start_io_p->status.status = status;
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    memory_req_p = fbe_transport_get_memory_request(packet_p);

    status = fbe_rdgen_memory_info_init(&mem_info, memory_req_p, 0 /* Use default chunk size */);

    if (status != FBE_STATUS_OK) 
    {
        fbe_rdgen_unlock();
		fbe_semaphore_release(&rdgen_start_io_semaphore, 0, 1 ,FALSE);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: unexpected status 0x%x from rdgen memory info init \n", __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_INSUFFICIENT_RESOURCES);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    status = fbe_rdgen_object_allocate_request(&mem_info, packet_p, start_io_p, &request_p);

    /* rdgen_object_allocate_request returns a !OK status
     * when request_p is not allocated... 
     */
    if (status != FBE_STATUS_OK) 
    {
        fbe_rdgen_unlock();
		fbe_semaphore_release(&rdgen_start_io_semaphore, 0, 1 ,FALSE);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: status %d unable to allocate rdgen request for object %d\n", 
                                __FUNCTION__, status, object_id);

        /* Could not allocate the request. */
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }

    /* Bump up the number of threads. Since we are starting more than one, we bump it up
     * so that any ts that is finishing does not think we are done. 
     * This is important since if a ts finishing thinks we are finished, it can free up 
     * the request while we are still trying to start other threads. 
     */

    fbe_rdgen_request_lock(request_p);
    fbe_rdgen_object_lock(object_p);
    fbe_rdgen_request_inc_num_threads(request_p);
    num_threads = fbe_rdgen_object_inc_num_threads(object_p);
    fbe_rdgen_object_unlock(object_p);
    fbe_rdgen_request_unlock(request_p);

    /* We can release this lock now since we incremented the num threads.  This prevents 
     * the object from going away. 
     */
    fbe_rdgen_unlock();

    if (status == FBE_STATUS_OK)
    {
		if (1/*num_threads == 0*/){
            if (start_io_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_BLOCK_DEVICE) {
			    status = fbe_rdgen_object_init_bdev(object_p);
            } else {
			    status = fbe_rdgen_object_initialize_capacity(object_p);
            }
		}else {
			status = FBE_STATUS_OK;
		}

		fbe_semaphore_release(&rdgen_start_io_semaphore, 0, 1 ,FALSE);

        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_status_t req_status;
            fbe_rdgen_request_lock(request_p);
            request_p->err_count++;
            req_status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_OBJECT_DOES_NOT_EXIST;
            req_status.status = status;
            req_status.context = request_p->status.context;
            fbe_rdgen_status_set(&req_status,
                                 status,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID);
            fbe_rdgen_request_save_status(request_p, &req_status);
            fbe_rdgen_request_unlock(request_p);
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: init of capacity for object_id: 0x%x failed with status 0x%x\n", 
                                    __FUNCTION__, object_id, status);
        }
    }

    if (status == FBE_STATUS_OK)
    {
        /* Start the I/O for this command.
         * We are handing off the packet to the below function and that code and the 
         * code it invokes is responsible for "eventually" completing the packet. 
         */
        status = fbe_rdgen_object_start_io(object_p, start_io_p, request_p, &mem_info, start_io_p->specification.threads);

        if (status != FBE_STATUS_OK)
        {
            /* Since we took a failure, setup the status in the request.
             */
            fbe_rdgen_status_t req_status;
            fbe_rdgen_request_lock(request_p);
            request_p->err_count++;
            fbe_rdgen_status_set(&req_status,
                                 status,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID);
            fbe_rdgen_request_save_status(request_p, &req_status);
            fbe_rdgen_request_unlock(request_p);
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: error no threads started. Status: 0x%x", 
                                    __FUNCTION__, status);
        }
    }

    /* Decrement the number of threads. This could potentially allow the request to get
     * freed up once we release the locks below. 
     */
    fbe_rdgen_request_lock(request_p);
    fbe_rdgen_object_lock(object_p);
    fbe_rdgen_request_dec_num_threads(request_p);
    fbe_rdgen_object_dec_num_threads(object_p);
    threads = fbe_rdgen_request_get_num_threads(request_p);
    fbe_rdgen_object_unlock(object_p);
    fbe_rdgen_request_unlock(request_p);

    /* At htis point either we started I/O or not.
     */
    if (status != FBE_STATUS_OK && object_id != FBE_OBJECT_ID_INVALID)
    {
        fbe_rdgen_check_destroy_object(object_id, &start_io_p->filter.device_name[0]);
    }

    /* If we hit an error, kick the request to finish since nothing has started yet.
     */
    if (threads == 0)
    {
        fbe_rdgen_request_finished(request_p);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_start_io_on_object()
 ******************************************/

/*!***************************************************************
 * fbe_rdgen_start_io_allocation_completion
 ****************************************************************
 * @brief
 *  This is the completion for allocating the request and
 *  all ts structures.
 *
 * @param packet_p - The packet being completed.
 * @param context - The tracking structure being completed.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_MORE_PROCESSING_REQUIRED.
 *
 * @author
 *  10/18/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_rdgen_start_io_allocation_completion(fbe_packet_t * packet_p, 
                                         fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_memory_request_t *memory_req_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_rdgen_control_start_io_t * start_io_p = NULL;
    fbe_payload_ex_t  *payload_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &start_io_p);
    memory_req_p = fbe_transport_get_memory_request(packet_p);

    if (memory_req_p->ptr == NULL)
    {
        /* Memory was not allocated.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: no memory allocated \n", __FUNCTION__);
        fbe_transport_complete_packet(packet_p);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    /* Increment our allocation count.  We will try to match this with our freed count.
     */
    fbe_rdgen_inc_req_allocated();

    /* Set a completion function to free memory when the packet is done.
     */
    fbe_transport_set_completion_function(packet_p, fbe_rdgen_io_completion_free_memory, packet_p);

    if (start_io_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_CLASS)
    {
        status = fbe_rdgen_start_class_io(packet_p, start_io_p, 
                                          start_io_p->filter.package_id, start_io_p->filter.class_id);
    }
    else if (start_io_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_RAID_GROUP)
    {
        status = fbe_rdgen_start_rg_io(packet_p, start_io_p, 
                                       start_io_p->filter.package_id, start_io_p->filter.object_id);
    }
    else if (start_io_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_PLAYBACK_SEQUENCE)
    {
        status = fbe_rdgen_start_playback_io(packet_p, start_io_p);
    }
    else
    {
        status = fbe_rdgen_start_io_on_object(packet_p, start_io_p, start_io_p->filter.package_id,
                                              start_io_p->filter.object_id);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_rdgen_start_io_allocation_completion
 **************************************/
/*!**************************************************************
 * fbe_rdgen_start_io_on_object_allocate()
 ****************************************************************
 * @brief
 *  Try to start I/O on a given object.
 *
 * @param packet_p - packet for the new control request.
 * @param start_io_p - ptr to new rdgen specification.
 * @param object_id - Object id to start on. 
 *
 * @return -
 *  FBE_STATUS_INSUFFICIENT_RESOURCES - Not enough resources.
 *  FBE_STATUS_OK - Successful.
 *
 * @author
 *  10/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_start_io_on_object_allocate(fbe_packet_t *packet_p, 
                                                          fbe_rdgen_control_start_io_t * start_io_p,
                                                          fbe_package_id_t package_id,
                                                          fbe_object_id_t object_id)
{
    fbe_status_t status;
    fbe_rdgen_object_t *object_p = NULL;

    fbe_rdgen_lock();
    status = fbe_rdgen_get_object(object_id, package_id, &start_io_p->filter.device_name[0], &object_p, start_io_p);
    fbe_rdgen_unlock();

    if (status != FBE_STATUS_OK)
    {
        /* Out of resources, cannot allocate object.
         */
        start_io_p->status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_OBJECT_DOES_NOT_EXIST;
        start_io_p->status.status = status;
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
    }
    else
    {
        fbe_transport_set_completion_function(packet_p, fbe_rdgen_start_io_allocation_completion, packet_p);

        status = fbe_rdgen_allocate_request_memory(packet_p, start_io_p->specification.threads);
        if (status != FBE_STATUS_OK) 
        { 
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: unexpected status 0x%x from allocate request memory \n", 
                                    __FUNCTION__, status);
            fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
            return status; 
        }
    }
    return status;
}
/**************************************
 * end fbe_rdgen_start_io_on_object_allocate
 **************************************/

/*!**************************************************************
 * fbe_rdgen_start_class_io_allocate()
 ****************************************************************
 * @brief
 *  Start I/O on all objects of a given class.
 *
 * @param packet_p - packet for the new control request.
 * @param start_p - ptr to new rdgen specification.
 * @param class_id - Class to start I/O on.
 * @param package_id - Package to run I/O against.
 *
 * @return -
 *  FBE_STATUS_INSUFFICIENT_RESOURCES - Not enough resources.
 *  FBE_STATUS_OK - Successful.
 *
 * @author
 *  9/25/2009 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_rdgen_start_class_io_allocate(fbe_packet_t * packet_p, 
                                                      fbe_rdgen_control_start_io_t *start_p,
                                                      fbe_package_id_t package_id,
                                                      fbe_class_id_t class_id)
{
    fbe_status_t status;
    fbe_u32_t num_ts;
    fbe_u32_t total_objects = 0;

    /* Validate this request by seeing if we can enumerate this class_id.
     */
    status = fbe_rdgen_get_total_objects_of_class(class_id, package_id, &total_objects);

    if (status != FBE_STATUS_OK)
    {
        /* Failed enumerating.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s failed getting total objects: 0x%x", 
                            __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }

    if (total_objects == 0)
    {
        /* No objects found.  Indicate error.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s no objects of this class found: 0x%x", 
                                __FUNCTION__, class_id);
        start_p->status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_NO_OBJECTS_OF_CLASS;
        start_p->status.status = FBE_STATUS_NO_OBJECT;
        start_p->statistics.error_count++;
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }
    /* Total number of ts to allocate is number of threads * number of objects.
     */
    num_ts = start_p->specification.threads * total_objects;

    fbe_transport_set_completion_function(packet_p, fbe_rdgen_start_io_allocation_completion, packet_p);

    status = fbe_rdgen_allocate_request_memory(packet_p, num_ts);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: unexpected status 0x%x from allocate request memory \n", 
                                __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return status; 
    }
    return status;
}
/******************************************
 * end fbe_rdgen_start_class_io_allocate()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_start_rg_io_allocate()
 ****************************************************************
 * @brief
 *  Start I/O on all objects of a given RG.
 *
 * @param packet_p - packet for the new control request.
 * @param start_p - ptr to new rdgen specification.
 * @param package_id - Package to run I/O against.
 * @param object_id - RG object id to start I/O on.
 *
 * @return -
 *  FBE_STATUS_INSUFFICIENT_RESOURCES - Not enough resources.
 *  FBE_STATUS_OK - Successful.
 *
 * @author
 *  3/26/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_rdgen_start_rg_io_allocate(fbe_packet_t * packet_p, 
                                                   fbe_rdgen_control_start_io_t *start_p,
                                                   fbe_package_id_t package_id,
                                                   fbe_object_id_t object_id)
{
    fbe_status_t status;
    fbe_u32_t num_ts;
    fbe_u32_t total_objects = 0;

    /* Validate this request by seeing if we can enumerate this rg.
     */
    status = fbe_rdgen_get_total_objects_of_rg(object_id, package_id, &total_objects);

    if (status != FBE_STATUS_OK)
    {
        /* Failed enumerating.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s failed getting total objects: 0x%x", 
                            __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }

    if (total_objects == 0)
    {
        /* No objects found.  Indicate error.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s no objects of this rg: 0x%x", 
                                __FUNCTION__, object_id);
        start_p->status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_NO_OBJECTS_OF_CLASS;
        start_p->status.status = FBE_STATUS_NO_OBJECT;
        start_p->statistics.error_count++;
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }
    /* Total number of ts to allocate is number of threads * number of objects.
     */
    num_ts = start_p->specification.threads * total_objects;

    fbe_transport_set_completion_function(packet_p, fbe_rdgen_start_io_allocation_completion, packet_p);

    status = fbe_rdgen_allocate_request_memory(packet_p, num_ts);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: unexpected status 0x%x from allocate request memory \n", 
                                __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return status; 
    }
    return status;
}
/******************************************
 * end fbe_rdgen_start_rg_io_allocate()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_start_playback_io_allocate()
 ****************************************************************
 * @brief
 *  Start playback of a sequence of I/O.
 *
 * @param packet_p - packet for the new control request.
 * @param start_p - ptr to new rdgen specification.
 * @param package_id - Package to run I/O against.
 * @param object_id - RG object id to start I/O on.
 *
 * @return -
 *  FBE_STATUS_INSUFFICIENT_RESOURCES - Not enough resources.
 *  FBE_STATUS_OK - Successful.
 *
 * @author
 *  7/29/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_rdgen_start_playback_io_allocate(fbe_packet_t * packet_p, 
                                                         fbe_rdgen_control_start_io_t *start_p)
{
    fbe_status_t status;
    fbe_u32_t num_ts;
    fbe_u32_t targets;
    fbe_u32_t target_index;
    fbe_rdgen_root_file_filter_t *filter_list_p = NULL;

    status = fbe_rdgen_playback_get_header(&targets, &filter_list_p, /* no filter list */ &start_p->filter.device_name[0]);
    if (status != FBE_STATUS_OK) { 
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: cannot read header file status %d\n", 
                                __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return status; 
    }

    /* Sum up the total threads per target.
     */
    num_ts = 0;
    for (target_index = 0; target_index < targets; target_index++) {
        num_ts += filter_list_p[target_index].threads;
    }
    fbe_memory_native_release(filter_list_p);

    fbe_transport_set_completion_function(packet_p, fbe_rdgen_start_io_allocation_completion, packet_p);

    status = fbe_rdgen_allocate_request_memory(packet_p, num_ts);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: unexpected status 0x%x from allocate request memory \n", 
                                __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return status; 
    }
    return status;
}
/******************************************
 * end fbe_rdgen_start_playback_io_allocate()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_check_destroy_object()
 ****************************************************************
 * @brief
 *  Check if an object needs to get destroyed.
 *
 * @param object_id - Id of object.
 * @param name_p - device name (if using irps).               
 *
 * @return None.  
 *
 * @author
 *  3/22/2012 - Created. Rob Foley
 *
 ****************************************************************/

static void fbe_rdgen_check_destroy_object(fbe_object_id_t object_id,
                                           fbe_char_t *name_p)
{
    fbe_rdgen_object_t *object_p = NULL;

    fbe_rdgen_lock();
    object_p = fbe_rdgen_find_object(object_id, name_p);
    if (object_p != NULL)
    {
        /* While the lock is held, check if we should get rid of this object.
         */
        if (fbe_rdgen_object_get_num_threads(object_p) == 0)
        {
            fbe_rdgen_dequeue_object(object_p);
            fbe_rdgen_dec_num_objects();
            fbe_rdgen_unlock();
            fbe_rdgen_object_destroy(object_p);
        }
        else
        {
            fbe_rdgen_unlock();
        }
    }
    else
    {
        fbe_rdgen_unlock();
    }
}
/******************************************
 * end fbe_rdgen_check_destroy_object()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_start_class_io()
 ****************************************************************
 * @brief
 *  Start I/O on all objects of a given class.
 *
 * @param packet_p - packet for the new control request.
 * @param start_p - ptr to new rdgen specification.
 * @param class_id - Class to start I/O on.
 * @param package_id - Package to run I/O against.
 *
 * @return -
 *  FBE_STATUS_INSUFFICIENT_RESOURCES - Not enough resources.
 *  FBE_STATUS_OK - Successful.
 *
 * @author
 *  9/25/2009 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_rdgen_start_class_io(fbe_packet_t * packet_p, 
                                             fbe_rdgen_control_start_io_t *start_p,
                                             fbe_package_id_t package_id,
                                             fbe_class_id_t class_id)
{
    fbe_status_t status;
    fbe_rdgen_object_t *object_p = NULL;
    fbe_u32_t threads;
    fbe_rdgen_request_t *request_p = NULL;
    fbe_u32_t object_index;
    fbe_u32_t total_objects = 0;
    fbe_u32_t actual_num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;
    fbe_rdgen_memory_info_t mem_info;
    fbe_memory_request_t *memory_req_p = NULL;



    memory_req_p = fbe_transport_get_memory_request(packet_p);

    status = fbe_rdgen_memory_info_init(&mem_info, memory_req_p, 0 /* Use default chunk size */);

    if (status != FBE_STATUS_OK) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: unexpected status 0x%x from rdgen memory info init \n", __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_INSUFFICIENT_RESOURCES);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }


    /* Validate this request by seeing if we can enumerate this class_id.
     */
    status = fbe_rdgen_get_total_objects_of_class(class_id, package_id, &total_objects);

    if (status != FBE_STATUS_OK)
    {
        /* Failed enumerating.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s failed getting total objects: 0x%x", 
                            __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }

    if (total_objects == 0)
    {
        /* No objects found.  Indicate error.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s no objects of this class found: 0x%x", 
                                __FUNCTION__, class_id);
        start_p->status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_NO_OBJECTS_OF_CLASS;
        start_p->status.status = FBE_STATUS_NO_OBJECT;
        start_p->statistics.error_count++;
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }

    /* Since the request contains the caterpillar_lba and each object could 
     * have a different capacity, we cannot allow caterpillar I/Os to all 
     * objects of a given class.
     */
    if ((start_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_CATERPILLAR_INCREASING) ||
        (start_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_CATERPILLAR_DECREASING)    )
    {
        /* Caterpillar to class is not allowed.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Caterpillar to class: 0x%x isn't supported", 
                                __FUNCTION__, class_id);
        start_p->status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_INVALID_OPERATION;
        start_p->status.status = FBE_STATUS_ATTRIBUTE_NOT_FOUND;
        start_p->statistics.error_count++;
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }

    /* Enumerate all objects for the given class but exclude the system objects
     */
    obj_list_p = (fbe_object_id_t *)fbe_memory_native_allocate(sizeof(fbe_object_id_t) * total_objects);
    if (obj_list_p == NULL) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s: cannot allocate memory for enumerate of %d class  package: %d objs: %d\n", 
                                                  __FUNCTION__, class_id, package_id, total_objects);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_set_memory(obj_list_p, (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);

    status = fbe_rdgen_enumerate_class_exclude_system_objects(class_id, 
                                                              package_id, 
                                                              obj_list_p, 
                                                              total_objects, 
                                                              &actual_num_objects);
    if (status != FBE_STATUS_OK)
    {
        fbe_memory_native_release(obj_list_p);
        /* Failed enumerating.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s failed enumerating: 0x%x class: 0x%x package: 0x%x total_objects: 0x%x", 
                            __FUNCTION__, status, class_id, package_id, total_objects);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }

    if (actual_num_objects == 0)
    {
        fbe_memory_native_release(obj_list_p);
        /* No objects found.  Indicate error.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s no objects of this class: 0x%x package: 0x%x", 
                                __FUNCTION__, class_id, package_id);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }

    fbe_rdgen_lock();
    status = fbe_rdgen_object_allocate_request(&mem_info, packet_p, start_p, &request_p);
    fbe_rdgen_unlock();
    if (status != FBE_STATUS_OK)
    {
        fbe_memory_native_release(obj_list_p);
        /* Could not allocate the request.
         */
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }

    /* Bump up the number of threads. Since we are starting more than one, we bump it up
     * so that any ts that is finishing does not think we are done. 
     * This is important since if a ts finishing thinks we are finished, it can free up 
     * the request while we are still trying to start other threads. 
     */
    fbe_rdgen_request_lock(request_p);
    fbe_rdgen_request_inc_num_threads(request_p);
    fbe_rdgen_request_unlock(request_p);

    for (object_index = 0; object_index < actual_num_objects; object_index++)
    {
        fbe_object_id_t object_id = obj_list_p[object_index];
        /* Grab the rdgen lock since we are going to fetch a reference to an object and 
         * we need to make sure it does not go away before we can inc the # of threads. 
         */
        fbe_rdgen_lock();
        status = fbe_rdgen_get_object(object_id, package_id, &start_p->filter.device_name[0], &object_p, start_p);
        if (status != FBE_STATUS_OK)
        {
            /* Out of resources, cannot allocate object.
             */
            fbe_rdgen_unlock();
            break;
        }
        else
        {
            /* Bump up the number of threads on this object so that it will 
             * stick around through us starting multiple requests. 
             */
            fbe_rdgen_object_lock(object_p);
            fbe_rdgen_object_inc_num_threads(object_p);
            fbe_rdgen_object_unlock(object_p);

            /* We can unlock since we inc'd num threads, which prevents the object from going away.
             */
            fbe_rdgen_unlock();

            status = fbe_rdgen_object_initialize_capacity(object_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_rdgen_object_lock(object_p);
                fbe_rdgen_object_dec_num_threads(object_p);
                fbe_rdgen_object_unlock(object_p);
                start_p->status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_OBJECT_DOES_NOT_EXIST;
                start_p->status.status = status;

                fbe_rdgen_check_destroy_object(object_id, &start_p->filter.device_name[0]);
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: init of capacity for object_id: 0x%x failed with status 0x%x\n", 
                                        __FUNCTION__, object_id, status);
                break;
            }

            if (status == FBE_STATUS_OK) 
            {
                /* Start the I/O for this command.
                 * We are handing off the packet to the below function and that code and the 
                 * code it invokes is responsible for "eventually" completing the packet. 
                 */
                status = fbe_rdgen_object_start_io(object_p, start_p, request_p, &mem_info, start_p->specification.threads);

                fbe_rdgen_object_lock(object_p);
                fbe_rdgen_object_dec_num_threads(object_p);
                fbe_rdgen_object_unlock(object_p);

                if (status != FBE_STATUS_OK)
                {
                    /* Since we took a failure, setup the status in the request.
                     */
                    fbe_rdgen_status_t req_status;
                    fbe_rdgen_request_lock(request_p);
                    request_p->err_count++;
                    fbe_rdgen_status_set(&req_status,
                                         status,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID);
                    fbe_rdgen_request_save_status(request_p, &req_status);
                    fbe_rdgen_request_unlock(request_p);

                    fbe_rdgen_check_destroy_object(object_id, &start_p->filter.device_name[0]);
                    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s: error no threads started. Status: 0x%x", 
                                            __FUNCTION__, status);
                    break;
                }

            }
        }
    
    } /* end for all objects to start on */
    fbe_memory_native_release(obj_list_p);

    /* Decrement the number of threads. This could potentially allow the request to get
     * freed up once we release the locks below. 
     */
    fbe_rdgen_request_lock(request_p);
    fbe_rdgen_request_dec_num_threads(request_p);
    threads = fbe_rdgen_request_get_num_threads(request_p);
    fbe_rdgen_request_unlock(request_p);

    /* If we hit an error, kick the request to finish since nothing has started yet.
     */
    if (threads == 0)
    {
        fbe_rdgen_request_finished(request_p);
    }
    return status;
}
/******************************************
 * end fbe_rdgen_start_class_io()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_start_rg_io()
 ****************************************************************
 * @brief
 *  Start I/O on all objects of a given class.
 *
 * @param packet_p - packet for the new control request.
 * @param start_p - ptr to new rdgen specification.
 * @param object_id - Class to start I/O on.
 * @param package_id - Package to run I/O against.
 *
 * @return -
 *  FBE_STATUS_INSUFFICIENT_RESOURCES - Not enough resources.
 *  FBE_STATUS_OK - Successful.
 *
 * @author
 *  3/26/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_rdgen_start_rg_io(fbe_packet_t * packet_p, 
                                          fbe_rdgen_control_start_io_t *start_p,
                                          fbe_package_id_t package_id,
                                          fbe_object_id_t object_id)
{
    fbe_status_t status;
    fbe_rdgen_object_t *object_p = NULL;
    fbe_u32_t threads;
    fbe_rdgen_request_t *request_p = NULL;
    fbe_u32_t object_index;
    fbe_u32_t total_objects = 0;
    fbe_u32_t actual_num_objects = 0;
    fbe_base_config_upstream_object_list_t *obj_list_p = NULL;
    fbe_rdgen_memory_info_t mem_info;
    fbe_memory_request_t *memory_req_p = NULL;

    memory_req_p = fbe_transport_get_memory_request(packet_p);

    status = fbe_rdgen_memory_info_init(&mem_info, memory_req_p, 0 /* Use default chunk size */);

    if (status != FBE_STATUS_OK) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: unexpected status 0x%x from rdgen memory info init \n", __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_INSUFFICIENT_RESOURCES);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }


    /* Validate this request by seeing if we can enumerate this object_id.
     */
    status = fbe_rdgen_get_total_objects_of_rg(object_id, package_id, &total_objects);

    if (status != FBE_STATUS_OK)
    {
        /* Failed enumerating.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s failed getting total objects: 0x%x", 
                            __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }

    if (total_objects == 0)
    {
        /* No objects found.  Indicate error.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s no objects of this rg found: 0x%x", 
                                __FUNCTION__, object_id);
        start_p->status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_NO_OBJECTS_OF_CLASS;
        start_p->status.status = FBE_STATUS_NO_OBJECT;
        start_p->statistics.error_count++;
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }

    /* Since the request contains the caterpillar_lba and each object could 
     * have a different capacity, we cannot allow caterpillar I/Os to all 
     * objects of a given class.
     */
    if ((start_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_CATERPILLAR_INCREASING) ||
        (start_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_CATERPILLAR_DECREASING)    )
    {
        /* Caterpillar to class is not allowed.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Caterpillar to rg: 0x%x isn't supported", 
                                __FUNCTION__, object_id);
        start_p->status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_INVALID_OPERATION;
        start_p->status.status = FBE_STATUS_ATTRIBUTE_NOT_FOUND;
        start_p->statistics.error_count++;
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }

    /* Enumerate all objects for the given class but exclude the system objects
     */
    obj_list_p = (fbe_base_config_upstream_object_list_t *)fbe_memory_native_allocate(sizeof(fbe_base_config_upstream_object_list_t));
    if (obj_list_p == NULL) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s: cannot allocate memory for enumerate of %d rg  package: %d objs: %d\n", 
                                                  __FUNCTION__, object_id, package_id, total_objects);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    status = fbe_rdgen_enumerate_rg_objects(object_id, package_id, obj_list_p);
    actual_num_objects = obj_list_p->number_of_upstream_objects;
    if (status != FBE_STATUS_OK)
    {
        fbe_memory_native_release(obj_list_p);
        /* Failed enumerating.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s failed enumerating: 0x%x rg: 0x%x package: 0x%x total_objects: 0x%x", 
                            __FUNCTION__, status, object_id, package_id, total_objects);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }

    if (actual_num_objects == 0)
    {
        fbe_memory_native_release(obj_list_p);
        /* No objects found.  Indicate error.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s no objects of this rg: 0x%x package: 0x%x", 
                                __FUNCTION__, object_id, package_id);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }

    fbe_rdgen_lock();
    status = fbe_rdgen_object_allocate_request(&mem_info, packet_p, start_p, &request_p);
    fbe_rdgen_unlock();
    if (status != FBE_STATUS_OK)
    {
        fbe_memory_native_release(obj_list_p);
        /* Could not allocate the request.
         */
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }

    /* Bump up the number of threads. Since we are starting more than one, we bump it up
     * so that any ts that is finishing does not think we are done. 
     * This is important since if a ts finishing thinks we are finished, it can free up 
     * the request while we are still trying to start other threads. 
     */
    fbe_rdgen_request_lock(request_p);
    fbe_rdgen_request_inc_num_threads(request_p);
    fbe_rdgen_request_unlock(request_p);

    for (object_index = 0; object_index < actual_num_objects; object_index++)
    {
        fbe_object_id_t object_id = obj_list_p->upstream_object_list[object_index];
        /* Grab the rdgen lock since we are going to fetch a reference to an object and 
         * we need to make sure it does not go away before we can inc the # of threads. 
         */
        fbe_rdgen_lock();
        status = fbe_rdgen_get_object(object_id, package_id, &start_p->filter.device_name[0], &object_p,
                                      start_p);
        if (status != FBE_STATUS_OK)
        {
            /* Out of resources, cannot allocate object.
             */
            fbe_rdgen_unlock();
            break;
        }
        else
        {
            /* Bump up the number of threads on this object so that it will 
             * stick around through us starting multiple requests. 
             */
            fbe_rdgen_object_lock(object_p);
            fbe_rdgen_object_inc_num_threads(object_p);
            fbe_rdgen_object_unlock(object_p);

            /* We can unlock since we inc'd num threads, which prevents the object from going away.
             */
            fbe_rdgen_unlock();

            status = fbe_rdgen_object_initialize_capacity(object_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_rdgen_object_lock(object_p);
                fbe_rdgen_object_dec_num_threads(object_p);
                fbe_rdgen_object_unlock(object_p);
                start_p->status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_OBJECT_DOES_NOT_EXIST;
                start_p->status.status = status;

                fbe_rdgen_check_destroy_object(object_id, &start_p->filter.device_name[0]);
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: init of capacity for object_id: 0x%x failed with status 0x%x\n", 
                                        __FUNCTION__, object_id, status);
                break;
            }

            if (status == FBE_STATUS_OK) 
            {
                /* Start the I/O for this command.
                 * We are handing off the packet to the below function and that code and the 
                 * code it invokes is responsible for "eventually" completing the packet. 
                 */
                status = fbe_rdgen_object_start_io(object_p, start_p, request_p, &mem_info, start_p->specification.threads);

                fbe_rdgen_object_lock(object_p);
                fbe_rdgen_object_dec_num_threads(object_p);
                fbe_rdgen_object_unlock(object_p);

                if (status != FBE_STATUS_OK)
                {
                    /* Since we took a failure, setup the status in the request.
                     */
                    fbe_rdgen_status_t req_status;
                    fbe_rdgen_request_lock(request_p);
                    request_p->err_count++;
                    fbe_rdgen_status_set(&req_status,
                                         status,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID);
                    fbe_rdgen_request_save_status(request_p, &req_status);
                    fbe_rdgen_request_unlock(request_p);

                    fbe_rdgen_check_destroy_object(object_id, &start_p->filter.device_name[0]);
                    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s: error no threads started. Status: 0x%x", 
                                            __FUNCTION__, status);
                    break;
                }

            }
        }
    
    } /* end for all objects to start on */
    fbe_memory_native_release(obj_list_p);

    /* Decrement the number of threads. This could potentially allow the request to get
     * freed up once we release the locks below. 
     */
    fbe_rdgen_request_lock(request_p);
    fbe_rdgen_request_dec_num_threads(request_p);
    threads = fbe_rdgen_request_get_num_threads(request_p);
    fbe_rdgen_request_unlock(request_p);

    /* If we hit an error, kick the request to finish since nothing has started yet.
     */
    if (threads == 0)
    {
        fbe_rdgen_request_finished(request_p);
    }
    return status;
}
/******************************************
 * end fbe_rdgen_start_rg_io()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_start_playback_io()
 ****************************************************************
 * @brief
 *  Start I/O for a sequence of I/O.
 *
 * @param packet_p - packet for the new control request.
 * @param start_p - ptr to new rdgen specification.
 *
 * @return -
 *  FBE_STATUS_INSUFFICIENT_RESOURCES - Not enough resources.
 *  FBE_STATUS_OK - Successful.
 *
 * @author
 *  7/29/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_start_playback_io(fbe_packet_t * packet_p, 
                                                fbe_rdgen_control_start_io_t *start_p)
{
    fbe_status_t status;
    fbe_rdgen_object_t *object_p = NULL;
    fbe_u32_t threads;
    fbe_rdgen_request_t *request_p = NULL;
    fbe_u32_t object_index;
    fbe_rdgen_memory_info_t mem_info;
    fbe_memory_request_t *memory_req_p = NULL;
    fbe_rdgen_root_file_filter_t *filter_list_p = NULL;
    fbe_u32_t targets;

    memory_req_p = fbe_transport_get_memory_request(packet_p);

    status = fbe_rdgen_memory_info_init(&mem_info, memory_req_p, 0 /* Use default chunk size */);

    if (status != FBE_STATUS_OK) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: unexpected status 0x%x from rdgen memory info init \n", __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_INSUFFICIENT_RESOURCES);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_rdgen_lock();
    status = fbe_rdgen_object_allocate_request(&mem_info, packet_p, start_p, &request_p);
    fbe_rdgen_unlock();
    if (status != FBE_STATUS_OK)
    {
        /* Could not allocate the request.
         */
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
        return FBE_STATUS_OK;
    }
    status = fbe_rdgen_playback_get_header(&targets, &filter_list_p, &start_p->filter.device_name[0]);
    if (status != FBE_STATUS_OK) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: cannot read header file status %d\n", __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_INSUFFICIENT_RESOURCES);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    /* Bump up the number of threads. Since we are starting more than one, we bump it up
     * so that any ts that is finishing does not think we are done. 
     * This is important since if a ts finishing thinks we are finished, it can free up 
     * the request while we are still trying to start other threads. 
     */
    fbe_rdgen_request_lock(request_p);
    fbe_rdgen_request_inc_num_threads(request_p);
    fbe_rdgen_request_unlock(request_p);

    for (object_index = 0; object_index < targets; object_index++)
    {
        fbe_object_id_t object_id = filter_list_p[object_index].object_id;
        /* Grab the rdgen lock since we are going to fetch a reference to an object and 
         * we need to make sure it does not go away before we can inc the # of threads. 
         */
        fbe_rdgen_lock();
        status = fbe_rdgen_get_object(object_id, filter_list_p[object_index].package_id, 
                                      &filter_list_p[object_index].device_name[0], &object_p, start_p);
        if (status != FBE_STATUS_OK)
        {
            /* Out of resources, cannot allocate object.
             */
            fbe_rdgen_unlock();
            break;
        }
        else
        {
            /* We do not expect more than one playback per object.
             */
            fbe_rdgen_object_lock(object_p);
            if (object_p->active_ts_count > 0){
                fbe_rdgen_object_unlock(object_p);
                fbe_rdgen_unlock();
                
                start_p->status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_INVALID_OPERATION;
                start_p->status.status = status;

                fbe_rdgen_check_destroy_object(object_id, &filter_list_p[object_index].device_name[0]);
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: playback already in progress for object 0x%x, cannot start another.\n", 
                                        __FUNCTION__, object_id);
                break;
            }

            /* Bump up the number of threads on this object so that it will 
             * stick around through us starting multiple requests. 
             */
            fbe_rdgen_object_inc_num_threads(object_p);
            fbe_rdgen_object_unlock(object_p);

            /* We can unlock since we inc'd num threads, which prevents the object from going away.
             */
            fbe_rdgen_unlock();

            status = fbe_rdgen_object_initialize_capacity(object_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_rdgen_object_lock(object_p);
                fbe_rdgen_object_dec_num_threads(object_p);
                fbe_rdgen_object_unlock(object_p);
                start_p->status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_OBJECT_DOES_NOT_EXIST;
                start_p->status.status = status;

                fbe_rdgen_check_destroy_object(object_id, &filter_list_p[object_index].device_name[0]);
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: init of capacity for object_id: 0x%x failed with status 0x%x\n", 
                                        __FUNCTION__, object_id, status);
                break;
            }
            fbe_rdgen_object_set_file(object_p, &start_p->filter.device_name[0]);
            if (status == FBE_STATUS_OK) 
            {
                /* Start the I/O for this command.
                 * We are handing off the packet to the below function and that code and the 
                 * code it invokes is responsible for "eventually" completing the packet. 
                 */
                fbe_rdgen_object_set_max_threads(object_p, filter_list_p[object_index].threads);
                status = fbe_rdgen_object_start_io(object_p, start_p, request_p, &mem_info, 
                                                   filter_list_p[object_index].threads);

                fbe_rdgen_object_lock(object_p);
                fbe_rdgen_object_dec_num_threads(object_p);
                fbe_rdgen_object_unlock(object_p);

                if (status != FBE_STATUS_OK)
                {
                    /* Since we took a failure, setup the status in the request.
                     */
                    fbe_rdgen_status_t req_status;
                    fbe_rdgen_request_lock(request_p);
                    request_p->err_count++;
                    req_status.context = request_p->status.context;
                    fbe_rdgen_status_set(&req_status,
                                         status,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID);
                    fbe_rdgen_request_save_status(request_p, &req_status);
                    fbe_rdgen_request_unlock(request_p);

                    fbe_rdgen_check_destroy_object(object_id, &filter_list_p[object_index].device_name[0]);
                    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s: error no threads started. Status: 0x%x", 
                                            __FUNCTION__, status);
                    break;
                }

            }
        }
    
    } /* end for all objects to start on */

    /* Decrement the number of threads. This could potentially allow the request to get
     * freed up once we release the locks below. 
     */
    fbe_rdgen_request_lock(request_p);
    fbe_rdgen_request_dec_num_threads(request_p);
    threads = fbe_rdgen_request_get_num_threads(request_p);
    fbe_rdgen_request_unlock(request_p);

    /* If we hit an error, kick the request to finish since nothing has started yet.
     */
    if (threads == 0)
    {
        fbe_rdgen_request_finished(request_p);
    }
    fbe_memory_native_release(filter_list_p);
    return status;
}
/******************************************
 * end fbe_rdgen_start_playback_io()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_validate_start_request()
 ****************************************************************
 * @brief
 *  Validate the new start request.
 *
 * @param start_io_p - ptr to new rdgen specification.
 *
 * @return FBE_STATUS_OK - Success.
 *        FBE_STATUS_GENERIC_FAILURE - If not a valid spec.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_validate_start_request(fbe_rdgen_control_start_io_t * start_io_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t lba_range;

    /* Make sure the operation is defined.
     */
    if ((start_io_p->specification.operation == FBE_RDGEN_OPERATION_INVALID) ||
        (start_io_p->specification.operation >= FBE_RDGEN_OPERATION_LAST))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "rdgn_val_start_req invalid op: 0x%x\n", 
                            start_io_p->specification.operation);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Make sure the thread count is valid.
     */
    if ((start_io_p->specification.threads == 0) ||
        (start_io_p->specification.threads > FBE_RDGEN_MAX_THREADS_PER_IO_SPECIFICATION))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "rdgn_val_start_req invalid thd count in specification: 0x%x\n", 
                            start_io_p->specification.threads);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Make sure the block_spec is defined.
     */
    if ((start_io_p->specification.block_spec == FBE_RDGEN_BLOCK_SPEC_INVALID) ||
        (start_io_p->specification.block_spec >= FBE_RDGEN_BLOCK_SPEC_LAST))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "rdgn_val_start_req inv blk_spec: 0x%x\n", 
                            start_io_p->specification.block_spec);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (start_io_p->specification.io_interface != FBE_RDGEN_INTERFACE_TYPE_PACKET)
    {
        if ((start_io_p->specification.operation == FBE_RDGEN_OPERATION_ZERO_ONLY) ||
            (start_io_p->specification.operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK))
        {
            /* We support any interface for this operation.
             * Any interface other than packet assumes a device interface. 
             */
            if ((start_io_p->specification.io_interface == FBE_RDGEN_INTERFACE_TYPE_IRP_DCA) &&
                (start_io_p->specification.operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK))
            {
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "rdgn_val_start_req dca irp interface this opcode %d not supported\n", 
                                        start_io_p->specification.operation);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else if (start_io_p->specification.io_interface == FBE_RDGEN_INTERFACE_TYPE_IRP_SGL)
        {
            if ((start_io_p->specification.operation != FBE_RDGEN_OPERATION_READ_ONLY) &&
                (start_io_p->specification.operation != FBE_RDGEN_OPERATION_WRITE_ONLY) &&
                (start_io_p->specification.operation != FBE_RDGEN_OPERATION_WRITE_READ_CHECK) &&
                (start_io_p->specification.operation != FBE_RDGEN_OPERATION_READ_CHECK))
            {
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "rdgn_val_start_req sgl irp interface this opcode %d not supported\n", 
                                        start_io_p->specification.operation);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else if (start_io_p->specification.io_interface == FBE_RDGEN_INTERFACE_TYPE_IRP_MJ)
        {
            if ((start_io_p->specification.operation != FBE_RDGEN_OPERATION_READ_ONLY) &&
                (start_io_p->specification.operation != FBE_RDGEN_OPERATION_WRITE_ONLY) &&
                (start_io_p->specification.operation != FBE_RDGEN_OPERATION_WRITE_READ_CHECK) &&
                (start_io_p->specification.operation != FBE_RDGEN_OPERATION_READ_CHECK))
            {
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "rdgn_val_start_req irp MJ interface this opcode %d not supported\n", 
                                        start_io_p->specification.operation);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else if ((start_io_p->specification.operation != FBE_RDGEN_OPERATION_READ_ONLY) &&
                 (start_io_p->specification.operation != FBE_RDGEN_OPERATION_WRITE_ONLY))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "rdgn_val_start_req irp interface, only read or write only supported\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Make sure the lba_spec is defined.
     */
    if ((start_io_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_INVALID) ||
        (start_io_p->specification.lba_spec >= FBE_RDGEN_LBA_SPEC_LAST))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "rdgn_val_start_req invalid lba_spec: 0x%x\n", 
                            start_io_p->specification.lba_spec);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Make sure the pattern is defined.
     */
    if ((start_io_p->specification.pattern == FBE_RDGEN_PATTERN_INVALID) ||
        (start_io_p->specification.pattern >= FBE_RDGEN_PATTERN_LAST))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "rdgn_val_start_req inv pattern: 0x%x\n", 
                             start_io_p->specification.pattern);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The min or max blocks should not be zero.
     */
    if ((start_io_p->specification.min_blocks == 0) ||
        (start_io_p->specification.max_blocks == 0))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "rdgn_val_start_req zero blks: min: 0x%llx max: 0x%llx\n", 
                            (unsigned long long)start_io_p->specification.min_blocks,
                            (unsigned long long)start_io_p->specification.max_blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The min blocks should be less than or equal to max.
     */
    if (start_io_p->specification.min_blocks > start_io_p->specification.max_blocks)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "rdgn_val_start_req inv blks min: 0x%llx max: 0x%llx\n", 
                            (unsigned long long)start_io_p->specification.min_blocks,
                            (unsigned long long)start_io_p->specification.max_blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The start min blocks should be less than or equal to max.
     */
    if (start_io_p->specification.start_lba > start_io_p->specification.max_lba)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "rdgn_val_start_req start lba > max, start: 0x%llx max: 0x%llx\n", 
                            (unsigned long long)start_io_p->specification.start_lba,
                            (unsigned long long)start_io_p->specification.max_lba);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (start_io_p->specification.max_lba != FBE_LBA_INVALID)
    {
        /* Make sure the start lba range is greater than or equal to the blocks.
         */
        lba_range = start_io_p->specification.max_lba - start_io_p->specification.start_lba + 1;

        if (start_io_p->specification.block_spec != FBE_RDGEN_BLOCK_SPEC_INCREASING)
        {
            if ((lba_range < start_io_p->specification.max_blocks) ||
                (lba_range < start_io_p->specification.min_blocks))
            {
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "rdgn_val_start_req inv start range: 0x%llx minl: 0x%llx "
                                    "maxl: 0x%llx minb: 0x%llx maxb: 0x%llx\n", 
                                    (unsigned long long)lba_range, 
                                    (unsigned long long)start_io_p->specification.min_lba,
                                    (unsigned long long)start_io_p->specification.max_lba,
                                    (unsigned long long)start_io_p->specification.min_blocks,
                                    (unsigned long long)start_io_p->specification.max_blocks);
                return FBE_STATUS_GENERIC_FAILURE;
            }
    
            lba_range = start_io_p->specification.max_lba - start_io_p->specification.min_lba + 1;
    
            /* Make sure the lba range is greater than or equal to the max block count
             * we will be testing with. 
             */
            if ((lba_range < start_io_p->specification.max_blocks) ||
                (lba_range < start_io_p->specification.min_blocks))
            {
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "rdgn_val_start_req inv blks range: 0x%llx minl: 0x%llx "
                                    "maxl: 0x%llx minb: 0x%llx maxb: 0x%llx\n", 
                                    (unsigned long long)lba_range, 
                                    (unsigned long long)start_io_p->specification.min_lba,
                                    (unsigned long long)start_io_p->specification.max_lba,
                                    (unsigned long long)start_io_p->specification.min_blocks,
                                    (unsigned long long)start_io_p->specification.max_blocks);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Make sure it is possible to start this number of threads on the given 
             * lba range with the given minimum block counts                                    .
             * This prevents us from starting threads that cannot compete in the given I/O range. 
             */
            if ((lba_range / start_io_p->specification.min_blocks) < start_io_p->specification.threads)
            {
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "rdgn_val_start_req inv thds lba range: 0x%llx "
                                        "min_b: 0x%llx max_b: 0x%llx thds: 0x%x\n", 
                                        (unsigned long long)lba_range, 
                                        (unsigned long long)start_io_p->specification.min_blocks,
                                        (unsigned long long)start_io_p->specification.max_blocks,
                                        start_io_p->specification.threads);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }

    /* The min lba should be less than or equal to max.
     */
    if ((start_io_p->specification.min_lba > start_io_p->specification.max_lba))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "rdgn_val_start_req inv lba min: 0x%llx max: 0x%llx\n", 
                            (unsigned long long)start_io_p->specification.min_lba,
                            (unsigned long long)start_io_p->specification.max_lba);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/******************************************
 * end fbe_rdgen_validate_start_request()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_service_start_threads()
 ****************************************************************
 * @brief
 *  Check if we need to launch threads.
 *
 * @param None
 *
 * @return fbe_status_t status of starting.
 *
 * @author
 *  5/21/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_rdgen_service_start_threads(void)
{
    fbe_status_t status;
    fbe_rdgen_service_t *service_p = NULL;

    /* If we are not yet initialized, threads are not created, and 
     * thus there is no way to send something to the thread. 
     * This might occur if we are being destroyed with I/Os pending. 
     */
    service_p = fbe_get_rdgen_service();
    if (!service_p->base_service.initialized) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: we are not initialized.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fbe_rdgen_get_num_threads() == 0) {

        fbe_semaphore_wait(&rdgen_start_io_semaphore, NULL);
        /* No threads are running.  Kick them off now.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Try to initialize threads.\n", __FUNCTION__);
        status = fbe_rdgen_initialize_threads();

		fbe_semaphore_release(&rdgen_start_io_semaphore, 0, 1, FALSE);
        if (status != FBE_STATUS_OK)
        {
             fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: failed to initialize threads\n", __FUNCTION__);
             return status;
        }
    }

    if (RDGEN_COND(fbe_rdgen_get_num_threads() == 0)) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_rdgen_get_num_threads == 0. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_rdgen_service_start_threads()
 **************************************/
/*!**************************************************************
 * fbe_rdgen_service_start_io()
 ****************************************************************
 * @brief
 *  Take a new control packet and attempt to start the I/O.
 *
 * @param - packet_p - Packet to start.
 *
 * @return fbe_status_t status of starting.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_service_start_io(fbe_packet_t * packet_p)
{
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_rdgen_control_start_io_t * start_io_p = NULL;
    fbe_u32_t len;
    fbe_status_t status;
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_u32_t num_requests;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &start_io_p);
    if (start_io_p == NULL)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_rdgen_control_start_io_t))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu \n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_rdgen_control_start_io_t));

        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!fbe_rdgen_service_is_flag_set(FBE_RDGEN_SERVICE_FLAGS_DPS_MEM_INITTED))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s rdgen memory is not initialized yet. Rdgen memory must be initted before Rdgen I/O can be started.\n", 
                                __FUNCTION__);
        
        start_io_p->status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_MEM_UNINITIALIZED;
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Check to see if the request is valid.
     */
    status = fbe_rdgen_validate_start_request(start_io_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s start request invalid \n", __FUNCTION__);
        start_io_p->status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_INVALID_OPERATION;
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }
    /* We want to limit the number of requests that can be started. 
     * Otherwise we will run out of memory and be unable to stop threads. 
     */
    num_requests = fbe_rdgen_get_num_requests();
    if (num_requests > FBE_RDGEN_MAX_REQUESTS)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s cannot start new thread current %d max is: %d. Stop some threads and try again.\n", 
                                __FUNCTION__, num_requests, FBE_RDGEN_MAX_REQUESTS);

        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }
    /* If we are here, check if threads need to get started.
     */
    status = fbe_rdgen_service_start_threads();

    if (status != FBE_STATUS_OK) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Threads are not initialized status: 0x%x\n", 
                                __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    if (start_io_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_CLASS)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: start class io class_id: 0x%x\n", __FUNCTION__, start_io_p->filter.class_id);
        status = fbe_rdgen_start_class_io_allocate(packet_p, start_io_p, 
                                                   start_io_p->filter.package_id, start_io_p->filter.class_id);
    }
    else if (start_io_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_RAID_GROUP)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: start rg io object: 0x%x\n", __FUNCTION__, start_io_p->filter.object_id);
        status = fbe_rdgen_start_rg_io_allocate(packet_p, start_io_p, 
                                                start_io_p->filter.package_id, start_io_p->filter.object_id);
    }
    else if (start_io_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_PLAYBACK_SEQUENCE)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: start playback\n", __FUNCTION__);
        status = fbe_rdgen_start_playback_io_allocate(packet_p, start_io_p);
    }
    else
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW,  FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: start object io object_id: 0x%x\n", __FUNCTION__, start_io_p->filter.object_id);
        status = fbe_rdgen_start_io_on_object_allocate(packet_p, start_io_p, 
                                                       start_io_p->filter.package_id, start_io_p->filter.object_id);
    }
    /* The above code is responsible for completing the packet.
     */
    return status;
}
/******************************************
 * end fbe_rdgen_service_start_io()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_scan_queues()
 ****************************************************************
 * @brief
 *  Scan the queues, looking for requests taking too long.
 *
 * @param none.       
 *
 * @return None.
 *
 * @author
 *  10/14/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_scan_queues(void)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_queue_head_t *active_ts_head_p = NULL;
    fbe_rdgen_request_t *request_p = NULL;
    fbe_rdgen_ts_t *ts_p = NULL;
    fbe_time_t elapsed_seconds = 0;
    fbe_time_t current_time = 0;

    /* First loop over the entire list of active requests.
     */
    fbe_rdgen_lock();

    /* We check again under the lock since someone else might have 
     * already scanned. 
     */
    if (!fbe_rdgen_is_time_to_scan_queues())
    {
        fbe_rdgen_unlock();
        return;
    }
    
    fbe_rdgen_get_active_request_queue(&head_p);
    request_p = (fbe_rdgen_request_t *) fbe_queue_front(head_p);
    while (request_p != NULL)
    {
        /* For each request, loop over the list of TS.
         */
        fbe_queue_element_t *queue_element_p = NULL;
        fbe_rdgen_request_t *next_request_p = NULL;
        fbe_rdgen_request_lock(request_p);
        fbe_rdgen_request_get_active_ts_queue(request_p, &active_ts_head_p);

        queue_element_p = fbe_queue_front(active_ts_head_p);
        while (queue_element_p != NULL)
        {
            /* For each TS see if it has been pending for too long.
             */
            ts_p = fbe_rdgen_ts_request_queue_element_to_ts_ptr(queue_element_p);
            current_time = fbe_get_time_in_us();

            if (current_time >= ts_p->last_send_time)
            {
                elapsed_seconds = (current_time - ts_p->last_send_time) / FBE_TIME_MICROSECONDS_PER_SECOND;
                if (elapsed_seconds > (fbe_rdgen_get_max_io_msecs()/FBE_TIME_MILLISECONDS_PER_SECOND))
                {
                    /* Panic - Logging CRITICAL ERROR will take care of panic. */
                    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                            FBE_TRACE_MESSAGE_ID_INFO, 
                                            "%s: rdgen ts %p elapsed time is %d seconds\n", 
                                            __FUNCTION__, ts_p, (int)elapsed_seconds);
                }
            }
            queue_element_p = fbe_queue_next(active_ts_head_p, queue_element_p);
        } /* while the queue element is not null. */

        next_request_p = (fbe_rdgen_request_t *)fbe_queue_next(head_p, &request_p->queue_element);
        fbe_rdgen_request_unlock(request_p);
        request_p = next_request_p;
    }
    fbe_rdgen_update_last_scan_time();
    fbe_rdgen_unlock();
    return;
}
/******************************************
 * end fbe_rdgen_scan_queues()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_scan_for_abort()
 ****************************************************************
 * @brief
 *  Scan the queues, looking for requests taking too long.
 *
 * @param max_wait_time_p - Max time remaining out of all requests.      
 *
 * @return None.
 *
 * @author
 *  4/13/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_scan_for_abort(fbe_time_t *max_wait_time_p)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_queue_head_t *active_ts_head_p = NULL;
    fbe_rdgen_request_t *request_p = NULL;
    fbe_rdgen_ts_t *ts_p = NULL;
    fbe_time_t current_time_us;

    *max_wait_time_p = FBE_RDGEN_THREAD_MAX_SCAN_SECONDS * FBE_TIME_MILLISECONDS_PER_SECOND;

    /* First loop over the entire list of active requests.
     */
    fbe_rdgen_lock();
    
    fbe_rdgen_get_active_request_queue(&head_p);
    request_p = (fbe_rdgen_request_t *) fbe_queue_front(head_p);
    while (request_p != NULL)
    {
        /* For each request, loop over the list of TS.
         */
        fbe_queue_element_t *queue_element_p = NULL;
        fbe_rdgen_request_t *next_request_p = NULL;

        /* Check if this request has a small wait time.
         */
        if (request_p->specification.msecs_to_abort != 0)
        {
            *max_wait_time_p = FBE_MIN(request_p->specification.msecs_to_abort,
                                       *max_wait_time_p);
        }

        fbe_rdgen_request_lock(request_p);
        fbe_rdgen_request_get_active_ts_queue(request_p, &active_ts_head_p);
        queue_element_p = fbe_queue_front(active_ts_head_p);
        while (queue_element_p != NULL)
        {
            fbe_packet_t *packet_p = NULL;
            /* For each TS see if it has been pending for too long.
             */
            ts_p = fbe_rdgen_ts_request_queue_element_to_ts_ptr(queue_element_p);
            fbe_rdgen_ts_lock(ts_p);
            if (ts_p->object_p->device_p)
            {
                /* Don't touch these irp type requests.  We don't yet support cancel of irps.
                 */
                *max_wait_time_p = 0;
                queue_element_p = fbe_queue_next(active_ts_head_p, queue_element_p);
                fbe_rdgen_ts_unlock(ts_p);
                continue;
            }
            packet_p = fbe_rdgen_ts_get_packet_ptr(ts_p);
            current_time_us = fbe_get_time_in_us();

            /* If the request is still outstanding and the last send time 
             * is beyond the time we want to wait for aborting, then 
             * go ahead and abort it now. 
             */
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s packet: %p current_time_us: %llx b_outstanding: %d "
                                    "last_send_time: %llx diff: %llx abt: %llx\n", 
                                    __FUNCTION__, packet_p,
                                    (unsigned long long)current_time_us,
                                    ts_p->b_outstanding,
                                    (unsigned long long)ts_p->last_send_time,
                                    (unsigned long long)(current_time_us - ts_p->last_send_time),
                                    ts_p->time_to_abort);

            /* Don't abort things we sent to the peer since the peer will abort them if they took too long.
             */
            if ((packet_p != NULL) && 
                ts_p->b_outstanding &&
                !ts_p->b_send_to_peer &&
                (current_time_us >= ts_p->last_send_time))
            {
                /* We abort if the time is greater than our time set for abort
                 * We do not abort if it is already aborted. 
                 * We do not abort modify operations if it is not on the modify phase which uses the write packet.  
                 */
                if ((packet_p->packet_state != FBE_PACKET_STATE_CANCELED) &&
                    (current_time_us >= ts_p->time_to_abort) &&
                    (!fbe_rdgen_operation_is_modify(ts_p->operation) || 
                     (packet_p == &ts_p->write_transport.packet)))
                {
                    fbe_trace_level_t trace_level = FBE_TRACE_LEVEL_DEBUG_LOW;
                    if ((request_p->specification.msecs_to_abort == 0) &&
                        !fbe_rdgen_specification_options_is_set(&request_p->specification,
                                                                FBE_RDGEN_OPTIONS_RANDOM_ABORT))
                    {
                        fbe_u32_t msec = fbe_get_elapsed_microseconds(ts_p->last_send_time);
                        /* An abort is not expected here.
                         */
                        trace_level = FBE_TRACE_LEVEL_ERROR;
                        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING,
                                                fbe_rdgen_object_get_object_id(ts_p->object_p),
                                                "pkt: %p expired (msec: %d) cur: 0x%llx sent: 0x%llx abort_time: 0x%llx\n", 
                                                packet_p, msec,
						(unsigned long long)current_time_us,
						(unsigned long long)ts_p->last_send_time,
						(unsigned long long)ts_p->time_to_abort);
                    }
                    /* Abort the request since it is taking too long.
                     */
                    fbe_rdgen_service_trace(trace_level,
                                            fbe_rdgen_object_get_object_id(ts_p->object_p),
                                            "%s cancel packet %p state: 0x%llx\n", __FUNCTION__, packet_p, (unsigned long long)ts_p->state);
                    fbe_transport_cancel_packet(packet_p);

                }
                else
                {
                    fbe_time_t time_to_wait;
                    time_to_wait = ts_p->time_to_abort - current_time_us;
                    time_to_wait /= FBE_TIME_MILLISECONDS_PER_MICROSECOND;
                    /* Just wait long enough to abort this request.
                     */ 
                    *max_wait_time_p = FBE_MIN(time_to_wait, *max_wait_time_p);
                }
            }
            else
            {
                /* Request is not outstanding, but we still need to come up with a 
                 * reasonable wait time. 
                 */
                fbe_time_t time_to_wait;
                if (request_p->specification.msecs_to_abort != 0)
                {
                    time_to_wait = request_p->specification.msecs_to_abort;
                }
                else
                {
                    time_to_wait = fbe_rdgen_get_max_io_msecs();
                }
                *max_wait_time_p = FBE_MIN(time_to_wait, *max_wait_time_p);
            }
            fbe_rdgen_ts_unlock(ts_p);
            queue_element_p = fbe_queue_next(active_ts_head_p, queue_element_p);
        } /* while the queue element is not null. */

        next_request_p = (fbe_rdgen_request_t *)fbe_queue_next(head_p, &request_p->queue_element);
        fbe_rdgen_request_unlock(request_p);
        request_p = next_request_p;
    }
    fbe_rdgen_unlock();
    return;
}
/******************************************
 * end fbe_rdgen_scan_for_abort()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_stop_requests()
 ****************************************************************
 * @brief
 *  Stop requests that match a given filter.
 *
 * @param filter_p - filter to match on when stopping.     
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  10/26/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_stop_requests(fbe_rdgen_filter_t *filter_p)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_rdgen_request_t *request_p = NULL;
    fbe_rdgen_request_t *next_request_p = NULL;

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: type: 0x%x obj: 0x%x class: 0x%x\n", 
                            __FUNCTION__, filter_p->filter_type, filter_p->object_id, filter_p->class_id );
    /* First loop over the entire list of active requests.
     */
    fbe_rdgen_lock();
    
    fbe_rdgen_get_active_request_queue(&head_p);
    request_p = (fbe_rdgen_request_t *) fbe_queue_front(head_p);
    while (request_p != NULL)
    {
        /* For each request, check its filter and mark stopped if needed.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "request_p: 0x%p type: 0x%x obj: 0x%x class: 0x%x\n", 
                                request_p, request_p->filter.filter_type, 
                                request_p->filter.object_id, request_p->filter.class_id);
        fbe_rdgen_request_lock(request_p);

        /* If the filter matches the request's filter, then stop.
         */
        if (filter_p->filter_type == FBE_RDGEN_FILTER_TYPE_INVALID)
        {
            /* User asked to stop all requests.
             */
            request_p->b_stop = FBE_TRUE;
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: stop request: %p\n", __FUNCTION__, request_p );
        }
        else if ((filter_p->filter_type == FBE_RDGEN_FILTER_TYPE_CLASS) &&
                 (request_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_CLASS) &&
                 (request_p->filter.class_id == filter_p->class_id))
        {
            /* User asked to stop all class requests for this class id.
             */
            request_p->b_stop = FBE_TRUE;
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, filter_p->class_id,
                                    "%s: stop class io request: %p\n", __FUNCTION__, request_p );
        }
        else if ((filter_p->filter_type == FBE_RDGEN_FILTER_TYPE_OBJECT) &&
                 (request_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_OBJECT) &&
                 (request_p->filter.object_id == filter_p->object_id))
        {
            /* User asked to stop all class requests for this class id.
             */
            request_p->b_stop = FBE_TRUE;
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, filter_p->object_id,
                                    "%s: stop object io request %p\n", __FUNCTION__, request_p );
        }
        else if ((filter_p->filter_type == FBE_RDGEN_FILTER_TYPE_DISK_INTERFACE) &&
                 (request_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_DISK_INTERFACE) &&
                 !csx_p_strncmp(&request_p->filter.device_name[0],
                                &filter_p->device_name[0], FBE_RDGEN_DEVICE_NAME_CHARS))
        {
            /* User asked to stop all class requests for this class id.
             */
            request_p->b_stop = FBE_TRUE;
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, filter_p->object_id,
                                    "%s: stop object io request %p\n", __FUNCTION__, request_p );
        }
        else if ((filter_p->filter_type == FBE_RDGEN_FILTER_TYPE_BLOCK_DEVICE) &&
                 (request_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_BLOCK_DEVICE) &&
                 !csx_p_strncmp(&request_p->filter.device_name[0],
                                &filter_p->device_name[0], FBE_RDGEN_DEVICE_NAME_CHARS))
        {
            /* User asked to stop all class requests for this class id.
             */
            request_p->b_stop = FBE_TRUE;
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, filter_p->object_id,
                                    "%s: stop object io request %p\n", __FUNCTION__, request_p );
        }
        next_request_p = (fbe_rdgen_request_t *)fbe_queue_next(head_p, &request_p->queue_element);

        if (fbe_rdgen_specification_options_is_set(&request_p->specification, FBE_RDGEN_OPTIONS_FILE)){
            /* We may have threads that are just waiting.  We kick the object thread to get these 
             * requests to finish. 
             */
            fbe_rdgen_request_stop_io(request_p);
        }
        fbe_rdgen_request_unlock(request_p);
        request_p = next_request_p;
    }
    fbe_rdgen_unlock();

    /* Stops all requests to block devices.
     */
    fbe_rdgen_lock();
    request_p = (fbe_rdgen_request_t *) fbe_queue_front(head_p);
    while (request_p != NULL)
    {
        next_request_p = (fbe_rdgen_request_t *)fbe_queue_next(head_p, &request_p->queue_element);
        if ((request_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_BLOCK_DEVICE) && 
            (request_p->b_stop == FBE_TRUE)) {
            fbe_rdgen_unlock();
            fbe_rdgen_request_stop_for_bdev(request_p);
            fbe_rdgen_lock();
            next_request_p = (fbe_rdgen_request_t *) fbe_queue_front(head_p);
        }
        request_p = next_request_p;
    }
    fbe_rdgen_unlock();

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_stop_requests()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_service_stop_io()
 ****************************************************************
 * @brief
 *  Stop all I/O based upon the input filter.
 *
 * @param packet_p - Packet contains a control operation with
 *                   an fbe_rdgen_control_start_io_t.
 *
 * @return status of the stop.
 *
 * @author
 *  10/26/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_service_stop_io(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_rdgen_control_stop_io_t * stop_io_p = NULL;
    fbe_u32_t len;
    fbe_status_t status;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &stop_io_p);
    if (stop_io_p == NULL)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_rdgen_control_stop_io_t))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu \n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_rdgen_control_stop_io_t));

        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    /* Stop I/O depending on the type of filter. 
     * A class filter stops every request with a given class. 
     * A io filter stops every request with a given object. 
     * An invalid filter stops everything. 
     */
    if (stop_io_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_INVALID)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Stop all rdgen I/O.\n", __FUNCTION__);
    }
    else if (stop_io_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_CLASS)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: stop class io class_id: 0x%x\n", __FUNCTION__, stop_io_p->filter.class_id);
    }
    else if (stop_io_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_OBJECT)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, stop_io_p->filter.object_id,
                                "%s: stop object io received\n", __FUNCTION__ );
    }
    else if (stop_io_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_DISK_INTERFACE)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: stop disk interface %s %d\n", 
                                __FUNCTION__, &stop_io_p->filter.device_name[0],
                                stop_io_p->filter.object_id);
    }
    else if (stop_io_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_BLOCK_DEVICE)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: stop block device %s %d\n", 
                                __FUNCTION__, &stop_io_p->filter.device_name[0],
                                stop_io_p->filter.object_id);
    }
    else
    {
         fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: unknown filter type 0x%x\n", __FUNCTION__, stop_io_p->filter.filter_type);
         fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
         return FBE_STATUS_OK;
    }

    /* Validation is done, now stop 
     * all the requests that match this filter. 
     */
    status = fbe_rdgen_stop_requests(&stop_io_p->filter);
    if (status == FBE_STATUS_OK)
    {
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_OK, FBE_STATUS_OK);
    }
    else
    {
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_service_stop_io()
 ******************************************/
void fbe_rdgen_ts_add_stats(fbe_rdgen_ts_t *ts_p,
                            fbe_rdgen_control_get_stats_t *stats_p)
{
    stats_p->in_process_io_stats.aborted_error_count += ts_p->abort_err_count;
    stats_p->in_process_io_stats.error_count += ts_p->err_count;
    stats_p->in_process_io_stats.io_count += ts_p->io_count;
    stats_p->in_process_io_stats.io_failure_error_count += ts_p->io_failure_err_count;
    stats_p->in_process_io_stats.congested_error_count += ts_p->congested_err_count;
    stats_p->in_process_io_stats.still_congested_error_count += ts_p->still_congested_err_count;
    stats_p->in_process_io_stats.invalid_request_error_count += ts_p->invalid_request_err_count;
    stats_p->in_process_io_stats.inv_blocks_count += ts_p->inv_blocks_count;
    stats_p->in_process_io_stats.bad_crc_blocks_count += ts_p->bad_crc_blocks_count;
    stats_p->in_process_io_stats.media_error_count += ts_p->media_err_count;
    stats_p->in_process_io_stats.pass_count += ts_p->pass_count;

    if (ts_p->request_p->specification.operation == FBE_RDGEN_OPERATION_READ_AND_BUFFER) {
        stats_p->in_process_io_stats.pin_count += 1;
    }
    return;
}


/*!**************************************************************
 * fbe_rdgen_get_stats()
 ****************************************************************
 * @brief
 *  Get info for a given filter.
 *
 * @param stats_p - Information to fetch.
 * @param filter_p - filter to match on when stopping.     
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  2/11/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_get_stats(fbe_rdgen_control_get_stats_t *stats_p,
                                 fbe_rdgen_filter_t *filter_p)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_queue_head_t *active_ts_head_p = NULL;
    fbe_rdgen_request_t *request_p = NULL;
    fbe_rdgen_ts_t *ts_p = NULL;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_u32_t num_threads = 0;
    fbe_rdgen_filter_t filter = *filter_p; /* saved copy of filter. */
    fbe_rdgen_io_statistics_t *historical_stats_p = NULL;
    fbe_zero_memory(stats_p, sizeof(fbe_rdgen_control_get_stats_t));

    /* First loop over the entire list of active requests.
     */
    fbe_rdgen_lock();
    
    fbe_rdgen_get_active_request_queue(&head_p);
    request_p = (fbe_rdgen_request_t *) fbe_queue_front(head_p);
    while (request_p != NULL)
    {
        /* For each request, check its filter and mark stopped if needed.
         */
        fbe_rdgen_request_t *next_request_p = NULL;
        fbe_bool_t b_check = FBE_FALSE;
        fbe_rdgen_request_lock(request_p);

        /* If the filter matches the request's filter, then stop.
         */
        if (filter.filter_type == FBE_RDGEN_FILTER_TYPE_INVALID)
        {
            /* User asked to stop all requests.
             */
            b_check = FBE_TRUE;
        }
        else if ((filter.filter_type == FBE_RDGEN_FILTER_TYPE_CLASS) &&
                 (request_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_CLASS) &&
                 (request_p->filter.class_id == filter.class_id))
        {
            /* User asked to stop all class requests for this class id.
             */
            b_check = FBE_TRUE;
        }
        else if ((filter.filter_type == FBE_RDGEN_FILTER_TYPE_OBJECT) &&
                 (request_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_OBJECT) &&
                 (request_p->filter.object_id == filter.object_id))
        {
            /* User asked to stop all class requests for this class id.
             */
            b_check = FBE_TRUE;
        }
        /* If this is a good request to check, then add the stats from all 
         * the threads to the stats_p. 
         */
        if (b_check)
        {
            fbe_rdgen_request_get_active_ts_queue(request_p, &active_ts_head_p);
            queue_element_p = fbe_queue_front(active_ts_head_p);

            while (queue_element_p != NULL)
            {
                ts_p = fbe_rdgen_ts_request_queue_element_to_ts_ptr(queue_element_p);
                fbe_rdgen_ts_add_stats(ts_p, stats_p);
                num_threads++;
                queue_element_p = fbe_queue_next(active_ts_head_p, &ts_p->request_queue_element);
            }
        }
        next_request_p = (fbe_rdgen_request_t *)fbe_queue_next(head_p, &request_p->queue_element);
        fbe_rdgen_request_unlock(request_p);
        request_p = next_request_p;
    }
    stats_p->objects = fbe_rdgen_get_num_objects();
    stats_p->threads = num_threads;
    stats_p->requests = fbe_rdgen_get_num_requests();
    stats_p->memory_size_mb = fbe_rdgen_get_memory_size();
    stats_p->memory_type = fbe_rdgen_get_memory_type();
    stats_p->io_timeout_msecs = fbe_rdgen_get_max_io_msecs();

    historical_stats_p = fbe_rdgen_get_historical_stats();
    stats_p->historical_stats = *historical_stats_p;
    fbe_rdgen_unlock();

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_get_stats()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_service_get_stats()
 ****************************************************************
 * @brief
 *  Return overall stats for an input filter.
 *
 * @param packet_p - Packet contains a control operation with
 *                   an fbe_rdgen_control_get_stats_t.
 *
 * @return status of the stop.
 *
 * @author
 *  2/11/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_service_get_stats(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_rdgen_control_get_stats_t * get_stats_p = NULL;
    fbe_u32_t len;
    fbe_status_t status;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &get_stats_p);
    if (get_stats_p == NULL)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_rdgen_control_get_stats_t))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu \n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_rdgen_control_get_stats_t));

        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }


    /* Get stats depending on the type of filter. 
     * A class filter gets every request with a given class .
     * A io filter gets every request with a given object An. 
     * invalid filter gets everything                       .
     */
    status = fbe_rdgen_get_stats(get_stats_p, &get_stats_p->filter);

    /* Validation is done, now stop 
     * all the requests that match this filter. 
     */
    if (status == FBE_STATUS_OK)
    {
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_OK, FBE_STATUS_OK);
    }
    else
    {
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_service_get_stats()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_reset_all_thread_stats()
 ****************************************************************
 * @brief
 *  Reset all thread stats.
 *
 * @param stats_p - Information to fetch.
 * @param filter_p - filter to match on when stopping.     
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  3/21/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_reset_all_thread_stats(void)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_queue_head_t *active_ts_head_p = NULL;
    fbe_rdgen_request_t *request_p = NULL;
    fbe_rdgen_ts_t *ts_p = NULL;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_u32_t num_threads = 0;

    /* First loop over the entire list of active requests.
     */
    fbe_rdgen_lock();
    
    fbe_rdgen_get_active_request_queue(&head_p);
    request_p = (fbe_rdgen_request_t *) fbe_queue_front(head_p);
    while (request_p != NULL)
    {
        /* For each request, check its filter and mark stopped if needed.
         */
        fbe_rdgen_request_t *next_request_p = NULL;
        fbe_rdgen_request_lock(request_p);

        /* If this is a good request to check, then add the stats from all 
         * the threads to the stats_p. 
         */
        fbe_rdgen_request_get_active_ts_queue(request_p, &active_ts_head_p);
        queue_element_p = fbe_queue_front(active_ts_head_p);

        fbe_rdgen_request_update_start_time(request_p);
        while (queue_element_p != NULL)
        {
            ts_p = fbe_rdgen_ts_request_queue_element_to_ts_ptr(queue_element_p);
            fbe_rdgen_ts_set_flag(ts_p, FBE_RDGEN_TS_FLAGS_RESET_STATS);
            fbe_rdgen_ts_update_start_time(ts_p);
            num_threads++;
            queue_element_p = fbe_queue_next(active_ts_head_p, &ts_p->request_queue_element);
        }
        next_request_p = (fbe_rdgen_request_t *)fbe_queue_next(head_p, &request_p->queue_element);
        fbe_rdgen_request_unlock(request_p);
        request_p = next_request_p;
    }
    fbe_rdgen_unlock();

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_reset_all_thread_stats()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_service_reset_stats()
 ****************************************************************
 * @brief
 *  Reset the overall rdgen stats.
 *
 * @param packet_p - 
 *
 * @return status of the reset.
 *
 * @author
 *  5/8/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_service_reset_stats(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_u32_t len;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != 0)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != 0 \n", __FUNCTION__, len);

        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "reset statistics.\n");
    fbe_rdgen_service_clear_io_statistics();
    fbe_rdgen_reset_all_thread_stats();
    fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_OK, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_service_reset_stats()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_get_thread_info()
 ****************************************************************
 * @brief
 *  Get info on all threads.
 *
 * @param thread_info_p - Information to fetch.
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  3/28/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_get_thread_info(fbe_rdgen_control_get_thread_info_t *thread_info_p)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_rdgen_ts_t *thread_p = NULL;
    fbe_rdgen_object_t *object_p = NULL;
    fbe_rdgen_object_t *next_object_p = NULL;
    fbe_u32_t thread_index = 0;

    /* First loop over the entire list of active objects.
     */
    fbe_rdgen_lock();
    
    fbe_rdgen_get_active_object_queue(&head_p);
    object_p = (fbe_rdgen_object_t *) fbe_queue_front(head_p);

    while (object_p != NULL)
    {
        if (thread_index >= thread_info_p->num_threads)
        {
            break;
        }
        /* For each object, loop over all threads.
         */
        fbe_rdgen_object_lock(object_p);

        thread_p = (fbe_rdgen_ts_t *) fbe_queue_front(&object_p->active_ts_head);

        while (thread_p != NULL)
        {
            if (thread_index >= thread_info_p->num_threads)
            {
                break;
            }
            thread_info_p->thread_info_array[thread_index].abort_err_count = thread_p->abort_err_count;
            thread_info_p->thread_info_array[thread_index].b_aborted = thread_p->b_aborted;
            thread_info_p->thread_info_array[thread_index].err_count = thread_p->err_count;
            thread_info_p->thread_info_array[thread_index].io_count = thread_p->io_count;
            thread_info_p->thread_info_array[thread_index].io_failure_err_count = thread_p->io_failure_err_count;
            thread_info_p->thread_info_array[thread_index].congested_err_count = thread_p->congested_err_count;
            thread_info_p->thread_info_array[thread_index].still_congested_err_count = thread_p->still_congested_err_count;
            thread_info_p->thread_info_array[thread_index].invalid_request_err_count = thread_p->invalid_request_err_count;
            thread_info_p->thread_info_array[thread_index].pass_count = thread_p->pass_count;
            thread_info_p->thread_info_array[thread_index].media_err_count = thread_p->media_err_count;
            thread_info_p->thread_info_array[thread_index].thread_handle = (fbe_rdgen_handle_t)(csx_ptrhld_t)thread_p;
            thread_info_p->thread_info_array[thread_index].object_handle = (fbe_rdgen_handle_t)(csx_ptrhld_t)thread_p->object_p;
            thread_info_p->thread_info_array[thread_index].request_handle = (fbe_rdgen_handle_t)(csx_ptrhld_t)thread_p->request_p;
            thread_info_p->thread_info_array[thread_index].partial_io_count = thread_p->partial_io_count;
            thread_info_p->thread_info_array[thread_index].verify_errors = thread_p->verify_errors;
            thread_info_p->thread_info_array[thread_index].elapsed_milliseconds = fbe_get_elapsed_milliseconds(thread_p->start_time);
            thread_info_p->thread_info_array[thread_index].cumulative_response_time = 
                thread_p->cumulative_response_time / FBE_TIME_MILLISECONDS_PER_MICROSECOND;
            thread_info_p->thread_info_array[thread_index].cumulative_response_time_us = thread_p->cumulative_response_time;
            thread_info_p->thread_info_array[thread_index].total_responses = thread_p->total_responses;
            thread_info_p->thread_info_array[thread_index].max_response_time = thread_p->max_response_time;
            thread_info_p->thread_info_array[thread_index].last_response_time = thread_p->last_response_time;
            thread_p = (fbe_rdgen_ts_t *)fbe_queue_next(&object_p->active_ts_head, &thread_p->queue_element);
            thread_index++;
        }
        next_object_p = (fbe_rdgen_object_t *)fbe_queue_next(head_p, &object_p->queue_element);
        fbe_rdgen_object_unlock(object_p);

        object_p = next_object_p;
    }
    thread_info_p->num_threads = thread_index;
    fbe_rdgen_unlock();

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_get_thread_info()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_get_request_info()
 ****************************************************************
 * @brief
 *  Get info on all objects.
 *
 * @param request_info_p - Information to fetch.
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  3/28/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_get_request_info(fbe_rdgen_control_get_request_info_t *request_info_p)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_rdgen_request_t *request_p = NULL;
    fbe_u32_t request_index = 0;

    /* First loop over the entire list of active requests.
     */
    fbe_rdgen_lock();
    
    fbe_rdgen_get_active_request_queue(&head_p);
    request_p = (fbe_rdgen_request_t *) fbe_queue_front(head_p);
    while (request_p != NULL)
    {
        /* For each request, check its filter and mark stopped if needed.
         */
        fbe_rdgen_request_t *next_request_p = NULL;

        if (request_index >= request_info_p->num_requests)
        {
            break;
        }
        fbe_rdgen_request_lock(request_p);
        request_info_p->request_info_array[request_index].request_handle = (fbe_rdgen_handle_t)(csx_ptrhld_t)request_p;
        request_info_p->request_info_array[request_index].filter = request_p->filter;
        request_info_p->request_info_array[request_index].aborted_err_count = request_p->aborted_err_count;
        request_info_p->request_info_array[request_index].active_ts_count = request_p->active_ts_count;
        request_info_p->request_info_array[request_index].b_stop = request_p->b_stop;
        request_info_p->request_info_array[request_index].err_count = request_p->err_count;
        request_info_p->request_info_array[request_index].io_count = request_p->io_count;
        request_info_p->request_info_array[request_index].io_failure_err_count = request_p->io_failure_err_count;
        request_info_p->request_info_array[request_index].invalid_request_err_count = request_p->invalid_request_err_count;
        request_info_p->request_info_array[request_index].media_err_count = request_p->media_err_count;
        request_info_p->request_info_array[request_index].pass_count = request_p->pass_count;
        request_info_p->request_info_array[request_index].specification = request_p->specification;
        request_info_p->request_info_array[request_index].elapsed_seconds = 
            fbe_get_elapsed_seconds(fbe_rdgen_request_get_start_time(request_p));
        request_info_p->request_info_array[request_index].verify_errors = request_p->verify_errors;
        next_request_p = (fbe_rdgen_request_t *)fbe_queue_next(head_p, &request_p->queue_element);
        fbe_rdgen_request_unlock(request_p);
        request_p = next_request_p;
        request_index++;
    }
    request_info_p->num_requests = request_index;
    fbe_rdgen_unlock();

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_get_request_info()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_get_object_info()
 ****************************************************************
 * @brief
 *  Get info on all objects.
 *
 * @param object_info_p - Information to fetch.
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  3/28/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_get_object_info(fbe_rdgen_control_get_object_info_t *object_info_p)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_rdgen_object_t *object_p = NULL;
    fbe_rdgen_object_t *next_object_p = NULL;
    fbe_u32_t object_index = 0;

    /* First loop over the entire list of active objects.
     */
    fbe_rdgen_lock();
    
    fbe_rdgen_get_active_object_queue(&head_p);
    object_p = (fbe_rdgen_object_t *) fbe_queue_front(head_p);

    while (object_p != NULL)
    {
        if (object_index >= object_info_p->num_objects)
        {
            break;
        }
        /* For each object, check its filter and mark stopped if needed.
         */
        fbe_rdgen_object_lock(object_p);
        object_info_p->object_info_array[object_index].object_handle = (fbe_rdgen_handle_t)(csx_ptrhld_t)object_p;
        object_info_p->object_info_array[object_index].active_request_count = object_p->active_request_count;
        object_info_p->object_info_array[object_index].active_ts_count = object_p->active_ts_count;
        object_info_p->object_info_array[object_index].block_size = object_p->block_size;
        object_info_p->object_info_array[object_index].capacity = object_p->capacity;
        object_info_p->object_info_array[object_index].num_ios = object_p->num_ios;
        object_info_p->object_info_array[object_index].num_passes = object_p->num_passes;
        object_info_p->object_info_array[object_index].object_id = object_p->object_id;
        object_info_p->object_info_array[object_index].package_id = object_p->package_id;
        object_info_p->object_info_array[object_index].stripe_size = object_p->stripe_size;
        csx_p_strncpy(&object_info_p->object_info_array[object_index].driver_object_name[0],
                      object_p->device_name, FBE_RDGEN_DEVICE_NAME_CHARS);
        next_object_p = (fbe_rdgen_object_t *)fbe_queue_next(head_p, &object_p->queue_element);
        fbe_rdgen_object_unlock(object_p);
        object_p = next_object_p;
        object_index++;
    }
    object_info_p->num_objects = object_index;
    fbe_rdgen_unlock();

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_get_object_info()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_service_get_thread_info()
 ****************************************************************
 * @brief
 *  Get information on the threads running in rdgen..
 *
 * @param packet_p - Packet contains a control operation with
 *                   an fbe_rdgen_control_get_thread_info_t.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/28/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_service_get_thread_info(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_rdgen_control_get_thread_info_t * get_thread_info_p = NULL;
    fbe_u32_t len;
    fbe_status_t status;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &get_thread_info_p);
    if (get_thread_info_p == NULL)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len < sizeof(fbe_rdgen_control_get_thread_info_t))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu \n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_rdgen_control_get_thread_info_t));

        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    /* Get the requests stats.
     */
    status = fbe_rdgen_get_thread_info(get_thread_info_p);

    if (status == FBE_STATUS_OK)
    {
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_OK, FBE_STATUS_OK);
    }
    else
    {
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_service_get_thread_info()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_service_get_request_info()
 ****************************************************************
 * @brief
 *  Get information on the requests rdgen is running to.
 *
 * @param packet_p - Packet contains a control operation with
 *                   an fbe_rdgen_control_get_stats_t.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/28/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_service_get_request_info(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_rdgen_control_get_request_info_t * get_request_info_p = NULL;
    fbe_u32_t len;
    fbe_status_t status;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &get_request_info_p);
    if (get_request_info_p == NULL)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len < sizeof(fbe_rdgen_control_get_request_info_t))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu \n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_rdgen_control_get_request_info_t));

        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    /* Get the request stats.
     */
    status = fbe_rdgen_get_request_info(get_request_info_p);

    if (status == FBE_STATUS_OK)
    {
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_OK, FBE_STATUS_OK);
    }
    else
    {
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_service_get_request_info()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_service_get_object_info()
 ****************************************************************
 * @brief
 *  Get information on the objects rdgen is running to.
 *
 * @param packet_p - Packet contains a control operation with
 *                   an fbe_rdgen_control_get_stats_t.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/28/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_service_get_object_info(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_rdgen_control_get_object_info_t * get_object_info_p = NULL;
    fbe_u32_t len;
    fbe_status_t status;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &get_object_info_p);
    if (get_object_info_p == NULL)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len < sizeof(fbe_rdgen_control_get_object_info_t))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu \n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_rdgen_control_get_object_info_t));

        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    /* Get the object stats.
     */
    status = fbe_rdgen_get_object_info(get_object_info_p);

    if (status == FBE_STATUS_OK)
    {
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_OK, FBE_STATUS_OK);
    }
    else
    {
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_service_get_object_info()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_service_alloc_peer_memory()
 ****************************************************************
 * @brief
 *  Allocate memory for peer requests.
 *
 * @param packet_p - Packet contains no buffer.
 *
 * @return fbe_status_t
 *
 * @author
 *  5/22/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_service_alloc_peer_memory(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    
    /* Alloc memory for the peer requests if not done already.
     */
    if (status == FBE_STATUS_OK)
    {
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_OK, FBE_STATUS_OK);
    }
    else
    {
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_service_alloc_peer_memory()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_service_set_timeout()
 ****************************************************************
 * @brief
 *  Set the rdgen timeout parameters.
 *
 * @param packet_p - Packet contains no buffer.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/9/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_service_set_timeout(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_rdgen_control_set_timeout_info_t *timeout_info_p = NULL;
    fbe_u32_t len;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);


    fbe_payload_control_get_buffer(control_operation_p, &timeout_info_p);
    if (timeout_info_p == NULL)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len < sizeof(fbe_rdgen_control_alloc_mem_info_t))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %d \n", __FUNCTION__, len, (int)sizeof(fbe_rdgen_control_set_timeout_info_t));
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    /* Make sure the time is not 0.
     */
    if (timeout_info_p->default_io_timeout == 0)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid timeout %lld msecs\n", __FUNCTION__, (long long)timeout_info_p->default_io_timeout);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s rdgen I/O timeout set to %lld \n", __FUNCTION__,  timeout_info_p->default_io_timeout);

    fbe_rdgen_set_max_io_msecs(timeout_info_p->default_io_timeout);
    fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_OK, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_service_set_timeout()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_enable_system_threads()
 ****************************************************************
 * @brief
 *  Enable system thread use in rdgen
 *
 * @param packet_p - Packet contains no buffer.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/24/2013 - Created. Ron Lesniak
 *
 ****************************************************************/
static fbe_status_t fbe_rdgen_enable_system_threads(fbe_packet_t * packet_p)
{
    fbe_rdgen_external_queue = fbe_rdgen_external_queue_location;   

    fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_OK, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_enable_system_threads()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_disable_system_threads()
 ****************************************************************
 * @brief
 *  Disable system thread use in rdgen
 *
 * @param packet_p - Packet contains no buffer.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/24/2013 - Created. Ron Lesniak
 *
 ****************************************************************/
static fbe_status_t fbe_rdgen_disable_system_threads(fbe_packet_t * packet_p)
{
    fbe_rdgen_external_queue = NULL;   

    fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_OK, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_disable_system_threads()
 ******************************************/

static fbe_memory_dps_init_parameters_t rdgen_dps_init_parameters;
/*!**************************************************************
 * fbe_rdgen_service_init_dps_memory()
 ****************************************************************
 * @brief
 *  Allocate memory for peer requests.
 *
 * @param packet_p - Packet contains no buffer.
 *
 * @return fbe_status_t
 *
 * @author
 *  5/22/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_service_init_dps_memory(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_rdgen_control_alloc_mem_info_t * mem_info_p = NULL;
    fbe_u32_t len;
    fbe_u64_t total_memory;
    fbe_u32_t num_64_block_chunks;
    fbe_u32_t chunk_size;
    #define FBE_RDGEN_MAX_MEMORY_SIZE_MB 100000
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    
    fbe_payload_control_get_buffer(control_operation_p, &mem_info_p);
    if (mem_info_p == NULL)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len < sizeof(fbe_rdgen_control_alloc_mem_info_t))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %d \n", __FUNCTION__, len, (int)sizeof(fbe_rdgen_control_alloc_mem_info_t));

        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }
    fbe_zero_memory(&rdgen_dps_init_parameters, sizeof(fbe_memory_dps_init_parameters_t));

    if ((mem_info_p->mem_size_in_mb == 0) || (mem_info_p->mem_size_in_mb >= FBE_RDGEN_MAX_MEMORY_SIZE_MB))
    { 
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Invalid memory size %d \n", __FUNCTION__, mem_info_p->mem_size_in_mb);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    /* If we already allocated memory, we will not allow it again.
     */
    if (fbe_rdgen_service_is_flag_set(FBE_RDGEN_SERVICE_FLAGS_DPS_MEM_INITTED))
    { 
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s memory already initialized.  Multiple memory inits not allowed.\n", __FUNCTION__);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }
    /* Set our 64 block pool since this is all rdgen uses.
     */
    //chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO * FBE_MEMORY_DPS_64_BLOCKS_IO_PER_MAIN_CHUNK;
	chunk_size = 1024 * 1024; /* 1 MB */
    total_memory = mem_info_p->mem_size_in_mb * (1024 * 1024);
    if (total_memory % chunk_size)
    {
        total_memory += chunk_size - (total_memory % chunk_size);
    }
    num_64_block_chunks = (fbe_u32_t) (total_memory / chunk_size);

    rdgen_dps_init_parameters.block_64_number_of_chunks = num_64_block_chunks;
    rdgen_dps_init_parameters.number_of_main_chunks = num_64_block_chunks;

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s allocate %d MB  num 64 block chunks: %d \n", 
                            __FUNCTION__, mem_info_p->mem_size_in_mb, num_64_block_chunks);

    /* Init the number of total chunks.
     */
    status = fbe_memory_dps_init_number_of_chunks(&rdgen_dps_init_parameters);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Failed to init memory chunks status: %d\n", __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_PENDING; 
    }

    /* Set the memory functions so we will use sp cache to allocate memory.
     */
    if ((mem_info_p->memory_type == FBE_RDGEN_MEMORY_TYPE_INVALID) ||
        (mem_info_p->memory_type == FBE_RDGEN_MEMORY_TYPE_CACHE))
    {
        /* By default we try to get cache memory.
         */
        mem_info_p->memory_type = FBE_RDGEN_MEMORY_TYPE_CACHE;
        status = fbe_memory_dps_set_memory_functions(fbe_rdgen_alloc_cache_mem, fbe_rdgen_release_cache_mem);
    }
    else
    {
        status = fbe_memory_dps_set_memory_functions(fbe_rdgen_cmm_memory_allocate, fbe_rdgen_cmm_memory_release);
    }
    if (status != FBE_STATUS_OK)
    { 
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Failed to init memory chunks status: %d\n", __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_PENDING; 
    }

    /* Tell dps to init the pool.  It will pass through to our supplied functions to allocate memory from cache.
     */
    status = fbe_memory_dps_init(FBE_FALSE /* don't retry forever */);
    /* Alloc memory for the peer requests if not done already.
     */
    if (status == FBE_STATUS_OK)
    {
        fbe_rdgen_service_set_flag(FBE_RDGEN_SERVICE_FLAGS_DPS_MEM_INITTED);
        fbe_rdgen_set_memory_size(mem_info_p->mem_size_in_mb);
        fbe_rdgen_set_memory_type(mem_info_p->memory_type);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_OK, FBE_STATUS_OK);
    }
    else
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Failed to allocate memory status: %d\n", __FUNCTION__, status);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
    }
    return FBE_STATUS_OK;
} 
/******************************************
 * end fbe_rdgen_service_init_dps_memory()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_service_release_dps_memory()
 ****************************************************************
 * @brief
 *  Release the DPS memory for rdgen.
 * 
 *  If it was not allocated, this has no effect.
 *
 * @param packet_p - Packet contains no buffer.
 *
 * @return fbe_status_t
 *
 * @author
 *  5/22/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_service_release_dps_memory(fbe_packet_t * packet_p)
{
    if (fbe_rdgen_service_is_flag_set(FBE_RDGEN_SERVICE_FLAGS_DPS_MEM_INITTED))
    {
        fbe_rdgen_release_memory();
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_OK, FBE_STATUS_OK);
    }
    else
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s DPS not initialized.\n", __FUNCTION__);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_OK);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_service_release_dps_memory()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_queue_set_external_handler()
 ****************************************************************
 * @brief
 *  Setter for fbe_rdgen_external_queue_location
 *
 * @param handler - Value provided.
 *
 * @author
 *  4/29/2013 - Created. Ron Lesniak
 *
 ****************************************************************/
void fbe_rdgen_queue_set_external_handler(void (*handler) (fbe_queue_element_t *, fbe_cpu_id_t))
{
    fbe_rdgen_external_queue_location = handler;
}
/******************************************
 * end fbe_rdgen_queue_set_external_handler()
 ******************************************/

fbe_status_t 
fbe_rdgen_complete_master(fbe_packet_t * packet_p, 
                          fbe_packet_completion_context_t context)
{
    fbe_bool_t is_empty;
    fbe_packet_t *master_packet_p = fbe_transport_get_master_packet(packet_p);

    if (master_packet_p != NULL) {
        fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);
    
        fbe_transport_complete_packet(master_packet_p);
    } else {
        memory_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                             "%s no master packet for subpacket %p\n", __FUNCTION__, packet_p);
    }
    return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_rdgen_control_unlock()
 ****************************************************************
 * @brief
 *  Restarts a paused request.  The request did some initial processing,
 *  marked itself waiting and then returned the packet.
 *  Now just find and kick off that waiting request.
 *
 * @param packet_p - Current packet.  
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  12/2/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_control_unlock(fbe_packet_t *packet_p)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_rdgen_request_t *request_p = NULL;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_rdgen_ts_t *ts_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_rdgen_control_start_io_t * unlock_p = NULL;
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_u32_t len;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &unlock_p);
    if (unlock_p == NULL){
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_rdgen_control_start_io_t)){
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu \n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_rdgen_control_start_io_t));

        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* First loop over the entire list of active requests.
     */
    fbe_rdgen_lock();
    
    fbe_rdgen_get_active_request_queue(&head_p);
    request_p = (fbe_rdgen_request_t *) fbe_queue_front(head_p);
    while (request_p != NULL)
    {
        /* For each request, check its filter and mark stopped if needed.
         */
        fbe_rdgen_request_t *next_request_p = NULL;
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "request_p: 0x%p type: 0x%x obj: 0x%x class: 0x%x\n", 
                                request_p, request_p->filter.filter_type, 
                                request_p->filter.object_id, request_p->filter.class_id);
        fbe_rdgen_request_lock(request_p);

        queue_element_p = fbe_queue_front(&request_p->active_ts_head);
        while (queue_element_p != NULL) {
            ts_p = fbe_rdgen_ts_request_queue_element_to_ts_ptr(queue_element_p);

            /* If the filter matches the request's filter, then stop.
             */
            if ((request_p->filter.filter_type == FBE_RDGEN_FILTER_TYPE_OBJECT) &&
                (request_p->filter.object_id == unlock_p->filter.object_id) &&
                ((unlock_p->status.context == (void*) ts_p) ||
                 (unlock_p->status.context == NULL))){
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, unlock_p->filter.object_id,
                                        "%s: restart object io request: %p ts: %p\n", __FUNCTION__, request_p, ts_p);
                fbe_rdgen_ts_clear_flag(ts_p, FBE_RDGEN_TS_FLAGS_WAIT_UNPIN);
                fbe_rdgen_request_unlock(request_p);
                break;
            }
            queue_element_p = fbe_queue_next(&request_p->active_ts_head, queue_element_p);
        }
        if (queue_element_p != NULL) {
            break;
        }
        next_request_p = (fbe_rdgen_request_t *)fbe_queue_next(head_p, &request_p->queue_element);

        fbe_rdgen_request_unlock(request_p);
        request_p = next_request_p;
    }
    fbe_rdgen_unlock();

    if (request_p == NULL) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s find from active queue failed\n", __FUNCTION__);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

   
    if ((fbe_rdgen_get_service_options() == FBE_RDGEN_SVC_OPTIONS_HOLD_UNPIN_FLUSH) &&
        (unlock_p->specification.operation == FBE_RDGEN_OPERATION_READ_AND_BUFFER_UNLOCK_FLUSH)){
        /* In this case we want to leave the existing request in place. 
         * Later on we will get another unlock from the test to cause the flush. 
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, unlock_p->filter.object_id,
                                "unlock and flush for context %p\n", unlock_p->status.context);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_OK, FBE_STATUS_OK);
        return FBE_STATUS_OK;
    }

    /* Wake up this requests.  When it finishes it will complete this packet.
     */
    fbe_rdgen_object_lock(ts_p->object_p);

    /* Associate the packets so that once the subpacket completes it will complete the master.
     */
    fbe_transport_add_subpacket(packet_p, request_p->packet_p);
    if (unlock_p->specification.operation == FBE_RDGEN_OPERATION_UNLOCK_RESTART){
        /* We need to kick off the operation so that it flushes.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_buffer_state6);

        /* Make sure that the master gets completed.
         */
        fbe_transport_set_completion_function(request_p->packet_p, fbe_rdgen_complete_master, packet_p);

        /* Mark the master packet with good status for when it gets completed.
         */
        unlock_p->status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_SUCCESS;
        unlock_p->status.status = FBE_STATUS_OK;
        unlock_p->status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    }
    fbe_rdgen_object_reset_restart_time(ts_p->object_p);
    fbe_rdgen_object_set_max_threads(ts_p->object_p, fbe_rdgen_object_get_num_threads(ts_p->object_p));
    fbe_rdgen_ts_enqueue_to_thread(ts_p);
    fbe_rdgen_object_unlock(ts_p->object_p);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_control_unlock()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_save_context()
 ****************************************************************
 * @brief
 *  Save the context to be returned with the packet.
 *
 * @param ts_p               
 *
 * @return None.
 *
 * @author
 *  12/9/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_ts_save_context(fbe_rdgen_ts_t *ts_p)
{
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_rdgen_control_start_io_t * start_io_p = NULL;
    fbe_payload_ex_t  *payload_p = NULL;

    payload_p = fbe_transport_get_payload_ex(ts_p->request_p->packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &start_io_p);
    start_io_p->status.context = ts_p;
    start_io_p->status.status = ts_p->last_packet_status.status;
    start_io_p->status.block_status = ts_p->last_packet_status.block_status;
    start_io_p->status.block_qualifier = ts_p->last_packet_status.block_qualifier;
    start_io_p->status.rdgen_status = ts_p->last_packet_status.rdgen_status;
}
/******************************************
 * end fbe_rdgen_ts_save_context()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_destroy_unpins()
 ****************************************************************
 * @brief
 *  Destroy unpin requests which are currently waiting.
 *
 * @param None.
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  5/29/2014 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_destroy_unpins(void)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_rdgen_request_t *request_p = NULL;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_rdgen_ts_t *ts_p = NULL;
    fbe_queue_head_t restart_queue_head;

    fbe_queue_init(&restart_queue_head);

    /* First loop over the entire list of active requests.
     */
    fbe_rdgen_lock();
    
    fbe_rdgen_get_active_request_queue(&head_p);
    request_p = (fbe_rdgen_request_t *) fbe_queue_front(head_p);
    while (request_p != NULL)
    {
        /* For each request, check its filter and mark stopped if needed.
         */
        fbe_rdgen_request_t *next_request_p = NULL;
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "request_p: 0x%p type: 0x%x obj: 0x%x class: 0x%x\n", 
                                request_p, request_p->filter.filter_type, 
                                request_p->filter.object_id, request_p->filter.class_id);
        fbe_rdgen_request_lock(request_p);

        queue_element_p = fbe_queue_front(&request_p->active_ts_head);
        while (queue_element_p != NULL) {
            ts_p = fbe_rdgen_ts_request_queue_element_to_ts_ptr(queue_element_p);
            
            /* If the filter matches the request's filter, then stop.
             */
            if (fbe_rdgen_ts_is_flag_set(ts_p, FBE_RDGEN_TS_FLAGS_WAIT_UNPIN)){
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, ts_p->object_p->object_id,
                                        "%s: destroy io request: %p ts: %p\n", __FUNCTION__, request_p, ts_p);
                fbe_rdgen_ts_clear_flag(ts_p, FBE_RDGEN_TS_FLAGS_WAIT_UNPIN);
                fbe_queue_push(&restart_queue_head, &ts_p->thread_queue_element); 
            }
            queue_element_p = fbe_queue_next(&request_p->active_ts_head, queue_element_p);
        }
        next_request_p = (fbe_rdgen_request_t *)fbe_queue_next(head_p, &request_p->queue_element);

        fbe_rdgen_request_unlock(request_p);
        request_p = next_request_p;
    }
    fbe_rdgen_unlock();

    /* Wake up this requests.  When it finishes it will complete this packet.
     */
    queue_element_p = fbe_queue_pop(&restart_queue_head);
    while (queue_element_p != NULL) {
         ts_p = fbe_rdgen_ts_thread_queue_element_to_ts_ptr(queue_element_p);
         fbe_rdgen_object_lock(ts_p->object_p);
         fbe_rdgen_object_reset_restart_time(ts_p->object_p);
         fbe_rdgen_object_set_max_threads(ts_p->object_p, fbe_rdgen_object_get_num_threads(ts_p->object_p));
         fbe_rdgen_ts_enqueue_to_thread(ts_p);
         fbe_rdgen_object_unlock(ts_p->object_p);
         queue_element_p = fbe_queue_pop(&restart_queue_head);
    }
    fbe_queue_destroy(&restart_queue_head);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_destroy_unpins()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_control_set_svc_options()
 ****************************************************************
 * @brief
 *  Set the rdgen service options.
 *
 * @param packet_p - Packet contains service options.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/28/2014 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_control_set_svc_options(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_rdgen_control_set_svc_options_t *options_p = NULL;
    fbe_u32_t len;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &options_p);
    if (options_p == NULL) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len < sizeof(fbe_rdgen_control_set_svc_options_t)) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid len %d != %d \n", __FUNCTION__, len, (int)sizeof(fbe_rdgen_control_set_svc_options_t));
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s rdgen service options set to %lld \n", __FUNCTION__,  options_p->options);

    fbe_rdgen_set_service_options(options_p->options);
    fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_OK, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_control_set_svc_options()
 ******************************************/
/*************************
 * end file fbe_rdgen_main.c
 *************************/
