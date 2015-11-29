/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file    fbe_hw_test_main.c
 ***************************************************************************
 *
 * @brief   This file contains the code to run the FBE test suite(s) on hardware.
 *
 * @version
 *          Initial version
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#define I_AM_NATIVE_CODE
#include "hw_tests.h"
#include "physical_package_tests.h"

#include "fbe/fbe_api_transport.h"
#include "fbe/fbe_api_transport_packet_interface.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"

#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_file.h"

#include "system_tests.h"
#include "fbe_test_common_utils.h"
#include "sep_utils.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe/fbe_api_user_server.h"
#include "fbe_test.h"
#include "sep_tests.h"
#include <windows.h>
#include "fbe/fbe_emcutil_shell_include.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_board.h"

/******************************
 *  FUNCTION PROTOTYPES
 ******************************/
static void fbe_hw_test_run_test_suites(void);
static void fbe_test_hw_suite_startup(void);
static void fbe_test_hw_suite_teardown(void);
static void fbe_test_hw_setup_sp(fbe_transport_connection_target_t sp);

void process_cmd_options(void);
void process_cmd_spa_ip(const char *option_name);
void process_cmd_spb_ip(const char *option_name);


/*****************************
 *   GLOBALS
 ******************************/
static fbe_bool_t remote_spa = FBE_FALSE, remote_spb = FBE_FALSE;
static fbe_u32_t connect_retry_times = 0; // 0 means infinitely retry

#define DEFAULT_CONNECT_RERY_TIMES 10
#define CMD_ARGV_MAX_NUM  8


static struct {
    const char * name;
    void (*func)(const char*);
    int least_num_of_args;
    BOOL show;
    const char * description;
} cmd_options[] = {
    {"-spa_ip",      process_cmd_spa_ip,      1, TRUE, "-spa_ip <spa server ip address>. 127.0.0.1 by default."},
    {"-spb_ip",      process_cmd_spb_ip,      1, TRUE, "-spb_ip <spb server ip address>. 127.0.0.1 by default."},
    {"-fbe_io_seconds",          NULL, 1, TRUE, "extends the default I/O time"},
    {"-fbe_threads",             NULL, 1, TRUE, "using thread count"},
    {"-fbe_small_io_count",      NULL, 1, TRUE, "using small io count"},
    {"-fbe_large_io_count",      NULL, 1, TRUE, "using large io count"},
    {"-fbe_default_trace_level", NULL, 1, TRUE, "using trace level"},
    {"-raid0_only",  NULL, 0, TRUE, "only test raid 0"},
    {"-raid1_only",  NULL, 0, TRUE, "only test raid 1"},
    {"-raid3_only",  NULL, 0, TRUE, "only test raid 3"},
    {"-raid5_only",  NULL, 0, TRUE, "only test raid 5"},
    {"-raid6_only",  NULL, 0, TRUE, "only test raid 6"},
    {"-raid10_only", NULL, 0, TRUE, "only test raid 10"},
    {"-parity_only", NULL, 0, TRUE, "only test raid 5, 3 and 6"},
    {"-mirror_only", NULL, 0, TRUE, "only test raid 1 and 10"},
    {"-raid_test_level", NULL, 1, TRUE, "or [-rtl] - Set raid test level (0-2)."},
    {"-rtl", NULL, 1, FALSE, "Set raid test level (0-2)"}, // Short alias to -raid_test_level
    {"-extended_test_level", NULL, 1, TRUE, "Set extended test level (0-2)"},
    {"-drive_insert_delay", NULL, 1, TRUE, "Set drive insert delay of: x milliseconds"},
    {"-drive_remove_delay", NULL, 1, TRUE, "Set drive remove delay of: x milliseconds"},
    {"-raid_library_debug_flags", NULL, 1, TRUE, "Set raid library debug flags for each object."},
    {"-raid_library_default_debug_flags", NULL, 1, TRUE, "Set raid library default debug flags."},
    {"-stop_sep_neit_on_error", NULL, 0, TRUE, "Stop the SP when it encounters trace of error level or critical error level in sep or neit packages."},
    {"-stop_sector_trace_on_error", NULL, 0, TRUE, "Stop the SP when it encounters specific type of sector trace."},
    {"-sector_trace_debug_flags", NULL, 1, TRUE, "Set sector trace debug flags, ignored in tests injecting errors."},
    {"-degraded_only", NULL, 0, FALSE,  "Only run degraded cases. Cookie Monster only."},
    {"-start_table", NULL, 1, FALSE, "First error table to use. Kermit only."},
    {"-start_index", NULL, 1, FALSE, "First index in the table to use. Kermit only."},
    {"-total_cases", NULL, 1, FALSE, "Total number of error cases to use. Kermit only."},
    {"-repeat_count", NULL, 1, FALSE, "Repeat each test case this many times. Kermit only."},
    {"-abort_cases_only", NULL, 0, FALSE, "Only test abort cases. Kermit only."},
    {"-fail_drive", NULL, 1, TRUE, "Fail a drive at onset of test suite"},
    {"-full_db_drive_rebuild", NULL, 0, FALSE, "Complete the DB drive rebuild fully. diego_test only."},
    {"-take_new_baseline", NULL, 0, TRUE, "Take new baseline for performance suite tests (if threshhold exceeded)"},
    {"-perf_suite_scaleout", NULL, 0, TRUE, "Test several LUN counts for performance suite tests instead of just maximum config."},
};

