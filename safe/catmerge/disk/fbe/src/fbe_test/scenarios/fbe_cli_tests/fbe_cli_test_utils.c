/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_cli_test_utils.c
 ***************************************************************************
 *
 * DESCRIPTION
 * Implementation of common util functions for fbe cli tests.
 * 
 * HISTORY
 *   08/19/2010:  Created. hud1
 *
 ***************************************************************************/

#include "fbe_cli_tests.h"
#include "fbe/fbe_api_physical_drive_interface.h"


/**************************************************************************
 *  check_expected_keywords()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function checks expected keywords in a string.
 *    Support check up to 16 expected keywords 
 *
 *  PARAMETERS:
 *    str, expected_keywords
 *
 *  RETURN VALUES/ERRORS:
 *    TRUE: all expected keywords are found 
 *....FALSE: (1)any expected keyword is not found even other keywords are found
 *           (2)parameter is invalid
 *
 *  NOTES:
 *
 *  HISTORY:
 *    08/16/2010:  Created. hud1
 **************************************************************************/

BOOL check_expected_keywords(const char* str,const char** expected_keywords) {

	int index = 0;
	BOOL result = TRUE;

	char *pdest;

	//check parameter str
	if(str == NULL){
		mut_printf(MUT_LOG_LOW, "The output of this fbe cli command is null.");
		return FALSE;
	}
	
	//check parameter expected_keywords
	if(expected_keywords == NULL){
		mut_printf(MUT_LOG_LOW, "Can not find expected keywords. Please check the fbe cli command, make sure it is a valid command.");
		return FALSE;
	}

	for(index = 0; index < CLI_KEYWORDS_ARRAY_LEN; index++){

		if(expected_keywords[index] == NULL)
			break;

		pdest = strstr(str,expected_keywords[index]);

		if(pdest != NULL){
			mut_printf(MUT_LOG_LOW, "Found expected keyword: \"%s\" ", expected_keywords[index]);
		}else{
			mut_printf(MUT_LOG_LOW, "Did not find expected keyword: \"%s\" ", expected_keywords[index]);
			result = FALSE;
			break;
		}
	}

	return result;

}

/**************************************************************************
 *  check_unexpected_keywords()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function checks unexpected keywords in a string.
 *    Support check up to 16 unexpected keywords
 *
 *  PARAMETERS:
 *    str, unexpected_keywords
 *
 *  RETURN VALUES/ERRORS:
 *    TRUE: all unexpected keywords are NOT found 
 *....FALSE: (1)any unexpected is found 
 *           (2)parameter is invalid
 *
 *  NOTES:
 *
 *  HISTORY:
 *    08/16/2010:  Created. hud1
 **************************************************************************/

BOOL check_unexpected_keywords(const char* str,const char** unexpected_keywords) {

	int index = 0;
	BOOL result = TRUE;

	char *pdest;

	//check parameter str
	if(str == NULL){
		mut_printf(MUT_LOG_LOW, "The output of this fbe cli command is null.");
		return FALSE;
	}
	
	//check parameter unexpected_keywords
	if(unexpected_keywords == NULL){
		mut_printf(MUT_LOG_LOW, "Can not find unexpected keywords. Please check the fbe cli command, make sure it is a valid command.");
		return FALSE;
	}

	for(index = 0; index < CLI_KEYWORDS_ARRAY_LEN; index++){

		if(unexpected_keywords[index] == NULL)
			break;

		pdest = strstr(str,unexpected_keywords[index]);

		if(pdest != NULL){
			mut_printf(MUT_LOG_LOW, "Found unexpected keyword: \"%s\" ", unexpected_keywords[index]);
			result = FALSE;
			break;
		}else{
			mut_printf(MUT_LOG_LOW, "Did not find unexpected keyword: \"%s\" ", unexpected_keywords[index]);
		}
	}

	return result;

}

/**************************************************************************
 *  init_test_control()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function initializes test control
 *
 *  PARAMETERS:
 *    cli_executable_name, cli_script_file_name,drive_type,sp_type,mode
 *
 *  RETURN VALUES/ERRORS:
 *
 *  NOTES:
 *
 *  HISTORY:
 *   .08/29/2010:  Created. hud1
 **************************************************************************/
void init_test_control(const char* cli_executable_name, const char* cli_script_file_name,const cli_drive_type_t drive_type,const cli_sp_type_t sp_type,const cli_mode_type_t mode){

	strcpy(current_control.cli_executable_name,cli_executable_name);
	strcpy(current_control.cli_script_file_name,cli_script_file_name);
	strcpy(current_control.drive_type,cli_drive_types[drive_type]);
	strcpy(current_control.sp_type,cli_sp_types[sp_type]);
	strcpy(current_control.mode,cli_mode_types[mode]);

}

