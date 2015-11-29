/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file kipper_health_check.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for Drive Health Check
 *
 * @version
 *   11/26/2013 - Created.  Wayne Garrett
 *
 ***************************************************************************/


/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_dest_injection_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe/fbe_random.h"

/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char * kipper_short_desc = "Drive Health Check";
char * kipper_long_desc =
    "\n"
    "\n"
    "The Kipper scenario tests drive Health Check with combination of SLF and Download\n"
    "\n"
    "Starting Config:\n"
    "\t [PP] 1-armada board\n"
    "\t [PP] 1-SAS PMC port\n"
    "\t [PP] 4-viper enclosure\n"
    "\t [PP] 60 SAS drives (PDO)\n"
    "\t [PP] 60 logical drives (LD)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "\t- Create and verify the initial physical package config.\n"
    "\t- Verify all the PDO, LDO and PVDs are created and in expected state.\n"
    "\n"
    "STEP 2: Create RG and LUNs\n"
    "\t- Create a raid object with I/O edges attached to all VDs.\n"
    "\t- Create a lun object with an I/O edge attached to the RAID\n"
    "\t  Lun object can be created with exact bind size\n"
    "\t  Lun object can be created with bind offset to test bind offset sizes\n"
    "\t- Verify that all configured objects are in the READY state.\n"
    "\n";

/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/
/*!*******************************************************************
 * @def KIPPER_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define KIPPER_TEST_RAID_GROUP_ID        0

/*!*******************************************************************
 * @def KIPPER_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define KIPPER_LUNS_PER_RAID_GROUP       1

/*!*******************************************************************
 * @def KIPPER_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define KIPPER_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def KIPPER_TEST_RAID_GROUP_WIDTH
 *********************************************************************
 * @brief RAID Group width used by this test
 *
 *********************************************************************/
#define KIPPER_TEST_RAID_GROUP_WIDTH     3

/*!*******************************************************************
 * @var kipper_test_context
 *********************************************************************
 * @brief This contains our context object for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t kipper_test_context[KIPPER_LUNS_PER_RAID_GROUP * 2];


/*!*******************************************************************
 * @var kipper_rg_configuration
 *********************************************************************
 * @brief This is rg_configuration
 *
 *********************************************************************/
static fbe_test_rg_configuration_t kipper_rg_configuration[] = 
{
 /* width,                             capacity, raid type,               class,               block size, RAID-id,                   bandwidth.*/
   { KIPPER_TEST_RAID_GROUP_WIDTH,     0xE000, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520,        KIPPER_TEST_RAID_GROUP_ID, 0},
    
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


/*!*******************************************************************
 * @def KIPPER_DOWNLOAD_SECONDS_PER_DRIVE
 *********************************************************************
 * @brief
 *  We will wait at least 30 seconds in between each drive download
 *  in a raid group.  We double this to cover any variations in
 *  timing caused by running multiple threads in parallel.
 *
 *********************************************************************/
#define KIPPER_DOWNLOAD_SECONDS_PER_DRIVE 60


/*-----------------------------------------------------------------------------
    EXTERN:
*/

extern fbe_status_t sleeping_beauty_wait_for_state(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_lifecycle_state_t state, fbe_u32_t timeout_ms);

/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/

void kipper_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void kipper_run_slf_hc_sequential_test(fbe_test_rg_configuration_t *rg_config_p);
void kipper_run_hc_slf_concurrent_test(fbe_test_rg_configuration_t *rg_config_p);
void kipper_run_slf_hc_concurrent_test(fbe_test_rg_configuration_t *rg_config_p);
void kipper_run_hc_download_test(fbe_test_rg_configuration_t *rg_config_p);
void kipper_run_continual_hc_test(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
fbe_status_t kipper_wait_until_emeh_state(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_raid_emeh_mode_t current_mode, fbe_u32_t timeout_ms);
void kipper_start_io(fbe_test_rg_configuration_t *rg_config_p, fbe_api_rdgen_context_t **rdgen_context_pp, fbe_sim_transport_connection_target_t this_sp);
void kipper_stop_io(fbe_test_rg_configuration_t *rg_config_p, fbe_api_rdgen_context_t *rdgen_context_p, fbe_sim_transport_connection_target_t this_sp);
void kipper_initiate_slf(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_sim_transport_connection_target_t slf_sp);
void kipper_verify_slf(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_sim_transport_connection_target_t slf_sp);
void kipper_initiate_health_check(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_sim_transport_connection_target_t hc_sp);
void kipper_initiate_another_health_check(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_sim_transport_connection_target_t hc_sp);
void kipper_verify_health_check(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_sim_transport_connection_target_t hc_sp);
void kipper_verify_pdo_state(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_sim_transport_connection_target_t hc_sp, fbe_lifecycle_state_t state);
void kipper_initiate_download(fbe_test_rg_configuration_t *rg_config_p, fbe_drive_configuration_control_fw_info_t *fw_info_p, fbe_sim_transport_connection_target_t dl_sp);
void kipper_verify_download(fbe_test_rg_configuration_t *rg_config_p, fbe_drive_configuration_control_fw_info_t *fw_info_p, fbe_sim_transport_connection_target_t dl_sp);
void kipper_restore_hc_sp(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_api_rdgen_context_t *rdgen_context_p, fbe_sim_transport_connection_target_t hc_sp);
void kipper_restore_slf_sp(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_api_rdgen_context_t *rdgen_context_p, fbe_sim_transport_connection_target_t slf_sp);

extern void         fozzie_slf_test_check_drive_slf(fbe_test_rg_configuration_t *rg_config_p, fbe_object_id_t pvd_id, fbe_sim_transport_connection_target_t fail_sp);
extern fbe_status_t fbe_test_dest_utils_init_error_injection_record(fbe_dest_error_record_t * error_injection_record);
extern void         fozzie_slf_test_check_drive_no_slf(fbe_test_rg_configuration_t *rg_config_p, fbe_object_id_t pvd_id);
extern void         sir_topham_hatt_create_fw(fbe_sas_drive_type_t drive_type, fbe_drive_configuration_control_fw_info_t *fw_info);
extern void         sir_topham_hatt_destroy_fw(fbe_drive_configuration_control_fw_info_t *fw_info_p);
extern fbe_status_t sir_topham_hatt_wait_download_process_state(fbe_drive_configuration_download_get_process_info_t * dl_status,
                                                                fbe_drive_configuration_download_process_state_t dl_state,
                                                                fbe_u32_t seconds);
extern void         biff_verify_fw_rev(fbe_test_rg_configuration_t *rg_config_p, const fbe_u8_t * expected_rev, fbe_sim_transport_connection_target_t sp);
extern fbe_status_t biff_test_wait_for_pvds_powerup(const fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t wait_time_ms);

void kipper_run_slf_hc_concurrent_1_test(fbe_test_rg_configuration_t *rg_config_p);
void kipper_initiate_slf_health_check(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_sim_transport_connection_target_t hc_sp);

/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/


/*!****************************************************************************
 * kipper_test()
 ******************************************************************************
 * @brief
 *  This is the main entry point for the kipper test  
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  11/26/2013 - Created. Wayne Garrett
 ******************************************************************************/

void kipper_health_check_dualsp_test(void)
{

    fbe_test_rg_configuration_t *rg_config_p = NULL;
    const fbe_char_t            *raid_type_string_p = NULL;

    
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    rg_config_p = &kipper_rg_configuration[0];

    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);


    /* Enable SLF */
    fbe_api_provision_drive_set_slf_enabled(FBE_TRUE);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_api_provision_drive_set_slf_enabled(FBE_TRUE);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
        
    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                   raid_type_string_p, rg_config_p->raid_type);
        MUT_FAIL_MSG("raid type not enabled");
        return;
    }


    /* Run Tests*/

    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, NULL,
                                                  kipper_run_continual_hc_test,
                                                  KIPPER_LUNS_PER_RAID_GROUP, 
                                                  KIPPER_CHUNKS_PER_LUN,
                                                  FBE_FALSE);


    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, NULL,
                                                  kipper_run_tests,
                                                  KIPPER_LUNS_PER_RAID_GROUP, 
                                                  KIPPER_CHUNKS_PER_LUN, 
                                                  FBE_FALSE);
}


