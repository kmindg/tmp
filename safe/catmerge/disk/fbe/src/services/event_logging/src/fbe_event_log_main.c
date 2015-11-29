/******************************************************************************
 * Copyright (C) EMC Corporation  2009 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation Unpublished 
 * - all rights reserved under the copyright laws 
 * of the United States
 *****************************************************************************/

/*!****************************************************************************
 * @file fbe_event_log_main.c
 ******************************************************************************
 *
 * @brief
 *      This file contains functions related to handling of Event
 *      Logging service. 
 *       This file contains platform independent code. Please add
 *      platform specific code or Kernel/User space implementation
 *      in the appropriate files
 * 
 * @version
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 ******************************************************************************/
#include "csx_ext.h"
#include "fbe/fbe_winddk.h"
#include "fbe_event_log.h"
#include "fbe/fbe_transport.h"
#include "fbe_base_service.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_event_log_utils.h"

/* Declare our service methods */
fbe_status_t fbe_event_log_control_entry(fbe_packet_t * packet); 
fbe_service_methods_t fbe_event_log_service_methods = {FBE_SERVICE_ID_EVENT_LOG, fbe_event_log_control_entry};

/*!*******************************************************************
 * @enum fbe_event_log_flags_t
 *********************************************************************
 * @brief The flags of the event log.
 *
 *********************************************************************/
typedef enum fbe_event_log_flags_e
{
    FBE_EVENT_LOG_FLAG_NONE = 0,
    FBE_EVENT_LOG_FLAG_DISABLE_TRACE = 0, /*!< Disable tracing for error injection tests so logs are not flooded. */
    FBE_EVENT_LOG_FLAG_LAST = 0x0002,
}
fbe_event_log_flags_t;

typedef struct fbe_event_log_service_s{
    fbe_base_service_t  base_service;
    fbe_spinlock_t      event_log_statistics_lock;
    fbe_event_log_statistics_t event_log_statistics;
    fbe_event_log_flags_t flags;
}fbe_event_log_service_t;

static fbe_event_log_service_t event_log_service;

/* Forward declaration */
static fbe_status_t fbe_event_log_clear_statistics(fbe_event_log_statistics_t *event_log_stat);
static fbe_status_t fbe_event_log_get_statistics(fbe_packet_t *  packet);
static fbe_status_t fbe_event_log_increment_msgs_dropped(fbe_event_log_statistics_t *event_log_stat);
static fbe_status_t fbe_event_log_increment_msgs_logged(fbe_event_log_statistics_t *event_log_stat);
static fbe_status_t fbe_event_log_get_count(fbe_packet_t *packet);
static fbe_status_t fbe_event_log_clear(fbe_packet_t * packet);
static fbe_status_t fbe_event_log_check_msg(fbe_packet_t *packet);
static fbe_status_t fbe_event_log_get_event_log_info(fbe_packet_t *  packet);
static fbe_status_t fbe_event_log_get_all_event_logs_info(fbe_packet_t *  packet);
static fbe_status_t fbe_event_log_clear_statistics_info(fbe_packet_t *  packet);

static fbe_bool_t fbe_event_log_is_trace_disabled(void)
{
    return event_log_service.flags & FBE_EVENT_LOG_FLAG_DISABLE_TRACE;
}
static fbe_status_t fbe_event_log_disable_trace(void)
{
    event_log_service.flags |= FBE_EVENT_LOG_FLAG_DISABLE_TRACE;
    return FBE_STATUS_OK;
}
static fbe_status_t fbe_event_log_enable_trace(void)
{
    event_log_service.flags &= ~FBE_EVENT_LOG_FLAG_DISABLE_TRACE;
    return FBE_STATUS_OK;
}
/*!****************************************************************************
 * @fn fbe_event_log_trace(fbe_trace_level_t trace_level,
 *                         fbe_trace_message_id_t message_id,
 *                         const char * fmt, ...)
 ******************************************************************************
 * @brief
 *      This function is the interface used by the event logging
 *      service to trace info
 *
 * @param trace_level - Trace level value that message needs to be
 *                      logged at
 * @param message_id - Message Id
 * @param fmt - Insertion Strings
 * 
 * @return
 *
 * @author:
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 ******************************************************************************/
void fbe_event_log_trace(fbe_trace_level_t trace_level,
                         fbe_trace_message_id_t message_id,
                         const char * fmt, ...)
                         __attribute__((format(__printf_func__,3,4)));
