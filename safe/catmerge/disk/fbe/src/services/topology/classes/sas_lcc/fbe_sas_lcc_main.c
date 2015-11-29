#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe_transport_memory.h"
#include "fbe_transport_memory.h"
#include "fbe_scsi.h"
#include "sas_lcc_private.h"
#include "fbe_ses.h"

/* Class methods forward declaration */
fbe_status_t sas_lcc_load(void);
fbe_status_t sas_lcc_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_sas_lcc_class_methods = {FBE_CLASS_ID_SAS_LCC,
												 sas_lcc_load,
												 sas_lcc_unload,
												 fbe_sas_lcc_create_object,
												 fbe_sas_lcc_destroy_object,
												 fbe_sas_lcc_control_entry,
												 fbe_sas_lcc_event_entry,
                                                 NULL,
												 fbe_sas_lcc_monitor_entry};


/* Forward declaration */
static fbe_status_t sas_lcc_create_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t sas_lcc_update_sas_element(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet);
static fbe_status_t sas_lcc_update_sas_element_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t sas_lcc_send_inquiry(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet);
static fbe_status_t sas_lcc_send_inquiry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t sas_lcc_get_config_page(fbe_sas_lcc_t * sas_lcc, fbe_packet_t *packet);
static fbe_status_t sas_lcc_get_config_page_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_lcc_process_config_page(fbe_sas_lcc_t * sas_lcc, fbe_u8_t *resp_buffer);

static fbe_status_t sas_lcc_update_phy_table(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet);
static fbe_status_t sas_lcc_update_phy_table_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_lcc_process_phy_table(fbe_sas_lcc_t * sas_lcc, fbe_sas_element_t * phy_table);
static fbe_status_t sas_lcc_parent_list_monitor(fbe_sas_lcc_t * sas_lcc);
static fbe_status_t sas_lcc_expansion_port_monitor(fbe_sas_lcc_t * sas_lcc);
static fbe_status_t sas_lcc_slot_monitor(fbe_sas_lcc_t * sas_lcc);

static fbe_status_t sas_lcc_get_parent_sas_element(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet);

static fbe_status_t sas_lcc_send_broadcast_event(fbe_sas_lcc_t * sas_lcc);
static fbe_status_t sas_lcc_event_monitor(fbe_sas_lcc_t * sas_lcc);


/* Monitors */
static fbe_status_t sas_lcc_monitor_destroy_pending(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet);
static fbe_status_t sas_lcc_monitor_destroy(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet);
static fbe_status_t sas_lcc_monitor_instantiated(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet);
static fbe_status_t sas_lcc_monitor_ready(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet);

fbe_status_t 
sas_lcc_load(void)
{
	KvTrace("%s entry\n", __FUNCTION__);
	return FBE_STATUS_OK;
}

