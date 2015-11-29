#ifndef FBE_TEST_H
#define FBE_TEST_H

/*******************************************************************
 * Copyright (C) EMC Corporation 2008-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/

/*******************************************************************
 *  fbe_test_utils.h
 *******************************************************************
 *
 * @brief
 *    This file contains definitions and APIs for the `fbe_test'
 *    simulation code.
 *
 * @version
 *  Initial
 *
 *******************************************************************/

/******************************
 * INCLUDES
 ******************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_sim_transport.h"


/******************************
 * LITERAL DEFINITIONS.
 ******************************/

/*!*******************************************************************
 * @def     FBE_TEST_NUM_OF_PORTS_RESERVED_FOR_SP
 *********************************************************************
 * @brief   Number of port reserved for future per sp
 *
 *********************************************************************/
#define FBE_TEST_NUM_OF_PORTS_RESERVED_FOR_SP           10

/*!*******************************************************************
 * @def     FBE_TEST_NUM_OF_PORTS_RESERVED_FOR_SP
 *********************************************************************
 * @brief   Number of port reserved for future per sp
 *
 *********************************************************************/
#define FBE_TEST_NUM_OF_PORTS_RESERVED_FOR_DISK_SERVER  1

#define FBE_TEST_PID_LOG_ENTRY_SIZE 100
#define FBE_TEST_PROC_NAME_SIZE 30
#define FBE_TEST_LOGDIR_OPTION "-logDir"

typedef enum fbe_test_process_e{
        FBE_TEST_SPA,       /* 0 */
        FBE_TEST_SPB,       /* 1 */
        FBE_TEST_MAIN,      /* 2 */
        FBE_TEST_DRIVE_SIM, /* 3 */
        FBE_TEST_LAST
}fbe_test_process_t;

typedef struct fbe_test_process_info_s{
    fbe_api_sim_server_pid pid;
    char process_name[FBE_TEST_PROC_NAME_SIZE];
    char log_entry[FBE_TEST_PID_LOG_ENTRY_SIZE];
}fbe_test_process_info_t;

#include "fbe/fbe_api_transport.h"
typedef void(*cmd_fun)(fbe_transport_connection_target_t which_sp, const char *option_name);
typedef void(*cmd_fun_sim)(fbe_sim_transport_connection_target_t which_sp, const char *option_name);
/*!***************************************************************************
 * @def     cmd_options[]
 ***************************************************************************** 
 * 
 * @brief   Command line options for `fbe_test' simulation
 *
 *****************************************************************************/
typedef struct fbe_test_cmd_options_s 
{
    const char * name;
    cmd_fun_sim func;
    int least_num_of_args;
    BOOL show;
    const char * description;
} fbe_test_cmd_options_t;

/*************************************** 
 * Imports for visibility.
 ***************************************/

/***************************************
 * fbe_test_main.c  
 ***************************************/
fbe_test_cmd_options_t *fbe_test_main_get_cmd_options(void);
fbe_u32_t fbe_test_main_get_number_cmd_options(void);

/***************************************
 * fbe_test_utils.c 
 ***************************************/
fbe_status_t fbe_test_util_store_process_ids(void);
void fbe_test_util_set_console_title(int argc , char **argv);
void fbe_test_util_generate_self_test_arg(int *argc_p, char ***argv_p);
void fbe_test_util_free_self_test_arg(int argc, char **argv);
void fbe_test_util_process_fbe_io_delay(void);
fbe_bool_t fbe_test_util_is_self_test_enabled(int argc, char **argv);

/***************************************
 * fbe_test_sp_sim.c 
 ***************************************/
fbe_status_t fbe_test_sp_sim_store_sp_pids(void);
void fbe_test_sp_sim_kill_orphan_sp_process(void);
void fbe_test_sp_sim_start(fbe_sim_transport_connection_target_t which_sp);
void fbe_test_sp_sim_stop(void);
void fbe_test_sp_sim_stop_single_sp(fbe_test_process_t which_sp);
void fbe_test_sp_sim_start_notification_single_sp(fbe_sim_transport_connection_target_t which_sp);
void fbe_test_sp_sim_start_notification(void);
void fbe_test_sp_sim_process_cmd_bat(fbe_sim_transport_connection_target_t which_sp, const char *option_name);
void fbe_test_sp_sim_process_cmd_debugger(fbe_sim_transport_connection_target_t which_sp, const char *option_name);
void fbe_test_sp_sim_process_cmd_debug(fbe_sim_transport_connection_target_t which_sp, const char *option_name);
void fbe_test_sp_sim_process_cmd_sp_log(fbe_sim_transport_connection_target_t which_sp, const char *option_name);
void fbe_test_sp_sim_process_cmd_port_base(fbe_sim_transport_connection_target_t which_sp, const char *option_name);
fbe_test_process_t fbe_test_conn_target_to_sp(fbe_transport_connection_target_t conn_target);
fbe_bool_t fbe_test_is_sp_up(fbe_test_process_t which_sp);
void fbe_test_mark_sp_down(fbe_test_process_t which_sp);
void fbe_test_mark_sp_up(fbe_test_process_t which_sp);

/***************************************
 * fbe_test_drive_sim.c 
 ***************************************/
void fbe_test_drive_sim_set_drive_server_port(fbe_u16_t drive_server_port);
fbe_u16_t fbe_test_drive_sim_get_drive_server_port(void);
void fbe_test_drive_sim_kill_orphan_drive_server_process(void);
void fbe_test_drive_sim_force_drive_sim_remote_memory(void);
void fbe_test_drive_sim_restore_default_drive_sim_type(void);
fbe_status_t fbe_test_drive_sim_start_drive_sim(fbe_bool_t running);
fbe_status_t fbe_test_drive_sim_start_drive_sim_for_sp(fbe_bool_t running,
                                                       fbe_sim_transport_connection_target_t sp);
fbe_status_t fbe_test_drive_sim_destroy_drive_sim(void);

void fbe_test_dualsp_suite_teardown(void);
void fbe_test_dualsp_suite_startup(void);
void fbe_test_base_suite_startup(void);
void fbe_test_base_suite_destroy_sp(void);
void fbe_test_base_suite_startup_single_sp(fbe_sim_transport_connection_target_t sp);
void fbe_test_base_suite_startup_single_sp_post_powercycle(fbe_sim_transport_connection_target_t sp);
void fbe_test_base_suite_startup_post_powercycle(void);
int __cdecl fbe_test_main (int argc , char **argv, unsigned long timeout);

#endif /* FBE_TEST_H */

