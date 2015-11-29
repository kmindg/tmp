#ifndef FBE_EVENT_LOG_API_H
#define FBE_EVENT_LOG_API_H
/******************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *****************************************************************************/

/*!****************************************************************************
 * @file fbe_event_log_api.h
 ******************************************************************************
 *
 * @brief
 *  This file contain declaration of function which use for event log service.
 *
 * @version
 *   20-May-2010 - Vaibhav Gaonkar Created
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_time.h"


/* Number of messages that will be saved */
#define FBE_EVENT_LOG_MESSAGE_COUNT  500

#define FBE_EVENT_LOG_MESSAGE_STRING_LENGTH 256

/*!************************************************************
* @enum fbe_event_log_control_code_t
*
* @brief 
*    This contains the enum data info for event log service control code.
*
***************************************************************/
typedef enum fbe_event_logcontrol_code_e {
    FBE_EVENT_LOG_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_EVENT_LOG),  /*!< Invalid Control code*/
    FBE_EVENT_LOG_CONTROL_CODE_GET_STATISTICS,    /*!<Get event log statistics */
    FBE_EVENT_LOG_CONTROL_CODE_CLEAR_LOG,         /*!<Clear the event log */
    FBE_EVENT_LOG_CONTROL_CODE_GET_MSG_COUNT,     /*!<Message count in the event log */
    FBE_EVENT_LOG_CONTROL_CODE_CHECK_MSG,         /* !< Check specific message exist in event log or not */
    FBE_EVENT_LOG_CONTROL_CODE_GET_LOG,           /*!<Get the Message  in the event log */
    FBE_EVENT_LOG_CONTROL_CODE_CLEAR_STATISTICS,  /*!<Clear the event log  statistics*/
    FBE_EVENT_LOG_CONTROL_CODE_GET_ALL_LOGS,           /*!<Get all the Messages in the event log */
    FBE_EVENT_LOG_CONTROL_CODE_DISABLE_TRACE,           /*!<Stop logging */
    FBE_EVENT_LOG_CONTROL_CODE_ENABLE_TRACE,           /*!< Start logging again */
    FBE_EVENT_LOG_CONTROL_CODE_LAST               /*!<Last control code*/

} fbe_event_log_control_code_t;


/*!********************************************************************* 
 * @struct fbe_event_log_info_t 
 *  
 * @brief 
 *  Contain event log data
 *
 **********************************************************************/
typedef struct fbe_event_log_info_s
{
    fbe_u32_t msg_id;
    fbe_u8_t msg_string[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH];
    fbe_system_time_t time_stamp;
}fbe_event_log_info_t;


fbe_status_t fbe_event_log_write(fbe_u32_t msg_id,
                                 fbe_u8_t *dump_data,
                                 fbe_u32_t dump_data_size,
                                 csx_char_t *fmt, 
                                 ...) __attribute__((format(__printf_func__,4,5)));



#endif /* FBE_EVENT_LOG_API_H */
