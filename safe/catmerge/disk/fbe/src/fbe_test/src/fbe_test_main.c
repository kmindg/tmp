/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file    fbe_test_main.c
 ***************************************************************************
 *
 * @brief   This file contains the methods to create and launch the `fbe_test'
 *          simulation environment.
 *
 * @version
 *          Initial version
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "mut.h"
#include "fbe_test.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_transport_packet_interface.h"
#include "self_tests.h"
#include "fbe_cli_tests.h"
#include "physical_package_tests.h"
#include "sep_tests.h"
#include "esp_tests.h"
#include "system_tests.h"

/******************************
 *   LOCAL FUNCTION PROTOTYPES
 ******************************/

/*****************************
 *   GLOBALS
 ******************************/

/*!***************************************************************************
 * @def     cmd_options[]
 ***************************************************************************** 
 * 
 * @brief   Command line options for `fbe_test' simulation
 *
 *****************************************************************************/
static fbe_test_cmd_options_t cmd_options[] = {
    {"-bat",        fbe_test_sp_sim_process_cmd_bat,      1, TRUE, "-bat <name_of_a_bat_file_that_launches_the_fbe_sp_sim.exe>"},
    {"-debugger",   fbe_test_sp_sim_process_cmd_debugger, 1, TRUE, "-debugger VS/windbg. When -debugger and -debug are both given, launch fbe_sp_sim with the given debugger for the given SP"},
    {"-debug",      fbe_test_sp_sim_process_cmd_debug,    1, TRUE, "-debug SPA/SPB/BOTH. Specify the SP you want to launch with the specified debugger"},
    {"-sp_log",     fbe_test_sp_sim_process_cmd_sp_log,   1, TRUE, "-sp_log file/console/both/none.  Set log destination.  Default sets to console"},
    {"-port_base",  fbe_test_sp_sim_process_cmd_port_base,1, TRUE, "-port_base <port_base>.  Set port base of SP and CMI."},
    {"-nogui",  NULL, 0, TRUE, "-nogui.  Run fbe_test without gui on linux."},
    {"-self_test",   NULL,   0, TRUE, "Run fbe_test self tests."},
    {"-sim_drive_type", NULL, 1, TRUE, "-sim_drive_type  1 - remote memory  2 - local memory(default)"},
    {"-panic_sp", NULL, 1, TRUE, "Panic the SP when the test starts. This is for testing core dumps of the SP."},
    {"-load_unload_test", NULL, 1, TRUE, "Just load and then unload the packages, skipping the actual test itself.."},
    {"-complete_test", NULL, 1, TRUE, "Just complete the test with success.  Used for testing the run script."},
    {"-fbe_io_seconds",          NULL, 1, TRUE, "extends the default I/O time"},
    {"-fbe_io_delay",          NULL, 1, TRUE, "Sets the I/O delay for the terminator, in milliseconds"},
    {"-fbe_threads",             NULL, 1, TRUE, "using thread count"},
    {"-fbe_small_io_count",      NULL, 1, TRUE, "using small io count"},
    {"-fbe_large_io_count",      NULL, 1, TRUE, "using large io count"},
    {"-fbe_default_trace_level", NULL, 1, TRUE, "using trace level"},
    {"-normal_io_only", NULL, 1, TRUE, "Only do normal, non-degraded I/Os, not aborts, shutdowns, errors, etc."},
    {"-raid0_only",  NULL, 0, TRUE, "only test raid 0"},
    {"-raid1_only",  NULL, 0, TRUE, "only test raid 1"},
    {"-raid3_only",  NULL, 0, TRUE, "only test raid 3"},
    {"-raid5_only",  NULL, 0, TRUE, "only test raid 5"},
    {"-raid6_only",  NULL, 0, TRUE, "only test raid 6"},
    {"-raid10_only", NULL, 0, TRUE, "only test raid 10"},
    {"-parity_only", NULL, 0, TRUE, "only test raid 5, 3 and 6"},
    {"-mirror_only", NULL, 0, TRUE, "only test raid 1 and 10"},
    {"-block_size", NULL, 0, TRUE, "Block size to use for ALL configurations. Valid values are: 520, 4k or mixed."},
    {"-raid_test_level", NULL, 1, TRUE, "or [-rtl] - Set raid test level (0-2)."},
    {"-rtl", NULL, 1, FALSE, "Set raid test level (0-2)"}, // Short alias to -raid_test_level
    {"-extended_test_level", NULL, 1, TRUE, "Set extended test level (0-2)"},
    {"-drive_insert_delay", NULL, 1, TRUE, "Set drive insert delay of: x milliseconds"},
    {"-drive_remove_delay", NULL, 1, TRUE, "Set drive remove delay of: x milliseconds"},
    {"-raid_group_trace_level", NULL, 1, TRUE, "Set trace level of configured raid group objects."},
    {"-raid_group_debug_flags", NULL, 1, TRUE, "Set debug flags of configured raid group objects."},
    {"-raid_library_debug_flags", NULL, 1, TRUE, "Set raid library debug flags for each object."},
    {"-raid_library_default_debug_flags", NULL, 1, TRUE, "Set raid library default debug flags."},
    {"-traffic_trace_flags", NULL, 1, TRUE, "[fbe_lun | fbe_rg | fbe_rg_fru | all | all_rg] Enable rba trace flags."},
    {"-stop_sep_neit_on_error", NULL, 0, TRUE, "Stop the SP when it encounters trace of error level or critical error level in sep or neit packages."},
    {"-stop_sector_trace_on_error", NULL, 0, TRUE, "Stop the SP when it encounters specific type of sector trace."},
    {"-sector_trace_debug_flags", NULL, 1, TRUE, "Set sector trace debug flags, ignored in tests injecting errors."},
    {"-rdgen_peer_option", NULL, 0, TRUE, "Valid arguments are: LOAD_BALANCE, RANDOM, PEER_ONLY"},
    {"-rdgen_trace_level", NULL, 1, TRUE, "Set rdgen service trace level."},
    {"-lei_trace_level", NULL, 1, TRUE, "Set logical error injection service trace level."},
    {"-pei_trace_level", NULL, 1, TRUE, "Set protocol error injection service trace level."},
    {"-degraded_only", NULL, 0, FALSE,  "Only run degraded cases. Cookie Monster only."},
    {"-shutdown_only", NULL, 0, FALSE,  "Only run shutdown cases. Big Bird and Oscar."},
    {"-start_table", NULL, 1, FALSE, "First error table to use. Kermit only."},
    {"-start_index", NULL, 1, FALSE, "First index in the table to use. Kermit only."},
    {"-total_cases", NULL, 1, FALSE, "Total number of error cases to use. Kermit only."},
    {"-repeat_case_count", NULL, 1, FALSE, "Repeat each test case this many times. Kermit only."},
    {"-abort_cases_only", NULL, 0, FALSE, "Only test abort cases. Kermit only."},
    {"-zero_only", NULL, 0, FALSE, "Only test zero cases."},
    {"-repeat_count", NULL, 1, TRUE, "Repeat test this many times. "},
    {"-sep_enable_system_drive_zeroing", NULL, 0, TRUE, "Force system drive background zeroing to always be enabled."},
    {"-sep_disable_system_drive_zeroing", NULL, 0, TRUE, "Force system drive background zeroing to always be disabled."},
    {"-stop_physical_package_on_error", NULL, 0, TRUE, "Stop the SP when it encounters error trace in physical package. Only applies to the tests that checks this option."},
    {"-pvd_class_debug_flags", NULL, 0, TRUE, "Set PVD's class debug flags."},
    {"-virtual_drive_debug_flags", NULL, 1, TRUE, "Set debug flags of all configured virtual drive objects."},
    {"-just_create_config", NULL, 0, TRUE, "This only creates the configuration and skips running tests cases."},
    {"-test_random_seed", NULL, 1, TRUE, "Set the random seed for the test code."},
    {"-neit_random_seed", NULL, 1, TRUE, "Set the random seed for the neit package."},
    {"-use_default_drives", NULL, 0, TRUE, "Use the default drives for the test (not random)."},
    {"-full_db_drive_rebuild", NULL, 0, FALSE, "Complete the DB drive rebuild fully. diego_test only."},
    {"-term_drive_debug_flags", NULL, 1, TRUE, "Set terminator drive debug flags."},
    {"-term_drive_debug_range", NULL, 1, TRUE, "Set range of terminator drives to debug. (0xffff - all drives)"},
    {"-graceful_shutdown", NULL, 0, TRUE, "Send non forced kill to allow slave processes to perform graceful cleanup"},
	{"-fmea", NULL, 0, TRUE, "Excute fmea test version."},
	{"-lifecycle_state_debug_flags",NULL,1,TRUE, "Set object lifecycle tracing debug flag."},
	{"-lifecycle_trace_object_id",NULL,1,TRUE, "Set lifecycle debug tracing for SEP object id."},
    {"-stop_load_raw_mirror", NULL, 0, TRUE, "Stop loading raw mirror service in SEP."},
    {"-term_set_completion_dpc", NULL, 1, TRUE, "Set (1) or clear (0) raise terminator I/O completion to DPC."},
    {"-cli", NULL, 0, TRUE, "[spa | spb | both] - Pause test and launch cli on after config is created."},
    {"-fail_test", NULL, 0, TRUE, "Intentionally fail the test.  Used for testing test launch scripts."},
    {"-run_test_cases", NULL, 0, TRUE, "select test cases to run."},
    {"-run_test_case", NULL, 1, FALSE, "select test case to run."},
    {"-unmap_default_drive_type", NULL, 0, FALSE, "use unmap capable drive types."},
};

