#ifndef FBE_DIPLEX_TRANSPORT_H
#define FBE_DIPLEX_TRANSPORT_H

#include "fbe/fbe_transport.h"
#include "fbe_base_transport.h"

typedef enum fbe_diplex_transport_control_code_e {
	FBE_DIPLEX_TRANSPORT_CONTROL_CODE_INVALID = FBE_TRANSPORT_CONTROL_CODE_INVALID_DEF(FBE_TRANSPORT_ID_DIPLEX),

	FBE_DIPLEX_TRANSPORT_CONTROL_CODE_ATTACH_EDGE, /* Client wants to attach edge to the server */
	FBE_DIPLEX_TRANSPORT_CONTROL_CODE_DETACH_EDGE, /* Client wants to detach edge from the server */

	FBE_DIPLEX_TRANSPORT_CONTROL_CODE_OPEN_EDGE, /* Client wants to clear FBE_DIPLEX_PATH_ATTR_CLOSED attribute */

	FBE_DIPLEX_TRANSPORT_CONTROL_CODE_GET_PATH_STATE, /* Usurper command for testability */
	//FBE_DIPLEX_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO, /* Usurper command for testability */

	FBE_DIPLEX_TRANSPORT_CONTROL_CODE_LAST
} fbe_diplex_transport_control_code_t;

typedef enum fbe_diplex_path_attr_flags_e {
	FBE_DIPLEX_PATH_ATTR_CLOSED         = 0x00000001,  /* set if server can not "see" the client */
	FBE_DIPLEX_PATH_ATTR_ELEMENT_CHANGE = 0x00000002,  /* set when there is a change to the element list */
	FBE_DIPLEX_PATH_ATTR_ALL            = 0xFFFFFFFF   /* Initialization value */
}fbe_diplex_path_attr_flags_t; /* This type may be used for debug purposes ONLY */

typedef struct fbe_diplex_edge_s{
	fbe_base_edge_t base_edge;
	fbe_diplex_address_t enclosure_address; /* miniport handle */
}fbe_diplex_edge_t;

/* DIPLEX transport commands */
/* FBE_DIPLEX_TRANSPORT_CONTROL_CODE_ATTACH_EDGE */
typedef struct fbe_diplex_transport_control_attach_edge_s{
	fbe_diplex_edge_t * diplex_edge; /* Pointer to diplex edge. The memory belong to the client, so only client can issue command */
}fbe_diplex_transport_control_attach_edge_t;

/* FBE_DIPLEX_TRANSPORT_CONTROL_CODE_DETACH_EDGE */
typedef struct fbe_diplex_transport_control_detach_edge_s{
	fbe_diplex_edge_t * diplex_edge; /* Pointer to diplex edge. The memory belong to the client, so only client can issue command */
}fbe_diplex_transport_control_detach_edge_t;

/* FBE_DIPLEX_TRANSPORT_CONTROL_CODE_OPEN_EDGE */
typedef enum fbe_diplex_transport_control_open_edge_status_e {
    FBE_DIPLEX_TRANSPORT_CONTROL_CODE_OPEN_EDGE_STATUS_INVALID,
    FBE_DIPLEX_TRANSPORT_CONTROL_CODE_OPEN_EDGE_STATUS_OK, /*!< Operation completed successfuly */
    FBE_DIPLEX_TRANSPORT_CONTROL_CODE_OPEN_EDGE_STATUS_NO_DEVICE, /*!< FC address not found or invalid generation code */
    FBE_DIPLEX_TRANSPORT_CONTROL_CODE_OPEN_EDGE_STATUS_LAST
}fbe_diplex_transport_control_open_edge_status_t;

typedef struct fbe_diplex_transport_control_open_edge_s{
	fbe_diplex_address_t fc_address; /* FC address of the device */
	fbe_generation_code_t generation_code; /* generation code for the FC address */
	fbe_diplex_transport_control_open_edge_status_t status;
}fbe_diplex_transport_control_open_edge_t;

/* FBE_DIPLEX_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO */
typedef struct fbe_diplex_transport_control_get_edge_info_s {
	fbe_base_transport_control_get_edge_info_t base_edge_info;
}fbe_diplex_transport_control_get_edge_info_t;


/* DIPLEX transport server */

/* fbe_base_transport_server declaration */
typedef struct fbe_diplex_transport_server_s {
	fbe_base_transport_server_t base_transport_server;
}fbe_diplex_transport_server_t;

static __forceinline fbe_status_t
fbe_diplex_transport_server_init(fbe_diplex_transport_server_t * diplex_transport_server)
{
	return fbe_base_transport_server_init((fbe_base_transport_server_t *) diplex_transport_server);
}

