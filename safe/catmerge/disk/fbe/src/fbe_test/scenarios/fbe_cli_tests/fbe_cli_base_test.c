
/***************************************************************************
* Copyright (C) EMC Corporation 2010
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_base_test.c
***************************************************************************
*
* DESCRIPTION
* This is base test for all fbe cli tests..
* 
* HISTORY
*   09/28/2010:  Created. hud1
*
***************************************************************************/

#include "fbe_cli_tests.h"
#include "fbe/fbe_api_sim_transport.h"


const char* default_cli_executable_name = "fbecli.exe";
const char* default_cli_script_file_name = "script.txt";
const cli_drive_type_t default_cli_drive_types = CLI_DRIVE_TYPE_SIMULATOR;
const cli_sp_type_t default_cli_sp_types = CLI_SP_TYPE_SPA;
const cli_mode_type_t default_cli_mode = CLI_MODE_TYPE_INTERACTIVE;

const char* cli_drive_types[] = {"k","s"};
const char* cli_sp_types[] = {"a","b"};
const char* cli_mode_types[] = {"interactive","script"};

struct fbe_cli_test_control_s current_control;



/**************************************************************************
*  fbe_cli_base_test()
**************************************************************************
*
*  DESCRIPTION:
*    A base cli test to simplify writing tests.
*    It includes a typical way to check expected keywords.To support more 
*    way of keyword checking, please take this function as a reference to create
*    other base tests.
*
*  PARAMETERS:
*    cli_cmds: fbe cli command such as "pdo","ldo",etc.
*
*  RETURN VALUES/ERRORS:
*    TRUE: all expected keywords are found and no unexpected keyword is found
*....FALSE: any expected keyword is not found or any unexpected keyword is found
*
*  NOTES:
*
*  HISTORY:
*    09/28/2010:  Created. hud1
**************************************************************************/

BOOL fbe_cli_base_test(fbe_cli_cmd_t cli_cmds)
{
    char** expected_keywords;
    char** unexpected_keywords;

    BOOL expected_keywords_check_result;
    BOOL unexpected_keywords_check_result;

    char *command_line_out = NULL;
    char command_line_str[CLI_STRING_MAX_LEN]= {"\0"};


    expected_keywords = cli_cmds.expected_keywords;
    unexpected_keywords = cli_cmds.unexpected_keywords;

    mut_printf(MUT_LOG_LOW, "Test fbecli command: \"%s\" ", cli_cmds.name);

    // Copy the Engineering Access Mode command to the command line string
    strcpy(command_line_str, ACCESS_ENG_MODE_STR);

    // CLI commands have to be on separate lines
    strcat(command_line_str, "\n");
    strcat(command_line_str, cli_cmds.name);
    strcat(command_line_str, "\n");

    //Write fbe cli command to script file in script mode
    if(!strcmp(current_control.mode,cli_mode_types[CLI_MODE_TYPE_SCRIPT])){
        if(!(write_text_to_file(current_control.cli_script_file_name, command_line_str))){
            mut_printf(MUT_LOG_LOW, "Cannot open script file: %s ", default_cli_script_file_name);
        }
    }

    // Exeucte the command line and get the output
    if(!execute_fbe_cli_command (command_line_str, &command_line_out)){
        mut_printf(MUT_LOG_LOW, "Cannot execute fbe cli command: %s ", command_line_str);
    }

    // Check expected keywords in the command line output 
    expected_keywords_check_result = check_expected_keywords(command_line_out,(const char **)expected_keywords);

    // Check unexpected keywords in the command line output 
    if(expected_keywords_check_result){
        unexpected_keywords_check_result = check_unexpected_keywords(command_line_out,(const char **)unexpected_keywords);
    }

    if (NULL != command_line_out) {
        csx_p_strfree(command_line_out);
    }
    return (expected_keywords_check_result && unexpected_keywords_check_result);

}


//Reset test control by default values
void reset_test_control(void){

    init_test_control(default_cli_executable_name,default_cli_script_file_name,default_cli_drive_types,default_cli_sp_types,default_cli_mode);

}
