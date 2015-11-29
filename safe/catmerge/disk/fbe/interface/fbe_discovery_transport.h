#ifndef FBE_DISCOVERY_TRANSPORT_H
#define FBE_DISCOVERY_TRANSPORT_H

#include "fbe/fbe_transport.h"
#include "fbe_base_transport.h"
#include "fbe_lifecycle.h"

typedef enum fbe_discovery_transport_control_code_e {
	FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_INVALID = FBE_TRANSPORT_CONTROL_CODE_INVALID_DEF(FBE_TRANSPORT_ID_DISCOVERY),

	FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_CREATE_EDGE, /* During instantiation server sends it to the client */

	FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_ATTACH_EDGE, /* Client wants to attach edge to the server */
	FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_DETACH_EDGE, /* Client wants to detach edge from the server */

	FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO,
	FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_DEATH_REASON,


	FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_LAST
} fbe_discovery_transport_control_code_t;

/* FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO */
typedef struct fbe_discovery_transport_control_get_edge_info_s {
	fbe_base_transport_control_get_edge_info_t base_edge_info;
}fbe_discovery_transport_control_get_edge_info_t;

typedef enum fbe_discovery_path_attr_flags_e {
	FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT			= 0x00000001,	/* This flag is deprecated */
	FBE_DISCOVERY_PATH_ATTR_REMOVED				= 0x00000002,	/* physically not present , not inserted */
	FBE_DISCOVERY_PATH_ATTR_BYPASSED_NONPERSIST	= 0x00000004,	/* unbypass may be issued */
	FBE_DISCOVERY_PATH_ATTR_BYPASSED_PERSIST	    = 0x00000008,	/* bypassed by hardware due to hardware problem or a usurper command */

	FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_NONPERSIST    = 0x00000010,	/* powered on may be issued */
	FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_PERSIST        = 0x00000020,	/* powered off by hardware due to hardware problem or a usurper command */

	FBE_DISCOVERY_PATH_ATTR_POWERSAVE_SUPPORTED	= 0x00000040,	/* power save is supported */
	FBE_DISCOVERY_PATH_ATTR_POWERSAVE_ON		= 0x00000080,	/* power save mode is on */

	FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_PENDING		= 0x00000100,	/* power cycle is pending */
	FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_COMPLETE		= 0x00000200,	/* power cycle is complete */

	FBE_DISCOVERY_PATH_ATTR_BYPASSED_UNRECOV	        = 0x00000400,	/* bypassed by for some unrecoveralbe reason */
	FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_UNRECOV	        = 0x00000800,	/* powered off for some unrecoverable reason */      

	FBE_DISCOVERY_PATH_ATTR_SPINUP_PENDING              = 0x00001000,
	FBE_DISCOVERY_PATH_ATTR_SPINUP_PERMITTED            = 0x00002000,	

	FBE_DISCOVERY_PATH_ATTR_LINK_READY            = 0x00004000,

	FBE_DISCOVERY_PATH_ATTR_POWERFAIL_DETECTED          = 0x00008000, /* Power failure detected */

    FBE_DISCOVERY_PATH_ATTR_POWERED_ON_NEED_DESTROY     = 0x00010000,

	FBE_DISCOVERY_PATH_ATTR_ALL				= 0xFFFFFFFF	/* Initialization value */
}fbe_discovery_path_attr_flags_t; /* This type may be used for debug purposes ONLY */

typedef struct fbe_discovery_edge_s{
	fbe_base_edge_t base_edge;

}fbe_discovery_edge_t;

/* Discovery transport commands */
/* FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_CREATE_EDGE */
typedef struct fbe_discovery_transport_control_create_edge_s{
	fbe_object_id_t	  server_id;		/* object id of the functional transport server */
	fbe_edge_index_t  server_index;		/* Unique identifier for this client */
}fbe_discovery_transport_control_create_edge_t;

/* FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_ATTACH_EDGE */
typedef struct fbe_discovery_transport_control_attach_edge_s{
	fbe_discovery_edge_t * discovery_edge; /* Pointer to discovery edge. The memory belong to the client, so only client can issue command */
}fbe_discovery_transport_control_attach_edge_t;

/* FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_DETACH_EDGE */
typedef struct fbe_discovery_transport_control_detach_edge_s{
	fbe_discovery_edge_t * discovery_edge; /* Pointer to discovery edge. The memory belong to the client, so only client can issue command */
}fbe_discovery_transport_control_detach_edge_t;