enum {
    CMD_NUM_OF_OPTIONS = (sizeof(cmd_options) / sizeof(cmd_options[0])),
};

/*!***************************************************************************
 *          fbe_test_main_get_cmd_options()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
fbe_test_cmd_options_t *fbe_test_main_get_cmd_options(void) 
{
    return &cmd_options[0];
}
/* end of fbe_test_main_get_cmd_options() */

/*!***************************************************************************
 *          fbe_test_main_get_number_cmd_options()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
fbe_u32_t fbe_test_main_get_number_cmd_options(void) 
{
    return CMD_NUM_OF_OPTIONS;
}
/* end of fbe_test_main_get_number_cmd_options() */

/*!***************************************************************************
 *          fbe_test_system_suite_startup()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_system_suite_startup(void) 
{

    // set up stuff that the system suite needs but doesn't do itself.

    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    /* kill orphan fbe_sp_sim.exe */
    fbe_test_sp_sim_kill_orphan_sp_process();

    /* kill orphan simulated_drive_user_server.exe */
    fbe_test_drive_sim_kill_orphan_drive_server_process();

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

    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_KMS,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);

    
    /*need to initialize the client to connect to the server*/
    fbe_api_sim_transport_reset_client(FBE_CLIENT_MODE_DEV_PC);

   return;
}
/* end of fbe_test_system_suite_startup() */

/*! @todo Needs to be resolved 
 Temporary hack - should be removed */
fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_PHYSICAL;
    return FBE_STATUS_OK;
}
/*!***************************************************************************
 *          fbe_test_base_suite_startup()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_base_suite_startup(void) 
{
    // currently used for all suites but 'system'

    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    /* Set the trace function used by fbe_ktrace, which will be used when the fbe api does its traces. 
     * We want to send the fbe api traces back to mut so it ends up inside the test logs. 
     */
    fbe_ktrace_set_trace_function(mut_ktrace);

    /* kill orphan fbe_sp_sim.exe */
    fbe_test_sp_sim_kill_orphan_sp_process();
    
    /* kill orphan simulated_drive_user_server.exe */
    fbe_test_drive_sim_kill_orphan_drive_server_process();

    /*we start B before A so the window of A will cover it, since most of the action will be on the A side anyway*/
    fbe_test_sp_sim_start(FBE_SIM_SP_B);
    fbe_test_sp_sim_start(FBE_SIM_SP_A);

    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(FBE_SIM_SP_B));
    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(FBE_SIM_SP_A));

    EmcutilSleep(2000);

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

    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_KMS,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);

    
    /*need to initialize the client to connect to the server*/
    fbe_api_sim_transport_reset_client(FBE_CLIENT_MODE_DEV_PC);

    status = fbe_api_sim_transport_init_client(FBE_SIM_SP_A, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,("\nCan't connect to FBE API Server, make sure FBE is running !!!\n"))
    
    status = fbe_api_sim_transport_init_client(FBE_SIM_SP_B, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, ("\nCan't connect to FBE API Server, make sure FBE is running, status %d !!!\n"));

    /* Now configure and start the specified drive simulator */
    status = fbe_test_drive_sim_start_drive_sim(FBE_FALSE);

    /* Configure the global I/O completion delay for the Terminator*/
    fbe_test_util_process_fbe_io_delay();

    /* save the process id of the SPs and fbe_test, so fbe_test.py can create dumps. */
    status = fbe_test_util_store_process_ids();

    fbe_test_sp_sim_store_sp_pids();

    fbe_test_sp_sim_start_notification();
}
/* end of fbe_test_base_suite_startup() */

/*!***************************************************************************
 *          fbe_test_dualsp_suite_startup()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_dualsp_suite_startup(void) 
{
    // For dualsp tests we MUST use the remote memory drive server
    fbe_test_drive_sim_force_drive_sim_remote_memory();

    // Use the base startup to start single SP
    fbe_test_base_suite_startup();

}
/* end of fbe_test_dualsp_suite_startup() */

/*!***************************************************************************
 *          fbe_test_system_suite_teardown()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_system_suite_teardown(void)
{
    // system test does the rest of the work (currently)
    fbe_api_common_destroy_sim();

    /*! @note It is up to the sp sim(s) to execute fbe_simulated_drive_cleanup 
     *        on each active sp.  Therefore the call to the simulated drive
     *        server destroy should only be done after the sp cleanup.
     */
    fbe_test_drive_sim_destroy_drive_sim();

    return;
}
/* end of fbe_test_system_suite_teardown */

/*!**************************************************************
 * fbe_test_reboot_sp()
 ****************************************************************
 * @brief
 *  - We created some LUNs that did initial verify.
 *  - We created other LUNs that did not do initial verify.
 *  - Reboot the SP.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  3/28/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_reboot_sp(fbe_test_rg_configuration_t *rg_config_p,
                        fbe_sim_transport_connection_target_t   target_sp)
{
    mut_printf(MUT_LOG_LOW, "Crash both SPs");

    /* Destroy the SPs and its current session
     */
    fbe_test_base_suite_destroy_sp();

    fbe_test_boot_sps(rg_config_p, target_sp);
}
/**************************************
 * end fbe_test_reboot_sp()
 **************************************/
/*!**************************************************************
 * fbe_test_reboot_both_sp()
 ****************************************************************
 * @brief
 *  - Reboot the SPs.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void fbe_test_reboot_both_sp(fbe_test_rg_configuration_t *rg_config_p)
{
    mut_printf(MUT_LOG_LOW, "reboot both SPs");

    /* Destroy the SPs and its current session
     */
    fbe_test_base_suite_destroy_sp();

    fbe_test_boot_both_sps(rg_config_p);
}
/**************************************
 * end fbe_test_reboot_both_sp()
 **************************************/
/*!**************************************************************
 * fbe_test_boot_both_sps()
 ****************************************************************
 * @brief
 *  Bring up both SPs.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void fbe_test_boot_both_sps(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_test_rg_configuration_t temp_config[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE];
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    /* Start the SPs with a new session
     */
    fbe_test_base_suite_startup_post_powercycle();

    fbe_test_sep_duplicate_config(rg_config_p, temp_config);

    /* load the physical config and all packages.
     */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_create_physical_config_for_rg(&temp_config[0], raid_group_count);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_create_physical_config_for_rg(&temp_config[0], raid_group_count);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    sep_config_load_sep_and_neit_both_sps();
}
/**************************************
 * end fbe_test_boot_sps()
 **************************************/
/*!**************************************************************
 * fbe_test_boot_sps()
 ****************************************************************
 * @brief
 *  Bring up this SP.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  9/26/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_boot_sps(fbe_test_rg_configuration_t *rg_config_p,
                      fbe_sim_transport_connection_target_t target_sp)
{
    fbe_test_rg_configuration_t temp_config[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE];
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    /* Start the SPs with a new session
     */
    fbe_test_base_suite_startup_post_powercycle();

    fbe_test_sep_duplicate_config(rg_config_p, temp_config);

    /* load the physical config and all packages.
     */
    fbe_test_create_physical_config_for_rg(&temp_config[0], raid_group_count);

    sep_config_load_sep_and_neit();
}
/**************************************
 * end fbe_test_boot_sps()
 **************************************/
