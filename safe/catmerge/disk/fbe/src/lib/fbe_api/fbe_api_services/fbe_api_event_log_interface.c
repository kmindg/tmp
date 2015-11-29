/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_event_log_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions which use to access the
 *  event log service.
 *  
 * @ingroup fbe_api_system_package_interface_class_files
 * @ingroup fbe_api_event_log_service_interface
 *
 * @version
 *   3-May - 2010 - Vaibhav Gaonkar Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_event_log_utils.h"

/**************************************
                Local variables
**************************************/
#define FBE_API_POLLING_INTERVAL 100 /*ms*/

/*!***************************************************************
 * @fn fbe_api_get_event_log_msg_count(fbe_event_log_msg_count_t *event_log_msg,
 *                                     fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This fbe api used to get event log message count for package.
 *
 * @param event_log_msg    pointer to message count
 * @param package_id    Package ID
 * 
 * @return fbe_status_t
 *
 * @author
 *  3-May - 2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_event_log_msg_count(fbe_event_log_msg_count_t *event_log_msg,
                                                          fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    if(package_id <= FBE_PACKAGE_ID_INVALID || 
       package_id >= FBE_PACKAGE_ID_LAST) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_api_common_send_control_packet_to_service(FBE_EVENT_LOG_CONTROL_CODE_GET_MSG_COUNT,
                                                           event_log_msg,
                                                           sizeof(fbe_event_log_msg_count_t),
                                                           FBE_SERVICE_ID_EVENT_LOG,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id); 
    return status;
}
/********************************************
    end fbe_api_get_event_log_msg_count()     
*********************************************/
/*!***************************************************************
 * @fn fbe_api_clear_event_log(fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This fbe api used to clear the event log message count for package.
 *
 * @param package_id    Package ID
 * 
 * @return fbe_status_t 
 *
 * @author
 *  3-May - 2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_clear_event_log(fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_EVENT_LOG_CONTROL_CODE_CLEAR_LOG,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_EVENT_LOG,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id); 
    return status;
}
/********************************************
    end fbe_api_clear_event_log()     
*********************************************/
/*!*****************************************************************************
 * @fn fbe_api_check_event_log_msg(fbe_package_id_t package_id,
 *                                 fbe_bool_t *is_msg_present,
 *                                 fbe_u32_t msg_id,
 *                                 ...)
 *******************************************************************************
 * @brief
 *  This fbe api used to check particular message present in evnet log.
 *
 * @param package_id     package ID
 * @param is_msg_present It return TRUE if message present in event log
 * @param msg_id         Message ID
 * @param Argument list for given messsage ID
 * 
 * @return fbe_status_t 
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_check_event_log_msg(fbe_package_id_t package_id,
                                                      fbe_bool_t *is_msg_present,
                                                      fbe_u32_t msg_id,
                                                      ...)
{
    fbe_status_t status;
    va_list args;
    fbe_event_log_check_msg_t event_log_msg;
    fbe_api_control_operation_status_info_t status_info;

    *is_msg_present = FALSE;
    
    /* Initialized the fbe_event_log_check_msg_t struct*/
    event_log_msg.msg_id = msg_id;
    event_log_msg.is_msg_present = FALSE;
    va_start(args, msg_id);
    status = fbe_event_log_get_full_msg(event_log_msg.msg,
                                        sizeof(event_log_msg.msg), 
                                        msg_id, 
                                        args);
    va_end(args);

    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in retrieving the message string, Status:%d\n", __FUNCTION__, status);
        return status;
    }
    status = fbe_api_common_send_control_packet_to_service(FBE_EVENT_LOG_CONTROL_CODE_CHECK_MSG,
                                                           &event_log_msg,
                                                           sizeof(fbe_event_log_check_msg_t),
                                                           FBE_SERVICE_ID_EVENT_LOG,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id); 
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet, Status:%d\n", __FUNCTION__, status); 
        return status;
    }
    
    *is_msg_present = event_log_msg.is_msg_present;
    return status;
}
/********************************************
    end fbe_api_check_event_log_msg()     
*********************************************/


