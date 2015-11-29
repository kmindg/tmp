/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_test_sep_region_io.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions for region related I/O.
 *
 * @version
 *   9/19/2011:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe_test_common_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_test_setup_region_test_contexts()
 ****************************************************************
 * @brief
 *  Setup the test context for a given LUN to run a region
 *  based test.  This is very similar to a fill pattern, except
 *  that we also
 *
 * @param context_p - Context to init.
 * @param rdgen_operation - Operation to use.
 * @param lun_object_id - lun to send I/O to.
 * @param max_blocks - Max blocks to use.
 * @param max_regions - Max number of regions to populate use.
 * @param capacity - capacity of the region to setup
 * @param peer_options - Invalid if we do not need to use the peer,
 *                       otherwise it indicates how we should use the peer.
 * @param pattern - Pattern for writing.
 *
 * @return None.
 *
 * @author
 *  9/6/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_setup_region_test_contexts(fbe_api_rdgen_context_t *context_p,
                                                fbe_rdgen_operation_t rdgen_operation,
                                                fbe_object_id_t lun_object_id, 
                                                fbe_block_count_t max_blocks, 
                                                fbe_u32_t max_regions,
                                                fbe_lba_t capacity,
                                                fbe_api_rdgen_peer_options_t peer_options,
                                                fbe_data_pattern_t pattern)
{   
    fbe_status_t status;
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                                  lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  rdgen_operation,
                                                  FBE_LBA_INVALID, /* use max capacity */
                                                  16 /* max blocks.  We override this later with regions.  */);

    status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                             peer_options);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification,
                                                        FBE_RDGEN_OPTIONS_USE_REGIONS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Create a random looking region list.
     */
    fbe_data_pattern_create_region_list(&context_p->start_io.specification.region_list[0],
                                        &context_p->start_io.specification.region_list_length,
                                        max_regions,
                                        0, /* start lba */
                                        max_blocks,
                                        capacity,
                                        pattern);
}
/******************************************
 * end fbe_test_setup_region_test_contexts()
 ******************************************/

/*!*******************************************************************
 * @def FBE_TEST_SEP_UTIL_SETUP_REGIONS_RANDOM_CASES
 *********************************************************************
 * @brief  This limits the size of the random number,
 *         Which allows us to pick how frequently various cases occur.
 *
 *********************************************************************/
#define FBE_TEST_SEP_UTIL_SETUP_REGIONS_RANDOM_CASES 20

/*!**************************************************************
 * fbe_test_get_lu_rand_blocks()
 ****************************************************************
 * @brief
 *  determine a random number of blocks we should use as the
 *  maximum for each region.
 *
 * @param max_blocks - Max blocks the caller can send for an I/O.
 * @param capacity - Max capacity of lun
 * @param rg_info_p - The raid group information.
 * @param current_max_blocks_p - Max number of blocks output.
 *
 * @return None. 
 *
 * @author
 *  9/13/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_get_lu_rand_blocks(fbe_block_count_t max_blocks,
                                          fbe_lba_t capacity,
                                          fbe_api_raid_group_class_get_info_t *rg_info_p,
                                          fbe_block_count_t *current_max_blocks_p)
{
    fbe_u32_t random_number;

    random_number = fbe_random() % FBE_TEST_SEP_UTIL_SETUP_REGIONS_RANDOM_CASES;
    /* Randomly select between large I/Os (max blocks) and 
     * smaller I/Os that cover the entire range of the list. 
     */
    if (random_number == 0)
    {
        /* Use very large I/Os.
         */
        *current_max_blocks_p = max_blocks;
    }
    else if (random_number == 1)
    {
        /* This case fills the region list.
         */
        *current_max_blocks_p = (capacity / FBE_RDGEN_IO_SPEC_REGION_LIST_LENGTH) * 2;
    }
    else if (random_number < 10)
    {
        /* We'll perform max of two stripes so we can get I/Os that cross stripes.
         */
        *current_max_blocks_p = (rg_info_p->element_size * rg_info_p->data_disks * 2);
    }
    else    /* random_number >= 10 */
    {
        /* We'll perform max of element size.
         */
        *current_max_blocks_p = rg_info_p->element_size * 2;
    }

    /* We won't create much of a range if the max_blocks is > capacity, since 
     * it will be possible for a single range to cover the entire lun. 
     * Thus, break up the max blocks in this case so we can have more of a range.
     */
    if (max_blocks > capacity)
    {
        *current_max_blocks_p = (capacity / FBE_RDGEN_IO_SPEC_REGION_LIST_LENGTH) * 2;
    }

}
/******************************************
 * end fbe_test_get_lu_rand_blocks()
 ******************************************/
