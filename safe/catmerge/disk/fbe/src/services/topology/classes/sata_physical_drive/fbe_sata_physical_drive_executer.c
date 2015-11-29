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

#include "fbe/fbe_types.h"
#include "fbe/fbe_sata_interface.h"
#include "sata_physical_drive_private.h"
#include "fbe_transport_memory.h"
#include "base_physical_drive_private.h"

/* Forward declarations */
static fbe_status_t fbe_sata_physical_drive_process_block_operation(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);

static fbe_status_t sata_physical_drive_read_fpdma_queued(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_read_fpdma_queued_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t sata_physical_drive_write_fpdma_queued(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_write_fpdma_queued_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t sata_physical_drive_write_same(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_write_same_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t sata_physical_drive_verify(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_verify_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t sata_physical_drive_write_verify(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);
static fbe_status_t sata_physical_drive_write_verify_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_write_verify_memory_alloc_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);
static fbe_status_t sata_physical_drive_write_verify_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t sata_physical_drive_identify(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);

static fbe_status_t sata_physical_drive_identify_device_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_read_native_max_address_ext_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_read_test_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_write_inscription_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_read_inscription_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_check_power_mode_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_execute_device_diag_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_flush_write_cache_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_disable_write_cache_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_enable_rla_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_set_pio_mode_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_set_udma_mode_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_enable_smart_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_enable_smart_autosave_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_smart_return_status_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_smart_read_attributes_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_sct_set_read_timer_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_sct_set_write_timer_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_reset_device_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_firmware_download_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_set_power_save_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_spin_down_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sata_physical_drive_write_uncorrectable_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

fbe_status_t 
fbe_sata_physical_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_transport_id_t transport_id;
    fbe_status_t status;
    fbe_sata_physical_drive_t *sata_physical_drive;
    sata_physical_drive = (fbe_sata_physical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    /* First we need to figure out to what transport this packet belong. */
    fbe_transport_get_transport_id(packet, &transport_id);
    switch (transport_id)
    {
        case FBE_TRANSPORT_ID_DISCOVERY:
            status = fbe_base_physical_drive_io_entry(object_handle, packet);
            break;
        case FBE_TRANSPORT_ID_BLOCK:
            status = fbe_base_physical_drive_bouncer_entry((fbe_base_physical_drive_t * )sata_physical_drive, packet);
            break;
        default:
            fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s, Uknown transport id = %X \n",
                                    __FUNCTION__, transport_id);

            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            break;
    };

    return status;
}

fbe_status_t
fbe_sata_physical_drive_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;

    sata_physical_drive = (fbe_sata_physical_drive_t *)context;

    status = fbe_sata_physical_drive_process_block_operation(sata_physical_drive, packet);
    return status;
}

fbe_status_t 
fbe_sata_physical_drive_process_block_operation(fbe_sata_physical_drive_t *sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_block_operation_opcode_t block_operation_opcode;

    payload = fbe_transport_get_payload_ex(packet);

    block_operation = fbe_payload_ex_get_block_operation(payload); 
    if(block_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_get_block_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_block_get_opcode(block_operation, &block_operation_opcode);

    switch(block_operation_opcode) {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            status = sata_physical_drive_read_fpdma_queued(sata_physical_drive, packet);
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
            status = sata_physical_drive_write_fpdma_queued(sata_physical_drive, packet);
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME:
            status = sata_physical_drive_write_same(sata_physical_drive, packet);
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
            status = sata_physical_drive_verify(sata_physical_drive, packet);
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
            status = sata_physical_drive_write_verify(sata_physical_drive, packet);
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_IDENTIFY:
            status = sata_physical_drive_identify(sata_physical_drive, packet);
            break;
        default:
            fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s, Uknown block_operation_opcode %X\n",
                                    __FUNCTION__, block_operation_opcode);

            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return status;
}

static fbe_status_t 
sata_physical_drive_read_fpdma_queued(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    fbe_lba_t lba;
    fbe_block_count_t block_count;
    fbe_block_size_t block_size;

    fbe_u8_t tag;
    
    payload = fbe_transport_get_payload_ex(packet);

    block_operation = fbe_payload_ex_get_block_operation(payload);       

    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_block_get_lba(block_operation, &lba);
    fbe_payload_block_get_block_count(block_operation, &block_count);
    fbe_payload_block_get_block_size(block_operation, &block_size);

    if ((block_size *block_count) >FBE_U32_MAX)
    {
       /* we do not expect the transfer count to go beyond 32bit
         */
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, Invalid transfer count 0x%llx \n", 
                                 __FUNCTION__,
                (unsigned long long)(block_size *block_count));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_transfer_count(fis_operation, (fbe_u32_t)(block_size * block_count));

    fbe_payload_fis_build_read_fpdma_queued(fis_operation, lba, block_count, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    status = fbe_transport_get_tag(packet, &tag);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_sata_physical_drive_allocate_tag failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_tag(fis_operation, tag);

    fbe_transport_set_completion_function(packet, sata_physical_drive_read_fpdma_queued_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_read_fpdma_queued_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)context;
    fbe_drive_threshold_and_exceptions_t threshold_rec = {0};
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_port_request_status_t request_status;
    fbe_drive_configuration_handle_t drive_configuration_handle;
    fbe_payload_cdb_fis_io_status_t io_status;
    fbe_payload_cdb_fis_error_t payload_cdb_fis_error;
    fbe_lba_t bad_lba = 0;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_base_physical_drive_get_configuration_handle(base_physical_drive, &drive_configuration_handle);

    status = fbe_payload_fis_completion(fis_operation, /* IN */
                                        drive_configuration_handle, /* IN */
                                        &io_status, /* OUT */
                                        &payload_cdb_fis_error, /* OUT */
                                        &bad_lba); /* OUT */

    if(payload_cdb_fis_error != FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR)
    {
        status = fbe_drive_configuration_get_threshold_info(drive_configuration_handle, &threshold_rec);

        if (status != FBE_STATUS_OK)
        {
            /* if we can't get threshold info with a valid handle then something seriously went wrong */
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                           "%s failed to get threshold rec. Invalidating handle.\n", __FUNCTION__);
        }

#ifdef BLOCKSIZE_512_SUPPORTED 
        /* When/If 512 is supported it will need to fix the following DIEH code to handle FIS commands and errors.  Currently DIEH expects SCSI */
        status = fbe_base_physical_drive_io_fail((fbe_base_physical_drive_t *) sata_physical_drive, drive_configuration_handle, &threshold_rec, payload_cdb_fis_error, payload_cdb_operation->cdb[0], 
                                                 FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_SCSI_SENSE_KEY_NO_SENSE, FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO, FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO); /* dieh weight change disabled for sata*/
#endif
    } 
    else 
    {
        fbe_base_physical_drive_io_success((fbe_base_physical_drive_t *) sata_physical_drive);
    }
    
    
    block_operation = fbe_payload_ex_get_prev_block_operation(payload);
    fbe_payload_fis_get_request_status(fis_operation, &request_status);
    
    if (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__, request_status);
                                    
        
        /*fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);*/

        if (request_status == FBE_PORT_REQUEST_STATUS_SATA_NCQ_ERROR)
        {
            fbe_sata_physical_drive_parse_read_log_extended(sata_physical_drive, fis_operation, (fbe_payload_ex_t *)payload);
        }
        else
        {
            fbe_sata_physical_drive_handle_block_operation_error((fbe_payload_ex_t *)payload, request_status);
        }

        //Update error counts
        ((fbe_base_physical_drive_t *)sata_physical_drive)->physical_drive_error_counts.error_count++;
    }
    else
    {
        fbe_base_physical_drive_set_retry_msecs ((fbe_payload_ex_t *)payload, FBE_SCSI_CC_NOERR);
        fbe_payload_block_set_status(block_operation,   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    }
    
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    return FBE_STATUS_OK;
}

static fbe_status_t 
sata_physical_drive_write_fpdma_queued(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    fbe_lba_t lba;
    fbe_block_count_t block_count;
    fbe_block_size_t block_size;
    fbe_u8_t tag;
    
    payload = fbe_transport_get_payload_ex(packet);

    block_operation = fbe_payload_ex_get_block_operation(payload);       

    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_block_get_lba(block_operation, &lba);
    fbe_payload_block_get_block_count(block_operation, &block_count);
    fbe_payload_block_get_block_size(block_operation, &block_size);

    if ((block_size *block_count) >FBE_U32_MAX)
    {
       /* we do not expect the transfer count to go beyond 32bit
         */
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, Invalid transfer count 0x%llx \n", 
                                 __FUNCTION__,
                (unsigned long long)(block_size *block_count));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_payload_fis_set_transfer_count(fis_operation, (fbe_u32_t)(block_size * block_count));

    fbe_payload_fis_build_write_fpdma_queued(fis_operation, lba, block_count, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    status = fbe_transport_get_tag(packet, &tag);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_sata_physical_drive_allocate_tag failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_tag(fis_operation, tag);

    fbe_transport_set_completion_function(packet, sata_physical_drive_write_fpdma_queued_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_write_fpdma_queued_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)context;
    fbe_drive_threshold_and_exceptions_t threshold_rec = {0};
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_sg_descriptor_t * fis_sg_descriptor = NULL;
    fbe_port_request_status_t request_status;
    fbe_drive_configuration_handle_t drive_configuration_handle;
    fbe_payload_cdb_fis_io_status_t io_status;
    fbe_payload_cdb_fis_error_t payload_cdb_fis_error;
    fbe_lba_t bad_lba = 0;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_base_physical_drive_get_configuration_handle(base_physical_drive, &drive_configuration_handle);

    status = fbe_payload_fis_completion(fis_operation, /* IN */
                                        drive_configuration_handle, /* IN */
                                        &io_status, /* OUT */
                                        &payload_cdb_fis_error, /* OUT */
                                        &bad_lba); /* OUT */

    if(payload_cdb_fis_error != FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR)
    {
        status = fbe_drive_configuration_get_threshold_info(drive_configuration_handle, &threshold_rec);

        if (status != FBE_STATUS_OK)
        {
            /* if we can't get threshold info with a valid handle then something seriously went wrong */
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                           "%s failed to get threshold rec. Invalidating handle.\n", __FUNCTION__);
        }
#ifdef BLOCKSIZE_512_SUPPORTED 
        /* When/If 512 is supported it will need to fix the following DIEH code to handle FIS commands and errors.  Currently DIEH expects SCSI */
        status = fbe_base_physical_drive_io_fail((fbe_base_physical_drive_t *) sata_physical_drive, drive_configuration_handle, &threshold_rec, payload_cdb_fis_error, payload_cdb_operation->cdb[0], 
                                                 FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_SCSI_SENSE_KEY_NO_SENSE, FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO, FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO); /* dieh weight change disabled for sata*/
#endif
    } 
    else 
    {
        fbe_base_physical_drive_io_success((fbe_base_physical_drive_t *) sata_physical_drive);
    }

    block_operation = fbe_payload_ex_get_prev_block_operation(payload);

    fbe_payload_fis_get_sg_descriptor(fis_operation, &fis_sg_descriptor);
    fbe_payload_sg_descriptor_set_repeat_count(fis_sg_descriptor, 0);

    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)
    {
        if (request_status == FBE_PORT_REQUEST_STATUS_SATA_NCQ_ERROR)
        {
            fbe_sata_physical_drive_parse_read_log_extended(sata_physical_drive, fis_operation, (fbe_payload_ex_t *)payload);
        }
        else
        {
            fbe_sata_physical_drive_handle_block_operation_error((fbe_payload_ex_t *)payload, request_status);
        }

        /*fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);*/
        
        //Update error counts
        ((fbe_base_physical_drive_t *)sata_physical_drive)->physical_drive_error_counts.error_count++;
    }
    else
    {
        fbe_base_physical_drive_set_retry_msecs ((fbe_payload_ex_t *)payload, FBE_SCSI_CC_NOERR);
        fbe_payload_block_set_status(block_operation,   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    }
    
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    return FBE_STATUS_OK;
}

static fbe_status_t 
sata_physical_drive_write_same(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    fbe_lba_t lba;
    fbe_block_count_t block_count;
    fbe_block_size_t block_size;
    fbe_u8_t tag;
    fbe_payload_sg_descriptor_t * block_sg_descriptor = NULL;
    fbe_payload_sg_descriptor_t * fis_sg_descriptor = NULL;
    fbe_u32_t repeat_count;
    fbe_u32_t sg_list_byte_count;
    fbe_sg_element_t * sg_list = NULL;
    fbe_sg_element_t * pre_sg_list = NULL;
    fbe_sg_element_t * post_sg_list = NULL;
    fbe_block_size_t optimum_block_size;
    fbe_u32_t new_repeat_count;
    fbe_u32_t optimum_blocks;

    payload = fbe_transport_get_payload_ex(packet);

    block_operation = fbe_payload_ex_get_block_operation(payload);       

    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_block_get_lba(block_operation, &lba);
    fbe_payload_block_get_block_count(block_operation, &block_count);
    fbe_payload_block_get_block_size(block_operation, &block_size);

    fbe_payload_block_get_sg_descriptor(block_operation, &block_sg_descriptor);
    fbe_payload_sg_descriptor_get_repeat_count(block_sg_descriptor, &repeat_count);
    /* temp fix for EFD drives */
    if ((repeat_count == 0) && (((fbe_base_physical_drive_t *)sata_physical_drive)->block_size == 520)) {
        repeat_count = 1;
    }
    fbe_payload_fis_get_sg_descriptor(fis_operation, &fis_sg_descriptor);

    if ((block_size *block_count) >FBE_U32_MAX)
    {
       /* we do not expect the transfer count to go beyond 32bit
         */
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, Invalid transfer count 0x%llx \n", 
                                 __FUNCTION__,
                (unsigned long long)(block_size *block_count));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_fis_set_transfer_count(fis_operation, (fbe_u32_t)(block_size * block_count));
    fbe_payload_block_get_optimum_block_size(block_operation, &optimum_block_size);

    if ((block_count / optimum_block_size) >FBE_U32_MAX)
    {
       /* we do not expect the optimum blocks to go beyond 32bit
         */
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, Invalid optimum blocks 0x%llx \n", 
                                 __FUNCTION__,
                (unsigned long long)(block_count / optimum_block_size));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    optimum_blocks = (fbe_u32_t)(block_count / optimum_block_size);
    new_repeat_count = repeat_count * optimum_blocks;

    fbe_payload_sg_descriptor_set_repeat_count(fis_sg_descriptor, new_repeat_count);

    /* Sanity checking (should be removed from production code) */
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    fbe_payload_ex_get_pre_sg_list(payload, &pre_sg_list);
    fbe_payload_ex_get_post_sg_list(payload, &post_sg_list);

    sg_list_byte_count = fbe_sg_element_count_list_bytes(pre_sg_list);
    sg_list_byte_count += fbe_sg_element_count_list_bytes(sg_list);
    sg_list_byte_count += fbe_sg_element_count_list_bytes(post_sg_list);

    if(((fbe_u64_t)block_size * block_count) !=  ((fbe_u64_t)sg_list_byte_count * new_repeat_count)) {
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, Invalid sg_list_byte_count\n",
                                __FUNCTION__);

    }

    fbe_payload_fis_build_write_fpdma_queued(fis_operation, lba, block_count, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    status = fbe_transport_get_tag(packet, &tag);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_sata_physical_drive_allocate_tag failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_tag(fis_operation, tag);

    fbe_transport_set_completion_function(packet, sata_physical_drive_write_same_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_write_same_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)context;
    fbe_drive_threshold_and_exceptions_t threshold_rec = {0};
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_port_request_status_t request_status;
    fbe_drive_configuration_handle_t drive_configuration_handle;
    fbe_payload_cdb_fis_io_status_t io_status;
    fbe_payload_cdb_fis_error_t payload_cdb_fis_error;
    fbe_lba_t bad_lba = 0;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_base_physical_drive_get_configuration_handle(base_physical_drive, &drive_configuration_handle);

    status = fbe_payload_fis_completion(fis_operation, /* IN */
                                        drive_configuration_handle, /* IN */
                                        &io_status, /* OUT */
                                        &payload_cdb_fis_error, /* OUT */
                                        &bad_lba); /* OUT */

    if(payload_cdb_fis_error != FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR)
    {
        status = fbe_drive_configuration_get_threshold_info(drive_configuration_handle, &threshold_rec);

        if (status != FBE_STATUS_OK)
        {
            /* if we can't get threshold info with a valid handle then something seriously went wrong */
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                           "%s failed to get threshold rec. Invalidating handle.\n", __FUNCTION__);
        }

#ifdef BLOCKSIZE_512_SUPPORTED 
        /* When/If 512 is supported it will need to fix the following DIEH code to handle FIS commands and errors.  Currently DIEH expects SCSI */
        status = fbe_base_physical_drive_io_fail((fbe_base_physical_drive_t *) sata_physical_drive, drive_configuration_handle, &threshold_rec, payload_cdb_fis_error, payload_cdb_operation->cdb[0], 
                                                 FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_SCSI_SENSE_KEY_NO_SENSE, FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO, FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO); /* dieh weight change disabled for sata*/
#endif
    } 
    else 
    {
        fbe_base_physical_drive_io_success((fbe_base_physical_drive_t *) sata_physical_drive);
    }
    
    block_operation = fbe_payload_ex_get_prev_block_operation(payload);

    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)
    {
        if (request_status == FBE_PORT_REQUEST_STATUS_SATA_NCQ_ERROR)
        {
            fbe_sata_physical_drive_parse_read_log_extended(sata_physical_drive, fis_operation, (fbe_payload_ex_t *)payload);
        }
        else
        {
            fbe_sata_physical_drive_handle_block_operation_error((fbe_payload_ex_t *)payload, request_status);
        }
        
        /*fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);*/

        //Update error counts
        ((fbe_base_physical_drive_t *)sata_physical_drive)->physical_drive_error_counts.error_count++;
    }
    else
    {
        fbe_base_physical_drive_set_retry_msecs ((fbe_payload_ex_t *)payload, FBE_SCSI_CC_NOERR);
        fbe_payload_block_set_status(block_operation,   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    }
    
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    return FBE_STATUS_OK;
}

static fbe_status_t 
sata_physical_drive_identify(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{

    fbe_sg_element_t *identity = NULL;    

    fbe_base_physical_drive_info_t * drive_info = NULL;
    
    fbe_payload_ex_t * payload = NULL;
        
    payload = fbe_transport_get_payload_ex(packet);
    
    fbe_payload_ex_get_sg_list(payload, &identity, NULL);
    
    if (identity != NULL && identity->address != NULL)
    {                       
        fbe_base_physical_drive_get_drive_info((fbe_base_physical_drive_t *) sata_physical_drive, &drive_info);

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
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
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

fbe_status_t 
fbe_sata_physical_drive_identify_device(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t * sg_list = NULL;

    payload = fbe_transport_get_payload_ex(packet);

    control_operation = fbe_payload_ex_get_control_operation(payload);

    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate buffer for identify device info */
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = FBE_SATA_IDENTIFY_DEVICE_DATA_SIZE;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SATA_IDENTIFY_DEVICE_DATA_SIZE);

    fbe_payload_fis_build_identify_device(fis_operation, FBE_SATA_PHYSICAL_DRIVE_EXIT_STANDBY_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_identify_device_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_identify_device_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;   
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_physical_drive_information_t * drive_info = NULL;
    fbe_u8_t * identify_device_data = NULL;
    fbe_status_t status;
    fbe_sata_drive_status_t drive_status;
    fbe_port_request_status_t request_status;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &drive_info);  

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);

    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    /* Check port status & FIS operation status */ 
    fbe_payload_fis_get_request_status(fis_operation, &request_status);
    
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__, request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        fbe_transport_release_buffer(sg_list);
        return FBE_STATUS_OK;
    } 

    identify_device_data = (fbe_u8_t *)sg_list[0].address;
    
    drive_status = fbe_sata_physical_drive_parse_identify(sata_physical_drive, identify_device_data, drive_info);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    
    /*if drive status returned from parse identify is retry, set control status to fail so that 
      identify device rotrary condition does not get cleared*/
    if(drive_status == FBE_SATA_DRIVE_STATUS_NEED_RETRY)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else if(drive_status == FBE_SATA_DRIVE_STATUS_PIO_MODE_UNSUPPORTED)
    {
        fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sata_physical_drive, FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_PIO_MODE_UNSUPPORTED);
        
        /* We need to set the BO:FAIL lifecycle condition */
        status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const, (fbe_base_object_t*)sata_physical_drive, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

        if (status != FBE_STATUS_OK) 
        {
           fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                    "%s can't set BO:FAIL condition, status: 0x%X\n",
                                    __FUNCTION__, status);
        }  
    }
    else if(drive_status == FBE_SATA_DRIVE_STATUS_UDMA_MODE_UNSUPPORTED)
    {
        fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sata_physical_drive, FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_UDMA_MODE_UNSUPPORTED);

        /* We need to set the BO:FAIL lifecycle condition */
        status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const, (fbe_base_object_t*)sata_physical_drive, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

        if (status != FBE_STATUS_OK) 
        {
           fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                    "%s can't set BO:FAIL condition, status: 0x%X\n",
                                    __FUNCTION__, status);
        }  
    }

    fbe_payload_ex_release_fis_operation(payload, fis_operation);
    fbe_transport_release_buffer(sg_list);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_read_native_max_address_ext(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);

    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SATA_READ_NATIVE_MAX_ADDRESS_EXT_DATA_SIZE);

    fbe_payload_fis_build_read_native_max_address_ext(fis_operation, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_read_native_max_address_ext_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_read_native_max_address_ext_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_lba_t block_count;    
    fbe_port_request_status_t request_status;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* Check port status & FIS operation status */
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__,
                                                                request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        return FBE_STATUS_OK;
    } 

    /* If we get there we should not have errors */
    block_count = 0;
    block_count = (fbe_lba_t)fis_operation->response_buffer[10] << 40;
    block_count += (fbe_lba_t)fis_operation->response_buffer[9] << 32;
    block_count += (fbe_lba_t)fis_operation->response_buffer[8] << 24;
    block_count += (fbe_lba_t)fis_operation->response_buffer[6] << 16;
    block_count += (fbe_lba_t)fis_operation->response_buffer[5] << 8;
    block_count += (fbe_lba_t)fis_operation->response_buffer[4];

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                        FBE_TRACE_LEVEL_INFO,
                        "%s block_count %llX\n", __FUNCTION__,
            (unsigned long long)block_count);

    fbe_base_physical_drive_set_block_count((fbe_base_physical_drive_t *) sata_physical_drive, block_count);


    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_write_inscription(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_u32_t block_size = ((fbe_base_physical_drive_t *)sata_physical_drive)->block_size;

    payload = fbe_transport_get_payload_ex(packet);

    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate buffer for SDD inscription block info */
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = block_size;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    //write inscription template according to model number
    if (fbe_equal_memory(sata_physical_drive->sata_drive_info.model_num, "ST31000340NS", 12))
    {   
        /* Return the inscription data from the template */
        fbe_copy_memory(sg_list[0].address, sata_drive_inscription_moose_data_template, FBE_SATA_DISK_DESIGNATOR_BLOCK_SIZE);
    }
    else if (fbe_equal_memory(sata_physical_drive->sata_drive_info.model_num, "SAMSUN", 6))
    {   
        if (fbe_equal_memory(&sata_physical_drive->sata_drive_info.model_num[8], "MCCPE2HGGAXP", 12)) {
            /* SAMSUNG 200GB drive */
            fbe_copy_memory(sg_list[0].address, sata_flash_drive_samsung_200gb_inscription_template, block_size);
        } else {
            /* SAMSUNG 100GB drive */
            fbe_copy_memory(sg_list[0].address, sata_flash_drive_samsung_100gb_inscription_template, block_size);
        }
    }
    else
    {
        /* Return the Hitachi Gemini K inscription data from the template */
        fbe_copy_memory(sg_list[0].address, sata_drive_inscription_geminik_data_template, FBE_SATA_DISK_DESIGNATOR_BLOCK_SIZE);
    }  

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    fbe_payload_fis_set_transfer_count(fis_operation, block_size);

    fbe_payload_fis_build_write_fpdma_queued(fis_operation, sata_physical_drive->sata_drive_info.native_capacity, 1, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_write_inscription_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_write_inscription_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;   
    fbe_port_request_status_t request_status;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL); 


    /* Check port status & FIS operation status */ 
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__,
                                                                request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        fbe_transport_release_buffer(sg_list);
        return FBE_STATUS_OK;
    } 

    /* If we get there we should not have errors */
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_release_buffer(sg_list);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_read_inscription(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_u32_t block_size = ((fbe_base_physical_drive_t *)sata_physical_drive)->block_size;

    payload = fbe_transport_get_payload_ex(packet);


    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate buffer for SDD inscription block info */
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = block_size;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    fbe_payload_fis_set_transfer_count(fis_operation, block_size);

    fbe_payload_fis_build_read_fpdma_queued(fis_operation, 
                                            sata_physical_drive->sata_drive_info.native_capacity,
               1,
              FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);


    fbe_transport_set_completion_function(packet, sata_physical_drive_read_inscription_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_read_inscription_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_sata_drive_status_t drive_status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_u8_t * inscription_data = NULL;
    fbe_physical_drive_information_t * drive_info = NULL;
    fbe_port_request_status_t request_status;
    fbe_bool_t fail_the_drive = FBE_FALSE;

    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
   
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &drive_info);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    inscription_data = (fbe_u8_t *)sg_list[0].address;

    /* Check port status & FIS operation status */
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__,
                                                                request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);       
        fbe_transport_release_buffer(sg_list);
        return FBE_STATUS_OK;
    } 

    drive_status = fbe_sata_physical_drive_parse_inscription(sata_physical_drive, 
                                                             inscription_data,
                                                             drive_info);   
    
    /*for all status cases status set control to be ok*/
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    
    if (drive_status == FBE_SATA_DRIVE_STATUS_INVALID_INSCRIPTION)
    {
        fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sata_physical_drive, FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_INVALID_INSCRIPTION);
        fail_the_drive = FBE_TRUE;
    }
    
    if (drive_status == FBE_SATA_DRIVE_STATUS_INVALID)
    {
        fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sata_physical_drive, FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SET_DEV_INFO);
        fail_the_drive = FBE_TRUE;
    }
    
    if (fail_the_drive == FBE_TRUE)
    {
        //Fail the drive        
        status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const, 
                                        (fbe_base_object_t*)sata_physical_drive, 
                                         FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
        if (status != FBE_STATUS_OK) 
        {
           fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                    "%s can't set BO:FAIL condition, status: 0x%X\n",
                                    __FUNCTION__, status);
        }  
    }
    
    fbe_payload_ex_release_fis_operation(payload, fis_operation);
    fbe_transport_release_buffer(sg_list);

    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_sata_physical_drive_check_power_mode(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);

    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    fbe_payload_fis_build_check_power_mode(fis_operation, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_check_power_mode_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_check_power_mode_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL; 
    fbe_port_request_status_t request_status;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* Check port status & FIS operation status */
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__,
                                                                request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        return FBE_STATUS_OK;
    } 

    /* If we get there we should not have errors */
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_execute_device_diag(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);


    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    fbe_payload_fis_build_execute_device_diag(fis_operation, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_execute_device_diag_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_execute_device_diag_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_port_request_status_t request_status;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* Check port status & FIS operation status */ 
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__,
                                                                request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        return FBE_STATUS_OK;
    } 

    /* If we get there we should not have errors */
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_flush_write_cache(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);


    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    fbe_payload_fis_build_flush_write_cache(fis_operation, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_flush_write_cache_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_flush_write_cache_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;     
    fbe_port_request_status_t request_status;

    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* Check port status & FIS operation status */
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__, request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        return FBE_STATUS_OK;
    } 

    /* If we get there we should not have errors */
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_disable_write_cache(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
     
    payload = fbe_transport_get_payload_ex(packet);


    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    fbe_payload_fis_build_disable_write_cache(fis_operation, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_disable_write_cache_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_disable_write_cache_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_port_request_status_t request_status;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* Check port status & FIS operation status */ 
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__, request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        return FBE_STATUS_OK;
    } 

    /* If we get there we should not have errors */
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_enable_rla(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
     
    payload = fbe_transport_get_payload_ex(packet);


    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    fbe_payload_fis_build_enable_rla(fis_operation, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_enable_rla_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_enable_rla_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL; 
    fbe_port_request_status_t request_status;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* Check port status & FIS operation status */
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__, request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);        
        return FBE_STATUS_OK;
    } 

    /* If we get there we should not have errors */
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_set_pio_mode(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);


    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    fbe_payload_fis_build_set_pio_mode(fis_operation, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, sata_physical_drive->sata_drive_info.PIOMode);

    fbe_transport_set_completion_function(packet, sata_physical_drive_set_pio_mode_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_set_pio_mode_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_port_request_status_t request_status;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* Check port status & FIS operation status */ 
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__, request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        return FBE_STATUS_OK;
    } 

    /* If we get there we should not have errors */
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_set_udma_mode(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);


    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    fbe_payload_fis_build_set_udma_mode(fis_operation, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, sata_physical_drive->sata_drive_info.UDMAMode);

    fbe_transport_set_completion_function(packet, sata_physical_drive_set_udma_mode_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_set_udma_mode_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_port_request_status_t request_status;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* Check port status & FIS operation status */ 
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__, request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        return FBE_STATUS_OK;
    } 

    /* If we get there we should not have errors */
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_enable_smart(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);


    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    fbe_payload_fis_build_enable_smart(fis_operation, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_enable_smart_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_enable_smart_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_port_request_status_t request_status;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* Check port status & FIS operation status */ 
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__, request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        return FBE_STATUS_OK;
    } 

    /* If we get there we should not have errors */
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_enable_smart_autosave(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);


    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    fbe_payload_fis_build_enable_smart_autosave(fis_operation, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_enable_smart_autosave_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_enable_smart_autosave_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_port_request_status_t request_status;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* Check port status & FIS operation status */ 
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__, request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        return FBE_STATUS_OK;
    } 

    /* If we get there we should not have errors */
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_smart_return_status(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
      
    payload = fbe_transport_get_payload_ex(packet);


    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    fbe_payload_fis_build_smart_return_status(fis_operation, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_smart_return_status_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_smart_return_status_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_port_request_status_t request_status;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* Check port status & FIS operation status */ 
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__, request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        return FBE_STATUS_OK;
    } 

    /* If we get there we should not have errors */

    //Check GLIST

    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_smart_read_attributes(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t * sg_list = NULL;

    payload = fbe_transport_get_payload_ex(packet);


    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate buffer for SMART attributes info */
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = FBE_SATA_SMART_ATTRIBUTES_DATA_SIZE;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SATA_SMART_ATTRIBUTES_DATA_SIZE);

    fbe_payload_fis_build_smart_read_attributes(fis_operation, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_smart_read_attributes_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_smart_read_attributes_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_port_request_status_t request_status;

    /* fbe_u64_t max_lba; */
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* Check port status & FIS operation status */ 
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__, request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        fbe_transport_release_buffer(sg_list);
        return FBE_STATUS_OK;
    } 

    //SAVE ATTRIBUTE DATA

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    fbe_payload_ex_release_fis_operation(payload, fis_operation);
    fbe_transport_release_buffer(sg_list);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_sct_set_read_timer(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t * sg_list = NULL;

    payload = fbe_transport_get_payload_ex(packet);


    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate buffer for identify device info */
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    //Set up SCT Read ERT timer buffer
    buffer[FBE_SATA_SCT_KEY_SECTOR_ACTION_CODE_OFFSET] = FBE_SATA_SCT_ERC_ACT_CODE;
    buffer[FBE_SATA_SCT_KEY_SECTOR_FUNCTION_CODE_OFFSET] = FBE_SATA_SCT_ERC_SET_NEW_VAL_FN_CODE;
    buffer[FBE_SATA_SCT_KEY_SECTOR_SELECTION_CODE_OFFSET] = FBE_SATA_SCT_ERC_READ_TIMER_SELCTN_CODE;
    buffer[FBE_SATA_SCT_KEY_SECTOR_VALUE_OFFSET] = FBE_SATA_SCT_ERC_TIME_LIMIT;

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = FBE_SMART_SCT_DATA_SIZE;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SMART_SCT_DATA_SIZE);

    fbe_payload_fis_build_sct_set_timer(fis_operation, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_sct_set_read_timer_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_sct_set_read_timer_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_port_request_status_t request_status;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* Check port status & FIS operation status */ 
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__, request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_transport_release_buffer(sg_list);
        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        return FBE_STATUS_OK;
    } 
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    fbe_payload_ex_release_fis_operation(payload, fis_operation);
    fbe_transport_release_buffer(sg_list);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_sct_set_write_timer(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t * sg_list = NULL;

    payload = fbe_transport_get_payload_ex(packet);

    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate buffer for identify device info */
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //Set up SCT Write ERT timer buffer
    buffer[FBE_SATA_SCT_KEY_SECTOR_ACTION_CODE_OFFSET] = FBE_SATA_SCT_ERC_ACT_CODE;
    buffer[FBE_SATA_SCT_KEY_SECTOR_FUNCTION_CODE_OFFSET] = FBE_SATA_SCT_ERC_SET_NEW_VAL_FN_CODE;
    buffer[FBE_SATA_SCT_KEY_SECTOR_SELECTION_CODE_OFFSET] = FBE_SATA_SCT_ERC_WRITE_TIMER_SELCTN_CODE;
    buffer[FBE_SATA_SCT_KEY_SECTOR_VALUE_OFFSET] = FBE_SATA_SCT_ERC_TIME_LIMIT;

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = FBE_SMART_SCT_DATA_SIZE;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SMART_SCT_DATA_SIZE);

    fbe_payload_fis_build_sct_set_timer(fis_operation, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_sct_set_write_timer_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_sct_set_write_timer_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_port_request_status_t request_status;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* Check port status & FIS operation status */ 
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__, request_status);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        fbe_transport_release_buffer(sg_list);
        return FBE_STATUS_OK;
    } 
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    fbe_payload_ex_release_fis_operation(payload, fis_operation);
    fbe_transport_release_buffer(sg_list);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_read_test(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_lba_t lba;
    fbe_block_size_t block_size;    
    
    payload = fbe_transport_get_payload_ex(packet);

    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Allocate buffer for identify device info */
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_physical_drive_get_capacity((fbe_base_physical_drive_t *) sata_physical_drive, &lba, &block_size);

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = block_size;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    fbe_payload_fis_set_transfer_count(fis_operation, block_size);

    fbe_payload_fis_build_read_fpdma_queued(fis_operation, 0 /*lba*/, 1/*block_count*/, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);
        
    /*  We are in ACTIVATE state, so it is safe assumption that there is no other outstanding I/O's */
    fbe_payload_fis_set_tag(fis_operation, 0);

    fbe_transport_set_completion_function(packet, sata_physical_drive_read_test_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_read_test_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_u8_t tag;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* Check FIS operation status */
    if(fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

        fbe_payload_fis_trace_fis_buff(fis_operation);
        fbe_payload_fis_trace_response_buff(fis_operation);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        fbe_transport_release_buffer(sg_list);
        return FBE_STATUS_OK;
    } 

    /* If we get there we should not have errors */
    fbe_payload_fis_get_tag(fis_operation, &tag);
    

    fbe_payload_ex_release_fis_operation(payload, fis_operation);
    fbe_transport_release_buffer(sg_list);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    return FBE_STATUS_OK;
}

static fbe_status_t 
sata_physical_drive_verify(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;   
    fbe_lba_t lba;
    fbe_block_count_t block_count;
    fbe_block_size_t block_size;
    fbe_lba_t drive_block_count;
    fbe_block_size_t drive_block_size;
    fbe_u8_t tag;
    fbe_payload_sg_descriptor_t * fis_sg_descriptor = NULL;
    fbe_u32_t repeat_count; 

    payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);       

    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_block_get_lba(block_operation, &lba);
    fbe_payload_block_get_block_count(block_operation, &block_count);
    fbe_payload_block_get_block_size(block_operation, &block_size);
    
    //check base_physical_drive block_size and switch to get appropriate bit bucket.
    fbe_base_physical_drive_get_capacity((fbe_base_physical_drive_t *) sata_physical_drive, &drive_block_count, &drive_block_size);
    
    switch(drive_block_size)
    {
        case 512:
            sg_list = fbe_memory_get_bit_bucket_sgl(FBE_MEMORY_BIT_BUCKET_SGL_TYPE_512);
            break;
        case 520:
            sg_list = fbe_memory_get_bit_bucket_sgl(FBE_MEMORY_BIT_BUCKET_SGL_TYPE_520);
            break;
        default:
            fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s, Unsupported block count %X\n",
                                    __FUNCTION__, drive_block_size);

            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_GENERIC_FAILURE;
            break; 
    }


    fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    /* Mini-port will reject IO transfer that exceeds 1M bytes */
    if (block_count > FBE_MAX_TRANSFER_BLOCKS) 
    {
        block_count = FBE_MAX_TRANSFER_BLOCKS;
    }

    fbe_payload_fis_set_transfer_count(fis_operation, (fbe_u32_t)(block_size * block_count));
    fbe_payload_fis_get_sg_descriptor(fis_operation, &fis_sg_descriptor);

    repeat_count = (fbe_u32_t)block_count;

    fbe_payload_sg_descriptor_set_repeat_count(fis_sg_descriptor, repeat_count);

    fbe_payload_fis_build_read_fpdma_queued(fis_operation, lba, block_count, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    status = fbe_transport_get_tag(packet, &tag);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_sata_physical_drive_allocate_tag failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_tag(fis_operation, tag);

    fbe_transport_set_completion_function(packet, sata_physical_drive_verify_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_verify_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)context;
    fbe_drive_threshold_and_exceptions_t threshold_rec = {0};
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_port_request_status_t request_status;
    fbe_drive_configuration_handle_t drive_configuration_handle;
    fbe_payload_cdb_fis_io_status_t io_status;
    fbe_payload_cdb_fis_error_t payload_cdb_fis_error;
    fbe_lba_t bad_lba = 0;
    fbe_lba_t lba = 0;
    fbe_block_count_t block_count;
    fbe_block_size_t block_size;
    fbe_u32_t transferred_count;
    fbe_u32_t transferred_block_count;

    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    
    fbe_base_physical_drive_get_configuration_handle(base_physical_drive, &drive_configuration_handle);

    status = fbe_payload_fis_completion(fis_operation, /* IN */
                                        drive_configuration_handle, /* IN */
                                        &io_status, /* OUT */
                                        &payload_cdb_fis_error, /* OUT */
                                        &bad_lba); /* OUT */

    if(payload_cdb_fis_error != FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR)
    {
        status = fbe_drive_configuration_get_threshold_info(drive_configuration_handle, &threshold_rec);

        if (status != FBE_STATUS_OK)
        {
            /* if we can't get threshold info with a valid handle then something seriously went wrong */
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                           "%s failed to get threshold rec. Invalidating handle.\n", __FUNCTION__);
        }

#ifdef BLOCKSIZE_512_SUPPORTED 
        /* When/If 512 is supported it will need to fix the following DIEH code to handle FIS commands and errors.  Currently DIEH expects SCSI */
        status = fbe_base_physical_drive_io_fail((fbe_base_physical_drive_t *) sata_physical_drive, drive_configuration_handle, &threshold_rec, payload_cdb_fis_error, payload_cdb_operation->cdb[0], 
                                                 FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_SCSI_SENSE_KEY_NO_SENSE, FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO, FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO); /* dieh weight change disabled for sata*/
#endif
    } 
    else 
    {
        fbe_base_physical_drive_io_success((fbe_base_physical_drive_t *) sata_physical_drive);
    }

    block_operation = fbe_payload_ex_get_prev_block_operation(payload);

    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    "%s, Error: Port Status = %X \n",
                                    __FUNCTION__, request_status);
        if (request_status == FBE_PORT_REQUEST_STATUS_SATA_NCQ_ERROR)
        {
            fbe_sata_physical_drive_parse_read_log_extended(sata_physical_drive, fis_operation, (fbe_payload_ex_t *)payload);
        }
        else
        {
            fbe_sata_physical_drive_handle_block_operation_error((fbe_payload_ex_t *)payload, request_status);
        }

        //Update error counts
        ((fbe_base_physical_drive_t *)sata_physical_drive)->physical_drive_error_counts.error_count++;
    }
    else
    {
        fbe_payload_block_get_block_count(block_operation, &block_count);
        fbe_payload_block_get_block_size(block_operation, &block_size);
        fbe_payload_fis_get_transferred_count(fis_operation, &transferred_count);
        transferred_block_count = transferred_count/block_size;
        /* The read is trancated. It needs more processing. */
        if (block_count > transferred_block_count) 
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    "%s, block_count: %llu, transferred_count: %d, transferred_block: %d\n",
                                    __FUNCTION__,
                    (unsigned long long)block_count,
                    transferred_count, transferred_block_count);
            fbe_payload_block_get_lba(block_operation, &lba);
            lba += transferred_block_count;
            block_count -= transferred_block_count;
            fbe_payload_block_set_lba(block_operation, lba);
            fbe_payload_block_set_block_count(block_operation, block_count);
            fbe_payload_ex_release_fis_operation(payload, fis_operation);
            /* Send for process again. */
            status = sata_physical_drive_verify(sata_physical_drive, packet);
            if (status == FBE_STATUS_OK) {
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
            else
            {
                /* We are not expecting an error here. There must be some software errors.
                 * Anyway, let's complete the packet here.
                 */
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s, Error to start verify operation! \n",
                                    __FUNCTION__);
                fbe_payload_ex_set_scsi_error_code (payload, 0);
                fbe_payload_block_set_status(block_operation,   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                                                FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
                return FBE_STATUS_OK;    
            }
        }

        fbe_base_physical_drive_set_retry_msecs ((fbe_payload_ex_t *)payload, FBE_SCSI_CC_NOERR);
        fbe_payload_block_set_status(block_operation,   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    }
    
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    return FBE_STATUS_OK;    
}