/*!****************************************************************************
 *  kipper_test_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the kipper test. It is responsible
 *  for loading the physical and logical configuration for this test suite.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_dualsp_setup(void)
{

    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for kipper test ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");

     if (fbe_test_util_is_simulation())
    {
         /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        scoobert_physical_config(520);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        scoobert_physical_config(520);
       
        sep_config_load_sep_and_neit_load_balance_both_sps();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;

} /* End kipper_test_setup() */


/*!****************************************************************************
 *  kipper_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Bubble Bass test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_dualsp_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for kipper test ===\n");

    if(fbe_test_util_is_simulation())
    {
        /* Turn off delay for I/O completion in the terminator */
        fbe_api_terminator_set_io_global_completion_delay(0);

        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();

        /* First execute teardown on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    return;
} 

/*!****************************************************************************
 *  kipper_run_tests
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the health check tests,
 *  which is called for each RG in the RG table.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{        
    kipper_run_slf_hc_sequential_test(rg_config_p);
    kipper_run_hc_slf_concurrent_test(rg_config_p);
    kipper_run_slf_hc_concurrent_test(rg_config_p);
    kipper_run_hc_download_test(rg_config_p);
    kipper_run_slf_hc_concurrent_1_test(rg_config_p);
}

/*!****************************************************************************
 *  kipper_run_slf_hc_sequential_test
 ******************************************************************************
 *
 * @brief
 *  This tests SLF and HC being initiated sequentially.
 *  1 - trigger SLF on one SP.
 *  2 - wait until SLF completes 
 *  3 - issue HC on other SP and verify it completes.
 *
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_run_slf_hc_sequential_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t rg_position = 0;  /* drive to test */
    fbe_sim_transport_connection_target_t slf_sp = FBE_SIM_SP_A;
    fbe_sim_transport_connection_target_t hc_sp =  FBE_SIM_SP_B;
    fbe_api_rdgen_context_t *rdgen_context_p = &kipper_test_context[0];

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test Begin ===", __FUNCTION__);
    
    kipper_start_io(rg_config_p, &rdgen_context_p, hc_sp);
    kipper_initiate_slf(rg_config_p, rg_position, slf_sp);    
    kipper_verify_slf(rg_config_p, rg_position, slf_sp);
    kipper_initiate_health_check(rg_config_p, rg_position, hc_sp);
    kipper_verify_health_check(rg_config_p, rg_position, hc_sp);
    kipper_stop_io(rg_config_p, rdgen_context_p, hc_sp);
    kipper_restore_hc_sp(rg_config_p, rg_position, rdgen_context_p, hc_sp);
    kipper_restore_slf_sp(rg_config_p, rg_position, rdgen_context_p, slf_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test End ===\n", __FUNCTION__);
}


