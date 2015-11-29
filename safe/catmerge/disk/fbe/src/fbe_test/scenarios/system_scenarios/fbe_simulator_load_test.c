/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_simulator_load_test.c
 ***************************************************************************
 *
 * @brief
 *  Make sure fbesim.exe loads fine
 *
 ***************************************************************************/

#define I_AM_NATIVE_CODE
#include <windows.h>
#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_sim_transport_packet_interface.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_api_system_interface.h"
#include "fbe/fbe_api_persist_interface.h"
#include "fbe_test.h"

static void fbe_sim_load_test_kill_fbesim(fbe_api_sim_server_pid current_pid);

char * fbe_sim_test_short_desc = "Make sure fbesim.exe loads fine";
static fbe_u16_t port_base = 0;

void fbe_sim_load_test(void)
{

	fbe_status_t				status = FBE_STATUS_GENERIC_FAILURE;
	fbe_u32_t					retry = 0;
	fbe_cmi_service_get_info_t	cmi_info;
	char fbesim_cmd[128] = "start fbesim.exe -SPA";
	fbe_api_sim_server_pid pid;
	fbe_api_system_get_failure_info_t err_info_ptr;

    /* if user specifies sp_port_base, update it here and pass it to fbesim.exe */
    if(port_base > 0)
    {
      status = fbe_api_sim_transport_set_server_mode_port(port_base);
      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
      sprintf(fbesim_cmd, "%s -port_base %d", fbesim_cmd, port_base);
    }

    pid = csx_p_get_process_id();
    sprintf(fbesim_cmd, "%s -caller_pid %d -store_pid", fbesim_cmd, (int)pid);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Loading fbesim.exe and waiting for it", __FUNCTION__);

	/*lets run fbesim*/
	system (fbesim_cmd);

	/*wait some reasonable time for it to start*/
	EmcutilSleep(18000);

	status = fbe_api_sim_transport_init_client(FBE_SIM_SP_A, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);/*just to be on the safe side*/

	/*and try to send some FBE API to it to check it's alive*/
	do {
		status = fbe_api_cmi_service_get_info(&cmi_info);
		retry++;
		EmcutilSleep(5000);
	}while ((status == FBE_STATUS_GENERIC_FAILURE) && retry < 10);

	/*test the system cleanup APIs*/
	status = fbe_api_system_reset_failure_info(FBE_PACKAGE_ID_SEP_0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	fbe_api_system_get_failure_info(&err_info_ptr);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_INT_EQUAL(err_info_ptr.error_counters[FBE_PACKAGE_ID_SEP_0].trace_error_counter, 0);

	/*cause some failure*/
	fbe_api_persist_abort_transaction(0x435512);/*use some bogus number*/

	fbe_api_system_get_failure_info(&err_info_ptr);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(err_info_ptr.error_counters[FBE_PACKAGE_ID_SEP_0].trace_error_counter, 0);

	fbe_api_sim_transport_destroy_client(FBE_SIM_SP_A);
    
	fbe_sim_load_test_kill_fbesim(pid);

	mut_printf(MUT_LOG_TEST_STATUS, "%s successful", __FUNCTION__);
}

void fbe_sim_load_test_set_port_base(fbe_u16_t port_base_num)
{
    port_base = port_base_num;
}

static void fbe_sim_load_test_kill_fbesim(fbe_api_sim_server_pid current_pid)
{
    fbe_file_handle_t pid_file_handle;
    csx_p_file_find_handle_t search_handle;
    csx_p_file_find_info_t find_data;
    fbe_u64_t bytes_read;
    fbe_api_sim_server_pid pid, fbesim_pid;
    fbe_char_t pid_str[16], *c1, *c2;
    fbe_u32_t len = sizeof("fbesim_by_") - 1;
    fbe_file_status_t err_status = CSX_STATUS_SUCCESS;

    // list all files in current directory and process pid files
    err_status = csx_p_file_find_first(&search_handle,
                                       "./fbesim_by_*.txt",
                                       CSX_P_FILE_FIND_FLAGS_NONE, &find_data);

    if(!CSX_SUCCESS(err_status))
    {
        return;
    }

    do
    {
        if(CSX_P_FILE_TYPE_DIRECTORY != find_data.file_query_info.file_type)
        {
            c1 = strstr(find_data.filename, "fbesim_by_");
            c2 = strstr(find_data.filename, ".txt");

            if(c1 == NULL || c2 == NULL || c1 + len >= c2)
            {
                return;
            }

            fbe_zero_memory(pid_str, sizeof(pid_str));
            fbe_copy_memory(pid_str, c1 + len, (fbe_u32_t)(c2 - c1 - len));
            pid = atoi(pid_str);

            // check if this fbe_simulator_load_test is running.
            if (csx_p_native_process_does_pid_exist((csx_process_id_t)pid))
            {
                // if it is the fbesim pid file for current process, terminate related fbesim.exe and delete the pid file
                if(pid == current_pid)
                {
                    pid_file_handle = fbe_file_open(find_data.pathname, FBE_FILE_RDONLY, 0, &err_status);
                    if (pid_file_handle == FBE_FILE_INVALID_HANDLE)
                    {
                        printf("%s can not open fbesim pid file %s, error 0x%x\n", __FUNCTION__, find_data.pathname, (int)err_status);
                        continue;
                    }
                    fbe_zero_memory(pid_str, sizeof(pid_str));
                    bytes_read = fbe_file_read(pid_file_handle, pid_str, sizeof(pid_str), &err_status);
                    /* make sure that we are successful in reading. */
                    if (bytes_read <= 0)
                    {
                        printf("%s failed to read from file %s, error 0x%x\n", __FUNCTION__, find_data.pathname, (int)err_status);
                        /* close out the file */
                        fbe_file_close(pid_file_handle);
                        continue;
                    }
                    fbesim_pid = atoi(pid_str);
                    fbe_file_close(pid_file_handle);
                    fbe_file_delete(find_data.pathname);

                    csx_p_native_process_force_exit_by_pid((csx_process_id_t)fbesim_pid);
                }
            }
            // if it is stopped, delete the file
            else
            {
                fbe_file_delete(find_data.pathname);
            }
        }
    } while (CSX_STATUS_SUCCESS == csx_p_file_find_next(search_handle, &find_data));

    csx_p_file_find_close(search_handle);
}