fbe_status_t 
sas_lcc_unload(void)
{
	KvTrace("%s entry\n", __FUNCTION__);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_lcc_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
	fbe_sas_lcc_t * sas_lcc;
	fbe_status_t status;

	KvTrace("%s entry\n", __FUNCTION__);

	/* Call parent constructor */
	status = fbe_base_lcc_create_object(packet, object_handle);
	if(status != FBE_STATUS_OK){
		return status;
	}

	/* Set class id */
	sas_lcc = (fbe_sas_lcc_t *)fbe_base_handle_to_pointer(*object_handle);
	fbe_base_object_set_class_id((fbe_base_object_t *) sas_lcc, FBE_CLASS_ID_SAS_LCC);	

	/* Set number of slots */
	fbe_base_lcc_set_number_of_slots((fbe_base_lcc_t *)sas_lcc, FBE_SAS_LCC_NUMBER_OF_SLOTS);
	fbe_base_lcc_set_first_slot_index((fbe_base_lcc_t *)sas_lcc, FBE_SAS_LCC_FIRST_SLOT_INDEX);

	fbe_base_lcc_set_number_of_expansion_ports((fbe_base_lcc_t *)sas_lcc, FBE_SAS_LCC_NUMBER_OF_EXPANSION_PORTS);
	fbe_base_lcc_set_first_expansion_port_index((fbe_base_lcc_t *)sas_lcc, FBE_SAS_LCC_FIRST_EXPANSION_PORT_INDEX);

	sas_lcc->sas_lcc_attributes = FBE_SAS_LCC_INQUIRY_REQUIERED; 
	sas_lcc->sas_lcc_attributes |= FBE_SAS_LCC_CONFIG_PAGE_REQUIERED; 
	sas_lcc->sas_lcc_attributes |= FBE_SAS_LCC_UPDATE_PHY_TABLE_REQUIERED;

	sas_lcc->sas_address = FBE_SAS_ADDRESS_INVALID;
    sas_lcc->cpd_device_id = FBE_TARGET_ID_INVALID;
    sas_lcc->lcc_type = FBE_SAS_DEVICE_TYPE_INVALID;
	
	/* We have to think about different allocation scheme */
	sas_lcc->phy_table = fbe_transport_allocate_buffer();
	fbe_zero_memory(sas_lcc->phy_table, FBE_MEMORY_CHUNK_SIZE);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_lcc_destroy_object( fbe_object_handle_t object_handle)
{
	fbe_status_t status;
	fbe_sas_lcc_t * sas_lcc;

	KvTrace("%s entry\n", __FUNCTION__);
	
	sas_lcc = (fbe_sas_lcc_t *)fbe_base_handle_to_pointer(object_handle);

	/* Check parent edges */
	/* Cleanup */
	fbe_transport_release_buffer(sas_lcc->phy_table);

	/* Call parent destructor */
	status = fbe_base_lcc_destroy_object(object_handle);
	return status;
}

fbe_status_t 
fbe_sas_lcc_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_lcc_t * sas_lcc = NULL;
	fbe_payload_control_operation_opcode_t control_code;

	sas_lcc = (fbe_sas_lcc_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace(	(fbe_base_object_t*)sas_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

	control_code = fbe_transport_get_control_code(packet);
	switch(control_code) {
		case FBE_BASE_OBJECT_CONTROL_CODE_MONITOR:
			status = fbe_sas_lcc_monitor(sas_lcc, packet);
			break;
		case FBE_BASE_OBJECT_CONTROL_CODE_CREATE_EDGE:
			status = fbe_sas_lcc_create_edge(sas_lcc, packet);
			break;
		case FBE_SAS_PORT_CONTROL_CODE_GET_PARENT_SAS_ELEMENT:
			status = sas_lcc_get_parent_sas_element(sas_lcc, packet);	
			break;
		default:
			status = fbe_base_lcc_control_entry(object_handle, packet);
			break;
	}

	return status;
}

fbe_status_t
fbe_sas_lcc_monitor(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet)
{
	fbe_object_state_t mgmt_state;
	fbe_status_t status;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entered\n", __FUNCTION__);

	/* We need to think about locking here */
	fbe_base_object_get_mgmt_state((fbe_base_object_t *) sas_lcc, &mgmt_state);

	switch(mgmt_state) {
		case FBE_OBJECT_STATE_INSTANTIATED:
			status = sas_lcc_monitor_instantiated(sas_lcc, packet);
			break;
		case FBE_OBJECT_STATE_READY:
			status = sas_lcc_monitor_ready(sas_lcc, packet);
			break;
		case FBE_OBJECT_STATE_DESTROY:
			status = sas_lcc_monitor_destroy(sas_lcc, packet);
			break;
		case FBE_OBJECT_STATE_DESTROY_PENDING:
			status = sas_lcc_monitor_destroy_pending(sas_lcc, packet);
			break;

        default:
			fbe_base_object_trace(	(fbe_base_object_t*)sas_lcc,
									FBE_TRACE_LEVEL_DEBUG_HIGH,
									FBE_TRACE_MESSAGE_ID_MGMT_STATE_INVALID,
									"%X invalid state\n", mgmt_state);

			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet);
			status = FBE_STATUS_GENERIC_FAILURE;
			break;
	}

	return status;
}

fbe_status_t 
fbe_sas_lcc_create_edge(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet)
{
	fbe_base_object_mgmt_create_edge_t * base_object_mgmt_create_edge = NULL;
	fbe_packet_t * new_packet = NULL;
	fbe_base_edge_t   * edge = NULL;
	fbe_status_t status;
	fbe_semaphore_t sem;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

	fbe_payload_control_get_buffer(control_operation, &base_object_mgmt_create_edge); 

	/* Build the edge. */ 
	status = fbe_base_object_build_edge((fbe_base_object_t *) sas_lcc,
										base_object_mgmt_create_edge->parent_index,
										base_object_mgmt_create_edge->child_id,
										base_object_mgmt_create_edge->child_index);

	/* Insert the edge. */
	fbe_semaphore_init(&sem, 0, 1);

	/* Allocate packet */
	new_packet = fbe_transport_allocate_packet();
	if(new_packet == NULL) {
		KvTrace("%s fbe_transport_allocate_packet fail\n", __FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_transport_initialize_packet(new_packet);
	status = fbe_base_object_get_edge((fbe_base_object_t *) sas_lcc, 
										base_object_mgmt_create_edge->parent_index,
										&edge);

	fbe_transport_build_control_packet(new_packet, 
								FBE_BASE_OBJECT_CONTROL_CODE_INSERT_EDGE,
								edge,
								sizeof(fbe_base_edge_t),
								0,
								sas_lcc_create_edge_completion,
								&sem);

	status = fbe_base_object_send_packet((fbe_base_object_t *) sas_lcc, new_packet, base_object_mgmt_create_edge->parent_index);

	fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

	status = fbe_transport_get_status_code(new_packet);
	if(status == FBE_STATUS_OK){ 
		/* We succesfully created edge, so our state is changed. Ask scheduler to pay attention to us */
		fbe_base_object_run_request((fbe_base_object_t *) sas_lcc);
	} else {
		KvTrace("\n\n %s Faild to insert edge\n\n", __FUNCTION__);
	}

    fbe_transport_copy_packet_status(new_packet, packet);
	fbe_transport_release_packet(new_packet);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_lcc_create_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
	return FBE_STATUS_OK;
}

static fbe_status_t
sas_lcc_monitor_instantiated(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_edge_state_t edge_state;
	fbe_physical_object_level_t physical_object_level;

	/* Check our edge state */
	fbe_base_object_get_edge_state((fbe_base_object_t *) sas_lcc, 0, &edge_state);
	if(edge_state & FBE_EDGE_STATE_FLAG_NOT_EXIST){
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/* Initialize physical_object_level. ( This functionality belong to base_object_monitor */
	status = fbe_base_object_get_physical_object_level((fbe_base_object_t *) sas_lcc, &physical_object_level);
	if(physical_object_level == FBE_PHYSICAL_OBJECT_LEVEL_INVALID){
		status = fbe_base_object_init_physical_object_level((fbe_base_object_t *) sas_lcc, packet);
		return status;
	}

	/* Initialize enclosure address */
	if(sas_lcc->sas_address == FBE_SAS_ADDRESS_INVALID){
        status = sas_lcc_update_sas_element(sas_lcc, packet);
        return status;
    }

	if(sas_lcc->sas_lcc_attributes & FBE_SAS_LCC_CONFIG_PAGE_REQUIERED) {
        status = sas_lcc_get_config_page(sas_lcc, packet);
        return status;
	}

	if(sas_lcc->sas_lcc_attributes & FBE_SAS_LCC_INQUIRY_REQUIERED) {
        status = sas_lcc_send_inquiry(sas_lcc, packet);
        return status;
	}


    fbe_base_object_lock((fbe_base_object_t *)sas_lcc);

    switch(sas_lcc->lcc_type) {
        case FBE_SAS_DEVICE_TYPE_SAS_BULLET_DAE:
            fbe_base_object_set_class_id((fbe_base_object_t *) sas_lcc, FBE_CLASS_ID_SAS_BULLET_LCC);	
            break;

        default:
            fbe_base_object_unlock((fbe_base_object_t *)sas_lcc);

            fbe_base_object_trace((fbe_base_object_t *) sas_lcc, 
                                   FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_LCC_TYPE_UNKNOWN,
                                    "%X LCC type unknown\n",sas_lcc->lcc_type);

            return FBE_STATUS_GENERIC_FAILURE;	
        }

	fbe_base_object_unlock((fbe_base_object_t *)sas_lcc);


	/* We are done with instantiated state - got to specialized */
	fbe_base_object_set_mgmt_state((fbe_base_object_t *)sas_lcc, FBE_OBJECT_STATE_SPECIALIZED);


	/* Our ready state changed, ask scheduler to pay attention to us. */
	fbe_base_object_run_request((fbe_base_object_t *)sas_lcc);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_lcc_monitor_ready(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_base_edge_t   * edge = NULL; 

	status = fbe_base_object_get_edge((fbe_base_object_t *) sas_lcc, 0, &edge);

	if(edge->edge_state & FBE_EDGE_STATE_FLAG_NOT_EXIST){
		fbe_base_object_set_mgmt_state((fbe_base_object_t *) sas_lcc, FBE_OBJECT_STATE_DESTROY_PENDING); 
		/* Our state is changed. Ask scheduler to pay attention to us */
		fbe_base_object_run_request((fbe_base_object_t *) sas_lcc);
     	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);	
		fbe_transport_complete_packet(packet);	
		return FBE_STATUS_OK;
	}

	/* Look if we have events */
	sas_lcc_event_monitor(sas_lcc);

	if(sas_lcc->sas_lcc_attributes & FBE_SAS_LCC_UPDATE_PHY_TABLE_REQUIERED){
		status = sas_lcc_update_phy_table(sas_lcc, packet);
		return status;
	}
    
	sas_lcc_parent_list_monitor(sas_lcc);

	if(sas_lcc->broadcast_event != 0){
		sas_lcc_send_broadcast_event(sas_lcc);
	}

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_lcc_event_entry(fbe_object_handle_t object_handle, fbe_event_t event)
{
	fbe_sas_lcc_t * sas_lcc = NULL;
	fbe_event_t current_event;

	sas_lcc = (fbe_sas_lcc_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace(	(fbe_base_object_t*)sas_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry.\n", __FUNCTION__);

	fbe_base_object_lock((fbe_base_object_t *) sas_lcc);

	fbe_base_object_get_event((fbe_base_object_t *) sas_lcc, &current_event);
	current_event |= event;
	fbe_base_object_set_event((fbe_base_object_t *) sas_lcc, current_event);

	fbe_base_object_unlock((fbe_base_object_t *) sas_lcc);

	/* The monitor should take care of the event. Ask scheduler to pay attention to us */
	fbe_base_object_run_request((fbe_base_object_t *) sas_lcc);

	return FBE_STATUS_OK;
}

static fbe_status_t
sas_lcc_monitor_destroy_pending(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet)
{
	fbe_u32_t number_of_slots;
	fbe_object_id_t parent_id;
	fbe_edge_index_t first_expansion_port_index;
	fbe_edge_index_t first_slot_index;
	fbe_base_edge_t * parent_edge = NULL;
	fbe_u32_t i;

	/* destroy all parent edges */
	fbe_base_lcc_get_number_of_slots((fbe_base_lcc_t *)sas_lcc, &number_of_slots);
	fbe_base_lcc_get_first_slot_index((fbe_base_lcc_t *)sas_lcc, &first_slot_index);

	for(i = 0; i < number_of_slots; i++){
		fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_lcc);
		parent_edge = fbe_base_object_get_parent_by_index((fbe_base_object_t *)sas_lcc, first_slot_index + i);
		if(parent_edge != NULL){
			parent_id = parent_edge->parent_id;
			fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lcc);
			fbe_base_object_destroy_parent_edge((fbe_base_object_t *)sas_lcc, parent_id);			
		} else {
			fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lcc);
		}
	}

	fbe_base_lcc_get_first_expansion_port_index((fbe_base_lcc_t *) sas_lcc, &first_expansion_port_index);
	fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_lcc);
	parent_edge = fbe_base_object_get_parent_by_index((fbe_base_object_t *)sas_lcc, first_expansion_port_index);
	if(parent_edge != NULL){
		parent_id = parent_edge->parent_id;
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lcc);
		fbe_base_object_destroy_parent_edge((fbe_base_object_t *)sas_lcc, parent_id);			
	} else {
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lcc);
	}

	/* check termination and pending queues */
	if(!fbe_base_object_is_terminator_queue_empty((fbe_base_object_t *)sas_lcc)){
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/* switch to destroy state */
	fbe_base_object_set_mgmt_state((fbe_base_object_t *) sas_lcc, FBE_OBJECT_STATE_DESTROY); 
	/* When the ready state changes, ask scheduler to pay attention to us. */
	fbe_base_object_run_request((fbe_base_object_t *)sas_lcc);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t
sas_lcc_monitor_destroy(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet)
{
    	/* Tell scheduler that object is no longer exist */
	fbe_transport_set_status(packet, FBE_STATUS_NO_DEVICE, 0);
	fbe_transport_complete_packet(packet);

    fbe_base_object_destroy_request((fbe_base_object_t *) sas_lcc);
	return FBE_STATUS_NO_DEVICE;
}

static fbe_status_t 
sas_lcc_update_sas_element(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet)
{
	fbe_packet_t * new_packet = NULL;
	fbe_status_t status;
	fbe_semaphore_t sem;
	fbe_sas_port_mgmt_get_parent_sas_element_t parent_sas_element;
	fbe_object_id_t my_id;
	fbe_semaphore_init(&sem, 0, 1);

	new_packet = fbe_transport_allocate_packet();
	fbe_transport_initialize_packet(new_packet);

	fbe_base_object_get_object_id((fbe_base_object_t *)sas_lcc, &my_id);

	parent_sas_element.parent_id = my_id; /* It is actually my object_id */
	fbe_zero_memory(&parent_sas_element.sas_element, sizeof(fbe_sas_element_t));

	fbe_transport_build_control_packet(new_packet, 
								FBE_SAS_PORT_CONTROL_CODE_GET_PARENT_SAS_ELEMENT,
								&parent_sas_element,
								sizeof(fbe_sas_port_mgmt_get_parent_sas_element_t),
								sizeof(fbe_sas_port_mgmt_get_parent_sas_element_t),
								sas_lcc_update_sas_element_completion,
								&sem);

	status = fbe_base_object_send_packet((fbe_base_object_t *) sas_lcc, new_packet, 0); /* for now hard coded control edge */

	fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

	if(status == FBE_STATUS_OK){
        sas_lcc->cpd_device_id = parent_sas_element.sas_element.cpd_device_id;
		sas_lcc->sas_address = parent_sas_element.sas_element.sas_address;

        KvTrace("%s. sas_address = %llX, cpd_device_id :%d \n",__FUNCTION__,
		(unsigned long long)sas_lcc->sas_address, sas_lcc->cpd_device_id);
		/* The sas changed, ask scheduler to pay attention to us. */
		fbe_base_object_run_request((fbe_base_object_t *)sas_lcc);
	}

    fbe_transport_copy_packet_status(new_packet, packet);
	fbe_transport_release_packet(new_packet);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_lcc_update_sas_element_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
	return FBE_STATUS_OK;
}

/* Deprecated */
fbe_status_t fbe_sas_lcc_get_bus_id(fbe_sas_lcc_t *sas_lcc, fbe_u32_t *bus_id)
{
    *bus_id = 0;
    
    return FBE_STATUS_OK;
}

fbe_status_t fbe_sas_lcc_get_target_id(fbe_sas_lcc_t *sas_lcc, fbe_u32_t *cpd_device_id)
{
    *cpd_device_id = sas_lcc->cpd_device_id;
    
    return FBE_STATUS_OK;
}

/* Deprecated */
fbe_status_t 
fbe_sas_lcc_get_lun_id(fbe_sas_lcc_t *sas_lcc, fbe_u32_t *lun_id)
{
    *lun_id = 0;
    
    return FBE_STATUS_OK;
}

/*
 * This method is called when in the FIBRE DRIVE is in the INSTANTIATED state and has not yet send inquiry
 * The FIBRE DRIVE creates a new io packet with inquiry CDB
 * and sends it upstream.
 */
static fbe_status_t 
sas_lcc_send_inquiry(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet)
{
	fbe_packet_t * new_packet = NULL;
	fbe_u8_t * buffer = NULL;
	fbe_sg_element_t  * sg_list = NULL;
	fbe_io_block_t * io_block = NULL;
	fbe_status_t status;

	new_packet = fbe_transport_allocate_packet();
	if (new_packet == NULL){
		fbe_base_object_trace(	(fbe_base_object_t*)sas_lcc,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s, fbe_transport_allocate_packet failed\n",
								__FUNCTION__);

		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	buffer = fbe_transport_allocate_buffer();
	if (buffer == NULL){
		fbe_base_object_trace(	(fbe_base_object_t*)sas_lcc,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s, fbe_transport_allocate_buffer failed\n",
								__FUNCTION__);

		fbe_transport_release_packet(new_packet);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	sg_list = (fbe_sg_element_t  *)buffer;
	sg_list[0].count = FBE_SCSI_INQUIRY_DATA_SIZE;
	sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

	sg_list[1].count = 0;
	sg_list[1].address = NULL;

	fbe_transport_initialize_packet(new_packet);
	fbe_transport_set_completion_function(new_packet, sas_lcc_send_inquiry_completion, sas_lcc);
	
	io_block = fbe_transport_get_io_block(new_packet);
	io_block_set_sg_list(io_block, sg_list);
	io_block_build_inquiry(io_block, sas_lcc->cpd_device_id , FBE_SAS_LCC_INQUIRY_TIMEOUT);

	/* Put our subpacket on the mgmt packet's queue of subpackets. */
	/* We have to temporarely disable the subpacket queue.
	 * We will reenable it again when mgmt and io will work from the same base 	 

	
	fbe_transport_add_subpacket(packet, subpacket);
	*/
	/* Temporary hack */
	new_packet->master_packet = (void *)packet;

	/* Put our mgmt packet on the termination queue while we wait for the subpacket
	 * with the unbypass command to complete.
	 */
	fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)sas_lcc);
	fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)sas_lcc, packet);

	status = fbe_base_object_send_io_packet((fbe_base_object_t *) sas_lcc, new_packet, 0); /* for now hard coded control edge */

	return status;
}

/*
 * This is the completion function for the inquiry packet sent to a drive.
 */
static fbe_status_t 
sas_lcc_send_inquiry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_packet_t * master_packet = NULL;
	fbe_sas_lcc_t * sas_lcc = NULL;
	fbe_sg_element_t  * sg_list = NULL; 
	fbe_u8_t vendor_id[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE + 1];
	fbe_u8_t product_id[FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1];
	fbe_io_block_t * io_block;

	sas_lcc = (fbe_sas_lcc_t *)context;
	master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);

	/* We HAVE to check status of packet before we continue */

	io_block = fbe_transport_get_io_block(packet);
	sg_list = io_block_get_sg_list(io_block);

	fbe_zero_memory(vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE + 1);
	fbe_copy_memory(vendor_id, ((fbe_u8_t *)(sg_list[0].address)) + FBE_SCSI_INQUIRY_VENDOR_ID_OFFSET, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE);			

	fbe_zero_memory(product_id, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1);
	fbe_copy_memory(product_id, ((fbe_u8_t *)(sg_list[0].address)) + FBE_SCSI_INQUIRY_PRODUCT_ID_OFFSET, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE);			

	KvTrace("%s cpd_device_id = %d, vendor_id  = %s\n", __FUNCTION__, sas_lcc->cpd_device_id, vendor_id);
	KvTrace("%s cpd_device_id = %d, product_id  = %s\n", __FUNCTION__, sas_lcc->cpd_device_id, product_id);

	sas_lcc->sas_lcc_attributes &= ~FBE_SAS_LCC_INQUIRY_REQUIERED;
	sas_lcc->lcc_type = FBE_SAS_DEVICE_TYPE_SAS_BULLET_DAE;

	/* Ask scheduler to pay attention to us */
	fbe_base_object_run_request((fbe_base_object_t *)sas_lcc);

	fbe_transport_release_buffer(sg_list);
	fbe_transport_release_packet(packet);

	fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)sas_lcc, master_packet);
	fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(master_packet);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_lcc_get_cpd_device_id(fbe_sas_lcc_t * sas_lcc, fbe_cpd_device_id_t * cpd_device_id)
{
	* cpd_device_id = sas_lcc->cpd_device_id;
	return FBE_STATUS_OK;
}


