
/***************************************************************************
* Copyright (C) EMC Corporation 2010
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_test_output_handler.c
***************************************************************************
*
* DESCRIPTION
* All functions to handle fbe cli output are implemented here.
* Redirect fbe cli output from testing process (fbecli.exe) by using windows APIs.
* 
* HISTORY
*   08/16/2010:  Created. hud1
*
***************************************************************************/


#define I_AM_NATIVE_CODE
#include "fbe_cli_tests.h"
#include <windows.h>
#include <string.h>

#pragma comment(lib, "User32.lib")

/**************************************************************************
*  execute_fbe_cli_command()
**************************************************************************
*
*  DESCRIPTION:
*    This function executes a fbe cli command, then get the output.
*
*  PARAMETERS:
*    fbe_cli_command: fbe cli command such as "pdo","pdo", etc
*    fbe_cli_output(out): the console output of the fbe cli command
*
*  RETURN VALUES/ERRORS:
*
*  NOTES:
*    fbe_cli_output needs to be freed using csx_p_strfree
*
*  HISTORY:
*   08/16/2010:  Created. hud1
*   12/13/2012:  Reworked for Windows API removal.
**************************************************************************/

BOOL execute_fbe_cli_command(const char* fbe_cli_command, char** fbe_cli_output)
{
    char         cmd_line_exe[CLI_STRING_MAX_LEN] = {"\0"};
    char         fbe_cli_command_str[CLI_STRING_MAX_LEN + 8] = {"\0"};
    char         *p;
    csx_size_t   str_size;
    csx_status_e status;
    csx_nsint_t  exit_code;
    BOOL         return_val = TRUE;

    /*
     * Sanity check that the fbecli command string is not longer than we expect.
     */
    if (strlen(fbe_cli_command) > CLI_STRING_MAX_LEN) {
        mut_printf(MUT_LOG_LOW,
                   "%s: fbe_cli_cmd too long: %s is larger than %d",
                   __FUNCTION__, fbe_cli_command, CLI_STRING_MAX_LEN);
        return FALSE;
    }

    /*
     * Make a local copy of the fbecli command(s). Add "quit" to the end, and
     * change all ';' separators to '\n'.
     */
    strcpy(fbe_cli_command_str, fbe_cli_command);
    strcat(fbe_cli_command_str, "\nquit\n");

    p = fbe_cli_command_str;
    while (*p) {
        if (*p == ';') {
            *p = '\n';
        }
        p++;
    }

    /*
     * Create the fbecli command line.
     */
    if (!strcmp(current_control.mode, cli_mode_types[CLI_MODE_TYPE_SCRIPT])) {
        //fbecli script mode
        if (current_control.sp_port_base > 0) {
            str_size = csx_p_snprintf(cmd_line_exe, CLI_STRING_MAX_LEN,
                                      "%s z\"%s\" %s %s p%d",
                                      current_control.cli_executable_name,
                                      current_control.cli_script_file_name,
                                      current_control.drive_type,
                                      current_control.sp_type,
                                      current_control.sp_port_base);
        } else {
            str_size = csx_p_snprintf(cmd_line_exe, CLI_STRING_MAX_LEN,
                                      "%s z\"%s\" %s %s",
                                      current_control.cli_executable_name,
                                      current_control.cli_script_file_name,
                                      current_control.drive_type,
                                      current_control.sp_type);
        }
        if (str_size >= CLI_STRING_MAX_LEN) {
            cmd_line_exe[CLI_STRING_MAX_LEN - 1] = '\0';
            mut_printf(MUT_LOG_LOW,
                       "%s: cmd_line_exe too long: %s is larger than %d",
                       __FUNCTION__, cmd_line_exe, CLI_STRING_MAX_LEN);
            return FALSE;
        }
        mut_printf(MUT_LOG_LOW,
                   "In script mode, launching test application: %s",
                   cmd_line_exe);
    } else {
        //fbecli interactive mode

        if(current_control.sp_port_base > 0) {
            str_size = csx_p_snprintf(cmd_line_exe, CLI_STRING_MAX_LEN,
                                      "%s %s %s p%d",
                                      current_control.cli_executable_name,
                                      current_control.drive_type,
                                      current_control.sp_type,
                                      current_control.sp_port_base);
        } else {
            str_size = csx_p_snprintf(cmd_line_exe, CLI_STRING_MAX_LEN,
                                      "%s %s %s",
                                      current_control.cli_executable_name,
                                      current_control.drive_type,
                                      current_control.sp_type);
        }
        if (str_size >= CLI_STRING_MAX_LEN) {
            cmd_line_exe[CLI_STRING_MAX_LEN - 1] = '\0';
            mut_printf(MUT_LOG_LOW,
                       "%s: cmd_line_exe too long: %s is larger than %d",
                       __FUNCTION__, cmd_line_exe, CLI_STRING_MAX_LEN);
            return FALSE;
        }
	mut_printf(MUT_LOG_LOW,
                   "In interactive mode,launching test application: %s",
                   cmd_line_exe);
    }

    status = csx_p_native_process_drive(cmd_line_exe, fbe_cli_command_str,
                                        fbe_cli_output, &exit_code);
    if (!CSX_SUCCESS(status)) {
        mut_printf(MUT_LOG_LOW, "Cannot execute fbe command line: %s",
                   cmd_line_exe);
        return FALSE;

    }

    return return_val;
}