enum 
{
    CMD_NUM_OF_OPTIONS = (sizeof(cmd_options) / sizeof(cmd_options[0])),
};

static char spa_ip[30] = "127.0.0.1";
static char spb_ip[30] = "127.0.0.1";

/* @!TODO: Temporary hack - should be removed */
fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_PHYSICAL;
    return FBE_STATUS_OK;
}

/*!***************************************************************************
 *          fbe_test_hw_suite_startup()
 ***************************************************************************** 
 * 
 * @brief   This is the startup function for the FBE tests to run on hardware.
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
static void fbe_test_hw_suite_startup(void) 
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_board_get_base_board_info_t base_board_info;
    fbe_object_id_t board_object_id;
    fbe_object_id_t *pvd_list;
    fbe_u32_t       pvd_count;
    fbe_u32_t       pvd_index;
    csx_p_thr_priority_e prev_priority;

    (void) csx_p_thr_prioritize(NULL, CSX_P_THR_PRIORITY_LOW, &prev_priority);

    /*initialize the simulation side of fbe api*/
    status = fbe_api_common_init_sim();

    /*set function entries for commands that would go to the physical package*/
    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_PHYSICAL,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);

    /*set function entries for commands that would go to the sep package*/
    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_SEP_0,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);
    /*set function entries for commands that would go to the neit package*/
    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_NEIT,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);

    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_ESP,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);
    
    /*need to initialize the client to connect to the server*/
    fbe_api_transport_reset_client(FBE_CLIENT_MODE_DEV_PC);

    status = fbe_api_transport_set_unregister_on_connect(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    /* need to do some additional setup on each SP */
    if (remote_spa)                                  
    {                                                

        status = fbe_api_transport_init_client(spa_ip, FBE_TRANSPORT_SP_A, FBE_TRUE, connect_retry_times);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,("\nCan't connect to FBE API Server on SPA, make sure FBE is running !!!\n"))
        
        fbe_test_hw_setup_sp(FBE_TRANSPORT_SP_A); 
        fbe_api_sim_transport_set_target_server(FBE_TRANSPORT_SP_A);
    }                                                
    if (remote_spb)                                  
    {                                                
        status = fbe_api_transport_init_client(spb_ip, FBE_TRANSPORT_SP_B, FBE_TRUE, connect_retry_times);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,("\nCan't connect to FBE API Server on SPB, make sure FBE is running !!!\n"))
       
        fbe_test_hw_setup_sp(FBE_TRANSPORT_SP_B);
        fbe_api_sim_transport_set_target_server(FBE_TRANSPORT_SP_B);
    }                                                
                                                     

    /* Remove any config on the array since the previous test might not have
     * torn down the config. 
     * All tests assume a clean slate when they start. 
     */
    status = fbe_test_sep_util_destroy_all_user_luns();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_destroy_all_user_raid_groups();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //clear all PVDs of EOL and persisted faults
    status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE, FBE_PACKAGE_ID_SEP_0, &pvd_list, &pvd_count);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    for (pvd_index = 0; pvd_index < pvd_count; pvd_index++) {
        status = fbe_api_provision_drive_clear_swap_pending(*(pvd_list+pvd_index));
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_provision_drive_clear_drive_fault(*(pvd_list+pvd_index));
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_provision_drive_clear_eol_state(*(pvd_list+pvd_index), FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    fbe_api_free_memory(pvd_list);

     /*! @todo we need to disable all error injection from protocol error injection  
     *        and from the logical error injection service.
     *  
     *        A prior test might not have completed and could have left errors being injected.
     */

    /* Initialize the PSL */
    status = fbe_api_get_board_object_id(&board_object_id);
    status = fbe_api_board_get_base_board_info(board_object_id, &base_board_info);
    status = fbe_private_space_layout_initialize_library(base_board_info.platformInfo.platformType);

}
/* end of fbe_test_hw_suite_startup() */


/*!***************************************************************************
 *          fbe_test_hw_suite_teardown()
 ***************************************************************************** 
 * 
 * @brief   This is the teardown function for the FBE tests to run on hardware.
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
static void fbe_test_hw_suite_teardown(void)
{
    fbe_status_t    status;

    fbe_api_transport_destroy_client(FBE_TRANSPORT_SP_A);
    fbe_api_transport_destroy_client(FBE_TRANSPORT_SP_B);

    status = fbe_api_common_destroy_sim();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/* end of fbe_test_hw_suite_teardown() */


/*!***************************************************************************
 *          fbe_test_hw_setup_sp()
 ***************************************************************************** 
 * 
 * @brief   This function performs some test setup for the specified SP.
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
static void fbe_test_hw_setup_sp(fbe_transport_connection_target_t sp)
{
    fbe_status_t    status;

    /* Set target server.
     */
    fbe_api_transport_set_target_server(sp);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: init %s",  __FUNCTION__, sp == FBE_TRANSPORT_SP_A ? "SP A" : "SP B");

    /* Make sure jobs will notify us.
     */
    status = fbe_api_common_init_job_notification();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Set control entry for drivers on SP server side.
     */
    status = fbe_api_user_server_set_driver_entries();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Register notification elements on SP server side.
     */
    status = fbe_api_user_server_register_notifications();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Reset the counters so we only see errors hit by this test.
     */
    status = fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

     /* Try to release rdgen memory first so we don't throw an error if it's already initialized */
    status = fbe_api_rdgen_release_dps_memory(); 

    /* Get memory from CMM since SP cache is not up when we enable fbe test mode. */
	status = fbe_api_rdgen_init_dps_memory(FBE_API_RDGEN_DEFAULT_MEM_SIZE_MB,
                                           FBE_RDGEN_MEMORY_TYPE_CMM); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/* end of fbe_test_hw_setup_sp() */


/*!***************************************************************************
 *          main()
 ***************************************************************************** 
 * 
 * @brief   This is the main function for this executable.  It creates and
 *          executes the FBE test suite(s).
 *
 * @param   argc - number of command line arguments
 *          argv - pointer to command line arguments
 *
 * @return  0 - Success   
 *
 *****************************************************************************/
#define FBE_HW_TEST_NOMINAL_TIMEOUT 1000
int __cdecl main(int argc , char **argv)
{
    int i;

#include "fbe/fbe_emcutil_shell_maincode.h"

    /* Mark that we are really testing on the hardware.
     */
    fbe_test_util_set_hardware_mode();
    reset_arguments(argc,argv);

    /* Register the pre-defined user options 
     */
    for (i = 0; i < sizeof(cmd_options) / sizeof(cmd_options[0]); ++i)
    {
        /* must be called before mut_init() */
        mut_register_user_option(cmd_options[i].name, cmd_options[i].least_num_of_args, cmd_options[i].show, cmd_options[i].description);
    }

    /* Initialize MUT infrastructure
     */
    mut_init(argc, argv);

    process_cmd_options();

    set_global_timeout(FBE_HW_TEST_NOMINAL_TIMEOUT);

    /* Run the test suites.
     * Note that if a test number is specified, the MUT infrastructure will process that request accordingly. 
     * If the -run_testsuites option is specified, for example, MUT will handle that processing as well.
     */
    fbe_hw_test_run_test_suites();

    exit(get_executionStatus());
}
/* end of main() */



/*!***************************************************************************
 *          fbe_hw_test_run_test_suites()
 ***************************************************************************** 
 * 
 * @brief   This function creates and runs all test suites.
 *
 * @param   None
 * 
 * @return  None   
 *
 *****************************************************************************/
static void fbe_hw_test_run_test_suites(void)
{
    mut_testsuite_t *sep_configuration_test_suite = NULL;    
    mut_testsuite_t *sep_degraded_io_test_suite = NULL;
    mut_testsuite_t *sep_disk_errors_test_suite = NULL;    
    mut_testsuite_t *sep_normal_io_test_suite = NULL; 
    mut_testsuite_t *sep_power_savings_test_suite = NULL;       
    mut_testsuite_t *sep_rebuild_test_suite = NULL;    
    mut_testsuite_t *sep_services_test_suite = NULL;    
    mut_testsuite_t *sep_sniff_test_suite = NULL;    
    mut_testsuite_t *sep_sparing_test_suite = NULL;    
    mut_testsuite_t *sep_special_ops_test_suite = NULL;    
    mut_testsuite_t *sep_verify_test_suite = NULL;   
    mut_testsuite_t *sep_zeroing_test_suite = NULL; 
    mut_testsuite_t *sep_performance_test_suite = NULL; 
    mut_testsuite_t *sep_dualsp_configuration_test_suite = NULL;
    mut_testsuite_t *sep_dualsp_degraded_io_test_suite = NULL;
    mut_testsuite_t *sep_dualsp_disk_errors_test_suite = NULL;
    mut_testsuite_t *sep_dualsp_normal_io_test_suite = NULL; 
    mut_testsuite_t *sep_dualsp_power_savings_test_suite = NULL;       
    mut_testsuite_t *sep_dualsp_rebuild_test_suite = NULL; 
    mut_testsuite_t *sep_dualsp_services_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_sniff_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_sparing_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_special_ops_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_verify_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_zeroing_test_suite = NULL;    
       

    /* Create test suites
     */
    sep_configuration_test_suite = hw_fbe_test_create_sep_configuration_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_services_test_suite = hw_fbe_test_create_sep_services_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_normal_io_test_suite = hw_fbe_test_create_sep_normal_io_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_degraded_io_test_suite = hw_fbe_test_create_sep_degraded_io_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_disk_errors_test_suite = hw_fbe_test_create_sep_disk_errors_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_power_savings_test_suite = hw_fbe_test_create_sep_power_savings_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_rebuild_test_suite = hw_fbe_test_create_sep_rebuild_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_sparing_test_suite = hw_fbe_test_create_sep_sparing_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_verify_test_suite = hw_fbe_test_create_sep_verify_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_sniff_test_suite = hw_fbe_test_create_sep_sniff_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_special_ops_test_suite = hw_fbe_test_create_sep_special_ops_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_zeroing_test_suite = hw_fbe_test_create_sep_zeroing_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_performance_test_suite = hw_fbe_test_create_sep_performance_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_dualsp_configuration_test_suite = hw_fbe_test_create_sep_dualsp_configuration_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_dualsp_normal_io_test_suite = hw_fbe_test_create_sep_dualsp_normal_io_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_dualsp_degraded_io_test_suite = hw_fbe_test_create_sep_dualsp_degraded_io_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_dualsp_disk_errors_test_suite = hw_fbe_test_create_sep_dualsp_disk_errors_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_dualsp_power_savings_test_suite = hw_fbe_test_create_sep_dualsp_power_savings_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_dualsp_rebuild_test_suite = hw_fbe_test_create_sep_dualsp_rebuild_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_dualsp_services_test_suite = hw_fbe_test_create_sep_dualsp_services_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_dualsp_sniff_test_suite = hw_fbe_test_create_sep_dualsp_sniff_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_dualsp_sparing_test_suite = hw_fbe_test_create_sep_dualsp_sparing_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_dualsp_special_ops_test_suite = hw_fbe_test_create_sep_dualsp_special_ops_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_dualsp_verify_test_suite = hw_fbe_test_create_sep_dualsp_verify_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    sep_dualsp_zeroing_test_suite = hw_fbe_test_create_sep_dualsp_zeroing_test_suite(fbe_test_hw_suite_startup,fbe_test_hw_suite_teardown);
    
    /* Run test suites
     */
    MUT_RUN_TESTSUITE(sep_configuration_test_suite);
    MUT_RUN_TESTSUITE(sep_services_test_suite);
    MUT_RUN_TESTSUITE(sep_normal_io_test_suite);
    MUT_RUN_TESTSUITE(sep_degraded_io_test_suite);
    MUT_RUN_TESTSUITE(sep_disk_errors_test_suite);
    MUT_RUN_TESTSUITE(sep_power_savings_test_suite);
    MUT_RUN_TESTSUITE(sep_rebuild_test_suite);
    MUT_RUN_TESTSUITE(sep_sparing_test_suite);
    MUT_RUN_TESTSUITE(sep_verify_test_suite);
    MUT_RUN_TESTSUITE(sep_sniff_test_suite);
    MUT_RUN_TESTSUITE(sep_special_ops_test_suite);
    MUT_RUN_TESTSUITE(sep_zeroing_test_suite);
    MUT_RUN_TESTSUITE(sep_performance_test_suite);
    MUT_RUN_TESTSUITE(sep_dualsp_configuration_test_suite);
    MUT_RUN_TESTSUITE(sep_dualsp_normal_io_test_suite);
    MUT_RUN_TESTSUITE(sep_dualsp_degraded_io_test_suite);
    MUT_RUN_TESTSUITE(sep_dualsp_disk_errors_test_suite);
    MUT_RUN_TESTSUITE(sep_dualsp_power_savings_test_suite);
    MUT_RUN_TESTSUITE(sep_dualsp_rebuild_test_suite);
    MUT_RUN_TESTSUITE(sep_dualsp_services_test_suite);
    MUT_RUN_TESTSUITE(sep_dualsp_sniff_test_suite);
    MUT_RUN_TESTSUITE(sep_dualsp_sparing_test_suite);
    MUT_RUN_TESTSUITE(sep_dualsp_special_ops_test_suite);
    MUT_RUN_TESTSUITE(sep_dualsp_verify_test_suite);
    MUT_RUN_TESTSUITE(sep_dualsp_zeroing_test_suite);

    return;
}
/* end of fbe_hw_test_run_tests_suites() */



/*!***************************************************************************
 *          process_cmd_spa_ip()
 ***************************************************************************** 
 * 
 * @brief   This function processes the -spa_ip option.
 *
 * @param   which_sp        - SP to process option on
 * @param   option_name_p   - pointer to the option name
 * 
 * @return  None   
 *
 *****************************************************************************/
void process_cmd_spa_ip(const char *option_name_p)
{
    char *value = mut_get_user_option_value(option_name_p);
    if (value != NULL)
    {
        strncpy(spa_ip, value, sizeof(spa_ip));
        remote_spa = FBE_TRUE;
    }
}
/* end of process_cmd_spa_ip() */


/*!***************************************************************************
 *          process_cmd_spb_ip()
 ***************************************************************************** 
 * 
 * @brief   This function processes the -spa_ip option.
 *
 * @param   which_sp        - SP to process option on
 * @param   option_name_p   - pointer to the option name
 * 
 * @return  None   
 *
 *****************************************************************************/
void process_cmd_spb_ip(const char *option_name_p)
{
    char *value = mut_get_user_option_value(option_name_p);

    if (value != NULL)
    {
        strncpy(spb_ip, value, sizeof(spb_ip));
            remote_spb = FBE_TRUE;
    }
}
/* end of process_cmd_spb_ip() */


/*!***************************************************************************
 *          process_cmd_options()
 ***************************************************************************** 
 * 
 * @brief   This function processes the command options provided by the user for
 *          the specified SP.
 *
 * @param   which_sp        - SP to process options on
 * 
 * @return  None   
 *
 *****************************************************************************/
void process_cmd_options()
{
    int i = 0;
    for (i = 0; i < CMD_NUM_OF_OPTIONS; ++i)
    {
        
        if (mut_option_selected(cmd_options[i].name)&&(cmd_options[i].func != NULL))
        {
            cmd_options[i].func(cmd_options[i].name);
        }
    }


    if(mut_option_selected("-nodebugger"))
    {
        connect_retry_times = DEFAULT_CONNECT_RERY_TIMES;
    }


}
/* end of process_cmd_options() */


