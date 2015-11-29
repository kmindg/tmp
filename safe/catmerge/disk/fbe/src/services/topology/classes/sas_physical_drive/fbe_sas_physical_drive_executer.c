#include "fbe/fbe_types.h"
#include "fbe_traffic_trace.h"
#include "sas_physical_drive_private.h"
#include "fbe_transport_memory.h"
#include "fbe_scsi.h"
#include "base_physical_drive_private.h"
#include "fbe_traffic_trace.h"

static fbe_status_t sas_physical_drive_send_block_cdb_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_resend_block_cdb_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/* Forward declarations */
static fbe_status_t sas_physical_drive_send_inquiry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_discover_drive_type_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_send_tur_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_send_tur_to_abort_rla_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_start_unit_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_mode_sense_10_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_mode_sense_10_saved_pages_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_mode_select_10_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_mode_sense_page_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_read_capacity_completion_10(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_read_capacity_completion_16(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static void         sas_physical_drive_handle_read_capacity_status(fbe_sas_physical_drive_t *sas_physical_drive, fbe_sas_drive_status_t validate_status, fbe_payload_control_operation_t *payload_control_operation);
static fbe_status_t sas_physical_drive_edge_state_change_event_entry(fbe_sas_physical_drive_t * sas_physical_drive, fbe_event_context_t event_context);
static fbe_status_t sas_physical_drive_check_attributes(fbe_sas_physical_drive_t * sas_physical_drive);

static fbe_status_t sas_physical_drive_send_reserve_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_send_reassign_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_send_dc_reassign_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_send_release_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_reset_device_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_download_device_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_set_power_save_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_spin_down_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_send_RDR_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_get_permission_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_disk_collect_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_dump_disk_log_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_send_read_disk_blocks_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_send_smart_dump_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_send_vpd_inquiry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sas_physical_drive_process_vpd_inquiry(fbe_sas_physical_drive_t * sas_physical_drive, fbe_u8_t *vpd_inq);
static fbe_status_t fbe_sas_physical_drive_send_qst_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_bool_t   sas_pdo_get_dieh_threshold_rec_on_error(fbe_sas_physical_drive_t * sas_physical_drive, fbe_drive_configuration_handle_t handle, fbe_payload_cdb_operation_t * cdb_operation, fbe_drive_configuration_record_t ** threshold_rec);
static fbe_status_t sas_physical_drive_send_log_sense_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_physical_drive_mode_select_10_block_size_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sas_physical_drive_send_format_block_size(fbe_sas_physical_drive_t* sas_physical_drive, fbe_packet_t* packet);
static fbe_status_t sas_physical_drive_send_format_block_size_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);




fbe_status_t 
fbe_sas_physical_drive_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_status_t status;

    sas_physical_drive = (fbe_sas_physical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* Handle the event we have received. */
    switch (event_type)
    {
        case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
            status = sas_physical_drive_edge_state_change_event_entry(sas_physical_drive, event_context);
            break;
        default:
            status = fbe_base_physical_drive_event_entry(object_handle, event_type, event_context);
            break;
    }

    return status;
}
 
static fbe_status_t 
sas_physical_drive_edge_state_change_event_entry(fbe_sas_physical_drive_t * sas_physical_drive, fbe_event_context_t event_context)
{
    fbe_path_state_t path_state;
    fbe_status_t status;
    fbe_transport_id_t transport_id;
    fbe_object_handle_t object_handle;
    fbe_path_attr_t path_attr;

    /* Get the transport type from the edge pointer. */
    fbe_base_transport_get_transport_id((fbe_base_edge_t *) event_context, &transport_id);

    /* Process ssp edge state change only */
    if (transport_id != FBE_TRANSPORT_ID_SSP) {
        object_handle = fbe_base_pointer_to_handle((fbe_base_t *) sas_physical_drive);
        status = fbe_base_physical_drive_event_entry(object_handle, FBE_EVENT_TYPE_EDGE_STATE_CHANGE, event_context);
        return status;
    }

    /* Get ssp edge state */
    status = fbe_ssp_transport_get_path_state(&sas_physical_drive->ssp_edge, &path_state);
    status = fbe_ssp_transport_get_path_attributes(&sas_physical_drive->ssp_edge, &path_attr);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t *)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                   "%s: SSP path_state=%d, path_attr=0x%x\n",__FUNCTION__, path_state, path_attr);

    /* To support Single Loop Failure (SLF), PDO needs to notify PVD that there is a link
       issue.  If there is a link issue the IO will be redirected to other SP.  If there is
       a link issue then our path state becomes unhealthy, such as when a drive logs out 
       (FBE_SSP_PATH_ATTR_CLOSED) or port object transitions out of Ready state, which includes
       Activate=DISABLED, Fail=BROKEN, or Destory=GONE.  -weg
    */
    if ((path_state == FBE_PATH_STATE_ENABLED) && (path_attr & FBE_SSP_PATH_ATTR_CLOSED) ||
        (path_state == FBE_PATH_STATE_DISABLED) ||
        (path_state == FBE_PATH_STATE_BROKEN) ||
        (path_state == FBE_PATH_STATE_GONE))
    {
        /* Notify monitor to set link_fault */
        fbe_base_pdo_set_flags_under_lock(&sas_physical_drive->base_physical_drive, FBE_PDO_FLAGS_SET_LINK_FAULT);        
    }

    switch(path_state){
        case FBE_PATH_STATE_ENABLED:
            status = sas_physical_drive_check_attributes(sas_physical_drive);
            break;
        case FBE_PATH_STATE_GONE:
            status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const,
                                            (fbe_base_object_t*)sas_physical_drive,
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
            break;
        case FBE_PATH_STATE_DISABLED:
            status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const,
                                            (fbe_base_object_t*)sas_physical_drive,
                                            FBE_BASE_DISCOVERED_LIFECYCLE_COND_PROTOCOL_EDGES_NOT_READY);
            break;
        case FBE_PATH_STATE_INVALID:
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

    }

    return status;
}

static fbe_status_t 
sas_physical_drive_check_attributes(fbe_sas_physical_drive_t * sas_physical_drive)
{
    fbe_status_t status;
    fbe_path_attr_t path_attr;

    status = fbe_ssp_transport_get_path_attributes(&sas_physical_drive->ssp_edge, &path_attr);
    if(path_attr & FBE_SSP_PATH_ATTR_CLOSED) {
        status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const,
                                        (fbe_base_object_t*)sas_physical_drive,
                                        FBE_BASE_DISCOVERED_LIFECYCLE_COND_PROTOCOL_EDGES_NOT_READY);
    }

    fbe_lifecycle_reschedule(&fbe_sas_physical_drive_lifecycle_const,
                            (fbe_base_object_t *) sas_physical_drive,
                            (fbe_lifecycle_timer_msec_t) 0); /* Immediate reschedule */

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_physical_drive_handle_block_io(fbe_sas_physical_drive_t *sas_physical_drive, fbe_packet_t * packet)
{
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t *)sas_physical_drive;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_block_operation_opcode_t operation_opcode;

    payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);
    
    if(block_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_get_block_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_block_get_opcode(block_operation, &operation_opcode);
    
    /* IO could have just come off our waiting queue and/or could be a RAID retry.  
       Check service time before proceeding.
    */
    if (fbe_base_drive_test_and_set_health_check(base_physical_drive, packet))
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                       "%s: initiated health check. pkt: %p\n",__FUNCTION__, packet);

        /* Fail this IO.  Returning NOT retryable since we know HC will cause
           us to change our state and edge go disabled.  This is the prefered way per Rob Foley.
        */                
        fbe_payload_block_set_status(block_operation, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }


    if ((operation_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_IDENTIFY) &&
        (operation_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE)) {
        return sas_physical_drive_process_send_block_cdb(sas_physical_drive, packet);
    }
    else if (operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_IDENTIFY)
    {

        fbe_sg_element_t *identity = NULL;        

        fbe_base_physical_drive_info_t * drive_info = NULL;
        
        fbe_payload_ex_get_sg_list(payload, &identity, NULL);

        if (identity != NULL && identity->address != NULL)
        {            
            drive_info = &((fbe_base_physical_drive_t *)sas_physical_drive)->drive_info;

            /*
            fbe_copy_memory (identity, drive_info->vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE);
            identity[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE] = 0;
            identity += (FBE_SCSI_INQUIRY_VENDOR_ID_SIZE + 1);
            */
            fbe_copy_memory (identity->address, drive_info->serial_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
            ((char*)identity->address)[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE] = 0;
        }
        else
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                                        FBE_TRACE_LEVEL_ERROR,
                                                        "%s, sg_list or sg_list_address is null\n",
                                                        __FUNCTION__);

            fbe_transport_set_status(packet, FBE_STATUS_FAILED, 0);
        
            fbe_transport_complete_packet(packet);

            return FBE_STATUS_FAILED;
        }
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }
    else // (operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE)
    {
        return fbe_base_physical_drive_handle_negotiate_block_size(base_physical_drive, packet);
    }
}


fbe_status_t 
fbe_sas_physical_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_transport_id_t transport_id;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sas_physical_drive_t *sas_physical_drive;
    sas_physical_drive = (fbe_sas_physical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    /* First we need to figure out to what transport this packet belong. */
    /* We may have packet from RDGEN and the base_edge will be NULL - it is block transport packet */
    if(packet->base_edge != NULL){
        fbe_transport_get_transport_id(packet, &transport_id);
    } else {
        transport_id = FBE_TRANSPORT_ID_BLOCK; /* Discovery packets will always have an edge */
    }

    switch (transport_id)
    {
        case FBE_TRANSPORT_ID_DISCOVERY:
            status = fbe_base_physical_drive_io_entry(object_handle, packet);
            break;
        case FBE_TRANSPORT_ID_BLOCK:
            /* status = fbe_sas_physical_drive_handle_block_io(sas_physical_drive, packet); */

            status = fbe_base_physical_drive_bouncer_entry((fbe_base_physical_drive_t * )sas_physical_drive, packet);
            if(status == FBE_STATUS_OK){ /* Keep stack small */
                status = fbe_sas_physical_drive_handle_block_io(sas_physical_drive, packet);
            }

            break;
        default:
            break;
    };

    return status;
}

fbe_status_t 
fbe_sas_physical_drive_send_inquiry(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);
    if(payload_control_operation == NULL)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s,  NULL control operation, packet: %64p.\n",
                                __FUNCTION__, packet);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = FBE_SCSI_INQUIRY_DATA_SIZE;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, FBE_SCSI_INQUIRY_DATA_SIZE);

    fbe_payload_cdb_build_inquiry(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_INQUIRY_TIMEOUT);

    fbe_transport_set_completion_function(packet, sas_physical_drive_send_inquiry_completion, sas_physical_drive);

    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);
    return status;
}


/*
 * This is the completion function for the inquiry packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_physical_drive_send_inquiry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_sas_drive_status_t validate_status = FBE_SAS_DRIVE_STATUS_OK;
    fbe_physical_drive_information_t * drive_info;
    fbe_u32_t transferred_count;     

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_control_get_buffer(payload_control_operation, &drive_info);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else 
    {
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        if (error == FBE_SCSI_XFERCOUNTNOTZERO)
        {
            fbe_payload_cdb_get_transferred_count(payload_cdb_operation, &transferred_count);
            if (transferred_count >= FBE_SCSI_INQUIRY_DATA_MINIMUM_SIZE)
            {
                /* Underrun is not an error */
                error = 0;
            }
        }
        if (sas_physical_drive_set_control_status(error, payload_control_operation) == FBE_PAYLOAD_CONTROL_STATUS_OK)
        {
            validate_status = fbe_sas_physical_drive_set_dev_info(sas_physical_drive, (fbe_u8_t *)(sg_list[0].address), drive_info);
            if (validate_status == FBE_SAS_DRIVE_STATUS_INVALID) 
            {
#ifdef FBE_HANDLE_UNKNOWN_TLA_SUFFIX
                if (fbe_dcs_param_is_enabled(FBE_DCS_IGNORE_INVALID_IDENTITY))
                {
                    fbe_pdo_maintenance_mode_set_flag(&sas_physical_drive->base_physical_drive.maintenance_mode_bitmap, 
                                                      FBE_PDO_MAINTENANCE_FLAG_INVALID_IDENTITY);        
                }
                else
#endif
                {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                          FBE_TRACE_LEVEL_WARNING,
                                          "%s Invalid packet status: 0x%X\n",
                                          __FUNCTION__, status);

                    fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SET_DEV_INFO);
                    /* Clear the maintenance flag if the dcs mode is not set */
                    fbe_pdo_maintenance_mode_clear_flag(&sas_physical_drive->base_physical_drive.maintenance_mode_bitmap, 
                                                      FBE_PDO_MAINTENANCE_FLAG_INVALID_IDENTITY);
                    /* We need to set the BO:FAIL lifecycle condition */
                    status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                                    FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                              FBE_TRACE_LEVEL_ERROR,
                                              "%s can't set BO:FAIL condition, status: 0x%X\n",
                                              __FUNCTION__, status);
                
                    }
                }
            }
            else if (validate_status == FBE_SAS_DRIVE_STATUS_NEED_RETRY) 
            {
                fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_payload_control_set_status_qualifier(payload_control_operation, FBE_SAS_DRIVE_STATUS_HARD_ERROR);
            }
            else /* FBE_SAS_DRIVE_STATUS_OK */
            {
                /* Clear the maintenance flag when the drive is not invalid */
                fbe_pdo_maintenance_mode_clear_flag(&sas_physical_drive->base_physical_drive.maintenance_mode_bitmap, 
                                                    FBE_PDO_MAINTENANCE_FLAG_INVALID_IDENTITY);

                if (drive_info->drive_parameter_flags & FBE_PDO_FLAG_DRIVE_TYPE_UNKNOWN)
                {                
                    /* Using the same packet to send a mode sense.  This depends on the cdb and sg_list that was previously allocated 
                       for inquiry. */
                    fbe_payload_cdb_build_mode_sense_page(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, FBE_MS_GEOMETRY_PG04);

                    fbe_transport_set_completion_function(packet, sas_physical_drive_discover_drive_type_completion, sas_physical_drive);

                    fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet); 

                    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
                }                
            }
        }
    }

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    fbe_transport_release_buffer(sg_list);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn sas_physical_drive_discover_drive_type_completion()
 ****************************************************************
 * @brief
 *  This is a completion function for mode page 4 (Geometry Page) for determining the 
 *  drive subclass type (SSD or HDD).  This is determined by looking at the RPM.  Zero will
 *  indicate an SSD,  where non-zero is an HDD.  The default spindown attributes are also set
 *  here.  Spindown is enabled for HDD and disabled for SSD.
 *
 * @author
 *  09/24/2015 - Created. Wayne Garrett
 *
 ****************************************************************/