/*FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_DEATH_REASON*/
typedef struct fbe_discovery_transport_control_get_death_reason_s{
	fbe_object_death_reason_t death_reason;
}fbe_discovery_transport_control_get_death_reason_t;

/* Discovery transport server */

/* fbe_base_transport_server declaration */
typedef struct fbe_discovery_transport_server_s {
	fbe_base_transport_server_t base_transport_server;
}fbe_discovery_transport_server_t;

static __forceinline fbe_status_t
fbe_discovery_transport_server_init(fbe_discovery_transport_server_t * discovery_transport_server)
{
	return fbe_base_transport_server_init((fbe_base_transport_server_t *) discovery_transport_server);
}

static __forceinline fbe_status_t
fbe_discovery_transport_server_destroy(fbe_discovery_transport_server_t * discovery_transport_server)
{
	return fbe_base_transport_server_destroy((fbe_base_transport_server_t *) discovery_transport_server);
}

static __forceinline fbe_status_t
fbe_discovery_transport_server_lock(fbe_discovery_transport_server_t * discovery_transport_server)
{
	return fbe_base_transport_server_lock((fbe_base_transport_server_t *) discovery_transport_server);
}

static __forceinline fbe_status_t
fbe_discovery_transport_server_unlock(fbe_discovery_transport_server_t * discovery_transport_server)
{
	return fbe_base_transport_server_unlock((fbe_base_transport_server_t *) discovery_transport_server);
}

static __forceinline fbe_status_t
fbe_discovery_transport_server_attach_edge(fbe_discovery_transport_server_t * discovery_transport_server, 
										   fbe_discovery_edge_t * discovery_edge,
										   fbe_lifecycle_const_t * p_class_const,
										   struct fbe_base_object_s * base_object)
{
	return fbe_base_transport_server_attach_edge((fbe_base_transport_server_t *) discovery_transport_server, 
													(fbe_base_edge_t *) discovery_edge,
													p_class_const,
													base_object);
}

static __forceinline fbe_status_t
fbe_discovery_transport_server_detach_edge(fbe_discovery_transport_server_t * discovery_transport_server, fbe_discovery_edge_t * discovery_edge)
{
	return fbe_base_transport_server_detach_edge((fbe_base_transport_server_t *) discovery_transport_server, (fbe_base_edge_t *) discovery_edge);
}

static __forceinline fbe_status_t
fbe_discovery_transport_server_get_number_of_clients(fbe_discovery_transport_server_t * discovery_transport_server, fbe_u32_t * number_of_clients)
{
	return fbe_base_transport_server_get_number_of_clients((fbe_base_transport_server_t *) discovery_transport_server, number_of_clients);
}

static __forceinline fbe_status_t
fbe_discovery_transport_server_set_path_attr(fbe_discovery_transport_server_t * discovery_transport_server,
											 fbe_edge_index_t  server_index, 
											 fbe_path_attr_t path_attr)
{
	return fbe_base_transport_server_set_path_attr((fbe_base_transport_server_t *) discovery_transport_server,
													server_index,
													path_attr);
}


static __forceinline fbe_status_t
fbe_discovery_transport_server_set_edge_path_state(fbe_discovery_transport_server_t * discovery_transport_server,
													fbe_edge_index_t  server_index, 
													fbe_path_state_t path_state)
{
	return fbe_base_transport_server_set_edge_path_state((fbe_base_transport_server_t *) discovery_transport_server,
															server_index,
															path_state);
}

static __forceinline fbe_status_t
fbe_discovery_transport_server_clear_path_attr(fbe_discovery_transport_server_t * discovery_transport_server,
											 fbe_edge_index_t  server_index, 
											 fbe_path_attr_t path_attr)
{
	return fbe_base_transport_server_clear_path_attr((fbe_base_transport_server_t *) discovery_transport_server,
													server_index,
													path_attr);
}

static __forceinline fbe_status_t
fbe_discovery_transport_server_set_path_state(fbe_discovery_transport_server_t * discovery_transport_server, fbe_path_state_t path_state)
{
	return fbe_base_transport_server_set_path_state((fbe_base_transport_server_t *) discovery_transport_server, path_state);
}

static __forceinline fbe_discovery_edge_t *
fbe_discovery_transport_server_get_client_edge_by_client_id(fbe_discovery_transport_server_t * discovery_transport_server,
															fbe_object_id_t object_id)
{
	return (fbe_discovery_edge_t *)fbe_base_transport_server_get_client_edge_by_client_id((fbe_base_transport_server_t *) discovery_transport_server,
																							object_id);
}

