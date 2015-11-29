#ifndef EMCUTIL_CSX_SHELL_INTERFACE_H_
#define EMCUTIL_CSX_SHELL_INTERFACE_H_

//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*! @file  EmcUTIL_CsxShell_Interface.h */
//
// Contents:
//      The master header file for the EMCUTIL Csx Shell.
//
//
// See /catmerge/EmcUTIL/Shell/TestCsxShell for a sample application
// See /catmerge/EmcUTIL/Shell/SampleCsxDll for a sample library

////////////////////////////////////

#include "csx_ext.h"

// TBD - this might be better somewhere else
#define EMCUTIL_SHELL_DEFCC CSX_CDECL   /* default calling convention for shell APIs */

#ifdef __cplusplus
extern "C" {
#endif

/* This function is called from main to initialize CSX and run any global constructors and DLL init rountines */
CSX_MOD_EXPORT void EMCUTIL_SHELL_DEFCC emcutil_csx_shell_exe_initialize_backend(csx_module_context_t *csx_module_context_g_for_exe, csx_cstring_t exe_name, csx_cstring_t container_args_for_exe, csx_cstring_t rt_args);
#define emcutil_csx_shell_exe_initialize(container_args_for_exe, rt_args) emcutil_csx_shell_exe_initialize_backend(&csx_module_context_g, CSX_STRINGIFY(EMCUTIL_CSXSHELL_PROGRAM_NAME), container_args_for_exe, rt_args)

/* This function is called from main to shut down CSX  and run any global destructors and DLL unload rountines */
CSX_MOD_EXPORT void EMCUTIL_SHELL_DEFCC emcutil_csx_shell_exe_deinitialize(void);

/* Setup special definitions for MUT test cases
 */ 
#ifndef ALAMOSA_WINDOWS_ENV
#define MUT_STANDALONE_TEST_DEFAULT_CONTAINER_ARGS "opt_use_grman=0 opt_set_proc_name=0 opt_use_kpci=0 opt_use_kdisp=0 opt_useif_kci=1 opt_useif_helper=1 opt_use_command_paths=1 opt_use_fast_command_paths=0 opt_use_sig_handlers=1 opt_use_env=1 opt_env_db_path=static:/tmp/nocare opt_no_pci=1 opt_no_cpuk=1 opt_no_cex=1 opt_disable_validation=0 opt_disable_accounting=0 opt_use_watch_dog=1 val_watchdog_panic_limit=3 opt_enable_core_dump=0 opt_default_rdevice_peer_ic_name=__self__"
#else
#define MUT_STANDALONE_TEST_DEFAULT_CONTAINER_ARGS "opt_use_grman=0 opt_set_proc_name=0 opt_use_kpci=0 opt_use_kdisp=0 opt_useif_kci=1 opt_useif_helper=1 opt_use_command_paths=1 opt_use_fast_command_paths=0 opt_use_sig_handlers=1 opt_disable_validation=0 opt_disable_accounting=0 opt_use_watch_dog=1 val_watchdog_panic_limit=3 opt_use_env=1 opt_env_db_path=static:/tmp/nocare opt_enable_core_dump=0 opt_default_rdevice_peer_ic_name=__self__"
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - config_differences */

#ifndef EMCUTIL_CSX_SHELL_ADDITIONAL_CONTAINER_ARGS
#define EMCUTIL_CSX_SHELL_ADDITIONAL_CONTAINER_ARGS
#endif
#ifndef EMCUTIL_CSX_SHELL_ADDITIONAL_RT_ARGS
#define EMCUTIL_CSX_SHELL_ADDITIONAL_RT_ARGS
#endif

/* These arguments are used to configure the CSX container.  This is a reasonable set of defaults for our example
 * simple CSX-enabled application
 */ 
#ifdef I_AM_A_MUT_TEST
#define EMCUTIL_CSX_SHELL_DEFAULT_CONTAINER_ARGS MUT_STANDALONE_TEST_DEFAULT_CONTAINER_ARGS EMCUTIL_CSX_SHELL_ADDITIONAL_CONTAINER_ARGS
#define EMCUTIL_CSX_SHELL_DEFAULT_RT_ARGS "opt_do_windows_print_style=1 opt_trace_all_output=1" EMCUTIL_CSX_SHELL_ADDITIONAL_RT_ARGS
#else /* I_AM_A_MUT_TEST */

#if (defined(UMODE_ENV) || defined(I_WANT_CSX_ARGS_LIKE_ON_ARRAY)) && !defined(I_WANT_CSX_ARGS_LIKE_SIMULATION)

/* case where code runs on array */
#ifndef ALAMOSA_WINDOWS_ENV
#ifdef C4_INTEGRATED
#define EMCUTIL_CSX_SHELL_DEFAULT_CONTAINER_ARGS "opt_set_proc_name=0 opt_use_kpci=0 opt_use_kdisp=0 opt_useif_kci=1 opt_useif_helper=1 opt_use_command_paths=0 opt_use_fast_command_paths=1 opt_use_sig_handlers=1 opt_use_env=1 opt_env_db_path=static:/tmp/dummy opt_no_pci=1 opt_no_cpuk=1 opt_no_cex=1 opt_disable_validation=0 opt_disable_accounting=0 opt_disable_peer_register=1 opt_enable_core_dump=1 val_dump_type=os" EMCUTIL_CSX_SHELL_ADDITIONAL_CONTAINER_ARGS
#define EMCUTIL_CSX_SHELL_DEFAULT_RT_ARGS "opt_do_windows_print_style=1 opt_trace_all_output=1" EMCUTIL_CSX_SHELL_ADDITIONAL_RT_ARGS
#else /* C4_INTEGRATED */
#define EMCUTIL_CSX_SHELL_DEFAULT_CONTAINER_ARGS "opt_set_proc_name=0 opt_use_kpci=0 opt_use_kdisp=0 opt_useif_kci=1 opt_useif_helper=1 opt_use_command_paths=0 opt_use_fast_command_paths=0 opt_use_sig_handlers=1 opt_use_env=1 opt_env_db_path=static:/tmp/dummy opt_no_pci=1 opt_no_cpuk=1 opt_no_cex=1 opt_disable_validation=0 opt_disable_accounting=0 opt_disable_peer_register=1 opt_enable_core_dump=1" EMCUTIL_CSX_SHELL_ADDITIONAL_CONTAINER_ARGS
#define EMCUTIL_CSX_SHELL_DEFAULT_RT_ARGS "opt_do_windows_print_style=1 opt_trace_all_output=1" EMCUTIL_CSX_SHELL_ADDITIONAL_RT_ARGS
#endif /* C4_INTEGRATED */
#else
#define EMCUTIL_CSX_SHELL_DEFAULT_CONTAINER_ARGS "opt_set_proc_name=0 opt_use_kpci=0 opt_use_kdisp=0 opt_useif_kci=1 opt_use_helper=0 opt_use_command_paths=0 opt_use_fast_command_paths=0 opt_use_sig_handlers=0 opt_use_env=0 opt_enable_core_dump=0 opt_use_grman=1 opt_no_pci=1 opt_no_cpuk=1 opt_enable_core_dump=0 opt_disable_validation=1" EMCUTIL_CSX_SHELL_ADDITIONAL_CONTAINER_ARGS
#define EMCUTIL_CSX_SHELL_DEFAULT_RT_ARGS "opt_do_windows_print_style=1 opt_trace_all_output=1" EMCUTIL_CSX_SHELL_ADDITIONAL_RT_ARGS
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - config_differences */

#else /* (defined(UMODE_ENV) || defined(I_WANT_CSX_ARGS_LIKE_ON_ARRAY)) && !defined(I_WANT_CSX_ARGS_LIKE_SIMULATION) */

/* case where code runs off array */
#ifndef ALAMOSA_WINDOWS_ENV
#define EMCUTIL_CSX_SHELL_DEFAULT_CONTAINER_ARGS "opt_set_proc_name=0 opt_use_kpci=0 opt_use_kdisp=0 opt_useif_kci=1 opt_useif_helper=1 opt_use_command_paths=1 opt_use_fast_command_paths=1 opt_use_sig_handlers=1 opt_use_env=1 opt_env_db_path=static:/tmp/dummy opt_no_pci=1 opt_no_cpuk=1 opt_no_cex=1 opt_disable_validation=0 opt_disable_accounting=0 opt_enable_core_dump=0" EMCUTIL_CSX_SHELL_ADDITIONAL_CONTAINER_ARGS
#define EMCUTIL_CSX_SHELL_DEFAULT_RT_ARGS "opt_do_windows_print_style=1 opt_trace_all_output=1" EMCUTIL_CSX_SHELL_ADDITIONAL_RT_ARGS
#else
#define EMCUTIL_CSX_SHELL_DEFAULT_CONTAINER_ARGS "opt_set_proc_name=0 opt_use_kpci=0 opt_use_kdisp=0 opt_use_kci=0 opt_use_helper=0 opt_use_command_paths=1 opt_use_fast_command_paths=0 opt_use_sig_handlers=0 opt_use_env=0 opt_enable_core_dump=0 opt_disable_validation=0" EMCUTIL_CSX_SHELL_ADDITIONAL_CONTAINER_ARGS
#define EMCUTIL_CSX_SHELL_DEFAULT_RT_ARGS "opt_do_windows_print_style=1 opt_trace_all_output=1" EMCUTIL_CSX_SHELL_ADDITIONAL_RT_ARGS
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - config_differences */

#endif /* (defined(UMODE_ENV) || defined(I_WANT_CSX_ARGS_LIKE_ON_ARRAY)) && !defined(I_WANT_CSX_ARGS_LIKE_SIMULATION) */

#endif /* I_AM_A_MUT_TEST */


/*
 *  These are the user's load/unload handlers for our DLL (provides equivalent to DllMain).
 *  In our SampleCsxDll example they are defined in dll.cpp
 */
csx_bool_t EMCUTIL_SHELL_DEFCC emcutil_dll_startup(void);
csx_bool_t EMCUTIL_SHELL_DEFCC emcutil_dll_shutdown(void);

/*
 *  These are some simple means for users to intercept EmcUTIL Shell world log and error output
 */
typedef void (*emcutil_world_log_handler_callback_t)(void *context, const char *format, va_list ap);
typedef void (*emcutil_world_error_handler_callback_t)(void *context, const char *format, va_list ap);

CSX_MOD_EXPORT void EMCUTIL_SHELL_DEFCC
emcutil_world_log_handler_callback_set(void *context, emcutil_world_log_handler_callback_t callback);

CSX_MOD_EXPORT void EMCUTIL_SHELL_DEFCC
emcutil_world_error_handler_callback_set(void *context, emcutil_world_error_handler_callback_t callback);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class Emcutil_Shell_MainApp
{
public:
    Emcutil_Shell_MainApp(csx_cstring_t container_args = EMCUTIL_CSX_SHELL_DEFAULT_CONTAINER_ARGS, csx_cstring_t rt_args = EMCUTIL_CSX_SHELL_DEFAULT_RT_ARGS) {
        emcutil_csx_shell_exe_initialize(container_args, rt_args);
    }
    ~Emcutil_Shell_MainApp() {
        emcutil_csx_shell_exe_deinitialize();
    }
};
#endif

//++
//.End file EmcUTIL_CsxShell_Interface.h
//--

#endif                                     /* EMCUTIL_CSX_SHELL_INTERFACE_H_ */