/*!****************************************************************************
 *  kipper_run_hc_slf_concurrent_test
 ******************************************************************************
 *
 * @brief
 *  This tests SLF and HC state machines being initiated concurrently,
 *  with HC started first. 
 *
 *
 * @param   rg_config_p - RG configuration.
 * @param   delay_ms - add delay between slf & hc to bring out timing issues.
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_run_hc_slf_concurrent_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t rg_position = 0;  /* drive to test */
    fbe_sim_transport_connection_target_t slf_sp = FBE_SIM_SP_A;
    fbe_sim_transport_connection_target_t hc_sp =  FBE_SIM_SP_B;
    fbe_api_rdgen_context_t *rdgen_context_p = &kipper_test_context[0];

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test Begin ===", __FUNCTION__);

    kipper_start_io(rg_config_p, &rdgen_context_p, hc_sp);
    kipper_initiate_health_check(rg_config_p, rg_position, hc_sp);
    kipper_initiate_slf(rg_config_p, rg_position, slf_sp);        
    kipper_verify_health_check(rg_config_p, rg_position, hc_sp);
    kipper_stop_io(rg_config_p, rdgen_context_p, hc_sp);
    kipper_restore_hc_sp(rg_config_p, rg_position, rdgen_context_p, hc_sp);
    kipper_restore_slf_sp(rg_config_p, rg_position, rdgen_context_p, slf_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test End ===\n", __FUNCTION__);
}


/*!****************************************************************************
 *  kipper_run_slf_hc_concurrent_test
 ******************************************************************************
 *
 * @brief
 *  This tests SLF and HC state machines being initiated concurrently, with
 *  SLF started first.
 *
 *
 * @param   rg_config_p - RG configuration.
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_run_slf_hc_concurrent_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t rg_position = 0;  /* drive to test */
    fbe_sim_transport_connection_target_t slf_sp = FBE_SIM_SP_A;
    fbe_sim_transport_connection_target_t hc_sp =  FBE_SIM_SP_B;
    fbe_api_rdgen_context_t *rdgen_context_p = &kipper_test_context[0];

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test Begin ===", __FUNCTION__);

    kipper_start_io(rg_config_p, &rdgen_context_p, hc_sp);
    kipper_initiate_slf(rg_config_p, rg_position, slf_sp);    
    kipper_initiate_health_check(rg_config_p, rg_position, hc_sp);
    kipper_verify_health_check(rg_config_p, rg_position, hc_sp);
    kipper_stop_io(rg_config_p, rdgen_context_p, hc_sp);
    kipper_restore_hc_sp(rg_config_p, rg_position, rdgen_context_p, hc_sp);
    kipper_restore_slf_sp(rg_config_p, rg_position, rdgen_context_p, slf_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test End ===\n", __FUNCTION__);
}

/*!****************************************************************************
 *  kipper_run_hc_download_test
 ******************************************************************************
 *
 * @brief
 *  This tests SLF and HC state machines being initiated concurrently, with
 *  SLF started first.
 *
 *
 * @param   rg_config_p - RG configuration.
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_run_hc_download_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t rg_position = 0;  /* drive to test */
    fbe_sim_transport_connection_target_t dl_sp = FBE_SIM_SP_A;
    fbe_sim_transport_connection_target_t hc_sp = FBE_SIM_SP_B;
    fbe_sim_transport_connection_target_t io_sp = FBE_SIM_SP_A;
    fbe_api_rdgen_context_t *rdgen_context_p = &kipper_test_context[0];
    fbe_drive_configuration_control_fw_info_t fw_info = {0};

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test Begin ===", __FUNCTION__);

    kipper_start_io(rg_config_p, &rdgen_context_p, io_sp);
    kipper_initiate_health_check(rg_config_p, rg_position, hc_sp);    
    kipper_initiate_download(rg_config_p, &fw_info, dl_sp);
    kipper_verify_health_check(rg_config_p, rg_position, hc_sp);
    kipper_verify_download(rg_config_p, &fw_info, dl_sp);
    kipper_stop_io(rg_config_p, rdgen_context_p, io_sp);
    kipper_restore_hc_sp(rg_config_p, rg_position, rdgen_context_p, hc_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test End ===\n", __FUNCTION__);
}

/*!****************************************************************************
 *  kipper_run_slf_hc_concurrent_1_test
 ******************************************************************************
 *
 * @brief
 *  This tests SLF and HC state machines being initiated concurrently on the same SP, with
 *  SLF started first.
 *
 *
 * @param   rg_config_p - RG configuration.
 *
 * @return  None 
 *
 * @author
 * 07/15/2013 - Created. Jibing Dong
 *****************************************************************************/
