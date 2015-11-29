/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raw_mirror_service_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry point for the raw mirror service. This
 *  service is used to test I/O to a raw mirror.
 *
 *  In production code, a raw mirror is used to access the system LUNs non-paged metadata
 *  and their configuration tables.  There is no object interface for this data.  It is
 *  accessed early in boot before any objects have been created.  
 * 
 *  The raw mirror service is used to test fbe_raid_raw_mirror_library
 *  and associated RAID library code.
 * 
 * @version
 *   10/2011:  Created. Susan Rundbaken 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_raw_mirror_service_private.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_memory.h"
#include "fbe_raid_library.h"
/*************************
 *   DEFINITIONS
 *************************/

/* Declare our service. 
 */
fbe_raw_mirror_service_t fbe_raw_mirror_service;
fbe_status_t fbe_raw_mirror_service_control_entry(fbe_packet_t * packet); 
fbe_service_methods_t fbe_raw_mirror_service_methods = {FBE_SERVICE_ID_RAW_MIRROR, fbe_raw_mirror_service_control_entry};

/* Declare a raw mirror. 
 */
static fbe_raw_mirror_t         fbe_raw_mirror;


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_status_t fbe_raw_mirror_service_init(fbe_packet_t * packet_p);
static fbe_status_t fbe_raw_mirror_service_init_thread_pool(void);
static fbe_status_t fbe_raw_mirror_service_initialize_threads(void);
static fbe_status_t fbe_raw_mirror_service_destroy(fbe_packet_t * packet_p);
static void fbe_raw_mirror_service_destroy_thread_pool(void);
static fbe_status_t fbe_raw_mirror_service_start_io(fbe_packet_t * packet_p);
static fbe_status_t fbe_raw_mirror_service_stop_io(fbe_packet_t * packet_p);
static fbe_status_t fbe_raw_mirror_service_raw_mirror_pvd_edge_enabled(fbe_packet_t * packet_p);
static void fbe_raw_mirror_service_request_enqueue(fbe_queue_element_t *queue_element_p);
static fbe_status_t fbe_raw_mirror_service_send_request_to_thread(fbe_raw_mirror_service_ts_t * raw_mirror_ts_p,
                                                                  fbe_u32_t thead_num);
static fbe_status_t fbe_raw_mirror_service_start_io_completion(fbe_packet_t * packet_p, 
                                                               fbe_packet_completion_context_t context);
static fbe_bool_t fbe_raw_mirror_service_ts_is_complete(fbe_raw_mirror_service_ts_t * ts_p);
static fbe_bool_t fbe_raw_mirror_service_destroy_ts(fbe_raw_mirror_service_ts_t * ts_p);
static fbe_status_t fbe_raw_mirror_service_get_block_operation(fbe_raw_mirror_service_io_specification_t * start_io_spec_p,
                                                               fbe_payload_block_operation_opcode_t * operation_p);
static void fbe_raw_mirror_service_clear_statistics(void);
static void fbe_raw_mirror_service_update_statistics(fbe_raw_mirror_service_ts_t *ts_p,
                                                     fbe_payload_block_operation_status_t    block_operation_status,
                                                     fbe_payload_block_operation_qualifier_t block_operation_qualifier);
static void fbe_raw_mirror_service_set_statistics_in_packet(fbe_raw_mirror_service_control_start_io_t *start_io_p,
                                                            fbe_payload_block_operation_status_t    block_operation_status,
                                                            fbe_payload_block_operation_qualifier_t block_operation_qualifier);


/*!**************************************************************
 *  fbe_get_raw_mirror_service()
 ****************************************************************
 * @brief
 *  Get a pointer to the raw mirror service.
 *
 * @param  None.          
 *
 * @return fbe_raw_mirror_service_t* (the raw mirror service).
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
fbe_raw_mirror_service_t *fbe_get_raw_mirror_service(void)
{
    return &fbe_raw_mirror_service;
}
/******************************************
 * end fbe_get_raw_mirror_service()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_init()
 ****************************************************************
 * @brief
 *  Initialize the raw mirror service.  As part of initialization,
 *  the raw mirror itself is created.
 *
 * @param packet_p - The packet sent with the init request.
 *
 * @return fbe_status_t 
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/
static fbe_status_t fbe_raw_mirror_service_init(fbe_packet_t * packet_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_private_space_layout_region_t region;
    fbe_lba_t           raw_mirror_offset;
    fbe_block_count_t   raw_mirror_capacity;


    fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_INFO,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: entry\n", __FUNCTION__);

    /* Set base service ID and trace level 
     */
    fbe_base_service_set_service_id((fbe_base_service_t *) &fbe_raw_mirror_service, FBE_SERVICE_ID_RAW_MIRROR);
    fbe_base_service_set_trace_level((fbe_base_service_t *) &fbe_raw_mirror_service, fbe_trace_get_default_trace_level());

    /* Initialize the raw mirror service
     */
    fbe_base_service_init((fbe_base_service_t *) &fbe_raw_mirror_service);
    fbe_queue_init(&fbe_raw_mirror_service.active_ts_head);
    fbe_spinlock_init(&fbe_raw_mirror_service.lock);
    fbe_raw_mirror_service.packet_p = NULL;
    fbe_raw_mirror_service.active_stop_b = FBE_FALSE;
    fbe_raw_mirror_service.num_threads = 0;
    status = fbe_raw_mirror_service_init_thread_pool();
    if (status != FBE_STATUS_OK) 
    { 
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: error initializing thread pool; status: 0x%x\n", 
                                     __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    /* Get the offset and capacity of the raw mirror.
     * For testing purposes, using the "reserved for future growth" region of the private disks.
     * Therefore not interfering with other data on the database drives used by system LUNs.
     */
    fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_FUTURE_GROWTH_RESERVED, &region);
    raw_mirror_offset      = region.starting_block_address;
    raw_mirror_capacity    = region.size_in_blocks;    

    /* Create the raw mirror
     */
    status = fbe_raw_mirror_init(&fbe_raw_mirror, raw_mirror_offset, 0,  raw_mirror_capacity);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: error creating raw mirror; status: 0x%x\n", 
                                     __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    status = fbe_raw_mirror_init_edges(&fbe_raw_mirror);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: error initing raw mirror edges; status: 0x%x\n", 
                                     __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    /* Complete this request with status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/******************************************
 * end fbe_raw_mirror_service_init()
 ******************************************/

