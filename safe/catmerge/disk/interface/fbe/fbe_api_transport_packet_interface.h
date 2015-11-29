#ifndef FBE_API_TRANSPORT_PACKET_INTERFACE_H
#define FBE_API_TRANSPORT_PACKET_INTERFACE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_transport.h"
#include "fbe_api_sim_transport_packet_interface.h"
FBE_API_CPP_EXPORT_START

fbe_status_t FBE_API_CALL fbe_api_transport_send_server_control_packet(fbe_u8_t *packet_buffer,
															  server_command_completion_t completion_function,
															  void * context);

fbe_status_t FBE_API_CALL fbe_api_transport_set_control_entry(fbe_service_control_entry_t control_entry, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_transport_get_control_entry(fbe_service_control_entry_t * control_entry, fbe_package_id_t package_id);
void FBE_API_CALL fbe_api_transport_reset_client(fbe_client_mode_t client_mode);
void FBE_API_CALL fbe_api_transport_reset_client_sp(fbe_sim_transport_connection_target_t target);

fbe_status_t fbe_api_transport_map_server_mode_to_port(fbe_transport_connection_target_t server, const fbe_u8_t **port_string);    
fbe_status_t FBE_API_CALL fbe_api_transport_send_client_notification_packet_to_targeted_server(fbe_packet_t * packet, fbe_transport_connection_target_t server_target);

fbe_status_t FBE_API_CALL fbe_api_transport_set_server_mode_port(fbe_u16_t port_base);

FBE_API_CPP_EXPORT_END

#endif /*FBE_API_TRANSPORT_PACKET_INTERFACE_H*/
