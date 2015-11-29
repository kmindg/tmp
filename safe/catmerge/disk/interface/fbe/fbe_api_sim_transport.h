#ifndef FBE_API_SIM_TRANSPORT_H
#define FBE_API_SIM_TRANSPORT_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"

FBE_API_CPP_EXPORT_START

#define FBE_API_SIM_TRANSPORT_STRING_LEN 512

typedef enum fbe_sim_transport_connection_target_e{
	FBE_SIM_INVALID_SERVER = 0,
	FBE_SIM_SP_A,
	FBE_SIM_SP_B,
	FBE_SIM_ADMIN_INTERFACE_PACKAGE,
	FBE_SIM_LAST_CONNECTION_TARGET
}fbe_sim_transport_connection_target_t;

typedef enum fbe_sim_server_control_code_e {
	FBE_SIM_SERVER_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_SIM_SERVER),
	FBE_SIM_SERVER_CONTROL_CODE_LOAD_PACKAGE,
	FBE_SIM_SERVER_CONTROL_CODE_INIT_FBE_API,
	FBE_SIM_SERVER_CONTROL_CODE_DESTROY_FBE_API,
	FBE_SIM_SERVER_CONTROL_CODE_SET_PACKAGE_ENTRIES,
	FBE_SIM_SERVER_CONTROL_CODE_UNLOAD_PACKAGE,
	FBE_SIM_SERVER_CONTROL_CODE_GET_PID,
	FBE_SIM_SERVER_CONTROL_CODE_SUITE_START_NOTIFICATION,
	FBE_SIM_SERVER_CONTROL_CODE_UNREGISTER_ALL_APPS,
    FBE_SIM_SERVER_CONTROL_CODE_GET_WINDOWS_CPU_UTILIZATION,
    FBE_SIM_SERVER_CONTROL_CODE_DISABLE_PACKAGE,
    FBE_SIM_SERVER_CONTROL_CODE_GET_ICA_STATUS,
	FBE_SIM_SERVER_CONTROL_CODE_LAST
} fbe_sim_server_control_code_t;


fbe_status_t FBE_API_CALL fbe_api_sim_transport_send_buffer(fbe_u8_t * packet,
											   fbe_u32_t length,
											   fbe_packet_completion_function_t completion_function,
											   fbe_packet_completion_context_t  completion_context,
											   fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_sim_transport_send_notification_buffer(fbe_u8_t * packet,
                                                 fbe_u32_t length,
                                                 fbe_packet_completion_function_t completion_function,
                                                 fbe_packet_completion_context_t  completion_context,
                                                 fbe_package_id_t package_id,
                                                 fbe_sim_transport_connection_target_t	server_target);

fbe_status_t FBE_API_CALL fbe_api_sim_transport_init_client(fbe_sim_transport_connection_target_t connect_to_sp, fbe_bool_t notification_enable);
fbe_status_t FBE_API_CALL fbe_api_sim_transport_init_client_connection(fbe_sim_transport_connection_target_t connect_to_sp);
fbe_status_t FBE_API_CALL fbe_api_sim_transport_control_init_client_control(fbe_sim_transport_connection_target_t connect_to_sp);

fbe_status_t FBE_API_CALL fbe_api_sim_transport_init_server(fbe_sim_transport_connection_target_t sp_mode);
fbe_status_t FBE_API_CALL fbe_api_sim_transport_control_init_server_control (void);
fbe_status_t FBE_API_CALL fbe_api_sim_transport_register_notifications(fbe_package_notification_id_mask_t package_mask);

fbe_status_t FBE_API_CALL fbe_api_sim_transport_destroy_server(void);
fbe_status_t FBE_API_CALL fbe_api_sim_transport_destroy_client(fbe_sim_transport_connection_target_t connect_to_sp);
fbe_status_t FBE_API_CALL fbe_api_sim_transport_destroy_client_connection(fbe_sim_transport_connection_target_t connect_to_sp);
fbe_status_t FBE_API_CALL fbe_api_sim_transport_set_target_server(fbe_sim_transport_connection_target_t target);
fbe_sim_transport_connection_target_t FBE_API_CALL fbe_api_sim_transport_get_target_server(void);

typedef fbe_u32_t fbe_sim_transport_application_id_t;

typedef struct fbe_api_sim_transport_register_notification_s{
	fbe_sim_transport_application_id_t	application_id;
}fbe_api_sim_transport_register_notification_t;

/* for FBE_SIM_SERVER_CONTROL_CODE_GET_PID */
typedef fbe_u64_t fbe_api_sim_server_pid;
typedef struct fbe_api_sim_server_get_pid_s{
	fbe_api_sim_server_pid	pid;
}fbe_api_sim_server_get_pid_t;

typedef struct fbe_api_sim_server_suite_start_notification_s{
    char  log_path[FBE_API_SIM_TRANSPORT_STRING_LEN];
    char  log_file_name[FBE_API_SIM_TRANSPORT_STRING_LEN];
}fbe_api_sim_server_suite_start_notification_t;

/* FBE_SIM_SERVER_CONTROL_CODE_GET_WINDOWS_CPU_UTILIZATION */
typedef struct fbe_api_sim_server_get_windows_cpu_utilization_s{
    char typeperf_str[1024];
}fbe_api_sim_server_get_windows_cpu_utilization_t;

typedef struct fbe_api_sim_server_get_cpu_util_s{ //return struct the same for windows and linux
    fbe_u32_t core_count; 
    fbe_u32_t cpu_usage[FBE_CPU_ID_MAX];
}fbe_api_sim_server_get_cpu_util_t;

/* FBE_SIM_SERVER_CONTROL_CODE_GET_ICA_STATUS */
typedef struct fbe_api_sim_server_get_ica_util_s{
    fbe_bool_t is_ica_done;
}fbe_api_sim_server_get_ica_util_t;

FBE_API_CPP_EXPORT_END

#endif /*FBE_API_SIM_TRANSPORT_H*/