void kipper_run_slf_hc_concurrent_1_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t rg_position = 0;  /* drive to test */
    fbe_sim_transport_connection_target_t slf_hc_sp = (fbe_random() % 2 ) ? FBE_SIM_SP_A : FBE_SIM_SP_B;
    fbe_api_rdgen_context_t *rdgen_context_p = &kipper_test_context[0];

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test Begin ===", __FUNCTION__);

    kipper_start_io(rg_config_p, &rdgen_context_p, slf_hc_sp);
    kipper_initiate_slf_health_check(rg_config_p, rg_position, slf_hc_sp);    
    kipper_verify_health_check(rg_config_p, rg_position, slf_hc_sp);
    kipper_stop_io(rg_config_p, rdgen_context_p, slf_hc_sp);
    kipper_restore_hc_sp(rg_config_p, rg_position, rdgen_context_p, slf_hc_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test End ===\n", __FUNCTION__);
}

/*!****************************************************************************
 *  kipper_run_continual_hc_test
 ******************************************************************************
 *
 * @brief
 *  This the slow drive scenario where drive continually goes into HC.  Verify
 *  that DIEH eventually shoots it.
 *
 *
 * @param   rg_config_p - RG configuration.
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_run_continual_hc_test(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_u32_t rg_position = 0;  /* drive to test */
    fbe_sim_transport_connection_target_t hc_sp = (fbe_random() % 2 ) ? FBE_SIM_SP_A : FBE_SIM_SP_B;
    fbe_api_rdgen_context_t *rdgen_context_p = &kipper_test_context[0];    
    fbe_status_t status;    
        
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test Begin ===", __FUNCTION__);

    status = kipper_wait_until_emeh_state(rg_config_p, rg_position, FBE_RAID_EMEH_MODE_ENABLED_NORMAL, 10000 /*ms*/);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "timeout waiting for emeh to clear");

    kipper_start_io(rg_config_p, &rdgen_context_p, hc_sp);
    kipper_initiate_health_check(rg_config_p, rg_position, hc_sp);
    kipper_verify_health_check(rg_config_p, rg_position, hc_sp);
    status = kipper_wait_until_emeh_state(rg_config_p, rg_position, FBE_RAID_EMEH_MODE_DEGRADED_HA, 10000 /*ms*/);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "timeout waiting for emeh to go degraded");
    kipper_verify_pdo_state(rg_config_p, rg_position, hc_sp, FBE_LIFECYCLE_STATE_READY);

    status = kipper_wait_until_emeh_state(rg_config_p, rg_position, FBE_RAID_EMEH_MODE_ENABLED_NORMAL, 10000 /*ms*/);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "timeout waiting for emeh to clear!");
    kipper_initiate_another_health_check(rg_config_p, rg_position, hc_sp);
    /* Verify we fail after second HC */
    kipper_verify_pdo_state(rg_config_p, rg_position, hc_sp, FBE_LIFECYCLE_STATE_FAIL);

    kipper_restore_hc_sp(rg_config_p, rg_position, rdgen_context_p, hc_sp);
    kipper_stop_io(rg_config_p, rdgen_context_p, hc_sp);    

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test End ===\n", __FUNCTION__);
}


fbe_status_t kipper_wait_until_emeh_state(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_raid_emeh_mode_t expect_mode, fbe_u32_t timeout_ms)
{
    fbe_status_t status;
    fbe_raid_group_get_extended_media_error_handling_t get_emeh;
    fbe_u32_t retry_interval_ms = 100;
    fbe_u32_t wait_time_ms = 0;
    fbe_object_id_t rg_object_id, pdo_id;
    fbe_physical_drive_dieh_stats_t dieh_stats;
    fbe_test_raid_group_disk_set_t *drive_to_test_p = &rg_config_p->rg_disk_set[rg_position];

    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    status = fbe_api_get_physical_drive_object_id_by_location(drive_to_test_p->bus, 
                                                              drive_to_test_p->enclosure,
                                                              drive_to_test_p->slot,
                                                              &pdo_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pdo_id != FBE_OBJECT_ID_INVALID);


    do
    {
        status = fbe_api_raid_group_get_extended_media_error_handling(rg_object_id, &get_emeh);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        
        status = fbe_api_physical_drive_get_dieh_info(pdo_id, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "%s: expect_mode=%d current_mode=%d dieh_mode=%d", __FUNCTION__, expect_mode, get_emeh.current_mode, dieh_stats.dieh_mode);

        if ( get_emeh.current_mode == expect_mode )
        {
            return FBE_STATUS_OK;
        }

        fbe_api_sleep(retry_interval_ms);
        wait_time_ms += retry_interval_ms;

    } while (wait_time_ms < timeout_ms);

    mut_printf(MUT_LOG_TEST_STATUS, "%s TIMEOUT. rg=0x%x current_mode=%d", __FUNCTION__, rg_object_id, get_emeh.current_mode);
    return FBE_STATUS_TIMEOUT;
}

