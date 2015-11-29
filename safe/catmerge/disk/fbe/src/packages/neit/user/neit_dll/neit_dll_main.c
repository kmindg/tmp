#include "windows.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_neit_package.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_terminator_api.h"

#include "neit_dll.h"
#include "fbe_cmi_sim.h"
#include "EmcUTIL_CsxShell_Interface.h"

CSX_MOD_EXPORT fbe_status_t __cdecl  neit_dll_init(fbe_neit_package_load_params_t *params_p)
{
    fbe_status_t status;
    status = neit_package_init(params_p);
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  neit_dll_destroy(void)
{
    fbe_status_t status;
    status = neit_package_destroy();
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  neit_dll_disable(void)
{
    fbe_status_t status;
    status = neit_package_disable();
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  neit_dll_get_control_entry(fbe_service_control_entry_t * service_control_entry)
{
    fbe_status_t status;
    status = neit_package_get_control_entry(service_control_entry);
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  neit_dll_set_physical_package_control_entry(fbe_service_control_entry_t control_entry)
{
    fbe_status_t status;
    status = fbe_service_manager_set_physical_package_control_entry(control_entry);
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  neit_dll_set_sep_control_entry(fbe_service_control_entry_t control_entry)
{
    fbe_status_t status;
    status = fbe_service_manager_set_sep_control_entry(control_entry);
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  neit_dll_set_physical_package_io_entry(fbe_io_entry_function_t io_entry)
{
    fbe_status_t status;
    status = fbe_topology_set_physical_package_io_entry(io_entry);
    return status;
}
CSX_MOD_EXPORT fbe_status_t __cdecl  neit_dll_set_sep_io_entry(fbe_io_entry_function_t io_entry)
{
    fbe_status_t status;
    status = fbe_topology_set_sep_io_entry(io_entry);
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

fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
	*package_id = FBE_PACKAGE_ID_NEIT;
	return FBE_STATUS_OK;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  
neit_dll_set_terminator_api_get_sp_id(fbe_terminator_api_get_sp_id_function_t function)
{
	fbe_status_t status;	
	status = fbe_cmi_sim_set_get_sp_id_func(function);
	return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl
neit_dll_set_terminator_api_get_cmi_port_base(fbe_terminator_api_get_cmi_port_base_function_t function)
{
    fbe_status_t status;
    status = fbe_cmi_sim_set_get_cmi_port_base_func(function);
    return status;
}
