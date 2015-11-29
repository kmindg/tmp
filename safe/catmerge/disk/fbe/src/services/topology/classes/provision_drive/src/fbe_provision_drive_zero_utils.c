/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_zero_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the code which provides generic function interface
 *  for the zeroing functionality.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   07/16/2010:  Created. Dhaval Patel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_provision_drive.h"
#include "base_object_private.h"
#include "fbe_database.h"
#include "fbe_provision_drive_private.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"


/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/

static fbe_status_t fbe_provision_drive_zero_utils_ask_zero_permission_completion(fbe_event_t * event_p,
                                                                                  fbe_event_completion_context_t context);

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

/*!****************************************************************************
 * fbe_provision_drive_zero_utils_can_zeroing_start()
 ******************************************************************************
 * @brief
 *  This function is used to determine whether we can start the background
 *  zeroing operation.
 *
 * @param provision_drive_p     - pointer to the provision drive object.
 * @param can_zeroing_start_p   - retuns whether it is ok to start zero operation.
 *
 * @return  fbe_status_t           
 *
 * @author
 *   07/19/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_zero_utils_can_zeroing_start(fbe_provision_drive_t * provision_drive_p,
                                                 fbe_bool_t * can_zeroing_start_p)
{

    fbe_bool_t  is_load_okay_b = FBE_FALSE;
    fbe_base_config_downstream_health_state_t   downstream_health_state;

    /* initialiez can zero start as false. */
    *can_zeroing_start_p = FBE_FALSE;

    /* can we perform the zero operation with current downstream health? */
    downstream_health_state = fbe_provision_drive_verify_downstream_health(provision_drive_p);
    if(downstream_health_state != FBE_DOWNSTREAM_HEALTH_OPTIMAL) {
         return FBE_STATUS_OK;
    }

    /*  See if we are allowed to do a zero I/O based on the current load. */
    fbe_provision_drive_zero_utils_check_permission_based_on_current_load(provision_drive_p, &is_load_okay_b);
    if (is_load_okay_b == FBE_FALSE) {
         return FBE_STATUS_OK;
    }

    /* all the checks succeed, we can start zeroing. */
    *can_zeroing_start_p = FBE_TRUE;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_utils_can_zeroing_start()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_zero_utils_ask_zero_permision()
 ******************************************************************************
 * @brief
 *  This function is used to allocate the event and send it to upstream object
 *  to get the permission for the zeroing operation.
 * 
 * @param virtual_drive_p - Virtual Drive object.
 * @param packet_p        - FBE packet pointer.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 * @author
 * 10/19/2009 - Created. Peter Puhov.
 * 
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_zero_utils_ask_zero_permision(fbe_provision_drive_t * provision_drive_p,
                                                  fbe_lba_t lba,
                                                  fbe_block_count_t block_count,
                                                  fbe_packet_t * packet_p)
{
    fbe_event_t *               event_p = NULL;
    fbe_event_stack_t *         event_stack = NULL;
    fbe_event_permit_request_t  permit_request = {0};
    fbe_medic_action_priority_t medic_action_priority;

    //event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));
	event_p = &provision_drive_p->permision_event;
    if(event_p == NULL)
    { 
        /* Do not worry we will send it later */
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                        "%s event allocatil failed", __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize the event. */
    fbe_event_init(event_p);
    fbe_event_set_master_packet(event_p, packet_p);

    /* setup the zero permit request event. */
    permit_request.permit_event_type = FBE_PERMIT_EVENT_TYPE_ZERO_REQUEST;
    fbe_event_set_permit_request_data(event_p, &permit_request);

    /* allocate the event stack. */
    event_stack = fbe_event_allocate_stack(event_p);

    /* fill the LBA range to request for the zero operation. */
    fbe_event_stack_set_extent(event_stack, lba, block_count);

    /* Set medic action priority. */
    fbe_medic_get_action_priority(FBE_MEDIC_ACTION_ZERO, &medic_action_priority);
    fbe_event_set_medic_action_priority(event_p, medic_action_priority);

    /* set the event completion function. */
    fbe_event_stack_set_completion_function(event_stack,
                                            fbe_provision_drive_zero_utils_ask_zero_permission_completion,
                                            provision_drive_p);

    /* send the zero permit request to upstream object. */
    fbe_base_config_send_event((fbe_base_config_t *)provision_drive_p, FBE_EVENT_TYPE_PERMIT_REQUEST, event_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_utils_ask_zero_permision()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_zero_utils_ask_zero_permission_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the event to ask for the 
 *  zeroing request permission to upstream object.
 * 
 * @param event_p   - Event pointer.
 * @param context_p - Context which was passed with event.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 * @author
 * 10/19/2009 - Created. Peter Puhov.
 * 
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_zero_utils_ask_zero_permission_completion(fbe_event_t * event_p,
                                                              fbe_event_completion_context_t context)
{
    fbe_event_stack_t *         event_stack_p = NULL;
    fbe_packet_t *              packet_p = NULL;
    fbe_provision_drive_t *     provision_drive_p = NULL;
    fbe_object_id_t             object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                  b_is_system_drive = FBE_FALSE;
    fbe_u32_t                   event_flags = 0;
    fbe_event_status_t          event_status = FBE_EVENT_STATUS_INVALID;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_lba_t                   zero_checkpoint = FBE_LBA_INVALID;
    fbe_block_count_t           chunk_size = 0;
    fbe_lba_t                   default_offset = FBE_LBA_INVALID;
    fbe_scheduler_hook_status_t hook_status;

    /* Get the provision drive object pointer and object id.
     */
    provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    b_is_system_drive = fbe_database_is_object_system_pvd(object_id);
    fbe_base_config_get_default_offset((fbe_base_config_t *)provision_drive_p, &default_offset);

    /* Get flag and status of the event. */
    event_stack_p = fbe_event_get_current_stack(event_p);
    fbe_event_get_master_packet(event_p, &packet_p);
    fbe_event_get_flags (event_p, &event_flags);
    fbe_event_get_status(event_p, &event_status);

    /*! @note Get the zero checkpoint from the ask permission request.  Do not 
     *        use the provision drive zero checkpoint since it may have been
     *        modified while we were waiting for permission.
     */
    zero_checkpoint = event_stack_p->lba;
    chunk_size = event_stack_p->block_count;

    /* Release the event.  */
    fbe_event_release_stack(event_p, event_stack_p);
    fbe_event_destroy(event_p);
    //fbe_memory_ex_release(event_p);

    /*! @note We have already set the completion to:
     *        fbe_provision_drive_run_background_zeroing_completion()
     *        which simply re-schedules the run background zeroing.
     */

    /* First check the event status.
     */
    if(event_status == FBE_EVENT_STATUS_BUSY)
    {
        /* Our upstream client is busy, we will need to try again later. */
        fbe_transport_set_status(packet_p, FBE_STATUS_BUSY, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_BUSY;
    }
    else if((event_status != FBE_EVENT_STATUS_OK) && 
            (event_status != FBE_EVENT_STATUS_NO_USER_DATA))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s received bad status(0x%x) from upstream object, can't proceed with zero request\n",
                              __FUNCTION__, event_status);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Trace under debug.
     */
    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_INFO, 
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                    "BG_ZERO: permit status: %d flags: 0x%x checkpoint: 0x%llx offset: 0x%llx \n",
                                    event_status, event_flags, zero_checkpoint, default_offset);

    /* Send the zero request to executor if event flag is not set to deny. 
     */
    if (event_flags & FBE_EVENT_FLAG_DENY)
    {
        fbe_provision_drive_check_hook(provision_drive_p,  
                                       SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO, 
                                       FBE_PROVISION_DRIVE_SUBSTATE_ZERO_NO_PERMISSION, 0,  &hook_status);
        /* No hook status is handled since we can only log here.
         */

        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s received deny status from upstream object, event_flags:0x%x\n",
                              __FUNCTION__, event_flags);

        /* If upstream object deny the zeroing request the complete the packet with error response. */
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
    }
    else if (event_status == FBE_EVENT_STATUS_NO_USER_DATA)
    {
        /* Else if the current background zero checkpoint is below the default
         * offset (this should only occur for the system drive) and there
         * there is no user data, advance the background zero checkpoint.
         */ 
        if (zero_checkpoint < default_offset)
        {
            /* System drives can have areas that are below the default_offset.
             */
            if (b_is_system_drive == FBE_TRUE)
            {
                /* Trace a debug message, set the completion function (done above) and 
                 * invoke method to update the zero checkpoint to the next chunk.
                 */
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                                                FBE_TRACE_MESSAGE_ID_INFO, 
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                                "BG_ZERO: zero checkpoint: 0x%llx below default_offset: 0x%llx occurred.\n",
                                                (unsigned long long)zero_checkpoint,
						(unsigned long long)default_offset);
    
                /* System drive area is below default_offset and not consumed.
                 * Advance zero checkpoint to next chunk.
                 */
                zero_checkpoint += FBE_PROVISION_DRIVE_CHUNK_SIZE;
                status = fbe_provision_drive_metadata_set_bz_chkpt_without_persist(provision_drive_p, packet_p, zero_checkpoint); 
            }
            else
            {
                /* Else the zero checkpoint should never be set below the 
                 * default_offset.  Log an error and fail the request.
                 */
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "BG_ZERO: zero checkpoint: 0x%llx below default_offset: 0x%llx unexpected.\n",
                                      (unsigned long long)zero_checkpoint,
				      (unsigned long long)default_offset);
                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else
        {
            fbe_provision_drive_check_hook(provision_drive_p,  
                                       SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO, 
                                       FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND, 0,  &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE)
            {
                fbe_transport_set_status( packet_p, FBE_STATUS_OK, 0 );
                fbe_transport_complete_packet( packet_p );
                return FBE_STATUS_OK;
            }

	    if (!provision_drive_p->bg_zeroing_log_status) {
		/* log disk zero start event */
                fbe_provision_drive_event_log_disk_zero_start_or_complete(provision_drive_p, FBE_TRUE, packet_p);
            }
            
            /* Else there is no upstream object and we are above the default_offset.
             * Issue the zero request.
             */
            status = fbe_provision_drive_send_monitor_packet(provision_drive_p,
                                                             packet_p,
                                                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_BACKGROUND_ZERO,
                                                             zero_checkpoint,
                                                             FBE_PROVISION_DRIVE_CHUNK_SIZE);
        }
    }
    else
    {
        fbe_provision_drive_check_hook(provision_drive_p,  
                                       SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO, 
                                       FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND, 0,  &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE)
        {
            fbe_transport_set_status( packet_p, FBE_STATUS_OK, 0 );
            fbe_transport_complete_packet( packet_p );
            return FBE_STATUS_OK;
        }

	if (!provision_drive_p->bg_zeroing_log_status) {
   	    /* log disk zero start event */
            fbe_provision_drive_event_log_disk_zero_start_or_complete(provision_drive_p, FBE_TRUE, packet_p);
        }

        /* Else proceed with background zeroing.
         */
        status = fbe_provision_drive_send_monitor_packet(provision_drive_p,
                                                         packet_p,
                                                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_BACKGROUND_ZERO,
                                                         zero_checkpoint,
                                                         FBE_PROVISION_DRIVE_CHUNK_SIZE);
    }

    /* Return the event processing status.
     */
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_zero_utils_ask_zero_permission_completion()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_zero_utils_check_permission_based_on_current_load()
 ******************************************************************************
 * @brief
 *   This function will check if a Zero is allowed based on the current
 *   I/O load and scheduler credits available for it.
 *
 * @param provision_drive_p           - pointer to the provision drive object.
 * @param is_ok_to_perform_zero_b_p   - retuns whether it is ok to perform zero
 *                                      operation.
 *
 * @return  fbe_status_t           
 *
 * @author
 *   07/19/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_zero_utils_check_permission_based_on_current_load(fbe_provision_drive_t * provision_drive_p,
                                                                      fbe_bool_t * is_ok_to_perform_zero_b_p)
{
    
    fbe_base_config_operation_permission_t  permission_data;
    
    /* Initialize the output parameter. By default, the zero can not proceed. */
    *is_ok_to_perform_zero_b_p = FBE_FALSE;

    /* Set the zero priority in the permission request. */
    permission_data.operation_priority = fbe_provision_drive_get_zero_priority(provision_drive_p);

    /* Set up the CPU and mbytes parameters for the permission request. */
    permission_data.credit_requests.cpu_operations_per_second = fbe_provision_drive_class_get_zeroing_cpu_rate();
    permission_data.credit_requests.mega_bytes_consumption     = 0;
    permission_data.io_credits                                 = 1;

    /* Make the permission request and set the result in the output data. */ 
    fbe_base_config_get_operation_permission((fbe_base_config_t*) provision_drive_p,
                                             &permission_data,
											 FBE_TRUE,
                                             is_ok_to_perform_zero_b_p);

	if(*is_ok_to_perform_zero_b_p == FBE_FALSE){
		fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
								 (fbe_base_object_t *) provision_drive_p,
								 (fbe_lifecycle_timer_msec_t) 500);
	}
    /* return status. */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_utils_check_permission_based_on_current_load()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_zero_utils_compute_crc()
 ******************************************************************************
 * @brief
 *  This function is used to find whether background zeroing is already 
 *  completed for the provision drive object.
 *
 * @param provision_drive_p         - Pointer to the provision drive object.
 * @param is_zeroing_complete_p     - pointer ot the is zeroing completed flag.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/22/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_zero_utils_determine_if_zeroing_complete(fbe_provision_drive_t * provision_drive_p,
                                                             fbe_lba_t zero_checkpoint,
                                                             fbe_bool_t * is_zeroing_complete_p)
{
    fbe_lba_t   exported_capacity;

    /* initialize the zeroing is completed as false. */
    *is_zeroing_complete_p = FBE_FALSE;

    /* get the exported capacity of the provision drive object. */
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &exported_capacity);

    if(zero_checkpoint >= exported_capacity)
    {
        *is_zeroing_complete_p = FBE_TRUE;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_utils_determine_if_zeroing_complete()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_zero_utils_compute_crc()
 ******************************************************************************
 * @brief
 *  This function is used to compute the CRC for the zeroing operation, it also
 *  stamps timestamp on each block.
 *
 * @param block_p               - Pointer to the block (with 520 size). 
 * @param number_of_blocks      - Number of blocks need to compute CRC.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/22/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_zero_utils_compute_crc(void * block_p, fbe_u32_t number_of_blocks)
{
   fbe_u32_t block_index;
   fbe_u32_t * reference_ptr; 

   /* If input pointer is null then return error. */
   if(block_p == NULL)
   {
       return FBE_STATUS_GENERIC_FAILURE;
   }
   
   /* Calculate the checksum and place it as a metadata on block. */
   for(block_index = 0; block_index < number_of_blocks; block_index++)
   {
       reference_ptr = ( fbe_u32_t * ) ((char *)block_p + FBE_METADATA_BLOCK_DATA_SIZE);
       *reference_ptr = fbe_xor_lib_calculate_checksum((fbe_u32_t*)block_p);
       (*reference_ptr) |= 0x7FFF0000;
       reference_ptr++;

       *reference_ptr = 0;
       reference_ptr++;

       block_p = (char *) block_p + FBE_METADATA_BLOCK_SIZE;
   }

   return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_utils_compute_crc()
 ******************************************************************************/

/*******************************
 * end fbe_provision_drive_zero_utils.c
 *******************************/