/*!*****************************************************************************
 * @fn fbe_api_wait_for_event_log_msg(fbe_u32_t timeoutMs,
 *                                 fbe_package_id_t package_id,
 *                                 fbe_bool_t *is_msg_present,
 *                                 fbe_u32_t msg_id,
 *                                 ...)
 *******************************************************************************
 * @brief
 *  This fbe api used to check particular message present in evnet log within 
 *  the timeout in milliseconds.
 *
 * @param timeoutMs - timeouts in Milliseconds.
 * @param package_id     package ID
 * @param is_msg_present It return TRUE if message present in event log
 * @param msg_id         Message ID
 * @param Argument list for given messsage ID
 * 
 * @return fbe_status_t 
 *
 * @author
 *  24-Jan-2012: PHE Created.
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_wait_for_event_log_msg(fbe_u32_t timeoutMs,
                                                      fbe_package_id_t package_id,
                                                      fbe_bool_t *is_msg_present,
                                                      fbe_u32_t msg_id,
                                                      ...)
{
    fbe_status_t status;
    va_list args;
    fbe_event_log_check_msg_t event_log_msg;
    fbe_api_control_operation_status_info_t status_info;
    fbe_u32_t   currentTime = 0;

    *is_msg_present = FALSE;
    
    /* Initialized the fbe_event_log_check_msg_t struct*/
    event_log_msg.msg_id = msg_id;
    event_log_msg.is_msg_present = FALSE;
    va_start(args, msg_id);
    status = fbe_event_log_get_full_msg(event_log_msg.msg,
                                        sizeof(event_log_msg.msg), 
                                        msg_id, 
                                        args);
    va_end(args);

    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in retrieving the message string, Status:%d\n", __FUNCTION__, status);
        return status;
    }

    while(currentTime < timeoutMs)
    {
        status = fbe_api_common_send_control_packet_to_service(FBE_EVENT_LOG_CONTROL_CODE_CHECK_MSG,
                                                               &event_log_msg,
                                                               sizeof(fbe_event_log_check_msg_t),
                                                               FBE_SERVICE_ID_EVENT_LOG,
                                                               FBE_PACKET_FLAG_NO_ATTRIB,
                                                               &status_info,
                                                               package_id); 
        if (status != FBE_STATUS_OK) 
        {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet, Status:%d\n", __FUNCTION__, status); 
            return status;
        }
    
        *is_msg_present = event_log_msg.is_msg_present;
    
        if (*is_msg_present == FBE_TRUE)
        {
            return FBE_STATUS_OK;
        }
        currentTime = currentTime + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    return FBE_STATUS_GENERIC_FAILURE;
}
/********************************************
    end fbe_api_wait_for_event_log_msg()     
*********************************************/

/*!***************************************************************
 * @fn fbe_api_get_event_log(fbe_event_log_get_event_log_info_t *event_log_info,
 *                                     fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This fbe api used to get event log info for package.
 *
 * @param event_log_info    pointer to event log info
 * @param package_id    Package ID
 * 
 * @return fbe_status_t
 *
 * @author
 *  25-October - 2010: Vishnu Sharma Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_event_log(fbe_event_log_get_event_log_info_t *event_log_info,
                                                          fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    if(package_id <= FBE_PACKAGE_ID_INVALID || 
       package_id >= FBE_PACKAGE_ID_LAST) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_api_common_send_control_packet_to_service(FBE_EVENT_LOG_CONTROL_CODE_GET_LOG,
                                                           event_log_info,
                                                           sizeof(fbe_event_log_get_event_log_info_t),
                                                           FBE_SERVICE_ID_EVENT_LOG,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id); 
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:0x%x, packet qualifier:0x%x, payload error:0x%x, payload qualifier:0x%x\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/********************************************
    end fbe_api_get_event_log()     
*********************************************/

