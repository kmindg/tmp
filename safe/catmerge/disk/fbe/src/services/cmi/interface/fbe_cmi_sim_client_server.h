#ifndef FBE_CMI_SIM_CLIENT_SERVER_H
#define FBE_CMI_SIM_CLIENT_SERVER_H

#include "fbe/fbe_types.h"
#include "fbe_cmi.h"
#include "fbe_cmi_private.h"

#define RECEIVE_BUFFER_SIZE	    2048
#define MAX_PACKET_BUFFER_SIZE  FBE_CMI_MAX_MESSAGE_SIZE

typedef enum fbe_cmi_sim_tcp_message_type_e {
	FBE_CMI_SIM_TCP_MESSAGE_TYPE_FLOAT_DATA_ONLY,
	FBE_CMI_SIM_TCP_MESSAGE_TYPE_WITH_FIXED_DATA,
}fbe_cmi_sim_tcp_message_type_t;

typedef struct fbe_cmi_sim_tcp_message_s{
	fbe_s32_t 							total_message_lenth;/*always first, don't remove*/
	fbe_u8_t *							msg_id;/*always second, we look at it when we get it to make sure that's what we sent*/
	fbe_cmi_conduit_id_t				conduit;
    fbe_cmi_message_t 					user_message;
    fbe_cmi_event_t						event_id;
	fbe_cmi_client_id_t 				client_id;
	fbe_cmi_event_callback_context_t	context;
	fbe_status_t						message_status;
	fbe_u32_t							msg_idx;
	fbe_cmi_sim_tcp_message_type_t      msg_type;
	fbe_u32_t                           fixed_data_length;
	fbe_sg_element_t *                  sg_list;
	void *                              dest_addr;
	void *                              control_buffer;
}fbe_cmi_sim_tcp_message_t;



fbe_status_t fbe_cmi_sim_init_conduit_client(fbe_cmi_conduit_id_t conduit_id, fbe_cmi_sp_id_t sp_id);
fbe_status_t fbe_cmi_sim_init_conduit_server(fbe_cmi_conduit_id_t conduit_id, fbe_cmi_sp_id_t sp_id);
fbe_status_t fbe_cmi_sim_map_conduit_to_client_port(fbe_cmi_conduit_id_t conduit_id, const fbe_u8_t **port_string, fbe_cmi_sp_id_t sp_id);
fbe_status_t fbe_cmi_sim_map_conduit_to_server_port(fbe_cmi_conduit_id_t conduit_id, const fbe_u8_t **port_string, fbe_cmi_sp_id_t sp_id);
fbe_status_t fbe_cmi_sim_client_send_message (fbe_cmi_sim_tcp_message_t *message);
fbe_status_t fbe_cmi_sim_destroy_conduit_client(fbe_cmi_conduit_id_t conduit_id);
fbe_status_t fbe_cmi_sim_destroy_conduit_server(fbe_cmi_conduit_id_t conduit_id);
fbe_status_t fbe_cmi_sim_init_spinlock(fbe_cmi_conduit_id_t conduit_id);
void fbe_cmi_client_clear_pending_messages(fbe_cmi_conduit_id_t conduit_id);
void verify_peer_connection_is_up(fbe_cmi_sp_id_t this_sp_id, fbe_cmi_conduit_id_t conduit_id);
fbe_status_t fbe_cmi_sim_wait_for_client_to_init(fbe_cmi_conduit_id_t conduit_id);
#endif /*FBE_CMI_SIM_CLIENT_SERVER_H*/
fbe_status_t fbe_cmi_sim_update_cmi_port(void);
