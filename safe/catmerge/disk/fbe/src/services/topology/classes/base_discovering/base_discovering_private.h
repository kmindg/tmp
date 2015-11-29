#ifndef BASE_DISCOVERING_PRIVATE_H
#define BASE_DISCOVERING_PRIVATE_H

#include "fbe_base.h"
#include "fbe_trace.h"

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_object.h"
#include "fbe_base_object.h"

#include "fbe_discovery_transport.h"
#include "base_discovered_private.h"


extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_discovering);

/* These are the lifecycle condition ids for a discovering class. */
typedef enum fbe_base_discovering_lifecycle_cond_id_e {
	/* If poll detected something new or something gone it should set this condition */
	FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_BASE_DISCOVERING),
	FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL,
    FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL_PER_DAY,

	/* This condition preset in destroy rotary and should set path state of all client discovery edges to GONE */
	/* FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_GONE, */
	/* This condition preset in destroy rotary and should ensure that all client discovery edges are detached */

	/*
	FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_SERVER_NOT_EMPTY,
	*/

	FBE_BASE_DISCOVERING_LIFECYCLE_COND_LAST /* must be last */
} fbe_base_discovering_lifecycle_cond_id_t;

typedef struct fbe_base_discovering_s {
	fbe_base_discovered_t				base_discovered;
	fbe_discovery_transport_server_t	discovery_transport_server;
	FBE_LIFECYCLE_DEF_INST_DATA;
	FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_BASE_DISCOVERING_LIFECYCLE_COND_LAST));
}fbe_base_discovering_t;


/* Methods */
fbe_status_t fbe_base_discovering_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_base_discovering_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_base_discovering_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_base_discovering_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);

fbe_status_t fbe_base_discovering_monitor_load_verify(void);

fbe_status_t fbe_base_discovering_attach_edge(fbe_base_discovering_t * base_discovering, fbe_discovery_edge_t * client_edge);
fbe_status_t fbe_base_discovering_detach_edge(fbe_base_discovering_t * base_discovering, fbe_discovery_edge_t * client_edge);

fbe_status_t fbe_base_discovering_instantiate_client(fbe_base_discovering_t * base_discovering, 
														fbe_class_id_t parent_class_id, 
														fbe_edge_index_t  server_index);

fbe_status_t fbe_base_discovering_get_number_of_clients(fbe_base_discovering_t * base_discovering, fbe_u32_t * number_of_clients);
fbe_status_t fbe_base_discovering_get_path_attr(fbe_base_discovering_t * base_discovering, fbe_edge_index_t server_index, fbe_path_attr_t * path_attr);
fbe_status_t fbe_base_discovering_set_path_attr(fbe_base_discovering_t * base_discovering, fbe_edge_index_t server_index, fbe_path_attr_t path_attr);
fbe_status_t fbe_base_discovering_clear_path_attr(fbe_base_discovering_t * base_discovering, fbe_edge_index_t server_index, fbe_path_attr_t path_attr);

/* This function is deprecated
fbe_status_t fbe_base_discovering_set_path_state(fbe_base_discovering_t * base_discovering, fbe_edge_index_t server_index, fbe_path_state_t path_state);
*/

fbe_status_t fbe_base_discovering_update_path_state(fbe_base_discovering_t * base_discovering);

fbe_status_t fbe_base_discovering_transport_server_lock(fbe_base_discovering_t * base_discovering);
fbe_status_t fbe_base_discovering_transport_server_unlock(fbe_base_discovering_t * base_discovering);

fbe_status_t fbe_base_discovering_get_server_index_by_client_id(fbe_base_discovering_t * base_discovering,
												   fbe_object_id_t client_id,
												   fbe_edge_index_t * server_index);

fbe_status_t
fbe_base_discovering_discovery_bouncer_entry(fbe_base_discovering_t * base_discovering,
												fbe_transport_entry_t transport_entry,
												fbe_packet_t * packet);

fbe_discovery_edge_t *
fbe_base_discovering_get_client_edge_by_server_index(fbe_base_discovering_t * base_discovering, fbe_edge_index_t server_index);

/* Executer functions */
fbe_status_t fbe_base_discovering_discovery_transport_entry(fbe_base_discovering_t * base_discovering, fbe_packet_t * packet);

#endif /* BASE_DISCOVERING_PRIVATE_H */
