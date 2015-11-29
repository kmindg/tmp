#ifndef FBE_STP_TRANSPORT_H
#define FBE_STP_TRANSPORT_H

#include "fbe/fbe_transport.h"
#include "fbe_base_transport.h"

typedef enum fbe_stp_transport_control_code_e {
	FBE_STP_TRANSPORT_CONTROL_CODE_INVALID = FBE_TRANSPORT_CONTROL_CODE_INVALID_DEF(FBE_TRANSPORT_ID_STP),

	FBE_STP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE, /* Client wants to attach edge to the server */
	FBE_STP_TRANSPORT_CONTROL_CODE_DETACH_EDGE, /* Client wants to detach edge from the server */

	FBE_STP_TRANSPORT_CONTROL_CODE_OPEN_EDGE, /* Client wants to clear FBE_STP_PATH_ATTR_CLOSED attribute */

	FBE_STP_TRANSPORT_CONTROL_CODE_GET_PATH_STATE, /* Userper command for testability */
	FBE_STP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO, /* Userper command for testability */
	FBE_STP_TRANSPORT_CONTROL_CODE_SEND_DIAGNOSTIC, /* Userper command for testability */
	FBE_STP_TRANSPORT_CONTROL_CODE_RESET_DEVICE, /* Reset the device.*/

	FBE_STP_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK,
	FBE_STP_TRANSPORT_CONTROL_CODE_REMOVE_EDGE_TAP_HOOK,

	FBE_STP_TRANSPORT_CONTROL_CODE_LAST
} fbe_stp_transport_control_code_t;

typedef enum fbe_stp_path_attr_flags_e {
	FBE_STP_PATH_ATTR_CLOSED		= 0x00000001,	/* set if server can not "see" the client */
	FBE_STP_PATH_ATTR_ALL			= 0xFFFFFFFF	/* Initialization value */
}fbe_stp_path_attr_flags_t; /* This type may be used for debug purposes ONLY */

typedef struct fbe_stp_edge_s{
	fbe_base_edge_t base_edge;
	fbe_cpd_device_id_t cpd_device_id; /* miniport handle */

	fbe_status_t (* io_entry)(fbe_object_handle_t object_handle, fbe_packet_t * packet);
    fbe_object_handle_t object_handle;

}fbe_stp_edge_t;

/* STP transport commands */
/* FBE_STP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE */
typedef struct fbe_stp_transport_control_attach_edge_s{
	fbe_stp_edge_t * stp_edge; /* Pointer to stp edge. The memory belong to the client, so only client can issue command */
}fbe_stp_transport_control_attach_edge_t;

/* FBE_STP_TRANSPORT_CONTROL_CODE_DETACH_EDGE */
typedef struct fbe_stp_transport_control_detach_edge_s{
	fbe_stp_edge_t * stp_edge; /* Pointer to stp edge. The memory belong to the client, so only client can issue command */
}fbe_stp_transport_control_detach_edge_t;

/* FBE_STP_TRANSPORT_CONTROL_CODE_OPEN_EDGE */
typedef struct fbe_stp_transport_control_open_edge_s{
	fbe_sas_address_t sas_address; /* SAS address of the device */
	fbe_generation_code_t generation_code; /* generation code for the SAS address */
}fbe_stp_transport_control_open_edge_t;

/* FBE_STP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO */
typedef struct fbe_stp_transport_control_get_edge_info_s {
	fbe_base_transport_control_get_edge_info_t base_edge_info;
}fbe_stp_transport_control_get_edge_info_t;


/* FBE_STP_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK */
/* use fbe_transport_control_set_edge_tap_hook_t in fbe_transport.h */

/* STP transport server */

/* fbe_base_transport_server declaration */
typedef struct fbe_stp_transport_server_s {
	fbe_base_transport_server_t base_transport_server;
}fbe_stp_transport_server_t;

static __forceinline fbe_status_t
fbe_stp_transport_server_init(fbe_stp_transport_server_t * stp_transport_server)
{
	return fbe_base_transport_server_init((fbe_base_transport_server_t *) stp_transport_server);
}

static __forceinline fbe_status_t
fbe_stp_transport_server_destroy(fbe_stp_transport_server_t * stp_transport_server)
{
	return fbe_base_transport_server_destroy((fbe_base_transport_server_t *) stp_transport_server);
}

static __forceinline fbe_status_t
fbe_stp_transport_server_lock(fbe_stp_transport_server_t * stp_transport_server)
{
	return fbe_base_transport_server_lock((fbe_base_transport_server_t *) stp_transport_server);
}

