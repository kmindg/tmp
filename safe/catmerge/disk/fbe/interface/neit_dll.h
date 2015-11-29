#ifndef NEIT_DLL_H
#define NEIT_DLL_H

#include "fbe/fbe_service.h"
#include "fbe/fbe_service_manager_interface.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_neit.h"

typedef fbe_status_t (CALLBACK* neit_dll_init_function_t)(fbe_neit_package_load_params_t *params_p);
CSX_MOD_EXPORT fbe_status_t __cdecl neit_dll_init(fbe_neit_package_load_params_t *params_p);

typedef fbe_status_t (CALLBACK* neit_dll_destroy_function_t)(void);
CSX_MOD_EXPORT fbe_status_t __cdecl neit_dll_destroy(void);

typedef fbe_status_t (CALLBACK* neit_dll_disable_function_t)(void);
CSX_MOD_EXPORT fbe_status_t __cdecl neit_dll_disable(void);

typedef fbe_status_t (CALLBACK* neit_dll_get_control_entry_function_t)(fbe_service_control_entry_t * service_control_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  neit_dll_get_control_entry(fbe_service_control_entry_t * service_control_entry);

typedef fbe_status_t (CALLBACK* neit_dll_set_physical_package_control_entry_function_t)(fbe_service_control_entry_t control_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  neit_dll_set_physical_package_control_entry(fbe_service_control_entry_t control_entry);

typedef fbe_status_t (CALLBACK* neit_dll_set_physical_package_io_entry_function_t)(fbe_service_control_entry_t control_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  neit_dll_set_physical_package_io_entry(fbe_service_control_entry_t control_entry);

typedef fbe_status_t (CALLBACK* neit_dll_set_sep_io_entry_function_t)(fbe_service_control_entry_t control_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  neit_dll_set_sep_io_entry(fbe_service_control_entry_t control_entry);

typedef fbe_status_t (CALLBACK* neit_dll_set_sep_control_entry_function_t)(fbe_service_control_entry_t control_entry);
CSX_MOD_EXPORT fbe_status_t __cdecl  neit_dll_set_sep_control_entry(fbe_service_control_entry_t control_entry);

typedef fbe_status_t (CALLBACK* neit_dll_set_terminator_api_get_sp_id_function_t)(fbe_terminator_api_get_sp_id_function_t function);
CSX_MOD_EXPORT fbe_status_t __cdecl  neit_dll_set_terminator_api_get_sp_id(fbe_terminator_api_get_sp_id_function_t function);

typedef fbe_status_t (CALLBACK* neit_dll_set_terminator_api_get_cmi_port_base_function_t)(fbe_terminator_api_get_cmi_port_base_function_t function);
CSX_MOD_EXPORT fbe_status_t __cdecl neit_dll_set_terminator_api_get_cmi_port_base(fbe_terminator_api_get_cmi_port_base_function_t function);
#endif /* NEIT_DLL_H */