static fbe_status_t 
sas_physical_drive_discover_drive_type_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;
    fbe_scsi_error_code_t error = 0;
    fbe_u8_t* buff_ptr = NULL;
    fbe_u16_t drive_RPM;
    fbe_physical_drive_information_t * drive_info = NULL;
    fbe_path_attr_t path_attr = 0;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_control_get_buffer(payload_control_operation, &drive_info);


    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);    
    buff_ptr = (fbe_u8_t *) sg_list[0].address;

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
    }
    else {
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        if (error == FBE_SCSI_XFERCOUNTNOTZERO)
        {
            /* Underrun is not an error in MODE SENSE */
            error = 0;
        }

        if (sas_physical_drive_set_control_status(error, payload_control_operation) == FBE_PAYLOAD_CONTROL_STATUS_OK)
        {
            buff_ptr += FBE_SCSI_MODE_SENSE_HEADER_LENGTH + 20;
            drive_RPM = (fbe_u16_t)(buff_ptr[0]) << 8 | (fbe_u16_t)(buff_ptr[1]);

            drive_info->drive_parameter_flags &= ~FBE_PDO_FLAG_DRIVE_TYPE_UNKNOWN;

            if (drive_RPM == 0)  /* This is a Flash drive.  Assuming MLC.  By default spindown is disabled. */
            {
                //fbe_base_object_change_class_id((fbe_base_object_t *) sas_physical_drive, FBE_CLASS_ID_SAS_FLASH_DRIVE);
                drive_info->drive_parameter_flags |= (FBE_PDO_FLAG_MLC_FLASH_DRIVE);                
                drive_info->spin_down_qualified = FBE_FALSE;
            } 
            else  /* This is an HDD.  By default they support spindown. */
            {
                status = fbe_base_discovered_get_path_attributes((fbe_base_discovered_t *)sas_physical_drive, &path_attr);
                if (status == FBE_STATUS_OK && (path_attr & FBE_DISCOVERY_PATH_ATTR_POWERSAVE_SUPPORTED))
                {
                    drive_info->spin_down_qualified = FBE_TRUE;
                }
            }
        }
    }

    fbe_transport_release_buffer(sg_list);

    /* Release payload and buffer. */
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);

    return FBE_STATUS_OK;

}


fbe_status_t 
fbe_sas_physical_drive_send_tur(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_status_t status;
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    
    fbe_payload_cdb_build_tur(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);
    
    fbe_transport_set_completion_function(packet, sas_physical_drive_send_tur_completion, sas_physical_drive);
 
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);

    return status;
}

/*
 * This is the completion function for the tur packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_physical_drive_send_tur_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;
    fbe_scsi_error_code_t error = 0;
    
    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    
    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
    }
    else
    {
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);

        if (sas_physical_drive_set_control_status(error, payload_control_operation) == FBE_PAYLOAD_CONTROL_STATUS_OK)
        {
            fbe_pdo_maintenance_mode_clear_flag(&sas_physical_drive->base_physical_drive.maintenance_mode_bitmap, FBE_PDO_MAINTENANCE_FLAG_SANITIZE_STATE);
        }
        else  /* failure */
        {
            fbe_payload_control_status_qualifier_t control_status_qualifier;
            fbe_payload_control_get_status_qualifier(payload_control_operation, &control_status_qualifier);

            if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_SANITIZE_IN_PROGRESS ||
                control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_SANITIZE_NEED_RESTART)
            {
                /* Sanitization errors indicate that drive has to be spun-up. The following utility functtion will mark 
                   this drive as maintenance needed and then change control status to OK so PDO goes Ready. 
                */               
                sas_pdo_process_sanitize_error_for_conditions(sas_physical_drive, payload_control_operation, payload_cdb_operation);

                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                           "%s In Sanitize state.  Allowing TUR to pass\n", __FUNCTION__);  
            }
        }
    }

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_send_tur_to_abort_rla()
 ****************************************************************
 * @brief
 *  This function sends a Test Unit Ready (TUR) outside the context
 *  of the monitor to abort Read Look Aheads (RLAs)
 *
 * @param sas_physical_drive - The drive on which RLAs will be aborted by the TUR.
 * @param verify_packet - The packet that was used to handle VERIFY CDB is being reused.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  07/01/2014 - Created. Darren Insko
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_physical_drive_send_tur_to_abort_rla(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * verify_packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_status_t status;
    
    payload = fbe_transport_get_payload_ex(verify_packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    
    fbe_payload_cdb_build_tur(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);
    
    fbe_transport_set_completion_function(verify_packet, sas_physical_drive_send_tur_to_abort_rla_completion, sas_physical_drive);
 
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, verify_packet);

    return status;
}
/****************************************************
 * end fbe_sas_physical_drive_send_tur_to_abort_rla()
 ****************************************************/

/*!**************************************************************
 * @fn fbe_sas_physical_drive_send_tur_to_abort_rla_completion()
 ****************************************************************
 * @brief
 *  This is a completion function for fbe_sas_physical_drive_send_tur_to_abort_rla()
 *
 * @param verify_packet - completion packet that's being reused from handling the VERIFY CDB.
 * @param context - context of the completion callback.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  07/01/2014 - Created. Darren Insko
 *
 ****************************************************************/
static fbe_status_t 
sas_physical_drive_send_tur_to_abort_rla_completion(fbe_packet_t * verify_packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    
    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    
    payload = fbe_transport_get_payload_ex(verify_packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    
    /* Don't do any error checking at all.  Just release the CDB operation. */
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);

    return FBE_STATUS_OK;
}
/***************************************************************
 * end fbe_sas_physical_drive_send_tur_to_abort_rla_completion()
 ***************************************************************/

fbe_status_t 
fbe_sas_physical_drive_start_unit(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_status_t status;
    fbe_object_id_t my_object_id;

    payload = fbe_transport_get_payload_ex(packet);

    fbe_base_object_get_object_id((fbe_base_object_t *)sas_physical_drive, &my_object_id);
    
    payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);

    fbe_payload_discovery_build_spin_up(payload_discovery_operation, my_object_id);

    fbe_transport_set_completion_function(packet, sas_physical_drive_start_unit_completion, sas_physical_drive);
  
    /* We are sending discovery packet via discovery edge */
    status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *) sas_physical_drive, packet);

    return status;
}

/*
 * This is the completion function for the start_stop_unit packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_physical_drive_start_unit_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_status_t discovery_status;
    fbe_status_t status;
    fbe_payload_control_operation_t * control_operation = NULL;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_discovery_get_status(payload_discovery_operation, &discovery_status);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
    }
    else
    {
        if(discovery_status != FBE_PAYLOAD_DISCOVERY_STATUS_OK) { 
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        } else {
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        }
    }

    fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_physical_drive_mode_sense_10(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_status_t status;
    fbe_bool_t get_default_pages = (sas_physical_drive->sas_drive_info.ms_retry_count == 0);

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);
    
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n", __FUNCTION__);
        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = FBE_SCSI_MAX_MODE_SENSE_10_SIZE;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);  /* address starts after the last sg_list element */

    sg_list[1].count = 0;
    sg_list[1].address = NULL;  

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, FBE_SCSI_MAX_MODE_SENSE_10_SIZE);

    fbe_payload_cdb_build_mode_sense_10(payload_cdb_operation, 
                                        FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, 
                                        get_default_pages, 
                                        FBE_SCSI_MAX_MODE_SENSE_10_SIZE);

    fbe_transport_set_completion_function(packet, sas_physical_drive_mode_sense_10_completion, sas_physical_drive);
    
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet); /* for now hard coded control edge */

    return status;
}

/* This is the completion function for the mode_sense packet sent to a physical_drive. */
static fbe_status_t 
sas_physical_drive_mode_sense_10_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;
    fbe_sas_drive_status_t validate_status = FBE_SAS_DRIVE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
   
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    
    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
    }
    else {
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        if (error == FBE_SCSI_XFERCOUNTNOTZERO)
        {
            /* Underrun is not an error in MODE SENSE */
            error = 0;
        }

        if (sas_physical_drive_set_control_status(error, payload_control_operation) == FBE_PAYLOAD_CONTROL_STATUS_OK)
        {
            validate_status = fbe_sas_physical_drive_process_mode_sense_10(sas_physical_drive, sg_list);
            if (validate_status == FBE_SAS_DRIVE_STATUS_INVALID)
            {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                      FBE_TRACE_LEVEL_INFO,
                                      "%s Invalid mode sense: pck status 0x%X\n",
                                      __FUNCTION__, status);

                fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_PROCESS_MODE_SENSE);

                status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                                (fbe_base_object_t*)sas_physical_drive, 
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
                if (status != FBE_STATUS_OK) {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          "%s can't set BO:FAIL condition, status: 0x%X\n",
                                          __FUNCTION__, status);
                }
             }

            /* Read saved mode pages */
            if ((validate_status == FBE_SAS_DRIVE_STATUS_OK) && (sas_physical_drive->sas_drive_info.ms_retry_count == 0))
            {
                fbe_payload_cdb_build_mode_sense_10(payload_cdb_operation, 
                                                    FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, 
                                                    FBE_FALSE,                      /* Get saved pages */
                                                    FBE_SCSI_MAX_MODE_SENSE_10_SIZE);

                fbe_transport_set_completion_function(packet, sas_physical_drive_mode_sense_10_saved_pages_completion, sas_physical_drive);
                fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
        }

    }

    fbe_transport_release_buffer(sg_list);

    /* Release payload and buffer. */
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
   
    return FBE_STATUS_OK;
}

static fbe_status_t 
sas_physical_drive_print_mode_pages(fbe_sas_physical_drive_t * sas_physical_drive, fbe_u16_t len, fbe_u8_t *data)
{
    fbe_u16_t i, max = (len+15)/16;
    fbe_u8_t *ptr = (fbe_u8_t *)data;

    for (i = 0; i<max; i++)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_INFO, "%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n",
                                *ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5), *(ptr+6), *(ptr+7),
                                *(ptr+8), *(ptr+9), *(ptr+10), *(ptr+11), *(ptr+12), *(ptr+13), *(ptr+14), *(ptr+15));
        ptr += 16;
    }

    return FBE_STATUS_OK;
}

/* This is the completion function for the mode_sense of saved pages. */
static fbe_status_t 
sas_physical_drive_mode_sense_10_saved_pages_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;
    fbe_scsi_error_code_t error = 0;
    fbe_u8_t *mp_ptr = NULL;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
   
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    mp_ptr = (fbe_u8_t *)sg_list[0].address;
    
    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
    }
    else {
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        if (error == FBE_SCSI_XFERCOUNTNOTZERO)
        {
            /* Underrun is not an error in MODE SENSE */
            error = 0;
        }

        if (sas_physical_drive_set_control_status(error, payload_control_operation) == FBE_PAYLOAD_CONTROL_STATUS_OK)
        {
            fbe_u16_t data_length = ((fbe_u16_t)(mp_ptr[0] << 8)) | ((fbe_u16_t)mp_ptr[1]);

            /* The first 4 bytes in mode page are already wiped out */
            if (((data_length + 2) == sas_physical_drive->sas_drive_info.ms_size) &&
				fbe_sas_physical_drive_compare_mode_pages(sas_physical_drive->sas_drive_info.ms_pages, mp_ptr))
            {
                sas_physical_drive->sas_drive_info.do_mode_select = FALSE;
                sas_physical_drive->sas_drive_info.ms_retry_count = 0;

                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                                           FBE_TRACE_LEVEL_INFO,
                                                           "%s Skip mode select!!!\n",
                                                           __FUNCTION__);
            }
#if 0
            else
            {
                sas_physical_drive_print_mode_pages(sas_physical_drive, data_length + 2, mp_ptr);
                sas_physical_drive_print_mode_pages(sas_physical_drive, sas_physical_drive->sas_drive_info.ms_size, sas_physical_drive->sas_drive_info.ms_pages);
            }
#endif
        }
    }

    fbe_transport_release_buffer(sg_list);

    /* Release payload and buffer. */
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
   
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_physical_drive_mode_select_10(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);

    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n", __FUNCTION__);
        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = sas_physical_drive->sas_drive_info.ms_size;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);
    memcpy(sg_list[0].address, sas_physical_drive->sas_drive_info.ms_pages, sas_physical_drive->sas_drive_info.ms_size);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, sas_physical_drive->sas_drive_info.ms_size);

    fbe_payload_cdb_build_mode_select_10(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, sas_physical_drive->sas_drive_info.ms_size);

    fbe_transport_set_completion_function(packet, sas_physical_drive_mode_select_10_completion, sas_physical_drive);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                              "%s Sending mode select!!!\n", __FUNCTION__);
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet); /* for now hard coded control edge */

    return status;
}

/* This is the completion function for the mode_select packet sent to a physical_drive. */
static fbe_status_t 
sas_physical_drive_mode_select_10_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;
    fbe_scsi_error_code_t error = 0;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
    }
    else
    {
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        sas_physical_drive_set_control_status(error, payload_control_operation);
    }

    fbe_transport_release_buffer(sg_list);

    /* Release payload and buffer. */
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
   
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_physical_drive_mode_sense_page(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_status_t status;
    fbe_u8_t page;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);
    
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n", __FUNCTION__);
        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = FBE_SCSI_MAX_MODE_SENSE_SIZE;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;  

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, FBE_SCSI_MAX_MODE_SENSE_SIZE);

    fbe_sas_physical_drive_get_current_page(sas_physical_drive, &page, NULL);
    fbe_payload_cdb_build_mode_sense_page(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, page);

    fbe_transport_set_completion_function(packet, sas_physical_drive_mode_sense_page_completion, sas_physical_drive);
    
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet); /* for now hard coded control edge */

    return status;
}


/* This is the completion function for geting a single mode page from the SAS physical drive. */
static fbe_status_t 
sas_physical_drive_mode_sense_page_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;
    fbe_scsi_error_code_t error = 0;
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    
    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
    }
    else
    {
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        if (error == FBE_SCSI_XFERCOUNTNOTZERO)
        {
            /* Underrun is not an error in MODE SENSE */
            error = 0;
        }

        if (sas_physical_drive_set_control_status(error, payload_control_operation) == FBE_PAYLOAD_CONTROL_STATUS_OK)
        {
            fbe_sas_physical_drive_process_mode_page_attributes(sas_physical_drive, sg_list);
        }
    }

    fbe_transport_release_buffer(sg_list);

    /* Release payload and buffer. */
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
   
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_physical_drive_read_capacity_10(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);
   
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n", __FUNCTION__);
        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = FBE_SCSI_READ_CAPACITY_DATA_SIZE;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, FBE_SCSI_READ_CAPACITY_DATA_SIZE);

    fbe_payload_cdb_build_read_capacity_10(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_READ_CAPACITY_TIMEOUT);

    fbe_transport_set_completion_function(packet, sas_physical_drive_read_capacity_completion_10, sas_physical_drive);

    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);

    return status;
}

/* This is the completion function for the 10 byte read_capacity command sent to a physical_drive. */
static fbe_status_t 
sas_physical_drive_read_capacity_completion_10(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_status_t status;
    fbe_scsi_error_code_t error = 0;
    fbe_sas_drive_status_t validate_status = FBE_SAS_DRIVE_STATUS_OK;
    
    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

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
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        if (sas_physical_drive_set_control_status(error, payload_control_operation) == FBE_PAYLOAD_CONTROL_STATUS_OK)
        {
            validate_status = fbe_sas_physical_drive_process_read_capacity_10(sas_physical_drive, sg_list);

            sas_physical_drive_handle_read_capacity_status(sas_physical_drive, validate_status, payload_control_operation);
        }
    }

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    fbe_transport_release_buffer(sg_list);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_physical_drive_read_capacity_16(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);
   
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n", __FUNCTION__);
        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = FBE_SCSI_READ_CAPACITY_DATA_SIZE_16;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, FBE_SCSI_READ_CAPACITY_DATA_SIZE_16);

    fbe_payload_cdb_build_read_capacity_16(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_READ_CAPACITY_TIMEOUT);

    fbe_transport_set_completion_function(packet, sas_physical_drive_read_capacity_completion_16, sas_physical_drive);

    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);

    return status;
}

