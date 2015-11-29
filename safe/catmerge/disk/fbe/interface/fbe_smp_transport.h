#ifndef FBE_SMP_TRANSPORT_H
#define FBE_SMP_TRANSPORT_H

#include "fbe/fbe_transport.h"
#include "fbe_base_transport.h"

typedef enum fbe_smp_transport_control_code_e {
	FBE_SMP_TRANSPORT_CONTROL_CODE_INVALID = FBE_TRANSPORT_CONTROL_CODE_INVALID_DEF(FBE_TRANSPORT_ID_SMP),

	FBE_SMP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE, /* Client wants to attach edge to the server */
	FBE_SMP_TRANSPORT_CONTROL_CODE_DETACH_EDGE, /* Client wants to detach edge from the server */

	FBE_SMP_TRANSPORT_CONTROL_CODE_OPEN_EDGE, /* Client wants to clear FBE_SMP_PATH_ATTR_CLOSED attribute */

	FBE_SMP_TRANSPORT_CONTROL_CODE_GET_PATH_STATE, /* Usurper command for testability */
	FBE_SMP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO, /* Usurper command for testability */

    FBE_SMP_TRANSPORT_CONTROL_CODE_RESET_END_DEVICE, /* Send reset phy request to SMP port - expander*/
	FBE_SMP_TRANSPORT_CONTROL_CODE_LAST
} fbe_smp_transport_control_code_t;

typedef enum fbe_smp_path_attr_flags_e {
	FBE_SMP_PATH_ATTR_CLOSED         = 0x00000001,  /* set if server can not "see" the client */
	FBE_SMP_PATH_ATTR_ELEMENT_CHANGE = 0x00000002,  /* set when there is a change to the element list */
	FBE_SMP_PATH_ATTR_ALL            = 0xFFFFFFFF   /* Initialization value */
}fbe_smp_path_attr_flags_t; /* This type may be used for debug purposes ONLY */

typedef struct fbe_smp_edge_s{
	fbe_base_edge_t base_edge;
	fbe_cpd_device_id_t cpd_device_id; /* miniport handle */
}fbe_smp_edge_t;

/* SMP transport commands */
/* FBE_SMP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE */
typedef struct fbe_smp_transport_control_attach_edge_s{
	fbe_smp_edge_t * smp_edge; /* Pointer to smp edge. The memory belong to the client, so only client can issue command */
}fbe_smp_transport_control_attach_edge_t;

/* FBE_SMP_TRANSPORT_CONTROL_CODE_DETACH_EDGE */
typedef struct fbe_smp_transport_control_detach_edge_s{
	fbe_smp_edge_t * smp_edge; /* Pointer to smp edge. The memory belong to the client, so only client can issue command */
}fbe_smp_transport_control_detach_edge_t;

/* FBE_SMP_TRANSPORT_CONTROL_CODE_OPEN_EDGE */
typedef enum fbe_smp_transport_control_open_edge_status_e {
    FBE_SMP_TRANSPORT_CONTROL_CODE_OPEN_EDGE_STATUS_INVALID,
    FBE_SMP_TRANSPORT_CONTROL_CODE_OPEN_EDGE_STATUS_OK, /*!< Operation completed successfuly */
    FBE_SMP_TRANSPORT_CONTROL_CODE_OPEN_EDGE_STATUS_NO_DEVICE, /*!< SAS address not found or invalid generation code */
    FBE_SMP_TRANSPORT_CONTROL_CODE_OPEN_EDGE_STATUS_LAST
}fbe_smp_transport_control_open_edge_status_t;

typedef struct fbe_smp_transport_control_open_edge_s{
	fbe_sas_address_t sas_address; /* SAS address of the device */
	fbe_generation_code_t generation_code; /* generation code for the SAS address */
	fbe_smp_transport_control_open_edge_status_t status;
}fbe_smp_transport_control_open_edge_t;

/* FBE_SMP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO */
typedef struct fbe_smp_transport_control_get_edge_info_s {
	fbe_base_transport_control_get_edge_info_t base_edge_info;
}fbe_smp_transport_control_get_edge_info_t;

typedef enum fbe_smp_transport_control_reset_end_device_status_e {
    FBE_SMP_TRANSPORT_CONTROL_CODE_RESET_END_DEVICE_STATUS_INVALID,
    FBE_SMP_TRANSPORT_CONTROL_CODE_RESET_END_DEVICE_STATUS_OK, /*!< Operation completed successfuly */
    FBE_SMP_TRANSPORT_CONTROL_CODE_RESET_END_DEVICE_STATUS_NO_DEVICE, /*!< SAS address not found or invalid generation code */
    FBE_SMP_TRANSPORT_CONTROL_CODE_RESET_END_DEVICE_STATUS_LAST
}fbe_smp_transport_control_reset_end_device_status_t;

