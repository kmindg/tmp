/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_cli_tests.h
 ***************************************************************************
 *
 * DESCRIPTION: FBE cli tests header
 * 
 * HISTORY
 *   08/19/2010:  Created. hud1
 *
 ***************************************************************************/

#ifndef FBE_CLI_TEST_H
#define FBE_CLI_TEST_H

#include <stdio.h>
#include <string.h>

#include "mut.h"
#include "sep_utils.h"

#include "fbe/fbe_types.h"


// populates the fbe cli suite.  Add all new test creation to this function
mut_testsuite_t * fbe_test_create_fbe_cli_test_suite(mut_function_t startup, mut_function_t teardown);


#define CLI_STRING_MAX_LEN 400
#define CLI_KEYWORDS_ARRAY_LEN 16

// Define the Engineering Access Mode
#define ACCESS_ENG_MODE_STR         "access -m 1"

typedef enum cli_drive_type_e {
    CLI_DRIVE_TYPE_KERNEL,
    CLI_DRIVE_TYPE_SIMULATOR,
}cli_drive_type_t;

typedef enum cli_sp_type_e {
    CLI_SP_TYPE_SPA,
    CLI_SP_TYPE_SPB,
}cli_sp_type_t;

typedef enum cli_mode_type_e {
    CLI_MODE_TYPE_INTERACTIVE,
    CLI_MODE_TYPE_SCRIPT,
}cli_mode_type_t;

typedef struct fbe_cli_test_control_s {
    char cli_executable_name[CLI_STRING_MAX_LEN];
    char cli_script_file_name[CLI_STRING_MAX_LEN];
    char drive_type[CLI_STRING_MAX_LEN];
    char sp_type[CLI_STRING_MAX_LEN];
    char mode[CLI_STRING_MAX_LEN];
    fbe_u16_t sp_port_base;
} fbe_cli_test_control_t;

typedef struct fbe_cli_cmd_s {
    char* name;
    char* expected_keywords[CLI_KEYWORDS_ARRAY_LEN];
    char* unexpected_keywords[CLI_KEYWORDS_ARRAY_LEN];
} fbe_cli_cmd_t;

typedef fbe_status_t (*fbe_cli_config_function_t)(void);

extern struct fbe_cli_test_control_s current_control;
extern const char* cli_drive_types[];
extern const char*cli_sp_types[];
extern const char* cli_mode_types[];

extern const char* default_cli_executable_name ;
extern const char* default_cli_script_file_name;
extern  const cli_drive_type_t default_cli_drive_types;
extern  const cli_sp_type_t default_cli_sp_types;
extern  const cli_mode_type_t default_cli_mode;

extern char * rdt_test_short_desc;
extern char * rdt_test_long_desc;
extern char * rderr_test_short_desc;
extern char * rderr_test_long_desc;
extern char * rginfo_test_short_desc;
extern char * rginfo_test_long_desc;
extern char * stinfo_test_short_desc;
extern char * stinfo_test_long_desc;
extern char * luninfo_test_short_desc;
extern char * luninfo_test_long_desc;
extern char * odt_test_short_desc;
extern char * odt_test_long_desc;
extern char * get_prom_info_test_short_desc;
extern char * get_prom_info_test_long_desc;
extern char * bgo_enable_disable_test_short_desc;
extern char * bgo_enable_disable_test_long_desc;
extern char * ddt_test_short_desc;
extern char * ddt_test_long_desc;
extern char * systemdump_test_short_desc;
extern char * systemdump_test_long_desc;


// setup functions of tests
void fbe_cli_ldo_setup(void);
void fbe_cli_pdo_setup(void);
void fbe_cli_rdt_setup(void);
void fbe_cli_rderr_setup(void);
void fbe_cli_rginfo_setup(void);
void fbe_cli_stinfo_setup(void);
void fbe_cli_luninfo_setup(void);
void fbe_cli_odt_setup(void);
void fbe_cli_sfpinfo_setup(void);
void fbe_cli_getwwn_setup(void);
void fbe_cli_db_transaction_setup(void);
void fbe_cli_forcerpread_setup(void);
void fbe_cli_get_prom_info_setup(void);
void fbe_cli_bgo_enable_disable_setup(void);

void fbe_cli_system_db_interface_setup(void);
void fbe_cli_sepls_setup(void);
void fbe_cli_ddt_setup(void);
void fbe_cli_systemdump_setup(void);