/*!**************************************************************
 * fbe_test_boot_sp()
 ****************************************************************
 * @brief
 *  Bring up this SP.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  9/26/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_boot_sp(fbe_test_rg_configuration_t *rg_config_p,
                      fbe_sim_transport_connection_target_t target_sp)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t temp_config[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE];
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    /* Start the SPs with a new session
     */
    fbe_test_base_suite_startup_single_sp(target_sp);

    //fbe_test_base_suite_startup_post_powercycle();

    /*! @note the above code might change the target server to A, so we need 
     * to change it back to our target. 
     */
    status = fbe_api_sim_transport_set_target_server(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_sep_duplicate_config(rg_config_p, temp_config);

    /* load the physical config and all packages.
     */
    fbe_test_create_physical_config_for_rg(&temp_config[0], raid_group_count);

    sep_config_load_sep_and_neit();
    if (fbe_test_sep_util_get_encryption_test_mode()) {
        sep_config_load_kms(NULL);
    }
}
/**************************************
 * end fbe_test_boot_sp()
 **************************************/
/*!**************************************************************
 * fbe_test_boot_first_sp()
 ****************************************************************
 * @brief
 *  Bring up the first of 2 SPs.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  9/26/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_boot_first_sp(fbe_test_rg_configuration_t *rg_config_p,
                            fbe_sim_transport_connection_target_t target_sp)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t temp_config[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE];
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    /* Start the SPs with a new session
     */
    fbe_test_sp_sim_start(target_sp);

    fbe_api_sim_transport_reset_client_sp(target_sp);

    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(target_sp));

    status = fbe_api_sim_transport_init_client(target_sp, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,("\nCan't connect to FBE API Server, make sure FBE is running !!!\n"));

    status = fbe_test_drive_sim_start_drive_sim_for_sp(FBE_TRUE, target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Configure the global I/O completion delay for the Terminator*/
    fbe_test_util_process_fbe_io_delay();

    fbe_test_sp_sim_start_notification_single_sp(target_sp);

    //fbe_test_base_suite_startup_post_powercycle();

    /*! @note the above code might change the target server to A, so we need 
     * to change it back to our target. 
     */
    status = fbe_api_sim_transport_set_target_server(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_sep_duplicate_config(rg_config_p, temp_config);

    /* load the physical config and all packages.
     */
    fbe_test_create_physical_config_for_rg(&temp_config[0], raid_group_count);

    sep_config_load_sep_and_neit();
    if (fbe_test_sep_util_get_encryption_test_mode()) {
        sep_config_load_kms(NULL);
    }
}
/**************************************
 * end fbe_test_boot_first_sp()
 **************************************/

/*!**************************************************************
 * fbe_test_boot_sp_for_config_array()
 ****************************************************************
 * @brief
 *  Bring up this SP.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  1/10/2014 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_boot_sp_for_config_array(fbe_test_rg_configuration_array_t *array_p,
                                       fbe_sim_transport_connection_target_t target_sp,
                                       fbe_sep_package_load_params_t *sep_params_p,
                                       fbe_neit_package_load_params_t *neit_params_p,
                                       fbe_bool_t b_first_sp)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);
    fbe_test_rg_configuration_array_t *tmp_array_p = NULL;
    fbe_u32_t raid_index;

    tmp_array_p = (fbe_test_rg_configuration_array_t *)fbe_api_allocate_memory(sizeof(fbe_test_rg_configuration_array_t) * 
                                                                               (raid_type_count + 1));
    for (raid_index = 0; raid_index < raid_type_count; raid_index++) {
        fbe_test_sep_duplicate_config(&array_p[raid_index][0], &tmp_array_p[raid_index][0]);
    }
    tmp_array_p[raid_index][0].width = FBE_U32_MAX;

    /* Start the SPs with a new session
     */
    if (b_first_sp) {
        fbe_test_sp_sim_start(target_sp);
    
        fbe_api_sim_transport_reset_client_sp(target_sp);

        fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(target_sp));
    
        status = fbe_api_sim_transport_init_client(target_sp, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,("\nCan't connect to FBE API Server, make sure FBE is running !!!\n"));
    
        status = fbe_test_drive_sim_start_drive_sim_for_sp(FBE_TRUE, target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        /* Configure the global I/O completion delay for the Terminator*/
        fbe_test_util_process_fbe_io_delay();
    
        fbe_test_sp_sim_start_notification_single_sp(target_sp);
    } else {
        fbe_test_base_suite_startup_single_sp(target_sp);
    }

    /*! @note the above code might change the target server to A, so we need 
     * to change it back to our target. 
     */
    status = fbe_api_sim_transport_set_target_server(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* load the physical config and all packages.
     */
    fbe_test_sep_util_rg_config_array_load_physical_config(tmp_array_p);

    if ( (sep_params_p != NULL) && (neit_params_p != NULL) ) {
        sep_config_load_sep_and_neit_params(sep_params_p, neit_params_p);
    } else {
        sep_config_load_sep_and_neit();
    }
    fbe_api_free_memory(tmp_array_p);
}
/**************************************
 * end fbe_test_boot_sp_for_config_array()
 **************************************/

/*!***************************************************************************
 *          fbe_test_boot_sp_with_extra_drives_load_physical_config()
 *****************************************************************************
 *
 * @brief   This function first copies the current array of test configurations
 *          into the temporary array, then starts teh SP specified.
 *  
 *
 * @param   array_p - Pointer to array of existing test configurations.
 * @param   target_sp - target SP to reboot (boot)
 *
 * @return None.
 *
 * @author
 *  11/14/2012  Ron Proulx  - Created from fbe_test_boot_sp().
 *
 *****************************************************************************/
void fbe_test_boot_sp_with_extra_drives_load_physical_config(fbe_test_rg_configuration_array_t *array_p,
                                                             fbe_sim_transport_connection_target_t target_sp)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t        *current_rg_config_p = NULL;
    fbe_u32_t                           byte_to_allocate = 0;
    fbe_test_rg_configuration_array_t  *temp_config = NULL;
    const fbe_char_t                   *raid_type_string_p = NULL;
    fbe_u32_t                           raid_type_index;
    fbe_u32_t                           raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);
    fbe_u32_t                           raid_group_count = 0;
    fbe_u32_t                           rg_index;

    /* Allocate the memory for the temporary array of array
     */
    byte_to_allocate = sizeof(fbe_test_rg_configuration_array_t) * FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE;
    temp_config = (fbe_test_rg_configuration_array_t *)fbe_allocate_contiguous_memory(byte_to_allocate);
    if (temp_config == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s Failed to allocate: %d bytes for temporary config array", 
                   __FUNCTION__, byte_to_allocate);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return;
    }

    /* Start the SPs with a new session
     */
    fbe_test_base_suite_startup_single_sp(target_sp);

    //fbe_test_base_suite_startup_post_powercycle();

    /*! @note the above code might change the target server to A, so we need 
     * to change it back to our target. 
     */
    status = fbe_api_sim_transport_set_target_server(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Copy into the structure.
     */
    for (raid_type_index = 0; raid_type_index < raid_type_count; raid_type_index++)
    {
        current_rg_config_p = &array_p[raid_type_index][0];
        fbe_test_sep_util_get_raid_type_string(current_rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(current_rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, current_rg_config_p->raid_type);
            continue;
        }

        /* Copy any enabled raid groups.
         */
        raid_group_count = fbe_test_get_rg_array_length(current_rg_config_p);
        if (raid_group_count == 0)
        {
            continue;
        }

        for (rg_index = 0; rg_index < raid_group_count; rg_index++) 
        {
            temp_config[raid_type_index][rg_index] = *current_rg_config_p;
            current_rg_config_p++;
        }

        /* Terminate the array */
        temp_config[raid_type_index][rg_index].width = FBE_U32_MAX;
    }

    /* Terminate the array of arrays*/
    temp_config[raid_type_index][0].width = FBE_U32_MAX;

    /*! @note Although this will populate the raid groups etc, we use a
     *        temporary array of arrays so there is no affect on the raid groups
     *        under test (i.e. the contents of array_p is not changed).
     *
     *        The purpose of this is to simply create the same drives that were
     *        created during the test.
     */
    fbe_test_sep_util_rg_config_array_with_extra_drives_load_physical_config(&temp_config[0]);
    sep_config_load_sep_and_neit();

    /* Free the allocated memory.
     */
    fbe_release_contiguous_memory(temp_config);

    return;
}
/*************************************************************************
 * end fbe_test_boot_sp_with_extra_drives_load_physical_config()
 *************************************************************************/