/*!**************************************************************
 * fbe_test_get_random_region_length()
 ****************************************************************
 * @brief
 *  Calculate a random number of region entries to use.
 *
 * @param None.               
 *
 * @return fbe_u32_t length of the region array to use.
 *
 * @author
 *  9/13/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_test_get_random_region_length(void)
{
    fbe_u32_t max_regions;
    fbe_u32_t random_number;

    /* Randomize how many regions we will allow.
     */
    random_number = (fbe_random() % FBE_TEST_SEP_UTIL_SETUP_REGIONS_RANDOM_CASES) + 1;
    max_regions = FBE_RDGEN_IO_SPEC_REGION_LIST_LENGTH / random_number;
    return max_regions;
}
/******************************************
 * end fbe_test_get_random_region_length()
 ******************************************/
/*!***************************************************************************
 *  fbe_test_setup_lun_region_contexts
 *****************************************************************************
 *
 * @brief   Run the rdgen test setup method passed against all luns for all
 *          raid groups specified.
 *
 * @param rg_config_p - Array of configs.
 * @param context_p - Array of contexts.
 * @param num_contexts - Number of contexts.
 * @param rdgen_operation - Operation to perform.
 * @param max_blocks - Max size to perform.
 * @param peer_options - Peer options to set.
 * @param pattern - The type of pattern to use by default.
 * 
 * @return fbe_status_t
 *
 * @note Capacity is the entire capacity of each LUN.
 *
 * @author
 *  8/22/2011 - Created. Rob Foley
 *
 *****************************************************************************/

fbe_status_t fbe_test_setup_lun_region_contexts(fbe_test_rg_configuration_t *rg_config_p,
                                                       fbe_api_rdgen_context_t *context_p,
                                                       fbe_u32_t num_contexts,
                                                       fbe_rdgen_operation_t rdgen_operation,
                                                       fbe_block_count_t max_blocks,
                                                       fbe_api_rdgen_peer_options_t peer_options,
                                                       fbe_data_pattern_t pattern)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_api_rdgen_context_t     *current_context_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_u32_t                   lun_index = 0;
    fbe_object_id_t             lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                   total_luns_processed = 0;
    fbe_block_count_t           current_max_blocks;

    /* Get the number of raid groups to run I/O against
     */
    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Iterate over all the raid groups
     */
    current_rg_config_p = rg_config_p;
    current_context_p = context_p;
    for (rg_index = 0; 
         (rg_index < rg_count) && (status == FBE_STATUS_OK); 
         rg_index++)
    {
        fbe_api_raid_group_class_get_info_t rg_info;
        fbe_u32_t max_regions;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_info.width = current_rg_config_p->width;
        rg_info.raid_type = current_rg_config_p->raid_type;
        rg_info.single_drive_capacity = FBE_U32_MAX;
        rg_info.exported_capacity = FBE_LBA_INVALID; /* set to invalid to indicate not used. */
        status = fbe_api_raid_group_class_get_info(&rg_info, current_rg_config_p->class_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* For this raid group iterate over all the luns
         */
        for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++)
        {
            fbe_lba_t lun_capacity = current_rg_config_p->logical_unit_configuration_list[lun_index].capacity;
            fbe_lun_number_t lun_number = current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number;
            /* Validate that we haven't exceeded the number of context available.
             */
            if (total_luns_processed >= num_contexts) 
            {
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "setup context for all luns: rg_config_p: 0x%p luns processed: %d exceeds contexts: %d",
                           rg_config_p, total_luns_processed, num_contexts); 
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Get the lun object id from the lun number
             */
            MUT_ASSERT_INT_EQUAL(current_rg_config_p->logical_unit_configuration_list[lun_index].raid_group_id, 
                                 current_rg_config_p->raid_group_id);
            status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            
            max_regions = fbe_test_get_random_region_length();
            fbe_test_get_lu_rand_blocks(max_blocks, lun_capacity, &rg_info, &current_max_blocks);

            /* Seup a random looking region array.
             */
            fbe_test_setup_region_test_contexts(current_context_p, rdgen_operation, lun_object_id,
                                                       current_max_blocks, max_regions, lun_capacity, peer_options, pattern );

            /* Increment the number of luns processed and the test context pointer
             */
            total_luns_processed++;
            current_context_p++;

        } /* end for all luns in a raid group */

        /* Goto next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups configured */

    /* Else return success
     */
    return FBE_STATUS_OK;
}
/************************************************************
 * end fbe_test_setup_lun_region_contexts()
 ************************************************************/

