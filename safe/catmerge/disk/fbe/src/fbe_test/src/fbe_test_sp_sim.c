/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file    fbe_test_drive_sim.c
 ***************************************************************************
 *
 * @brief   This file contains the methods to create and launch the sp
 *          simulator that is used by the `fbe_test' simulator.
 *
 * @version
 *          Initial version
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#define I_AM_NATIVE_CODE
#include "fbe/fbe_types.h"
#include "mut.h"
#include "fbe_test.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_api_sim_transport_packet_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_cli_tests.h"
#include "system_tests.h"
#include <windows.h>

/*************************
 *   GLOBALS
 *************************/
static fbe_thread_t fbe_sp_sim_thread[2];
static fbe_bool_t fbe_sp_sim_thread_running[2] = {FBE_FALSE, FBE_FALSE};
static fbe_semaphore_t fbe_sp_sim_thread_semaphore[2];
static EMCPAL_LARGE_INTEGER fbe_sp_sim_timeout;
static char* default_sim_exe_str_spa = " fbe_sp_sim.exe SPA";
static char* default_sim_exe_str_spb = " fbe_sp_sim.exe SPB";
enum fbe_test_enum_t  
{
    FBE_TEST_MAX_SIM_CMD_LEN = 2048,
};
static char start_sp_sim_cmd[2][FBE_TEST_MAX_SIM_CMD_LEN];
static char debugger_choice[2][100];
static fbe_api_sim_server_pid sp_pids[FBE_TEST_LAST];
static fbe_char_t sp_pid_file_name[32];
static char sp_log_choice[32] = "\0";
static char sp_port_base_str[32] = "\0";
static char cmi_port_base_str[32] = "\0";
static char disk_server_port_base_str[32] = "\0";
static int request_soft_kill = 0;

/*************************
 *   FUNCTIONS
 *************************/


/*!***************************************************************************
 *          fbe_test_sp_sim_process_cmd_bat
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_sp_sim_process_cmd_bat(fbe_sim_transport_connection_target_t which_sp, const char *option_name)
{
    char *value = mut_get_user_option_value(option_name);

    if (value != NULL)
    {
            if (which_sp == FBE_SIM_SP_A)
            {
                strcat(start_sp_sim_cmd[FBE_TEST_SPA], value);
            }
            else
            {
                strcat(start_sp_sim_cmd[FBE_TEST_SPB], value);
            }
    }
}
/* end of fbe_test_sp_sim_process_cmd_bat */

/*!***************************************************************************
 *          fbe_test_sp_sim_process_cmd_debugger
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_sp_sim_process_cmd_debugger(fbe_sim_transport_connection_target_t which_sp, const char *option_name)
{
    int j = 0;

    int cmd_option_values_num = 3;

#ifdef ALAMOSA_WINDOWS_ENV /* Use gdb on Linux */
    const char * cmd_option_values[][3] = {{"VS", " Devenv.exe /debugexe "}, {"windbgx", "windbgx -o "}, {"windbg", "windbg -o "}};
#else 
	const char * cmd_option_values[][3] = {{"valgrind", "/tmp/valgrindme "}, {"ddd", "ddd --args "}, {"gdb", "gdb --args "}};
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

    char *value = mut_get_user_option_value(option_name);

    if (value != NULL)
    {
            for (j = 0; j < cmd_option_values_num; ++j)
            {
                if (strcmp(cmd_option_values[j][0], value) == 0)
                {
                    if (which_sp == FBE_SIM_SP_A)
                    {
                        sprintf(debugger_choice[FBE_TEST_SPA], cmd_option_values[j][1]);
                    }
                    else
                    {
                        sprintf(debugger_choice[FBE_TEST_SPB], cmd_option_values[j][1]);
                    }
                    break;
                }
            }
    }
}
/* end of fbe_test_sp_sim_process_cmd_debugger */

/*!***************************************************************************
 *          fbe_test_sp_sim_process_cmd_debug
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_sp_sim_process_cmd_debug(fbe_sim_transport_connection_target_t which_sp, const char *option_name)
{
    int j = 0;

    int cmd_option_values_num = 3;

    const char * cmd_option_values[][1] = {{"SPA"}, {"SPB"}/* placeholder for SPB */, {"BOTH"}/* placeholder for BOTH */}; 

    char *value = mut_get_user_option_value(option_name);

    if (value != NULL)
    {
            for (j = 0; j < cmd_option_values_num;++j)
            {
                if (strcmp(cmd_option_values[j][0], value) == 0)
                {
                    if (which_sp == FBE_SIM_SP_A)
                    {
                        if ((strcmp(cmd_option_values[j][0], "SPA") == 0) || (strcmp(cmd_option_values[j][0], "BOTH") == 0))
                        {
                            strcat(start_sp_sim_cmd[FBE_TEST_SPA], debugger_choice[FBE_TEST_SPA]);
                            strcat(start_sp_sim_cmd[FBE_TEST_SPA], default_sim_exe_str_spa);
                        }
                    }
                    else
                    {
                        if ((strcmp(cmd_option_values[j][0], "SPB") == 0) || (strcmp(cmd_option_values[j][0], "BOTH") == 0))
                        {
                            strcat(start_sp_sim_cmd[FBE_TEST_SPB], debugger_choice[FBE_TEST_SPB]);
                            strcat(start_sp_sim_cmd[FBE_TEST_SPB], default_sim_exe_str_spb);
                        }
                    }
                    break;
                }
            }
    }
}
/* end of fbe_test_sp_sim_process_cmd_debug */

/*!***************************************************************************
 *          fbe_test_sp_sim_process_cmd_sp_log
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_sp_sim_process_cmd_sp_log(fbe_sim_transport_connection_target_t which_sp, const char *option_name)
{

    char *value = mut_get_user_option_value(option_name);

    if (value != NULL && sp_log_choice[0]=='\0') // only set the string if its currently empty-- we call this function twice for no good reason.
    {
            strcat(sp_log_choice, " log_");
            strcat(sp_log_choice, value);
            strcat(sp_log_choice, " ");
    }
}
/* end of fbe_test_sp_sim_process_cmd_sp_log */

/*!***************************************************************************
 *          fbe_test_sp_sim_process_cmd_port_base
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_sp_sim_process_cmd_port_base(fbe_sim_transport_connection_target_t which_sp, const char *option_name)
{
    fbe_u16_t sp_port_base = 0, cmi_port_base = 0, disk_server_port_base = 0;
    char *value = mut_get_user_option_value(option_name);

    if (value != NULL)
    {
            /* process SP port base */
            sp_port_base = atoi(value);
            /* set port base for SPs at server side */
            sprintf(sp_port_base_str, " -sp_port_base %d", sp_port_base);
            /* set port base for SPs at client side */
            fbe_api_sim_transport_set_server_mode_port(sp_port_base);
            /* set port base to fbecli for the fbecli testsuite */
            fbe_cli_tests_set_port_base(sp_port_base);
            /* set sp&cmi port base to fbe_sim_load_test */
            fbe_sim_load_test_set_port_base(sp_port_base);

            /* process remote Simulated Drive server port base */
            disk_server_port_base = sp_port_base + FBE_TEST_NUM_OF_PORTS_RESERVED_FOR_SP;
            fbe_test_drive_sim_set_drive_server_port(disk_server_port_base);
            sprintf(disk_server_port_base_str, " -disk_server_port_base %d", disk_server_port_base);

            /* process CMI port base */
            cmi_port_base = disk_server_port_base + FBE_TEST_NUM_OF_PORTS_RESERVED_FOR_DISK_SERVER;
            sprintf(cmi_port_base_str, " -cmi_port_base %d", cmi_port_base);
    }
}
/* end of fbe_test_sp_sim_process_cmd_port_base */

/*!***************************************************************************
 *          fbe_test_sp_sim_process_cmd_options()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
static void fbe_test_sp_sim_process_cmd_options(fbe_sim_transport_connection_target_t which_sp)
{
    int i = 0;
    int cmd_bat_flag = 0;
    char *result;
    bool autoTestModeSet = false;
    fbe_test_cmd_options_t *cmd_options;
    bool gracefulShutdownSet = false;

    

#ifdef ALAMOSA_WINDOWS_ENV /* The command line is different on Linux */
    if (which_sp == FBE_SIM_SP_A) {
        sprintf(start_sp_sim_cmd[FBE_TEST_SPA],"start /min ");
    } else {
        sprintf(start_sp_sim_cmd[FBE_TEST_SPB],"start /min ");
    }
#else /* We are running on Linux */
    if (which_sp == FBE_SIM_SP_A) 
    {
        if (mut_option_selected("-nogui"))
        {
            start_sp_sim_cmd[FBE_TEST_SPA][0] = '\0';
        }
        else
        {
            sprintf(start_sp_sim_cmd[FBE_TEST_SPA],"xterm -iconic -sb -sl 50000 -e ");
        }
    } 
    else 
    {
        if (mut_option_selected("-nogui"))
        {
            start_sp_sim_cmd[FBE_TEST_SPB][0] = '\0';
        }
        else
        {
            sprintf(start_sp_sim_cmd[FBE_TEST_SPB],"xterm -iconic -sb -sl 50000 -e ");
        }
    }
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

    cmd_options = fbe_test_main_get_cmd_options();
    for (i = 0; i < fbe_test_main_get_number_cmd_options(); ++i)
    {
        char *value = mut_get_user_option_value(cmd_options[i].name);
        if (value != NULL && cmd_options[i].func != NULL)
        {
            cmd_options[i].func(which_sp, cmd_options[i].name);
            if (i == 0)
            {
                cmd_bat_flag = 1;
                break;  // if -bat was specified, no need to handle debugger and debug command options
            }
        }
    }

    if (which_sp == FBE_SIM_SP_A)
    {
        result = strstr( start_sp_sim_cmd[FBE_TEST_SPA], default_sim_exe_str_spa );
    }
    else
    {
        result = strstr( start_sp_sim_cmd[FBE_TEST_SPB], default_sim_exe_str_spb );
    }

    if (cmd_bat_flag == 0 && result == NULL && which_sp == FBE_SIM_SP_A)
    {
        strcat(start_sp_sim_cmd[FBE_TEST_SPA], default_sim_exe_str_spa);
    }
    else if (cmd_bat_flag == 0 && result == NULL && which_sp == FBE_SIM_SP_B)
    {
        strcat(start_sp_sim_cmd[FBE_TEST_SPB], default_sim_exe_str_spb);
    }

    if (which_sp == FBE_SIM_SP_A)
    {
        strcat(start_sp_sim_cmd[FBE_TEST_SPA], sp_log_choice);
        strcat(start_sp_sim_cmd[FBE_TEST_SPA], sp_port_base_str);
        strcat(start_sp_sim_cmd[FBE_TEST_SPA], cmi_port_base_str);
        strcat(start_sp_sim_cmd[FBE_TEST_SPA], disk_server_port_base_str);
    }
    else
    {
        strcat(start_sp_sim_cmd[FBE_TEST_SPB], sp_log_choice);
        strcat(start_sp_sim_cmd[FBE_TEST_SPB], sp_port_base_str);
        strcat(start_sp_sim_cmd[FBE_TEST_SPB], cmi_port_base_str);
        strcat(start_sp_sim_cmd[FBE_TEST_SPB], disk_server_port_base_str);
    }

	// Get the command line and figure out if 'autotestmode' is set
	if(strstr(_strlwr(mut_get_cmdLine()), "nodebugger") != NULL)
		autoTestModeSet = true;

    if (mut_option_selected("-graceful_shutdown")) {
        gracefulShutdownSet = true;
        request_soft_kill = 1;
    }
	// Add the log dir
    if (which_sp == FBE_SIM_SP_A)
    {
		strcat(start_sp_sim_cmd[FBE_TEST_SPA], " ");
		strcat(start_sp_sim_cmd[FBE_TEST_SPA], FBE_TEST_LOGDIR_OPTION);
		strcat(start_sp_sim_cmd[FBE_TEST_SPA], " ");
        strcat(start_sp_sim_cmd[FBE_TEST_SPA], mut_get_logdir());
		strcat(start_sp_sim_cmd[FBE_TEST_SPA], " ");
		if(autoTestModeSet == true)
			strcat(start_sp_sim_cmd[FBE_TEST_SPA], " nodebugger ");

		if(gracefulShutdownSet== true)
			strcat(start_sp_sim_cmd[FBE_TEST_SPA], " graceful_shutdown ");
    }
    else
    {
		strcat(start_sp_sim_cmd[FBE_TEST_SPB], " ");
		strcat(start_sp_sim_cmd[FBE_TEST_SPB], FBE_TEST_LOGDIR_OPTION);
		strcat(start_sp_sim_cmd[FBE_TEST_SPB], " ");
        strcat(start_sp_sim_cmd[FBE_TEST_SPB], mut_get_logdir());
		strcat(start_sp_sim_cmd[FBE_TEST_SPB], " ");
		if(autoTestModeSet == true)
			strcat(start_sp_sim_cmd[FBE_TEST_SPB], " nodebugger ");
		if(gracefulShutdownSet== true)
			strcat(start_sp_sim_cmd[FBE_TEST_SPB], " graceful_shutdown ");
    }
    return;
}
/* end of fbe_test_sp_sim_process_cmd_options */

/*!***************************************************************************
 *          fbe_test_sp_sim_start_process
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
static void fbe_test_sp_sim_start_process(fbe_sim_transport_connection_target_t which_sp)
{
    int result;
    char pid_cmd[24] = {'\0'};
    char ic_id_cmd[24] = {'\0'};
    fbe_u32_t current_fbe_pid = csx_p_get_process_id();
    csx_ic_id_t current_fbe_ic_id = mut_get_master_ic_id();
    
    fbe_test_sp_sim_process_cmd_options(which_sp);
    _snprintf(pid_cmd,sizeof(pid_cmd)-1,"%s %d", "-fbe_test_pid", current_fbe_pid);
    _snprintf(ic_id_cmd,sizeof(ic_id_cmd)-1,"%s %d", " -fbe_test_ic_id", current_fbe_ic_id);
    if (which_sp == FBE_SIM_SP_A) {
        strcat(start_sp_sim_cmd[FBE_TEST_SPA],pid_cmd); 
        strcat(start_sp_sim_cmd[FBE_TEST_SPA],ic_id_cmd); 
#ifndef ALAMOSA_WINDOWS_ENV
        strcat(start_sp_sim_cmd[FBE_TEST_SPA], " 2>&1 > /dev/null &");
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
        mut_printf(MUT_LOG_HIGH, "Launching program %s\n", start_sp_sim_cmd[FBE_TEST_SPA]);
        result = system(start_sp_sim_cmd[FBE_TEST_SPA]);
    }else{
        strcat(start_sp_sim_cmd[FBE_TEST_SPB],pid_cmd); 
        strcat(start_sp_sim_cmd[FBE_TEST_SPB],ic_id_cmd); 
#ifndef ALAMOSA_WINDOWS_ENV
        strcat(start_sp_sim_cmd[FBE_TEST_SPB], " 2>&1 > /dev/null &");
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
        mut_printf(MUT_LOG_HIGH, "Launching program %s\n", start_sp_sim_cmd[FBE_TEST_SPB]);
        result = system(start_sp_sim_cmd[FBE_TEST_SPB]);
    }
    
    if(result ==1 ){
        /* start command line failed, then try to start the default sim exe */
        if (which_sp == FBE_SIM_SP_A) {
            strcpy(start_sp_sim_cmd[FBE_TEST_SPA], "start /min ");
            strcat(start_sp_sim_cmd[FBE_TEST_SPA], default_sim_exe_str_spa);
            strcat(start_sp_sim_cmd[FBE_TEST_SPA], sp_log_choice);
#ifndef ALAMOSA_WINDOWS_ENV
            strcat(start_sp_sim_cmd[FBE_TEST_SPA], " 2>&1 > /dev/null &");
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
            mut_printf(MUT_LOG_HIGH, "Launching program %s\n", start_sp_sim_cmd[FBE_TEST_SPA]);
            system(start_sp_sim_cmd[FBE_TEST_SPA]);
        }else{
            strcpy(start_sp_sim_cmd[FBE_TEST_SPB], "start /min ");
            strcat(start_sp_sim_cmd[FBE_TEST_SPB] ,default_sim_exe_str_spb);
            strcat(start_sp_sim_cmd[FBE_TEST_SPB], sp_log_choice);
#ifndef ALAMOSA_WINDOWS_ENV
            strcat(start_sp_sim_cmd[FBE_TEST_SPB], " 2>&1 > /dev/null &");
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
            mut_printf(MUT_LOG_HIGH, "Launching program %s\n", start_sp_sim_cmd[FBE_TEST_SPA]);
            system(start_sp_sim_cmd[FBE_TEST_SPB]);
        }
       
    }
}
/* end of fbe_test_sp_sim_start_process() */

/*!***************************************************************************
 *          fbe_test_sp_sim_store_process_ids()
 ***************************************************************************** 
 * 
 * @brief   This function retrieves and stores the process ids for this
 *          invocation of  `fbe_test'.
 *
 * @param   None
 *
 * @return  status
 *
 *****************************************************************************/