/*!**************************************************************
 * fbe_raw_mirror_service_init_thread_pool()
 ****************************************************************
 * @brief
 *  Allocate the memory for our thread pool.
 *  This allocates memory for one thread per core.
 *
 * @param None.
 *
 * @return fbe_status_t -   FBE_STATUS_OK - Success.
 *                          FBE_STATUS_INSUFFICIENT_RESOURCES - No memory available.
 *
 * @author
 *  11/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/
static fbe_status_t fbe_raw_mirror_service_init_thread_pool(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t cores;
    void *thread_pool_p = NULL;

    cores = fbe_get_cpu_count();

    thread_pool_p = fbe_memory_native_allocate(sizeof(fbe_raw_mirror_service_thread_t) * cores);
    if (thread_pool_p != NULL)
    {
        fbe_raw_mirror_service_set_thread_pool_in_service(thread_pool_p);
    }
    else
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: cannot allocate thread pool\n",__FUNCTION__);
        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}
/******************************************
 * end fbe_raw_mirror_service_init_thread_pool()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_initialize_threads()
 ****************************************************************
 * @brief
 *  Initialize the raw mirror service threads.
 *
 * @param  None.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  11/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/
static fbe_status_t fbe_raw_mirror_service_initialize_threads(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t thread_num;
    fbe_u32_t max_threads;
    fbe_raw_mirror_service_thread_t *thread_p = NULL;


    /* Init one thread per CPU.
     */
	max_threads = fbe_get_cpu_count();
    for (thread_num = 0; thread_num < max_threads; thread_num++)
    {
        fbe_raw_mirror_service_get_thread(thread_num, &thread_p);
        status = fbe_raw_mirror_service_init_thread(thread_p, thread_num);

        if (status != FBE_STATUS_OK)
        {
            fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: error initializing thread number: %d status: 0x%x\n", 
                                         __FUNCTION__, thread_num, status);
            break;
        }
        else
        {
            fbe_raw_mirror_service_inc_num_threads();
        }
    }

    return status;
}
/******************************************
 * end fbe_raw_mirror_service_initialize_threads()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_trace()
 ****************************************************************
 * @brief
 *  Trace function for use by the raw mirror service.
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
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
void fbe_raw_mirror_service_trace(fbe_trace_level_t trace_level,
                                  fbe_trace_message_id_t message_id,
                                  const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    /* Assume we are using the default trace level.
     */
    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();

    /* The global default trace level overrides the service trace level.
     */
    if (fbe_base_service_is_initialized(&fbe_raw_mirror_service.base_service))
    {
        service_trace_level = fbe_base_service_get_trace_level(&fbe_raw_mirror_service.base_service);
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
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_RAW_MIRROR,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
    return;
}
/******************************************
 * end fbe_raw_mirror_service_trace()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_destroy()
 ****************************************************************
 * @brief
 *  Destroy the raw mirror service.
 *  This will fail if I/Os are getting generated.
 *
 * @param packet_p - Packet for the destroy operation.          
 *
 * @return FBE_STATUS_OK - Destroy successful.
 *         FBE_STATUS_GENERIC_FAILURE - Destroy Failed.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
static fbe_status_t fbe_raw_mirror_service_destroy(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raw_mirror_service_t *raw_mirror_service_p = fbe_get_raw_mirror_service();
    fbe_u32_t num_threads = fbe_raw_mirror_service_get_num_threads();
    fbe_u32_t thread_count;


    fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_INFO, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: entry\n", __FUNCTION__);

    /* Destroy the raw mirror thread pool and its threads.
     */
    for (thread_count = 0; thread_count < num_threads; thread_count++)
    {
        status = fbe_raw_mirror_service_destroy_thread(&raw_mirror_service_p->thread_pool[thread_count]);
    }
    fbe_raw_mirror_service_destroy_thread_pool();

    raw_mirror_service_p->num_threads = 0;
    fbe_queue_destroy(&raw_mirror_service_p->active_ts_head);
    fbe_spinlock_destroy(&raw_mirror_service_p->lock);
	fbe_raw_mirror_destroy(&fbe_raw_mirror);

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raw_mirror_service_destroy()
 ******************************************/

