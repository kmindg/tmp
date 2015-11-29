/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_logical_error_injection_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry point for the logical_error_injection service.
 *
 * @version
 *  12/16/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_logical_error_injection_private.h"
#include "fbe_transport_memory.h"
#include "fbe_logical_error_injection_proto.h"


/* Declare our service methods */
fbe_status_t fbe_logical_error_injection_service_control_entry(fbe_packet_t * packet); 
fbe_service_methods_t fbe_logical_error_injection_service_methods = {FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION, fbe_logical_error_injection_service_control_entry};

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t fbe_logical_error_injection_service_enable(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_disable(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_enable_poc(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_disable_poc(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_modify_objects(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_enable_objects(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_disable_objects(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_get_stats(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_get_object_stats(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_create_record(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_delete_record(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_modify_record(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_get_table_info(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_get_active_table_info(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_get_records(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_load_table(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_disable_records(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_destroy_objects(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_destroy_objects(void);
static fbe_status_t fbe_logical_error_injection_service_enable_ignore_corrupr_crc_data_errs(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_disable_ignore_corrupr_crc_data_errs(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_remove_object(fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_error_injection_service_disable_lba_normalize(fbe_packet_t * packet_p);


/*!*******************************************************************
 * @var fbe_logical_error_injection_service
 *********************************************************************
 * @brief This is the global definition of the logical error injection
 *        service.
 *
 *********************************************************************/
static fbe_logical_error_injection_service_t fbe_logical_error_injection_service;

/*!**************************************************************
 * fbe_get_logical_error_injection_service()
 ****************************************************************
 * @brief
 *  Returns the global logical_error_injection service ptr.
 *
 * @param None             
 *
 * @return - The global logical_error_injection service.  
 *
 ****************************************************************/

fbe_logical_error_injection_service_t *fbe_get_logical_error_injection_service(void)
{
    return &fbe_logical_error_injection_service;
}
/******************************************
 * end fbe_get_logical_error_injection_service()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_init()
 ****************************************************************
 * @brief
 *  Initialize the logical_error_injection service.
 *
 * @param packet_p - The packet sent with the init request.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_init(fbe_packet_t * packet_p)
{
    fbe_zero_memory(&fbe_logical_error_injection_service, sizeof(fbe_logical_error_injection_service_t));
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);
    
    fbe_base_service_set_service_id((fbe_base_service_t *) &fbe_logical_error_injection_service, FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION);
    fbe_base_service_set_trace_level((fbe_base_service_t *) &fbe_logical_error_injection_service, fbe_trace_get_default_trace_level());

    fbe_base_service_init((fbe_base_service_t *) &fbe_logical_error_injection_service);

    fbe_queue_init(&fbe_logical_error_injection_service.active_object_head);
    //fbe_queue_init(&fbe_logical_error_injection_service.active_record_head);
    fbe_spinlock_init(&fbe_logical_error_injection_service.lock);

    fbe_logical_error_injection_set_seconds_per_trace(0);
    fbe_logical_error_injection_set_last_trace_time(0);
    fbe_logical_error_injection_set_trace_per_pass_count(0);
    fbe_logical_error_injection_set_trace_per_io_count(50);
    fbe_logical_error_injection_set_seconds_per_trace(5);

    /* Init table flags to the default.  This allows new records to get added and
     * activated. 
     */
    fbe_logical_error_injection_set_table_flags((FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_CORRECTABLE_TABLE |
                                                 FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_ALL_RAID_TYPES      ));    
    fbe_zero_memory(&fbe_logical_error_injection_service.table_desc[0],
                    sizeof(fbe_logical_error_injection_table_description_t));

    /* Initialize the delay thread
     */
    fbe_logical_error_injection_delay_thread_init(&fbe_logical_error_injection_service.delay_thread);

    /* Always complete this request with good status.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_init()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_trace()
 ****************************************************************
 * @brief
 *  Trace function for use by the logical_error_injection service.
 *  Takes into account the current global trace level and
 *  the locally set trace level.
 *
 * @param trace_level - trace level of this message.
 * @param message_id - generic identifier.
 * @param fmt... - variable argument list starting with format.
 *
 * @return None.  
 *
 ****************************************************************/

void fbe_logical_error_injection_service_trace(fbe_trace_level_t trace_level,
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
    if (fbe_base_service_is_initialized(&fbe_logical_error_injection_service.base_service))
    {
        service_trace_level = fbe_base_service_get_trace_level(&fbe_logical_error_injection_service.base_service);
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
                     FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
    return;
}
/******************************************
 * end fbe_logical_error_injection_service_trace()
 ******************************************/
/*!**************************************************************
 * fbe_logical_error_injection_destroy_object()
 ****************************************************************
 * @brief
 *  Destroy the specified object.
 *
 * @param object_p            
 *
 * @return fbe_status_t - Status of the destroy.
 *
 * @author
 *  09/07/2012  Ron Proulx  - Created.
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_destroy_object(fbe_logical_error_injection_object_t *object_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Prevent things from changing while we are processing the queue.
     */
    fbe_logical_error_injection_lock();
    
    /* Simply destroy the object.
     */
    status = fbe_logical_error_injection_object_disable(object_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s status 0x%x from disable object id: 0x%x\n", 
                                                  __FUNCTION__, status, object_p->object_id);
    }

    /* Make sure injection is not running while we are destroying this object.
     */
    fbe_logical_error_injection_object_lock(object_p);
    status = fbe_logical_error_injection_object_prepare_destroy(object_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s status 0x%x from prepare destroy object id: 0x%x\n", 
                                                  __FUNCTION__, status, object_p->object_id);
    }
    fbe_queue_remove((fbe_queue_element_t *)object_p);
    fbe_logical_error_injection_dec_num_objects();

    /* We already took the object off the queue, so no-one
     * can get a reference to the object. 
     * Unlock the object so we can destroy it. 
     */
    fbe_logical_error_injection_object_unlock(object_p);
    fbe_logical_error_injection_object_destroy(object_p);
    fbe_memory_native_release(object_p);

    /* Unlock since we are done.
     */
    fbe_logical_error_injection_unlock();

    return status;
}
/******************************************
 * end fbe_logical_error_injection_destroy_object
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_destroy_objects()
 ****************************************************************
 * @brief
 *  Destroy all the logical_error_injection objects.
 *
 * @param None.             
 *
 * @return fbe_status_t - Status of the destroy.
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_destroy_objects(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_logical_error_injection_object_t *object_p = NULL;
    fbe_u32_t iterations = 0;

    /* Prevent things from changing while we are processing the queue.
     */
    fbe_logical_error_injection_lock();
    
    /* Simply drain the queue of objects, destroying each item Until there are no more
     * items. 
     */
    while (fbe_queue_is_empty(&fbe_logical_error_injection_service.active_object_head) == FBE_FALSE)
    {
        queue_element_p = fbe_queue_front(&fbe_logical_error_injection_service.active_object_head);
        object_p = (fbe_logical_error_injection_object_t *)queue_element_p;

        status = fbe_logical_error_injection_object_disable(object_p);

        if (status != FBE_STATUS_OK) 
        { 
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                      FBE_TRACE_MESSAGE_ID_INFO,
                                                      "%s status 0x%x from disable object id: 0x%x\n", 
                                                      __FUNCTION__, status, object_p->object_id);
        }
        /* Make sure injection is not running while we are destroying this object.
         */
        fbe_logical_error_injection_object_lock(object_p);
        status = fbe_logical_error_injection_object_prepare_destroy(object_p);

        if (status != FBE_STATUS_OK) 
        { 
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                      FBE_TRACE_MESSAGE_ID_INFO,
                                                      "%s status 0x%x from prepare destroy object id: 0x%x\n", 
                                                      __FUNCTION__, status, object_p->object_id);
        }
        fbe_queue_remove(queue_element_p);
        fbe_logical_error_injection_dec_num_objects();

        /* We already took the object off the queue, so no-one
         * can get a reference to the object. 
         * Unlock the object so we can destroy it. 
         */
        fbe_logical_error_injection_object_unlock(object_p);
        fbe_logical_error_injection_unlock();
        fbe_logical_error_injection_object_destroy(object_p);
        fbe_memory_native_release(object_p);

        /* Re-lock so we can check the queue.
         */
        fbe_logical_error_injection_lock();
        if (iterations++ > 0x00ffffff)
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                      FBE_TRACE_MESSAGE_ID_INFO,
                                                      "%s error looped queue\n", __FUNCTION__);          
            break;
        }
    }

    /* Unlock since we are done.
     */
    fbe_logical_error_injection_unlock();

    return status;
}
/******************************************
 * end fbe_logical_error_injection_destroy_objects()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_destroy()
 ****************************************************************
 * @brief
 *  Destroy the logical_error_injection service.
 *  This will fail if I/Os are getting generated.
 *
 * @param packet_p - Packet for the destroy operation.          
 *
 * @return FBE_STATUS_OK - Destroy successful.
 *         FBE_STATUS_GENERIC_FAILURE - Destroy Failed.
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_destroy(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    status = fbe_logical_error_injection_destroy_objects();

    fbe_logical_error_injection_service_destroy_threads();
    /*! @todo We should first drain I/O. 
     * For now, fail if we are running I/O. 
     */
    fbe_logical_error_injection_lock();
    /*! @todo tear down lists. 
     */
    fbe_logical_error_injection_unlock();

	fbe_spinlock_destroy(&fbe_logical_error_injection_service.lock);

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return status;
}
/******************************************
 * end fbe_logical_error_injection_service_destroy()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_control_entry()
 ****************************************************************
 * @brief
 *  This is the main entry point for the logical_error_injection service.
 *
 * @param packet_p - Packet of the control operation.
 *
 * @return fbe_status_t.
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_service_control_entry(fbe_packet_t * packet_p)
{    
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;

    control_code = fbe_transport_get_control_code(packet_p);

    /* Handle the init control code first.
     */
    if (control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) 
    {
        status = fbe_logical_error_injection_service_init(packet_p);
        return status;
    }

    /* If we are not initialized we do not handle any other control packets.
     */
    if (!fbe_base_service_is_initialized((fbe_base_service_t *) &fbe_logical_error_injection_service))
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    /* Handle the remainder of the control packets.
     */
    switch(control_code) 
    {
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ENABLE:
            fbe_logical_error_injection_service_enable(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE:
            fbe_logical_error_injection_service_disable(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ENABLE_POC:
            fbe_logical_error_injection_service_enable_poc(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_POC:
            fbe_logical_error_injection_service_disable_poc(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_MODIFY_OBJECTS:
            fbe_logical_error_injection_service_modify_objects(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ENABLE_OBJECTS:
            fbe_logical_error_injection_service_enable_objects(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_OBJECTS:
            fbe_logical_error_injection_service_disable_objects(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DESTROY_OBJECTS:
            fbe_logical_error_injection_service_destroy_objects(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_STATS:
            fbe_logical_error_injection_service_get_stats(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_OBJECT_STATS:
            fbe_logical_error_injection_service_get_object_stats(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_CREATE_RECORD:
            fbe_logical_error_injection_service_create_record(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DELETE_RECORD:
            fbe_logical_error_injection_service_delete_record(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_MODIFY_RECORD:
            fbe_logical_error_injection_service_modify_record(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_TABLE_INFO:
            fbe_logical_error_injection_service_get_table_info(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_ACTIVE_TABLE_INFO:
            fbe_logical_error_injection_service_get_active_table_info(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_RECORDS:
            fbe_logical_error_injection_service_get_records(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_LOAD_TABLE:
            fbe_logical_error_injection_service_load_table(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_RECORDS:
            fbe_logical_error_injection_service_disable_records(packet_p);
            break;
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = fbe_logical_error_injection_service_destroy(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ENABLE_IGNORE_CORRUPT_CRC_DATA_ERRORS:
            fbe_logical_error_injection_service_enable_ignore_corrupr_crc_data_errs(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_IGNORE_CORRUPT_CRC_DATA_ERRORS:
            fbe_logical_error_injection_service_disable_ignore_corrupr_crc_data_errs(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_REMOVE_OBJECT:
            fbe_logical_error_injection_service_remove_object(packet_p);
            break;
        case FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_LBA_NORMALIZE:
            fbe_logical_error_injection_service_disable_lba_normalize(packet_p);
            break;

            /* By default pass down to our base class to handle the 
             * operations it is aware of such as setting the trace 
             * level, etc. 
             */
        default:
            return fbe_base_service_control_entry((fbe_base_service_t *) &fbe_logical_error_injection_service, packet_p);
            break;
    };

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_control_entry()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_enable()
 ****************************************************************
 * @brief
 *  enable the logical_error_injection service.
 *  This will fail if I/Os are getting generated.
 *
 * @param packet_p - Packet for the enable operation.          
 *
 * @return FBE_STATUS_OK - enable successful.
 *         FBE_STATUS_GENERIC_FAILURE - enable Failed.
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_enable(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_initialized;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    fbe_logical_error_injection_get_initialized(&b_initialized);

    /*! @todo Don't allow injection if we are not initialized.
     */
    if (0 && b_initialized == FBE_FALSE)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  "%s: trying to enable an uninitialized table.\n", __FUNCTION__);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
     /* First make sure that the logging is initialized.
     */
    if (fbe_logical_error_injection_service.log_p == NULL)
    {
        //rg_error_sim_init_log();
    }
    fbe_logical_error_injection_lock();

    /* We need to clear out our counters prior to enabling so that 
     * we get an accurate count of errors encountered with this table. 
     */
    fbe_logical_error_injection_set_num_errors_injected(0);
    fbe_logical_error_injection_set_correctable_errors_detected(0);
    fbe_logical_error_injection_set_uncorrectable_errors_detected(0);
    fbe_logical_error_injection_set_num_validations(0);
    fbe_logical_error_injection_set_num_failed_validations(0);

    /* Validate and randomize the table.
     */
    status = fbe_logical_error_injection_validate_and_randomize_table();
    if (status == FBE_STATUS_OK)
    {
        /*! enable the service injection of errors.
         */
        fbe_logical_error_injection_enable();
    }
    fbe_logical_error_injection_unlock();

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_enable()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_disable()
 ****************************************************************
 * @brief
 *  disable the logical_error_injection service.
 *  This will fail if I/Os are getting generated.
 *
 * @param packet_p - Packet for the disable operation.          
 *
 * @return FBE_STATUS_OK - disable successful.
 *         FBE_STATUS_GENERIC_FAILURE - disable Failed.
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_disable(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    /*! Disable the service from injecting errors.
     */
    fbe_logical_error_injection_lock();
    fbe_logical_error_injection_disable();
    fbe_logical_error_injection_wakeup_delay_thread();
    fbe_logical_error_injection_unlock();

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_disable()
 ******************************************/



/*!**************************************************************
 * fbe_logical_error_injection_service_enable_poc()
 ****************************************************************
 * @brief
 *  enable the POC injection
 *  This will fail if I/Os are getting generated.
 *
 * @param packet_p - Packet for the POC enable operation.          
 *
 * @return FBE_STATUS_OK - enable successful.
 *         FBE_STATUS_GENERIC_FAILURE - enable Failed.
 *
 * @author
 *  02/10/2011 - Created. Jyoti Ranjan
 *
 ****************************************************************/
static fbe_status_t fbe_logical_error_injection_service_enable_poc(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_initialized;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    fbe_logical_error_injection_get_initialized(&b_initialized);

    /*! @todo Don't allow POC injection if we are not initialized.
     */
    if (0 && b_initialized == FBE_FALSE)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  "%s: trying to enable an uninitialized table.\n", __FUNCTION__);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_logical_error_injection_lock();
    fbe_logical_error_injection_set_poc_injection(FBE_TRUE);
    fbe_logical_error_injection_unlock();

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/*******************************************************
 * end fbe_logical_error_injection_service_enable_poc()
 ******************************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_disable_poc()
 ****************************************************************
 * @brief
 *  disable the POC injection
 *  This will fail if I/Os are getting generated.
 *
 * @param packet_p - Packet for the disable POC operation.          
 *
 * @return FBE_STATUS_OK - disable successful.
 *         FBE_STATUS_GENERIC_FAILURE - disable Failed.
 *
 * @author
 *  02/10/2011 - Created. Jyoti Ranjan
 *
 ****************************************************************/
static fbe_status_t fbe_logical_error_injection_service_disable_poc(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    /*! Disable the service from injecting errors.
     */
    fbe_logical_error_injection_lock();
    fbe_logical_error_injection_set_poc_injection(FBE_FALSE);
    fbe_logical_error_injection_unlock();

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/*******************************************************
 * end fbe_logical_error_injection_service_disable_poc()
 *******************************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_enable_ignore_corrupr_crc_data_errs()
 ****************************************************************
 * @brief
 *  enable the ignore crc and data erorrs flag
 *
 * @param packet_p - Packet for the ignore errors operation.          
 *
 * @return FBE_STATUS_OK - enable successful.
 *         FBE_STATUS_GENERIC_FAILURE - enable Failed.
 *
 * @author
 *  04/21/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
static fbe_status_t fbe_logical_error_injection_service_enable_ignore_corrupr_crc_data_errs(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_initialized;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    fbe_logical_error_injection_get_initialized(&b_initialized);

    
    if (0 && b_initialized == FBE_FALSE)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  "%s: trying to enable an uninitialized table.\n", __FUNCTION__);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_logical_error_injection_lock();
    fbe_logical_error_injection_set_ignore_corrupt_crc_data_errs(FBE_TRUE);
    fbe_logical_error_injection_unlock();

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_logical_error_injection_service_enable_ignore_corrupr_crc_data_errs()
 ****************************************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_disable_ignore_corrupr_crc_data_errs()
 ****************************************************************
 * @brief
 *  disable the ignore crc and data erorrs flag
 *
 * @param packet_p - Packet for the disable ignore error operation.          
 *
 * @return FBE_STATUS_OK - disable successful.
 *         FBE_STATUS_GENERIC_FAILURE - disable Failed.
 *
 * @author
 *  04/21/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
static fbe_status_t fbe_logical_error_injection_service_disable_ignore_corrupr_crc_data_errs(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    /*! Disable the service from injecting errors.
     */
    fbe_logical_error_injection_lock();
    fbe_logical_error_injection_set_ignore_corrupt_crc_data_errs(FBE_FALSE);
    fbe_logical_error_injection_unlock();

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/*****************************************************************
 * end fbe_logical_error_injection_service_disable_ignore_corrupr_crc_data_errs()
 *****************************************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_modify_objects()
 ****************************************************************
 * @brief   modify the error injection object as specified:
 *  enable the edge hook for the bitmask of positions specified.
 *  By default all valid edges will have the edge hook added.
 *  Update `injection_lba_adjustment' in the object.
 *
 * @param packet_p - Packet for the enable operation.          
 *
 * @return FBE_STATUS_OK - enable successful.
 *         FBE_STATUS_GENERIC_FAILURE - enable Failed.
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_modify_objects(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_logical_error_injection_modify_objects_t *modify_objects_p = NULL;
    fbe_u32_t len;
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &modify_objects_p);
    if (modify_objects_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_logical_error_injection_modify_objects_t))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %d\n", __FUNCTION__, len, (int)sizeof(fbe_logical_error_injection_modify_objects_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    if (modify_objects_p->filter.object_id != FBE_OBJECT_ID_INVALID)
    {
        fbe_logical_error_injection_object_t *object_p = NULL;
        /* Enabling on just a single object.
         */
        status = fbe_logical_error_injection_get_object(modify_objects_p->filter.object_id,
                                                        modify_objects_p->filter.package_id,
                                                        &object_p);
        if ((status != FBE_STATUS_OK) || (object_p == NULL))
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                      "%s unable to get object status %d\n", __FUNCTION__, status);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        status = fbe_logical_error_injection_object_modify(object_p, 
                                                           modify_objects_p->edge_hook_enable_bitmask,
                                                           modify_objects_p->lba_injection_adjustment);
    }
    else
    {
        /* Otherwise enabling on a class of objects.
         */
        status = fbe_logical_error_injection_object_modify_for_class(&modify_objects_p->filter, 
                                                                     modify_objects_p->edge_hook_enable_bitmask,
                                                                     modify_objects_p->lba_injection_adjustment);
    }
    fbe_logical_error_injection_lock();
    /*! enable the service injection of errors for a given set of objects.
     */
    fbe_logical_error_injection_unlock();

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_logical_error_injection_service_modify_objects()
 **********************************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_enable_objects()
 ****************************************************************
 * @brief
 *  enable the logical_error_injection service.
 *  This will fail if I/Os are getting generated.
 *
 * @param packet_p - Packet for the enable operation.          
 *
 * @return FBE_STATUS_OK - enable successful.
 *         FBE_STATUS_GENERIC_FAILURE - enable Failed.
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_enable_objects(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_logical_error_injection_enable_objects_t * enable_p = NULL;
    fbe_u32_t len;
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &enable_p);
    if (enable_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_logical_error_injection_enable_objects_t))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu\n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_logical_error_injection_enable_objects_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    if (enable_p->filter.object_id != FBE_OBJECT_ID_INVALID)
    {
        fbe_logical_error_injection_object_t *object_p = NULL;
        /* Enabling on just a single object.
         */
        status = fbe_logical_error_injection_get_object(enable_p->filter.object_id,
                                                        enable_p->filter.package_id,
                                                        &object_p);
        if ((status != FBE_STATUS_OK) || (object_p == NULL))
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                      "%s unable to get object status %d\n", __FUNCTION__, status);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        status = fbe_logical_error_injection_object_enable(object_p);
    }
    else
    {
        /* Otherwise enabling on a class of objects.
         */
        status = fbe_logical_error_injection_object_enable_class(&enable_p->filter);
    }
    fbe_logical_error_injection_lock();
    /*! enable the service injection of errors for a given set of objects.
     */
    fbe_logical_error_injection_unlock();

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_enable_objects()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_disable_objects()
 ****************************************************************
 * @brief
 *  disable the logical_error_injection service.
 *  This will fail if I/Os are getting generated.
 *
 * @param packet_p - Packet for the disable operation.          
 *
 * @return FBE_STATUS_OK - disable successful.
 *         FBE_STATUS_GENERIC_FAILURE - disable Failed.
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_disable_objects(fbe_packet_t * packet_p)
{
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_logical_error_injection_disable_objects_t * disable_p = NULL;
    fbe_u32_t len;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &disable_p);
    if (disable_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_logical_error_injection_disable_objects_t))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu\n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_logical_error_injection_disable_objects_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (disable_p->filter.object_id != FBE_OBJECT_ID_INVALID)
    {
        fbe_logical_error_injection_object_t *object_p = NULL;
        /* Enabling on just a single object.
         */
        status = fbe_logical_error_injection_get_object(disable_p->filter.object_id,
                                                        disable_p->filter.package_id,
                                                        &object_p);
        if ((status != FBE_STATUS_OK) || (object_p == NULL))
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                      "%s unable to get object status %d\n", __FUNCTION__, status);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        status = fbe_logical_error_injection_object_disable(object_p);
    }
    else
    {
        /* Otherwise enabling on a class of objects.
         */
        status = fbe_logical_error_injection_object_disable_class(&disable_p->filter);
    }

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_disable_objects()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_destroy_objects()
 ****************************************************************
 * @brief
 *  Destroy the objects.
 *
 * @param packet_p - Packet for the operation.          
 *
 * @return FBE_STATUS_OK - successful.
 *         FBE_STATUS_GENERIC_FAILURE - Failed.
 *
 * @author
 *  8/31/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_destroy_objects(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    /* Eliminate all the objects. */
    status = fbe_logical_error_injection_destroy_objects();

    /* Complete the packet with the status from above. */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed. */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_destroy_objects()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_get_stats()
 ****************************************************************
 * @brief
 *  Get overall service stats.
 *
 * @param packet_p - Packet.          
 *
 * @return FBE_STATUS_OK - successful.
 *         FBE_STATUS_GENERIC_FAILURE - Failed.
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_get_stats(fbe_packet_t * packet_p)
{
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_logical_error_injection_get_stats_t * get_stats_p = NULL;
    fbe_u32_t len;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &get_stats_p);
    if (get_stats_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_logical_error_injection_get_stats_t))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu\n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_logical_error_injection_get_stats_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Disable the service from injecting errors.
     */
    fbe_logical_error_injection_lock();
    fbe_logical_error_injection_get_enable(&get_stats_p->b_enabled);
    fbe_logical_error_injection_get_num_objects(&get_stats_p->num_objects);
    fbe_logical_error_injection_get_num_errors_injected(&get_stats_p->num_errors_injected);
    fbe_logical_error_injection_get_correctable_errors_detected(&get_stats_p->correctable_errors_detected);
    fbe_logical_error_injection_get_uncorrectable_errors_detected(&get_stats_p->uncorrectable_errors_detected);
    fbe_logical_error_injection_get_num_failed_validations(&get_stats_p->num_failed_validations);
    fbe_logical_error_injection_get_num_validations(&get_stats_p->num_validations);
    fbe_logical_error_injection_get_num_records(&get_stats_p->num_records);
    fbe_logical_error_injection_get_num_objects_enabled(&get_stats_p->num_objects_enabled);
    fbe_logical_error_injection_unlock();

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_get_stats()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_get_object_stats()
 ****************************************************************
 * @brief
 *  Get stats for this object.
 *
 * @param packet_p - Packet for the disable operation.          
 *
 * @return FBE_STATUS_OK - successful.
 *         FBE_STATUS_GENERIC_FAILURE - Failed.
 *
 * @author
 *  2/8/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_get_object_stats(fbe_packet_t * packet_p)
{
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_logical_error_injection_get_object_stats_t * get_object_stats_p = NULL;
    fbe_u32_t len;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_error_injection_object_t *object_p = NULL;
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);


    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &get_object_stats_p);
    if (get_object_stats_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_logical_error_injection_get_object_stats_t))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu\n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_logical_error_injection_get_object_stats_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Check if we are even injecting errors on this object id.
     */
    object_p = fbe_logical_error_injection_find_object(get_object_stats_p->object_id);

    if (object_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                  "%s unable to get object status %d\n", __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_logical_error_injection_lock();
    fbe_logical_error_injection_object_lock(object_p);
    fbe_logical_error_injection_object_get_num_rd_media_errors(object_p, &get_object_stats_p->num_read_media_errors_injected);
    fbe_logical_error_injection_object_get_num_wr_remaps(object_p, &get_object_stats_p->num_write_verify_blocks_remapped);
    fbe_logical_error_injection_object_get_num_errors(object_p, &get_object_stats_p->num_errors_injected);
    fbe_logical_error_injection_object_get_num_validations(object_p, &get_object_stats_p->num_validations);
    fbe_logical_error_injection_object_unlock(object_p);
    fbe_logical_error_injection_unlock();

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_get_object_stats()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_get_record_at_index()
 ****************************************************************
 * @brief
 *  Get a record based on a structure the user passed in.
 *
 * @param None.
 *
 * @return fbe_logical_error_injection_record_t *
 *
 * @author
 *  9/21/2010 - Created. Swati Fursule
 *
 ****************************************************************/

fbe_logical_error_injection_record_t * fbe_logical_error_injection_get_record_at_index(fbe_u64_t record_index)
{
    fbe_logical_error_injection_record_t *rec_p = NULL;

    if (record_index >= FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS)
    {
        return NULL;
    }
    /* First get the first index in the error table.
     */
    fbe_logical_error_injection_get_error_info(&rec_p, 0);
    return &rec_p[record_index];
}
/******************************************
 * end fbe_logical_error_injection_get_record_at_index()
 ******************************************/

fbe_status_t fbe_logical_error_injection_find_record(fbe_queue_head_t *head_p,
                                                     fbe_logical_error_injection_record_t *find_rec_p,
                                                     fbe_logical_error_injection_record_t **return_rec_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_error_injection_record_t *rec_p = NULL;
    fbe_u32_t err_index;

    fbe_logical_error_injection_get_error_info(&rec_p, 0);

    /* Loop over the entire table.
     */
    for (err_index = 0; err_index < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; err_index++)
    {
        if (rec_p == find_rec_p)
        {
            break;
        }
        rec_p++;
    }

#if 0
    rec_p = (fbe_logical_error_injection_record_t*)fbe_queue_front(head_p);

    while (rec_p != NULL)
    {
        if (rec_p == find_rec_p)
        {
            break;
        }
        rec_p = (fbe_logical_error_injection_record_t*)fbe_queue_next(head_p, &rec_p->queue_element);
    }
#endif
    if (rec_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        *return_rec_p = rec_p;
    }
    return status;
}
fbe_status_t fbe_logical_error_injection_remove_one_record(fbe_u32_t index)
{
    fbe_logical_error_injection_record_t *prev_rec_p = NULL;
    fbe_logical_error_injection_record_t *current_rec_p = NULL;
    fbe_u32_t compress_index;
   
    fbe_logical_error_injection_get_error_info(&prev_rec_p, index);
    for (compress_index = index + 1; compress_index < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; compress_index++)
    {
        fbe_logical_error_injection_get_error_info(&current_rec_p, compress_index);
        *prev_rec_p = *current_rec_p;
        fbe_zero_memory(current_rec_p, sizeof(fbe_logical_error_injection_record_t));
        fbe_logical_error_injection_get_error_info(&prev_rec_p, compress_index);
    }

    fbe_logical_error_injection_dec_num_records();
    return FBE_STATUS_OK;
}
fbe_status_t fbe_logical_error_injection_remove_record(fbe_logical_error_injection_remove_record_t *remove_rec_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_error_injection_record_t *rec_p = NULL;
    fbe_u32_t num_records;

    if (remove_rec_p->record_handle >= FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s record handle %llx > max 0x%x\n", __FUNCTION__,
                                                  (unsigned long long)remove_rec_p->record_handle, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    rec_p = fbe_logical_error_injection_get_record_at_index(remove_rec_p->record_handle);

    if (rec_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s cannot find record handle %llx\n", __FUNCTION__,
                                                  (unsigned long long)remove_rec_p->record_handle);
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
    /* The way that we remove the record is to zero and invalidate it.
     */
    fbe_logical_error_injection_get_num_records(&num_records);

    if (num_records == 0)
    { 
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "LEI: Remove record.  There are no records, cannot remove record.\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_logical_error_injection_remove_one_record((fbe_u32_t)remove_rec_p->record_handle);
    }

    return status;
}
void fbe_logical_error_injection_copy_create_record_to_record(fbe_logical_error_injection_record_t *rec_p,
                                                              fbe_logical_error_injection_create_record_t *create_record_p)
{

    rec_p->pos_bitmap = create_record_p->pos_bitmap;
    rec_p->width = create_record_p->width;
    rec_p->opcode = create_record_p->opcode;
    rec_p->lba = create_record_p->lba;
    rec_p->blocks = create_record_p->blocks;
    rec_p->err_type = create_record_p->err_type;
    rec_p->err_mode = create_record_p->err_mode;
    rec_p->err_count = create_record_p->err_count;
    rec_p->err_limit = create_record_p->err_limit;
    rec_p->skip_count = create_record_p->skip_count;
    rec_p->skip_limit = create_record_p->skip_limit;
    rec_p->err_adj = create_record_p->err_adj;
    rec_p->start_bit = create_record_p->start_bit;
    rec_p->num_bits = create_record_p->num_bits;
    rec_p->bit_adj = create_record_p->bit_adj;
    rec_p->crc_det = create_record_p->crc_det;
    rec_p->object_id = create_record_p->object_id;
    return;
}
/*!**************************************************************
 * fbe_logical_error_injection_get_new_record()
 ****************************************************************
 * @brief
 *  Add a record based on a structure the user passed in.
 *
 * @param None.
 *
 * @return fbe_logical_error_injection_record_t *
 *
 * @author
 *  1/4/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_logical_error_injection_record_t * fbe_logical_error_injection_get_new_record(void)
{
    fbe_logical_error_injection_record_t *rec_p = NULL;
    fbe_u32_t err_index;
#if 0
    /* Allocate memory, and add to the queue.
     */
    rec_p = fbe_memory_native_allocate(sizeof(fbe_logical_error_injection_record_t));
    if (rec_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s cannot allocate record\n", __FUNCTION__);
        return rec_p;
    }
#endif

    /* First get the first index in the error table.
     */
    fbe_logical_error_injection_get_error_info(&rec_p, 0);

    for (err_index = 0; err_index < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; err_index++)
    {
        if (rec_p->err_type == FBE_XOR_ERR_TYPE_NONE)
        {
            break;
        }
        rec_p++;
    }
    /* If we exceed the table, just return NULL.
     */
    if (err_index >= FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS)
    {
        rec_p = NULL;
    }
    return rec_p;
}
/******************************************
 * end fbe_logical_error_injection_get_new_record()
 ******************************************/
/*!**************************************************************
 * fbe_logical_error_injection_add_record()
 ****************************************************************
 * @brief
 *  Add a record based on a structure the user passed in.
 *
 * @param create_rec_p - Record from user to create.
 *
 * @return fbe_status_t
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_add_record(fbe_logical_error_injection_create_record_t *create_rec_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_error_injection_record_t *rec_p = NULL;
    fbe_u32_t current_count;

    fbe_logical_error_injection_get_num_records(&current_count);

    /* We do not allow records beyond the max.
     */
    if (current_count >= FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING,FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s cannot add records, already at %d records\n", 
                                                  __FUNCTION__, current_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate the record to add.
     */
    if (create_rec_p->blocks == 0)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING,FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s cannot add record with block count == 0\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if ((create_rec_p->err_mode == FBE_LOGICAL_ERROR_INJECTION_MODE_INVALID) ||
        (create_rec_p->err_mode >= FBE_LOGICAL_ERROR_INJECTION_MODE_UNKNOWN))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING,FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s cannot add record with invalid err mode 0x%x\n", 
                                                  __FUNCTION__, create_rec_p->err_mode);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if ( ((create_rec_p->err_type == FBE_XOR_ERR_TYPE_DELAY_DOWN) ||
          (create_rec_p->err_type == FBE_XOR_ERR_TYPE_DELAY_UP)) &&
         (create_rec_p->err_limit > FBE_LOGICAL_ERROR_INJECTION_MAX_DELAY_MS) )
    {
        /* The error type of delay uses the error limit field as the
         * number of milliseconds to delay the I/O for. 
         * Enforce the limit on this field. 
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING,FBE_TRACE_MESSAGE_ID_INFO,
                                                  "cannot add delay %s record with delay ms: %d > limit_ms: %d\n", 
                                                  (create_rec_p->err_type == FBE_XOR_ERR_TYPE_DELAY_DOWN) ? "down" : "up",
                                                  create_rec_p->err_limit, FBE_LOGICAL_ERROR_INJECTION_MAX_DELAY_MS);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_logical_error_injection_validate_record_type(create_rec_p->err_type);
    if (status != FBE_STATUS_OK)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING,FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s cannot add record with invalid err type 0x%x\n", 
                                                  __FUNCTION__, create_rec_p->err_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    rec_p = fbe_logical_error_injection_get_new_record();

    /* Copy data into the record.
     */
    fbe_logical_error_injection_copy_create_record_to_record(rec_p, create_rec_p);

    fbe_logical_error_injection_inc_num_records();
    return status;
}
/******************************************
 * end fbe_logical_error_injection_add_record()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_modify_record()
 ****************************************************************
 * @brief
 *  Add a record based on a structure the user passed in.
 *
 * @param create_rec_p - Record from user to create.
 *
 * @return fbe_status_t
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_modify_record(fbe_logical_error_injection_modify_record_t *modify_rec_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_error_injection_record_t *rec_p = NULL;
#if 0
    fbe_logical_error_injection_get_active_head(&head_p);
#endif    
    rec_p = fbe_logical_error_injection_get_record_at_index(modify_rec_p->record_handle);
    /*status = fbe_logical_error_injection_find_record(head_p,
                                                     (fbe_logical_error_injection_record_t*)modify_rec_p->record_handle,
                                                     &rec_p);*/
    if ((status != FBE_STATUS_OK) || (rec_p == NULL))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s cannot find record %llx\n", __FUNCTION__,
                                                  (unsigned long long)modify_rec_p->record_handle);
        return status;
    }

    /* Copy data into the record.
     */
    fbe_logical_error_injection_copy_create_record_to_record(rec_p, &(modify_rec_p->modify_record));

    return status;
}
/******************************************
 * end fbe_logical_error_injection_modify_record()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_create_record()
 ****************************************************************
 * @brief
 *  Create a new record.
 *
 * @param packet_p - Packet for the disable operation.          
 *
 * @return fbe_status_t
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_create_record(fbe_packet_t * packet_p)
{
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_logical_error_injection_create_record_t * new_record_p = NULL;
    fbe_u32_t len;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &new_record_p);
    if (new_record_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_logical_error_injection_create_record_t))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu\n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_logical_error_injection_create_record_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Disable the service from injecting errors.
     */
    fbe_logical_error_injection_lock();
    status = fbe_logical_error_injection_add_record(new_record_p);
    fbe_logical_error_injection_unlock();

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_create_record()
 ******************************************/
/*!**************************************************************
 * fbe_logical_error_injection_service_delete_record()
 ****************************************************************
 * @brief
 *  Delete a record.
 *
 * @param packet_p - Packet for the disable operation.          
 *
 * @return fbe_status_t
 *
 * @author
 *  09/21/2010 - Created. Swati Fursule
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_delete_record(fbe_packet_t * packet_p)
{
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_logical_error_injection_remove_record_t * new_record_p = NULL;
    fbe_u32_t len;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);


    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &new_record_p);
    if (new_record_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_logical_error_injection_remove_record_t))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu\n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_logical_error_injection_create_record_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! delete the record now
     */
    fbe_logical_error_injection_lock();
    status = fbe_logical_error_injection_remove_record(new_record_p);
    fbe_logical_error_injection_unlock();

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_delete_record()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_modify_record()
 ****************************************************************
 * @brief
 *  Modify a record.
 *
 * @param packet_p - Packet for the disable operation.          
 *
 * @return fbe_status_t
 *
 * @author
 *  09/21/2010 - Created. Swati Fursule
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_modify_record(fbe_packet_t * packet_p)
{
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_logical_error_injection_modify_record_t * new_record_p = NULL;
    fbe_u32_t len;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &new_record_p);
    if (new_record_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_logical_error_injection_modify_record_t))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu\n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_logical_error_injection_create_record_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Now modify the record.
     */
    fbe_logical_error_injection_lock();
    status = fbe_logical_error_injection_modify_record(new_record_p);
    fbe_logical_error_injection_unlock();

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_modify_record()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_get_table_info()
 ****************************************************************
 * @brief
 *  Get table information
 *
 * @param packet_p - Packet for the disable operation.          
 *
 * @return fbe_status_t
 *
 * @author
 *  09/21/2010 - Created. Swati Fursule
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_get_table_info(fbe_packet_t * packet_p)
{
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_logical_error_injection_load_table_t * load_table_p = NULL;
    fbe_logical_error_injection_table_description_t table_desc;
    fbe_u32_t len;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sg_element_t *  sg_element = NULL;
    fbe_u32_t           sg_count = 0;
    fbe_payload_ex_t  *payload_p = NULL;


    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_ex_get_sg_list(payload_p, &sg_element, &sg_count);

    fbe_payload_control_get_buffer(control_operation_p, &load_table_p);
    if (load_table_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_logical_error_injection_load_table_t))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu\n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_logical_error_injection_load_table_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    /*! Get table info
     */
    /*fbe_logical_error_injection_lock();*/
    status = fbe_logical_error_injection_get_table_info(load_table_p->table_index, 
                                                         load_table_p->b_simulation,
                                                         &table_desc);
    /*fbe_logical_error_injection_unlock();*/
    /* set sg list with proper table information*/
    /* Copy the table description 
     */
    fbe_copy_memory(fbe_sg_element_address(sg_element),
                    &table_desc,
                    sizeof(fbe_logical_error_injection_table_description_t));

    payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_set_sg_list(payload_p, sg_element, sg_count);

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_get_table_info()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_get_active_table_info()
 ****************************************************************
 * @brief
 *  Get active table information
 *
 * @param packet_p - Packet for the given operation.          
 *
 * @return fbe_status_t
 *
 * @author
 *  09/21/2010 - Created. Swati Fursule
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_get_active_table_info(fbe_packet_t * packet_p)
{
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_logical_error_injection_load_table_t * load_table_p = NULL;
    fbe_logical_error_injection_table_description_t table_desc;
    fbe_u32_t len;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sg_element_t *  sg_element = NULL;
    fbe_u32_t           sg_count = 0;
    fbe_payload_ex_t  *payload_p = NULL;


    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_ex_get_sg_list(payload_p, &sg_element, &sg_count);

    fbe_payload_control_get_buffer(control_operation_p, &load_table_p);
    if (load_table_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_logical_error_injection_load_table_t))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu\n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_logical_error_injection_load_table_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    /*! Get table info
     */
    /*fbe_logical_error_injection_lock();*/
    status = fbe_logical_error_injection_get_active_table_info(load_table_p->b_simulation,
                                                         &table_desc);
    /*fbe_logical_error_injection_unlock();*/
    /* set sg list with proper table information*/
    /* Copy the table description 
     */
    fbe_copy_memory(fbe_sg_element_address(sg_element),
                    &table_desc,
                    sizeof(fbe_logical_error_injection_table_description_t));

    payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_set_sg_list(payload_p, sg_element, sg_count);

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_get_table_info()
 ******************************************/

/*!***************************************************************************
 * fbe_logical_error_injection_service_remove_edge_hook()
 *****************************************************************************
 *
 * @brief   Remove all the edge hooks associated with the object specified then
 *          remove the object from the logical error injection service.
 *
 * @param   packet_p - Packet for the disable operation.          
 *
 * @return  fbe_status_t
 *
 * @author
 *  09/10/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/

static fbe_status_t fbe_logical_error_injection_service_remove_object(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_payload_control_operation_t *new_control_operation_p = NULL;
    fbe_logical_error_injection_remove_object_t *remove_object_p = NULL;
    fbe_logical_error_injection_object_t *object_p = NULL;
    fbe_transport_control_remove_edge_tap_hook_t    transport_remove_hook;
    fbe_u32_t len;
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);


    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &remove_object_p);
    if (remove_object_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_logical_error_injection_remove_object_t))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %d\n", __FUNCTION__, len, (int)sizeof(fbe_logical_error_injection_remove_object_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*  Validate that the object is enabled
     */
    object_p = fbe_logical_error_injection_find_object(remove_object_p->object_id);
    if (object_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                  "%s unable to get object status %d\n", __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now send the proper usurper to the base transport.
     */
    fbe_zero_memory(&transport_remove_hook, sizeof(fbe_transport_control_remove_edge_tap_hook_t));
    transport_remove_hook.object_id = remove_object_p->object_id;
    new_control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    fbe_payload_control_build_operation(new_control_operation_p,
                                        FBE_BLOCK_TRANSPORT_CONTROL_CODE_REMOVE_EDGE_TAP_HOOK,
                                        &transport_remove_hook,
                                        sizeof(fbe_transport_control_remove_edge_tap_hook_t));
    /* Set packet address.
     */
    fbe_transport_set_address(packet_p,
                              remove_object_p->package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              remove_object_p->object_id); 
    
    /* Mark packet with the attribute, either traverse or not.
     */
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_SYNC);
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    status = fbe_service_manager_send_control_packet(packet_p);

    /* Wait for completion.
     * The packet is always completed so the status above need not be checked.
     */
    fbe_transport_wait_completion(packet_p);
	status = fbe_transport_get_status_code(packet_p);
	if (status != FBE_STATUS_OK)
	{
		fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                  "%s obj: 0x%x failed status: 0x%x\n", 
                                                  __FUNCTION__, remove_object_p->object_id, status);
	}
    fbe_payload_ex_release_control_operation(payload_p, new_control_operation_p);

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/************************************************************
 * end fbe_logical_error_injection_service_remove_object()
 ************************************************************/



void fbe_logical_error_injection_copy_rec_to_get_record(fbe_logical_error_injection_record_t *rec_p,
                                                        fbe_logical_error_injection_get_record_t *output_record_p)
{

    output_record_p->pos_bitmap = rec_p->pos_bitmap;
    output_record_p->width = rec_p->width;
    output_record_p->lba = rec_p->lba;
    output_record_p->blocks = rec_p->blocks;
    output_record_p->err_type = rec_p->err_type;
    output_record_p->err_mode = rec_p->err_mode;
    output_record_p->err_count = rec_p->err_count;
    output_record_p->err_limit = rec_p->err_limit;
    output_record_p->skip_count = rec_p->skip_count;
    output_record_p->skip_limit = rec_p->skip_limit;
    output_record_p->err_adj = rec_p->err_adj;
    output_record_p->start_bit = rec_p->start_bit;
    output_record_p->num_bits = rec_p->num_bits;
    output_record_p->bit_adj = rec_p->bit_adj;
    output_record_p->crc_det = rec_p->crc_det;
    return;
}
  
/*!**************************************************************
 * fbe_logical_error_injection_get_records()
 ****************************************************************
 * @brief
 *  Get the information on active records.
 *
 * @param get_records_p - pointer to the struct to return.
 *
 * @return fbe_status_t
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_get_records(fbe_logical_error_injection_get_records_t *get_records_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_error_injection_record_t *rec_p = NULL;
    fbe_u32_t record_index = 0;
    fbe_u32_t num_records = 0;

    /* First get the first index in the error table.
     */
    fbe_logical_error_injection_get_error_info(&rec_p, 0);

    for (record_index = 0; record_index < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; record_index++)
    {
        //if (rec_p->err_type != FBE_XOR_ERR_TYPE_NONE)
        {
            fbe_logical_error_injection_copy_rec_to_get_record(rec_p, &get_records_p->records[record_index]);
            num_records++;
        }
        rec_p++;
    }
    fbe_logical_error_injection_get_num_records(&get_records_p->num_records);

#if 0
    fbe_logical_error_injection_get_active_head(&head_p);

    /* Get the head of the list.
     */
    rec_p = (fbe_logical_error_injection_record_t*)fbe_queue_front(head_p);

    while (rec_p != NULL)
    {
        fbe_logical_error_injection_copy_rec_to_get_record(rec_p, &get_records_p->records[record_index]);
        rec_p = (fbe_logical_error_injection_record_t*)fbe_queue_next(head_p, &rec_p->queue_element);
        record_index++;
    }
    if (num_records != get_records_p->num_records)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid record count %d != %lld\n", __FUNCTION__, 
                             record_index, get_records_p->num_records );
        return FBE_STATUS_GENERIC_FAILURE;
    }
#endif
    return status;
}
/**************************************
 * end fbe_logical_error_injection_get_records()
 **************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_get_records()
 ****************************************************************
 * @brief
 *  Return all the records that are active.
 *
 * @param packet_p - Packet for the get operation.
 *
 * @return fbe_status_t
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_get_records(fbe_packet_t * packet_p)
{
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_logical_error_injection_get_records_t * get_records_p = NULL;
    fbe_u32_t len;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &get_records_p);
    if (get_records_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_logical_error_injection_get_records_t))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu\n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_logical_error_injection_get_records_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Disable the service from injecting errors.
     */
    fbe_logical_error_injection_lock();
    status = fbe_logical_error_injection_get_records(get_records_p);
    fbe_logical_error_injection_unlock();

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_get_records()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_find_object()
 ****************************************************************
 * @brief
 *  Find the given object that matches the input object id on
 *  the list of objects in the logical_error_injection service.
 *
 * @param object_id - Object to search for.
 *
 * @return fbe_logical_error_injection_object_t * -  Ptr to matching object id
 *                                 or NULL if not found.
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_logical_error_injection_object_t *fbe_logical_error_injection_find_object(fbe_object_id_t object_id)
{
    fbe_logical_error_injection_object_t *object_p = NULL;

    fbe_logical_error_injection_lock();
    object_p = fbe_logical_error_injection_find_object_no_lock(object_id);
    fbe_logical_error_injection_unlock();
    return object_p;
}
/******************************************
 * end fbe_logical_error_injection_find_object()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_find_object_no_lock()
 ****************************************************************
 * @brief
 *  Find the given object that matches the input object id on
 *  the list of objects in the logical_error_injection service.
 *
 * @param object_id - Object to search for.
 *
 * @return fbe_logical_error_injection_object_t * -  Ptr to matching object id
 *                                 or NULL if not found.
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_logical_error_injection_object_t *fbe_logical_error_injection_find_object_no_lock(fbe_object_id_t object_id)
{
    fbe_logical_error_injection_object_t *object_p = NULL;
    fbe_queue_head_t *head_p = NULL;
    fbe_queue_element_t *element_p = NULL;

    head_p = &fbe_logical_error_injection_service.active_object_head;
    element_p = fbe_queue_front(head_p);

    while (element_p != NULL)
    {
        /* We can cast the element to the object since the 
         * element happens to be defined at the top of the object. 
         */
        object_p = (fbe_logical_error_injection_object_t*) element_p;
        if (object_p->object_id == object_id)
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
 * end fbe_logical_error_injection_find_object_no_lock()
 ******************************************/
/*!**************************************************************
 * fbe_logical_error_injection_get_object()
 ****************************************************************
 * @brief
 *  Get a ptr to an object.
 *  This function will look through the global logical_error_injection list
 *  first.  If the object is not found, it will attempt to
 *  allocate one.
 *
 * @param object_id - id of object to find.
 * @param package_id - package this object is in.
 * @param object_pp - ptr ptr of object to return.
 *
 * @return FBE_STATUS_OK if object was either found or created.
 *        FBE_STATUS_INSUFFICIENT_RESOURCES if object id not found
 *                                          and could not be allocated.
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_get_object(fbe_object_id_t object_id,
                                                    fbe_package_id_t package_id, 
                                                    fbe_logical_error_injection_object_t **object_pp)
{   
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_error_injection_object_t *object_p = NULL;

    /* Next get the ptr to the object.
     */
    object_p = fbe_logical_error_injection_find_object(object_id);

    if (object_p == NULL)
    {
        /* Object does not exist yet, simply allocate and init it.
         */
        object_p = fbe_memory_native_allocate(sizeof(fbe_logical_error_injection_object_t));

        if (object_p == NULL)
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: unable to allocate object for object_id: 0x%x\n", __FUNCTION__, object_id);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        status = fbe_logical_error_injection_object_init(object_p, object_id, package_id);
        if (status != FBE_STATUS_OK)
        {
            fbe_memory_native_release(object_p);
            return status;
        }
        fbe_logical_error_injection_lock();
        fbe_logical_error_injection_enqueue_object(object_p);
        fbe_logical_error_injection_inc_num_objects();
        fbe_logical_error_injection_unlock();
    }
    *object_pp = object_p;
    return status;
}
/******************************************
 * end fbe_logical_error_injection_get_object()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_complete_packet()
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
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_logical_error_injection_complete_packet(fbe_packet_t *packet_p,
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
 * end fbe_logical_error_injection_complete_packet()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_load_table()
 ****************************************************************
 * @brief
 *  Load a new static table.
 *
 * @param packet_p - Packet for the operation          
 *
 * @return fbe_status_t
 *
 * @author
 *  1/5/2010 - Created. Rob Foley
 * 
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_load_table(fbe_packet_t * packet_p)
{
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_logical_error_injection_load_table_t * load_table_p = NULL;
    fbe_u32_t len;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_enabled;
    fbe_payload_ex_t  *payload_p = NULL;

    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &load_table_p);
    if (load_table_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_logical_error_injection_load_table_t))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu\n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_logical_error_injection_load_table_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_logical_error_injection_get_enable(&b_enabled);
    if (b_enabled)
    {
        /* We do not allow changing tables until the when the service is disabled.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING,
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                  "%s attempt to load table when lei not disabled.\n", 
                                                  __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO,
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                  "%s loading new table: %d b_simulation: %d\n", 
                                                  __FUNCTION__, load_table_p->table_index, load_table_p->b_simulation);
    
        /*! Load the new table.
         */
        fbe_logical_error_injection_lock();
        status = fbe_logical_error_injection_load_table(load_table_p->table_index, load_table_p->b_simulation);
        fbe_logical_error_injection_unlock();
    }

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_load_table()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_disable_records()
 ****************************************************************
 * @brief
 *  Clear the current table.
 *
 * @param packet_p - Packet for the operation          
 *
 * @return fbe_status_t
 *
 * @author
 *  2/5/2010 - Created. Rob Foley
 * 
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_disable_records(fbe_packet_t * packet_p)
{
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_logical_error_injection_disable_records_t * disable_records_p = NULL;
    fbe_u32_t len;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_enabled;
    fbe_payload_ex_t  *payload_p = NULL;

    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &disable_records_p);
    if (disable_records_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_logical_error_injection_disable_records_t))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu\n", __FUNCTION__, len, (unsigned long long)sizeof(fbe_logical_error_injection_disable_records_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_logical_error_injection_get_enable(&b_enabled);
    if (b_enabled)
    {
        /* We do not allow changing tables until the when the service is disabled.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING,
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                  "%s attempt to clear table records when injection not disabled.\n", 
                                                  __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO,
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                  "%s clearing records start index: %d count: %d\n", 
                                                  __FUNCTION__, disable_records_p->start_index, disable_records_p->count);
    
        /*! Load the new table.
         */
        fbe_logical_error_injection_lock();
        status = fbe_logical_error_injection_disable_records(disable_records_p->start_index, disable_records_p->count);
        fbe_logical_error_injection_unlock();
    }

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_disable_records()
 ******************************************/


/*!**************************************************************
 * fbe_logical_error_injection_service_disable_lba_normalize()
 ****************************************************************
 * @brief
 *  disable the normalization of table_start_lba by setting the 
 *  max_lba to FBE_LBA_INVALID.
 *
 * @param packet_p - Packet for the operation.          
 *
 * @return FBE_STATUS_OK - enable successful.
 *         FBE_STATUS_GENERIC_FAILURE - enable Failed.
 *
 * @author
 *  04/08/2013 - Created. Lili Chen
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_service_disable_lba_normalize(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_initialized;

    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: entry\n", __FUNCTION__);

    fbe_logical_error_injection_get_initialized(&b_initialized);

    /*! Don't allow disable if we are not initialized.
     */
    if (b_initialized == FBE_FALSE)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  "%s: trying to disable normalize an uninitialized table.\n", __FUNCTION__);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the max_lba to FBE_LBA_INVALID.
     */
    fbe_logical_error_injection_set_max_lba(FBE_LBA_INVALID);

    /* Complete the packet with the status from above.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    /* Return good status since the packet was completed.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_service_disable_lba_normalize()
 ******************************************/


/*************************
 * end file fbe_logical_error_injection_main.c
 *************************/
