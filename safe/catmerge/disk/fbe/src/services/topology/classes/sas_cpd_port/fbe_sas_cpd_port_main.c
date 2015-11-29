#include "ktrace.h"

#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_port.h"

#include "fbe_base_object.h"
#include "fbe_transport_memory.h"
#include "sas_cpd_port_private.h"
#include "fbe_cpd_shim.h"
#include "fbe_base_board.h"
#include "fbe_sas_port.h"

/* Class methods forward declaration */
fbe_status_t fbe_sas_cpd_port_load(void);
fbe_status_t fbe_sas_cpd_port_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_sas_cpd_port_class_methods = {FBE_CLASS_ID_SAS_CPD_PORT,
							        			      fbe_sas_cpd_port_load,
												      fbe_sas_cpd_port_unload,
												      fbe_sas_cpd_port_create_object,
												      fbe_sas_cpd_port_destroy_object,
												      fbe_sas_cpd_port_control_entry,
												      fbe_sas_cpd_port_event_entry,
                                                      fbe_sas_cpd_port_io_entry,
													  fbe_sas_cpd_port_monitor_entry};


/* Forward declaration */
static fbe_status_t fbe_sas_cpd_port_init(fbe_sas_cpd_port_t * sas_cpd_port);

/* static fbe_status_t fbe_sas_cpd_port_create_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context); */
static fbe_status_t fbe_sas_cpd_port_get_parent_address(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet);

static fbe_status_t sas_port_cpd_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context);

static fbe_status_t sas_cpd_port_parent_list_monitor(fbe_sas_cpd_port_t * sas_cpd_port);
static fbe_status_t sas_cpd_port_get_parent_sas_element(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet);
/* static fbe_status_t sas_cpd_port_get_phy_table(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet); */

/* Monitor functions */
/*
static fbe_status_t fbe_sas_cpd_port_monitor_state_specialized(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_cpd_port_monitor_state_ready(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_cpd_port_monitor_destroy_pending(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_cpd_port_monitor_destroy(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet);
*/

fbe_status_t fbe_sas_cpd_port_load(void)
{
	KvTrace("%s entry\n", __FUNCTION__);
	FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_sas_cpd_port_t) < FBE_MEMORY_CHUNK_SIZE);

	fbe_cpd_shim_init();
	return fbe_sas_cpd_port_monitor_load_verify();
}

fbe_status_t fbe_sas_cpd_port_unload(void)
{
	KvTrace("%s entry\n", __FUNCTION__);
	fbe_cpd_shim_destroy();
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_cpd_port_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
	fbe_sas_cpd_port_t * sas_cpd_port;
	fbe_status_t status;
	

	KvTrace("%s entry\n", __FUNCTION__);

	/* Call parent constructor */
	status = fbe_sas_port_create_object(packet, object_handle);
	if(status != FBE_STATUS_OK){
		return status;
	}

	/* Set class id */
	sas_cpd_port = (fbe_sas_cpd_port_t *)fbe_base_handle_to_pointer(*object_handle);
	fbe_base_object_set_class_id((fbe_base_object_t *) sas_cpd_port, FBE_CLASS_ID_SAS_CPD_PORT);	

	fbe_sas_cpd_port_init(sas_cpd_port);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_cpd_port_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_sas_cpd_port_t *sas_cpd_port;
	fbe_port_number_t port_number;

	KvTrace("%s entry\n", __FUNCTION__);
    sas_cpd_port = (fbe_sas_cpd_port_t *)fbe_base_handle_to_pointer(object_handle);

	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_cpd_port, &port_number);

	/* unregister io_completion */
	fbe_cpd_shim_port_unregister_io_completion(port_number);

	/* destroy cpd port */
	fbe_cpd_shim_port_destroy(port_number);

	if(sas_cpd_port->sas_table != NULL){
		fbe_memory_native_release(sas_cpd_port->sas_table);
	}

	sas_cpd_port->sas_table = NULL;
	/* Call parent destructor */
	status = fbe_sas_port_destroy_object(object_handle);
	return status;
}