/*!**************************************************************
 * fbe_raw_mirror_service_destroy_thread_pool()
 ****************************************************************
 * @brief
 *  Destroy the thread pool memory in the service.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  11/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/
static void fbe_raw_mirror_service_destroy_thread_pool(void)
{
    fbe_raw_mirror_service_t *raw_mirror_service_p = fbe_get_raw_mirror_service();

    fbe_memory_native_release(raw_mirror_service_p->thread_pool);
    raw_mirror_service_p->thread_pool = NULL;

    return;
}
/******************************************
 * end fbe_raw_mirror_service_destroy_thread_pool()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_control_entry()
 ****************************************************************
 * @brief
 *  This is the main entry point for the raw mirror service.
 *
 * @param packet_p - Packet of the control operation.
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_service_control_entry(fbe_packet_t * packet_p)
{    
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;

    control_code = fbe_transport_get_control_code(packet_p);

    /* Handle the init control code first.
     */
    if (control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) 
    {
        status = fbe_raw_mirror_service_init(packet_p);
        return status;
    }

    /* If we are not initialized we do not handle any other control packets.
     */
    if (!fbe_base_service_is_initialized((fbe_base_service_t *) &fbe_raw_mirror_service))
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    /* Handle the remainder of the control packets.
     */
    switch(control_code) 
    {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = fbe_raw_mirror_service_destroy(packet_p);
            break;

        case FBE_RAW_MIRROR_SERVICE_CONTROL_CODE_START_IO:
            status = fbe_raw_mirror_service_start_io(packet_p);
            break;

        case FBE_RAW_MIRROR_SERVICE_CONTROL_CODE_STOP_IO:
            status = fbe_raw_mirror_service_stop_io(packet_p);
            break;

        case FBE_RAW_MIRROR_SERVICE_CONTROL_CODE_PVD_EDGE_ENABLED:
            status = fbe_raw_mirror_service_raw_mirror_pvd_edge_enabled(packet_p);
            break;

        /* By default pass down to our base class to handle the 
         * operations it is aware of such as setting the trace 
         * level, etc. 
         */
        default:
            return fbe_base_service_control_entry((fbe_base_service_t *) &fbe_raw_mirror_service, packet_p);
            break;
    };

    return status;
}
/******************************************
 * end fbe_raw_mirror_service_control_entry()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_start_io()
 ****************************************************************
 * @brief
 *  Send the I/O request to the raw mirror.
 *
 * @param packet_p - Packet for the start I/O operation.          
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/ 
static fbe_status_t fbe_raw_mirror_service_start_io(fbe_packet_t * packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_raw_mirror_service_control_start_io_t * start_io_p = NULL;
    fbe_u32_t                           len;
    fbe_payload_ex_t *                  sep_payload_p = NULL;
    fbe_raw_mirror_service_ts_t       * raw_mirror_ts_p;
    fbe_u32_t                           thread_count;


    /* Get the payload and control operation from incoming packet
     */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &start_io_p);
    if (start_io_p == NULL)
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_raw_mirror_service_control_start_io_t))
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Invalid len %d != %llu \n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_raw_mirror_service_control_start_io_t));

        fbe_transport_set_status(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Clear statistics.
     */
    fbe_raw_mirror_service_clear_statistics();

    /* Set pointer to incoming packet in service.
     */
    fbe_raw_mirror_service_set_test_packet(packet_p);

    /* Allocate memory for our tracking structures; 1 per thread.
     */
    for (thread_count = 0; thread_count < start_io_p->specification.num_threads; thread_count++)
    {
        raw_mirror_ts_p = (fbe_raw_mirror_service_ts_t *)fbe_memory_native_allocate(sizeof(fbe_raw_mirror_service_ts_t));
        if (raw_mirror_ts_p == NULL)
        {
            fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR,
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                         "%s could not allocate memory for packet\n", __FUNCTION__);
    
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
    
        /* Initialize the tracking structure.
         */
        fbe_queue_element_init(&raw_mirror_ts_p->thread_queue_element);
        raw_mirror_ts_p->pass_count = 0;
        raw_mirror_ts_p->start_io_spec.block_count = start_io_p->specification.block_count;
        raw_mirror_ts_p->start_io_spec.block_size = start_io_p->specification.block_size;
        raw_mirror_ts_p->start_io_spec.dmc_expected_b = start_io_p->specification.dmc_expected_b;
        raw_mirror_ts_p->start_io_spec.max_passes = start_io_p->specification.max_passes;
        raw_mirror_ts_p->start_io_spec.num_threads = start_io_p->specification.num_threads;
        raw_mirror_ts_p->start_io_spec.operation = start_io_p->specification.operation;
        raw_mirror_ts_p->start_io_spec.pattern = start_io_p->specification.pattern;
        raw_mirror_ts_p->start_io_spec.sequence_id = start_io_p->specification.sequence_id;
        raw_mirror_ts_p->start_io_spec.sequence_num = start_io_p->specification.sequence_num;
        raw_mirror_ts_p->start_io_spec.start_lba = start_io_p->specification.start_lba;

        /* Put tracking structure on service request queue.
         */
        fbe_raw_mirror_service_request_enqueue(&raw_mirror_ts_p->request_queue_element);

        /* Send the tracking structure to the raw mirror thread for processing.
         */
        status = fbe_raw_mirror_service_send_request_to_thread(raw_mirror_ts_p, thread_count);
    
        if (status != FBE_STATUS_OK)
        {
            fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: error initializing thread status: 0x%x\n", 
                                         __FUNCTION__, status );
            fbe_memory_native_release(raw_mirror_ts_p);
            break;
        }
    }

    if ((status != FBE_STATUS_OK) &&
        (thread_count == 0))
    {
        /* Could not start any threads for this request; complete packet with error.
         */
        fbe_transport_set_status(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        fbe_transport_complete_packet(packet_p);
    }

    return status;
}
/******************************************
 * end fbe_raw_mirror_service_start_io()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_stop_io()
 ****************************************************************
 * @brief
 *  Stop the I/O request to the raw mirror.
 *
 * @param packet_p - Packet for the stop I/O operation.          
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/ 
static fbe_status_t fbe_raw_mirror_service_stop_io(fbe_packet_t * packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_raw_mirror_service_control_start_io_t * start_io_p = NULL;
    fbe_u32_t                           len;
    fbe_payload_ex_t *                  sep_payload_p = NULL;


    /* Get the payload and control operation from incoming packet
     */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &start_io_p);
    if (start_io_p == NULL)
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_raw_mirror_service_control_start_io_t))
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Invalid len %d != %llu \n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_raw_mirror_service_control_start_io_t));

        fbe_transport_set_status(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the stop bit in the service.
     */
    fbe_raw_mirror_service_stop_request();

    /* Complete the packet.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/******************************************
 * end fbe_raw_mirror_service_stop_io()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_raw_mirror_pvd_edge_enabled()
 ****************************************************************
 * @brief
 *  Determine if a PVD edge to the raw mirror is enabled.
 *
 * @param packet_p - Packet for the "PVD edge enabled" operation.          
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/ 
static fbe_status_t fbe_raw_mirror_service_raw_mirror_pvd_edge_enabled(fbe_packet_t * packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_raw_mirror_service_control_start_io_t * start_io_p = NULL;
    fbe_u32_t                           len;
    fbe_payload_ex_t *                  sep_payload_p = NULL;
    fbe_u32_t                           pvd_edge_index;
    fbe_u16_t                           raw_mirror_degraded_bitmask;
    fbe_bool_t                          pvd_degraded_b;


    /* Get the payload and control operation from incoming packet
     */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &start_io_p);
    if (start_io_p == NULL)
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_raw_mirror_service_control_start_io_t))
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Invalid len %d != %llu \n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_raw_mirror_service_control_start_io_t));

        fbe_transport_set_status(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the edge index from the payload.
     */
    pvd_edge_index = start_io_p->pvd_edge_index;

    /* Validate the edge index.
     */
    if (pvd_edge_index > FBE_RAW_MIRROR_WIDTH)
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Invalid PVD edge index %d\n", __FUNCTION__, pvd_edge_index);

        fbe_transport_set_status(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;    
    }

    /* Get the degraded bitmask for the raw mirror.
     */
    fbe_raw_mirror_get_degraded_bitmask(&fbe_raw_mirror, &raw_mirror_degraded_bitmask);

    fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_INFO,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                 "%s: rl_bitmask 0x%x\n", __FUNCTION__, 
                                 raw_mirror_degraded_bitmask);


    /* Determine if edge degraded or not and set boolean in payload accordingly.
     */
    pvd_degraded_b = (raw_mirror_degraded_bitmask & pvd_edge_index);
    if (pvd_degraded_b)
    {
        start_io_p->pvd_edge_enabled_b = FBE_FALSE;
    }
    else
    {
        start_io_p->pvd_edge_enabled_b = FBE_TRUE;
    }

    /* Complete the packet.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/******************************************
 * end fbe_raw_mirror_service_raw_mirror_pvd_edge_enabled()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_send_request_to_thread()
 ****************************************************************
 * @brief
 *  Send the I/O request to the raw mirror thread for processing.
 *
 * @param raw_mirror_ts_p - Pointer to tracking structure for I/O request. 
 * @param thread_num - Which thread to use in thread pool.         
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/ 
static fbe_status_t fbe_raw_mirror_service_send_request_to_thread(fbe_raw_mirror_service_ts_t * raw_mirror_ts_p,
                                                                  fbe_u32_t thead_num)
{
    fbe_status_t status;
    fbe_raw_mirror_service_thread_t *thread_p = NULL;


    /* If we are not yet initialized, threads are not created, and 
     * thus there is no way to send something to the thread. 
     * This might occur if we are being destroyed with I/Os pending. 
     */
    if (!fbe_raw_mirror_service_is_initialized())
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: we are not initialized.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fbe_raw_mirror_service_get_num_threads() == 0)
    {
        /* No threads are running.  Kick them off now.
         */
        status = fbe_raw_mirror_service_initialize_threads();

        if (status != FBE_STATUS_OK)
        {
             fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s: failed to initialize threads\n", __FUNCTION__);
             return status;
        }
    }

    if (fbe_raw_mirror_service_get_num_threads() == 0)
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: fbe_raw_mirror_get_num_threads == 0. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Scale the thread number if needed.
     */
    if (thead_num >= fbe_raw_mirror_service_get_num_threads())
    {
        thead_num = thead_num % fbe_raw_mirror_service_get_num_threads();
    }

    /* Get the thread.
     */
    status = fbe_raw_mirror_service_get_thread(thead_num, &thread_p);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: fbe_raw_mirror_service_get_thread() returned %d\n", __FUNCTION__, status);
        return status; 
    }

    fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_INFO, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: thread: 0x%p, ts: 0x%p\n", __FUNCTION__, thread_p, raw_mirror_ts_p);

    /* Put tracking structure on thread queue.
     */
    fbe_raw_mirror_service_thread_enqueue(thread_p, &raw_mirror_ts_p->thread_queue_element);

    return FBE_STATUS_OK;
 }