/*!***************************************************************************
 *          fbe_test_base_suite_destroy_sp()
 ***************************************************************************** 
 * 
 * @brief   This function destroys the SPs and fbe_sp_sim.exe.. 
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_base_suite_destroy_sp(void)
{
    /* First destroy the transports */
    fbe_api_sim_transport_destroy_client(FBE_SIM_SP_A);
    fbe_api_sim_transport_destroy_client(FBE_SIM_SP_B);

    /* Now stop fbe_sp_sim.exe */
    fbe_test_sp_sim_stop();
    fbe_api_common_destroy_sim();

    fbe_test_mark_sp_down(fbe_test_conn_target_to_sp(FBE_SIM_SP_B));
    fbe_test_mark_sp_down(fbe_test_conn_target_to_sp(FBE_SIM_SP_A));

    return;
}
/* end of fbe_test_base_suite_destroy_sp() */

/*!***************************************************************************
 *          fbe_test_base_suite_startup_post_powercycle()
 *****************************************************************************
 *
 * @brief   This function creates the SPs and fbe_sp_sim.exe..
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void fbe_test_base_suite_startup_post_powercycle(void)
{
	fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

	/*we start B before A so the window of A will cover it, since most of the action will be on the A side anyway*/
	fbe_test_sp_sim_start(FBE_SIM_SP_B);
	fbe_test_sp_sim_start(FBE_SIM_SP_A);

    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(FBE_SIM_SP_B));
    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(FBE_SIM_SP_A));

	//fbe_api_sleep(2000);

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

	fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_KMS,
												   fbe_api_sim_transport_send_io_packet,
												   fbe_api_sim_transport_send_client_control_packet);

	/*need to initialize the client to connect to the server*/
	fbe_api_sim_transport_reset_client(FBE_CLIENT_MODE_DEV_PC);

	status = fbe_api_sim_transport_init_client(FBE_SIM_SP_A, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,("\nCan't connect to FBE API Server, make sure FBE is running !!!\n"))

	status = fbe_api_sim_transport_init_client(FBE_SIM_SP_B, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, ("\nCan't connect to FBE API Server, make sure FBE is running, status %d !!!\n"));

	/* Now configure and start the specified drive simulator */
	status = fbe_test_drive_sim_start_drive_sim(FBE_TRUE);

	/* Configure the global I/O completion delay for the Terminator*/
	fbe_test_util_process_fbe_io_delay();

	/* save the process id of the SPs and fbe_test, so fbe_test.py can create dumps. */
	status = fbe_test_util_store_process_ids();

	fbe_test_sp_sim_store_sp_pids();

	fbe_test_sp_sim_start_notification();

    return;
}
/* end of fbe_test_base_suite_startup_post_powercycle() */

