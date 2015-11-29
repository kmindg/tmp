/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file strider_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a test for degraded and shutdown raid groups with
 *  paged metadata errors.
 *
 * @version
 *   11/20/2012 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
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
#include "fbe/fbe_api_block_transport_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * strider_short_desc = "Various scenarios of shutting down a raid group."; 
char * strider_long_desc = "\
This test contains test cases for shutting down the raid group under different circumstances and making sure it can\n\
get back to ready successfully.\n\
\n\
-raid_test_level 0 and 1\n\
   - We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
\n\
Test 1: Spanning chunk boundary flush test.\n\
\n\
 STEP 1: Degrade the raid group\n\
\n\
 STEP 2: Send an I/O spanning a chunk boundary with error injection enabled.\n\
         The I/O is expected to see an error.\n\
\n\
 STEP 3: Send I/O to mark first chunk as needing verify. \n\
         This causes the I/O to span a range where the chunks have different verify bits.\n\
\n\
 STEP 3: Wait for the error to be hit.\n\
\n\
 STEP 4: Pull another drive to break the raid group.\n\
\n\
 STEP 5: Wait for the raid group to become FAIL\n\
\n\
 STEP 6: Re-insert the drives and wait for RG to become READY and for rebuilds to occur.\n\
         When the raid group comes back through activate it will perform a flush, which should\n\
         span a chunk boundary and see different verify bits for the first and subsequent chunk.\n\
\n\
\n\
Test 2: Paged error injection test\n\
        test shuts down a raid group that has errors on the paged, and then attempts to bring it back degraded.\n\
 STEP 1: Inject errors on the paged metadata\n\
\n\
 STEP 2: Degrade the raid group\n\
\n\
 STEP 3: Shut down the raid group by pulling more than one drive.\n\
\n\
 STEP 4: Bring the raid group back, but leave the first drive still pulled.\n\
\n\
 STEP 5: Validate that the paged has been reconstructed.\n\
\n\
 STEP 6: Bring back the final removed drive and wait for the rebuild.\n\
\n\
Description last updated: 12/12/2012.\n";

/*!*******************************************************************
 * @def STRIDER_TEST_MAX_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define STRIDER_TEST_MAX_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def STRIDER_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define STRIDER_TEST_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def STRIDER_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define STRIDER_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def STRIDER_MAX_RGS
 *********************************************************************
 * @brief Max number of raid groups.
 *
 *********************************************************************/
#define STRIDER_MAX_RGS 7

/*!*******************************************************************
 * @def STRIDER_MAX_DRIVES_TO_REMOVE 
 *********************************************************************
 * @brief Max number of drives we will remove to make the raid group degraded.
 *
 *********************************************************************/
#define STRIDER_MAX_DRIVES_TO_REMOVE 2

