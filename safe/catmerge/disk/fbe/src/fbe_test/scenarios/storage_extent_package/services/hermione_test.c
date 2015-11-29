/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file hermione_test.c
 ***************************************************************************
 *
 * @brief
 *  The tests in this file test paged metadata I/O error handling.
 * 
 *  In production code, the PVD and RG objects store a small amount of
 *  information on every chunk of capacity that they manage.  This information
 *  is stored in paged metadata memory.
 * 
 *  These tests confirm that when we issue I/O to an object's paged metadata,
 *  errors are handled correctly.  Many of these tests are destructive in that
 *  they corrupt paged metadata to confirm errors are reported correctly and
 *  the paged metadata is reconstructed correctly if possible.
 * 
 * @version
 *  12/2012  Susan Rundbaken  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_hook.h"
#include "sep_test_io.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "sep_rebuild_utils.h"
#include "fbe/fbe_payload_block_operation.h"
#include "fbe_metadata.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe_test.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "neit_utils.h"
#include "fbe/fbe_api_system_bg_service_interface.h"

/*************************
 *    DEFINITIONS
 *************************/

char * hermione_short_desc = "Paged metadata I/O error handling test.";
char * hermione_long_desc ="\
The Hermione scenario tests I/O error handling to an object's paged metadata.\n\
The RG and PVD object's have some data they store in paged metadata memory.\n\
This data is stored on disk and cached in memory.\n\
These tests confirm that paged metadata I/O errors are handled properly.\n\
STEP 1: Load the physical configuration and SEP/NEIT packages.\n\
STEP 2: Create a user RG.\n\
STEP 3: Disable all background services to ensure RG monitor will not run.\n\
STEP 4: Corrupt RG paged metadata.\n\
STEP 5: Set the rebuild checkpoint to valid checkpoint.\n\
STEP 6: Enabled all background services to let RG monitor run.\n\
STEP 7: Ensure I/O error reported properly for paged metadata.\n\
STEP 8: Ensure paged metadata reconstructed correctly.\n\
STEP 9: Repeat Steps 3 - 8 for verify.\n\
STEP 10: Repeat rebuild test, but reboot SP before corruption complete.\n\
STEP 11: Destroy the configuration.\n\
\n\
Description last updated: 12/16/2011.\n";


/*!*******************************************************************
 * @def HERMIONE_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs in a raid group.
 *********************************************************************/
#define HERMIONE_LUNS_PER_RAID_GROUP    3


/*!*******************************************************************
 * @def HERMIONE_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks in a LUN.
 *********************************************************************/
#define HERMIONE_CHUNKS_PER_LUN    3


/*!*******************************************************************
 * @def HERMIONE_DRIVE_BASE_OFFSET
 *********************************************************************
 * @brief All drives have this base offset that we need to add on.
 *
 *********************************************************************/
#define HERMIONE_DRIVE_BASE_OFFSET 0x10000


/*!*******************************************************************
 * @def HERMIONE_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define HERMIONE_WAIT_MSEC 120000


/*!*******************************************************************
 * @def HERMIONE_LUN_ELEMENT_SIZE
 *********************************************************************
 * @brief Element size of our LUNs.
 *
 *********************************************************************/
#define HERMIONE_LUN_ELEMENT_SIZE 128


/*!*******************************************************************
 * @var hermione_rdgen_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t hermione_rdgen_test_contexts[HERMIONE_LUNS_PER_RAID_GROUP];


/*!********************************************************************* 
 * @struct hermione_lei_test_params_t
 *  
 * @brief 
 *   Contains some of the Logical Error Injection test parameters for a LEI test.
 *   These are the parameters that can vary for each RG test.
 *
 **********************************************************************/
typedef struct hermione_lei_test_params_s
{
    fbe_u32_t                                   position_bitmap;
    fbe_api_logical_error_injection_type_t      error_type;
}
hermione_lei_test_params_t;


/*!*******************************************************************
 * @var lei_test_case
 *********************************************************************
 * @brief This is the array of LEI test data we will loop over. This data
 *          lines up with the array types below.
 *
 *********************************************************************/
hermione_lei_test_params_t  lei_test_case[] =
{
  /*   pos bitmap,      error type to inject */
    //{0x3,            FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR},    /* RAID1 */
    //{0x6,            FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR},    /* RAID5 */
    {0xf,            FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR},    /* RAID6 */              
    {FBE_U32_MAX,    FBE_U32_MAX /* Terminator. */},       
};


/*!*******************************************************************
 * @var hermione_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t hermione_raid_group_config_qual[] = 
{
  /* width,   capacity    raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    //{2,       0x32000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            1,          0},
    //{3,       0x32000,    FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            2,          0},
    {4,       0x32000,    FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            2,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static void hermione_run_tests(fbe_test_rg_configuration_t * in_rg_config_p, void * context_p);
static void hermione_run_corrupt_paged_rebuild_test(fbe_test_rg_configuration_t * rg_config_p, 
                                                    hermione_lei_test_params_t * lei_test_case_p, 
                                                    fbe_lba_t chkpt,
                                                    fbe_bool_t reboot_b);
static void hermione_run_corrupt_paged_verify_test(fbe_test_rg_configuration_t * rg_config_p, 
                                                   hermione_lei_test_params_t * lei_test_case_p, 
                                                   fbe_lba_t chkpt,
                                                   fbe_bool_t reboot_b);
static void hermione_corrupt_paged_metadata(fbe_test_rg_configuration_t * rg_config_p, 
                                            hermione_lei_test_params_t * lei_test_case_p,
                                            fbe_bool_t reboot_b);
static void hermione_test_run_io(fbe_object_id_t      object_id,
                                 fbe_lba_t            start_lba,
                                 fbe_u32_t            blocks,
                                 fbe_block_size_t     block_size,
                                 fbe_payload_block_operation_opcode_t opcode,
                                 fbe_data_pattern_flags_t pattern);
static void hermione_confirm_verify_checkpoint_info(fbe_object_id_t rg_object_id,
                                                    fbe_raid_group_set_verify_checkpoint_t rg_verify_checkpoint_info);
static void hermione_disable_all_bg_services_on_peer(void);
static void hermione_enable_all_bg_services_on_peer(void);
static void hermione_clear_paged_md_cache_on_peer(fbe_object_id_t object_id);
static void hermione_wait_for_lun_verify_completion(fbe_object_id_t            in_object_id,
                                                    fbe_lun_verify_report_t*   in_verify_report_p,
                                                    fbe_u32_t                  in_pass_count);
static void hermione_write_background_pattern(fbe_test_rg_configuration_t * in_rg_config_p);
static void hermione_check_background_pattern(fbe_test_rg_configuration_t * in_rg_config_p);
static void hermione_add_debug_hook(fbe_object_id_t rg_object_id, fbe_u32_t state, fbe_u32_t substate);
static fbe_status_t hermione_wait_for_target_SP_hook(fbe_object_id_t rg_object_id,fbe_u32_t state, fbe_u32_t substate);
static fbe_status_t hermione_delete_debug_hook(fbe_object_id_t rg_object_id, fbe_u32_t state, fbe_u32_t substate);
static void hermione_wait_for_activate_scheduler_hooks(fbe_object_id_t  in_object_id);
static void hermione_inject_error_record(
                                fbe_object_id_t                             rg_object_id,
                                fbe_u16_t                                   position_bitmap,
                                fbe_u16_t                                   rg_width,
                                fbe_lba_t                                   lba,
                                fbe_block_count_t                           num_blocks,
                                fbe_u32_t                                   op_code,
                                fbe_api_logical_error_injection_type_t      error_type);
static void hermione_wait_for_error_injection_complete(
                                fbe_test_rg_configuration_t * rg_config_p,
                                fbe_u16_t position_bitmap,
                                fbe_u16_t err_record_cnt);
static void hermione_reboot_sp(fbe_object_id_t rg_object_id, fbe_u32_t state, fbe_u32_t substate);

static void hermione_corrupt_pmd_region(fbe_test_rg_configuration_t *rg_config_p,
                                        fbe_u32_t                   disk_position,
                                        fbe_u32_t                   offset_lba,
                                        fbe_u32_t                   lba_count);

static void hermione_run_metadata_err_verify_test(fbe_test_rg_configuration_t * rg_config_p,
                                                  hermione_lei_test_params_t * lei_test_case_p,
                                                  fbe_bool_t reboot_b);

static void hermione_run_metadata_reconstruction_test(fbe_test_rg_configuration_t * rg_config_p);
static void hermione_run_peer_death_metadata_reconstruction_test(fbe_test_rg_configuration_t * rg_config_p);


/*!**************************************************************
 *  hermione_test()
 ****************************************************************
 * @brief
 *  Run a series of I/O tests to an object's paged metadata.
 *  The test runs single-SP here.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 12/2012   Created.   Susan Rundbaken
 *
 ****************************************************************/
void hermione_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    rg_config_p = &hermione_raid_group_config_qual[0];
    fbe_test_run_test_on_rg_config(rg_config_p, 
                                   NULL, 
                                   hermione_run_tests,
                                   HERMIONE_LUNS_PER_RAID_GROUP,
                                   HERMIONE_CHUNKS_PER_LUN);

    return;
}
/******************************************
 * end hermione_test()
 ******************************************/

/*!**************************************************************
 *  hermione_setup()
 ****************************************************************
 * @brief
 *  Setup the Hermione test for single-SP testing.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  12/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/
void hermione_setup(void)
{    
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        rg_config_p = &hermione_raid_group_config_qual[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_esp_sep_and_neit();
    }

    /* The following utility function does some setup for hardware. 
     */
    fbe_test_common_util_test_setup_init();
}
/**************************************
 * end hermione_setup()
 **************************************/

/*!**************************************************************
 *  hermione_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the Hermione test for single-SP testing.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 12/2011  Created. Susan Rundbaken 
 *
 ****************************************************************/
void hermione_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_esp_neit_sep_physical();
    }
    return;
}
/******************************************
 * end hermione_cleanup()
 ******************************************/

