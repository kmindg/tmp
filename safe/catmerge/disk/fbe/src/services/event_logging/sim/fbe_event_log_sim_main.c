/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/
/*!****************************************************************************
 * @file fbe_event_log_sim_main.c
 ******************************************************************************
 *
 * @brief
 *  This file contains functions related to handling of Event
 *  Logging service.for simulator.
 *
 ***************************************************************************/
#include "fbe_event_log.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_event_log_utils.h"


static fbe_event_log_info_t event_log[FBE_EVENT_LOG_MESSAGE_COUNT];
static fbe_u32_t current_index = 0;
static fbe_bool_t  is_complete_buffer_in_use = FBE_FALSE; /* to determine if ring buffer is full and we have started
                                                                                        * wrap around in ring
                                                                                        */
static fbe_spinlock_t event_log_lock;

/************************************************************************************
 *                          fbe_event_log_write_log
 ************************************************************************************
 *
 * DESCRIPTION:
 *   This is the simulation specific funtion to actually write
 *   the events to the logs(in this case we just save it to
 *   memory)
 *
 *  PARAMETERS:
 *      msg_id - Message ID
 *      dump_data - Buffer that contains raw data that needs to
 *      be saved
 *      dump_data_size - Size of the dump data buffer
 *      fmt - Insertion strings
 * 
 *  RETURN VALUES/ERRORS:
 *       fbe_status_t
 *
 *  NOTES:
 *      
 * HISTORY:
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 ***********************************************************************************/
fbe_status_t fbe_event_log_write_log(fbe_u32_t msg_id,
                                     fbe_u8_t *dump_data,
                                     fbe_u32_t dump_data_size,
                                     fbe_char_t *fmt,
                                     va_list argList)
{
    fbe_status_t status;
    fbe_u32_t local_current_index = 0;
    fbe_u8_t full_msg_string[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH];

    fbe_zero_memory(full_msg_string, sizeof(full_msg_string));
    status = fbe_event_log_get_full_msg(full_msg_string,
                                         FBE_EVENT_LOG_MESSAGE_STRING_LENGTH,
                                         msg_id,
                                         argList);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Get the current_index value locally */
    fbe_spinlock_lock(&event_log_lock);
    if(current_index >= FBE_EVENT_LOG_MESSAGE_COUNT) 
    {
        current_index = 0;
        is_complete_buffer_in_use = FBE_TRUE;
    }
    local_current_index = current_index; 
    current_index++;
    fbe_spinlock_unlock(&event_log_lock);

    /* Set the event_log for local_current_index*/
    fbe_copy_memory(event_log[local_current_index].msg_string,full_msg_string, sizeof(full_msg_string));
    event_log[local_current_index].msg_id = msg_id;
    fbe_get_system_time(&(event_log[local_current_index].time_stamp));

    return status;
}

/************************************************************************************
 *                          fbe_event_log_init
 ************************************************************************************
 *
 * DESCRIPTION:
 *   This function that initializes the event log data
 *   structures
 *
 *  PARAMETERS:
 *      None
 * 
 *  RETURN VALUES/ERRORS:
 *       fbe_status_t
 *
 *  NOTES:
 *      
 * HISTORY:
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 ***********************************************************************************/
fbe_status_t fbe_event_log_init_log()
{
    fbe_spinlock_init(&event_log_lock);
    current_index = 0;
    is_complete_buffer_in_use = FBE_FALSE;
    fbe_zero_memory(&event_log, sizeof(event_log));
    return FBE_STATUS_OK;
}

/************************************************************************************
 *                          fbe_event_log_clear_log
 ************************************************************************************
 *
 * DESCRIPTION:
 *   This function clears the events from the logs 
 *
 *  PARAMETERS:
 *      None
 * 
 *  RETURN VALUES/ERRORS:
 *       fbe_status_t
 *
 *  NOTES:
 *      
 * HISTORY:
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 ***********************************************************************************/
fbe_status_t fbe_event_log_clear_log()
{
    fbe_spinlock_lock(&event_log_lock);
    current_index = 0;
    is_complete_buffer_in_use = FBE_FALSE;    
    fbe_spinlock_unlock(&event_log_lock);
    return FBE_STATUS_OK;
}

/************************************************************************************
 *                          fbe_event_log_destroy_log
 ************************************************************************************
 *
 * DESCRIPTION:
 *   This function performs the necessary clean up that needs to
 *   be done when the event log service is destroyed
 *
 *  PARAMETERS:
 *      None
 * 
 *  RETURN VALUES/ERRORS:
 *       fbe_status_t
 *
 *  NOTES:
 *      
 * HISTORY:
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 ***********************************************************************************/
fbe_status_t fbe_event_log_destroy_log()
{
    fbe_spinlock_destroy(&event_log_lock);
    return FBE_STATUS_OK;
}

