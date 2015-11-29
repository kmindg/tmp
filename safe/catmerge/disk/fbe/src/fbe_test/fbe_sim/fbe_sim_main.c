#include "fbe_sim.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_package.h"
#include "fbe_trace.h"
#include "fbe/fbe_trace_interface.h"
#include <signal.h>
#include <windows.h>
#include "mut.h"
#include "fbe/fbe_api_sim_transport_packet_interface.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_test.h"
#include "fbe/fbe_emcutil_shell_include.h"
#include "fbe/fbe_registry.h"

static void __cdecl fbe_sim_destroy(int in);
static void CSX_GX_RT_DEFCC fbe_sim_csx_cleanup_handler(csx_rt_proc_cleanup_context_t context);
static void fbe_sim_config(void);
static void store_pid_to_file(void);
static void  fbe_sim_create_registry_file(void);
static fbe_sim_transport_connection_target_t		target_sp = FBE_SIM_INVALID_SERVER;
static fbe_api_sim_server_pid caller_pid = 0;
static fbe_bool_t store_pid = FBE_FALSE;

int __cdecl main (int argc , char **argv)
{
    mut_testsuite_t 	*system_test_suite;
    char *value;

#include "fbe/fbe_emcutil_shell_maincode.h"
#if 0 /* SAFEMESS - should let CSX handle this */
	signal(SIGINT, fbe_sim_destroy);/*route ctrl C to make sure we nicely clode the connections*/
#else
    csx_rt_proc_cleanup_handler_register(fbe_sim_csx_cleanup_handler, fbe_sim_csx_cleanup_handler, NULL);
#endif

	fbe_trace_init();
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);
	if (argc < 2) {
		printf("\nPlease choose -SPA or -SPB as arguments\n");
		exit (-1);
	}
	
	reset_arguments(argc,argv);
	
	/* before proceeding we need to initialize MUT infrastructure, so MUT_ASSERT can be used */
	mut_register_user_option("-SPA", 0, TRUE, "Start the simulator as SPA");
	mut_register_user_option("-SPB", 0, TRUE, "Start the simulator as SPB");
	mut_register_user_option("-port_base", 1, TRUE, "Set SP&CMI port base");
	mut_register_user_option("-caller_pid", 1, TRUE, "Pass in caller PID");
	mut_register_user_option("-store_pid", 0, TRUE, "Store fbesim.exe PID to the file named fbesim_by_<caller_pid>.txt");
	mut_register_user_option("-sim_drive_type", 1, TRUE, "Set the simulation drive type. 0:remote, 1:memory. Default uses 1:memory");
    mut_register_user_option("-encl_type", 1, TRUE, "Set the simulation encl type. ancho, tabasco, cayenne, naga, viking, miranda, rhea, calypso. Default uses derringer");

    mut_init(argc, argv);
     /* fbesim depends on MUT which uses a default timeout to abort tests that take too long.
        Disabling it since this is running as a simulated SP */
    set_global_timeout(100000);
    system_test_suite = MUT_CREATE_TESTSUITE("system_test_suite")


    if (mut_option_selected("-SPA")){
        target_sp = FBE_SIM_SP_A;
        (void) csx_uap_native_console_title_set("FBE_SP_Simulator_SPA");
    } else if (mut_option_selected("-SPB")) {
        target_sp = FBE_SIM_SP_B;
        (void) csx_uap_native_console_title_set("FBE_SP_Simulator_SPB");
    }

    value = mut_get_user_option_value("-port_base");
    if (value != NULL) {
        fbe_u16_t port_base = atoi(value);
        fbe_api_sim_transport_set_server_mode_port(port_base);
        // first 10 ports are used and reserved for SP ports
        fbe_terminator_api_set_cmi_port_base(port_base + 10);
    }

    value = mut_get_user_option_value("-caller_pid");
    if (value != NULL) {
        caller_pid = atoi(value);
    }
    if (mut_option_selected("-store_pid")) {
        store_pid = FBE_TRUE;
    }
    if (FBE_SIM_INVALID_SERVER == target_sp) {
        printf("\nPlease choose -SPA or -SPB as arguments\n");
        exit (-1);
    }

    /* if caller requires to store fbesim.exe PID, store it in fbesim_by_<caller_pid>.txt */
    if(store_pid && caller_pid > 0)
    {
        store_pid_to_file();
    }

    MUT_ADD_TEST_WITH_DESCRIPTION(system_test_suite, fbe_sim_config, NULL, NULL, NULL, NULL);
    MUT_RUN_TESTSUITE(system_test_suite);

    csx_rt_proc_cleanup_handler_deregister(fbe_sim_csx_cleanup_handler);
    fbe_sim_destroy(0);
}


fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
	*package_id = FBE_PACKAGE_ID_PHYSICAL;
	return FBE_STATUS_OK;
}