/**************************************************************************
 *  write_text_to_file()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function writes text content to a file.
 *
 *  PARAMETERS:
 *    file_name, content
 *
 *  RETURN VALUES/ERRORS:
 *
 *  NOTES:
 *
 *  HISTORY:
 *    08/16/2010:  Created. hud1
 **************************************************************************/

BOOL write_text_to_file(const char* file_name,const char* content)
{

	FILE *fp;

	if((fp = fopen(file_name, "w"))==NULL) {
		return FALSE;
	}

	fprintf(fp, "%s %s",content, "\n");
	fclose(fp);

	return TRUE;

}

/**************************************************************************
 *  fbe_cli_tests_set_port_base()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets sp port base which fbecli connects to.
 *
 *  PARAMETERS:
 *    port_base
 *
 *  RETURN VALUES/ERRORS:
 *
 *  NOTES:
 *
 *  HISTORY:
 *    10/14/2010:  Created. gaob3
 **************************************************************************/
void fbe_cli_tests_set_port_base(fbe_u16_t sp_port_base)
{
    current_control.sp_port_base = sp_port_base;
}

/*!**************************************************************
 * fbe_cli_find_string()
 ****************************************************************
 * @brief
 *  This function returns true if the input string contains find string
 *
 * @param input_str - Input string in which old string to replace.
 * @param find_str - string to find
 *
 * @return fbe_bool_t - if FBE_TRUE if string is found at least once
 *  else FBE_FALSE
 *  
 *
 * @author
 *  11/15/2010  Swati Fursule  - Created.
 *
 ****************************************************************/
fbe_bool_t fbe_cli_find_string(const fbe_char_t *input_str, 
                               const fbe_char_t *find_str)
{
    fbe_bool_t is_found = FBE_FALSE;
    fbe_u32_t findlen = (fbe_u32_t)strlen(find_str);

    while (*input_str) 
    {
        if (strstr(input_str, find_str) == input_str) 
        {
            input_str += findlen;
            is_found = FBE_TRUE;
        }
        else
        {
            input_str++;
        }
    }
    return is_found;
}
/*************************
 * end fbe_cli_find_string
 *************************/


/*!**************************************************************
 * fbe_cli_replace_string()
 ****************************************************************
 * @brief
 *  This function replaces the old_str string with new_str in 
 *  input_str and return ret_str. Ret string should be allocated
 *  by caller.
 *
 * @param input_str - Input string in which old string to replace.
 * @param old_str - string to replace
 * @param new_str - new string to replace with 
 * @param ret_str - return string containing replaced strings
 *
 * @return fbe_bool_t - if FBE_TRUE if string is replaced at least once
 *  else FBE_FALSE
 *  
 *
 * @author
 *  11/15/2010  Swati Fursule  - Created.
 *
 ****************************************************************/
fbe_bool_t fbe_cli_replace_string(const fbe_char_t *input_str, 
                                  const fbe_char_t *old_str, 
                                  const fbe_char_t *new_str, 
                                  fbe_char_t *ret_str)
{
    fbe_bool_t is_replaced = FBE_FALSE;
    fbe_u32_t i = 0;
    fbe_u32_t newlen = (fbe_u32_t)strlen(new_str);
    fbe_u32_t oldlen = (fbe_u32_t)strlen(old_str);
    if ((input_str == NULL) ||
        (old_str == NULL) ||
        (new_str == NULL) ||
        (ret_str == NULL) )
    {
        return is_replaced;
    }
    while (*input_str) 
    {
        if (strstr(input_str, old_str) == input_str) 
        {
            strcpy(&ret_str[i], new_str);
            i += newlen;
            input_str += oldlen;
            is_replaced = FBE_TRUE;
        } 
        else
        {
            ret_str[i++] = *input_str++;
        }
    }
    ret_str[i] = '\0';

    return is_replaced;
}
/*************************
 * end fbe_cli_replace_string
 *************************/

/*!**************************************************************
 * fbe_cli_run_single_cli_cmd()
 ****************************************************************
 * @brief
 *  Test function execute one entry from cli_cmd_and_keywords
 *
 * @param rg_config_p
 * @param single_cli_cmd
 * @param object_id_str
 * @param new_object_id_str
 *
 * @return fbe_bool_t.   
 *
 * @revision
 *  10/20/2010 - Created. Swati Fursule
 *
 ****************************************************************/
