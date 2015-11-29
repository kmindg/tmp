#include "sas_physical_drive_private.h"
#include "fbe_sas_port.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_perfstats_interface.h"

/* Forward declaration */
static fbe_status_t sas_physical_drive_attach_ssp_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_open_ssp_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_get_edge_info(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
static fbe_status_t sas_physical_drive_set_ssp_edge_tap_hook(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet_p);
/*static fbe_status_t sas_physical_drive_send_diagnostic(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
 *static fbe_status_t sas_physical_drive_send_diagnostic_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
 */
static fbe_status_t sas_physical_drive_send_pass_thru_cdb(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet_p);
static fbe_status_t sas_physical_drive_send_passthru_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sas_physical_drive_receive_diag_page_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sas_physical_drive_get_disk_error_log_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sas_physical_drive_get_port_info(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
static fbe_status_t fbe_sas_physical_drive_set_write_uncorrectable(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
static fbe_status_t fbe_sas_physical_drive_enable_disable_perf_stats(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
static fbe_status_t fbe_sas_physical_drive_get_perf_stats(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);

static fbe_status_t fbe_sas_physical_drive_health_check_test(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
static fbe_status_t fbe_sas_physical_drive_get_log_pages(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
static fbe_status_t fbe_sas_physical_drive_mode_select_usurper(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
static fbe_status_t fbe_sas_physical_drive_usurper_set_enhanced_queuing_timer(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
static fbe_status_t fbe_sas_physical_drive_usurper_sanitize_drive(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
static fbe_status_t fbe_sas_physical_drive_usurper_get_sanitize_status(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
static fbe_status_t fbe_sas_physical_drive_get_rla_abort_required(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
static fbe_status_t fbe_sas_physical_drive_usurper_format_block_size(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);


fbe_status_t 
fbe_sas_physical_drive_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;

    sas_physical_drive = (fbe_sas_physical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    control_code = fbe_transport_get_control_code(packet);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry. control_code:0x%x\n", __FUNCTION__, control_code);

    switch(control_code) {
        case FBE_SSP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO:
            status = sas_physical_drive_get_edge_info( sas_physical_drive, packet);
            break;
        case FBE_SSP_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK:
            status = sas_physical_drive_set_ssp_edge_tap_hook(sas_physical_drive, packet);
            break;
/*    case FBE_SSP_TRANSPORT_CONTROL_CODE_SEND_DIAGNOSTIC:
 *          status = sas_physical_drive_send_diagnostic(sas_physical_drive, packet);
 *          break; 
 */   
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SEND_PASS_THRU_CDB:
            status = sas_physical_drive_send_pass_thru_cdb(sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_RECEIVE_DIAG_PAGE:
            status = fbe_sas_physical_drive_receive_diag_page(sas_physical_drive, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_GET_PORT_INFO:
            status = fbe_sas_physical_drive_get_port_info (sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DISK_ERROR_LOG:
            status = fbe_sas_physical_drive_get_disk_error_log(sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_SMART_DUMP:
            status = fbe_sas_physical_drive_get_smart_dump(sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DC_RCV_DIAG_ENABLED:
            status = fbe_sas_physical_drive_dc_rcv_diag_enabled(sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_HEALTH_CHECK_INITIATE:  /* issued by user */
            status = fbe_sas_physical_drive_usurper_health_check_initiate(sas_physical_drive, packet);     
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_HEALTH_CHECK_ACK:
            /* Issue drive health check. */
            status = fbe_sas_physical_drive_health_check_ack(sas_physical_drive, packet);            
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_SATA_WRITE_UNCORRECTABLE:
            status = fbe_sas_physical_drive_set_write_uncorrectable(sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_ENABLE_DISABLE_PERF_STATS:
            status = fbe_sas_physical_drive_enable_disable_perf_stats(sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_PERF_STATS:
            status = fbe_sas_physical_drive_get_perf_stats(sas_physical_drive, packet);
            break;
            //TEMP code for testing HC
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_HEALTH_CHECK_TEST:
            status = fbe_sas_physical_drive_health_check_test(sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOG_PAGES:
            status = fbe_sas_physical_drive_get_log_pages(sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_MODE_SELECT:
            status = fbe_sas_physical_drive_mode_select_usurper(sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_READ_LONG:
            status = fbe_sas_physical_drive_read_long(sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_WRITE_LONG:
            status = fbe_sas_physical_drive_write_long(sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_ENHANCED_QUEUING_TIMER:
            status = fbe_sas_physical_drive_usurper_set_enhanced_queuing_timer(sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_ERASE_ONLY:
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_OVERWRITE_AND_ERASE:
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_AFSSI:
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_NSA:
            status = fbe_sas_physical_drive_usurper_sanitize_drive(sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_SANITIZE_STATUS:
            status = fbe_sas_physical_drive_usurper_get_sanitize_status(sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_RLA_ABORT_REQUIRED:
            status = fbe_sas_physical_drive_get_rla_abort_required(sas_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_FORMAT_BLOCK_SIZE:
            status = fbe_sas_physical_drive_usurper_format_block_size(sas_physical_drive, packet);
            break;

        default:
            status = fbe_base_physical_drive_control_entry(object_handle, packet);
            break;
    }

    return status;
}


fbe_status_t
fbe_sas_physical_drive_attach_ssp_edge(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_ssp_transport_control_attach_edge_t ssp_transport_control_attach_edge; /* sent to the sas_port object */
    fbe_packet_t * new_packet = NULL;
    fbe_payload_ex_t * new_payload = NULL;
    fbe_payload_control_operation_t * new_control_operation = NULL;
    fbe_path_state_t path_state;
    fbe_object_id_t my_object_id;
    fbe_object_id_t port_object_id;

    fbe_status_t status;
    fbe_semaphore_t sem;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry\n", __FUNCTION__);

    /* Build the edge. ( We have to perform some sanity checking here) */ 
    fbe_ssp_transport_get_path_state(&sas_physical_drive->ssp_edge, &path_state);
    if(path_state != FBE_PATH_STATE_INVALID) { /* ssp edge may be connected only once */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "path_state %X\n",path_state);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_discovered_get_port_object_id((fbe_base_discovered_t *)sas_physical_drive, &port_object_id);

    fbe_ssp_transport_set_server_id(&sas_physical_drive->ssp_edge, port_object_id);
    /* We have to provide sas_address and generation code to help port object to identify the client */

    fbe_ssp_transport_set_transport_id(&sas_physical_drive->ssp_edge);

    fbe_base_object_get_object_id((fbe_base_object_t *)sas_physical_drive, &my_object_id);

    fbe_ssp_transport_set_server_id(&sas_physical_drive->ssp_edge, port_object_id);

    fbe_ssp_transport_set_client_id(&sas_physical_drive->ssp_edge, my_object_id);
    fbe_ssp_transport_set_client_index(&sas_physical_drive->ssp_edge, 0); /* We have only one ssp edge */

    ssp_transport_control_attach_edge.ssp_edge = &sas_physical_drive->ssp_edge;

    fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet */
    new_packet = fbe_transport_allocate_packet();
    if(new_packet == NULL) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "fbe_transport_allocate_packet failed\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_packet(new_packet);

    new_payload = fbe_transport_get_payload_ex(new_packet);
    new_control_operation = fbe_payload_ex_allocate_control_operation(new_payload);

    fbe_payload_control_build_operation(new_control_operation,  // control operation
                                    FBE_SSP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE,  // opcode
                                    &ssp_transport_control_attach_edge, // buffer
                                    sizeof(fbe_ssp_transport_control_attach_edge_t));   // buffer_length 

    status = fbe_transport_set_completion_function(new_packet, sas_physical_drive_attach_ssp_edge_completion, &sem);


    /* We are sending control packet, so we have to form address manually */
    /* Set packet address */
    fbe_transport_set_address(  new_packet,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                port_object_id);

    /* Control packets should be sent via service manager */
    status = fbe_service_manager_send_control_packet(new_packet);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);


    status = fbe_transport_get_status_code(new_packet);
    if(status != FBE_STATUS_OK){ 
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "sas_physical_drive_create_edge failed status %X\n", status);
    }

    fbe_transport_copy_packet_status(new_packet, packet);
    fbe_payload_ex_release_control_operation(new_payload, new_control_operation);

    fbe_transport_release_packet(new_packet);

    //fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t 
sas_physical_drive_attach_ssp_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_sas_physical_drive_open_ssp_edge(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_ssp_transport_control_open_edge_t ssp_transport_control_open_edge; /* sent to the sas_port object */
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_address_t address;

    fbe_status_t status;
    fbe_semaphore_t sem;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry\n", __FUNCTION__);

    fbe_base_physical_drive_get_adderess((fbe_base_physical_drive_t *)sas_physical_drive, &address);

    ssp_transport_control_open_edge.sas_address = address.sas_address;
    ssp_transport_control_open_edge.generation_code =((fbe_base_physical_drive_t *)sas_physical_drive)->generation_code;


    fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload); 
    if(control_operation == NULL) {    
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Failed to allocate control operation\n",__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;    
    }


    fbe_payload_control_build_operation(control_operation,  // control operation
                                    FBE_SSP_TRANSPORT_CONTROL_CODE_OPEN_EDGE,  // opcode
                                    &ssp_transport_control_open_edge, // buffer
                                    sizeof(fbe_ssp_transport_control_open_edge_t));   // buffer_length 

    status = fbe_transport_set_completion_function(packet, sas_physical_drive_open_ssp_edge_completion, &sem);

    fbe_ssp_transport_send_control_packet(&sas_physical_drive->ssp_edge, packet);

    fbe_semaphore_wait(&sem, NULL); /* We have to wait because ssp_transport_control_open_edge is on the stack */
    fbe_semaphore_destroy(&sem);

    return FBE_STATUS_OK;
}

static fbe_status_t 
sas_physical_drive_open_ssp_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload); 
    fbe_payload_ex_release_control_operation(payload, control_operation);

    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

static fbe_status_t 
sas_physical_drive_get_edge_info(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_ssp_transport_control_get_edge_info_t * get_edge_info = NULL; /* INPUT */
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &get_edge_info); 
    if(get_edge_info == NULL) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "fbe_payload_control_get_buffer failed\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_ssp_transport_get_client_id(&sas_physical_drive->ssp_edge, &get_edge_info->base_edge_info.client_id);
    status = fbe_ssp_transport_get_server_id(&sas_physical_drive->ssp_edge, &get_edge_info->base_edge_info.server_id);

    status = fbe_ssp_transport_get_client_index(&sas_physical_drive->ssp_edge, &get_edge_info->base_edge_info.client_index);
    status = fbe_ssp_transport_get_server_index(&sas_physical_drive->ssp_edge, &get_edge_info->base_edge_info.server_index);

    status = fbe_ssp_transport_get_path_state(&sas_physical_drive->ssp_edge, &get_edge_info->base_edge_info.path_state);
    status = fbe_ssp_transport_get_path_attributes(&sas_physical_drive->ssp_edge, &get_edge_info->base_edge_info.path_attr);

    get_edge_info->base_edge_info.transport_id = FBE_TRANSPORT_ID_SSP;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn sas_physical_drive_set_ssp_edge_tap_hook(fbe_sas_physical_drive_t * sas_physical_drive, 
 *                                     fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function sets the edge tap hook function for our block edge.
 *
 * @param logical_drive_p - The logical drive to set the hook.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  12/15/2008 - Created. Vicky Guo.
 *
 ****************************************************************/
static fbe_status_t
sas_physical_drive_set_ssp_edge_tap_hook(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet_p)
{
    fbe_transport_control_set_edge_tap_hook_t * hook_info = NULL;    /* INPUT */
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;  

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &hook_info);
    if (hook_info == NULL)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "fbe_payload_control_get_buffer failed\n");

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_ssp_transport_edge_set_hook(&sas_physical_drive->ssp_edge, hook_info->edge_tap_hook);

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end sas_physical_drive_set_edge_tap_hook()
 **************************************/

#if 0
/* TODO: Will move to other file later */
static fbe_status_t 
sas_physical_drive_send_diagnostic(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    
    fbe_transport_set_completion_function(packet, sas_physical_drive_send_diagnostic_completion, sas_physical_drive);
    
    sas_physical_drive_cdb_build_send_diagnostics(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_INQUIRY_TIMEOUT);

    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, new_packet);

    return status;
}

/*
 * This is the completion function for the reassign packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_physical_drive_send_diagnostic_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_status_t status;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    
    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        /* check error code here */
    }
         
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    
    return FBE_STATUS_OK;
}
#endif

/*
This command is used to send passthru commands from Flare, all the way down to the physical drive.
Currently it is being used only to send mode pages to enable error injection on SAS drives with special firmware on them.
*/
static fbe_status_t sas_physical_drive_send_pass_thru_cdb(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_mgmt_send_pass_thru_cdb_t *  pass_thru_info = NULL;
    fbe_payload_control_buffer_length_t             out_len = 0;
    fbe_payload_ex_t *                                 payload = NULL;
    fbe_payload_control_operation_t *               control_operation = NULL;
    fbe_payload_cdb_operation_t *                   payload_cdb_operation = NULL;
    fbe_u8_t *                                      cdb = NULL;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                               FBE_TRACE_LEVEL_DEBUG_LOW,
                                               "%s entry\n", __FUNCTION__);
    

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &pass_thru_info);
    if(pass_thru_info == NULL){
        KvTrace("%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_send_pass_thru_cdb_t)){
        KvTrace("%s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*if we are here, we are good to go, let's prepare the CDB operation*/
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    /*set some basic CDB stuff, the sg list we get from free in this kind of opcode since the shim set it before*/
    fbe_payload_cdb_set_parameters(payload_cdb_operation,
                                   FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT,
                                   pass_thru_info->payload_cdb_task_attribute,
                                   pass_thru_info->payload_cdb_flags,
                                   pass_thru_info->cdb_length);

    fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb);
    fbe_copy_memory(cdb, pass_thru_info->cdb, pass_thru_info->cdb_length);

    /* workaround: SAS uses DIAG page 0x82 */
    if ((cdb[0] == FBE_SCSI_RECEIVE_DIAGNOSTIC) && (cdb[2] == 0x80)) {
        cdb[2] = 0x82;
    }

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                               "%s CDB Opcode:0x%x\n", __FUNCTION__, cdb[0] );


    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, pass_thru_info->transfer_count);

    /*set the return function*/
    fbe_transport_set_completion_function(packet, sas_physical_drive_send_passthru_completion, sas_physical_drive);

    return fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);

}

/*pass thru operations return here*/
static fbe_status_t sas_physical_drive_send_passthru_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t *                      sas_physical_drive = NULL;
    fbe_sg_element_t  *                             sg_list = NULL; 
    fbe_payload_ex_t *                                 payload = NULL;
    fbe_payload_cdb_operation_t *                   payload_cdb_operation = NULL;
    fbe_payload_control_operation_t *               payload_control_operation = NULL;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_physical_drive_mgmt_send_pass_thru_cdb_t *  pass_thru_info = NULL;
    fbe_u8_t *                                      sense_buffer = NULL;
    fbe_scsi_error_code_t                           error = 0;
    fbe_u32_t                                       transferred_count = 0;     
    fbe_port_request_status_t                       cdb_request_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    fbe_payload_cdb_scsi_status_t                   cdb_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD;
    fbe_base_physical_drive_t *                     base_physical_drive = NULL;
    
    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                               FBE_TRACE_LEVEL_DEBUG_LOW,
                                               "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_cdb_get_transferred_count(payload_cdb_operation, &transferred_count);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_payload_control_get_buffer(payload_control_operation, &pass_thru_info);
        fbe_payload_cdb_get_request_status(payload_cdb_operation, &cdb_request_status);
        fbe_payload_cdb_get_scsi_status(payload_cdb_operation, &cdb_scsi_status);
        base_physical_drive = (fbe_base_physical_drive_t *)sas_physical_drive;

        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        if (error == 0 || error == FBE_SCSI_XFERCOUNTNOTZERO)
        {
            pass_thru_info->transfer_count = transferred_count;
            
            /* Write the diag page to the disk if this is a Receive Diagnostic command. */
/*            if (pass_thru_info->cdb[0] == FBE_SCSI_RECEIVE_DIAGNOSTIC) {
                fbe_sas_physical_drive_fill_receive_diag_info(sas_physical_drive, sg_list[0].address, RECEIVE_DIAG_PG82_SIZE);
            }
*/ 
        }
        else
        {
            pass_thru_info->transfer_count = 0;
            pass_thru_info->payload_cdb_scsi_status = cdb_scsi_status;
            pass_thru_info->port_request_status = cdb_request_status;

            if (cdb_scsi_status == FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION)
            {
                /* In check condition, copy the sense buffer */
                fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_buffer);
                fbe_copy_memory(pass_thru_info->sense_info_buffer, sense_buffer, pass_thru_info->sense_buffer_lengh);
            }
        }
    }

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    return FBE_STATUS_OK;
}


static fbe_status_t
fbe_sas_physical_drive_get_port_info(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t        status = FBE_STATUS_OK;
    //fbe_base_edge_t *   base_edge_p;
    //fbe_path_state_t    path_state;

    //base_edge_p = fbe_base_transport_server_get_client_edge_by_server_index ((fbe_base_transport_server_t *) sas_physical_drive, 0);
    //if(base_edge_p == NULL) {
    //    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
    //    fbe_transport_complete_packet (packet);
    //    return FBE_STATUS_GENERIC_FAILURE;
    //}
    
    //fbe_base_transport_get_path_state(base_edge_p, &path_state);
    //if(path_state != FBE_PATH_STATE_ENABLED) {
    //    fbe_transport_set_status(packet, FBE_STATUS_EDGE_NOT_ENABLED, 0);
    //    fbe_transport_complete_packet (packet);
    //    return FBE_STATUS_EDGE_NOT_ENABLED;
    //}

    status = fbe_ssp_transport_send_control_packet(&sas_physical_drive->ssp_edge, packet);
    return status;
}

fbe_status_t 
fbe_sas_physical_drive_receive_diag_page(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_receive_diag_page_t * receive_diag_page = NULL;
    fbe_payload_control_buffer_length_t len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Check buffer in control payload */
    fbe_payload_control_get_buffer(control_operation, &receive_diag_page);
    if(receive_diag_page == NULL){
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_physical_drive_mgmt_receive_diag_page_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_transport_set_completion_function(packet, fbe_sas_physical_drive_receive_diag_page_completion, sas_physical_drive);

    /* Send to executer for inquiry. */
    status = fbe_sas_physical_drive_send_RDR(sas_physical_drive, packet, receive_diag_page->page_code);

    return FBE_STATUS_PENDING;
}

static fbe_status_t 
fbe_sas_physical_drive_receive_diag_page_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    
    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    return FBE_STATUS_OK;
}
fbe_status_t 
fbe_sas_physical_drive_get_disk_error_log(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_get_disk_error_log_t * get_lba = NULL;
    fbe_payload_control_buffer_length_t len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Check buffer in control payload */
    fbe_payload_control_get_buffer(control_operation, &get_lba);
    if(get_lba == NULL){
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_physical_drive_mgmt_get_disk_error_log_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_transport_set_completion_function(packet, fbe_sas_physical_drive_get_disk_error_log_completion, sas_physical_drive);

    /* Send to executer for inquiry. */
    status = fbe_sas_physical_drive_send_read_disk_blocks(sas_physical_drive, packet);

    return FBE_STATUS_PENDING;
}

static fbe_status_t 
fbe_sas_physical_drive_get_disk_error_log_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    
    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_physical_drive_get_smart_dump(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
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

    /* Send to executer for inquiry. */
    status = fbe_sas_physical_drive_send_smart_dump(sas_physical_drive, packet);

    return FBE_STATUS_PENDING;
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_dc_rcv_diag_enabled(fbe_sas_physical_drive_t * sas_physical_drive, 
 *                                     fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function sets receive diagnostic page enabled or disabled for the disk collect.
 *
 * @param logical_drive_p - The logical drive to set the hook.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  10/10/20108 - Created. CLC.
 *
 ****************************************************************/
fbe_status_t
fbe_sas_physical_drive_dc_rcv_diag_enabled(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet_p)
{
    fbe_u8_t * flag = NULL;    /* INPUT */
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;  
    fbe_payload_control_buffer_length_t len = 0;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &flag);
    if (flag == NULL)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "fbe_payload_control_get_buffer failed\n");

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_u8_t)){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%X len invalid\n", len);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_sas_physical_drive_set_dc_rcv_diag_enabled(sas_physical_drive, *flag);

    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_sas_physical_drive_dc_rcv_diag_enabled()
 **************************************/

/*!**************************************************************
 * @fn fbe_sas_physical_drive_usurper_health_check_initiate
 ****************************************************************
 * @brief
 *  This function is called by user to initate a health check, as
 *  opposed to being triggered by service timeout.
 *
 * @param sas_physical_drive - The drive on which to enable its health check.
 * @param packet - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/20/2013 : Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_physical_drive_usurper_health_check_initiate(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;  
    fbe_payload_control_buffer_length_t len = 0;


    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != 0){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "usurper_health_check_initiate: len invalid. expected=%d actual=%d\n", 0, len);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If we are in a middle of a download when this happens, then fail request since we don't support HC during download.
    */
    if (fbe_base_physical_drive_is_local_state_set((fbe_base_physical_drive_t*)sas_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD))
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                   "usurper_health_check_initiate: failed. download in progress. \n");
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                               "usurper_health_check_initiate: inititated by client. \n");

    fbe_base_drive_initiate_health_check((fbe_base_physical_drive_t*)sas_physical_drive, FBE_FALSE /*not due to service timeout*/);

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_sas_physical_drive_health_check_ack(fbe_sas_physical_drive_t * sas_physical_drive, 
 *                                         fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function represents what the PDO does when receiving the
 *  FBE_PHYSICAL_DRIVE_CONTROL_CODE_HEALTH_CHECK usurper command.
 *
 * @param sas_physical_drive - The drive on which to enable its health check.
 * @param packet - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  07/05/2012 - Created. Darren Insko
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_physical_drive_health_check_ack(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_status_t return_status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;  
    fbe_payload_control_buffer_length_t len = 0;
    fbe_physical_drive_health_check_req_status_t *request_status = NULL;


    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &request_status);
    if (NULL == request_status)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_physical_drive_health_check_req_status_t)){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s len invalid. expected=%d actual=%d\n", __FUNCTION__, (int)sizeof(fbe_physical_drive_health_check_req_status_t),len);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                               "%s entry. request_status=%d\n", __FUNCTION__, *request_status);

    /* PVD has acknowledged either HC can proceed or it should be aborted. At this point, if status was proceed then 
       PVD is in activate state waiting for PDO HC to complete or fail. If request status was to abort then PDO simply
       needs to clean up and transition back to Ready state. Most likely scenario for this case is that the peer side 
       requested HC first.  It should only be done on one side. */


    /* The orginal attr request can now be cleared. */
    fbe_block_transport_server_clear_path_attr_all_servers(&((fbe_base_physical_drive_t *)sas_physical_drive)->block_transport_server,
                                                           FBE_BLOCK_PATH_ATTR_FLAGS_HEALTH_CHECK_REQUEST);

    if (FBE_DRIVE_HC_STATUS_PROCEED == *request_status)
    {                   
        /* Init the retry count and control status & qualifier. */
        payload->physical_drive_transaction.retry_count = 0;
        
        /* Initializing to retry so it can be sent the first time */
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_payload_control_set_status_qualifier(control_operation, FBE_DRIVE_CMD_STATUS_FAIL_RETRYABLE);
        
        /*TODO: add pkt before setting condition. */
        /* Do the actual Health Check.*/
        status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const,
                                        (fbe_base_object_t*)sas_physical_drive,
                                        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_HEALTH_CHECK);
    
        if (status != FBE_STATUS_OK)
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        "%s: Could not set FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_HEALTH_CHECK, status: 0x%X.\n",
                                        __FUNCTION__, status);
            fbe_transport_set_status(packet_p, status, 0);
            fbe_transport_complete_packet(packet_p);
            return status;
        }
        else
        {
            /* Enqueue usurper packet */
            /* note, we are adding packet to usurper Q after setting the condition.  There is a race condition
               here.  It's possible first time condition is executed that pkt doesn't exist.  Condition function
               needs to handle this case correctly */
            fbe_base_object_add_to_usurper_queue((fbe_base_object_t *)sas_physical_drive, packet_p);
            return_status = FBE_STATUS_PENDING;
        }
    
    
        /* set last HC condition to do any cleanup. Note, any recovery actions taken which use conditions,
           must have conditions ordered in rotary before this one.  */
        fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                               (fbe_base_object_t*)sas_physical_drive, 
                               FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_HEALTH_CHECK_COMPLETE);

        /* Clear the initiate_health_check cond now that the QST cmd can be sent.  This condition
           was used to keep PDO in Activate state until QST starts. 
        */
        fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(sas_physical_drive), (fbe_base_object_t*)sas_physical_drive,
                                       FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_INITIATE_HEALTH_CHECK);
    }
    else /* abort */
    {
        fbe_base_physical_drive_clear_local_state((fbe_base_physical_drive_t*)sas_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_HEALTH_CHECK);

        /* Clear the initiate_health_check cond now that the QST cmd can be sent.  This condition
           was used to keep PDO in Activate state until QST starts. 
        */
        fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(sas_physical_drive), (fbe_base_object_t*)sas_physical_drive,
                                       FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_INITIATE_HEALTH_CHECK);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return_status = FBE_STATUS_OK;
    }
 
    return return_status;
}
/*******************************************
 * end fbe_sas_physical_drive_health_check_ack()
 *******************************************/

/*!**************************************************************
 * @fn fbe_sas_physical_drive_set_write_uncorrectable(fbe_sas_physical_drive_t * sas_physical_drive, 
 *                                     fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  The intention of this function is to set a Write Uncorrectable to a SATA drive
 *  to trigger a media error.  However, this is a SAS drive so the command is
 *  not supported. Note that some drives may appear to be SATA drives to Flare,
 *  but have a SAS interface(e.g. sata_paddlecard_drive), so Flare could attempt
 *  this call.  This function will always return failure.
 *
 * @param sas_physical_drive - The drive to set the media error.
 * @param packet - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  04/07/2011 - Created. Wayne Garrett.
 *
 ****************************************************************/
static fbe_status_t
fbe_sas_physical_drive_set_write_uncorrectable(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;  

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "%s Write Uncorrectable not supported for SAS drives\n", __FUNCTION__); 

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  


    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_set_write_uncorrectable_t)){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid out_buffer_length %X \n", __FUNCTION__, out_len); 
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
                  
    /* 
       We don't support this function for SAS drives. 
    */
    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK; 
        
    return status; 
}
/**************************************
 * end fbe_sas_physical_drive_set_write_uncorrectable()
 **************************************/

/*!**************************************************************
 * @fn fbe_sas_physical_drive_enable_disable_perf_stats(fbe_sas_physical_drive_t * sas_physical_drive, 
 *                                                      fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function enables/disables the SAS physcial drive objects's
 *  ability to modify PDO-specific performance statistics counters.
 *
 * @param sas_physical_drive - The drive on which to enable/disable its performance statistics.
 * @param packet - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  05/07/2012 - Created. Darren Insko
 *
 ****************************************************************/
static fbe_status_t 
fbe_sas_physical_drive_enable_disable_perf_stats(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_bool_t * b_set_enable = NULL;    /* INPUT */
    fbe_payload_control_buffer_length_t len = 0;
    fbe_pdo_performance_counters_t * temp_pdo_counters = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t *)sas_physical_drive;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &b_set_enable);
    if (b_set_enable == NULL)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "fbe_payload_control_get_buffer failed\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_bool_t)){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Save a local copy of the pointer to the pdo_counters in case it's set to NULL before we access it */
    temp_pdo_counters = sas_physical_drive->performance_stats.counter_ptr.pdo_counters;
    if (*b_set_enable && temp_pdo_counters) 
    {
        fbe_u8_t serial_number[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+4];
        sas_physical_drive->b_perf_stats_enabled = *b_set_enable;
        fbe_copy_memory(serial_number, 
                        sas_physical_drive->performance_stats.counter_ptr.pdo_counters->serial_number,
                        sizeof(fbe_u8_t) * (FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+4));

        fbe_zero_memory(temp_pdo_counters, sizeof(*temp_pdo_counters));

        //repopulate object ID and serial number
        fbe_base_object_get_object_id((fbe_base_object_t *)sas_physical_drive,
                                      &sas_physical_drive->performance_stats.counter_ptr.pdo_counters->object_id);

        fbe_copy_memory(sas_physical_drive->performance_stats.counter_ptr.pdo_counters->serial_number,
                        serial_number,
                        sizeof(fbe_u8_t) * (FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+4));

        fbe_base_physical_drive_reset_statistics(base_physical_drive);

        status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const,
                              (fbe_base_object_t*)sas_physical_drive,
                               FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_CHECK_DRIVE_BUSY);

        if (status != FBE_STATUS_OK)
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "Could not start check drive busy condition\n");
            return status;
        }
    }
    else
    {
        if (temp_pdo_counters)
        {
            fbe_zero_memory(temp_pdo_counters, sizeof(*temp_pdo_counters));            
        }

        fbe_base_physical_drive_reset_statistics(base_physical_drive);
        sas_physical_drive->b_perf_stats_enabled = FBE_FALSE;       
    }

    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/********************************************************
 * end fbe_sas_physical_drive_enable_disable_perf_stats()
 ********************************************************/

/*!**************************************************************
 * @fn fbe_sas_physical_drive_get_perf_stats(fbe_sas_physical_drive_t * sas_physical_drive, 
 *                                                      fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function lets you get a direct copy of the latest performance statistics of a PDO object.
 * 
 * WARNING: this function was only created because we needed a way to copy performance
 * statistics across the transport buffer so fbe_hw_test could access the counter space instead
 * of copying the entire container on each check! Use the PERFSTATS service to access the performance
 * statistics in non-test environments.
 *
 * @param sas_physical_drive - The drive on which to enable/disable its performance statistics.
 * @param packet - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  07/11/2012 - Created. Ryan Gadsby
 *
 ****************************************************************/
static fbe_status_t 
fbe_sas_physical_drive_get_perf_stats(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_pdo_performance_counters_t              * perf_counters = NULL;
    fbe_pdo_performance_counters_t              * temp_pdo_counters = NULL;
    fbe_payload_ex_t *                          payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;

    if (sas_physical_drive->b_perf_stats_enabled == FBE_FALSE)
    {
        status =  FBE_STATUS_GENERIC_FAILURE;
    }
    else
    { //copy the stat memory
         payload = fbe_transport_get_payload_ex(packet);
         control_operation = fbe_payload_ex_get_control_operation(payload);

         fbe_payload_control_get_buffer(control_operation, &perf_counters);
         
         /* Save a local copy of the pointer to the pdo_counters in case it's set to NULL before we access it */
         temp_pdo_counters = sas_physical_drive->performance_stats.counter_ptr.pdo_counters;
         if (perf_counters == NULL || temp_pdo_counters == NULL) 
         {
             status = FBE_STATUS_GENERIC_FAILURE;
         }
         else
         {
             memcpy(perf_counters, temp_pdo_counters, sizeof(fbe_pdo_performance_counters_t));
         }
    }
    fbe_transport_set_status(packet, status, 0);
    status = fbe_transport_complete_packet(packet);
    return status;
}
/********************************************************
 * end fbe_sas_physical_drive_get_perf_stats()
 ********************************************************/


/*!**************************************************************
 * @fn fbe_sas_physical_drive_health_check_test(fbe_sas_physical_drive_t * sas_physical_drive, 
 *                                         fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function sends complete HC sequence by triggering HC INIT condition.
 *
 * @param sas_physical_drive - The drive on which to enable its health check.
 * @param packet - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  04/24/2012 - Created. Darren Insko
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_physical_drive_health_check_test(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;  
    fbe_status_t status;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry\n", __FUNCTION__);

    /* Transition to ACTIVATE state.*/
    status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const,
                                    (fbe_base_object_t*)sas_physical_drive,
                                    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_INITIATE_HEALTH_CHECK);

    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
/**************************************
 * end fbe_sas_physical_drive_health_check_test()
 **************************************/

/*!**************************************************************
 * @fn fbe_sas_physical_drive_get_log_pages(fbe_sas_physical_drive_t * sas_physical_drive, 
 *                                         fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function sends log sense to get the log pages from the drive.
 *
 * @param sas_physical_drive - The drive on which to get the log pages.
 * @param packet - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  11/27/2012 - Created. Christina Chiang
 *
 ****************************************************************/
static fbe_status_t 
fbe_sas_physical_drive_get_log_pages(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_get_log_page_t * get_log_page = NULL;
    fbe_payload_control_buffer_length_t len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Check buffer in control payload */
    fbe_payload_control_get_buffer(control_operation, &get_log_page);
    if(get_log_page == NULL){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_physical_drive_mgmt_get_log_page_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Send to executer for inquiry. */
    status = fbe_sas_physical_drive_send_log_sense(sas_physical_drive, packet);

    return FBE_STATUS_PENDING;
}
/*********************************************
 * end fbe_sas_physical_drive_get_log_pages()
 ********************************************/

/*!****************************************************************
 * fbe_sas_physical_drive_mode_select()
 ******************************************************************
 * @brief
 *  This function initiates a mode select
 *
 * @param sas_physical_drive - The SAS physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/01/2013  Wayne Garrett  - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_sas_physical_drive_mode_select_usurper(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;

    fbe_base_physical_drive_customizable_trace( (fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                "%s entry.\n", __FUNCTION__);

    /* first do a mode sense.  this will compare against the additional overrides and if there is a difference
       a mode select will be sent. */
    sas_physical_drive->sas_drive_info.use_additional_override_tbl = FBE_TRUE;

    status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)sas_physical_drive, 
                                    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_MODE_PAGES);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return status;
}
/*!**************************************************************
 * @fn fbe_sas_physical_drive_read_long(fbe_sas_physical_drive_t* sas_physical_drive,
 *                                      fbe_packet_t* packet)
 ****************************************************************
 * @brief
 *  This function sends the read long command to executor.
 *
 * @param sas_physical_drive - Pointer to the drive object 
 * @param packet - The packet requesting this operation..
 *
 * @return status - success of failure.
 *
 * HISTORY:
 **08/21/2013: Dipak Patel - Ported from Joe B.'s code on R31 code base
 ****************************************************************/