static __forceinline fbe_status_t
fbe_diplex_transport_server_destroy(fbe_diplex_transport_server_t * diplex_transport_server)
{
	return fbe_base_transport_server_destroy((fbe_base_transport_server_t *) diplex_transport_server);
}

static __forceinline fbe_status_t
fbe_diplex_transport_server_lock(fbe_diplex_transport_server_t * diplex_transport_server)
{
	return fbe_base_transport_server_lock((fbe_base_transport_server_t *) diplex_transport_server);
}

static __forceinline fbe_status_t
fbe_diplex_transport_server_unlock(fbe_diplex_transport_server_t * diplex_transport_server)
{
	return fbe_base_transport_server_unlock((fbe_base_transport_server_t *) diplex_transport_server);
}

static __forceinline fbe_status_t
fbe_diplex_transport_server_attach_edge(fbe_diplex_transport_server_t * diplex_transport_server, 
									 fbe_diplex_edge_t * diplex_edge,
									 fbe_lifecycle_const_t * p_class_const,
									 struct fbe_base_object_s * base_object)
{
	return fbe_base_transport_server_attach_edge((fbe_base_transport_server_t *) diplex_transport_server, 
													(fbe_base_edge_t *) diplex_edge,
													p_class_const,
													base_object);
}

static __forceinline fbe_status_t
fbe_diplex_transport_server_detach_edge(fbe_diplex_transport_server_t * diplex_transport_server, fbe_diplex_edge_t * diplex_edge)
{
	return fbe_base_transport_server_detach_edge((fbe_base_transport_server_t *) diplex_transport_server, (fbe_base_edge_t *) diplex_edge);
}

static __forceinline fbe_status_t
fbe_diplex_transport_server_get_number_of_clients(fbe_diplex_transport_server_t * diplex_transport_server, fbe_u32_t * number_of_clients)
{
	return fbe_base_transport_server_get_number_of_clients((fbe_base_transport_server_t *) diplex_transport_server, number_of_clients);
}

static __forceinline fbe_status_t
fbe_diplex_transport_server_set_path_attr(fbe_diplex_transport_server_t * diplex_transport_server,
											 fbe_edge_index_t  server_index, 
											 fbe_path_attr_t path_attr)
{
	return fbe_base_transport_server_set_path_attr((fbe_base_transport_server_t *) diplex_transport_server,
													server_index,
													path_attr);
}


static __forceinline fbe_status_t
fbe_diplex_transport_server_set_edge_path_state(fbe_diplex_transport_server_t * diplex_transport_server,
											 fbe_edge_index_t  server_index, 
											 fbe_path_state_t path_state)
{
	return fbe_base_transport_server_set_edge_path_state((fbe_base_transport_server_t *) diplex_transport_server,
															server_index,
															path_state);
}

static __forceinline fbe_status_t
fbe_diplex_transport_server_clear_path_attr(fbe_diplex_transport_server_t * diplex_transport_server,
											 fbe_edge_index_t  server_index, 
											 fbe_path_attr_t path_attr)
{
	return fbe_base_transport_server_clear_path_attr((fbe_base_transport_server_t *) diplex_transport_server,
													server_index,
													path_attr);
}

static __forceinline fbe_status_t
fbe_diplex_transport_server_set_path_state(fbe_diplex_transport_server_t * diplex_transport_server, fbe_path_state_t path_state)
{
	return fbe_base_transport_server_set_path_state((fbe_base_transport_server_t *) diplex_transport_server, path_state);
}

static __forceinline fbe_diplex_edge_t *
fbe_diplex_transport_server_get_client_edge_by_client_id(fbe_diplex_transport_server_t * diplex_transport_server,
															fbe_object_id_t object_id)
{
	return (fbe_diplex_edge_t *)fbe_base_transport_server_get_client_edge_by_client_id((fbe_base_transport_server_t *) diplex_transport_server,
																							object_id);
}
static __forceinline fbe_diplex_edge_t *
fbe_diplex_transport_server_get_client_edge_by_server_index(fbe_diplex_transport_server_t * diplex_transport_server,
															fbe_edge_index_t server_index)
{
	return (fbe_diplex_edge_t *)fbe_base_transport_server_get_client_edge_by_server_index((fbe_base_transport_server_t *) diplex_transport_server,
																							server_index);
}
static __forceinline fbe_status_t
fbe_diplex_transport_send_functional_packet(fbe_diplex_edge_t * diplex_edge, fbe_packet_t * packet)
{
	fbe_status_t status=FBE_STATUS_OK;
	fbe_payload_ex_t * payload = NULL;

	payload = fbe_transport_get_payload_ex(packet);
	//status = fbe_payload_increment_diplex_operation_level(payload); 
    //<<--comment for time being.Requires Naizhong's chnages for diplex payload.
	if(status != FBE_STATUS_OK){
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
		return status;
	}

	status = fbe_base_transport_send_functional_packet((fbe_base_edge_t *) diplex_edge, packet, FBE_PACKAGE_ID_PHYSICAL);
	return status;
}

