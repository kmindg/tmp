/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_job_service_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the job service.
 *
 * @author
 *  23-Dec-2010 - Created. Hari Singh Chauhan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object.h"
#include "fbe_job_service_debug.h"
#include "fbe_job_service.h"
#include "fbe_job_service_private.h"
#include "pp_ext.h"

char *fbe_job_service_getJobTypeString(fbe_job_type_t job_type);
char *fbe_job_service_getJobActionStateString(fbe_job_action_state_t job_action_state);
char *fbe_job_service_getThreadFlagStatusString(fbe_job_service_thread_flags_t thread_flag);
fbe_status_t fbe_job_service_command_data_debug_trace(const fbe_u8_t * module_name,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_dbgext_ptr command_data,
                                                      fbe_job_type_t job_type);


/*!*******************************************************************
 * @var fbe_base_service_field_info
 *********************************************************************
 * @brief Information about each of the fields of base service
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_service_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("trace_level", fbe_trace_level_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("initialized", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_base_service_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base service.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_service_t,
                              fbe_base_service_struct_info,
                              &fbe_base_service_field_info[0]);


/*!**************************************************************
 * fbe_base_service_debug_trace()
 ****************************************************************
 * @brief
 *  Display info on base service element.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t
 *
 * @author
 *  23- Dec- 2010 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_base_service_debug_trace(const fbe_u8_t * module_name,
                                          fbe_dbgext_ptr base_p,
                                          fbe_trace_func_t trace_func,
                                          fbe_trace_context_t trace_context,
                                          fbe_debug_field_info_t *field_info_p,
                                          fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t base_service_p = base_p + field_info_p->offset;

    trace_func(NULL,"\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(NULL,"service_id: %s","FBE_SERVICE_ID_JOB_SERVICE");

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, base_service_p + field_info_p->offset,
                                         &fbe_base_service_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    return status;
}


/*!*******************************************************************
 * @var fbe_job_service_field_info
 *********************************************************************
 * @brief Information about each of the fields of job service
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_job_service_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_service", fbe_base_service_t, FBE_TRUE, "0x%x",
                                     fbe_base_service_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("job_number", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("job_packet_p", fbe_packet_t, FBE_FALSE, "0x%x",
                                     fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_job_service_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        job service.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_job_service_t,
                              fbe_job_service_struct_info,
                              &fbe_job_service_field_info[0]);

/*!**************************************************************
 * fbe_job_service_debug_trace()
 ****************************************************************
 * @brief
 *  Display job_service Information.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - context to send to trace function
 * @param last_queue_element - This is the queue element number.
 * @param queue_type - This is the queue type value. * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  23- Dec-2010 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_job_service_debug_trace(const fbe_u8_t * module_name,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
                                         fbe_u64_t last_queue_element,
                                         fbe_job_control_queue_type_t queue_type)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_dbgext_ptr job_service_ptr = 0;
    fbe_u32_t offset =0;
    fbe_u32_t spaces_to_indent = 4;
    fbe_bool_t queue_access = 0;
    fbe_u32_t number_objects =0;
    fbe_u32_t job_service_ptr_size = 0;
    fbe_job_service_thread_flags_t thread_flags = 0;

    /* Get the job service. */
    FBE_GET_EXPRESSION(module_name, fbe_job_service, &job_service_ptr);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(NULL, "job_service_ptr: 0x%llx\n",
           (unsigned long long)job_service_ptr);

    /* Get the size of the job service. */
    FBE_GET_TYPE_SIZE(module_name, fbe_job_service_t, &job_service_ptr_size);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(NULL, "job_service_size: %d \n", job_service_ptr_size);

    if(job_service_ptr == 0){
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL, "job_service_ptr is not available \n");
        return status;
    }

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, job_service_ptr,
                                         &fbe_job_service_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status != FBE_STATUS_OK) 
    { 
        return status; 
    }
    /* Display the job service  information.
     */
    FBE_GET_FIELD_OFFSET(module_name,fbe_job_service_t,"thread_flags",&offset);
    FBE_READ_MEMORY(job_service_ptr + offset, &thread_flags, sizeof(fbe_job_service_thread_flags_t));

    trace_func(NULL,"\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(NULL,"thread_flags: %s",fbe_job_service_getThreadFlagStatusString(thread_flags));

    if(queue_type == FBE_JOB_CREATION_QUEUE || queue_type == FBE_JOB_INVALID_QUEUE)
    {
        FBE_GET_FIELD_OFFSET(module_name,fbe_job_service_t,"number_creation_objects",&offset);
        FBE_READ_MEMORY(job_service_ptr + offset, &number_objects, sizeof(fbe_u32_t));

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"number_creation_objects: %u",number_objects);

        FBE_GET_FIELD_OFFSET(module_name,fbe_job_service_t,"creation_queue_access",&offset);
        FBE_READ_MEMORY(job_service_ptr + offset, &queue_access, sizeof(fbe_bool_t));

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"creation_queue_access: %u",queue_access);

        FBE_GET_FIELD_OFFSET(module_name,fbe_job_service_t,"creation_q_head",&offset);

       /* Do not display job service queues if active queue display is disabled 
         * using !disable_active_queue_display' debug macro
         */
        if(get_active_queue_display_flag() == FBE_TRUE)
        {
            trace_func(NULL,"\n");
            fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
            trace_func(NULL,"creation_q_head:");
            status = fbe_job_service_queue_debug_trace(module_name, trace_func, trace_context, job_service_ptr + offset, last_queue_element, FBE_JOB_CREATION_QUEUE);
        }
        if(status != FBE_STATUS_OK) 
        {
            return status; 
        }
    }

    if(queue_type == FBE_JOB_RECOVERY_QUEUE || queue_type == FBE_JOB_INVALID_QUEUE)
    {
        FBE_GET_FIELD_OFFSET(module_name,fbe_job_service_t,"number_recovery_objects",&offset);
        FBE_READ_MEMORY(job_service_ptr + offset, &number_objects, sizeof(fbe_u32_t));

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"number_recovery_objects: %u",number_objects);

        FBE_GET_FIELD_OFFSET(module_name,fbe_job_service_t,"recovery_queue_access",&offset);
        FBE_READ_MEMORY(job_service_ptr + offset, &queue_access, sizeof(fbe_bool_t));

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"recovery_queue_access: %u",queue_access);

        FBE_GET_FIELD_OFFSET(module_name,fbe_job_service_t,"recovery_q_head",&offset);

        /* check if queue display is not disabled using !disable_active_queue_display*/
        if(get_active_queue_display_flag() == FBE_TRUE)
        {
            trace_func(NULL,"\n");
            fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
            trace_func(NULL,"recovery_q_head:");
            status = fbe_job_service_queue_debug_trace(module_name, trace_func, trace_context, job_service_ptr + offset, last_queue_element, FBE_JOB_RECOVERY_QUEUE);
        }
    }

    trace_func(NULL,"\n");

    return status;
}

