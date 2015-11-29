/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_virtual_drive_swap.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the virtual_drive object sparing functionality.
 * 
 *  This includes the
 *  @ref sparing related utility function to carry out the permanent sparing,
 *  hot sparing operation.
 * 
 *  * @ingroup virtual_drive_class_files
 * 
 * @version
 *   11/12/2009:  Created. Dhaval Patel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe_spare.h"
#include "fbe_transport_memory.h"
#include "fbe_virtual_drive_private.h"
#include "fbe_virtual_drive_event_logging.h"
#include "fbe_raid_group_needs_rebuild.h"
#include "fbe_raid_group_rebuild.h"
#include "fbe_job_service.h"
#include "fbe_raid_library.h"
#include "fbe_cmi.h"
#include "fbe_notification.h"
#include "fbe/fbe_event_log_utils.h"                    //  for message codes

/* Job service request completion. */
static fbe_status_t fbe_virtual_drive_swap_send_request_to_job_service_completion(fbe_packet_t * packet_p,
                                                                                  fbe_packet_completion_context_t context);

fbe_status_t fbe_virtual_drive_swap_send_request_to_job_service(fbe_virtual_drive_t * virtual_drive_p,
                                                                fbe_packet_t * packet_p,
                                                                fbe_payload_control_operation_opcode_t control_code,
                                                                fbe_payload_control_buffer_t buffer,
                                                                fbe_payload_control_buffer_length_t buffer_length);

static fbe_status_t fbe_virtual_drive_swap_ask_user_copy_permission_completion(fbe_event_t * event_p,
                                                                               fbe_event_completion_context_t context);

static fbe_status_t fbe_virtual_drive_swap_ask_copy_abort_go_degraded_permission_completion(fbe_event_t *event_p,
                                                            fbe_event_completion_context_t context);