/*!*******************************************************************
 * @var strider_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t strider_test_contexts[STRIDER_TEST_MAX_LUNS_PER_RAID_GROUP * 2];

/*!*******************************************************************
 * @var strider_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t strider_raid_group_config_extended[STRIDER_MAX_RGS] = 
{
    /* width,   capacity    raid type,                  class,               block size     RAID-id.    bandwidth.*/

    { 6 /*FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520, 0, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {12 /*FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID6,   FBE_CLASS_ID_PARITY,  520, 1, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {6 /*FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID5,   FBE_CLASS_ID_PARITY,  520, 2, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {4 /*FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID3,   FBE_CLASS_ID_PARITY,  520, 3, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {2, 0xE000, FBE_RAID_GROUP_TYPE_RAID1,   FBE_CLASS_ID_MIRROR,  520, 4, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var strider_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t strider_raid_group_config_qual[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {5, 0xE000, FBE_RAID_GROUP_TYPE_RAID5,   FBE_CLASS_ID_PARITY, 520, 4, 0},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


/*!**************************************************************
 * strider_shutdown_paged_errors()
 ****************************************************************
 * @brief
 *  Run paged error injection test.
 *
 * @param rg_config_p - Current configuration
 * @param raid_group_count - number of rgs under test.
 * @param luns_per_rg - Number of LUNs for each RG.
 * @param peer_options - How to run rdgen I/O local or with peer.            
 *
 * @return None.
 *
 * @author
 *  11/20/2012 - Created. Rob Foley
 *
 ****************************************************************/

void strider_shutdown_paged_errors(fbe_test_rg_configuration_t * const rg_config_p,
                                   fbe_u32_t raid_group_count,
                                   fbe_u32_t luns_per_rg,
                                   fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_api_rdgen_context_t *context_p = &strider_test_contexts[0];
    fbe_status_t status;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t rg_object_ids[STRIDER_MAX_RGS];
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t num_drives_to_remove[STRIDER_MAX_RGS];
    fbe_test_drive_removal_mode_t removal_mode = FBE_DRIVE_REMOVAL_MODE_RANDOM; /* Default to Random. */
    fbe_api_rdgen_send_one_io_params_t params;
    fbe_u32_t drive_number;

    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting peer options: %d==", __FUNCTION__, peer_options);

    big_bird_remove_all_drives(rg_config_p, raid_group_count, 1,
                               FBE_TRUE, /* We are pulling and reinserting same drive */
                               0, /* msec wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    /* Send a single I/O to corrupt the paged.
     */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        fbe_api_raid_group_get_info_t rg_info;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[index];

        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_api_rdgen_io_params_init(&params);
        params.object_id = rg_object_id;
        params.class_id = FBE_CLASS_ID_INVALID;
        params.package_id = FBE_PACKAGE_ID_SEP_0;
        params.msecs_to_abort = 0;
        params.msecs_to_expiration = 0;
        params.block_spec = FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE;

        params.rdgen_operation = FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK;
        params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
        params.lba = rg_info.paged_metadata_start_lba;
        params.blocks = 1;

        mut_printf(MUT_LOG_TEST_STATUS, "corrupt paged for rg %d (object 0x%x) lba 0x%llx bl 0x%llx",
                   current_rg_config_p->raid_group_id, rg_object_id,
                   params.lba, params.blocks);
        status = fbe_api_rdgen_send_one_io_params(context_p, &params);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 1);
        mut_printf(MUT_LOG_TEST_STATUS, "corrupt paged for rg %d (object 0x%x)...SUCCESS",
                   current_rg_config_p->raid_group_id, rg_object_id);

        status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
        {
            num_drives_to_remove[index] = 3;
        }
        else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            /* Raid 10 goes sequential to insure a shutdown.
             */
            removal_mode = FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL;
            num_drives_to_remove[index] = 2;
        }
        else
        {
            num_drives_to_remove[index] = 2;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "remove %d drives for rg %d",
                   num_drives_to_remove[index] - 1, current_rg_config_p->raid_group_id);
        big_bird_remove_all_drives(current_rg_config_p, 1, /* rgs */ num_drives_to_remove[index] - 1,
                                   FBE_TRUE, /* We are pulling and reinserting same drive */
                                   0, /* msec wait between removals */
                                   FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
        current_rg_config_p++;
    }
        
    /* Make sure the raid groups goto fail.
     */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[index];
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_FAIL, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }

    /* Put the drives back in.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting drives ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        for (drive_number = 0; drive_number < num_drives_to_remove[index] - 1; drive_number++)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "== rg: %d inserting drive index %d of %d. ==",
                       current_rg_config_p->raid_group_id, drive_number, num_drives_to_remove[index] - 1);
            big_bird_insert_drives(current_rg_config_p, 1, FBE_FALSE);    /* don't fail pvd */

            mut_printf(MUT_LOG_TEST_STATUS, "== inserting drive index %d of %d. Complete. ==", 
                       drive_number, num_drives_to_remove[index] - 1);
        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting drives successful. ==", __FUNCTION__);

    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        fbe_api_raid_group_get_info_t rg_info;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[index];
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_api_rdgen_io_params_init(&params);
        params.object_id = rg_object_id;
        params.class_id = FBE_CLASS_ID_INVALID;
        params.package_id = FBE_PACKAGE_ID_SEP_0;
        params.msecs_to_abort = 0;
        params.msecs_to_expiration = 0;
        params.block_spec = FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE;

        params.rdgen_operation = FBE_RDGEN_OPERATION_READ_ONLY;
        params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
        params.lba = rg_info.paged_metadata_start_lba;
        params.blocks = 1;
        mut_printf(MUT_LOG_TEST_STATUS, "read paged for rg %d (object 0x%x)",
                   current_rg_config_p->raid_group_id, rg_object_id);
        status = fbe_api_rdgen_send_one_io_params(context_p, &params);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        mut_printf(MUT_LOG_TEST_STATUS, "read paged for rg %d (object 0x%x)...SUCCESS",
                   current_rg_config_p->raid_group_id, rg_object_id);

        status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    big_bird_insert_all_drives(rg_config_p, raid_group_count, 1,
                               FBE_TRUE    /* We are pulling and reinserting same drive */);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        big_bird_wait_for_rebuilds(current_rg_config_p, 1, num_drives_to_remove[index]);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);

    /* Wait for all objects to become ready including raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end strider_shutdown_paged_errors()
 ******************************************/