/*!**************************************************************
 *  hermione_dualsp_test()
 ****************************************************************
 * @brief
 *  Run a series of I/O tests to an object's paged metadata.
 *  The test runs dual-SP here.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 12/2012   Created.   Susan Rundbaken
 *
 ****************************************************************/
void hermione_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    rg_config_p = &hermione_raid_group_config_qual[0];
    fbe_test_run_test_on_rg_config(rg_config_p, 
                                   NULL, 
                                   hermione_run_tests,
                                   HERMIONE_LUNS_PER_RAID_GROUP,
                                   HERMIONE_CHUNKS_PER_LUN);

    return;
}
/******************************************
 * end hermione_dualsp_test()
 ******************************************/

/*!**************************************************************
 *  hermione_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup the Hermione test for dual-SP testing.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  12/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/
void hermione_dualsp_setup(void)
{    
    fbe_sim_transport_connection_target_t sp;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        rg_config_p = &hermione_raid_group_config_qual[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        //elmo_load_sep_and_neit();

         /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    
        /* Load the physical config on the target server */    
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        //elmo_load_sep_and_neit();

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */

        sep_config_load_esp_sep_and_neit_both_sps();
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    /* The following utility function does some setup for hardware. 
     */
    fbe_test_common_util_test_setup_init();
}
/**************************************
 * end hermione_dualsp_setup()
 **************************************/

/*!**************************************************************
 *  hermione_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the Hermione test for dual-SP testing.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 12/2011  Created. Susan Rundbaken 
 *
 ****************************************************************/
void hermione_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_esp_neit_sep_physical_both_sps();
    }
    return;
}
/******************************************
 * end hermione_dualsp_cleanup()
 ******************************************/

/*!**************************************************************************
 *  hermione_run_tests
 ****************************************************************************
 * @brief
 *  Collection of tests to run for paged metadata I/O error handling, focusing on reconstruction.
 *
 * @param rg_config_p - pointer to rg to use for test.
 * @param context_p - test context (currently not used)
 *
 * @return None.
 *
 * @author
 * 01/2012  Created. Susan Rundbaken 
 *
 ****************************************************************/
void hermione_run_tests(fbe_test_rg_configuration_t * rg_config_p, void * context_p)
{
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t       iter;
    hermione_lei_test_params_t * lei_test_case_p = NULL;
    fbe_lba_t       chkpt;
    fbe_bool_t      reboot_b = FBE_FALSE;

    
    for (iter = 0; iter < raid_group_count; iter++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");
        mut_printf(MUT_LOG_TEST_STATUS, "**** %s Started for RG %d ****\n", __FUNCTION__, iter);
        mut_printf(MUT_LOG_TEST_STATUS, "==============================\n");

        /* Get a pointer to the LEI test case parameters for the test for this RG.
         */
        lei_test_case_p = &lei_test_case[iter];
       
        /* Metdata error verify test.
         */
        hermione_run_metadata_err_verify_test(rg_config_p, lei_test_case_p, FALSE);

        /* Corrupt paged metadata for rebuild and confirm reconstruction completes successfully.
         * Repeat for a different checkpoint.
         */
        chkpt = 0x0;
        hermione_run_corrupt_paged_rebuild_test(rg_config_p, lei_test_case_p, chkpt, reboot_b);

        chkpt = 0x800;
        hermione_run_corrupt_paged_rebuild_test(rg_config_p, lei_test_case_p, chkpt, reboot_b);

        /* Corrupt paged metadata for verify and confirm reconstruction completes successfully.
         * Repeat for a different checkpoint.
         */
        chkpt = 0x0;
        hermione_run_corrupt_paged_verify_test(rg_config_p, lei_test_case_p, chkpt, reboot_b);
   
        chkpt = 0x800;
        hermione_run_corrupt_paged_verify_test(rg_config_p, lei_test_case_p, chkpt, reboot_b);

        /* Corrupt paged metadata for a rebuild and confirm reconstruction completes successfully
         * after a reboot.
         */
        reboot_b = FBE_TRUE;
        chkpt = 0x800;
        hermione_run_corrupt_paged_rebuild_test(rg_config_p, lei_test_case_p, chkpt, reboot_b);

        // Metadata incomplete write tests.
        if(fbe_test_sep_util_get_dualsp_test_mode())
        {
			hermione_run_peer_death_metadata_reconstruction_test(rg_config_p);
        }
        else
        {
            hermione_run_metadata_reconstruction_test(rg_config_p);
        }

        mut_printf(MUT_LOG_TEST_STATUS, "**** %s Completed successfully for RG %d ****", __FUNCTION__, iter);

        rg_config_p++;
    }

    return;
}
/******************************************
 * end hermione_run_tests()
 ******************************************/

/*!**************************************************************
 *  hermione_run_corrupt_paged_rebuild_test()
 ****************************************************************
 * @brief
 *  Run test that corrupts RG paged metadata and confirms
 *  reconstructed on error properly as part of triggering a rebuild.
 *
 * @param rg_config_p - pointer to rg to use for test.
 * @param lei_test_case - logical error injection parameters for test.
 * @param chkpt - checkpoint value to set
 * @param reboot_b - boolean indicating if reboot during test or not
 *
 * @return None.
 *
 * @author
 * 12/2011   Created.   Susan Rundbaken
 *
 ****************************************************************/
static void hermione_run_corrupt_paged_rebuild_test(fbe_test_rg_configuration_t * rg_config_p,
                                                    hermione_lei_test_params_t * lei_test_case_p,
                                                    fbe_lba_t chkpt,
                                                    fbe_bool_t reboot_b)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_raid_group_get_info_t   rg_info;
    fbe_raid_group_set_rb_checkpoint_t rg_rb_checkpoint_info;
    fbe_scheduler_debug_hook_t hook;
    fbe_sim_transport_connection_target_t active_target;
    fbe_bool_t b_run_on_dualsp = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_test_logical_unit_configuration_t*  logical_unit_configuration_p = NULL;
    fbe_u32_t                               lun_index;
    

    mut_printf(MUT_LOG_TEST_STATUS, "\n");
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s entry; reboot_b: %d ****\n", __FUNCTION__, reboot_b);

    /* For dual-SP, make sure we are the active side.
     * All work is done on the active SP.
     */
    if (b_run_on_dualsp)
    {
        fbe_test_sep_get_active_target_id(&active_target);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_target);

        fbe_api_sim_transport_set_target_server(active_target);
    }

    /* Write a background pattern to the LUNs in the RG.
     */
    hermione_write_background_pattern(rg_config_p);

    /* Make sure the background pattern was written okay.
     */
    hermione_check_background_pattern(rg_config_p);

    /* Clear all debug hooks.
     */
    fbe_api_scheduler_clear_all_debug_hooks(&hook);

    /* Get the RG object id.
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    /* Add a debug hook so we confirm we get to ACTIVATE state for reconstruction.
     */
    hermione_add_debug_hook(rg_object_id, SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE,
                            FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_REQUEST);

    /* Add a debug hook so we confirm we get to "verify paged metadata".
     */
    hermione_add_debug_hook(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                            FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);

    hermione_add_debug_hook(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                            FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED);

    /* Disable all background services.
     */
    mut_printf(MUT_LOG_HIGH, "%s: disable all background servces", __FUNCTION__);
    fbe_api_base_config_disable_all_bg_services();

    /* For dual-sp mode, do this on the peer as well.
     */
    if (b_run_on_dualsp)
    {
        hermione_disable_all_bg_services_on_peer();
    }

    /* Corrupt the RG's paged metadata on disk so we can force reconstruction.
     */
    hermione_corrupt_paged_metadata(rg_config_p, lei_test_case_p, reboot_b);

    /* Set RG rebuild checkpoint to a valid checkpoint so we will try to do a rebuild
     * which will read paged metadata that we have corrupted.
     */
    rg_rb_checkpoint_info.position = 0;
    rg_rb_checkpoint_info.rebuild_checkpoint = chkpt;
    mut_printf(MUT_LOG_HIGH, "%s: set rebuild checkpoint 0x%llx for position 0x%x", 
               __FUNCTION__, 
               (unsigned long long)rg_rb_checkpoint_info.rebuild_checkpoint,
               rg_rb_checkpoint_info.position);
    status = fbe_api_raid_group_set_rebuild_checkpoint(rg_object_id, rg_rb_checkpoint_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Confirm the rebuild checkpoint is set as expected.
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                         FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (rg_rb_checkpoint_info.rebuild_checkpoint != rg_info.rebuild_checkpoint[rg_rb_checkpoint_info.position])
    {
        MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Failed to set rebuild checkpoint");
    }

    /* If reboot parameter specified, reboot the SP now.
     * Note: this applies to single-SP for now.
     */
    if (reboot_b && !b_run_on_dualsp)
    {
        /* Add a debug hook so we confirm we get to "verify paged metadata" in activate state on reboot.
         */
        hermione_reboot_sp(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE, 
                           FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);

        status = hermione_wait_for_target_SP_hook(rg_object_id, 
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        hermione_delete_debug_hook(rg_object_id, 
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                   FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);
        
    }
    else
    {
        /* Enable all background services.
         * This will allow the rebuild to run.
         */
        mut_printf(MUT_LOG_HIGH, "%s: enable all background services", __FUNCTION__);
        fbe_api_base_config_enable_all_bg_services();
    
        /* For dual-sp mode, do this on the peer as well.
         */
        if (b_run_on_dualsp)
        {
            hermione_enable_all_bg_services_on_peer();
        }

        /* Confirm we have transitioned to the CLUSTERED_ACTIVATE state by using our scheduler hooks.
         * We do reconstruction from this state.
         */
        hermione_wait_for_activate_scheduler_hooks(rg_object_id);
        status = hermione_wait_for_target_SP_hook(rg_object_id, 
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED);
        /* Make sure errors were injected okay.
         */
        hermione_wait_for_error_injection_complete(rg_config_p,
                                                   lei_test_case_p->position_bitmap,
                                                   2    /* number of LEI records to wait for */);

     status = hermione_delete_debug_hook(rg_object_id, 
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED);
    }

     
     /* Wait for object to get back to the READY state.
      * We transition back to READY after successful reconstruction.
      */
    mut_printf(MUT_LOG_HIGH, "%s: confirm RG 0x%x in READY state", __FUNCTION__, rg_object_id);
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                          HERMIONE_WAIT_MSEC*3, FBE_PACKAGE_ID_SEP_0);

    /* Confirm reconstruction successful.
     * Wait for rebuild to be complete.
     */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, rg_rb_checkpoint_info.position);

    /* Confirm LUNs in READY state.
     */
    logical_unit_configuration_p = rg_config_p->logical_unit_configuration_list;
    for (lun_index = 0; lun_index < HERMIONE_LUNS_PER_RAID_GROUP; lun_index++)
    {
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        mut_printf(MUT_LOG_HIGH, "%s: confirm LUN 0x%x in READY state", __FUNCTION__, lun_object_id);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                              HERMIONE_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);

        /* Goto next LUN.
         */
        logical_unit_configuration_p++;
    }

    /* Make sure the background pattern still okay.
     */
    hermione_check_background_pattern(rg_config_p);

    return;
}
/******************************************
 * end hermione_run_corrupt_paged_rebuild_test()
 ******************************************/