#if 0
fbe_status_t 
fbe_sas_cpd_port_create_edge(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet)
{
	fbe_base_object_mgmt_create_edge_t * base_object_mgmt_create_edge;
	fbe_packet_t * new_packet = NULL;
	fbe_base_edge_t   * edge = NULL;
	fbe_status_t status;
	fbe_semaphore_t sem;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_cpd_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    
	fbe_payload_control_get_buffer(control_operation, &base_object_mgmt_create_edge); 

	/* Build the edge. */ 
	status = fbe_base_object_build_edge((fbe_base_object_t *) sas_cpd_port,
										base_object_mgmt_create_edge->parent_index,
										base_object_mgmt_create_edge->child_id,
										base_object_mgmt_create_edge->child_index);

	/* Insert the edge. */
	fbe_semaphore_init(&sem, 0, 1);
	/* Allocate packet */
	new_packet = fbe_transport_allocate_packet();
	if(new_packet == NULL) {
		KvTrace("fbe_sas_lsi_lcc_main: %s fbe_transport_allocate_packet fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
    	fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_transport_initialize_packet(new_packet);
	status = fbe_base_object_get_edge((fbe_base_object_t *) sas_cpd_port, 
										base_object_mgmt_create_edge->parent_index,
										&edge);

	fbe_transport_build_control_packet(new_packet, 
								FBE_BASE_OBJECT_CONTROL_CODE_INSERT_EDGE,
								edge,
								sizeof(fbe_base_edge_t),
								0,
								fbe_sas_cpd_port_create_edge_completion,
								&sem);

	status = fbe_base_object_send_packet((fbe_base_object_t *) sas_cpd_port, new_packet, base_object_mgmt_create_edge->parent_index);

	fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

	status = fbe_transport_get_status_code(new_packet);
	if(status == FBE_STATUS_OK){ 
		/* We succesfully created edge, so our state is changed. Ask scheduler to pay attention to us */
		fbe_base_object_run_request((fbe_base_object_t *) sas_cpd_port);
	} else {
		KvTrace("\n\n fbe_sas_lsi_lcc_main: %s Faild to insert edge\n\n", __FUNCTION__);
	}

    fbe_transport_copy_packet_status(new_packet, packet);
	fbe_transport_release_packet(new_packet);
	/* We have to move this code to the monitor */
	fbe_base_port_set_port_number((fbe_base_port_t *) sas_cpd_port, base_object_mgmt_create_edge->child_index);
	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_sas_cpd_port_create_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t * )context;

	/*KvTrace("fbe_sas_lsi_lcc_main: %s entry\n", __FUNCTION__);*/

    fbe_semaphore_release(sem, 0, 1, FALSE);

	return FBE_STATUS_OK;
}
#endif



#if 0
static fbe_status_t
sas_cpd_port_get_parent_sas_element(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet)
{
	fbe_sas_port_mgmt_get_parent_sas_element_t * get_parent_sas_element = NULL;
	fbe_base_edge_t * parent_edge = NULL;
	fbe_payload_control_buffer_length_t in_len = 0;
	fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
        
	fbe_base_object_trace(	(fbe_base_object_t*)sas_cpd_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s function entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

	fbe_payload_control_get_buffer_length(control_operation, &in_len); 
	fbe_payload_control_get_buffer_length(control_operation, &out_len); 

	if(in_len != sizeof(fbe_sas_port_mgmt_get_parent_sas_element_t)){
		KvTrace("%s Invalid in_len %X\n", __FUNCTION__, in_len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	if(out_len != sizeof(fbe_sas_port_mgmt_get_parent_sas_element_t)){
		KvTrace("%s Invalid out_len %X\n", __FUNCTION__, out_len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer(control_operation, &get_parent_sas_element); 
	if(get_parent_sas_element == NULL){
		KvTrace("%s Ifbe_transport_get_buffer failed \n", __FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Since we are dealing with the parent_list we have to grub the lock */
	fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_cpd_port);
    parent_edge = fbe_base_object_get_parent_by_id((fbe_base_object_t *) sas_cpd_port, get_parent_sas_element->parent_id);
	if(parent_edge == NULL){
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_cpd_port);

		fbe_base_object_trace((fbe_base_object_t *) sas_cpd_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_base_object_get_parent_by_id fail\n");

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_copy_memory(&get_parent_sas_element->sas_element, &sas_cpd_port->phy_table[parent_edge->child_index], sizeof(fbe_sas_element_t));

	fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_cpd_port);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}
#endif

#if 0
static fbe_status_t 
fbe_sas_cpd_port_monitor_destroy_pending(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet)
{
	fbe_base_edge_t * parent_edge = NULL; 
	fbe_object_id_t parent_id;
	fbe_port_number_t port_number;
	fbe_status_t status;

	/* The first thing to do is to destroy all parent edges */
	/* It is make sense to support this functionality in base_object, but for now it is OK to do it here */
	fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_cpd_port);
	parent_edge = fbe_base_object_get_parent_by_index((fbe_base_object_t *)sas_cpd_port, 0);
	if(parent_edge != NULL){
		parent_id = parent_edge->parent_id;
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_cpd_port);
		fbe_base_object_destroy_parent_edge((fbe_base_object_t *)sas_cpd_port, parent_id);			
	} else{
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_cpd_port);
	}

	/* check termination and pending queues */
	if(!fbe_base_object_is_terminator_queue_empty((fbe_base_object_t *)sas_cpd_port)){
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/* We have to unregister CPD callback */
	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_cpd_port, &port_number);
	status = fbe_cpd_shim_port_unregister_callback(port_number);

	/* switch to destroy state */
	fbe_base_object_set_mgmt_state((fbe_base_object_t *) sas_cpd_port, FBE_LIFECYCLE_STATE_DESTROY); 

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}
#endif

static fbe_status_t 
fbe_sas_cpd_port_init(fbe_sas_cpd_port_t * sas_cpd_port)
{
	sas_cpd_port->sas_table = NULL;
	sas_cpd_port->sas_table = (sas_cps_port_sas_table_element_t *)fbe_memory_native_allocate(FBE_SAS_CPD_PORT_MAX_PHY_NUMBER * sizeof(sas_cps_port_sas_table_element_t));

	if(sas_cpd_port->sas_table != NULL){
		/* Init topology table */
		fbe_zero_memory(sas_cpd_port->sas_table, FBE_SAS_CPD_PORT_MAX_PHY_NUMBER * sizeof(sas_cps_port_sas_table_element_t));
	}
	sas_cpd_port->generation_code = 0;
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_port_cpd_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
	fbe_status_t status;
	fbe_sas_cpd_port_t * sas_cpd_port = NULL;
	fbe_port_number_t port_number;

	sas_cpd_port = (fbe_sas_cpd_port_t *)context;

	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_cpd_port, &port_number);
    /* examine the CPD_EVENT_INFO to determine if the event is relavant */
    switch(callback_info->callback_type)
    {
    case FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN:
		KvTrace("%s CPD_EVENT_LOGIN port %X, cpd_device_id = %X \n", __FUNCTION__, port_number, callback_info->info.login.cpd_device_id);
		break;
    case FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN_FAILED: /* for backend recovery */
		KvTrace("%s CPD_EVENT_LOGIN_FAILED port %X, cpd_device_id = %X \n", __FUNCTION__,port_number, callback_info->info.login.cpd_device_id);
		break;
    case FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT:
    case FBE_CPD_SHIM_CALLBACK_TYPE_DEVICE_FAILED:
          /*  MirrorAsyncEventString(pInfo) it is nice translation from pInfo->type to string */
		KvTrace("%s port %X,  pInfo->type = %X  \n", __FUNCTION__, port_number, callback_info->callback_type);
        break;

	case FBE_CPD_SHIM_CALLBACK_TYPE_DISCOVERY:
		switch(callback_info->info.discovery.discovery_event_type) {
			case FBE_CPD_SHIM_DISCOVERY_EVENT_TYPE_START:
				KvTrace("%s port %X CPD_CM_LOOP_NEEDS_DISCOVERY CPD_CM_LOOP_xxx_CHANGE\n", __FUNCTION__, port_number);
				break;
			case FBE_CPD_SHIM_DISCOVERY_EVENT_TYPE_COMPLETE:
				KvTrace("%s port %X  CPD_CM_LOOP_NEEDS_DISCOVERY CPD_CM_LOOP_DISCOVERY_COMPLETE\n", __FUNCTION__, port_number);
				break;
		}
		break;
    default: /* ignore the rest */
		KvTrace("%s port %X,  pInfo->type = %X  \n", __FUNCTION__, port_number, callback_info->callback_type);
        break;
    } /* end of switch(Info->Type) */

	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_cpd_port_parent_list_monitor(fbe_sas_cpd_port_t * sas_cpd_port)
{
#if 0
	fbe_u32_t parents_number;
	fbe_object_id_t parent_id;
	fbe_status_t status;
	fbe_base_edge_t * parent_edge = NULL; 

	/* Check the parents number */
	status = fbe_base_object_get_parents_number((fbe_base_object_t *) sas_cpd_port, &parents_number);

	/* Simple straight forward and needs to be modifyed */
	/* For now we are going to deal with "discovery" edges only
	   I think ssp/scsi/ edges should be organised in separate list
	*/
	if(( sas_cpd_port->phy_table[0].present == 1 ) && (parents_number == 0)) {
		status = fbe_base_object_instantiate_parent((fbe_base_object_t *) sas_cpd_port, FBE_CLASS_ID_SAS_LCC, 0);
		sas_cpd_port->phy_table[0].swap = 0;
	}

	if( (sas_cpd_port->phy_table[0].present == 0) && (parents_number > 0)){
		fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_cpd_port);
		parent_edge = fbe_base_object_get_parent_by_index((fbe_base_object_t *)sas_cpd_port, 0);
		if(parent_edge != NULL){
			parent_id = parent_edge->parent_id;
			fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_cpd_port);
			fbe_base_object_destroy_parent_edge((fbe_base_object_t *)sas_cpd_port, parent_id);			
		} else{
			fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_cpd_port);
		}
	}
#endif
	return FBE_STATUS_OK;
}

#if 0
static fbe_status_t
sas_cpd_port_get_phy_table(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet)
{
	fbe_sas_port_mgmt_get_phy_table_t * mgmt_get_phy_table = NULL;
	fbe_status_t status;
	fbe_payload_control_buffer_length_t len = 0;
	fbe_port_number_t port_number;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
        
	fbe_base_object_trace(	(fbe_base_object_t*)sas_cpd_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s function entry\n", __FUNCTION__);

	fbe_payload_control_get_buffer_length(control_operation, &len); 

	if(len != sizeof(fbe_sas_port_mgmt_get_phy_table_t)){
		KvTrace("%s Invalid in_len %X\n", __FUNCTION__, len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer(control_operation, &mgmt_get_phy_table); 
	if(mgmt_get_phy_table == NULL){
		KvTrace("%s fbe_transport_get_buffer failed \n", __FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_cpd_port, &port_number);

	/* Get phy table */
	status = fbe_cpd_shim_get_phy_table(port_number, 
										mgmt_get_phy_table->sas_address,
										mgmt_get_phy_table->phy_table,
										FBE_SAS_PORT_MAX_PHY_NUMBER_PER_DEVICE);


	fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}
#endif

#if 0
fbe_status_t
fbe_sas_cpd_port_process_phy_table(fbe_sas_cpd_port_t * sas_cpd_port, fbe_sas_element_t * phy_table)
{
	fbe_status_t status;
	fbe_bool_t discovery_update_requiered = FALSE;

	if(sas_cpd_port->phy_table[0].present == 0){ /* We where not connected to expander */
		if(phy_table->present == 1){ /* We are connected to expander now */
			sas_cpd_port->phy_table[0].sas_address = phy_table->sas_address;
			sas_cpd_port->phy_table[0].element_type = phy_table->element_type;
			sas_cpd_port->phy_table[0].present = phy_table->present;
			sas_cpd_port->phy_table[0].change_count = phy_table->change_count;
			sas_cpd_port->phy_table[0].swap = 1;
			sas_cpd_port->phy_table[0].cpd_device_id = phy_table->cpd_device_id;

			discovery_update_requiered = TRUE;
		}
	} else { /* We where connected to expander */
		if(phy_table->present == 1){ /* And we are connected to expander now */
			/* Check if it is the same expander */
			if(sas_cpd_port->phy_table[0].sas_address != phy_table->sas_address) { /* It is not the same expander */
				sas_cpd_port->phy_table[0].sas_address = phy_table->sas_address;
				sas_cpd_port->phy_table[0].element_type = phy_table->element_type;
				sas_cpd_port->phy_table[0].present = phy_table->present;
				sas_cpd_port->phy_table[0].change_count = phy_table->change_count;
				sas_cpd_port->phy_table[0].swap = 1; /* Expander was swapped */
				sas_cpd_port->phy_table[0].cpd_device_id = phy_table->cpd_device_id;

				discovery_update_requiered = TRUE;
			}
		} else { /* But we are not connected to expander now */
			sas_cpd_port->phy_table[0].sas_address = 0;
			sas_cpd_port->phy_table[0].element_type = FBE_SAS_DEVICE_TYPE_UKNOWN;
			sas_cpd_port->phy_table[0].present = 0;
			sas_cpd_port->phy_table[0].change_count = 0;
			sas_cpd_port->phy_table[0].swap = 0;
			sas_cpd_port->phy_table[0].cpd_device_id = 0;

			discovery_update_requiered = TRUE;
		}
	}

	if(discovery_update_requiered == TRUE){
		status = fbe_lifecycle_set_cond(&fbe_sas_cpd_port_lifecycle_const, 
							(fbe_base_object_t*)sas_cpd_port, 
							FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);

		if (status != FBE_STATUS_OK) {
			fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s can't set discovery update condition, status: 0x%X",
									__FUNCTION__, status);
		}
	}

	return FBE_STATUS_OK;
}
#endif


fbe_status_t
fbe_sas_cpd_port_update_sas_table(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet)
{
	fbe_sas_element_t * phy_table = NULL;
	fbe_status_t status;
	fbe_port_number_t port_number;
	fbe_u32_t number_of_phys;
	fbe_u32_t i;

	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_cpd_port, &port_number);

	status = fbe_cpd_shim_get_phy_table(port_number, &phy_table, &number_of_phys);

	/* Lock discovery table */
	
	/* For now we should add entries to the discovery table */
	for(i = 0 ; (i < number_of_phys) && (i < FBE_SAS_CPD_PORT_MAX_PHY_NUMBER); i++){
		sas_cpd_port->sas_table[i].element_type = phy_table[i].element_type;
		sas_cpd_port->sas_table[i].sas_address = phy_table[i].sas_address;
		sas_cpd_port->sas_table[i].cpd_device_id = phy_table[i].cpd_device_id;
		sas_cpd_port->sas_table[i].generation_code = fbe_atomic_increment(&sas_cpd_port->generation_code);
		sas_cpd_port->sas_table[i].phy_number = phy_table[i].phy_number;
		sas_cpd_port->sas_table[i].enclosure_chain_depth = phy_table[i].enclosure_chain_depth;

		sas_cpd_port->sas_table[i].parent_sas_address = phy_table[i].parent_sas_address;
		sas_cpd_port->sas_table[i].parent_cpd_device_id = phy_table[i].parent_cpd_device_id;
	}

	/* Unlock discovery table */
	
	/* Set discovery update condition */
	status = fbe_lifecycle_set_cond(&fbe_sas_cpd_port_lifecycle_const, 
						(fbe_base_object_t*)sas_cpd_port, 
						FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);

	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't set discovery update condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_STATUS_OK;
}


