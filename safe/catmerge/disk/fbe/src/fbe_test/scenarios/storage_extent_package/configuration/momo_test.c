/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file momo_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests cases demos for setting up a single SP test, which starts from the bootflash mode
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

char* momo_short_desc = "this test tries to demo how to setup a single sp test, which starts from boot flash mode";
char* momo_long_desc =
"\n\
this test tries to demo how to setup a single sp test, which starts from boot flash mode.\n\
Description last updated: 16/03/2015.\n";
//------------------------------------------------------------------------------

// luns per rg for the extended test. 
#define MOMO_LUNS_PER_RAID_GROUP 1

// Number of chunks each LUN will occupy.
#define MOMO_CHUNKS_PER_LUN 3


static fbe_sep_package_load_params_t momo_sep_params = { 0 };

fbe_test_rg_configuration_t momo_rg_config_g[] = 
{
    //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
#if 0
    {6, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520, 0, 0},
    {8, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520, 1, 0},
    {16, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520, 2, 0},
#endif
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

static void momo_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
static void momo_load_sep_and_neit(void);


/*!**************************************************************
 * momo_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/
void momo_setup(void)
{
    fbe_u32_t num_raid_groups;
    
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        if (extended_testing_level == 0)
        {
            /* Qual.
             */
            rg_config_p = momo_rg_config_g;
        }
        else
        {
            /* Extended. 
             */
            rg_config_p = momo_rg_config_g;
        }
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
        fbe_test_create_physical_config_for_rg(rg_config_p,num_raid_groups);

        momo_load_sep_and_neit();
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
    return;
}

/*!**************************************************************
 * momo_load_sep_and_neit()
 ****************************************************************
 * @brief
 *  Create a configuration given a set of parameters.
 *
 * @param none.             
 *
 * @return None.
 *
 * @author
 *  16/03/2015 - Created. Geng Han
 *
 ****************************************************************/

void momo_load_sep_and_neit(void)
{
    fbe_sep_package_load_params_t sep_params; 
    fbe_neit_package_load_params_t neit_params; 

    sep_config_fill_load_params(&sep_params);
    neit_config_fill_load_params(&neit_params);

    /* boot SP to bootflash mode */
    sep_params.flags |= FBE_SEP_LOAD_FLAG_BOOTFLASH_MODE;

    sep_config_load_sep_and_neit_params(&sep_params, &neit_params);

    /* Setup some default I/O flags, which includes checking to see if they were 
     * included on the command line. 
     */
    fbe_test_sep_util_set_default_io_flags();
    return;
}


/*!**************************************************************
 * momo_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the banio test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *
 ****************************************************************/
void momo_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

/*!**************************************************************
 * momo_test()
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
void momo_test()
{
    fbe_u32_t                   extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Run the error cases for all raid types supported
     */
    mut_printf(MUT_LOG_TEST_STATUS, "extended_testing_level = %d\n", extended_testing_level);
    if (extended_testing_level == 0)
    {
        /* Qual.
        */
        rg_config_p = momo_rg_config_g;
    }
    else
    {
        /* Extended. 
        */
        rg_config_p = momo_rg_config_g;
    }
        
    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, NULL, 
                                                  momo_test_rg_config,
                                                  MOMO_LUNS_PER_RAID_GROUP,
                                                  MOMO_CHUNKS_PER_LUN,
                                                  FBE_FALSE);
    return;
}

/*!**************************************************************
 * momo_test_rg_config()
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
static void momo_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t status;


    mut_printf(MUT_LOG_TEST_STATUS, "running momo test");

    fbe_api_sleep(10000);

    fbe_api_trace_clear_error_counter(FBE_PACKAGE_ID_SEP_0);
    

    mut_printf(MUT_LOG_TEST_STATUS, "reboot the SP to normal mode");
    sep_config_fill_load_params(&momo_sep_params);

    status = fbe_test_common_reboot_this_sp(&momo_sep_params, NULL /* No params for NEIT*/); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_trace_clear_error_counter(FBE_PACKAGE_ID_SEP_0);
    
    return;
}