/*!****************************************************************************
 * fbe_virtual_drive_swap_get_need_replacement_drive_start_time()
 ******************************************************************************
 * @brief
 * This function is used to get the time in milliseconds that the replacement
 * drive was first request.
 *
 * @param virtual_drive_p                   - virtual drive object.
 * @param need_replacement_drive_start_time_p - Pointer to start time to update
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/10/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_get_need_replacement_drive_start_time(fbe_virtual_drive_t  *virtual_drive_p,
                                                             fbe_time_t *need_replacement_drive_start_time_p)
{
    /* If the `need replacement drive' flag is not set it is an error.
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE))
    {
        /* Trace the error and fail the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: get need replacement flag: 0x%x not set. \n",
                              FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return the `need replacement drive' start time.
     */
    *need_replacement_drive_start_time_p = virtual_drive_p->need_replacement_drive_start_time;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_get_need_replacement_drive_start_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_set_need_replacement_drive_start_time()
 ******************************************************************************
 * @brief
 * This function is used to set the need replacement start time (and associated
 * flag).
 *
 * @param virtual_drive_p                   - virtual drive object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/10/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_set_need_replacement_drive_start_time(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_time_t              need_replacement_drive_start_time;
    fbe_lifecycle_state_t   my_state = FBE_LIFECYCLE_STATE_INVALID;

    /* Get the lifecycle state.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);

    /*! @note If the `need replacement drive' flag is set it may indicate 
     *        something is wrong so we generate an error trace but the 
     *        request is allowed to proceed.
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE))
    {
        /* Trace the error.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: set need replacement life: %d flag: 0x%x is already set.\n",
                              my_state, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE);
    }

    /* Initialize the `need replacement drive' start time and set the flag.
     */
    need_replacement_drive_start_time = fbe_get_time();
    virtual_drive_p->need_replacement_drive_start_time = need_replacement_drive_start_time;
    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE);

    /* Log some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: set need replacement life: %d start time to: 0x%llx\n",
                          my_state, (unsigned long long)need_replacement_drive_start_time);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_set_need_replacement_drive_start_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_init_need_replacement_drive_start_time()
 ******************************************************************************
 * @brief
 * This function is used to initialize the `need replacement start time' and flag.
 *
 * @param virtual_drive_p                   - virtual drive object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/18/2012  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_init_need_replacement_drive_start_time(fbe_virtual_drive_t *virtual_drive_p)
{
    virtual_drive_p->need_replacement_drive_start_time = 0;
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_init_need_replacement_drive_start_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_clear_need_replacement_drive_start_time()
 ******************************************************************************
 * @brief
 * This function is used to clear the need replacement start time (and associated
 * flag).
 *
 * @param virtual_drive_p                   - virtual drive object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/18/2012  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_clear_need_replacement_drive_start_time(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_time_t                  current_time;
    fbe_virtual_drive_flags_t   flags;

    /* Get the flags.
     */
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);

    /*! @note If the `need replacement drive' flag is not set it may indicate 
     *        something is wrong so we generate an error trace but the 
     *        request is allowed to proceed.
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE))
    {
        /* Trace the error.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: clear need replacement flags: 0x%x flag: 0x%x not set. \n",
                              flags, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE);
    }

    /* Log some information.
     */
    current_time = fbe_get_time();
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: clear need replacement flags: 0x%x - started: 0x%llx current: 0x%llx\n",
                          flags, virtual_drive_p->need_replacement_drive_start_time, current_time);

    /* Clear the following flags
     *  o No Spare Reported
     *  o Spare Request Denied
     *  o Abort copy request denied
     */
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SET_NO_SPARE_REPORTED);
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SPARE_REQUEST_DENIED);
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_BROKEN_REPORTED);
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DENIED_REPORTED);
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DEGRADED_REPORTED);

    /* Else invoke the method to initialize the need spare timer
     */
    status = fbe_virtual_drive_swap_init_need_replacement_drive_start_time(virtual_drive_p);
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_clear_need_replacement_drive_start_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_reset_need_replacement_drive_start_time()
 ******************************************************************************
 * @brief
 * This function is used to reset the need replacement start time.
 *
 * @param virtual_drive_p                   - virtual drive object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/18/2012  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_reset_need_replacement_drive_start_time(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_time_t  need_replacement_drive_start_time;

    /* If the `need replacement drive' flag is not set it is an error.
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE))
    {
        /* Trace the error and fail the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: reset need replacement flag: 0x%x not set. \n",
                              FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize the `need replacement drive' start time.
     */
    need_replacement_drive_start_time = fbe_get_time();
    virtual_drive_p->need_replacement_drive_start_time = need_replacement_drive_start_time;

    /* Log some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: reset need replacement start time to: 0x%llx\n",
                          (unsigned long long)need_replacement_drive_start_time);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_reset_need_replacement_drive_start_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_init_drive_fault_start_time()
 ******************************************************************************
 * @brief
 * This function is used to initialize the `need replacement start time' and flag.
 *
 * @param virtual_drive_p                   - virtual drive object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/17/2012  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_init_drive_fault_start_time(fbe_virtual_drive_t *virtual_drive_p)
{
    virtual_drive_p->drive_fault_start_time = 0;
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_DRIVE_FAULT_DETECTED);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_init_drive_fault_start_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_set_drive_fault_start_time()
 ******************************************************************************
 * @brief
 * This function is used to set the drive fault start time.
 *
 * @param virtual_drive_p                   - virtual drive object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/17/2012  Ron Proulx  - Created
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_swap_set_drive_fault_start_time(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_time_t  drive_fault_start_time;

    /*! @note If the `need replacement drive' flag is set it may indicate 
     *        something is wrong so we generate an error trace but the 
     *        request is allowed to proceed.
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_DRIVE_FAULT_DETECTED))
    {
        /* Trace the error.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: set drive fault flag: 0x%x is already set.\n",
                              FBE_VIRTUAL_DRIVE_FLAG_DRIVE_FAULT_DETECTED);
    }

    /* Initialize the `need replacement drive' start time and set the flag.
     */
    drive_fault_start_time = fbe_get_time();
    virtual_drive_p->drive_fault_start_time = drive_fault_start_time;
    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_DRIVE_FAULT_DETECTED);

    /* Log some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: set drive fault start time to: 0x%llx\n",
                          (unsigned long long)drive_fault_start_time);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_set_drive_fault_start_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_clear_drive_fault_start_time()
 ******************************************************************************
 *
 * @brief   This function is used to clear the drive fault start time.
 *
 *
 * @param virtual_drive_p                   - virtual drive object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/17/2012  Ron Proulx  - Created.
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_swap_clear_drive_fault_start_time(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_time_t                  current_time;
    fbe_virtual_drive_flags_t   flags;

    /*! @note We do not validate that the FBE_VIRTUAL_DRIVE_FLAG_DRIVE_FAULT_DETECTED
     *        flag is set.
     */
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_DRIVE_FAULT_DETECTED))
    {
        /* Log some information.
         */
        current_time = fbe_get_time();
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: clear drive fault start time flags: 0x%x - started: 0x%llx current: 0x%llx\n",
                              flags, (unsigned long long)virtual_drive_p->drive_fault_start_time, (unsigned long long)current_time);
    
        /* Else invoke the method to initialize the drive fault start time
         */
        status = fbe_virtual_drive_swap_init_drive_fault_start_time(virtual_drive_p);
    }

    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_clear_drive_fault_start_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_get_drive_fault_start_time()
 ****************************************************************************** 
 * 
 * @brief   This function is used to get the time in milliseconds that the drive
 *          fault was detected.
 *
 * @param   virtual_drive_p         - virtual drive object.
 * @param   drive_fault_start_time_p - Pointer to start time to update
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/17/2012  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_swap_get_drive_fault_start_time(fbe_virtual_drive_t  *virtual_drive_p,
                                                               fbe_time_t *need_replacement_drive_start_time_p)
{
    /* If the `need replacement drive' flag is not set it is an error.
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_DRIVE_FAULT_DETECTED))
    {
        /* Trace the error and fail the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: get drive fault flag: 0x%x not set. \n",
                              FBE_VIRTUAL_DRIVE_FLAG_DRIVE_FAULT_DETECTED);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return the `drive fault' start time.
     */
    *need_replacement_drive_start_time_p = virtual_drive_p->drive_fault_start_time;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_get_drive_fault_start_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_init_need_proactive_copy_start_time()
 ******************************************************************************
 * @brief
 * This function is used to initialize the `proactive copy' flag.
 *
 * @param virtual_drive_p                   - virtual drive object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/08/2013  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_init_need_proactive_copy_start_time(fbe_virtual_drive_t *virtual_drive_p)
{
    virtual_drive_p->need_proactive_copy_start_time = 0;
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_PROACTIVE_COPY);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_need_proactive_copy_start_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_set_need_proactive_copy_start_time()
 ******************************************************************************
 * @brief
 * This function is used to set the need proactive copy start time.
 *
 * @param virtual_drive_p                   - virtual drive object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/08/2013  Ron Proulx  - Created
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_set_need_proactive_copy_start_time(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_time_t  need_proactive_copy_start_time;

    /*! @note If the `need proactive copy' flag is set it may indicate 
     *        something is wrong so we generate an error trace but the 
     *        request is allowed to proceed.
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_PROACTIVE_COPY))
    {
        /* Trace the error.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: set need proactive copy flag: 0x%x is already set.\n",
                              FBE_VIRTUAL_DRIVE_FLAG_NEED_PROACTIVE_COPY);
    }

    /* Initialize the `need proactive copy' start time and set the flag.
     */
    need_proactive_copy_start_time = fbe_get_time();
    virtual_drive_p->need_proactive_copy_start_time = need_proactive_copy_start_time;
    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_PROACTIVE_COPY);

    /* Log some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: set need proactive copy start time to: 0x%llx\n",
                          (unsigned long long)need_proactive_copy_start_time);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_set_need_proactive_copy_start_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_clear_need_proactive_copy_start_time()
 ******************************************************************************
 *
 * @brief   This function is used to clear the need proactive copy start time.
 *
 *
 * @param virtual_drive_p - virtual drive object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/08/2013  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_clear_need_proactive_copy_start_time(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_time_t                  current_time;
    fbe_virtual_drive_flags_t   flags;

    /*! @note We do not validate that the FBE_VIRTUAL_DRIVE_FLAG_proactive_denied_DETECTED
     *        flag is set.
     */
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_PROACTIVE_COPY))
    {
        /* Log some information.
         */
        current_time = fbe_get_time();
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: clear need proactive copy start time flags: 0x%x - start: 0x%llx cur: 0x%llx\n",
                              flags, (unsigned long long)virtual_drive_p->need_proactive_copy_start_time, (unsigned long long)current_time);
    
        /* Else invoke the method to initialize the need proactive copy start time
         */
        status = fbe_virtual_drive_swap_init_need_proactive_copy_start_time(virtual_drive_p);
    }

    /* Clear any flags associated with the proactive copy success, failure cases.
     */
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SET_NO_SPARE_REPORTED);
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_PROACTIVE_REQUEST_DENIED);
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_BROKEN_REPORTED);
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DENIED_REPORTED);
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DEGRADED_REPORTED);
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_IN_PROGRESS_REPORTED);
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_SOURCE_DRIVE_DEGRADED);
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_clear_need_proactive_copy_start_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_get_need_proactive_copy_start_time()
 ****************************************************************************** 
 * 
 * @brief   This function is used to get the time in milliseconds that the need
 *          proactive copy was detected.
 *
 * @param   virtual_drive_p         - virtual drive object.
 * @param   need_proactive_copy_start_time_p - Pointer to start time to update
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/08/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_swap_get_need_proactive_copy_start_time(fbe_virtual_drive_t  *virtual_drive_p,
                                                               fbe_time_t *need_proactive_copy_start_time_p)
{
    /* If the `need proactive copy' flag is not set it is an error.
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_PROACTIVE_COPY))
    {
        /* Trace the error and fail the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: get need proactive copy flag: 0x%x not set. \n",
                              FBE_VIRTUAL_DRIVE_FLAG_NEED_PROACTIVE_COPY);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return the `need proactive copy' start time.
     */
    *need_proactive_copy_start_time_p = virtual_drive_p->need_proactive_copy_start_time;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_get_need_proactive_copy_start_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_reset_need_proactive_copy_start_time()
 ******************************************************************************
 * @brief
 * This function is used to reset the need proactive copy start time.
 *
 * @param virtual_drive_p                   - virtual drive object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/08/2013  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_reset_need_proactive_copy_start_time(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_time_t  need_proactive_copy_start_time;

    /* If the `need proactive copy' flag is not set it is an error.
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_NEED_PROACTIVE_COPY))
    {
        /* Trace the error and fail the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: reset need proactive copy flag: 0x%x not set. \n",
                              FBE_VIRTUAL_DRIVE_FLAG_NEED_PROACTIVE_COPY);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize the `need proactive copy' start time.
     */
    need_proactive_copy_start_time = fbe_get_time();
    virtual_drive_p->need_proactive_copy_start_time = need_proactive_copy_start_time;

    /* Log some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: reset need proactive copy start time to: 0x%llx\n",
                          (unsigned long long)need_proactive_copy_start_time);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_reset_need_proactive_copy_start_time()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_virtual_drive_swap_check_if_proactive_spare_needed()
 ******************************************************************************
 * @brief
 *  This function determines whether we need to swap-in proactive spare with 
 *  respect to the current state of the downstream edges.
 *   
 * @param virtual_drive_p                   - pointer to the object.
 * @param is_proactive_sparing_required_p   - Returns TRUE if proactive sparing
 *                                            is required else it returns FALSE.
 * @param proactive_spare_edge_to_swap_in_p - returns edge index where we can
 *                                            swap-in proactive spare.
 * @return  fbe_status_t           
 *
 * @author
 *   03/02/2010 - Created. Dhaval Patel. 
 *
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_swap_check_if_proactive_spare_needed(fbe_virtual_drive_t * virtual_drive_p,
                                                       fbe_bool_t * is_proactive_sparing_required_p,
                                                       fbe_edge_index_t * proactive_spare_edge_to_swap_in_p)
{
    fbe_path_state_t                            first_edge_path_state;
    fbe_path_state_t                            second_edge_path_state;
    fbe_path_attr_t                             first_edge_path_attr;
    fbe_path_attr_t                             second_edge_path_attr;
    fbe_lba_t                                   first_edge_checkpoint;
    fbe_lba_t                                   second_edge_checkpoint;
    fbe_virtual_drive_configuration_mode_t      configuration_mode;

    *is_proactive_sparing_required_p = FBE_FALSE;
    *proactive_spare_edge_to_swap_in_p = FBE_EDGE_INDEX_INVALID;

    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                            &first_edge_path_attr);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                            &second_edge_path_attr);

    /* get the rebuild checkpoint for both the edges. */
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *) virtual_drive_p, FIRST_EDGE_INDEX, &first_edge_checkpoint);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *) virtual_drive_p, SECOND_EDGE_INDEX, &second_edge_checkpoint);

    /* Get the configuration mode of the virtual drive object. */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Verify the different edge state and attribute based on VD object's current configuration mode. */
    if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) {
        if((first_edge_path_state == FBE_PATH_STATE_ENABLED) &&
           (first_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE) &&
           (first_edge_checkpoint == FBE_LBA_INVALID) &&
           (second_edge_path_state == FBE_PATH_STATE_INVALID))
        {
            /* Current state of the virtual drive object indicates that we need a proactive spare. */
            *is_proactive_sparing_required_p = FBE_TRUE;
            *proactive_spare_edge_to_swap_in_p = SECOND_EDGE_INDEX;
        }
    }
    else if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) {
        if((second_edge_path_state == FBE_PATH_STATE_ENABLED) &&
           (second_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE) &&
           (second_edge_checkpoint == FBE_LBA_INVALID) &&
           (first_edge_path_state == FBE_PATH_STATE_INVALID))
        {
            /* Current state of the virtual drive object indicates that we need a proactive spare. */
            *is_proactive_sparing_required_p = FBE_TRUE;
            *proactive_spare_edge_to_swap_in_p = FIRST_EDGE_INDEX;
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_check_if_proactive_spare_needed()
*******************************************************************************/

/*!****************************************************************************
 *  fbe_virtual_drive_swap_check_if_user_copy_is_allowed()
 ******************************************************************************
 * @brief
 *  This function determines whether we can swap-in proactive spare with 
 *  respect to the current state of the downstream edges.
 *   
 * @param virtual_drive_p                   - pointer to the object.
 * @param virtual_drive_swap_in_validation_p - Pointer to swap-in request
 * @return  fbe_status_t           
 *
 * @author
 *   03/02/2010 - Created. Dhaval Patel. 
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_check_if_user_copy_is_allowed(fbe_virtual_drive_t *virtual_drive_p,
                                                     fbe_virtual_drive_swap_in_validation_t *virtual_drive_swap_in_validation_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_path_state_t                        first_edge_path_state;
    fbe_path_state_t                        second_edge_path_state;
    fbe_path_attr_t                         first_edge_path_attr;
    fbe_path_attr_t                         second_edge_path_attr;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_lifecycle_state_t                   lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_lba_t                               vd_rebuild_checkpoint = 0;
    fbe_raid_position_bitmask_t             rl_bitmask = FBE_RAID_POSITION_BITMASK_ALL_SET;
    fbe_raid_position_bitmask_t             source_edge_bitmask = FBE_RAID_POSITION_BITMASK_ALL_SET;

    /* Initialize the default values.
     */
    virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_INVALID;
    
    /* First validate the virtual drive lifecycle state.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &lifecycle_state);
    if ((lifecycle_state != FBE_LIFECYCLE_STATE_READY)             &&
        (lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)         &&
        (lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_HIBERNATE)    )
    {
        /* Trace a warning and fail the request.
         */
        virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_VIRTUAL_DRIVE_BROKEN;
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "virtual_drive: copy allowed life: %d doesn't support command: %d swap status: %d\n",
                              lifecycle_state, virtual_drive_swap_in_validation_p->command_type,
                              virtual_drive_swap_in_validation_p->swap_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                            &first_edge_path_attr);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                            &second_edge_path_attr);

    /* Get the configuration mode of the virtual drive object. */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_raid_group_get_rb_logging_bitmask((fbe_raid_group_t *)virtual_drive_p, &rl_bitmask);

    /* Verify the different edge state and attribute based on VD object's current configuration mode. */
    if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) 
    {
        source_edge_bitmask = (1 << FIRST_EDGE_INDEX);
        fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, FIRST_EDGE_INDEX, &vd_rebuild_checkpoint);
        if(((first_edge_path_state == FBE_PATH_STATE_ENABLED) || (first_edge_path_state == FBE_PATH_STATE_SLUMBER)) &&
             (second_edge_path_state == FBE_PATH_STATE_INVALID))
        {
            /* Current state of the virtual drive object indicates that we can proceed with proactive sparing. */
            virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_OK;
        }
        else
        {
            /* Else one of the path state attributes is bad */
            virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_CONFIG_MODE_DOESNT_SUPPORT;
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: copy allowed mode: %d life: %d path state:[%d-%d] attr:[0x%x-0x%x] unexpected\n",
                              configuration_mode, lifecycle_state, first_edge_path_state, second_edge_path_state,
                              first_edge_path_attr, second_edge_path_attr);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) 
    {
        source_edge_bitmask = (1 << SECOND_EDGE_INDEX);
        fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, SECOND_EDGE_INDEX, &vd_rebuild_checkpoint);
        if(((second_edge_path_state == FBE_PATH_STATE_ENABLED) || (second_edge_path_state == FBE_PATH_STATE_SLUMBER)) &&
             (first_edge_path_state == FBE_PATH_STATE_INVALID))
        {
            /* Current state of the virtual drive object indicates that we can proceed with proactive sparing. */
            virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_OK;
        }
        else
        {
            /* Else one of the path state attributes is bad */
            virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_CONFIG_MODE_DOESNT_SUPPORT;
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: copy allowed mode: %d life: %d path state:[%d-%d] attr:[0x%x-0x%x] unexpected\n",
                              configuration_mode, lifecycle_state, first_edge_path_state, second_edge_path_state,
                              first_edge_path_attr, second_edge_path_attr);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        /* Else bad configuration mode */
        virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_CONFIG_MODE_DOESNT_SUPPORT;
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: copy allowed mode: %d life: %d path state:[%d-%d] attr:[0x%x-0x%x] failed\n",
                              configuration_mode, lifecycle_state, first_edge_path_state, second_edge_path_state,
                              first_edge_path_attr, second_edge_path_attr);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* First validate the virtual drive is not degraded.
     */
    if (((rl_bitmask & source_edge_bitmask) != 0)  ||
        (vd_rebuild_checkpoint != FBE_LBA_INVALID)    )
    {
        /* Trace a warning and fail the request.
         */
        virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_VIRTUAL_DRIVE_DEGRADED;
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "virtual_drive: copy allowed mode: %d rl_bitmask: 0x%x rebuild chkpt: 0x%llx swap status: %d degraded\n",
                              configuration_mode, rl_bitmask, (unsigned long long)vd_rebuild_checkpoint,
                              virtual_drive_swap_in_validation_p->swap_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return the status.
     */
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_check_if_user_copy_is_allowed()
*******************************************************************************/

/*!****************************************************************************
 *  fbe_virtual_drive_swap_is_proactive_spare_swapped_in()
 ******************************************************************************
 * @brief
 *  This function determines whether we have already swapped-in proactive spare.
 *   
 * @param virtual_drive_p                  - pointer to the object.
 * @param is_proactive_spare_swapped_in_p  - returns TRUE if proactive spare is
 *                                           swapped-in else it return FALSE.
 * @return  fbe_status_t           
 *
 * @author
 *   03/02/2010 - Created. Dhaval Patel. 
 *
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_swap_is_proactive_spare_swapped_in(fbe_virtual_drive_t * virtual_drive_p,
                                                     fbe_bool_t * is_proactive_spare_swapped_in_p)
{
    fbe_path_state_t                            first_edge_path_state;
    fbe_path_state_t                            second_edge_path_state;
    fbe_virtual_drive_configuration_mode_t      configuration_mode;

    *is_proactive_spare_swapped_in_p = FBE_FALSE;

    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);

    /* Get the configuration mode of the virtual drive object. */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Verify the different edge state and attribute based on VD object's current configuration mode. */
    if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) && 
       (second_edge_path_state != FBE_PATH_STATE_INVALID))
    {
            /* Current state of the virtual drive object indicates that proactive spare is swapped-in. */
            *is_proactive_spare_swapped_in_p = FBE_TRUE;
    }
    else if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) &&
            (first_edge_path_state != FBE_PATH_STATE_INVALID))
    {
            /* Current state of the virtual drive object indicates that proactive spare is swapped-in. */
            *is_proactive_spare_swapped_in_p = FBE_TRUE;
    }
    else if(fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p))
    {
        /* Mirror configuration mode indicates that proactive spare is swapped-in. */
        *is_proactive_spare_swapped_in_p = FBE_TRUE;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_is_proactive_spare_swapped_in()
*******************************************************************************/

/*!****************************************************************************
 *  fbe_virtual_drive_swap_check_if_permanent_spare_needed()
 ******************************************************************************
 * @brief
 *   This function determines whether permanent sparing is required or not.
 *   
 * @param virtual_drive_p                   - pointer to the object.
 * @param is_permanent_sparing_required_p   - pointer to output that gets set 
 *                                            to FBE_TRUE if the permanent 
 *                                            sparing is needed.
 *
 * @return  fbe_status_t           
 *
 * @author
 *   03/02/2010 - Created. Dhaval Patel. 
 *
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_swap_check_if_permanent_spare_needed(fbe_virtual_drive_t * virtual_drive_p,
                                                       fbe_bool_t *is_permanent_sparing_required_p)
{
    fbe_base_config_downstream_health_state_t   downstream_health_state;    

    *is_permanent_sparing_required_p = FBE_FALSE;    
    
    /* Look for all the edges with downstream objects and verify its current
     * downstream health.
     */
    downstream_health_state = fbe_virtual_drive_verify_downstream_health(virtual_drive_p);

    /* If downstream health is broken and we have not started permanent
     * sparing yet then return the permanent sparing required as TRUE.
     */
    if ((downstream_health_state == FBE_DOWNSTREAM_HEALTH_BROKEN)                                        ||
        ((downstream_health_state == FBE_DOWNSTREAM_HEALTH_DISABLED)                                 && 
         fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_ATTACH_EDGE_TIMEDOUT)    )   )
    {
        *is_permanent_sparing_required_p = FBE_TRUE;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_check_if_permanent_spare_needed()
*******************************************************************************/

/*!****************************************************************************
 *  fbe_virtual_drive_swap_validate_permanent_spare_request()
 ******************************************************************************
 * @brief
 *   This function validates a permanent sparing request.
 *   
 * @param virtual_drive_p                   - pointer to the object.
 * @param virtual_drive_swap_in_validation_p - Pointer to request to validate
 *
 * @return  fbe_status_t           
 *
 * @author
 *   03/02/2010 - Created. Dhaval Patel. 
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_validate_permanent_spare_request(fbe_virtual_drive_t *virtual_drive_p,
                                                  fbe_virtual_drive_swap_in_validation_t *virtual_drive_swap_in_validation_p)
{
    fbe_status_t                                status = FBE_STATUS_FAILED; /*! @note by default the request will fail. */
    fbe_base_config_downstream_health_state_t   downstream_health_state; 
    fbe_u32_t                                   width;  
    fbe_virtual_drive_configuration_mode_t      configuration_mode;
    fbe_bool_t                                  b_is_pass_thru_mode = FBE_TRUE;
    fbe_path_state_t                            first_edge_path_state;
    fbe_path_state_t                            second_edge_path_state;
    
    /* The default status is invalid.
     */
    virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_INVALID;

    /* Validate the command.
     */
    if (virtual_drive_swap_in_validation_p->command_type != FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND)
    {
        /* If the command type is not correct fail the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Unexpected command: %d\n",
                              __FUNCTION__, virtual_drive_swap_in_validation_p->command_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate the requiring a permanent spare.
     */
    fbe_base_config_get_width((fbe_base_config_t *)virtual_drive_p, &width);
    if (virtual_drive_swap_in_validation_p->swap_edge_index >= width)
    {
        /* If the edge index is not correct fail the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Unexpected edge index: %d\n",
                              __FUNCTION__, virtual_drive_swap_in_validation_p->swap_edge_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that we are in pass-thru mode.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    b_is_pass_thru_mode = fbe_virtual_drive_is_pass_thru_configuration_mode_set(virtual_drive_p);
    if (b_is_pass_thru_mode == FBE_FALSE)
    {
        /* If the virtual drive is configured as mirror we cannot swap-in a
         * permanent spare.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s virtual drive in mirror mode: %d\n",
                              __FUNCTION__, configuration_mode);
        virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_CONFIG_MODE_DOESNT_SUPPORT;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Since a permanent sparing request will not change the virtual drive
     * configuration mode, we must validate that the requested edge to permanently
     * spare matches the configuration mode.
     */
    if (((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) &&
         (virtual_drive_swap_in_validation_p->swap_edge_index != FIRST_EDGE_INDEX)     )  ||
        ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) &&
         (virtual_drive_swap_in_validation_p->swap_edge_index != SECOND_EDGE_INDEX)     )    )
    {
        /* Trace a warning (the edge might have changed)
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s config mode: %d doesn't agree with swap index: %d\n",
                              __FUNCTION__, configuration_mode, virtual_drive_swap_in_validation_p->swap_edge_index);
        virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_INTERNAL_ERROR;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the path state and attributes to validate the proper swap-in edge
     * and that a permanent spare is still required.
     */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);

    /* Look for all the edges with downstream objects and verify its current
     * downstream health.
     */
    downstream_health_state = fbe_virtual_drive_verify_downstream_health(virtual_drive_p);

    /* If downstream health is broken and we have not started permanent
     * sparing yet then return the permanent sparing required as TRUE.
     */
    if ((downstream_health_state == FBE_DOWNSTREAM_HEALTH_BROKEN)                                        ||
        ((FBE_DOWNSTREAM_HEALTH_DISABLED == downstream_health_state)                                 &&
         fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_ATTACH_EDGE_TIMEDOUT)    )   )
    {
        /* Validate that the edge requiring a permanent spare is the same as 
         * requested.  We should not be swapping into an edge that is not
         * in the `invalid state'.
         */
        status = FBE_STATUS_OK;

        /* Just validate first or second health.
         */
        if (virtual_drive_swap_in_validation_p->swap_edge_index == FIRST_EDGE_INDEX)
        {
            /* If the edge state is not `invalid' something is wrong.
             */
            switch(first_edge_path_state)
            {
                case FBE_PATH_STATE_BROKEN:
                case FBE_PATH_STATE_GONE:
                case FBE_PATH_STATE_INVALID:
                case FBE_PATH_STATE_DISABLED:                    
                    /* These are valid.
                     */
                    status = FBE_STATUS_OK;
                    break;

                case FBE_PATH_STATE_ENABLED:
                case FBE_PATH_STATE_SLUMBER:
                default:
                     /* These are unexpected.
                      */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: validate permanent spare request mode: %d health: %d index: %d path state:[%d-%d] unexpected\n",
                              configuration_mode, downstream_health_state, 
                              virtual_drive_swap_in_validation_p->swap_edge_index,
                              first_edge_path_state, second_edge_path_state);
                    virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_INTERNAL_ERROR;
                    return FBE_STATUS_FAILED;
            }
        }
        else
        {
            /* Else the edge state is not `invalid' something is wrong.
             */
            switch(second_edge_path_state)
            {
                case FBE_PATH_STATE_BROKEN:
                case FBE_PATH_STATE_GONE:
                case FBE_PATH_STATE_INVALID:
                case FBE_PATH_STATE_DISABLED:                    
                    /* These are valid.
                     */
                    status = FBE_STATUS_OK;
                    break;

                case FBE_PATH_STATE_ENABLED:
                case FBE_PATH_STATE_SLUMBER:
                default:
                     /* These are unexpected.
                      */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: validate permanent spare request mode: %d health: %d index: %d path state:[%d-%d] unexpected\n",
                              configuration_mode, downstream_health_state, 
                              virtual_drive_swap_in_validation_p->swap_edge_index,
                              first_edge_path_state, second_edge_path_state);
                    virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_INTERNAL_ERROR;
                    return FBE_STATUS_FAILED;
            }
        }
    }
    else
    {
        /* Trace some information (warning) and set the status to `not required'
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: validate permanent spare request mode: %d health: %d index: %d path state:[%d-%d] not required\n",
                              configuration_mode, downstream_health_state, 
                              virtual_drive_swap_in_validation_p->swap_edge_index,
                              first_edge_path_state, second_edge_path_state);
        virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_PERMANENT_SPARE_NOT_REQUIRED;
    }

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_validate_permanent_spare_request()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_virtual_drive_swap_validate_proactive_copy_request()
 ******************************************************************************
 * @brief
 *   This function validates a proactive copy request.
 *   
 * @param virtual_drive_p                   - pointer to the object.
 * @param virtual_drive_swap_in_validation_p - Pointer to request to validate
 *
 * @return  fbe_status_t           
 *
 * @author
 *  10/12/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_validate_proactive_copy_request(fbe_virtual_drive_t *virtual_drive_p,
                                                  fbe_virtual_drive_swap_in_validation_t *virtual_drive_swap_in_validation_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               width;
    fbe_path_state_t                        first_edge_path_state;
    fbe_path_state_t                        second_edge_path_state;
    fbe_path_attr_t                         first_edge_path_attr;
    fbe_path_attr_t                         second_edge_path_attr;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_lifecycle_state_t                   lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_lba_t                               vd_rebuild_checkpoint = 0;
    fbe_raid_position_bitmask_t             rl_bitmask = FBE_RAID_POSITION_BITMASK_ALL_SET;
    fbe_raid_position_bitmask_t             source_edge_bitmask = FBE_RAID_POSITION_BITMASK_ALL_SET;
    fbe_edge_index_t                        proactive_spare_edge_to_swap_in = FBE_EDGE_INDEX_INVALID;
    fbe_bool_t                              b_does_config_require_swap = FBE_FALSE;
    
    /* The default status is invalid.
     */
    virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_INVALID;

    /* Validate the command.
     */
    if (virtual_drive_swap_in_validation_p->command_type != FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND)
    {
        /* If the command type is not correct fail the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Unexpected command: %d\n",
                              __FUNCTION__, virtual_drive_swap_in_validation_p->command_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate the requiring a proactive spare.
     */
    fbe_base_config_get_width((fbe_base_config_t *)virtual_drive_p, &width);
    if (virtual_drive_swap_in_validation_p->swap_edge_index >= width)
    {
        /* If the edge index is not correct fail the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Unexpected edge index: %d\n",
                              __FUNCTION__, virtual_drive_swap_in_validation_p->swap_edge_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* First validate the virtual drive lifecycle state.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &lifecycle_state);
    if ((lifecycle_state != FBE_LIFECYCLE_STATE_READY)             &&
        (lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)         &&
        (lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_HIBERNATE)    )
    {
        /* Trace a warning and fail the request.
         */
        virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_VIRTUAL_DRIVE_BROKEN;
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "virtual_drive: validate proactive copy life: %d doesn't support command: %d swap status: %d\n",
                              lifecycle_state, virtual_drive_swap_in_validation_p->command_type,
                              virtual_drive_swap_in_validation_p->swap_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                            &first_edge_path_attr);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                            &second_edge_path_attr);

    /* Get the configuration mode of the virtual drive object. */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_raid_group_get_rb_logging_bitmask((fbe_raid_group_t *)virtual_drive_p, &rl_bitmask);

    /* Verify the different edge state and attribute based on VD object's current configuration mode. */
    if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) 
    {
        source_edge_bitmask = (1 << FIRST_EDGE_INDEX);
        fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, FIRST_EDGE_INDEX, &vd_rebuild_checkpoint);
        if(((first_edge_path_state == FBE_PATH_STATE_ENABLED) || (first_edge_path_state == FBE_PATH_STATE_SLUMBER)) &&
             (second_edge_path_state == FBE_PATH_STATE_INVALID))
        {
            /* Current state of the virtual drive object indicates that we can proceed with proactive sparing. */
            virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_OK;
        }
        else
        {
            /* Else one of the path state attributes is bad */
            virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_CONFIG_MODE_DOESNT_SUPPORT;
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: validate proactive copy mode: %d life: %d path state:[%d-%d] attr:[0x%x-0x%x] unexpected\n",
                              configuration_mode, lifecycle_state, first_edge_path_state, second_edge_path_state,
                              first_edge_path_attr, second_edge_path_attr);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) 
    {
        source_edge_bitmask = (1 << SECOND_EDGE_INDEX);
        fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, SECOND_EDGE_INDEX, &vd_rebuild_checkpoint);
        if(((second_edge_path_state == FBE_PATH_STATE_ENABLED) || (first_edge_path_state == FBE_PATH_STATE_SLUMBER)) &&
             (first_edge_path_state == FBE_PATH_STATE_INVALID))
        {
            /* Current state of the virtual drive object indicates that we can proceed with proactive sparing. */
            virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_OK;
        }
        else
        {
            /* Else one of the path state attributes is bad */
            virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_CONFIG_MODE_DOESNT_SUPPORT;
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: validate proactive copy mode: %d life: %d path state:[%d-%d] attr:[0x%x-0x%x] unexpected\n",
                              configuration_mode, lifecycle_state, first_edge_path_state, second_edge_path_state,
                              first_edge_path_attr, second_edge_path_attr);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        /* Else bad configuration mode */
        virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_CONFIG_MODE_DOESNT_SUPPORT;
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: validate proactive copy mode: %d life: %d path state:[%d-%d] attr:[0x%x-0x%x] failed\n",
                              configuration_mode, lifecycle_state, first_edge_path_state, second_edge_path_state,
                              first_edge_path_attr, second_edge_path_attr);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* First validate the virtual drive is not degraded.
     */
    if (((rl_bitmask & source_edge_bitmask) != 0)  ||
        (vd_rebuild_checkpoint != FBE_LBA_INVALID)    )
    {
        /* Trace a warning and fail the request.
         */
        virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_VIRTUAL_DRIVE_DEGRADED;
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "virtual_drive: validate proactive copy mode: %d rl_bitmask: 0x%x rebuild chkpt: 0x%llx swap status: %d degraded\n",
                              configuration_mode, rl_bitmask, (unsigned long long)vd_rebuild_checkpoint,
                              virtual_drive_swap_in_validation_p->swap_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* verify whether we need to swap-in proactive spare. */
    fbe_virtual_drive_swap_check_if_proactive_spare_needed(virtual_drive_p,
                                                           &b_does_config_require_swap,
                                                           &proactive_spare_edge_to_swap_in);
    if (b_does_config_require_swap == FBE_FALSE)
    {
        /* Trace a warning (the edge might have changed)
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "virtual_drive: validate proactive copy mode: %d swap index: %d doesn't require proactive copy\n",
                              configuration_mode, virtual_drive_swap_in_validation_p->swap_edge_index);
        virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_PROACTIVE_SPARE_NOT_REQUIRED;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_validate_proactive_copy_request()
*******************************************************************************/

/*!****************************************************************************
 *  fbe_virtual_drive_can_we_initiate_replacement_request()
 ******************************************************************************
 * @brief
 *   This function determines if we can allow a replacement drive request to
 *   proceed.  The use cases are:
 *      o Normal case - Virtual drive is in pass-thru mode and has entered the
 *          fail state.  Wait for peer to transition to fail.
 *      o Mirror Mode Source Drive Failed - Virtual drive has transitioned to
 *          fail state.  Wait for peer to transition to fail.
 *      o Mirror Mode Destination Drive Failed - Virtual drive remains in Ready
 *          state.  No need to check the peer state.
 *   
 * @param virtual_drive_p   - pointer to the object.
 * @param b_can_initiate_p  - pointer to output that gets set 
 *                                            to FBE_TRUE if the permanent 
 *                                            sparing is needed.
 * @return  fbe_status_t           
 *
 * @author
 *   03/17/2011 - Created. Ashwin Tammilarasan. 
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_can_we_initiate_replacement_request(fbe_virtual_drive_t *virtual_drive_p,
                                                                   fbe_bool_t *b_can_initiate_p)
{
    fbe_bool_t              b_is_active = FBE_TRUE; /*! @note By default we can initiate the swap-out. */
    fbe_lifecycle_state_t   my_state = FBE_LIFECYCLE_STATE_INVALID;    
    fbe_lifecycle_state_t   peer_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;  
    fbe_path_state_t        first_edge_path_state;
    fbe_path_state_t        second_edge_path_state; 
    fbe_bool_t              b_is_mirror_mode = FBE_FALSE;
    fbe_bool_t              b_is_destination_failed = FBE_FALSE; 

    /* By default we allow the swap-out to proceed.
     */
    *b_can_initiate_p = FBE_TRUE;

    /* Determine our configuration mode.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t *)virtual_drive_p, &peer_state);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* determine if we are passive or active? */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)virtual_drive_p);
    if (!b_is_active)
    {
        /* If we are passive we cannot initiate.
         */
        *b_can_initiate_p = FBE_FALSE; 
        return FBE_STATUS_OK;                              
    } 
   
    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);

    /* Determine our configuration mode.
     */
    switch (configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            if (second_edge_path_state != FBE_PATH_STATE_ENABLED)
            {
                b_is_destination_failed = FBE_TRUE;
            }
            b_is_mirror_mode = FBE_TRUE;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            if (first_edge_path_state != FBE_PATH_STATE_ENABLED)
            {
                b_is_destination_failed = FBE_TRUE;
            }
            b_is_mirror_mode = FBE_TRUE;
            break;
        default:
            /* Nothing to do for non-mirror mode.
             */
            break;
    }

    /* If the peer is alive determine the configuration mode and if the source 
     * or destination failed.
     */
    if (fbe_cmi_is_peer_alive())
    {
        /* If we in pass-thru and the peer is still in the Ready state we need
         * to wait for the peer to goto fail.
         */
        if (b_is_mirror_mode == FBE_FALSE)
        {
            /* We are in pass-thru mode wait for the peer to goto fail.
             */
            if (peer_state == FBE_LIFECYCLE_STATE_READY)
            {
                /* Wait for peer to goto fail.
                 */
                *b_can_initiate_p = FBE_FALSE;
            }
        }
        else if (b_is_destination_failed == FBE_FALSE)
        {
            /* If the source drive failed wait for the peer to goto fail.
             */
            if (peer_state == FBE_LIFECYCLE_STATE_READY)
            {
                /* Wait for peer to goto fail.
                 */
                *b_can_initiate_p = FBE_FALSE;
            }
        }
        else
        {
            /* Else it means the destination failed. There is no need to
             * wait for the peer.
             */
        }
    }

    /* If we denied the the swap-out request trace some information.
     */
    if (*b_can_initiate_p == FBE_FALSE)            
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: can initiate replacement mode: %d life: %d peer: %d path state:[%d-%d] denied.\n",
                              configuration_mode, my_state, peer_state, first_edge_path_state, second_edge_path_state);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_can_we_initiate_replacement_request()
*******************************************************************************/

