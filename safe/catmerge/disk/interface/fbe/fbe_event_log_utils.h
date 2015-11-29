#ifndef FBE_EVENT_LOG_UTILS_H
#define FBE_EVENT_LOG_UTILS_H
/******************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *****
 /*!****************************************************************************
 * @file fbe_event_log_utils.h
 ******************************************************************************
 *
 * @brief
 *  This file contain declaration of function which use for event log message.
 *
 * @version
 *   28-June-2010: Vaibhav Gaonkar Created
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "EspMessages.h"
#include "PhysicalPackageMessages.h"
#include "SEPMessages.h"


 /* Sample Test message to test event log infrastructure.
  * The message string added for Physical Package.
  */
#define SAMPLE_TEST_MESSAGE  0Xe11c8FFF  // Warning severity message 
#define SAMPLE_TEST_MESSAGE_STRING "**This message string is 256 char long to test the buffer overflow ************************************************************************************************************************************************************************************************"




fbe_status_t
fbe_event_log_get_full_msg(fbe_u8_t *msg,
                           fbe_u32_t  msg_string_buffer_len,
                           fbe_u32_t msg_id,
                           va_list arg);

/* ESP Event log Messages*/
fbe_status_t 
fbe_event_log_get_esp_msg(fbe_u32_t msg_id, 
                          fbe_u8_t *msg,
                          fbe_u32_t msg_len);

/* Physical Package Event log Meassages*/
fbe_status_t 
fbe_event_log_get_physical_msg(fbe_u32_t msg_id, 
                               fbe_u8_t *msg,
                               fbe_u32_t msg_len);

/* SEP Event log Messages*/
fbe_status_t 
fbe_event_log_get_sep_msg(fbe_u32_t msg_id, 
                          fbe_u8_t *msg,
                          fbe_u32_t msg_len);

#endif /*end of FBE_EVENT_LOG_UTILS_H*/
