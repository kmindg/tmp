#ifndef SEP_DLL_H
#define SEP_DLL_H

#include "fbe/fbe_service.h"
#include "fbe/fbe_service_manager_interface.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_sep_shim_simulation_interface.h"
#include "fbe/fbe_sep.h"
#include "../src/services/cluster_memory/interface/memory/fbe_cms_memory.h"


typedef fbe_status_t (CALLBACK* sep_dll_init_function_t)(fbe_sep_package_load_params_t *params_p);
CSX_MOD_EXPORT fbe_status_t __cdecl sep_dll_init(fbe_sep_package_load_params_t *params_p);

typedef fbe_status_t (CALLBACK* sep_dll_destroy_function_t)(void);
CSX_MOD_EXPORT fbe_status_t __cdecl sep_dll_destroy(void);

typedef fbe_status_t (CALLBACK* sep_dll_get_control_entry_function_t)(fbe_service_control_entry_t * service_control_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_get_control_entry(fbe_service_control_entry_t * service_control_entry);

typedef fbe_status_t (CALLBACK* sep_dll_get_io_entry_function_t)(fbe_io_entry_function_t * io_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_get_io_entry(fbe_io_entry_function_t * io_entry);

typedef fbe_status_t (CALLBACK* sep_dll_set_physical_package_control_entry_function_t)(fbe_service_control_entry_t control_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_set_physical_package_control_entry(fbe_service_control_entry_t control_entry);

typedef fbe_status_t (CALLBACK* sep_dll_set_neit_package_control_entry_function_t)(fbe_service_control_entry_t control_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_set_neit_package_control_entry(fbe_service_control_entry_t control_entry);

typedef fbe_status_t (CALLBACK* sep_dll_set_physical_package_io_entry_function_t)(fbe_io_entry_function_t io_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_set_physical_package_io_entry(fbe_io_entry_function_t io_entry);

typedef fbe_status_t (CALLBACK* sep_dll_set_terminator_api_get_sp_id_function_t)(fbe_terminator_api_get_sp_id_function_t function);
CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_set_terminator_api_get_sp_id(fbe_terminator_api_get_sp_id_function_t function);

typedef fbe_status_t (CALLBACK* sep_dll_set_terminator_api_get_cmi_port_base_function_t)(fbe_terminator_api_get_cmi_port_base_function_t function);
CSX_MOD_EXPORT fbe_status_t __cdecl sep_dll_set_terminator_api_get_cmi_port_base(fbe_terminator_api_get_cmi_port_base_function_t function);

typedef fbe_status_t (CALLBACK* sep_dll_get_sep_shim_lun_info_entry_t)(fbe_sep_shim_sim_map_device_name_to_block_edge_func_t *lun_map_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_get_sep_shim_lun_info_entry(fbe_sep_shim_sim_map_device_name_to_block_edge_func_t *lun_map_entry);

typedef fbe_status_t (CALLBACK* sep_dll_get_sep_cache_io_entry_t)(fbe_sep_shim_sim_cache_io_entry_t *cache_io_entry_ptr);
CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_get_sep_shim_cache_io_entry(fbe_sep_shim_sim_cache_io_entry_t * cache_io_entry_ptr);

typedef fbe_status_t (CALLBACK* sep_dll_set_esp_package_control_entry_function_t)(fbe_service_control_entry_t control_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_set_esp_package_control_entry(fbe_service_control_entry_t control_entry);

typedef fbe_status_t (CALLBACK* sep_dll_set_memory_persistence_pointers_function_t)(fbe_cms_memory_persistence_pointers_t * pointers);
CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_set_memory_persistence_pointers(fbe_cms_memory_persistence_pointers_t * pointers);

#endif /* SEP_DLL_H */
