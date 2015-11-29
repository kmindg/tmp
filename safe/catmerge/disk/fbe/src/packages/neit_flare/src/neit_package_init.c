#include "csx_ext.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_queue.h"
#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_neit_package.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_object_map_interface.h"
#include "fbe_neit_private.h"
#include "fbe/fbe_time.h"

static fbe_queue_head_t neit_error_queue;
static fbe_spinlock_t   neit_error_queue_lock;
static fbe_bool_t       neit_started = FBE_FALSE;
static fbe_trace_level_t neit_trace_level = FBE_TRACE_LEVEL_INFO; /* Set INFO by default for debugging purposes */

//static fbe_service_control_entry_t neit_physical_package_control_entry = NULL;
static fbe_status_t neit_package_edge_hook_function(fbe_packet_t * packet);
static void neit_package_destroy_notification_callback(void * context);
static fbe_status_t neit_package_handle_fis_error_type (fbe_payload_fis_operation_t * fis_operation, fbe_neit_error_record_t * neit_error_record);
static fbe_status_t neit_package_handle_port_error(fbe_packet_t * packet, fbe_neit_error_record_t * error_record);
static fbe_status_t neit_package_destroy_services(void);



typedef fbe_u32_t fbe_neit_magic_number_t;
enum {
    FBE_NEIT_MAGIC_NUMBER = 0x4E454954,  /* ASCII for NEIT */
};

typedef struct fbe_neit_error_element_s {
    fbe_queue_element_t queue_element;
    fbe_neit_magic_number_t magic_number;
    fbe_neit_error_record_t neit_error_record;
}fbe_neit_error_element_t;

static fbe_neit_error_element_t * neit_package_find_error_element_by_object_id(fbe_object_id_t object_id);
static fbe_status_t neit_package_insert_scsi_error(fbe_packet_t * packet, fbe_payload_cdb_operation_t * cdb_operation, 
                                                   fbe_neit_error_element_t * neit_error_element);
static fbe_status_t neit_package_check_error_record(fbe_payload_cdb_operation_t * cdb_operation, fbe_neit_error_element_t * neit_error_element);

static void
neit_package_log_error(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));

static void
neit_package_log_error(const char * fmt, ...)
{
    va_list args;
    if(FBE_TRACE_LEVEL_ERROR <= neit_trace_level) {
        va_start(args, fmt);
        fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                        FBE_PACKAGE_ID_NEIT,
                        FBE_TRACE_LEVEL_ERROR,
                        0, /* message_id */
                        fmt, 
                        args);
        va_end(args);
    }
}

static void
neit_package_log_info(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));

static void
neit_package_log_info(const char * fmt, ...)
{
    va_list args;

    if(FBE_TRACE_LEVEL_INFO <= neit_trace_level)  {
        va_start(args, fmt);
        fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                        FBE_PACKAGE_ID_NEIT,
                        FBE_TRACE_LEVEL_INFO,
                        0, /* message_id */
                        fmt, 
                        args);
        va_end(args);
    }
}

fbe_status_t 
neit_flare_package_init(void)
{
    fbe_notification_type_t state_mask;
 
    neit_package_log_info("%s entry\n", __FUNCTION__);
    fbe_queue_init(&neit_error_queue);
    fbe_spinlock_init(&neit_error_queue_lock);

    //fbe_neit_initialize_physical_package_control_entry(&neit_physical_package_control_entry);
    //fbe_api_common_init();
    
    state_mask = FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_DESTROY | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY;

    fbe_api_object_map_interface_register_notification(state_mask, neit_package_destroy_notification_callback);
    return FBE_STATUS_OK;
}