static __forceinline fbe_status_t
fbe_stp_transport_server_unlock(fbe_stp_transport_server_t * stp_transport_server)
{
	return fbe_base_transport_server_unlock((fbe_base_transport_server_t *) stp_transport_server);
}

static __forceinline fbe_status_t
fbe_stp_transport_server_attach_edge(fbe_stp_transport_server_t * stp_transport_server, 
									 fbe_stp_edge_t * stp_edge,
									 fbe_lifecycle_const_t * p_class_const,
									 struct fbe_base_object_s * base_object)
{
	return fbe_base_transport_server_attach_edge((fbe_base_transport_server_t *) stp_transport_server, 
													(fbe_base_edge_t *) stp_edge,
													p_class_const,
													base_object);
}

static __forceinline fbe_status_t
fbe_stp_transport_server_detach_edge(fbe_stp_transport_server_t * stp_transport_server, fbe_stp_edge_t * stp_edge)
{
	return fbe_base_transport_server_detach_edge((fbe_base_transport_server_t *) stp_transport_server, (fbe_base_edge_t *) stp_edge);
}

static __forceinline fbe_status_t
fbe_stp_transport_server_get_number_of_clients(fbe_stp_transport_server_t * stp_transport_server, fbe_u32_t * number_of_clients)
{
	return fbe_base_transport_server_get_number_of_clients((fbe_base_transport_server_t *) stp_transport_server, number_of_clients);
}

static __forceinline fbe_status_t
fbe_stp_transport_server_set_path_attr(fbe_stp_transport_server_t * stp_transport_server,
											 fbe_edge_index_t  server_index, 
											 fbe_path_attr_t path_attr)
{
	return fbe_base_transport_server_set_path_attr((fbe_base_transport_server_t *) stp_transport_server,
													server_index,
													path_attr);
}


static __forceinline fbe_status_t
fbe_stp_transport_server_set_edge_path_state(fbe_stp_transport_server_t * stp_transport_server,
											 fbe_edge_index_t  server_index, 
											 fbe_path_state_t path_state)
{
	return fbe_base_transport_server_set_edge_path_state((fbe_base_transport_server_t *) stp_transport_server,
															server_index,
															path_state);
}

static __forceinline fbe_status_t
fbe_stp_transport_server_clear_path_attr(fbe_stp_transport_server_t * stp_transport_server,
											 fbe_edge_index_t  server_index, 
											 fbe_path_attr_t path_attr)
{
	return fbe_base_transport_server_clear_path_attr((fbe_base_transport_server_t *) stp_transport_server,
													server_index,
													path_attr);
}

static __forceinline fbe_status_t
fbe_stp_transport_server_set_path_state(fbe_stp_transport_server_t * stp_transport_server, fbe_path_state_t path_state)
{
	return fbe_base_transport_server_set_path_state((fbe_base_transport_server_t *) stp_transport_server, path_state);
}

static __forceinline fbe_stp_edge_t *
fbe_stp_transport_server_get_client_edge_by_client_id(fbe_stp_transport_server_t * stp_transport_server,
															fbe_object_id_t object_id)
{
	return (fbe_stp_edge_t *)fbe_base_transport_server_get_client_edge_by_client_id((fbe_base_transport_server_t *) stp_transport_server,
																							object_id);
}
static __forceinline fbe_stp_edge_t *
fbe_stp_transport_server_get_client_edge_by_server_index(fbe_stp_transport_server_t * stp_transport_server,
															fbe_edge_index_t server_index)
{
	return (fbe_stp_edge_t *)fbe_base_transport_server_get_client_edge_by_server_index((fbe_base_transport_server_t *) stp_transport_server,
																							server_index);
}

fbe_lifecycle_status_t
fbe_stp_transport_server_pending(fbe_stp_transport_server_t * stp_transport_server,
									   fbe_lifecycle_const_t * p_class_const,
									   struct fbe_base_object_s * base_object);

static __forceinline fbe_status_t
fbe_stp_transport_get_path_state(fbe_stp_edge_t * stp_edge, fbe_path_state_t * path_state)
{	
	return fbe_base_transport_get_path_state((fbe_base_edge_t *) stp_edge, path_state);
}

static __forceinline fbe_status_t
fbe_stp_transport_get_path_attributes(fbe_stp_edge_t * stp_edge, fbe_path_attr_t * path_attr)
{	
	return fbe_base_transport_get_path_attributes((fbe_base_edge_t *) stp_edge, path_attr);
}