/*!**************************************************************
 * fbe_job_service_queue_element_debug_trace()
 ****************************************************************
 * @brief
 *  Display job_service Queue Elements Information.
 * 
 * @param  module_name - This is the name of the module we are debugging.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - context to send to trace function
 * @param queue_element - job queue elements Offset Value.  
 * @param last_queue_element - Number which represents How
 *                                           many queue element display from last 
 * @param queue_type - This is the queue type value. *
 * 
 * @return fbe_status_t
 *
 * @author
 *  23- Dec-2010 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_job_service_queue_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr queue_element,
                                               fbe_u64_t last_queue_element,
                                               fbe_job_control_queue_type_t queue_type)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 8;
    fbe_u64_t start_head;
    fbe_u64_t queue_head = 0;
    fbe_u64_t queue_head_element = 0;
    fbe_u64_t queue_next_head = 0;
    fbe_u32_t queue_index = 0;

    FBE_READ_MEMORY(queue_element, &start_head, sizeof(fbe_u64_t));

    if(start_head != NULL)
    {
        FBE_READ_MEMORY(start_head, &queue_head, sizeof(fbe_u64_t));
        queue_head_element = queue_head;

        if(start_head == queue_head)
        {
            trace_func(NULL,"\n");
            fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
            trace_func(NULL, "queue (queue_head): %s","EMPTY");
            return FBE_STATUS_OK;
        }
        else
        {
            status = fbe_job_service_queue_element_debug_trace(module_name, trace_func, trace_context, start_head, queue_index);

            if(status != FBE_STATUS_OK)
            {
                return status;
            }

            FBE_READ_MEMORY(queue_head_element, &queue_head, sizeof(fbe_u64_t));
            queue_next_head = queue_head;
            ++queue_index;

            while(start_head != queue_head_element && start_head != queue_next_head)
            {
                if(FBE_CHECK_CONTROL_C())
                {
                    return FBE_STATUS_CANCELED;
                }

                status = fbe_job_service_queue_element_debug_trace(module_name, trace_func, trace_context, queue_head_element, queue_index);

                if(status != FBE_STATUS_OK)
                {
                    return status;
                }

                FBE_READ_MEMORY(queue_head_element, &queue_head, sizeof(fbe_u64_t));
                queue_head_element = queue_head;
                ++queue_index;

                FBE_READ_MEMORY(queue_head_element, &queue_head, sizeof(fbe_u64_t));
                queue_next_head = queue_head;

                if(last_queue_element && (queue_index >= last_queue_element))
                    break;
            }
        }

        if(last_queue_element > queue_index)
        {
            trace_func(NULL,"\n\n");
            fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
            trace_func(NULL, "Only %d elements is in queue", queue_index);
        }
        else if(queue_type == FBE_JOB_CREATION_QUEUE)
        {
            trace_func(NULL,"\n\n");
            fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
            trace_func(NULL, "Number of jobs in Creation_q_head : %d", queue_index);
        }
        else if(queue_type == FBE_JOB_RECOVERY_QUEUE)
        {
            trace_func(NULL,"\n\n");
            fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
            trace_func(NULL, "Number of jobs in Recovery_q_head : %d", queue_index);
        }

        trace_func(NULL,"\n");
    }

    return status;
}


/*!*******************************************************************
 * @var fbe_job_service_queue_element_field_info
 *********************************************************************
 * @brief Information about each of the fields of job service queue element
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_job_service_queue_element_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("timestamp", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("queue_access", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("job_number", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_objects", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};


/*!*******************************************************************
 * @var fbe_job_queue_element_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        job service queue element.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_job_queue_element_t,
                              fbe_job_service_queue_element_struct_info,
                              &fbe_job_service_queue_element_field_info[0]);


/*!**************************************************************
 * fbe_job_service_queue_element_debug_trace()
 ****************************************************************
 * @brief
 *  Display info on job service queue element.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param queue_head - Offset value of queue_head.
 * @param queue_index - queue element index. 
 * @param queue_type - This is the queue type value.
 * 
 * @return fbe_status_t
 *
 * @author
 *  23- Dec- 2010 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_job_service_queue_element_debug_trace(const fbe_u8_t * module_name,
                                                       fbe_trace_func_t trace_func,
                                                       fbe_trace_context_t trace_context,
                                                       fbe_u64_t queue_head,
                                                       fbe_u32_t queue_index)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t offset = 0;
    fbe_u32_t spaces_to_indent = 8;
    fbe_job_type_t job_type = 0;
    fbe_object_id_t object_id = 0;
    fbe_job_action_state_t state = FBE_JOB_ACTION_STATE_INVALID;
    ULONG64 max_toplogy_entries_p = 0;
    fbe_u32_t max_entries;
    
    trace_func(NULL,"\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(NULL,"index:%d",queue_index);

    FBE_GET_FIELD_OFFSET(module_name,fbe_job_queue_element_t,"job_type",&offset);
    FBE_READ_MEMORY(queue_head + offset, &job_type, sizeof(fbe_job_type_t));

    trace_func(NULL,"\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
    trace_func(NULL,"job_type: %s",fbe_job_service_getJobTypeString(job_type));

    FBE_GET_FIELD_OFFSET(module_name,fbe_job_queue_element_t,"object_id",&offset);
    FBE_READ_MEMORY(queue_head + offset, &object_id, sizeof(fbe_object_id_t));

    FBE_GET_EXPRESSION(module_name, max_supported_objects, &max_toplogy_entries_p);
    FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    trace_func(NULL,"\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
    if(object_id > max_entries)
        trace_func(NULL,"object_id: %s","INVALID_OBJECT_ID");
    else
        trace_func(NULL,"object_id: 0x%x",object_id);

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, queue_head,
                                         &fbe_job_service_queue_element_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 4);

    if(status != FBE_STATUS_OK) 
    {
        return status; 
    }
    /* Display the job service queue element  information.
     */
    FBE_GET_FIELD_OFFSET(module_name,fbe_job_queue_element_t,"current_state",&offset);
    FBE_READ_MEMORY(queue_head + offset, &state, sizeof(fbe_job_action_state_t));

    trace_func(NULL,"\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
    trace_func(NULL,"current_state: %s",fbe_job_service_getJobActionStateString(state));

    FBE_GET_FIELD_OFFSET(module_name,fbe_job_queue_element_t,"previous_state",&offset);
    FBE_READ_MEMORY(queue_head + offset, &state, sizeof(fbe_job_action_state_t));

    trace_func(NULL,"\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
    trace_func(NULL,"previous_state: %s",fbe_job_service_getJobActionStateString(state));

    FBE_GET_FIELD_OFFSET(module_name,fbe_job_queue_element_t,"command_data",&offset);

    trace_func(NULL,"\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
    trace_func(NULL,"command_data:");

    fbe_job_service_command_data_debug_trace(module_name, trace_func, trace_context, queue_head + offset, job_type);

    return status;
}

/*!**************************************************************
 * fbe_job_service_command_data_debug_trace()
 ****************************************************************
 * @brief
 *  Display job_service Command Data Information. like lun create info, raid create info etc.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param command_data - command_data Offset Value. * 
 * @param job_type - enum job type value.
 *
 * @return fbe_status_t
 *
 * @author
 *  30- Dec-2010 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_job_service_command_data_debug_trace(const fbe_u8_t * module_name,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_dbgext_ptr command_data,
                                                      fbe_job_type_t job_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(job_type)
    {
        case FBE_JOB_TYPE_DRIVE_SWAP:
                status = fbe_job_service_drive_swap_info_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
        case FBE_JOB_TYPE_RAID_GROUP_CREATE :
                status = fbe_job_service_raid_group_create_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
        case FBE_JOB_TYPE_LUN_CREATE:
                status = fbe_job_service_lun_create_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
        case FBE_JOB_TYPE_LUN_DESTROY:
                status = fbe_job_service_lun_destroy_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
        case FBE_JOB_TYPE_RAID_GROUP_DESTROY:
                status = fbe_job_service_raid_group_destroy_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
        case FBE_JOB_TYPE_UPDATE_RAID_GROUP:
                status = fbe_job_service_update_raid_group_debug_trace(module_name, trace_func, trace_context,command_data);
                break;
        case FBE_JOB_TYPE_UPDATE_VIRTUAL_DRIVE:
                status = fbe_job_service_update_virtual_drive_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
        case FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE:
                status = fbe_job_service_update_provision_drive_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
        case FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE:
                status = fbe_job_service_update_provision_drive_block_size_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
        case FBE_JOB_TYPE_UPDATE_SPARE_CONFIG:
                status = fbe_job_service_update_spare_config_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
        case FBE_JOB_TYPE_CHANGE_SYSTEM_POWER_SAVING_INFO:
                status = fbe_job_service_change_system_power_saving_info_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
        case FBE_JOB_TYPE_UPDATE_DB_ON_PEER:
                status = fbe_job_service_update_db_on_peer_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
        case FBE_JOB_TYPE_UPDATE_JOB_ELEMENTS_ON_PEER:
                //status = fbe_job_service_update_job_elements_on_peer_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
        case FBE_JOB_TYPE_CHANGE_SYSTEM_ENCRYPTION_INFO:
                status = fbe_job_service_change_system_encryption_info_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
         case FBE_JOB_TYPE_SET_ENCRYPTION_PAUSED:
                status = fbe_job_service_change_system_encryption_info_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
        case FBE_JOB_TYPE_UPDATE_ENCRYPTION_MODE:
                //status = fbe_job_service_change_system_encryption_info_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
        case FBE_JOB_TYPE_SCRUB_OLD_USER_DATA:
                break;
        case FBE_JOB_TYPE_VALIDATE_DATABASE:
                status = fbe_job_service_validate_database_debug_trace(module_name, trace_func, trace_context, command_data);
                break;
        case FBE_JOB_TYPE_CREATE_EXTENT_POOL:
                break;
        case FBE_JOB_TYPE_CREATE_EXTENT_POOL_LUN:
                break;
        case FBE_JOB_TYPE_DESTROY_EXTENT_POOL:
                break;
        case FBE_JOB_TYPE_DESTROY_EXTENT_POOL_LUN:
                break;
        default:
                trace_func(NULL, "FBE_JOB_TYPE_INVALID");
                break;
    }

    return status;
}


/*!*******************************************************************
 * @var fbe_job_service_drive_swap_field_info
 *********************************************************************
 * @brief Information about each of the fields of job service drive swap.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_job_service_drive_swap_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("command_type", fbe_spare_swap_command_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("job_number", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("spare_object_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("swap_edge_index", fbe_edge_index_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("status_code", fbe_job_service_error_type_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("internal_error", fbe_spare_internal_error_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("vd_object_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("mirror_swap_edge_index", fbe_edge_index_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_job_service_drive_swap_request_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        job service drive swap request.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_job_service_drive_swap_request_t,
                              fbe_job_service_drive_swap_struct_info,
                              &fbe_job_service_drive_swap_field_info[0]);

/*!**************************************************************
 * fbe_job_service_drive_swap_info_debug_trace()
 ****************************************************************
 * @brief
 *  Display drive swap Information.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - object_ptr Offset Value. * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  17- Jan-2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_job_service_drive_swap_info_debug_trace(const fbe_u8_t * module_name,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_dbgext_ptr object_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 16;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_job_service_drive_swap_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status == FBE_STATUS_OK)
    {
        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL, "transaction_id: %s ","NULL");
    }

    return status;
}

/*!*******************************************************************
 * @var fbe_job_service_raid_group_create_field_info
 *********************************************************************
 * @brief Information about each of the fields of job service raid group create
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_job_service_raid_group_create_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("raid_group_id", fbe_raid_group_number_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("raid_type", fbe_raid_group_type_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("drive_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("job_number", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("is_private", fbe_job_service_tri_state_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    //FBE_DEBUG_DECLARE_FIELD_INFO("explicit_removal", fbe_bool_t, FBE_FALSE,"0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("placement", fbe_block_transport_placement_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("b_bandwidth", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("power_saving_idle_time_in_seconds", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("max_raid_latency_time_in_seconds", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_job_service_raid_group_create_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        job service raid group create.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_job_service_raid_group_create_t,
                              fbe_job_service_raid_group_create_struct_info,
                              &fbe_job_service_raid_group_create_field_info[0]);

/*!**************************************************************
 * fbe_job_service_raid_group_create_debug_trace()
 ****************************************************************
 * @brief
 *  Display raid group create Information.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - object_ptr Offset Value. * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  17- Jan-2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_job_service_raid_group_create_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_dbgext_ptr object_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 16;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_job_service_raid_group_create_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status == FBE_STATUS_OK)
    {
        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"raid_group_object_id: %s ","NULL");

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"transaction_id: %s ","NULL");

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"status: %s ","NULL");
    }

    return status;
}

/*!*******************************************************************
 * @var fbe_job_service_raid_group_destroy_field_info
 *********************************************************************
 * @brief Information about each of the fields of job service raid group destroy
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_job_service_raid_group_destroy_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("raid_group_id", fbe_raid_group_number_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("raid_type", fbe_raid_group_type_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("number_of_virtual_drive_objects", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("number_of_mirror_under_striper_objects", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("job_number", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_job_service_raid_group_destroy_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        job service raid group destroy.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_job_service_raid_group_destroy_t,
                              fbe_job_service_raid_group_destroy_struct_info,
                              &fbe_job_service_raid_group_destroy_field_info[0]);

/*!**************************************************************
 * fbe_job_service_raid_group_destroy_debug_trace()
 ****************************************************************
 * @brief
 *  Display raid group destroy Information.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - object_ptr Offset Value. * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  17- Jan-2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_job_service_raid_group_destroy_debug_trace(const fbe_u8_t * module_name,
                                                            fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_dbgext_ptr object_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 16;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_job_service_raid_group_destroy_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status == FBE_STATUS_OK)
    {
        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"raid_group_object_id: %s","NULL");

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"status: %s","NULL");

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"transaction_id: %s","NULL");
    }

    return status;
}

/*!*******************************************************************
 * @var fbe_job_service_update_raid_group_field_info
 *********************************************************************
 * @brief Information about each of the fields of job service update raid group
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_job_service_update_raid_group_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("job_number", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("power_save_idle_time_in_sec", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("update_type", fbe_update_raid_type_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_job_service_update_raid_group_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        job service update raid group.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_job_service_update_raid_group_t,
                              fbe_job_service_update_raid_group_struct_info,
                              &fbe_job_service_update_raid_group_field_info[0]);

/*!**************************************************************
 * fbe_job_service_update_raid_group_debug_trace()
 ****************************************************************
 * @brief
 *  Display update raid group Information.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - object_ptr Offset Value. * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  17- Jan-2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_job_service_update_raid_group_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_dbgext_ptr object_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 16;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_job_service_update_raid_group_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status == FBE_STATUS_OK)
    {
        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"transaction_id: %s","NULL");
    }

    return status;
}


/*!*******************************************************************
 * @var fbe_job_service_lun_create_field_info
 *********************************************************************
 * @brief Information about each of the fields of job service lun create
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_job_service_lun_create_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("raid_type", fbe_raid_group_type_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(), 
    FBE_DEBUG_DECLARE_FIELD_INFO("raid_group_id", fbe_raid_group_number_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("lun_number", fbe_lun_number_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("job_number", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("addroffset", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("align_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("placement", fbe_block_transport_placement_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("bvd_edge_offset", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("lun_object_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("bvd_object_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("lun_exported_capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("lun_imported_capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("ndb_b", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("rg_block_offset", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("rg_client_index", fbe_edge_index_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("rg_stripe_size", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_job_service lun_create_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        job service lun create.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_job_service_lun_create_t,
                              fbe_job_service_lun_create_struct_info,
                              &fbe_job_service_lun_create_field_info[0]);

/*!**************************************************************
 * fbe_job_service_lun_create_debug_trace()
 ****************************************************************
 * @brief
 *  Display lun create Information.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - object_ptr Offset Value. * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  17- Jan-2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_job_service_lun_create_debug_trace(const fbe_u8_t * module_name,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_dbgext_ptr object_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 16;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_job_service_lun_create_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status == FBE_STATUS_OK)
    {
        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"fbe_status: %s","NULL");
    }

    return status;
}

/*!*******************************************************************
 * @var fbe_job_service_lun_destroy_field_info
 *********************************************************************
 * @brief Information about each of the fields of job service lun destroy.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_job_service_lun_destroy_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("lun_number", fbe_lun_number_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("lun_object_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("bvd_object_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("job_number", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_job_service_lun_destroy_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        job service lun destroy.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_job_service_lun_destroy_t,
                              fbe_job_service_lun_destroy_struct_info,
                              &fbe_job_service_lun_destroy_field_info[0]);

/*!**************************************************************
 * fbe_job_service_lun_destroy_debug_trace()
 ****************************************************************
 * @brief
 *  Display lun destroy Information.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - object_ptr Offset Value. * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  17- Jan-2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_job_service_lun_destroy_debug_trace(const fbe_u8_t * module_name,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     fbe_dbgext_ptr object_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 16;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_job_service_lun_destroy_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status == FBE_STATUS_OK)
    {
        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"transaction_id: %s","NULL");

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"fbe_status: %s","NULL");
    }

    return status;
}

/*!*******************************************************************
 * @var fbe_job_service_update_virtual_drive_field_info
 *********************************************************************
 * @brief Information about each of the fields of job service update virtual drive
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_job_service_update_virtual_drive_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("configuration_mode", fbe_virtual_drive_configuration_mode_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("update_type", fbe_update_vd_type_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("job_number", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_job_service_update_virtual_drive_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        job service update virtual drive.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_job_service_update_virtual_drive_t,
                              fbe_job_service_update_virtual_drive_struct_info,
                              &fbe_job_service_update_virtual_drive_field_info[0]);

/*!**************************************************************
 * fbe_job_service_update_virtual_drive_debug_trace()
 ****************************************************************
 * @brief
 *  Display update virtual drive Information.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - object_ptr Offset Value. * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  18- Jan-2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_job_service_update_virtual_drive_debug_trace(const fbe_u8_t * module_name,
                                                              fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_dbgext_ptr object_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 16;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_job_service_update_virtual_drive_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status == FBE_STATUS_OK)
    {
        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL, "transaction_id: %s","NULL");
    }

    return status;
}

/*!*******************************************************************
 * @var fbe_job_service_update_provision_drive_field_info
 *********************************************************************
 * @brief Information about each of the fields of job service update provision drive
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_job_service_update_provision_drive_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("config_type", fbe_provision_drive_config_type_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("update_type", fbe_update_pvd_type_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("job_number", fbe_u64_t, FBE_FALSE, "0x%x"), 
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("sniff_verify_state", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_job_service_update_provision_drive_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        job service update provision drive.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_job_service_update_provision_drive_t,
                              fbe_job_service_update_provision_drive_struct_info,
                              &fbe_job_service_update_provision_drive_field_info[0]);

/*!**************************************************************
 * fbe_job_service_update_provision_drive_debug_trace()
 ****************************************************************
 * @brief
 *  Display update provision drive Information.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - object_ptr Offset Value. * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  18- Jan-2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_job_service_update_provision_drive_debug_trace(const fbe_u8_t * module_name,
                                                                fbe_trace_func_t trace_func,
                                                                fbe_trace_context_t trace_context, 
                                                                fbe_dbgext_ptr object_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 16;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_job_service_update_provision_drive_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status == FBE_STATUS_OK)
    {
        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"transaction_id: %s","NULL");
    }

    return status;
}

/*!*******************************************************************
 * @var fbe_job_service_update_provision_drive_block_size_field_info
 *********************************************************************
 * @brief Information about each of the fields of job service update provision drive
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_job_service_update_pvd_block_size_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_size", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("job_number", fbe_u64_t, FBE_FALSE, "0x%x"), 
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_job_service_update_provision_drive_block_size_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        job service update provision drive.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_job_service_update_pvd_block_size_t,
                              fbe_job_service_update_pvd_block_size_struct_info,
                              &fbe_job_service_update_pvd_block_size_field_info[0]);


/*!**************************************************************
 * fbe_job_service_update_provision_drive_block_size_debug_trace()
 ****************************************************************
 * @brief
 *  Display update provision drive block_size Information.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - object_ptr Offset Value. * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  06/11/2014 - Created. Wayne Garrett
 *
 ****************************************************************/
fbe_status_t fbe_job_service_update_provision_drive_block_size_debug_trace(const fbe_u8_t * module_name,
                                                                fbe_trace_func_t trace_func,
                                                                fbe_trace_context_t trace_context, 
                                                                fbe_dbgext_ptr object_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 16;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_job_service_update_pvd_block_size_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status == FBE_STATUS_OK)
    {
        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"transaction_id: %s","NULL");
    }

    return status;
}

