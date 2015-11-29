/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file    ultra_hd_test.c
 ***************************************************************************
 *
 * @brief   This file contains non-degraded I/O using 4K drives for all raid
 *          types.
 *
 * @version
 *  11/22/2013  Ron Proulx  - Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
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
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * ultra_hd_short_desc = "This scenario tests non-degraded I/O to raid groups with 4K drives.";
char * ultra_hd_long_desc = "\
This scenario tests non-degraded I/O to raid groups with 4K drives on all raid types.\n\
\n"\
"-raid_test_level 0 and 1\n\
     We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
\n\
STEP 1: Run a get-info test for raid groups with 4K drives.\n\
        - The purpose of this test is to validate the functionality of the\n\
          get-info usurper.  This usurper is used when we create a raid group to\n\
          validate the parameters of raid group creation such as width, element size, etc.\n\
        - Issue get info usurper to the striper class and validate various\n\
          test cases including normal and error cases.\n\
\n\
STEP 2: configure all raid groups and LUNs.\n\
        - We always test at least one rg with 512 block size drives.\n\
        - All raid groups have 3 LUNs each\n\
\n\
STEP 3: write a background pattern to all luns and then read it back and make sure it was written successfully\n\
\n\
STEP 4: Peform the standard I/O test sequence.\n\
        - Perform write/read/compare for small I/os.\n\
        - Perform verify-write/read/compare for small I/Os.\n\
        - Perform verify-write/read/compare for large I/Os.\n\
        - Perform write/read/compare for large I/Os.\n\
\n\
STEP 5: Peform the standard I/O test sequence above with aborts.\n\
\n\
STEP 6: Peform the caterpillar I/O sequence.\n\
        - Caterpillar is multi-threaded sequential I/O that causes\n\
          stripe lock contention at the raid group.\n\
        - Perform write/read/compare for small I/os.\n\
        - Perform verify-write/read/compare for small I/Os.\n\
        - Perform verify-write/read/compare for large I/Os.\n\
        - Perform write/read/compare for large I/Os.\n\
\n"\
"STEP 7: Run the block operation usurper test.\n\
        - Issues single small I/Os for every LUN and every operation type.\n\
          - Operations tested on the LUN include:\n\
            - write, corrupt data, corrupt crc, verify, zero.\n\
          - Operations tested on the raid group include:\n\
            - verify, zero.\n\
\n\
STEP 8: Run the block operation usurper test (above) with large I/Os.\n\
\n"\
"STEP 9: Destroy raid groups and LUNs.\n\
\n\
Test Specific Switches:\n\
          -abort_cases_only - Only run the abort tests.\n\
\n\
Description last updated: 11/22/2013.\n";

/*!*******************************************************************
 * @def ULTRA_HD_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define ULTRA_HD_TEST_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def     ULTRA_HD_CHUNKS_PER_LUN
 *********************************************************************
 * @brief   Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define ULTRA_HD_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def ULTRA_HD_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define ULTRA_HD_SMALL_IO_SIZE_MAX_BLOCKS 1024

/*!*******************************************************************
 * @def ULTRA_HD_TEST_BLOCK_OP_MAX_BLOCK
 *********************************************************************
 * @brief Max number of blocks for block operation.
 *********************************************************************/
#define ULTRA_HD_TEST_BLOCK_OP_MAX_BLOCK 4096

/*!*******************************************************************
 * @def ULTRA_HD_LARGE_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define ULTRA_HD_LARGE_IO_SIZE_MAX_BLOCKS ULTRA_HD_TEST_MAX_WIDTH * FBE_RAID_MAX_BE_XFER_SIZE

/*!*******************************************************************
 * @def ULTRA_HD_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define ULTRA_HD_TEST_MAX_WIDTH 16

/*!*******************************************************************
 * @var ultra_hd_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t ultra_hd_raid_group_config_qual[] = 
{
    /* width,   capacity  raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {2,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   4160,            0,          0},
    {5,         0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   4160,            1,          0},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_TEST_RG_CONFIG_RANDOM_TAG,  FBE_CLASS_ID_INVALID, 4160, 2, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/*!*******************************************************************
 * @var ultra_hd_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t ultra_hd_raid_group_config_extended[] = 
{
    /* width, capacity    raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {3,         0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,  4160,            0,          0},
    {2,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   4160,            1,          0},
    {5,         0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   4160,            2,          0},
    {6,         0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   4160,            3,          0},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_TEST_RG_CONFIG_RANDOM_TAG,  FBE_CLASS_ID_INVALID, 4160, 4, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!**************************************************************
 * ultra_hd_test_rg_config()
 ****************************************************************
 * @brief
 *  Run I/O test cases against raid groups using 4K block size
 *  drives.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  11/22/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void ultra_hd_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_bool_t b_run_dualsp_io = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_bool_t b_abort_cases = fbe_test_sep_util_get_cmd_option("-abort_cases_only");

    fbe_test_raid_group_get_info(rg_config_p);

    /* Run I/O to this set of raid groups with the I/O sizes specified
     * (no aborts and no peer I/O)
     */
    if (!b_abort_cases)
    {
        fbe_test_sep_io_run_test_sequence(rg_config_p,
                                          FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_PREFILL,
                                          ULTRA_HD_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          ULTRA_HD_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE,                            /* Don't inject aborts */
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);
    }

    /* Now run random I/Os and periodically abort some outstanding requests.
     */
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_ABORTS,
                                      ULTRA_HD_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      ULTRA_HD_LARGE_IO_SIZE_MAX_BLOCKS,
                                      FBE_TRUE,                             /* Inject aborts */
                                      FBE_TEST_RANDOM_ABORT_TIME_ZERO_MSEC,
                                      b_run_dualsp_io,
                                      FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);

    /* Now run caterpillar I/Os and periodically abort some outstanding requests.
     */
    if (!b_abort_cases)
    {
        fbe_test_sep_io_run_test_sequence(rg_config_p,
                                          FBE_TEST_SEP_IO_SEQUENCE_TYPE_CATERPILLAR,
                                          ULTRA_HD_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          ULTRA_HD_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE,                         /* Inject aborts */
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);

        /* We do not expect any errors.
         */
        fbe_test_sep_util_validate_no_raid_errors_both_sps();

        if (!b_run_dualsp_io)
        {
            fbe_test_sep_io_run_test_sequence(rg_config_p,
                                              FBE_TEST_SEP_IO_SEQUENCE_TYPE_TEST_BLOCK_OPERATIONS,
                                              ULTRA_HD_SMALL_IO_SIZE_MAX_BLOCKS, 
                                              ULTRA_HD_TEST_BLOCK_OP_MAX_BLOCK,
                                              FBE_FALSE,    /* Don't inject aborts */
                                              FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                              FBE_FALSE,
                                              FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);  
        }
    }

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ultra_hd_test_rg_config()
 ******************************************/

