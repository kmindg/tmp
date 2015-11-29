/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                fbe_service_manager_main.c
 ***************************************************************************/

#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_service.h"
#include "fbe/fbe_service_manager_interface.h"
#include "fbe/fbe_transport.h"
#include "fbe_service_manager.h"

typedef struct fbe_service_manager_service_s{
    fbe_base_service_t base_service;
}fbe_service_manager_service_t;

static fbe_service_control_entry_t physical_package_control_entry = NULL;
static fbe_service_control_entry_t sep_control_entry = NULL;
static fbe_service_control_entry_t esp_package_control_entry = NULL;
static fbe_service_control_entry_t neit_package_control_entry = NULL;
static fbe_service_control_entry_t kms_package_control_entry = NULL;

static const fbe_service_methods_t ** service_manager_service_table = NULL;

static fbe_status_t fbe_service_manager_control_entry(fbe_packet_t * packet);

fbe_service_methods_t fbe_service_manager_service_methods = {FBE_SERVICE_ID_SERVICE_MANAGER, fbe_service_manager_control_entry};

static fbe_service_manager_service_t service_manager_service;

/* Forward declaration */
static const fbe_service_methods_t * service_manager_get_service(fbe_service_id_t service_id);

static fbe_status_t service_manager_get_composition(fbe_packet_t * packet);
static fbe_status_t service_manager_set_physical_package_control_entry(fbe_packet_t * packet);

/* Declare base service methods */
fbe_status_t fbe_service_manager_control_entry(fbe_packet_t * packet); 
void  fbe_service_manager_user_induced_panic_sp(fbe_packet_t * packet);