static __forceinline fbe_discovery_edge_t *
fbe_discovery_transport_server_get_client_edge_by_server_index(fbe_discovery_transport_server_t * discovery_transport_server,
																fbe_edge_index_t server_index)
{
	return (fbe_discovery_edge_t *)fbe_base_transport_server_get_client_edge_by_server_index((fbe_base_transport_server_t *) discovery_transport_server,
																								server_index);
}

fbe_lifecycle_status_t
fbe_discovery_transport_server_pending(fbe_discovery_transport_server_t * discovery_transport_server,
									   fbe_lifecycle_const_t * p_class_const,
									   struct fbe_base_object_s * base_object);

static __forceinline fbe_status_t
fbe_discovery_transport_get_path_state(fbe_discovery_edge_t * discovery_edge, fbe_path_state_t * path_state)
{	
	return fbe_base_transport_get_path_state((fbe_base_edge_t *) discovery_edge, path_state);
}

static __forceinline fbe_status_t
fbe_discovery_transport_get_path_attributes(fbe_discovery_edge_t * discovery_edge, fbe_path_attr_t * path_attr)
{	
	return fbe_base_transport_get_path_attributes((fbe_base_edge_t *) discovery_edge, path_attr);
}

static __forceinline fbe_status_t
fbe_discovery_transport_set_path_attributes(fbe_discovery_edge_t * discovery_edge, fbe_path_attr_t  path_attr)
{	
	return fbe_base_transport_set_path_attributes((fbe_base_edge_t *) discovery_edge, path_attr);
}

static __forceinline fbe_status_t
fbe_discovery_transport_set_server_id(fbe_discovery_edge_t * discovery_edge, fbe_object_id_t server_id)
{	
	return fbe_base_transport_set_server_id((fbe_base_edge_t *) discovery_edge, server_id);
}

static __forceinline fbe_status_t
fbe_discovery_transport_get_server_id(fbe_discovery_edge_t * discovery_edge, fbe_object_id_t * server_id)
{	
	return fbe_base_transport_get_server_id((fbe_base_edge_t *) discovery_edge, server_id);
}


static __forceinline fbe_status_t
fbe_discovery_transport_get_server_index(fbe_discovery_edge_t * discovery_edge, fbe_edge_index_t * server_index)
{	
	return fbe_base_transport_get_server_index((fbe_base_edge_t *) discovery_edge, server_index);
}

static __forceinline fbe_status_t
fbe_discovery_transport_set_server_index(fbe_discovery_edge_t * discovery_edge, fbe_edge_index_t server_index)
{	
	return fbe_base_transport_set_server_index((fbe_base_edge_t *) discovery_edge, server_index);
}

static __forceinline fbe_status_t
fbe_discovery_transport_set_client_id(fbe_discovery_edge_t * discovery_edge, fbe_object_id_t client_id)
{	
	return fbe_base_transport_set_client_id((fbe_base_edge_t *) discovery_edge, client_id);
}

static __forceinline fbe_status_t
fbe_discovery_transport_get_client_id(fbe_discovery_edge_t * discovery_edge, fbe_object_id_t * client_id)
{	
	return fbe_base_transport_get_client_id((fbe_base_edge_t *) discovery_edge, client_id);
}


static __forceinline fbe_status_t
fbe_discovery_transport_get_client_index(fbe_discovery_edge_t * discovery_edge, fbe_edge_index_t * client_index)
{	
	return fbe_base_transport_get_client_index((fbe_base_edge_t *) discovery_edge, client_index);
}

static __forceinline fbe_status_t
fbe_discovery_transport_set_client_index(fbe_discovery_edge_t * discovery_edge, fbe_edge_index_t client_index)
{	
	return fbe_base_transport_set_client_index((fbe_base_edge_t *) discovery_edge, client_index);
}

static __forceinline fbe_status_t
fbe_discovery_transport_set_transport_id(fbe_discovery_edge_t * discovery_edge)
{
	return fbe_base_transport_set_transport_id((fbe_base_edge_t *) discovery_edge, FBE_TRANSPORT_ID_DISCOVERY);
}

/* This function is deprecated */
/* fbe_status_t fbe_discovery_transport_send_packet(fbe_discovery_edge_t * discovery_edge, fbe_packet_t * packet); */

fbe_status_t fbe_discovery_transport_send_control_packet(fbe_discovery_edge_t * discovery_edge, fbe_packet_t * packet);
fbe_status_t fbe_discovery_transport_send_functional_packet(fbe_discovery_edge_t * discovery_edge, fbe_packet_t * packet);

#endif /* FBE_DISCOVERY_TRANSPORT_H */
