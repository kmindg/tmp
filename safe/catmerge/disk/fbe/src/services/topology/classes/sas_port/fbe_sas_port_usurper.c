#include "fbe_sas_port.h"
#include "sas_port_private.h"



/* Forward declaration */
static fbe_status_t sas_port_attach_ssp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet);
static fbe_status_t sas_port_detach_ssp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet);
static fbe_status_t sas_port_open_ssp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet);

static fbe_status_t sas_port_attach_smp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet);
static fbe_status_t sas_port_detach_smp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet);
static fbe_status_t sas_port_open_smp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet);

static fbe_status_t sas_port_attach_stp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet);
static fbe_status_t sas_port_detach_stp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet);
static fbe_status_t sas_port_open_stp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet);

fbe_status_t 
fbe_sas_port_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_port_t * sas_port = NULL;
	fbe_payload_control_operation_opcode_t control_code;

	sas_port = (fbe_sas_port_t *)fbe_base_handle_to_pointer(object_handle);
	fbe_base_object_trace(	(fbe_base_object_t*)sas_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

	control_code = fbe_transport_get_control_code(packet);
	switch(control_code) {
		case FBE_SSP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
			status = sas_port_attach_ssp_edge(sas_port, packet);
			break;
		case FBE_SSP_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
			status = sas_port_detach_ssp_edge(sas_port, packet);
			break;
		case FBE_SSP_TRANSPORT_CONTROL_CODE_OPEN_EDGE:
			status = sas_port_open_ssp_edge(sas_port, packet);
			break;

		case FBE_SMP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
			status = sas_port_attach_smp_edge(sas_port, packet);
			break;
		case FBE_SMP_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
			status = sas_port_detach_smp_edge(sas_port, packet);
			break;
		case FBE_SMP_TRANSPORT_CONTROL_CODE_OPEN_EDGE:
			status = sas_port_open_smp_edge(sas_port, packet);
			break;

		case FBE_STP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
			status = sas_port_attach_stp_edge(sas_port, packet);
			break;
		case FBE_STP_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
			status = sas_port_detach_stp_edge(sas_port, packet);
			break;
		case FBE_STP_TRANSPORT_CONTROL_CODE_OPEN_EDGE:
			status = sas_port_open_stp_edge(sas_port, packet);
			break;

        default:
			status = fbe_base_port_control_entry(object_handle, packet);
			break;
	}

	return status;
}

static fbe_status_t 
sas_port_attach_ssp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet)
{
	fbe_ssp_transport_control_attach_edge_t * ssp_transport_control_attach_edge = NULL;
	fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &ssp_transport_control_attach_edge); 
	if(ssp_transport_control_attach_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &len);
	if(len != sizeof(fbe_ssp_transport_control_attach_edge_t)){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
								"%X len invalid\n", len);

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
	    return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Right now we do not know all the path state and path attributes rules,
	   so we will do something straight forward and may be not exactly correct.
	 */

	fbe_ssp_transport_server_lock(&sas_port->ssp_transport_server);

	fbe_ssp_transport_server_attach_edge(&sas_port->ssp_transport_server, 
										ssp_transport_control_attach_edge->ssp_edge,
										&fbe_sas_port_lifecycle_const,
										(fbe_base_object_t *) sas_port);

	ssp_transport_control_attach_edge->ssp_edge->io_entry = sas_port->ssp_transport_server.base_transport_server.io_entry;
	ssp_transport_control_attach_edge->ssp_edge->object_handle = (fbe_object_handle_t)sas_port;

	fbe_ssp_transport_server_unlock(&sas_port->ssp_transport_server);


    fbe_ssp_transport_set_server_index(ssp_transport_control_attach_edge->ssp_edge, FBE_EDGE_INDEX_INVALID);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_port_detach_ssp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet)
{
	fbe_ssp_transport_control_detach_edge_t * ssp_transport_control_detach_edge = NULL;
	fbe_payload_control_buffer_length_t len = 0;
	fbe_u32_t number_of_clients;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry \n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &ssp_transport_control_detach_edge); 
	if(ssp_transport_control_detach_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &len);
	if(len != sizeof(fbe_ssp_transport_control_detach_edge_t)){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
								"%X len invalid\n", len);

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
	    return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Right now we do not know all the path state and path attributes rules,
	   so we will do something straight forward and may be not exactly correct.
	 */
	fbe_ssp_transport_server_lock(&sas_port->ssp_transport_server);
	fbe_ssp_transport_server_detach_edge(&sas_port->ssp_transport_server, 
										ssp_transport_control_detach_edge->ssp_edge);
	fbe_ssp_transport_server_unlock(&sas_port->ssp_transport_server);

	fbe_ssp_transport_server_get_number_of_clients(&sas_port->ssp_transport_server, &number_of_clients);

	/* It is possible that we waiting for ssp server to become empty.
		It whould be nice if we can reschedule monitor when we have no clients any more 
	*/
	fbe_base_object_trace(	(fbe_base_object_t *)sas_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH ,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s number_of_clients = %d\n", __FUNCTION__, number_of_clients);

	if(number_of_clients == 0){
		fbe_lifecycle_reschedule(&fbe_sas_port_lifecycle_const,
								(fbe_base_object_t *) sas_port,
								(fbe_lifecycle_timer_msec_t) 0); /* Immediate reschedule */
	}

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_port_open_ssp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet)
{
	fbe_ssp_transport_control_open_edge_t * ssp_transport_control_open_edge = NULL;
	fbe_payload_control_buffer_length_t len = 0;
	fbe_ssp_edge_t * ssp_edge = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &ssp_transport_control_open_edge);
	if(ssp_transport_control_open_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &len); 
	if(len != sizeof(fbe_ssp_transport_control_open_edge_t)){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
								"%X len invalid\n", len);

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
	    return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Right now we do not know all the path state and path attributes rules,
	   so we will do something straight forward and may be not exactly correct.
	 */

	/* We need to find the entry in the table with same address and generation code.
		If we were succesful we should update path state and attributes of the edge.
	*/

	ssp_edge = (fbe_ssp_edge_t *)fbe_transport_get_edge(packet);

	/* We should look in the SAS table find sas address and generation code 
		and set device_id to the edge 
	*/
	fbe_ssp_transport_set_device_id(ssp_edge, 0xBAD);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}