/*!**************************************************************
 * strider_send_mark_verify()
 ****************************************************************
 * @brief
 *  Kick off an operation to mark some chunks for verify.
 *
 * @param object_id
 * @param opcode
 * @param start_lba
 * @param block_count
 * 
 * @return fbe_status_t
 *
 * @author
 *  12/13/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t strider_send_mark_verify(fbe_object_id_t object_id,
                                             fbe_payload_block_operation_opcode_t opcode,
                                             fbe_lba_t start_lba, 
                                             fbe_block_count_t block_count)
{
    fbe_raid_verify_error_counts_t verify_error_counts = {0};
    fbe_block_transport_block_operation_info_t  block_operation_info;
    fbe_payload_block_operation_t block_operation;
    fbe_status_t status;
    /* Create appropriate payload.
     */
    fbe_payload_block_build_operation(&block_operation,
                                      opcode,
                                      start_lba,
                                      block_count,
                                      520,
                                      1, /* optimum block size */
                                      NULL);
    
    block_operation_info.block_operation = block_operation;
    block_operation_info.verify_error_counts = verify_error_counts;

    mut_printf(MUT_LOG_LOW, "=== sending mark verify I/O object: 0x%x LBA:0x%llx, blocks:%llu\n", object_id, start_lba, block_count);

    status = fbe_api_block_transport_send_block_operation(object_id, 
                                                          FBE_PACKAGE_ID_SEP_0, 
                                                          &block_operation_info, 
                                                          NULL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    return FBE_STATUS_OK;
}
/******************************************
 * end strider_send_mark_verify()
 ******************************************/

/*!**************************************************************
 * strider_spanning_flush_rg_is_enabled()
 ****************************************************************
 * @brief
 *  Determine if this raid group is allowed for this test case.
 *  We only allow non-mirrors.
 *
 * @param rg_config_p
 * 
 * @return fbe_bool_t
 *
 * @author
 *  12/13/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t strider_spanning_flush_rg_is_enabled(fbe_test_rg_configuration_t * const rg_config_p)
{
    if (!fbe_test_rg_config_is_enabled(rg_config_p) || 
        (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) || 
        (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
    {
        return FBE_FALSE;
    }
    return FBE_TRUE;
}
/******************************************
 * end strider_spanning_flush_rg_is_enabled()
 ******************************************/
/*!**************************************************************
 * strider_spanning_flush_test()
 ****************************************************************
 * @brief
 *  Run test where we cause a journal flush to occur to a
 *  range that spans two chunks where one chunk is marked
 *  for verify and the other chunk is not.
 *
 * @param rg_config_p - Current configuration
 * @param raid_group_count - number of rgs under test.
 * @param luns_per_rg - Number of LUNs for each RG.
 * @param peer_options - How to run rdgen I/O local or with peer.            
 *
 * @return None.
 *
 * @author
 *  12/11/2012 - Created. Rob Foley
 *
 ****************************************************************/

