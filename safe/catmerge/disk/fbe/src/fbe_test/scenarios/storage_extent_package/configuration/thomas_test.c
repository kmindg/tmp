/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file thomas_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test to validate the 4K handling after commit 
 *
 * @author
 *  06/05/2014 - Created. Ashok Tamilarasan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "sep_tests.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_dps_memory_interface.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe_database.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe_test.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_trace_interface.h"
#include "pp_utils.h"
#include "sep_rebuild_utils.h"
#include "ndu_toc_common.h"
#include "sep_rebuild_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * thomas_short_desc = "This scenario will test Journal related updates after 4K commit";
char * thomas_long_desc ="\
The scenario will test Journal related updates after 4K commit.\n\
\n"\
"-raid_test_level 0 and 1\n\
     We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
\n\
\n\
Description last updated: 06/5/2014.\n";

/*!*******************************************************************
 * @def THOMAS_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define THOMAS_TEST_LUNS_PER_RAID_GROUP 2

/*!*******************************************************************
 * @def THOMAS_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define THOMAS_CHUNKS_PER_LUN 15


/*!*******************************************************************
 * @def THOMAS_TEST_LUNS_PER_RAID_GROUP_UNBIND_TEST
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define THOMAS_TEST_LUNS_PER_RAID_GROUP_UNBIND_TEST THOMAS_TEST_LUNS_PER_RAID_GROUP *4

/*!*******************************************************************
 * @def THOMAS_CHUNKS_PER_LUN_UNBIND_TEST
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define THOMAS_CHUNKS_PER_LUN_UNBIND_TEST THOMAS_CHUNKS_PER_LUN / 4

/*!*******************************************************************
 * @def THOMAS_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define THOMAS_SMALL_IO_SIZE_MAX_BLOCKS 1024
/*!*******************************************************************
 * @def THOMAS_LIGHT_THREAD_COUNT
 *********************************************************************
 * @brief Lightly loaded test to allow rekey to proceed.
 *
 *********************************************************************/
#define THOMAS_LIGHT_THREAD_COUNT 1

/*!*******************************************************************
 * @def THOMAS_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define THOMAS_MAX_IO_SIZE_BLOCKS (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE //4*0x400*6

/*!*******************************************************************
 * @var thomas_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t thomas_raid_group_config_qual[] = 
{
    /* width,   capacity  raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            2,          0},
    {6,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            3,          0},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            4,          0},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


static void thomas_test_set_commit_level(fbe_u32_t commit_level);
static void thomas_test_rg_config(fbe_test_rg_configuration_t * const rg_config_p , void * context_p);
static fbe_status_t thomas_test_validate_journal_info(fbe_test_rg_configuration_t * const rg_config_p,
                                                      fbe_u32_t write_log_slot_size, fbe_u32_t write_log_slot_count);
static fbe_status_t thomas_test_validate_journal_for_520(fbe_test_rg_configuration_t * const rg_config_p);
static fbe_status_t thomas_test_validate_journal_for_4k(fbe_test_rg_configuration_t * const rg_config_p);

static void thomas_test_set_commit_level(fbe_u32_t commit_level)
{
    fbe_status_t status;
    database_system_db_header_t db_header;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    
    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    status = fbe_api_database_get_system_db_header(&db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Get system db header failed\n");

    db_header.persisted_sep_version = commit_level;

    mut_printf(MUT_LOG_TEST_STATUS, "---modify and write system db header\n");
    status = fbe_api_database_set_system_db_header(&db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Update system db header failed \n");
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(other_sp);
        mut_printf(MUT_LOG_TEST_STATUS, "---modify and write system db header on SP:%d \n", other_sp);
        status = fbe_api_database_set_system_db_header(&db_header);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Update system db header failed  on peer SP\n");
        fbe_api_sim_transport_set_target_server(this_sp);
    }
}

/*!**************************************************************
 * thomas_test()
 ****************************************************************
 * @brief
 *  Initial setup to run the test. This function also changes the 
 * commit level to pre-4K to validate that the write log entries are
 * setup correctly after "commit"
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  06/08/2014 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
void thomas_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;

    rg_config_p = &thomas_raid_group_config_qual[0];
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL, thomas_test_rg_config,
                                                         THOMAS_TEST_LUNS_PER_RAID_GROUP,
                                                         THOMAS_CHUNKS_PER_LUN,
                                                         FBE_TRUE);
    params.b_encrypted = FBE_FALSE;
    thomas_test_set_commit_level(FLARE_COMMIT_LEVEL_ROCKIES_MR1_CBE);
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    
    return;
}
/******************************************
 * end thomas_test()
 ******************************************/