/*!*******************************************************************
 * @var fbe_job_service_update_spare_config_field_info
 *********************************************************************
 * @brief Information about each of the fields of job service update spare config
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_job_service_update_spare_config_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("update_type", fbe_database_update_spare_config_type_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("job_number", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_job_service_update_spare_config_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *       job service update spare config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_job_service_update_spare_config_t,
                              fbe_job_service_update_spare_config_struct_info,
                              &fbe_job_service_update_spare_config_field_info[0]);

/*!**************************************************************
 * fbe_job_service_update_spare_config_debug_trace()
 ****************************************************************
 * @brief
 *  Display update spare config Information.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - object_ptr Offset Value. *  
 * 
 * @return fbe_status_t
 *
 * @author
 *  18- Jan-2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_job_service_update_spare_config_debug_trace(const fbe_u8_t * module_name,
                                                             fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context, 
                                                             fbe_dbgext_ptr object_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t offset = 0;
    fbe_u32_t spaces_to_indent = 16;
    fbe_u64_t permanent_spare_trigger_time = 0;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_job_service_update_spare_config_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status == FBE_STATUS_OK) 
    {
        FBE_GET_FIELD_OFFSET(module_name,fbe_job_service_update_spare_config_t,"system_spare_info",&offset);
        FBE_READ_MEMORY(object_ptr + offset, &permanent_spare_trigger_time, sizeof(fbe_u64_t));

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL," permanent_spare_trigger_time: 0x%llx ",
           (unsigned long long)permanent_spare_trigger_time);

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"transaction_id: %s ","NULL");
    }

    return status;
}

/*!*******************************************************************
 * @var fbe_job_service_change_system_power_saving_field_info
 *********************************************************************
 * @brief Information about each of the fields of job service change system power saving
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_job_service_change_system_power_saving_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("job_number", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_job_service_change_system_power_saving_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        job service change system power saving.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_job_service_change_system_power_saving_info_t,
                              fbe_job_service_change_system_power_saving_struct_info,
                              &fbe_job_service_change_system_power_saving_field_info[0]);

/*!**************************************************************
 * fbe_job_service_change_system_power_saving_info_debug_trace()
 ****************************************************************
 * @brief
 *  Display change system power saving Information.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - object_ptr Offset Value. * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  18- Jan-2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_job_service_change_system_power_saving_info_debug_trace(const fbe_u8_t * module_name,
                                                                         fbe_trace_func_t trace_func,
                                                                         fbe_trace_context_t trace_context,
                                                                         fbe_dbgext_ptr object_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t offset = 0;
    fbe_u32_t spaces_to_indent = 16;
    fbe_u32_t hibermation_offset = 0;
    fbe_u64_t system_power_save_info = 0;
    fbe_u64_t hibernation_wake_up_time_in_minutes = 0;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_job_service_change_system_power_saving_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status == FBE_STATUS_OK) 
    {
        FBE_GET_FIELD_OFFSET(module_name,fbe_job_service_change_system_power_saving_info_t,"system_power_save_info",&offset);
        FBE_READ_MEMORY(object_ptr + offset, &system_power_save_info, sizeof(fbe_bool_t));

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"enabled: %llu ",
           (unsigned long long)system_power_save_info);

        FBE_GET_FIELD_OFFSET(module_name,fbe_system_power_saving_info_t,"hibernation_wake_up_time_in_minutes",&hibermation_offset);
        FBE_READ_MEMORY(object_ptr + offset + hibermation_offset, &hibernation_wake_up_time_in_minutes, sizeof(fbe_bool_t));

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"hibernation_wake_up_time_in_minutes: 0x%llx ",
           (unsigned long long)hibernation_wake_up_time_in_minutes);

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"transaction_id: %s ","NULL");
    }

    return status;
}

/*!**************************************************************
 * fbe_job_service_getJobTypeString()
 ****************************************************************
 * @brief
 *  this function take job type value as a argument and return job_type string value
 *  
 * @param job_type - enum job type value *
 *
 * @return char* 
 *
 * @author
 *  28- Dec-2010 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
char *fbe_job_service_getJobTypeString (fbe_job_type_t job_type)
{
    char jobTypeStringTemp[50] = {'\0'};
    char *jobTypeString = &jobTypeStringTemp[0];

    switch (job_type)
    {
        case FBE_JOB_TYPE_LAST:
            jobTypeString = "FBE_JOB_TYPE_LAST";
            break;
        case FBE_JOB_TYPE_DRIVE_SWAP:
            jobTypeString = "FBE_JOB_TYPE_DRIVE_SWAP";
            break;
        case FBE_JOB_TYPE_LUN_CREATE:
            jobTypeString = "FBE_JOB_TYPE_LUN_CREATE";
            break;
        case FBE_JOB_TYPE_LUN_DESTROY:
            jobTypeString = "FBE_JOB_TYPE_LUN_DESTROY";
            break;
        case FBE_JOB_TYPE_LUN_UPDATE:
            jobTypeString = "FBE_JOB_TYPE_LUN_UPDATE";
            break;
        case FBE_JOB_TYPE_RAID_GROUP_CREATE:
            jobTypeString = "FBE_JOB_TYPE_RAID_GROUP_CREATE";
            break;
        case FBE_JOB_TYPE_RAID_GROUP_DESTROY:
            jobTypeString = "FBE_JOB_TYPE_RAID_GROUP_DESTROY";
            break;
        case FBE_JOB_TYPE_UPDATE_RAID_GROUP:
            jobTypeString = "FBE_JOB_TYPE_UPDATE_RAID_GROUP";
            break;
        case FBE_JOB_TYPE_UPDATE_VIRTUAL_DRIVE:
            jobTypeString = "FBE_JOB_TYPE_UPDATE_VIRTUAL_DRIVE";
            break;
        case FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE:
            jobTypeString = "FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE";
            break;
        case FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE:
            jobTypeString = "FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE";
            break;
        case FBE_JOB_TYPE_UPDATE_SPARE_CONFIG:
            jobTypeString = "FBE_JOB_TYPE_UPDATE_SPARE_CONFIG";
            break;
         case FBE_JOB_TYPE_ADD_ELEMENT_TO_QUEUE_TEST:
            jobTypeString = "FBE_JOB_TYPE_ADD_ELEMENT_TO_QUEUE_TEST";
            break;
         case FBE_JOB_TYPE_CHANGE_SYSTEM_POWER_SAVING_INFO:
            jobTypeString = "FBE_JOB_TYPE_CHANGE_SYSTEM_POWER_SAVING_INFO";
            break;
        case FBE_JOB_TYPE_UPDATE_DB_ON_PEER:
            jobTypeString = "FBE_JOB_TYPE_UPDATE_DB_ON_PEER";
            break;
        case FBE_JOB_TYPE_UPDATE_JOB_ELEMENTS_ON_PEER:
            jobTypeString = "FBE_JOB_TYPE_UPDATE_JOB_ELEMENTS_ON_PEER";
            break;
        case FBE_JOB_TYPE_CHANGE_SYSTEM_ENCRYPTION_INFO:
           jobTypeString = "FBE_JOB_TYPE_CHANGE_SYSTEM_ENCRYPTION_INFO";
           break;
        case FBE_JOB_TYPE_SET_ENCRYPTION_PAUSED:
           jobTypeString = "FBE_JOB_TYPE_SET_ENCRYPTION_PAUSED";
           break;
        case FBE_JOB_TYPE_UPDATE_ENCRYPTION_MODE:
           jobTypeString = "FBE_JOB_TYPE_UPDATE_ENCRYPTION_MODE";
           break;
        case FBE_JOB_TYPE_PROCESS_ENCRYPTION_KEYS:
            jobTypeString  = "FBE_JOB_TYPE_PROCESS_ENCRYPTION_KEYS";
            break;
        case FBE_JOB_TYPE_SCRUB_OLD_USER_DATA:
            jobTypeString = "FBE_JOB_TYPE_SCRUB_OLD_USER_DATA";
            break;
        case FBE_JOB_TYPE_VALIDATE_DATABASE:
            jobTypeString = "FBE_JOB_TYPE_VALIDATE_DATABASE";
            break;
        case FBE_JOB_TYPE_CREATE_EXTENT_POOL:
           jobTypeString = "FBE_JOB_TYPE_CREATE_EXTENT_POOL";
           break;
        case FBE_JOB_TYPE_CREATE_EXTENT_POOL_LUN:
           jobTypeString = "FBE_JOB_TYPE_CREATE_EXTENT_POOL_LUN";
           break;
        case FBE_JOB_TYPE_DESTROY_EXTENT_POOL:
           jobTypeString = "FBE_JOB_TYPE_DESTROY_EXTENT_POOL";
           break;
        case FBE_JOB_TYPE_DESTROY_EXTENT_POOL_LUN:
           jobTypeString = "FBE_JOB_TYPE_DESTROY_EXTENT_POOL_LUN";
           break;
        default:
            jobTypeString = "UNKNOWN_JOB_TYPE"; 
            break;
    }

    return jobTypeString;
}

/*!**************************************************************
 * fbe_job_service_getJobActionStateString()
 ****************************************************************
 * @brief
 *  this function take job action state enum value as a argument
 *  and return job_action_state string value
 *  
 * @param job_action_state - enum job action state value *
 *
 * @return char* 
 *
 * @author
 *  28- Dec-2010 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
char *fbe_job_service_getJobActionStateString(fbe_job_action_state_t job_action_state)
{
    char jobActionStateTemp[50] = {'\0'};
    char *jobActionStateString = &jobActionStateTemp[0];

    switch (job_action_state)
    {
        case FBE_JOB_ACTION_STATE_INIT:
            jobActionStateString = "FBE_JOB_ACTION_STATE_INIT";
            break;
        case FBE_JOB_ACTION_STATE_IN_QUEUE:
            jobActionStateString = "FBE_JOB_ACTION_STATE_IN_QUEUE";
            break;
        case FBE_JOB_ACTION_STATE_DEQUEUED:
            jobActionStateString = "FBE_JOB_ACTION_STATE_DEQUEUED";
            break;
        case FBE_JOB_ACTION_STATE_VALIDATE:
            jobActionStateString = "FBE_JOB_ACTION_STATE_VALIDATE";
            break;
        case FBE_JOB_ACTION_STATE_SELECTION:
            jobActionStateString = "FBE_JOB_ACTION_STATE_SELECTION";
            break;
        case FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY:
            jobActionStateString = "FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY";
            break;
        case FBE_JOB_ACTION_STATE_PERSIST:
            jobActionStateString = "FBE_JOB_ACTION_STATE_PERSIST";
            break;
        case FBE_JOB_ACTION_STATE_ROLLBACK:
            jobActionStateString = "FBE_JOB_ACTION_STATE_ROLLBACK";
            break;
        case FBE_JOB_ACTION_STATE_COMMIT:
            jobActionStateString = "FBE_JOB_ACTION_STATE_COMMIT";
            break;
        case FBE_JOB_ACTION_STATE_DONE:
            jobActionStateString = "FBE_JOB_ACTION_STATE_DONE";
            break;
        case FBE_JOB_ACTION_STATE_EXIT:
            jobActionStateString = "FBE_JOB_ACTION_STATE_EXIT";
            break;
        case FBE_JOB_ACTION_STATE_INVALID:
            jobActionStateString = "FBE_JOB_ACTION_STATE_INVALID";
            break;
        default:
            jobActionStateString = "UNKNOWN_JOB_ACTION_STATE";
            break;
    }

    return jobActionStateString;
}

/*!**************************************************************
 * fbe_job_service_getThreadFlagStatusString()
 ****************************************************************
 * @brief
 *  this function take thread flag enum value as a argument
 *  and return thread_flag string value
 *  
 * @param thread_flag - enum thread flag value *
 *
 * @return char* 
 *
 * @author
 *  28- Dec-2010 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
char *fbe_job_service_getThreadFlagStatusString (fbe_job_service_thread_flags_t thread_flag)
{
    char threadFlagStringTemp[50] = {'\0'};
    char *threadFlagString = &threadFlagStringTemp[0];

    switch (thread_flag)
    {
        case FBE_JOB_SERVICE_THREAD_FLAGS_INVALID:
            threadFlagString = "FBE_JOB_SERVICE_THREAD_FLAGS_INVALID";
            break;
        case FBE_JOB_SERVICE_THREAD_FLAGS_RUN:
            threadFlagString = "FBE_JOB_SERVICE_THREAD_FLAGS_RUN";
            break;
        case FBE_JOB_SERVICE_THREAD_FLAGS_STOPPED:
            threadFlagString = "FBE_JOB_SERVICE_THREAD_FLAGS_STOPPED";
            break;
        case FBE_JOB_SERVICE_THREAD_FLAGS_DONE:
            threadFlagString = "FBE_JOB_SERVICE_THREAD_FLAGS_DONE";
            break;
        case FBE_JOB_SERVICE_THREAD_FLAGS_LAST:
            threadFlagString = "FBE_JOB_SERVICE_THREAD_FLAGS_LAST";
            break;
        default:
            threadFlagString = "UNKNOWN_THREAD_FLAG_TYPE"; 
            break;
    }

    return threadFlagString;
}

/*!**************************************************************
 * fbe_job_service_change_system_power_saving_info_debug_trace()
 ****************************************************************
 * @brief
 *  Display change system power saving Information.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - object_ptr Offset Value. * 
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_job_service_update_db_on_peer_debug_trace(const fbe_u8_t * module_name,
                                                                         fbe_trace_func_t trace_func,
                                                                         fbe_trace_context_t trace_context,
                                                                         fbe_dbgext_ptr object_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    #if 0
    fbe_u32_t offset = 0;
    fbe_u32_t spaces_to_indent = 16;
    fbe_u32_t hibermation_offset = 0;
    fbe_u64_t system_power_save_info = 0;
    fbe_u64_t hibernation_wake_up_time_in_minutes = 0;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_job_service_change_system_power_saving_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status == FBE_STATUS_OK) 
    {
        FBE_GET_FIELD_OFFSET(module_name,fbe_job_service_change_system_power_saving_info_t,"system_power_save_info",&offset);
        FBE_READ_MEMORY(object_ptr + offset, &system_power_save_info, sizeof(fbe_bool_t));

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"enabled: %u ",system_power_save_info);

        FBE_GET_FIELD_OFFSET(module_name,fbe_system_power_saving_info_t,"hibernation_wake_up_time_in_minutes",&hibermation_offset);
        FBE_READ_MEMORY(object_ptr + offset + hibermation_offset, &hibernation_wake_up_time_in_minutes, sizeof(fbe_bool_t));

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"hibernation_wake_up_time_in_minutes: 0x%llx ", hibernation_wake_up_time_in_minutes);

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"transaction_id: %s ","NULL");
    }
    #endif
    return status;
}

/*!*******************************************************************
 * @var fbe_job_service_change_system_encryption_field_info
 *********************************************************************
 * @brief Information about each of the fields of job service change system encryption
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_job_service_change_system_encryption_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("job_number", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_job_service_change_system_encryption_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        job service change system power saving.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_job_service_change_system_encryption_info_t,
                              fbe_job_service_change_system_encryption_struct_info,
                              &fbe_job_service_change_system_encryption_field_info[0]);

/*!**************************************************************
 * fbe_job_service_change_system_encryption_info_debug_trace()
 ****************************************************************
 * @brief
 *  Display change system encryption Information.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - object_ptr Offset Value. * 
 * 
 * @return fbe_status_t
 *
 * @author
 *
 ****************************************************************/
