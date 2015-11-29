#ifndef ESP_DLL_H
#define ESP_DLL_H

#include "fbe/fbe_service.h"
#include "fbe/fbe_service_manager_interface.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_terminator_api.h"

typedef fbe_status_t (CALLBACK* esp_dll_init_function_t)(void);
CSX_MOD_EXPORT fbe_status_t __cdecl esp_dll_init(void);

typedef fbe_status_t (CALLBACK* esp_dll_destroy_function_t)(void);
CSX_MOD_EXPORT fbe_status_t __cdecl esp_dll_destroy(void);

typedef fbe_status_t (CALLBACK* esp_dll_get_control_entry_function_t)(fbe_service_control_entry_t * service_control_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  esp_dll_get_control_entry(fbe_service_control_entry_t * service_control_entry);

typedef fbe_status_t (CALLBACK* esp_dll_get_io_entry_function_t)(fbe_io_entry_function_t * io_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  esp_dll_get_io_entry(fbe_io_entry_function_t * io_entry);

typedef fbe_status_t (CALLBACK* esp_dll_set_physical_package_control_entry_function_t)(fbe_service_control_entry_t control_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  esp_dll_set_physical_package_control_entry(fbe_service_control_entry_t control_entry);

typedef fbe_status_t (CALLBACK* esp_dll_set_physical_package_io_entry_function_t)(fbe_io_entry_function_t io_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  esp_dll_set_physical_package_io_entry(fbe_io_entry_function_t io_entry);

typedef fbe_status_t (CALLBACK* esp_dll_set_sep_package_control_entry_function_t)(fbe_service_control_entry_t control_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  esp_dll_set_sep_package_control_entry(fbe_service_control_entry_t control_entry);

typedef fbe_status_t (CALLBACK* esp_dll_set_terminator_api_get_sp_id_function_t)(fbe_terminator_api_get_sp_id_function_t function);
CSX_MOD_EXPORT fbe_status_t __cdecl  esp_dll_set_terminator_api_get_sp_id(fbe_terminator_api_get_sp_id_function_t function);

typedef fbe_status_t (CALLBACK* esp_dll_set_terminator_api_get_cmi_port_base_function_t)(fbe_terminator_api_get_cmi_port_base_function_t function);
CSX_MOD_EXPORT fbe_status_t __cdecl  esp_dll_set_terminator_api_get_cmi_port_base(fbe_terminator_api_get_cmi_port_base_function_t function);

#endif /* ESP_DLL_H */