/*!**************************************************************
 *  hermione_run_corrupt_paged_verify_test()
 ****************************************************************
 * @brief
 *  Run test that corrupts RG paged metadata and confirms
 *  reconstructed on error properly as part of triggering a verify.
 *
 * @param rg_config_p - pointer to rg to use for test.
 * @param lei_test_case - logical error injection parameters for test.
 * @param chkpt - checkpoint value to set
 * @param reboot_b - boolean indicating if reboot during test or not
 *
 * @return None.
 *
 * @author
 * 12/2011   Created.   Susan Rundbaken
 *
 ****************************************************************/
static void hermione_run_corrupt_paged_verify_test(fbe_test_rg_configuration_t * rg_config_p,
                                                   hermione_lei_test_params_t * lei_test_case_p,
                                                   fbe_lba_t chkpt,
                                                   fbe_bool_t reboot_b)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_raid_group_set_verify_checkpoint_t  rg_verify_checkpoint_info;
    fbe_test_logical_unit_configuration_t*  logical_unit_configuration_p = NULL;
    fbe_object_id_t                         first_lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_lun_verify_report_t                 verify_report[HERMIONE_LUNS_PER_RAID_GROUP] = {0};
    fbe_u32_t                               pass_count;
    fbe_u32_t                               lun_index;
    fbe_scheduler_debug_hook_t              hook;
    fbe_sim_transport_connection_target_t active_target;
    fbe_bool_t b_run_on_dualsp = fbe_test_sep_util_get_dualsp_test_mode();

    mut_printf(MUT_LOG_TEST_STATUS, "\n");
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s entry; reboot_b: %d ****\n", __FUNCTION__, reboot_b);

    /* For dual-SP, make sure we are the active side.
     * All work is done on the active SP.
     */
    if (b_run_on_dualsp)
    {
        fbe_test_sep_get_active_target_id(&active_target);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_target);

        fbe_api_sim_transport_set_target_server(active_target);
    }

    /* Clear all debug hooks.
     */
    fbe_api_scheduler_clear_all_debug_hooks(&hook);

    /* Get the RG object id.
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    /* Add a debug hook so we confirm we get to ACTIVATE state for reconstruction.
     */
    hermione_add_debug_hook(rg_object_id, SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE,
                            FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_REQUEST);

    /* Add a debug hook so we confirm we get to "verify paged metadata".
     */
    hermione_add_debug_hook(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                            FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);

    hermione_add_debug_hook(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                            FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED);

    /* Disable all background services.
     */
    mut_printf(MUT_LOG_HIGH, "%s: disable all background servces", __FUNCTION__);
    fbe_api_base_config_disable_all_bg_services();

    /* For dual-sp mode, do this on the peer as well.
     */
    if (b_run_on_dualsp)
    {
        hermione_disable_all_bg_services_on_peer();
    }

    /* Corrupt the RG's paged metadata on disk so we can force reconstruction.
     */
    hermione_corrupt_paged_metadata(rg_config_p, lei_test_case_p, reboot_b);

    /* Set a RG verify checkpoint to a valid checkpoint so we will try to do a verify
     * which will read paged metadata that we have corrupted.
     */
    rg_verify_checkpoint_info.verify_flags = FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE;
    rg_verify_checkpoint_info.verify_checkpoint = chkpt;
    mut_printf(MUT_LOG_HIGH, "%s: set verify checkpoint 0x%llx for type 0x%x", 
               __FUNCTION__, 
               (unsigned long long)rg_verify_checkpoint_info.verify_checkpoint, 
               rg_verify_checkpoint_info.verify_flags);
    status = fbe_api_raid_group_set_verify_checkpoint(rg_object_id, rg_verify_checkpoint_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Confirm the verify checkpoint is set as expected.
     */
    hermione_confirm_verify_checkpoint_info(rg_object_id, rg_verify_checkpoint_info);

    /* Get first LUN object ID.
     */
    logical_unit_configuration_p = rg_config_p->logical_unit_configuration_list;
    fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &first_lun_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, first_lun_object_id);

    /* Clear verify report.
     */
    fbe_test_sep_util_lun_clear_verify_report(first_lun_object_id);

    /* If reboot parameter specified, reboot the SP now.
     * Note: this applies to single-SP for now.
     */
    if (reboot_b && !b_run_on_dualsp)
    {
        /* Add a debug hook so we confirm we get to "verify paged metadata" in activate state on reboot.
         */
        hermione_reboot_sp(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE, 
                           FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);

        status = hermione_wait_for_target_SP_hook(rg_object_id, 
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        hermione_delete_debug_hook(rg_object_id, 
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                   FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);
        
    }
    else
    {
        /* Enable all background services.
         * This will allow the rebuild to run.
         */
        mut_printf(MUT_LOG_HIGH, "%s: enable all background services", __FUNCTION__);
        fbe_api_base_config_enable_all_bg_services();
    
        /* For dual-sp mode, do this on the peer as well.
         */
        if (b_run_on_dualsp)
        {
            hermione_enable_all_bg_services_on_peer();
        }

        /* Confirm we have transitioned to the CLUSTERED_ACTIVATE state by using our scheduler hooks.
         * We do reconstruction from this state.
         */
        hermione_wait_for_activate_scheduler_hooks(rg_object_id);
        status = hermione_wait_for_target_SP_hook(rg_object_id, 
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                              FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED);
        
        /*  Make sure errors were injected okay.
         */   
        hermione_wait_for_error_injection_complete(rg_config_p,
                                                   lei_test_case_p->position_bitmap,
                                                   2    /* number of LEI records to wait for */);

        hermione_delete_debug_hook(rg_object_id, 
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                   FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED);
    }

    /* Wait for object to get back to the READY state.
     * We transition back to READY after successful reconstruction.
     */
    mut_printf(MUT_LOG_HIGH, "%s: confirm RG in READY state", __FUNCTION__);
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                          HERMIONE_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
     
    /* Confirm LUNs in READY state.
     */
    logical_unit_configuration_p = rg_config_p->logical_unit_configuration_list;
    for (lun_index = 0; lun_index < HERMIONE_LUNS_PER_RAID_GROUP; lun_index++)
    {
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        mut_printf(MUT_LOG_HIGH, "%s: confirm LUN 0x%x in READY state", __FUNCTION__, lun_object_id);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                              HERMIONE_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);

        /* Goto next LUN.
         */
        logical_unit_configuration_p++;
    }

    /* Wait for the verify to complete on the first LUN in the RG.
     * We would be able to verify the LUNs after successful paged metadata reconstruction.
     */
    pass_count = 1;
    mut_printf(MUT_LOG_HIGH, "%s: wait for LUN 0x%x verify to complete", __FUNCTION__, first_lun_object_id);
    hermione_wait_for_lun_verify_completion(first_lun_object_id, &verify_report[0], pass_count);

    /* Confirm LUNs in READY state.
    */
    logical_unit_configuration_p = rg_config_p->logical_unit_configuration_list;
    for (lun_index = 0; lun_index < HERMIONE_LUNS_PER_RAID_GROUP; lun_index++)
    {
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
    
        mut_printf(MUT_LOG_HIGH, "%s: confirm LUN 0x%x in READY state", __FUNCTION__, lun_object_id);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                              HERMIONE_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);

        logical_unit_configuration_p++;
    }
    

    return;
}
/******************************************
 * end hermione_run_corrupt_paged_verify_test()
 ******************************************/

/*!**************************************************************
 *  hermione_corrupt_paged_metadata()
 ****************************************************************
 * @brief
 *  This function corrupts the paged metadata for the specified RG.
 *
 * @param rg_config_p - pointer to rg to use for test.
 * @param lei_test_case_p - pointer to the LEI test case parameters for this rg
 * @param reboot_b - boolean indicating if this is a reboot test or not
 *
 * @return None.
 *
 * @author
 * 12/2011   Created.   Susan Rundbaken
 *
 ****************************************************************/