/*!**************************************************************
 * thomas_dualsp_test()
 ****************************************************************
 * @brief
 *  Initial setup to run the test. This function also changes the 
 * commit level to pre-4K to validate that the write log entries are
 * setup correctly after "commit"
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  06/08/2014 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
void thomas_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;

    rg_config_p = &thomas_raid_group_config_qual[0];

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

   fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL, thomas_test_rg_config,
                                                         THOMAS_TEST_LUNS_PER_RAID_GROUP,
                                                         THOMAS_CHUNKS_PER_LUN,
                                                         FBE_TRUE);
    params.b_encrypted = FBE_FALSE;

    thomas_test_set_commit_level(FLARE_COMMIT_LEVEL_ROCKIES_MR1_CBE);

    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    
    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end thomas_dualsp_test()
 ******************************************/

/*!**************************************************************
 * thomas_setup()
 ****************************************************************
 * @brief
 *  Setup for a Thomas journal write log setup.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *    06/08/2014 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
void thomas_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        rg_config_p = &thomas_raid_group_config_qual[0];

        /* Use simulator PSL so that it does not take forever and a day to zero the system drives.
         */
        fbe_test_set_use_fbe_sim_psl(FBE_TRUE);

         /* We no longer create the raid groups during the setup
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
       // fbe_test_rg_setup_random_block_sizes(rg_config_p);
       // fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        elmo_load_config(rg_config_p, 
                           THOMAS_TEST_LUNS_PER_RAID_GROUP, 
                           THOMAS_CHUNKS_PER_LUN);
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *	      the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_test_disable_background_ops_system_drives();
    
    
    return;
}
/**************************************
 * end thomas_setup()
 **************************************/

/*!**************************************************************
 * thomas_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a Thomas journal write log setup.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *    06/08/2014 - Created. Ashok Tamilarasan
 ****************************************************************/
void thomas_dualsp_setup(void)
{
    fbe_status_t status;
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        rg_config_p = &thomas_raid_group_config_qual[0]; 

        /* Use simulator PSL so that it does not take forever and a day to zero the system drives.
         */
        fbe_test_set_use_fbe_sim_psl(FBE_TRUE);

                 /* We no longer create the raid groups during the setup
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
      //  fbe_test_rg_setup_random_block_sizes(rg_config_p);
       // fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        
        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
        
    }

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *	      the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Disable load balancing so that our read and pin will occur on the same SP that we are running the encryption. 
     * This is needed so that rdgen can do the locking on the same SP where the read and pin is occuring.
     */
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_disable_background_ops_system_drives();

   
    return;
}
/**************************************
 * end thomas_dualsp_setup()
 **************************************/

/*!**************************************************************
 * thomas_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the thomas test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  06/05/2014 - Created. Ashok Tamilarasan
 *
 ****************************************************************/