/*!****************************************************************************
 *  fbe_virtual_drive_check_and_send_notification()
 ******************************************************************************
 * @brief
 *   This function determines whether permanent sparing is required or not.
 *   
 * @param virtual_drive_p                   - pointer to the object.
 * @param is_permanent_sparing_required_p   - pointer to output that gets set 
 *                                            to FBE_TRUE if the permanent 
 *                                            sparing is needed.
 * @return  fbe_status_t           
 *
 * @author
 *   03/17/2011 - Created. Ashwin Tammilarasan. 
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_check_and_send_notification(fbe_virtual_drive_t *virtual_drive_p)                                              
{

    fbe_virtual_drive_configuration_mode_t         configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
    fbe_object_id_t                                spare_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                                orig_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                                vd_object_id = FBE_OBJECT_ID_INVALID;

    fbe_base_object_get_object_id((fbe_base_object_t*)virtual_drive_p, &vd_object_id);

    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
        
    if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) 
    {
        fbe_vitual_drive_get_server_object_id_for_edge(virtual_drive_p, FIRST_EDGE_INDEX, &spare_pvd_object_id);        
    } 
    else if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) 
    {
        fbe_vitual_drive_get_server_object_id_for_edge(virtual_drive_p, SECOND_EDGE_INDEX, &spare_pvd_object_id);
    }

    fbe_virtual_drive_get_orig_pvd_object_id(virtual_drive_p, &orig_pvd_object_id);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: check and send notification for vd obj: 0x%x orig: 0x%x spare: 0x%x \n",
                          vd_object_id, orig_pvd_object_id, spare_pvd_object_id);

    /* There are cases where the original drive is re-inserted.  In this case
     * do not send a swap notification.
     */
    if ((orig_pvd_object_id != FBE_OBJECT_ID_INVALID) && (orig_pvd_object_id != spare_pvd_object_id)) 
    {
        /* Set the swap command, send the notification then clear it.
         */
        fbe_virtual_drive_set_copy_request_type(virtual_drive_p, FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND);
        fbe_virtual_drive_send_swap_notification(virtual_drive_p, configuration_mode);
        fbe_virtual_drive_set_copy_request_type(virtual_drive_p, FBE_SPARE_SWAP_INVALID_COMMAND);
    }
    
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_check_and_send_notification()
*******************************************************************************/

/*!****************************************************************************
 *  fbe_virtual_drive_swap_check_if_permanent_sparing_can_start()
 ******************************************************************************
 * @brief
 *   This function determines whether permanent sparing can start currently 
 *   or not, it waits for the configured timer and make sure that both sp does
 *   not see the drive before it start permanent sparing.
 *   
 * @param virtual_drive_p                   - pointer to the object.
 * @param can_permanent_sparing_start_b_p   - pointer to output that gets set 
 *                                            to FBE_TRUE if the permanent 
 *                                            sparing can start.
 * @return  fbe_status_t           
 *
 * @author
 *   09/13/2010 - Created. Dhaval Patel. 
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_check_if_permanent_sparing_can_start(fbe_virtual_drive_t *virtual_drive_p,
                                                                         fbe_bool_t *can_permanent_sparing_start_b_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u64_t       permanent_spare_trigger_time;
    fbe_u64_t       elapsed_seconds;
    fbe_time_t      need_permanent_spare_start_time;
    fbe_time_t      drive_fault_start_time;

    /* Initialize permanent sparing can start as FALSE. 
     */
    *can_permanent_sparing_start_b_p = FBE_FALSE;

    /* Check downstream broken only if it attached edge.
     */
    if(!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_ATTACH_EDGE_TIMEDOUT))
    {
        /* If the drive is `broken' (i.e. it is not missing and is known to be
         * permanently broken) then do not wait for the `need replacement start time'.
         */
        if (fbe_virtual_drive_is_downstream_drive_broken(virtual_drive_p) == FBE_TRUE)
        {
            /*! @note It takes a short period for the parent raid group to detect 
             *        the broken edge.  Therefore the first time we set a flag so
             *        that we wait a short period before attempting the spare request.
             */
            if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_DRIVE_FAULT_DETECTED))
            {
                /* We have detected the drive and have waited a short period.  Request
                 * a permanent spare.  We clear the flag here.
                 */             
                fbe_virtual_drive_swap_get_drive_fault_start_time(virtual_drive_p, &drive_fault_start_time);
                elapsed_seconds = fbe_get_elapsed_seconds(drive_fault_start_time);
                if (elapsed_seconds >= 1)
                {
                    /* (1) Second has transpired.  Ok to request spare.
                     */
                    fbe_virtual_drive_swap_clear_drive_fault_start_time(virtual_drive_p);
                    *can_permanent_sparing_start_b_p = FBE_TRUE;
                } 
            }
            else
            {
                    /* Else set the flag indicating that we detected the drive fault
                    * and wait a short period before we request a spare.
                    */
                    fbe_virtual_drive_swap_set_drive_fault_start_time(virtual_drive_p);
            }

            /* Log a informational message and indicate the we should replace the
             * drive immediately.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: can permanent sparing start: %d drive is broken. \n",
                              *can_permanent_sparing_start_b_p);
            return FBE_STATUS_OK;        

        } /* end if drive fault detected */

    } /* end if edge timeout is set */

    /* Get the `need replacement drive' start time.
     */
    fbe_virtual_drive_swap_clear_drive_fault_start_time(virtual_drive_p);
    status = fbe_virtual_drive_swap_get_need_replacement_drive_start_time(virtual_drive_p,
                                                                          &need_permanent_spare_start_time);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Get need replacement start failed. status: 0x%x \n",
                              __FUNCTION__, status);
        return status;
    }

    /* get the permanent sparing trigger time and compare with elapsed seconds. */
    fbe_virtual_drive_get_permanent_spare_trigger_time(&permanent_spare_trigger_time);

    /* get the elapsed time betweeen this check and previous check. */
    elapsed_seconds = fbe_get_elapsed_seconds(need_permanent_spare_start_time);

    /* if elapsed time is less than permanent spare trigger time then wait until elapsed
     * time becomes greater than permanent spare trigger time.
     */
    if (elapsed_seconds < permanent_spare_trigger_time)
    {
        return FBE_STATUS_OK;
    }

    /*! @note Cannot fail `can start' request due to `no spare reported'
     *        since must allow the spare request to proceed.
     */

    /* set the permanent sparing can start as true if all the condition met. */
    *can_permanent_sparing_start_b_p = FBE_TRUE;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_check_if_permanent_sparing_can_start()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_virtual_drive_swap_check_if_permanent_sparing_can_start()
 ******************************************************************************
 * @brief
 *   This function determines whether permanent sparing can start currently 
 *   or not, it waits for the configured timer and make sure that both sp does
 *   not see the drive before it start permanent sparing.
 *   
 * @param   virtual_drive_p - pointer to the object.
 * @param   b_can_swap_out_start_p - Pointer to bool to update:
 *              FBE_TRUE - Sufficient time has transpired and it is time to
 *                  swap-out the failed drive.
 *              FBE_FALSE - We have not waited long enough to give up on
 *                  the copy operation. Therefore stay in mirror mode.
 *
 * @return  fbe_status_t           
 *
 * @author
 *   09/13/2010 - Created. Dhaval Patel. 
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_check_if_swap_out_can_start(fbe_virtual_drive_t *virtual_drive_p,
                                                                fbe_bool_t *b_can_swap_out_start_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u64_t       permanent_spare_trigger_time;
    fbe_u64_t       elapsed_seconds;
    fbe_time_t      need_swap_out_start_time;
    fbe_time_t      drive_fault_start_time;

    /* initialize ok to swap-out to FALSE. */
    *b_can_swap_out_start_p = FBE_FALSE;

    /* Check downstream broken only if it attached edge.
     */
    if(!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_ATTACH_EDGE_TIMEDOUT))
    {
        /* If the drive is `broken' (i.e. it is not missing and is known to be
         * permanently broken) then do not wait for the `need replacement start time'.
         */
        if (fbe_virtual_drive_is_downstream_drive_broken(virtual_drive_p) == FBE_TRUE)
        {
            /*! @note It takes a short period for the parent raid group to detect 
             *        the broken edge.  Therefore the first time we set a flag so
             *        that we wait a short period before attempting the spare request.
             */
            if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_DRIVE_FAULT_DETECTED))
            {
                /* We have detected the drive and have waited a short period.  Request
                 * a permanent spare.  We clear the flag here.
                 */
                fbe_virtual_drive_swap_get_drive_fault_start_time(virtual_drive_p, &drive_fault_start_time);
                elapsed_seconds = fbe_get_elapsed_seconds(drive_fault_start_time);
                if (elapsed_seconds >= 1)
                {
                    /* (1) Second has transpired.  Ok to request spare.
                     */
                    fbe_virtual_drive_swap_clear_drive_fault_start_time(virtual_drive_p);
                    *b_can_swap_out_start_p = FBE_TRUE;
                }
            }
            else
            {
                /* Else set the flag indicating that we detected the drive fault
                 * and wait a short period before we request a spare.
                 */
                fbe_virtual_drive_swap_set_drive_fault_start_time(virtual_drive_p);
            }

            /* Log a informational message and indicate the we should replace the
             * drive immediately.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: can swap out start: %d drive is broken. \n",
                                  *b_can_swap_out_start_p);
            return FBE_STATUS_OK;        
        }
    }

    /* Get the `need replacement drive' start time.
     */
    fbe_virtual_drive_swap_clear_drive_fault_start_time(virtual_drive_p);
    status = fbe_virtual_drive_swap_get_need_replacement_drive_start_time(virtual_drive_p,
                                                                          &need_swap_out_start_time);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Get need replacement start failed. status: 0x%x \n",
                              __FUNCTION__, status);
        return status;
    }

    /* get the permanent sparing trigger time and compare with elapsed seconds. */
    fbe_virtual_drive_get_permanent_spare_trigger_time(&permanent_spare_trigger_time);

    /* get the elapsed time betweeen this check and previous check. */
    elapsed_seconds = fbe_get_elapsed_seconds(need_swap_out_start_time);

    /* if elapsed time is less than permanent spare trigger time then wait until elapsed
     * time becomes greater than permanent spare trigger time.
     */
    if (elapsed_seconds < permanent_spare_trigger_time)
    {
        return FBE_STATUS_OK;
    }

    /*! @note Cannot fail `can start' request due to `no spare reported'
     *        since must allow the spare request to proceed.
     */

    /* set the permanent sparing can start as true if all the condition met. */
    *b_can_swap_out_start_p = FBE_TRUE;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_check_if_swap_out_can_start()
*******************************************************************************/