/*!***************************************************************
 * @fn be_api_get_all_events_logs(fbe_event_log_get_event_log_info_t *event_log_info, fbe_u32_t *actual_events,  fbe_u32_t total_events,
                                                          fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This fbe api used to get all events log info for package.
 *
 * @param event_log_info    pointer to event log info
 * @param fbe_u32_t *actual_events    number of events copied into buffer
 * @param fbe_u32_t total_events     total number of events that event buffer can accomodate
 * @param package_id    Package ID
 * 
 * @return fbe_status_t
 *
 * @author
 *  26-Feb-2011 Trupti Ghate Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_all_events_logs(fbe_event_log_get_event_log_info_t *event_log_info, fbe_u32_t *actual_events,  fbe_u32_t total_events,
                                                          fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sg_element_t                       sg_list[2];
    fbe_u8_t                                        sg_count = 0;
    fbe_event_log_get_all_events_log_info_t   enumerate_events_info;/*structure is small enough to be on the stack*/

    if(package_id <= FBE_PACKAGE_ID_INVALID || 
       package_id >= FBE_PACKAGE_ID_LAST) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    enumerate_events_info.total_events = total_events;
    /* Initialize the sg list */
    fbe_sg_element_init(sg_list, 
                        sizeof(fbe_event_log_get_event_log_info_t) * enumerate_events_info.total_events, 
                        event_log_info);
    sg_count++;
    /* no more elements*/
    fbe_sg_element_terminate(&sg_list[1]);
    sg_count++;
 
    status = fbe_api_common_send_control_packet_to_service_with_sg_list(FBE_EVENT_LOG_CONTROL_CODE_GET_ALL_LOGS,
                                                           &enumerate_events_info,
                                                           sizeof(fbe_event_log_get_all_events_log_info_t),
                                                           FBE_SERVICE_ID_EVENT_LOG,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           sg_list,
                                                           2,
                                                           &status_info,
                                                           package_id); 
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:0x%x, packet qualifier:0x%x\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier);
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:  payload error:0x%x, payload qualifier:0x%x\n", 
            __FUNCTION__,status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *actual_events = enumerate_events_info.total_events_copied;

    return status;
}
/********************************************
    end fbe_api_get_all_events_logs()     
*********************************************/


/*!***************************************************************
 * @fn fbe_api_get_event_log_statistics(fbe_event_log_statistics_t *event_log_statistics,
 *                                     fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This fbe api used to get event log statistics info for package.
 *
 * @param event_log_statistics    pointer to event log statistics
 * @param package_id    Package ID
 * 
 * @return fbe_status_t
 *
 * @author
 *  25-October - 2010: Vishnu Sharma Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_event_log_statistics(fbe_event_log_statistics_t *event_log_statistics,
                                                          fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    if(package_id <= FBE_PACKAGE_ID_INVALID || 
       package_id >= FBE_PACKAGE_ID_LAST) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_api_common_send_control_packet_to_service(FBE_EVENT_LOG_CONTROL_CODE_GET_STATISTICS,
                                                           event_log_statistics,
                                                           sizeof(fbe_event_log_statistics_t),
                                                           FBE_SERVICE_ID_EVENT_LOG,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id); 
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:0x%x, packet qualifier:0x%x, payload error:0x%x, payload qualifier:0x%x\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/********************************************
    end fbe_api_get_event_log_statistics()     
*********************************************/
/*!***************************************************************
 * @fn fbe_api_clear_event_log_statistics(fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This fbe api used to clear event log statistics info for package.
 *
 * @param package_id    Package ID
 * 
 * @return fbe_status_t
 *
 * @author
 *  25-October - 2010: Vishnu Sharma Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_clear_event_log_statistics(fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    if(package_id <= FBE_PACKAGE_ID_INVALID || 
       package_id >= FBE_PACKAGE_ID_LAST) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_api_common_send_control_packet_to_service(FBE_EVENT_LOG_CONTROL_CODE_CLEAR_STATISTICS,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_EVENT_LOG,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id); 
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:0x%x, packet qualifier:0x%x, payload error:0x%x, payload qualifier:0x%x\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/********************************************
    end fbe_api_clear_event_log_statistics()     
*********************************************/

/*!***************************************************************
 * @fn fbe_api_event_log_disable()
 ****************************************************************
 * @brief
 *  Stop logging events.  Used for error injection
 *  testing when lots of errors are injected.
 *
 * @param package_id Package ID
 * 
 * @return fbe_status_t 
 *
 * @author
 *  5/15/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_event_log_disable(fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_EVENT_LOG_CONTROL_CODE_DISABLE_TRACE,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_EVENT_LOG,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id); 
    return status;
}
/**************************************
 * end fbe_api_event_log_disable
 **************************************/

/*!***************************************************************
 * @fn fbe_api_event_log_enable()
 ****************************************************************
 * @brief
 *  Start logging events.
 *
 * @param package_id Package ID
 * 
 * @return fbe_status_t 
 *
 * @author
 *  5/15/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_event_log_enable(fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_EVENT_LOG_CONTROL_CODE_ENABLE_TRACE,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_EVENT_LOG,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id); 
    return status;
}
/**************************************
 * end fbe_api_event_log_enable
 **************************************/