/******************************************
 * end fbe_raw_mirror_service_send_request_to_thread()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_request_enqueue()
 ****************************************************************
 * @brief
 *  Enqueue the element on the raw mirror service active request queue.
 *
 * @param queue_element_p - Pointer to element to queue to thread.         
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/ 
static void fbe_raw_mirror_service_request_enqueue(fbe_queue_element_t *queue_element_p)
{
    fbe_raw_mirror_service_t * raw_mirror_service_p = fbe_get_raw_mirror_service();

    /* Prevent things from changing while we are adding to the queue.
     */
    fbe_raw_mirror_service_lock(raw_mirror_service_p);

    /* Put it on the queue.
     */
    fbe_queue_push(&raw_mirror_service_p->active_ts_head, queue_element_p);
    
    /* Unlock since we are done.
     */
    fbe_raw_mirror_service_unlock(raw_mirror_service_p);

    return;
}
/******************************************
 * end fbe_raw_mirror_service_request_enqueue()
 ******************************************/

/*!***************************************************************
 * fbe_raw_mirror_service_ts_thread_queue_element_to_ts_ptr()
 ****************************************************************
 * @brief
 *  Convert from the fbe_raw_mirror_service_ts_t->thread_queue_element
 *  back to the fbe_raw_mirror_service_ts_t *.
 * 
 *  struct fbe_raw_mirror_service_ts_t { <-- We want to return the ptr to here.
 *  ...
 *    thread_queue_element   <-- Ptr to here is passed in.
 *  ...
 *  };
 *
 * @param thread_queue_element_p - This is the thread queue element that we wish to
 *                                 get the ts ptr for.
 *
 * @return fbe_raw_mirror_service_ts_t - The ts that has the input thread queue element.
 *
 * @author
 *  11/2011 - Created from fbe_rdgen_ts_thread_queue_element_to_ts_ptr(). Susan Rundbaken
 *
 ****************************************************************/