static fbe_status_t 
sas_port_attach_smp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet)
{
	fbe_smp_transport_control_attach_edge_t * smp_transport_control_attach_edge = NULL;
	fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &smp_transport_control_attach_edge); 
	if(smp_transport_control_attach_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &len); 
	if(len != sizeof(fbe_smp_transport_control_attach_edge_t)){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
								"%X len invalid\n", len);

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
	    return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Right now we do not know all the path state and path attributes rules,
	   so we will do something straight forward and may be not exactly correct.
	 */

	fbe_smp_transport_server_lock(&sas_port->smp_transport_server);

	fbe_smp_transport_server_attach_edge(&sas_port->smp_transport_server, 
										smp_transport_control_attach_edge->smp_edge,
										&fbe_sas_port_lifecycle_const,
										(fbe_base_object_t *) sas_port);

	fbe_smp_transport_server_unlock(&sas_port->smp_transport_server);

    fbe_smp_transport_set_server_index(smp_transport_control_attach_edge->smp_edge, FBE_EDGE_INDEX_INVALID);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_port_detach_smp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet)
{
	fbe_smp_transport_control_detach_edge_t * smp_transport_control_detach_edge = NULL;
	fbe_payload_control_buffer_length_t len = 0;
	fbe_u32_t number_of_clients;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry \n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &smp_transport_control_detach_edge); 
	if(smp_transport_control_detach_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &len);
	if(len != sizeof(fbe_smp_transport_control_detach_edge_t)){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
								"%X len invalid\n", len);

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
	    return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Right now we do not know all the path state and path attributes rules,
	   so we will do something straight forward and may be not exactly correct.
	 */
	fbe_smp_transport_server_lock(&sas_port->smp_transport_server);
	fbe_smp_transport_server_detach_edge(&sas_port->smp_transport_server, 
										smp_transport_control_detach_edge->smp_edge);
	fbe_smp_transport_server_unlock(&sas_port->smp_transport_server);

	fbe_smp_transport_server_get_number_of_clients(&sas_port->smp_transport_server, &number_of_clients);

	/* It is possible that we waiting for smp server to become empty.
		It whould be nice if we can reschedule monitor when we have no clients any more 
	*/
	fbe_base_object_trace(	(fbe_base_object_t *)sas_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH ,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s number_of_clients = %d\n", __FUNCTION__, number_of_clients);

	if(number_of_clients == 0){
		fbe_lifecycle_reschedule(&fbe_sas_port_lifecycle_const,
								(fbe_base_object_t *) sas_port,
								(fbe_lifecycle_timer_msec_t) 0); /* Immediate reschedule */
	}

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_port_open_smp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet)
{
	fbe_smp_transport_control_open_edge_t * smp_transport_control_open_edge = NULL;
	fbe_payload_control_buffer_length_t len = 0;
	/* fbe_smp_edge_t * smp_edge = NULL; */
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &smp_transport_control_open_edge);
	if(smp_transport_control_open_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &len); 
	if(len != sizeof(fbe_smp_transport_control_open_edge_t)){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
								"%X len invalid\n", len);

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
	    return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Right now we do not know all the path state and path attributes rules,
	   so we will do something straight forward and may be not exactly correct.
	 */

	/* We need to find the entry in the table with same address and generation code.
		If we were succesful we should update path state and attributes of the edge.
	*/

	/* smp_edge = (fbe_smp_edge_t *)fbe_transport_get_edge(packet); */

	/* We should look in the SAS table find sas address and generation code 
		and set device_id to the edge 
	*/

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_port_attach_stp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet)
{
	fbe_stp_transport_control_attach_edge_t * stp_transport_control_attach_edge = NULL;
	fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &stp_transport_control_attach_edge);
	if(stp_transport_control_attach_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &len); 
	if(len != sizeof(fbe_stp_transport_control_attach_edge_t)){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
								"%X len invalid\n", len);

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
	    return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Right now we do not know all the path state and path attributes rules,
	   so we will do something straight forward and may be not exactly correct.
	 */
    fbe_stp_transport_set_server_index(stp_transport_control_attach_edge->stp_edge, FBE_EDGE_INDEX_INVALID);
	fbe_stp_transport_set_path_attributes(stp_transport_control_attach_edge->stp_edge, FBE_SSP_PATH_ATTR_CLOSED);

	stp_transport_control_attach_edge->stp_edge->io_entry = sas_port->ssp_transport_server.base_transport_server.io_entry;
	stp_transport_control_attach_edge->stp_edge->object_handle = (fbe_object_handle_t)sas_port;

	fbe_stp_transport_server_lock(&sas_port->stp_transport_server);

	fbe_stp_transport_server_attach_edge(&sas_port->stp_transport_server, 
										stp_transport_control_attach_edge->stp_edge,
										&fbe_sas_port_lifecycle_const,
										(fbe_base_object_t *) sas_port);

	fbe_stp_transport_server_unlock(&sas_port->stp_transport_server);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_port_detach_stp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet)
{
	fbe_stp_transport_control_detach_edge_t * stp_transport_control_detach_edge = NULL;
	fbe_payload_control_buffer_length_t len = 0;
	fbe_u32_t number_of_clients;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry \n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &stp_transport_control_detach_edge);
	if(stp_transport_control_detach_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &len); 
	if(len != sizeof(fbe_stp_transport_control_detach_edge_t)){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
								"%X len invalid\n", len);

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
	    return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Right now we do not know all the path state and path attributes rules,
	   so we will do something straight forward and may be not exactly correct.
	 */
	fbe_stp_transport_server_lock(&sas_port->stp_transport_server);
	fbe_stp_transport_server_detach_edge(&sas_port->stp_transport_server, 
										stp_transport_control_detach_edge->stp_edge);
	fbe_stp_transport_server_unlock(&sas_port->stp_transport_server);

	fbe_stp_transport_server_get_number_of_clients(&sas_port->stp_transport_server, &number_of_clients);

	/* It is possible that we waiting for stp server to become empty.
		It whould be nice if we can reschedule monitor when we have no clients any more 
	*/
	fbe_base_object_trace(	(fbe_base_object_t *)sas_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH ,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s number_of_clients = %d\n", __FUNCTION__, number_of_clients);

	if(number_of_clients == 0){
		fbe_lifecycle_reschedule(&fbe_sas_port_lifecycle_const,
								(fbe_base_object_t *) sas_port,
								(fbe_lifecycle_timer_msec_t) 0); /* Immediate reschedule */
	}

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_port_open_stp_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet)
{
	fbe_stp_transport_control_open_edge_t * stp_transport_control_open_edge = NULL;
	fbe_payload_control_buffer_length_t len = 0;
	/* fbe_stp_edge_t * stp_edge = NULL; */
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &stp_transport_control_open_edge);
	if(stp_transport_control_open_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &len);
	if(len != sizeof(fbe_stp_transport_control_open_edge_t)){
		fbe_base_object_trace((fbe_base_object_t *) sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
								"%X len invalid\n", len);

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
	    return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Right now we do not know all the path state and path attributes rules,
	   so we will do something straight forward and may be not exactly correct.
	 */

	/* We need to find the entry in the table with same address and generation code.
		If we were succesful we should update path state and attributes of the edge.
	*/

	/* stp_edge = (fbe_stp_edge_t *)fbe_transport_get_edge(packet); */

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