/* This is the completion function for the read_capacity packet sent to a physical_drive. */
static fbe_status_t 
sas_physical_drive_read_capacity_completion_16(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_sas_drive_status_t validate_status = FBE_SAS_DRIVE_STATUS_OK;
    fbe_status_t status;
    fbe_scsi_error_code_t error = 0;


    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

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
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        if (sas_physical_drive_set_control_status(error, payload_control_operation) == FBE_PAYLOAD_CONTROL_STATUS_OK)
        {
            validate_status = fbe_sas_physical_drive_process_read_capacity_16(sas_physical_drive, sg_list);

            sas_physical_drive_handle_read_capacity_status(sas_physical_drive, validate_status, payload_control_operation);
        } 
    }

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    fbe_transport_release_buffer(sg_list);

    return FBE_STATUS_OK;
}


static void sas_physical_drive_handle_read_capacity_status(fbe_sas_physical_drive_t *sas_physical_drive, fbe_sas_drive_status_t validate_status, fbe_payload_control_operation_t *payload_control_operation)
{
    fbe_status_t status;

    if (validate_status == FBE_SAS_DRIVE_STATUS_INVALID ||
        validate_status == FBE_SAS_DRIVE_STATUS_NOT_SUPPORTED_IN_SYSTEM_SLOT) 
    {
        fbe_object_death_reason_t death_reason = FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_PROCESS_READ_CAPACITY;

        if (validate_status == FBE_SAS_DRIVE_STATUS_NOT_SUPPORTED_IN_SYSTEM_SLOT)
        {
            death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION;
        }

        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                                   FBE_TRACE_LEVEL_WARNING,
                                                   "%s drive failed: 0x%X\n",
                                                   __FUNCTION__, validate_status);

        fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, death_reason);

        /* We need to set the BO:FAIL lifecycle condition */
        status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

        if (status != FBE_STATUS_OK)
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  "%s can't set BO:DESTROY condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }        
    }
    else if (validate_status == FBE_SAS_DRIVE_STATUS_NEED_RETRY) 
    {
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_payload_control_set_status_qualifier(payload_control_operation, FBE_SAS_DRIVE_STATUS_HARD_ERROR);
    }

}


fbe_status_t 
sas_physical_drive_process_send_block_cdb(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_cdb_operation_t *   cdb_operation = NULL;
    fbe_payload_block_operation_opcode_t block_operation_opcode; 
    fbe_time_t  timeout;
    fbe_packet_priority_t packet_priority;
    fbe_time_t io_start_time;

    payload = fbe_transport_get_payload_ex(packet);

    block_operation = fbe_payload_ex_get_block_operation(payload);       
    if(block_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_get_block_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note I/O MUST be aligned to the physical block size. 
     * Except for reads, which the PDO handles aligning. 
     */
    if ((block_operation->block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) &&
        !fbe_base_physical_drive_is_io_aligned_to_block_size((fbe_base_physical_drive_t *)sas_physical_drive,
                                                            block_operation->block_size, /* Client block size */
                                                            block_operation->lba,
                                                            block_operation->block_count))
    {
        /* Report invalid request and complete packet with success.
         */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                                   FBE_TRACE_LEVEL_WARNING,
                                                   "%s packet: %p drive block size: %d\n",
                                                   __FUNCTION__, packet,
                                                   sas_physical_drive->base_physical_drive.block_size);
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   "    lba: 0x%llx blocks: 0x%llx not aligned to drive block size\n",
                                                   block_operation->lba, block_operation->block_count);
        fbe_payload_block_set_status(block_operation, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                                      FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNALIGNED_REQUEST);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    if(cdb_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_cdb_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Update start time of IO in PDO */
    io_start_time = fbe_get_time_in_us();
    fbe_payload_cdb_set_physical_drive_io_start_time(cdb_operation, io_start_time);

    fbe_payload_block_get_opcode(block_operation, &block_operation_opcode); 
    fbe_base_physical_drive_get_default_timeout((fbe_base_physical_drive_t *) sas_physical_drive, &timeout);

    fbe_transport_get_packet_priority(packet, &packet_priority);
    switch(packet_priority) {
        case FBE_PACKET_PRIORITY_INVALID:
            cdb_operation->payload_cdb_priority = FBE_PAYLOAD_CDB_PRIORITY_NONE;
            break;
        case FBE_PACKET_PRIORITY_URGENT:
            cdb_operation->payload_cdb_priority = FBE_PAYLOAD_CDB_PRIORITY_URGENT;
            break;
        case FBE_PACKET_PRIORITY_NORMAL:
            cdb_operation->payload_cdb_priority = FBE_PAYLOAD_CDB_PRIORITY_NORMAL;
            break;
        case FBE_PACKET_PRIORITY_LOW:
            cdb_operation->payload_cdb_priority = FBE_PAYLOAD_CDB_PRIORITY_LOW;
            break;
        case FBE_PACKET_PRIORITY_LAST:
            cdb_operation->payload_cdb_priority = FBE_PAYLOAD_CDB_PRIORITY_LAST;
            break;
        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Array of function pointers for converting block operations to cdb's */
    //status = sas_physical_drive->sas_drive_info.fbe_scsi_build_block_cdb[block_operation_opcode](sas_physical_drive, payload, cdb_operation, timeout);
	status = sas_physical_drive_block_to_cdb(sas_physical_drive, payload, cdb_operation);
    if (status != FBE_STATUS_OK) {
        /* If the status was bad, then return status immediately. */
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, sas_physical_drive_block_to_cdb Failed status %d\n", __FUNCTION__, status);
        fbe_payload_ex_release_cdb_operation(payload, cdb_operation);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    /* Now that we're about to issue the CDB to the drive, increment the number of IOs in progress
     * for statistics purposes.
     */
    if (sas_physical_drive->b_perf_stats_enabled)
    {
        fbe_sas_physical_drive_perf_stats_inc_num_io_in_progress(sas_physical_drive, packet->cpu_id);
    }

    payload->physical_drive_transaction.last_error_code = 0;
    base_pdo_payload_transaction_flag_init(&payload->physical_drive_transaction); 

    /* If this is a 4K drive and there is data transfer being done set the flag appropriately so that
     * we can let miniport know. This is mostly for encryption related purposes
     */
    if ((sas_physical_drive->base_physical_drive.block_size == FBE_BLOCK_SIZES_4160) &&
        (cdb_operation->payload_cdb_flags != FBE_PAYLOAD_CDB_FLAGS_NO_DATA_TRANSFER))
    {
        cdb_operation->payload_cdb_flags |= FBE_PAYLOAD_CDB_FLAGS_BLOCK_SIZE_4160;
    }

    fbe_transport_set_completion_function(packet, sas_physical_drive_send_block_cdb_completion, sas_physical_drive);
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet); 

    return status;    
}


static fbe_status_t 
sas_physical_drive_send_block_cdb_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t *)context;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_status_t return_status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_scsi_error_code_t error = 0;
    fbe_lba_t bad_lba = 0;
    fbe_u32_t sense_code = 0;
    fbe_drive_configuration_handle_t drive_configuration_handle;
    fbe_payload_cdb_fis_io_status_t io_status;
    fbe_payload_cdb_fis_error_t payload_cdb_fis_error;
    fbe_time_t io_end_time = 0;
    fbe_drive_configuration_record_t * threshold_rec = NULL;
    fbe_scsi_sense_key_t sense_key = FBE_SCSI_SENSE_KEY_NO_SENSE; 
    fbe_scsi_additional_sense_code_t asc = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO; 
    fbe_scsi_additional_sense_code_qualifier_t ascq = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;
    fbe_port_request_status_t port_request_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    fbe_u32_t outstanding_io_count = 0;
    fbe_block_size_t client_block_size, physical_block_size;
    fbe_payload_block_operation_flags_t block_operation_flags;
    
    /* Now that the CDB has completed, decrement the number of IOs in progress
     * for statistics purposes.
     */
    if (sas_physical_drive->b_perf_stats_enabled)
    {
        fbe_sas_physical_drive_perf_stats_dec_num_io_in_progress(sas_physical_drive);
    }

    /* Check payload cdb operation. */
    payload = fbe_transport_get_payload_ex(packet);
    block_operation_p = fbe_payload_ex_get_prev_block_operation(payload);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    
    if (payload_cdb_operation == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive,
                             FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s, Not a cdb operation.\n", __FUNCTION__);
                                     
       fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE , 0);
       return FBE_STATUS_GENERIC_FAILURE;       
    }

    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    if (status == FBE_STATUS_CANCELED)
    { 
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                                "%s, packet status cancelled:0x%X\n", __FUNCTION__, status);

        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED);    
        return status;  
    }
    else if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, packet status failed:0x%X\n", __FUNCTION__, status);

        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        return status;  
    }

    fbe_transport_update_physical_drive_service_time(packet);

    /* Get copy of DIEH handle and use throughout processing to guard against a DIEH table update. */
    fbe_base_physical_drive_get_configuration_handle(base_physical_drive, &drive_configuration_handle);

    /* Add exception for micron buckhorns which return a port error for write same with unmap
     * but the command is supported
     */
    fbe_payload_block_get_flags(block_operation_p, &block_operation_flags);
    fbe_payload_cdb_get_request_status(payload_cdb_operation, &port_request_status); 
    if (((block_operation_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_UNMAP) != 0) && 
        port_request_status == FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN &&
        fbe_base_physical_drive_is_unmap_supported_override(base_physical_drive))
    {
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SUCCESS);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);
    }


    /* This is an optimization.  Only get DIEH threshold info if there was an error.  The following
       function fbe_payload_cdb_completion() will only use this info if there was one. */
    sas_pdo_get_dieh_threshold_rec_on_error(sas_physical_drive, drive_configuration_handle, payload_cdb_operation, &threshold_rec);

    status = fbe_payload_cdb_completion(payload_cdb_operation, /* IN */
                                        threshold_rec, /* IN */
                                        &io_status, /* OUT */    
                                        &payload_cdb_fis_error, /* OUT */
                                        &bad_lba); /* OUT */
    /* TODO: io_status is never used.  We should use this status for setting block_status rtn values, unless 
       overriden by calls below.  DIEH should also pass this value so the exception_list can override it. 
       The three calls (made from this function) which can change the IO error handling are:
           fbe_payload_cdb_completion(),
           fbe_base_physical_drive_io_fail(), and
           sas_physical_drive_process_scsi_error().  -wayne 
    */

	if(payload_cdb_fis_error == FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR){
		fbe_base_physical_drive_io_success((fbe_base_physical_drive_t *) sas_physical_drive);
	} else {
        fbe_payload_cdb_scsi_status_t scsi_status;

        fbe_payload_cdb_get_request_status(payload_cdb_operation, &port_request_status);
        fbe_payload_cdb_get_scsi_status(payload_cdb_operation, &scsi_status);
        
        if (FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION == scsi_status){
            fbe_u8_t *sense_data = NULL;
            fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_data);
            fbe_payload_cdb_parse_sense_data(sense_data, &sense_key, &asc, &ascq, NULL, NULL);
        }

        status = fbe_base_physical_drive_io_fail(base_physical_drive, drive_configuration_handle, threshold_rec, payload_cdb_fis_error, payload_cdb_operation->cdb[0], port_request_status, sense_key, asc, ascq);
    } 

    sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, &bad_lba, &sense_code);
    fbe_payload_ex_set_scsi_error_code (payload, error);/*sharel: still needed until we remove Flare to allow qual*/
    fbe_payload_ex_set_sense_data(payload, sense_code);/*sharel: still needed until we remove Flare to allow qual*/

    /* Set retry time in block operation. */
    fbe_base_physical_drive_set_retry_msecs(payload, error);

    /* Currently set the physical LBA in the payload.  This will be used if PDO needs to remap it.  If we are going to return
       a media error back to client then it will be converted to the client's LBA at that time. */
    fbe_payload_ex_set_media_error_lba(payload, bad_lba);

    /* Update the end time for this IO */
    io_end_time = fbe_get_time_in_us();
    fbe_payload_cdb_set_physical_drive_io_end_time(payload_cdb_operation, io_end_time);

    
    if (error == 0)
    {
        return_status = FBE_STATUS_OK;

       /* The Block operation completed successfully so
           check to see if RBA Tracing is turned on and log it if it is
        */
       if (fbe_traffic_trace_is_enabled (KTRC_TPDO))
       {
		   fbe_sas_physical_drive_trace_completion(sas_physical_drive, packet, block_operation_p, payload_cdb_operation, error);
       }
       
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);    

        /* IO has been completed, so update this PDO's performance counters 
           if the collection of those statistics is enabled
         */
        if (sas_physical_drive->b_perf_stats_enabled) {
            fbe_sas_physical_drive_perf_stats_update_counters_io_completion(sas_physical_drive, block_operation_p, packet->cpu_id, payload_cdb_operation);
        }

        /* If a Seagate disk is currently running a version of firmware that does not fix the Read Look Ahead (RLA)
         * problem, it just completed a VERIFY command, and that's the only outstanding I/O in the disk's queue,
         * then RLAs must be aborted by issuing a Test Unit Ready (TUR).
         * NOTE: We do not set the TUR condition and have the ready rotary handle the TUR, because that would use
         * another I/O packet resource and the monitor thread runs at a lower priority than the I/O thread.
         */
        if ((payload_cdb_operation->cdb[0] == FBE_SCSI_VERIFY_10 || 
             payload_cdb_operation->cdb[0] == FBE_SCSI_VERIFY_16) &&
            sas_physical_drive->sas_drive_info.rla_abort_required == FBE_TRUE)
        {
            fbe_block_transport_server_get_outstanding_io_count(&base_physical_drive->block_transport_server,
                                                                &outstanding_io_count);

            if (outstanding_io_count == 1) /* Must count this VERIFY as outstanding*/
            {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                                                           "%s: VERIFY opcode:0x%x Issuing TUR to abort RLA\n",
                                                           __FUNCTION__, payload_cdb_operation->cdb[0]);

                fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
                fbe_sas_physical_drive_send_tur_to_abort_rla(sas_physical_drive, packet);
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
            else
            {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                                                           "%s: VERIFY opcode:0x%x outstanding IOs:%d TUR not issued\n",
                                                           __FUNCTION__, payload_cdb_operation->cdb[0],
                                                           outstanding_io_count);
            }
        }

        /* Release actual CDB operation from payload */
        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        payload_cdb_operation = NULL;
    }
    else /* We got an error */
    {
	    fbe_payload_cdb_operation_t temp_payload_cdb_operation = {0};

	    /* Copy CDB operation for temporary so we can use it while logging event */
		fbe_copy_memory(&temp_payload_cdb_operation, payload_cdb_operation, sizeof(fbe_payload_cdb_operation_t));

		/* Release actual CDB operation from payload */
		fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
		payload_cdb_operation = NULL;

        fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, error:0x%X\n", __FUNCTION__, error);

        /* Process SCSI errors. This will set the block status to be returned to client.*/
        return_status = sas_physical_drive_process_scsi_error(sas_physical_drive, packet, error, &temp_payload_cdb_operation);


        if (FBE_STATUS_MORE_PROCESSING_REQUIRED != return_status)
        {
            /* We are done with this IO.  Set the appropriate fields for the client. */

            /* Convert bad_lba to client's logical LBA */
            if (FBE_LBA_INVALID != bad_lba)                
            {            
                fbe_payload_block_get_block_size(block_operation_p, &client_block_size); 
                physical_block_size = sas_physical_drive->base_physical_drive.block_size;
                sas_physical_drive_cdb_convert_to_logical_block_size(sas_physical_drive, client_block_size, physical_block_size, &bad_lba, NULL);
                fbe_payload_ex_set_media_error_lba(payload, bad_lba);
            }
        }

		/* IO has been completed, so update this PDO's performance counters 
		   if the collection of those statistics is enabled
		*/
		if (sas_physical_drive->b_perf_stats_enabled) {
			fbe_sas_physical_drive_perf_stats_update_counters_io_completion(sas_physical_drive, block_operation_p, packet->cpu_id, &temp_payload_cdb_operation);
		}
    }

    /* It's possible IO was outstanding to drive for a very long time, causing the service
       time to be exceeded.   If so we need to trigger HC.
    */
    if (fbe_base_drive_test_and_set_health_check(base_physical_drive, packet))
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                   "%s: initiated health check. pkt: %p\n", __FUNCTION__, packet);

        /* let the IO complete as it usually would */
    }
    

    return return_status;
}