fbe_raw_mirror_service_ts_t *fbe_raw_mirror_service_ts_thread_queue_element_to_ts_ptr(fbe_queue_element_t * thread_queue_element_p)
{
    /* We're converting an address to a queue element into an address to the containing
     * raw mirror ts. 
     * In order to do this we need to subtract the offset to the queue element from the 
     * address of the queue element. 
     */
    fbe_raw_mirror_service_ts_t * ts_p;

    ts_p = (fbe_raw_mirror_service_ts_t  *)((fbe_u8_t *)thread_queue_element_p - 
                                            (fbe_u8_t *)(&((fbe_raw_mirror_service_ts_t  *)0)->thread_queue_element));
    return ts_p;
}
/**************************************
 * end fbe_raw_mirror_service_ts_thread_queue_element_to_ts_ptr()
 **************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_start_io_request_packet()
 ****************************************************************
 * @brief
 *  Send I/O to the raw mirror.
 *
 * @param ts_p - Pointer to tracking structure for the start I/O operation.          
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/ 
fbe_status_t fbe_raw_mirror_service_start_io_request_packet(fbe_raw_mirror_service_ts_t * ts_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_packet_t *                      new_io_packet_p = NULL;
    fbe_payload_ex_t *                 new_io_sep_payload_p = NULL;
    fbe_payload_block_operation_t *     new_io_block_operation_p = NULL;
    fbe_block_size_t                    block_size;
    fbe_u32_t                           block_count;
    fbe_payload_block_operation_opcode_t    operation;
    fbe_cpu_id_t                        cpu_id;


    fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: ts: 0x%p\n", __FUNCTION__, ts_p);

    /* Get the block size and block count.
     */
    block_size = ts_p->start_io_spec.block_size;
    block_count = ts_p->start_io_spec.block_count;

    /* Create a new packet for the I/O request.
     */
    new_io_packet_p = (fbe_packet_t *)fbe_memory_native_allocate(sizeof(fbe_packet_t));
    if (new_io_packet_p == NULL)
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s could not allocate memory for packet\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(new_io_packet_p);  
    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(new_io_packet_p, cpu_id);
    fbe_transport_set_packet_attr(new_io_packet_p, FBE_PACKET_FLAG_NO_ATTRIB);
    new_io_sep_payload_p = fbe_transport_get_payload_ex(new_io_packet_p);

    /* Allocate a SG list and set it in payload.
     */
    status = fbe_raw_mirror_service_sg_list_create(new_io_sep_payload_p, &ts_p->start_io_spec);
    if (status != FBE_STATUS_OK)
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s could not create SG list\n", __FUNCTION__);

        fbe_memory_native_release(new_io_packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the block operation for the payload.
     */
    fbe_raw_mirror_service_get_block_operation(&ts_p->start_io_spec, &operation);

    /* Build the block operation.
     */
    new_io_block_operation_p = fbe_payload_ex_allocate_block_operation(new_io_sep_payload_p);
    fbe_payload_block_build_operation(new_io_block_operation_p,
                                      operation,
                                      ts_p->start_io_spec.start_lba,    /* LBA */
                                      block_count,      /* blocks */
                                      block_size,       /* block size */
                                      1,                /* optimum block size */
                                      NULL);            /* preread descriptor */

    fbe_payload_ex_set_verify_error_count_ptr(new_io_sep_payload_p, &ts_p->verify_report);

    /* Increment the tracking structure pass count.
     */
    ts_p->pass_count++;

    /* Set completion function and pointer to tracking structure.
     */
    fbe_transport_set_completion_function(new_io_packet_p, fbe_raw_mirror_service_start_io_completion, ts_p);

    /* Increment the block operation level to make the block operation the active payload.
     */
    fbe_payload_ex_increment_block_operation_level(new_io_sep_payload_p);

    /* Send the new I/O packet to the raw mirror for processing. 
     */
    fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                 "%s send I/O request operation 0x%x\n", __FUNCTION__, operation);

    status = fbe_raw_mirror_send_io_packet_to_raid_library(&fbe_raw_mirror, new_io_packet_p);

    return status;
}
/******************************************
 * end fbe_raw_mirror_service_start_io_request_packet()
 ******************************************/