/*!****************************************************************************
 *  fbe_virtual_drive_swap_create_job_service_drive_swap_request()
 ******************************************************************************
 * @brief
 *   Create swap request to send it to job service.
 *   
 * @param drive_swap_request_p - Pointer to job service swap request to populate
 * @param vd_object_id - Virtual drive object id
 * @param swap_edge_index - Virtual drive edge index that is be swapped/copy from 
 * @param mirror_swap_edge_index - The other edge index (new edge index for copy)
 * @param original_pvd_object_id - The pvd object id of the originla(source) drive
 * @param swap_command_type  - swap command type.
 * 
 * @return  fbe_status_t     - status of the operation.
 *
 * @author
 *   03/02/2010 - Created. Dhaval Patel. 
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_swap_create_job_service_drive_swap_request(fbe_job_service_drive_swap_request_t *drive_swap_request_p,
                                                             fbe_object_id_t vd_object_id,
                                                             fbe_edge_index_t swap_edge_index,
                                                             fbe_edge_index_t mirror_swap_edge_index,
                                                             fbe_object_id_t original_pvd_object_id,
                                                             fbe_spare_swap_command_t swap_command_type)
{
    /* verify the drive swap request. */
    if (drive_swap_request_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_job_service_drive_swap_request_set_swap_command_type(drive_swap_request_p, swap_command_type);
    fbe_job_service_drive_swap_request_set_swap_edge_index(drive_swap_request_p, swap_edge_index);
    fbe_job_service_drive_swap_request_set_mirror_swap_edge_index(drive_swap_request_p, mirror_swap_edge_index);
    fbe_job_service_drive_swap_request_set_virtual_drive_object_id(drive_swap_request_p, vd_object_id);
    fbe_job_service_drive_swap_request_set_original_pvd_object_id(drive_swap_request_p, original_pvd_object_id);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_create_swap_request()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_initiate_swap_operation()
 ******************************************************************************
 * @brief
 *  This function is used to create and send the swap request to job service.
 *
 * @param virtual_drive_p       - virtual dirve object.
 * @param packet_p              - job service packet pointer.
 * @param swap_edge_index       - specific edge index to apply the swap to.
 * @param mirror_swap_edge_index - The alternate edge index.
 * @param spare_swap_command    - spare swap command.
 *
 * @return  status - Typically FBE_STATUS_MORE_PROCESSING_REQUIRED.
 *
 * @author
 *  11/03/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_initiate_swap_operation(fbe_virtual_drive_t * virtual_drive_p,
                                                       fbe_packet_t * packet_p,
                                                       fbe_edge_index_t swap_edge_index,
                                                       fbe_edge_index_t mirror_swap_edge_index,
                                                       fbe_spare_swap_command_t swap_command_type)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_service_drive_swap_request_t    drive_swap_request;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                              is_active_sp;
    fbe_object_id_t                         original_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_virtual_drive_flags_t               flags = 0;

    /* determine if we are passive or active? */
    is_active_sp = fbe_base_config_is_active((fbe_base_config_t *) virtual_drive_p);
    if(!is_active_sp)
    {
        return FBE_STATUS_OK;                              
    } 

    /* If there is already a swap request in progress something is wrong.
     */
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS))
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_TRANSACTION_FAILED,
                              "%s vd obj: 0x%x cmd: %d flags: 0x%x swap in progress\n",
                              __FUNCTION__, vd_object_id, swap_command_type, flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize the swap request with default values. */
    fbe_base_object_get_object_id((fbe_base_object_t *)virtual_drive_p, &vd_object_id);
    fbe_zero_memory(&drive_swap_request, sizeof(fbe_job_service_drive_swap_request_t));
    fbe_job_service_drive_swap_request_init(&drive_swap_request);

    /* Get the original drive pvd id using the swap edge index.
     */
    if ((swap_edge_index != FIRST_EDGE_INDEX)  &&
        (swap_edge_index != SECOND_EDGE_INDEX)    )  
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_TRANSACTION_FAILED,
                              "%s vd obj: 0x%x cmd: %d invalid swap index: %d\n",
                              __FUNCTION__, vd_object_id, swap_command_type, swap_edge_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the original drive pvd id using the mirror edge index.
     */
    if ((mirror_swap_edge_index != FIRST_EDGE_INDEX)  &&
        (mirror_swap_edge_index != SECOND_EDGE_INDEX)    )  
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_TRANSACTION_FAILED,
                              "%s vd obj: 0x%x cmd: %d invalid mirror swap index: %d\n",
                              __FUNCTION__, vd_object_id, swap_command_type, mirror_swap_edge_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the original pvd id.
     */
    fbe_virtual_drive_get_orig_pvd_object_id(virtual_drive_p, &original_pvd_object_id);

    /* Determine the type of copy (proactive or user).  The default is proactive
     * copy.
     */
    if (fbe_virtual_drive_swap_is_primary_edge_healthy(virtual_drive_p))
    {
        /* If this is not a proactive copy clear the `b_is_proactive_copy' flag.
         */
        fbe_job_service_drive_swap_request_set_is_proactive_copy(&drive_swap_request, FBE_FALSE);
    }

    /* Validate the swap command type.
     */
    switch(swap_command_type)
    {
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
        case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
        case FBE_SPARE_COMPLETE_COPY_COMMAND:
        case FBE_SPARE_ABORT_COPY_COMMAND:
            break;

        default:
            /* Unsupported command.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_TRANSACTION_FAILED,
                              "%s vd obj: 0x%x unsupported swap cmd: %d\n",
                              __FUNCTION__, vd_object_id, swap_command_type);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If any of the operation complete flags is set trace an error and 
     * clear them. But continue.
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_OPERATION_COMPLETE_MASK))
    {
        /* Trace an error, clear the flags and continue.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s operation complete flags: 0x%x set: 0x%x\n",
                              __FUNCTION__, 
                              FBE_VIRTUAL_DRIVE_FLAG_OPERATION_COMPLETE_MASK,
                              flags);

        /* Clear the flags and continue
         */
        fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_OPERATION_COMPLETE_MASK);
    }

    /* Create swap request with given edge index. */ 
    fbe_virtual_drive_swap_create_job_service_drive_swap_request(&drive_swap_request,
                                                                 vd_object_id,
                                                                 swap_edge_index,
                                                                 mirror_swap_edge_index,
                                                                 original_pvd_object_id,
                                                                 swap_command_type);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: initiate swap vd obj: 0x%x cmd: %d flags: 0x%x swap index: %d orig pvd obj: 0x%x\n",
                          drive_swap_request.vd_object_id, drive_swap_request.command_type, flags,
                          drive_swap_request.swap_edge_index, drive_swap_request.orig_pvd_object_id);

    /* Flag the fact that we have a job in-progress for this virtual drive.
     */
    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS);

    /* Else set the flag that indicates there is a change request in
     * progress.
     */
    fbe_raid_group_set_clustered_flag((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS);

    /* Send swap-out request to job service. */
    status = fbe_virtual_drive_swap_send_request_to_job_service(virtual_drive_p,
                                                                packet_p,
                                                                FBE_JOB_CONTROL_CODE_DRIVE_SWAP,
                                                                &drive_swap_request,
                                                                sizeof(fbe_job_service_drive_swap_request_t));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_virtual_drive_initiate_swap_operation()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_change_configuration_mode()
 ******************************************************************************
 * @brief
 *  This function is used to change the configuration for the virtual drive
 *  object.
 *
 * @param base_config_p         - Pointer to the base config object.
 * @param packet_p              - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/23/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_change_configuration_mode(fbe_virtual_drive_t * virtual_drive_p)
{
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_virtual_drive_configuration_mode_t  new_configuration_mode;
    fbe_lifecycle_state_t                   my_state;

    /* Quiesce only if Ready.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_get_new_configuration_mode(virtual_drive_p, &new_configuration_mode);
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);

    /* Trace some information.
     */ 
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: change config: life: %d current mode: %d new mode: %d \n",
                          my_state, configuration_mode, new_configuration_mode);

    /* A mode of `unknown' does not change anything.
     */
    if (new_configuration_mode != FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN)
    {
        /* If we are changing from pass-thru to mirror.
         */
        if (fbe_virtual_drive_is_pass_thru_configuration_mode_set(virtual_drive_p))
        {
            if ((new_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE) ||
                (new_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)   )
            {
                /* The job service is not running on the passive SP so we need
                 * `determine' and set the type of swap-in operation
                 */
                fbe_virtual_drive_set_swap_in_command(virtual_drive_p);

                /* Set a condition to quiese. */
                if (my_state == FBE_LIFECYCLE_STATE_READY) {
                    fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const, 
                                           (fbe_base_object_t *) virtual_drive_p,
                                           FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE);
                }

                /* Set the condition to change the configuration mode to mirror. */
                fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                       (fbe_base_object_t *) virtual_drive_p,
                                       FBE_RAID_GROUP_LIFECYCLE_COND_CONFIGURATION_CHANGE);

                /*  Set a condition to unquiese, it is located after the quiesce condition in rotary and
                 *  assumption is all the condition which needs a quiesce operation will be added in
                 *  between these two condition in rotary.
                 */ 
                if (my_state == FBE_LIFECYCLE_STATE_READY) {
                    fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const, 
                                           (fbe_base_object_t *) virtual_drive_p,
                                           FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE);
                }

            }
        }
        else if (fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p))
        {
            /* Else we are changing from mirror back to pass-thru.
             */
            if ((new_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) ||
                (new_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)   )
            {
                /* The job service is not running on the passive SP so we need
                 * `determine' and set the type of swap-in operation
                 */
                fbe_virtual_drive_set_swap_out_command(virtual_drive_p);

                /* Set a condition to quiese. */
                if (my_state == FBE_LIFECYCLE_STATE_READY) {
                    fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const, 
                                           (fbe_base_object_t *) virtual_drive_p,
                                           FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE);
                }

                /* Set the condition to change the configuration mode to pass through. */
                fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                       (fbe_base_object_t *) virtual_drive_p,
                                       FBE_RAID_GROUP_LIFECYCLE_COND_CONFIGURATION_CHANGE);

                /*  Set a condition to unquiese, it is located after the quiesce condition in rotary and
                 *  assumption is all the condition which needs a quiesce operation will be added in
                 *  between these two condition in rotary.
                 */ 
                if (my_state == FBE_LIFECYCLE_STATE_READY) {
                    fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const, 
                                           (fbe_base_object_t *) virtual_drive_p,
                                           FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE);
                }

            }
        }
        else
        {
            /* Trace and error. */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s - Invalid configuraiton mode: %d \n",
                                  __FUNCTION__, configuration_mode);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_virtual_drive_swap_change_configuration_mode()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_send_request_to_job_service()
 ******************************************************************************
 * @brief
 *  This function creates job control element and sends the swap request to the
 *  job service to perform the specific swap operation.
 * 
 * @param virtual_drive_p               - virtual Drive object.
 * @param packet_p                      - master packet to associate with swap 
 *                                        request.
 * @param control_code                  - job service control code.
 * @param buffer                        - payload buffer.
 * @param buffer_length                 - payload buffer length.
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 * 10/19/2009 - Created. Dhaval Patel.
 * 
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_swap_send_request_to_job_service(fbe_virtual_drive_t * virtual_drive_p,
                                                   fbe_packet_t * packet_p,
                                                   fbe_payload_control_operation_opcode_t control_code,
                                                   fbe_payload_control_buffer_t buffer,
                                                   fbe_payload_control_buffer_length_t buffer_length)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t *                     new_payload_p = NULL;
    fbe_payload_ex_t *                     payload_p = NULL;
    fbe_payload_control_operation_t *       new_control_operation_p = NULL;
    fbe_packet_t *                          new_packet_p = NULL;
    fbe_raid_iots_t *                       raid_iots_p = NULL;
    fbe_payload_control_buffer_t *          job_service_request_p = NULL;
    
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

     /* Get the payload of the master packet. */
     payload_p = fbe_transport_get_payload_ex(packet_p);

    /* Allocate the new sub packet to which will be attached to monitor packet
     * and used to send it to job control service.
     */
    new_packet_p = fbe_transport_allocate_packet();
    if(new_packet_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_release_buffer(buffer);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;        
    }

    /* Initialize the packet and get the payload to create a control packet. */
    fbe_transport_initialize_sep_packet(new_packet_p);
    new_payload_p = fbe_transport_get_payload_ex(new_packet_p);
    new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_payload_p);
    if(new_control_operation_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_release_packet(new_packet_p);
        fbe_transport_set_status (packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    
    /* allocate the buffer for the job service request. */
    job_service_request_p = fbe_transport_allocate_buffer();
    fbe_copy_memory(job_service_request_p, buffer, buffer_length);
    
    /* build the control operation with Job service request. */
    fbe_payload_control_build_operation(new_control_operation_p,
                                        control_code,
                                        job_service_request_p,
                                        buffer_length);
    
    /* Set our completion function. */
    status = fbe_transport_set_completion_function(new_packet_p, 
                                                   fbe_virtual_drive_swap_send_request_to_job_service_completion,
                                                   virtual_drive_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_release_buffer(job_service_request_p);
        fbe_payload_ex_release_control_operation(new_payload_p, new_control_operation_p);
        fbe_transport_release_packet(new_packet_p);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    
    /* Add the newly created packet in subpacket queue and put the master packet to
     * terminator queue, we always make sure so that iots status get set to not used
     * whenever we add any packet to terminator queue from virtual drive object.
     */
    fbe_transport_add_subpacket(packet_p, new_packet_p);
    fbe_payload_ex_get_iots(payload_p, &raid_iots_p);

    /* Initialize the iots with default state. */
    fbe_raid_iots_basic_init(raid_iots_p, packet_p, NULL);
    fbe_raid_iots_basic_init_common(raid_iots_p);
    fbe_raid_iots_set_as_not_used(raid_iots_p);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)virtual_drive_p, packet_p);
    
    /* We are sending control packet to job service. */
    fbe_transport_set_address(new_packet_p,
                          FBE_PACKAGE_ID_SEP_0,
                          FBE_SERVICE_ID_JOB_SERVICE,
                          FBE_CLASS_ID_INVALID,
                          FBE_OBJECT_ID_INVALID);
    
    /* Control packets should be sent via service manager.
    */
    status = fbe_service_manager_send_control_packet(new_packet_p);
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_send_swap_request_to_job_service()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_send_request_to_job_service_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the swap request subpacket,
 *  it is used to copy the status to master packet and then cleanup the 
 *  subpacket which is used to send the swap request to job service.
 * 
 * @param packet_p      - master packet to associate with swap request.
 * @param context       - packet completion context.
 *
 * @return fbe_status_t - status of the operation.

 * @author
 * 10/19/2009 - Created. Dhaval Patel.
 * 
 ******************************************************************************/
static fbe_status_t 
fbe_virtual_drive_swap_send_request_to_job_service_completion(fbe_packet_t * packet_p,
                                                              fbe_packet_completion_context_t context)
{
    fbe_virtual_drive_t *               virtual_drive_p = NULL;
    fbe_packet_t *                      master_packet_p = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_buffer_t        job_service_request_p = NULL;
    fbe_raid_iots_t *iots_p = NULL;

    virtual_drive_p = (fbe_virtual_drive_t *)context;
    master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "virtual drive job service request failed with status: %d\n", status);
    }

    /* We need to remove packet from master queue. */
    fbe_transport_remove_subpacket(packet_p);

    /* Copy the status back to the master. */
    fbe_transport_copy_packet_status(packet_p, master_packet_p);

    /* Free the resources we allocated previously. */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &job_service_request_p);
    fbe_transport_release_buffer(job_service_request_p);
    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_transport_release_packet(packet_p);

    /* Remove master packet from the termination queue. */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)virtual_drive_p, master_packet_p);

    fbe_payload_ex_get_iots(payload, &iots_p);
    fbe_raid_iots_basic_destroy(iots_p);

    /* Complete the master packet which will allow the clear the condition. */
    fbe_transport_complete_packet(master_packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_send_request_to_job_service_completion()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_handle_permanent_spare_request_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the completion we have received from the job 
 *  service (swap library) for a permanent spare swap request.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object that requires the spare
 * @param   packet_p - The orginating packet
 * @param   command_type - The swap command
 * @param   status_code - The job status
 *
 * @return fbe_status_t status of the error response handling.
 *
 * @author
 *  04/25/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_swap_handle_permanent_spare_request_completion(fbe_virtual_drive_t *virtual_drive_p,
                                                               fbe_packet_t *packet_p,
                                                               fbe_spare_swap_command_t command_type,
                                                               fbe_job_service_error_type_t status_code)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_bool_t      b_no_spare_reported = FBE_FALSE;
    fbe_object_id_t original_pvd_object_id;
    fbe_object_id_t spare_pvd_object_id = FBE_OBJECT_ID_INVALID;

    /* Clear the `swap-in' edge flag.
     */
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE);

    /* This should have only been invoked for permanent spare requests.
     */
    if (command_type != FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND)
    {
        /* Generate an error trace.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Unexpected swap command: %d \n", 
                              __FUNCTION__, command_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Determine if we need to set or clear the `no spare reported' non-paged
     * flag.
     */
    fbe_virtual_drive_metadata_has_no_spare_been_reported(virtual_drive_p, &b_no_spare_reported);

    /* Using the virtual drive configuration mode get the original provision
     * drive object id. (There is no spare so the object id is `invalid').
     */
    fbe_virtual_drive_event_logging_get_original_pvd_object_id(virtual_drive_p,
                                                               &original_pvd_object_id,
                                                               FBE_FALSE /* Copy is not in progress */);

    /*! @note Always clear or reset the `need replacement start time.
     */
    switch(status_code)
    {
        case FBE_JOB_SERVICE_ERROR_NO_ERROR:
            /* Clearing the `need replacement drive' timer automatically clears
             * the following flags:
             *  o No Spare Reported
             *  o Spare Request Denied
             *  o etc
             */
            fbe_virtual_drive_swap_clear_need_replacement_drive_start_time(virtual_drive_p);
            break;

        case FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SPARES_AVAILABLE:
            /* No spares in the system.  If we have not reported that there are 
             * no spares available report it now.  Set the virtual drive flag 
             * that indicates we need to set the `no spare found' bit in the 
             * non-paged metadata.
             */
            if ( (b_no_spare_reported == FBE_FALSE)                                                           &&
                !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SET_NO_SPARE_REPORTED)    )
            {
                /* Generate the event.
                 */
                status = fbe_virtual_drive_write_event_log(virtual_drive_p,
                                                           SEP_WARN_SPARE_NO_SPARES_AVAILABLE,
                                                           FBE_SPARE_INTERNAL_ERROR_NONE,
                                                           original_pvd_object_id,
                                                           spare_pvd_object_id,
                                                           packet_p);
                
                /* If successfull set the flag that indicates we should not
                 * report this event again.
                 */
                if (status == FBE_STATUS_OK)
                {
                    /* Setting this flag will set the `no spare reported' np bit.
                     */
                    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SET_NO_SPARE_REPORTED);
                }
                else
                {
                    /* Else report the failure.
                     */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Failed to generate `no spares' event. status: 0x%x\n", 
                                          __FUNCTION__, status);
                    return status;
                }
            }
            break;

        case FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SUITABLE_SPARE:
            /* No suitable spare found. If we have not reported that there are 
             * no spares available report it now.  Set the virtual drive flag 
             * that indicates we need to set the `no spare found' bit in the 
             * non-paged metadata.
             */
            if ( (b_no_spare_reported == FBE_FALSE)                                                           &&
                !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SET_NO_SPARE_REPORTED)    )
            {
                /* Generate the event.
                 */
                status = fbe_virtual_drive_write_event_log(virtual_drive_p,
                                                           SEP_WARN_SPARE_NO_SUITABLE_SPARE_AVAILABLE,
                                                           FBE_SPARE_INTERNAL_ERROR_NONE,
                                                           original_pvd_object_id,
                                                           spare_pvd_object_id,
                                                           packet_p);

                /* If successfull set the flag that indicates we should not
                 * report this event again.
                 */
                if (status == FBE_STATUS_OK)
                {
                    /* Setting this flag will set the `no spare reported' np bit.
                     */
                    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SET_NO_SPARE_REPORTED);
                }
                else
                {
                    /* Else report the failure.
                     */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Failed to generate `no spares' event. status: 0x%x\n", 
                                          __FUNCTION__, status);
                    return status;
                }
            }
            break;

        case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_BROKEN:
            /* Raid group is shutdown.  Reset the timer.
             */
            fbe_virtual_drive_swap_reset_need_replacement_drive_start_time(virtual_drive_p);
            if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_BROKEN_REPORTED))
            {
                /* Generate the event.
                 */
                status = fbe_virtual_drive_write_event_log(virtual_drive_p,
                                                           SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_BROKEN,
                                                           FBE_SPARE_INTERNAL_ERROR_NONE,
                                                           original_pvd_object_id,
                                                           spare_pvd_object_id,
                                                           packet_p);

                /* If successfull set the flag that indicates we should not
                 * report this event again.
                 */
                if (status == FBE_STATUS_OK)
                {
                    /* Set the `raid group broken'
                     */
                    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_BROKEN_REPORTED);
                }
                else
                {
                    /* Else report the failure.
                     */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Failed to generate `raid group broken' event. status: 0x%x\n", 
                                          __FUNCTION__, status);
                    return status;
                }
            }
            break;
 
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DENIED:
            /* Raid group denied.  Reset the timer.
             */
            fbe_virtual_drive_swap_reset_need_replacement_drive_start_time(virtual_drive_p);
            if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DENIED_REPORTED))
            {
                /* Generate the event.
                 */
                status = fbe_virtual_drive_write_event_log(virtual_drive_p,
                                                           SEP_ERROR_CODE_SWAP_FAILED_RAID_GROUP_DENIED,
                                                           FBE_SPARE_INTERNAL_ERROR_NONE,
                                                           original_pvd_object_id,
                                                           spare_pvd_object_id,
                                                           packet_p);

                /* If successfull set the flag that indicates we should not
                 * report this event again.
                 */
                if (status == FBE_STATUS_OK)
                {
                    /* Set the `raid group broken'
                     */
                    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DENIED_REPORTED);
                }
                else
                {
                    /* Else report the failure.
                     */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Failed to generate `raid group denied' event. status: 0x%x\n", 
                                          __FUNCTION__, status);
                    return status;
                }
            }
            break;
                     
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DEGRADED:
            /* Raid group is degraded.  Reset the timer.
             */
            fbe_virtual_drive_swap_reset_need_replacement_drive_start_time(virtual_drive_p);
            if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DEGRADED_REPORTED))
            {
                /* Generate the event.
                 */
                status = fbe_virtual_drive_write_event_log(virtual_drive_p,
                                                           SEP_ERROR_CODE_COPY_RAID_GROUP_DEGRADED,
                                                           FBE_SPARE_INTERNAL_ERROR_NONE,
                                                           original_pvd_object_id,
                                                           spare_pvd_object_id,
                                                           packet_p);

                /* If successfull set the flag that indicates we should not
                 * report this event again.
                 */
                if (status == FBE_STATUS_OK)
                {
                    /* Set the `raid group broken'
                     */
                    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DEGRADED_REPORTED);
                }
                else
                {
                    /* Else report the failure.
                     */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Failed to generate `raid group degraded' event. status: 0x%x\n", 
                                          __FUNCTION__, status);
                    return status;
                }
            }
            break;
             
        case FBE_JOB_SERVICE_ERROR_SWAP_PERMANENT_SPARE_NOT_REQUIRED:
            /* Original drive came back (there will always be a race condition
             * where we initiate the swap-in but during the validation we determine
             * that the original drive came back).
             */
            fbe_virtual_drive_swap_clear_need_replacement_drive_start_time(virtual_drive_p);
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s swap status: %d original drive re-inserted\n", 
                                  __FUNCTION__, status_code);
            break;

        case FBE_JOB_SERVICE_ERROR_INVALID_ORIGINAL_OBJECT_ID:
            /* A terminal error because something failed.  For instance the spare
             * request completed but the SP PANICed before the job was complete.
             * The surviving SP will attempt to restart the job but the downstream
             * state (for instance provision drive object id) has changed.
             */
            fbe_virtual_drive_swap_clear_need_replacement_drive_start_time(virtual_drive_p);
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s permanent spare request failed. swap status: %d\n", 
                                  __FUNCTION__, status_code);
            break;

        case FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_UNCONSUMED:
            /* A non-terminal error (user could bind a LUN at any time).
             * reset the timer and report a warning.
             */
            fbe_virtual_drive_swap_reset_need_replacement_drive_start_time(virtual_drive_p);
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s permanent spare request failed. swap status: %d\n", 
                                  __FUNCTION__, status_code);
            break;

        default:
            /* Else clear the start time and report the failure.
             */
            fbe_virtual_drive_swap_clear_need_replacement_drive_start_time(virtual_drive_p);
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unexpected swap status: %d\n", 
                                  __FUNCTION__, status_code);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return the status.
     */
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_handle_permanent_spare_request_completion()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_handle_proactive_copy_request_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the completion we have received from the job 
 *  service (swap library) for a proactive spare swap request.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object that requires the spare
 * @param   packet_p - The orginating packet
 * @param   command_type - The swap command
 * @param   status_code - The job status
 *
 * @return fbe_status_t status of the error response handling.
 *
 * @author
 *  09/24/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_swap_handle_proactive_copy_request_completion(fbe_virtual_drive_t *virtual_drive_p,
                                                               fbe_packet_t *packet_p,
                                                               fbe_spare_swap_command_t command_type,
                                                               fbe_job_service_error_type_t status_code)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_bool_t              b_no_spare_reported = FBE_FALSE;
    fbe_object_id_t         original_pvd_object_id;
    fbe_object_id_t         spare_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t *)virtual_drive_p);

    /* Clear the `swap-in' edge flag.
     */
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE);

    /* This should have only been invoked for proactive requests.
     */
    if (command_type != FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND)
    {
        /* Generate an error trace.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Unexpected swap command: %d \n", 
                              __FUNCTION__, command_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Determine if we need to set or clear the `no spare reported' non-paged
     * flag.
     */
    fbe_virtual_drive_metadata_has_no_spare_been_reported(virtual_drive_p, &b_no_spare_reported);

    /* Using the virtual drive configuration mode get the original provision
     * drive object id. (There is no spare so the object id is `invalid').
     */
    fbe_virtual_drive_event_logging_get_original_pvd_object_id(virtual_drive_p,
                                                               &original_pvd_object_id,
                                                               FBE_FALSE /* Copy is not in progress */);

    /*! @note There is no delay before starting a proactive copy.  Therefore
     *        the `need replacement drive start time' does not apply to
     *        proactive copy.
     */
    switch(status_code)
    {
        case FBE_JOB_SERVICE_ERROR_NO_ERROR:
            /* Clearing the `need proactive copy' timer automatically clears
             * the following flags:
             *  o No Spare Reported
             *  o Spare Request Denied
             *  o Proactive copy request denied
             *  o etc
             */
            fbe_virtual_drive_swap_clear_need_proactive_copy_start_time(virtual_drive_p);

            /* Set the proactive copy flag.
             */
            fbe_raid_geometry_set_attribute(raid_geometry_p, FBE_RAID_ATTRIBUTE_PROACTIVE_SPARING);
            break;

        case FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SPARES_AVAILABLE:
            /* No spares in the system.  If we have not reported that there are 
             * no spares available report it now.  Set the virtual drive flag 
             * that indicates we need to set the `no spare found' bit in the 
             * non-paged metadata.
             */
            fbe_virtual_drive_swap_reset_need_proactive_copy_start_time(virtual_drive_p);
            if ( (b_no_spare_reported == FBE_FALSE)                                                           &&
                !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SET_NO_SPARE_REPORTED)    )
            {
                /* Generate the event.
                 */
                status = fbe_virtual_drive_write_event_log(virtual_drive_p,
                                                           SEP_WARN_SPARE_NO_SPARES_AVAILABLE,
                                                           FBE_SPARE_INTERNAL_ERROR_NONE,
                                                           original_pvd_object_id,
                                                           spare_pvd_object_id,
                                                           packet_p);
                
                /* If successfull set the flag that indicates we should not
                 * report this event again.
                 */
                if (status == FBE_STATUS_OK)
                {
                    /* Setting this flag will set the `no spare reported' np bit.
                     */
                    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SET_NO_SPARE_REPORTED);
                }
                else
                {
                    /* Else report the failure.
                     */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Failed to generate `no spares' event. status: 0x%x\n", 
                                          __FUNCTION__, status);
                    return status;
                }
            }

            /* Set the condition to retry the proactive copy operation.
             */
            fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                   (fbe_base_object_t *)virtual_drive_p,
                                   FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE);
            break;

        case FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SUITABLE_SPARE:
            /* No suitable spare found. If we have not reported that there are 
             * no spares available report it now.  Set the virtual drive flag 
             * that indicates we need to set the `no spare found' bit in the 
             * non-paged metadata.
             */
            fbe_virtual_drive_swap_reset_need_proactive_copy_start_time(virtual_drive_p);
            if ( (b_no_spare_reported == FBE_FALSE)                                                           &&
                !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SET_NO_SPARE_REPORTED)    )
            {
                /* Generate the event.
                 */
                status = fbe_virtual_drive_write_event_log(virtual_drive_p,
                                                           SEP_WARN_SPARE_NO_SUITABLE_SPARE_AVAILABLE,
                                                           FBE_SPARE_INTERNAL_ERROR_NONE,
                                                           original_pvd_object_id,
                                                           spare_pvd_object_id,
                                                           packet_p);

                /* If successfull set the flag that indicates we should not
                 * report this event again.
                 */
                if (status == FBE_STATUS_OK)
                {
                    /* Setting this flag will set the `no spare reported' np bit.
                     */
                    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SET_NO_SPARE_REPORTED);
                }
                else
                {
                    /* Else report the failure.
                     */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Failed to generate `no spares' event. status: 0x%x\n", 
                                          __FUNCTION__, status);
                    return status;
                }
            }

            /* Set the condition to retry the proactive copy operation.
             */
            fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                   (fbe_base_object_t *)virtual_drive_p,
                                   FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE);
            break;

        case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_BROKEN:
            /* Raid group is shutdown.
             */
            fbe_virtual_drive_swap_reset_need_proactive_copy_start_time(virtual_drive_p);
            if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_BROKEN_REPORTED))
            {
                /* Generate the event.
                 */
                status = fbe_virtual_drive_write_event_log(virtual_drive_p,
                                                           SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_BROKEN,
                                                           FBE_SPARE_INTERNAL_ERROR_NONE,
                                                           original_pvd_object_id,
                                                           spare_pvd_object_id,
                                                           packet_p);

                /* If successfull set the flag that indicates we should not
                 * report this event again.
                 */
                if (status == FBE_STATUS_OK)
                {
                    /* Setting this flag will set the `no spare reported' np bit.
                     */
                    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_BROKEN_REPORTED);
                }
                else
                {
                    /* Else report the failure.
                     */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Failed to generate `raid group broken' event. status: 0x%x\n", 
                                          __FUNCTION__, status);
                    return status;
                }
            }

            /* Set the condition to retry the proactive copy operation.
             */
            fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                   (fbe_base_object_t *)virtual_drive_p,
                                   FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE);
            break;
   
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DENIED:
            /* Raid group denied.
             */
            fbe_virtual_drive_swap_reset_need_proactive_copy_start_time(virtual_drive_p);
            if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DENIED_REPORTED))
            {
                /* Generate the event.
                 */
                status = fbe_virtual_drive_write_event_log(virtual_drive_p,
                                                           SEP_ERROR_CODE_SWAP_FAILED_RAID_GROUP_DENIED,
                                                           FBE_SPARE_INTERNAL_ERROR_NONE,
                                                           original_pvd_object_id,
                                                           spare_pvd_object_id,
                                                           packet_p);

                /* If successfull set the flag that indicates we should not
                 * report this event again.
                 */
                if (status == FBE_STATUS_OK)
                {
                    /* Setting this flag will set the `no spare reported' np bit.
                     */
                    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DENIED_REPORTED);
                }
                else
                {
                    /* Else report the failure.
                     */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Failed to generate `raid group denied' event. status: 0x%x\n", 
                                          __FUNCTION__, status);
                    return status;
                }
            }

            /* Set the condition to retry the proactive copy operation.
             */
            fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                   (fbe_base_object_t *)virtual_drive_p,
                                   FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE);
            break;
                    
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DEGRADED:
            /* Raid group is degraded.
             */
            fbe_virtual_drive_swap_reset_need_proactive_copy_start_time(virtual_drive_p);
            if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DEGRADED_REPORTED))
            {
                /* Generate the event.
                 */
                status = fbe_virtual_drive_write_event_log(virtual_drive_p,
                                                           SEP_ERROR_CODE_COPY_RAID_GROUP_DEGRADED,
                                                           FBE_SPARE_INTERNAL_ERROR_NONE,
                                                           original_pvd_object_id,
                                                           spare_pvd_object_id,
                                                           packet_p);

                /* If successfull set the flag that indicates we should not
                 * report this event again.
                 */
                if (status == FBE_STATUS_OK)
                {
                    /* Setting this flag will set the `no spare reported' np bit.
                     */
                    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DEGRADED_REPORTED);
                }
                else
                {
                    /* Else report the failure.
                     */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Failed to generate `raid group degraded' event. status: 0x%x\n", 
                                          __FUNCTION__, status);
                    return status;
                }
            }

            /* Set the condition to retry the proactive copy operation.
             */
            fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                   (fbe_base_object_t *)virtual_drive_p,
                                   FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE);
            break;
               
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_HAS_COPY_IN_PROGRESS:
            /* Need to wait for current copy operation to complete.
             */
            fbe_virtual_drive_swap_reset_need_proactive_copy_start_time(virtual_drive_p);
            if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_IN_PROGRESS_REPORTED))
            {
                /* Generate the event.
                 */
                status = fbe_virtual_drive_write_event_log(virtual_drive_p,
                                                           SEP_ERROR_CODE_RAID_GROUP_HAS_COPY_IN_PROGRESS,
                                                           FBE_SPARE_INTERNAL_ERROR_NONE,
                                                           original_pvd_object_id,
                                                           spare_pvd_object_id,
                                                           packet_p);

                /* If successfull set the flag that indicates we should not
                 * report this event again.
                 */
                if (status == FBE_STATUS_OK)
                {
                    /* Setting this flag will set the `no spare reported' np bit.
                     */
                    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_IN_PROGRESS_REPORTED);
                }
                else
                {
                    /* Else report the failure.
                     */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Failed to generate `copy in progress' event. status: 0x%x\n", 
                                          __FUNCTION__, status);
                    return status;
                }
            }

            /* Set the condition to retry the proactive copy operation.
             */
            fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                   (fbe_base_object_t *)virtual_drive_p,
                                   FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE);
            break;

        case FBE_JOB_SERVICE_ERROR_PRESENTLY_SOURCE_DRIVE_DEGRADED:
            /* Need to wait for source drive checkpoint to be set to end marker.
             */
            fbe_virtual_drive_swap_reset_need_proactive_copy_start_time(virtual_drive_p);
            if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_SOURCE_DRIVE_DEGRADED))
            {
                /* Generate the event.
                 */
                status = fbe_virtual_drive_write_event_log(virtual_drive_p,
                                                           SEP_ERROR_CODE_COPY_SOURCE_DRIVE_DEGRADED,
                                                           FBE_SPARE_INTERNAL_ERROR_NONE,
                                                           original_pvd_object_id,
                                                           spare_pvd_object_id,
                                                           packet_p);

                /* If successfull set the flag that indicates we should not
                 * report this event again.
                 */
                if (status == FBE_STATUS_OK)
                {
                    /* Setting this flag will set the `no spare reported' np bit.
                     */
                    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_SOURCE_DRIVE_DEGRADED);
                }
                else
                {
                    /* Else report the failure.
                     */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Failed to generate `source drive degraded event' event. status: 0x%x\n", 
                                          __FUNCTION__, status);
                    return status;
                }
            }

            /* Set the condition to retry the proactive copy operation.
             */
            fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                   (fbe_base_object_t *)virtual_drive_p,
                                   FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE);
            break;

        case FBE_JOB_SERVICE_ERROR_SWAP_PROACTIVE_SPARE_NOT_REQUIRED:
            /* Original drive came back (there will always be a race condition
             * where we initiate the swap-in but during the validation we determine
             * that the original drive came back).
             */
            fbe_virtual_drive_swap_clear_need_proactive_copy_start_time(virtual_drive_p);
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s swap status: %d original drive re-inserted\n", 
                                  __FUNCTION__, status_code);
            break;

        case FBE_JOB_SERVICE_ERROR_SWAP_VIRTUAL_DRIVE_BROKEN:
            /* Original drive failed before proactive copy operation was started.
             */
            fbe_virtual_drive_swap_clear_need_proactive_copy_start_time(virtual_drive_p);
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s swap status: %d original drive failed\n", 
                                  __FUNCTION__, status_code);
            break;

        case FBE_JOB_SERVICE_ERROR_INVALID_ORIGINAL_OBJECT_ID:
            /* A terminal error because something failed.  For instance the spare
             * request completed but the SP PANICed before the job was complete.
             * The surviving SP will attempt to restart the job but the downstream
             * state (for instance provision drive object id) has changed.
             */
            fbe_virtual_drive_swap_clear_need_proactive_copy_start_time(virtual_drive_p);
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s proactive copy request failed. swap status: %d\n", 
                                  __FUNCTION__, status_code);
            break;

        case FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_UNCONSUMED:
            /* A non-terminal error (user could bind a LUN at any time).
             * reset the timer and report a warning.
             */
            fbe_virtual_drive_swap_reset_need_proactive_copy_start_time(virtual_drive_p);
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s proactive copy request failed. swap status: %d\n", 
                                  __FUNCTION__, status_code);
            break;

        default:
            /* Else clear all the report flags report the failure.
             */
            fbe_virtual_drive_swap_clear_need_proactive_copy_start_time(virtual_drive_p);
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unexpected swap status: %d\n", 
                                  __FUNCTION__, status_code);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return the status.
     */
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_handle_proactive_copy_request_completion()
*******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_completion_cleanup()
 *****************************************************************************
 *
 * @brief   This function is called from EITHER:
 *              o Job service - thru `swap complete' usurper
 *                          OR
 *              o The virtual drive monitor
 *          Since the virtual drive should control (i.e. modify) the flags
 *          associated with any swap request, the swap completion should be
 *          executed from the monitor context.  But for testing purposes we
 *          allow asynchronous job process (a.k.a. we allow operation confirmation
 *          to be disabled).
 *
 * @param   virtual_drive_p - virtual drive object.
 * @param   packet_p - Pointer to either usurper or monitor packet
 *
 * @return  fbe_status_t
 *
 * @author
 *  04/26/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_completion_cleanup(fbe_virtual_drive_t *virtual_drive_p,
                                                       fbe_packet_t *packet_p)
{
    fbe_spare_swap_command_t        swap_command_type = FBE_SPARE_SWAP_INVALID_COMMAND;
    fbe_trace_level_t               job_error_trace_level = FBE_TRACE_LEVEL_WARNING;
    fbe_u32_t                       job_status;
    fbe_job_service_error_type_t    status_code;
    fbe_lifecycle_state_t           my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_raid_geometry_t            *raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t *)virtual_drive_p);

    /* Get the lifecycle state.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);

    /* Get and validate the swap command.
     */
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &swap_command_type);

    /* Get the status of the job.
     */
    fbe_virtual_drive_get_job_status(virtual_drive_p, &job_status);
    status_code = (fbe_job_service_error_type_t)job_status;

    /* This method should only be called once.  If the cleanup swap request
     * flag is already set trace an error but return success.
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CLEANUP_SWAP_REQUEST))
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Called multiple times\n", 
                              __FUNCTION__);
        return FBE_STATUS_OK;
    }

    /* Set the flag indicating that this method was called.
     */
    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CLEANUP_SWAP_REQUEST);

    /*! @note Simply log the error if the swap command failed.  But we continue
     *        processing the request so that we `cleanup':
     *          o Clear any conditions set
     */
    if (status_code != FBE_JOB_SERVICE_ERROR_NO_ERROR)
    {
        /* If the error is `no spare' lower trace level
         */
        if ((status_code == FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SPARES_AVAILABLE) ||
            (status_code == FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SUITABLE_SPARE)      )
        {
            job_error_trace_level = FBE_TRACE_LEVEL_DEBUG_HIGH;
        }
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              job_error_trace_level,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s swap command: %d job status: %d\n", 
                              __FUNCTION__, swap_command_type, status_code);

        /* Clear any outstanding conditions
         */
        fbe_lifecycle_force_clear_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
                                       FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SWAP_IN_EDGE);
        fbe_lifecycle_force_clear_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
                                       FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SWAP_OUT_EDGE);
        fbe_lifecycle_force_clear_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
                                       FBE_RAID_GROUP_LIFECYCLE_COND_CONFIGURATION_CHANGE);
        fbe_lifecycle_force_clear_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
                                       FBE_RAID_GROUP_LIFECYCLE_COND_SET_REBUILD_CHECKPOINT_TO_END_MARKER);
        fbe_lifecycle_force_clear_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
                                       FBE_RAID_GROUP_LIFECYCLE_COND_DEST_DRIVE_FAILED_SET_REBUILD_CHECKPOINT_TO_END_MARKER);
    }

    /* clear the swap-in/swap-out progress flag when we receive the status. */
    if (swap_command_type == FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND)
    {
        /* Invoke method to handle permanent spare swap request completion.
         */
        fbe_virtual_drive_swap_handle_permanent_spare_request_completion(virtual_drive_p,
                                                                         packet_p,
                                                                         swap_command_type,
                                                                         status_code);
    }
    else if (swap_command_type == FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND)
    {
        /* Invoke method to handle proactive spare swap request completion.
         */
        fbe_virtual_drive_swap_handle_proactive_copy_request_completion(virtual_drive_p,
                                                                        packet_p,
                                                                        swap_command_type,
                                                                        status_code);
    }
    else if ((swap_command_type == FBE_SPARE_INITIATE_USER_COPY_COMMAND)    ||
             (swap_command_type == FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND)    )
    {
        /* No additional actions for user requests.
         */
    }
    else if ((swap_command_type == FBE_SPARE_COMPLETE_COPY_COMMAND) ||
             (swap_command_type == FBE_SPARE_ABORT_COPY_COMMAND)       )
    {   
        /* Clear the proactive copy attribute.
         */
        fbe_raid_geometry_clear_attribute(raid_geometry_p, FBE_RAID_ATTRIBUTE_PROACTIVE_SPARING);

        /* Re-evaluate the downstream health.
         */ 
        if (my_state == FBE_LIFECYCLE_STATE_READY)
        {
            fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p,
                                   FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_EVALUATE_DOWNSTREAM_HEALTH);
        }
    }
    else
    {
        /* Unsupported command report an error but continue
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s unsupported swap command: %d\n", 
                              __FUNCTION__, swap_command_type);
    }

    /* Clear the raid group `change in progress' flag.  Even though there are
     * more steps to complete or abort a copy operation (`request in progress
     * will still be set') we can allow the raid group to join etc.
     */
    fbe_raid_group_clear_clustered_flag((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS);

    /*! @note We don't clear the request in progress here.  It is only cleared
     *        when the job is about to complete.
     */

    /* If there a peer join in progress set the condition to re-start the join.
     */
    fbe_raid_group_set_join_condition_if_required((fbe_raid_group_t *)virtual_drive_p);

    /* Always return success
     */
    return FBE_STATUS_OK;
}
/*************************************************** 
 * end fbe_virtual_drive_swap_completion_cleanup()
 **************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_handle_swap_request_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the completion we have received from the job 
 *  service (swap library) for the specific swap operation.
 *
 * @param virtual_drive_p - virtual dirve object.
 * @param packet_p -        job service packet pointer.
 *
 * @return fbe_status_t
 *  status of the error response handling.
 *
 * @author
 *  11/03/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_handle_swap_request_completion(fbe_virtual_drive_t *virtual_drive_p,
                                                             fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_service_drive_swap_request_t   *drive_swap_request_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_job_service_error_type_t            status_code;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_spare_swap_command_t                swap_command_type;
    fbe_edge_index_t                        swap_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_flags_t               flags;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;

    /* Get the virtual drive flags (contain state).
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);

    /* Get the payload.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p,
                                                          sizeof(fbe_job_service_drive_swap_request_t),
                                                          (fbe_payload_control_buffer_t *)&drive_swap_request_p);
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Get some information
     */
    fbe_job_service_drive_swap_request_get_status(drive_swap_request_p, &status_code);
    fbe_job_service_drive_swap_request_get_swap_command_type(drive_swap_request_p, &swap_command_type);
    fbe_job_service_drive_swap_request_get_swap_edge_index(drive_swap_request_p, &swap_edge_index);

    /* Trace some information
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: usurper swap complete mode: %d life: %d cmd: %d flags: 0x%x index: %d job status: %d\n",
                          configuration_mode, my_state, swap_command_type, flags, swap_edge_index, status_code);

    /* User copy requests are initiated by the job service (all other swap 
     * requests are initiated by the virtual drive and therefore we must
     * perform the cleanup).  If a user copy request failed the validation it
     * most likely was never started.  We only wait for a user copy
     * completion if it was started.
     */
    if (((swap_command_type == FBE_SPARE_INITIATE_USER_COPY_COMMAND)    ||
         (swap_command_type == FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND)    )                               &&
        (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CONFIRMATION_DISABLED) &&
         !fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_USER_COPY_STARTED)        )    )
    {
        /* We expect the swap status to be failure.
         */
        if (status_code == FBE_JOB_SERVICE_ERROR_NO_ERROR)
        {
            /* Generate an error trace and fail the request.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: usurper swap complete mode: %d life: %d cmd: %d job status: %d user copy not started\n",
                          configuration_mode, my_state, swap_command_type, status_code);

            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, status, 0);
            fbe_transport_complete_packet(packet_p);
            return status;
        }

        /* The user copy request failed validation.  Therefore it was never started so
         * so there is no cleanup required.
         */
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* Validate that we the swap in-progress is set
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS))
    {
        /*! @note Special case are user initiated commands.  If they fail 
         *        they fail during validation.
         */
        if ((status_code == FBE_JOB_SERVICE_ERROR_NO_ERROR)                     ||
            ((swap_command_type != FBE_SPARE_INITIATE_USER_COPY_COMMAND)    &&
             (swap_command_type != FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND)    )   ) 
        {
            /* Logic error.  Trace and error and fail the request.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s swap in-progress not set flags: 0x%x\n", 
                              __FUNCTION__, flags);

            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Set the job status in the virtual drive
     */
    fbe_virtual_drive_set_job_status(virtual_drive_p, drive_swap_request_p->status_code);

    /*! @note For testing purposes we allow `asynchronous' completion.
     *        Normally all operations should be completed in the monitor
     *        context but when we hit a debug hook we don't want to block
     *        the job service so we allow the job to complete without waiting
     *        for the virtual drive monitor.
     */ 
    if (drive_swap_request_p->b_operation_confirmation == FBE_FALSE)
    {
        /* Generate a warning message and perform the cleanup now
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: usurper swap complete mode: %d life: %d cmd: %d confirmation disabled\n",
                              configuration_mode, my_state, swap_command_type);

        /* Ignore the return status.
         */
        fbe_virtual_drive_swap_completion_cleanup(virtual_drive_p, packet_p);

        /* Clear the cleanup swap request flag.
         */
        fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CLEANUP_SWAP_REQUEST);

        /*! @note Do not clear the FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS
         *        flag here.  It can only be cleared when the job is totally done.
         */
    }
    else
    {
        /* Else set the virtual drive monitor condition that will perform the 
         * cleanup in the monitor context.
         */
        fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                               (fbe_base_object_t *)virtual_drive_p,
                               FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SWAP_OPERATION_COMPLETE);

        /* Reschedule immediately
         */
        fbe_lifecycle_reschedule(&fbe_base_config_lifecycle_const,
                                 (fbe_base_object_t *)virtual_drive_p,
                                 (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */
    }

    /* Return success
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_handle_swap_request_completion()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_get_permanent_spare_edge_index()
 ******************************************************************************
 * @brief
 *  This function is used to get the edge index where we can swap-in the
 *  permanent spare drive.
 *
 * @param virtual_drive_p                   - virtual dirve object.
 * @param permanent_spare_edge_index_p      - pointer to permanent spare edge
 *                                            index.
 * 
 * @return fbe_status_t                     - status of the operation.
 *
 * @author
 *  11/03/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_swap_get_permanent_spare_edge_index(fbe_virtual_drive_t * virtual_drive_p,
                                                      fbe_edge_index_t * permanent_spare_edge_index_p)
{
    fbe_virtual_drive_configuration_mode_t configuration_mode;

    *permanent_spare_edge_index_p = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) {
        /* swap-in permanent spare with first edge. */
        *permanent_spare_edge_index_p = FIRST_EDGE_INDEX;
    }
    else if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) {
        /* swap-in permanent spare with second edge. */
        *permanent_spare_edge_index_p = SECOND_EDGE_INDEX;
    }
    else {
        *permanent_spare_edge_index_p = FBE_EDGE_INDEX_INVALID;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_get_permanent_spare_edge_index()
*******************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_swap_get_swap_out_edge_index()
 ******************************************************************************
 * @brief
 *  This function is used to get the edge index which we need to swap-out based
 *  on its current downstream health. 

 * @note 
 *  This function should be called only from monitor context from the swap-out
 *  edge condition.
 *
 * @param virtual_drive_p                   - virtual dirve object.
 * @param edge_index_to_swap_out_p          - pointer to edge index which we
 *                                            wanted to swap-out.                    
 * 
 * @return fbe_status_t                     - status of the operation.
 *
 * @author
 *  06/05/2013  Ron Proulx  - Updated.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_get_swap_out_edge_index(fbe_virtual_drive_t * virtual_drive_p,
                                                            fbe_edge_index_t *edge_index_to_swap_out_p)
{
    fbe_path_state_t                        first_edge_path_state;
    fbe_path_state_t                        second_edge_path_state;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_bool_t                              b_is_copy_complete;
    fbe_bool_t                              b_is_destination_healthy;

    /* Initialize the updated value to invalid.
     */
    *edge_index_to_swap_out_p = FBE_EDGE_INDEX_INVALID;

    /* If the copy is complete we may change which edge is swapped out.
     */
    b_is_copy_complete = fbe_virtual_drive_is_copy_complete(virtual_drive_p);
    b_is_destination_healthy = fbe_virtual_drive_swap_is_secondary_edge_healthy(virtual_drive_p);

    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);

    /* get the virtual drive configuration mode. */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* If the virtual drive is in pass-thru mode we will always swap out
     * the secondary edge.
     */
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            /* If both edges were enabled and now both edges are broken and
             * the virtual drive is in pass-thru mode, we should swap-out
             * the secondary edge.
             */
            *edge_index_to_swap_out_p = SECOND_EDGE_INDEX;
            return FBE_STATUS_OK;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* If both edges were enabled and now both edges are broken and
             * the virtual drive is in pass-thru mode, we should swap-out
             * the secondary edge.
             */
            *edge_index_to_swap_out_p = FIRST_EDGE_INDEX;
            return FBE_STATUS_OK;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* The code below will determine the swap-out edge based on
             * path state.
             */
            break;

        default:
            /* Cannot have an unsupported configuration mode.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unsupported mode: %d \n",
                                  __FUNCTION__, configuration_mode);

            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* At this point we know the virtual drive is in mirror mode
     */
    if ((first_edge_path_state == FBE_PATH_STATE_INVALID)  ||
        (second_edge_path_state == FBE_PATH_STATE_INVALID)    )
    {
        /* we do not swap-out an edge if one of the downstream edge is invalid. */
        return FBE_STATUS_OK;
    }

    /* If both edges are either disabled or broken select the primary edge.
     */
    if (((first_edge_path_state == FBE_PATH_STATE_DISABLED) ||
         (first_edge_path_state == FBE_PATH_STATE_BROKEN)       ) &&
        ((second_edge_path_state == FBE_PATH_STATE_DISABLED) ||
         (second_edge_path_state == FBE_PATH_STATE_BROKEN)      )     )
    {
        /* If both downstream edge path states are broken then swap-out the 
         * edge based on configuration mode.
         */
        if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)
        {
            /* If we are in mirror mode and both positions have failed
             * swap-out the primary position.  In most case the primary
             * was already unhealthy so don't continue to use it.
             */
            *edge_index_to_swap_out_p = FIRST_EDGE_INDEX;
        } 
        else
        {
            /* If we are in mirror mode and both positions have failed
             * swap-out the primary position.  In most case the primary
             * was already unhealthy so don't continue to use it.
             */
            *edge_index_to_swap_out_p = SECOND_EDGE_INDEX;
        }
    }
    else if ((first_edge_path_state == FBE_PATH_STATE_DISABLED) ||
             (first_edge_path_state == FBE_PATH_STATE_BROKEN)      )
    {
        /* If one of the downstream edge is in the disabled or broken state 
         * then swap-out the edge with broken edge.
         */
        *edge_index_to_swap_out_p = FIRST_EDGE_INDEX;
    }
    else if ((second_edge_path_state == FBE_PATH_STATE_DISABLED) ||
             (second_edge_path_state == FBE_PATH_STATE_BROKEN)      )
    {
        /* If one of the downstream edge is in the disabled or broken state 
         * then swap-out the edge with broken state. 
         */
        *edge_index_to_swap_out_p = SECOND_EDGE_INDEX;
    }
    else if ((first_edge_path_state == FBE_PATH_STATE_ENABLED)  &&
             (second_edge_path_state == FBE_PATH_STATE_ENABLED)    )
    {
        /* If both downstream edge path states are enabled then swap-out the 
         * edge based on configuration mode and if the drive is healthy.
         */
        if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)
        {
            /* If both edges are enabled and the copy is complete remove
             * the source edge.  Else if the destination drive is not healthy
             * remove the destination drive.
             */
            if (b_is_copy_complete == FBE_TRUE)
            {
                /* Copy is complete.  Swap out the source drive.
                 */
                *edge_index_to_swap_out_p = FIRST_EDGE_INDEX;
            }
            else if (b_is_destination_healthy == FBE_FALSE)
            {
                /* Copy not complete and the destination drive is not
                 * healthy.  Swap out the destination drive.
                 */
                *edge_index_to_swap_out_p = SECOND_EDGE_INDEX;
            }
        }
        else
        {
            /* If both edges are enabled and the copy is complete remove
             * the source edge.  Else if the destination drive is not healthy
             * remove the destination drive.
             */
            if (b_is_copy_complete == FBE_TRUE)
            {
                /* Copy is complete.  Swap out the source drive.
                 */
                *edge_index_to_swap_out_p = SECOND_EDGE_INDEX;
            }
            else if (b_is_destination_healthy == FBE_FALSE)
            {
                /* Copy not complete and the destination drive is not
                 * healthy.  Swap out the destination drive.
                 */
                *edge_index_to_swap_out_p = FIRST_EDGE_INDEX;
            }
        }
    } /* end if both edges are enabled */

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_get_swap_out_edge_index()
*******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_find_broken_edge()
 *****************************************************************************
 *
 * @brief   This function is used to get the edge index which is broken.  It
 *          is used for either a permanent spare request or a mirror swap-out
 *          edge request.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   broken_edge_index_p - Address of `broken' edge index to populate.
 *              If no broken edge is located `invalid edge index' is used.  
 * 
 * @return  fbe_status_t - status of the operation.
 *
 * @author
 *  08/30/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_swap_find_broken_edge(fbe_virtual_drive_t *virtual_drive_p,
                                                     fbe_edge_index_t *broken_edge_index_p)
{
    fbe_path_state_t                        first_edge_path_state;
    fbe_path_state_t                        second_edge_path_state;
    fbe_path_attr_t                         first_edge_path_attr;
    fbe_path_attr_t                         second_edge_path_attr;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_lba_t                               first_edge_rebuild_checkpoint = FBE_LBA_INVALID;
    fbe_lba_t                               second_edge_rebuild_checkpoint = FBE_LBA_INVALID;
    fbe_lba_t                               exported_disk_capacity;     //  exported disk capacity

    /* By default there is no broekn edge.
     */
    *broken_edge_index_p = FBE_EDGE_INDEX_INVALID;

    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                            &first_edge_path_attr);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                            &second_edge_path_attr); 

    /* get the virtual drive configuration mode. */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If both edges are broken use the configuration mode to determine which
     * edge is the secondary edge.
     */
    if ((first_edge_path_state == FBE_PATH_STATE_BROKEN)  &&
        (second_edge_path_state == FBE_PATH_STATE_BROKEN)    )
    {
        /* Both edges are in the broken state.  This implies (since in the 
         * permanent sparing case we can only have (1) broken edge since the other
         * is in the `invalid' state) that we were in the middle of a copy operation
         * when we failed.  We may have already changed to pass-thru so always
         * select the secondary edge.
         */
        if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)    ||
            (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)    )
        {
            *broken_edge_index_p = SECOND_EDGE_INDEX;
        }
        else if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)    ||
                 (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)    )
        {
            *broken_edge_index_p = FIRST_EDGE_INDEX;
        }
    }
    else if (first_edge_path_state == FBE_PATH_STATE_BROKEN)
    {
        /* Else if the first path state is broken, that is the edge to return.
         */
        *broken_edge_index_p = FIRST_EDGE_INDEX;
    }
    else if (second_edge_path_state == FBE_PATH_STATE_BROKEN)
    {
        /* Else if the second path state is broken that is the edge to return.
         */
        *broken_edge_index_p = SECOND_EDGE_INDEX;
    }
    else if ((first_edge_path_state == FBE_PATH_STATE_ENABLED)  &&
             (second_edge_path_state == FBE_PATH_STATE_ENABLED)    )
    {
        /* Else if both edges are enabled we must be in mirror mode.
         */
        fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *) virtual_drive_p, FIRST_EDGE_INDEX, &first_edge_rebuild_checkpoint);
        fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *) virtual_drive_p, SECOND_EDGE_INDEX, &second_edge_rebuild_checkpoint);

        //  Get the exported capacity of raid group object
        exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity((fbe_raid_group_t *)virtual_drive_p);

        /* if EOL set on both pick destination.*/
        if ((first_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE)  &&
            (second_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE)    )
        {
            /* if mirror is not fully rebuid and then take out the proactive spare edge. */
            if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE) ||
               (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE))
            {
                *broken_edge_index_p = SECOND_EDGE_INDEX;
            }
            else if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE) ||
                    (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE))
            {
                *broken_edge_index_p = FIRST_EDGE_INDEX;
            }
        }
        else if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE) &&
                 (second_edge_rebuild_checkpoint == FBE_LBA_INVALID)                        )
        {
            /* If mirror is fully rebuild then take out the source edge. */
            *broken_edge_index_p = FIRST_EDGE_INDEX;
        }
        else if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE) &&  
                 (first_edge_rebuild_checkpoint == FBE_LBA_INVALID)                          )
        {
            /* If mirror is fully rebuild then take out the source edge. */
            *broken_edge_index_p = SECOND_EDGE_INDEX;
        }
        else if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)
        {
            /* If both edges are enabled but we are in pass-thru, swap-out the 
             * secondary edge. */
            *broken_edge_index_p = SECOND_EDGE_INDEX;
        }
        else if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)
        {
            /* If both edges are enabled but we are in pass-thru, swap-out the 
             * secondary edge. */
            *broken_edge_index_p = FIRST_EDGE_INDEX;
        }
        if ((first_edge_rebuild_checkpoint == FBE_LBA_INVALID)  &&
            (second_edge_rebuild_checkpoint == FBE_LBA_INVALID)    )
        {
            /* if mirror is fully rebuild then take out the source edge (proactive candidate). */
            if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE) ||
               (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE))
            {
                *broken_edge_index_p = FIRST_EDGE_INDEX;
            }
            else if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE) ||
                    (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE))
            {
                *broken_edge_index_p = SECOND_EDGE_INDEX;
            }  
        }

    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_find_broken_edge()
