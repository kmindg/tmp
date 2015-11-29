/**************************************************************************
*  
*  *****************************************************************
*  *** 2012_03_30 SATA DRIVES ARE FAULTED IF PLACED IN THE ARRAY ***
*  *****************************************************************
*      SATA drive support was added as black content to R30
*      but SATA drives were never supported for performance reasons.
*      The code has not been removed in the event that we wish to
*      re-address the use of SATA drives at some point in the future.
*
***************************************************************************/

#include "sata_physical_drive_private.h"
#include "fbe_sas_port.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_stp_transport.h"

/* Forward declaration */
static fbe_status_t sata_physical_drive_attach_stp_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_open_stp_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_detach_stp_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t sata_physical_drive_get_edge_info(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_set_stp_edge_tap_hook(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);

static fbe_status_t sata_physical_drive_get_dev_info(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_get_dev_info_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_set_write_uncorrectable(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_send_pass_thru_cdb(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);
static fbe_status_t fbe_sata_physical_drive_get_port_info(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);

fbe_status_t 
fbe_sata_physical_drive_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;

    sata_physical_drive = (fbe_sata_physical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry.\n", __FUNCTION__);

    control_code = fbe_transport_get_control_code(packet);
    switch(control_code) {
        case FBE_STP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO:
            status = sata_physical_drive_get_edge_info( sata_physical_drive, packet);
            break;
        case FBE_STP_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK:
            status = sata_physical_drive_set_stp_edge_tap_hook(sata_physical_drive, packet);
            break;
/*      case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION:
 *          status = sata_physical_drive_get_dev_info(sata_physical_drive, packet);
 *          break;
 */
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_SATA_WRITE_UNCORRECTABLE:
            status = sata_physical_drive_set_write_uncorrectable(sata_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SEND_PASS_THRU_CDB:
            status = sata_physical_drive_send_pass_thru_cdb(sata_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_SMART_DUMP:
            status = fbe_sata_physical_drive_get_smart_dump(sata_physical_drive, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_GET_PORT_INFO:
            status = fbe_sata_physical_drive_get_port_info (sata_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_ENHANCED_QUEUING_TIMER:
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        default:
            status = fbe_base_physical_drive_control_entry(object_handle, packet);
            break;
    }

    return status;
}


fbe_status_t
fbe_sata_physical_drive_attach_stp_edge(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_stp_transport_control_attach_edge_t stp_transport_control_attach_edge; /* sent to the sata_port object */
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_path_state_t path_state;
    fbe_object_id_t my_object_id;
    fbe_object_id_t port_object_id;
    fbe_status_t status;
    fbe_semaphore_t sem;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry\n", __FUNCTION__);

    /* Build the edge. ( We have to perform some sanity checking here) */ 
    fbe_stp_transport_get_path_state(&sata_physical_drive->stp_edge, &path_state);
    if(path_state != FBE_PATH_STATE_INVALID) { /* STP edge may be connected only once */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s path_state %X\n", __FUNCTION__, path_state);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_discovered_get_port_object_id((fbe_base_discovered_t *)sata_physical_drive, &port_object_id);

    fbe_stp_transport_set_server_id(&sata_physical_drive->stp_edge, port_object_id);
    /* We have to provide sas_address and generation code to help port object to identify the client */

    fbe_stp_transport_set_transport_id(&sata_physical_drive->stp_edge);

    fbe_base_object_get_object_id((fbe_base_object_t *)sata_physical_drive, &my_object_id);

    fbe_stp_transport_set_server_id(&sata_physical_drive->stp_edge, port_object_id);

    fbe_stp_transport_set_client_id(&sata_physical_drive->stp_edge, my_object_id);
    fbe_stp_transport_set_client_index(&sata_physical_drive->stp_edge, 0); /* We have only one stp edge */

    stp_transport_control_attach_edge.stp_edge = &sata_physical_drive->stp_edge;

    fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if(payload_control_operation == NULL) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s fbe_payload_ex_allocate_control_operation failed\n",__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_build_operation(payload_control_operation, 
                                                    FBE_STP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE, 
                                                    &stp_transport_control_attach_edge, 
                                                    sizeof(fbe_stp_transport_control_attach_edge_t));

    status = fbe_transport_set_completion_function(packet, sata_physical_drive_attach_stp_edge_completion, &sem);


    /* We are sending control packet, so we have to form address manually */
    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                port_object_id);

    /* Control packets should be sent via service manager */
    status = fbe_service_manager_send_control_packet(packet);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    return FBE_STATUS_OK;
}

static fbe_status_t 
sata_physical_drive_attach_stp_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_ex_release_control_operation(payload, payload_control_operation);

    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_sata_physical_drive_open_stp_edge(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_stp_transport_control_open_edge_t stp_transport_control_open_edge; /* sent to the sata_port object */
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_address_t address;

    fbe_status_t status;
    fbe_semaphore_t sem;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry\n", __FUNCTION__);

    fbe_base_physical_drive_get_adderess((fbe_base_physical_drive_t *)sata_physical_drive, &address);

    stp_transport_control_open_edge.sas_address = address.sas_address;
    stp_transport_control_open_edge.generation_code =((fbe_base_physical_drive_t *)sata_physical_drive)->generation_code;


    fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if(payload_control_operation == NULL) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s fbe_payload_ex_allocate_control_operation failed\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    status =  fbe_payload_control_build_operation(payload_control_operation, 
                                                    FBE_STP_TRANSPORT_CONTROL_CODE_OPEN_EDGE, 
                                                    &stp_transport_control_open_edge, 
                                                    sizeof(fbe_stp_transport_control_open_edge_t));

    status = fbe_transport_set_completion_function(packet, sata_physical_drive_open_stp_edge_completion, &sem);

    fbe_stp_transport_send_control_packet(&sata_physical_drive->stp_edge, packet);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    return FBE_STATUS_OK;
}

static fbe_status_t 
sata_physical_drive_open_stp_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_ex_release_control_operation(payload, payload_control_operation);

    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

static fbe_status_t 
sata_physical_drive_get_edge_info(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_stp_transport_control_get_edge_info_t * get_edge_info = NULL; /* INPUT */
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;


    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(payload_control_operation, &get_edge_info);
    if(get_edge_info == NULL) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "fbe_payload_control_get_buffer failed\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_stp_transport_get_client_id(&sata_physical_drive->stp_edge, &get_edge_info->base_edge_info.client_id);
    status = fbe_stp_transport_get_server_id(&sata_physical_drive->stp_edge, &get_edge_info->base_edge_info.server_id);

    status = fbe_stp_transport_get_client_index(&sata_physical_drive->stp_edge, &get_edge_info->base_edge_info.client_index);
    status = fbe_stp_transport_get_server_index(&sata_physical_drive->stp_edge, &get_edge_info->base_edge_info.server_index);

    status = fbe_stp_transport_get_path_state(&sata_physical_drive->stp_edge, &get_edge_info->base_edge_info.path_state);
    status = fbe_stp_transport_get_path_attributes(&sata_physical_drive->stp_edge, &get_edge_info->base_edge_info.path_attr);

    get_edge_info->base_edge_info.transport_id = FBE_TRANSPORT_ID_STP;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}



fbe_status_t
fbe_sata_physical_drive_detach_stp_edge(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_stp_transport_control_detach_edge_t stp_transport_control_detach_edge; /* sent to the sata_port object */
    fbe_payload_ex_t * payload = NULL;
    fbe_path_state_t path_state;
    fbe_payload_control_operation_t * payload_control_operation = NULL;

    fbe_status_t status;
    fbe_semaphore_t sem;

    fbe_base_object_trace(  (fbe_base_object_t *)sata_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);


    /* Remove the edge hook if any.*/
    fbe_stp_transport_edge_remove_hook(&sata_physical_drive->stp_edge, packet);

    status = fbe_stp_transport_get_path_state(&sata_physical_drive->stp_edge, &path_state);
    /* If we don't have an edge to detach, complete the packet and return Good status. */
    if(status != FBE_STATUS_OK || path_state == FBE_PATH_STATE_INVALID)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                            FBE_TRACE_LEVEL_DEBUG_LOW,
                                            "%s STP edge is Invalid. No need to detach it.\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    stp_transport_control_detach_edge.stp_edge = &sata_physical_drive->stp_edge;

    fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if(payload_control_operation == NULL) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s fbe_payload_ex_allocate_control_operation failed\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_build_operation(payload_control_operation, 
                                                    FBE_STP_TRANSPORT_CONTROL_CODE_DETACH_EDGE, 
                                                    &stp_transport_control_detach_edge, 
                                                    sizeof(fbe_stp_transport_control_detach_edge_t));

    status = fbe_transport_set_completion_function(packet, sata_physical_drive_detach_stp_edge_completion, &sem);

    fbe_stp_transport_send_control_packet(&sata_physical_drive->stp_edge, packet);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    return FBE_STATUS_OK;
}

static fbe_status_t 
sata_physical_drive_detach_stp_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_ex_release_control_operation(payload, payload_control_operation);

    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn sata_physical_drive_set_stp_edge_tap_hook(fbe_sata_physical_drive_t * sata_physical_drive, 
 *                                     fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function sets the edge tap hook function for our STP edge.
 *
 * @param sata_physical_drive - The sata drive to set the hook.
 * @param packet - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  02/10/2009 - Created. Peter Puhov.
 *
 ****************************************************************/
static fbe_status_t
sata_physical_drive_set_stp_edge_tap_hook(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_transport_control_set_edge_tap_hook_t * hook_info = NULL;    /* INPUT */
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;  

    fbe_base_object_trace(  (fbe_base_object_t *)sata_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &hook_info);
    if (hook_info == NULL)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "fbe_payload_control_get_buffer failed\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_stp_transport_edge_set_hook(&sata_physical_drive->stp_edge, hook_info->edge_tap_hook);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/* end sata_physical_drive_set_edge_tap_hook()*/

static fbe_status_t 
sata_physical_drive_get_dev_info(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_get_drive_information_t * physical_drive_mgmt_get_drive_information = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Check buffer in control payload */
    fbe_payload_control_get_buffer(control_operation, &physical_drive_mgmt_get_drive_information);
    if(physical_drive_mgmt_get_drive_information == NULL){
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_get_drive_information_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //initialize the retry count 
    sata_physical_drive->get_dev_info_retry_count = 0;

    status = fbe_transport_set_completion_function(packet, sata_physical_drive_get_dev_info_completion, sata_physical_drive);

    if (sata_physical_drive->sata_drive_info.is_inscribed)
    {
        /* Send to executer to read inscription. */
        status = fbe_sata_physical_drive_read_inscription(sata_physical_drive, packet);
    }
    else
    {
        /* Send to executer for identify. */
        status = fbe_sata_physical_drive_identify_device(sata_physical_drive, packet);
    }
    return FBE_STATUS_PENDING;
}

static fbe_status_t 
sata_physical_drive_get_dev_info_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = (fbe_sata_physical_drive_t*)context;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_physical_drive_information_t * drive_info;
    fbe_lba_t cap64 = 0;
    fbe_status_t status = FBE_STATUS_OK;
    
    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    fbe_payload_control_get_buffer(control_operation, &drive_info);
    if (drive_info == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK){
        /* Set additional information. */
        drive_info->product_rev_len = FBE_SCSI_INQUIRY_REVISION_SIZE;
        drive_info->gross_capacity = ((fbe_base_physical_drive_t *)sata_physical_drive)->block_count; 
        drive_info->speed_capability = FBE_DRIVE_SPEED_3GB; 
        drive_info->drive_type = FBE_DRIVE_TYPE_SATA;

        cap64 =drive_info->gross_capacity;
        cap64 *= 512;
        cap64 /= 1000*1000*1000; 
        fbe_zero_memory(drive_info->drive_description_buff, FBE_SCSI_DESCRIPTION_BUFF_SIZE + 1);
        fbe_sprintf (drive_info->drive_description_buff, FBE_SCSI_DESCRIPTION_BUFF_SIZE, "%llu GB GEN Disk \n", (unsigned long long)cap64);
    }
    else
    {
        /* If retry counter exceeds, return the request */
        if (sata_physical_drive->get_dev_info_retry_count >= FBE_SATA_PHYSICAL_DRIVE_MAX_GET_INFO_RETRIES) {
            return status;
        }

        /* Retry this by setting ready state condition*/
        status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)sata_physical_drive, 
                                    FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_RETRY_GET_DEV_INFO);

        if (status != FBE_STATUS_OK) {
			fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
									FBE_TRACE_LEVEL_ERROR,
									"%s can't set retry_get_dev_info condition, status: 0x%X\n",
									__FUNCTION__, status);
		}
                
        
        if (status == FBE_STATUS_OK)
        {
            /* Enqueue usurper packet */
            status = fbe_transport_set_completion_function(packet, sata_physical_drive_get_dev_info_completion, sata_physical_drive);
            fbe_base_object_add_to_usurper_queue((fbe_base_object_t *)sata_physical_drive, packet);
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            fbe_transport_set_status(packet, status, 0);
        }
    }

    return status;
}

static fbe_status_t
fbe_sata_physical_drive_get_port_info(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t        status = FBE_STATUS_OK;
    //fbe_base_edge_t *   base_edge_p;
    //fbe_path_state_t    path_state;

    //base_edge_p = fbe_base_transport_server_get_client_edge_by_server_index ((fbe_base_transport_server_t *) sata_physical_drive, 0);
    //if(base_edge_p == NULL) {
    //    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
    //    fbe_transport_complete_packet (packet);
    //    return FBE_STATUS_GENERIC_FAILURE;
    //}

    //fbe_base_transport_get_path_state (base_edge_p, &path_state);
    //if(path_state != FBE_PATH_STATE_ENABLED) {
    //    fbe_transport_set_status(packet, FBE_STATUS_EDGE_NOT_ENABLED, 0);
    //    fbe_transport_complete_packet (packet);
    //    return FBE_STATUS_EDGE_NOT_ENABLED;
    //}

    status = fbe_stp_transport_send_control_packet(&sata_physical_drive->stp_edge, packet);
    return status;
}
/*!**************************************************************
 * @fn sata_physical_drive_set_write_uncorrectable(fbe_sata_physical_drive_t * sata_physical_drive, 
 *                                     fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function sets Write Uncorrectable to SATA drive to trigger a media error.
 *
 * @param sata_physical_drive - The sata drive to set the media error.
 * @param packet - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  07/29/2009 - Created. Christina Chiang.
 *
 ****************************************************************/
static fbe_status_t
sata_physical_drive_set_write_uncorrectable(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_mgmt_set_write_uncorrectable_t * set_lba = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;  

    fbe_base_object_trace(  (fbe_base_object_t *)sata_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &set_lba);
    if (set_lba == NULL)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "fbe_payload_control_get_buffer failed\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_set_write_uncorrectable_t)){
        KvTrace("%s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
       
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_SATA_WRITE_UNCORRECTABLE: 0x%llx\n", (unsigned long long)set_lba->lba); 

    /* Call the function in executer.c to send a write uncorrectable down to mini-port. */ 
    status = fbe_sata_physical_drive_write_uncorrectable(sata_physical_drive, packet, set_lba->lba);
        
    return status; 
}
/* end sata_physical_drive_set_media_error()*/

static fbe_status_t sata_physical_drive_send_pass_thru_cdb(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_mgmt_send_pass_thru_cdb_t *  pass_thru_info = NULL;
    fbe_payload_control_buffer_length_t             out_len = 0;
    fbe_payload_ex_t *                                 payload = NULL;
    fbe_payload_control_operation_t *               control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &pass_thru_info);
    if(pass_thru_info == NULL){
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_send_pass_thru_cdb_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We do not support passthru commands in SATA drive types. */
    pass_thru_info->transfer_count = 0;
    pass_thru_info->payload_cdb_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD;
    pass_thru_info->port_request_status = FBE_PORT_REQUEST_STATUS_PROTOCOL_ERROR;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_sata_physical_drive_get_smart_dump(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_get_smart_dump_t * get_smart_dump = NULL;
    fbe_payload_control_buffer_length_t len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Check buffer in control payload */
    fbe_payload_control_get_buffer(control_operation, &get_smart_dump);
    if(get_smart_dump == NULL){
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_physical_drive_mgmt_get_smart_dump_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* 
       We don't support this function for SATA drives.   Only drives with paddlecards.
    */
    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK; 
}