static __forceinline fbe_lifecycle_status_t
fbe_diplex_transport_server_pending(fbe_diplex_transport_server_t * diplex_transport_server,
									   fbe_lifecycle_const_t * p_class_const,
									   struct fbe_base_object_s * base_object)
{
	return fbe_base_transport_server_pending((fbe_base_transport_server_t *) diplex_transport_server,
												p_class_const,
												base_object);

}

static __forceinline fbe_status_t
fbe_diplex_transport_get_path_state(fbe_diplex_edge_t * diplex_edge, fbe_path_state_t * path_state)
{	
	return fbe_base_transport_get_path_state((fbe_base_edge_t *) diplex_edge, path_state);
}

static __forceinline fbe_status_t
fbe_diplex_transport_get_path_attributes(fbe_diplex_edge_t * diplex_edge, fbe_path_attr_t * path_attr)
{	
	return fbe_base_transport_get_path_attributes((fbe_base_edge_t *) diplex_edge, path_attr);
}

static __forceinline fbe_status_t
fbe_diplex_transport_set_path_attributes(fbe_diplex_edge_t * diplex_edge, fbe_path_attr_t  path_attr)
{	
	return fbe_base_transport_set_path_attributes((fbe_base_edge_t *) diplex_edge, path_attr);
}

static __forceinline fbe_status_t
fbe_diplex_transport_set_server_id(fbe_diplex_edge_t * diplex_edge, fbe_object_id_t server_id)
{	
	return fbe_base_transport_set_server_id((fbe_base_edge_t *) diplex_edge, server_id);
}

static __forceinline fbe_status_t
fbe_diplex_transport_get_server_id(fbe_diplex_edge_t * diplex_edge, fbe_object_id_t * server_id)
{	
	return fbe_base_transport_get_server_id((fbe_base_edge_t *) diplex_edge, server_id);
}


static __forceinline fbe_status_t
fbe_diplex_transport_get_server_index(fbe_diplex_edge_t * diplex_edge, fbe_edge_index_t * server_index)
{	
	return fbe_base_transport_get_server_index((fbe_base_edge_t *) diplex_edge, server_index);
}

static __forceinline fbe_status_t
fbe_diplex_transport_set_server_index(fbe_diplex_edge_t * diplex_edge, fbe_edge_index_t server_index)
{	
	return fbe_base_transport_set_server_index((fbe_base_edge_t *) diplex_edge, server_index);
}

static __forceinline fbe_status_t
fbe_diplex_transport_set_client_id(fbe_diplex_edge_t * diplex_edge, fbe_object_id_t client_id)
{	
	return fbe_base_transport_set_client_id((fbe_base_edge_t *) diplex_edge, client_id);
}

static __forceinline fbe_status_t
fbe_diplex_transport_get_client_id(fbe_diplex_edge_t * diplex_edge, fbe_object_id_t * client_id)
{	
	return fbe_base_transport_get_client_id((fbe_base_edge_t *) diplex_edge, client_id);
}

static __forceinline fbe_status_t
fbe_diplex_transport_get_client_index(fbe_diplex_edge_t * diplex_edge, fbe_edge_index_t * client_index)
{	
	return fbe_base_transport_get_client_index((fbe_base_edge_t *) diplex_edge, client_index);
}

static __forceinline fbe_status_t
fbe_diplex_transport_set_client_index(fbe_diplex_edge_t * diplex_edge, fbe_edge_index_t client_index)
{	
	return fbe_base_transport_set_client_index((fbe_base_edge_t *) diplex_edge, client_index);
}

static __forceinline fbe_status_t
fbe_diplex_transport_set_transport_id(fbe_diplex_edge_t * diplex_edge)
{
	return fbe_base_transport_set_transport_id((fbe_base_edge_t *) diplex_edge, FBE_TRANSPORT_ID_DIPLEX);
}

static __forceinline fbe_status_t
fbe_diplex_transport_get_enclosure_address(fbe_diplex_edge_t * diplex_edge, fbe_diplex_address_t * enclosure_address)
{   
    *enclosure_address = diplex_edge->enclosure_address;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_diplex_transport_set_enclosure_address(fbe_diplex_edge_t * diplex_edge, fbe_diplex_address_t  enclosure_address)
{   
    diplex_edge->enclosure_address = enclosure_address;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_diplex_transport_send_control_packet(fbe_diplex_edge_t * diplex_edge, fbe_packet_t * packet)
{
	return(fbe_base_transport_send_control_packet((fbe_base_edge_t *)diplex_edge, packet));
}

#endif /* FBE_DIPLEX_TRANSPORT_H */
