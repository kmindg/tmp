
#ifndef FBE_API_TRANSPORT_PACKET_INTERFACE_PRIVATE_H
#define FBE_API_TRANSPORT_PACKET_INTERFACE_PRIVATE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_transport_packet_interface.h"
#include "fbe/fbe_api_common.h"

typedef enum fbe_transport_control_thread_flag_e{
    THREAD_NULL,
    THREAD_RUN,
    THREAD_STOP,
    THREAD_DONE
}fbe_transport_control_thread_flag_t;

typedef struct fbe_api_client_send_context_s{
    FBE_ALIGN(8) fbe_u32_t	total_msg_length;/*MUST BE FIRST for performance reasons, we read it on the server side as one field since we don't need the rest*/
    FBE_ALIGN(8)void *	user_buffer;/*opaque*/
	FBE_ALIGN(8)void *	completion_function;
	FBE_ALIGN(8)void *  	completion_context;
    FBE_ALIGN(8)fbe_u32_t	packet_lengh;
}fbe_api_client_send_context_t;

void fbe_api_transport_control_destroy_server_control(void);
void fbe_api_transport_control_destroy_client_control(fbe_transport_connection_target_t connect_to_sp);

fbe_status_t fbe_api_transport_register_notification_callback(fbe_packet_t *packet);
fbe_status_t fbe_api_transport_unregister_notification_callback(fbe_packet_t *packet);
fbe_packet_t * fbe_api_transport_convert_buffer_to_packet(fbe_u8_t *packet_buffer,
                                                              server_command_completion_t completion_function,
                                                              void * context);

fbe_status_t fbe_api_transport_get_control_operation(fbe_packet_t * packet, fbe_payload_control_operation_t **control_operation);
fbe_status_t fbe_api_transport_get_package_mask_for_sever_target(fbe_transport_connection_target_t connect_to_sp, fbe_package_notification_id_mask_t *package_mask);

fbe_status_t fbe_api_transport_control_clear_fstc_notifications_queue(fbe_package_id_t package_id);
static void move_packet_levels(fbe_packet_t *packet);

fbe_status_t fbe_transport_control_register_notification(fbe_packet_t *packet, fbe_package_id_t package_id);
fbe_status_t fbe_transport_control_unregister_notification(fbe_packet_t *packet,fbe_package_id_t package_id);
fbe_status_t fbe_transport_control_unregister_all_notifications(fbe_packet_t *packet);
fbe_status_t fbe_transport_control_get_event(fbe_packet_t *packet, fbe_package_id_t package_id);
fbe_status_t fbe_transport_control_get_pid (fbe_packet_t * packet);
fbe_status_t fbe_transport_control_suite_start_notification (fbe_packet_t * packet);

fbe_status_t fbe_transport_control_register_notification_element(fbe_package_id_t package_id,
																 void * context,
																 fbe_notification_type_t notification_type,
																 fbe_topology_object_type_t object_type);

fbe_status_t fbe_transport_control_unregister_notification_element(fbe_package_id_t package_id);
fbe_bool_t fbe_api_transport_is_notification_registered(fbe_package_notification_id_mask_t package_id);
fbe_status_t fbe_transport_send_client_control_packet_to_server(fbe_packet_t * packet);

fbe_transport_connection_target_t
fbe_transport_translate_fbe_sim_connection_target_to_fbe_transport_connection_target (fbe_sim_transport_connection_target_t sim_target);
fbe_status_t fbe_api_sim_transport_handle_server_packet(fbe_u8_t *packet_buffer,
                                                        server_command_completion_t completion_function,
                                                        void * context);
fbe_status_t fbe_transport_clear_all_notifications(fbe_bool_t unregister_notification_flag);

#endif /*FBE_API_TRANSPORT_PACKET_INTERFACE_PRIVATE_H*/
