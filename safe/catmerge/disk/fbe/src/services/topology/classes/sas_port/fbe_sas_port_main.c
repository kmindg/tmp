#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_board.h"
#include "fbe_cpd_shim.h"
#include "fbe_transport_memory.h"
#include "sas_port_private.h"

/* Class methods forward declaration */
fbe_status_t fbe_sas_port_load(void);
fbe_status_t fbe_sas_port_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_sas_port_class_methods = {FBE_CLASS_ID_SAS_PORT,
							        			  fbe_sas_port_load,
												  fbe_sas_port_unload,
												  fbe_sas_port_create_object,
												  fbe_sas_port_destroy_object,
												  fbe_sas_port_control_entry,
												  fbe_sas_port_event_entry,
                                                  NULL,
												  fbe_sas_port_monitor_entry};


/* Forward declaration */
static fbe_status_t fbe_sas_port_get_parent_address(fbe_sas_port_t * sas_port, fbe_packet_t * packet);
static fbe_status_t sas_port_get_port_driver_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/* Monitor functions */
static fbe_status_t fbe_sas_port_monitor_specialized(fbe_sas_port_t * sas_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_port_monitor_instantiated(fbe_sas_port_t * sas_port, fbe_packet_t * packet);


fbe_status_t 
fbe_sas_port_load(void)
{
	/* KvTrace("%s entry\n", __FUNCTION__); */
	return fbe_sas_port_monitor_load_verify();
}

fbe_status_t fbe_sas_port_unload(void)
{
	/*KvTrace("%s entry\n", __FUNCTION__);*/
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_port_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
	fbe_sas_port_t * sas_port;
	fbe_status_t status;

	/* Call parent constructor */
	status = fbe_base_port_create_object(packet, object_handle);
	if(status != FBE_STATUS_OK){
		return status;
	}

	/* Set class id */
	sas_port = (fbe_sas_port_t *)fbe_base_handle_to_pointer(*object_handle);
	fbe_base_object_set_class_id((fbe_base_object_t *) sas_port, FBE_CLASS_ID_SAS_PORT);	

    fbe_sas_port_init(sas_port);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_port_init(fbe_sas_port_t * sas_port)
{
	sas_port->enclosures_number = 0;

	/* Init ssp transport server */
	fbe_ssp_transport_server_init(&sas_port->ssp_transport_server);

	/* Init smp transport server */
	fbe_smp_transport_server_init(&sas_port->smp_transport_server);

	/* Init stp transport server */
	fbe_stp_transport_server_init(&sas_port->stp_transport_server);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_port_destroy_object( fbe_object_handle_t object_handle)
{
	fbe_sas_port_t * sas_port = NULL;
	fbe_status_t status;

	/* KvTrace("%s entry\n", __FUNCTION__); */
	
	sas_port = (fbe_sas_port_t *)fbe_base_handle_to_pointer(object_handle);

	/* Check parent edges */
	/* Cleanup */
	fbe_ssp_transport_server_destroy(&sas_port->ssp_transport_server);
	fbe_smp_transport_server_destroy(&sas_port->smp_transport_server);
	fbe_stp_transport_server_destroy(&sas_port->stp_transport_server);

	/* Cancel all packets from terminator queue */

	
	/* Call parent destructor */
	status = fbe_base_port_destroy_object(object_handle);
	return status;
}

fbe_status_t
fbe_sas_port_ssp_transport_entry(fbe_sas_port_t * sas_port,
									fbe_transport_entry_t transport_entry,
									fbe_packet_t * packet)
{
	/* fbe_cpd_device_id_t cpd_device_id; */
	/* fbe_ssp_edge_t * ssp_edge = NULL; */
	/* fbe_io_block_t * io_block = NULL; */

	/* We have to evaluate our state */
	/* We have to evaluate the state of the edge */	
	/* We have to evaluate the number of outstanding I/O's to maintain the queue depth */	

	/* We have to increment the number of outstanding I/O's */
	/* Finally, we can send the packet to the executer of the specific object */
	/* We are running in executer context, so we are good to extract and fill the data from the protocol */
	/* io_block = fbe_transport_get_io_block (packet); */

	/* ssp_edge = (fbe_ssp_edge_t *)fbe_transport_get_edge(packet); */

	/* fbe_ssp_transport_get_device_id(ssp_edge, &cpd_device_id); */

	/* fbe_io_block_execute_cdb_set_device_id(io_block, cpd_device_id); */

	transport_entry(sas_port, packet);
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_sas_port_set_ssp_path_attr(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index, fbe_path_attr_t path_attr)
{
	fbe_status_t status;
	status = fbe_ssp_transport_server_set_path_attr(&sas_port->ssp_transport_server, server_index, path_attr);
	return status;
}

fbe_status_t
fbe_sas_port_clear_ssp_path_attr(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index, fbe_path_attr_t path_attr)
{
	fbe_status_t status;
	status = fbe_ssp_transport_server_clear_path_attr(&sas_port->ssp_transport_server, server_index, path_attr);
	return status;
}

fbe_status_t
fbe_sas_port_smp_transport_entry(fbe_sas_port_t * sas_port,
                                 fbe_transport_entry_t transport_entry,
                                 fbe_packet_t * packet)
{
	return transport_entry(sas_port, packet);
}

fbe_status_t
fbe_sas_port_set_smp_path_attr(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index, fbe_path_attr_t path_attr)
{
	fbe_status_t status;
	status = fbe_smp_transport_server_set_path_attr(&sas_port->smp_transport_server, server_index, path_attr);
	return status;
}

fbe_status_t
fbe_sas_port_clear_smp_path_attr(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index, fbe_path_attr_t path_attr)
{
	fbe_status_t status;
	status = fbe_smp_transport_server_clear_path_attr(&sas_port->smp_transport_server, server_index, path_attr);
	return status;
}

fbe_status_t
fbe_sas_port_set_stp_path_attr(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index, fbe_path_attr_t path_attr)
{
	fbe_status_t status;
	status = fbe_stp_transport_server_set_path_attr(&sas_port->stp_transport_server, server_index, path_attr);
	return status;
}

fbe_status_t
fbe_sas_port_clear_stp_path_attr(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index, fbe_path_attr_t path_attr)
{
	fbe_status_t status;
	status = fbe_stp_transport_server_clear_path_attr(&sas_port->stp_transport_server, server_index, path_attr);
	return status;
}

fbe_smp_edge_t *
fbe_sas_port_get_smp_edge_by_server_index(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index)
{
	return fbe_smp_transport_server_get_client_edge_by_server_index(&sas_port->smp_transport_server, server_index);
}

fbe_ssp_edge_t *
fbe_sas_port_get_ssp_edge_by_server_index(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index)
{
	return fbe_ssp_transport_server_get_client_edge_by_server_index(&sas_port->ssp_transport_server, server_index);
}

fbe_stp_edge_t *
fbe_sas_port_get_stp_edge_by_server_index(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index)
{
	return fbe_stp_transport_server_get_client_edge_by_server_index(&sas_port->stp_transport_server, server_index);
}

fbe_status_t
fbe_sas_port_stp_transport_entry(fbe_sas_port_t * sas_port,
									fbe_transport_entry_t transport_entry,
									fbe_packet_t * packet)
{
	/* We have to evaluate our state */
	/* We have to evaluate the state of the edge */	
	/* We have to evaluate the number of outstanding I/O's to maintain the queue depth */	

	/* We have to increment the number of outstanding I/O's */
	/* Finally, we can send the packet to the executer of the specific object */
	/* We are running in executer context, so we are good to extract and fill the data from the protocol */

	transport_entry(sas_port, packet);
	return FBE_STATUS_OK;
}

fbe_bool_t
fbe_sas_port_get_power_failure_detected(fbe_sas_port_t *sas_port)
{
    return sas_port->power_failure_detected;
}