fbe_status_t
fbe_sas_physical_drive_read_long(fbe_sas_physical_drive_t* sas_physical_drive, fbe_packet_t* packet)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_create_read_uncorrectable_t*  read_uncor_info = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    
    /* Check that the buffer we are using to specify the lba and also to specify the 
       size of the transfer is valid (not NULL) */
    fbe_payload_control_get_buffer(control_operation, &read_uncor_info);
    if(read_uncor_info == NULL)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Also check that the buffer we are using to specify the lba and also to specify the 
       size of the transfer is of the proper length. */ 
    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_physical_drive_mgmt_create_read_uncorrectable_t))
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Set the completion function */
    status = fbe_transport_set_completion_function(packet, fbe_sas_physical_drive_read_long_completion, sas_physical_drive);
    
    /* Send to executer for execution. */
    status = fbe_sas_physical_drive_send_read_long(sas_physical_drive, packet);
    
    return FBE_STATUS_PENDING;
    
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_read_long_completion(fbe_packet_t * packet,
 *                                                 fbe_packet_completion_context_t context)
 ****************************************************************
 * @brief
 *  This is a completion function for fbe_sas_physical_drive_read_long().
 *
 * @param packet - completion packet.
 * @param context - context of the completion callback.
 *
 * @return status - success of failure.
 *
 * HISTORY:
 **08/21/2013: Dipak Patel - Ported from Joe B.'s code on R31 code base
 ****************************************************************/