void strider_spanning_flush_test(fbe_test_rg_configuration_t * const rg_config_p,
                                 fbe_u32_t raid_group_count,
                                 fbe_u32_t luns_per_rg,
                                 fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_api_rdgen_context_t *context_p = &strider_test_contexts[0];
    fbe_status_t status;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t rg_object_ids[STRIDER_MAX_RGS];
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t num_drives_to_remove[STRIDER_MAX_RGS];
    fbe_test_drive_removal_mode_t removal_mode = FBE_DRIVE_REMOVAL_MODE_RANDOM; /* Default to Random. */
    fbe_api_rdgen_send_one_io_params_t params;
    fbe_u32_t drive_number;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_u32_t wait_time_ms;
    fbe_u32_t total_rgs = 0;

    fbe_api_logical_error_injection_record_t error_record =
    { 
        0x2, /* pos_bitmap */
        0x10, /* width */
        0x0,  /* lba */
        0x1000,  /* blocks */
        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR,  /* error type */
        FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS, /* error mode */
        0x0,  /* error count */
        0x0 , /* error limit */
        0x0,  /* skip count */
        0x0 , /* skip limit */
        0x1,  /* error adjacency */
        0x0,  /* start bit */
        0x0,  /* number of bits */
        0x0,  /* erroneous bits */
        0x0,  /* crc detectable */
        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode  */
    };

    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting peer options: %d==", __FUNCTION__, peer_options);

    big_bird_remove_all_drives(rg_config_p, raid_group_count, 1,
                               FBE_TRUE, /* We are pulling and reinserting same drive */
                               0, /* msec wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

   
    // disable any previous error injection records
    status = fbe_api_logical_error_injection_disable_records( 0, 255 );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // add new error injection record for selected error case
    status = fbe_api_logical_error_injection_create_record( &error_record );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable injection on Raid Groups ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        if (!strider_spanning_flush_rg_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[index];

        /* Enable injection on this rg.
         */
        status = fbe_api_logical_error_injection_enable_object( rg_object_id, FBE_PACKAGE_ID_SEP_0 );
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

        status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RW_START,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
        total_rgs++;
    }

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    status = fbe_api_logical_error_injection_get_stats( &stats );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    MUT_ASSERT_INT_EQUAL( stats.b_enabled, FBE_TRUE );

    /* Make sure we have the expected number of records RGs under test.
     */
    MUT_ASSERT_INT_EQUAL( stats.num_records, 1 );
    MUT_ASSERT_INT_EQUAL( stats.num_objects_enabled, total_rgs);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Send I/Os with error injection enabled. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        fbe_api_raid_group_get_info_t rg_info;
        if (!strider_spanning_flush_rg_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[index];
        context_p = &strider_test_contexts[index];

        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_api_rdgen_io_params_init(&params);
        params.object_id = rg_object_id;
        params.class_id = FBE_CLASS_ID_INVALID;
        params.package_id = FBE_PACKAGE_ID_SEP_0;
        params.msecs_to_abort = 0;
        params.msecs_to_expiration = 0;
        params.block_spec = FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE;

        params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
        params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
        params.lba = (rg_info.num_data_disk * FBE_RAID_DEFAULT_CHUNK_SIZE) - 1;
        params.blocks = rg_info.num_data_disk * rg_info.element_size;
        params.b_async = FBE_TRUE;

        mut_printf(MUT_LOG_TEST_STATUS, "send I/O for rg %d (object 0x%x) lba: 0x%llx bl: 0x%llx",
                   current_rg_config_p->raid_group_id, rg_object_id,
                   params.lba, params.blocks);
        status = fbe_api_rdgen_send_one_io_params(context_p, &params);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "send I/O for rg %d (object 0x%x)...SUCCESS",
                   current_rg_config_p->raid_group_id, rg_object_id);

        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Send I/Os with error injection enabled.  Complete. ==", __FUNCTION__);

    /* Kick off mark verify for first chunk.
     * This causes the I/O in progress to now have a request that spans a 
     * region with one chunk marked for verify and the other not marked. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Mark first chunk for verify. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        fbe_api_raid_group_get_info_t rg_info;
        if (!strider_spanning_flush_rg_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[index];

        mut_printf(MUT_LOG_TEST_STATUS, "mark verify start for object: 0x%x", rg_object_id);
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        strider_send_mark_verify(rg_object_id,
                                 FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_USER_VERIFY,
                                 0, /* lba */ 0x800 /* blocks */);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Mark first chunk for verify.  Complete. ==", __FUNCTION__);

    /* Wait for error to be injected.  This is needed since we want the I/O to fail
     * at the point where it is writing to the live stripe. 
     * Since we only inject the error on the live stripe, the write to the parity write 
     * log will succeed, but the write to the live stripe will fail. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for error injection. ==", __FUNCTION__);
    wait_time_ms = 0;
    while (wait_time_ms < FBE_TEST_WAIT_TIMEOUT_MS)
    {
        status = fbe_api_logical_error_injection_get_stats(&stats);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        if(stats.num_errors_injected >= 1)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "== %s found %llu errors injected ==\n", __FUNCTION__, stats.num_errors_injected);
            break;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for error injection. ==", __FUNCTION__);
        fbe_api_sleep(500);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for error injection.  Complete.==", __FUNCTION__);

    /* Now pull the drive to fail the raid group.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove Drives to fail RGs. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        if (!strider_spanning_flush_rg_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
        {
            num_drives_to_remove[index] = 3;
        }
        else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            /* Raid 10 goes sequential to insure a shutdown.
             */
            removal_mode = FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL;
            num_drives_to_remove[index] = 2;
        }
        else
        {
            num_drives_to_remove[index] = 2;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "remove %d drives for rg %d",
                   num_drives_to_remove[index] - 1, current_rg_config_p->raid_group_id);
        big_bird_remove_all_drives(current_rg_config_p, 1, /* rgs */ num_drives_to_remove[index] - 1,
                                   FBE_TRUE, /* We are pulling and reinserting same drive */
                                   0, /* msec wait between removals */
                                   FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove Drives to fail RGs.  Complete.==", __FUNCTION__);
    
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    /* Disable error injection and make sure the raid groups goto fail.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for RGs to FAIL. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        if (!strider_spanning_flush_rg_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[index];
        context_p = &strider_test_contexts[index];

        status = fbe_api_logical_error_injection_disable_object( rg_object_id, FBE_PACKAGE_ID_SEP_0 );
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_FAIL, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_rdgen_wait_for_ios(context_p, FBE_PACKAGE_ID_NEIT, 1);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_rdgen_test_context_destroy(context_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for RGs to FAIL. Complete.  ==", __FUNCTION__);

    /* Put the drives back in.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting drives ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        if (!strider_spanning_flush_rg_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        for (drive_number = 0; drive_number < num_drives_to_remove[index] - 1; drive_number++)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "== rg: %d inserting drive index %d of %d. ==",
                       current_rg_config_p->raid_group_id, drive_number, num_drives_to_remove[index] - 1);
            big_bird_insert_drives(current_rg_config_p, 1, FBE_FALSE);    /* don't fail pvd */

            mut_printf(MUT_LOG_TEST_STATUS, "== inserting drive index %d of %d. Complete. ==", 
                       drive_number, num_drives_to_remove[index] - 1);
        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting drives successful. ==", __FUNCTION__);

    /* Wait for the RGs to come back online.
     */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        fbe_api_raid_group_get_info_t rg_info;
        if (!strider_spanning_flush_rg_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[index];
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    big_bird_insert_all_drives(rg_config_p, raid_group_count, 1,
                               FBE_TRUE    /* We are pulling and reinserting same drive */);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        if (!strider_spanning_flush_rg_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        
        rg_object_id = rg_object_ids[index];
        status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RW_START,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        big_bird_wait_for_rebuilds(current_rg_config_p, 1, num_drives_to_remove[index]);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);

    /* Wait for all objects to become ready including raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end strider_spanning_flush_test()
 ******************************************/
/*!**************************************************************
 * strider_run_test()
 ****************************************************************
 * @brief
 *  Run error injection test.
 *
 * @param rg_config_p - Config to create.
 * @param unused_context_p - 
 *
 * @return none
 *
 * @author
 *  11/20/2012 - Created. Rob Foley
 *
 ****************************************************************/
void strider_run_test(fbe_test_rg_configuration_t *rg_config_p, void * unused_context_p)
{
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;
    fbe_u32_t luns_per_rg = STRIDER_TEST_LUNS_PER_RAID_GROUP;
    fbe_u32_t raid_group_count =  fbe_test_get_rg_array_length(rg_config_p);

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_u32_t random_number = fbe_random() % 2;
        if (random_number == 0)
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
        }
        else
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing peer_options %d", __FUNCTION__, peer_options);
    }

    /* If we don't wait for initial verify, the system verify will interfere with error verify test later */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p); 

    /* Run a test where we force a condition where we flush spanning a chunk boundary
     * where one chunk is marked for verify and the other is not. 
     */
    strider_spanning_flush_test(rg_config_p, raid_group_count, luns_per_rg, peer_options);

    /* Run a test where we force errors in paged and make sure we can fix them when we are
     * still degraded. 
     */
    strider_shutdown_paged_errors(rg_config_p, raid_group_count, luns_per_rg, peer_options);
}
/******************************************
 * end strider_run_test()
 ******************************************/

/*!**************************************************************
 * strider_test()
 ****************************************************************
 * @brief
 *  Run an error test to a set of raid 5 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/20/2012 - Created. Rob Foley
 *
 ****************************************************************/

void strider_test(void)
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
        rg_config_p = &strider_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &strider_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, strider_run_test,
                                   STRIDER_TEST_LUNS_PER_RAID_GROUP,
                                   STRIDER_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end strider_test()
 ******************************************/
/*!**************************************************************
 * strider_setup()
 ****************************************************************
 * @brief
 *  Setup for a paged error injection test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void strider_setup(void)
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
            rg_config_p = &strider_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &strider_raid_group_config_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();
    }

    /* Do not spare.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(7200/* very long so no spares swap in */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_sep_util_disable_system_drive_zeroing();

    return;
}
/**************************************
 * end strider_setup()
 **************************************/

/*!**************************************************************
 * strider_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the strider test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void strider_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end strider_cleanup()
 ******************************************/
/*************************
 * end file strider_test.c
 *************************/


