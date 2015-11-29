#include "fbe_ssp_transport.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"


fbe_status_t
fbe_ssp_transport_send_control_packet(fbe_ssp_edge_t * ssp_edge, fbe_packet_t * packet)
{
	fbe_status_t status;

	status = fbe_base_transport_send_control_packet((fbe_base_edge_t *) ssp_edge, packet);
	return status;
}

fbe_status_t
fbe_ssp_transport_send_functional_packet(fbe_ssp_edge_t * ssp_edge, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_payload_ex_t * payload = NULL;

	payload = fbe_transport_get_payload_ex(packet);
	status = fbe_payload_ex_increment_cdb_operation_level(payload);
	if(status != FBE_STATUS_OK){
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
		return status;
	}

	if((ssp_edge->io_entry != NULL) && (ssp_edge->object_handle != NULL)){
		status = fbe_base_transport_send_functional_packet_directly((fbe_base_edge_t *) ssp_edge, packet, FBE_PACKAGE_ID_PHYSICAL,
												   ssp_edge->io_entry, ssp_edge->object_handle);
	} else {
		status = fbe_base_transport_send_functional_packet((fbe_base_edge_t *) ssp_edge, packet, FBE_PACKAGE_ID_PHYSICAL);
	}

	return status;
}

fbe_lifecycle_status_t
fbe_ssp_transport_server_pending(fbe_ssp_transport_server_t * ssp_transport_server,
									   fbe_lifecycle_const_t * p_class_const,
									   struct fbe_base_object_s * base_object)
{
	return fbe_base_transport_server_pending((fbe_base_transport_server_t *) ssp_transport_server,
												p_class_const,
												base_object);
}