*******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_is_primary_edge_healthy()
 *****************************************************************************
 *
 * @brief   Simply determines if the primary (i.e. position rebuilding from) 
 *          edge is healthy (i.e. does not have EOL set) or not.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * 
 * @return  bool - FBE_TRUE - The primary edge is healthy
 *                 FBE_FALSE - The primary edge is not healthy
 *
 * @author
 *  05/22/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_bool_t fbe_virtual_drive_swap_is_primary_edge_healthy(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_path_attr_t                         first_edge_path_attr;
    fbe_path_attr_t                         second_edge_path_attr;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_bool_t                              b_primary_edge_healthy = FBE_TRUE;

    /* Get the path attributes of both the downstream edges. 
     */
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                            &first_edge_path_attr);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                            &second_edge_path_attr);

    /* get the virtual drive configuration mode. */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            /* Primary is first.*/
            if ((first_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE) != 0)
            {
                b_primary_edge_healthy = FBE_FALSE;
            }
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* Primary is second. */
            if ((second_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE) != 0)
            {
                b_primary_edge_healthy = FBE_FALSE;
            }
            break;

        default:
            /* Unexpected.*/
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unsupported mode: %d \n",
                                  __FUNCTION__, configuration_mode);
            return FBE_FALSE;
    }

    /* Return the result.
     */
    return b_primary_edge_healthy;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_is_primary_edge_healthy()
