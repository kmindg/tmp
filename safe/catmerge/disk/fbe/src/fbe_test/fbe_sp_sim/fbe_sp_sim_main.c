#include "fbe_sp_sim.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_file.h"
#include "fbe_trace.h"
#include "fbe/fbe_trace_interface.h"
#include <signal.h>
#include "simulation_log.h"
#include "CrashHelper.h"
#include "fbe/fbe_emcutil_shell_include.h"
#include "mut.h"
#include "EmcUTIL_DllLoader_Interface.h"

static void Remove_VectoredExceptionHandler(void);
static csx_status_e VectoredExceptionHandler(csx_rt_proc_exception_info_t *ExceptionInfo);
static csx_pvoid_t pVectoredExceptionHandler = NULL;
static void CSX_GX_RT_DEFCC fbe_sp_sim_csx_cleanup_handler(csx_rt_proc_cleanup_context_t context);
static void fbe_sp_sim_destroy(int exitCode);
static char	exeName[50]={'\0'};
static char logDir[EMCUTIL_MAX_PATH];
static EMCUTIL_DLL_HANDLE simulation_log_dll_handle;
static fbe_sim_transport_connection_target_t target_sp; // now also used by exception handler to ident sp in dump file name

// autotestmode only

void CSX_GX_RT_DEFCC force_dump_handler(csx_cstring_t file, csx_nuint_t line){
    // passing this function to the DbgBreakPoint lib will throw an exception that the 
    // exception handler will trigger on and we'll get a dump.
    // this is only used in "autotestmode".  In non-autotestmode,
    // a regular int 3 happens to enter the debugger.

    volatile int k = 5;
    volatile int i = 5;
    volatile int j;
    j=1/(i-k);
    return;
}
// end autotestmode only


