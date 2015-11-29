#ifndef FBE_API_TRANSPORT_H
#define FBE_API_TRANSPORT_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"

FBE_API_CPP_EXPORT_START

typedef enum fbe_transport_connection_target_e{
	FBE_TRANSPORT_INVALID_SERVER = 0,
	FBE_TRANSPORT_SP_A,
	FBE_TRANSPORT_SP_B,
	FBE_TRANSPORT_ADMIN_INTERFACE_PACKAGE,
	FBE_TRANSPORT_LAST_CONNECTION_TARGET
}fbe_transport_connection_target_t;

typedef enum fbe_user_server_control_code_e {
    FBE_USER_SERVER_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_USER_SERVER),
    FBE_USER_SERVER_CONTROL_CODE_GET_PID,
    FBE_USER_SERVER_CONTROL_CODE_REGISTER_NTIFICATIONS,
    FBE_USER_SERVER_CONTROL_CODE_SET_DRIVER_ENTRIES,
    FBE_USER_SERVER_CONTROL_CODE_LAST
} fbe_user_server_control_code_t;

typedef enum fbe_transport_server_mode_e{
    FBE_TRANSPORT_SERVER_MODE_INVALID,
    FBE_TRANSPORT_SERVER_MODE_SIM,
    FBE_TRANSPORT_SERVER_MODE_USER,
    FBE_TRANSPORT_SERVER_MODE_LAST
}fbe_transport_server_mode_t;

fbe_status_t FBE_API_CALL fbe_api_transport_send_buffer(fbe_u8_t * packet,
											   fbe_u32_t length,
											   fbe_packet_completion_function_t completion_function,
											   fbe_packet_completion_context_t  completion_context,
											   fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_transport_send_notification_buffer(fbe_u8_t * packet,
                                                 fbe_u32_t length,
                                                 fbe_packet_completion_function_t completion_function,
                                                 fbe_packet_completion_context_t  completion_context,
                                                 fbe_package_id_t package_id,
                                                 fbe_transport_connection_target_t	server_target);

fbe_status_t FBE_API_CALL fbe_api_transport_init_client(const char *server_name, fbe_transport_connection_target_t connect_to_sp,
                                                                                fbe_bool_t notification_enable, fbe_u32_t connect_retry_times);
fbe_status_t FBE_API_CALL fbe_api_transport_init_client_connection(const char *server_name, fbe_transport_connection_target_t connect_to_sp,
                                                                                fbe_u32_t connect_retry_times);
fbe_status_t FBE_API_CALL fbe_api_transport_control_init_client_control(fbe_transport_connection_target_t connect_to_sp);

fbe_status_t FBE_API_CALL fbe_api_transport_init_server(fbe_transport_connection_target_t sp_mode);
fbe_status_t FBE_API_CALL fbe_api_transport_control_init_server_control (void);
fbe_status_t FBE_API_CALL fbe_api_transport_register_notifications(fbe_package_notification_id_mask_t package_mask);

fbe_status_t FBE_API_CALL fbe_api_transport_destroy_server(void);
fbe_status_t FBE_API_CALL fbe_api_transport_destroy_client(fbe_transport_connection_target_t connect_to_sp);
fbe_status_t FBE_API_CALL fbe_api_transport_destroy_client_connection(fbe_transport_connection_target_t connect_to_sp);
fbe_status_t FBE_API_CALL fbe_api_transport_set_target_server(fbe_transport_connection_target_t target);
fbe_transport_connection_target_t FBE_API_CALL fbe_api_transport_get_target_server(void);
fbe_status_t fbe_api_transport_set_unregister_on_connect(fbe_bool_t b_value);
fbe_bool_t FBE_API_CALL fbe_api_transport_is_target_initted(fbe_transport_connection_target_t target);

typedef fbe_u32_t fbe_transport_application_id_t;

typedef struct fbe_api_transport_register_notification_s{
	fbe_transport_application_id_t	application_id;
}fbe_api_transport_register_notification_t;

/* for FBE_SERVER_CONTROL_CODE_GET_PID */
typedef fbe_u64_t fbe_api_transport_server_pid;
typedef struct fbe_api_transport_server_get_pid_s{
	fbe_api_transport_server_pid	pid;
}fbe_api_transport_server_get_pid_t;

FBE_API_CPP_EXPORT_END

#endif /*FBE_API_TRANSPORT_H*/
