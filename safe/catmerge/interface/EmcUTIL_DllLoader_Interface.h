#ifndef EMCUTIL_DLL_LOADER_INTERFACE_H_
#define EMCUTIL_DLL_LOADER_INTERFACE_H_

//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*! @file  EmcUTIL_DllLoader_Interface.h */
//
// Contents:
//      The master header file for the EMCUTIL DllLoader.
//
//

////////////////////////////////////

#include "csx_ext.h"
#include "EmcUTIL_CsxShell_Interface.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
   csx_pvoid_t shlib_os_handle;
   csx_module_context_t module_context;
   csx_p_imported_global_handle_t module_handle;
   csx_bool_t is_dynamic;
   csx_bool_t is_an_emcutil_dynlink;
   csx_bool_t is_a_csx_module;
   csx_dlist_entry_t persist_list_entry;
} EMCUTIL_DLL_LOADER_PARAMS, *PEMCUTIL_DLL_LOADER_PARAMS;


CSX_MOD_EXPORT csx_status_e EMCUTIL_SHELL_DEFCC emcutil_dll_loader_load(csx_cstring_t module_name,
                                                   csx_bool_t persist_dll,
                                                   PEMCUTIL_DLL_LOADER_PARAMS params_rv);

CSX_MOD_EXPORT csx_status_e EMCUTIL_SHELL_DEFCC emcutil_dll_loader_lookup(csx_cstring_t function_name_to_lookup,
                                                     csx_pvoid_t* returned_function_ptr,
                                                     PEMCUTIL_DLL_LOADER_PARAMS params);

CSX_MOD_EXPORT csx_status_e EMCUTIL_SHELL_DEFCC emcutil_dll_loader_unload(PEMCUTIL_DLL_LOADER_PARAMS params);

/* simple convenience wrappers */

typedef void * EMCUTIL_DLL_HANDLE;

CSX_MOD_EXPORT EMCUTIL_DLL_HANDLE EMCUTIL_SHELL_DEFCC emcutil_simple_dll_load(const char *dll_name);

CSX_MOD_EXPORT void * EMCUTIL_SHELL_DEFCC emcutil_simple_dll_lookup(EMCUTIL_DLL_HANDLE dll_handle, const char *function_name);

CSX_MOD_EXPORT void EMCUTIL_SHELL_DEFCC emcutil_simple_dll_unload(EMCUTIL_DLL_HANDLE dll_handle);

CSX_MOD_EXPORT EMCUTIL_DLL_HANDLE EMCUTIL_SHELL_DEFCC emcutil_dll_loader_instantiate_native_dll(const char *dll_name);

CSX_MOD_EXPORT void EMCUTIL_SHELL_DEFCC emcutil_dll_loader_deinstantiate_native_dll(EMCUTIL_DLL_HANDLE dll_handle);

CSX_MOD_EXPORT void EMCUTIL_SHELL_DEFCC emcutil_shell_delay_load_init(const char *csx_ci_args, const char *csx_rt_args, void ** module_context_rv);

#ifdef __cplusplus
}
#endif

//++
//.End file EmcUTIL_DllLoader_Interface.h
//--

#endif                                     /* EMCUTIL_DLL_LOADER_INTERFACE_H_ */