/*!****************************************************************************
 *  kipper_start_io
 ******************************************************************************
 *
 * @brief
 *  Starts rdgen IO, which is load balanced between SPs
 *
 *
 * @param   rg_config_p - RG configuration.
 * @param   rdgen_context_pp - RDGEN context
 * @param   this_sp -  SP to start IO from
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_start_io(fbe_test_rg_configuration_t *rg_config_p, fbe_api_rdgen_context_t **rdgen_context_pp, fbe_sim_transport_connection_target_t this_sp)
{
    mut_printf(MUT_LOG_TEST_STATUS, "== %s ==", __FUNCTION__);

    fbe_api_sim_transport_set_target_server(this_sp);    

    big_bird_start_io(rg_config_p, rdgen_context_pp, FBE_U32_MAX, 4096, 
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE);
}


/*!****************************************************************************
 *  kipper_stop_io
 ******************************************************************************
 *
 * @brief
 *  Stop rdgen IO.
 *
 *
 * @param   rg_config_p - RG configuration.
 * @param   rdgen_context_pp - RDGEN context
 * @param   this_sp -  SP to start IO from
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_stop_io(fbe_test_rg_configuration_t *rg_config_p, fbe_api_rdgen_context_t *rdgen_context_p, fbe_sim_transport_connection_target_t this_sp)
{
    fbe_status_t status;
    fbe_u32_t num_contexts = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s ==", __FUNCTION__);

    fbe_api_sim_transport_set_target_server(this_sp);
    
    /* stop IO */   
    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_rdgen_stop_tests(rdgen_context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure there is no error. */
    MUT_ASSERT_INT_EQUAL(rdgen_context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_EQUAL(rdgen_context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    status = fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&rdgen_context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    
}

/*!****************************************************************************
 *  kipper_initiate_slf
 ******************************************************************************
 *
 * @brief
 *  Initiates SLF by removing drive on one SP only, causing a link fault.
 *
 * @param   rg_config_p - RG configuration.
 * @param   rg_position - drive to fault
 * @param   slf_sp -  SP to initiate SLF
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_initiate_slf(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_sim_transport_connection_target_t slf_sp)
{
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = &rg_config_p->rg_disk_set[rg_position];
    fbe_object_id_t pvd_id;    
    fbe_status_t status;
    fbe_api_terminator_device_handle_t drive_handle;

    fbe_api_sim_transport_set_target_server(slf_sp);
 
    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, 
                                                            drive_to_remove_p->enclosure,
                                                            drive_to_remove_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s SP%s %d_%d_%d pvd:0x%x ==", 
               __FUNCTION__, (slf_sp==FBE_SIM_SP_A)?"A":"B", drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot, pvd_id);


    /* Cause link fault by logging out drive on one side */    
    status = fbe_api_terminator_get_drive_handle(drive_to_remove_p->bus, 
                                                 drive_to_remove_p->enclosure,
                                                 drive_to_remove_p->slot,
                                                 &drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);        

    status = fbe_api_terminator_force_logout_device(drive_handle);            
    mut_printf(MUT_LOG_TEST_STATUS, "== Logout drive %d_%d_%d on SP %d Completed. ==", 
               drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot, fbe_api_sim_transport_get_target_server());
}


/*!****************************************************************************
 *  kipper_verify_slf
 ******************************************************************************
 *
 * @brief
 *  Verify PVD is in SLF.
 *
 *
 * @param   rg_config_p - RG configuration.
 * @param   rg_position - drive in SLF
 * @param   slf_sp -  SP which SLF was triggered from.
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_verify_slf(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_sim_transport_connection_target_t slf_sp)
{
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = &rg_config_p->rg_disk_set[rg_position];
    fbe_object_id_t pvd_id;    
    fbe_status_t status;

    fbe_api_sim_transport_set_target_server(slf_sp);

    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, 
                                                            drive_to_remove_p->enclosure,
                                                            drive_to_remove_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* check and wait untl SLF completes its state machine */
    fozzie_slf_test_check_drive_slf(rg_config_p, pvd_id, slf_sp);
}


/*!****************************************************************************
 *  kipper_initiate_health_check
 ******************************************************************************
 *
 * @brief
 *  Initiate Health Check be delaying an IO which will trip the Service Time.
 *
 *
 * @param   rg_config_p - RG configuration.
 * @param   rg_position - drive to test
 * @param   hc_sp -  SP which HC will be triggered from.
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_initiate_health_check(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_sim_transport_connection_target_t hc_sp)
{
    fbe_status_t status;
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = &rg_config_p->rg_disk_set[rg_position];
    fbe_object_id_t pdo_id, pvd_id;
    fbe_time_t service_time_limit_ms = 400;  
    fbe_dest_error_record_t dest_record = {0};
    fbe_dest_record_handle_t dest_record_handle;

    fbe_api_sim_transport_set_target_server(hc_sp);

    status = fbe_api_get_physical_drive_object_id_by_location(drive_to_remove_p->bus, 
                                                              drive_to_remove_p->enclosure,
                                                              drive_to_remove_p->slot,
                                                              &pdo_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pdo_id != FBE_OBJECT_ID_INVALID);

    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, 
                                                            drive_to_remove_p->enclosure,
                                                            drive_to_remove_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s SP%s %d_%d_%d pvd:0x%x ==", 
               __FUNCTION__, (hc_sp==FBE_SIM_SP_A)?"A":"B", drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot, pvd_id);

    
    /* Enable HC and clear error counts. 
    */
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_HEALTH_CHECK_ENABLED, FBE_TRUE /*enable*/);
    fbe_api_physical_drive_set_service_time(pdo_id, service_time_limit_ms);
    
    fbe_api_physical_drive_clear_error_counts(pdo_id, FBE_PACKET_FLAG_NO_ATTRIB);  

    /* Inject delay to trigger HC.
    */
    fbe_test_dest_utils_init_error_injection_record(&dest_record);
    dest_record.object_id = pdo_id;
    dest_record.dest_error_type = FBE_DEST_ERROR_TYPE_PORT;
    dest_record.dest_error.dest_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT;
    dest_record.lba_start = 0;
    dest_record.lba_end = FBE_U64_MAX; 
    dest_record.dest_error.dest_scsi_error.scsi_command[0] = 0xFF;  /*any*/
    dest_record.dest_error.dest_scsi_error.scsi_command_count = 1;
    dest_record.num_of_times_to_insert = 1;
    dest_record.delay_io_msec = (fbe_u32_t)service_time_limit_ms + 100; /* just long enough to trip HC*/

    status = fbe_api_dest_injection_add_record(&dest_record, &dest_record_handle);    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Start error injection */
    status = fbe_api_dest_injection_start();        

    /* An alternate way of initiating health check is through an API.  However, error injection is a more
       accurate way.     
       fbe_api_physical_drive_health_check(pdo_id);  
    */ 
}


