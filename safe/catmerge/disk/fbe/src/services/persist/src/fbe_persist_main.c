#include "fbe_persist_private.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe_cmi.h"
#include "fbe_persist_database_layout.h"
#include "fbe_persist_debug.h"
#include "fbe_persist_database.h"
									  
static fbe_status_t persist_control_entry(fbe_packet_t * packet);
fbe_service_methods_t fbe_persist_service_methods = {FBE_SERVICE_ID_PERSIST, persist_control_entry};
fbe_persist_service_t fbe_persist_service;
static fbe_status_t fbe_persist_get_layout_info(fbe_packet_t * packet);
static fbe_status_t fbe_persist_get_required_lun_size(fbe_packet_t * packet);
static fbe_status_t persist_perform_hook(fbe_packet_t * packet);
static fbe_status_t persist_get_entry_info(fbe_packet_t * packet);
static fbe_status_t persist_validate_entry(fbe_packet_t * packet);


static fbe_status_t
persist_init(fbe_packet_t * packet)
{
    fbe_status_t status;

    fbe_base_service_set_service_id((fbe_base_service_t *) &fbe_persist_service, FBE_SERVICE_ID_PERSIST);
    fbe_base_service_set_trace_level((fbe_base_service_t *) &fbe_persist_service, fbe_trace_get_default_trace_level());

    fbe_persist_service.database_lun_object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_persist_database_init();
    if (status != FBE_STATUS_OK) {
		fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = fbe_persist_transaction_init();
    if (status != FBE_STATUS_OK) {
        fbe_persist_database_destroy();
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

	#if 0/*not needed anymore, managed via the DB*/
	status = fbe_persist_cmi_init();
	if (status != FBE_STATUS_OK) {
		fbe_persist_database_destroy();

		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s failed to init CMI logic\n", __FUNCTION__);

		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}
	#endif

    persist_hook_init();

    fbe_base_service_init((fbe_base_service_t *) &fbe_persist_service);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * persist_destroy()
 ****************************************************************
 * @brief
 * Destroy the persist service. Destroy the xaction and database
 * tables before destorying the service itself.
 *
 * @param packet_p - Packet for the destroy operation.          
 *
 * @return FBE_STATUS_OK - Destroy successful.
 *         FBE_STATUS_GENERIC_FAILURE - Destroy Failed.
 *
 * @author
 *
 ****************************************************************/

static fbe_status_t
persist_destroy(fbe_packet_t * packet)
{
    fbe_persist_transaction_destroy();
    fbe_persist_database_destroy();

	#if 0/*not needed anymore, managed via the DB*/
    fbe_persist_cmi_destroy();
	#endif
    
    /* Destroy the service after destroying the xaction and database */    
    fbe_base_service_destroy((fbe_base_service_t *) &fbe_persist_service);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/* This is a local function for handling FBE_PERSIST_CONTROL_CODE_SET_LUN. */
static fbe_status_t
persist_set_database_lun(fbe_packet_t * packet)
{
    fbe_payload_ex_t *payload;
    fbe_payload_control_operation_t *control_operation;
    fbe_persist_control_set_lun_t *set_lun;
    fbe_u32_t buffer_length;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_transport_get_payload_ex() failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_ex_get_control_operation() failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    set_lun = NULL;
    fbe_payload_control_get_buffer(control_operation, &set_lun);
    if (set_lun == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length != sizeof(fbe_persist_control_set_lun_t)){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid control buffer length %d, expected %llu\n",
                          __FUNCTION__, buffer_length, (unsigned long long)sizeof(fbe_persist_control_set_lun_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*any chance it's already set ?*/
	if (fbe_persist_service.database_lun_object_id != FBE_OBJECT_ID_INVALID) {
		fbe_persist_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Lun setting can be done only once, LU is already: %d\n",
                          __FUNCTION__, fbe_persist_service.database_lun_object_id);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_payload_control_set_status_qualifier(control_operation,FBE_PERSIST_STATUS_LUN_ALREADY_SET);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_persist_service.database_lun_object_id =  set_lun->lun_object_id;
    set_lun->journal_replayed = FBE_TRI_STATE_FALSE;

	fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s:LUN set to ObjId:0x%x\n", __FUNCTION__, fbe_persist_service.database_lun_object_id);

    fbe_transport_set_completion_function(packet, persist_set_database_lun_completion, NULL);

    /*now we have to go and read the persist db header from the very first of this LUN to decide whether to
     *replay journal*/
    return fbe_persist_read_persist_db_header_and_replay_journal(packet);
}

/* This is a local function for handling FBE_PERSIST_CONTROL_CODE_UNSET_LUN. */
static fbe_status_t
persist_unset_database_lun(fbe_packet_t * packet, fbe_cmi_sp_state_t state)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *payload;
    fbe_payload_control_operation_t *control_operation;
    fbe_u32_t buffer_length;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_transport_get_payload_ex() failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_ex_get_control_operation() failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length != 0){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid control buffer length %d, expected %d\n",
                          __FUNCTION__, buffer_length, 0);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*any chance it's already set ?*/
	if (fbe_persist_service.database_lun_object_id != FBE_OBJECT_ID_INVALID) {
        fbe_persist_service.database_lun_object_id = FBE_OBJECT_ID_INVALID;
		fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Lun unset to ObjId:0x%x\n",
                          __FUNCTION__, fbe_persist_service.database_lun_object_id);
	}

    fbe_persist_transaction_reset();
	fbe_persist_database_destroy();
    status = fbe_persist_database_init();
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/* This is a local function for handling FBE_PERSIST_CONTROL_CODE_READ_SECTOR. */
static fbe_status_t
fbe_persist_read_sector(fbe_packet_t * packet)
{
    fbe_payload_ex_t *							payload;
    fbe_payload_control_operation_t *			control_operation;
    fbe_persist_control_read_sector_t *			read_sector;
    fbe_u32_t 									buffer_length;

    if (fbe_persist_service.database_lun_object_id == FBE_OBJECT_ID_INVALID) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s db LUN has not been set (maybe db srv is still proc lost peer). Please retry.\n",
                          __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_transport_get_payload_ex() failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_ex_get_control_operation() failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    read_sector = NULL;
    fbe_payload_control_get_buffer(control_operation, &read_sector);
    if (read_sector == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length != sizeof(fbe_persist_control_read_sector_t)){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid control buffer length %d, expected %llu\n",
                          __FUNCTION__, buffer_length, (unsigned long long)sizeof(fbe_persist_control_read_sector_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return fbe_persist_database_read_sector(packet, read_sector);   
}

/* This is a local function for handling FBE_PERSIST_CONTROL_CODE_READ_ENTRY. it will read a single entry off the disk using entry id*/
static fbe_status_t
persist_read_entry(fbe_packet_t * packet)
{
    fbe_payload_ex_t * 				payload;
    fbe_payload_control_operation_t * 	control_operation;
    fbe_persist_control_read_entry_t * 	control_read;
    fbe_u32_t 							control_read_length;
    fbe_sg_element_t * 					sg_element;
    fbe_u32_t 							sg_element_count;

    if (fbe_persist_service.database_lun_object_id == FBE_OBJECT_ID_INVALID) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s db LUN has not been set (maybe db srv is still proc lost peer). Please retry.\n",
                          __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_sep_payload failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_control_operation failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_read = NULL;
    fbe_payload_control_get_buffer(control_operation, &control_read);
    if (control_read == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_read_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &control_read_length);
    if (control_read_length != sizeof(fbe_persist_control_read_entry_t)) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid len %d != %llu \n", __FUNCTION__,
                          control_read_length, (unsigned long long)sizeof(fbe_persist_control_read_entry_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_element = NULL;
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_element_count);
    if (sg_element == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_ex_get_sg_list failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (sg_element->address == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s sg_list not found\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (sg_element->count > FBE_PERSIST_DATA_BYTES_PER_ENTRY) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid write entry size, got: 0x%x, max: 0x%x\n" ,
                          __FUNCTION__, sg_element->count, FBE_PERSIST_DATA_BYTES_PER_ENTRY);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return fbe_persist_database_read_single_entry(packet, control_read);
}

/* This is a local function for handling FBE_PERSIST_CONTROL_CODE_WRITE_ENTRY. it will eventuall generate a handle
that can be later used to modify the enty if needed*/
static fbe_status_t
persist_write_entry(fbe_packet_t * packet, fbe_payload_control_operation_opcode_t control_code)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_persist_control_write_entry_t * control_write;
    fbe_u32_t control_write_length;
    fbe_sg_element_t * sg_element;
    fbe_u32_t sg_element_count;
    fbe_status_t status;
	fbe_bool_t	auto_insert_entry_id = FBE_FALSE;/*when true, we will overwrite the first 64 bit of the user data with the entry id*/

    if (fbe_persist_service.database_lun_object_id == FBE_OBJECT_ID_INVALID) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s db LUN has not been set (maybe db srv is still proc lost peer). Please retry.\n",
                          __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_sep_payload failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_control_operation failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_write = NULL;
    fbe_payload_control_get_buffer(control_operation, &control_write);
    if (control_write == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_write_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &control_write_length);
    if (control_write_length != sizeof(fbe_persist_control_write_entry_t)) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid len %d != %llu \n", __FUNCTION__,
                          control_write_length, (unsigned long long)sizeof(fbe_persist_control_write_entry_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_element = NULL;
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_element_count);
    if (sg_element == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_ex_get_sg_list failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (sg_element->address == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s sg_list not found\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (sg_element->count > FBE_PERSIST_DATA_BYTES_PER_ENTRY) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid write entry size, got: 0x%x, max: 0x%x\n" ,
                          __FUNCTION__, sg_element->count, FBE_PERSIST_DATA_BYTES_PER_ENTRY);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	auto_insert_entry_id = (control_code == FBE_PERSIST_CONTROL_CODE_WRITE_ENTRY ? FBE_FALSE : FBE_TRUE);

    status = fbe_persist_transaction_write_entry(control_write->tran_handle,
												 control_write->target_sector,
                                                 &control_write->entry_id,
                                                 sg_element->address,
                                                 sg_element->count,
												 auto_insert_entry_id);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/* This is a local function for handling FBE_PERSIST_CONTROL_CODE_MODIFY_ENTRY. */
static fbe_status_t
persist_modify_entry(fbe_packet_t * packet)
{
    fbe_payload_ex_t * 					payload;
    fbe_payload_control_operation_t * 		control_operation;
    fbe_persist_control_modify_entry_t * 	control_modify;
    fbe_u32_t 								control_write_length;
    fbe_sg_element_t * 						sg_element;
    fbe_u32_t 								sg_element_count;
    fbe_status_t 							status;
	fbe_bool_t 								entry_deleted = FBE_FALSE;

    if (fbe_persist_service.database_lun_object_id == FBE_OBJECT_ID_INVALID) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s db LUN has not been set (maybe db srv is still proc lost peer). Please retry.\n",
                          __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_sep_payload failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_control_operation failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_modify = NULL;
    fbe_payload_control_get_buffer(control_operation, &control_modify);
    if (control_modify == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_write_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &control_write_length);
    if (control_write_length != sizeof(fbe_persist_control_modify_entry_t)) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid len %d != %llu \n", __FUNCTION__,
                          control_write_length, (unsigned long long)sizeof(fbe_persist_control_write_entry_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_element = NULL;
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_element_count);
    if (sg_element == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_ex_get_sg_list failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (sg_element->address == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s sg_list not found\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (sg_element->count > FBE_PERSIST_DATA_BYTES_PER_ENTRY) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid write entry size, got: 0x%x, max: 0x%x\n" ,
                          __FUNCTION__, sg_element->count, FBE_PERSIST_DATA_BYTES_PER_ENTRY);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*any chance the user already deleted this entry ?*/
	status = fbe_persist_transaction_is_entry_deleted(control_modify->entry_id, &entry_deleted);
	if ((status != FBE_STATUS_OK) || (FBE_IS_TRUE(entry_deleted))) {
		 fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry already marked for deletion: id: %llu\n",
			  __FUNCTION__, (unsigned long long)control_modify->entry_id);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;

	}

    status = fbe_persist_transaction_modify_entry(control_modify->tran_handle,
                                                 control_modify->entry_id,
                                                 sg_element->address,
                                                 sg_element->count);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/* This is a local function for handling FBE_PERSIST_CONTROL_CODE_DELETE_ENTRY. */
static fbe_status_t
persist_delete_entry(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_persist_control_delete_entry_t * control_delete;
    fbe_u32_t control_delete_length;
    fbe_status_t status;

    if (fbe_persist_service.database_lun_object_id == FBE_OBJECT_ID_INVALID) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s db LUN has not been set (maybe db srv is still proc lost peer). Please retry.\n",
                          __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_sep_payload failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_control_operation failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_delete = NULL;
    fbe_payload_control_get_buffer(control_operation, &control_delete);
    if (control_delete == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_delete_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &control_delete_length);
    if (control_delete_length != sizeof(fbe_persist_control_delete_entry_t)) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid len %d != %llu \n", __FUNCTION__,
                          control_delete_length, (unsigned long long)sizeof(fbe_persist_control_delete_entry_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

   
    status = fbe_persist_transaction_delete_entry(control_delete->tran_handle,
												  control_delete->entry_id);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/* This is a local function for handling FBE_PERSIST_CONTROL_CODE_TRANSACTION. */
static fbe_status_t
persist_transaction(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_persist_control_transaction_t * control_tran;
    fbe_u32_t control_tran_length;
    fbe_status_t status;

	if (fbe_persist_service.database_lun_object_id == FBE_OBJECT_ID_INVALID) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s db LUN has not been set (maybe db srv is still proc lost peer). Please retry.\n",
                          __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_sep_payload failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_control_operation failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_tran = NULL;
    fbe_payload_control_get_buffer(control_operation, &control_tran);
    if (control_tran == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_tran_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &control_tran_length);
    if (control_tran_length != sizeof(fbe_persist_control_transaction_t)) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid len %d != %llu \n", __FUNCTION__,
                          control_tran_length, (unsigned long long)sizeof(fbe_persist_control_transaction_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (control_tran->tran_op_type)
    {
        case FBE_PERSIST_TRANSACTION_OP_START :
            status = fbe_persist_transaction_start(packet, &control_tran->tran_handle);
            if (status != FBE_STATUS_OK) {
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(packet, status, 0);
                fbe_transport_complete_packet(packet);
                return status;
            }
            break;

    case FBE_PERSIST_TRANSACTION_OP_ABORT:
            status = fbe_persist_transaction_abort(control_tran->tran_handle);
            if (status != FBE_STATUS_OK) {
                fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(packet, status, 0);
                fbe_transport_complete_packet(packet);
                return status;
            }
            break;

	case FBE_PERSIST_TRANSACTION_OP_COMMIT:
			/*do we have a LUN to commit to ?*/
			if (fbe_persist_service.database_lun_object_id == FBE_OBJECT_ID_INVALID) {
				fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't commit w/o a LUN being set first\n", __FUNCTION__);

				fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
			}

			/* Do not complete the packet on the commit, the commit performs several asysnchronous I/Os
             * and will complete the packet once these are performed. */

            return fbe_persist_transaction_commit(packet, control_tran->tran_handle);
        default:
            fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s unsupported transaction op\n", __FUNCTION__);
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
persist_write_single_entry(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_persist_control_write_single_entry_t * control_write;
    fbe_u32_t control_write_length;
    fbe_sg_element_t * sg_element;
    fbe_u32_t sg_element_count;
    fbe_status_t status;
	fbe_persist_transaction_handle_t tran_handle;

    if (fbe_persist_service.database_lun_object_id == FBE_OBJECT_ID_INVALID) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s db LUN has not been set (maybe db srv is still proc lost peer). Please retry.\n",
                          __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_sep_payload failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_control_operation failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_write = NULL;
    fbe_payload_control_get_buffer(control_operation, &control_write);
    if (control_write == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_write_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &control_write_length);
    if (control_write_length != sizeof(fbe_persist_control_write_single_entry_t)) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid len %d != %llu \n", __FUNCTION__,
                          control_write_length,
			  (unsigned long long)sizeof(fbe_persist_control_write_entry_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_element = NULL;
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_element_count);
    if (sg_element == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_ex_get_sg_list failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (sg_element->address == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s sg_list not found\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (sg_element->count > FBE_PERSIST_DATA_BYTES_PER_ENTRY) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid write entry size, got: 0x%x, max: 0x%x\n" ,
                          __FUNCTION__, sg_element->count, FBE_PERSIST_DATA_BYTES_PER_ENTRY);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*let's first open a transaction*/
	status = fbe_persist_transaction_start(packet, &tran_handle);
	if (status != FBE_STATUS_OK) {
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s can't open transaction, status:%d \n", __FUNCTION__, status);

		if (status == FBE_STATUS_BUSY) {
			fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_BUSY);
		}else{
			fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		}

		fbe_transport_set_status(packet, status, 0);
		fbe_transport_complete_packet(packet);
		return status;
	}

    status = fbe_persist_transaction_write_entry(tran_handle,
												 control_write->target_sector,
                                                 &control_write->entry_id,
                                                 sg_element->address,
                                                 sg_element->count,
												 FBE_FALSE);
    if (status != FBE_STATUS_OK) {
		fbe_persist_transaction_abort(tran_handle);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*now let's commit it*/
	return fbe_persist_transaction_commit(packet, tran_handle);
}


static fbe_status_t
persist_modify_single_entry(fbe_packet_t * packet)
{
    fbe_payload_ex_t * 						payload;
    fbe_payload_control_operation_t * 			control_operation;
    fbe_persist_control_modify_single_entry_t * control_modify;
    fbe_u32_t 									control_write_length;
    fbe_sg_element_t * 							sg_element;
    fbe_u32_t 									sg_element_count;
    fbe_status_t 								status;
	fbe_bool_t 									entry_deleted = FBE_FALSE;
	fbe_persist_transaction_handle_t 			tran_handle;

    if (fbe_persist_service.database_lun_object_id == FBE_OBJECT_ID_INVALID) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s db LUN has not been set (maybe db srv is still proc lost peer). Please retry.\n",
                          __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_sep_payload failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_control_operation failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_modify = NULL;
    fbe_payload_control_get_buffer(control_operation, &control_modify);
    if (control_modify == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_write_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &control_write_length);
    if (control_write_length != sizeof(fbe_persist_control_modify_single_entry_t)) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid len %d != %llu \n", __FUNCTION__,
                          control_write_length,
			  (unsigned long long)sizeof(fbe_persist_control_write_entry_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_element = NULL;
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_element_count);
    if (sg_element == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_ex_get_sg_list failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (sg_element->address == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s sg_list not found\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (sg_element->count > FBE_PERSIST_DATA_BYTES_PER_ENTRY) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid write entry size, got: 0x%x, max: 0x%x\n" ,
                          __FUNCTION__, sg_element->count, FBE_PERSIST_DATA_BYTES_PER_ENTRY);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*any chance the user already deleted this entry ?*/
	status = fbe_persist_transaction_is_entry_deleted(control_modify->entry_id, &entry_deleted);
	if ((status != FBE_STATUS_OK) || (FBE_IS_TRUE(entry_deleted))) {
		 fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry already marked for deletion: id: %lld\n" ,__FUNCTION__, (long long)control_modify->entry_id);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;

	}

	/*let's first open a transaction*/
	status = fbe_persist_transaction_start(packet, &tran_handle);
	if (status != FBE_STATUS_OK) {
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s can't open transaction, status:%d \n", __FUNCTION__, status);

		if (status == FBE_STATUS_BUSY) {
			fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_BUSY);
		}else{
			fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		}

		fbe_transport_set_status(packet, status, 0);
		fbe_transport_complete_packet(packet);
		return status;
	}

    status = fbe_persist_transaction_modify_entry(tran_handle,
                                                 control_modify->entry_id,
                                                 sg_element->address,
                                                 sg_element->count);
    if (status != FBE_STATUS_OK) {
		fbe_persist_transaction_abort(tran_handle);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now let's commit it*/
	return fbe_persist_transaction_commit(packet, tran_handle);
}

static fbe_status_t
persist_delete_single_entry(fbe_packet_t * packet)
{
	fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_persist_control_delete_single_entry_t * control_delete;
    fbe_u32_t control_delete_length;
    fbe_status_t status;
	fbe_persist_transaction_handle_t 			tran_handle;

    if (fbe_persist_service.database_lun_object_id == FBE_OBJECT_ID_INVALID) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s db LUN has not been set (maybe db srv is still proc lost peer). Please retry.\n",
                          __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_sep_payload failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_control_operation failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_delete = NULL;
    fbe_payload_control_get_buffer(control_operation, &control_delete);
    if (control_delete == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_delete_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &control_delete_length);
    if (control_delete_length != sizeof(fbe_persist_control_delete_single_entry_t)) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid len %d != %llu \n", __FUNCTION__,
                          control_delete_length,
			  (unsigned long long)sizeof(fbe_persist_control_delete_entry_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }


	/*let's first open a transaction*/
	status = fbe_persist_transaction_start(packet, &tran_handle);
	if (status != FBE_STATUS_OK) {
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s can't open transaction, status:%d \n", __FUNCTION__, status);

		if (status == FBE_STATUS_BUSY) {
			fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_BUSY);
		}else{
			fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		}

		fbe_transport_set_status(packet, status, 0);
		fbe_transport_complete_packet(packet);
		return status;
	}

    status = fbe_persist_transaction_delete_entry(tran_handle, control_delete->entry_id);
    if (status != FBE_STATUS_OK) {
		fbe_persist_transaction_abort(tran_handle);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now let's commit it*/
	return fbe_persist_transaction_commit(packet, tran_handle);

}

/* This is the entry point for handling persist service control requests. */
static fbe_status_t
persist_control_entry(fbe_packet_t * packet)
{
    fbe_status_t 							status;
    fbe_payload_control_operation_opcode_t 	control_code;
	fbe_cmi_sp_state_t						state = FBE_CMI_STATE_PASSIVE;

    control_code = fbe_transport_get_control_code(packet);

    /* The service must be intitialized before other control codes can be processed. */
    if (control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        status = persist_init(packet);
        return status;
    }
    if (!(fbe_base_service_is_initialized((fbe_base_service_t *)&fbe_persist_service))) {
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

	fbe_cmi_get_current_sp_state(&state);/*when we come here even the first time, CMI can't be BUSY anymore so we don't have to check*/

	/*these operations can be doe on either active or passive side*/
    switch(control_code) {
		case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = persist_destroy(packet);
            return status;
		case FBE_PERSIST_CONTROL_CODE_GET_LAYOUT_INFO:
			status = fbe_persist_get_layout_info(packet);
			return status;
		case FBE_PERSIST_CONTROL_CODE_GET_REQUEIRED_LUN_SIZE:
			status = fbe_persist_get_required_lun_size(packet);
			return status;
        case FBE_PERSIST_CONTROL_CODE_UNSET_LUN:
            status = persist_unset_database_lun(packet, state);            
            return status;
        case FBE_PERSIST_CONTROL_CODE_HOOK:
            status = persist_perform_hook(packet);
            return status;
	}

	/*are we the active side*/
	if (state != FBE_CMI_STATE_ACTIVE) {
		 fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, /* This is not error if SP is shuting down */
						  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						  "%s:SP state is not active(%d), can't do persist operation:%d\n", __FUNCTION__, state, control_code);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}
	

    /* The following control codes can be handled only after the database lun has been set and on the active side only. */
    switch(control_code) {
		case FBE_PERSIST_CONTROL_CODE_SET_LUN:
			status = persist_set_database_lun(packet);
			return status;
        case FBE_PERSIST_CONTROL_CODE_READ_ENTRY:
            status = persist_read_entry(packet);
            break;
		case FBE_PERSIST_CONTROL_CODE_WRITE_ENTRY:
		case FBE_PERSIST_CONTROL_CODE_WRITE_ENTRY_WITH_AUTO_ENTRY_ID:
            status = persist_write_entry(packet, control_code);
            break;
        case FBE_PERSIST_CONTROL_CODE_TRANSACTION:
            status = persist_transaction(packet);
            break;
		case FBE_PERSIST_CONTROL_CODE_MODIFY_ENTRY:
            status = persist_modify_entry(packet);
            break;
		case FBE_PERSIST_CONTROL_CODE_DELETE_ENTRY:
			status = persist_delete_entry(packet);
            break;
        case FBE_PERSIST_CONTROL_CODE_READ_SECTOR:
			status = fbe_persist_read_sector(packet);
			break;
		case FBE_PERSIST_CONTROL_CODE_WRITE_SINGLE_ENTRY:
			status = persist_write_single_entry(packet);
			break;
		case FBE_PERSIST_CONTROL_CODE_MODIFY_SINGLE_ENTRY:
			status = persist_modify_single_entry(packet);
			break;
		case FBE_PERSIST_CONTROL_CODE_DELETE_SINGLE_ENTRY:
			status = persist_delete_single_entry(packet);
			break;
        case FBE_PERSIST_CONTROL_CODE_GET_ENTRY_INFO:
            status = persist_get_entry_info(packet);
            break;
        case FBE_PERSIST_CONTROL_CODE_VALIDATE_ENTRY:
            status = persist_validate_entry(packet);
            break;
        default:
            return fbe_base_service_control_entry((fbe_base_service_t *) &fbe_persist_service, packet);
            break;
    }

    return status;
}

void
fbe_persist_trace(fbe_trace_level_t trace_level,
                  fbe_trace_message_id_t message_id,
                  const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized(&fbe_persist_service.base_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&fbe_persist_service.base_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_PERSIST,
                     trace_level,
                     message_id,
                     fmt,
                     args);
    va_end(args);
}


static fbe_status_t fbe_persist_get_layout_info(fbe_packet_t * packet)
{
    fbe_payload_ex_t *							payload;
    fbe_payload_control_operation_t *			control_operation;
    fbe_persist_control_get_layout_info_t *		get_layout;
    fbe_u32_t 									buffer_length;
	fbe_status_t								status;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_sep_payload failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_control_operation() failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_layout = NULL;
    fbe_payload_control_get_buffer(control_operation, &get_layout);
    if (get_layout == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length != sizeof(fbe_persist_control_get_layout_info_t)){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid control buffer length %d, expected %llu\n",
                          __FUNCTION__, buffer_length, (unsigned long long)sizeof(fbe_persist_control_get_layout_info_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	get_layout->lun_object_id = fbe_persist_service.database_lun_object_id;

	status = fbe_persist_database_get_lba_layout_info(get_layout);
	if(status != FBE_STATUS_OK){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s faild to get layout info from database\n",
                          __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
	
   
}

static fbe_status_t fbe_persist_get_required_lun_size(fbe_packet_t * packet)
{
	fbe_payload_ex_t *								payload;
    fbe_payload_control_operation_t *				control_operation;
    fbe_persist_control_get_required_lun_size_t *	get_size;
    fbe_u32_t 										buffer_length;
    fbe_u32_t										entry_count;
	fbe_lba_t										total_blocks = 0;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_sep_payload failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_control_operation() failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_size = NULL;
    fbe_payload_control_get_buffer(control_operation, &get_size);
    if (get_size == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length != sizeof(fbe_persist_control_get_required_lun_size_t)){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid control buffer length %d, expected %llu\n",
                          __FUNCTION__, buffer_length,
			  (unsigned long long)sizeof(fbe_persist_control_get_layout_info_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*first of all see how many entries we have*/
	fbe_persist_database_layout_get_max_total_entries(&entry_count);

	/*on top of that we'll add the 1 block for stamp and 4 full transaction entries we have at the start of the LUN*/
	entry_count *= FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY;
	total_blocks += entry_count;
	total_blocks += FBE_PERSIST_BLOCKS_PER_HEADER;/*for the header block*/
	total_blocks += (FBE_PERSIST_TRANSACTION_COUNT_IN_JOURNAL_AREA * FBE_PERSIST_BLOCKS_PER_TRAN);

	get_size->total_block_needed = total_blocks;

	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}


/*!*******************************************************************************************
 * @fn persist_perform_hook
 *********************************************************************************************
* @brief
*  set/unset/ hook or get hook state from this function
*
* @param packet - request packet with fbe_persist_control_hook_t control buffer in
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/23/2012: Zhipeng Hu - created
 *
 ********************************************************************************************/ 
static fbe_status_t persist_perform_hook(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload;
    fbe_payload_control_operation_t*    control_operation;
    fbe_persist_control_hook_t*         hook_operation;
    fbe_u32_t                           buffer_length;
    fbe_status_t                        status;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: get_sep_payload failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: get_control_operation() failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    hook_operation = NULL;
    fbe_payload_control_get_buffer(control_operation, &hook_operation);
    if (hook_operation == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length != sizeof(fbe_persist_control_hook_t)){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s Invalid control buffer length %d, expected %d\n",
            __FUNCTION__, buffer_length, (int)sizeof(fbe_persist_control_hook_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch(hook_operation->hook_op_code)
    {
    case FBE_PERSIST_HOOK_OPERATION_GET_STATE:
        status = persist_get_hook_state(hook_operation->hook_type,
                                        &hook_operation->is_triggered, &hook_operation->is_set_esp,
                                        &hook_operation->is_set_sep);
        break;
    case FBE_PERSIST_HOOK_OPERATION_REMOVE_HOOK:
        status = persist_remove_hook(hook_operation->hook_type, hook_operation->is_set_esp,
                                     hook_operation->is_set_sep);
        break;

    case FBE_PERSIST_HOOK_OPERATION_SET_HOOK:
        status = persist_set_hook(hook_operation->hook_type, hook_operation->is_set_esp,
                                  hook_operation->is_set_sep);
        break;

    default:
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    if(FBE_STATUS_OK != status)
    {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s failed opt. hook type: %d, hook opt: %d\n",
            __FUNCTION__, hook_operation->hook_type, hook_operation->hook_op_code);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/* This is a local function for handling FBE_PERSIST_CONTROL_CODE_GET_ENTRY_INFO. Gets information about the state of the entry.*/
static fbe_status_t
persist_get_entry_info(fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_persist_control_get_entry_info_t   *get_entry_info = NULL;
    fbe_u32_t                               get_entry_info_length = 0;

    if (fbe_persist_service.database_lun_object_id == FBE_OBJECT_ID_INVALID) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s db LUN has not been set (maybe db srv is still proc lost peer). Please retry.\n",
                          __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_sep_payload failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_control_operation failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &get_entry_info);
    if (get_entry_info == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &get_entry_info_length);
    if (get_entry_info_length != sizeof(fbe_persist_control_get_entry_info_t)) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid len %d != %llu \n", __FUNCTION__,
                          get_entry_info_length, (unsigned long long)sizeof(fbe_persist_control_get_entry_info_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Populate the return values.*/
    status = fbe_persist_entry_exists(get_entry_info->entry_id, &get_entry_info->b_does_entry_exist);
    if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_persist_entry_exits failed - status: 0x%x\n", __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/* This is a local function for handling FBE_PERSIST_CONTROL_CODE_VALIDATE_ENTRY. */
static fbe_status_t persist_validate_entry(fbe_packet_t * packet)
{
    fbe_payload_ex_t * 					payload;
    fbe_payload_control_operation_t * 		control_operation;
    fbe_persist_control_validate_entry_t * 	control_validate;
    fbe_u32_t 								control_write_length;
    fbe_status_t 							status;

    if (fbe_persist_service.database_lun_object_id == FBE_OBJECT_ID_INVALID) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s db LUN has not been set (maybe db srv is still proc lost peer). Please retry.\n",
                          __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_sep_payload failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_control_operation failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_validate = NULL;
    fbe_payload_control_get_buffer(control_operation, &control_validate);
    if (control_validate == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_write_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &control_write_length);
    if (control_write_length != sizeof(fbe_persist_control_validate_entry_t)) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid len %d != %llu \n", __FUNCTION__,
                          control_write_length, (unsigned long long)sizeof(fbe_persist_control_validate_entry_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate the entry*/
    status = fbe_persist_transaction_validate_entry(control_validate->tran_handle,
                                                    control_validate->entry_id);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