typedef struct fbe_smp_transport_control_reset_end_device_s{
	fbe_sas_address_t sas_address; /* SAS address of the SMP target */
	fbe_generation_code_t generation_code; /* generation code for the SMP port SAS address */
    fbe_u8_t              phy_id;/* Phy ID of the device to be reset.*/
	fbe_smp_transport_control_reset_end_device_status_t status;
}fbe_smp_transport_control_reset_end_device_t;

/* SMP transport server */

/* fbe_base_transport_server declaration */
typedef struct fbe_smp_transport_server_s {
	fbe_base_transport_server_t base_transport_server;
}fbe_smp_transport_server_t;

static __forceinline fbe_status_t
fbe_smp_transport_server_init(fbe_smp_transport_server_t * smp_transport_server)
{
	return fbe_base_transport_server_init((fbe_base_transport_server_t *) smp_transport_server);
}

static __forceinline fbe_status_t
fbe_smp_transport_server_destroy(fbe_smp_transport_server_t * smp_transport_server)
{
	return fbe_base_transport_server_destroy((fbe_base_transport_server_t *) smp_transport_server);
}

static __forceinline fbe_status_t
fbe_smp_transport_server_lock(fbe_smp_transport_server_t * smp_transport_server)
{
	return fbe_base_transport_server_lock((fbe_base_transport_server_t *) smp_transport_server);
}

static __forceinline fbe_status_t
fbe_smp_transport_server_unlock(fbe_smp_transport_server_t * smp_transport_server)
{
	return fbe_base_transport_server_unlock((fbe_base_transport_server_t *) smp_transport_server);
}

static __forceinline fbe_status_t
fbe_smp_transport_server_attach_edge(fbe_smp_transport_server_t * smp_transport_server, 
									 fbe_smp_edge_t * smp_edge,
									 fbe_lifecycle_const_t * p_class_const,
									 struct fbe_base_object_s * base_object)
{
	return fbe_base_transport_server_attach_edge((fbe_base_transport_server_t *) smp_transport_server, 
													(fbe_base_edge_t *) smp_edge,
													p_class_const,
													base_object);
}

static __forceinline fbe_status_t
fbe_smp_transport_server_detach_edge(fbe_smp_transport_server_t * smp_transport_server, fbe_smp_edge_t * smp_edge)
{
	return fbe_base_transport_server_detach_edge((fbe_base_transport_server_t *) smp_transport_server, (fbe_base_edge_t *) smp_edge);
}

static __forceinline fbe_status_t
fbe_smp_transport_server_get_number_of_clients(fbe_smp_transport_server_t * smp_transport_server, fbe_u32_t * number_of_clients)
{
	return fbe_base_transport_server_get_number_of_clients((fbe_base_transport_server_t *) smp_transport_server, number_of_clients);
}

static __forceinline fbe_status_t
fbe_smp_transport_server_set_path_attr(fbe_smp_transport_server_t * smp_transport_server,
											 fbe_edge_index_t  server_index, 
											 fbe_path_attr_t path_attr)
{
	return fbe_base_transport_server_set_path_attr((fbe_base_transport_server_t *) smp_transport_server,
													server_index,
													path_attr);
}


static __forceinline fbe_status_t
fbe_smp_transport_server_set_edge_path_state(fbe_smp_transport_server_t * smp_transport_server,
											 fbe_edge_index_t  server_index, 
											 fbe_path_state_t path_state)
{
	return fbe_base_transport_server_set_edge_path_state((fbe_base_transport_server_t *) smp_transport_server,
															server_index,
															path_state);
}

static __forceinline fbe_status_t
fbe_smp_transport_server_clear_path_attr(fbe_smp_transport_server_t * smp_transport_server,
											 fbe_edge_index_t  server_index, 
											 fbe_path_attr_t path_attr)
{
	return fbe_base_transport_server_clear_path_attr((fbe_base_transport_server_t *) smp_transport_server,
													server_index,
													path_attr);
}

static __forceinline fbe_status_t
fbe_smp_transport_server_set_path_state(fbe_smp_transport_server_t * smp_transport_server, fbe_path_state_t path_state)
{
	return fbe_base_transport_server_set_path_state((fbe_base_transport_server_t *) smp_transport_server, path_state);
}

static __forceinline fbe_smp_edge_t *
fbe_smp_transport_server_get_client_edge_by_client_id(fbe_smp_transport_server_t * smp_transport_server,
															fbe_object_id_t object_id)
{
	return (fbe_smp_edge_t *)fbe_base_transport_server_get_client_edge_by_client_id((fbe_base_transport_server_t *) smp_transport_server,
																							object_id);
}
static __forceinline fbe_smp_edge_t *
fbe_smp_transport_server_get_client_edge_by_server_index(fbe_smp_transport_server_t * smp_transport_server,
															fbe_edge_index_t server_index)
{
	return (fbe_smp_edge_t *)fbe_base_transport_server_get_client_edge_by_server_index((fbe_base_transport_server_t *) smp_transport_server,
																							server_index);
}
static __forceinline fbe_status_t
fbe_smp_transport_send_functional_packet(fbe_smp_edge_t * smp_edge, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_payload_ex_t * payload = NULL;

	payload = fbe_transport_get_payload_ex(packet);
	status = fbe_payload_ex_increment_smp_operation_level(payload);
	if(status != FBE_STATUS_OK){
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
		return status;
	}

	status = fbe_base_transport_send_functional_packet((fbe_base_edge_t *) smp_edge, packet, FBE_PACKAGE_ID_PHYSICAL);
	return status;
}

