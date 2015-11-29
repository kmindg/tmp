/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file snuffy_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains an I/O test for raid 3 objects.
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
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "sep_test_io.h"
#include "fbe/fbe_random.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * snuffy_short_desc = "raid 3 read and write I/O test.";
char * snuffy_long_desc ="\
The Snuffy scenario tests raid-3 non-degraded I/O.\n\
\n"\
"-raid_test_level 0 and 1\n\
     We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
\n\
STEP 1: configure all raid groups and LUNs.\n\
        - We always test at least one rg with 512 block size drives.\n\
        - All raid groups have 3 LUNs each\n\
\n\
STEP 2: write a background pattern to all luns and then read it back and make sure it was written successfully\n\
\n\
STEP 3: Peform the standard I/O test sequence.\n\
        - Perform write/read/compare for small I/os.\n\
        - Perform verify-write/read/compare for small I/Os.\n\
        - Perform verify-write/read/compare for large I/Os.\n\
        - Perform write/read/compare for large I/Os.\n\
\n\
STEP 4: Peform the standard I/O test sequence above with aborts.\n\
\n\
STEP 5: Peform the caterpillar I/O sequence.\n\
        - Caterpillar is multi-threaded sequential I/O that causes\n\
          stripe lock contention at the raid group.\n\
        - Perform write/read/compare for small I/os.\n\
        - Perform verify-write/read/compare for small I/Os.\n\
        - Perform verify-write/read/compare for large I/Os.\n\
        - Perform write/read/compare for large I/Os.\n\
\n"\
"STEP 6: Run the block operation usurper test.\n\
        - Issues single small I/Os for every LUN and every operation type.\n\
          - Operations tested on the LUN include:\n\
            - write, corrupt data, corrupt crc, verify, zero.\n\
          - Operations tested on the raid group include:\n\
            - verify, zero.\n\
\n\
STEP 7: Run the block operation usurper test (above) with large I/Os.\n\
\n"\
"STEP 8: Destroy raid groups and LUNs.\n\
\n\
Test Specific Switches:\n\
          -abort_cases_only - Only run the abort tests.\n\
\n\
Description last updated: 10/03/2011.\n";

/*!*******************************************************************
 * @def SNUFFY_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define SNUFFY_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def SNUFFY_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define SNUFFY_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def SNUFFY_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define SNUFFY_TEST_MAX_WIDTH 9

/*!*******************************************************************
 * @def SNUFFY_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define SNUFFY_SMALL_IO_SIZE_MAX_BLOCKS 1024

/*!*******************************************************************
 * @def SNUFFY_LARGE_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define SNUFFY_LARGE_IO_SIZE_MAX_BLOCKS SNUFFY_TEST_MAX_WIDTH * FBE_RAID_MAX_BE_XFER_SIZE

/*!*******************************************************************
 * @def SNUFFY_TEST_BLOCK_OP_MAX_BLOCK
 *********************************************************************
 * @brief Max number of blocks for block operation.
 *********************************************************************/
#define SNUFFY_TEST_BLOCK_OP_MAX_BLOCK 4096

/*!*******************************************************************
 * @var snuffy_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t snuffy_raid_group_config_qual[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            2,          0},
    {9,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            3,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var snuffy_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t snuffy_raid_group_config_extended[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            1,          0},
    {9,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            2,          0},

    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            3,          1},
    {9,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            4,          1},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!**************************************************************
 * snuffy_test_rg_config()
 ****************************************************************
 * @brief
 *  Run a raid 3 I/O test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  2/28/2011 - Created. Rob Foley
 *
 ****************************************************************/

