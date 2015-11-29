/***************************************************************************
 * Copyright (C) EMC Corporation 2011-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_virtual_drive_event_logging.c
 ***************************************************************************
 *
 * @brief
 *  This file contains function definations  that are used to log
 *  events from virtual drive object.
 * 
 * 
 * @ingroup virtual_drive_class_files
 * 
 * @version
 *   05/23/2011:  Created. Vishnu Sharma
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_virtual_drive_event_logging.h"
#include "fbe/fbe_event_log_api.h"                      //  for fbe_event_log_write
#include "fbe/fbe_event_log_utils.h"                    //  for message codes
#include "fbe/fbe_enclosure.h"                          //  for fbe_base_enclosure_mgmt_get_slot_number_t
#include "fbe/fbe_provision_drive.h"


/*!****************************************************************************
 *  fbe_virtual_drive_logging_send_control_packet_and_get_response   
 ******************************************************************************
 *
 * @brief
 *   This function will send a control operation to the given destination.  The
 *   caller will pass in the control operation type and a buffer for it that 
 *   has been pre-loaded with any input information.  The function will then 
 *   send the request and wait for the response to it, which gets set in the 
 *   buffer. 
 *
 *   Note, this function only sends packets to the SEP package. 
 * 
 * @param in_packet_p               - pointer to a packet 
 * @param in_destination_service    - service to send the request to 
 * @param in_destination_object_id  - object to send the request to 
 * @param in_control_code           - control code to send
 * @param io_buffer_p               - pointer to buffer with input/output data
 * @param in_buffer_size            - size of buffer
 * 
 * @return  fbe_status_t 
 *
 * @author
 *   09/20/2010 - Created. Jean Montes. 
 *
 *****************************************************************************/
