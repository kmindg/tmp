#ifdef UMODE_ENV

#ifndef ALAMOSA_WINDOWS_ENV
#define MY_CONTAINER_ARGS "opt_use_grman=1 opt_use_kpci=0 opt_use_kdisp=0 opt_useif_kci=1 opt_useif_helper=1 opt_use_command_paths=1 opt_use_fast_command_paths=0 opt_use_sig_handlers=1 opt_use_env=0 opt_env_db_path=static:/tmp/nocare opt_no_pci=1 opt_no_cpuk=1 opt_no_cex=1 opt_disable_validation=0 opt_disable_accounting=0 opt_use_watch_dog=0 opt_enable_core_dump=0"
#else
#define MY_CONTAINER_ARGS "opt_use_grman=1 opt_set_proc_name=0 opt_use_kpci=0 opt_use_kdisp=0 opt_useif_kci=1 opt_useif_helper=1 opt_use_command_paths=1 opt_use_fast_command_paths=0 opt_use_sig_handlers=1 opt_use_env=0 opt_disable_validation=1 opt_disable_accounting=0 opt_use_env=1 opt_env_db_path=static:/tmp/nocare opt_use_watch_dog=0 opt_enable_core_dump=0 opt_no_cpuk=1 opt_no_pci=1"
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - config_differences */
#define MY_RT_ARGS "opt_use_realtime_threads=0 opt_trace_all_output=1"

#elif defined(SIMMODE_ENV)
#ifndef ALAMOSA_WINDOWS_ENV
#define MY_CONTAINER_ARGS "opt_xlevel_validation_should_panic=1 opt_use_grman=0 opt_use_kpci=0 opt_use_kdisp=0 opt_useif_kci=1 opt_useif_helper=1 opt_use_command_paths=1 opt_use_fast_command_paths=0 opt_use_sig_handlers=1 opt_use_env=0 opt_env_db_path=static:/tmp/nocare opt_no_pci=1 opt_no_cpuk=1 opt_no_cex=1 opt_disable_validation=0 opt_disable_accounting=0 opt_use_watch_dog=0 opt_enable_core_dump=0"
#else
#define MY_CONTAINER_ARGS "opt_xlevel_validation_should_panic=1 opt_use_grman=1 opt_set_proc_name=0 opt_use_kpci=0 opt_use_kdisp=0 opt_useif_kci=1 opt_useif_helper=1 opt_use_command_paths=1 opt_use_fast_command_paths=0 opt_use_sig_handlers=1 opt_use_env=0 opt_disable_validation=0 opt_disable_accounting=0 opt_use_env=1 opt_env_db_path=static:/tmp/nocare opt_use_watch_dog=0 opt_enable_core_dump=0 opt_no_cpuk=1 opt_no_pci=1"
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - config_differences */
#define MY_RT_ARGS "opt_use_realtime_threads=0 opt_trace_all_output=1"

#endif

emcutil_csx_shell_exe_initialize(MY_CONTAINER_ARGS, MY_RT_ARGS);