/************************************************************************************
 *                          fbe_event_log_get_msg_count
 ************************************************************************************
 *
 * DESCRIPTION:
 *   This functions returns the count of message ID requested
 *
 *  PARAMETERS:
 *      msg_id - THe message ID that needs to be searched for
 *      count - Buffer to store the count
 * 
 *  RETURN VALUES/ERRORS:
 *       fbe_status_t
 *
 *  NOTES:
 *      
 * HISTORY:
 *      04-Mar-2009    Ashok Tamilarsasan  Created.
 *
 ***********************************************************************************/
fbe_status_t fbe_event_log_get_msg_count(fbe_u32_t msg_id, fbe_u32_t *count)
{

    fbe_u32_t i;
    fbe_u32_t   end_scan_index;
    fbe_spinlock_lock(&event_log_lock);
    if ((current_index == 0) && (is_complete_buffer_in_use == FBE_FALSE))
    {
        fbe_spinlock_unlock(&event_log_lock);
        *count = 0;
        return FBE_STATUS_OK;
    }

    if(is_complete_buffer_in_use == FBE_TRUE)
    {
        end_scan_index = FBE_EVENT_LOG_MESSAGE_COUNT;
    }
    else
    {
        end_scan_index = current_index;
    }
        

    for(i = 0; i < end_scan_index; i++) 
    {
        if(event_log[i].msg_id == msg_id) 
        {
            (*count)++;
        }
    }
 
    fbe_spinlock_unlock(&event_log_lock);
    return FBE_STATUS_OK;
}

/**************************************
* end of fbe_event_log_get_msg_count()
***************************************/
/*!***************************************************************
 * @fn fbe_event_log_message_check(fbe_u32_t msg_id, fbe_u8_t *msg)
 ****************************************************************
 * @brief
 *  This function is used to check particular message present in
 *  event log.
 *
 * @param msg_id Message ID 
 * @param msg    Message Sting
 * 
 * @return fbe_bool_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
fbe_bool_t 
fbe_event_log_message_check(fbe_u32_t msg_id, fbe_u8_t *msg)
{
    fbe_u32_t index;
    fbe_u32_t   local_current_index;

    fbe_spinlock_lock(&event_log_lock);
    if ((current_index == 0) && (is_complete_buffer_in_use == FBE_FALSE))
    {
        fbe_spinlock_unlock(&event_log_lock);
        return FBE_FALSE;
    }

    /* Scan the event records from latest to older one in circular fashion to make the search efficient.
     * If we are utilize full buffer then we need to scan entire ring buffer. current_index is
     * the index where we are going to insert next event.
     */
    local_current_index= current_index ;
    for(index = 0; index <FBE_EVENT_LOG_MESSAGE_COUNT; index++)
    {

        local_current_index --;

        if(( is_complete_buffer_in_use == FBE_FALSE ) && (index >= current_index))
        {
            break;
        }

        if(event_log[local_current_index].msg_id == msg_id)
        {
            if(!strcmp(event_log[local_current_index].msg_string, msg))
            {
                fbe_spinlock_unlock(&event_log_lock);
                return FBE_TRUE;
            } 
        }

        if(local_current_index == 0)
        {
            /* set the search index at the end of buffer */
            local_current_index = FBE_EVENT_LOG_MESSAGE_COUNT;
        }
        
    }
   
    fbe_spinlock_unlock(&event_log_lock);
    return FBE_FALSE;
}
/**************************************
* end of fbe_event_log_message_check()
***************************************/
/*!****************************************************************************
 *@fn fbe_event_log_get_log(fbe_u32_t event_log_index,fbe_event_log_info_t *event_log_info)
 ******************************************************************************
 *
 * @brief
 *      This is the handler function for returning the event log
 *      info at the given index.
 *
 * @param event_log_index - index of  event log info.
 * @param event_log_info - pointer to event log info structure.
 * 
 * @return      fbe_status_t
 *      
 * @author
 *      10-October-2010    Vishnu Sharma  Created.
 *
 ***********************************************************************************/
fbe_status_t fbe_event_log_get_log(fbe_u32_t event_log_index,fbe_event_log_info_t *event_log_info)
{

    fbe_spinlock_lock(&event_log_lock);
    if(is_complete_buffer_in_use == FBE_FALSE)
    {
        if(current_index == 0 || event_log_index >= current_index) 
        {
            fbe_spinlock_unlock(&event_log_lock);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        if( current_index == 0 || event_log_index >= FBE_EVENT_LOG_MESSAGE_COUNT) 
        {
            fbe_spinlock_unlock(&event_log_lock);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    
    fbe_copy_memory(event_log_info,(event_log + event_log_index),sizeof(fbe_event_log_info_t));
    fbe_spinlock_unlock(&event_log_lock);
    return FBE_STATUS_OK;

}
/**************************************
* end of fbe_event_log_get_log()
***************************************/

fbe_u32_t  fbe_event_get_current_index()
{
     return current_index;
}

fbe_bool_t  fbe_event_get_is_complete_buffer_in_use()
{
     return is_complete_buffer_in_use;
}