fbe_status_t
fbe_sas_physical_drive_read_long_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    
    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_write_long(fbe_sas_physical_drive_t* sas_physical_drive,
 *                                       fbe_packet_t* packet)
 ****************************************************************
 * @brief
 *  This function sends the write long command to executor.
 *
 * @param sas_physical_drive - Pointer to the drive object 
 * @param packet - The packet requesting this operation..
 *
 * @return status - success of failure.
 *
 * HISTORY:
 **08/21/2013: Dipak Patel - Ported from Joe B.'s code on R31 code base
 ****************************************************************/
fbe_status_t
fbe_sas_physical_drive_write_long(fbe_sas_physical_drive_t* sas_physical_drive, fbe_packet_t* packet)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_create_read_uncorrectable_t*  read_uncor_info = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    
    /* Check that the buffer we are using to specify the lba and also to specify the 
    size of the transfer is valid (not NULL) */
    fbe_payload_control_get_buffer(control_operation, &read_uncor_info);
    if(read_uncor_info == NULL)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Also check that the buffer we are using to specify the lba and also to specify the 
    size of the transfer is of the proper length. */    
    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_physical_drive_mgmt_create_read_uncorrectable_t))
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Set the completion function */
    status = fbe_transport_set_completion_function(packet, fbe_sas_physical_drive_write_long_completion, sas_physical_drive);
    
    /* Send to executer for execution. */
    status = fbe_sas_physical_drive_send_write_long(sas_physical_drive, packet);
    
    return FBE_STATUS_PENDING;  
    
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_write_long_completion(fbe_packet_t * packet,
 *                                                  fbe_packet_completion_context_t context)
 ****************************************************************
 * @brief
 *  This is a completion function for fbe_sas_physical_drive_write_long().
 *
 * @param packet - completion packet.
 * @param context - context of the completion callback.
 *
 * @return status - success of failure.
 *
 * HISTORY:
 **08/21/2013: Dipak Patel - Ported from Joe B.'s code on R31 code base
 ****************************************************************/