/*!***************************************************************************
 *          ultra_hd_test()
 *****************************************************************************
 *
 * @brief   Run the 4K drive block size tests.  One mirror and one parity type
 *          in addition to one random raid type are tested.  This is a 
 *          non-degraded I/O test.
 *
 * @param   None.           
 *
 * @return  None.
 *
 * @author
 *  11/22/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
void ultra_hd_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &ultra_hd_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &ultra_hd_raid_group_config_qual[0];
    }

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, ultra_hd_test_rg_config,
                                   ULTRA_HD_TEST_LUNS_PER_RAID_GROUP,
                                   ULTRA_HD_CHUNKS_PER_LUN);

    return;
}
/******************************************
 * end ultra_hd_test()
 ******************************************/

/*!***************************************************************************
 *          ultra_hd_test()
 *****************************************************************************
 *
 * @brief   Run the 4K drive block size tests.  One mirror and one parity type
 *          in addition to one random raid type are tested.  This is a 
 *          non-degraded I/O test.
 *
 * @param   None.           
 *
 * @return  None.
 *
 * @author
 *  11/22/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
void ultra_hd_dualsp_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &ultra_hd_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &ultra_hd_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, ultra_hd_test_rg_config,
                                   ULTRA_HD_TEST_LUNS_PER_RAID_GROUP,
                                   ULTRA_HD_CHUNKS_PER_LUN);

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end ultra_hd_dualsp_test()
 ******************************************/

/*!**************************************************************
 * ultra_hd_setup()
 ****************************************************************
 * @brief
 *  Setup for a 4K block size tests.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  11/22/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void ultra_hd_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        if (test_level > 0)
        {
            rg_config_p = &ultra_hd_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &ultra_hd_raid_group_config_qual[0];
        }

        /* We no longer create the raid groups during the setup
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        elmo_load_config(rg_config_p, 
                           ULTRA_HD_TEST_LUNS_PER_RAID_GROUP, 
                           ULTRA_HD_CHUNKS_PER_LUN);

        /* Enable 4K  drives */
        fbe_api_drive_configuration_set_control_flag(FBE_DCS_4K_ENABLED, FBE_TRUE);
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/**************************************
 * end ultra_hd_setup()
 **************************************/

/*!**************************************************************
 * ultra_hd_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a 4K block size tests.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 * 11/22/2013   Ron Proulx  - Created.
 *
 ****************************************************************/
void ultra_hd_dualsp_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        if (test_level > 0)
        {
            rg_config_p = &ultra_hd_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &ultra_hd_raid_group_config_qual[0];
        }

        /* We no longer create the raid groups during the setup
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
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

        /* Enable 4K drives */    
        fbe_api_drive_configuration_set_control_flag(FBE_DCS_4K_ENABLED, FBE_TRUE);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_api_drive_configuration_set_control_flag(FBE_DCS_4K_ENABLED, FBE_TRUE);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    return;
}
/**************************************
 * end ultra_hd_dualsp_setup()
 **************************************/

/*!**************************************************************
 * ultra_hd_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the 4K block size tests.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/22/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void ultra_hd_cleanup(void)
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
 * end ultra_hd_cleanup()
 ******************************************/

/*!**************************************************************
 * ultra_hd_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the 4K block size tests.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/22/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void ultra_hd_dualsp_cleanup(void)
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
 * end ultra_hd_dualsp_cleanup()
 ******************************************/


/*************************
 * end file ultra_hd_test.c
 *************************/