static void hermione_corrupt_paged_metadata(fbe_test_rg_configuration_t * rg_config_p,
                                            hermione_lei_test_params_t * lei_test_case_p,
                                            fbe_bool_t reboot_b)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_raid_group_get_info_t   rg_info;
    fbe_lba_t                       start_lba;
    fbe_bool_t b_run_on_dualsp = fbe_test_sep_util_get_dualsp_test_mode();


    /* Get the RG info.
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                         FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (reboot_b)
    {
        fbe_u32_t iter;
        fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;

        /* Corrupt RG paged metadata on the underlying PVDs.
         * We are doing this persistently for the reboot test.
         * !@todo: this applies to mirror raid groups; need to update for parity RG.
         */
        for (iter = 0; iter < rg_config_p->width; iter++)
        {
            /* Get the RG PVD object ID.
             */
            status = fbe_api_provision_drive_get_obj_id_by_location(
                rg_config_p->rg_disk_set[iter].bus,
                rg_config_p->rg_disk_set[iter].enclosure,
                rg_config_p->rg_disk_set[iter].slot,
                &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
            /* Get the RG paged metadata start LBA for the PVD.
             * Need to account for the edge offset.
             */
            start_lba = rg_info.paged_metadata_start_lba / rg_info.num_data_disk;
            start_lba += HERMIONE_DRIVE_BASE_OFFSET;
    
            mut_printf(MUT_LOG_HIGH, "%s: pvd 0x%x: will corrupt paged metadata at lba 0x%x", 
                       __FUNCTION__,
                       pvd_object_id, 
                       (unsigned int)(unsigned long long)start_lba);
    
            /* Corrupt RG paged metadata on the PVD.
             */
            hermione_test_run_io(pvd_object_id, 
                                 start_lba,
                                 64, /* num blocks */
                                 FBE_METADATA_BLOCK_SIZE,
                                 FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                 FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA);
        }
    }
    else
    {
        /* Get the RG paged metadata start LBA.
         */
        start_lba = rg_info.paged_metadata_start_lba / rg_info.num_data_disk;

        mut_printf(MUT_LOG_HIGH, "%s: will corrupt paged metadata at lba 0x%x", 
                   __FUNCTION__,
                   (unsigned int)start_lba);
    
        /* Corrupt RG paged metadata by using LEI to seed an error.
         */
        hermione_inject_error_record(rg_object_id,
                                     lei_test_case_p->position_bitmap,
                                     rg_config_p->width,
                                     start_lba,
                                     300, /* num blocks */
                                     0,
                                     lei_test_case_p->error_type);
    }
    
    /* Ensure the paged metadata memory cache is cleared on RG so will be forced to get data from disk.
     */
    status = fbe_api_base_config_metadata_paged_clear_cache(rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Clear cache on peer as well, if available.
     */
    if (b_run_on_dualsp)
    {
        hermione_clear_paged_md_cache_on_peer(rg_object_id);
    }

    return;
}
/******************************************
 * end hermione_corrupt_paged_metadata()
 ******************************************/

/*!**************************************************************
 *  hermione_test_run_io()
 ****************************************************************
 * @brief
 *  Issue I/O to the given object at the given LBA for the specified
 *  number of blocks.
 *
 * @param object_id - object ID of object to issue I/O to.
 * @param start_lba - starting LBA for I/O request
 * @param blocks    - number of blocks for I/O request
 * @param block_size - number of bytes per block
 * @param opcode    - type of I/O request (read or write)
 * @param pattern   - pattern for writes
 *
 * @return None.
 *
 * @author
 * 12/2011   Created.   Susan Rundbaken
 *
 ****************************************************************/
static void hermione_test_run_io(fbe_object_id_t      object_id,
                                 fbe_lba_t            start_lba,
                                 fbe_u32_t            blocks,
                                 fbe_block_size_t     block_size,
                                 fbe_payload_block_operation_opcode_t opcode,
                                 fbe_data_pattern_flags_t pattern)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sg_element_t * sg_list_p;
    fbe_u8_t * buffer;
    fbe_payload_block_operation_t		block_operation;
    fbe_block_transport_block_operation_info_t	block_operation_info;
    fbe_raid_verify_error_counts_t		verify_err_counts={0};
    fbe_u64_t                                   corrupt_blocks_bitmap= 0;


    /* Allocate memory for a 2-element sg list.
     */
    buffer = fbe_api_allocate_memory(blocks * block_size + 2*sizeof(fbe_sg_element_t));
    sg_list_p = (fbe_sg_element_t *)buffer;

    /* Create sg list.
     */
    sg_list_p[0].address = buffer + 2*sizeof(fbe_sg_element_t);
    sg_list_p[0].count = blocks * block_size;
    sg_list_p[1].address = NULL;
    sg_list_p[1].count = 0;

    /* For a write request, set data pattern in sg list.
     */
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)
    {
        fbe_data_pattern_info_t     data_pattern_info;
        fbe_u32_t                   sg_element_count = fbe_sg_element_count_entries(sg_list_p);

        status = fbe_data_pattern_build_info(&data_pattern_info,
                                             FBE_DATA_PATTERN_LBA_PASS,
                                             pattern,
                                             (fbe_u32_t)start_lba,
                                             0x55,  /* sequence id used in seed; arbitrary */
                                             0,     /* no header words */
                                             NULL   /* no header array */);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (pattern == FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA)
        {
            corrupt_blocks_bitmap = (fbe_u64_t)-1;
        }
        status = fbe_data_pattern_fill_sg_list(sg_list_p,
                                               &data_pattern_info,
                                               block_size,
                                               corrupt_blocks_bitmap, /* Corrupt data corrupts all blocks */
                                               sg_element_count);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    
    }

    /* Create appropriate payload.
     */
    fbe_payload_block_build_operation(&block_operation,
                                      opcode,
                                      start_lba,
                                      blocks,
                                      block_size,
                                      1, /* optimum block size */
                                      NULL);
    
    block_operation_info.block_operation = block_operation;
    block_operation_info.verify_error_counts = verify_err_counts;

    mut_printf(MUT_LOG_LOW, "=== sending IO\n");

    status = fbe_api_block_transport_send_block_operation(object_id, 
                                                          FBE_PACKAGE_ID_SEP_0, 
                                                          &block_operation_info, 
                                                          sg_list_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

    return;
}
/******************************************
 * end hermione_test_run_io()
 ******************************************/

/*!**************************************************************
 * hermione_wait_for_target_SP_hook()
 ****************************************************************
 * @brief
 *  Wait for the hook to be hit
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  01/31/2012 - Copied from Doodle. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t hermione_wait_for_target_SP_hook(fbe_object_id_t rg_object_id,
                                              fbe_u32_t state,
                                              fbe_u32_t substate)
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = HERMIONE_WAIT_MSEC*3;
    
    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.object_id = rg_object_id;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.val1 = NULL;
    hook.val2 = NULL;
    hook.counter = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the hook. rg obj id: 0x%X state: %d substate: %d.", 
               rg_object_id, state, substate);

    while (current_time < timeout_ms){

        status = fbe_api_scheduler_get_debug_hook(&hook);

        if (hook.counter != 0) {
            mut_printf(MUT_LOG_TEST_STATUS,
		       "We have hit the debug hook %llu times",
		       (unsigned long long)hook.counter);
            return status;
        }
        current_time = current_time + 200;
        fbe_api_sleep (200);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object %d did not hit state %d substate %d in %d ms!\n", 
                  __FUNCTION__, rg_object_id, state, substate, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end hermione_wait_for_target_SP_hook()
 ******************************************/
 
/*!**************************************************************
 * hermione_delete_debug_hook()
 ****************************************************************
 * @brief
 *  Delete the debug hook for the given state and substate.
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  01/31/2012 - Copied from Doodle. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t hermione_delete_debug_hook(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t state, 
                                                   fbe_u32_t substate) 
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;

    hook.object_id = rg_object_id;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.val1 = NULL;
    hook.val2 = NULL;
    hook.counter = 0;

    status = fbe_api_scheduler_get_debug_hook(&hook);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "The hook was hit %llu times",
	       (unsigned long long)hook.counter);

    mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook: rg obj id 0x%X state %d, substate %d", rg_object_id, state, substate);

    status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                            state,
                            substate,
                            NULL,
                            NULL,
                            SCHEDULER_CHECK_STATES,
                            SCHEDULER_DEBUG_ACTION_PAUSE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return FBE_STATUS_OK;
}
/******************************************
 * end hermione_delete_debug_hook()
 ******************************************/
 
/*!**************************************************************
 * hermione_add_debug_hook()
 ****************************************************************
 * @brief
 *  Add the debug hook for the given state and substate.
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  01/31/2012 - Copied from Doodle. Ashok Tamilarasan
 *
 ****************************************************************/
static void hermione_add_debug_hook(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t state, 
                                                   fbe_u32_t substate) 
{
    fbe_status_t    status;

    mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook: rg obj id 0x%X state %d substate %d", rg_object_id, state, substate);

    status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                            state,
                            substate,
                            NULL,
                            NULL,
                            SCHEDULER_CHECK_STATES,
                            SCHEDULER_DEBUG_ACTION_PAUSE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return ;
}
/******************************************
 * end hermione_add_debug_hook()
 ******************************************/

/*!**************************************************************
 * hermione_wait_for_activate_scheduler_hooks()
 ****************************************************************
 * @brief
 *  This function makes sure we are in the CLUSTERED ACTIVATE state using scheduler hooks.
 *
 * @param in_object_id          - object ID of object to wait for
 *
 * @return void
 * 
 * @author
 * 01/2012   Created.   Susan Rundbaken
 *
 ****************************************************************/
static void hermione_wait_for_activate_scheduler_hooks(fbe_object_id_t  in_object_id)
{
    fbe_status_t    status;

    /* Make sure ACTIVATE condition cleared.
     */
    hermione_add_debug_hook(in_object_id, SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE,
                            FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_CLEARED);

    /* Confirm we are in the ACTIVATE state across available SPs.
     */
    mut_printf(MUT_LOG_HIGH, "%s: confirm RG in ACTIVATE state", __FUNCTION__);
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          in_object_id, FBE_LIFECYCLE_STATE_ACTIVATE,
                                                                          HERMIONE_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);

    hermione_delete_debug_hook(in_object_id, 
                               SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE,
                               FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_REQUEST);

    status = hermione_wait_for_target_SP_hook(in_object_id, 
                                              SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE,
                                              FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_CLEARED);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    hermione_delete_debug_hook(in_object_id, 
                               SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE,
                               FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_CLEARED);

    /* Confirm we get to the "verify paged metadata" condition.
     */
    status = hermione_wait_for_target_SP_hook(in_object_id, 
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                              FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    hermione_delete_debug_hook(in_object_id, 
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                               FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);

    return;
}   
/******************************************
 * end hermione_wait_for_activate_scheduler_hooks()
 ******************************************/