/*!**************************************************************
 * @fn sas_physical_drive_process_resend_block_cdb()
 ****************************************************************
 * @brief
 *  This function will resend an IO.  This should only be called
 *  for cases where RAID isn't able to do retries.  These are
 *  cases which are very specific to the drive,  such as remap 
 *  handling or recovery for specific failure modes (e.g. drive
 *  lockup).
 *
 * @param packet - completion packet.
 * @param context - context of the completion callback.
 *
 * @return status - success of failure.
 *
 * @note A cdb_payload is re-allocated for each resend.  This can
 *   be problematic if you wish to carry over info such as retry
 *   counts. 
 *
 * HISTORY:
 *  01/25/2013 - Modified.  Wayne Garrett
 *
 ****************************************************************/
fbe_status_t 
sas_physical_drive_process_resend_block_cdb(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;    
    fbe_payload_cdb_operation_t *   cdb_operation = NULL;    
    fbe_time_t io_start_time = 0;
    
    payload = fbe_transport_get_payload_ex(packet);

    cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    if(cdb_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_cdb_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                               "%s Entry. pkt:%p\n", __FUNCTION__, packet);

    /* Process the payload operation. */
    
    //status = sas_physical_drive->sas_drive_info.fbe_scsi_build_block_cdb[block_operation_opcode](sas_physical_drive, payload, cdb_operation, timeout);
	status = sas_physical_drive_block_to_cdb(sas_physical_drive, payload, cdb_operation);
    if (status != FBE_STATUS_OK)
    {
        /* If the status was bad, then return status immediately.
            */
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, sas_physical_drive_block_to_cdb Failed startus %d\n", __FUNCTION__, status);
        fbe_payload_ex_release_cdb_operation(payload, cdb_operation);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    /* This is a re-send IO. We will consider as new IO from PDO perspective */
    /* Update the new ST */
    io_start_time = fbe_get_time_in_us();
    fbe_payload_cdb_set_physical_drive_io_start_time(cdb_operation, io_start_time);
    fbe_payload_cdb_set_physical_drive_io_end_time(cdb_operation, 0);

    /* Now that we're about to issue the CDB to the drive, increment the number of IOs in progress
     * for statistics purposes.
     */
    if (sas_physical_drive->b_perf_stats_enabled)
    {
        fbe_sas_physical_drive_perf_stats_inc_num_io_in_progress(sas_physical_drive, packet->cpu_id);
    }

    /* If this is a 4K drive and there is data transfer being done set the flag appropriately so that
     * we can let miniport know. This is mostly for encryption related purposes
     */
    if ((sas_physical_drive->base_physical_drive.block_size == FBE_BLOCK_SIZES_4160) &&
        (cdb_operation->payload_cdb_flags != FBE_PAYLOAD_CDB_FLAGS_NO_DATA_TRANSFER))
    {
        cdb_operation->payload_cdb_flags |= FBE_PAYLOAD_CDB_FLAGS_BLOCK_SIZE_4160;
    }

    payload->physical_drive_transaction.last_error_code = 0;
    fbe_transport_set_completion_function(packet, sas_physical_drive_resend_block_cdb_completion, sas_physical_drive);
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet); 

    return status;    
}

/* This is the completion function for the packet sent to a drive. */ 
static fbe_status_t 
sas_physical_drive_resend_block_cdb_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)context;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_cdb_operation_t temp_payload_cdb_operation = {0};
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_scsi_error_code_t error = 0;
    fbe_lba_t bad_lba = 0;
    fbe_u32_t sense_code = 0;
    fbe_drive_configuration_handle_t drive_configuration_handle;
    fbe_payload_cdb_fis_io_status_t io_status;
    fbe_payload_cdb_fis_error_t payload_cdb_fis_error;
    fbe_time_t io_end_time = 0;
    fbe_drive_configuration_record_t * threshold_rec = NULL;
    fbe_scsi_sense_key_t sense_key = FBE_SCSI_SENSE_KEY_NO_SENSE; 
    fbe_scsi_additional_sense_code_t asc = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO; 
    fbe_scsi_additional_sense_code_qualifier_t ascq = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;
    fbe_port_request_status_t port_request_status = FBE_PORT_REQUEST_STATUS_SUCCESS;


    /* Now that the CDB has completed, decrement the number of IOs in progress
     * for statistics purposes.
     */
    if (sas_physical_drive->b_perf_stats_enabled)
    {
        fbe_sas_physical_drive_perf_stats_dec_num_io_in_progress(sas_physical_drive);
    }
        
    /* Check payload cdb operation. */
    payload = fbe_transport_get_payload_ex(packet);
    block_operation_p = fbe_payload_ex_get_prev_block_operation(payload);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    
    if (payload_cdb_operation == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive,
                             FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s, Not a cdb operation.\n", __FUNCTION__);
                                     
       fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE , 0);
       return FBE_STATUS_GENERIC_FAILURE;       
    }   
        
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive,
                                     FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s, packet status failed:0x%X\n", __FUNCTION__, status);
                                     
       fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
       return status;       
    }

    fbe_transport_update_physical_drive_service_time(packet);

    /* Get copy of DIEH handle and use throughout processing to guard against a DIEH table update. */
    fbe_base_physical_drive_get_configuration_handle(base_physical_drive, &drive_configuration_handle);

    /* This is an optimization.  Only get DIEH threshold info if there was an error.  The following
       function fbe_payload_cdb_completion() will only use this info if there was one. */
    sas_pdo_get_dieh_threshold_rec_on_error(sas_physical_drive, drive_configuration_handle, payload_cdb_operation, &threshold_rec);

    status = fbe_payload_cdb_completion(payload_cdb_operation, /* IN */
                                        threshold_rec, /* IN */
                                        &io_status, /* OUT */
                                        &payload_cdb_fis_error, /* OUT */
                                        &bad_lba); /* OUT */

    if(payload_cdb_fis_error != FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR){
        fbe_payload_cdb_scsi_status_t scsi_status;

        fbe_payload_cdb_get_request_status(payload_cdb_operation, &port_request_status);
        fbe_payload_cdb_get_scsi_status(payload_cdb_operation, &scsi_status);

        if (FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION == scsi_status){
            fbe_u8_t *sense_data = NULL;
            fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_data);
            fbe_payload_cdb_parse_sense_data(sense_data, &sense_key, &asc, &ascq, NULL, NULL);
        }

        status = fbe_base_physical_drive_io_fail(base_physical_drive, drive_configuration_handle, threshold_rec, payload_cdb_fis_error, payload_cdb_operation->cdb[0], port_request_status, sense_key, asc, ascq);
    } else {
        fbe_base_physical_drive_io_success((fbe_base_physical_drive_t *) sas_physical_drive);
    }

    sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, &bad_lba, &sense_code);

    
    /* Update the end time for this IO */
    io_end_time = fbe_get_time_in_us();
    fbe_payload_cdb_set_physical_drive_io_end_time(payload_cdb_operation, io_end_time);
    
    /* Copy CDB operation for temporary so we can use it while logging event */
    fbe_copy_memory(&temp_payload_cdb_operation, payload_cdb_operation, sizeof(fbe_payload_cdb_operation_t));

    /* Release actual CDB operation from payload */
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    payload_cdb_operation = NULL;

    /* Initialize returned block status to success.  If there is an error then it will be overwritten.*/
    if (base_pdo_payload_transaction_flag_is_set(&payload->physical_drive_transaction, BASE_PDO_TRANSACTION_STATE_IN_REMAP))
    {
        /* RAID expects SUCCESS/COMPLETE_WITH_REMAP if IO was remapped. */
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_COMPLETE_WITH_REMAP);    
    }
    else
    {
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);  
    }


    /* Special check for drive lockup case.  If retry was not a timeout then drive is not locked up, so resume IO.
       Note, this doesn't explicitly check for the IO that we retried, since if any IO returns a non-timeout status
       then we are not in a lockup situation.  If retry is a timeout it will be handled in sas_physical_drive_process_scsi_error().
    */
    fbe_base_physical_drive_lock(base_physical_drive);
    if (fbe_base_physical_drive_is_local_state_set(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DRIVE_LOCKUP_RECOVERY) &&
        FBE_SCSI_IO_TIMEOUT_ABORT != error )  /* not a timeout */         
    {           
        fbe_base_physical_drive_clear_local_state(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DRIVE_LOCKUP_RECOVERY);                
        fbe_base_physical_drive_unlock(base_physical_drive);

        fbe_base_physical_drive_customizable_trace(base_physical_drive,  FBE_TRACE_LEVEL_INFO,
                                                   "resend_block_cdb_completion: no lockup. error:0x%x pkt:0x%p\n", error, packet);

        fbe_block_transport_server_resume(&base_physical_drive->block_transport_server); 
        fbe_block_transport_server_process_io_from_queue(&base_physical_drive->block_transport_server);            
    }
    else
    {
        fbe_base_physical_drive_unlock(base_physical_drive);
    }

    /* The Block operation completed so
        check to see if RBA Tracing is turned on and log it if it is
     */
    if (fbe_traffic_trace_is_enabled (KTRC_TPDO))
    {
        fbe_sas_physical_drive_trace_completion(sas_physical_drive, packet, block_operation_p, &temp_payload_cdb_operation, error);
    }


    if (error != 0)
    {        
        /* Set scsi error in block operation. */
        fbe_payload_ex_set_scsi_error_code (payload, error);

        /* Currently set the physical LBA in the payload.  This will be used if PDO needs to remap it again.  If we are going to return
           a media error back to client then it will be converted to the client's LBA at that time. */
        fbe_payload_ex_set_media_error_lba(payload, bad_lba);

        fbe_payload_ex_set_sense_data(payload, sense_code);

        fbe_base_physical_drive_customizable_trace(base_physical_drive,  FBE_TRACE_LEVEL_INFO,
                                          "resend_block_cdb_completion, error:0x%x, pkt:0x%p\n", error, packet);
                          
        /* Process SCSI errors. */
        status = sas_physical_drive_process_scsi_error(sas_physical_drive, packet, error, &temp_payload_cdb_operation);
    }


    /* Whether IO was successful or not, we need to check if service time has exceeded.  The only time
       we don't need to do it is when we are going for another remap,  since the remap handling does its
       own checks.  When going for a remap status is set to more_processing_required.
     */
    if (FBE_STATUS_MORE_PROCESSING_REQUIRED != status)
    {
        fbe_block_size_t client_block_size, physical_block_size;

        /* We are done with this IO.  Set the appropriate fields for the client. */

        /* Convert bad_lba to client's logical LBA */
        if (FBE_LBA_INVALID != bad_lba)                
        {    
            fbe_payload_block_get_block_size(block_operation_p, &client_block_size); 
            physical_block_size = sas_physical_drive->base_physical_drive.block_size;
            sas_physical_drive_cdb_convert_to_logical_block_size(sas_physical_drive, client_block_size, physical_block_size, &bad_lba, NULL);
            fbe_payload_ex_set_media_error_lba(payload, bad_lba);
        }

        /* It's possible this IO was held too long due to previous remaping attemps, causing the service
           time to be exceeded.   If so we need to trigger HC to prevent a panic at upper 
           layers due to other IO blocked on this one.
        */
        if (fbe_base_drive_test_and_set_health_check(base_physical_drive, packet))
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                       "resend_block_cdb_completion: initiated health check. pkt: %p\n", packet);
            /* let the IO complete as it usually would */
        }
    }   

    /* IO has been completed, so update this PDO's performance counters 
       if the collection of those statistics is enabled
    */
    if (sas_physical_drive->b_perf_stats_enabled)
    {
        fbe_sas_physical_drive_perf_stats_update_counters_io_completion(sas_physical_drive, block_operation_p, packet->cpu_id, &temp_payload_cdb_operation);
    }
        
    return status;
}


fbe_status_t
fbe_sas_physical_drive_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    status = fbe_sas_physical_drive_handle_block_io(sas_physical_drive, packet);
    return status;
}


fbe_status_t 
fbe_sas_physical_drive_send_reserve(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * remap_packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_time_t io_start_time;

    payload = fbe_transport_get_payload_ex(remap_packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    /* We will consider it as new IO*/
    /* It's time to update start time of new remap IO in PDO */
    io_start_time = fbe_get_time_in_us();
    fbe_payload_cdb_set_physical_drive_io_start_time(payload_cdb_operation, io_start_time);

    sas_physical_drive_cdb_build_reserve(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);
   
    fbe_transport_set_completion_function(remap_packet, sas_physical_drive_send_reserve_completion, sas_physical_drive);
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, remap_packet);

    return status;
}