fbe_status_t fbe_job_service_change_system_encryption_info_debug_trace(const fbe_u8_t * module_name,
                                                                         fbe_trace_func_t trace_func,
                                                                         fbe_trace_context_t trace_context,
                                                                         fbe_dbgext_ptr object_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t offset = 0;
    fbe_u32_t spaces_to_indent = 16;
    fbe_u32_t encryption_offset = 0;
    fbe_u32_t system_encryption_info = 0;
    fbe_u32_t encryption_state = 0;


    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_job_service_change_system_encryption_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status == FBE_STATUS_OK) 
    {
        FBE_GET_FIELD_OFFSET(module_name,fbe_job_service_change_system_encryption_info_t,"system_encryption_info",&offset);
        FBE_READ_MEMORY(object_ptr + offset, &system_encryption_info, sizeof(fbe_u32_t));

        FBE_GET_FIELD_OFFSET(module_name,fbe_system_encryption_info_t,"encryption_state",&encryption_offset);
        FBE_READ_MEMORY(object_ptr + offset + encryption_offset, &encryption_state, sizeof(fbe_u32_t));

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"encryption_state: 0x%x ",
           encryption_state);

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"transaction_id: %s ","NULL");
    }

    return status;
}

/*!**************************************************************
 * fbe_job_service_change_system_encryption_info_debug_trace()
 ****************************************************************
 * @brief
 *  Display change system encryption Information.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - object_ptr Offset Value. * 
 * 
 * @return fbe_status_t
 *
 * @author
 *
 ****************************************************************/