static fbe_status_t fbe_virtual_drive_logging_send_control_packet_and_get_response(
                                        fbe_packet_t*                           in_packet_p, 
                                        fbe_service_id_t                        in_destination_service,
                                        fbe_object_id_t                         in_destination_object_id,
                                        fbe_payload_control_operation_opcode_t  in_control_code,
                                        void*                                   io_buffer_p,
                                        fbe_u32_t                               in_buffer_size)
{
    fbe_payload_ex_t*                  sep_payload_p;
    fbe_payload_control_operation_t*    control_op_p;
    fbe_status_t                        packet_status;
    fbe_payload_control_status_t        control_status;


    //  Build a control operation with the control code and buffer pointer passed in 
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    control_op_p  = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    fbe_payload_control_build_operation(control_op_p, in_control_code, io_buffer_p, in_buffer_size);
        
    //  Set the address to send the packet to 
    fbe_transport_set_address(in_packet_p, FBE_PACKAGE_ID_SEP_0, in_destination_service, FBE_CLASS_ID_INVALID, 
            in_destination_object_id);

    //  Set the packet flags.  The traverse flag will cause the destination object to forward the packet if it
    //  does not handle it itself (in the cases where it is going to an object).
    fbe_transport_set_packet_attr(in_packet_p, FBE_PACKET_FLAG_TRAVERSE);
    fbe_transport_set_sync_completion_type(in_packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    //  Send the packet and wait for its completion 
    fbe_service_manager_send_control_packet(in_packet_p);
    fbe_transport_wait_completion(in_packet_p);

    //  Get the control operation status and packet status 
    fbe_payload_control_get_status(control_op_p, &control_status);
    packet_status = fbe_transport_get_status_code(in_packet_p);

    //  Release the control operation 
    fbe_payload_ex_release_control_operation(sep_payload_p, control_op_p);

    //  If there was a control error, return a generic failure status
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //  Return the packet status
    return packet_status;

}
/******************************************************************************
 * end fbe_virtual_drive_logging_send_control_packet_and_get_response()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_event_logging_get_provision_drive_info()
 ******************************************************************************
 * @brief
 * This function sends the get drive info control packet to provision drive
 * object and in return it gets exported drive information from provision
 * drive object.
 *
 * @param virtual_drive_p           - Pointer to virtual drive
 * @param packet_p                  - The packet requesting this operation.
 * @param in_pvd_object_id          - pvd object id.
 * @param in_provision_drive_info_p - provision drive information.
 *
 * @return status                   - The status of the operation.
 *
 * @author
 *  10/26/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_event_logging_get_provision_drive_info(fbe_virtual_drive_t *virtual_drive_p,
                                                                             fbe_packet_t *packet_p,
                                                                             fbe_object_id_t in_pvd_object_id,
                                                                             fbe_provision_drive_info_t *in_provision_drive_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Verify the passed-in provision drive object-id.
     */
    if ((in_pvd_object_id == 0)                     ||
        (in_pvd_object_id == FBE_OBJECT_ID_INVALID)    )
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s provision pvd obj: 0x%x is invalid \n",
                              __FUNCTION__, in_pvd_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize provision drive information with default values. */
    in_provision_drive_info_p->capacity = FBE_LBA_INVALID;
    in_provision_drive_info_p->config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED;
    in_provision_drive_info_p->port_number = FBE_INVALID_PORT_NUM;
    in_provision_drive_info_p->enclosure_number = FBE_INVALID_ENCL_NUM;
    in_provision_drive_info_p->slot_number = FBE_INVALID_SLOT_NUM;
    in_provision_drive_info_p->serial_num[0] = '\0';

    /* Send the control packet and wait for its completion. */
    status = fbe_virtual_drive_logging_send_control_packet_and_get_response(packet_p, 
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     in_pvd_object_id,
                                                     FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,
                                                     in_provision_drive_info_p,
                                                     sizeof(fbe_provision_drive_info_t));
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s provision object id: 0x%x failed. status: 0x%x\n",
                              __FUNCTION__, in_pvd_object_id, status);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_event_logging_get_provision_drive_info()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_event_logging_get_original_pvd_object_id()
 ******************************************************************************
 * @brief
 * Helper function to log the  event from  virtual drive object .
 * 
 * @param   virtual_drive_p             - virtual drive pointer.
 * @param   original_pvd_object_id_p - Pointer to the `original' object id.  This
 *              can either be the source drive for a copy operation for a failing
 *              drive for a permanent spare request.
 * @param   b_is_copy_in_progress - FBE_TRUE - We know that a copy operation is
 *              in progress.
 * 
 * @return status                          - The status of the operation.
 *
 * @author
 *  04/26/2012  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_event_logging_get_original_pvd_object_id(fbe_virtual_drive_t *virtual_drive_p,
                                                                        fbe_object_id_t *original_pvd_object_id_p,
                                                                        fbe_bool_t b_is_copy_in_progress)

{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_object_id_t                         pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                        edge_index;
    
    /* Initialize the response to `invalid object'
     */
    *original_pvd_object_id_p = FBE_OBJECT_ID_INVALID;
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Based on the configuration mode and the setting of `copy in progress'
     * determine the correct edge and get the pvd id.
     */
    if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)     ||
        (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)    )
    {
        /* Validate the status and object id.  The possible options are:
         *  + This is a copy operation where copy is in progress or completed:
         *      o If the configuration mode is mirror first edge: original is first edge
         *      o If the configuration mode is pass-thru second edge: original is first edge
         *  + Else this is a permanent spare or failed copy
         *      o If the configuration mode is mirror first edge: original is second edge
         *      o If the configuration mode is pass-thru second edge: original is second edge
         */
        if (b_is_copy_in_progress == FBE_TRUE)
        {
            edge_index = FIRST_EDGE_INDEX;
        }
        else
        {
            edge_index = SECOND_EDGE_INDEX;
        }
        status = fbe_vitual_drive_get_server_object_id_for_edge(virtual_drive_p,
                                                                edge_index,
                                                                &pvd_object_id);
        if ((status != FBE_STATUS_OK)                      ||
            ((pvd_object_id == 0)                     ||
             (pvd_object_id == FBE_OBJECT_ID_INVALID)    )    )
        {
            /* This isn't an error but display a warning.
             */
            status = FBE_STATUS_NO_OBJECT;
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "event logging get original pvd: mode: %d index: %d no valid pvd object found \n",
                                  configuration_mode, edge_index);
        }
        else
        {
            /* Else populate the output.
             */
            *original_pvd_object_id_p = pvd_object_id;
            status = FBE_STATUS_OK; 
        }
    }
    else if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)   ||
             (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)    )
    {
        /* Validate the status and object id.  The possible options are:
         *  + This is a copy operation where copy is in progress or completed:
         *      o If the configuration mode is mirror second edge: original is second edge
         *      o If the configuration mode is pass-thru first edge: original is second edge
         *  + Else this is a permanent spare or failed copy
         *      o If the configuration mode is mirror second edge: original is first edge
         *      o If the configuration mode is pass-thru first edge: original is first edge
         */
        if (b_is_copy_in_progress == FBE_TRUE)
        {
            edge_index = SECOND_EDGE_INDEX;
        }
        else
        {
            edge_index = FIRST_EDGE_INDEX;
        }
        status = fbe_vitual_drive_get_server_object_id_for_edge(virtual_drive_p,
                                                                edge_index,
                                                                &pvd_object_id);
        if ((status != FBE_STATUS_OK)                      ||
            ((pvd_object_id == 0)                     ||
             (pvd_object_id == FBE_OBJECT_ID_INVALID)    )    )
        {
            /* This isn't an error but display a warning.
             */
            status = FBE_STATUS_NO_OBJECT;
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "event logging get original pvd: mode: %d index: %d no valid pvd object found \n",
                                  configuration_mode, edge_index);
        }
        else
        {
            /* Else populate the output.
             */
            *original_pvd_object_id_p = pvd_object_id;
            status = FBE_STATUS_OK; 
        }
    }
    
    /* Return the execution status
     */
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_event_logging_get_original_pvd_object_id()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_event_logging_get_spare_pvd_object_id()
 ******************************************************************************
 * @brief
 * Helper function to log the  event from  virtual drive object .
 * 
 * @param   virtual_drive_p             - virtual drive pointer.
 * @param   spare_pvd_object_id_p - Pointer to the `original' object id.  This
 *              can either be the source drive for a copy operation for a failing
 *              drive for a permanent spare request.
 * @param   b_is_copy_in_progress - FBE_TRUE - We know that a copy operation is
 *              in progress.
 * 
 * @return status                          - The status of the operation.
 *
 * @author
 *  04/26/2012  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_event_logging_get_spare_pvd_object_id(fbe_virtual_drive_t *virtual_drive_p,
                                                                     fbe_object_id_t *spare_pvd_object_id_p,
                                                                     fbe_bool_t b_is_copy_in_progress)

