/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file rick_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains an user zero I/O test for all raid types
 *
 * @version
 *   23/12/2015 - Created. Geng Han
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
#include "fbe/fbe_api_database_interface.h"
#include "sep_test_io.h"
#include "fbe/fbe_random.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * rick_short_desc = "user zero I/O test for all raid types.";
char * rick_long_desc ="\
The Rick scenario tests all raid types on user zero I/O.\n\
\n"\
"-raid_test_level 0 and 1\n\
     We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
\n\
STEP 1: configure all raid groups and LUNs.\n\
        - We will randomize the drive block size, all the RG types will be tested.\n\
        - All raid groups have 3 LUNs each\n\
\n\
STEP 2: write a non-zero background pattern to to first 32 stripe elements\n\
\n\
STEP 3: send a uzer zero IO, the IO lba range is restricted to the first 32 stripe elements.\n\
\n\
STEP 4: verify the blocks ahead of the zero IO is not corrupted \n\
\n\
STEP 5: verify the blocks after the zero IO is not corrupted.\n\
        - Caterpillar is multi-threaded sequential I/O that causes\n\
          stripe lock contention at the raid group.\n\
        - Perform write/read/compare for small I/os.\n\
        - Perform verify-write/read/compare for small I/Os.\n\
        - Perform verify-write/read/compare for large I/Os.\n\
        - Perform write/read/compare for large I/Os.\n\
\n"
"Description last updated: 24/12/2014.\n";

/*!*******************************************************************
 * @def RICK_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define RICK_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def RICK_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define RICK_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def RICK_MAX_RAID_GROUPS
 *********************************************************************
 * @brief Max number of raid groups configured in this test
 *
 *********************************************************************/
#define RICK_MAX_RAID_GROUPS 12


/*!*******************************************************************
 * @def RICK_TEST_ELEMENT_SIZE
 *********************************************************************
 * @brief the element size 
 *
 *********************************************************************/
#define RICK_TEST_ELEMENT_SIZE 0x80


/*!*******************************************************************
 * @var rick_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t rick_raid_group_config_qual[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {4,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,     520,            1,          1},
    {2,       0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,      520,            2,          0},
    {6,       0xE000,     FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,     520,            3,          0},
    {4,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,      520,            4,          0},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,      520,            5,          0},
    {6,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,      520,            6,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var rick_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t rick_raid_group_config_extended[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {4,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,     520,            1,          1},
    {2,       0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,      520,            2,          0},
    {6,       0xE000,     FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,     520,            3,          0},
    {4,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,      520,            4,          0},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,      520,            5,          0},
    {6,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,      520,            6,          0},

    {4,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,     520,            7,          1},
    {2,       0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,      520,            8,          1},
    {6,       0xE000,     FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,     520,            9,          1},
    {4,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,      520,            10,          1},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,      520,            11,          1},
    {6,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,      520,            12,          1},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


// Start a I/O to a perticular LUN 
static void rick_test_start_io(
                               fbe_test_rg_configuration_t*    in_rg_config_p,
                               fbe_api_rdgen_context_t*        in_rdgen_context_p,
                               fbe_lba_t                       in_lba,
                               fbe_block_count_t               in_num_blocks,
                               fbe_rdgen_operation_t           in_rdgen_op,
                               fbe_object_id_t                 in_lun_object_id);

/*!**************************************************************
 * rick_test_rg_config()
 ****************************************************************
 * @brief
 *  Run user zero I/O test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  24/12/2014 - Created. Geng Han
 *
 ****************************************************************/