/*!****************************************************************************
 *  kipper_initiate_health_check
 ******************************************************************************
 *
 * @brief
 *  Initiate Health Check be delaying an IO which will trip the Service Time.
 *
 *
 * @param   rg_config_p - RG configuration.
 * @param   rg_position - drive to test
 * @param   hc_sp -  SP which HC will be triggered from.
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_initiate_another_health_check(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_sim_transport_connection_target_t hc_sp)
{
    fbe_status_t status;
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = &rg_config_p->rg_disk_set[rg_position];
    fbe_object_id_t pdo_id, pvd_id;
    fbe_time_t service_time_limit_ms = 400;  
    fbe_dest_error_record_t dest_record = {0};
    fbe_dest_record_handle_t dest_record_handle;

    fbe_api_sim_transport_set_target_server(hc_sp);

    status = fbe_api_get_physical_drive_object_id_by_location(drive_to_remove_p->bus, 
                                                              drive_to_remove_p->enclosure,
                                                              drive_to_remove_p->slot,
                                                              &pdo_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pdo_id != FBE_OBJECT_ID_INVALID);

    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, 
                                                            drive_to_remove_p->enclosure,
                                                            drive_to_remove_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s SP%s %d_%d_%d pvd:0x%x ==", 
               __FUNCTION__, (hc_sp==FBE_SIM_SP_A)?"A":"B", drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot, pvd_id);

    
    /* Inject delay to trigger HC.
    */
    fbe_test_dest_utils_init_error_injection_record(&dest_record);
    dest_record.object_id = pdo_id;
    dest_record.dest_error_type = FBE_DEST_ERROR_TYPE_PORT;
    dest_record.dest_error.dest_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT;
    dest_record.lba_start = 0;
    dest_record.lba_end = FBE_U64_MAX; 
    dest_record.dest_error.dest_scsi_error.scsi_command[0] = 0xFF;  /*any*/
    dest_record.dest_error.dest_scsi_error.scsi_command_count = 1;
    dest_record.num_of_times_to_insert = 1;
    dest_record.delay_io_msec = (fbe_u32_t)service_time_limit_ms + 100; /* just long enough to trip HC*/

    status = fbe_api_dest_injection_add_record(&dest_record, &dest_record_handle);    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Start error injection */
    status = fbe_api_dest_injection_start();        

    /* An alternate way of initiating health check is through an API.  However, error injection is a more
       accurate way.     
       fbe_api_physical_drive_health_check(pdo_id);  
    */ 
}


/*!****************************************************************************
 *  kipper_verify_health_check
 ******************************************************************************
 *
 * @brief
 *  Verify Health Check was successful.
 *
 *
 * @param   rg_config_p - RG configuration.
 * @param   rg_position - drive to test
 * @param   hc_sp -  SP which HC was triggered from.
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_verify_health_check(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_sim_transport_connection_target_t hc_sp)
{
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = &rg_config_p->rg_disk_set[rg_position];
    fbe_status_t status;
    fbe_object_id_t pvd_id;
    fbe_provision_drive_get_health_check_info_t health_check_info = {0};
    fbe_bool_t hit_state = FBE_FALSE;
    fbe_u32_t wait_time;
    fbe_u32_t sleep_interval = 100; /*ms*/


    fbe_api_sim_transport_set_target_server(hc_sp);

    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, 
                                                            drive_to_remove_p->enclosure,
                                                            drive_to_remove_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s SP%s %d_%d_%d pvd:0x%x ==", 
               __FUNCTION__, (hc_sp==FBE_SIM_SP_A)?"A":"B", drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot, pvd_id);

    /* verify we enter HC */
    hit_state = FBE_FALSE;
    for (wait_time=0; wait_time<30000; wait_time+=sleep_interval)
    {   
        fbe_api_provision_drive_get_health_check_info(pvd_id, &health_check_info);
        if (health_check_info.local_state != 0)  
        {
            hit_state = FBE_TRUE;
            break;
        }
        fbe_api_sleep(sleep_interval);
    }
    MUT_ASSERT_INT_EQUAL(hit_state, FBE_TRUE);

    /* verify HC completes*/
    hit_state = FBE_FALSE;
    for (wait_time=0; wait_time<30000; wait_time+=sleep_interval)
    {   
        fbe_api_provision_drive_get_health_check_info(pvd_id, &health_check_info);
        if (health_check_info.local_state == 0)  
        {
            hit_state = FBE_TRUE;
            break;
        }
        fbe_api_sleep(sleep_interval);
    }
    MUT_ASSERT_INT_EQUAL(hit_state, FBE_TRUE);
}