void
service_manager_trace(fbe_trace_level_t trace_level,
                      fbe_trace_message_id_t message_id,
                      const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized(&service_manager_service.base_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&service_manager_service.base_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_SERVICE_MANAGER,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
}

/* Service table initialization */
fbe_status_t 
fbe_service_manager_init_service_table(const fbe_service_methods_t * service_table[])
{
    const fbe_service_methods_t *** service_manager_service_table_ptr = &service_manager_service_table;

    *service_manager_service_table_ptr = service_table;

	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_service_manager_init(fbe_packet_t * packet)
{
	service_manager_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	if(service_manager_service_table == NULL){
		service_manager_trace(FBE_TRACE_LEVEL_ERROR,
		                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                      "%s service_manager_service_table not initialized\n", __FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_base_service_init((fbe_base_service_t *) &service_manager_service);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_service_manager_destroy(fbe_packet_t * packet)
{
	service_manager_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	fbe_base_service_destroy((fbe_base_service_t *) &service_manager_service);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_service_manager_control_entry(fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_payload_control_operation_opcode_t control_code;

	control_code = fbe_transport_get_control_code(packet);
	if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
		status = fbe_service_manager_init(packet);
		return status;
	}

	if(!fbe_base_service_is_initialized((fbe_base_service_t *) &service_manager_service)){
		fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_NOT_INITIALIZED;
	}

	switch(control_code) {
		case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
			status = fbe_service_manager_destroy( packet);
			break;

		case FBE_BASE_SERVICE_CONTROL_CODE_GET_COMPOSITION:
			status = service_manager_get_composition( packet);
			break;

		case FBE_SERVICE_MANAGER_CONTROL_CODE_SET_PHYSICAL_PACKAGE_CONTROL_ENTRY:
			status = service_manager_set_physical_package_control_entry( packet);
			break;

        case FBE_BASE_SERVICE_CONTROL_CODE_USER_INDUCED_PANIC:
            fbe_service_manager_user_induced_panic_sp( packet);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

		default:
			service_manager_trace(FBE_TRACE_LEVEL_ERROR,
			                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
			                      "%s: Unknown control code: 0x%X\n", __FUNCTION__, control_code);
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			status = FBE_STATUS_GENERIC_FAILURE;
			fbe_transport_complete_packet(packet);
			break;
	}

	return status;
}

static const fbe_service_methods_t * 
service_manager_get_service(fbe_service_id_t service_id)
{   
    const fbe_service_methods_t * pservice = NULL;
	const fbe_service_methods_t * retval = NULL;
    fbe_u32_t i;

	if(service_manager_service_table == NULL){
		return NULL;
	}

    for(i = 0; service_manager_service_table[i] != NULL; i++) {
        pservice = service_manager_service_table[i];
        if(pservice->service_id == service_id) {
			retval = pservice;
            break;
        }
    }
    return retval;
}

fbe_status_t
fbe_service_manager_send_control_packet(fbe_packet_t * packet)
{
    const fbe_service_methods_t * pservice;
    fbe_service_id_t service_id;
    fbe_status_t status;
	fbe_payload_ex_t * payload = NULL;
	fbe_package_id_t my_package_id;
	fbe_package_id_t destination_package_id;

	fbe_get_package_id(&my_package_id);
	fbe_transport_get_package_id(packet, &destination_package_id);
  
	if(my_package_id != destination_package_id){
		if((destination_package_id == FBE_PACKAGE_ID_PHYSICAL) && (physical_package_control_entry != NULL)){
			status = physical_package_control_entry(packet);
			return status;
		}
		else if((destination_package_id == FBE_PACKAGE_ID_SEP_0) && (sep_control_entry != NULL)){
		    status = sep_control_entry(packet);
		    return status;

  		} else if((destination_package_id == FBE_PACKAGE_ID_ESP) && (esp_package_control_entry != NULL)){
		    status = esp_package_control_entry(packet);
		    return status;
		} else if((destination_package_id == FBE_PACKAGE_ID_NEIT) && (neit_package_control_entry != NULL)){
		    status = neit_package_control_entry(packet);
		    return status;
		} else if((destination_package_id == FBE_PACKAGE_ID_KMS) && (kms_package_control_entry != NULL)){
		    status = kms_package_control_entry(packet);
		    return status;
		} else {
			service_manager_trace(FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s: Invalid destination package id %d \n", __FUNCTION__, destination_package_id);

            payload = fbe_transport_get_payload_ex(packet);
            fbe_payload_ex_increment_control_operation_level(payload);

            fbe_transport_set_status(packet, FBE_STATUS_COMPONENT_NOT_FOUND, 0);
			fbe_transport_complete_packet(packet);
			return FBE_STATUS_COMPONENT_NOT_FOUND;
		}
	}

	status = fbe_transport_get_service_id(packet, &service_id);

    pservice = service_manager_get_service(service_id);
    if(pservice == NULL){
		service_manager_trace(FBE_TRACE_LEVEL_ERROR,
			      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
			      "%s: service_manager_get_service fail\n", __FUNCTION__);

        payload = fbe_transport_get_payload_ex(packet);
        fbe_payload_ex_increment_control_operation_level(payload);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_increment_control_operation_level(payload);

    status =  pservice->control_entry(packet); 
    return status;
}


static fbe_status_t
service_manager_get_composition(fbe_packet_t * packet)
{
	fbe_base_service_get_composition_t * service_mgmt_get_composition = NULL;
	fbe_u32_t i;
    const fbe_service_methods_t * pservice = NULL;
	fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	service_manager_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer_length(control_operation, &len); 
	if(len != sizeof(fbe_base_service_get_composition_t)){
		service_manager_trace(FBE_TRACE_LEVEL_ERROR,
		                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                      "%s Invalid buffer_len \n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer(control_operation, &service_mgmt_get_composition); 
	if(service_mgmt_get_composition == NULL){
		service_manager_trace(FBE_TRACE_LEVEL_ERROR,
		                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                      "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Clean up the array */
	for (i = 0 ; i < FBE_BASE_SERVICE_MAX_ELEMENTS; i++){
		service_mgmt_get_composition->element_id_array[i] = FBE_SERVICE_ID_INVALID;
	}

	/* Scan the service_table */
	service_mgmt_get_composition->number_of_elements = 0;

    for(i = 0; service_manager_service_table[i] != NULL; i++) {
        pservice = service_manager_service_table[i];
        service_mgmt_get_composition->element_id_array[service_mgmt_get_composition->number_of_elements] = pservice->service_id;
		service_mgmt_get_composition->number_of_elements++;
    }

	service_manager_trace(FBE_TRACE_LEVEL_INFO, 
	                      FBE_TRACE_MESSAGE_ID_INFO,
	                      "%s: number_of_elements 0x%X \n", __FUNCTION__, service_mgmt_get_composition->number_of_elements);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}


static fbe_status_t
service_manager_set_physical_package_control_entry(fbe_packet_t * packet)
{
	fbe_service_manager_control_set_physical_package_control_entry_t * set_physical_package_control_entry = NULL;
	fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	service_manager_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer_length(control_operation, &len); 
	if(len != sizeof(fbe_service_manager_control_set_physical_package_control_entry_t)){
		service_manager_trace(FBE_TRACE_LEVEL_ERROR,
		                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                      "%s Invalid buffer_len \n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer(control_operation, &set_physical_package_control_entry); 
	if(set_physical_package_control_entry == NULL){
		service_manager_trace(FBE_TRACE_LEVEL_ERROR,
		                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                      "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}
	
	fbe_service_manager_set_physical_package_control_entry(set_physical_package_control_entry->control_entry);

	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_service_manager_set_physical_package_control_entry(fbe_service_control_entry_t control_entry)
{
	physical_package_control_entry = control_entry;
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_service_manager_set_sep_control_entry(fbe_service_control_entry_t control_entry)
{
	sep_control_entry = control_entry;
	return FBE_STATUS_OK;
}
fbe_status_t
fbe_service_manager_set_neit_control_entry(fbe_service_control_entry_t control_entry)
{
	neit_package_control_entry = control_entry;
	return FBE_STATUS_OK;
}

fbe_status_t fbe_service_manager_set_esp_package_control_entry(fbe_service_control_entry_t control_entry)
{
	esp_package_control_entry = control_entry;
	return FBE_STATUS_OK;
}

fbe_status_t fbe_service_manager_set_kms_control_entry(fbe_service_control_entry_t control_entry)
{
	kms_package_control_entry = control_entry;
	return FBE_STATUS_OK;
}

/*!**************************************************************************
 * @fbe_service_manager_panic_sp.c
 ***************************************************************************
 * @brief
 *  This function calls FBE_PANIC_SP Macro.
 *
 *@param packet - The packet that is arriving.
 *
 *
 * @author-Hardik Joshi
 * 
 *
 ***************************************************************************/
void fbe_service_manager_user_induced_panic_sp(fbe_packet_t * packet)
{
    service_manager_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry. User Induced Panic\n", __FUNCTION__);
}