void rick_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                 status;             
    fbe_u32_t                    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t                    rg_index = 0;
    fbe_api_rdgen_context_t      rdgen_context[RICK_MAX_RAID_GROUPS][RICK_LUNS_PER_RAID_GROUP];
    fbe_test_rg_configuration_t* cur_rg_config_p;
    fbe_u64_t                    background_io_size = (RICK_TEST_ELEMENT_SIZE * 32);
    fbe_u64_t                    zero_start_lba;
    fbe_u64_t                    zero_end_lba;
    fbe_object_id_t                         raid_group_id;
    

    MUT_ASSERT_TRUE(raid_group_count <= RICK_MAX_RAID_GROUPS);

    // write an non-zero background pattern to the first 32 stripe elements
    cur_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) 
    {
        // get the RG number
        status = fbe_api_database_lookup_raid_group_by_number(cur_rg_config_p->raid_group_id, &raid_group_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        mut_printf(MUT_LOG_TEST_STATUS, "send background IO to rg: %d, lba:0x%llx, blks: 0x%llx", 
                   cur_rg_config_p->raid_group_id,
                   (fbe_u64_t)0, background_io_size);
        rick_test_start_io(cur_rg_config_p, 
                           rdgen_context[rg_index],
                           0x0,  /* lba */
                           background_io_size, /* blks */
                           FBE_RDGEN_OPERATION_WRITE_ONLY,
                           FBE_OBJECT_ID_INVALID);

        cur_rg_config_p++;
    }

    // randomize the zero_start_lba and zero_blocks
    zero_start_lba = fbe_random() % background_io_size;
    zero_end_lba = zero_start_lba + fbe_random() % (background_io_size - zero_start_lba) + 1;
    
    // send a 4k unaligned zero IO to the middle of the first 32 stripe elements
    cur_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) 
    {
        mut_printf(MUT_LOG_TEST_STATUS, "send zero IO to rg: %d, lba:0x%llx, blks: 0x%llx", 
                   cur_rg_config_p->raid_group_id,
                   zero_start_lba, (zero_end_lba - zero_start_lba));
        rick_test_start_io(cur_rg_config_p, 
                           rdgen_context[rg_index],
                           zero_start_lba,  /* lba */
                           (zero_end_lba - zero_start_lba), /* blks */
                           FBE_RDGEN_OPERATION_ZERO_READ_CHECK,
                           FBE_OBJECT_ID_INVALID);

        cur_rg_config_p++;
    }

    // check the blks ahead the zero IO were not corrupted by the zero IO
    if (zero_start_lba > 0)
    {
        cur_rg_config_p = rg_config_p;
        for (rg_index = 0; rg_index < raid_group_count; rg_index++) 
        {
            mut_printf(MUT_LOG_TEST_STATUS, "send background check IO (ahead) to rg: %d, lba:0x%llx, blks: 0x%llx", 
                       cur_rg_config_p->raid_group_id,
                       (fbe_u64_t)0, zero_start_lba);
            rick_test_start_io(cur_rg_config_p, 
                               rdgen_context[rg_index],
                               0,  /* lba */
                               zero_start_lba, /* blks */
                               FBE_RDGEN_OPERATION_READ_CHECK,
                               FBE_OBJECT_ID_INVALID);
            cur_rg_config_p++;
        }
    }
    
    // check the blks behind the zero IO were not corrupted by the zero IO
    if (zero_end_lba < background_io_size)
    {
        cur_rg_config_p = rg_config_p;
        for (rg_index = 0; rg_index < raid_group_count; rg_index++) 
        {
            mut_printf(MUT_LOG_TEST_STATUS, "send background check IO (behind) to rg: %d, lba:0x%llx, blks: 0x%llx", 
                       cur_rg_config_p->raid_group_id,
                       zero_end_lba, background_io_size - zero_end_lba);
            rick_test_start_io(cur_rg_config_p, 
                               rdgen_context[rg_index],
                               zero_end_lba,  /* lba */
                               background_io_size - zero_end_lba, /* blks */
                               FBE_RDGEN_OPERATION_READ_CHECK,
                               FBE_OBJECT_ID_INVALID);
            cur_rg_config_p++;
        }
    }

    return;
}
/******************************************
 * end rick_test_rg_config()
 ******************************************/

/*!**************************************************************
 * rick_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 3 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  24/12/2014 - Created. Geng Han
 *
 ****************************************************************/
void rick_test(void)
{    
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &rick_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &rick_raid_group_config_qual[0];
    }

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, rick_test_rg_config,
                                   RICK_LUNS_PER_RAID_GROUP,
                                   RICK_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end rick_test()
 ******************************************/

/*!**************************************************************
 * rick_setup()
 ****************************************************************
 * @brief
 *  Setup for a I/O test on all raid types
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  24/12/2014 - Created. Geng Han
 *
 ****************************************************************/
void rick_setup(void)
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
            rg_config_p = &rick_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &rick_raid_group_config_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
    
        elmo_load_config(rg_config_p, 
                         RICK_LUNS_PER_RAID_GROUP, 
                         RICK_CHUNKS_PER_LUN);
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
 * end rick_setup()
 **************************************/


/*!**************************************************************
 * rick_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the rick test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  24/12/2014 - Created. Geng Han
 *
 ****************************************************************/
void rick_cleanup(void)
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
 * end rick_cleanup()
 ******************************************/