void thomas_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {		
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end thomas_cleanup()
 ******************************************/

/*!**************************************************************
 * thomas_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the thomas test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  06/05/2014 - Created. Ashok Tamilarasan
 *
 ****************************************************************/

void thomas_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {		
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    return;
}
/******************************************
 * end thomas_dualsp_cleanup()
 ******************************************/
/*!**************************************************************
 * thomas_test_rg_config()
 ****************************************************************
 * @brief
 *  The function validates that write log entries are setup correctly
 * while the RG is degraded, after the RG comes ready and then after
 * the SEP is committed
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  06/05/2014 - Created. Ashok Tamilarasan
 *
 ****************************************************************/

void thomas_test_rg_config(fbe_test_rg_configuration_t * const rg_config_p, void * rg_context_p)
{
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t rg_index;
    fbe_sep_package_load_params_t sep_params = {0,};

    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    mut_printf(MUT_LOG_TEST_STATUS, "Disable Automatic HS...");
    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Unable to turn off HS\n");

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    big_bird_write_background_pattern();

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O. ==", __FUNCTION__);
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, THOMAS_MAX_IO_SIZE_BLOCKS, 
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, FBE_RDGEN_PEER_OPTIONS_INVALID);
    fbe_test_io_wait_for_io_count(context_p, num_luns, 5, FBE_TEST_WAIT_TIMEOUT_MS);
    fbe_api_sim_transport_set_target_server(this_sp);

    current_rg_config_p = rg_config_p;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Pull 1 drive from each of the RGs. ==", __FUNCTION__);
    for (rg_index = 0; rg_index < rg_count; rg_index++){
        /*  Remove one drive from each of the RG to make it degraded */
        fbe_test_pull_drive(current_rg_config_p, 0);
        current_rg_config_p++;
        
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate Journal info for 520. ==", __FUNCTION__);
    /* Validate that journal information is set to 520 */
    thomas_test_validate_journal_for_520(rg_config_p);

    fbe_test_sep_util_validate_no_raid_errors_both_sps();

    /* Change the commit level to one that supports 4K */
    thomas_test_set_commit_level(SEP_PACKAGE_VERSION);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate Journal info for 520. ==", __FUNCTION__);

    /* Validate that journal information does not change until the RG is not degraded */
    thomas_test_validate_journal_for_520(rg_config_p);

    fbe_test_sep_util_validate_no_raid_errors_both_sps();

    /* Reinsert all the drives */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reinsert the drives... ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++){
        /*  Reinsert the drive */
        fbe_test_reinsert_drive(current_rg_config_p, 0);
        current_rg_config_p++;
    }

    /* Wait for Rebuilds to complete */
    current_rg_config_p = rg_config_p;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for Rebuilding to complete. ==", __FUNCTION__);
    for (rg_index = 0; rg_index < rg_count; rg_index++){
        sep_rebuild_utils_wait_for_rb_comp(current_rg_config_p, 0);
        current_rg_config_p++;
    }

    fbe_test_sep_util_validate_no_raid_errors_both_sps();

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate Journal info for 4K. ==", __FUNCTION__);

    /* Validate that the Journal information reflects the fact it has been updated to support 4K */
    thomas_test_validate_journal_for_4k(rg_config_p);

    /* Stop I/O and check for errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s halting I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s  I/O halted ==", __FUNCTION__);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    status = fbe_api_control_automatic_hot_spare(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Reboot the SP and make sure things are set as it should be */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot SPs ==", __FUNCTION__);
    if(fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_test_common_reboot_both_sps();
    } else {
        fbe_test_common_reboot_this_sp(&sep_params, NULL);
    }
    thomas_test_validate_journal_for_4k(rg_config_p);

    return;
}

static fbe_status_t thomas_test_validate_journal_info(fbe_test_rg_configuration_t * const rg_config_p,
                                                      fbe_u32_t write_log_slot_size, fbe_u32_t write_log_slot_count)
{
    fbe_object_id_t             rg_object_id;
    fbe_parity_get_write_log_info_t write_log_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_u32_t time = 0;
    fbe_u8_t sp_count, index;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    //  Get the RG object id from the RG number
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    sp_count = fbe_test_sep_util_get_dualsp_test_mode()? 2 : 1;

    for(index = 0; index < sp_count; index++)
    {
        while (time < 120000)
        {
            fbe_api_raid_group_get_write_log_info(rg_object_id, &write_log_info, FBE_PACKET_FLAG_NO_ATTRIB);
            if ((write_log_slot_size != write_log_info.slot_size) ||
                (write_log_slot_count != write_log_info.slot_count))
            {
                fbe_api_sleep(500);
                time += 500;
            } else  {
                break;
            }
        }
        MUT_ASSERT_INT_EQUAL_MSG(write_log_slot_size, write_log_info.slot_size, "Incorrect Write Log Size\n");
        MUT_ASSERT_INT_EQUAL_MSG(write_log_slot_count, write_log_info.slot_count, "Incorrect Write Log Count\n");
        time = 0;
        fbe_api_sim_transport_set_target_server(other_sp);
    }
    fbe_api_sim_transport_set_target_server(this_sp);
    return FBE_STATUS_OK;
}