fbe_status_t fbe_test_sp_sim_store_sp_pids(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_file_handle_t pid_file_handle;
    fbe_u32_t proc_index;
    fbe_u64_t bytes_written;
    fbe_char_t pid_str[16];
    fbe_file_status_t err_status;

    sp_pids[FBE_TEST_MAIN] = csx_p_get_process_id();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_sim_server_get_pid(&sp_pids[FBE_TEST_SPA]);
    if (status != FBE_STATUS_OK)
    {
        printf("%s failed to get SPA pid, status:%d\n", __FUNCTION__ , status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_sim_server_get_pid(&sp_pids[FBE_TEST_SPB]);
    if (status != FBE_STATUS_OK)
    {
        printf("%s failed to get SPB pid, status:%d\n", __FUNCTION__ , status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    sprintf(sp_pid_file_name, "fbe_test_sp_pid_%llu.txt",
	    (unsigned long long)sp_pids[FBE_TEST_MAIN]);

    pid_file_handle = fbe_file_open(sp_pid_file_name, FBE_FILE_WRONLY | FBE_FILE_CREAT, 0, &err_status);
    if (pid_file_handle == FBE_FILE_INVALID_HANDLE)
    {
        printf("%s can not create %s, error 0x%x\n", __FUNCTION__, sp_pid_file_name, (int)err_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Create a entry to fbe_test */
    for (proc_index = 0; proc_index < FBE_TEST_LAST; proc_index ++)
    {
        fbe_zero_memory(pid_str, sizeof(pid_str));
        sprintf(pid_str, "%d", (int)sp_pids[proc_index]);
        bytes_written = fbe_file_write(pid_file_handle, pid_str, sizeof(pid_str), &err_status);
        /* make sure that we are succesful in writing. */
        if (bytes_written <= 0)
        {
            printf("%s failed to write to file %s, error 0x%x\n", __FUNCTION__, sp_pid_file_name, (int)err_status);
            /* close out the file */
            fbe_file_close(pid_file_handle);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* close out the file */
    fbe_file_close(pid_file_handle);
    return FBE_STATUS_OK;
}
/* end of fbe_test_sp_sim_store_process_ids() */

/*!***************************************************************************
 *          fbe_test_sp_sim_kill_orphan_sp_process
 ***************************************************************************** 
 * 
 * @brief   This function kills any orphan sp simulation processes.
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_sp_sim_kill_orphan_sp_process(void)
{
    fbe_file_handle_t pid_file_handle;
    csx_p_file_find_handle_t search_handle;
    csx_p_file_find_info_t find_data;
    fbe_u64_t bytes_read;
    fbe_api_sim_server_pid pids[FBE_TEST_LAST];
    fbe_char_t pid_str[16];
    fbe_u32_t proc_index;
    csx_status_e status;
    fbe_file_status_t err_status;

    if (mut_option_selected("-graceful_shutdown")) {
        request_soft_kill = 1;
    }
   
    // list all files in current directory and process sp pid files
    status = csx_p_file_find_first(&search_handle,
                                   "./fbe_test_sp_pid_*.txt",
                                   CSX_P_FILE_FIND_FLAGS_NONE, &find_data);

    if(!CSX_SUCCESS(status))
    {
        return;
    }

    do
    {
        if(CSX_P_FILE_TYPE_DIRECTORY != find_data.file_query_info.file_type)
        {
            pid_file_handle = fbe_file_open(find_data.pathname, FBE_FILE_RDONLY,
                                            0, &err_status);
            if (pid_file_handle == FBE_FILE_INVALID_HANDLE)
            {
                printf("%s can not open sp pid file %s, error 0x%x\n",
                       __FUNCTION__, find_data.pathname, (int)err_status);
                continue;
            }
            for (proc_index = 0; proc_index < FBE_TEST_LAST; proc_index ++)
            {
                bytes_read = fbe_file_read(pid_file_handle, pid_str,
                                           sizeof(pid_str), &err_status);
                /* make sure that we are succesful in reading. */
                if (bytes_read != sizeof(pid_str))
                {
                    printf("%s failed to read from file %s, error 0x%x\n",
                           __FUNCTION__, find_data.pathname, (int)err_status);
                    /* close out the file and continue to next process */
                    fbe_file_close(pid_file_handle);
                    continue;
                }
                pids[proc_index] = atoi(pid_str);
            }
            fbe_file_close(pid_file_handle);
        }

        if (csx_p_native_process_does_pid_exist((csx_process_id_t)pids[FBE_TEST_MAIN])) {
            continue;
        }

	mut_printf(MUT_LOG_TEST_STATUS, "%s: killing SPA pid: %d\n",
                   __FUNCTION__, (int)pids[FBE_TEST_SPA]);

        csx_p_native_process_force_exit_by_pid((csx_process_id_t)pids[FBE_TEST_SPA]);

	mut_printf(MUT_LOG_TEST_STATUS, "%s: killing SPB pid: %d\n",
                   __FUNCTION__, (int)pids[FBE_TEST_SPB]);

        csx_p_native_process_force_exit_by_pid((csx_process_id_t)pids[FBE_TEST_SPB]);

        while (csx_p_native_process_does_pid_exist((csx_process_id_t)pids[FBE_TEST_SPA])) {
            printf("waiting for SPA (%d) to exit\n", (int) pids[FBE_TEST_SPA]);
            csx_p_thr_sleep_secs(1);
        }
        while (csx_p_native_process_does_pid_exist((csx_process_id_t)pids[FBE_TEST_SPB])) {
            printf("waiting for SPB (%d) to exit\n", (int) pids[FBE_TEST_SPB]);
            csx_p_thr_sleep_secs(1);
        }

        // delete the sp pid file
        fbe_file_delete(find_data.pathname);
    } while (CSX_STATUS_SUCCESS == csx_p_file_find_next(search_handle, &find_data));

    csx_p_file_find_close(search_handle);
}
/* end of fbe_test_sp_sim_kill_orphan_sp_process */


/*!***************************************************************************
 *          fbe_test_sp_sim_func()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   context - Determines which sp to run
 *
 * @return  None   
 *
 *****************************************************************************/
static void fbe_test_sp_sim_func(void * context)
{    
    fbe_sim_transport_connection_target_t which_sp = (fbe_sim_transport_connection_target_t)context;
    
    fbe_api_trace(FBE_TRACE_LEVEL_INFO,"%s entry\n", __FUNCTION__);

    /* for the SP shutdown and reboot case, 
     * we have to add 2s delay to make sure the TCP sockets created by the killed process change to TIME_WAIT state. 
     * otherwise we might have socket bind error
     */
    EmcutilSleep(2000);

    fbe_test_sp_sim_start_process(which_sp);

    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s done\n", __FUNCTION__);

    if (which_sp == FBE_SIM_SP_A) {
        fbe_semaphore_release(&fbe_sp_sim_thread_semaphore[FBE_TEST_SPA], 0, 1,
                              FALSE);
    } else {
        fbe_semaphore_release(&fbe_sp_sim_thread_semaphore[FBE_TEST_SPB], 0, 1,
                              FALSE);
    }
    
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}
/* end of fbe_test_sp_sim_func() */


/*!***************************************************************************
 *          fbe_test_sp_sim_start()
 ***************************************************************************** 
 * 
 * @brief   This function start a `fbe_sp_sim.exe" for (1) SP
 *
 * @param   which_sp - Target sp id
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_sp_sim_start(fbe_sim_transport_connection_target_t which_sp)
{
    fbe_sp_sim_timeout.QuadPart = -10000 * 100;

    if (which_sp == FBE_SIM_SP_A) {
        fbe_semaphore_init(&fbe_sp_sim_thread_semaphore[FBE_TEST_SPA], 0, 1);
        fbe_sp_sim_thread_running[FBE_TEST_SPA] = FBE_TRUE;
        fbe_thread_init(&fbe_sp_sim_thread[FBE_TEST_SPA], "fbe_test_sp_simA",
                        fbe_test_sp_sim_func, (void *)which_sp);
    }else{
        fbe_semaphore_init(&fbe_sp_sim_thread_semaphore[FBE_TEST_SPB], 0, 1);
        fbe_sp_sim_thread_running[FBE_TEST_SPB] = FBE_TRUE;
        fbe_thread_init(&fbe_sp_sim_thread[FBE_TEST_SPB], "fbe_test_sp_simB",
                        fbe_test_sp_sim_func, (void *)which_sp);
    }

    /* wait for server startup */
    EmcutilSleep(2300);
}
/* end of fbe_test_sp_sim_start() */

/*!***************************************************************************
 *          fbe_test_sp_sim_delete_sp_sim_pid_files()
 ***************************************************************************** 
 * 
 * @brief   Delete the files and directory used by this sp pid
 *
 * @param   pid -- process id of the sp we are deleting files for
 *
 * @return  None   
 *
 *****************************************************************************/
static void fbe_test_sp_sim_delete_sp_sim_pid_files(fbe_api_sim_server_pid pid)
{
    csx_status_e status;
    csx_p_file_find_handle_t search_handle;
    csx_p_file_find_info_t find_data;
    fbe_u8_t file_path[64] = {'\0'};
  
    /* first delete the files
     */
    sprintf(file_path, "./sp_sim_pid%d/*.txt", (int)pid);

    status = csx_p_file_find_first(&search_handle,
                                   file_path,
                                   CSX_P_FILE_FIND_FLAGS_NONE,
                                   &find_data);

    if(CSX_SUCCESS(status))
    {
        do
        {
            fbe_file_delete(find_data.pathname);

        } while (CSX_STATUS_SUCCESS == csx_p_file_find_next(search_handle, &find_data));

        csx_p_file_find_close(search_handle);
    }

    /* then delete the directory
     */
    sprintf(file_path, "./sp_sim_pid%d", (int)pid);

    csx_p_dir_remove(file_path);
}
/* end of fbe_test_sp_sim_delete_sp_sim_pid_files() */


/*!***************************************************************************
 *          fbe_test_sp_sim_kill_single_sp_sim()
 ***************************************************************************** 
 * 
 * @brief   Kill single sp simulation processes
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
static void fbe_test_sp_sim_kill_single_sp_sim(fbe_test_process_t which_sp)
{
    csx_process_id_t pid = (csx_process_id_t)sp_pids[which_sp];

    mut_printf(MUT_LOG_TEST_STATUS, "%s: %s kill of pid %d", __FUNCTION__,
               (request_soft_kill ? "soft" : "forced"), (int)pid);

    if (request_soft_kill) {
        csx_p_native_process_request_clean_exit_by_pid(pid);
    } else {
        csx_p_native_process_force_exit_by_pid(pid);
    }

    // delete the sp pid file
    fbe_file_delete(sp_pid_file_name);

    // delete the sp_sim_pid files and directory
    fbe_test_sp_sim_delete_sp_sim_pid_files(sp_pids[which_sp]);
}
/* end of fbe_test_sp_sim_kill_single_sp_sim() */

/*!***************************************************************************
 *          fbe_test_sp_sim_kill_sp_sims()
 ***************************************************************************** 
 * 
 * @brief   Kill BOTH sp simulation processes
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
static void fbe_test_sp_sim_kill_sp_sims(void)
{
    if (request_soft_kill) {
    	mut_printf(MUT_LOG_TEST_STATUS,
		   "%s: requesting clean exit for SPA pid %llu", __FUNCTION__,
		   (unsigned long long)sp_pids[FBE_TEST_SPA]);
        csx_p_native_process_request_clean_exit_by_pid((csx_process_id_t)sp_pids[FBE_TEST_SPA]);
    } else {
    	mut_printf(MUT_LOG_TEST_STATUS,
		   "%s: forcing termination for SPA pid %llu", __FUNCTION__,
		   (unsigned long long)sp_pids[FBE_TEST_SPA]);
        csx_p_native_process_force_exit_by_pid((csx_process_id_t)sp_pids[FBE_TEST_SPA]);
    } 

    if (request_soft_kill) {
        mut_printf(MUT_LOG_TEST_STATUS,
		   "%s: requesting clean exit for SPB pid %llu", __FUNCTION__,
		   (unsigned long long)sp_pids[FBE_TEST_SPB]);
        csx_p_native_process_request_clean_exit_by_pid((csx_process_id_t)sp_pids[FBE_TEST_SPB]);
    } else {
    	mut_printf(MUT_LOG_TEST_STATUS,
		   "%s: forcing termination for SPB pid %llu", __FUNCTION__,
		   (unsigned long long)sp_pids[FBE_TEST_SPB]);
        csx_p_native_process_force_exit_by_pid((csx_process_id_t)sp_pids[FBE_TEST_SPB]);
    } 

    while (csx_p_native_process_does_pid_exist((csx_process_id_t)sp_pids[FBE_TEST_SPA])) {
//        printf("waiting for SPA (%d) to exit\n", (int) pids[FBE_TEST_SPA]);
        csx_p_thr_sleep_secs(1);
    }

    while (csx_p_native_process_does_pid_exist((csx_process_id_t)sp_pids[FBE_TEST_SPB])) {
//        printf("waiting for SPB (%d) to exit\n", (int) pids[FBE_TEST_SPA]);
        csx_p_thr_sleep_secs(1);
    }

    // delete the sp pid file
    fbe_file_delete(sp_pid_file_name);

    // delete the sp_sim_pid files and directories
    fbe_test_sp_sim_delete_sp_sim_pid_files(sp_pids[FBE_TEST_SPA]);
    fbe_test_sp_sim_delete_sp_sim_pid_files(sp_pids[FBE_TEST_SPB]);
}
/* end of fbe_test_sp_sim_kill_sp_sims() */
/*!***************************************************************************
 *          fbe_test_sp_sim_stop_single_sp()
 ***************************************************************************** 
 * 
 * @brief   First kills one SP sim processes then destroys the associated threads.
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_sp_sim_stop_single_sp(fbe_test_process_t which_sp)
{
    EMCPAL_STATUS emcpal_status;

    /* Mark the SP down so we will stop notifications.
     */
    fbe_test_mark_sp_down(which_sp);

    /* Kill the sp simulation processes*/
    fbe_test_sp_sim_kill_single_sp_sim(which_sp);

    /*wait for threads to end*/
    if (fbe_sp_sim_thread_running[which_sp]) {

        fbe_sp_sim_thread_running[which_sp] = FBE_FALSE;

        emcpal_status = fbe_semaphore_wait(&fbe_sp_sim_thread_semaphore[which_sp],
                                           &fbe_sp_sim_timeout);

         if (EMCPAL_STATUS_SUCCESS == emcpal_status) {
             fbe_thread_wait(&fbe_sp_sim_thread[which_sp]);
         } else {
             fbe_api_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                           "%s: sp %d thread (%p) did not exit!\n", __FUNCTION__,
                           (int)which_sp, fbe_sp_sim_thread[which_sp].thread);
         }
        fbe_semaphore_destroy(&fbe_sp_sim_thread_semaphore[which_sp]);
        fbe_thread_destroy(&fbe_sp_sim_thread[which_sp]);
    }


    /* Clean up this queue, so that no prior notifications are leftover. 
     */
    fbe_api_common_clean_job_notification_queue();

    return;
}
/* end of fbe_test_sp_sim_stop_single_sp() */

/*!***************************************************************************
 *          fbe_test_sp_sim_stop()
 ***************************************************************************** 
 * 
 * @brief   First kills BOTH SP sim processes then deetroys BOTH the associated
 *          threads.
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_sp_sim_stop()
{
    int i;
    EMCPAL_STATUS emcpal_status;

    /* Kill both sp simulation processes*/
    fbe_test_sp_sim_kill_sp_sims();

    /*wait for threads to end*/
    for (i = 0; i <= 1; i++) {
        if (fbe_sp_sim_thread_running[i]) {

            fbe_sp_sim_thread_running[i] = FBE_FALSE;

            emcpal_status = fbe_semaphore_wait(&fbe_sp_sim_thread_semaphore[i],
                                               &fbe_sp_sim_timeout);

            if (EMCPAL_STATUS_SUCCESS == emcpal_status) {
                fbe_thread_wait(&fbe_sp_sim_thread[i]);
            } else {
                fbe_api_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                              "%s: sp %d thread (%p) did not exit!\n",
                              __FUNCTION__, i, fbe_sp_sim_thread[i].thread);
            }
            fbe_semaphore_destroy(&fbe_sp_sim_thread_semaphore[i]);
            fbe_thread_destroy(&fbe_sp_sim_thread[i]);
        }
    }
    
    return;
}
/* end of fbe_test_sp_sim_stop() */