fbe_status_t
fbe_sas_physical_drive_write_long_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    
    /* Get control status and qualifier */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    return FBE_STATUS_OK;
}

/*!****************************************************************
 * fbe_sas_physical_drive_usurper_set_enhanced_queuing_timer()
 ******************************************************************
 * @brief
 *  This function processes usurper to set enhanced queuing timer.
 *
 * @param sas_physical_drive - The SAS physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_physical_drive_usurper_set_enhanced_queuing_timer(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_set_enhanced_queuing_timer_t * set_timer = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_u32_t lpq_timer_ms, hpq_timer_ms;
    fbe_bool_t timer_changed;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Check buffer in control payload */
    fbe_payload_control_get_buffer(control_operation, &set_timer);
    if(set_timer == NULL){
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_physical_drive_mgmt_set_enhanced_queuing_timer_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lpq_timer_ms = set_timer->lpq_timer_ms;
    hpq_timer_ms = set_timer->hpq_timer_ms;
    if ((lpq_timer_ms == ENHANCED_QUEUING_TIMER_MS_INVALID) || (hpq_timer_ms == ENHANCED_QUEUING_TIMER_MS_INVALID)) {
        /* Get timer from DCS. */
        status = fbe_sas_physical_drive_get_queuing_info_from_DCS(sas_physical_drive, &lpq_timer_ms, &hpq_timer_ms);
        /* No record found from DCS. We are done here. */
        if (status != FBE_STATUS_OK) {
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
    }

    /* Set timer and start setting mode pages. */
    status = fbe_sas_physical_drive_set_enhanced_queuing_timer(sas_physical_drive, lpq_timer_ms, hpq_timer_ms, &timer_changed);
    if (timer_changed) {        
        sas_physical_drive->sas_drive_info.use_additional_override_tbl = FBE_FALSE;
        status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                        (fbe_base_object_t*)sas_physical_drive, 
                                        FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_MODE_PAGES);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!****************************************************************
 * fbe_sas_physical_drive_usurper_sanitize_drive()
 ******************************************************************
 * @brief
 *  This function securely erases (sanitizes) an SSD, including all
 *  the over-provisioned areas.
 *
 * @param sas_physical_drive - The SAS physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @version
 *  07/01/2014 : Wayne Garrett  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_sas_physical_drive_usurper_sanitize_drive(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    /* Only the sas_flash_drive subclass should be called for this cmd.   This subclass represents all sas HDDs.
     */

    fbe_base_physical_drive_customizable_trace( (fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                                "%s Failure. Only supported for flash drives.\n", __FUNCTION__);

    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
    fbe_transport_complete_packet(packet);    
    return FBE_STATUS_GENERIC_FAILURE;
      
}


static fbe_status_t fbe_sas_physical_drive_usurper_get_sanitize_status(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    /* Only the sas_flash_drive subclass should be called for this cmd.   This subclass represents all sas HDDs.
     */
    fbe_base_physical_drive_customizable_trace( (fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                                "%s Failure. Only supported for flash drives.\n", __FUNCTION__);

    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_GENERIC_FAILURE;
}


/*!**************************************************************
 * @fn fbe_sas_physical_drive_get_rla_abort_required()
 ****************************************************************
 * @brief
 *  This function gets the SAS physical drive's state that indicates
 *  whether or not Read Look Ahead (RLA) abort is required.
 *
 * @param sas_physical_drive - The drive on which to get its RLA abort required state
 * @param packet - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  07/01/2014 - Created. Darren Insko
 *
 ****************************************************************/
static fbe_status_t 
fbe_sas_physical_drive_get_rla_abort_required(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_bool_t * b_is_required_p = NULL;
    fbe_payload_control_buffer_length_t len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &b_is_required_p);
    if (b_is_required_p == NULL)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_bool_t)){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s: %X len invalid\n", __FUNCTION__, len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *b_is_required_p = sas_physical_drive->sas_drive_info.rla_abort_required;

    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/*****************************************************
 * end fbe_sas_physical_drive_get_rla_abort_required()
 *****************************************************/


static fbe_status_t fbe_sas_physical_drive_usurper_format_block_size(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_set_format_block_size_t * block_size = NULL;
    fbe_payload_control_buffer_length_t len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Check buffer in control payload */
    fbe_payload_control_get_buffer(control_operation, &block_size);
    if(block_size == NULL){
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(*block_size)){
        fbe_base_physical_drive_customizable_trace( (fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                                    "%s Invalid len. \n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* only valid block size supported are 4160 and 520 */
    if ( (*block_size) != 4160 && (*block_size) != 520)
    {
        fbe_base_physical_drive_customizable_trace( (fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                                    "%s Invalid block size %d.\n", __FUNCTION__, *block_size);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_sas_physical_drive_format_block_size(sas_physical_drive, packet, *block_size);
    return FBE_STATUS_PENDING;
}

