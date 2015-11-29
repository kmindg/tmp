#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe_transport_memory.h"
#include "fbe_ses.h"
#include "sas_bullet_enclosure_private.h"


/* Export class methods  */
fbe_class_methods_t fbe_sas_bullet_enclosure_class_methods = {FBE_CLASS_ID_SAS_BULLET_ENCLOSURE,
												        fbe_sas_bullet_enclosure_load,
												        fbe_sas_bullet_enclosure_unload,
												        fbe_sas_bullet_enclosure_create_object,
												        fbe_sas_bullet_enclosure_destroy_object,
												        fbe_sas_bullet_enclosure_control_entry,
												        fbe_sas_bullet_enclosure_event_entry,
                                                        fbe_sas_bullet_enclosure_io_entry,
														fbe_sas_bullet_enclosure_monitor_entry};


/* Forward declaration */
static fbe_status_t fbe_sas_bullet_enclosure_get_encl_device_details(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t *packet);

static fbe_status_t fbe_sas_bullet_enclosure_get_encl_device_details_completion(fbe_packet_t * sub_packet, fbe_packet_completion_context_t context);

static fbe_status_t sas_bullet_enclosure_get_status_page(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t *packet);

static fbe_status_t sas_bullet_enclosure_get_status_page_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t sas_bullet_enclosure_process_status_page(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_u8_t *resp_buffer);

static fbe_status_t fbe_sas_bullet_enclosure_get_device_info_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_sas_bullet_enclosure_get_device_info(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t *packet);

static fbe_status_t sas_bullet_enclosure_slot_monitor(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_u8_t *resp_buffer);

static fbe_status_t fbe_sas_bullet_enclosure_get_parent_address(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet);

static fbe_status_t fbe_sas_bullet_enclosure_turn_fault_led_on(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t *packet, fbe_u32_t slot_id);

static fbe_status_t fbe_sas_bullet_enclosure_turn_fault_led_on_completion(fbe_packet_t * sub_packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_ses_build_encl_control_data(fbe_u8_t *cmd, fbe_u32_t cmd_size);

static fbe_status_t fbe_sas_bullet_enclosure_set_drive_fault_led(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet);

/* Monitors */
static fbe_status_t fbe_sas_bullet_enclosure_monitor_instantiated(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet);
static fbe_status_t fbe_sas_bullet_enclosure_monitor_specialized(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet);
static fbe_status_t fbe_sas_bullet_enclosure_monitor_ready(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet);

/* fbe_bool_t first_time = TRUE; */
fbe_status_t 
fbe_sas_bullet_enclosure_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

	FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_sas_bullet_enclosure_t) < FBE_MEMORY_CHUNK_SIZE);

    fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_LOW, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: object size %llu\n", __FUNCTION__, (unsigned long long)sizeof(fbe_sas_bullet_enclosure_t));

	return fbe_sas_bullet_enclosure_monitor_load_verify();
}

fbe_status_t 
fbe_sas_bullet_enclosure_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_bullet_enclosure_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
	fbe_sas_bullet_enclosure_t * sas_bullet_enclosure;
	fbe_status_t status;

    fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

	/* Call parent constructor */
	status = fbe_base_enclosure_create_object(packet, object_handle);
	if(status != FBE_STATUS_OK){
		return status;
	}

	/* Set class id */
	sas_bullet_enclosure = (fbe_sas_bullet_enclosure_t *)fbe_base_handle_to_pointer(*object_handle);
	fbe_base_object_set_class_id((fbe_base_object_t *) sas_bullet_enclosure, FBE_CLASS_ID_SAS_BULLET_ENCLOSURE);	

	fbe_sas_bullet_enclosure_init(sas_bullet_enclosure);

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_bullet_enclosure_destroy_object( fbe_object_handle_t object_handle)
{
	fbe_status_t status;
	fbe_sas_bullet_enclosure_t * sas_bullet_enclosure;

    fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
	
	sas_bullet_enclosure = (fbe_sas_bullet_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

	/* Cleanup */
	fbe_transport_release_buffer(sas_bullet_enclosure->drive_info);	

	/* Call parent destructor */
	status = fbe_sas_enclosure_destroy_object(object_handle);
	return status;
}



fbe_status_t 
fbe_sas_bullet_enclosure_init(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure)
{
	fbe_u32_t i;

	FBE_ASSERT_AT_COMPILE_TIME((FBE_SAS_BULLET_ENCLOSURE_NUMBER_OF_SLOTS * sizeof(fbe_sas_bullet_enclosure_drive_info_t)) < FBE_MEMORY_CHUNK_SIZE);

	sas_bullet_enclosure->drive_info = fbe_transport_allocate_buffer();

	for (i = 0; i < FBE_SAS_BULLET_ENCLOSURE_NUMBER_OF_SLOTS ; i++){
		sas_bullet_enclosure->drive_info[i].sas_address = FBE_SAS_ADDRESS_INVALID;
		sas_bullet_enclosure->drive_info[i].generation_code = 0;
	}

	sas_bullet_enclosure->expansion_port_info.sas_address = FBE_SAS_ADDRESS_INVALID;
	sas_bullet_enclosure->expansion_port_info.generation_code = 0;


	return FBE_STATUS_OK;
}

