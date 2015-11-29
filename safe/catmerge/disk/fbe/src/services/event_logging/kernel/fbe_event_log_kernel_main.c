#include <ntddk.h>
#include "fbe_event_log.h"
#include "klogservice.h"
#include "fbe/fbe_event_log_utils.h"
#include "EmcUTIL.h"


EMCPAL_STATUS get_package_device_object(PEMCPAL_DEVICE_OBJECT *device_object);

static fbe_event_log_info_t event_log[FBE_EVENT_LOG_MESSAGE_COUNT];
static fbe_u32_t current_index = 0;
static fbe_spinlock_t event_log_lock;    /* Used for accessing current_index value */
static fbe_bool_t  is_complete_buffer_in_use = FBE_FALSE; /* to determine if ring buffer is full and we have started
                                                                                        * wrap around in ring
                                                                                        */


/************************************************************************************
 *                          fbe_event_log_write_log
 ************************************************************************************
 *
 * DESCRIPTION:
 *   This is the kernel specific funtion to actually write
 *   the events to the logs
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
    PEMCPAL_DEVICE_OBJECT device_object = NULL;
    fbe_status_t status;
    KLOG_RC tstatus;
    fbe_u32_t local_current_index = 0;
    fbe_u8_t full_msg_string[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH];
#ifndef ALAMOSA_WINDOWS_ENV
    va_list argListCopy;
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - need to do va_copy() to use list multiple times */

    /*Set the event log to windows event log service*/
    get_package_device_object(&device_object);

#ifdef ALAMOSA_WINDOWS_ENV
    tstatus = EmcutilEventLogVector(msg_id, dump_data_size,
                                 dump_data, fmt, argList);
#else
    va_copy(argListCopy, argList);
    tstatus = EmcutilEventLogVector(msg_id, dump_data_size,
                                 dump_data, fmt, argListCopy);
    va_end(argListCopy);
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - need to do va_copy() to use list multiple times */

    //If message is not successfully logged return error
    if (tstatus != LRCSuccess)
    {
        return FBE_STATUS_GENERIC_FAILURE ;
    }

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
    return(status);
    
}
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
/* The following functions are not implemented in the kernel
 *  space because there is no windows kernel interface to do
 *  these things currently and also we dont want to expose this
*   functionality in kernel space. So these are just stubs for
*   now
 */
fbe_status_t fbe_event_log_init_log(void)
{
    fbe_spinlock_init(&event_log_lock);
    current_index = 0;
    is_complete_buffer_in_use = FBE_FALSE;
    fbe_zero_memory(&event_log, sizeof(event_log));
    return FBE_STATUS_OK;
}

fbe_status_t fbe_event_log_destroy_log(void)
{
    fbe_spinlock_destroy(&event_log_lock);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_event_log_clear_log(void)
{
    fbe_spinlock_lock(&event_log_lock);
    current_index = 0;
    is_complete_buffer_in_use = FBE_FALSE;
    fbe_spinlock_unlock(&event_log_lock);
    return FBE_STATUS_OK;
}


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

fbe_bool_t fbe_event_log_message_check(fbe_u32_t msg_id, fbe_u8_t  *msg)
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

fbe_u32_t  fbe_event_get_current_index(void)
{
     return current_index;
}
fbe_bool_t  fbe_event_get_is_complete_buffer_in_use(void)
{
     return is_complete_buffer_in_use;
}