int __cdecl main (int argc , char **argv)
{
    csx_size_t          path_len;
    csx_status_e        csx_status;
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
    int i;
    csx_process_id_t    pid;
    char 		title[100] ={'\0'};
    char                fbe_test_pid_string[10]={'\0'};
    csx_ic_id_t         fbe_test_ic_id = CSX_IC_ID_INVALID;
    int                 register_cleanup = 0;
    
    fbe_u16_t sp_port_base = 0, cmi_port_base = 0, disk_server_port_base = 0;
    int					len;
    char				*ptrPos;
    fbe_trace_error_limit_t  error_limit;
    int         log_both=0, log_console=0, log_file = 0, log_none = 0;
    simulation_log_add_destination_func_t simulation_log_add_destination_func = NULL;
    simulation_log_remove_destination_func_t simulation_log_remove_destination_func = NULL;
    simulation_log_set_name_suffix_func_t simulation_log_set_name_suffix_func = NULL;
    csx_status_e file_status;
    csx_p_file_find_handle_t search_handle;
    csx_p_file_find_info_t find_data;
    fbe_u8_t file_path[64] = {'\0'};

#include "fbe/fbe_emcutil_shell_maincode.h"
    simulation_log_dll_handle = emcutil_simple_dll_load("simulation_log");
    if(simulation_log_dll_handle == NULL)
    {
        printf("Cannot load simulation_log.dll!\n");
        exit (-1);
    }
    simulation_log_add_destination_func = (simulation_log_add_destination_func_t)emcutil_simple_dll_lookup(simulation_log_dll_handle, "simulation_log_add_destination");
    simulation_log_remove_destination_func = (simulation_log_remove_destination_func_t)emcutil_simple_dll_lookup(simulation_log_dll_handle, "simulation_log_remove_destination");
    simulation_log_set_name_suffix_func = (simulation_log_set_name_suffix_func_t)emcutil_simple_dll_lookup(simulation_log_dll_handle, "simulation_log_set_name_suffix");

    // Default log location to the current dir. Will be updated later by what is passed in as an argv.
    csx_status = csx_p_native_working_directory_get(logDir, EMCUTIL_MAX_PATH, &path_len);
    if (!CSX_SUCCESS(csx_status))
    {
        if (CSX_STATUS_BUFFER_OVERFLOW == csx_status)
        {
            printf("Cannot get current directory: buffer too small: "
                   "size is %lld but need %lld\n",
                   (long long)EMCUTIL_MAX_PATH, (long long)path_len);
        }
        else
        {
            printf("Cannot get current directory: 0x%x\n", (int)csx_status);
        }
        exit (-1);
    }
	CSX_TRY {
		// If 'nodebugger' is passed in as a cmd line param setup the vectored exception handler.
		for(i = 1; i < argc; ++i)
		{
			if(!strcmp(argv[i], "nodebugger"))
			{
				// Used to set the vectored exception handler
				pVectoredExceptionHandler = csx_rt_proc_register_early_exception_callback(VectoredExceptionHandler, CSX_TRUE);
                csx_rt_assert_breakpoint_handler_set(force_dump_handler);
			}
            else if (!strcmp(argv[i], "graceful_shutdown"))
            {
                register_cleanup = 1;
            }

		}

        if (register_cleanup) {
            csx_rt_proc_cleanup_handler_register(fbe_sp_sim_csx_cleanup_handler, fbe_sp_sim_csx_cleanup_handler, NULL);
        }

		fbe_trace_init();
		fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

		/* stop sp_sim and terminator on any error trace. */
		error_limit.action = FBE_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM;
		error_limit.num_errors = 0;
		error_limit.trace_level = FBE_TRACE_LEVEL_CRITICAL_ERROR;
		fbe_trace_set_error_limit(&error_limit);
		error_limit.trace_level = FBE_TRACE_LEVEL_ERROR;
		fbe_trace_set_error_limit(&error_limit);


		if (argc < 2) {
			printf("\nPlease choose SPA or SPB as arguments\n");
			exit (-1);
		}

		// Parse the args

		// Save the exe name (remove the extension first if there is one)
		ptrPos = strchr(argv[0], '.');
		if(ptrPos != NULL)
		{
			len = (int)(ptrPos - argv[0]); // Length to the '.'?
			strncpy(exeName, argv[0], len);
		}
		else
		{
			// No '.', copy the whole string
			strncpy(exeName, argv[0], sizeof(exeName));
		}

		// Do the rest of them
		for(i = 1; i < argc; ++i)
		{
			if (strstr(argv[i], "SPA"))
			{
				target_sp = FBE_SIM_SP_A;
			}
            else if (strstr(argv[i], "SPB"))
			{
				target_sp = FBE_SIM_SP_B;
			}
            else if(!strcmp(argv[i], "log_both"))
			{
                log_both = 1;
			}
			else if(!strcmp(argv[i], "log_console"))
			{
                log_console =1;
			}
            else if(!strcmp(argv[i], "log_file"))
			{
                log_file =1;
            }
            else if(!strcmp(argv[i], "log_none"))
            {
                log_none=1;
            }
			else if(!strcmp(argv[i], "-logDir"))
			{   // at the moment this parameter needs to be processed first,
                // or the log_add_destination calls will fail.

                // Is the next index valid? If so it's the logdir
				if(i+1 <= argc) {
				    simulation_log_set_logdir_func_t simulation_log_set_logdir_func = NULL;
					strncpy(logDir, argv[i+1], EMCUTIL_MAX_PATH);
					simulation_log_set_logdir_func = (simulation_log_set_logdir_func_t)emcutil_simple_dll_lookup(simulation_log_dll_handle, "simulation_log_set_logdir");
					if(simulation_log_set_logdir_func != NULL)
					{
					    simulation_log_set_logdir_func(logDir);
					}
                }

            }   
			
            else if(!strcmp(argv[i], "-sp_port_base"))
            {
                if(i < argc - 1)
                {
                    ++i;
                    sp_port_base = atoi(argv[i]);
                }
            }
            else if(!strcmp(argv[i], "-cmi_port_base"))
            {
                if(i < argc - 1)
                {
                    ++i;
                    cmi_port_base = atoi(argv[i]);
                }
            }
            else if(!strcmp(argv[i], "-disk_server_port_base"))
            {
                if(i < argc - 1)
                {
                    ++i;
                    disk_server_port_base = atoi(argv[i]);
                }
            }
           else if(!strcmp(argv[i], "-fbe_test_pid"))
            {
                if(i < (argc - 1))
                {

                    ++i;
                    strcpy(fbe_test_pid_string, argv[i]);
                }
            }
            else if (!strcmp(argv[i], "-fbe_test_ic_id")) {
                if(i < (argc - 1))
                {
                    ++i;
                    fbe_test_ic_id = atoi(argv[i]);
                }
            }			
		}
        {
            csx_status_e status;
            
            /* The test process can rety so we must handle the `already connected' status*/
            status = csx_p_grman_connection_establish(CSX_MY_MODULE_CONTEXT(), fbe_test_ic_id);
            if (CSX_FAILURE(status)) 
            {
                CSX_ASSERT_STATUS_H_RDC(CSX_STATUS_ALREADY_CONNECTED, status);
            }
        }
        pid =  csx_p_get_process_id();
		//set console title with fbe sim and test process id
        if (target_sp == FBE_SIM_SP_A)
        {

            sprintf(title, "FBE SP Simulator SPA, Process ID:%d", pid);
            if(simulation_log_set_name_suffix_func != NULL)
            {
                simulation_log_set_name_suffix_func("spa");
            }
        }
        else
        {
            sprintf(title, "FBE SP Simulator SPB, Process ID:%d", pid);
            if(simulation_log_set_name_suffix_func != NULL)
            {
                simulation_log_set_name_suffix_func("spb");
            }
        }

        if (fbe_test_pid_string[0])
        {
            strcat(title, ", FBE Test Process ID: ");

            strcat (title,fbe_test_pid_string);
        }
        (void) csx_uap_native_console_title_set(title);

        /* Delete any leftover sp_sim_pid files and directory
         * First delete the files
         */
        sprintf(file_path, "./sp_sim_pid%d/*.txt", pid);

        file_status = csx_p_file_find_first(&search_handle,
                                       file_path,
                                       CSX_P_FILE_FIND_FLAGS_NONE,
                                       &find_data);

        if(CSX_SUCCESS(file_status))
        {
            do
            {
                fbe_file_delete(find_data.pathname);

            } while (CSX_STATUS_SUCCESS == csx_p_file_find_next(search_handle, &find_data));

            csx_p_file_find_close(search_handle);
        }

        /* then delete the directory
         */
        sprintf(file_path, "./sp_sim_pid%d", pid);

        csx_p_dir_remove(file_path);

        /* done removing sp_sim_pid files and directory */

        if (log_both != 0)
        {
            if(simulation_log_add_destination_func != NULL)
            {
                simulation_log_add_destination_func(SIMULATION_LOG_DESTINATION_CONSOLE);
                simulation_log_add_destination_func(SIMULATION_LOG_DESTINATION_FILE);
            }
        }
        if (log_console !=0)
        {
            if(simulation_log_add_destination_func != NULL)
            {
                simulation_log_add_destination_func(SIMULATION_LOG_DESTINATION_CONSOLE);
            }
            if(simulation_log_remove_destination_func != NULL)
            {
                simulation_log_remove_destination_func(SIMULATION_LOG_DESTINATION_FILE);
            }
       }
        if (log_file != 0)
        {
            if(simulation_log_remove_destination_func != NULL)
            {
                simulation_log_remove_destination_func(SIMULATION_LOG_DESTINATION_CONSOLE);
            }
            if(simulation_log_add_destination_func != NULL)
            {
                simulation_log_add_destination_func(SIMULATION_LOG_DESTINATION_FILE);
            }
        }
        if (log_none != 0)
        {
            if(simulation_log_remove_destination_func != NULL)
            {
                simulation_log_remove_destination_func(SIMULATION_LOG_DESTINATION_CONSOLE);
                simulation_log_remove_destination_func(SIMULATION_LOG_DESTINATION_FILE);
            }
        }
        status  = fbe_sp_sim_config_init(target_sp, sp_port_base, cmi_port_base, disk_server_port_base);
		if (status != FBE_STATUS_OK) {
		exit(-1);
		}
 
		while(1){
			fbe_api_sleep(3600000);
		}
		fbe_sp_sim_destroy(0);
	}
    CSX_FINALLY {
		// Remove the vectored exception handler
		Remove_VectoredExceptionHandler();
	} CSX_END_TRY_FINALLY;

    if (register_cleanup) {
        csx_rt_proc_cleanup_handler_deregister(fbe_sp_sim_csx_cleanup_handler);
    }
}

