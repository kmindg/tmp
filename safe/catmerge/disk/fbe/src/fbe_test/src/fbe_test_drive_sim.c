/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file    fbe_test_drive_sim.c
 ***************************************************************************
 *
 * @brief   This file contains the methods to create and launch the drive
 *          simulation server that is used by the `fbe_test' simulator.
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
#include "fbe_simulated_drive.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_api_terminator_interface.h"
#include <windows.h>

/*************************
 *   GLOBALS
 *************************/
static fbe_bool_t   fbe_test_drive_sim_dualsp_force_drive_sim = FBE_FALSE;
static terminator_simulated_drive_type_t fbe_test_drive_sim_simulated_drive_type = TERMINATOR_DEFAULT_SIMULATED_DRIVE_TYPE;
static fbe_thread_t fbe_test_drive_sim_server_thread;
static fbe_semaphore_t fbe_test_drive_sim_server_thread_semaphore;
static EMCPAL_LARGE_INTEGER fbe_test_drive_sim_timeout;
static fbe_char_t *fbe_test_drive_sim_drive_server_name = SIMULATED_DRIVE_DEFAULT_SERVER_NAME;
static fbe_u16_t fbe_test_drive_sim_drive_server_port = SIMULATED_DRIVE_DEFAULT_PORT_NUMBER;
static fbe_api_sim_server_pid fbe_test_drive_sim_drive_server_pid = -1;
static const fbe_char_t *fbe_test_drive_sim_server_name_wo_extension = "fbe_simulated_drive_user_server";
static fbe_char_t fbe_test_drive_sim_drive_server_pid_file_name[64];

/*!*******************************************************************
 * @def FBE_TEST_SIM_DRIVE_INIT_MAX_WAIT_MS
 *********************************************************************
 * @brief Max milliseconds we will wait for the simulated drive server to come up.
 *
 *********************************************************************/
#define FBE_SIM_DRIVE_INIT_MAX_WAIT_MS 60000

/*!*******************************************************************
 * @def FBE_SIM_DRIVE_WAIT_TIME
 *********************************************************************
 * @brief Time to wait when we're polling for existence of the
 *        simulated drive server.
 *
 *********************************************************************/
#define FBE_SIM_DRIVE_WAIT_TIME 500

/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************************
 *          fbe_test_drive_sim_start_drive_server()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
static void fbe_test_drive_sim_start_drive_server(void)
{
    csx_ic_id_t current_fbe_ic_id = mut_get_master_ic_id();
    fbe_u32_t current_fbe_pid = csx_p_get_process_id();
#ifdef ALAMOSA_WINDOWS_ENV /* The command line is different on Linux */
    char startup_string[120] = "start /min";
#else /* We are running on Linux */
    char startup_string[200] = "";
    if (!mut_option_selected("-nogui"))
    {
        sprintf(startup_string, "xterm -iconic -sb -sl 50000 -e ");
    }
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

    sprintf(startup_string, "%s %s.exe -temp -fbe_test_ic_id %d", startup_string, fbe_test_drive_sim_server_name_wo_extension, (int)current_fbe_ic_id);
    if(fbe_test_drive_sim_drive_server_port > 0)
    {
        sprintf(startup_string, "%s -port %d -testpid %d", startup_string, fbe_test_drive_sim_drive_server_port, current_fbe_pid);
    }
#ifndef ALAMOSA_WINDOWS_ENV
    strcat(startup_string, " 2>&1 > /dev/null &");
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
    mut_printf(MUT_LOG_HIGH, "Launching program %s\n", startup_string);
    system(startup_string);

    return;
}
/* end of fbe_test_drive_sim_start_drive_server() */

/*!***************************************************************************
 *          fbe_test_drive_sim_kill_drive_server()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
static void fbe_test_drive_sim_kill_drive_server(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "%s: killing pid %lld", __FUNCTION__,
               fbe_test_drive_sim_drive_server_pid);

    csx_p_native_process_force_exit_by_pid((csx_process_id_t)fbe_test_drive_sim_drive_server_pid);

    // delete the sp pid file
    fbe_file_delete(fbe_test_drive_sim_drive_server_pid_file_name);
}
/* end of fbe_test_drive_sim_kill_drive_server() */

/*!***************************************************************************
 *          fbe_test_drive_sim_store_drive_server_pid()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  status
 *
 *****************************************************************************/
static fbe_status_t fbe_test_drive_sim_store_drive_server_pid(void)
{
    fbe_file_handle_t pid_file_handle;
    fbe_s64_t bytes_written;
    fbe_char_t pid_str[16];
    fbe_api_sim_server_pid fbe_test_pid = csx_p_get_process_id();
    fbe_api_sim_server_pid drive_server_pid = fbe_test_drive_sim_drive_server_pid;
    fbe_bool_t  b_success = FBE_FALSE;
    fbe_u32_t elapsed_ms = 0;
    fbe_file_status_t err_status;

   /*! @note First initialize the simulated drive client, then issue the 
     *        request to get the simulated drive server pid then destroy
     *        the simulated drive server socket.
     */
    b_success = fbe_simulated_drive_init(fbe_test_drive_sim_drive_server_name, fbe_test_drive_sim_drive_server_port, 0);

    /* If the drive server is not up yet, then poll for it.
     */
    while (!b_success && (elapsed_ms < FBE_SIM_DRIVE_INIT_MAX_WAIT_MS))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s sim drive not up yet, try again after 500ms\n", __FUNCTION__);
        b_success = fbe_simulated_drive_init(fbe_test_drive_sim_drive_server_name, fbe_test_drive_sim_drive_server_port, 0);

        fbe_api_sleep(FBE_SIM_DRIVE_WAIT_TIME);
        elapsed_ms += FBE_SIM_DRIVE_WAIT_TIME;
    }

    /* If the drive server is not up yet, then there is something wrong.
     */
    MUT_ASSERT_TRUE_MSG(b_success != FBE_FALSE, "Failed to initialize simulated drive server");

    drive_server_pid = fbe_simulated_drive_get_server_pid();
    b_success = fbe_simulated_drive_cleanup();
    MUT_ASSERT_TRUE_MSG(b_success != FBE_FALSE, "Failed to cleanup simulated drive server");
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s Simulated drive server pid: %llu",
               __FUNCTION__, (unsigned long long)drive_server_pid);
    fbe_test_drive_sim_drive_server_pid = drive_server_pid;
    sprintf(fbe_test_drive_sim_drive_server_pid_file_name,
	    "fbe_test_drive_server_pid_%llu.txt",
	    (unsigned long long)fbe_test_pid);

    pid_file_handle = fbe_file_open(fbe_test_drive_sim_drive_server_pid_file_name, FBE_FILE_WRONLY | FBE_FILE_CREAT, 0, &err_status);
    if (pid_file_handle == FBE_FILE_INVALID_HANDLE)
    {
        printf("%s can not create %s, error 0x%x\n", __FUNCTION__, fbe_test_drive_sim_drive_server_pid_file_name, (int)err_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Record current pid */
    fbe_zero_memory(pid_str, sizeof(pid_str));
    sprintf(pid_str, "%d", (int)fbe_test_pid);
    bytes_written = fbe_file_write(pid_file_handle, pid_str, sizeof(pid_str), &err_status);
    /* make sure that we are succesful in writing. */
    if (bytes_written <= 0)
    {
        printf("%s failed to write to file %s, error 0x%x\n", __FUNCTION__, fbe_test_drive_sim_drive_server_pid_file_name, (int)err_status);
        /* close out the file */
        fbe_file_close(pid_file_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Record disk server pid */
    fbe_zero_memory(pid_str, sizeof(pid_str));
    sprintf(pid_str, "%d", (int)drive_server_pid);
    bytes_written = fbe_file_write(pid_file_handle, pid_str, sizeof(pid_str), &err_status);
    /* make sure that we are succesful in writing. */
    if (bytes_written <= 0)
    {
        printf("%s failed to write to file %s, error 0x%x\n", __FUNCTION__, fbe_test_drive_sim_drive_server_pid_file_name, (int)err_status);
        /* close out the file */
        fbe_file_close(pid_file_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* close out the file */
    fbe_file_close(pid_file_handle);
    return FBE_STATUS_OK;
}
/* end of fbe_test_drive_sim_store_drive_server_pid() */

/*!***************************************************************************
 *          fbe_test_drive_sim_set_drive_server_port()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  status
 *
 *****************************************************************************/
void fbe_test_drive_sim_set_drive_server_port(fbe_u16_t drive_server_port)
{
    fbe_test_drive_sim_drive_server_port = drive_server_port;
    return;
}
/* end of fbe_test_drive_sim_set_drive_server_port()*/

/*!***************************************************************************
 *          fbe_test_drive_sim_get_drive_server_port()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  status
 *
 *****************************************************************************/
fbe_u16_t fbe_test_drive_sim_get_drive_server_port(void)
{
    return(fbe_test_drive_sim_drive_server_port);
}
/* end of fbe_test_drive_sim_get_drive_server_port()*/

/*!***************************************************************************
 *          fbe_test_drive_sim_kill_orphan_drive_server_process()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_drive_sim_kill_orphan_drive_server_process(void)
{
    fbe_file_handle_t pid_file_handle;
    csx_p_file_find_handle_t search_handle;
    csx_p_file_find_info_t find_data;
    fbe_s64_t bytes_read;
    fbe_api_sim_server_pid fbe_test_pid, fbe_test_drive_sim_drive_server_pid;
    fbe_char_t pid_str[16];
    csx_status_e status;
    fbe_file_status_t err_status;

    // list all files in current directory and process sp pid files
    status = csx_p_file_find_first(&search_handle,
                                   "./fbe_test_drive_server_pid_*.txt",
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
            /* Read fbe_test pid */
            bytes_read = fbe_file_read(pid_file_handle, pid_str,
                                       sizeof(pid_str), &err_status);
            /* make sure that we are succesful in reading. */
            if (bytes_read != sizeof(pid_str))
            {
                printf("%s failed to read from file %s, error 0x%x\n",
                       __FUNCTION__, find_data.pathname, (int)err_status);
                /* close out the file */
                fbe_file_close(pid_file_handle);
                continue;
            }
            fbe_test_pid = atoi(pid_str);
            /* Read disk server pid */
            bytes_read = fbe_file_read(pid_file_handle, pid_str,
                                       sizeof(pid_str), &err_status);
            /* make sure that we are succesful in reading. */
            if (bytes_read != sizeof(pid_str))
            {
                printf("%s failed to read from file %s, error 0x%x\n",
                       __FUNCTION__, find_data.pathname, (int)err_status);
                /* close out the file */
                fbe_file_close(pid_file_handle);
                continue;
            }
            fbe_test_drive_sim_drive_server_pid = atoi(pid_str);
            fbe_file_close(pid_file_handle);
        }
        // check if this fbe_test is running, if not kill related sp processes.
        if (csx_p_native_process_does_pid_exist((csx_process_id_t)fbe_test_pid))
        {
            continue;
        }
	mut_printf(MUT_LOG_TEST_STATUS, "%s: killing pid %lld", __FUNCTION__,
                   fbe_test_drive_sim_drive_server_pid);

        csx_p_native_process_force_exit_by_pid((csx_process_id_t)fbe_test_drive_sim_drive_server_pid);

        // delete the sp pid file
        fbe_file_delete(find_data.pathname);
    } while (CSX_SUCCESS(csx_p_file_find_next(search_handle, &find_data)));

    csx_p_file_find_close(search_handle);
}
/* end of fbe_test_drive_sim_kill_orphan_drive_server_process() */

/*!***************************************************************************
 *          fbe_test_drive_sim_func()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
static void fbe_test_drive_sim_func(void * context)
{    
    FBE_UNREFERENCED_PARAMETER(context);
    
    fbe_api_trace(FBE_TRACE_LEVEL_INFO,"%s entry\n", __FUNCTION__);

    fbe_test_drive_sim_start_drive_server();

    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s done\n", __FUNCTION__);
    
    fbe_semaphore_release(&fbe_test_drive_sim_server_thread_semaphore, 0, 1,
                          FALSE);

    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}
/* end of fbe_test_drive_sim_func() */

/*!***************************************************************************
 *          fbe_test_drive_sim_start()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
static void fbe_test_drive_sim_drive_server_start(void)
{
    fbe_test_drive_sim_timeout.QuadPart = -10000 * 100;

    fbe_semaphore_init(&fbe_test_drive_sim_server_thread_semaphore, 0, 1);
    fbe_thread_init(&fbe_test_drive_sim_server_thread, "fbe_test_drv_sim",
                    fbe_test_drive_sim_func, NULL);

    /* wait for server startup */
    EmcutilSleep(500);
}
/* end of fbe_test_drive_sim_drive_server_start() */



/*!***************************************************************************
 *          fbe_test_drive_sim_drive_server_stop()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
static void fbe_test_drive_sim_drive_server_stop(void)
{
    EMCPAL_STATUS emcpal_status;

    /*! @note It is up to each sp to disconnect.  Since there is no client 
     *        connection form fbe_test to the drive simulator, there is nothing
     *        to cleanup.
     */

    /* Now wait for simulated drive server to be killed */
    fbe_test_drive_sim_kill_drive_server();

    /*wait for threads to end*/
    emcpal_status = fbe_semaphore_wait(&fbe_test_drive_sim_server_thread_semaphore,
                                       &fbe_test_drive_sim_timeout);

    if (EMCPAL_STATUS_SUCCESS == emcpal_status) {
        fbe_thread_wait(&fbe_test_drive_sim_server_thread);
    } else {
        fbe_api_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                      "%s: test drive sim server thread (%p) did not exit!\n",
                      __FUNCTION__, fbe_test_drive_sim_server_thread.thread);
    }
    fbe_semaphore_destroy(&fbe_test_drive_sim_server_thread_semaphore);
    fbe_thread_destroy(&fbe_test_drive_sim_server_thread);
}
/* end of fbe_test_drive_sim_drive_server_stop() */

/*!***************************************************************************
 *          fbe_test_drive_sim_get_sim_drive_type()
 ***************************************************************************** 
 * 
 * @brief   Based on the configuration, type of tests (i.e. dualsp etc) and
 *          user options, determine which type of simulated drive should be
 *          used.
 *
 * @param   sim_drive_type_p - Pointer to drive type to update
 *
 * @return  status  
 *
 *****************************************************************************/
static fbe_status_t fbe_test_drive_sim_get_sim_drive_type(terminator_simulated_drive_type_t *sim_drive_type_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    char           *value = mut_get_user_option_value("-sim_drive_type");
    int             int_value;
    terminator_simulated_drive_type_t sim_drive_type = fbe_test_drive_sim_simulated_drive_type;
    fbe_bool_t      b_drive_sim_enabled;

    /* Get current value */
    b_drive_sim_enabled = (sim_drive_type == TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY) ? FBE_TRUE : FBE_FALSE;

    /* dualsp test must use drive sim */
    if (fbe_test_drive_sim_dualsp_force_drive_sim == FBE_TRUE)
    {
        MUT_ASSERT_TRUE(b_drive_sim_enabled == FBE_TRUE);
        *sim_drive_type_p = sim_drive_type;
        return FBE_STATUS_OK;
    }

    /* Only if value isn't null */
    if (value != NULL)
    {
        int_value = atoi(value);
        sim_drive_type = (terminator_simulated_drive_type_t)int_value;

        /* Validate value */
        switch(sim_drive_type)
        {
            case TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY:
            case TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_MEMORY:
                break;

            case TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE:
                /*! @note Due to the 183GB capacity of the system drives, this simulated drive 
                 *        type is no longer supported.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "get sim drive type: -sim_drive_type %d (local file) is no longer supported",
                           TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE);
                return FBE_STATUS_GENERIC_FAILURE;
                break;

            default:
                /* print an error but continue*/
                mut_printf(MUT_LOG_TEST_STATUS, 
                       "get sim drive type: -sim_drive_type must be remote mem(%d) or local mem(%d)",
                       TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY, 
                       TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_MEMORY);
                return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* If the user specified a different value update our local */
    if (sim_drive_type != fbe_test_drive_sim_simulated_drive_type)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "get sim drive type: changing simulated drive type from: %d to %d",
                   fbe_test_drive_sim_simulated_drive_type, sim_drive_type);
        fbe_test_drive_sim_simulated_drive_type = sim_drive_type;
    }

    /* Now simply determine if sim dirve is enabled or not */
    if (fbe_test_drive_sim_simulated_drive_type != TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY)
    {
        b_drive_sim_enabled = FBE_FALSE;
    }

    /* Update drive type and return success */
    *sim_drive_type_p = fbe_test_drive_sim_simulated_drive_type;
    return(status);
}
/* end of fbe_test_drive_sim_get_sim_drive_type() */

/*!***************************************************************************
 *          fbe_test_drive_sim_force_drive_sim_remote_memory()
 ***************************************************************************** 
 * 
 * @brief   Force the use of drive simulated server
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_drive_sim_force_drive_sim_remote_memory(void)
{
    // Override any command line parameters
    fbe_test_drive_sim_dualsp_force_drive_sim = FBE_TRUE;
    fbe_test_drive_sim_simulated_drive_type = TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY;

    return;
}
/* end of fbe_test_drive_sim_force_drive_sim_remote_memory()*/

/*!***************************************************************************
 *          fbe_test_drive_sim_restore_default_drive_sim_type()
 ***************************************************************************** 
 * 
 * @brief   Restore the drive simulation type to the default
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_drive_sim_restore_default_drive_sim_type(void)
{
    // Override any command line parameters
    fbe_test_drive_sim_dualsp_force_drive_sim = FBE_FALSE;
    fbe_test_drive_sim_simulated_drive_type = TERMINATOR_DEFAULT_SIMULATED_DRIVE_TYPE;

    return;
}
/* end of fbe_test_util_restore_default_drive_sim_type()*/

/*!***************************************************************************
 *          fbe_test_drive_sim_start_drive_sim()
 ***************************************************************************** 
 * 
 * @brief   This function first determines which type of drive simulation to
 *          then it executes the proper initialization and starts the drive
 *          simulation server if required.
 *
 * @param   running - TRUE is the drive server is already running.
 *
 * @return  status 
 *
 *****************************************************************************/
fbe_status_t fbe_test_drive_sim_start_drive_sim(fbe_bool_t running)
{
    fbe_status_t    status = FBE_STATUS_OK;
    terminator_simulated_drive_type_t sim_drive_type = TERMINATOR_SIMULATED_DRIVE_TYPE_INVALID;

    /* First get the drive type
     */
    status = fbe_test_drive_sim_get_sim_drive_type(&sim_drive_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Simply switch on the simulated drive type
     */
    switch(sim_drive_type)
    {
        case TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY:
            /* The following code is for using the file base simulated drive system.
             * This allows I/O and persisted changes to be visible on both SPs.
             */
        	if (!running)
        	{
        		fbe_test_drive_sim_drive_server_start();
        	}

            /* Now in the following order, set the simulated drive server name
             * simulated server port and simulated drive type for each sp.
             */
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            fbe_api_terminator_set_simulated_drive_server_name(fbe_test_drive_sim_drive_server_name);
            fbe_api_terminator_set_simulated_drive_server_port(fbe_test_drive_sim_drive_server_port);
            fbe_api_terminator_set_simulated_drive_type(TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY);
            /* Set both SPs to use remote memory
             */
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            fbe_api_terminator_set_simulated_drive_server_name(fbe_test_drive_sim_drive_server_name);
            fbe_api_terminator_set_simulated_drive_server_port(fbe_test_drive_sim_drive_server_port);
            fbe_api_terminator_set_simulated_drive_type(TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            /* save the drive server process id
             */
            if (!running)
            {
            	fbe_test_drive_sim_store_drive_server_pid();
            }
            break;

        case TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_MEMORY:
            /* Else set the drive type of both sps to local memory */
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            fbe_api_terminator_set_simulated_drive_type(TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_MEMORY);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            fbe_api_terminator_set_simulated_drive_type(TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_MEMORY);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            break;

        case TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE:
            /* Else set the drive type of both sps to local file */
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            fbe_api_terminator_set_simulated_drive_type(TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            fbe_api_terminator_set_simulated_drive_type(TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            break;

        default:
            /* Just assert
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "start drive sim: unsupported drive sim type: %d",
                       sim_drive_type);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* Display the simulated drive type we are using
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "start drive sim: using simulated drive type: %d",
               sim_drive_type);

    /* Now return the status
     */
    return(status);
}
/* end of fbe_test_drive_sim_start_drive_sim() */
/*!***************************************************************************
 *          fbe_test_drive_sim_start_drive_sim_for_sp()
 ***************************************************************************** 
 * 
 * @brief   This function first determines which type of drive simulation to
 *          then it executes the proper initialization and starts the drive
 *          simulation server if required.
 *
 * @param   running - TRUE is the drive server is already running.
 *
 * @return  status 
 *
 *****************************************************************************/
fbe_status_t fbe_test_drive_sim_start_drive_sim_for_sp(fbe_bool_t running,
                                                       fbe_sim_transport_connection_target_t sp)
{
    fbe_status_t    status = FBE_STATUS_OK;
    terminator_simulated_drive_type_t sim_drive_type = TERMINATOR_SIMULATED_DRIVE_TYPE_INVALID;

    /* First get the drive type
     */
    status = fbe_test_drive_sim_get_sim_drive_type(&sim_drive_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Simply switch on the simulated drive type
     */
    switch(sim_drive_type)
    {
        case TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY:
            /* The following code is for using the file base simulated drive system.
             * This allows I/O and persisted changes to be visible on both SPs.
             */
        	if (!running)
        	{
        		fbe_test_drive_sim_drive_server_start();
        	}

            /* Now in the following order, set the simulated drive server name
             * simulated server port and simulated drive type for each sp.
             */
            fbe_api_sim_transport_set_target_server(sp);
            fbe_api_terminator_set_simulated_drive_server_name(fbe_test_drive_sim_drive_server_name);
            fbe_api_terminator_set_simulated_drive_server_port(fbe_test_drive_sim_drive_server_port);
            fbe_api_terminator_set_simulated_drive_type(TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY);
            /* save the drive server process id
             */
            if (!running)
            {
            	fbe_test_drive_sim_store_drive_server_pid();
            }
            break;

        case TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_MEMORY:
            /* Else set the drive type of both sps to local memory */
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            fbe_api_terminator_set_simulated_drive_type(TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_MEMORY);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            fbe_api_terminator_set_simulated_drive_type(TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_MEMORY);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            break;

        case TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE:
            /* Else set the drive type of both sps to local file */
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            fbe_api_terminator_set_simulated_drive_type(TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            fbe_api_terminator_set_simulated_drive_type(TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            break;

        default:
            /* Just assert
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "start drive sim: unsupported drive sim type: %d",
                       sim_drive_type);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* Display the simulated drive type we are using
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "start drive sim: using simulated drive type: %d",
               sim_drive_type);

    /* Now return the status
     */
    return(status);
}
/* end of fbe_test_drive_sim_start_drive_sim_for_sp() */

/*!***************************************************************************
 *          fbe_test_drive_sim_destroy_drive_sim()
 ***************************************************************************** 
 * 
 * @brief   This function first determines which type of drive simulation to
 *          then it executes the proper destroy.
 *
 * @param   None
 *
 * @return  status 
 *
 *****************************************************************************/
fbe_status_t fbe_test_drive_sim_destroy_drive_sim(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    terminator_simulated_drive_type_t sim_drive_type = TERMINATOR_SIMULATED_DRIVE_TYPE_INVALID;

    /* First get the drive type
     */
    status = fbe_test_drive_sim_get_sim_drive_type(&sim_drive_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Simply switch on the simulated drive type
     */
    switch(sim_drive_type)
    {
        case TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY:
            /* Simply stop the simulated drive server
             */
            fbe_test_drive_sim_drive_server_stop();
            break;

        case TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_MEMORY:
            /*! @todo Anything to do for this?
             */
            break;

        case TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE:
            /*! @todo Need to close open files etc...
             */
            break;

        default:
            /* Just assert
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "destroy drive sim : unsupported drive sim type: %d",
                       sim_drive_type);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* Now return the status
     */
    return(status);
}
/* end of fbe_test_drive_sim_destroy_drive_sim() */


/*******************************
 * end file fbe_test_drive_sim.c
 *******************************/