/*!****************************************************************************
 *  kipper_verify_pdo_state
 ******************************************************************************
 *
 * @brief
 *  Verify pdo lifecycle state.
 *
 *
 * @param   rg_config_p - RG configuration.
 * @param   rg_position - drive to test
 * @param   hc_sp -  SP which HC was triggered from.
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_verify_pdo_state(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_sim_transport_connection_target_t hc_sp, fbe_lifecycle_state_t state)
{
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = &rg_config_p->rg_disk_set[rg_position];
    fbe_status_t status;

    fbe_api_sim_transport_set_target_server(hc_sp);

    status = fbe_test_pp_util_verify_pdo_state(drive_to_remove_p->bus, 
                                               drive_to_remove_p->enclosure,
                                               drive_to_remove_p->slot,
                                               state, 
                                               30000 /*timeout_ms*/);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status)        
}


/*!****************************************************************************
 *  kipper_initiate_download
 ******************************************************************************
 *
 * @brief
 *  Initiate drive fw download.
 *
 *
 * @param   rg_config_p - RG configuration.
 * @param   rg_position - drive to test
 * @param   dl_sp -  SP which download will be triggered from.
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_initiate_download(fbe_test_rg_configuration_t *rg_config_p, fbe_drive_configuration_control_fw_info_t *fw_info_p, fbe_sim_transport_connection_target_t dl_sp)
{
    fbe_status_t status;
    fbe_test_raid_group_disk_set_t * drive_to_download_p;
    fbe_drive_selected_element_t *drive;
    fbe_u32_t i;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s SP%s ==", __FUNCTION__, (dl_sp==FBE_SIM_SP_A)?"A":"B");

    fbe_api_sim_transport_set_target_server(dl_sp);

    sir_topham_hatt_create_fw(FBE_SAS_DRIVE_SIM_520, fw_info_p);

    /* Select all drives in RG */
    fw_info_p->header_image_p->num_drives_selected = rg_config_p->width;
    fw_info_p->header_image_p->cflags |= FBE_FDF_CFLAGS_SELECT_DRIVE;
    drive = &fw_info_p->header_image_p->first_drive;
    for (i = 0; i < rg_config_p->width; i++)
    {
        drive_to_download_p = &rg_config_p->rg_disk_set[i];
        drive->bus = drive_to_download_p->bus;
        drive->enclosure = drive_to_download_p->enclosure;
        drive->slot = drive_to_download_p->slot;
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Sending download to %d_%d_%d ==", __FUNCTION__,
                   drive->bus, drive->enclosure, drive->slot);
        drive++;
    }

    /* Send download to DCS */
    status = fbe_api_drive_configuration_interface_download_firmware(fw_info_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}


/*!****************************************************************************
 *  kipper_verify_download
 ******************************************************************************
 *
 * @brief
 *  Verify drive fw download completed successfully
 *
 *
 * @param   rg_config_p - RG configuration.
 * @param   rg_position - drive to test
 * @param   dl_sp -  SP which download was triggered from.
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_verify_download(fbe_test_rg_configuration_t *rg_config_p, fbe_drive_configuration_control_fw_info_t *fw_info_p, fbe_sim_transport_connection_target_t dl_sp)
{
    fbe_status_t status;
    fbe_drive_configuration_download_get_process_info_t dl_status = {0};

    mut_printf(MUT_LOG_TEST_STATUS, "== %s SP%s ==", __FUNCTION__, (dl_sp==FBE_SIM_SP_A)?"A":"B");

    fbe_api_sim_transport_set_target_server(dl_sp);

    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_RUNNING, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE);

    /* Check download process finished.   Estimate 30 seconds for each drive. */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, 
                                                         FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL, 
                                                         KIPPER_DOWNLOAD_SECONDS_PER_DRIVE * fw_info_p->header_image_p->num_drives_selected);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE, dl_status.fail_code);

    biff_verify_fw_rev(rg_config_p, "FBE_", dl_sp);

    biff_test_wait_for_pvds_powerup(rg_config_p, 10000 /*msec*/);

    /* Clean up */
    sir_topham_hatt_destroy_fw(fw_info_p);    
}


/*!****************************************************************************
 *  kipper_restore_hc_sp
 ******************************************************************************
 *
 * @brief
 *  Restore the SP state which was used to test Health Check
 *
 *
 * @param   rg_config_p - RG configuration.
 * @param   rg_position - drive to test 
 * @param   rdgen_context_p - RDGEN context
 * @param   hc_sp -  SP which HC was triggered from.
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_restore_hc_sp(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_api_rdgen_context_t *rdgen_context_p, fbe_sim_transport_connection_target_t hc_sp)
{
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = &rg_config_p->rg_disk_set[rg_position];
    fbe_status_t status;
    fbe_object_id_t pdo_id;
    fbe_physical_drive_control_get_service_time_t get_service_time;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s HC:SP%s ==", __FUNCTION__,  (hc_sp==FBE_SIM_SP_A)?"A":"B");

    fbe_api_sim_transport_set_target_server(hc_sp);


    status = fbe_api_get_physical_drive_object_id_by_location(drive_to_remove_p->bus, 
                                                              drive_to_remove_p->enclosure,
                                                              drive_to_remove_p->slot,
                                                              &pdo_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* reset service time*/
    status = fbe_api_physical_drive_get_service_time(pdo_id, &get_service_time);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_physical_drive_set_service_time(pdo_id, get_service_time.default_service_time_ms);

    /* cleanup DEST */    
    fbe_api_dest_injection_stop(); 
    fbe_api_dest_injection_remove_all_records();    
}