/*!***************************************************************
 *  fbe_raw_mirror_service_start_io_completion
 ****************************************************************
 * @brief
 *  This is the completion function for a start io request to the raw mirror.
 *
 * @param packet_p - The packet being completed.
 * @param context - The tracking structure being completed.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/
static fbe_status_t 
fbe_raw_mirror_service_start_io_completion(fbe_packet_t * io_packet_p, 
                                           fbe_packet_completion_context_t context)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_packet_t *                          test_packet_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_block_operation_status_t    block_operation_status;
    fbe_payload_block_operation_qualifier_t block_operation_qualifier;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_raw_mirror_service_control_start_io_t *     start_io_p = NULL;
    fbe_sg_element_t *                      sg_list_p = NULL;
    fbe_u32_t                               sg_element_count;
    fbe_raw_mirror_service_ts_t  *          ts_p;


    /* Get the sep payload and block operation from the incoming I/O packet.
     * This is the packet that was sent to RAID.
     */
    sep_payload_p = fbe_transport_get_payload_ex(io_packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

    /* Get the block operation status and qualifier.
     */
    fbe_transport_get_status_code(io_packet_p);
    fbe_payload_block_get_status(block_operation_p, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_operation_qualifier);
    
    /* Get the SG list and count.
     */
    fbe_payload_ex_get_sg_list(sep_payload_p, &sg_list_p, &sg_element_count);

    /* Get the tracking structure pointer from the context.  
     */
    ts_p = (fbe_raw_mirror_service_ts_t *)context;
    if (ts_p == NULL)
    {
        /* Free SG list and packet.
         */
        fbe_raw_mirror_service_sg_list_free(sg_list_p);
        fbe_transport_destroy_packet(io_packet_p);
        fbe_memory_native_release(io_packet_p);

        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now destroy I/O packet.
     */
    fbe_transport_destroy_packet(io_packet_p);
    fbe_memory_native_release(io_packet_p);

    fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: ts: 0x%p\n", __FUNCTION__, ts_p);

    /* If a read-check operation requested, check the data returned.
     */
    if (ts_p->start_io_spec.operation == FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK)
    {  
        status = fbe_raw_mirror_service_sg_list_check(&ts_p->start_io_spec,
                                                      sg_list_p,
                                                      sg_element_count);
    }

    /* Free SG list.
     */
    fbe_raw_mirror_service_sg_list_free(sg_list_p);

    /* Update statistics.
     */
    fbe_raw_mirror_service_update_statistics(ts_p,
                                             block_operation_status,
                                             block_operation_qualifier);

    /* Start again if not complete.
     */
    if (!fbe_raw_mirror_service_ts_is_complete(ts_p))
    {
        fbe_raw_mirror_service_start_io_request_packet(ts_p);
        return FBE_STATUS_OK;
    }

    /* Tracking structure complete; destroy tracking structure.
     * We are done with this request.
     * If the request queue is empty, get the request packet from the service and update it accordingly.
     */
    if (fbe_raw_mirror_service_destroy_ts(ts_p))
    {
        /* Get test packet from raw mirror service.
         */
        test_packet_p = fbe_raw_mirror_service_get_test_packet();
        sep_payload_p = fbe_transport_get_payload_ex(test_packet_p);
        control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
        fbe_payload_control_get_buffer(control_operation_p, &start_io_p);
        if (start_io_p == NULL)
        {
            fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR,
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                         "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
    
            /* Complete test packet with error.
             */
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
            fbe_transport_set_status(test_packet_p, status, 0);
            fbe_transport_complete_packet(test_packet_p);
    
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        /* Set statistics in test packet.
         */
        fbe_raw_mirror_service_set_statistics_in_packet(start_io_p,
                                                        block_operation_status,
                                                        block_operation_qualifier);
        
        /* Complete test packet.
         */
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(test_packet_p, status, 0);
        fbe_transport_complete_packet(test_packet_p);
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raw_mirror_service_start_io_completion
 **************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_ts_is_complete()
 ****************************************************************
 * @brief
 *  Determine if this ts is complete and should exit.
 *
 * @param ts_p - Tracking structure for this I/O request.
 *
 * @return FBE_TRUE if we are done, FBE_FALSE otherwise
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
static fbe_bool_t fbe_raw_mirror_service_ts_is_complete(fbe_raw_mirror_service_ts_t * ts_p)
{
    fbe_u32_t max_passes;

    max_passes = ts_p->start_io_spec.max_passes;

    /* We are complete if the request is stopped in the service.
     */
    if (fbe_raw_mirror_service_is_request_stopped())
    {
        return FBE_TRUE;
    }

    /* We are complete if we finished the expected number of I/Os.
     */
    if ((max_passes != 0) &&
        (ts_p->pass_count >= max_passes))
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/******************************************
 * end fbe_raw_mirror_service_ts_is_complete()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_destroy_ts()
 ****************************************************************
 * @brief
 *  Destroy the tracking structure.  Also remove it from the request queue.
 *
 * @param ts_p - Tracking structure for this I/O request.
 *
 * @return fbe_bool_t if the active queue is empty
 *
 * @author
 *  11/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
static fbe_bool_t fbe_raw_mirror_service_destroy_ts(fbe_raw_mirror_service_ts_t * ts_p)
{
    fbe_bool_t queue_empty;

    fbe_raw_mirror_service_t *raw_mirror_service_p = fbe_get_raw_mirror_service();

    fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_INFO, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: destroy ts: 0x%p\n", __FUNCTION__, ts_p);

    /* Remove tracking structure from service request queue.
     */
    fbe_raw_mirror_service_lock(raw_mirror_service_p);
    fbe_queue_remove(&ts_p->request_queue_element);
    queue_empty = fbe_queue_is_empty(&raw_mirror_service_p->active_ts_head);
    fbe_raw_mirror_service_unlock(raw_mirror_service_p);

    /* Destroy the tracking structure
     */
    fbe_memory_native_release(ts_p);

    return queue_empty;
}
/******************************************
 * end fbe_raw_mirror_service_destroy_ts()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_get_block_operation()
 ****************************************************************
 * @brief
 *  Return the block operation for the given I/O specification.
 *
 * @param start_io_p - I/O specification for start I/O.
 * @param operation_p - pointer to block operation.          
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
static fbe_status_t fbe_raw_mirror_service_get_block_operation(fbe_raw_mirror_service_io_specification_t * start_io_spec_p,
                                                               fbe_payload_block_operation_opcode_t * operation_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    switch (start_io_spec_p->operation)
    {
        case FBE_RAW_MIRROR_SERVICE_OPERATION_READ_ONLY:
        case FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK:
            *operation_p = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ;
            break;
        case FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_ONLY:
            *operation_p = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;
            break;
        case FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_VERIFY:
            *operation_p = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY;
            break;
        default:
            break;
    };

    return status;
}
/******************************************
 * end fbe_raw_mirror_service_get_block_operation()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_clear_statistics()
 ****************************************************************
 * @brief
 *  Initialize the raw mirror service statistics to default values.
 *
 * @param   None.          
 *
 * @return  None.
 *
 * @author
 *  11/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
static void fbe_raw_mirror_service_clear_statistics(void)
{
    fbe_raw_mirror_service_t * raw_mirror_service_p = fbe_get_raw_mirror_service();

    raw_mirror_service_p->stats.total_pass_count = 0;
    raw_mirror_service_p->stats.total_rm_magic_bitmap = 0;
    raw_mirror_service_p->stats.total_rm_seq_bitmap = 0;
    raw_mirror_service_p->stats.total_c_coh_count = 0;
    raw_mirror_service_p->stats.total_c_crc_count = 0;
    raw_mirror_service_p->stats.total_c_crc_multi_count = 0;
    raw_mirror_service_p->stats.total_c_crc_single_count= 0;
    raw_mirror_service_p->stats.total_c_media_count = 0;
    raw_mirror_service_p->stats.total_c_rm_magic_count = 0;
    raw_mirror_service_p->stats.total_c_rm_seq_count = 0;
    raw_mirror_service_p->stats.total_c_soft_media_count = 0;
    raw_mirror_service_p->stats.total_c_ss_count = 0;
    raw_mirror_service_p->stats.total_c_ts_count = 0;
    raw_mirror_service_p->stats.total_c_ws_count = 0;
    raw_mirror_service_p->stats.total_non_retryable_errors = 0;
    raw_mirror_service_p->stats.total_retryable_errors = 0;
    raw_mirror_service_p->stats.total_shutdown_errors = 0;
    raw_mirror_service_p->stats.total_u_coh_count = 0;
    raw_mirror_service_p->stats.total_u_crc_count = 0;
    raw_mirror_service_p->stats.total_u_crc_multi_count = 0;
    raw_mirror_service_p->stats.total_u_crc_single_count = 0;
    raw_mirror_service_p->stats.total_u_media_count = 0;
    raw_mirror_service_p->stats.total_u_rm_magic_count = 0;
    raw_mirror_service_p->stats.total_u_ss_count = 0;
    raw_mirror_service_p->stats.total_u_ts_count = 0;
    raw_mirror_service_p->stats.total_err_count = 0;
    raw_mirror_service_p->stats.total_dead_err_count = 0;

    return;
}
/******************************************
 * end fbe_raw_mirror_service_clear_statistics()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_update_statistics()
 ****************************************************************
 * @brief
 *  Update the raw mirror service statistics.
 *
 * @param   ts_p - Pointer to tracking structure.
 * @param   block_operation_status - Block status from I/O.
 * @param   block_operation_qualifier - Block qualifier from I/O.        
 *
 * @return  None.
 *
 * @author
 *  11/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
static void fbe_raw_mirror_service_update_statistics(fbe_raw_mirror_service_ts_t *ts_p,
                                                     fbe_payload_block_operation_status_t    block_operation_status,
                                                     fbe_payload_block_operation_qualifier_t block_operation_qualifier)
{
    fbe_raw_mirror_service_t * raw_mirror_service_p = fbe_get_raw_mirror_service();

    fbe_raw_mirror_service_lock(raw_mirror_service_p);

    raw_mirror_service_p->stats.total_pass_count += ts_p->pass_count;

    raw_mirror_service_p->stats.total_rm_magic_bitmap |= ts_p->verify_report.raw_mirror_magic_bitmap;
    raw_mirror_service_p->stats.total_rm_seq_bitmap |= ts_p->verify_report.raw_mirror_seq_bitmap;

    raw_mirror_service_p->stats.total_c_coh_count += ts_p->verify_report.verify_errors_counts.c_coh_count;
    raw_mirror_service_p->stats.total_c_crc_count += ts_p->verify_report.verify_errors_counts.c_crc_count;
    raw_mirror_service_p->stats.total_c_crc_multi_count += ts_p->verify_report.verify_errors_counts.c_crc_multi_count;
    raw_mirror_service_p->stats.total_c_crc_single_count += ts_p->verify_report.verify_errors_counts.c_crc_single_count;
    raw_mirror_service_p->stats.total_c_media_count += ts_p->verify_report.verify_errors_counts.c_media_count;
    raw_mirror_service_p->stats.total_c_rm_magic_count += ts_p->verify_report.verify_errors_counts.c_rm_magic_count;
    raw_mirror_service_p->stats.total_c_rm_seq_count += ts_p->verify_report.verify_errors_counts.c_rm_seq_count;
    raw_mirror_service_p->stats.total_c_soft_media_count += ts_p->verify_report.verify_errors_counts.c_soft_media_count;
    raw_mirror_service_p->stats.total_c_ss_count += ts_p->verify_report.verify_errors_counts.c_ss_count;
    raw_mirror_service_p->stats.total_c_ts_count += ts_p->verify_report.verify_errors_counts.c_ts_count;
    raw_mirror_service_p->stats.total_c_ws_count += ts_p->verify_report.verify_errors_counts.c_ws_count;
    raw_mirror_service_p->stats.total_non_retryable_errors += ts_p->verify_report.verify_errors_counts.non_retryable_errors;
    raw_mirror_service_p->stats.total_retryable_errors += ts_p->verify_report.verify_errors_counts.retryable_errors;
    raw_mirror_service_p->stats.total_shutdown_errors += ts_p->verify_report.verify_errors_counts.shutdown_errors;
    raw_mirror_service_p->stats.total_u_coh_count += ts_p->verify_report.verify_errors_counts.u_coh_count;
    raw_mirror_service_p->stats.total_u_crc_count += ts_p->verify_report.verify_errors_counts.u_crc_count;
    raw_mirror_service_p->stats.total_u_crc_multi_count += ts_p->verify_report.verify_errors_counts.u_crc_multi_count;
    raw_mirror_service_p->stats.total_u_crc_single_count += ts_p->verify_report.verify_errors_counts.u_crc_single_count;
    raw_mirror_service_p->stats.total_u_media_count += ts_p->verify_report.verify_errors_counts.u_media_count;
    raw_mirror_service_p->stats.total_u_rm_magic_count += ts_p->verify_report.verify_errors_counts.u_rm_magic_count;
    raw_mirror_service_p->stats.total_u_ss_count += ts_p->verify_report.verify_errors_counts.u_ss_count;
    raw_mirror_service_p->stats.total_u_ts_count += ts_p->verify_report.verify_errors_counts.u_ts_count;
    if (block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED)
    {
        raw_mirror_service_p->stats.total_dead_err_count++;
    }

    raw_mirror_service_p->stats.total_err_count +=  (raw_mirror_service_p->stats.total_c_coh_count +
                                                    raw_mirror_service_p->stats.total_c_crc_count +
                                                    raw_mirror_service_p->stats.total_c_crc_multi_count +
                                                    raw_mirror_service_p->stats.total_c_crc_single_count +
                                                    raw_mirror_service_p->stats.total_c_media_count +
                                                    raw_mirror_service_p->stats.total_c_rm_magic_count +
                                                    raw_mirror_service_p->stats.total_c_rm_seq_count +
                                                    raw_mirror_service_p->stats.total_c_soft_media_count +
                                                    raw_mirror_service_p->stats.total_c_ss_count +
                                                    raw_mirror_service_p->stats.total_c_ts_count +
                                                    raw_mirror_service_p->stats.total_c_ws_count +
                                                    raw_mirror_service_p->stats.total_dead_err_count +
                                                    raw_mirror_service_p->stats.total_non_retryable_errors +
                                                    raw_mirror_service_p->stats.total_retryable_errors +
                                                    raw_mirror_service_p->stats.total_shutdown_errors +
                                                    raw_mirror_service_p->stats.total_u_coh_count +
                                                    raw_mirror_service_p->stats.total_u_crc_count +
                                                    raw_mirror_service_p->stats.total_u_crc_multi_count +
                                                    raw_mirror_service_p->stats.total_u_crc_single_count +
                                                    raw_mirror_service_p->stats.total_u_media_count +
                                                    raw_mirror_service_p->stats.total_u_rm_magic_count +
                                                    raw_mirror_service_p->stats.total_u_ss_count +
                                                    raw_mirror_service_p->stats.total_u_ts_count +
                                                    raw_mirror_service_p->stats.total_u_ws_count);
                                         
    fbe_raw_mirror_service_unlock(raw_mirror_service_p);

    if (block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED)
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_INFO,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                     "%s block operation status: 0x%x; block operation qualifier: 0x%x\n", 
                                     __FUNCTION__,
                                     block_operation_status,
                                     block_operation_qualifier);
    }

    return;
}
/******************************************
 * end fbe_raw_mirror_service_update_statistics()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_set_statistics_in_packet()
 ****************************************************************
 * @brief
 *  Set the raw mirror service statistics in control operation of test packet.
 *
 * @param   start_io_p - Pointer to packet control operation structure.
 * @param   block_operation_status - Block status from I/O.
 * @param   block_operation_qualifier - Block qualifier from I/O.
 *
 * @return  None.
 *
 * @author
 *  11/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
static void fbe_raw_mirror_service_set_statistics_in_packet(fbe_raw_mirror_service_control_start_io_t *start_io_p,
                                                            fbe_payload_block_operation_status_t    block_operation_status,
                                                            fbe_payload_block_operation_qualifier_t block_operation_qualifier)
{
    fbe_raw_mirror_service_t * raw_mirror_service_p = fbe_get_raw_mirror_service();

    start_io_p->block_status    = block_operation_status;       /* Will be status and qualifier from last I/O. Useful if max_passes = 1. */
    start_io_p->block_qualifier = block_operation_qualifier;

    start_io_p->io_count        = raw_mirror_service_p->stats.total_pass_count;
    start_io_p->err_count       = raw_mirror_service_p->stats.total_err_count;
    start_io_p->dead_err_count  = raw_mirror_service_p->stats.total_dead_err_count;

    start_io_p->verify_errors.raw_mirror_magic_bitmap                   = raw_mirror_service_p->stats.total_rm_magic_bitmap;
    start_io_p->verify_errors.raw_mirror_seq_bitmap                     = raw_mirror_service_p->stats.total_rm_seq_bitmap;

    start_io_p->verify_errors.verify_errors_counts.c_coh_count          = raw_mirror_service_p->stats.total_c_coh_count;
    start_io_p->verify_errors.verify_errors_counts.c_crc_count          = raw_mirror_service_p->stats.total_c_crc_count;
    start_io_p->verify_errors.verify_errors_counts.c_crc_multi_count    = raw_mirror_service_p->stats.total_c_crc_multi_count;
    start_io_p->verify_errors.verify_errors_counts.c_crc_single_count   = raw_mirror_service_p->stats.total_c_crc_single_count;
    start_io_p->verify_errors.verify_errors_counts.c_media_count        = raw_mirror_service_p->stats.total_c_media_count;
    start_io_p->verify_errors.verify_errors_counts.c_rm_magic_count     = raw_mirror_service_p->stats.total_c_rm_magic_count;
    start_io_p->verify_errors.verify_errors_counts.c_rm_seq_count       = raw_mirror_service_p->stats.total_c_rm_seq_count;
    start_io_p->verify_errors.verify_errors_counts.c_soft_media_count   = raw_mirror_service_p->stats.total_c_soft_media_count;
    start_io_p->verify_errors.verify_errors_counts.c_ss_count           = raw_mirror_service_p->stats.total_c_ss_count;
    start_io_p->verify_errors.verify_errors_counts.c_ts_count           = raw_mirror_service_p->stats.total_c_ts_count;
    start_io_p->verify_errors.verify_errors_counts.c_ws_count           = raw_mirror_service_p->stats.total_c_ws_count;
    start_io_p->verify_errors.verify_errors_counts.non_retryable_errors = raw_mirror_service_p->stats.total_non_retryable_errors;
    start_io_p->verify_errors.verify_errors_counts.retryable_errors     = raw_mirror_service_p->stats.total_retryable_errors;
    start_io_p->verify_errors.verify_errors_counts.shutdown_errors      = raw_mirror_service_p->stats.total_shutdown_errors;
    start_io_p->verify_errors.verify_errors_counts.u_coh_count          = raw_mirror_service_p->stats.total_u_coh_count;
    start_io_p->verify_errors.verify_errors_counts.u_crc_count          = raw_mirror_service_p->stats.total_u_crc_count;
    start_io_p->verify_errors.verify_errors_counts.u_crc_multi_count    = raw_mirror_service_p->stats.total_u_crc_multi_count;
    start_io_p->verify_errors.verify_errors_counts.u_crc_single_count   = raw_mirror_service_p->stats.total_u_crc_single_count;
    start_io_p->verify_errors.verify_errors_counts.u_media_count        = raw_mirror_service_p->stats.total_u_media_count;
    start_io_p->verify_errors.verify_errors_counts.u_rm_magic_count     = raw_mirror_service_p->stats.total_u_rm_magic_count;
    start_io_p->verify_errors.verify_errors_counts.u_ss_count           = raw_mirror_service_p->stats.total_u_ss_count;
    start_io_p->verify_errors.verify_errors_counts.u_ts_count           = raw_mirror_service_p->stats.total_u_ts_count;
    start_io_p->verify_errors.verify_errors_counts.u_ws_count           = raw_mirror_service_p->stats.total_u_ws_count;

    return;
}
/******************************************
 * end fbe_raw_mirror_service_set_statistics_in_packet()
 ******************************************/


/*************************
 * end file fbe_raw_mirror_service_main.c
 *************************/