{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_object_id_t                         pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                        edge_index;
    
    /* Initialize the response to `invalid object'
     */
    *spare_pvd_object_id_p = FBE_OBJECT_ID_INVALID;
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Based on the configuration mode and the setting of `copy in progress'
     * determine the correct edge and get the pvd id.
     */
    if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)     ||
        (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)    )
    {
        /* Validate the status and object id.  The possible options are:
         *  + This is a copy operation where copy is in progress or completed:
         *      o If the configuration mode is mirror first edge: spare is second edge
         *      o If the configuration mode is pass-thru second edge: spare is second edge
         *  + Else this is a permanent spare or failed copy
         *      o If the configuration mode is mirror first edge: spare is first edge
         *      o If the configuration mode is pass-thru second edge: spare is first edge
         */
        if (b_is_copy_in_progress == FBE_TRUE)
        {
            edge_index = SECOND_EDGE_INDEX;
        }
        else
        {
            edge_index = FIRST_EDGE_INDEX;
        }
        status = fbe_vitual_drive_get_server_object_id_for_edge(virtual_drive_p,
                                                                edge_index,
                                                                &pvd_object_id);
        if ((status != FBE_STATUS_OK)                      ||
            ((pvd_object_id == 0)                     ||
             (pvd_object_id == FBE_OBJECT_ID_INVALID)    )    )
        {
            /* This isn't an error but display a warning.
             */
            status = FBE_STATUS_NO_OBJECT;
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "event logging get spare pvd: mode: %d index: %d no valid pvd object found \n",
                                  configuration_mode, edge_index);
        }
        else
        {
            /* Else populate the output.
             */
            *spare_pvd_object_id_p = pvd_object_id;
            status = FBE_STATUS_OK; 
        }
    }
    else if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)   ||
             (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)    )
    {
        /* Validate the status and object id.  The possible options are:
         *  + This is a copy operation where copy is in progress or completed:
         *      o If the configuration mode is mirror second edge: spare is first edge
         *      o If the configuration mode is pass-thru first edge: spare is first edge
         *  + Else this is a permanent spare or failed copy
         *      o If the configuration mode is mirror second edge: spare is second edge
         *      o If the configuration mode is pass-thru first edge: spare is second edge
         */
        if (b_is_copy_in_progress == FBE_TRUE)
        {
            edge_index = FIRST_EDGE_INDEX;
        }
        else
        {
            edge_index = SECOND_EDGE_INDEX;
        }
        status = fbe_vitual_drive_get_server_object_id_for_edge(virtual_drive_p,
                                                                edge_index,
                                                                &pvd_object_id);
        if ((status != FBE_STATUS_OK)                      ||
            ((pvd_object_id == 0)                     ||
             (pvd_object_id == FBE_OBJECT_ID_INVALID)    )    )
        {
            /* This isn't an error but display a warning.
             */
            status = FBE_STATUS_NO_OBJECT;
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "event logging get spare pvd: mode: %d index: %d no valid pvd object found \n",
                                  configuration_mode, edge_index);
        }
        else
        {
            /* Else populate the output.
             */
            *spare_pvd_object_id_p = pvd_object_id;
            status = FBE_STATUS_OK; 
        }
    }
    
    /* Return the execution status
     */
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_event_logging_get_spare_pvd_object_id()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_write_event_log()
 ****************************************************************************** 
 * 
 * @brief   Helper function to log the  event from  virtual drive object .
 * 
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   event_code - The event code to generate
 * @param   swap_status - The status of teh swap request
 * @param   original_pvd_object_id - The pvd object id of the original drive
 * @param   spare_pvd_object_id - The pvd object id of the spare drive
 * @param   packet_p - Pointer to packet (to be used to retrieve the location(s))
 * 
 * @return  status - The status of the operation.
 *
 * @author
 *  04/26/2012  Ron Proulx  - Created.
 * 
 ******************************************************************************/
fbe_status_t  fbe_virtual_drive_write_event_log(fbe_virtual_drive_t *virtual_drive_p,
                                                fbe_u32_t event_code,
                                                fbe_spare_internal_error_t internal_error_status,
                                                fbe_object_id_t original_pvd_object_id,
                                                fbe_object_id_t spare_pvd_object_id,
                                                fbe_packet_t *packet_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_provision_drive_info_t  original_drive_info;
    fbe_provision_drive_info_t  spare_drive_info;

    /* If the original drive pvd object id is valid (it is possible that it is
     * no longer accessible) get the location information (this is required for
     * the event log).  We just send this to the downstream object and it will
     * forward it all the way down to the provision drive object which contains
     * the location information.
     */
    if (original_pvd_object_id != FBE_OBJECT_ID_INVALID)
    {
        /* Get the drive information (location and serial number) of the
         * original provision drive.
         */
        status = fbe_virtual_drive_event_logging_get_provision_drive_info(virtual_drive_p,
                                                                          packet_p,
                                                                          original_pvd_object_id,
                                                                          &original_drive_info);
        if (status != FBE_STATUS_OK)
        {
            /* We were give a valid object id but retrieving the location failed.
             * This is unexpected.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Failed to get info for original pvd: 0x%x status: 0x%x\n",
                                  original_pvd_object_id, status);
            return status;
        }
    }

    /* If the spare drive pvd object id is valid (it is possible that it is
     * no longer accessible) get the location information (this is required for
     * the event log).  We just send this to the downstream object and it will
     * forward it all the way down to the provision drive object which contains
     * the location information.
     */
    if (spare_pvd_object_id != FBE_OBJECT_ID_INVALID)
    {
        /* Get the drive information (location and serial number) of the
         * spare provision drive.
         */
        status = fbe_virtual_drive_event_logging_get_provision_drive_info(virtual_drive_p,
                                                                          packet_p,
                                                                          spare_pvd_object_id,
                                                                          &spare_drive_info);
        if (status != FBE_STATUS_OK)
        {
            /* We were give a valid object id but retrieving the location failed.
             * This is unexpected.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Failed to get location for spare pvd: 0x%x status: 0x%x\n",
                                  spare_pvd_object_id, status);
            return status;
        }
    }
    
    /* Based on the event to log validate that we have the required information.
     */
    switch(event_code)
    {
        case SEP_INFO_SPARE_PERMANENT_SPARE_SWAP_IN_OPERATION_INITIATED:
        case SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN:
        case SEP_INFO_SPARE_PROACTIVE_SPARE_SWAP_IN_OPERATION_INITIATED:
        case SEP_INFO_SPARE_PROACTIVE_SPARE_DRIVE_SWAPPED_IN:
        case SEP_INFO_SPARE_USER_COPY_SPARE_DRIVE_SWAPPED_IN:
        case SEP_INFO_SPARE_USER_COPY_INITIATED:
        case SEP_INFO_SPARE_USER_COPY_DESTINATION_DRIVE_SWAPPED_IN:
        case SEP_INFO_SPARE_COPY_OPERATION_COMPLETED:
        case SEP_ERROR_CODE_SWAP_ABORT_COPY_REQUEST_DENIED:
            /* Both the orignal and spare drive information is required.
             */
            if ((original_pvd_object_id == FBE_OBJECT_ID_INVALID) ||
                (spare_pvd_object_id == FBE_OBJECT_ID_INVALID)       )
            {
                /* Report the error.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: write event: 0x%08X either original: 0x%x or spare: 0x%x object id is invalid \n",
                                      event_code, original_pvd_object_id, spare_pvd_object_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        case SEP_WARN_SPARE_NO_SPARES_AVAILABLE:
        case SEP_WARN_SPARE_NO_SUITABLE_SPARE_AVAILABLE:
        case SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_BROKEN:
        case SEP_ERROR_CODE_COPY_RAID_GROUP_DEGRADED:
        case SEP_ERROR_CODE_SPARE_UNEXPECTED_ERROR:
        case SEP_INFO_SPARE_PERMANENT_SPARE_REQUEST_DENIED:
        case SEP_INFO_SPARE_PROACTIVE_SPARE_REQUEST_DENIED:
        case SEP_INFO_SPARE_USER_COPY_REQUEST_DENIED:
        case SEP_ERROR_CODE_COPY_SOURCE_DRIVE_REMOVED:
        case SEP_ERROR_CODE_RAID_GROUP_HAS_COPY_IN_PROGRESS:
        case SEP_ERROR_CODE_COPY_SOURCE_DRIVE_DEGRADED:
        case SEP_INFO_SPARE_DRIVE_SWAP_OUT:
            /* These event codes only require the original drive.
             */
            if (original_pvd_object_id == FBE_OBJECT_ID_INVALID)
            {
                /* Report the error.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: write event: 0x%08X original: 0x%x object id is invalid \n",
                                      event_code, original_pvd_object_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        case SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_REMOVED:
            /* These event codes only require the original drive.
             */
            if (spare_pvd_object_id == FBE_OBJECT_ID_INVALID)
            {
                /* Report the error.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: write event: 0x%08X Src: 0x%x object id is invalid \n",
                                  event_code, spare_pvd_object_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        default:
            /* Unsupported event code.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: write event: 0x%08X unsupported event. original: 0x%x spare: 0x%x\n",
                                  event_code, original_pvd_object_id, spare_pvd_object_id);
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    /* Based on the event code process the event.  All the event with the same
     * format are grouped together.  If you add an event with a different
     * format then you must add a unique case.
     */
    switch(event_code)
    {
        case SEP_INFO_SPARE_PERMANENT_SPARE_SWAP_IN_OPERATION_INITIATED:
        case SEP_INFO_SPARE_PROACTIVE_SPARE_SWAP_IN_OPERATION_INITIATED:
        case SEP_INFO_SPARE_USER_COPY_INITIATED:
        case SEP_INFO_SPARE_COPY_OPERATION_COMPLETED:
        case SEP_ERROR_CODE_SWAP_ABORT_COPY_REQUEST_DENIED:
            /* Format is replacement disk location followed by original
             * disk location.
             */
            fbe_event_log_write(event_code, NULL, 0, 
                        "%d %d %d %d %d %d",spare_drive_info.port_number,
                        spare_drive_info.enclosure_number,spare_drive_info.slot_number,
                        original_drive_info.port_number,original_drive_info.enclosure_number,
                        original_drive_info.slot_number);
            break;
            
        case SEP_INFO_SPARE_DRIVE_SWAP_OUT:
            fbe_event_log_write(event_code, NULL, 0, 
                        "%d %d %d",
                        original_drive_info.port_number,original_drive_info.enclosure_number,
                        original_drive_info.slot_number);
            break;
            
        case SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN:
        case SEP_INFO_SPARE_PROACTIVE_SPARE_DRIVE_SWAPPED_IN:
        case SEP_INFO_SPARE_USER_COPY_SPARE_DRIVE_SWAPPED_IN:
        case SEP_INFO_SPARE_USER_COPY_DESTINATION_DRIVE_SWAPPED_IN:
            /* Format is spare disk and serial number followed by failing
             * disk and serial number.
             */
            fbe_event_log_write(event_code, NULL, 0, 
                        "%d %d %d %s %d %d %d %s",spare_drive_info.port_number,
                        spare_drive_info.enclosure_number,spare_drive_info.slot_number,spare_drive_info.serial_num,
                        original_drive_info.port_number,original_drive_info.enclosure_number,
                        original_drive_info.slot_number,original_drive_info.serial_num);
            break;
                            
        case SEP_WARN_SPARE_NO_SPARES_AVAILABLE:
        case SEP_WARN_SPARE_NO_SUITABLE_SPARE_AVAILABLE:
        case SEP_INFO_SPARE_PERMANENT_SPARE_REQUEST_DENIED:
        case SEP_INFO_SPARE_PROACTIVE_SPARE_REQUEST_DENIED:
        case SEP_INFO_SPARE_USER_COPY_REQUEST_DENIED:
            /* Format is failing disk location and serial number.
             */
            fbe_event_log_write(event_code, NULL, 0, 
                        "%d %d %d %s",
                        original_drive_info.port_number,
                        original_drive_info.enclosure_number,
                        original_drive_info.slot_number,
                        original_drive_info.serial_num);
            break;

        case SEP_ERROR_CODE_COPY_SOURCE_DRIVE_REMOVED:
        case SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_BROKEN:
        case SEP_ERROR_CODE_COPY_RAID_GROUP_DEGRADED:
        case SEP_ERROR_CODE_RAID_GROUP_HAS_COPY_IN_PROGRESS:
        case SEP_ERROR_CODE_COPY_SOURCE_DRIVE_DEGRADED:
            /* Format is failing disk location.
             */
            fbe_event_log_write(event_code, NULL, 0, 
                        "%d %d %d",
                        original_drive_info.port_number,
                        original_drive_info.enclosure_number,
                        original_drive_info.slot_number);
            break;

        case SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_REMOVED:
            /* Format is failing disk location.
             */
            fbe_event_log_write(event_code, NULL, 0, 
                    "%d %d %d",
                    spare_drive_info.port_number,
                    spare_drive_info.enclosure_number,
                    spare_drive_info.slot_number);
            break;

        case SEP_ERROR_CODE_SPARE_UNEXPECTED_ERROR:
            /* Format is swap status followed by original disk location.
             */
            fbe_event_log_write(event_code, NULL, 0, 
                        "%d %d %d %d",
                        internal_error_status,
                        original_drive_info.port_number,
                        original_drive_info.enclosure_number,
                        original_drive_info.slot_number);
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: write event: 0x%08X internal_error_status: %d orig: 0x%x(%d_%d_%d) \n",
                                  event_code, internal_error_status, original_pvd_object_id,
                                  original_drive_info.port_number, original_drive_info.enclosure_number, 
                                  original_drive_info.slot_number);
            break;

        default:
            /* Unsupported event code.  Generate an error trace and fail the
             * request.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: write event: 0x%08X unsupported. internal_error_status: %d orig: 0x%x(%d_%d_%d) \n",
                                  event_code, internal_error_status, original_pvd_object_id,
                                  original_drive_info.port_number, original_drive_info.enclosure_number, 
                                  original_drive_info.slot_number);
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    /* Return the execution status.
     */
    return status;
}
/*****************************************
 * end fbe_virtual_drive_write_event_log()
 *****************************************/

/***************************************
 * end fbe_virtual_drive_event_logging.c
 ***************************************/