/*!**************************************************************
 *  hermione_confirm_verify_checkpoint_info()
 ****************************************************************
 * @brief
 *  This function confirms the given checkpoint info is set in the RG as expected.
 *
 * @param rg_object_id - object ID of RG
 * @param rg_verify_checkpoint_info - verify checkpoint info to confirm.
 *
 * @return None.
 *
 * @author
 * 01/2012   Created.   Susan Rundbaken
 *
 ****************************************************************/
static void hermione_confirm_verify_checkpoint_info(fbe_object_id_t rg_object_id,
                                                    fbe_raid_group_set_verify_checkpoint_t rg_verify_checkpoint_info)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_raid_group_get_info_t   rg_info;


    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                         FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    switch (rg_verify_checkpoint_info.verify_flags)
    {
        case FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE:
    
            if (rg_verify_checkpoint_info.verify_checkpoint != rg_info.rw_verify_checkpoint)
            {
                MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Failed to set read-write verify checkpoint");
            }
            break;

        case FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY:
    
            if (rg_verify_checkpoint_info.verify_checkpoint != rg_info.ro_verify_checkpoint)
            {
                MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Failed to set read-only verify checkpoint");
            }
            break;

        case FBE_RAID_BITMAP_VERIFY_ERROR:
    
            if (rg_verify_checkpoint_info.verify_checkpoint != rg_info.error_verify_checkpoint)
            {
                MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Failed to set error verify checkpoint");
            }
            break;

        default:
            mut_printf(MUT_LOG_HIGH, "%s: invalid verify flag: 0x%x", __FUNCTION__, rg_verify_checkpoint_info.verify_flags);
            MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Invalid verify operation");
    }

    return;
}
/******************************************
 * end hermione_confirm_verify_checkpoint_info()
 ******************************************/

/*!**************************************************************
 *  hermione_disable_all_bg_services_on_peer()
 ****************************************************************
 * @brief
 *  This function disabled all background services on the peer SP.
 *
 * @param  None.
 *
 * @return None.
 *
 * @author
 * 01/2012   Created.   Susan Rundbaken
 *
 ****************************************************************/
static void hermione_disable_all_bg_services_on_peer(void)
{
    fbe_sim_transport_connection_target_t   local_sp;               
    fbe_sim_transport_connection_target_t   peer_sp;                


    /*  Get the ID of the local SP. 
     */
    local_sp = fbe_api_sim_transport_get_target_server();

    /*  Get the ID of the peer SP.
     */ 
    if (FBE_SIM_SP_A == local_sp)
    {
        peer_sp = FBE_SIM_SP_B;
    }
    else
    {
        peer_sp = FBE_SIM_SP_A;
    }

    /*  Set the target server to the peer and disable BGS there.
     */ 
    fbe_api_sim_transport_set_target_server(peer_sp);

    mut_printf(MUT_LOG_HIGH, "%s: disable all background services on peer", __FUNCTION__);
    fbe_api_base_config_disable_all_bg_services();

    /*  Set the target server back to the local SP.
     */
    fbe_api_sim_transport_set_target_server(local_sp);

    return;
}
/******************************************
 * end hermione_disable_all_bg_services_on_peer()
 ******************************************/

/*!**************************************************************
 *  hermione_enable_all_bg_services_on_peer()
 ****************************************************************
 * @brief
 *  This function enables all background services on the peer SP.
 *
 * @param  None.
 *
 * @return None.
 *
 * @author
 * 01/2012   Created.   Susan Rundbaken
 *
 ****************************************************************/
static void hermione_enable_all_bg_services_on_peer(void)
{
    fbe_sim_transport_connection_target_t   local_sp;               
    fbe_sim_transport_connection_target_t   peer_sp;                


    /*  Get the ID of the local SP. 
     */
    local_sp = fbe_api_sim_transport_get_target_server();

    /*  Get the ID of the peer SP.
     */ 
    if (FBE_SIM_SP_A == local_sp)
    {
        peer_sp = FBE_SIM_SP_B;
    }
    else
    {
        peer_sp = FBE_SIM_SP_A;
    }

    /*  Set the target server to the peer and enable BGS there.
     */ 
    fbe_api_sim_transport_set_target_server(peer_sp);

    mut_printf(MUT_LOG_HIGH, "%s: enable all background services on peer", __FUNCTION__);
    fbe_api_base_config_enable_all_bg_services();

    /*  Set the target server back to the local SP.
     */
    fbe_api_sim_transport_set_target_server(local_sp);

    return;
}
/******************************************
 * end hermione_enable_all_bg_services_on_peer()
 ******************************************/

static fbe_status_t hermione_enable_selected_bg_services_on_peer(fbe_base_config_control_system_bg_service_flags_t flags)
{
    fbe_sim_transport_connection_target_t   local_sp;               
    fbe_sim_transport_connection_target_t   peer_sp;                
    fbe_status_t                                        status;
    fbe_base_config_control_system_bg_service_t			bg_service_info;

    /*  Get the ID of the local SP. 
     */
    local_sp = fbe_api_sim_transport_get_target_server();

    /*  Get the ID of the peer SP.
     */ 
    if (FBE_SIM_SP_A == local_sp)
    {
        peer_sp = FBE_SIM_SP_B;
    }
    else
    {
        peer_sp = FBE_SIM_SP_A;
    }

    /*  Set the target server to the peer and enable BGS there.
     */ 
    fbe_api_sim_transport_set_target_server(peer_sp);

    mut_printf(MUT_LOG_HIGH, "%s: enable selected background services on peer", __FUNCTION__);


    bg_service_info.enable = FBE_TRUE;
    bg_service_info.bgs_flag = flags;
    bg_service_info.issued_by_ndu = FBE_FALSE;

    status = fbe_api_control_system_bg_service(&bg_service_info);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to enable selected system background services.\n", __FUNCTION__);
    }


    /*  Set the target server back to the local SP.
     */
    fbe_api_sim_transport_set_target_server(local_sp);

    return(status);
}

/*!**************************************************************
 *  hermione_clear_paged_md_cache_on_peer()
 ****************************************************************
 * @brief
 *  This function clears the paged metadata cache on the peer for the
 *  specified object.
 *
 * @param  object_id - object ID of object to clear paged md cache
 *
 * @return None.
 *
 * @author
 * 01/2012   Created.   Susan Rundbaken
 *
 ****************************************************************/
static void hermione_clear_paged_md_cache_on_peer(fbe_object_id_t object_id)
{
    fbe_sim_transport_connection_target_t   local_sp;               
    fbe_sim_transport_connection_target_t   peer_sp;  
    fbe_status_t status;              


    /*  Get the ID of the local SP. 
     */
    local_sp = fbe_api_sim_transport_get_target_server();

    /*  Get the ID of the peer SP.
     */ 
    if (FBE_SIM_SP_A == local_sp)
    {
        peer_sp = FBE_SIM_SP_B;
    }
    else
    {
        peer_sp = FBE_SIM_SP_A;
    }

    /*  Set the target server to the peer and clear the cache there.
     */ 
    fbe_api_sim_transport_set_target_server(peer_sp);

    status = fbe_api_base_config_metadata_paged_clear_cache(object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Set the target server back to the local SP.
     */
    fbe_api_sim_transport_set_target_server(local_sp);

    return;
}
/******************************************
 * end hermione_clear_paged_md_cache_on_peer()
 ******************************************/

/*!**************************************************************
 * hermione_wait_for_lun_verify_completion()
 ****************************************************************
 * @brief
 *  This function waits for the user verify operation to complete
 *  on the specified LUN.
 *
 * @param in_object_id          - pointer to LUN object
 * @param in_verify_report_p    - pointer to verify report for LUN
 * @param in_pass_count         - expected pass count (number of times LUN verified)
 *
 * @return void
 * 
 * @author
 * 01/2012   Created from clifford_wait_for_lun_verify_completion().   Susan Rundbaken
 *
 ****************************************************************/
static void hermione_wait_for_lun_verify_completion(fbe_object_id_t            in_object_id,
                                                    fbe_lun_verify_report_t*   in_verify_report_p,
                                                    fbe_u32_t                  in_pass_count)
{
    fbe_u32_t                   sleep_time;


    for (sleep_time = 0; sleep_time < HERMIONE_WAIT_MSEC*10; sleep_time++)
    {
        // Get the verify report for this LUN.
        fbe_test_sep_util_lun_get_verify_report(in_object_id, in_verify_report_p);

        if(in_verify_report_p->pass_count == in_pass_count)
        {
            return;
        }

        // Sleep 100 miliseconds
        fbe_api_sleep(100);
    }

    //  The verify operation took too long and timed out.
    MUT_ASSERT_TRUE(0)
    return;

}   
/******************************************
 * end hermione_wait_for_lun_verify_completion()
 ******************************************/

/*!**************************************************************
 * hermione_write_background_pattern()
 ****************************************************************
 * @brief
 *  Write a background pattern to the LUNs in the given RG.
 * 
 * @param in_rg_config_p - rg test configuration.
 *
 * @return None.
 *
 * @author
 * 2/2012   Created.   Susan Rundbaken
 *
 ****************************************************************/
static void hermione_write_background_pattern(fbe_test_rg_configuration_t * in_rg_config_p)
{
    fbe_api_rdgen_context_t *context_p = &hermione_rdgen_test_contexts[0];
    fbe_status_t            status;
    fbe_u32_t               lun_index;
    fbe_object_id_t         lun_object_id;
    fbe_test_logical_unit_configuration_t*      logical_unit_configuration_p = NULL;


    /* Fill each LUN in the raid group with a fixed pattern
     */
    logical_unit_configuration_p = in_rg_config_p->logical_unit_configuration_list;
    for (lun_index = 0; lun_index < HERMIONE_LUNS_PER_RAID_GROUP; lun_index++)
    {
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        mut_printf(MUT_LOG_TEST_STATUS, "Writing background pattern to LUN 0x%x", lun_object_id);

        fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                                      lun_object_id,
                                                      FBE_CLASS_ID_INVALID,
                                                      FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                      FBE_LBA_INVALID, /* use max capacity */
                                                      HERMIONE_LUN_ELEMENT_SIZE);

        /* Goto next LUN.
         */
        context_p++;
        logical_unit_configuration_p++;
    }

    /* Run our I/O test to write the background pattern.
     */
    context_p = &hermione_rdgen_test_contexts[0];
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, HERMIONE_LUNS_PER_RAID_GROUP);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.io_count, 0);

    return;
}
/******************************************
 * end hermione_write_background_pattern()
 ******************************************/