/*
 * This is the completion function for the reserve packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_physical_drive_send_reserve_completion(fbe_packet_t * remap_packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_cdb_operation_t temp_payload_cdb_operation = {0};
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_packet_t * io_packet = NULL;
    fbe_time_t io_end_time;
    
    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    payload = fbe_transport_get_payload_ex(remap_packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
 
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                            FBE_TRACE_LEVEL_INFO,
                            "%s retry count: %d\n",
                            __FUNCTION__, payload->physical_drive_transaction.retry_count);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(remap_packet);

    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        io_packet = (fbe_packet_t *)fbe_transport_get_master_packet(remap_packet);
        fbe_transport_set_status(io_packet, status, 0);
        sas_physical_drive_end_remap(sas_physical_drive, remap_packet);
        return FBE_STATUS_OK;
    }

    sas_physical_drive_get_scsi_error_code(sas_physical_drive, remap_packet, payload_cdb_operation, &error, NULL, NULL);

    /* Update the end time for this IO */
    io_end_time = fbe_get_time_in_us();
    fbe_payload_cdb_set_physical_drive_io_end_time(payload_cdb_operation, io_end_time);

    /* before releasing payload, make a copy for event logging */
    fbe_copy_memory(&temp_payload_cdb_operation, payload_cdb_operation, sizeof(fbe_payload_cdb_operation_t));

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    payload_cdb_operation = NULL;

    if ((payload->physical_drive_transaction.retry_count >= FBE_PAYLOAD_CDB_MAX_RESERVE_COUNT) ||
        (error == 0) ||
        (error == FBE_SCSI_CC_PFA_THRESHOLD_REACHED) ||
        (error == FBE_SCSI_CC_SUPER_CAP_FAILURE) ||
        (error == FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR) ||
        (error == FBE_SCSI_CC_AUTO_REMAPPED) ||
        (error == FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP) ||
        (error == FBE_SCSI_CC_NOERR) ||
        (error == FBE_SCSI_CC_RECOVERED_ERR) ||
        (error == FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH) ||
        (error == FBE_SCSI_CC_DIE_RETIREMENT_START)  ||
        (error == FBE_SCSI_CC_DIE_RETIREMENT_END))
    {
        /* Success */

        if (error)
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                       "%s Recovered Error: 0x%x", __FUNCTION__, error);

            /* Log the event, even on a recovered error*/
            (void)fbe_base_physical_drive_log_event((fbe_base_physical_drive_t*)sas_physical_drive, error, remap_packet, payload->physical_drive_transaction.last_error_code, &temp_payload_cdb_operation);
        }

        payload->physical_drive_transaction.retry_count = 0;
        fbe_sas_physical_drive_send_reassign(sas_physical_drive, remap_packet);
    }
    else /*if (error == FBE_SCSI_DEVICE_RESERVE)*/
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                   "%s Failed. Error: 0x%x", __FUNCTION__, error);

        /* Log the event on error*/
        (void)fbe_base_physical_drive_log_event((fbe_base_physical_drive_t*)sas_physical_drive, error, remap_packet, payload->physical_drive_transaction.last_error_code, &temp_payload_cdb_operation);

        payload->physical_drive_transaction.retry_count++;
        fbe_sas_physical_drive_send_reserve(sas_physical_drive, remap_packet);
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

fbe_status_t 
fbe_sas_physical_drive_send_reassign(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * remap_packet)
{
    fbe_packet_t * io_packet = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_block_operation_t *block_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t * defect_list_ptr = NULL;
    fbe_lba_t bad_lba = 0;
    fbe_bool_t longlba = FALSE;
    fbe_u8_t transfer_count = FBE_PAYLOAD_CDB_REMAP_DEFECT_LIST_BYTE_SIZE_SHORT;
    fbe_time_t io_start_time;

    fbe_base_physical_drive_t *base_physical_drive = (fbe_base_physical_drive_t *)sas_physical_drive;
    
    payload = fbe_transport_get_payload_ex(remap_packet);      
    if (base_pdo_payload_transaction_flag_is_set(&payload->physical_drive_transaction, BASE_PDO_TRANSACTION_STATE_IN_DC_REMAP)){
        /* if this is the Disk Collect, then det bad lba from the base_physical_drive. */
        bad_lba =  base_physical_drive->dc_lba;
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "%s DC bad lba: 0x%X\n",
                                __FUNCTION__, (unsigned int)bad_lba);     
    }
    else {
        /* Get bad lba from master packet. */
        io_packet = (fbe_packet_t *)fbe_transport_get_master_packet(remap_packet);
        payload = fbe_transport_get_payload_ex(io_packet);
        block_operation = fbe_payload_ex_get_block_operation(payload);

        fbe_payload_ex_get_media_error_lba(payload, &bad_lba);   /*this will be physical LBA not logical*/
           
        payload = fbe_transport_get_payload_ex(remap_packet);

        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "%s bad lba: 0x%X\n",
                                __FUNCTION__, (unsigned int)bad_lba);
    }
    
    if (bad_lba > 0xFFFFFFFF) {
        longlba = TRUE;
        transfer_count = FBE_PAYLOAD_CDB_REMAP_DEFECT_LIST_BYTE_SIZE;
    }
 
    /* Setup sg_list. */
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    /* We will consider it as new IO*/
    /* It's time to update start time of new remap IO in PDO */
    io_start_time = fbe_get_time_in_us();
    fbe_payload_cdb_set_physical_drive_io_start_time(payload_cdb_operation, io_start_time);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    sg_list[0].count = transfer_count;
    sg_list[0].address = (fbe_u8_t *)sg_list + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    /* Setup defect list */
    defect_list_ptr = sg_list[0].address;
    fbe_zero_memory(defect_list_ptr, transfer_count);
    /* Please refer to SBC-3 for the reassign blocks command format.
     */
    if (longlba) {
        *(defect_list_ptr+3) = sizeof(fbe_u64_t);
        *((fbe_u64_t *) (defect_list_ptr+4)) = swap64(bad_lba);
    } else {
        *(defect_list_ptr+3) = sizeof(fbe_u32_t);
        *((fbe_u32_t *) (defect_list_ptr+4)) = swap32((fbe_u32_t)(bad_lba & 0xFFFFFFFF));
    }

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, transfer_count);
    
    sas_physical_drive_cdb_build_reassign(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, longlba);

    if (base_pdo_payload_transaction_flag_is_set(&payload->physical_drive_transaction, BASE_PDO_TRANSACTION_STATE_IN_DC_REMAP)) {
        fbe_transport_set_completion_function(remap_packet, sas_physical_drive_send_dc_reassign_completion, sas_physical_drive);
    }
    else {
        fbe_transport_set_completion_function(remap_packet, sas_physical_drive_send_reassign_completion, sas_physical_drive);
    }
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, remap_packet);

    return status;
}

/*
 * This is the completion function for the reassign packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_physical_drive_send_reassign_completion(fbe_packet_t * remap_packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_packet_t * io_packet = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_ex_t * master_payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_cdb_operation_t temp_payload_cdb_operation = {0};
    fbe_payload_block_operation_t * master_block_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_time_t io_end_time = 0;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    
    payload = fbe_transport_get_payload_ex(remap_packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    io_packet = (fbe_packet_t *)fbe_transport_get_master_packet(remap_packet);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(remap_packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        fbe_transport_set_status(io_packet, status, 0);
        payload->physical_drive_transaction.retry_count = 0;
        fbe_sas_physical_drive_send_release(sas_physical_drive, remap_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }


    sas_physical_drive_get_scsi_error_code(sas_physical_drive, remap_packet, payload_cdb_operation, &error, NULL, NULL);

    /* Update the end time for reassign IO*/
    io_end_time = fbe_get_time_in_us();
    fbe_payload_cdb_set_physical_drive_io_end_time(payload_cdb_operation, io_end_time);

    /* Copy CDB operation for temporary so we can use it while logging event */
    fbe_copy_memory(&temp_payload_cdb_operation, payload_cdb_operation, sizeof(fbe_payload_cdb_operation_t));

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    payload_cdb_operation = NULL;


    /* A Reassign cmd is only successful if no errors occur.   This includes recovered, unrecovered
       and port errors.  A recommendation made by SBMT. If a media error occurs during a reassign then this
       is an indication that the drive is in very bad shape and will be shot immediately.  For all other
       errors proactive sparing will be recommended.
    */
    if (error == 0)
    {
        /* Success */
        payload->physical_drive_transaction.retry_count = 0;
        fbe_sas_physical_drive_send_release(sas_physical_drive, remap_packet);
        ((fbe_base_physical_drive_t *)sas_physical_drive)->physical_drive_error_counts.remap_block_count++;
    }
    else if ( (error == FBE_SCSI_CC_PFA_THRESHOLD_REACHED) ||
              (error == FBE_SCSI_CC_SUPER_CAP_FAILURE) ||
              (error == FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR) ||
              (error == FBE_SCSI_CC_AUTO_REMAPPED) ||
              (error == FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP) ||
              (error == FBE_SCSI_CC_NOERR) ||
              (error == FBE_SCSI_CC_RECOVERED_ERR) ||
              (error == FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH) ||
              (error == FBE_SCSI_CC_RECOVERED_WRITE_FAULT) ||
              (error == FBE_SCSI_CC_RECOVERED_BAD_BLOCK)   ||
              (error == FBE_SCSI_CC_DIE_RETIREMENT_START)  ||
              (error == FBE_SCSI_CC_DIE_RETIREMENT_END))
    {
        /* Success, but a recovered error occured, which should never happen.    Remap will be completed with
           proactive sparing being requested. */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                              FBE_TRACE_LEVEL_WARNING,
                              "%s Recovered Error: 0x%X\n",
                              __FUNCTION__, error);

        /* Log the event on error*/
        (void)fbe_base_physical_drive_log_event((fbe_base_physical_drive_t*)sas_physical_drive, error, remap_packet, payload->physical_drive_transaction.last_error_code, &temp_payload_cdb_operation);
        
        /*force the drive to act as going to die soon, this should trigger proactive sparing at upper levels*/
        fbe_base_physical_drive_set_path_attr((fbe_base_physical_drive_t*)sas_physical_drive, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);

        payload->physical_drive_transaction.retry_count = 0;
        fbe_sas_physical_drive_send_release(sas_physical_drive, remap_packet);
        ((fbe_base_physical_drive_t *)sas_physical_drive)->physical_drive_error_counts.remap_block_count++;

        /* We got Remap IO error. Log the event as we are going to release after this */
        status = fbe_base_physical_drive_log_event((fbe_base_physical_drive_t*)sas_physical_drive, 
                                                   error, 
                                                   remap_packet, 
                                                   payload->physical_drive_transaction.last_error_code, 
                                                   &temp_payload_cdb_operation);

    }
    else if ((payload->physical_drive_transaction.retry_count >= FBE_PAYLOAD_CDB_MAX_REASSIGN_COUNT) ||
             (error == FBE_SCSI_CC_HARD_BAD_BLOCK) ||
             (error == FBE_SCSI_CC_MEDIA_ERROR_WRITE_FAULT) ||
             (error == FBE_SCSI_CC_MEDIA_ERR_WRITE_ERROR) ||
             (error == FBE_SCSI_CC_MEDIA_ERR_DO_REMAP))
    {
        /* Media error occurred.  Shoot drive immediately
         */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                              FBE_TRACE_LEVEL_WARNING,
                              "%s Media or threshold exceeded. Error: 0x%X\n",
                              __FUNCTION__, error);

        /* Log the event on error*/
        (void)fbe_base_physical_drive_log_event((fbe_base_physical_drive_t*)sas_physical_drive, error, remap_packet, payload->physical_drive_transaction.last_error_code, &temp_payload_cdb_operation);

        /* Set block status for master packet. */
        master_payload = fbe_transport_get_payload_ex(io_packet);
        master_block_operation = fbe_payload_ex_get_block_operation(master_payload);       
   
        if (master_block_operation != NULL)
        {
            fbe_payload_block_set_status(master_block_operation, 
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
        }

        fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_REASSIGN);

        /* We need to set the BO:FAIL lifecycle condition */
        status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                  FBE_TRACE_LEVEL_ERROR,
                                  "%s can't set BO:FAIL condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }

        /* We still need to release the drive. */
        payload->physical_drive_transaction.retry_count = 0;
        fbe_sas_physical_drive_send_release(sas_physical_drive, remap_packet);
    }
    else  // recommend proactive sparing
    {
        /* No errors should ever happen during reassign, as stated by SBMT.   If an error occurs
           we will mark it for Proactive Sparing and retry.  We are retrying because IO cannot
           be failed back, since RAID does not expect write IOs to fail.  Worst case we kill
           the drive after exhausting retries.
        */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                              FBE_TRACE_LEVEL_WARNING,
                              "%s Remap failed. Error: 0x%X\n",
                              __FUNCTION__, error);

        /* Log the event on error*/
        (void)fbe_base_physical_drive_log_event((fbe_base_physical_drive_t*)sas_physical_drive, error, remap_packet, payload->physical_drive_transaction.last_error_code, &temp_payload_cdb_operation);

        /*force the drive to act as going to die soon, this should trigger proactive sparing at upper levels*/
        fbe_base_physical_drive_set_path_attr((fbe_base_physical_drive_t*)sas_physical_drive, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);

        payload->physical_drive_transaction.retry_count++;
        fbe_sas_physical_drive_send_reassign(sas_physical_drive, remap_packet);

    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*
 * This is the completion function for the reassign packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_physical_drive_send_dc_reassign_completion(fbe_packet_t * remap_packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_cdb_operation_t temp_payload_cdb_operation = {0};
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_time_t io_end_time;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    
    payload = fbe_transport_get_payload_ex(remap_packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
   
    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(remap_packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        payload->physical_drive_transaction.retry_count = 0;
        fbe_sas_physical_drive_send_release(sas_physical_drive, remap_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    sas_physical_drive_get_scsi_error_code(sas_physical_drive, remap_packet, payload_cdb_operation, &error, NULL, NULL);

    io_end_time = fbe_get_time_in_us();
    fbe_payload_cdb_set_physical_drive_io_end_time(payload_cdb_operation, io_end_time);

    /* Copy CDB operation for temporary so we can use it while logging event */
    fbe_copy_memory(&temp_payload_cdb_operation, payload_cdb_operation, sizeof(fbe_payload_cdb_operation_t));

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    payload_cdb_operation = NULL;
       
    
    /* A Reassign cmd is only successful if no errors occur.   This includes recovered, unrecovered
       and port errors.  A recommendation made by SBMT. If a media error occurs during a reassign then this
       is an indication that the drive is in very bad shape state and will be shot immediately.  For all other
       errors proactive sparing will be recommended.
    */
    if (error == 0)
    {
         /* Success */
         payload->physical_drive_transaction.retry_count = 0;
         fbe_sas_physical_drive_send_release(sas_physical_drive, remap_packet);
         ((fbe_base_physical_drive_t *)sas_physical_drive)->physical_drive_error_counts.remap_block_count++;
    }
    else if ( (error == FBE_SCSI_CC_PFA_THRESHOLD_REACHED) ||
              (error == FBE_SCSI_CC_SUPER_CAP_FAILURE) ||
              (error == FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR) ||
              (error == FBE_SCSI_CC_AUTO_REMAPPED) ||
              (error == FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP) ||
              (error == FBE_SCSI_CC_NOERR) ||
              (error == FBE_SCSI_CC_RECOVERED_ERR) ||
              (error == FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH) ||
              (error == FBE_SCSI_CC_RECOVERED_WRITE_FAULT) ||
              (error == FBE_SCSI_CC_RECOVERED_BAD_BLOCK)   ||
              (error == FBE_SCSI_CC_DIE_RETIREMENT_START)  ||
              (error == FBE_SCSI_CC_DIE_RETIREMENT_END))
    {
        /* Success, but a recovered error occured, which should never happen.    Remap will be completed with
           proactive sparing being requested. */

         fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                              FBE_TRACE_LEVEL_WARNING,
                              "%s Recovered error: 0x%X\n",
                              __FUNCTION__, error);

        /* Log the event on error*/
        (void)fbe_base_physical_drive_log_event((fbe_base_physical_drive_t*)sas_physical_drive, error, remap_packet, payload->physical_drive_transaction.last_error_code, &temp_payload_cdb_operation);

        /*force the drive to act as going to die soon, this should trigger proactive sparing at upper levels*/
        fbe_base_physical_drive_set_path_attr((fbe_base_physical_drive_t*)sas_physical_drive, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);

        payload->physical_drive_transaction.retry_count = 0;
        fbe_sas_physical_drive_send_release(sas_physical_drive, remap_packet);
        ((fbe_base_physical_drive_t *)sas_physical_drive)->physical_drive_error_counts.remap_block_count++;
    }
    else if ((payload->physical_drive_transaction.retry_count >= FBE_PAYLOAD_CDB_MAX_REASSIGN_COUNT) ||
             (error == FBE_SCSI_CC_HARD_BAD_BLOCK) ||
             (error == FBE_SCSI_CC_MEDIA_ERROR_WRITE_FAULT) ||
             (error == FBE_SCSI_CC_MEDIA_ERR_WRITE_ERROR) ||
             (error == FBE_SCSI_CC_MEDIA_ERR_DO_REMAP))
    {
        /* Media error occurred.  Shoot drive immediately.
         */
         fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                              FBE_TRACE_LEVEL_WARNING,
                              "%s Media or threshold exceeded. Error: 0x%X\n",
                              __FUNCTION__, error);

         /* Log the event on error*/
         (void)fbe_base_physical_drive_log_event((fbe_base_physical_drive_t*)sas_physical_drive, error, remap_packet, payload->physical_drive_transaction.last_error_code, &temp_payload_cdb_operation);


         fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_REASSIGN);
         
         /* We need to set the BO:FAIL lifecycle condition */
         status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                         FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

         if (status != FBE_STATUS_OK) {
             fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                  FBE_TRACE_LEVEL_ERROR,
                                  "%s can't set BO:FAIL condition, status: 0x%X\n",
                                  __FUNCTION__, status);
         }

         /* We still need to release the drive. */
         payload->physical_drive_transaction.retry_count = 0;
         fbe_sas_physical_drive_send_release(sas_physical_drive, remap_packet);
    }
    else  // recommend proactive sparing
    {
        /* No errors should ever happen during reassign, as stated by SBMT.   If an error occurs
           we will mark it for Proactive Sparing and retry.  We are retrying because IO cannot
           be failed back, since RAID does not expect write IOs to fail.  Worst case we kill
           the drive after exhausting retries.
        */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                              FBE_TRACE_LEVEL_WARNING,
                              "%s Remap failed. Error: 0x%X\n",
                              __FUNCTION__, error);

        /* Log the event on error*/
        (void)fbe_base_physical_drive_log_event((fbe_base_physical_drive_t*)sas_physical_drive, error, remap_packet, payload->physical_drive_transaction.last_error_code, &temp_payload_cdb_operation);

        /*force the drive to act as going to die soon, this should trigger proactive sparing at upper levels*/
        fbe_base_physical_drive_set_path_attr((fbe_base_physical_drive_t*)sas_physical_drive, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);

        payload->physical_drive_transaction.retry_count++;
        fbe_sas_physical_drive_send_reassign(sas_physical_drive, remap_packet);
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

