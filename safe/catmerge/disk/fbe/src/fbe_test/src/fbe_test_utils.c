/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file    fbe_test_utils.c
 ***************************************************************************
 *
 * @brief   This file contains the utility methods for the `fbe_test'
 *          simulation.
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
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_api_sim_server.h"
#include "self_tests.h"
#include <windows.h>

/*************************
 *   GLOBALS
 *************************/
static fbe_bool_t is_self_test = FBE_FALSE;
static fbe_bool_t spec_list = FBE_FALSE;
static fbe_bool_t spec_info = FBE_FALSE;

/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************************
 *          fbe_test_util_store_process_ids()
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
fbe_status_t fbe_test_util_store_process_ids(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_process_info_t fbe_test_process_table[FBE_TEST_LAST] = 
                        {{0, "fbe_sp_sim.exe SPA", ""},
                         {0, "fbe_sp_sim.exe SPB", ""},
                         {0, "fbe_test.exe", ""}};
    fbe_file_handle_t pid_file_handle;
    fbe_u32_t proc_index;
    fbe_u64_t bytes_written;
    fbe_file_status_t err_status;

    fbe_test_process_table[FBE_TEST_MAIN].pid = csx_p_get_process_id();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_sim_server_get_pid(&fbe_test_process_table[FBE_TEST_SPA].pid);
    if (status != FBE_STATUS_OK)
    {
        printf("%s failed to get SPA pid, status:%d\n", __FUNCTION__ , status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_sim_server_get_pid(&fbe_test_process_table[FBE_TEST_SPB].pid);
    if (status != FBE_STATUS_OK)
    {
        printf("%s failed to get SPB pid, status:%d\n", __FUNCTION__ , status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Set the flags so that we can read/write and create the file */
    fbe_file_delete("fbe_test_pid.txt");
    pid_file_handle = fbe_file_open("fbe_test_pid.txt", FBE_FILE_WRONLY | FBE_FILE_CREAT, 0, &err_status);
    if (pid_file_handle == FBE_FILE_INVALID_HANDLE)
    {
        printf("%s can not create %s, error 0x%x\n", __FUNCTION__, "fbe_test_pid.txt", (int)err_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Create a entry to fbe_test */
    for (proc_index = 0; proc_index < FBE_TEST_LAST; proc_index ++)
    {
        fbe_zero_memory(&fbe_test_process_table[proc_index].log_entry, 
                        sizeof(fbe_test_process_table[proc_index].log_entry));
        fbe_sprintf(fbe_test_process_table[proc_index].log_entry, 
                    sizeof(fbe_test_process_table[proc_index].log_entry),
                    "Process Name: %s, PID: %llu\n", 
                    fbe_test_process_table[proc_index].process_name, 
                    (unsigned long long)fbe_test_process_table[proc_index].pid);
        bytes_written = fbe_file_write(pid_file_handle,
                                       &fbe_test_process_table[proc_index].log_entry,
                                       sizeof(fbe_test_process_table[proc_index].log_entry), &err_status);
        /* make sure that we are successful in writing. */
        if (bytes_written != sizeof(fbe_test_process_table[proc_index].log_entry))
        {
            printf("%s failed to write to file %s, error 0x%x\n", __FUNCTION__, "fbe_test_pid.txt", (int)err_status);
            /* close out the file */
            fbe_file_close(pid_file_handle);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* close out the file */
    fbe_file_close(pid_file_handle);
    return FBE_STATUS_OK;
}
/* end of fbe_test_util_store_process_ids() */


/*!***************************************************************************
 *          fbe_test_util_set_console_title()
 ***************************************************************************** 
 * 
 * @brief   This function simply set the console title line
 *
 * @param   argc
 * @param   argv
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_util_set_console_title(int argc , char **argv)
{
    char current_console_title[232] = {'\0'};
    char new_console_title[256] = {'\0'};
    fbe_u32_t current_fbe_pid = csx_p_get_process_id();
    BOOL fbe_test_list_cmd_flag = false;
    int i;

    // check whether command line options have -list option
    for(i = 1; i < argc; ++i)
    {
        if(!strcmp(argv[i], "-list"))
        {
            fbe_test_list_cmd_flag = true;
        }
    }

    if(!fbe_test_list_cmd_flag)
    {
        (void) csx_uap_native_console_title_get(current_console_title,232);
        _snprintf(new_console_title, sizeof(new_console_title)-1, "FbeTestPID: %d, ", current_fbe_pid);
        strncat(new_console_title, current_console_title,(sizeof(new_console_title)-1-strlen(new_console_title)));
        (void) csx_uap_native_console_title_set(new_console_title);
     }

}
/* end of fbe_test_util_set_console_title() */

/*!***************************************************************************
 *          fbe_test_util_generate_self_test_arg()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_util_generate_self_test_arg(int *argc_p, char ***argv_p)
{
    /* we specify the argc and argv for self test */
    const char *argv_array[] = {"fbe_test.exe", "-logdir", FBE_TEST_SELF_TEST_LOG_DIR, "-sp_log", "both"};
    int i = 0;
    fbe_u32_t length = 0;

    *argc_p = sizeof(argv_array) / sizeof(const char*);

    /* check if -list or -info is specified */
    if(spec_list)
    {
        ++(*argc_p);
    }
    if(spec_info)
    {
        ++(*argc_p);
    }

    /* allocate and assign argv */
    *argv_p = malloc((*argc_p + 1) * sizeof(char *));
    if(*argv_p == NULL)
    {
        return;
    }

    for(i = 0; i < sizeof(argv_array) / sizeof(const char*); ++i)
    {
        length = (fbe_u32_t)(strlen(argv_array[i]) + 1);
        (*argv_p)[i] = malloc(length);
        if((*argv_p)[i] == NULL)
        {
            return;
        }
        memset((*argv_p)[i], 0, length);
        memcpy((*argv_p)[i], argv_array[i], length - 1);
    }
    if(spec_list)
    {
        length = (fbe_u32_t)(sizeof("-list") + 1);
        (*argv_p)[i] = malloc(length);
        if((*argv_p)[i] == NULL)
        {
            return;
        }
        memset((*argv_p)[i], 0, length);
        memcpy((*argv_p)[i], "-list", length - 1);
        ++i;
    }
    if(spec_info)
    {
        length = (fbe_u32_t)(sizeof("-info") + 1);
        (*argv_p)[i] = malloc(length);
        if((*argv_p)[i] == NULL)
        {
            return;
        }
        memset((*argv_p)[i], 0, length);
        memcpy((*argv_p)[i], "-info", length - 1);
        ++i;
    }
    /* one extra NULL pointer */
    (*argv_p)[i] = NULL;
}
/* end of fbe_test_util_generate_self_test_arg() */


/*!***************************************************************************
 *          fbe_test_util_process_fbe_io_delay()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_util_process_fbe_io_delay(void)
{
    char *value = mut_get_user_option_value("-fbe_io_delay");
    fbe_u32_t delay = 0;

    if (value != NULL)
    {
        delay = atoi(value);
        mut_printf(MUT_LOG_HIGH, "Setting Terminator I/O completion delay to %d ms\n", delay);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_api_terminator_set_io_global_completion_delay(delay);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_api_terminator_set_io_global_completion_delay(delay);
    }
}
/* end of fbe_test_util_process_fbe_io_delay() */

/*!***************************************************************************
 *          fbe_test_util_free_self_test_arg()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_util_free_self_test_arg(int argc, char **argv)
{
    int i = 0;

    for(i = 0; i < argc; ++i)
    {
        free(argv[i]);
    }

    free(argv);
}
/* end of fbe_test_util_free_self_test_arg() */



/*!***************************************************************************
 *          fbe_test_util_is_self_test_enabled()
 ***************************************************************************** 
 * 
 * @brief   check if -self_test is specified
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
fbe_bool_t fbe_test_util_is_self_test_enabled(int argc, char **argv)
{
    int i = 0;

    is_self_test = FBE_FALSE;
    spec_list = FBE_FALSE;
    spec_info = FBE_FALSE;

    for(i = 0; i < argc; ++i)
    {
        if(!strcmp(argv[i], "-self_test"))
        {
            is_self_test = FBE_TRUE;
        }
        else if(!strcmp(argv[i], "-list"))
        {
            spec_list = FBE_TRUE;
        }
        else if(!strcmp(argv[i], "-info"))
        {
            spec_info = FBE_TRUE;
        }
    }

    return(is_self_test);
}
/* end of fbe_test_util_is_self_test_enabled() */

/****************************
 * end file fbe_test_utils.c
 ****************************/