fbe_bool_t fbe_cli_run_single_cli_cmd(fbe_test_rg_configuration_t *rg_config_p,
                          fbe_cli_cmd_t single_cli_cmd,
                          fbe_char_t *object_id_str,
                          fbe_char_t *new_object_id_str)
{
    fbe_bool_t result;
    fbe_bool_t is_replaced = FBE_FALSE;
    fbe_cli_cmd_t         new_cmd_struct;
    fbe_char_t            cmd_name[CLI_STRING_MAX_LEN]= {"\0"};
    fbe_u32_t             keyword_arr_len = 0;
    /*Check if command contains object id which needs to be  replaced with the value obtained via fbe api*/
    is_replaced = fbe_cli_replace_string(single_cli_cmd.name,
                                         object_id_str,/*"0xffffffff"*/
                                         new_object_id_str, 
                                         cmd_name);

    if(is_replaced)
    {
        new_cmd_struct.name = cmd_name;
        while(keyword_arr_len < CLI_KEYWORDS_ARRAY_LEN)
        {
            new_cmd_struct.expected_keywords[keyword_arr_len] = single_cli_cmd.expected_keywords[keyword_arr_len];
            new_cmd_struct.unexpected_keywords[keyword_arr_len] = single_cli_cmd.unexpected_keywords[keyword_arr_len];
            keyword_arr_len++;
        }
    }
    else
    {
        new_cmd_struct = single_cli_cmd;
    }
    /* As an example, all expected keywords are found and unexpected keywords are not found, the result is TRUE, so this test is passed*/
    result = fbe_cli_base_test(new_cmd_struct);
    return result;
}
/******************************************
 * end fbe_cli_run_single_cli_cmd()
 ******************************************/
/*!**************************************************************
 * fbe_cli_run_all_cli_cmds()
 ****************************************************************
 * @brief
 *  Test function execute all cli commands from cli_cmd_and_keywords
 *  This also takes care of replacing object ids (Rg and PDO) with 
 *  object id recieved from fbe_api.
 *
 * @param rg_config_p
 * @param cli_cmds
 * @param max_cli_cmds
 *
 * @return NONE
 *
 * @revision
 *  10/20/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_run_all_cli_cmds(fbe_test_rg_configuration_t *rg_config_p,
                              fbe_cli_cmd_t* cli_cmds,
                              fbe_u32_t max_cli_cmds)
{
    fbe_bool_t result;
    fbe_u32_t number = 0;
    /* run all cli rdt commands*/
    while(number<max_cli_cmds)
    {
        if (fbe_cli_find_string(cli_cmds[number].name, "0xffffffff"))
        {
            fbe_char_t            buffer[10];
            fbe_object_id_t       rg_object_id;

            fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
             /*get string for object id*/
            _snprintf(buffer, 10, "0x%x", rg_object_id);

            result = fbe_cli_run_single_cli_cmd(rg_config_p, cli_cmds[number], "0xffffffff", buffer);
        }
        else if (fbe_cli_find_string(cli_cmds[number].name, "0xfffffpdo"))
        {
            fbe_char_t            buffer[10];
            fbe_u32_t port_number;
            fbe_u32_t encl_number;
            fbe_u32_t slot_number;
            fbe_status_t status;
            fbe_object_id_t       pdo_object_id;

            port_number = rg_config_p->rg_disk_set[0].bus;
            encl_number = rg_config_p->rg_disk_set[0].enclosure;
            slot_number = rg_config_p->rg_disk_set[0].slot;
            status = fbe_api_get_physical_drive_object_id_by_location(port_number, encl_number, slot_number, &pdo_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(pdo_object_id, FBE_OBJECT_ID_INVALID);

             /*get string for object id*/
            _snprintf(buffer, 10, "0x%x", pdo_object_id);
            result = fbe_cli_run_single_cli_cmd(rg_config_p, cli_cmds[number], "0xfffffpdo", buffer);
        }
        else
        {
            result = fbe_cli_base_test(cli_cmds[number]);
        }
        number++;
        MUT_ASSERT_TRUE(result);
    }
}
/******************************************
 * end fbe_cli_run_single_cli_cmd()
 ******************************************/


/*!**************************************************************
 * fbe_test_cli_cmd_get_count()
 ****************************************************************
 * @brief
 *  Get the number of test cases in the array.
 *
 * @param cli_cmds - Array of test cases               
 *
 * @return count of test cases.
 *
 ****************************************************************/
fbe_u32_t fbe_test_cli_cmd_get_count(fbe_cli_cmd_t *cli_cmds_p)
{
    fbe_u32_t num_cases = 0;

    /* Loop until we find the table number or we hit the terminator.
     */
    while (cli_cmds_p->name != NULL)
    {
        cli_cmds_p++;
        num_cases++;
    }
    return num_cases;
}
/**************************************
 * end fbe_test_cli_cmd_get_count()
 **************************************/

/*!**************************************************************
 * fbe_test_cli_run_cmds()
 ****************************************************************
 * @brief
 *  Get the number of test cases in the array.
 *
 * @param cli_cmds - Array of test cases               
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_cli_run_cmds(fbe_cli_cmd_t *cli_cmds_p)
{
    fbe_u32_t count = fbe_test_cli_cmd_get_count(cli_cmds_p);
    fbe_u32_t index;
    fbe_bool_t b_result;

    for ( index = 0; index < count; index++)
    {
        b_result = fbe_cli_base_test(cli_cmds_p[index]);
        MUT_ASSERT_TRUE(b_result);
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_test_cli_run_cmds()
 **************************************/

/*************************
 * end file fbe_cli_test_utils.c
 *************************/