/*!***************************************************************************
 *          fbe_test_sp_sim_start_notification()
 ***************************************************************************** 
 * 
 * @brief   send suite start notification to both SPs
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_sp_sim_start_notification(void)
{
    fbe_sim_transport_connection_target_t current_target = fbe_api_sim_transport_get_target_server(),
            other_target = (current_target == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    fbe_api_sim_server_suite_start_notification(mut_get_logdir(), mut_get_log_file_name());

    fbe_api_sim_transport_set_target_server(other_target);

    fbe_api_sim_server_suite_start_notification(mut_get_logdir(), mut_get_log_file_name());

    fbe_api_sim_transport_set_target_server(current_target);
}
/* end of fbe_test_sp_sim_start_notification() */

/*!***************************************************************************
 *          fbe_test_sp_sim_start_notification_single_sp()
 ***************************************************************************** 
 * 
 * @brief   send suite start notification to single SP
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_sp_sim_start_notification_single_sp(fbe_sim_transport_connection_target_t sp)
{
    fbe_sim_transport_connection_target_t current_target = fbe_api_sim_transport_get_target_server();

    fbe_api_sim_transport_set_target_server(sp);

    fbe_api_sim_server_suite_start_notification(mut_get_logdir(), mut_get_log_file_name());

    fbe_api_sim_transport_set_target_server(current_target);
}
/* end of fbe_test_sp_sim_start_notification_single_sp() */

/*******************************
 * end file fbe_test_sp_sim.c
 *******************************/
