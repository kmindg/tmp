#ifndef FBE_API_SIM_SERVER_H
#define FBE_API_SIM_SERVER_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_transport.h"

FBE_API_CPP_EXPORT_START

fbe_status_t FBE_API_CALL fbe_api_sim_server_load_package(fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_sim_server_init_fbe_api(void);
fbe_status_t FBE_API_CALL fbe_api_sim_server_unload_package(fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_sim_server_disable_package(fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_sim_server_set_package_entries(fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_sim_server_set_package_entries_no_wait(fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_sim_server_wait_for_database_service(void);
fbe_status_t FBE_API_CALL fbe_api_sim_server_get_pid(fbe_api_sim_server_pid *pid);
fbe_status_t FBE_API_CALL fbe_api_sim_server_suite_start_notification(const char *log_path, const char *log_file_name);
fbe_status_t FBE_API_CALL fbe_api_sim_server_get_windows_cpu_utilization(fbe_api_sim_server_get_cpu_util_t *cpu_util);
fbe_status_t FBE_API_CALL fbe_api_sim_server_load_package_params(fbe_package_id_t package_id,
                                                                 void *buffer_p,
                                                                 fbe_u32_t buffer_size);
fbe_status_t FBE_API_CALL fbe_api_sim_server_get_ica_status(fbe_bool_t *is_ica_complete);

FBE_API_CPP_EXPORT_END


#endif /*FBE_API_SIM_SERVER_H*/