/*!***************************************************************************
 *          fbe_test_base_suite_startup_single_sp_post_powercycle()
 *****************************************************************************
 *
 * @brief   This function creates the SP and fbe_sp_sim.exe..
 *
 * @param   sp - the SP to startup
 *
 * @return  None
 *
 *****************************************************************************/
void fbe_test_base_suite_startup_single_sp_post_powercycle(fbe_sim_transport_connection_target_t sp)
{
	fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

	/*we start B before A so the window of A will cover it, since most of the action will be on the A side anyway*/
	fbe_test_sp_sim_start(sp);

	fbe_api_sleep(2000);

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

	fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_KMS,
												   fbe_api_sim_transport_send_io_packet,
												   fbe_api_sim_transport_send_client_control_packet);

	status = fbe_api_sim_transport_init_client(sp, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,("\nCan't connect to FBE API Server, make sure FBE is running !!!\n"))

	/* Now configure and start the specified drive simulator */
	status = fbe_test_drive_sim_start_drive_sim(FBE_TRUE);

	/* Configure the global I/O completion delay for the Terminator*/
	fbe_test_util_process_fbe_io_delay();

	/* save the process id of the SPs and fbe_test, so fbe_test.py can create dumps. */
	status = fbe_test_util_store_process_ids();

	fbe_test_sp_sim_store_sp_pids();

	fbe_test_sp_sim_start_notification();

    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(sp));

	return;
} /* end of fbe_test_base_suite_startup_single_sp_post_powercycle() */

/*!***************************************************************************
 *          fbe_test_base_suite_startup_single_sp()
 *****************************************************************************
 *
 * @brief   This function creates the SP and fbe_sp_sim.exe..
 *
 * @param   sp - the SP to startup
 *
 * @return  None
 *
 *****************************************************************************/
void fbe_test_base_suite_startup_single_sp(fbe_sim_transport_connection_target_t sp)
{
	fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(sp));

	/*we start B before A so the window of A will cover it, since most of the action will be on the A side anyway*/
	fbe_test_sp_sim_start(sp);

	//fbe_api_sleep(2000);

	/*need to initialize the client to connect to the server*/
    fbe_api_sim_transport_reset_client_sp(sp);

    status = fbe_api_sim_transport_init_client(sp, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,("\nCan't connect to FBE API Server, make sure FBE is running !!!\n"));

    /* Now configure and start the specified drive simulator */
	status = fbe_test_drive_sim_start_drive_sim(FBE_TRUE);
    
    /*fbe_api_sim_transport_reset_client(FBE_CLIENT_MODE_DEV_PC);

	status = fbe_api_sim_transport_init_client(sp, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,("\nCan't connect to FBE API Server, make sure FBE is running !!!\n"))*/



	/* Configure the global I/O completion delay for the Terminator*/
	fbe_test_util_process_fbe_io_delay();

	/* save the process id of the SPs and fbe_test, so fbe_test.py can create dumps. */
	status = fbe_test_util_store_process_ids();

	fbe_test_sp_sim_store_sp_pids();

	fbe_test_sp_sim_start_notification_single_sp(sp);

    return;
}
/* end of fbe_test_base_suite_startup_single_sp() */

/*!***************************************************************************
 *          fbe_test_base_suite_teardown()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_base_suite_teardown(void)
{
    /* First destroy the SPs */
    fbe_test_base_suite_destroy_sp();

    /*! @note It is up to the sp sim(s) to execute fbe_simulated_drive_cleanup 
     *        on each active sp.  Therefore the call to the simulated drive
     *        server destroy should only be done after the sp cleanup.
     */
    fbe_test_drive_sim_destroy_drive_sim();

    return;
}
/* end of fbe_test_base_suite_teardown() */

/*!***************************************************************************
 *          fbe_test_dualsp_suite_teardown()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
void fbe_test_dualsp_suite_teardown(void){

    /* Simply use base teardown */
    fbe_test_base_suite_teardown();

    /* Restore the simulated drive type to the default */
    fbe_test_drive_sim_restore_default_drive_sim_type();
}
/* end of fbe_test_dualsp_suite_teardown() */

/*!***************************************************************************
 *          fbe_test_main()
 ***************************************************************************** 
 * 
 * @brief   This function
 *
 * @param   None
 *
 * @return  None   
 *
 *****************************************************************************/