*******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_is_primary_edge_broken()
 *****************************************************************************
 *
 * @brief   Simply determines if the primary (i.e. full built) edge is broken
 *          or not.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * 
 * @return  bool - FBE_TRUE - The primary edge is broken
 *
 * @author
 *  10/12/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_bool_t fbe_virtual_drive_swap_is_primary_edge_broken(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_path_state_t                        first_edge_path_state;
    fbe_path_state_t                        second_edge_path_state;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_bool_t                              b_primary_edge_broken = FBE_FALSE;


    /* Get the path state aof both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);

    /* get the virtual drive configuration mode. */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            /* Primary is first.*/
            if ((first_edge_path_state == FBE_PATH_STATE_DISABLED) ||
                (first_edge_path_state == FBE_PATH_STATE_BROKEN)   ||
                (first_edge_path_state == FBE_PATH_STATE_GONE)     ||
                (first_edge_path_state == FBE_PATH_STATE_INVALID)    )
            {
                b_primary_edge_broken = FBE_TRUE;
            }
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* Primary is second. */
            if ((second_edge_path_state == FBE_PATH_STATE_DISABLED) ||
                (second_edge_path_state == FBE_PATH_STATE_BROKEN)   ||
                (second_edge_path_state == FBE_PATH_STATE_GONE)     ||
                (second_edge_path_state == FBE_PATH_STATE_INVALID)     )
            {
                b_primary_edge_broken = FBE_TRUE;
            }
            break;

        default:
            b_primary_edge_broken = FBE_TRUE;
            /* Unexpected.*/
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unsupported mode: %d \n",
                                  __FUNCTION__, configuration_mode);
            return FBE_FALSE;
    }

    /* Return the result.
     */
    return b_primary_edge_broken;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_is_primary_edge_broken()
*******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_is_secondary_edge_broken()
 *****************************************************************************
 *
 * @brief   Simply determines if the secondary (i.e. position being rebuilt) 
 *          edge is broken or not.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * 
 * @return  bool - FBE_TRUE - The secondary edge is broken
 *
 * @author
 *  10/18/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_bool_t fbe_virtual_drive_swap_is_secondary_edge_broken(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_path_state_t                        first_edge_path_state;
    fbe_path_state_t                        second_edge_path_state;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_bool_t                              b_secondary_edge_broken = FBE_FALSE;

    /* Get the path state of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);

    /* get the virtual drive configuration mode. */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            /* Secondary is second.*/
            if ((second_edge_path_state == FBE_PATH_STATE_DISABLED) ||
                (second_edge_path_state == FBE_PATH_STATE_BROKEN)      )
            {
                b_secondary_edge_broken = FBE_TRUE;
            }
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* Secondary is first. */
            if ((first_edge_path_state == FBE_PATH_STATE_DISABLED) ||
                (first_edge_path_state == FBE_PATH_STATE_BROKEN)      )
            {
                b_secondary_edge_broken = FBE_TRUE;
            }
            break;

        default:
            /* Unexpected.*/
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unsupported mode: %d \n",
                                  __FUNCTION__, configuration_mode);
            return FBE_FALSE;
    }

    /* Return the result.
     */
    return b_secondary_edge_broken;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_is_secondary_edge_broken()
*******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_is_secondary_edge_healthy()
 *****************************************************************************
 *
 * @brief   Simply determines if the secondary (i.e. position being rebuilt) 
 *          edge is healthy (i.e. does not have EOL set) or not.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * 
 * @return  bool - FBE_TRUE - The secondary edge is healthy
 *                 FBE_FALSE - The secondary edge is not healthy
 *
 * @author
 *  06/05/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_bool_t fbe_virtual_drive_swap_is_secondary_edge_healthy(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_path_attr_t                         first_edge_path_attr;
    fbe_path_attr_t                         second_edge_path_attr;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_bool_t                              b_secondary_edge_healthy = FBE_TRUE;

    /* Get the path attributes of both the downstream edges. 
     */
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                            &first_edge_path_attr);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                            &second_edge_path_attr);

    /* get the virtual drive configuration mode. */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            /* Secondary is second.*/
            if ((second_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE) != 0)
            {
                b_secondary_edge_healthy = FBE_FALSE;
            }
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* Secondary is first. */
            if ((first_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE) != 0)
            {
                b_secondary_edge_healthy = FBE_FALSE;
            }
            break;

        default:
            /* Unexpected.*/
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unsupported mode: %d \n",
                                  __FUNCTION__, configuration_mode);
            return FBE_FALSE;
    }

    /* Return the result.
     */
    return b_secondary_edge_healthy;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_is_secondary_edge_healthy()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_validate_swap_out_request()
 ******************************************************************************
 * @brief
 *  This function is used to validate the swap-out request.

 * @note 
 *  This function should be called only from monitor context from the swap-out
 *  edge condition.
 *
 * @param virtual_drive_p                   - virtual dirve object.
 * @param virtual_drive_swap_out_validation_p - Pointer to swap-out request                  
 * 
 * @return fbe_status_t                     - status of the operation.
 *
 * @author
 *  11/28/2012  Ron Proulx  - Updated.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_validate_swap_out_request(fbe_virtual_drive_t *virtual_drive_p,
                                                              fbe_virtual_drive_swap_out_validation_t *virtual_drive_swap_out_validation_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_edge_index_t                        edge_index_to_swap_out = FBE_EDGE_INDEX_INVALID;
    fbe_path_state_t                        first_edge_path_state;
    fbe_path_state_t                        second_edge_path_state;
    fbe_path_attr_t                         first_edge_path_attr;
    fbe_path_attr_t                         second_edge_path_attr;

    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                            &first_edge_path_attr);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                            &second_edge_path_attr); 

    /* Validate the virtual drive configuration mode. 
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* The above are supported mode for a swap-out request.
             */
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN:
        default:
            /* Cannot swap-out if any other mode is set.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Unsupported config mode: %d \n",
                              __FUNCTION__, configuration_mode);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Invoke the method that determines the edge index to swap-out.
     */
    status = fbe_virtual_drive_swap_get_swap_out_edge_index(virtual_drive_p, &edge_index_to_swap_out);
    if (status != FBE_STATUS_OK)
    {
        /* Even if the swap-out index couldn't be determined the method shouldn't
         * fail.  Therefore report an error trace.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Find swap-out index failed - status: 0x%x\n",
                              __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the edge request to swap-out doesn't agree with the edge that needs 
     * swapping out, then something went wrong.
     */
    if (edge_index_to_swap_out != virtual_drive_swap_out_validation_p->swap_edge_index)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s mode: %d swap index: %d path state:[%d-%d] attr:[0x%x-0x%x] expected index: %d\n",
                              __FUNCTION__, configuration_mode, edge_index_to_swap_out,
                              first_edge_path_state, second_edge_path_state,
                              first_edge_path_attr, second_edge_path_attr,
                              virtual_drive_swap_out_validation_p->swap_edge_index);
        return FBE_STATUS_FAILED;
    }
    
    /* If we could not determine the swap-out index then don't continue.
     */
    if ((virtual_drive_swap_out_validation_p->swap_edge_index != FIRST_EDGE_INDEX)  &&
        (virtual_drive_swap_out_validation_p->swap_edge_index != SECOND_EDGE_INDEX)    )
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s mode: %d swap index: %d path state:[%d-%d] attr:[0x%x-0x%x] unexpected\n",
                              __FUNCTION__, configuration_mode, virtual_drive_swap_out_validation_p->swap_edge_index,
                              first_edge_path_state, second_edge_path_state,
                              first_edge_path_attr, second_edge_path_attr);
        return FBE_STATUS_FAILED;
    }

    /* Else success
     */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_validate_swap_out_request()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_send_swap_notification()
 ******************************************************************************
 * @brief
 * Helper function to send the notification for the VD object.
 * 
 * @param virtual_drive_p        
 * @param configuration_mode   
 * 
 * @return status                   - The status of the operation.
 *
 * @author
 *  02/14/2013  Ron Proulx  - Updated
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_send_swap_notification(fbe_virtual_drive_t* virtual_drive_p,
                                                 fbe_virtual_drive_configuration_mode_t configuration_mode)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_bool_t                  b_is_active;
    fbe_notification_info_t     notification_info; 
    fbe_object_id_t             vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             orig_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             spare_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_spare_swap_command_t    swap_command_type = FBE_SPARE_SWAP_INVALID_COMMAND;

    /* Determine if we are active or passive.
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)virtual_drive_p);

    /* Get the virtual drive object id and the command type.
     */
    fbe_base_object_get_object_id((fbe_base_object_t*)virtual_drive_p, &vd_object_id);
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &swap_command_type);

    /* We generate the swap notification based on configuraiton mode.
     */
    if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)
    {
        fbe_vitual_drive_get_server_object_id_for_edge(virtual_drive_p, FIRST_EDGE_INDEX,
                                                       &orig_pvd_object_id);

        fbe_vitual_drive_get_server_object_id_for_edge(virtual_drive_p, SECOND_EDGE_INDEX,
                                                       &spare_pvd_object_id);
    }

    else if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)
    {
        fbe_vitual_drive_get_server_object_id_for_edge(virtual_drive_p, SECOND_EDGE_INDEX,
                                                       &orig_pvd_object_id);

        fbe_vitual_drive_get_server_object_id_for_edge(virtual_drive_p, FIRST_EDGE_INDEX,
                                                       &spare_pvd_object_id);
    }

    else if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)
    {        
        fbe_vitual_drive_get_server_object_id_for_edge(virtual_drive_p, FIRST_EDGE_INDEX,
                                                       &spare_pvd_object_id);

        fbe_virtual_drive_get_orig_pvd_object_id(virtual_drive_p, &orig_pvd_object_id);
    }

    else if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)
    {        
        fbe_vitual_drive_get_server_object_id_for_edge(virtual_drive_p, SECOND_EDGE_INDEX,
                                                       &spare_pvd_object_id);

        fbe_virtual_drive_get_orig_pvd_object_id(virtual_drive_p, &orig_pvd_object_id);
    }

    else
    {
        /* Else unsupported configuration mode.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s unknown mode: %d cmd: %d orig: 0x%x spare: 0x%x \n",
                              __FUNCTION__, configuration_mode, swap_command_type, orig_pvd_object_id, spare_pvd_object_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    /* fill up the notification information before sending notification. */    
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_SWAP_INFO;
    notification_info.source_package = FBE_PACKAGE_ID_SEP_0;
    notification_info.class_id = FBE_CLASS_ID_VIRTUAL_DRIVE;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE;
    notification_info.notification_data.swap_info.orig_pvd_object_id = orig_pvd_object_id;
    notification_info.notification_data.swap_info.spare_pvd_object_id = spare_pvd_object_id;
    notification_info.notification_data.swap_info.command_type = swap_command_type;
    notification_info.notification_data.swap_info.vd_object_id = vd_object_id;

    /* Trace the notification.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "SWAP: send notification%s type: 0x%x cmd: %d orig: 0x%x spare: 0x%x \n",
                          (b_is_active) ? "" : " peer",
                          FBE_NOTIFICATION_TYPE_SWAP_INFO, swap_command_type, orig_pvd_object_id, spare_pvd_object_id);

    /* If the command type is not valid trace an error.
     */
    if ((swap_command_type == FBE_SPARE_SWAP_INVALID_COMMAND) ||
        (swap_command_type >= FBE_SPARE_SWAP_COMMAND_LAST)       )
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: send swap notification cmd: %d is invalid orig: 0x%x spare: 0x%x \n",
                              swap_command_type, orig_pvd_object_id, spare_pvd_object_id);
    }

    /* send the notification to registered callers. */
    status = fbe_notification_send(vd_object_id, notification_info);
   
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_send_swap_notification()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_vitual_drive_get_server_object_id_for_edge()
 ******************************************************************************
 * @brief
 *  This function is used to get the server object id of the edge.
 *
 *
 * @param object_id         - upstream object edge connected to 
 * @param edge_index        - index of edge we need to get the information about
 * @param object_id_p       - parameter to store the output server object id
 *
 * @return status - status of the operation
 *
 * @author
 *  03/15/2011 - Created. Ashwin Tamilarasan
 ******************************************************************************/