static fbe_status_t 
sata_physical_drive_write_verify(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;
    fbe_lba_t lba;
    fbe_block_count_t block_count;
    fbe_block_size_t block_size;
    fbe_u8_t tag;
        
    payload = fbe_transport_get_payload_ex(packet);

    block_operation = fbe_payload_ex_get_block_operation(payload);       

    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_block_get_lba(block_operation, &lba);
    fbe_payload_block_get_block_count(block_operation, &block_count);
    fbe_payload_block_get_block_size(block_operation, &block_size);

    if ((block_size *block_count) >FBE_U32_MAX)
    {
       /* we do not expect the transfer count to go beyond 32bit
         */
        fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                                                FBE_TRACE_LEVEL_WARNING,
                                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                "%s, Invalid transfer count 0x%llx \n", 
                                                 __FUNCTION__, (unsigned long long)(block_size *block_count));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_fis_set_transfer_count(fis_operation, (fbe_u32_t)(block_size * block_count));

    fbe_payload_fis_build_write_fpdma_queued(fis_operation, lba, block_count, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    status = fbe_transport_get_tag(packet, &tag);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_sata_physical_drive_allocate_tag failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_tag(fis_operation, tag);

    fbe_transport_set_completion_function(packet, sata_physical_drive_write_verify_write_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 
    return status;    
}

static fbe_status_t 
sata_physical_drive_write_verify_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_memory_request_t *    memory_request = NULL;

    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    //block_operation = fbe_payload_ex_get_prev_block_operation(payload);

    /* Check error codes and fis status */
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    memory_request = fbe_transport_get_memory_request(packet);
    memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_PACKET;
    memory_request->number_of_objects = 1;
    memory_request->ptr = NULL;
    memory_request->completion_function = (fbe_memory_completion_function_t) sata_physical_drive_write_verify_memory_alloc_completion;
    memory_request->completion_context = context;
    fbe_memory_allocate(memory_request);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t 
sata_physical_drive_write_verify_memory_alloc_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_packet_t * new_packet = NULL;
    fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * master_payload = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;    
    fbe_payload_block_operation_t * block_operation = NULL;  
    fbe_payload_sg_descriptor_t * fis_sg_descriptor = NULL;
    fbe_lba_t lba;
    fbe_block_count_t block_count;
    fbe_block_size_t block_size;
    fbe_lba_t drive_block_count;
    fbe_block_size_t drive_block_size;
    fbe_u32_t repeat_count; 
    fbe_u8_t tag;
    fbe_status_t status = FBE_STATUS_OK;

    void ** chunk_array = memory_request->ptr;

    sata_physical_drive = (fbe_sata_physical_drive_t *)context;

    master_packet = fbe_transport_memory_request_to_packet(memory_request);
    master_payload = fbe_transport_get_payload_ex(master_packet);

    new_packet = chunk_array[0];

    //check base_physical_drive block_size and switch to get appropriate bit bucket.
    fbe_base_physical_drive_get_capacity((fbe_base_physical_drive_t *) sata_physical_drive, &drive_block_count, &drive_block_size);
    
    switch(drive_block_size)
    {
        case 512:
            sg_list = fbe_memory_get_bit_bucket_sgl(FBE_MEMORY_BIT_BUCKET_SGL_TYPE_512);
            break;
        case 520:
            sg_list = fbe_memory_get_bit_bucket_sgl(FBE_MEMORY_BIT_BUCKET_SGL_TYPE_520);
            break;
        default:
            fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s, Unsupported block count %X\n",
                                    __FUNCTION__, drive_block_size);

            fbe_transport_release_packet(new_packet);
            fbe_transport_set_status(master_packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(master_packet);
            status = FBE_STATUS_GENERIC_FAILURE;
            break; 
    }

    /* Initialize sub packet. */
    fbe_transport_initialize_packet(new_packet);
    payload = fbe_transport_get_payload_ex(new_packet);

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    
    block_operation = fbe_payload_ex_get_block_operation(master_payload); 
    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);

    if(fis_operation == NULL){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);
        
        fbe_transport_release_packet(new_packet);
        fbe_transport_set_status(master_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(master_packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_block_get_lba(block_operation, &lba);
    fbe_payload_block_get_block_count(block_operation, &block_count);
    fbe_payload_block_get_block_size(block_operation, &block_size);

    if ((block_size *block_count) >FBE_U32_MAX)
    {
       /* we do not expect the transfer count to go beyond 32bit
         */
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, Invalid transfer count 0x%llx \n", 
                                 __FUNCTION__,
                (unsigned long long)(block_size *block_count));
        fbe_transport_release_packet(new_packet);
        fbe_transport_set_status(master_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(master_packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_fis_set_transfer_count(fis_operation, (fbe_u32_t)(block_size * block_count));
    fbe_payload_fis_get_sg_descriptor(fis_operation, &fis_sg_descriptor);

    if (block_count >FBE_U32_MAX)
    {
       /* we do not expect repeat count to go beyond 32bit
         */
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, Invalid repeat count 0x%llx \n", 
                                 __FUNCTION__,
                (unsigned long long)block_count);
        fbe_transport_release_packet(new_packet);
        fbe_transport_set_status(master_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(master_packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    repeat_count = (fbe_u32_t)block_count;

    fbe_payload_sg_descriptor_set_repeat_count(fis_sg_descriptor, repeat_count);

    fbe_payload_fis_build_read_fpdma_queued(fis_operation, lba, block_count, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    status = fbe_transport_get_tag(master_packet, &tag);

    if(status != FBE_STATUS_OK){
        fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_sata_physical_drive_allocate_tag failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        fbe_transport_release_packet(new_packet);
        fbe_transport_set_status(master_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(master_packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_tag(fis_operation, tag);

    /* We need to assosiate newly allocated packet with original one */
    fbe_transport_add_subpacket(master_packet, new_packet);

     /* Put packet on the termination queue while we wait for the subpacket to complete. */
    fbe_transport_set_cancel_function(master_packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)sata_physical_drive);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)sata_physical_drive, master_packet);

    fbe_transport_set_completion_function(new_packet, sata_physical_drive_write_verify_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, new_packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_write_verify_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t * master_packet;
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)context;
    fbe_drive_threshold_and_exceptions_t threshold_rec = {0};
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_ex_t * master_payload = NULL;
    fbe_port_request_status_t request_status;

    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
        
    fbe_drive_configuration_handle_t drive_configuration_handle;
    fbe_payload_cdb_fis_io_status_t io_status;
    fbe_payload_cdb_fis_error_t payload_cdb_fis_error;
    fbe_lba_t bad_lba = 0;
        
    master_packet = fbe_transport_get_master_packet(packet);
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)sata_physical_drive, master_packet);

    master_payload = fbe_transport_get_payload_ex(master_packet);
        
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    
    /* Check payload fis operation. */
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_base_physical_drive_get_configuration_handle(base_physical_drive, &drive_configuration_handle);

    status = fbe_payload_fis_completion(fis_operation, /* IN */
                                        drive_configuration_handle, /* IN */
                                        &io_status, /* OUT */
                                        &payload_cdb_fis_error, /* OUT */
                                        &bad_lba); /* OUT */

    if(payload_cdb_fis_error != FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR)
    {
        status = fbe_drive_configuration_get_threshold_info(drive_configuration_handle, &threshold_rec);

        if (status != FBE_STATUS_OK)
        {
            /* if we can't get threshold info with a valid handle then something seriously went wrong */
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                           "%s failed to get threshold rec. Invalidating handle.\n", __FUNCTION__);
        }

#ifdef BLOCKSIZE_512_SUPPORTED 
        /* When/If 512 is supported it will need to fix the following DIEH code to handle FIS commands and errors.  Currently DIEH expects SCSI */
        status = fbe_base_physical_drive_io_fail((fbe_base_physical_drive_t *) sata_physical_drive, drive_configuration_handle, &threshold_rec, payload_cdb_fis_error, payload_cdb_operation->cdb[0], 
                                                 FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_SCSI_SENSE_KEY_NO_SENSE, FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO, FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO); /* dieh weight change disabled for sata*/
#endif
    } 
    else 
    {
        fbe_base_physical_drive_io_success((fbe_base_physical_drive_t *) sata_physical_drive);
    }

    block_operation = fbe_payload_ex_get_block_operation(master_payload);


    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)
    {
        if (request_status == FBE_PORT_REQUEST_STATUS_SATA_NCQ_ERROR)
        {
            fbe_sata_physical_drive_parse_read_log_extended(sata_physical_drive, fis_operation, (fbe_payload_ex_t *)payload);
        }
        else
        {
            fbe_sata_physical_drive_handle_block_operation_error((fbe_payload_ex_t *)payload, request_status);
        }

        //Update error counts
        ((fbe_base_physical_drive_t *)sata_physical_drive)->physical_drive_error_counts.error_count++;
    }
    else
    {
        fbe_base_physical_drive_set_retry_msecs ((fbe_payload_ex_t *)payload, FBE_SCSI_CC_NOERR);
        fbe_payload_block_set_status(block_operation,   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    }
    
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    fbe_transport_remove_subpacket(packet);
    fbe_transport_release_packet(packet);
    fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet);
    return FBE_STATUS_OK;    
}

fbe_status_t
fbe_sata_physical_drive_reset_device(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;

    fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if(payload_control_operation == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_ex_allocate_control_operation failed\n",__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_build_operation(payload_control_operation, 
                                        FBE_STP_TRANSPORT_CONTROL_CODE_RESET_DEVICE, 
                                        NULL, 0);

    status = fbe_transport_set_completion_function(packet, sata_physical_drive_reset_device_completion, sata_physical_drive);

    status = fbe_stp_transport_send_control_packet(&sata_physical_drive->stp_edge, packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
sata_physical_drive_reset_device_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(payload_control_operation, &control_status);

    fbe_payload_ex_release_control_operation(payload, payload_control_operation);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_sata_physical_drive_firmware_download(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * payload_fis_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_physical_drive_mgmt_fw_download_t * download_info = NULL;

    fbe_base_object_trace(  (fbe_base_object_t*)sata_physical_drive,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(payload_control_operation, &download_info);
    /* status = fbe_payload_ex_increment_control_operation_level(payload); */

    payload_fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(payload_fis_operation == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_ex_allocate_fis_operation failed\n",__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (download_info->transfer_count % 0x200 != 0)
    {
        //Allow dummy simulation value
        if (download_info->transfer_count != 0xFBE0)
        {
            //Error
            fbe_base_object_trace(  (fbe_base_object_t *)sata_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s fw image size invalid \n",__FUNCTION__);

            fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_payload_ex_release_fis_operation(payload, payload_fis_operation);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_payload_fis_set_transfer_count(payload_fis_operation, download_info->transfer_count);

    fbe_payload_fis_build_firmware_download(payload_fis_operation, (fbe_block_count_t)(download_info->transfer_count / 0x200), FBE_SATA_PHYSICAL_DRIVE_FW_DOWNLOAD_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_firmware_download_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet);

    return status;
}

static fbe_status_t 
sata_physical_drive_firmware_download_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * payload_fis_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_physical_drive_mgmt_fw_download_t * download_info = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_port_request_status_t request_status;
    
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    payload = fbe_transport_get_payload_ex(packet);
    payload_fis_operation = fbe_payload_ex_get_fis_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_control_get_buffer(payload_control_operation, &download_info);
 
    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);

    if((status != FBE_STATUS_OK) || (download_info == NULL)){
        fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_payload_ex_release_fis_operation(payload, payload_fis_operation);
        
        return FBE_STATUS_OK;
    }
    
    fbe_payload_fis_get_request_status(payload_fis_operation, &request_status);
    
    if ((payload_fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS))
    {
        /* Failed. Retry in SHIM. */
        fbe_sata_physical_drive_map_port_status_to_scsi_error_code(request_status, &error);

        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        download_info->download_error_code = error;
        download_info->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
        download_info->qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE;

        ((fbe_base_physical_drive_t *)sata_physical_drive)->physical_drive_error_counts.error_count++;
        fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Download Failed, port status: 0x%X, error: 0x%x\n",
                                __FUNCTION__, request_status, payload_fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS]);
    }
    else
    {
        /* Success */
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        download_info->download_error_code = 0;
        download_info->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
        download_info->qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
    }

    fbe_payload_ex_release_fis_operation(payload, payload_fis_operation);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_set_power_save(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet, fbe_bool_t on)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_status_t status;
    fbe_object_id_t my_object_id;

    payload = fbe_transport_get_payload_ex(packet);

    fbe_base_object_get_object_id((fbe_base_object_t *)sata_physical_drive, &my_object_id);
    
    payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);

    if (on) {
        fbe_payload_discovery_build_common_command(payload_discovery_operation, 
                                                   FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_SAVE_ON, my_object_id);
    } else {
        fbe_payload_discovery_build_common_command(payload_discovery_operation, 
                                                   FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_SAVE_OFF, my_object_id);
    }

    fbe_transport_set_completion_function(packet, sata_physical_drive_set_power_save_completion, sata_physical_drive);
  
    /* We are sending discovery packet via discovery edge */
    status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *) sata_physical_drive, packet);

    return status;
}

/*
 * This is the completion function for the start_stop_unit packet sent to sata_physical_drive.
 */
static fbe_status_t 
sata_physical_drive_set_power_save_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_status_t discovery_status;
    fbe_status_t status;
    fbe_payload_control_operation_t * control_operation = NULL;

    sata_physical_drive = (fbe_sata_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_discovery_get_status(payload_discovery_operation, &discovery_status);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
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
fbe_sata_physical_drive_spin_down(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * payload_fis_operation = NULL;
    fbe_status_t status;
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_fis_operation = fbe_payload_ex_allocate_fis_operation(payload);

    /* Coverity Fix. */
    if(payload_fis_operation == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                                __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_payload_fis_build_standby(payload_fis_operation, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);
    
    fbe_transport_set_completion_function(packet, sata_physical_drive_spin_down_completion, sata_physical_drive);
 
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet);

    return status;
}

/*
 * This is the completion function for spindown packet sent to sata_physical_drive.
 */
static fbe_status_t 
sata_physical_drive_spin_down_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * payload_fis_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;
    fbe_port_request_status_t request_status;
    
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_fis_operation = fbe_payload_ex_get_fis_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);

        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_payload_ex_release_fis_operation(payload, payload_fis_operation);
        return FBE_STATUS_OK;
    }

    /* Check port status & FIS operation status */
    fbe_payload_fis_get_request_status(payload_fis_operation, &request_status);
    if((payload_fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Port Status: 0x%X \n", __FUNCTION__,
                                request_status);

        fbe_payload_fis_trace_fis_buff(payload_fis_operation);
        fbe_payload_fis_trace_response_buff(payload_fis_operation);
        
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_payload_ex_release_fis_operation(payload, payload_fis_operation);
        return FBE_STATUS_OK;
    } 

    fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_ex_release_fis_operation(payload, payload_fis_operation);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_write_uncorrectable(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet, fbe_lba_t lba)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t *   fis_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);

    fis_operation = fbe_payload_ex_allocate_fis_operation(payload);
    if(fis_operation == NULL){
        fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, fbe_payload_ex_allocate_fis_operation failed\n",
                               __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_fis_set_transfer_count(fis_operation, FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    fbe_payload_fis_build_write_uncorrectable(fis_operation, lba, FBE_SATA_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    fbe_transport_set_completion_function(packet, sata_physical_drive_write_uncorrectable_completion, sata_physical_drive);
    status = fbe_stp_transport_send_functional_packet(&sata_physical_drive->stp_edge, packet); 

    return status;    
}

static fbe_status_t 
sata_physical_drive_write_uncorrectable_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;   
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_status_t status;
    fbe_port_request_status_t request_status;
        
    sata_physical_drive = (fbe_sata_physical_drive_t *)context;
    payload = fbe_transport_get_payload_ex(packet);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    
    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);

    if(status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    /* Check port status & FIS operation status */ 
    fbe_payload_fis_get_request_status(fis_operation, &request_status);

    if((fis_operation->response_buffer[FBE_SATA_RESPONSE_STATUS] & FBE_SATA_RESPONSE_STATUS_ERR_BIT) ||
        (request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Port Status: 0x%X \n", __FUNCTION__,
                               request_status);

        fbe_base_object_trace((fbe_base_object_t*)sata_physical_drive,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s ERROR %02X %02X %02X %02X\n", __FUNCTION__,
                               fis_operation->response_buffer[0],
                               fis_operation->response_buffer[1],
                               fis_operation->response_buffer[2],
                               fis_operation->response_buffer[3]);

        fbe_payload_ex_release_fis_operation(payload, fis_operation);
        return FBE_STATUS_OK;
    } 

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_ex_release_fis_operation(payload, fis_operation);

    return FBE_STATUS_OK;
}
