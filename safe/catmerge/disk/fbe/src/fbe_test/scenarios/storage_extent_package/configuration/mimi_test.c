/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file mimi_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests cases demos for setting up a dual SP test, 
 *   in this test, we could control each SP to start to bootflash mode or normal mode
 *
 * @version
 *   16/03/2015 - Created. Geng, Han
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_logical_drive_interface.h"
#include "fbe/fbe_api_bvd_interface.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe_test_esp_utils.h"
#include "sep_rebuild_utils.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_api_panic_sp_interface.h"
#include "fbe_test.h"
#include "fbe/fbe_api_sector_trace_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_raid_error.h"

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* mimi_short_desc = "this test tries to demo how to setup a dual sp test, which starts from boot flash mode";
char* mimi_long_desc =
"\n\
        this test tries to demo how to setup a dual sp test, which starts from boot flash mode.\n\
        Description last updated: 16/03/2015.\n";
//------------------------------------------------------------------------------

// luns per rg for the extended test. 
#define MIMI_LUNS_PER_RAID_GROUP 1

// Number of chunks each LUN will occupy.
#define MIMI_CHUNKS_PER_LUN 3


fbe_test_rg_configuration_t mimi_rg_config_g[] = 
{
    //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
    {6, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520, 0, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

static void mimi_test_rg_config_bootflash_mode(void);
static void mimi_test_rg_config_normal_mode(fbe_test_rg_configuration_t *rg_config_p, void * context_p);


fbe_sep_package_load_params_t spa_sep_params = {0};
fbe_sep_package_load_params_t spb_sep_params = {0};

void mimi_dualsp_setup(void)
{  
    if (fbe_test_util_is_simulation())
    { 
        fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(mimi_rg_config_g);
        fbe_test_sep_util_init_rg_configuration_array(mimi_rg_config_g);
        /* initialize the number of extra drive required by each rg which will be use by
           simulation test and hardware test */
        fbe_test_sep_util_populate_rg_num_extra_drives(mimi_rg_config_g);

        /* Instantiate the drives on SP A
        */
        mut_printf(MUT_LOG_LOW, "=== %s Loading Physical Config on SPA ===", __FUNCTION__);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(mimi_rg_config_g, num_raid_groups);

        /*  Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        mut_printf(MUT_LOG_LOW, "=== %s Loading Physical Config on SPB ===", __FUNCTION__);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(mimi_rg_config_g, num_raid_groups);
        
        sep_config_fill_load_params(&spa_sep_params);
        sep_config_fill_load_params(&spb_sep_params);

        /* you can take various flag combinations here to start each SP to the mode you expect */
        spa_sep_params.flags |= FBE_SEP_LOAD_FLAG_BOOTFLASH_MODE;
        sep_config_load_sep_and_neit_load_balance_both_sps_with_sep_params(&spa_sep_params, &spb_sep_params);
    }


    /* Initialize any required fields and perform cleanup if required
    */
    fbe_test_common_util_test_setup_init();

    return;

} //  End tico_dualsp_setup()




/*!****************************************************************************
 *  mimi_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the common cleanup function for the tico test for single-SP testing.
 *   It destroys the physical configuration and unloads the packages.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void mimi_dualsp_cleanup(void)
{  
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;

} //  End tico_dualsp_cleanup()


/*!**************************************************************
 * mimi_test()
 ****************************************************************
 * @brief
 *  Run an I/O error retry test.
 *
 * @param raid_type      
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/
void mimi_dualsp_test()
{
    mimi_test_rg_config_bootflash_mode();
    return;
}

/*!**************************************************************
 * mimi_test_rg_config_bootflash_mode()
 ****************************************************************
 * @brief
 *  Run an IW write test case
 *
 * @param .               
 *     rg_config_p -- the RG configuration on which the test run
 *     context_p   -- could be typecast to (fbe_banio_r6_iw_case_t **)
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/
static void mimi_test_rg_config_bootflash_mode(void)
{
    fbe_status_t status;
    fbe_u32_t num_raid_groups;
    fbe_u32_t extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    mut_printf(MUT_LOG_TEST_STATUS, "running mimi test on bootflash mode");

    fbe_api_sleep(10000);

    fbe_api_trace_clear_error_counter(FBE_PACKAGE_ID_SEP_0);


    mut_printf(MUT_LOG_TEST_STATUS, "reboot the SP to normal mode");
    sep_config_fill_load_params(&spa_sep_params);

    status = fbe_test_common_reboot_this_sp(&spa_sep_params, NULL /* No params for NEIT*/); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "extended_testing_level = %d\n", extended_testing_level);
    if (extended_testing_level == 0)
    {
        /* Qual.
        */
        rg_config_p = mimi_rg_config_g;
    }
    else
    {
        /* Extended. 
        */
        rg_config_p = mimi_rg_config_g;
    }
    
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, NULL, 
                                                  mimi_test_rg_config_normal_mode,
                                                  MIMI_LUNS_PER_RAID_GROUP,
                                                  MIMI_CHUNKS_PER_LUN,
                                                  FBE_FALSE);

    fbe_api_trace_clear_error_counter(FBE_PACKAGE_ID_SEP_0);

    return;
}

/*!**************************************************************
 * mimi_test_rg_config_normal_mode()
 ****************************************************************
 * @brief
 *  Run an IW write test case
 *
 * @param .               
 *     rg_config_p -- the RG configuration on which the test run
 *     context_p   -- could be typecast to (fbe_banio_r6_iw_case_t **)
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/
void mimi_test_rg_config_normal_mode(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    mut_printf(MUT_LOG_TEST_STATUS, "running mimi test on normal mode");
}



