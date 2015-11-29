#ifndef FBE_BASE_TRANSPORT_H
#define FBE_BASE_TRANSPORT_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_payload_control_operation.h"
#include "fbe_base_service.h"
#include "fbe_topology.h"


typedef struct fbe_transport_service_s{
	fbe_base_service_t	base_service;
}fbe_transport_service_t;

/* Declared in fbe_transport_main.c */
extern fbe_transport_service_t transport_service;

/* It was decided to separate the control code name spaces
   create unique (per transport) control codes.
*/

#define FBE_TRANSPORT_CONTROL_CODE_INVALID_DEF(_transport_id) \
	((FBE_PAYLOAD_CONTROL_CODE_TYPE_TRANSPORT << FBE_PAYLOAD_CONTROL_CODE_TYPE_SHIFT) | (_transport_id << FBE_PAYLOAD_CONTROL_CODE_ID_SHIFT))


typedef enum fbe_base_transport_control_code_e {
	FBE_BASE_TRANSPORT_CONTROL_CODE_INVALID = FBE_TRANSPORT_CONTROL_CODE_INVALID_DEF(FBE_TRANSPORT_ID_BASE),
	
	FBE_BASE_TRANSPORT_CONTROL_CODE_LAST
} fbe_base_transport_control_code_t;

/* This structure will be returned as part of the get_edge_info structure of the specific transport */
typedef struct fbe_base_transport_control_get_edge_info_s {
	fbe_object_id_t		client_id;
	fbe_object_id_t		server_id;

	fbe_edge_index_t	client_index;
	fbe_edge_index_t	server_index;

	fbe_path_state_t	path_state;
	fbe_path_attr_t		path_attr;
	fbe_transport_id_t	transport_id;
}fbe_base_transport_control_get_edge_info_t;

typedef void * fbe_transport_entry_context_t;
/* This function is provided by object when it calls to the policy engine */
typedef fbe_status_t (* fbe_transport_entry_t)(fbe_transport_entry_context_t context, fbe_packet_t * packet);
typedef fbe_status_t (* fbe_transport_entry_with_tag_t)(fbe_transport_entry_context_t context, fbe_packet_t * packet, fbe_u8_t tag); /* Used for SATA */

/* fbe_base_transport_server declaration */
typedef struct fbe_base_transport_server_s {
	fbe_base_edge_t	  * client_list;		/*!< Single linked list of connected clients */
	fbe_spinlock_t		client_list_lock;	/*!< Spinlock used to protect client list */
	fbe_status_t (* io_entry)(fbe_object_handle_t object_handle, fbe_packet_t * packet);
}fbe_base_transport_server_t;


static __forceinline fbe_status_t
fbe_base_transport_server_init(fbe_base_transport_server_t * base_transport_server)
{
	base_transport_server->client_list = NULL;
	fbe_spinlock_init(&base_transport_server->client_list_lock);
	base_transport_server->io_entry = NULL;
	return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_transport_server_destroy(fbe_base_transport_server_t * base_transport_server)
{
	base_transport_server->client_list = NULL; /* We may want to check if the list is empty */
	fbe_spinlock_destroy(&base_transport_server->client_list_lock);
	return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_transport_server_lock(fbe_base_transport_server_t * base_transport_server)
{
	fbe_spinlock_lock(&base_transport_server->client_list_lock);
	return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_transport_server_unlock(fbe_base_transport_server_t * base_transport_server)
{
	fbe_spinlock_unlock(&base_transport_server->client_list_lock);
	return FBE_STATUS_OK;
}

/* Caller must take the server lock */
fbe_status_t fbe_base_transport_server_attach_edge(fbe_base_transport_server_t * base_transport_server,
									  fbe_base_edge_t * base_edge,
									  fbe_lifecycle_const_t * p_class_const,
									  struct fbe_base_object_s * base_object);

/* Caller must take the server lock */
fbe_status_t fbe_base_transport_server_get_edge(fbe_base_transport_server_t * base_transport_server,
												fbe_edge_index_t server_index, 
												fbe_base_edge_t ** client_edge);

/* Caller must take the server lock */
fbe_status_t fbe_base_transport_server_detach_edge(fbe_base_transport_server_t * base_transport_server, 
												   fbe_base_edge_t * client_edge);

fbe_status_t fbe_base_transport_server_get_number_of_clients(fbe_base_transport_server_t * base_transport_server,
															 fbe_u32_t * number_of_clients);


fbe_status_t fbe_base_transport_server_set_path_attr(fbe_base_transport_server_t * base_transport_server,
													 fbe_edge_index_t  server_index, 
													 fbe_path_attr_t path_attr);

fbe_status_t fbe_base_transport_server_clear_path_attr(fbe_base_transport_server_t * base_transport_server,
														fbe_edge_index_t  server_index, 
														fbe_path_attr_t path_attr);


fbe_status_t fbe_base_transport_server_set_edge_path_state(fbe_base_transport_server_t * base_transport_server,
															fbe_edge_index_t  server_index,
															fbe_path_state_t path_state);


fbe_status_t fbe_base_transport_server_set_path_state(fbe_base_transport_server_t * base_transport_server,
													  fbe_path_state_t path_state);


/* Caller MUST hold the lock */
fbe_base_edge_t * fbe_base_transport_server_get_client_edge_by_client_id(fbe_base_transport_server_t * base_transport_server,
																		 fbe_object_id_t object_id);


fbe_base_edge_t * fbe_base_transport_server_get_client_edge_by_server_index(fbe_base_transport_server_t * base_transport_server,
																			fbe_edge_index_t server_index);



fbe_lifecycle_status_t
fbe_base_transport_server_pending(fbe_base_transport_server_t * base_transport_server,
									   fbe_lifecycle_const_t * p_class_const,
									   struct fbe_base_object_s * base_object);


fbe_status_t
fbe_base_transport_server_update_path_state(fbe_base_transport_server_t * base_transport_server,
											fbe_lifecycle_const_t * p_class_const,
											struct fbe_base_object_s * base_object);

fbe_status_t fbe_base_transport_server_lifecycle_to_path_state(fbe_lifecycle_state_t lifecycle_state, fbe_path_state_t *	path_state);

/* fbe_base_transport_client declaration */
fbe_status_t fbe_base_transport_send_control_packet(fbe_base_edge_t * base_edge, fbe_packet_t * packet);
fbe_status_t fbe_base_transport_send_functional_packet(fbe_base_edge_t * base_edge, fbe_packet_t * packet, fbe_package_id_t package_id);

fbe_status_t
fbe_base_transport_send_functional_packet_directly(fbe_base_edge_t * base_edge, 
												   fbe_packet_t * packet, 
												   fbe_package_id_t package_id,
												   fbe_status_t (* io_entry)(fbe_object_handle_t object_handle, fbe_packet_t * packet),
												   fbe_object_handle_t object_handle);


#endif /* FBE_BASE_TRANSPORT_H */