/*!**************************************************************
 * fbe_test_setup_dual_region_contexts()
 ****************************************************************
 * @brief
 *  Setup two region contexts.  One context is for the read
 *  that combines regions with a background pattern and
 *  the foreground pattern.
 * 
 *  The write context only includes the foreground write of a
 *  pattern.
 *  
 * @param rg_config_p - The raid group config under test.
 * @param peer_options - Peer options for I/O.
 * @param background_pattern - Background pattern caller set on lun.
 * @param pattern - foreground pattern caller wants put into the
 *                  write test context.
 * @param read_context_p - read that includes background pattern and
 *                         foreground write pattern.
 * @param write_context_p - write of foreground pattern.
 * @param num_luns - Number of LUNs under test.
 * @param background_pass_count - Pass count for background pattern.
 * @param max_io_size - I/O size to use.
 *
 * @return None.
 *
 * @author
 *  9/19/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_setup_dual_region_contexts(fbe_test_rg_configuration_t *const rg_config_p, 
                                         fbe_api_rdgen_peer_options_t peer_options, 
                                         fbe_data_pattern_t background_pattern, 
                                         fbe_data_pattern_t pattern, 
                                         fbe_api_rdgen_context_t **read_context_pp, 
                                         fbe_api_rdgen_context_t **write_context_pp, 
                                         fbe_u32_t background_pass_count,
                                         fbe_u32_t max_io_size)
{
    fbe_status_t status; 
    fbe_u32_t index;
    fbe_u32_t num_luns;
    fbe_api_rdgen_context_t *read_context_p = NULL; 
    fbe_api_rdgen_context_t *write_context_p = NULL; 

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Allocate the context to read continually during the test.
     */
    status = fbe_test_sep_io_allocate_rdgen_test_context_for_all_luns(&read_context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_setup_lun_region_contexts(rg_config_p, read_context_p, num_luns,
                                                FBE_RDGEN_OPERATION_READ_CHECK,
                                                max_io_size, peer_options,
                                                background_pattern);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the context we will use to write holes in our background pattern.
     */
    status = fbe_test_sep_io_allocate_rdgen_test_context_for_all_luns(&write_context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_setup_lun_region_contexts(rg_config_p, write_context_p, num_luns,
                                                FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                max_io_size, peer_options,
                                                background_pattern);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    for (index = 0; index < num_luns; index++)
    {
        /* Fill a pattern into our regions.
         * This gives us our overall set of regions we will compare to. 
         */
        fbe_data_pattern_region_list_set_random_pattern(&read_context_p[index].start_io.specification.region_list[0],
                                                        read_context_p[index].start_io.specification.region_list_length,
                                                        pattern, background_pattern, background_pass_count,
                                                        FBE_DATA_PATTERN_SP_ID_A    /* background pattern writer */);
        status = fbe_data_pattern_region_list_validate(&read_context_p[index].start_io.specification.region_list[0],
                                                       read_context_p[index].start_io.specification.region_list_length);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Since the background pattern is already set, we only need to write for the areas that do not have the
         * background pattern.  Extract these entries into our write context 
         */
        fbe_data_pattern_region_list_copy_pattern(&read_context_p[index].start_io.specification.region_list[0],
                                                  read_context_p[index].start_io.specification.region_list_length,
                                                  &write_context_p[index].start_io.specification.region_list[0],
                                                  &write_context_p[index].start_io.specification.region_list_length,
                                                  pattern,    /* Pattern to match on */
                                                  /* Exclude the background pattern pass count when we have matching
                                                     patterns. */
                                                  (pattern == background_pattern), 
                                                  /* Exclude the background pass count if the pattern is not enough
                                                     to search by.  Otherwise we ignore the pass count in the match */
                                                  (pattern == background_pattern) ? background_pass_count : FBE_U32_MAX);

        status = fbe_data_pattern_region_list_validate(&read_context_p[index].start_io.specification.region_list[0],
                                                       read_context_p[index].start_io.specification.region_list_length);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    *write_context_pp = write_context_p;
    *read_context_pp = read_context_p;
}
/******************************************
 * end fbe_test_setup_dual_region_contexts()
 ******************************************/
/*!**************************************************************
 * fbe_test_run_foreground_region_write()
 ****************************************************************
 * @brief
 *  Run a foreground write, where the read context is already populated
 *  with a background pattern.
 *
 * @param rg_config_p - The raid group config under test.
 * @param read_context_p - read that includes background pattern and
 *                         foreground write pattern.
 * @param write_context_p - write of foreground pattern.
 *
 * @return None
 *
 * @author
 *  9/19/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_run_foreground_region_write(fbe_test_rg_configuration_t *const rg_config_p, 
                                          fbe_data_pattern_t background_pattern, 
                                          fbe_api_rdgen_context_t * read_context_p, 
                                          fbe_api_rdgen_context_t * write_context_p)
{
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_u32_t index;
    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Run write the non background pattern entries.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s write pattern %d ==", __FUNCTION__, background_pattern);
    status = fbe_api_rdgen_test_context_run_all_tests(write_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(write_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_reinit(write_context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s write pattern %d. Complete. ==", __FUNCTION__, background_pattern);

    for (index = 0; index < num_luns; index++)
    {
        /* Validate source.
         */
        status = fbe_data_pattern_region_list_validate(&write_context_p[index].start_io.specification.region_list[0],
                                                       write_context_p[index].start_io.specification.region_list_length);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Get the sp id that wrote the pattern into the read's list.
         */
        status = fbe_data_pattern_region_list_copy_sp_id(&write_context_p[index].start_io.specification.region_list[0],
                                                         write_context_p[index].start_io.specification.region_list_length,
                                                         &read_context_p[index].start_io.specification.region_list[0],
                                                         read_context_p[index].start_io.specification.region_list_length);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Validate dest.
         */
        status = fbe_data_pattern_region_list_validate(&read_context_p[index].start_io.specification.region_list[0],
                                                       read_context_p[index].start_io.specification.region_list_length);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_data_pattern_region_list_validate_pair(&write_context_p[index].start_io.specification.region_list[0],
                                                            write_context_p[index].start_io.specification.region_list_length,
                                                            &read_context_p[index].start_io.specification.region_list[0],
                                                            read_context_p[index].start_io.specification.region_list_length);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Destroy the context semaphore when done.. */
    for ( index = 0; index < num_luns; index++)
    {
        fbe_semaphore_destroy(&write_context_p->semaphore);
        write_context_p++;
    }

}
/******************************************
 * end fbe_test_run_foreground_region_write()
 ******************************************/

/*!***************************************************************************
 *  fbe_test_setup_active_region_context
 *****************************************************************************
 *
 * @brief
 *  Setup the region for local or peer based on the active SP for this RG.
 *
 * @param rg_config_p - Array of configs.
 * @param context_p - Array of contexts.
 * @param num_contexts
 * @param b_active - FBE_TRUE to run I/O on active side, FBE_FALSE for passive.
 * 
 * @return fbe_status_t
 *
 * @note Capacity is the entire capacity of each LUN.
 *
 * @author
 *  11/12/2013 - Created. Rob Foley
 *
 *****************************************************************************/

fbe_status_t fbe_test_setup_active_region_context(fbe_test_rg_configuration_t *rg_config_p,
                                                  fbe_api_rdgen_context_t *context_p,
                                                  fbe_u32_t num_contexts,
                                                  fbe_bool_t b_active)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_api_rdgen_context_t     *current_context_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_u32_t                   lun_index = 0;
    fbe_u32_t                   total_luns_processed = 0;
    fbe_sim_transport_connection_target_t  active_sp;
    fbe_sim_transport_connection_target_t  passive_sp;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_object_id_t rg_object_id; 

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();

    /* Get the number of raid groups to run I/O against
     */
    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Iterate over all the raid groups
     */
    current_rg_config_p = rg_config_p;
    current_context_p = context_p;
    for (rg_index = 0; 
         (rg_index < rg_count) && (status == FBE_STATUS_OK); 
         rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_test_sep_util_get_active_passive_sp(rg_object_id, &active_sp, &passive_sp);
        fbe_api_sim_transport_set_target_server(active_sp);

        /* For this raid group iterate over all the luns
         */
        for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++){
            /* Validate that we haven't exceeded the number of context available.
             */
            if (total_luns_processed >= num_contexts) {
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "setup context for all luns: rg_config_p: 0x%p luns processed: %d exceeds contexts: %d",
                           rg_config_p, total_luns_processed, num_contexts); 
                return FBE_STATUS_GENERIC_FAILURE;
            }
            if ( ((active_sp == this_sp) && b_active) ||
                 ((passive_sp == this_sp) && !b_active) ) {
                status = fbe_api_rdgen_io_specification_set_peer_options(&current_context_p->start_io.specification,
                                                                         FBE_RDGEN_PEER_OPTIONS_INVALID);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            } else {

                status = fbe_api_rdgen_io_specification_set_peer_options(&current_context_p->start_io.specification,
                                                                         FBE_RDGEN_PEER_OPTIONS_SEND_THRU_PEER);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
            /* Increment the number of luns processed and the test context pointer
             */
            total_luns_processed++;
            current_context_p++;

        } /* end for all luns in a raid group */

        /* Goto next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups configured */
    fbe_api_sim_transport_set_target_server(this_sp);
    /* Else return success
     */
    return FBE_STATUS_OK;
}
/************************************************************
 * end fbe_test_setup_active_region_context()
 ************************************************************/
/*************************
 * end file fbe_test_sep_region_io.c
 *************************/