static void __cdecl fbe_sim_destroy(int in)
{
	fbe_sim_config_destroy();
	fflush(stdout);
	exit(0);
}

static void CSX_GX_RT_DEFCC
fbe_sim_csx_cleanup_handler(
    csx_rt_proc_cleanup_context_t context)
{
    CSX_UNREFERENCED_PARAMETER(context);
    if (CSX_IS_FALSE(csx_rt_assert_get_is_panic_in_progress())) {
        fbe_sim_destroy(0);
    }
}

/* treat this as a test, so that MUT ASSERT is available */
static void fbe_sim_config(void)
{
	fbe_status_t		status = FBE_STATUS_GENERIC_FAILURE;

        /*create registry file*/
        fbe_sim_create_registry_file();

	status  = fbe_sim_config_init(target_sp);/*take it from the command line*/
	if (status != FBE_STATUS_OK) {
		exit(-1);
	}

    EmcutilSleep(INFINITE);
    return;
}

static void store_pid_to_file(void)
{
    fbe_file_handle_t pid_file_handle;
    fbe_u64_t bytes_written;
    fbe_char_t pid_str[16];
    fbe_api_sim_server_pid pid = 0;
    fbe_char_t pid_file_name[32];
    fbe_file_status_t err_status = CSX_STATUS_SUCCESS;

    pid = csx_p_get_process_id();

    fbe_zero_memory(pid_file_name, sizeof(pid_file_name));
    sprintf(pid_file_name, "fbesim_by_%llu.txt",
	    (unsigned long long)caller_pid);

    pid_file_handle = fbe_file_open(pid_file_name, FBE_FILE_WRONLY | FBE_FILE_CREAT, 0, &err_status);
    if (pid_file_handle == FBE_FILE_INVALID_HANDLE)
    {
        printf("%s can not create %s, error %d\n", __FUNCTION__, pid_file_name, err_status);
    }
    /* Create a entry to fbe_test */
    sprintf(pid_str, "%llu", (unsigned long long)pid);
    bytes_written = fbe_file_write(pid_file_handle, pid_str, sizeof(pid_str), &err_status);
    /* make sure that we are succesful in writing. */
    if (bytes_written <= 0)
    {
        printf("%s failed to write to file %s, error %d\n", __FUNCTION__, pid_file_name, err_status);
        /* close out the file */
        fbe_file_close(pid_file_handle);
    }

    /* close out the file */
    fbe_file_close(pid_file_handle);
}

/*!**************************************************************
 * fbe_sim_create_registry_file()
 ****************************************************************
 * @brief
 *  Create some initial config data for SEP.
 *  Currently we just create local emv registry, which simulates the behavior of SPID
 *
 * @param  none
 *
 * @return none
 *
 * @author
 *  26-MAY-2012 Zhipeng Hu
 *
 ****************************************************************/
static void  fbe_sim_create_registry_file(void)
{
    fbe_status_t    status;
    fbe_u32_t   default_local_emv = 4;  /*default local emv registry is 4GB*/
    fbe_api_sim_server_pid sp_pid = 0;
    fbe_char_t file_path[64] = {'\0'};
    fbe_char_t pid_str[16] = {'\0'};
    fbe_file_handle_t fp = NULL;
    csx_status_e dir_status;  

    /*first create registry file*/
    sp_pid = csx_p_get_process_id();

    /* Need to create sp_sim_pid directory if it doesn't exist.
     */
    sprintf(file_path, "./sp_sim_pid%d", (int)sp_pid);

    dir_status = csx_p_dir_create(file_path);
    if(!(CSX_SUCCESS(dir_status) || (dir_status == CSX_STATUS_OBJECT_ALREADY_EXISTS)))
    {
        printf("set_local_emv_registy: could not create sp_sim_pid dir\n");
        exit(-1);
    }

    /* Then create the file
     */
    sprintf(file_path, "./sp_sim_pid%d/esp_reg_pid%d.txt", (int)sp_pid, (int)sp_pid);

    fp = fbe_file_open(file_path, FBE_FILE_RDWR, 0, NULL);
    if(FBE_FILE_INVALID_HANDLE == fp)
    {
        fp = fbe_file_creat(file_path, FBE_FILE_RDWR);
        fbe_file_close(fp);
    }
    else
    {
        fbe_file_close(fp);
    }

    /*now write the local emv registry*/
    sprintf(pid_str,  "%d", (int)sp_pid);
    status = fbe_registry_write(pid_str, 
                                   K10DriverConfigRegPath, 
                                   K10EXPECTEDMEMORYVALUEKEY, 
                                   FBE_REGISTRY_VALUE_DWORD, 
                                   &default_local_emv, 
                                   sizeof(fbe_u32_t));
    if(FBE_STATUS_OK != status)
    {
        printf("set_local_emv_registy: set local emv fail\n");
        exit(-1);
    }

}

