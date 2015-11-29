#include "windows.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_physical_package.h"
#include "fbe_cpd_shim.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_base_board.h"
#include "EmcUTIL_CsxShell_Interface.h"
#include "csx_build_defines.h"
#include "csx_ext_p_shims.h"


CSX_MOD_EXPORT fbe_status_t __cdecl  physical_package_dll_init(void)
{
	fbe_status_t status;
	status = physical_package_init();
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  physical_package_dll_destroy(void)
{
	fbe_status_t status;
	status = physical_package_destroy();
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  physical_package_dll_get_control_entry(fbe_service_control_entry_t * service_control_entry)
{
	fbe_status_t status;
	status = physical_package_get_control_entry(service_control_entry);
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  
physical_package_dll_set_miniport_interface_port_shim_sim_pointers(struct fbe_terminator_miniport_interface_port_shim_sim_pointers_s * miniport_pointers)
{
	fbe_status_t status;
	status = fbe_cpd_shim_sim_set_terminator_miniport_pointers(miniport_pointers);
	return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  
physical_package_dll_set_terminator_api_get_board_info(fbe_terminator_api_get_board_info_function_t function)
{
	fbe_status_t status;	
	status = fbe_base_board_sim_set_terminator_api_get_board_info_function(function);
	return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  
physical_package_dll_set_terminator_api_get_sp_id(fbe_terminator_api_get_sp_id_function_t function)
{
	fbe_status_t status;	
	status = fbe_base_board_sim_set_terminator_api_get_sp_id_function(function);
	return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl
physical_package_dll_set_terminator_api_is_single_sp_system(fbe_terminator_api_is_single_sp_system_function_t function)
{
	fbe_status_t status;
	status = fbe_base_board_sim_set_terminator_api_is_single_sp_system_function(function);
	return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl
physical_package_dll_set_terminator_api_process_specl_sfi_mask_data_queue_function(fbe_terminator_api_process_specl_sfi_mask_data_queue_function_t function)
{
    fbe_status_t status;
    status = fbe_base_board_sim_set_terminator_api_process_specl_sfi_mask_data_queue_function(function);
    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  physical_package_dll_get_io_entry(fbe_io_entry_function_t * io_entry)
{
	fbe_status_t status;
	status = physical_package_get_io_entry(io_entry);

    return status;
}

CSX_MOD_EXPORT fbe_status_t __cdecl  physical_package_dll_get_specl_sfi_entry(speclSFIEntryType * specl_sfi_entry)
{
    fbe_status_t status;
    status = physical_package_get_specl_sfi_entry(specl_sfi_entry);

    return status;
}

/*
 * put it here temporary because kernel mode SFI entry is not ready.
 */
fbe_status_t
physical_package_get_specl_sfi_entry(speclSFIEntryType * specl_sfi_entry)
{
    * specl_sfi_entry = speclSFIGetNSetCacheData;
    return FBE_STATUS_OK;
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
	*package_id = FBE_PACKAGE_ID_PHYSICAL;
	return FBE_STATUS_OK;
}