static fbe_status_t 
sas_lcc_get_config_page(fbe_sas_lcc_t * sas_lcc, fbe_packet_t *packet)
{
	fbe_packet_t * new_packet = NULL;
	fbe_u8_t * buffer = NULL;
	fbe_sg_element_t  * sg_list = NULL;
	fbe_io_block_t * io_block = NULL;
	fbe_u8_t * cdb = NULL;
	fbe_status_t status;
	fbe_cpd_device_id_t cpd_device_id;
	fbe_io_block_t * in_io_block = fbe_transport_get_io_block(packet);

	fbe_base_object_trace((fbe_base_object_t*)sas_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);
    
	/* Allocate packet */
	new_packet = fbe_transport_allocate_packet();
	if (new_packet == NULL){
		fbe_base_object_trace(	(fbe_base_object_t*)sas_lcc,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s, fbe_transport_allocate_packet failed\n",
								__FUNCTION__);

		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	buffer = fbe_transport_allocate_buffer();
	if (buffer == NULL){
		fbe_base_object_trace(	(fbe_base_object_t*)sas_lcc,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s, fbe_transport_allocate_buffer failed\n",
								__FUNCTION__);

		fbe_transport_release_packet(new_packet);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_zero_memory(buffer, FBE_MEMORY_CHUNK_SIZE);

	sg_list = (fbe_sg_element_t  *)buffer;
	sg_list[0].count = FBE_MEMORY_CHUNK_SIZE;
	sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

	sg_list[1].count = 0;
	sg_list[1].address = NULL;
    
	fbe_transport_initialize_packet(new_packet);
	fbe_transport_set_completion_function(new_packet, sas_lcc_get_config_page_completion, sas_lcc);
	
	io_block = fbe_transport_get_io_block(new_packet);
	io_block_set_sg_list(io_block, sg_list);
	
	fbe_sas_lcc_get_cpd_device_id((fbe_sas_lcc_t *) sas_lcc, &cpd_device_id);

    io_block_convert_to_execute_cdb (in_io_block, cpd_device_id, io_block);

	cdb = fbe_io_block_get_cdb(io_block);

    fbe_ses_build_encl_config_page(cdb, SAS_PORT_MAX_SES_CDB_SIZE, FBE_SES_ENCLOSURE_CONFIGURATION_PAGE_SIZE);

    /* Put our new packet on the packet's queue of subpackets. */	
	fbe_transport_add_subpacket(packet, new_packet);

	/* Put our packet on the termination queue while we wait for the subpacket to complete. */
	fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *) sas_lcc);
	fbe_base_object_add_to_terminator_queue((fbe_base_object_t *) sas_lcc, packet);

	status = fbe_base_object_send_io_packet((fbe_base_object_t *) sas_lcc, new_packet, 0); /* for now hard coded control edge */

	return status;
}