// tests of fbecli commands
void fbe_cli_ldo_test(void);
void fbe_cli_pdo_test(void);
void fbe_cli_rdt_test(void);
void fbe_cli_rderr_test(void);
void fbe_cli_rginfo_test(void);
void fbe_cli_stinfo_test(void);
void fbe_cli_luninfo_test(void);
void fbe_cli_odt_test(void);
void fbe_cli_sfpinfo_test(void);
void fbe_cli_getwwn_test(void);
void fbe_cli_db_transaction_test(void);
void fbe_cli_forcerpread_test(void);
void fbe_cli_get_prom_info_test(void);
void fbe_cli_system_db_interface_test(void);
void fbe_cli_sepls_test(void);
void fbe_cli_ddt_test(void);
void fbe_cli_systemdump_test(void);

void fbe_cli_bgo_enable_disable_test(void);

// teardown functions of tests
void fbe_cli_ldo_teardown(void);
void fbe_cli_pdo_teardown(void);
void fbe_cli_rdt_teardown(void);
void fbe_cli_rderr_teardown(void);
void fbe_cli_rginfo_teardown(void);
void fbe_cli_stinfo_teardown(void);
void fbe_cli_luninfo_teardown(void);
void fbe_cli_odt_teardown(void);
void fbe_cli_sfpinfo_teardown(void);
void fbe_cli_getwwn_teardown(void);
void fbe_cli_db_transaction_teardown(void);
void fbe_cli_forcerpread_teardown(void);
void fbe_cli_system_db_interface_teardown(void);
void fbe_cli_get_prom_info_teardown(void);
void fbe_cli_sepls_teardown(void);
void fbe_cli_ddt_teardown(void);
void fbe_cli_systemdump_teardown(void);


void fbe_cli_bgo_enable_disable_teardown(void);


// function to execute fbe cli command and get command output
BOOL execute_fbe_cli_command (const char* fbe_cli_command,char** console_output);

// check expected keywords in a string(output)
BOOL check_expected_keywords(const char* str,const char** expected_keywords);

// check unexpected keywords in a string(output)
BOOL check_unexpected_keywords(const char* str,const char** unexpected_keywords);

// function to write text to a file
BOOL write_text_to_file(const char* file_name,const char* content);

// initialize test control
void init_test_control(const char* cli_executable_name, const char* cli_script_file_name,const cli_drive_type_t drive_type,const cli_sp_type_t sp_type,const cli_mode_type_t mode);

fbe_bool_t fbe_cli_replace_string(const fbe_char_t *input_str, 
                                  const fbe_char_t *old_str, 
                                  const fbe_char_t *new_str, 
                                  fbe_char_t *ret_str);
fbe_bool_t fbe_cli_find_string(const fbe_char_t *input_str, 
                               const fbe_char_t *find_str);
fbe_bool_t fbe_cli_run_single_cli_cmd(fbe_test_rg_configuration_t *rg_config_p,
                          fbe_cli_cmd_t single_cli_cmd,
                          fbe_char_t *object_id_str,
                          fbe_char_t *new_object_id_str);
void fbe_cli_run_all_cli_cmds(fbe_test_rg_configuration_t *rg_config_p,
                              fbe_cli_cmd_t* cli_cmds,
                              fbe_u32_t max_cli_cmds);
fbe_status_t fbe_test_cli_run_cmds(fbe_cli_cmd_t *cli_cmds_p);
fbe_u32_t fbe_test_cli_cmd_get_count(fbe_cli_cmd_t *cli_cmds_p);

// config functions
fbe_status_t fbe_test_load_pdo_config(void);
fbe_status_t fbe_test_load_ldo_config(void);
void fbe_test_load_rdt_config(fbe_test_rg_configuration_array_t* array_p);
void fbe_test_load_rderr_config(fbe_test_rg_configuration_array_t* array_p);
void fbe_test_load_rginfo_config(fbe_test_rg_configuration_array_t* array_p);
void fbe_test_load_sepls_config(fbe_test_rg_configuration_array_t* array_p);

// common setup and teardwon functions
fbe_status_t fbe_cli_common_setup(fbe_cli_config_function_t config_function);
fbe_status_t fbe_cli_common_teardown(void);

// to simplify writting case, here is an example to create a base test
BOOL fbe_cli_base_test(fbe_cli_cmd_t cli_cmds);

// reset test control by defautl values
void reset_test_control(void);

// set sp port base
void fbe_cli_tests_set_port_base(fbe_u16_t sp_port_base);

void fbe_cli_mminfo_setup(void);
void fbe_cli_mminfo_test(void);
void fbe_cli_mminfo_teardown(void);

#endif 