/*!**************************************************************
 * rick_test_start_io()
 ****************************************************************
 * @brief
 *  start a single lba fixed IO to a particular LUN or to the all the luns on a RG
 *
 * @param rg_config_p - config to run test against.               
 * @in_rdgen_context_p - rdgen context array
 * @in_lba - fixed lba
 * @in_num_blocks - fixed IO blocks
 * @in_rdgen_op - rdgen operation
 * @in_lun_object_id - lun object ID, the INVALID object ID indicates sending the IO to all the luns under the RG
 * @return None.   
 *
 * @author
 *  24/12/2014 - Created. Geng Han
 *
 ****************************************************************/
void rick_test_start_io(
                        fbe_test_rg_configuration_t*    in_rg_config_p,
                        fbe_api_rdgen_context_t*        in_rdgen_context_p,
                        fbe_lba_t                       in_lba,
                        fbe_block_count_t               in_num_blocks,
                        fbe_rdgen_operation_t           in_rdgen_op,
                        fbe_object_id_t                 in_lun_object_id)
{
    fbe_status_t                            status;
    fbe_u32_t                               lun_index;
    fbe_object_id_t                         lun_object_id;
    fbe_test_logical_unit_configuration_t  *logical_unit_configuration_p = NULL;

    //  See if the user only wants to issue I/O to a particular LUN in the RG
    if (FBE_OBJECT_ID_INVALID != in_lun_object_id)
    {
        //  Send a single I/O to the LU object provided as input
        status = fbe_api_rdgen_send_one_io_asynch(in_rdgen_context_p,
                                                  in_lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_PACKAGE_ID_SEP_0,
                                                  in_rdgen_op,
                                                  FBE_RDGEN_PATTERN_LBA_PASS,
                                                  in_lba,
                                                  in_num_blocks,
                                                  FBE_RDGEN_OPTIONS_INVALID);
    
        //  Make sure the I/O was sent successfully
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        // wait for IO complete
        fbe_api_rdgen_wait_for_ios(in_rdgen_context_p, FBE_PACKAGE_ID_SEP_0, 1);

        // check IO status
        MUT_ASSERT_INT_EQUAL(in_rdgen_context_p->start_io.statistics.error_count, 0);

        mut_printf(MUT_LOG_TEST_STATUS, "rick_test_start_io for object id: 0x%x complete. passes: 0x%x io_count: 0x%x error_count: 0x%x",
                   in_rdgen_context_p->object_id,
                   in_rdgen_context_p->start_io.statistics.pass_count,
                   in_rdgen_context_p->start_io.statistics.io_count,
                   in_rdgen_context_p->start_io.statistics.error_count);

        // destroy the context
        status = fbe_api_rdgen_test_context_destroy(in_rdgen_context_p);
        
        return;
    }

    //  If got this far, user wants to issue I/O to all LUNs in the RG
    logical_unit_configuration_p = in_rg_config_p->logical_unit_configuration_list;
    for (lun_index = 0; lun_index < RICK_LUNS_PER_RAID_GROUP; lun_index++)
    {
        //  Get the LUN object ID of a LUN in the RG
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
   
        //  Send a single I/O to the LU object
        status = fbe_api_rdgen_send_one_io_asynch(&in_rdgen_context_p[lun_index],
                                                  lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_PACKAGE_ID_SEP_0,
                                                  in_rdgen_op,
                                                  FBE_RDGEN_PATTERN_LBA_PASS,
                                                  in_lba,
                                                  in_num_blocks,
                                                  FBE_RDGEN_OPTIONS_INVALID);
    
        //  Make sure the I/O was sent successfully
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        // wait for IO complete
        fbe_api_rdgen_wait_for_ios(&in_rdgen_context_p[lun_index], FBE_PACKAGE_ID_SEP_0, 1);

        // check IO status
        MUT_ASSERT_INT_EQUAL(in_rdgen_context_p[lun_index].start_io.statistics.error_count, 0);

        mut_printf(MUT_LOG_TEST_STATUS, "rick_test_start_io for object id: 0x%x complete. passes: 0x%x io_count: 0x%x error_count: 0x%x",
                   in_rdgen_context_p[lun_index].object_id,
                   in_rdgen_context_p[lun_index].start_io.statistics.pass_count,
                   in_rdgen_context_p[lun_index].start_io.statistics.io_count,
                   in_rdgen_context_p[lun_index].start_io.statistics.error_count);

        // destroy the context
        status = fbe_api_rdgen_test_context_destroy(&in_rdgen_context_p[lun_index]);
        
        // Goto the next LUN
        logical_unit_configuration_p++;
    }

    return;
}   // End rick_test_start_io()


/*************************
 * end file rick_test.c
 *************************/

