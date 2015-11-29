/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_event_log_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contain function that return message string for given msg_id
 * 
 * @version
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe/fbe_event_log_api.h"


/*!***************************************************************
 * @fn fbe_event_log_get_full_msg(fbe_u8_t *msg,
 *                                fbe_u32_t msg_string_buffer_len,
 *                                fbe_u32_t  msg_id,
 *                                va_list arg)
 ****************************************************************
 * @brief
 *  This function is used to retrive the event log message string.
 *
 * @param msg_string_buffer Buffer to store message string
 * @param msg_string_buffer_len Buffer length
 * @param msg_id Message ID
 * @param arg    Argument list
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_event_log_get_full_msg(fbe_u8_t *msg_string_buffer,
                           fbe_u32_t msg_string_buffer_len,
                           fbe_u32_t msg_id,
                           va_list arg)
{
    fbe_status_t status;
    fbe_u8_t full_msg[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH];
    fbe_u32_t mask_bit =  0x0FFFF000;
    fbe_u32_t package_msg_code;

    fbe_zero_memory(full_msg, sizeof(full_msg));
    if (msg_string_buffer_len < FBE_EVENT_LOG_MESSAGE_STRING_LENGTH)
    {
        KvTrace("fbe_event_log_utils.c : %s Insufficient buffer size \n", __FUNCTION__);
        return  FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check the msg_id belongs to which package.*/
    package_msg_code = mask_bit & msg_id;
    package_msg_code = package_msg_code >> 16;

    switch(package_msg_code)
    {
 
        case FACILITY_ESP_PACKAGE:
            status = fbe_event_log_get_esp_msg(msg_id, full_msg, sizeof(full_msg));
        break;
        
        case FACILITY_PHYSICAL_PACKAGE:
            status =  fbe_event_log_get_physical_msg(msg_id, full_msg, sizeof(full_msg));
        break;

        case FACILITY_SEP_PACKAGE:
            status = fbe_event_log_get_sep_msg(msg_id, full_msg, sizeof(full_msg));
        break;

        default:
            KvTrace("fbe_event_log_utils.c : %s Invalid Message ID: 0x%x\n", __FUNCTION__, msg_id);
            return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if(status ==  FBE_STATUS_OK)
    {
        csx_size_t required = csx_p_vsnprintf(msg_string_buffer, 
                                    (FBE_EVENT_LOG_MESSAGE_STRING_LENGTH), 
                                    full_msg, 
                                    arg);
        if(required > FBE_EVENT_LOG_MESSAGE_STRING_LENGTH)
        {
            KvTrace("fbe_event_log_utils.c : %s Insufficient buffer size \n",__FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
       KvTrace("fbe_event_log_utils.c : %s Message string does not exist for MsgID : 0x%x\n", __FUNCTION__, msg_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return FBE_STATUS_OK;
}