static fbe_status_t thomas_test_validate_journal_for_520(fbe_test_rg_configuration_t * const rg_config_p)
{
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_object_id_t             rg_object_id;
    fbe_status_t                status;
    fbe_u32_t   write_log_slot_size;
    fbe_u32_t   write_log_slot_count;
    fbe_api_raid_group_get_info_t raid_info;
    fbe_u32_t rg_index;

    rg_count = fbe_test_get_rg_array_length(rg_config_p);
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++){
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        fbe_api_raid_group_get_info(rg_object_id, &raid_info, FBE_PACKET_FLAG_NO_ATTRIB);
        /* The Journal is always aligned to 4K (4160 - 8 520-bps blocks) */
        if (raid_info.element_size == FBE_RAID_SECTORS_PER_ELEMENT) {
            write_log_slot_size = FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_NORM_4K;
            write_log_slot_count = FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_NORM_4K;
        } else {
            write_log_slot_size = FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_BANDWIDTH_4K;
            write_log_slot_count = FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_BANDWIDTH_4K;
        }
        status = thomas_test_validate_journal_info(current_rg_config_p,
                                                   write_log_slot_size,
                                                   write_log_slot_count);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }
    return FBE_STATUS_OK;

}

static fbe_status_t thomas_test_validate_journal_for_4k(fbe_test_rg_configuration_t * const rg_config_p)
{
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_object_id_t             rg_object_id;
    fbe_status_t                status;
    fbe_u32_t   write_log_slot_size;
    fbe_u32_t   write_log_slot_count;
    fbe_api_raid_group_get_info_t raid_info;
    fbe_u32_t rg_index;

    rg_count = fbe_test_get_rg_array_length(rg_config_p);
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++){
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        fbe_api_raid_group_get_info(rg_object_id, &raid_info, FBE_PACKET_FLAG_NO_ATTRIB);
        if (raid_info.element_size == FBE_RAID_SECTORS_PER_ELEMENT) {
            write_log_slot_size = FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_NORM_4K;
            write_log_slot_count = FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_NORM_4K;
        } else {
            write_log_slot_size = FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_BANDWIDTH_4K;
            write_log_slot_count = FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_BANDWIDTH_4K;
        }
        status = thomas_test_validate_journal_info(current_rg_config_p,
                                                   write_log_slot_size,
                                                   write_log_slot_count);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }
    return status;
}

void fbe_test_pull_drive(fbe_test_rg_configuration_t *rg_config_p,
                         fbe_u32_t position_to_remove)
{
    fbe_u32_t           				number_physical_objects = 0;
    fbe_status_t status;

    /* need to remove a drive here */
	status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
	sep_rebuild_utils_remove_drive_and_verify(rg_config_p, position_to_remove,
                                              number_physical_objects-1, &rg_config_p->drive_handle[position_to_remove]);
}

void fbe_test_reinsert_drive(fbe_test_rg_configuration_t *rg_config_p,
                             fbe_u32_t position_to_insert)
{
    fbe_u32_t number_physical_objects;

    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: raid group id: %d position: %d", 
               __FUNCTION__, rg_config_p->raid_group_id, position_to_insert);

    fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, position_to_insert, 
                                                (number_physical_objects + 1),
                                                &rg_config_p->drive_handle[position_to_insert]);
    return;
}