fbe_status_t
fbe_vitual_drive_get_server_object_id_for_edge(fbe_virtual_drive_t* virtual_drive_p,
                                               fbe_edge_index_t edge_index,
                                               fbe_object_id_t * object_id_p)
{   
    //fbe_block_transport_control_get_edge_info_t edge_info;
    fbe_status_t     status;
    fbe_block_edge_t *block_edge;

    fbe_base_config_get_block_edge((fbe_base_config_t *)virtual_drive_p, &block_edge, edge_index);
    status = fbe_block_transport_get_server_id(block_edge, object_id_p);

    return status;
}
/******************************************************************************
 * end fbe_vitual_drive_get_server_object_id_for_edge()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_ask_user_copy_permission()
 ******************************************************************************
 * @brief
 *  This function is used to allocate the event and send it to RAID object to
 *  ask for the user copy which virtual drive object want to perform.
 * 
 * @param virtual_drive_p - Virtual Drive object.
 * @param packet_p        - FBE packet pointer.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 * @author
 * 3/15/2011 - Created. Maya Dagon
 * 
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_ask_user_copy_permission(fbe_virtual_drive_t * virtual_drive_p,
                                                             fbe_packet_t * packet_p)
{
    fbe_event_t *           event_p = NULL;
    fbe_event_stack_t *     event_stack = NULL;
    fbe_lba_t               capacity;

    event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));
    if(event_p == NULL){ 
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_event_init(event_p);
    fbe_event_set_master_packet(event_p, packet_p);
    event_stack = fbe_event_allocate_stack(event_p);

    /* Fill LBA data. */
    fbe_base_config_get_capacity((fbe_base_config_t *)virtual_drive_p, &capacity);
    fbe_event_stack_set_extent(event_stack, 0, (fbe_block_count_t) capacity);

    /* Set the event completion function. */
    fbe_event_stack_set_completion_function(event_stack,
                                            fbe_virtual_drive_swap_ask_user_copy_permission_completion,
                                            virtual_drive_p);

    fbe_base_config_send_event((fbe_base_config_t *)virtual_drive_p, FBE_EVENT_TYPE_COPY_REQUEST, event_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_virtual_drive_ask_user_copy_permission()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_ask_user_copy_permission_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the event to ask for the
 *  permission to RAID object.
 * 
 * @param event_p   - Event pointer.
 * @param context_p - Context which was passed with event.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 * @author
 * 3/15/2011 - Created. Maya Dagon.
 * 
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_swap_ask_user_copy_permission_completion(fbe_event_t * event_p,
                                                            fbe_event_completion_context_t context)
{
    fbe_event_stack_t *                         event_stack = NULL;
    fbe_packet_t *                              packet_p = NULL;
    fbe_virtual_drive_t *                       virtual_drive_p = NULL;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_u32_t                                   event_flags = 0;
    fbe_event_status_t                          event_status = FBE_EVENT_STATUS_INVALID;
    fbe_payload_ex_t *                         payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_virtual_drive_swap_in_validation_t *    virtual_drive_swap_in_validation_p = NULL;    
   
    event_stack = fbe_event_get_current_stack(event_p);
    fbe_event_get_master_packet(event_p, &packet_p);

    virtual_drive_p = (fbe_virtual_drive_t *) context;

    /* Get flag and status of the event. */
    fbe_event_get_flags (event_p, &event_flags);
    fbe_event_get_status(event_p, &event_status);

    /* Release the event. */
    fbe_event_release_stack(event_p, event_stack);
    fbe_event_destroy(event_p);
    fbe_memory_ex_release(event_p);

    if (event_status == FBE_EVENT_STATUS_BUSY)
    {
        /* Our upstream client is busy.
         * we cannot start the proactive sparing operation now.
         */
        status = FBE_STATUS_BUSY;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet (packet_p);
        return status;
    }
    else if (event_status != FBE_EVENT_STATUS_OK)
    {
        fbe_base_object_trace( (fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s received bad status from client:%d , can't proceed with user copy.\n",__FUNCTION__, event_status);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet (packet_p);
        return status;
    }

    /* check deny flag  
     */
    if (event_flags & FBE_EVENT_FLAG_DENY)
    { 
        // change validation flag in the master packet
        payload_p = fbe_transport_get_payload_ex(packet_p);
        control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    
        fbe_payload_control_get_buffer(control_operation_p, &virtual_drive_swap_in_validation_p);
        if (virtual_drive_swap_in_validation_p == NULL)
        {
            fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        fbe_payload_control_get_buffer_length (control_operation_p, &length);
        if(length != sizeof(fbe_virtual_drive_swap_in_validation_t))
        {
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s payload buffer lenght mismatch.\n",
                                    __FUNCTION__);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* If the request is denied it means the parent raid group doesn't
         * support this swap request.
         */
        virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_UPSTREAM_DENIED_USER_COPY_REQUEST; 
        if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_USER_COPY_DENIED))
        {
            /* We have not reported the `denied' status log it now.
             */

            /* Set the condition to log the event.
             */
            fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                   (fbe_base_object_t *)virtual_drive_p,
                                   FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_REPORT_COPY_DENIED);

            /* Set the flag so that we don't report it again.
             */
            fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_USER_COPY_DENIED);
        }  
    }
    else
    {
        /* Clear the flag.
         */
        fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_USER_COPY_DENIED);
    }
   
    //complete packet 
    status = FBE_STATUS_OK;
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_ask_user_copy_permission_completion()
*******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_log_permanent_spare_denied()
 ***************************************************************************** 
 * 
 * @brief   The parent raid group has denied the permanent sparing request.
 *          Generate the proper event log to let the user know why the spare
 *          was not swapped in.
 * 
 * @param   virtual_drive_p - Virtual Drive object
 * @param   packet_p - Original packet pointer needed to log event.
 *
 * @return  status - FBE_STATUS_OK
 * 
 * @author
 *  05/23/2012  Ron Proulx  - Created.
 * 
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_swap_log_permanent_spare_denied(fbe_virtual_drive_t *virtual_drive_p,
                                                               fbe_packet_t *packet_p)
{
    fbe_object_id_t original_pvd_object_id;
    fbe_object_id_t spare_pvd_object_id;

    /* Generate the information required for the event.
     */
    fbe_virtual_drive_event_logging_get_original_pvd_object_id(virtual_drive_p,
                                                               &original_pvd_object_id,
                                                               FBE_FALSE /* Copy is not in progress */);
    fbe_virtual_drive_event_logging_get_spare_pvd_object_id(virtual_drive_p,
                                                            &spare_pvd_object_id,
                                                            FBE_FALSE /* Copy is not in progress */);

    /* Generate the event.
     */
    fbe_virtual_drive_write_event_log(virtual_drive_p,
                                      SEP_INFO_SPARE_PERMANENT_SPARE_REQUEST_DENIED,
                                      FBE_SPARE_INTERNAL_ERROR_RAID_GROUP_DENIED,
                                      original_pvd_object_id,
                                      spare_pvd_object_id,
                                      packet_p);

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/*********************************************************
 * end fbe_virtual_drive_swap_log_permanent_spare_denied()
 *********************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_log_proactive_spare_denied()
 ***************************************************************************** 
 * 
 * @brief   The parent raid group has denied the proactive sparing request.
 *          Generate the proper event log to let the user know why the spare
 *          was not swapped in.
 * 
 * @param   virtual_drive_p - Virtual Drive object
 * @param   packet_p - Original packet pointer needed to log event.
 *
 * @return  status - FBE_STATUS_OK
 * 
 * @author
 *  05/23/2012  Ron Proulx  - Created.
 * 
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_swap_log_proactive_spare_denied(fbe_virtual_drive_t *virtual_drive_p,
                                                               fbe_packet_t *packet_p)
{
    fbe_object_id_t original_pvd_object_id;
    fbe_object_id_t spare_pvd_object_id;

    /* Generate the information required for the event.
     */
    fbe_virtual_drive_event_logging_get_original_pvd_object_id(virtual_drive_p,
                                                               &original_pvd_object_id,
                                                               FBE_FALSE /* Copy is not in progress */);
    fbe_virtual_drive_event_logging_get_spare_pvd_object_id(virtual_drive_p,
                                                            &spare_pvd_object_id,
                                                            FBE_FALSE /* Copy is not in progress */);

    /* Generate the event.
     */
    fbe_virtual_drive_write_event_log(virtual_drive_p,
                                      SEP_INFO_SPARE_PROACTIVE_SPARE_REQUEST_DENIED,
                                      FBE_SPARE_INTERNAL_ERROR_RAID_GROUP_DENIED,
                                      original_pvd_object_id,
                                      spare_pvd_object_id,
                                      packet_p);

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/*********************************************************
 * end fbe_virtual_drive_swap_log_proactive_spare_denied()
 *********************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_log_user_copy_denied()
 ***************************************************************************** 
 * 
 * @brief   The parent raid group has denied the user copy request.
 *          Generate the proper event log to let the user know why the spare
 *          was not swapped in.
 * 
 * @param   virtual_drive_p - Virtual Drive object
 * @param   packet_p - Original packet pointer needed to log event.
 *
 * @return  status - FBE_STATUS_OK
 * 
 * @author
 *  05/23/2012  Ron Proulx  - Created.
 * 
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_swap_log_user_copy_denied(fbe_virtual_drive_t *virtual_drive_p,
                                                         fbe_packet_t *packet_p)
{
    fbe_object_id_t original_pvd_object_id;
    fbe_object_id_t spare_pvd_object_id = FBE_OBJECT_ID_INVALID;

    /* Generate the information required for the event.
     */
    fbe_virtual_drive_event_logging_get_original_pvd_object_id(virtual_drive_p,
                                                               &original_pvd_object_id,
                                                               FBE_FALSE /* Copy is not in progress */);

    /* Generate the event.
     */
    fbe_virtual_drive_write_event_log(virtual_drive_p,
                                      SEP_INFO_SPARE_USER_COPY_REQUEST_DENIED,
                                      FBE_SPARE_INTERNAL_ERROR_RAID_GROUP_DENIED,
                                      original_pvd_object_id,
                                      spare_pvd_object_id,
                                      packet_p);

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/*********************************************************
 * end fbe_virtual_drive_swap_log_user_copy_denied()
 *********************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_log_abort_copy_denied()
 ***************************************************************************** 
 * 
 * @brief   The parent raid group has denied the permanent sparing request.
 *          Generate the proper event log to let the user know why the spare
 *          was not swapped in.
 * 
 * @param   virtual_drive_p - Virtual Drive object
 * @param   packet_p - Original packet pointer needed to log event.
 *
 * @return  status - FBE_STATUS_OK
 * 
 * @author
 *  05/23/2012  Ron Proulx  - Created.
 * 
 *****************************************************************************/
static fbe_status_t fbe_virtual_drive_swap_log_abort_copy_denied(fbe_virtual_drive_t *virtual_drive_p,
                                                                 fbe_packet_t *packet_p)
{
    fbe_object_id_t original_pvd_object_id;
    fbe_object_id_t spare_pvd_object_id;

    /* Generate the information required for the event.
     */
    fbe_virtual_drive_event_logging_get_original_pvd_object_id(virtual_drive_p,
                                                               &original_pvd_object_id,
                                                               FBE_TRUE /* Copy is in progress */);
    fbe_virtual_drive_event_logging_get_spare_pvd_object_id(virtual_drive_p,
                                                            &spare_pvd_object_id,
                                                            FBE_TRUE /* Copy is in progress */);

    /* Generate the event.
     */
    fbe_virtual_drive_write_event_log(virtual_drive_p,
                                      SEP_ERROR_CODE_SWAP_ABORT_COPY_REQUEST_DENIED,
                                      FBE_SPARE_INTERNAL_ERROR_RAID_GROUP_DENIED,
                                      original_pvd_object_id,
                                      spare_pvd_object_id,
                                      packet_p);

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/*********************************************************
 * end fbe_virtual_drive_swap_log_abort_copy_denied()
 *********************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_save_original_pvd_object_id()
 ***************************************************************************** 
 * 
 * @brief   Save the original pvd object id so that when a new drive is swapped
 *          in we can report the provision drive object id of the original drive
 *          in the `swap' notification.  This is needed so that SEP clients can
 *          update any databases etc with which drive is being used by a virtual
 *          drive.
 * 
 * @param   virtual_drive_p - Virtual Drive object
 *
 * @return  status - FBE_STATUS_OK
 * 
 * @author
 *  08/28/2012  Ron Proulx  - Created.
 * 
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_swap_save_original_pvd_object_id(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_virtual_drive_configuration_mode_t  configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
    fbe_edge_index_t                        orig_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_object_id_t                         saved_orig_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         orig_pvd_object_id = FBE_OBJECT_ID_INVALID;

    /* Determine which edge is the primary edge based on configuration mode.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            orig_edge_index = FIRST_EDGE_INDEX;
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            orig_edge_index = SECOND_EDGE_INDEX;
            break;

        default:
            /* Unexpected.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unexpected configuation mode: %d \n",
                                  __FUNCTION__, configuration_mode);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the pvd object for the edge specified.
     */
    fbe_vitual_drive_get_server_object_id_for_edge(virtual_drive_p,
                                                   orig_edge_index,
                                                   &orig_pvd_object_id);

    /* Get the current original pvd object id.  Only if there is a change do
     * we save the new value.
     */
    fbe_virtual_drive_get_orig_pvd_object_id(virtual_drive_p, 
                                             &saved_orig_pvd_object_id);

    /* If the current value is not invalid and is not the same as the saved
     * update the saved value.
     */
    if ((orig_pvd_object_id != FBE_OBJECT_ID_INVALID)    &&
        (orig_pvd_object_id != saved_orig_pvd_object_id)    )
    {
        /* Trace an informational message.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: save orig pvd id: 0x%x edge: %d mode: %d \n",
                          orig_pvd_object_id, orig_edge_index, configuration_mode);

        /* Set the original pvd object id and return success.
         */
        fbe_virtual_drive_set_orig_pvd_object_id(virtual_drive_p, orig_pvd_object_id);
    }

    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_virtual_drive_swap_save_original_pvd_object_id()
 **********************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_set_degraded_needs_rebuild_if_needed()
 ***************************************************************************** 
 * 
 * @brief   Check and set (if required) the `degraded needs rebuild' path
 *          attribute on the upstream edge.  This attribute indicates that
 *          the source drive of a copy operation has failed prior to the
 *          virtual drive being completely rebuilt.  It indicates to the
 *          upstream raid group that it must rebuild the virtual drive position
 *          before proceeding.
 *   
 * @param   virtual_drive_p - pointer to the object.
 * @param   packet_p - Pointer to monitor packet (needed to change metadata)
 * @param   downstream_health_state - Downstream health (for debug)
 * 
 * @return  status - FBE_STATUS_OK - complete
 *                   FBE_STATUS_MORE_PROCESSING_REQUIRED - Updating non-paged
 *
 * @author
 *  04/13/2013  Ron Proulx  - Updated.
 *
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_swap_set_degraded_needs_rebuild_if_needed(fbe_virtual_drive_t *virtual_drive_p,
                                                                         fbe_packet_t *packet_p,
                                                                         fbe_base_config_downstream_health_state_t downstream_health_state)                                           
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
    fbe_lba_t                               first_edge_rebuild_checkpoint;
    fbe_lba_t                               second_edge_rebuild_checkpoint;
    fbe_bool_t                              b_set_degraded_needs_rebuild = FBE_FALSE;
    fbe_bool_t                              b_is_degraded_needs_rebuild = FBE_FALSE;

    /* Determine if the source drive has failed prior to the rebuild being
     * complete.  This can only be the case if we are in pass-thru mode with 
     * the source drive degraded. 
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_metadata_is_degraded_needs_rebuild(virtual_drive_p, &b_is_degraded_needs_rebuild);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, FIRST_EDGE_INDEX, &first_edge_rebuild_checkpoint);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, SECOND_EDGE_INDEX, &second_edge_rebuild_checkpoint);

    /* Check the configuration mode and checkpoint.
     */
    if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) 
    {
        /* If we are not fully rebuilt set `degraded needs rebuild' upstream
         * path attribute.
         */
        if (first_edge_rebuild_checkpoint != FBE_LBA_INVALID)
        {
            /* Flag the fact that the degraded needs rebuild attribute needs to
             * be set.
             */
            b_set_degraded_needs_rebuild = FBE_TRUE;
        }
    } 
    else if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) 
    {
        /* If we are not fully rebuilt set `degraded needs rebuild' upstream
         * path attribute.
         */
        if (second_edge_rebuild_checkpoint != FBE_LBA_INVALID)
        {
            /* Flag the fact that the degraded needs rebuild attribute needs to
             * be set.
             */
            b_set_degraded_needs_rebuild = FBE_TRUE;
        }
    }

    /* Check if we needs to set the degraded needs rebuild
     */
    if (b_set_degraded_needs_rebuild == FBE_TRUE)
    {
        /* Trace some information and set the `degraded needs rebuild' attribute
         * on the upstream edge.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: set degraded needs rebuild mode: %d degraded: %d health: %d checkpoint: 0x%llx \n",
                              configuration_mode, downstream_health_state, b_is_degraded_needs_rebuild,
                              (unsigned long long)first_edge_rebuild_checkpoint);

        /*! @note The `degraded needs rebuild' path attribute allows the 
         *        raid group to continue even though the virtual drive is
         *        technically broken (the source drive was removed prior to
         *        the copy operation being complete).
         */
        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(
                    &((fbe_base_config_t *)virtual_drive_p)->block_transport_server,
                    FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED_NEEDS_REBUILD,  /* Path attribute to set */
                    FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED_NEEDS_REBUILD   /* Mask of attributes to not set if already set*/);

        /* If the `degraded needs rebuild' non-paged flag is not set, set it now.
         */
        if (b_is_degraded_needs_rebuild == FBE_FALSE)
        {
            status = fbe_virtual_drive_metadata_set_degraded_needs_rebuild(virtual_drive_p, packet_p);
        }
    }
    else
    {
        /* Else clear the upstream path attribute `degraded needs rebuild'
         * (it will only clear it if it is set).
         */
        fbe_block_transport_server_clear_path_attr_all_servers(&((fbe_base_config_t *)virtual_drive_p)->block_transport_server,
                                                               FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED_NEEDS_REBUILD);
    }

    /* Return status
     */
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_set_degraded_needs_rebuild_if_needed()
*******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_ask_copy_abort_go_degraded_permission()
 *****************************************************************************
 *
 * @brief   This function is used to ask the upstream raid group if it is Ok
 *          to give up on the copy operation.  If we are allowed to give up
 *          we will swap-out the source drive without the destination drive
 *          being fully rebuilt which will result in this position being
 *          degraded.
 * 
 * @param   virtual_drive_p - Virtual Drive object.
 * @param   packet_p        - FBE packet pointer.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  09/17/2012  Ron Proulx  - Created.
 * 
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_swap_ask_copy_abort_go_degraded_permission(fbe_virtual_drive_t *virtual_drive_p,
                                                                          fbe_packet_t *packet_p)
{
    fbe_event_t        *event_p = NULL;
    fbe_event_stack_t  *event_stack = NULL;
    fbe_lba_t           capacity;

    event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));
    if (event_p == NULL)
    { /* Do not worry we will send it later */
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_event_init(event_p);
    fbe_event_set_master_packet(event_p, packet_p);
    event_stack = fbe_event_allocate_stack(event_p);

    /* Fill LBA data. */
    fbe_base_config_get_capacity((fbe_base_config_t *)virtual_drive_p, &capacity);
    fbe_event_stack_set_extent(event_stack, 0, (fbe_block_count_t) capacity);

    /* Set the event completion function. */
    fbe_event_stack_set_completion_function(event_stack,
                                            fbe_virtual_drive_swap_ask_copy_abort_go_degraded_permission_completion,
                                            virtual_drive_p);

    fbe_base_config_send_event((fbe_base_config_t *)virtual_drive_p, FBE_EVENT_TYPE_ABORT_COPY_REQUEST, event_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_ask_copy_abort_go_degraded_permission()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_swap_ask_copy_abort_go_degraded_permission_completion()
 ****************************************************************************** 
 * 
 * @brief   This function is used as completion function for the event to ask
 *          for permission (from the parent raid group) if it is Ok to swap-out
 *          the source drive which will result in the virtual drive being
 *          degraded.
 * 
 * @param   event_p   - Event pointer.
 * @param   context_p - Context which was passed with event.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 * 
 * @author
 *  09/17/2012  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_swap_ask_copy_abort_go_degraded_permission_completion(fbe_event_t *event_p,
                                                            fbe_event_completion_context_t context)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_event_stack_t      *event_stack = NULL;
    fbe_packet_t           *packet_p = NULL;
    fbe_virtual_drive_t    *virtual_drive_p = NULL;
    fbe_u32_t               event_flags = 0;
    fbe_event_status_t      event_status = FBE_EVENT_STATUS_INVALID;
    fbe_object_id_t         original_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t         spare_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_virtual_drive_configuration_mode_t configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
    fbe_edge_index_t        mirror_mode_edge_index_to_swap_out = FBE_EDGE_INDEX_INVALID;
    fbe_bool_t              b_source_drive_failed = FBE_FALSE; 
    fbe_bool_t              b_destination_drive_failed = FBE_FALSE; 

    event_stack = fbe_event_get_current_stack(event_p);
    fbe_event_get_master_packet(event_p, &packet_p);

    virtual_drive_p = (fbe_virtual_drive_t *) context;

    /* Get flag and status of the event. */
    fbe_event_get_flags (event_p, &event_flags);
    fbe_event_get_status(event_p, &event_status);

    /* Release the event. */
    fbe_event_release_stack(event_p, event_stack);
    fbe_event_destroy(event_p);
    fbe_memory_ex_release(event_p);

    /* Get the configuration mode.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Get the edge index to swap out.
     */
    status = fbe_virtual_drive_swap_get_swap_out_edge_index(virtual_drive_p, &mirror_mode_edge_index_to_swap_out);

    /* Determine if the source drive failed (need to handle the case where both 
     * the source and destination drive have failed).
     */
    b_source_drive_failed = fbe_virtual_drive_swap_is_primary_edge_broken(virtual_drive_p);

    /* Based on the configuration mode determine if the source, destination or both failed.
     */
    if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)
    {
        if (mirror_mode_edge_index_to_swap_out == FIRST_EDGE_INDEX)
        {
            b_source_drive_failed = FBE_TRUE;
        }
        else
        {
            b_destination_drive_failed = FBE_TRUE;
        }
    }
    else
    {
        /* Else mirror second edge.
         */
        if (mirror_mode_edge_index_to_swap_out == SECOND_EDGE_INDEX)
        {
            b_source_drive_failed = FBE_TRUE;
        }
        else
        {
            b_destination_drive_failed = FBE_TRUE;
        }
    }

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: ask copy abort go degraded event status: 0x%x flags: 0x%x\n",
                          event_status, event_flags);

    /* Clear the abort request in progress flag.
     */
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_REQUEST_ABORT_COPY);

    /* If we are busy return that status.
     */
    if (event_status == FBE_EVENT_STATUS_BUSY)
    {
        /* Our upstream client is busy, we will need to try again later, 
         * we cannot start the abort copy operation now.
         */
        status = FBE_STATUS_BUSY;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet (packet_p);
        return status;
    }
    else if (event_status != FBE_EVENT_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed. event status: 0x%x unexpected\n",
                              __FUNCTION__, event_status);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet (packet_p);
        return status;
    }

    /* Log the event indicating that the copy operation has completed. */
    fbe_virtual_drive_event_logging_get_original_pvd_object_id(virtual_drive_p,
                                                               &original_pvd_object_id,
                                                               FBE_TRUE /* Copy is in-progress */);
    fbe_virtual_drive_event_logging_get_spare_pvd_object_id(virtual_drive_p,
                                                            &spare_pvd_object_id,
                                                            FBE_TRUE /* Copy is in-progress */);

    /* If we are allowed continue with the swap-out request.
     */
    if (!(event_flags & FBE_EVENT_FLAG_DENY))
    {
        /* Clear the `denied' flag.
         */
        fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_ABORT_COPY_REQUEST_DENIED);

        /* Upstream raid group allows the swap-out.  Return success and proceed.
         */
        /* Log the correct event (source, destination, source and destination)
         */
        if (b_source_drive_failed == FBE_TRUE)
        {
            /* Either just the source or both the source and destination drives
             * failed.
             */
            status = fbe_virtual_drive_write_event_log(virtual_drive_p,
                                                       SEP_ERROR_CODE_COPY_SOURCE_DRIVE_REMOVED,
                                                       FBE_SPARE_INTERNAL_ERROR_NONE,
                                                       original_pvd_object_id,
                                                       spare_pvd_object_id,
                                                       packet_p);
        }
        else 
        {
            /* Else just the destination drive failed.
             */
            status = fbe_virtual_drive_write_event_log(virtual_drive_p,
                                                       SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_REMOVED,
                                                       FBE_SPARE_INTERNAL_ERROR_NONE,
                                                       original_pvd_object_id,
                                                       spare_pvd_object_id,
                                                       packet_p);
         }

        status = FBE_STATUS_OK;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet (packet_p);
    }
    else
    {
        /* If RAID sends negative response then complete the packet with error status.
         */
        if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_ABORT_COPY_REQUEST_DENIED))
        {
            /* Log the event.
             */
            fbe_virtual_drive_swap_log_abort_copy_denied(virtual_drive_p, packet_p);

            /* Set the flag.
             */
            fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_ABORT_COPY_REQUEST_DENIED);
        }
        status = FBE_STATUS_BUSY;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
    }

    return FBE_STATUS_OK;
}
/********************************************************************************
 * end fbe_virtual_drive_swap_ask_copy_abort_go_degraded_permission_completion()
*********************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_set_copy_complete_in_progress_if_allowed()
 ***************************************************************************** 
 * 
 * @brief   This function checks if the virtual drive is in the proper state to
 *          `complete' the copy operation.  This is used to `postpone' the
 *          configuration change and swap-out if the virtual drive (and associated
 *          raid group) are not in the proper state to allow a configuration change.
 *          If allowed, it set the `change in-progress' flag.
 * 
 * @param   virtual_drive_p - Pointer to virtual drive object
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 * 
 * @author
 *  11/27/2012  Ron Proulx - Created.
 * 
 *****************************************************************************/