int __cdecl fbe_test_main(int argc , char **argv, unsigned long timeout)
{
    mut_testsuite_t *physical_package_test_suite = NULL;   /* pointer to testsuite structure */ 
    mut_testsuite_t *sep_configuration_test_suite = NULL;    
    mut_testsuite_t *sep_copy_test_suite = NULL;    
    mut_testsuite_t *sep_degraded_io_test_suite = NULL;
    mut_testsuite_t *sep_disk_errors_test_suite = NULL;    
    mut_testsuite_t *sep_normal_io_test_suite = NULL;    
    mut_testsuite_t *sep_power_savings_test_suite = NULL;    
    mut_testsuite_t *sep_rebuild_test_suite = NULL;    
    mut_testsuite_t *sep_services_test_suite = NULL;    
    mut_testsuite_t *sep_sniff_test_suite = NULL;    
    mut_testsuite_t *sep_sparing_test_suite = NULL;    
    mut_testsuite_t *sep_special_ops_test_suite = NULL;     
    mut_testsuite_t *sep_encryption_test_suite = NULL;    
    mut_testsuite_t *sep_verify_test_suite = NULL;    
    mut_testsuite_t *sep_zeroing_test_suite = NULL;
    mut_testsuite_t *sep_extent_pool_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_configuration_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_copy_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_degraded_io_test_suite = NULL;
    mut_testsuite_t *sep_dualsp_disk_errors_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_normal_io_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_power_savings_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_rebuild_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_services_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_sniff_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_sparing_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_special_ops_test_suite = NULL;  
    mut_testsuite_t *sep_dualsp_encryption_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_verify_test_suite = NULL;    
    mut_testsuite_t *sep_dualsp_zeroing_test_suite = NULL;
    mut_testsuite_t *sep_dualsp_extent_pool_test_suite = NULL;    
    mut_testsuite_t *esp_test_suite = NULL;                /* pointer to testsuite structure */
    mut_testsuite_t *system_test_suite = NULL;             /* pointer to testsuite structure */
    mut_testsuite_t *fbe_cli_test_suite = NULL;            /* pointer to testsuite structure */
    //mut_testsuite_t *dualsp_test_suite = NULL;             /* pointer to testsuite structure */
    mut_testsuite_t *self_test_suite = NULL;             /* pointer to testsuite structure */
    int i;
    int new_argc;
    char **new_argv;

    /* if it is self test, we generate argc and argv for self test, this avoid recursive calls like mut_selftest */
    if(fbe_test_util_is_self_test_enabled(argc, argv) == FBE_TRUE)
    {
        fbe_test_util_generate_self_test_arg(&new_argc, &new_argv);
    }
    else
    {
        new_argc = argc;
        new_argv = argv;
    }


	reset_arguments(new_argc, new_argv);

    /* register the pre-defined user options */
    for (i = 0; i < sizeof(cmd_options) / sizeof(cmd_options[0]); ++i)
    {
        /* must be called before mut_init() */
        mut_register_user_option(cmd_options[i].name, cmd_options[i].least_num_of_args, cmd_options[i].show, cmd_options[i].description);
    }

    mut_init(new_argc, new_argv);                           /* before proceeding we need to initialize MUT infrastructure */

    mut_printf(MUT_LOG_TEST_STATUS, "\nfinish mut init\n");

    if(mut_isTimeoutSet() == 0){
		set_global_timeout(timeout);
	}
	
    fbe_test_util_set_console_title(new_argc,new_argv);     /* set fbe test console title with process id */

    // test suite creation start
    // these functions are under .../fbe_test/scenarios/<package>/<package>_test.c , the same file name as the respective .h files

    if(fbe_test_util_is_self_test_enabled(argc, argv) == FBE_TRUE)
    {
        self_test_suite = fbe_test_create_self_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        MUT_RUN_TESTSUITE(self_test_suite);
        /* if we generate new argv, free it */
        fbe_test_util_free_self_test_arg(new_argc, new_argv);
    }
    else
    {
        fbe_cli_test_suite = fbe_test_create_fbe_cli_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);

        physical_package_test_suite = fbe_test_create_physical_package_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        sep_configuration_test_suite = fbe_test_create_sep_configuration_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        sep_services_test_suite = fbe_test_create_sep_services_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        sep_normal_io_test_suite = fbe_test_create_sep_normal_io_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        sep_degraded_io_test_suite = fbe_test_create_sep_degraded_io_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        sep_disk_errors_test_suite = fbe_test_create_sep_disk_errors_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        sep_rebuild_test_suite = fbe_test_create_sep_rebuild_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        sep_sparing_test_suite = fbe_test_create_sep_sparing_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        sep_copy_test_suite = fbe_test_create_sep_copy_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        sep_verify_test_suite = fbe_test_create_sep_verify_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        sep_zeroing_test_suite = fbe_test_create_sep_zeroing_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        sep_sniff_test_suite = fbe_test_create_sep_sniff_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        sep_power_savings_test_suite = fbe_test_create_sep_power_savings_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        sep_special_ops_test_suite = fbe_test_create_sep_special_ops_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        sep_encryption_test_suite = fbe_test_create_sep_encryption_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);
        sep_extent_pool_test_suite = fbe_test_create_sep_extent_pool_test_suite(fbe_test_base_suite_startup,fbe_test_base_suite_teardown);

        sep_dualsp_configuration_test_suite = fbe_test_create_sep_dualsp_configuration_test_suite(fbe_test_dualsp_suite_startup, 
                                                                                                  fbe_test_dualsp_suite_teardown);
        sep_dualsp_services_test_suite = fbe_test_create_sep_dualsp_services_test_suite(fbe_test_dualsp_suite_startup, 
                                                                                        fbe_test_dualsp_suite_teardown);
        sep_dualsp_normal_io_test_suite = fbe_test_create_sep_dualsp_normal_io_test_suite(fbe_test_dualsp_suite_startup, 
                                                                                          fbe_test_dualsp_suite_teardown);
        sep_dualsp_degraded_io_test_suite = fbe_test_create_sep_dualsp_degraded_io_test_suite(fbe_test_dualsp_suite_startup, 
                                                                                              fbe_test_dualsp_suite_teardown);
        sep_dualsp_disk_errors_test_suite = fbe_test_create_sep_dualsp_disk_errors_test_suite(fbe_test_dualsp_suite_startup, 
                                                                                              fbe_test_dualsp_suite_teardown);
        sep_dualsp_rebuild_test_suite = fbe_test_create_sep_dualsp_rebuild_test_suite(fbe_test_dualsp_suite_startup, 
                                                                                      fbe_test_dualsp_suite_teardown);
        sep_dualsp_sparing_test_suite = fbe_test_create_sep_dualsp_sparing_test_suite(fbe_test_dualsp_suite_startup, 
                                                                                      fbe_test_dualsp_suite_teardown);
        sep_dualsp_copy_test_suite = fbe_test_create_sep_dualsp_copy_test_suite(fbe_test_dualsp_suite_startup, 
                                                                                fbe_test_dualsp_suite_teardown);
        sep_dualsp_verify_test_suite = fbe_test_create_sep_dualsp_verify_test_suite(fbe_test_dualsp_suite_startup, 
                                                                                    fbe_test_dualsp_suite_teardown);
        sep_dualsp_zeroing_test_suite = fbe_test_create_sep_dualsp_zeroing_test_suite(fbe_test_dualsp_suite_startup, 
                                                                                      fbe_test_dualsp_suite_teardown);
        sep_dualsp_sniff_test_suite = fbe_test_create_sep_dualsp_sniff_test_suite(fbe_test_dualsp_suite_startup, 
                                                                                  fbe_test_dualsp_suite_teardown);
        sep_dualsp_power_savings_test_suite = fbe_test_create_sep_dualsp_power_savings_test_suite(fbe_test_dualsp_suite_startup, 
                                                                                                  fbe_test_dualsp_suite_teardown);
        sep_dualsp_special_ops_test_suite = fbe_test_create_sep_dualsp_special_ops_test_suite(fbe_test_dualsp_suite_startup, 
                                                                                              fbe_test_dualsp_suite_teardown);
        sep_dualsp_encryption_test_suite = fbe_test_create_sep_dualsp_encryption_test_suite(fbe_test_dualsp_suite_startup, 
                                                                                             fbe_test_dualsp_suite_teardown);
        sep_dualsp_extent_pool_test_suite = fbe_test_create_sep_dualsp_extent_pool_test_suite(fbe_test_dualsp_suite_startup, 
                                                                                             fbe_test_dualsp_suite_teardown);

        esp_test_suite = fbe_test_create_esp_test_suite(fbe_test_dualsp_suite_startup,fbe_test_dualsp_suite_teardown);

        system_test_suite = fbe_test_create_system_test_suite(fbe_test_system_suite_startup,fbe_test_system_suite_teardown);

        // test suite creation end.

        MUT_RUN_TESTSUITE(fbe_cli_test_suite);

        MUT_RUN_TESTSUITE(physical_package_test_suite);

        MUT_RUN_TESTSUITE(sep_configuration_test_suite);
        MUT_RUN_TESTSUITE(sep_services_test_suite);
        MUT_RUN_TESTSUITE(sep_normal_io_test_suite);
        MUT_RUN_TESTSUITE(sep_degraded_io_test_suite);
        MUT_RUN_TESTSUITE(sep_disk_errors_test_suite);
        MUT_RUN_TESTSUITE(sep_rebuild_test_suite);
        MUT_RUN_TESTSUITE(sep_sparing_test_suite);
        MUT_RUN_TESTSUITE(sep_copy_test_suite);
        MUT_RUN_TESTSUITE(sep_verify_test_suite);
        MUT_RUN_TESTSUITE(sep_zeroing_test_suite);
        MUT_RUN_TESTSUITE(sep_sniff_test_suite);
        MUT_RUN_TESTSUITE(sep_power_savings_test_suite);
        MUT_RUN_TESTSUITE(sep_special_ops_test_suite);
        MUT_RUN_TESTSUITE(sep_encryption_test_suite);
        MUT_RUN_TESTSUITE(sep_extent_pool_test_suite);

        MUT_RUN_TESTSUITE(sep_dualsp_configuration_test_suite);
        MUT_RUN_TESTSUITE(sep_dualsp_services_test_suite);
        MUT_RUN_TESTSUITE(sep_dualsp_normal_io_test_suite);
        MUT_RUN_TESTSUITE(sep_dualsp_degraded_io_test_suite);
        MUT_RUN_TESTSUITE(sep_dualsp_disk_errors_test_suite);
        MUT_RUN_TESTSUITE(sep_dualsp_rebuild_test_suite);
        MUT_RUN_TESTSUITE(sep_dualsp_sparing_test_suite);
        MUT_RUN_TESTSUITE(sep_dualsp_copy_test_suite);
        MUT_RUN_TESTSUITE(sep_dualsp_verify_test_suite);
        MUT_RUN_TESTSUITE(sep_dualsp_zeroing_test_suite);
        MUT_RUN_TESTSUITE(sep_dualsp_sniff_test_suite);
        MUT_RUN_TESTSUITE(sep_dualsp_power_savings_test_suite);
        MUT_RUN_TESTSUITE(sep_dualsp_special_ops_test_suite);
        MUT_RUN_TESTSUITE(sep_dualsp_encryption_test_suite);
        MUT_RUN_TESTSUITE(sep_dualsp_extent_pool_test_suite);

        MUT_RUN_TESTSUITE(esp_test_suite);
    
        /*this suite can be used for anything we want to run after we stopped the SPs, it can be a test of any of the applications
        or anythign else*/
        MUT_RUN_TESTSUITE(system_test_suite);
    }
    return get_executionStatus();
}
/* end of fbe_test_main() */


/****************************
 * end file fbe_test_main.c
 ****************************/