void fbe_event_log_trace(fbe_trace_level_t trace_level,
                         fbe_trace_message_id_t message_id,
                         const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized((fbe_base_service_t *) &event_log_service)) {
        service_trace_level = fbe_base_service_get_trace_level((fbe_base_service_t *) &event_log_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_EVENT_LOG,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
}
/******************************
    end fbe_event_log_trace()     
*******************************/
/*!****************************************************************************
 * @fn fbe_event_log_init(fbe_packet_t * packet)
 ******************************************************************************
 *
 * @brief
 *      This function initializes the event log service and calls
 *      any platform specific initialization routines
 *
 * @param packet - Pointer to the packet received by the event log
 *                 service
 * 
 * @return fbe_status_t
 *
 * @author
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_event_log_init(fbe_packet_t * packet)
{
    fbe_event_log_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: entry\n", __FUNCTION__);

    fbe_base_service_set_service_id((fbe_base_service_t *) &event_log_service, FBE_SERVICE_ID_EVENT_LOG);
    fbe_base_service_set_trace_level((fbe_base_service_t *) &event_log_service, fbe_trace_get_default_trace_level());

    fbe_base_service_init((fbe_base_service_t *) &event_log_service);
    fbe_spinlock_init(&event_log_service.event_log_statistics_lock);
    fbe_event_log_clear_statistics(&event_log_service.event_log_statistics);
    event_log_service.flags = FBE_EVENT_LOG_FLAG_NONE;

    /* Call the implementation specific init */
    fbe_event_log_init_log();

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
/******************************
    end fbe_event_log_init()     
*******************************/
/*!****************************************************************************
 * @fn fbe_event_log_destroy(fbe_packet_t * packet)
 ******************************************************************************
 *
 * @brief
 *      This function performs the necessary clean up in
 *      preparation to stop the event log service
 *
 * @param  packet - Pointer to the packet received by the event log
 *                  service
 * 
 * @return  fbe_status_t
 *
 * @author
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_event_log_destroy(fbe_packet_t * packet)
{
    fbe_event_log_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: entry\n", __FUNCTION__);

    /* Call the implementation specific cleanup */
    fbe_event_log_destroy_log();

    fbe_event_log_clear_statistics(&event_log_service.event_log_statistics);
    fbe_spinlock_destroy(&event_log_service.event_log_statistics_lock);
    fbe_base_service_destroy((fbe_base_service_t *) &event_log_service);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
/******************************
    end fbe_event_log_destroy() 
*******************************/
/*!**********************************************************************
 * @fn fbe_event_log_control_entry(fbe_packet_t * packet)
 ************************************************************************
 *
 * @brief
 *      This function dispatches the control code to the
 *      appropriate handler functions
 *
 * @param  packet - Pointer to the packet received by the event log
 *                  service
 * 
 * @return  fbe_status_t
 *
 * @author
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 *************************************************************************/
fbe_status_t 
fbe_event_log_control_entry(fbe_packet_t * packet)
{    
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code; 

    control_code = fbe_transport_get_control_code(packet);
    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        status = fbe_event_log_init(packet);
        return status;
    }

    /* No operation is allowed until the event log service is
     *  initialized. Return the status immediately
     */
    if(!fbe_base_service_is_initialized((fbe_base_service_t *) &event_log_service)){
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    switch(control_code)
    {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = fbe_event_log_destroy( packet);
            break;
        case FBE_EVENT_LOG_CONTROL_CODE_GET_STATISTICS:
            status = fbe_event_log_get_statistics(packet);
            break;
        case FBE_EVENT_LOG_CONTROL_CODE_CLEAR_LOG:
            status = fbe_event_log_clear(packet);
            break;
        case FBE_EVENT_LOG_CONTROL_CODE_GET_MSG_COUNT:
            status = fbe_event_log_get_count(packet);
            break;
        case FBE_EVENT_LOG_CONTROL_CODE_CHECK_MSG:
            status =  fbe_event_log_check_msg(packet);
            break;
        case FBE_EVENT_LOG_CONTROL_CODE_GET_LOG:
            status =  fbe_event_log_get_event_log_info(packet);
            break;
        case FBE_EVENT_LOG_CONTROL_CODE_GET_ALL_LOGS:
            status =  fbe_event_log_get_all_event_logs_info(packet);
            break;
        case FBE_EVENT_LOG_CONTROL_CODE_CLEAR_STATISTICS:
            status =  fbe_event_log_clear_statistics_info(packet);
            break;
        case FBE_EVENT_LOG_CONTROL_CODE_DISABLE_TRACE:
            status = fbe_event_log_disable_trace();
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            break;
        case FBE_EVENT_LOG_CONTROL_CODE_ENABLE_TRACE:
            status =  fbe_event_log_enable_trace();
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            break;
        default:
            status = fbe_base_service_control_entry((fbe_base_service_t*)&event_log_service, packet);
            break;
    }
    return status;
}
/*************************************
    end fbe_event_log_control_entry()     
***************************************/
/*!****************************************************************************
 * @fn  fbe_event_log_write(fbe_u32_t msg_id,
 *                          fbe_u8_t *dump_data,
 *                          fbe_u32_t dump_data_size,
 *                          fbe_wide_char_t *fmt, 
 *                          ...)
 ******************************************************************************
 *
 * @brief
 *      This function calls the implementation specific funtion to
 *      actually write the events to the logs
 *
 * @param msg_id - Message ID
 * @param dump_data - Buffer that contains raw data that needs to
 *                     be saved
 * @param dump_data_size - Size of the dump data buffer
 * @param fmt - Insertion strings
 * 
 * @return fbe_status_t
 *
 * @author
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 ******************************************************************************/
fbe_status_t fbe_event_log_write(fbe_u32_t msg_id,
                                 fbe_u8_t *dump_data,
                                 fbe_u32_t dump_data_size,
                                 fbe_char_t *fmt, 
                                 ...)
{
    fbe_status_t status;
    va_list argList;
    va_list argListCopy;
    fbe_u8_t full_msg[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH];
    fbe_zero_memory(full_msg, sizeof(full_msg));
    
    /* Since we are saving statistics information, we dont allow
     *  any events to be logged until the event log service is
     *  initialized
     */
     if(!fbe_base_service_is_initialized((fbe_base_service_t *) &event_log_service)){
        return FBE_STATUS_NOT_INITIALIZED;
    }

     if (fbe_event_log_is_trace_disabled()){
         return FBE_STATUS_OK;
     }

#ifndef va_copy
#define va_copy(dst, src) ((void)((dst) = (src)))
#endif

    va_start(argList, fmt);
    va_copy(argListCopy, argList);

    /* Do Tracing before logging an event */
    status = fbe_event_log_get_full_msg(full_msg, 
                                        FBE_EVENT_LOG_MESSAGE_STRING_LENGTH, 
                                        msg_id, 
                                        argList);
    if(status == FBE_STATUS_OK)
    {
        fbe_event_log_trace(FBE_TRACE_LEVEL_INFO, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "0x%x   %s\n", msg_id, full_msg);
    } 
    else
    {
        fbe_event_log_trace(FBE_TRACE_LEVEL_ERROR, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Message string error for MsgID : 0x%x\n", msg_id);
    }

    status = fbe_event_log_write_log(msg_id, dump_data, dump_data_size, fmt, argListCopy);
    va_end(argListCopy);

    va_end(argList);

    fbe_spinlock_lock(&event_log_service.event_log_statistics_lock);

    if(status != FBE_STATUS_OK) {
        /* The message was not logged. Increment the dropped count
         */
        fbe_event_log_increment_msgs_dropped(&event_log_service.event_log_statistics);
    } else {
        /* The message was logged correctly. Increment the logged count
         */
        fbe_event_log_increment_msgs_logged(&event_log_service.event_log_statistics);
    }

    fbe_spinlock_unlock(&event_log_service.event_log_statistics_lock);

    return(status);
    
}
/******************************
    end fbe_event_log_write()
*******************************/
/*!****************************************************************************
 * @fn  fbe_event_log_clear_statistics(fbe_event_log_statistics_t *event_log_stat)
 ******************************************************************************
 *
 * @brief
 *      This function clears the statistics information
 *
 * @param event_log_stat _ pointer to the buffer whose statistics
 * @param information needs to be cleared
 * 
 * @return fbe_status_t
 *
 * @remarks 
 *      The lock needs to be held by the caller, if needed
 * 
 * @author
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_event_log_clear_statistics(fbe_event_log_statistics_t *event_log_stat)
{
    event_log_stat->total_msgs_logged = 0;
    event_log_stat->total_msgs_queued = 0;
    event_log_stat->total_msgs_dropped = 0;
    event_log_stat->current_msgs_logged = 0;

    return(FBE_STATUS_OK);
}
/*****************************************
    end fbe_event_log_clear_statistics()     
*******************************************/
/*!****************************************************************************
 *@fn fbe_event_log_get_statistics(fbe_packet_t *  packet)
 ******************************************************************************
 *
 * @brief
 *      This is the handler function for returning the event log
 *      statistics information
 *
 * @param packet - Pointer to the packet received by the event log
 *                 service
 * 
 * @return      fbe_status_t
 *      
 * @author
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 ***********************************************************************************/
static fbe_status_t fbe_event_log_get_statistics(fbe_packet_t *  packet)
{
    fbe_event_log_statistics_t * event_log_statistics = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &event_log_statistics);
    if(event_log_statistics == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
		
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length < sizeof(fbe_event_log_statistics_t)){
        fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Need to get the lock first then copy the data to the buffer
     * passed in
     */
    fbe_spinlock_lock(&event_log_service.event_log_statistics_lock);
    fbe_copy_memory(event_log_statistics, &event_log_service.event_log_statistics, buffer_length);
    fbe_spinlock_unlock(&event_log_service.event_log_statistics_lock);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/****************************************
    end fbe_event_log_get_statistics()     
*****************************************/

/*!****************************************************************************
 *@fn fbe_event_log_increment_msgs_dropped(fbe_event_log_statistics_t *event_log_stat)
 ******************************************************************************
 *
 *@brief
 *   This function increments the dropped message count
 *
 *@param event_log_stat _ pointer to the statistics buffer 
 * 
 *@return fbe_status_t
 *
 * @remarks 
 *      The lock needs to be held by the caller, if needed
 * 
 * @author
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_event_log_increment_msgs_dropped(fbe_event_log_statistics_t *event_log_stat)
{
    event_log_stat->total_msgs_dropped++;
    
    return(FBE_STATUS_OK);
}
/************************************************
    end fbe_event_log_increment_msgs_dropped()     
*************************************************/

/*!****************************************************************************
 *@fn  fbe_event_log_increment_msgs_logged(fbe_event_log_statistics_t *event_log_stat)
 ******************************************************************************
 *
 * @brief
 *      This function increments the logged message count
 *
 * @param event_log_stat _ pointer to the statistics buffer 
 * 
 * @return fbe_status_t
 *
 * @remarks 
 *      The lock needs to be held by the caller, if needed
 * 
 * @author
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_event_log_increment_msgs_logged(fbe_event_log_statistics_t *event_log_stat)
{
    event_log_stat->total_msgs_logged++;
    event_log_stat->current_msgs_logged = fbe_event_get_current_index();
    
    return(FBE_STATUS_OK);
}
/************************************************
    end fbe_event_log_increment_msgs_logged()     
*************************************************/
/*!***********************************************************************************
 * @fn fbe_event_log_clear()
 ************************************************************************************
 *
 * @brief
 *      This function clears the event log
 *
 * @param packet - Pointer to the packet received by the event log
 *                 service
 * 
 * @return 
 *
 * @remarks 
 *      The lock needs to be held by the caller, if needed
 * 
 * @author
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 ***********************************************************************************/
static fbe_status_t fbe_event_log_clear(fbe_packet_t * packet)
{
    fbe_status_t status;

    status = fbe_event_log_clear_log();

    /*complete the packet*/
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK; 
}
/******************************
    end fbe_event_log_clear()     
*******************************/
/*!****************************************************************************
 * @fn  fbe_event_log_clear_statistics_info(fbe_packet_t *  packet)
 ******************************************************************************
 *
 * @brief
 *      This function clears the statistics information
 *
 * @param Pointer to the packet received by the event log
 *                 service
 * 
 * @return fbe_status_t
 *
 * @author
 *      28-October-2010    Vishnu Sharma  Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_event_log_clear_statistics_info(fbe_packet_t *  packet)
{
    fbe_status_t status;

    fbe_spinlock_lock(&event_log_service.event_log_statistics_lock);
    status = fbe_event_log_clear_statistics(&event_log_service.event_log_statistics);
    fbe_spinlock_unlock(&event_log_service.event_log_statistics_lock);
    /*complete the packet*/
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return status; 
}
/*************************************************
    end fbe_event_log_clear_statistics_info()     
***************************************************/
/*!***********************************************************************************
 * @fn  fbe_event_log_get_count(fbe_packet_t * packet)
 ************************************************************************************
 *
 * @brief
 *      This function get the number of message with a particular
 *      message ID
 *
 * @param packet - Pointer to the packet received by the event log
 *                 service
 * 
 * @return   fbe_status_t
 *
 * @remarks 
 *      The lock needs to be held by the caller, if needed
 * 
 * @author
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *      03-May-2010    Vaibhav Gaonkar - Added control code to access this function.
 *
 ***********************************************************************************/
static fbe_status_t fbe_event_log_get_count(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_event_log_msg_count_t  *event_log_msg_count;    
    fbe_payload_control_buffer_length_t buffer_length;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &event_log_msg_count);
    if(event_log_msg_count == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length < sizeof(fbe_event_log_msg_count_t)){
        fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
     /* Initialized message count to zero*/
     event_log_msg_count->msg_count = 0;
	 
    /*  Copy the message count*/
    fbe_event_log_get_msg_count(event_log_msg_count->msg_id, 
                                &event_log_msg_count->msg_count);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK; 
}
/**********************************
    end fbe_event_log_get_count()     
***********************************/
/*!***************************************************************
 * @fn fbe_event_log_check_msg(fbe_packet_t *packet)
 ****************************************************************
 * @brief
 *  This function is used to check particular message present in
 *  event log.
 *
 * @param packet - Pointer to the packet received by the event log
 *                 service
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
static fbe_status_t
fbe_event_log_check_msg(fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_event_log_check_msg_t  *event_log_msg;    
    fbe_payload_control_buffer_length_t buffer_length;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &event_log_msg);
    if(event_log_msg == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length < sizeof(fbe_event_log_check_msg_t)){
        fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    event_log_msg->is_msg_present = fbe_event_log_message_check(event_log_msg->msg_id,
                                                                event_log_msg->msg);
    
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    
    return FBE_STATUS_OK;
}
/**********************************
    end fbe_event_log_check_msg()     
***********************************/
/*!****************************************************************************
 *@fn fbe_event_log_get_event_log_info(fbe_packet_t *  packet)
 ******************************************************************************
 *
 * @brief
 *      This is the handler function for returning the event log
 *      message
 *
 * @param packet - Pointer to the packet received by the event log
 *                 service
 * 
 * @return      fbe_status_t
 *      
 * @author
 *      10-October-2010    Vishnu Sharma  Created.
 *
 ***********************************************************************************/
static fbe_status_t fbe_event_log_get_event_log_info(fbe_packet_t *  packet)
{
    fbe_event_log_get_event_log_info_t * event_log_info = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_transport_get_payload_ex failed", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &event_log_info);
    if(event_log_info == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length < sizeof(fbe_event_log_get_event_log_info_t)){
        fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*get event log info at the index*/
    status = fbe_event_log_get_log(event_log_info->event_log_index,&event_log_info->event_log_info);
    
    if(status != FBE_STATUS_OK){
        fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid event log index %d", __FUNCTION__, event_log_info->event_log_index);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/****************************************
    end fbe_event_log_get_event_log_info()     
*****************************************/
/*!****************************************************************************
 *@fn fbe_event_log_get_all_event_logs_info(fbe_packet_t *  packet)
 ******************************************************************************
 *
 * @brief
 *      This is the handler function for returning the event log
 *      message
 *
 * @param packet - Pointer to the packet received by the event log
 *                 service
 * 
 * @return      fbe_status_t
 *      
 * @author
 *      26-Feb-2011 Trupti Ghate  Created.
 *
 ***********************************************************************************/
static fbe_status_t fbe_event_log_get_all_event_logs_info(fbe_packet_t *  packet)
{
    fbe_event_log_get_all_events_log_info_t * get_event_log_info = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_status_t status;
    fbe_sg_element_t * sg_element;
    fbe_u32_t sg_element_count;
    fbe_event_log_get_event_log_info_t *sg_list_event_log_info;
    fbe_event_log_info_t event_log_info;
    fbe_u32_t loop_counter=0, event_current_index = 0;
    fbe_bool_t event_is_complete_buffer_full = FBE_FALSE;

    /* Get the payload */
    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_transport_get_payload_ex failed", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get control operation */
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &get_event_log_info);

    if(get_event_log_info == NULL){
        fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

   /* Get the buffer length */
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length != sizeof(fbe_event_log_get_all_events_log_info_t)){
        fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid buffer_length %d", __FUNCTION__, buffer_length);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the sg list which comtains buffer to store events */
    sg_element = NULL;
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_element_count);
    if (sg_element == NULL) {
        fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_ex_get_sg_list failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify that buffer of sg_list is not null */
    if (sg_element->address == NULL) {
        fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s sg list not found\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list_event_log_info = (fbe_event_log_get_event_log_info_t*)fbe_sg_element_address(sg_element);
    event_current_index = fbe_event_get_current_index();
    event_is_complete_buffer_full = fbe_event_get_is_complete_buffer_in_use();
    get_event_log_info->total_events_copied=0;

   if(event_is_complete_buffer_full == FBE_TRUE && 
       event_current_index != FBE_EVENT_LOG_MESSAGE_COUNT)
   {
       loop_counter = event_current_index + 1;
   }

   while(get_event_log_info->total_events_copied <FBE_EVENT_LOG_MESSAGE_COUNT)
    {
       if((event_is_complete_buffer_full == FBE_FALSE && loop_counter >= event_current_index) ||
           (event_is_complete_buffer_full == FBE_TRUE  && loop_counter == event_current_index))
       {
           break;
       }
        /*get event log info at the index*/
        status = fbe_event_log_get_log(loop_counter, &event_log_info);
        if(status != FBE_STATUS_OK){
            fbe_base_service_trace((fbe_base_service_t *) &event_log_service, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid event log index %d", __FUNCTION__, 1);
    
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_copy_memory(&(sg_list_event_log_info->event_log_info),(&event_log_info),sizeof(fbe_event_log_info_t));
        get_event_log_info->total_events_copied++;
        sg_list_event_log_info++;
        loop_counter = (loop_counter + 1) % FBE_EVENT_LOG_MESSAGE_COUNT;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/****************************************
    end fbe_event_log_get_all_event_logs_info()     
*****************************************/