/*!****************************************************************************
 *  kipper_restore_slf_sp
 ******************************************************************************
 *
 * @brief
 *  Restore the SP state which was used to test Single Loop Failure
 *
 *
 * @param   rg_config_p - RG configuration.
 * @param   rg_position - drive to test 
 * @param   rdgen_context_p - RDGEN context
 * @param   slf_sp -  SP which SLF was triggered from.
 *
 * @return  None 
 *
 * @author
 * 11/26/2013 - Created. Wayne Garrett
 *****************************************************************************/
void kipper_restore_slf_sp(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_api_rdgen_context_t *rdgen_context_p, fbe_sim_transport_connection_target_t slf_sp)
{
    fbe_test_raid_group_disk_set_t *drive_to_insert_p = &rg_config_p->rg_disk_set[rg_position];
    fbe_status_t status;
    fbe_object_id_t pvd_id;
    fbe_api_terminator_device_handle_t drive_handle;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s restore SLF:SP%s drive %d_%d_%d ==", 
               __FUNCTION__, (slf_sp==FBE_SIM_SP_A)?"A":"B", drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot);

    fbe_api_sim_transport_set_target_server(slf_sp);
    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_insert_p->bus, 
                                                            drive_to_insert_p->enclosure,
                                                            drive_to_insert_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* clear slf */
    status = fbe_api_terminator_get_drive_handle(drive_to_insert_p->bus, 
                                                 drive_to_insert_p->enclosure,
                                                 drive_to_insert_p->slot,
                                                 &drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   

    status = fbe_api_terminator_force_login_device(drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== login drive %d_%d_%d on SP %d Completed. ==", 
        drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot, fbe_api_sim_transport_get_target_server());

    fozzie_slf_test_check_drive_no_slf(rg_config_p, pvd_id);
}


/*!****************************************************************************
 *  kipper_initiate_slf_health_check
 ******************************************************************************
 *
 * @brief
 *  Initiate SLF and Health Check be delaying an IO which will trip the Service Time.
 *
 *
 * @param   rg_config_p - RG configuration.
 * @param   rg_position - drive to test
 * @param   hc_sp -  SP which HC will be triggered from.
 *
 * @return  None 
 *
 * @author
 * 07/15/2013 - Created. Jibing Dong
 *****************************************************************************/
void kipper_initiate_slf_health_check(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t rg_position, fbe_sim_transport_connection_target_t hc_sp)
{
    fbe_status_t status;
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = &rg_config_p->rg_disk_set[rg_position];
    fbe_object_id_t pdo_id, pvd_id;
    fbe_time_t service_time_limit_ms = 400;  
    fbe_dest_error_record_t dest_record = {0};
    fbe_dest_record_handle_t dest_record_handle;

    fbe_api_sim_transport_set_target_server(hc_sp);

    status = fbe_api_get_physical_drive_object_id_by_location(drive_to_remove_p->bus, 
                                                              drive_to_remove_p->enclosure,
                                                              drive_to_remove_p->slot,
                                                              &pdo_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pdo_id != FBE_OBJECT_ID_INVALID);

    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, 
                                                            drive_to_remove_p->enclosure,
                                                            drive_to_remove_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s SP%s %d_%d_%d pvd:0x%x ==", 
               __FUNCTION__, (hc_sp==FBE_SIM_SP_A)?"A":"B", drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot, pvd_id);

    
    /* Enable HC and clear error counts. 
    */
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_HEALTH_CHECK_ENABLED, FBE_TRUE /*enable*/);
    fbe_api_physical_drive_set_service_time(pdo_id, service_time_limit_ms);
    
    fbe_api_physical_drive_clear_error_counts(pdo_id, FBE_PACKET_FLAG_NO_ATTRIB);  

    /* Inject delay to trigger HC.
    */
    fbe_test_dest_utils_init_error_injection_record(&dest_record);
    dest_record.object_id = pdo_id;
    dest_record.dest_error_type = FBE_DEST_ERROR_TYPE_PORT;
    dest_record.dest_error.dest_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT;
    dest_record.lba_start = 0;
    dest_record.lba_end = FBE_U64_MAX; 
    dest_record.dest_error.dest_scsi_error.scsi_command[0] = 0xFF;  /*any*/
    dest_record.dest_error.dest_scsi_error.scsi_command_count = 1;
    dest_record.num_of_times_to_insert = 1;
    dest_record.delay_io_msec = (fbe_u32_t)service_time_limit_ms + 100; /* just long enough to trip HC*/

    status = fbe_api_dest_injection_add_record(&dest_record, &dest_record_handle);    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Start error injection */
    status = fbe_api_dest_injection_start();    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* An alternate way of initiating health check is through an API.  However, error injection is a more
       accurate way.     
       fbe_api_physical_drive_health_check(pdo_id);  
    */ 
}