fbe_bool_t fbe_virtual_drive_swap_set_copy_complete_in_progress_if_allowed(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_bool_t          b_is_complete_copy_allowed = FBE_TRUE;
    fbe_raid_group_t   *raid_group_p = (fbe_raid_group_t *)virtual_drive_p;
    fbe_raid_group_local_state_t local_state = raid_group_p->local_state;

    /* Take the virtual drive lock and check if there is a join in progress.
     */
    fbe_virtual_drive_lock(virtual_drive_p);

    /* If there is a join in progress deny the request.
     */
    if ((local_state & FBE_RAID_GROUP_LOCAL_STATE_JOIN_MASK) != 0)
    {
        b_is_complete_copy_allowed = FBE_FALSE;
    }
    else
    {
        /* Else set the flag that indicates there is a change request in
         * progress.
         */
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS);
    }

    /* Release the lock.
     */
    fbe_virtual_drive_unlock(virtual_drive_p);

    /* If the request was not allowed trace an informational message.
     */
    if (b_is_complete_copy_allowed == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: set copy complete not allowed - join in progress \n");
    }

    /* Return if the complete copy was allowed or not.
     */
    return b_is_complete_copy_allowed;
}
/***********************************************************************
 * end fbe_virtual_drive_swap_set_copy_complete_in_progress_if_allowed()
 ***********************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_set_abort_copy_in_progress_if_allowed()
 ***************************************************************************** 
 * 
 * @brief   This function checks if the virtual drive is in the proper state to
 *          `abort' the copy operation.  This is used to `postpone' the
 *          configuration change and swap-out if the virtual drive (and associated
 *          raid group) are not in the proper state to allow a configuration change.
 *          If allowed, it set the `change in-progress' flag.
 * 
 * @param   virtual_drive_p - Pointer to virtual drive object
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 * 
 * @author
 *  11/27/2012  Ron Proulx - Created.
 * 
 *****************************************************************************/
fbe_bool_t fbe_virtual_drive_swap_set_abort_copy_in_progress_if_allowed(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_bool_t          b_is_abort_copy_allowed = FBE_TRUE;
    fbe_raid_group_t   *raid_group_p = (fbe_raid_group_t *)virtual_drive_p;

    /* Take the virtual drive lock and check if there is a join in progress.
     */
    fbe_virtual_drive_lock(virtual_drive_p);

    /* If there is a join in progress deny the request.
     */
    if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_JOIN_REQUEST))
    {
        b_is_abort_copy_allowed = FBE_FALSE;
    }
    else
    {
        /* Else set the flag that indicates there is a change request in
         * progress.
         */
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS);
    }

    /* Release the lock.
     */
    fbe_virtual_drive_unlock(virtual_drive_p);

    /* If the request was not allowed trace an informational message.
     */
    if (b_is_abort_copy_allowed == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: set abort copy not allowed - join in progress \n");
    }

    /* Return if the complete copy was allowed or not.
     */
    return b_is_abort_copy_allowed;
}
/***********************************************************************
 * end fbe_virtual_drive_swap_set_abort_copy_in_progress_if_allowed()
 ***********************************************************************/

/*!****************************************************************************
 *  fbe_virtual_drive_swap_check_if_proactive_copy_can_start()
 ******************************************************************************
 * @brief
 *   This function determines whether proactive copy can start currently 
 *   or not, it waits for the hard coded timer and make sure that both sps see
 *   the EOL state.
 *   
 * @param virtual_drive_p                   - pointer to the object.
 * @param can_proactive_copy_start_b_p   - pointer to output that gets set 
 *                                            to FBE_TRUE if the proactive 
 *                                            sparing can start.
 * @return  fbe_status_t           
 *
 * @author
 *  02/11/2013  Ron Proulx  - Created. 
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_swap_check_if_proactive_copy_can_start(fbe_virtual_drive_t *virtual_drive_p,
                                                                         fbe_bool_t *can_proactive_copy_start_b_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u64_t       proactive_copy_trigger_time;
    fbe_u64_t       elapsed_seconds;
    fbe_time_t      need_proactive_copy_start_time;

    /* Initialize proactive copy can start as FALSE. 
     */
    *can_proactive_copy_start_b_p = FBE_FALSE;

    /* Get the `need proactive copy' start time.
     */
    status = fbe_virtual_drive_swap_get_need_proactive_copy_start_time(virtual_drive_p,
                                                                       &need_proactive_copy_start_time);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Get need proactive copy start time failed - status: 0x%x \n",
                              __FUNCTION__, status);
        return status;
    }

    /* get the proactive copy trigger time and compare with elapsed seconds. */
    proactive_copy_trigger_time = FBE_VIRTUAL_DRIVE_RETRY_PROACTIVE_COPY_TRIGGER_SECONDS;

    /* get the elapsed time betweeen this check and previous check. */
    elapsed_seconds = fbe_get_elapsed_seconds(need_proactive_copy_start_time);

    /* if elapsed time is less than proactive copy trigger time then wait until elapsed
     * time becomes greater than permanent spare trigger time.
     */
    if (elapsed_seconds < proactive_copy_trigger_time)
    {
        return FBE_STATUS_OK;
    }

    /*! @note Cannot fail `can start' request due to `no spare reported'
     *        since must allow the spare request to proceed.
     */

    /* set the proactive copy can start as true if all the condition met. */
    *can_proactive_copy_start_b_p = FBE_TRUE;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_check_if_proactive_copy_can_start()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_virtual_drive_can_we_initiate_proactive_copy_request()
 ******************************************************************************
 * @brief
 *   This function determines if we can allow a proactive copy request to
 *   proceed.  The use cases are:
 *      o We are the active in pass-thru mode
 *   
 * @param virtual_drive_p   - pointer to the object.
 * @param b_can_initiate_p  - pointer to output that gets set 
 *                                            to FBE_TRUE if the proactive 
 *                                            copy is needed.
 * @return  fbe_status_t           
 *
 * @author
 *  02/11/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_can_we_initiate_proactive_copy_request(fbe_virtual_drive_t *virtual_drive_p,
                                                                      fbe_bool_t *b_can_initiate_p)
{
    fbe_bool_t              b_is_active = FBE_TRUE; /*! @note By default we can initiate the swap-out. */
    fbe_lifecycle_state_t   my_state = FBE_LIFECYCLE_STATE_INVALID;    
    fbe_lifecycle_state_t   peer_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;  
    fbe_path_state_t        first_edge_path_state;
    fbe_path_state_t        second_edge_path_state; 

    /* By default we allow the swap-out to proceed.
     */
    *b_can_initiate_p = FBE_TRUE;

    /* Determine our configuration mode.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t *)virtual_drive_p, &peer_state);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* determine if we are passive or active? */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)virtual_drive_p);
    if (!b_is_active)
    {
        /* If we are passive we cannot initiate.
         */
        *b_can_initiate_p = FBE_FALSE; 
        return FBE_STATUS_OK;                              
    } 
   
    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);

    /* If our path state is not enabled we cannot start
     */
    switch (configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            if (first_edge_path_state != FBE_PATH_STATE_ENABLED)
            {
                *b_can_initiate_p = FBE_FALSE;
            }
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            if (second_edge_path_state != FBE_PATH_STATE_ENABLED)
            {
                *b_can_initiate_p = FBE_FALSE;
            }
            break;
        default:
            /* Cannot allow proactive copy if we are in mirror mode
             */
            *b_can_initiate_p = FBE_FALSE;
            break;
    }

    /* If we denied the the swap-out request trace some information.
     */
    if (*b_can_initiate_p == FBE_FALSE)            
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: can initiate proactive mode: %d life: %d peer: %d path state:[%d-%d] denied.\n",
                              configuration_mode, my_state, peer_state, first_edge_path_state, second_edge_path_state);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_can_we_initiate_proactive_copy_request()
*******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_set_swap_in_command()
 ***************************************************************************** 
 * 
 * @brief   Using the new and current virtual drive configuration determine the
 *          most likely swap-in command.
 *
 * @param   virtual_drive_p - Pointer to the virtual drive 
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  03/01/2013  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_set_swap_in_command(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_bool_t                              b_is_active = FBE_TRUE;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_virtual_drive_configuration_mode_t  new_configuration_mode;
    fbe_path_attr_t                         first_edge_path_attr;
    fbe_path_attr_t                         second_edge_path_attr;
    fbe_spare_swap_command_t                swap_command_type = FBE_SPARE_SWAP_INVALID_COMMAND;

    /* Get the configuration mode.
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)virtual_drive_p);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_get_new_configuration_mode(virtual_drive_p, &new_configuration_mode);
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &swap_command_type);

    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                            &first_edge_path_attr);
    fbe_block_transport_get_path_attributes(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                            &second_edge_path_attr);

    /* A mode of `unknown' does not change anything. Should only need to set
     * the swap command on the passive.
     */
    if ((new_configuration_mode != FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN) &&
        (b_is_active == FBE_FALSE)                                           )
    {
        /* If we are changing from pass-thru to mirror.
         */
        if (fbe_virtual_drive_is_pass_thru_configuration_mode_set(virtual_drive_p))
        {
            /* Check if EOL is set to determine the command type.
             */
            if (new_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)
            {
                /* If EOL is set then the command is proactive copy.
                 */
                 if (first_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE)
                 {
                     if (swap_command_type != FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND)
                     {
                         fbe_virtual_drive_set_copy_request_type(virtual_drive_p, FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND);
                     }
                 }
                 else if ((swap_command_type != FBE_SPARE_INITIATE_USER_COPY_COMMAND)    &&
                          (swap_command_type != FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND)    )
                 {
                      fbe_virtual_drive_set_copy_request_type(virtual_drive_p, FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND);
                 }
            }
            else if (new_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)
            {
                /* If EOL is set then the command is proactive copy.
                 */
                 if (second_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE)
                 {
                     if (swap_command_type != FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND)
                     {
                         fbe_virtual_drive_set_copy_request_type(virtual_drive_p, FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND);
                     }
                 }
                 else if ((swap_command_type != FBE_SPARE_INITIATE_USER_COPY_COMMAND)    &&
                          (swap_command_type != FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND)    )
                 {
                      fbe_virtual_drive_set_copy_request_type(virtual_drive_p, FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND);
                 }
            }
        }
    }

    /* If we never set the copy command report an error
     */
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &swap_command_type);
    if (swap_command_type == FBE_SPARE_SWAP_INVALID_COMMAND)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: set swap-in cmd invalid. mode: %d new mode: %dpath attr:[0x%x-0x%x] \n",
                              configuration_mode, new_configuration_mode, first_edge_path_attr, first_edge_path_attr);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_virtual_drive_set_swap_in_command()
***********************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_set_swap_out_command()
 ***************************************************************************** 
 * 
 * @brief   Using the new and current virtual drive configuration determine the
 *          most likely swap-out command.
 *
 * @param base_config_p         - Pointer to the base config object.
 * @param packet_p              - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/01/2013  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_set_swap_out_command(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_bool_t                              b_is_active = FBE_TRUE;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_virtual_drive_configuration_mode_t  new_configuration_mode;
    fbe_path_state_t                        first_edge_path_state;
    fbe_path_state_t                        second_edge_path_state;
    fbe_spare_swap_command_t                swap_command_type = FBE_SPARE_SWAP_INVALID_COMMAND;

    /* Get the configuration mode.
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)virtual_drive_p);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_get_new_configuration_mode(virtual_drive_p, &new_configuration_mode);
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &swap_command_type);

    /* Get the path state and path attributes of both the downstream edges. */
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[FIRST_EDGE_INDEX],
                                       &first_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[SECOND_EDGE_INDEX],
                                       &second_edge_path_state);

    /* A mode of `unknown' does not change anything.  Only set the command
     * if we are not active.
     */
    if (fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p) &&
        (b_is_active == FBE_FALSE)                                             )
    {
        /* Else we are changing from mirror back to pass-thru.
         */
        if (new_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)
        {
            /* If were in mirror mode second edge and the first edge
             * is enabled then we are successfully completing a copy operation.
             */
            if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE) &&
                (first_edge_path_state == FBE_PATH_STATE_ENABLED)                           )

            {
                if ((swap_command_type != FBE_SPARE_COMPLETE_COPY_COMMAND) &&
                    (swap_command_type != FBE_SPARE_ABORT_COPY_COMMAND)       )
                {
                    fbe_virtual_drive_set_copy_request_type(virtual_drive_p, FBE_SPARE_COMPLETE_COPY_COMMAND);
                }
            }
            else if ((swap_command_type != FBE_SPARE_COMPLETE_COPY_COMMAND) &&
                     (swap_command_type != FBE_SPARE_ABORT_COPY_COMMAND)       )
            {
                fbe_virtual_drive_set_copy_request_type(virtual_drive_p, FBE_SPARE_ABORT_COPY_COMMAND);
            }
        }
        else if (new_configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)
        {
            /* If were in mirror mode first edge and the second edge
             * is enabled then we are successfully completing a copy operation.
             */
            if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE) &&
                (second_edge_path_state == FBE_PATH_STATE_ENABLED)                         )

            {
                if ((swap_command_type != FBE_SPARE_COMPLETE_COPY_COMMAND) &&
                    (swap_command_type != FBE_SPARE_ABORT_COPY_COMMAND)       )
                {
                    fbe_virtual_drive_set_copy_request_type(virtual_drive_p, FBE_SPARE_COMPLETE_COPY_COMMAND);
                }
            }
            else if ((swap_command_type != FBE_SPARE_COMPLETE_COPY_COMMAND) &&
                     (swap_command_type != FBE_SPARE_ABORT_COPY_COMMAND)       )
            {
                fbe_virtual_drive_set_copy_request_type(virtual_drive_p, FBE_SPARE_ABORT_COPY_COMMAND);
            }
        }
    }

    /* If we never set the copy command report an error
     */
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &swap_command_type);
    if (swap_command_type == FBE_SPARE_SWAP_INVALID_COMMAND)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: set swap-out cmd invalid. mode: %d new mode: %d path state:[%d-%d] \n",
                              configuration_mode, new_configuration_mode, first_edge_path_state, second_edge_path_state);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_virtual_drive_set_swap_out_command()
***********************************************/

/*****************************************************************************
 *          fbe_virtual_drive_handle_config_change()
 ***************************************************************************** 
 * 
 * @brief   The job service has requested that we change from mirror mode to
 *          pass-thru mode.  The following events have occurred:
 *              1a: + I/O is quiesced (could have items on terminator queue)
 *          This method needs to perform the following:
 *              1b: + Mark the I/Os `shutdown' so they are returned from library
 *                  + Return the I/Os upstream with a status of:
 *                      o FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED
 *                      o FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE
 *                  + Change to pass-thru mode
 *          After these I/O are no longer on the terminator queue:
 *              1c: + Unquiesce I/Os (with virtual drive now in pass-thru mode with
 *                    (2) edges)
 *
 * @param   virtual_drive_p - Pointer to virtual drive which is a super-class of
 *              the raid group class.
 *
 * @return  status
 *
 * @author
 *  04/18/2013  Ron Proulx  - Created from fbe_raid_group_handle_shutdown.
 *
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_handle_config_change(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_group_t       *raid_group_p = (fbe_raid_group_t *)virtual_drive_p;
    fbe_queue_head_t       *termination_queue_p = &raid_group_p->base_config.base_object.terminator_queue_head;
    fbe_base_object_t      *base_object_p = &raid_group_p->base_config.base_object;
    fbe_packet_t           *current_packet_p = NULL;
    fbe_raid_iots_t        *iots_p = NULL;
    fbe_queue_head_t        restart_queue;
    fbe_queue_element_t    *next_element_p = NULL;
    fbe_queue_element_t    *queue_element_p = NULL;
    fbe_u32_t               restart_siots_count = 0;
    fbe_bool_t              b_done = FBE_TRUE;

    /* Must be done under lock
     */
    fbe_virtual_drive_lock(virtual_drive_p);

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s fl: 0x%x\n", 
                          __FUNCTION__, raid_group_p->flags);

    fbe_spinlock_lock(&raid_group_p->base_config.base_object.terminator_queue_lock);

    /*! @note Do not unquiesce since that would allow new I/O
     */
    //fbe_base_config_clear_clustered_flag((fbe_base_config_t *) raid_group_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED);

    /* Loop over the termination queue and generate a list of siots that need to be
     * restarted. 
     */
    fbe_queue_init(&restart_queue);

    queue_element_p = fbe_queue_front(termination_queue_p);

    while (queue_element_p != NULL)
    {
        fbe_payload_ex_t *sep_payload_p = NULL;
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload_p = fbe_transport_get_payload_ex(current_packet_p);

        next_element_p = fbe_queue_next(termination_queue_p, &current_packet_p->queue_element);

        fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

        /* Add the siots for this iots that need to be restarted. 
         * We will kick them off below once we have finished iterating over 
         * the termination queue. 
         */
        if ((iots_p->status == FBE_RAID_IOTS_STATUS_NOT_STARTED_TO_LIBRARY) ||
            (iots_p->status == FBE_RAID_IOTS_STATUS_WAITING_FOR_QUIESCE))
        {
            /* Not started to the library yet.
             * Set this up so it can get completed through our standard completion path. 
             */
            if (iots_p->callback != NULL) 
            { 
                fbe_base_object_trace(base_object_p, 
                                      FBE_TRACE_LEVEL_ERROR, 
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s iots: %p callback is not null (0x%p)\n", 
                                      __FUNCTION__, iots_p, iots_p->callback);
            }
            if (iots_p->callback_context == NULL)
            {
                fbe_base_object_trace(base_object_p, 
                                      FBE_TRACE_LEVEL_ERROR, 
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s iots: %p callback_context is null\n", 
                                      __FUNCTION__, iots_p);
            }
            if (iots_p->common.state == NULL) 
            {
                fbe_base_object_trace(base_object_p, 
                                      FBE_TRACE_LEVEL_ERROR, 
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s iots: %p state is null\n", 
                                      __FUNCTION__, iots_p);
            }
            fbe_raid_iots_mark_complete(iots_p);
            fbe_raid_iots_set_callback(iots_p, fbe_raid_group_iots_cleanup, raid_group_p);
            fbe_raid_common_wait_enqueue_tail(&restart_queue, &iots_p->common);
            /* We set retry possible since the client can retry. 
             */
            fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
            fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
        }
        else if(iots_p->status == FBE_RAID_IOTS_STATUS_NOT_USED)
        {
            /* If iots is not used then skip this terminated packet, eventually
             * packet will complete and it will handle the failure. 
             */
            queue_element_p = next_element_p;
            continue;
        }
        else
        {
            /* This iots was started to the library.
             */
            status = fbe_raid_iots_handle_config_change(iots_p, &restart_queue);

            /* Since we don't go thru iots finished we must decrement the queisce
             * count here.
             */
            if ((iots_p->status == FBE_RAID_IOTS_STATUS_COMPLETE)                  &&
                fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_WAS_QUIESCED)    )
            {
                fbe_raid_group_dec_quiesced_count(raid_group_p);

                /* Trace some information if enabled.
                 */
                fbe_virtual_drive_trace(virtual_drive_p,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_VIRTUAL_DRIVE_DEBUG_FLAG_QUIESCE_TRACING,
                                        "virtual_drive: config change was quiesced iots:0x%x lba/bl: 0x%llx/0x%llx flags:0x%x cnt:%d\n",
                                        (fbe_u32_t)iots_p,
                                        (unsigned long long)iots_p->lba,
                                        (unsigned long long)iots_p->blocks,
                                        iots_p->flags, raid_group_p->quiesced_count);

                /* Clear the flag.
                 */
                fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_WAS_QUIESCED);
            }
        }
        /* If we found anything on the termination queue then we are not done.
         */
        b_done = FBE_FALSE;
        queue_element_p = next_element_p;
    }

    fbe_spinlock_unlock(&raid_group_p->base_config.base_object.terminator_queue_lock);
    fbe_virtual_drive_unlock(virtual_drive_p);

    /* Restart all the items we found.
     */
    restart_siots_count = fbe_raid_restart_common_queue(&restart_queue);

    /* Trace out the number of siots we kicked off.
     */
    if (restart_siots_count)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s %d siots restarted for config change\n", 
                              __FUNCTION__, restart_siots_count);
    }
    return (b_done) ? FBE_STATUS_OK : FBE_STATUS_PENDING;
}
/******************************************
 * fbe_virtual_drive_handle_config_change()
 ******************************************/


/**********************************
 * end fbe_virtual_drive_swap.c
 **********************************/
