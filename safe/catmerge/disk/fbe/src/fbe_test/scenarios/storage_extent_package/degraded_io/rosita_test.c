/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file rosita_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a degraded I/O test for raid 10 objects.
 *
 * @version
 *   9/10/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_virtual_drive_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * rosita_short_desc = "raid 1/0 degraded read and write I/O test.";
/* uses big bird long description since we are based on big bird. */
char * rosita_long_desc = NULL;

/*!*******************************************************************
 * @def ROSITA_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define ROSITA_TEST_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def ROSITA_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define ROSITA_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @var rosita_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t rosita_raid_group_config_extended[] = 
{
    /* width,   capacity    raid type,                  class,              block size  RAID-id.    bandwidth.*/

    {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            0,          0},
    {6,       0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            1,          0},
    {16,      0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            2,          0},

    {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            3,          1},
    {6,       0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            4,          1},
    {16,      0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            5,          1},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var rosita_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t rosita_raid_group_config_qual[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/

    {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            0,          0},
    {16,      0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            1,          1},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!**************************************************************
 * rosita_single_degraded_test()
 ****************************************************************
 * @brief
 *  Run degraded I/O to a set of raid 10 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void rosita_single_degraded_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &rosita_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &rosita_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, 
                                                   (void*)1,/* drives to remove */
                                                   big_bird_normal_degraded_io_test,
                                                   ROSITA_TEST_LUNS_PER_RAID_GROUP,
                                                   ROSITA_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end rosita_single_degraded_test()
 ******************************************/
/*!**************************************************************
 * rosita_single_degraded_zero_abort_test()
 ****************************************************************
 * @brief
 *  Run degraded I/O to a set of raid 10 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void rosita_single_degraded_zero_abort_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &rosita_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &rosita_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, 
                                                   (void*)1,/* drives to remove */
                                                   big_bird_zero_and_abort_io_test,
                                                   ROSITA_TEST_LUNS_PER_RAID_GROUP,
                                                   ROSITA_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end rosita_single_degraded_zero_abort_test()
 ******************************************/
/*!**************************************************************
 * rosita_shutdown_test()
 ****************************************************************
 * @brief
 *  Run degraded I/O to a set of raid 10 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void rosita_shutdown_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &rosita_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &rosita_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, 
                                                   NULL,
                                                   big_bird_run_shutdown_test,
                                                   ROSITA_TEST_LUNS_PER_RAID_GROUP,
                                                   ROSITA_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end rosita_shutdown_test()
 ******************************************/

/*!**************************************************************
 * rosita_single_degraded_encryption_test()
 ****************************************************************
 * @brief
 *  Run degraded I/O to a set of raid 5 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  1/24/2014 - Created. Rob Foley
 *
 ****************************************************************/

void rosita_single_degraded_encryption_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &rosita_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &rosita_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, (void*)1,/* drives to remove */
                                                   big_bird_normal_degraded_encryption_test,
                                                   ROSITA_TEST_LUNS_PER_RAID_GROUP,
                                                   ROSITA_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end rosita_test_single_degraded_test()
 ******************************************/
/*!**************************************************************
 * rosita_setup()
 ****************************************************************
 * @brief
 *  Setup the rosita test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void rosita_setup(void)
{
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
         */
        if (test_level > 0)
        {
            /* Run test for normal element size.
             */
            rg_config_p = &rosita_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &rosita_raid_group_config_qual[0];
        }

        /* Initialize the array of RGs */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        /* Add more chunks for the array of raid groups we create.*/
        fbe_test_sep_util_update_extra_chunk_size(rg_config_p, ROSITA_CHUNKS_PER_LUN);

        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg which will use while running test
            in simulation and on hardware */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            sep_config_load_kms(NULL);
        }
    }

    /* This test will pull a drive out and insert a new drive
     * with a different serial number in the same slot. 
     * We configure this drive as a spare and we want this drive to swap in immediately. 
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1 /* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    fbe_test_disable_background_ops_system_drives();
    
    return;
}
/**************************************
 * end rosita_setup()
 **************************************/

/*!**************************************************************
 * rosita_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the rosita test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void rosita_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    big_bird_cleanup();
    return;
}
/******************************************
 * end rosita_cleanup()
 ******************************************/

void rosita_dualsp_single_degraded_test(void)
{
    rosita_single_degraded_test();
}
void rosita_dualsp_single_degraded_zero_abort_test(void)
{
    rosita_single_degraded_zero_abort_test();
}
void rosita_dualsp_shutdown_test(void)
{
    rosita_shutdown_test();
    return;
}
void rosita_dualsp_single_degraded_encryption_test(void)
{
    rosita_single_degraded_encryption_test();
}
/*!**************************************************************
 * rosita_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 10 degraded I/O test.
 *
 * @param   None.               
 *
 * @return  None.   
 *
 * @note    Must run in dual-sp mode
 *
 * @author
 *  4/27/2011 - Created. Rob Foley
 *
 ****************************************************************/
void rosita_dualsp_setup(void)
{
    fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    if (fbe_test_util_is_simulation())
    { 
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
         */
        if (test_level > 0)
        {
            /* Run test for normal element size.
             */
            rg_config_p = &rosita_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &rosita_raid_group_config_qual[0];
        }

        /* Initialize the array of RGs */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        /* Add more chunks for the array of raid groups we create.*/
        fbe_test_sep_util_update_extra_chunk_size(rg_config_p, ROSITA_CHUNKS_PER_LUN);

        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg which will be use by
            simulation test and hardware test */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

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
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            sep_config_load_kms_both_sps(NULL);
        }
    }

    /* This test will pull a drive out and insert a new drive
     * with a different serial number in the same slot. 
     * We configure this drive as a spare and we want this drive to swap in immediately. 
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1 /* 1 second */);

    /* Initialize any required fields and perform cleanup if required */
    fbe_test_common_util_test_setup_init();
    fbe_test_disable_background_ops_system_drives();
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    return;
}
/******************************************
 * end rosita_dualsp_setup()
 ******************************************/

void rosita_dualsp_encryption_setup(void)
{
    fbe_status_t status;
    fbe_test_sep_util_set_encryption_test_mode(FBE_TRUE);

    rosita_dualsp_setup();
    /* Disable load balancing so that our read and pin will occur on the same SP that we are running the encryption. 
     * This is needed so that rdgen can do the locking on the same SP where the read and pin is occuring.
     */
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
void rosita_encryption_setup(void)
{
    fbe_test_sep_util_set_encryption_test_mode(FBE_TRUE);
    rosita_setup();
}
/*!**************************************************************
 * rosita_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the rosita test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  4/27/2011 - Created. Rob Foley
 *
 ****************************************************************/

void rosita_dualsp_cleanup(void)
{
    big_bird_dualsp_cleanup();

    return;
}
/******************************************
 * end rosita_dualsp_cleanup()
 ******************************************/

/*************************
 * end file rosita_test.c
 *************************/