static fbe_status_t
sas_lcc_get_config_page_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
	fbe_sas_lcc_t * sas_lcc = (fbe_sas_lcc_t *)context;
	fbe_packet_t * master_packet = NULL;
	fbe_sg_element_t  * sg_list = NULL; 
	fbe_io_block_t * io_block;
    
	fbe_base_object_trace(	(fbe_base_object_t*)sas_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

	master_packet = fbe_transport_get_master_packet(packet); 

	/* We HAVE to check status of packet before we continue */
	io_block = fbe_transport_get_io_block(packet);
	sg_list = io_block_get_sg_list(io_block);
    
    /* Remove the packet from the  master_packet subpacket queue. */
	fbe_transport_remove_subpacket(packet);
    
    /* Remove the master_packet from the termination queue */
	fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)sas_lcc, master_packet);	

	status = fbe_transport_get_status_code(packet);

	if(status != FBE_STATUS_OK){ /* Something bad happen to child, just release packet */
        KvTrace("%s Failed \n", __FUNCTION__);
	    fbe_transport_release_buffer(sg_list);
		fbe_transport_release_packet(packet);

        /* We HAVE to check if subpacket queue is empty */
        fbe_transport_set_status(master_packet, status, 0);
        fbe_transport_complete_packet(master_packet);
		return FBE_STATUS_OK;
	}

	sas_lcc_process_config_page(sas_lcc, sg_list[0].address);


	fbe_transport_release_buffer(sg_list);
	fbe_transport_release_packet(packet);

    /* We HAVE to check if subpacket queue is empty */
    fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet);

	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_lcc_process_config_page(fbe_sas_lcc_t * sas_lcc, fbe_u8_t *resp_buffer)
{
	fbe_ses_config_page_t * config_page = NULL;
	fbe_ses_enclosure_descriptor_t * enclosure_descriptor = NULL;
	fbe_ses_type_descriptor_t * type_descriptor_list = NULL;
	fbe_u32_t i = 0;
	fbe_u32_t descriptor_index = 0;

	config_page = (fbe_ses_config_page_t *)resp_buffer;
	enclosure_descriptor = (fbe_ses_enclosure_descriptor_t *)(resp_buffer + sizeof(fbe_ses_config_page_t));

	KvTrace("%s number_of_type_descriptor_headers = %d\n", __FUNCTION__, enclosure_descriptor->number_of_type_descriptor_headers );

	type_descriptor_list = (fbe_ses_type_descriptor_t *)(((fbe_u8_t *)enclosure_descriptor) + enclosure_descriptor->encl_descriptor_length + 4);

	descriptor_index = 0;
	for(i = 0; i < enclosure_descriptor->number_of_type_descriptor_headers; i++) {
		KvTrace("%s type_descriptor[%d] %02X %02X %02X %02X\n", __FUNCTION__,i, 
																			type_descriptor_list[i].element_type,
																			type_descriptor_list[i].number_of_possible_elements,
																			type_descriptor_list[i].subenclosure_identifier,
																			type_descriptor_list[i].type_descriptor_text_lenght);
		 
		switch(type_descriptor_list[i].element_type) {
		case FBE_SES_ELEMENT_TYPE_SAS_CONNECTOR:
				sas_lcc->sas_connector_index = descriptor_index;
				sas_lcc->sas_connector_count = type_descriptor_list[i].number_of_possible_elements;
			break;
		}
		descriptor_index += type_descriptor_list[i].number_of_possible_elements + 1; /* "+ 1" to count Overall status element */
	}

	sas_lcc->sas_lcc_attributes &= ~FBE_SAS_LCC_CONFIG_PAGE_REQUIERED;
	/* Our attribute is changed. Ask scheduler to pay attention to us */
	fbe_base_object_run_request((fbe_base_object_t *) sas_lcc);

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_lcc_get_sas_connector_descriptor(fbe_sas_lcc_t * sas_lcc, fbe_u32_t *sas_connector_index, fbe_u32_t *sas_connector_count)
{
	*sas_connector_index = sas_lcc->sas_connector_index;
	*sas_connector_count = sas_lcc->sas_connector_count;

	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_lcc_update_phy_table(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet)
{
	fbe_packet_t * new_packet = NULL;
	fbe_status_t status;
	fbe_semaphore_t sem;
	fbe_sas_port_mgmt_get_phy_table_t * mgmt_get_phy_table = NULL;
	fbe_semaphore_init(&sem, 0, 1);

	new_packet = fbe_transport_allocate_packet();
	mgmt_get_phy_table = fbe_transport_allocate_buffer();

	fbe_transport_initialize_packet(new_packet);

	mgmt_get_phy_table->sas_address = sas_lcc->sas_address; /* It is actually my object_id */
	fbe_zero_memory(&mgmt_get_phy_table->phy_table, FBE_SAS_PORT_MAX_PHY_NUMBER_PER_DEVICE * sizeof(fbe_sas_element_t));

	fbe_transport_build_control_packet(new_packet, 
								FBE_SAS_PORT_CONTROL_CODE_GET_PHY_TABLE,
								mgmt_get_phy_table,
								sizeof(fbe_sas_port_mgmt_get_phy_table_t),
								sizeof(fbe_sas_port_mgmt_get_phy_table_t),
								sas_lcc_update_phy_table_completion,
								&sem);

	/* Mark packet for traversal. */
	fbe_transport_set_packet_type(new_packet, FBE_PACKET_FLAG_TRAVERSE);

	status = fbe_base_object_send_packet((fbe_base_object_t *) sas_lcc, new_packet, 0); /* for now hard coded control edge */

	fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

	if(status == FBE_STATUS_OK){
		sas_lcc_process_phy_table(sas_lcc, mgmt_get_phy_table->phy_table);

		sas_lcc->sas_lcc_attributes &= ~FBE_SAS_LCC_UPDATE_PHY_TABLE_REQUIERED;
		/* Our attribute is changed. Ask scheduler to pay attention to us */
		fbe_base_object_run_request((fbe_base_object_t *) sas_lcc);
	}

    fbe_transport_copy_packet_status(new_packet, packet);
	fbe_transport_release_buffer(mgmt_get_phy_table);
	fbe_transport_release_packet(new_packet);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_lcc_update_phy_table_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_lcc_process_phy_table(fbe_sas_lcc_t * sas_lcc, fbe_sas_element_t * phy_table)
{
	fbe_u32_t phy_num;

	for(phy_num = 0; phy_num < FBE_SAS_PORT_MAX_PHY_NUMBER_PER_DEVICE; phy_num++){
		if(sas_lcc->phy_table[phy_num].present == 0){
			if(phy_table[phy_num].present == 1){ 
				sas_lcc->phy_table[phy_num].sas_address = phy_table[phy_num].sas_address;
				sas_lcc->phy_table[phy_num].element_type = phy_table[phy_num].element_type;
				sas_lcc->phy_table[phy_num].present = phy_table[phy_num].present;
				sas_lcc->phy_table[phy_num].change_count = phy_table[phy_num].change_count;
				sas_lcc->phy_table[phy_num].swap = 1;
				sas_lcc->phy_table[phy_num].cpd_device_id = phy_table[phy_num].cpd_device_id;
				/* Our attribute is changed. Ask scheduler to pay attention to us */
				fbe_base_object_run_request((fbe_base_object_t *) sas_lcc);
			}
		} else { 
			if(phy_table[phy_num].present == 1){ 
				/* Check if it is the same device */
				if(sas_lcc->phy_table[phy_num].sas_address != phy_table[phy_num].sas_address) { /* It is not the same device */
					sas_lcc->phy_table[phy_num].sas_address = phy_table[phy_num].sas_address;
					sas_lcc->phy_table[phy_num].element_type = phy_table[phy_num].element_type;
					sas_lcc->phy_table[phy_num].present = phy_table[phy_num].present;
					sas_lcc->phy_table[phy_num].change_count = phy_table[phy_num].change_count;
					sas_lcc->phy_table[phy_num].swap = 1; /* device was swapped */
					sas_lcc->phy_table[phy_num].cpd_device_id = phy_table[phy_num].cpd_device_id;
					/* Our attribute is changed. Ask scheduler to pay attention to us */
					fbe_base_object_run_request((fbe_base_object_t *) sas_lcc);
				}
			} else { 
				sas_lcc->phy_table[phy_num].sas_address = 0;
				sas_lcc->phy_table[phy_num].element_type = FBE_SAS_DEVICE_TYPE_UKNOWN;
				sas_lcc->phy_table[phy_num].present = 0;
				sas_lcc->phy_table[phy_num].change_count = 0;
				sas_lcc->phy_table[phy_num].swap = 0;
				sas_lcc->phy_table[phy_num].cpd_device_id = 0;
				/* Our attribute is changed. Ask scheduler to pay attention to us */
				fbe_base_object_run_request((fbe_base_object_t *) sas_lcc);
			}	
		}
	}
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_lcc_parent_list_monitor(fbe_sas_lcc_t * sas_lcc)
{
	sas_lcc_expansion_port_monitor(sas_lcc);
	sas_lcc_slot_monitor(sas_lcc);

	return FBE_STATUS_OK;
}

static fbe_status_t
sas_lcc_get_parent_sas_element(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet)
{
	fbe_sas_port_mgmt_get_parent_sas_element_t * get_parent_sas_element = NULL;
	fbe_base_edge_t * parent_edge = NULL;
	fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
        
	fbe_base_object_trace(	(fbe_base_object_t*)sas_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s function entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

	fbe_payload_control_get_buffer_length(control_operation, &len); 

	if(len != sizeof(fbe_sas_port_mgmt_get_parent_sas_element_t)){
		KvTrace("%s Invalid len %X\n", __FUNCTION__, len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_payload_control_get_buffer(control_operation, &get_parent_sas_element); 
	if(get_parent_sas_element == NULL){
		KvTrace("%s fbe_transport_get_buffer failed \n", __FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Since we are dealing with the parent_list we have to grub the lock */
	fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_lcc);
    parent_edge = fbe_base_object_get_parent_by_id((fbe_base_object_t *) sas_lcc, get_parent_sas_element->parent_id);
	if(parent_edge == NULL){
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lcc);

		fbe_base_object_trace((fbe_base_object_t *) sas_lcc, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_base_object_get_parent_by_id fail\n");

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_copy_memory(&get_parent_sas_element->sas_element, &sas_lcc->phy_table[parent_edge->child_index], sizeof(fbe_sas_element_t));

	fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lcc);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t
sas_lcc_event_monitor(fbe_sas_lcc_t * sas_lcc)
{
	fbe_event_t current_event;

	fbe_base_object_lock((fbe_base_object_t *) sas_lcc);
	fbe_base_object_get_event((fbe_base_object_t *) sas_lcc, &current_event);
	
	if(current_event & FBE_EVENT_FLAG_DISCOVERY_COMPLETE){
		sas_lcc->broadcast_event |= FBE_EVENT_FLAG_DISCOVERY_COMPLETE;
		sas_lcc->sas_lcc_attributes |= FBE_SAS_LCC_UPDATE_PHY_TABLE_REQUIERED;
		current_event &= ~FBE_EVENT_FLAG_DISCOVERY_COMPLETE;
		/* Ask scheduler to pay attention to us */
		fbe_base_object_run_request((fbe_base_object_t *) sas_lcc);
	}

	fbe_base_object_set_event((fbe_base_object_t *) sas_lcc, current_event);
	fbe_base_object_unlock((fbe_base_object_t *) sas_lcc);

	return FBE_STATUS_OK;
}

static fbe_status_t
sas_lcc_send_broadcast_event(fbe_sas_lcc_t * sas_lcc)
{
	fbe_u32_t number_of_slots;
	fbe_edge_index_t first_expansion_port_index;
	fbe_edge_index_t first_slot_index;
	fbe_base_edge_t * parent_edge = NULL;
	fbe_u32_t i;

	fbe_base_lcc_get_number_of_slots((fbe_base_lcc_t *)sas_lcc, &number_of_slots);
	fbe_base_lcc_get_first_slot_index((fbe_base_lcc_t *)sas_lcc, &first_slot_index);

	for(i = 0; i < number_of_slots; i++){
		fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_lcc);
		parent_edge = fbe_base_object_get_parent_by_index((fbe_base_object_t *)sas_lcc, first_slot_index + i);
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lcc);

		if(parent_edge != NULL){
			fbe_base_object_send_event((fbe_base_object_t *) sas_lcc, first_slot_index + i, sas_lcc->broadcast_event);
		} 
	}

	fbe_base_lcc_get_first_expansion_port_index((fbe_base_lcc_t *) sas_lcc, &first_expansion_port_index);
	fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_lcc);
	parent_edge = fbe_base_object_get_parent_by_index((fbe_base_object_t *)sas_lcc, first_expansion_port_index);
	fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lcc);
	if(parent_edge != NULL){
		fbe_base_object_send_event((fbe_base_object_t *) sas_lcc, first_expansion_port_index, sas_lcc->broadcast_event);
	} 

	sas_lcc->broadcast_event = 0;
	return FBE_STATUS_OK;
}


/*!
 * \ingroup FBE_SAS_LCC_INTERNAL_FUNCTION
 *
 * \fn static fbe_status_t sas_lcc_slot_monitor(fbe_sas_lcc_t * sas_lcc, fbe_packet_t * packet)
 *
 * This is a monitor function called out of the READY execution flow.  It correlates the existence of edges
 * with the bits in the LCC's shadow registers to decide if there have been any drive slot state changes.
 *
 * \param sas_lcc
 * 		A pointer to the sas_lcc object.
 *
 * \param packet
 * 		A pointer to the management packet used to drive the LCC monitor.
 *
 * \return
 * 		FBE_STATUS_OK if no slot operations are needed, FBE_STATUS_PENDING if a slot operation
 * 		is pending, otherwise an error status code is returned.
 */
static fbe_status_t 
sas_lcc_slot_monitor(fbe_sas_lcc_t * sas_lcc)
{
	fbe_status_t status;
	fbe_u32_t number_of_slots;
	fbe_u32_t i;
	fbe_edge_index_t first_slot_index;
	fbe_base_edge_t * parent_edge = NULL;

	fbe_base_lcc_get_number_of_slots((fbe_base_lcc_t *)sas_lcc, &number_of_slots);
	fbe_base_lcc_get_first_slot_index((fbe_base_lcc_t *) sas_lcc, &first_slot_index);

	/* This function called in monitor context, so we do not need to obtain the lock */
	for (i = 0; i < number_of_slots; i++) {
		/* Correlate the existence of edges with the bits in the presence register from the last diplex READ. */
		fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_lcc);
		parent_edge = fbe_base_object_get_parent_by_index((fbe_base_object_t *)sas_lcc, first_slot_index + i);
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lcc);

		/* Does this slot have a parent edge? */
		if (parent_edge != NULL) {
			/* We have an edge so we should also have a present flag. */
			if (sas_lcc->phy_table[first_slot_index + i].present == 0) {
				/* The drive is NOT present, but we have a parent edge.  We need to destroy the edge, this tells
				 * the parent object that it must destroy itself. */
				fbe_base_object_destroy_parent_edge((fbe_base_object_t *)sas_lcc, parent_edge->parent_id);
			}
		}
		else {
			/* We do not have an edge so we should not find a corresponding present flag. */
			if (sas_lcc->phy_table[first_slot_index + i].present != 0) {
				/* The drive IS present, so we need to create a corresponding drive object for it. */
				if(sas_lcc->phy_table[first_slot_index + i].element_type == FBE_SAS_DEVICE_TYPE_SATA_DRIVE){
					sas_lcc->phy_table[first_slot_index + i].swap = 0;
					status = fbe_base_object_instantiate_parent((fbe_base_object_t *)sas_lcc, FBE_CLASS_ID_SATA_DRIVE, (first_slot_index + i));
				}

				if(sas_lcc->phy_table[first_slot_index + i].element_type == FBE_SAS_DEVICE_TYPE_SAS_DRIVE){
					sas_lcc->phy_table[first_slot_index + i].swap = 0;
					status = fbe_base_object_instantiate_parent((fbe_base_object_t *)sas_lcc, FBE_CLASS_ID_SAS_DRIVE, (first_slot_index + i));
				}

			}
		}
	}
	return status;
}


/*!
 * \ingroup FBE_SAS_LCC_INTERNAL_FUNCTION
 *
 * \fn static fbe_status_t sas_lcc_expansion_port_monitor(fbe_sas_lcc_t * sas_lcc)
 *
 * This is a monitor function called out of the READY execution flow.  It correlates the existence of an
 * expansion port edge with the expansion port hardware state.i  If the hardware indicates there is hardware
 * on the expansion port but there is no edge, a parent LCC is created with its corresponding edge.  Conversely
 * if there is an expansion port edge but the hardware indicates the expansion port is unconnected, the parent LCC
 * is destroyed.
 *
 * \param sas_lcc
 * 		A pointer to the Fibre LCC object.
 *
 * \return
 * 		Always returns FBE_STATUS_OK.
 */
static fbe_status_t 
sas_lcc_expansion_port_monitor(fbe_sas_lcc_t * sas_lcc)
{
	fbe_status_t status;
	fbe_edge_index_t first_expansion_port_index;
	fbe_base_edge_t * parent_edge = NULL;
	fbe_object_id_t parent_id;

	fbe_base_lcc_get_first_expansion_port_index((fbe_base_lcc_t *) sas_lcc, &first_expansion_port_index);

	fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_lcc);
	parent_edge = fbe_base_object_get_parent_by_index((fbe_base_object_t *)sas_lcc, first_expansion_port_index);

	if(parent_edge == NULL){ /* This expansion_port have no parent */
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lcc);
		if (sas_lcc->phy_table[first_expansion_port_index].present != 0) {
			/* There is some new hardware on the expansion_port */
			sas_lcc->phy_table[first_expansion_port_index].swap = 0;
			status = fbe_base_object_instantiate_parent((fbe_base_object_t *) sas_lcc, FBE_CLASS_ID_SAS_LCC, first_expansion_port_index);
		}
	} else { 
		/* We have parent , but the expander is gone - we have to destroy the edge */
		parent_id = parent_edge->parent_id;
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lcc);
		if(sas_lcc->phy_table[first_expansion_port_index].present == 0){ /* There is no hardware in the expansion_port */
			fbe_base_object_destroy_parent_edge((fbe_base_object_t *)sas_lcc, parent_id);	
		}
	}

	return FBE_STATUS_OK;
}