fbe_status_t 
fbe_sas_physical_drive_send_release(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * remap_packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_time_t io_start_time;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO, 
                                               "%s \n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(remap_packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    /* We will consider it as new IO*/
    /* It's time to update start time of new remap IO in PDO */
    io_start_time = fbe_get_time_in_us();
    fbe_payload_cdb_set_physical_drive_io_start_time(payload_cdb_operation, io_start_time);

    sas_physical_drive_cdb_build_release(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);
    
    fbe_transport_set_completion_function(remap_packet, sas_physical_drive_send_release_completion, sas_physical_drive);
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, remap_packet);
    
    return status;
}

/*
 * This is the completion function for the reserve packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_physical_drive_send_release_completion(fbe_packet_t * remap_packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_cdb_operation_t temp_payload_cdb_operation = {0};
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_packet_t * io_packet = NULL;
    fbe_time_t io_end_time;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    
    payload = fbe_transport_get_payload_ex(remap_packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                            FBE_TRACE_LEVEL_INFO,
                            "%s retry count: %d\n",
                            __FUNCTION__, payload->physical_drive_transaction.retry_count);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(remap_packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        io_packet = (fbe_packet_t *)fbe_transport_get_master_packet(remap_packet);
        fbe_transport_set_status(io_packet, status, 0);
        sas_physical_drive_end_remap(sas_physical_drive, remap_packet);
        return FBE_STATUS_OK;
    }

    sas_physical_drive_get_scsi_error_code(sas_physical_drive, remap_packet, payload_cdb_operation, &error, NULL, NULL);

    io_end_time = fbe_get_time_in_us();
    fbe_payload_cdb_set_physical_drive_io_end_time(payload_cdb_operation, io_end_time);

    /* Copy CDB operation for temporary so we can use it while logging event */
    fbe_copy_memory(&temp_payload_cdb_operation, payload_cdb_operation, sizeof(fbe_payload_cdb_operation_t));

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    payload_cdb_operation = NULL;

    /*TODO: bug here checking retry_count.  On the last retry it will always send a release,
      but the following check we always fail it.  Basically we send a useless release on the
      very last one. */
    if ((payload->physical_drive_transaction.retry_count >= FBE_PAYLOAD_CDB_MAX_RELEASE_COUNT) ||
        (error == 0) ||
        (error == FBE_SCSI_CC_PFA_THRESHOLD_REACHED) ||
        (error == FBE_SCSI_CC_SUPER_CAP_FAILURE) ||
        (error == FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR) ||
        (error == FBE_SCSI_CC_AUTO_REMAPPED) ||
        (error == FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP) ||
        (error == FBE_SCSI_CC_NOERR) ||
        (error == FBE_SCSI_CC_RECOVERED_ERR) ||
        (error == FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH) ||
        (error == FBE_SCSI_DEVICE_RESERVED)  ||
        (error == FBE_SCSI_CC_DIE_RETIREMENT_START)  ||
        (error == FBE_SCSI_CC_DIE_RETIREMENT_END))
    {
        /* Success */


        if (error)
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                  "%s Recovered Error: 0x%X\n", __FUNCTION__, error);

            /* Log the event on error*/
            (void)fbe_base_physical_drive_log_event((fbe_base_physical_drive_t*)sas_physical_drive, error, remap_packet, payload->physical_drive_transaction.last_error_code, &temp_payload_cdb_operation);
        }

        payload->physical_drive_transaction.retry_count = 0;
        
        if (base_pdo_payload_transaction_flag_is_set(&payload->physical_drive_transaction, BASE_PDO_TRANSACTION_STATE_IN_DC_REMAP)) {   
            sas_physical_drive_end_dc_remap(sas_physical_drive, remap_packet);
        }
        else {
            sas_physical_drive_end_remap(sas_physical_drive, remap_packet);
        }
    }
    else
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                              "%s Failed. Error: 0x%X\n", __FUNCTION__, error);

        /* Log the event on error*/
        (void)fbe_base_physical_drive_log_event((fbe_base_physical_drive_t*)sas_physical_drive, error, remap_packet, payload->physical_drive_transaction.last_error_code, &temp_payload_cdb_operation);

        payload->physical_drive_transaction.retry_count++;
        fbe_sas_physical_drive_send_release(sas_physical_drive, remap_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    return FBE_STATUS_OK;
}


fbe_status_t
fbe_sas_physical_drive_reset_device(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_INFO,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if(payload_control_operation == NULL) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s fbe_payload_ex_allocate_control_operation failed\n",__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_build_operation(payload_control_operation, 
                                        FBE_SSP_TRANSPORT_CONTROL_CODE_RESET_DEVICE, 
                                        NULL, 0);

    status = fbe_transport_set_completion_function(packet, sas_physical_drive_reset_device_completion, sas_physical_drive);

    status = fbe_ssp_transport_send_control_packet(&((fbe_sas_physical_drive_t *)sas_physical_drive)->ssp_edge, packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
sas_physical_drive_reset_device_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(payload_control_operation, &control_status);

    fbe_payload_ex_release_control_operation(payload, payload_control_operation);

    return FBE_STATUS_OK;
}


fbe_status_t
fbe_sas_physical_drive_download_device(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_physical_drive_mgmt_fw_download_t * download_info = NULL;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_INFO,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(payload_control_operation, &download_info);

    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    if(payload_cdb_operation == NULL) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s fbe_payload_ex_allocate_cdb_operation failed\n",__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, download_info->transfer_count);

    sas_physical_drive_cdb_build_download(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_WRITE_BUFFER_TIMEOUT, download_info->transfer_count);
   
    fbe_transport_set_completion_function(packet, sas_physical_drive_download_device_completion, sas_physical_drive);
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);

    return status;
}

static fbe_status_t 
sas_physical_drive_download_device_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_physical_drive_mgmt_fw_download_t * download_info = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    
    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_control_get_buffer(payload_control_operation, &download_info);
 
    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);

    if((status != FBE_STATUS_OK) || (download_info == NULL)){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        
        return FBE_STATUS_OK;
    }

    sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    payload_cdb_operation = NULL;

    if ((error == 0) ||
        (error == FBE_SCSI_CC_PFA_THRESHOLD_REACHED) ||
        (error == FBE_SCSI_CC_SUPER_CAP_FAILURE) ||
        (error == FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR) ||
        (error == FBE_SCSI_CC_AUTO_REMAPPED) ||
        (error == FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP) ||
        (error == FBE_SCSI_CC_NOERR) ||
        (error == FBE_SCSI_CC_RECOVERED_ERR) ||
        (error == FBE_SCSI_CC_RECOVERED_WRITE_FAULT) ||
        (error == FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH) ||
        (error == FBE_SCSI_CC_DIE_RETIREMENT_START)  ||
        (error == FBE_SCSI_CC_DIE_RETIREMENT_END))
    {
        /* Success */
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        download_info->download_error_code = 0;  /*TODO: use a defined value such as FBE_SCSI_CC_NOERR.  */
        download_info->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
        download_info->qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
    }
    else if((error == FBE_SCSI_CC_DEFECT_LIST_ERROR) ||
            (error == FBE_SCSI_CC_HARDWARE_ERROR_FIRMWARE) ||
            (error == FBE_SCSI_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION) ||
            (error == FBE_SCSI_CC_FORMAT_CORRUPTED) ||
            (error == FBE_SCSI_CC_SANITIZE_INTERRUPTED) ||
            (error == FBE_SCSI_BUSSHUTDOWN))
    {
        /* Failed. No retry. */
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        download_info->download_error_code = error;
        download_info->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
        download_info->qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE;
    }
    else
    {
        /* Failed. Retry in SHIM. */
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        download_info->download_error_code = error;
        download_info->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
        download_info->qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE;
    }

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_physical_drive_set_power_save(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet, 
                                      fbe_bool_t on, fbe_bool_t passive)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_status_t status;
    fbe_object_id_t my_object_id;

    payload = fbe_transport_get_payload_ex(packet);

    fbe_base_object_get_object_id((fbe_base_object_t *)sas_physical_drive, &my_object_id);
    
    payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);

    if (on) {
        if (passive) {
            fbe_payload_discovery_build_common_command(payload_discovery_operation, 
                                                       FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_SAVE_ON_PASSIVE, my_object_id);
        } else {
            fbe_payload_discovery_build_common_command(payload_discovery_operation, 
                                                       FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_SAVE_ON, my_object_id);
        }
    } else {
        fbe_payload_discovery_build_common_command(payload_discovery_operation, 
                                                   FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_SAVE_OFF, my_object_id);
    }

    fbe_transport_set_completion_function(packet, sas_physical_drive_set_power_save_completion, sas_physical_drive);
  
    /* We are sending discovery packet via discovery edge */
    status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *) sas_physical_drive, packet);

    return status;
}

/*
 * This is the completion function for the start_stop_unit packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_physical_drive_set_power_save_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_status_t discovery_status;
    fbe_status_t status;
    fbe_payload_control_operation_t * control_operation = NULL;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_discovery_get_status(payload_discovery_operation, &discovery_status);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        if(discovery_status != FBE_PAYLOAD_DISCOVERY_STATUS_OK) { 
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        } else {
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        }
    }

    fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_physical_drive_spin_down(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet, fbe_bool_t spinup)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_status_t status;
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    
    fbe_payload_cdb_build_start_stop_unit(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, spinup);
    
    fbe_transport_set_completion_function(packet, sas_physical_drive_spin_down_completion, sas_physical_drive);
 
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);

    return status;
}

/*
 * This is the completion function for spindown packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_physical_drive_spin_down_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;
    fbe_scsi_error_code_t error = 0;
    
    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    
    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);

        if (sas_physical_drive_set_control_status(error, payload_control_operation) != FBE_PAYLOAD_CONTROL_STATUS_OK)
        {
            fbe_payload_control_status_qualifier_t control_status_qualifier;
            fbe_payload_control_get_status_qualifier(payload_control_operation, &control_status_qualifier);

            if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_SANITIZE_IN_PROGRESS ||
                control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_SANITIZE_NEED_RESTART)
            {
                /* Sanitization errors indicate that drive has to be spun-up. The following utility function will mark 
                   this drive as maintenance needed and then change control status to OK so PDO goes Ready. 
                */               
                sas_pdo_process_sanitize_error_for_conditions(sas_physical_drive, payload_control_operation, payload_cdb_operation);

                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                           "%s In Sanitize state.  Allowing start_stop_unit to pass\n", __FUNCTION__);  
            }
        }        
    }

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_physical_drive_send_RDR(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet,
                                fbe_u8_t page_code)
{
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_status_t status;
    fbe_u16_t resp_size = FBE_MEMORY_CHUNK_SIZE - (2 * sizeof(fbe_sg_element_t));

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = resp_size;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, resp_size);

    sas_physical_drive_cdb_build_receive_diag(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, 
                                              page_code, resp_size);

    fbe_transport_set_completion_function(packet, sas_physical_drive_send_RDR_completion, sas_physical_drive);

    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);
    return status;
}

/*
 * This is the completion function for receive_diag packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_physical_drive_send_RDR_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
    }
    else {
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        if (!error || (error == FBE_SCSI_XFERCOUNTNOTZERO)) 
        {
            status = fbe_sas_physical_drive_process_receive_diag_page(sas_physical_drive, packet);
        }
        else
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_payload_control_set_status (payload_control_operation, 
        (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    fbe_transport_release_buffer(sg_list);

    return status;
}

fbe_status_t 
fbe_sas_physical_drive_get_permission(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet, 
                                      fbe_bool_t get)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_status_t status;
    fbe_object_id_t my_object_id;

    payload = fbe_transport_get_payload_ex(packet);

    fbe_base_object_get_object_id((fbe_base_object_t *)sas_physical_drive, &my_object_id);
    
    payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);

    if (get) {
        fbe_payload_discovery_build_common_command(payload_discovery_operation, 
                                                   FBE_PAYLOAD_DISCOVERY_OPCODE_GET_SPINUP_CREDIT, my_object_id);
    } else {
        fbe_payload_discovery_build_common_command(payload_discovery_operation, 
                                                   FBE_PAYLOAD_DISCOVERY_OPCODE_RELEASE_SPINUP_CREDIT, my_object_id);
    }

    fbe_transport_set_completion_function(packet, sas_physical_drive_get_permission_completion, sas_physical_drive);
  
    /* We are sending discovery packet via discovery edge */
    status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *) sas_physical_drive, packet);

    return status;
}