fbe_status_t fbe_job_service_set_encryption_paused_debug_trace(const fbe_u8_t * module_name,
                                                                         fbe_trace_func_t trace_func,
                                                                         fbe_trace_context_t trace_context,
                                                                         fbe_dbgext_ptr object_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t offset = 0;
    fbe_u32_t spaces_to_indent = 16;
    fbe_u32_t encryption_offset = 0;
    fbe_u32_t system_encryption_info = 0;
    fbe_bool_t encryption_paused = 0;


    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_job_service_change_system_encryption_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status == FBE_STATUS_OK) 
    {
        FBE_GET_FIELD_OFFSET(module_name,fbe_job_service_change_system_encryption_info_t,"system_encryption_info",&offset);
        FBE_READ_MEMORY(object_ptr + offset, &system_encryption_info, sizeof(fbe_u32_t));

        FBE_GET_FIELD_OFFSET(module_name,fbe_system_encryption_info_t,"encryption_paused",&encryption_offset);
        FBE_READ_MEMORY(object_ptr + offset + encryption_offset, &encryption_paused, sizeof(fbe_bool_t));

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"encryption_paused: 0x%x ",
           encryption_paused);

        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL,"transaction_id: %s ","NULL");
    }

    return status;
}
/*!*******************************************************************
 * @var fbe_job_service_validate_database_field_info
 *********************************************************************
 * @brief Information about each of the fields of job service validate
 *        database request.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_job_service_validate_database_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("transaction_id", fbe_u64_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("job_number", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("validate_caller", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("failure_action", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("error_code", fbe_job_service_error_type_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_job_service_validate_database_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        job service validate database request.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_job_service_validate_database_t,
                              fbe_job_service_validate_database_struct_info,
                              &fbe_job_service_validate_database_field_info[0]);

/*!**************************************************************
 * fbe_job_service_update_virtual_drive_debug_trace()
 ****************************************************************
 * @brief
 *  Display validate database information.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - object_ptr Offset Value. * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  07/01/2014  Created
 *
 ****************************************************************/
fbe_status_t fbe_job_service_validate_database_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_dbgext_ptr object_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 16;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_job_service_validate_database_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    if(status == FBE_STATUS_OK)
    {
        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(NULL, "transaction_id: %s","NULL");
    }

    return status;
}

/*************************
 * end file fbe_job_service_debug.c
 *************************/