// Remove the vectored exception handler
static void Remove_VectoredExceptionHandler(void)
{
	// Is it active?
	if(pVectoredExceptionHandler != NULL)
	{
		csx_rt_proc_unregister_early_exception_callback(pVectoredExceptionHandler);
		pVectoredExceptionHandler = NULL;
	}
}

// Vectored exception handler callback function
static csx_status_e VectoredExceptionHandler(csx_rt_proc_exception_info_t *ExceptionInfo)
{
	char timeDateStamp[TIME_DATE_STAMP_SIZE]={'\0'};
	char exceptionPathAndFile[EMCUTIL_MAX_PATH]={'\0'};
	char fileName[EMCUTIL_MAX_PATH]={'\0'};
	char spBuffer[10]={'\0'};
    simulation_get_timedate_stamp_func_t simulation_get_timedate_stamp_func = NULL;
    simulation_log_get_name_suffix_func_t simulation_log_get_name_suffix_func = NULL;

	// Remove the vectored exception handler
	Remove_VectoredExceptionHandler();

    simulation_get_timedate_stamp_func = (simulation_get_timedate_stamp_func_t)emcutil_simple_dll_lookup(simulation_log_dll_handle, "simulation_get_timedate_stamp");
    simulation_log_get_name_suffix_func = (simulation_log_get_name_suffix_func_t)emcutil_simple_dll_lookup(simulation_log_dll_handle, "simulation_log_get_name_suffix_func");

	// Which sp?	
    if(simulation_log_get_name_suffix_func != NULL)
    {
        simulation_log_get_name_suffix_func(spBuffer, (int)sizeof(spBuffer));
    }

	// Build an exception path and file 
    if(simulation_get_timedate_stamp_func != NULL)
    {
        simulation_get_timedate_stamp_func(timeDateStamp, TIME_DATE_STAMP_SIZE);
    }
    _snprintf(exceptionPathAndFile, EMCUTIL_MAX_PATH, "%s", logDir);
    _snprintf(fileName, EMCUTIL_MAX_PATH, "%s_%s_%s_%s.dmp", exeName, spBuffer,
              timeDateStamp, (target_sp == FBE_SIM_SP_A ? "spa":"spb"));
    EmcutilFilePathExtend(exceptionPathAndFile, EMCUTIL_MAX_PATH, fileName, NULL);
        
    // tell CSX we're going to panic.  This will lower the priority of any realtime threads before the dump to prevent potential
    // priority inversion issues
    csx_rt_proc_run_panic_handlers();

	// Write the exception file out
    CreateDumpFile(exceptionPathAndFile, ExceptionInfo->native_exception_info);

    fflush(stdout);
    exit(-2); // Since we're dumping out of the program anyway, I see no need to clean up, also
    // exiting prevents recursive dumping.
    // 
    // 
	// Cleanup and leave, return a -2 to signify an exception was detected
	//fbe_sp_sim_destroy(-2);

	// Need to return something even though we are exiting before this
	return CSX_STATUS_EXCEPTION_EXECUTE_HANDLER;
}

fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
	*package_id = FBE_PACKAGE_ID_PHYSICAL;
	return FBE_STATUS_OK;
}

static void fbe_sp_sim_destroy(int exitCode)
{
	fbe_sp_sim_config_destroy();
	fbe_trace_destroy();
	fflush(stdout);
	exit(exitCode);
}

static void CSX_GX_RT_DEFCC
fbe_sp_sim_csx_cleanup_handler(
    csx_rt_proc_cleanup_context_t context)
{
    CSX_UNREFERENCED_PARAMETER(context);
    if (CSX_IS_FALSE(csx_rt_assert_get_is_panic_in_progress())) {
#ifdef CSX_BV_ENVIRONMENT_LINUX
        /* CGCG - cancel alarm set by CSX for now to aid in debug so we have unlimited time to cleanup */
        alarm(0);
#endif
        fbe_sp_sim_destroy(0);
    }
}