static __forceinline fbe_lifecycle_status_t
fbe_smp_transport_server_pending(fbe_smp_transport_server_t * smp_transport_server,
									   fbe_lifecycle_const_t * p_class_const,
									   struct fbe_base_object_s * base_object)
{
	return fbe_base_transport_server_pending((fbe_base_transport_server_t *) smp_transport_server,
												p_class_const,
												base_object);

}

static __forceinline fbe_status_t
fbe_smp_transport_get_path_state(fbe_smp_edge_t * smp_edge, fbe_path_state_t * path_state)
{	
	return fbe_base_transport_get_path_state((fbe_base_edge_t *) smp_edge, path_state);
}

static __forceinline fbe_status_t
fbe_smp_transport_get_path_attributes(fbe_smp_edge_t * smp_edge, fbe_path_attr_t * path_attr)
{	
	return fbe_base_transport_get_path_attributes((fbe_base_edge_t *) smp_edge, path_attr);
}

static __forceinline fbe_status_t
fbe_smp_transport_set_path_attributes(fbe_smp_edge_t * smp_edge, fbe_path_attr_t  path_attr)
{	
	return fbe_base_transport_set_path_attributes((fbe_base_edge_t *) smp_edge, path_attr);
}

static __forceinline fbe_status_t
fbe_smp_transport_set_server_id(fbe_smp_edge_t * smp_edge, fbe_object_id_t server_id)
{	
	return fbe_base_transport_set_server_id((fbe_base_edge_t *) smp_edge, server_id);
}

static __forceinline fbe_status_t
fbe_smp_transport_get_server_id(fbe_smp_edge_t * smp_edge, fbe_object_id_t * server_id)
{	
	return fbe_base_transport_get_server_id((fbe_base_edge_t *) smp_edge, server_id);
}


static __forceinline fbe_status_t
fbe_smp_transport_get_server_index(fbe_smp_edge_t * smp_edge, fbe_edge_index_t * server_index)
{	
	return fbe_base_transport_get_server_index((fbe_base_edge_t *) smp_edge, server_index);
}

static __forceinline fbe_status_t
fbe_smp_transport_set_server_index(fbe_smp_edge_t * smp_edge, fbe_edge_index_t server_index)
{	
	return fbe_base_transport_set_server_index((fbe_base_edge_t *) smp_edge, server_index);
}

static __forceinline fbe_status_t
fbe_smp_transport_set_client_id(fbe_smp_edge_t * smp_edge, fbe_object_id_t client_id)
{	
	return fbe_base_transport_set_client_id((fbe_base_edge_t *) smp_edge, client_id);
}

static __forceinline fbe_status_t
fbe_smp_transport_get_client_id(fbe_smp_edge_t * smp_edge, fbe_object_id_t * client_id)
{	
	return fbe_base_transport_get_client_id((fbe_base_edge_t *) smp_edge, client_id);
}

static __forceinline fbe_status_t
fbe_smp_transport_get_client_index(fbe_smp_edge_t * smp_edge, fbe_edge_index_t * client_index)
{	
	return fbe_base_transport_get_client_index((fbe_base_edge_t *) smp_edge, client_index);
}

static __forceinline fbe_status_t
fbe_smp_transport_set_client_index(fbe_smp_edge_t * smp_edge, fbe_edge_index_t client_index)
{	
	return fbe_base_transport_set_client_index((fbe_base_edge_t *) smp_edge, client_index);
}

static __forceinline fbe_status_t
fbe_smp_transport_set_transport_id(fbe_smp_edge_t * smp_edge)
{
	return fbe_base_transport_set_transport_id((fbe_base_edge_t *) smp_edge, FBE_TRANSPORT_ID_SMP);
}

static __forceinline fbe_status_t
fbe_smp_transport_get_device_id(fbe_smp_edge_t * smp_edge, fbe_cpd_device_id_t * cpd_device_id)
{   
    *cpd_device_id = smp_edge->cpd_device_id;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_smp_transport_set_device_id(fbe_smp_edge_t * smp_edge, fbe_cpd_device_id_t  cpd_device_id)
{   
    smp_edge->cpd_device_id = cpd_device_id;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_smp_transport_send_control_packet(fbe_smp_edge_t * smp_edge, fbe_packet_t * packet)
{
	return(fbe_base_transport_send_control_packet((fbe_base_edge_t *)smp_edge, packet));
}

#endif /* FBE_SMP_TRANSPORT_H */