/*
 * This is the completion function for the start_stop_unit packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_physical_drive_get_permission_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_status_t discovery_status;
    fbe_status_t status;
    fbe_payload_control_operation_t * control_operation = NULL;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_discovery_get_status(payload_discovery_operation, &discovery_status);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        if(discovery_status != FBE_PAYLOAD_DISCOVERY_STATUS_OK) { 
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        } else {
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        }
    }

    fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);
    return FBE_STATUS_OK;
}

fbe_status_t 
sas_physical_drive_disk_collect_write(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_lba_t lba = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t *   cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_block_size_t block_size = 0;
    fbe_u8_t sp_id;
    fbe_u8_t id = 0;
    fbe_dc_system_time_t system_time;
            
    fbe_base_physical_drive_t *base_physical_drive = (fbe_base_physical_drive_t *)sas_physical_drive;
    block_size = base_physical_drive->block_size;
    //block_size = 520;
    id = base_physical_drive->dc.number_record;
    
    /* Check if death reason for the disk is set, then add it to DC. */
    if ((base_physical_drive->death_reason != 0) && (id < FBE_DC_IN_MEMORY_MAX_COUNT))
    {
        base_physical_drive->dc.record[id].extended_A07 = base_physical_drive->death_reason;
        base_physical_drive->death_reason = 0; /* Reset it after it is saved. */
    }
    
    fbe_base_physical_drive_get_sp_id(base_physical_drive, &sp_id);
    
    lba = ((sp_id == 0) ? FBE_DC_SPA_START_LBA : FBE_DC_SPB_START_LBA); 
    lba += base_physical_drive->dc_lba;
             
    payload = fbe_transport_get_payload_ex(packet);
    cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);
    
    base_physical_drive->sg_list[0].count = block_size;
    base_physical_drive->sg_list[0].address = (fbe_u8_t *)&base_physical_drive->dc;
   
    base_physical_drive->sg_list[1].count = 0;
    base_physical_drive->sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, base_physical_drive->sg_list, 1);
    
    fbe_payload_cdb_set_transfer_count(cdb_operation, block_size);
    
    fbe_get_dc_system_time(&system_time);
    base_physical_drive->dc.dc_timestamp = system_time;

    fbe_payload_cdb_build_disk_collect_write(cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, block_size, lba, 1);
    
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "%s DCtype0:%d DCtype1:%d to lba:0x%llx RecordCnt:%d.\n",
                                __FUNCTION__, base_physical_drive->dc.record[0].dc_type, base_physical_drive->dc.record[1].dc_type, (unsigned long long)lba, base_physical_drive->dc.number_record);

    fbe_transport_set_completion_function(packet, sas_physical_drive_disk_collect_write_completion, sas_physical_drive);
    
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet); 
    
    return status;
}

/* This is the completion function for the packet sent to a drive. */ 
static fbe_status_t 
sas_physical_drive_disk_collect_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_base_physical_drive_t * base_physical_drive = NULL; 
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_scsi_error_code_t error = 0;
        
    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    base_physical_drive = &sas_physical_drive->base_physical_drive;
    /* Check payload cdb operation. */
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    
    if (payload_cdb_operation == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive,
                             FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s, Not a cdb operation.\n", __FUNCTION__);
                                     
       fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE , 0);
       return FBE_STATUS_GENERIC_FAILURE;       
    }   
        
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, packet status failed:0x%X\n", __FUNCTION__, status);
   }
   else
   {
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        status = sas_physical_drive_set_dc_control_status(sas_physical_drive, error, payload_control_operation);
   }
  
   /* Release payload and buffer. */
   fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
   
   return status;
}

fbe_status_t 
sas_physical_drive_dump_disk_log(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_status_t status;
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    
    sas_physical_drive_cdb_build_dump_disk_log(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);
    
    fbe_transport_set_completion_function(packet, sas_physical_drive_dump_disk_log_completion, sas_physical_drive);
 
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);

    return status;
}

/*
 * This is the completion function for dumping disk internal log packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_physical_drive_dump_disk_log_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;
    fbe_scsi_error_code_t error = 0;
    
    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    
    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        sas_physical_drive_set_control_status(error, payload_control_operation);
    }

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_physical_drive_send_read_disk_blocks(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_get_disk_error_log_t * data = NULL;
    fbe_status_t status;
    fbe_block_size_t block_size = 0;
   
    block_size = sas_physical_drive->base_physical_drive.block_size;
     
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &data);
    
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, block_size);

    sas_physical_drive_cdb_build_read_disk_blocks(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, 
                                              data->lba);

    fbe_transport_set_completion_function(packet, sas_physical_drive_send_read_disk_blocks_completion, sas_physical_drive);

    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);
    return status;
}

/*
 * This is the completion function for read_disk_blocks packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_physical_drive_send_read_disk_blocks_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    //fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_physical_drive_mgmt_get_disk_error_log_t * data_buffer = NULL;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_control_get_buffer(payload_control_operation, &data_buffer);
   // fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
    }
    else {
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        if (!error) 
        {
             // fbe_copy_memory(data_buffer, sg_list[0].address, sizeof(fbe_physical_drive_mgmt_get_disk_error_log_t));
              status = FBE_STATUS_OK;
        }
        else
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_payload_control_set_status (payload_control_operation, 
        (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    //fbe_transport_release_buffer(sg_list);

    return status;
}

fbe_status_t 
fbe_sas_physical_drive_send_smart_dump(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_get_smart_dump_t * get_smart_dump = NULL;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &get_smart_dump);

    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, get_smart_dump->transfer_count);

    sas_physical_drive_cdb_build_smart_dump(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, 
                                            get_smart_dump->transfer_count);

    fbe_transport_set_completion_function(packet, sas_physical_drive_send_smart_dump_completion, sas_physical_drive);

    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);
    return status;
}

/*
 * This is the completion function for receive_diag packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_physical_drive_send_smart_dump_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_physical_drive_mgmt_get_smart_dump_t * get_smart_dump = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_u32_t transferred_count;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_control_get_buffer(payload_control_operation, &get_smart_dump);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
    } else {
        sas_physical_drive_get_scsi_error_code((fbe_sas_physical_drive_t *)sas_physical_drive, packet,
                                               payload_cdb_operation, &error, NULL, NULL);
        if (!error || (error == FBE_SCSI_XFERCOUNTNOTZERO)) 
        {
            status = FBE_STATUS_OK;
            fbe_payload_cdb_get_transferred_count(payload_cdb_operation, &transferred_count);
            get_smart_dump->transfer_count = transferred_count;
        }
        else
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_payload_control_set_status (payload_control_operation, 
        (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);

    return status;
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_send_vpd_inquiry(
 *            fbe_sas_physical_drive_t * sas_physical_drive,
 *            fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *   This is the function to send a VPD inquiry packet.
 *  
 * @param sas_physical_drive - pointer to sas physical drive object.
 * @param packet - pointer to packet. 
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  4/24/2012 - Created. Steve Morley
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_physical_drive_send_vpd_inquiry(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);
    if(payload_control_operation == NULL)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s,  NULL control operation, packet: %64p.\n",
                                __FUNCTION__, packet);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = FBE_SCSI_INQUIRY_DATA_SIZE;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, FBE_SCSI_INQUIRY_DATA_SIZE);

    fbe_payload_cdb_build_vpd_inquiry(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_INQUIRY_TIMEOUT, FBE_SCSI_INQUIRY_VPD_PAGE_F3);

    fbe_transport_set_completion_function(packet, sas_physical_drive_send_vpd_inquiry_completion, sas_physical_drive);

    status = fbe_ssp_transport_send_functional_packet(&((fbe_sas_physical_drive_t *)sas_physical_drive)->ssp_edge, packet);
    return status;
}

/*!**************************************************************
 * @fn sas_physical_drive_send_vpd_inquiry_completion(
 *            fbe_sas_physical_drive_t * sas_physical_drive,
 *            fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *   This is the completion function for VPD inquiry packet.
 *  
 * @param packet - pointer to packet. 
 * @param context - completion context.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  4/24/2012 - Created. Steve Morley
 *
 ****************************************************************/
static fbe_status_t 
sas_physical_drive_send_vpd_inquiry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_sas_drive_status_t validate_status = FBE_SAS_DRIVE_STATUS_OK;
    fbe_u32_t         transferred_count;
    fbe_u32_t         control_status;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* Check status of packet before we continue
     */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        sas_physical_drive_get_scsi_error_code((fbe_sas_physical_drive_t *)sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        if (error == FBE_SCSI_XFERCOUNTNOTZERO)
        {
            fbe_payload_cdb_get_transferred_count(payload_cdb_operation, &transferred_count);
            if (transferred_count <= FBE_SCSI_INQUIRY_DATA_SIZE)
            {
                /* Underrun is not an error
                 */
                error = 0;
            }
        }
        
        control_status = sas_physical_drive_set_control_status(error, payload_control_operation);
        if (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
        {
            validate_status = fbe_sas_physical_drive_process_vpd_inquiry(sas_physical_drive, (fbe_u8_t *)(sg_list[0].address));
            if (validate_status == FBE_SAS_DRIVE_STATUS_INVALID) 
            {
                fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FW_UPGRADE_REQUIRED);

                status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                                (fbe_base_object_t*)sas_physical_drive, 
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                          FBE_TRACE_LEVEL_ERROR,
                                          "%s can't set BO:FAIL condition, status: 0x%X\n",
                                          __FUNCTION__, status);
            
                }        
            }
            else if (validate_status == FBE_SAS_DRIVE_STATUS_NEED_MORE_PROCESSING) 
            {
                fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                fbe_payload_control_set_status_qualifier(payload_control_operation, FBE_SAS_DRIVE_STATUS_NEED_MORE_PROCESSING);
            }
        }
        else
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                  FBE_TRACE_LEVEL_ERROR,
                  "%s error in control status, status: %d\n",
                  __FUNCTION__, status);

        }
    }

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    fbe_transport_release_buffer(sg_list);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_process_vpd_inquiry(
 *            fbe_sas_physical_drive_t * sas_physical_drive,
 *            fbe_u8_t *vpd_inq)
 ****************************************************************
 * @brief
 *   This is the function to send a VPD inquiry packet.
 *  
 * @param sas_physical_drive - pointer to sas flash drive object.
 * @param vpd_inq - pointer to inquiry string. 
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  4/24/2012 - Created. Steve Morley
 *
 ****************************************************************/
static fbe_status_t 
fbe_sas_physical_drive_process_vpd_inquiry(fbe_sas_physical_drive_t * sas_physical_drive, fbe_u8_t *vpd_inq)
{
    fbe_base_physical_drive_t * base_physical_drive = &sas_physical_drive->base_physical_drive;
    fbe_u8_t vpd_page_code = *(vpd_inq + FBE_SCSI_INQUIRY_VPD_PAGE_CODE_OFFSET);


    /* Check if Enhanced Queuing supported
     */
    fbe_base_physical_drive_set_enhanced_queuing_supported(base_physical_drive, FBE_FALSE);
    if(vpd_page_code == FBE_SCSI_INQUIRY_VPD_PAGE_F3)
    {
        vpd_inq += FBE_SCSI_INQUIRY_VPD_F3_SUPPORTS_ENHANCED_QUEUING_OFFSET;
        if(*vpd_inq & FBE_SCSI_INQUIRY_VPD_PAGE_F3_ENHANCED_QUEUING_BIT_MASK)
        {
            fbe_base_physical_drive_set_enhanced_queuing_supported(base_physical_drive, FBE_TRUE);
            return FBE_STATUS_OK;
        }
        else
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                                       FBE_TRACE_LEVEL_WARNING,
                                                       "EQ not supported, vpd_inq=0x%X\n",*vpd_inq);
        }
    }
    else
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                                   FBE_TRACE_LEVEL_WARNING,
                                                   "F3 not found on VPD, vpd_page_code=0x%X\n",vpd_page_code);
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_send_qst(fbe_sas_physical_drive_t * sas_physical_drive, 
 *                                     fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function sends the Send Diagnostic CDB with both the 
 *  "Self Test" bit (Byte 1, Bit 2) and "Quick Self Test" bit (Byte 1, Bit 1)
 *  set to enable the Quick Self Test (QST).
 *
 * @param sas_physical_drive - The drive on which to enable its health check.
 * @param packet - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  04/24/2012 - Created. Darren Insko
 *  07/05/2012 - Renamed and moved from usurper to executer. Darren Insko
 *
 ****************************************************************/
fbe_status_t
fbe_sas_physical_drive_send_qst(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    if (payload_cdb_operation == NULL)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s: fbe_payload_ex_allocate_cdb_operation failed\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_set_completion_function(packet, fbe_sas_physical_drive_send_qst_completion, sas_physical_drive);
    
    /* Build a Send Diagnostic CDB that has its Quick Self Test (QST) bits set. */
    sas_physical_drive_cdb_build_send_diagnostics(payload_cdb_operation,
                                                  FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT,
                                                  FBE_SCSI_SEND_DIAGNOSTIC_QUICK_SELF_TEST_BITS);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "PDO: Enabling the SAS drive's Health Check QST by issuing the SEND DIAGNOSTIC CDB.\n");
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);

    return status;
}
/************************************************
 * end fbe_sas_physical_drive_send_qst()
 ************************************************/

/*!**************************************************************
 * @fn fbe_sas_physical_drive_send_qst_completion(fbe_packet_t * packet,
 *                                                fbe_packet_completion_context_t context)
 ****************************************************************
 * @brief
 *  This is a completion function for fbe_sas_physical_drive_send_health_check().
 *
 * @param packet - completion packet.
 * @param context - context of the completion callback.
 *
 * @return status - success of failure.
 *
 * HISTORY:
 *  04/24/2012 - Created. Darren Insko
 *  07/05/2012 - Renamed and moved from usurper to executer. Darren Insko
 *
 ****************************************************************/
static fbe_status_t 
fbe_sas_physical_drive_send_qst_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t *        sas_physical_drive = NULL;
    fbe_base_physical_drive_t *       base_physical_drive = NULL;
    fbe_payload_ex_t *                payload = NULL;
    fbe_payload_cdb_operation_t     * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_payload_control_status_t      control_status = 0;
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_scsi_error_code_t             error = 0;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    base_physical_drive = (fbe_base_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    
    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "PDO: Invalid packet status: 0x%x\n",
                                status);
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);

        /* Set the control status for errors.*/
        control_status = sas_physical_drive_set_qst_control_status(sas_physical_drive, error, payload_control_operation);
        
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                FBE_TRACE_LEVEL_INFO,
                "PDO: Health Check Set SCSI Errors: 0x%x to Control status: 0x%x & qualifier: 0x%x.\n", 
                error, control_status, payload_control_operation->status_qualifier );

        fbe_base_physical_drive_log_event((fbe_base_physical_drive_t*)sas_physical_drive, error, packet,
                                          payload->physical_drive_transaction.last_error_code, payload_cdb_operation);
    }
         
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    
    return status;
}
/***********************************************************
 * end fbe_sas_physical_drive_send_qst_completion()
 ***********************************************************/