fbe_status_t 
neit_flare_package_destroy(void)
{   
    fbe_neit_error_element_t * neit_error_element = NULL;
    fbe_status_t status;

    neit_package_log_info("%s entry\n", __FUNCTION__);

    neit_started = FBE_FALSE;

    neit_error_element = (fbe_neit_error_element_t *)fbe_queue_front(&neit_error_queue);
    while(neit_error_element != NULL){
        neit_package_remove_record(neit_error_element);
        neit_error_element = (fbe_neit_error_element_t *)fbe_queue_front(&neit_error_queue);
    }

    status = neit_package_destroy_services();
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: neit_package_destroy_services failed, status: %X\n",
                __FUNCTION__, status);
    }    

    fbe_api_object_map_interface_unregister_notification(neit_package_destroy_notification_callback);
            
    fbe_queue_destroy(&neit_error_queue);
    fbe_spinlock_destroy(&neit_error_queue_lock);
    
    return FBE_STATUS_OK;
}

fbe_status_t 
neit_package_add_record(fbe_neit_error_record_t * neit_error_record, fbe_neit_record_handle_t * neit_record_handle)
{
    fbe_status_t status;
    fbe_neit_error_element_t * neit_error_element = NULL;
    fbe_api_set_edge_hook_t hook_info;
    fbe_class_id_t class_id;

    * neit_record_handle = NULL;

    neit_error_element = fbe_allocate_contiguous_memory(sizeof(fbe_neit_error_element_t));
    if(neit_error_element == NULL){
        neit_package_log_error("%s Can't allocate memory\n", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    neit_error_element->magic_number = FBE_NEIT_MAGIC_NUMBER;
    fbe_copy_memory(&neit_error_element->neit_error_record, neit_error_record, sizeof(fbe_neit_error_record_t));

    hook_info.hook = neit_package_edge_hook_function;

    /* Let's figure out if it is SAS or SATA drive or SAS enclosure */
    fbe_api_get_object_class_id (neit_error_element->neit_error_record.object_id, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    if((class_id > FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST)/* SAS drive */
       ||(class_id > FBE_CLASS_ID_SAS_ENCLOSURE_FIRST) && (class_id < FBE_CLASS_ID_SAS_ENCLOSURE_LAST)) /*SAS Enclosures */
    { 
        status = fbe_api_set_ssp_edge_hook ( neit_error_element->neit_error_record.object_id, &hook_info);
    }else if((class_id > FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_LAST)){ /* SATA drive */
        status = fbe_api_set_stp_edge_hook ( neit_error_element->neit_error_record.object_id, &hook_info);
    } else {
        neit_package_log_error("%s Invalid class_id %X\n", __FUNCTION__, class_id);
        fbe_release_contiguous_memory(neit_error_element);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (status != FBE_STATUS_OK) {
        neit_package_log_error("%s Set Edge Hook Error %d\n", __FUNCTION__, status);
        fbe_release_contiguous_memory(neit_error_element);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_spinlock_lock(&neit_error_queue_lock);
    fbe_queue_push(&neit_error_queue, &neit_error_element->queue_element);
    fbe_spinlock_unlock(&neit_error_queue_lock);
    * neit_record_handle = neit_error_element;
    return FBE_STATUS_OK;
}

/* Removes all records for particular object */
fbe_status_t 
neit_package_remove_object(fbe_object_id_t object_id)
{
    fbe_neit_error_element_t * neit_error_element = NULL;
    fbe_api_set_edge_hook_t hook_info;
    fbe_status_t status;
    fbe_class_id_t class_id;

    fbe_spinlock_lock(&neit_error_queue_lock);
    neit_error_element = (fbe_neit_error_element_t *)fbe_queue_front(&neit_error_queue);
    while(neit_error_element != NULL){
        if(neit_error_element->neit_error_record.object_id == object_id){ /* We found our record */
            fbe_queue_remove((fbe_queue_element_t *)neit_error_element);
            neit_error_element = (fbe_neit_error_element_t *)fbe_queue_front(&neit_error_queue); /* Start from the beginning */
        } else {
            neit_error_element = (fbe_neit_error_element_t *)fbe_queue_next(&neit_error_queue, (fbe_queue_element_t *)neit_error_element);
        }
    }/* while(neit_error_element != NULL) */
    fbe_spinlock_unlock(&neit_error_queue_lock);

    hook_info.hook = NULL;
    /* Let's figure out if it is SAS or SATA drive */
    fbe_api_get_object_class_id (object_id, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    if((class_id > FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST)/* SAS drive */
       ||(class_id > FBE_CLASS_ID_SAS_ENCLOSURE_FIRST) && (class_id < FBE_CLASS_ID_SAS_ENCLOSURE_LAST)) /*SAS Enclosures */
    { 
        status = fbe_api_set_ssp_edge_hook ( object_id, &hook_info);
    }else if((class_id > FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_LAST)){ /* SATA drive */
        status = fbe_api_set_stp_edge_hook ( object_id, &hook_info);
    }

    return FBE_STATUS_OK;
}

fbe_status_t 
neit_package_remove_record(fbe_neit_record_handle_t neit_record_handle)
{
    fbe_status_t status;
    fbe_neit_error_element_t * neit_error_element = NULL;
    fbe_api_set_edge_hook_t hook_info;

    neit_error_element = (fbe_neit_error_element_t *)neit_record_handle;

    if(neit_error_element->magic_number != FBE_NEIT_MAGIC_NUMBER) {
        neit_package_log_error("%s Invalid handle\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&neit_error_queue_lock);
    fbe_queue_remove(&neit_error_element->queue_element);
    fbe_spinlock_unlock(&neit_error_queue_lock);

    /* If we have not records for this object id we should disable the hook */
    if(NULL == neit_package_find_error_element_by_object_id(neit_error_element->neit_error_record.object_id)){
        hook_info.hook = NULL;
        status = fbe_api_set_ssp_edge_hook ( neit_error_element->neit_error_record.object_id, &hook_info);
    }

    fbe_release_contiguous_memory(neit_error_element);
    return FBE_STATUS_OK;
}

fbe_status_t 
neit_package_get_record(fbe_neit_record_handle_t neit_record_handle, fbe_neit_error_record_t * neit_error_record)
{
    fbe_neit_error_element_t * neit_error_element = NULL;
   
    neit_error_element = (fbe_neit_error_element_t *)neit_record_handle;

    if((neit_error_element == NULL) || (neit_error_element->magic_number != FBE_NEIT_MAGIC_NUMBER)) {
        neit_package_log_error("%s Invalid handle\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(neit_error_record, &neit_error_element->neit_error_record, sizeof(fbe_neit_error_record_t));

    return FBE_STATUS_OK;
}

fbe_status_t 
neit_package_start(void)
{
    neit_started = FBE_TRUE;
    return FBE_STATUS_OK;
}

fbe_status_t 
neit_package_stop(void)
{
    neit_started = FBE_FALSE;
    return FBE_STATUS_OK;
}

static fbe_status_t 
neit_package_edge_hook_function(fbe_packet_t * packet)
{
    /* Find record by object_id */
    fbe_neit_error_element_t * neit_error_element = NULL;
    fbe_base_edge_t * edge;
    fbe_object_id_t client_id;
    fbe_transport_id_t transport_id;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * cdb_operation = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_status_t status;

    if(!neit_started){
        return FBE_STATUS_CONTINUE;
    }

    edge = fbe_transport_get_edge(packet);
    payload = fbe_transport_get_payload_ex(packet);
    fbe_base_transport_get_client_id(edge, &client_id);
    fbe_base_transport_get_transport_id(edge, &transport_id);

    if(transport_id == FBE_TRANSPORT_ID_SSP){
        cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
        /* We don't send REASSIGN request to drive for inserted errors */
        if ((cdb_operation->cdb[0] == FBE_SCSI_REASSIGN_BLOCKS) && 
            (payload->physical_drive_transaction.last_error_code == 0xFFFFFFFF)) {
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);                        
            return FBE_STATUS_OK;
        }
    }else if(transport_id == FBE_TRANSPORT_ID_STP){
        fis_operation = fbe_payload_ex_get_fis_operation(payload);
    } else {
        neit_package_log_error("%s Invalid transport_id %X\n", __FUNCTION__, transport_id);
        return FBE_STATUS_CONTINUE; /* We do not want to be too disruptive */
    }

    fbe_spinlock_lock(&neit_error_queue_lock);
    neit_error_element = (fbe_neit_error_element_t *)fbe_queue_front(&neit_error_queue);
    while(neit_error_element != NULL){
        if(neit_error_element->neit_error_record.object_id == client_id){ /* We found our record */
                       
            switch(neit_error_element->neit_error_record.neit_error_type)
            {
                case FBE_NEIT_ERROR_TYPE_PORT:
                    status = neit_package_handle_port_error(packet, &neit_error_element->neit_error_record);
                    fbe_spinlock_unlock(&neit_error_queue_lock);
                    if(status != FBE_STATUS_CONTINUE){
                        fbe_transport_set_status(packet, status, 0);
                        fbe_transport_complete_packet(packet);                        
                    }
                    return status; 
                    
                    break;
                case FBE_NEIT_ERROR_TYPE_FIS:
                    if (neit_error_element->neit_error_record.num_of_times_to_insert > neit_error_element->neit_error_record.times_inserted)
                    {
                        /* Coverity fix to prevent the fis_operation is NULL.
                         * The NEIT doesn't support SATA drives yet. This will be removed when it is supported later. */
                        if (fis_operation == NULL) {
                            fbe_spinlock_unlock(&neit_error_queue_lock);
                            neit_package_log_error("%s Invalid Fis_operation %p\n", __FUNCTION__, fis_operation);
                            return FBE_STATUS_CONTINUE; 
                        }
                        
                        status = neit_package_handle_fis_error_type(fis_operation, &neit_error_element->neit_error_record);
                        if (status == FBE_STATUS_OK)
                        {
                            fbe_spinlock_unlock(&neit_error_queue_lock);
                            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                            fbe_transport_complete_packet(packet);
                            return status;
                        }
                    }
                    break;

                case FBE_NEIT_ERROR_TYPE_SCSI:
                    status = neit_package_insert_scsi_error(packet, cdb_operation, neit_error_element);
                    if (status == FBE_STATUS_OK) {                                
                        fbe_spinlock_unlock(&neit_error_queue_lock);                        
                        fbe_transport_set_status(packet, status, 0);
                        fbe_transport_complete_packet(packet);
                        return status;
                    }
                    break;

                default: 
                    fbe_spinlock_unlock(&neit_error_queue_lock);
                    neit_package_log_error("%s Invalid neit_error_type %X\n", __FUNCTION__, neit_error_element->neit_error_record.neit_error_type);
                    return FBE_STATUS_CONTINUE; /* We do not want to be too disruptive */
            }
        } /* It is not our record */

        neit_error_element = (fbe_neit_error_element_t *)fbe_queue_next(&neit_error_queue, (fbe_queue_element_t *)neit_error_element);
    }/* while(neit_error_element != NULL) */

    fbe_spinlock_unlock(&neit_error_queue_lock);

    return FBE_STATUS_CONTINUE;
}

static fbe_neit_error_element_t * 
neit_package_find_error_element_by_object_id(fbe_object_id_t object_id)
{
    fbe_neit_error_element_t * neit_error_element = NULL;

    fbe_spinlock_lock(&neit_error_queue_lock);
    neit_error_element = (fbe_neit_error_element_t *)fbe_queue_front(&neit_error_queue);
    while(neit_error_element != NULL){
        if(neit_error_element->neit_error_record.object_id == object_id){ /* We found our record */
            fbe_spinlock_unlock(&neit_error_queue_lock);
            return neit_error_element;
        } else {
            neit_error_element = (fbe_neit_error_element_t *)fbe_queue_next(&neit_error_queue, (fbe_queue_element_t *)neit_error_element);
        }
    }/* while(neit_error_element != NULL) */

    fbe_spinlock_unlock(&neit_error_queue_lock);
    return NULL;
}

static void 
neit_package_destroy_notification_callback(void * context)
{
    update_object_msg_t * update_object_msg = NULL;
    
    update_object_msg = (update_object_msg_t *)context;

    neit_package_remove_object(update_object_msg->object_id);
}

static fbe_status_t 
neit_package_handle_fis_error_type (fbe_payload_fis_operation_t * fis_operation, fbe_neit_error_record_t * neit_error_record)
{
    if ((neit_error_record->neit_error.neit_sata_error.sata_command == 0xFF) || 
        (neit_error_record->neit_error.neit_sata_error.sata_command == fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET]))
    {
        if(neit_error_record->neit_error.neit_sata_error.port_status == FBE_PORT_REQUEST_STATUS_SATA_NCQ_ERROR)
        {
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_LOW_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_LOW_OFFSET];
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_MID_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_MID_OFFSET];
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_HIGH_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_HIGH_OFFSET];
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_LOW_EXT_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_LOW_EXT_OFFSET];
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_MID_EXT_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_MID_EXT_OFFSET];
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_HIGH_EXT_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_HIGH_EXT_OFFSET];
        }
        else
        {
            fbe_payload_fis_set_request_status(fis_operation, neit_error_record->neit_error.neit_sata_error.port_status);
            fbe_copy_memory(fis_operation->response_buffer, neit_error_record->neit_error.neit_sata_error.response_buffer, FBE_PAYLOAD_FIS_RESPONSE_BUFFER_SIZE);                       
        }
        
        neit_package_log_info("NEIT: %s,Object ID:0x%x,sata_command:0x%x,port_status:0x%x\n",
                      __FUNCTION__, neit_error_record->object_id, 
                      neit_error_record->neit_error.neit_sata_error.sata_command,
                      neit_error_record->neit_error.neit_sata_error.port_status);

        neit_error_record->times_inserted++;
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_CONTINUE;
}

static fbe_status_t 
neit_package_insert_scsi_error(fbe_packet_t * packet,
                               fbe_payload_cdb_operation_t * cdb_operation,
                               fbe_neit_error_element_t * neit_error_element)
{
    fbe_u8_t * sense_buffer = NULL;
    fbe_lba_t lba = 0, bad_lba;
    fbe_neit_error_record_t *error_record = &(neit_error_element->neit_error_record);
    fbe_neit_scsi_error_t *scsi_error_record = &(neit_error_element->neit_error_record.neit_error.neit_scsi_error);
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;

    /* Check error rules */
    status = neit_package_check_error_record(cdb_operation, neit_error_element);
    if(status != FBE_STATUS_OK){
        return FBE_STATUS_CONTINUE;
    }


    if(error_record->frequency != 0){
        if((error_record->times_inserted % error_record->frequency) != 0){
            error_record->times_inserted++;
            return FBE_STATUS_CONTINUE;
        }
    }
    
    error_record->times_inserted++;

    if ( error_record->times_inserted >= error_record->num_of_times_to_insert )
    {
        /* Set the timestamp so that we know the exact time
         * when we went over the threshold.
         */
        error_record->max_times_hit_timestamp = fbe_get_time();
    }

    /* Mark the payload to be inserted error */
    payload = fbe_transport_get_payload_ex(packet);
    if (payload) {
        payload->physical_drive_transaction.last_error_code = 0xFFFFFFFF;
    }
    
    /* If port error is inserted, then handle the port error first. */
    if(scsi_error_record->port_status != FBE_PORT_REQUEST_STATUS_SUCCESS) {
        fbe_payload_cdb_set_scsi_status(cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);
        fbe_payload_cdb_set_request_status(cdb_operation, scsi_error_record->port_status);
        neit_package_log_info("NEIT: %s,Object ID:0x%x, port status: %d,Lba:0x%llX\n",
                      __FUNCTION__, error_record->object_id, scsi_error_record->port_status,
                      (unsigned long long)error_record->lba_start);
        return FBE_STATUS_OK;
    }
    
    /* Insert scsi status and sense data. */
    fbe_payload_cdb_set_scsi_status(cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
    fbe_payload_cdb_operation_get_sense_buffer(cdb_operation, &sense_buffer);
    sense_buffer[FBE_SCSI_SENSE_DATA_RESPONSE_CODE_OFFSET] = 0xf0;        
    sense_buffer[FBE_SCSI_SENSE_DATA_SENSE_KEY_OFFSET] = scsi_error_record->scsi_sense_key;    
    sense_buffer[FBE_SCSI_SENSE_DATA_ASC_OFFSET] = scsi_error_record->scsi_additional_sense_code;    
    sense_buffer[FBE_SCSI_SENSE_DATA_ASCQ_OFFSET] = scsi_error_record->scsi_additional_sense_code_qualifier;

    /* Get start lba in cdb payload. */
    if (cdb_operation->cdb_length == 6)
    {
          lba = fbe_get_six_byte_cdb_lba(&cdb_operation->cdb[0]);
    }
    else if (cdb_operation->cdb_length == 10)
    {
          lba = fbe_get_ten_byte_cdb_lba(&cdb_operation->cdb[0]);
    }
    else if (cdb_operation->cdb_length == 16)
    {
          lba = fbe_get_sixteen_byte_cdb_lba(&cdb_operation->cdb[0]);
    }
  
    /* Get bad lba. */
    if ( lba <= error_record->lba_start )
    {
          bad_lba = error_record->lba_start;
    }
    else 
    {
          bad_lba = lba;
    }
 
    /* Fill up the LBA value. This will help to be the bad LBA for media errors. */
    sense_buffer[6] = (fbe_u8_t)(0xFF & bad_lba);    
    sense_buffer[5] = (fbe_u8_t)(0xFF & (bad_lba >> 8));    
    sense_buffer[4] = (fbe_u8_t)(0xFF & (bad_lba >> 16));    
    sense_buffer[3] = (fbe_u8_t)(0xFF & (bad_lba >> 24));

    neit_package_log_info("NEIT: %s,Object ID:0x%x,SK:0x%x,ASC/ASCQ:0x%x/0x%x,cdb:0x%x,Lba:0x%llX\n",
                      __FUNCTION__, error_record->object_id, scsi_error_record->scsi_sense_key,
                      scsi_error_record->scsi_additional_sense_code,
                      scsi_error_record->scsi_additional_sense_code_qualifier,
                      (cdb_operation->cdb)[0], (unsigned long long)bad_lba);
    return FBE_STATUS_OK;
}

static fbe_status_t 
neit_package_check_error_record(fbe_payload_cdb_operation_t * cdb_operation,
                               fbe_neit_error_element_t * neit_error_element)
{
    fbe_lba_t lba_start = 0;
    fbe_block_count_t blocks = 0;
    fbe_neit_error_record_t *error_record = &(neit_error_element->neit_error_record);
    fbe_u32_t elapsed_seconds = 0;
    fbe_u8_t *cmd, i = 0;

    /* Get start lba_start and blocks in cdb payload. */
    if (cdb_operation->cdb_length == 6)
    {
        lba_start = fbe_get_six_byte_cdb_lba(&cdb_operation->cdb[0]);
        blocks = fbe_get_six_byte_cdb_blocks(&cdb_operation->cdb[0]);
    }
    else if (cdb_operation->cdb_length == 10)
    {
        lba_start = fbe_get_ten_byte_cdb_lba(&cdb_operation->cdb[0]);
        blocks = fbe_get_ten_byte_cdb_blocks(&cdb_operation->cdb[0]);
    }
    else if (cdb_operation->cdb_length == 16)
    {
        lba_start = fbe_get_sixteen_byte_cdb_lba(&cdb_operation->cdb[0]);
        blocks = fbe_get_sixteen_byte_cdb_blocks(&cdb_operation->cdb[0]);
    }
  
    /* This error record may no longer be active but
     * may be eligible to be reactivated.  
     */
    elapsed_seconds = (fbe_u32_t)(fbe_get_time() - error_record->max_times_hit_timestamp)/1000;

    if ((error_record->times_inserted >= error_record->num_of_times_to_insert) &&
        (error_record->secs_to_reactivate != FBE_NEIT_INVALID) &&
        (elapsed_seconds >= error_record->secs_to_reactivate) &&
        (error_record->times_reset < error_record->num_of_times_to_reactivate)){
            /* This record should be reactivated now by resetting
            * times_inserted.
            */
            error_record->times_inserted = 0;
            /* Increment the number of times we have reset
            * in the times_reset field.
            */
            error_record->times_reset++;
    }
        
    /* There is a match if the records overlap in some way
     * AND the record is active
     */
    if ((lba_start <= error_record->lba_end) &&                        
        (error_record->lba_start < (lba_start + blocks)) &&
        (error_record->times_inserted < error_record->num_of_times_to_insert) )
    {
        /* There is a match of the specific LBA range. Now look for
         * specific opcode. If there is no match, check if there is
         * any thing with any range.  
         */
        cmd = error_record->neit_error.neit_scsi_error.scsi_command;
        if (cmd[0] == 0xFF)
        {
            /* This is the case when any opcode is allowed. */
            return FBE_STATUS_OK;
        }

        while ((i < MAX_CMDS) && (cmd[i]!= 0xFF)) 
        {
            if (cmd[i] == cdb_operation->cdb[0])
            {
                /* We got an exact match here. */
                return FBE_STATUS_OK;
            }
            i++;
        }
    }    

    return FBE_STATUS_CONTINUE;
}


static fbe_status_t 
neit_package_handle_port_error(fbe_packet_t * packet, fbe_neit_error_record_t * error_record)
{
    fbe_base_edge_t * edge = NULL;
    fbe_transport_id_t transport_id;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * cdb_operation = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;

    edge = fbe_transport_get_edge(packet);
    payload = fbe_transport_get_payload_ex(packet);
    fbe_base_transport_get_transport_id(edge, &transport_id);

    if(transport_id == FBE_TRANSPORT_ID_SSP){
        cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    }else if(transport_id == FBE_TRANSPORT_ID_STP){
        fis_operation = fbe_payload_ex_get_fis_operation(payload);
    } else {
        neit_package_log_error("%s Invalid transport_id %X\n", __FUNCTION__, transport_id);
        return FBE_STATUS_CONTINUE; /* We do not want to be too disruptive */
    }

    /* Check num_of_times_to_insert */
    if(error_record->num_of_times_to_insert == 0){
        return FBE_STATUS_CONTINUE;
    }

    error_record->times_inserted++;

    if( error_record->num_of_times_to_insert < error_record->times_inserted){
        return FBE_STATUS_CONTINUE;
    }

    if(error_record->frequency != 0){
        if((error_record->times_inserted % error_record->frequency) != 0){
            return FBE_STATUS_CONTINUE;
        }
    }

    if(transport_id == FBE_TRANSPORT_ID_SSP){                   
        fbe_payload_cdb_set_request_status(cdb_operation, error_record->neit_error.neit_port_error.port_status);
        return FBE_STATUS_OK;
    }else {
        /* transport_id = FBE_TRANSPORT_ID_STP */
        fbe_payload_fis_set_request_status(fis_operation, error_record->neit_error.neit_port_error.port_status);
        return FBE_STATUS_OK;
    } 
}

fbe_status_t 
neit_package_cleanup_queue(void)
{
    fbe_neit_error_element_t * neit_error_element = NULL;

    neit_package_log_info("%s entry\n", __FUNCTION__);
 
    /* If error record is existed, free memory. */
    neit_error_element = (fbe_neit_error_element_t *)fbe_queue_front(&neit_error_queue);
    while(neit_error_element != NULL){
        neit_package_remove_record(neit_error_element);
        neit_error_element = (fbe_neit_error_element_t *)fbe_queue_front(&neit_error_queue);
    }

    /* Init the neit error queue. */
    fbe_spinlock_lock(&neit_error_queue_lock);
    fbe_queue_init(&neit_error_queue);
    fbe_spinlock_unlock(&neit_error_queue_lock);

    return FBE_STATUS_OK;
}

fbe_status_t neit_package_destroy_services(void)
{   
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    neit_package_log_info("%s entry\n", __FUNCTION__);


    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_MEMORY);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: fbe_memory_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }    

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_SERVICE_MANAGER);
    if(status != FBE_STATUS_OK) {
        neit_package_log_error("%s: fbe_service_manager destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    return FBE_STATUS_OK;
}
