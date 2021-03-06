#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_board.h"
#include "fbe_cpd_shim.h"
#include "fbe_transport_memory.h"
#include "fc_port_private.h"

/* Class methods forward declaration */
fbe_status_t fbe_fc_port_load(void);
fbe_status_t fbe_fc_port_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_fc_port_class_methods = {FBE_CLASS_ID_FC_PORT,
							        			  fbe_fc_port_load,
												  fbe_fc_port_unload,
												  fbe_fc_port_create_object,
												  fbe_fc_port_destroy_object,
												  fbe_fc_port_control_entry,
												  fbe_fc_port_event_entry,
                                                  NULL,
												  fbe_fc_port_monitor_entry};


/* Forward declaration */
static fbe_status_t fbe_fc_port_get_parent_address(fbe_fc_port_t * fc_port, fbe_packet_t * packet);
static fbe_status_t fc_port_get_port_driver_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/* Monitor functions */
static fbe_status_t fbe_fc_port_monitor_specialized(fbe_fc_port_t * fc_port, fbe_packet_t * packet);
static fbe_status_t fbe_fc_port_monitor_instantiated(fbe_fc_port_t * fc_port, fbe_packet_t * packet);


fbe_status_t 
fbe_fc_port_load(void)
{
	/* KvTrace("%s entry\n", __FUNCTION__); */
	return fbe_fc_port_monitor_load_verify();
}

fbe_status_t fbe_fc_port_unload(void)
{
	/*KvTrace("%s entry\n", __FUNCTION__);*/
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_fc_port_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
	fbe_fc_port_t * fc_port;
	fbe_status_t status;

	/* Call parent constructor */
	status = fbe_base_port_create_object(packet, object_handle);
	if(status != FBE_STATUS_OK){
		return status;
	}

	/* Set class id */
	fc_port = (fbe_fc_port_t *)fbe_base_handle_to_pointer(*object_handle);
	fbe_base_object_set_class_id((fbe_base_object_t *) fc_port, FBE_CLASS_ID_FC_PORT);	

    fbe_fc_port_init(fc_port);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_fc_port_init(fbe_fc_port_t * fc_port)
{
	fc_port->enclosures_number = 0;


	/* Init smp transport server 
	fbe_smp_transport_server_init(&fc_port->smp_transport_server);
    */

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_fc_port_destroy_object( fbe_object_handle_t object_handle)
{
	/* fbe_fc_port_t * fc_port = NULL; */
	fbe_status_t status = FBE_STATUS_OK;

	/* KvTrace("%s entry\n", __FUNCTION__); */
	
	/* fc_port = (fbe_fc_port_t *)fbe_base_handle_to_pointer(object_handle); */

	/* Check parent edges */
	/* Cleanup */
	//fbe_smp_transport_server_destroy(&fc_port->smp_transport_server);	

	/* Cancel all packets from terminator queue */

	
	/* Call parent destructor */
	status = fbe_base_port_destroy_object(object_handle);
	return status;
}

#if 0
fbe_status_t
fbe_fc_port_smp_transport_entry(fbe_fc_port_t * fc_port,
                                 fbe_transport_entry_t transport_entry,
                                 fbe_packet_t * packet)
{
	return transport_entry(fc_port, packet);
}

fbe_status_t
fbe_fc_port_set_smp_path_attr(fbe_fc_port_t * fc_port, fbe_edge_index_t server_index, fbe_path_attr_t path_attr)
{
	fbe_status_t status;
	status = fbe_smp_transport_server_set_path_attr(&fc_port->smp_transport_server, server_index, path_attr);
	return status;
}

fbe_status_t
fbe_fc_port_clear_smp_path_attr(fbe_fc_port_t * fc_port, fbe_edge_index_t server_index, fbe_path_attr_t path_attr)
{
	fbe_status_t status;
	status = fbe_smp_transport_server_clear_path_attr(&fc_port->smp_transport_server, server_index, path_attr);
	return status;
}

fbe_smp_edge_t *
fbe_fc_port_get_smp_edge_by_server_index(fbe_fc_port_t * fc_port, fbe_edge_index_t server_index)
{
	return fbe_smp_transport_server_get_client_edge_by_server_index(&fc_port->smp_transport_server, server_index);
}
#endif