static __forceinline fbe_status_t
fbe_stp_transport_set_path_attributes(fbe_stp_edge_t * stp_edge, fbe_path_attr_t  path_attr)
{	
	return fbe_base_transport_set_path_attributes((fbe_base_edge_t *) stp_edge, path_attr);
}

static __forceinline fbe_status_t
fbe_stp_transport_set_server_id(fbe_stp_edge_t * stp_edge, fbe_object_id_t server_id)
{	
	return fbe_base_transport_set_server_id((fbe_base_edge_t *) stp_edge, server_id);
}

static __forceinline fbe_status_t
fbe_stp_transport_get_server_id(fbe_stp_edge_t * stp_edge, fbe_object_id_t * server_id)
{	
	return fbe_base_transport_get_server_id((fbe_base_edge_t *) stp_edge, server_id);
}


static __forceinline fbe_status_t
fbe_stp_transport_get_server_index(fbe_stp_edge_t * stp_edge, fbe_edge_index_t * server_index)
{	
	return fbe_base_transport_get_server_index((fbe_base_edge_t *) stp_edge, server_index);
}

static __forceinline fbe_status_t
fbe_stp_transport_set_server_index(fbe_stp_edge_t * stp_edge, fbe_edge_index_t server_index)
{	
	return fbe_base_transport_set_server_index((fbe_base_edge_t *) stp_edge, server_index);
}

static __forceinline fbe_status_t
fbe_stp_transport_set_client_id(fbe_stp_edge_t * stp_edge, fbe_object_id_t client_id)
{	
	return fbe_base_transport_set_client_id((fbe_base_edge_t *) stp_edge, client_id);
}

static __forceinline fbe_status_t
fbe_stp_transport_get_client_id(fbe_stp_edge_t * stp_edge, fbe_object_id_t * client_id)
{	
	return fbe_base_transport_get_client_id((fbe_base_edge_t *) stp_edge, client_id);
}


static __forceinline fbe_status_t
fbe_stp_transport_get_client_index(fbe_stp_edge_t * stp_edge, fbe_edge_index_t * client_index)
{	
	return fbe_base_transport_get_client_index((fbe_base_edge_t *) stp_edge, client_index);
}

static __forceinline fbe_status_t
fbe_stp_transport_set_client_index(fbe_stp_edge_t * stp_edge, fbe_edge_index_t client_index)
{	
	return fbe_base_transport_set_client_index((fbe_base_edge_t *) stp_edge, client_index);
}

static __forceinline fbe_status_t
fbe_stp_transport_set_transport_id(fbe_stp_edge_t * stp_edge)
{
	return fbe_base_transport_set_transport_id((fbe_base_edge_t *) stp_edge, FBE_TRANSPORT_ID_STP);
}

static __forceinline fbe_status_t
fbe_stp_transport_get_device_id(fbe_stp_edge_t * stp_edge, fbe_cpd_device_id_t * cpd_device_id)
{	
	*cpd_device_id = stp_edge->cpd_device_id;
	return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_stp_transport_set_device_id(fbe_stp_edge_t * stp_edge, fbe_cpd_device_id_t  cpd_device_id)
{	
	stp_edge->cpd_device_id = cpd_device_id;
	return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_stp_transport_edge_set_hook(fbe_stp_edge_t * stp_edge, fbe_edge_hook_function_t hook)
{	
	return fbe_base_transport_set_hook_function((fbe_base_edge_t *) stp_edge, hook);
}

static __forceinline fbe_status_t
fbe_stp_transport_edge_remove_hook(fbe_stp_edge_t *stp_edge, fbe_packet_t *packet_p)
{	
	return fbe_base_transport_remove_hook_function((fbe_base_edge_t *)stp_edge, packet_p, FBE_STP_TRANSPORT_CONTROL_CODE_REMOVE_EDGE_TAP_HOOK);
}

/* This function is deprecated */
fbe_status_t fbe_stp_transport_send_packet(fbe_stp_edge_t * stp_edge, fbe_packet_t * packet);

fbe_status_t fbe_stp_transport_send_control_packet(fbe_stp_edge_t * stp_edge, fbe_packet_t * packet);
fbe_status_t fbe_stp_transport_send_functional_packet(fbe_stp_edge_t * stp_edge, fbe_packet_t * packet);


#endif /* FBE_STP_TRANSPORT_H */