/*!**************************************************************
 * hermione_check_background_pattern()
 ****************************************************************
 * @brief
 *  Checks a previously written background pattern for the LUNs in the given RG.
 * 
 * @param in_rg_config_p - rg test configuration.
 *
 * @return None.
 *
 * @author
 * 2/2012   Created.   Susan Rundbaken
 *
 ****************************************************************/
static void hermione_check_background_pattern(fbe_test_rg_configuration_t * in_rg_config_p)
{
    fbe_api_rdgen_context_t *context_p = &hermione_rdgen_test_contexts[0];
    fbe_status_t            status;
    fbe_u32_t               lun_index;
    fbe_object_id_t         lun_object_id;
    fbe_test_logical_unit_configuration_t*      logical_unit_configuration_p = NULL;


    /* Read back a pattern and check it was written OK.
     */
    logical_unit_configuration_p = in_rg_config_p->logical_unit_configuration_list;
    for (lun_index = 0; lun_index < HERMIONE_LUNS_PER_RAID_GROUP; lun_index++)
    {
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        mut_printf(MUT_LOG_TEST_STATUS, "Checking background pattern to LUN 0x%x", lun_object_id);

        fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                                      lun_object_id,
                                                      FBE_CLASS_ID_INVALID,
                                                      FBE_RDGEN_OPERATION_READ_CHECK,
                                                      FBE_LBA_INVALID, /* use max capacity */
                                                      HERMIONE_LUN_ELEMENT_SIZE);

        /* Goto next LUN.
         */
        context_p++;
        logical_unit_configuration_p++;
    }

    /* Run our I/O test to check the background pattern written.
     */
    context_p = &hermione_rdgen_test_contexts[0];
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, HERMIONE_LUNS_PER_RAID_GROUP);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.io_count, 0);

    return;
}
/******************************************
 * end hermione_check_background_pattern()
 ******************************************/

/*!**************************************************************
 * hermione_inject_error_record()
 ****************************************************************
 * @brief
 *  Injects an error record for the specified RG.
 * 
 * @param rg_object_id      - object ID of RG
 * @param position_bitmap   - disk position(s) in RG to inject error
 * @param width             - width of RG
 * @param lba               - LBA on disk to inject error
 * @param num_blocks        - number of blocks to extend error
 * @param error_type        - type of error
 *
 * @return None.
 *
 * @author
 * 2/2012   Created.   Susan Rundbaken
 *
 ****************************************************************/
static void hermione_inject_error_record(
                                fbe_object_id_t                             rg_object_id,
                                fbe_u16_t                                   position_bitmap,
                                fbe_u16_t                                   rg_width,
                                fbe_lba_t                                   lba,
                                fbe_block_count_t                           num_blocks,
                                fbe_u32_t                                   op_code,
                                fbe_api_logical_error_injection_type_t      error_type)
{
    fbe_status_t                                status;
    fbe_api_logical_error_injection_record_t    error_record;
    fbe_api_logical_error_injection_get_stats_t stats;

    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Disable error injection records in the error injection table (initializes records).
     */
    status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Setup error record for error injection.
     *  Inject error on given disk position and LBA.
     */
    error_record.pos_bitmap = position_bitmap;                  /*  Injecting error on given disk position */
    error_record.width      = rg_width;                         /*  RG width */
    error_record.lba        = lba;                              /*  Physical address to begin errors */
    error_record.blocks     = num_blocks;                       /*  Blocks to extend errors */
    error_record.err_type   = error_type;                       /* Error type */
    error_record.err_mode   = FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS;  /* Error mode */
    error_record.err_count  = 0;                                /*  Errors injected so far */
    error_record.err_limit  = rg_width;                         /*  Limit of errors to inject */
    error_record.skip_count = 0;                                /*  Limit of errors to skip */
    error_record.skip_limit = 0;                                /*  Limit of errors to inject */
    error_record.err_adj    = 0;                                /*  Error adjacency bitmask */
    error_record.start_bit  = 0;                                /*  Starting bit of an error */
    error_record.num_bits   = 0;                                /*  Number of bits of an error */
    error_record.bit_adj    = 0;                                /*  Indicates if erroneous bits adjacent */
    error_record.crc_det    = 0;                                /*  Indicates if CRC detectable */
    error_record.opcode     = op_code;                          /*  Opcode for which errors shoule be injected. */

    /*  Create error injection record 
     */
    status = fbe_api_logical_error_injection_create_record(&error_record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Enable the error injection service
     */
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Enable error injection on the RG.
     */
    status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Check error injection stats
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 1);

    return;
}
/******************************************
 * end hermione_inject_error_record()
 ******************************************/

/*!**************************************************************
 * hermione_wait_for_error_injection_complete()
 ****************************************************************
 *
 * @brief   Wait for the error injection to complete
 * 
 * @param   rg_config_p      - pointer to RG to use for test
 * @param   position_bitmap  - disk position(s) in RG to wait for error
 * @param   err_record_cnt   - number of errors records to wait for
 *
 * @return  None
 *
 * @author
 * 2/2012   Created.   Susan Rundbaken
 *
 ****************************************************************/
static void hermione_wait_for_error_injection_complete(
                                fbe_test_rg_configuration_t * rg_config_p,
                                fbe_u16_t position_bitmap,
                                fbe_u16_t err_record_cnt)
{
    fbe_status_t                                        status;
    fbe_u32_t                                           sleep_count;
    fbe_u32_t                                           sleep_period_ms;
    fbe_u32_t                                           max_sleep_count;
    fbe_api_logical_error_injection_get_stats_t         stats; 
    fbe_object_id_t                                     rg_object_id;


    /*  Intialize local variables.
     */
    sleep_period_ms = 100;
    max_sleep_count = HERMIONE_WAIT_MSEC / sleep_period_ms;

    /*  Wait for errors to be injected on the RG's PVD objects.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all errors to be injected(detected) ==\n", __FUNCTION__);

    for (sleep_count = 0; sleep_count < max_sleep_count; sleep_count++)
    {
        status = fbe_api_logical_error_injection_get_stats(&stats);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if(stats.num_errors_injected >= err_record_cnt)
        {
            /* Break out of the loop.
             */
            mut_printf(MUT_LOG_TEST_STATUS,
		       "== %s found %llu errors injected ==\n", __FUNCTION__,
		       (unsigned long long)stats.num_errors_injected);
            break;
        }
                        
        fbe_api_sleep(sleep_period_ms);
    }

    /*  The error injection took too long and timed out.
     */
    if (sleep_count >= max_sleep_count)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection failed ==\n", __FUNCTION__);
        MUT_ASSERT_TRUE(sleep_count < max_sleep_count);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection successful ==\n", __FUNCTION__);

    /* Disable error injection on the RG.
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Disable error injection: on RG ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* Disable the error injection service.
         */
        status = fbe_api_logical_error_injection_disable();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
} 
/******************************************
 * end hermione_wait_for_error_injection_complete()
 ******************************************/

/*!**************************************************************
 * hermione_reboot_sp()
 ****************************************************************
 *
 * @brief   Reboot the SP.
 * 
 * @param   rg_object_id - the rg object id to wait for on reboot
 * @param   state - the state in the monitor for scheduler hook
 * @param   substate - the substate in the monitor for scheduler hook
 *
 * @return  None
 *
 * @author
 * 2/2012   Created.   Susan Rundbaken
 *
 ****************************************************************/