void snuffy_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_bool_t b_abort_cases = fbe_test_sep_util_get_cmd_option("-abort_cases_only");
    fbe_bool_t b_run_dualsp_io = fbe_test_sep_util_get_dualsp_test_mode();

    fbe_test_raid_group_get_info(rg_config_p);

    /* Run I/O to this set of raid groups with the I/O sizes specified
     * (no aborts and no peer I/O)
     */
    if (!b_abort_cases)
    {
        fbe_test_sep_io_run_test_sequence(rg_config_p,
                                          FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_PREFILL,
                                          SNUFFY_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          SNUFFY_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE,                            /* Don't inject aborts */
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);       
    }

    /* Now run random I/Os and periodically abort some outstanding requests.
     */
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_ABORTS,
                                      SNUFFY_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      SNUFFY_LARGE_IO_SIZE_MAX_BLOCKS,
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
                                          SNUFFY_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          SNUFFY_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE,    /* Inject aborts */
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);

        fbe_test_sep_util_validate_no_raid_errors_both_sps();
    }

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end snuffy_test_rg_config()
 ******************************************/

/*!**************************************************************
 * snuffy_test_rg_config_block_ops()
 ****************************************************************
 * @brief
 *  Run a raid 3 block operations test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  3/28/2014 - Created. Rob Foley
 *
 ****************************************************************/

void snuffy_test_rg_config_block_ops(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_TEST_BLOCK_OPERATIONS,
                                      SNUFFY_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      SNUFFY_TEST_BLOCK_OP_MAX_BLOCK,
                                      FBE_FALSE,    /* Don't inject aborts */
                                      FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                      FBE_FALSE,
                                      FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID); 
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end snuffy_test_rg_config_block_ops()
 ******************************************/
/*!**************************************************************
 * snuffy_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 3 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/20/2009 - Created. Rob Foley
 *
 ****************************************************************/

void snuffy_test(void)
{    
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &snuffy_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &snuffy_raid_group_config_qual[0];
    }

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, snuffy_test_rg_config,
                                   SNUFFY_LUNS_PER_RAID_GROUP,
                                   SNUFFY_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end snuffy_test()
 ******************************************/

/*!**************************************************************
 * snuffy_block_opcode_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 3 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/20/2009 - Created. Rob Foley
 *
 ****************************************************************/

void snuffy_block_opcode_test(void)
{    
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &snuffy_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &snuffy_raid_group_config_qual[0];
    }

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, snuffy_test_rg_config_block_ops,
                                   SNUFFY_LUNS_PER_RAID_GROUP,
                                   SNUFFY_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end snuffy_block_opcode_test()
 ******************************************/

/*!**************************************************************
 * snuffy_dualsp_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 6 objects on both SPs.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  5/18/2011 - Created. Rob Foley
 *  
 ****************************************************************/

void snuffy_dualsp_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &snuffy_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &snuffy_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, snuffy_test_rg_config,
                                   SNUFFY_LUNS_PER_RAID_GROUP,
                                   SNUFFY_CHUNKS_PER_LUN);

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end snuffy_dualsp_test()
 ******************************************/

/*!**************************************************************
 * snuffy_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 3 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  11/20/2009 - Created. Rob Foley
 *
 ****************************************************************/
void snuffy_setup(void)
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
            rg_config_p = &snuffy_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &snuffy_raid_group_config_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
    
        elmo_load_config(rg_config_p, 
                         SNUFFY_LUNS_PER_RAID_GROUP, 
                         SNUFFY_CHUNKS_PER_LUN);
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

    return;
}
/**************************************
 * end snuffy_setup()
 **************************************/

/*!**************************************************************
 * snuffy_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 6 I/O test to run on both sps.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  5/18/2011 - Created. Rob Foley
 *
 ****************************************************************/
void snuffy_dualsp_setup(void)
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
            rg_config_p = &snuffy_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &snuffy_raid_group_config_qual[0];
        }

        /* We no longer create the raid groups during the setup
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
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


    return;
}
/**************************************
 * end snuffy_dualsp_setup()
 **************************************/

/*!**************************************************************
 * snuffy_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the snuffy test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/20/2009 - Created. Rob Foley
 *
 ****************************************************************/

void snuffy_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end snuffy_cleanup()
 ******************************************/
/*!**************************************************************
 * snuffy_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the snuffy test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/18/2011 - Created. Rob Foley
 *
 ****************************************************************/

void snuffy_dualsp_cleanup(void)
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
 * end snuffy_dualsp_cleanup()
 ******************************************/
/*************************
 * end file snuffy_test.c
 *************************/