/*!**************************************************************
 * @fn sas_pdo_get_dieh_threshold_rec_on_error
 ****************************************************************
 * @brief
 *  This function will return the DIEH threshold info only if
 *  an IO error was detected and the handle is valid.  
 *
 * @param handle -        IN:  DIEH configuration handle
 * @param cdb_operation - IN:  Packet's payload for the CDB operation
 * @param threshold_rec - OUT: DIEH threshold record to be returned.
 *
 * @return fbe_bool_t : FBE_TRUE if threshold_rec was obtained successfully.
 * 
 * @note
 *  Caller is expected to initalized the threshold rec.  Rec will not be
 *  modified if we fail to obtain it.
 *
 * @author
 *  10/02/2012  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_bool_t sas_pdo_get_dieh_threshold_rec_on_error(fbe_sas_physical_drive_t * sas_physical_drive, 
                                                          fbe_drive_configuration_handle_t handle, 
                                                          fbe_payload_cdb_operation_t * cdb_operation, 
                                                          fbe_drive_configuration_record_t ** threshold_rec)
{
    fbe_payload_cdb_scsi_status_t   cdb_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD;
    fbe_port_request_status_t       cdb_request_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    fbe_status_t                    status;

    fbe_payload_cdb_get_request_status(cdb_operation, &cdb_request_status);
    fbe_payload_cdb_get_scsi_status(cdb_operation, &cdb_scsi_status);

    if (cdb_request_status != FBE_PORT_REQUEST_STATUS_SUCCESS ||
        cdb_scsi_status != FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD)
    {    
        if (handle != FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE)
        {
            status = fbe_drive_configuration_get_threshold_info_ptr(handle, threshold_rec);
        
            if (status != FBE_STATUS_OK)
            {
                /* if we can't get threshold info with a valid handle then something seriously went wrong */
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                               "%s failed to get threshold rec.\n", __FUNCTION__);
                /* notify PDO to re-register */
                fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                       (fbe_base_object_t*)sas_physical_drive, 
                                       FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_CONFIG_CHANGED);
                return FBE_FALSE;
            }

            return FBE_TRUE;
        }            
    }

    return FBE_FALSE;
}
/*!**************************************************************
 * @fn fbe_sas_physical_drive_send_log_sense()
 ****************************************************************
 * @brief
 *  This function sends the log sense (0x4D) CDB to get the log pages.
 *
 * @param sas_physical_drive - The drive on which to get its log pages.
 * @param packet - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  11/27/2012 - Created. Christina Chiang
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_physical_drive_send_log_sense(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_get_log_page_t * get_log_page = NULL;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &get_log_page);

    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, get_log_page->transfer_count);

    sas_physical_drive_cdb_build_log_sense(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, 
                                           get_log_page->page_code, get_log_page->transfer_count);

    fbe_transport_set_completion_function(packet, sas_physical_drive_send_log_sense_completion, sas_physical_drive);

    status = fbe_ssp_transport_send_functional_packet(&((fbe_sas_physical_drive_t *)sas_physical_drive)->ssp_edge, packet);
    return status;
}
/************************************************
 * end fbe_sas_physical_drive_send_log_sense()
 ************************************************/

/*!**************************************************************
 * @fn sas_physical_drive_send_log_sense_completion(fbe_packet_t * packet,
 *                                                         fbe_packet_completion_context_t context)
 ****************************************************************
 * @brief
 *  This is a completion function for fbe_sas_physical_drive_send_log_sense().
 *
 * @param packet - completion packet.
 * @param context - context of the completion callback.
 *
 * @return status - success of failure.
 *
 * HISTORY:
 *  11/27/2012 - Created. Christina Chiang
 *
 ****************************************************************/
static fbe_status_t 
sas_physical_drive_send_log_sense_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
//    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_physical_drive_mgmt_get_log_page_t * get_log_page = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_u32_t  transferred_count;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_control_get_buffer(payload_control_operation, &get_log_page);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                FBE_TRACE_LEVEL_ERROR,
                "%s Invalid packet status: 0x%X\n",
                __FUNCTION__, status);
    } else {
        sas_physical_drive_get_scsi_error_code((fbe_sas_physical_drive_t *)sas_physical_drive, packet,
                                               payload_cdb_operation, &error, NULL, NULL);
        if (!error || (error == FBE_SCSI_XFERCOUNTNOTZERO)) 
        {
            status = FBE_STATUS_OK;
            fbe_payload_cdb_get_transferred_count(payload_cdb_operation, &transferred_count);
            get_log_page->transfer_count = transferred_count;
        }
        else
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_payload_control_set_status (payload_control_operation, 
                                    (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);

    return status;
}
/************************************************
 * end sas_physical_drive_send_log_sense_completion()
 ************************************************/



/*!**************************************************************
 * @fn fbe_sas_physical_drive_send_read_long(fbe_sas_physical_drive_t* sas_physical_drive,
 *                                           fbe_packet_t* packet)
 ****************************************************************
 * @brief
 *  This function sends the read long command.
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
fbe_sas_physical_drive_send_read_long(fbe_sas_physical_drive_t* sas_physical_drive, fbe_packet_t* packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_create_read_uncorrectable_t*  read_uncor_info = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &read_uncor_info);
    
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    //fbe_payload_cdb_set_transfer_count(payload_cdb_operation, FBE_DC_BLOCK_SIZE);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, read_uncor_info->long_block_size);    

    if (read_uncor_info->lba > 0x00000000FFFFFFFF)
    {
        sas_physical_drive_cdb_build_read_long_16(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, 
                                              read_uncor_info->lba, (fbe_u32_t)(read_uncor_info->long_block_size));
    }
    else
    {
        sas_physical_drive_cdb_build_read_long_10(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, 
                                                  read_uncor_info->lba, (fbe_u32_t)(read_uncor_info->long_block_size));
    }
        
    fbe_transport_set_completion_function(packet, fbe_sas_physical_drive_send_read_long_completion, sas_physical_drive);
    
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);
    return status;  
    
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_send_read_long_completion(fbe_packet_t * packet,
 *                                                      fbe_packet_completion_context_t context)
 ****************************************************************
 * @brief
 *  This is a completion function for fbe_sas_physical_drive_send_read_long().
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
fbe_sas_physical_drive_send_read_long_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_physical_drive_mgmt_create_read_uncorrectable_t*  read_uncor_info = NULL;
    
    fbe_payload_cdb_scsi_status_t cdb_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD;
    fbe_u8_t* sense_data = NULL;
    fbe_scsi_sense_key_t sense_key = FBE_SCSI_SENSE_KEY_NO_SENSE; 
    fbe_scsi_additional_sense_code_t asc = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO; 
    fbe_scsi_additional_sense_code_qualifier_t ascq = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;    
    fbe_bool_t valid_bit = FBE_FALSE;
    fbe_lba_t info_field = 0;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_control_get_buffer(payload_control_operation, &read_uncor_info);
    
    fbe_payload_cdb_get_scsi_status(payload_cdb_operation, &cdb_scsi_status);
    fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_data);
    
    /* Check the status of the transport of the packet */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   "%s Invalid packet status: 0x%X\n",
                                                   __FUNCTION__, status);
    }
    else
    {
        /* Do we have a scsi error */
        
        /* Check cdb scsi status and cdb request status. */
        if(cdb_scsi_status != FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD)
        {
            /* Handle SCSI status 
            */
            switch (cdb_scsi_status)
            {
                case FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION:
                {
                    /* Parse the sense data the parameters valid_bit and info_field
                    *  are checked in the function
                    */
                    fbe_payload_cdb_parse_sense_data(sense_data, &sense_key, &asc, &ascq, &valid_bit, &info_field);
                    
                    // if we get a check condition see if
                    //  1.) the sense key is set to Illegal Request and the asc is Invalid Field in CDB
                    //  2.) the valid bit is set to one (and ILI bits are set to one for fixed format sense data)
                    //  3.) then the information field is set to the difference of the requested length minus 
                    //         the actual length where negative numbers are indicated by two's complement notation.
                    //  The information field will be returned in read_uncor_info->long_block_size
                    if ( (sense_key == FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST) &&
                         (asc == FBE_SCSI_ASC_INVALID_FIELD_IN_CDB) && valid_bit )
                    {
                        read_uncor_info->long_block_size = (fbe_s32_t)(info_field);
                        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                                                   FBE_TRACE_LEVEL_ERROR,
                                                                   "%s Read Long Info_Field: %d Sense_Key: 0x%X asc: 0x%X\n",
                                                                   __FUNCTION__, read_uncor_info->long_block_size,
                                                                   sense_key, asc);                     
                    }
                    break;
                }
            case FBE_PAYLOAD_CDB_SCSI_STATUS_BUSY:
                error = FBE_SCSI_DEVICE_BUSY;
                break;
                
            case FBE_PAYLOAD_CDB_SCSI_STATUS_RESERVATION_CONFLICT:
                error = FBE_SCSI_DEVICE_RESERVED;
                break;
                
            default: 
                /* We've receive a status that we do not understand.
                */
                error = FBE_SCSI_UNKNOWNRESPONSE;
            } /* end of Switch */                   
        } /* end of if */
    }       
    if (!error) 
    {
        status = FBE_STATUS_OK;
    }
    else
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }       
    
    fbe_payload_control_set_status (payload_control_operation, 
                                    (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    
    return status;  
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_send_write_long(fbe_sas_physical_drive_t* sas_physical_drive,
 *                                            fbe_packet_t* packet)
 ****************************************************************
 * @brief
 *  This function sends the write long command.
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
fbe_sas_physical_drive_send_write_long(fbe_sas_physical_drive_t* sas_physical_drive, fbe_packet_t* packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_create_read_uncorrectable_t*  read_uncor_info = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &read_uncor_info);
    
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, read_uncor_info->long_block_size);    

    if (read_uncor_info->lba > 0x00000000FFFFFFFF)
    {   
        sas_physical_drive_cdb_build_write_long_16(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, 
                                              read_uncor_info->lba, (fbe_u32_t)(read_uncor_info->long_block_size));
    }
    else
    {   
        sas_physical_drive_cdb_build_write_long_10(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, 
                                                   read_uncor_info->lba, (fbe_u32_t)(read_uncor_info->long_block_size));
    }                                             
    
    fbe_transport_set_completion_function(packet, fbe_sas_physical_drive_send_read_long_completion, sas_physical_drive);
                                              
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);
    
    return status;      
    
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_send_write_long_completion(fbe_packet_t * packet,
 *                                                       fbe_packet_completion_context_t context)
 ****************************************************************
 * @brief
 *  This is a completion function for fbe_sas_physical_drive_send_write_long().
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
fbe_sas_physical_drive_send_write_long_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_physical_drive_mgmt_create_read_uncorrectable_t* data_buffer = NULL;
    
    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_control_get_buffer(payload_control_operation, &data_buffer);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   "%s Invalid packet status: 0x%X\n",
                                                   __FUNCTION__, status);
    }
    else {
        sas_physical_drive_get_scsi_error_code(sas_physical_drive,packet, payload_cdb_operation, &error, NULL, NULL);
        if (!error) 
        {
            status = FBE_STATUS_OK;
        }
        else
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }
    
    fbe_payload_control_set_status (payload_control_operation, 
                                    (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    return status;

}   



fbe_status_t
fbe_sas_physical_drive_format_block_size(fbe_sas_physical_drive_t* sas_physical_drive, fbe_packet_t* packet, fbe_u32_t block_size)
{

    /* First mode select new capacity. The completion function will then issue the format cmd.
    */

    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;
    fbe_scsi_mode_parameter_list_10_t  *mp_list_p;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);

    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n", __FUNCTION__);
        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    //sg_list[0].count = sas_physical_drive->sas_drive_info.ms_size;
    sg_list[0].count = sizeof(fbe_scsi_mode_parameter_list_10_t);
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);
    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    /* update mode parameter list to change block size */
    mp_list_p = (fbe_scsi_mode_parameter_list_10_t*)sg_list[0].address;
    fbe_zero_memory(mp_list_p, sizeof(fbe_scsi_mode_parameter_list_10_t));

    mp_list_p->header.block_descriptor_len[0] = 0;
    mp_list_p->header.block_descriptor_len[1] = 8;
    mp_list_p->block_descriptor.block_len[0] =  (fbe_u8_t)((block_size >> 16) & 0xFF);
    mp_list_p->block_descriptor.block_len[1] =  (fbe_u8_t)((block_size >> 8)  & 0xFF);
    mp_list_p->block_descriptor.block_len[2] =  (fbe_u8_t)((block_size)       & 0xFF);

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    //fbe_payload_cdb_set_transfer_count(payload_cdb_operation, sas_physical_drive->sas_drive_info.ms_size);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, sizeof(fbe_scsi_mode_parameter_list_10_t));

    //fbe_payload_cdb_build_mode_select_10(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, sas_physical_drive->sas_drive_info.ms_size);
    fbe_payload_cdb_build_mode_select_10(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, sizeof(fbe_scsi_mode_parameter_list_10_t));

    fbe_transport_set_completion_function(packet, sas_physical_drive_mode_select_10_block_size_completion, sas_physical_drive);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                              "%s Sending mode select!!!\n", __FUNCTION__);
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet); /* for now hard coded control edge */

    return status;
}


static fbe_status_t 
sas_physical_drive_mode_select_10_block_size_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;
    fbe_scsi_error_code_t error = 0;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
    }
    else
    {
        sas_physical_drive_get_scsi_error_code(sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        sas_physical_drive_set_control_status(error, payload_control_operation);

    }

    /* Release payload and buffer. */
    fbe_transport_release_buffer(sg_list);    
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
   

    /* If no errors on mode select then send format cmd*/
    if (status == FBE_STATUS_OK && 
        error == 0)
    {
        fbe_sas_physical_drive_send_format_block_size(sas_physical_drive, packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,  FBE_TRACE_LEVEL_ERROR,
                                "%s Not sending Format.  status:%d, error:%d\n", __FUNCTION__, status, error);
    }

    return status;
}



static fbe_status_t
fbe_sas_physical_drive_send_format_block_size(fbe_sas_physical_drive_t* sas_physical_drive, fbe_packet_t* packet)
{
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_cdb_operation_t *           payload_cdb_operation = NULL;
    fbe_sg_element_t  *                     sg_list = NULL;     
    fbe_u8_t *                              buffer = NULL;
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    const fbe_u32_t param_list_size = 4;
        
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,  FBE_TRACE_LEVEL_INFO,
                            "%s etnry\n", __FUNCTION__);
    
    
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n", __FUNCTION__);
        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = param_list_size;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);  /* data starts after sg_list terminator */
    sg_list[1].count = 0;
    sg_list[1].address = NULL;
        
    fbe_payload_ex_set_sg_list(payload, sg_list, 1);    
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, param_list_size);                     

    sas_physical_drive_cdb_build_format(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_FORMAT_TIMEOUT, sg_list[0].address, param_list_size);
    
    fbe_transport_set_completion_function(packet, sas_physical_drive_send_format_block_size_completion, sas_physical_drive);
                                              
    status = fbe_ssp_transport_send_functional_packet(&sas_physical_drive->ssp_edge, packet);
    
    return status;          
}


static fbe_status_t
sas_physical_drive_send_format_block_size_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = (fbe_sas_physical_drive_t *)context;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_sg_element_t * sg_list = NULL;
        
    payload = fbe_transport_get_payload_ex(packet);    
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   "%s Invalid packet status: 0x%X\n",
                                                   __FUNCTION__, status);
    }
    else
    {
        /* caution... this call can initiate DiskCollect if last two args are not NULL.  This means if we have any issues with this cmd
           that DC will attempt to write to the drive, which could also get expected errors if format was initiated.  Drive could then
           be shot.  Review code path before changing the following function. */
        sas_physical_drive_get_scsi_error_code((fbe_sas_physical_drive_t*)sas_physical_drive, packet, payload_cdb_operation, &error, NULL, NULL);

        if (error != 0)
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                       "%s cmd failed. error:0x%x\n", __FUNCTION__, error);

            fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        }
        else // successfully sent
        {            
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                       "%s cmd successful\n", __FUNCTION__);
        }
    }
    
    /* Release payload and buffer. */    
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    fbe_transport_release_buffer(sg_list);
        
    return status;
}   