static void hermione_reboot_sp(fbe_object_id_t rg_object_id,
                               fbe_u32_t state, 
                               fbe_u32_t substate)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sep_package_load_params_t sep_params_p;
    
    mut_printf(MUT_LOG_TEST_STATUS, "\n");
    mut_printf(MUT_LOG_HIGH, "%s: entry\n", __FUNCTION__);

    fbe_zero_memory(&sep_params_p, sizeof(fbe_sep_package_load_params_t));

    /* Initialize the scheduler hook
     */
    sep_params_p.scheduler_hooks[0].object_id = rg_object_id;
    sep_params_p.scheduler_hooks[0].monitor_state = state;
    sep_params_p.scheduler_hooks[0].monitor_substate = substate;
    sep_params_p.scheduler_hooks[0].check_type = SCHEDULER_CHECK_STATES;
    sep_params_p.scheduler_hooks[0].action = SCHEDULER_DEBUG_ACTION_PAUSE;
    sep_params_p.scheduler_hooks[0].val1 = NULL;
    sep_params_p.scheduler_hooks[0].val2 = NULL;

    /* This will be our signal to stop
     */
    sep_params_p.scheduler_hooks[1].object_id = FBE_OBJECT_ID_INVALID;

    /* Reboot SEP.
     * When load sep and neit packages, do so with preset debug hook.  
     */
    if (fbe_test_sep_util_get_encryption_test_mode()) {
        fbe_test_sep_util_destroy_kms();
    }
    fbe_test_sep_util_destroy_esp_neit_sep();
    sep_config_load_esp_sep_and_neit_params(&sep_params_p, NULL);
    status = fbe_test_sep_util_wait_for_database_service(HERMIONE_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/******************************************
 * end hermione_reboot_sp()
 ******************************************/


/*!**************************************************************
 *  hermione_corrupt_pmd_region()
 ****************************************************************
 * @brief
 *  This function corrupts specified paged metadata region on specified disk.
 *
 * @param rg_config_p - pointer to rg to use for test.
 * @param disk_position - disk position to corrupt.
 * @param offset_lba - offset from start of metadata region.
 * @param lba_count - number of blocks to corrupt.
 *
 * @return None.
 *
 * @author
 * 03/13/2012   Created.   Prahlad Purohit
 *
 ****************************************************************/
static void hermione_run_metadata_err_verify_test(fbe_test_rg_configuration_t * rg_config_p,
                                                  hermione_lei_test_params_t * lei_test_case_p,
                                                  fbe_bool_t reboot_b)
{

    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_raid_group_get_info_t   rg_info;
    fbe_raid_group_set_rb_checkpoint_t rg_rb_checkpoint_info;
    fbe_scheduler_debug_hook_t hook;
    fbe_sim_transport_connection_target_t active_target;
    fbe_bool_t b_run_on_dualsp = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_test_logical_unit_configuration_t*  logical_unit_configuration_p = NULL;
    fbe_u32_t                               lun_index;
    fbe_object_id_t                         lun_object_id = FBE_OBJECT_ID_INVALID;    

    mut_printf(MUT_LOG_TEST_STATUS, "\n");
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s entry; reboot_b: %d ****\n", __FUNCTION__, reboot_b);

    /* For dual-SP, make sure we are the active side.
     * All work is done on the active SP.
     */
    if (b_run_on_dualsp)
    {
        fbe_test_sep_get_active_target_id(&active_target);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_target);

        fbe_api_sim_transport_set_target_server(active_target);
    }

    /* Write a background pattern to the LUNs in the RG.
     */
    hermione_write_background_pattern(rg_config_p);

    /* Make sure the background pattern was written okay.
     */
    hermione_check_background_pattern(rg_config_p);

    /* Clear all debug hooks.
     */
    fbe_api_scheduler_clear_all_debug_hooks(&hook);

    /* Get the RG object id.
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    /* Disable all background services.
     */
    mut_printf(MUT_LOG_HIGH, "%s: disable all background servces", __FUNCTION__);
    fbe_api_base_config_disable_all_bg_services();

    /* For dual-sp mode, do this on the peer as well.
     */
    if (b_run_on_dualsp)
    {
        hermione_disable_all_bg_services_on_peer();
    }
        
    /* Corrupt the RG's paged metadata on one disk so when we rebuild user region 
     * it hits error verify.
     */
    // hermione_corrupt_pmd_region(rg_config_p, 0, 0, 0x800);
    hermione_corrupt_paged_metadata(rg_config_p, lei_test_case_p, FALSE);

    /* Set RG rebuild checkpoint to a 0 so we will try to do a rebuild
     * which will read paged metadata that we have corrupted.
     */
    rg_rb_checkpoint_info.position = 0;
    rg_rb_checkpoint_info.rebuild_checkpoint = 0;
    mut_printf(MUT_LOG_HIGH, "%s: set rebuild checkpoint 0x%x for position 0x%x", 
               __FUNCTION__, 
               (unsigned int)rg_rb_checkpoint_info.rebuild_checkpoint,
               rg_rb_checkpoint_info.position);
    status = fbe_api_raid_group_set_rebuild_checkpoint(rg_object_id, rg_rb_checkpoint_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Confirm the rebuild checkpoint is set as expected.
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                         FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (rg_rb_checkpoint_info.rebuild_checkpoint != rg_info.rebuild_checkpoint[rg_rb_checkpoint_info.position])
    {
        MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Failed to set rebuild checkpoint");
    }

     /* Enable all background services.
      * This will allow the rebuild to run.
      */
     mut_printf(MUT_LOG_HIGH, "%s: enable all background services", __FUNCTION__);
     fbe_api_base_config_enable_all_bg_services();
        
    /* For dual-sp mode, do this on the peer as well.
     */
    if (b_run_on_dualsp)
    {
        hermione_enable_all_bg_services_on_peer();
    }

    /* Make sure errors were injected okay.
     */
    hermione_wait_for_error_injection_complete(rg_config_p,
                                               lei_test_case_p->position_bitmap,
                                               2    /* number of LEI records to wait for */);
    
    /* Disable the error injection service.
     */
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    

    
    /* Confirm rebuild was successful.
     * Wait for rebuild to be complete.
     */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, rg_rb_checkpoint_info.position);

    /* Confirm LUNs in READY state.
     */
    logical_unit_configuration_p = rg_config_p->logical_unit_configuration_list;
    for (lun_index = 0; lun_index < HERMIONE_LUNS_PER_RAID_GROUP; lun_index++)
    {
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        mut_printf(MUT_LOG_HIGH, "%s: confirm LUN 0x%x in READY state", __FUNCTION__, lun_object_id);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                              HERMIONE_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);

        /* Goto next LUN.
         */
        logical_unit_configuration_p++;
    }    
}


/*!**************************************************************
 *  hermione_corrupt_pmd_region()
 ****************************************************************
 * @brief
 *  This function corrupts specified paged metadata region on specified disk.
 *
 * @param rg_config_p - pointer to rg to use for test.
 * @param disk_position - disk position to corrupt.
 * @param offset_lba - offset from start of metadata region.
 * @param lba_count - number of blocks to corrupt.
 *
 * @return None.
 *
 * @author
 * 03/13/2012   Created.   Prahlad Purohit
 *
 ****************************************************************/
static void hermione_corrupt_pmd_region(fbe_test_rg_configuration_t *rg_config_p,
                                        fbe_u32_t                   disk_position,
                                        fbe_u32_t                   offset_lba,
                                        fbe_u32_t                   lba_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_raid_group_get_info_t   rg_info;
    fbe_lba_t                       start_lba;
    fbe_object_id_t                 pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                      b_run_on_dualsp = fbe_test_sep_util_get_dualsp_test_mode();    

    /* Get the RG info.
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                         FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the RG PVD object ID.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(
                 rg_config_p->rg_disk_set[disk_position].bus,
                 rg_config_p->rg_disk_set[disk_position].enclosure,
                 rg_config_p->rg_disk_set[disk_position].slot,
                 &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    start_lba = rg_info.paged_metadata_start_lba + offset_lba;

    mut_printf(MUT_LOG_HIGH, "%s: pvd 0x%x: will corrupt paged metadata at lba 0x%x",
               __FUNCTION__,
               pvd_object_id,
               (unsigned int)start_lba);

    /* Corrupt RG paged metadata on the PVD.
     */
    hermione_test_run_io(pvd_object_id, 
                         start_lba,
                         lba_count,
                         FBE_METADATA_BLOCK_SIZE,
                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                         FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA);
    
    /* Ensure the paged metadata memory cache is cleared on RG so will be forced to get data from disk.
     */
    status = fbe_api_base_config_metadata_paged_clear_cache(rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Clear cache on peer as well, if available.
     */
    if (b_run_on_dualsp)
    {
        hermione_clear_paged_md_cache_on_peer(rg_object_id);
    }    

    return;
}
/******************************************
 * end hermione_corrupt_pmd_region()
 ******************************************/

void hermione_shutdown_peer_sp(void) 
{
    fbe_sim_transport_connection_target_t   target_sp;
    fbe_sim_transport_connection_target_t   peer_sp;
    fbe_status_t status = FBE_STATUS_OK;

    /* Get the local SP ID */
    target_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, target_sp);

    /* set the passive SP */
    peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == SHUTDOWN PEER SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(peer_sp);
    fbe_test_sp_sim_stop_single_sp(peer_sp == FBE_SIM_SP_A? FBE_TEST_SPA: FBE_TEST_SPB);
    /* we can't destroy fbe_api, because it's shared between two SPs */
    fbe_test_base_suite_startup_single_sp(peer_sp);

    status = fbe_api_sim_transport_set_target_server(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}


void hermione_startup_peer_sp(void) 
{
    fbe_sim_transport_connection_target_t   target_sp;
    fbe_sim_transport_connection_target_t   peer_sp;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t num_raid_groups = 0;

    rg_config_p = &hermione_raid_group_config_qual[0];
    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);    

    /* Get the local SP ID */
    target_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, target_sp);

    /* set the passive SP */
    peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == STARTUP SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* Load the physical config on the target server */    
    elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
    elmo_load_esp_sep_and_neit();

    status = fbe_api_sim_transport_set_target_server(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

}


void hermione_reboot_peer_sp(void) 
{
    mut_printf(MUT_LOG_LOW, " == REBOOT PEER SP == ");
    hermione_shutdown_peer_sp();
    fbe_api_sleep(2000);
    hermione_startup_peer_sp();
}

static fbe_status_t hermione_wait_for_target_hook(fbe_scheduler_debug_hook_t *hook)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       current_time = 0;
    fbe_u32_t       target_wait_msec = HERMIONE_WAIT_MSEC;

    while (current_time < target_wait_msec )
    {

        status = fbe_api_scheduler_get_debug_hook(hook);

        if (hook->counter != 0) 
        {
            mut_printf(MUT_LOG_TEST_STATUS, "We have hit the debug hook %d times", (int)hook->counter);
            return status;
        }

        current_time = current_time + 200;
        fbe_api_sleep (200);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object %d did not hit state %d substate %d in %d ms!\n", 
                  __FUNCTION__, hook->object_id, hook->monitor_state, hook->monitor_substate, HERMIONE_WAIT_MSEC);

    return FBE_STATUS_GENERIC_FAILURE;
}


static fbe_status_t hermione_wait_for_verify_checkpoint_hook(fbe_object_id_t raid_group_object_id, fbe_lba_t checkpoint)
{
    fbe_scheduler_debug_hook_t hook;
    fbe_status_t status;

    hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY;
    hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RO_START;
    hook.val1 = checkpoint;
    hook.val2 = NULL;
    hook.check_type = SCHEDULER_CHECK_VALS_EQUAL;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.counter = 0;
    hook.object_id = raid_group_object_id;

    status = hermione_wait_for_target_hook(&hook);
    return status;
}

