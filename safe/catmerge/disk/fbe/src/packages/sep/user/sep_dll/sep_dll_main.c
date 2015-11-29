#include "windows.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_sep.h"
#include "fbe/fbe_topology_interface.h"

#include "sep_dll.h"
#include "fbe_cmi_sim.h"
#include "fbe/fbe_sep_shim_simulation_interface.h"
#include "EmcUTIL_CsxShell_Interface.h"

#if 0 /*CMS disabled*/
#include "../../../../services/cluster_memory/interface/memory/fbe_cms_memory.h"
#endif

CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_init(fbe_sep_package_load_params_t *params_p)
{
	fbe_status_t status;
	status = sep_init(params_p);
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_destroy(void)
{
	fbe_status_t status;
	status = sep_destroy();
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_get_control_entry(fbe_service_control_entry_t * service_control_entry)
{
	fbe_status_t status;
	status = sep_get_control_entry(service_control_entry);
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_set_physical_package_control_entry(fbe_service_control_entry_t control_entry)
{
	fbe_status_t status;
	status = fbe_service_manager_set_physical_package_control_entry(control_entry);
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_set_neit_package_control_entry(fbe_service_control_entry_t control_entry)
{
	fbe_status_t status;
	status = fbe_service_manager_set_neit_control_entry(control_entry);
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_get_io_entry(fbe_io_entry_function_t * io_entry)
{
	fbe_status_t status;
	status = sep_get_io_entry(io_entry);

    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_set_physical_package_io_entry(fbe_io_entry_function_t io_entry)
{
	fbe_status_t status;
	status = fbe_topology_set_physical_package_io_entry(io_entry);
    return status;
}

csx_bool_t EMCUTIL_SHELL_DEFCC emcutil_dll_startup(void)
{
    return CSX_TRUE;
}

csx_bool_t EMCUTIL_SHELL_DEFCC emcutil_dll_shutdown(void)
{
    return CSX_TRUE;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  
sep_dll_set_terminator_api_get_sp_id(fbe_terminator_api_get_sp_id_function_t function)
{
	fbe_status_t status;	
	status = fbe_cmi_sim_set_get_sp_id_func(function);
	return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl
sep_dll_set_terminator_api_get_cmi_port_base(fbe_terminator_api_get_cmi_port_base_function_t function)
{
    fbe_status_t status;
    status = fbe_cmi_sim_set_get_cmi_port_base_func(function);
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_get_sep_shim_lun_info_entry(fbe_sep_shim_sim_map_device_name_to_block_edge_func_t * lun_map_entry_ptr)
{
	*lun_map_entry_ptr = fbe_sep_shim_sim_map_device_name_to_block_edge;
    return FBE_STATUS_OK;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_get_sep_shim_cache_io_entry(fbe_sep_shim_sim_cache_io_entry_t * cache_io_entry_ptr)
{
	*cache_io_entry_ptr = fbe_sep_shim_sim_cache_io_entry;
    return FBE_STATUS_OK;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_set_esp_package_control_entry(fbe_service_control_entry_t control_entry)
{
	fbe_status_t status;
	status = fbe_service_manager_set_esp_package_control_entry(control_entry);
    return status;
}

#if 0 /*CMS disbaled*/
CSX_MOD_EXPORT fbe_status_t __cdecl  sep_dll_set_memory_persistence_pointers(fbe_cms_memory_persistence_pointers_t * pointers)
{
	fbe_status_t status;
	status = fbe_cms_memory_set_memory_persistence_pointers(pointers);
    return status;
}
#endif