static void hermione_inject_retriable_write_error(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t position, fbe_protocol_error_injection_record_handle_t * out_record_handle_p)
{
    fbe_protocol_error_injection_error_record_t     record;                 // error injection record 
    fbe_object_id_t                                 object_id;
    
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_object_id_t                                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_raid_group_get_info_t                   rg_info;
    fbe_lba_t                                       start_lba = FBE_LBA_INVALID;
    fbe_lba_t                                       lba_count = FBE_LBA_INVALID;
    fbe_u32_t                                       i = 0;

    /* Get the RG info.
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                         FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
   
    start_lba = rg_info.physical_offset + (rg_info.paged_metadata_start_lba / rg_info.num_data_disk);

    lba_count = rg_info.paged_metadata_capacity / rg_info.num_data_disk;
    
    fbe_test_neit_utils_init_error_injection_record(&record);        

    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[position].bus,
                                                              rg_config_p->rg_disk_set[position].enclosure,
                                                              rg_config_p->rg_disk_set[position].slot,
                                                              &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    record.object_id                = object_id; 
    record.lba_start                = start_lba;
    record.lba_end                  = start_lba + lba_count - 1;
    record.num_of_times_to_insert   = FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE;

    record.protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = FBE_SCSI_WRITE;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[1] = FBE_SCSI_WRITE_6;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[2] = FBE_SCSI_WRITE_10;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[3] = FBE_SCSI_WRITE_12;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[4] = FBE_SCSI_WRITE_16;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[5] = FBE_SCSI_WRITE_VERIFY;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[6] = FBE_SCSI_WRITE_VERIFY_10;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[7] = FBE_SCSI_WRITE_VERIFY_16;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = FBE_SCSI_SENSE_KEY_ABORTED_CMD;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_GENERAL_QUALIFIER;
    for (i = 0; i < PROTOCOL_ERROR_INJECTION_MAX_ERRORS ; i++) {
        if (i <= (record.lba_end - record.lba_start)) {
            record.b_active[i] = FBE_TRUE;
        }
    } 

    //  Add the error injection record to the service, which also returns the record handle for later use
    status = fbe_api_protocol_error_injection_add_record(&record, out_record_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    //  Start the error injection 
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
}


/*!**************************************************************
 *  hermione_run_metadata_reconstruction_test()
 ****************************************************************
 * @brief
 *  This function runs metdata reconstruction test.
 *
 * @param rg_config_p - pointer to rg to use for test.
 * @param lei_test_case - logical error injection parameters for test.
 *
 * @return None.
 *
 * @author
 * 03/13/2012   Created.   Prahlad Purohit
 *
 ****************************************************************/
static void hermione_run_metadata_reconstruction_test(fbe_test_rg_configuration_t * rg_config_p)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_scheduler_debug_hook_t              hook;
    fbe_sim_transport_connection_target_t   active_target;
    fbe_bool_t b_run_on_dualsp = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_test_logical_unit_configuration_t*  logical_unit_configuration_p = NULL;
    fbe_u32_t                               lun_index;
    fbe_object_id_t                         lun_object_id = FBE_OBJECT_ID_INVALID;    
    fbe_protocol_error_injection_record_handle_t    out_record_handle;
    fbe_protocol_error_injection_error_record_t     record;

    mut_printf(MUT_LOG_TEST_STATUS, "\n");
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s entry; ****\n", __FUNCTION__);

    /* For dual-SP, make sure we are the active side.
     * All work is done on the active SP.
     */
    if (b_run_on_dualsp)
    {
        fbe_test_sep_get_active_target_id(&active_target);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_target);
    
        fbe_api_sim_transport_set_target_server(active_target);
    }
    
    /* Clear all debug hooks.
     */
    fbe_api_scheduler_clear_all_debug_hooks(&hook);
    
    /* Get the RG object id.
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    /* Add a debug hook so we confirm we mark the metdata for reconstruction.
     */
    status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                            SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                            FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_NEEDS_REBUILD,
                            NULL,
                            NULL,
                            SCHEDULER_CHECK_STATES,
                            SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    

    /* Disable all background services.
     */
    mut_printf(MUT_LOG_HIGH, "%s: disable all background servces", __FUNCTION__);
    fbe_api_base_config_disable_all_bg_services();
    
    /* For dual-sp mode, do this on the peer as well.
     */
    if (b_run_on_dualsp)
    {
        hermione_disable_all_bg_services_on_peer();
    }

    /* Get first LUN object ID.
     */
    logical_unit_configuration_p = rg_config_p->logical_unit_configuration_list;
    fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);


    /* Initiate verify on first LUN.
     */
    status = fbe_api_lun_initiate_verify(lun_object_id,
                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                         FBE_LUN_VERIFY_TYPE_USER_RO,
                                         FBE_TRUE,
                                         FBE_LUN_VERIFY_START_LBA_LUN_START,
                                         FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* Inject errors on metadata write operations.
     */
    hermione_inject_retriable_write_error(rg_config_p, 3, &out_record_handle);

    mut_printf(MUT_LOG_HIGH, "%s: enable all background services", __FUNCTION__);
    fbe_api_base_config_enable_all_bg_services();
    if (b_run_on_dualsp)
    {
        hermione_enable_all_bg_services_on_peer();
    }

    /* Wait for reconstruct mark condition to hit.
     */      
    status = fbe_test_wait_for_debug_hook(rg_object_id,
                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                         FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_NEEDS_REBUILD,
                                         SCHEDULER_CHECK_STATES,
                                         SCHEDULER_DEBUG_ACTION_CONTINUE,
                                         NULL, NULL);
    if(status != FBE_STATUS_OK)
    {
        status = fbe_api_protocol_error_injection_get_record(out_record_handle, &record);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "Protocol Error was injected %d times\n", record.times_inserted);
        CSX_BREAK();
        MUT_FAIL_MSG("Debug hook SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL was not hit");
    }

    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Confirm LUNs in READY state.
     */
    for (lun_index = 0; lun_index < HERMIONE_LUNS_PER_RAID_GROUP; lun_index++)
    {
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
    
        mut_printf(MUT_LOG_HIGH, "%s: confirm LUN 0x%x in READY state", __FUNCTION__, lun_object_id);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                              HERMIONE_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        /* Goto next LUN.
         */
        logical_unit_configuration_p++;
    }

    return;
}


/*!**************************************************************
 *  hermione_run_peer_death_metadata_reconstruction_test()
 ****************************************************************
 * @brief
 *  This function runs metdata reconstruction test for peer death scenario.
 *
 * @param rg_config_p - pointer to rg to use for test.
 *
 * @return None.
 *
 * @note Applicable only for dual SP scenario.
 *
 * @author
 * 03/13/2012   Created.   Prahlad Purohit
 *
 ****************************************************************/
static void hermione_run_peer_death_metadata_reconstruction_test(fbe_test_rg_configuration_t * rg_config_p)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_scheduler_debug_hook_t              hook;
    fbe_sim_transport_connection_target_t   active_target;
    fbe_test_logical_unit_configuration_t*  logical_unit_configuration_p = NULL;
    fbe_object_id_t                         lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_sim_transport_connection_target_t   peer_sp;

    mut_printf(MUT_LOG_TEST_STATUS, "\n");
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s entry; ****\n", __FUNCTION__);

    /* Make sure we are the active side. All work is done on the active SP.
     */
    fbe_test_sep_get_active_target_id(&active_target);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_target);

    fbe_api_sim_transport_set_target_server(active_target);

    peer_sp = (FBE_SIM_SP_A == active_target)? FBE_SIM_SP_B : FBE_SIM_SP_A;      

    /* Clear all debug hooks.
     */
    fbe_api_scheduler_clear_all_debug_hooks(&hook);
    
    /* Get the RG object id.
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    /* Add a debug hook to pause while it is verifying the user region.
     */
    status = fbe_api_scheduler_add_debug_hook(rg_object_id, 
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                              FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RO_START, 
                                              0x0,
                                              NULL, 
                                              SCHEDULER_CHECK_VALS_EQUAL, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Set reconstruct condition hook on peer SP.
     */
    fbe_api_sim_transport_set_target_server(peer_sp);

    hermione_add_debug_hook(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_VERIFY,
                            FBE_RAID_GROUP_SUBSTATE_METADATA_MARK_RECONSTRUCT);  

    fbe_api_sim_transport_set_target_server(active_target);    

    /* Disable all background services.
     */
    mut_printf(MUT_LOG_HIGH, "%s: disable all background servces", __FUNCTION__);
    fbe_api_base_config_disable_all_bg_services();
    hermione_disable_all_bg_services_on_peer();

    /* Get first LUN object ID.
     */
    logical_unit_configuration_p = rg_config_p->logical_unit_configuration_list;
    fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

    /* Initiate verify on first LUN.
     */
    status = fbe_api_lun_initiate_verify(lun_object_id,
                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                         FBE_LUN_VERIFY_TYPE_USER_RO,
                                         FBE_TRUE,
                                         FBE_LUN_VERIFY_START_LBA_LUN_START,
                                         FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable all background services.
     * This will allow the verify to run.
     */
    mut_printf(MUT_LOG_HIGH, "%s: enable all background services", __FUNCTION__);
    fbe_api_base_config_enable_all_bg_services();    
    hermione_enable_all_bg_services_on_peer();

    /* Wait for the target SP to start verify and then kill this SP.
     * At this point peer SP will detect paged lock held and it will mark
     * MD for reconstruction.
     */
    status = hermione_wait_for_verify_checkpoint_hook(rg_object_id, 0x0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    hermione_reboot_sp(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE, 
                       FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);
    
//    hermione_reboot_peer_sp();

    fbe_api_sim_transport_set_target_server(peer_sp);

    /* Wait for reconstruct mark condition to hit.
     */
    status = hermione_wait_for_target_SP_hook(rg_object_id, 
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_VERIFY,
                                              FBE_RAID_GROUP_SUBSTATE_METADATA_MARK_RECONSTRUCT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    hermione_delete_debug_hook(rg_object_id, 
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_VERIFY,
                               FBE_RAID_GROUP_SUBSTATE_METADATA_MARK_RECONSTRUCT);

    return;
}


/***************************
 * end file hermione_test.c
 ***************************/


