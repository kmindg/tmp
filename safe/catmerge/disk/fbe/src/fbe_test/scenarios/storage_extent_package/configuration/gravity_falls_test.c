
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file gravity_falls_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests RG Expansion
 *
 *
 ***************************************************************************/


/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/

#include "sep_tests.h"
#include "sep_utils.h"
#include "sep_hook.h"
#include "sep_test_io.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_random.h"


/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char* gravity_falls_short_desc = "RG Expansion";
char* gravity_falls_long_desc =
    "\n"
    "\n"
    "The Gravity test is checking the ability to do RG Expansion, including PSL RG Expansion:\n"
    "\n"
    "\n"
    "STEP 1: Exapand a regular RG\n"
    "STEP 2: Expans the 3 way mirror\n"
    "\n"
    "Description last updated: 10/4/2013.\n";        

/*!*******************************************************************
 * @def GRAVITY_FALLS_DUALSP_TEST_NEW_SEP_VERSION
 *********************************************************************
 * @brief new SEP version we want to set.
 *
 *********************************************************************/
#define GRAVITY_FALLS_DUALSP_TEST_NEW_SEP_VERSION 20140121

/*!*******************************************************************
 * @def GRAVITY_FALLS_TEST_MAX_RG_WIDTH
 *********************************************************************
 * @brief Max number of drives in a RG
 *
 *********************************************************************/
#define GRAVITY_FALLS_TEST_MAX_RG_WIDTH               16

/*!*******************************************************************
 * @def GRAVITY_FALLS_MAX_DATABASE_DRIVE_SLOT
 *********************************************************************
 * @brief Max system drive slot
 *
 *********************************************************************/
#define GRAVITY_FALLS_MAX_DATABASE_DRIVE_SLOT      (FBE_PRIVATE_SPACE_LAYOUT_MAX_FRUS_PER_REGION - 1)

/*!*******************************************************************
 * @def GRAVITY_FALLS_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
#define GRAVITY_FALLS_TEST_NUMBER_OF_PHYSICAL_OBJECTS 34 


/*!*******************************************************************
 * @def GRAVITY_FALLS_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define GRAVITY_FALLS_TEST_DRIVE_CAPACITY             (TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + 0x40000)

/*!*******************************************************************
 * @def PLANKTON_TEST_LUN_CHUNK_SIZE
 *********************************************************************
 * @brief LUN chunk size
 *
 *********************************************************************/
#define GRAVITY_FALLS_TEST_LUN_CHUNK_SIZE             0x800

/*!*******************************************************************
 * @def  GRAVITY_FALLS_MAX_RG_SIZE
 *********************************************************************
 * @brief max size of a raid group.
 *
 *********************************************************************/
#define GRAVITY_FALLS_MAX_RG_SIZE 100 * GRAVITY_FALLS_TEST_LUN_CHUNK_SIZE


#define GRAVITY_FALLS_TEST_RAID_GROUP_COUNT 1
#define GRAVITY_FALLS_TEST_PRIVATE_RAID_GROUP_COUNT fbe_private_space_layout_get_number_of_system_raid_groups()

#define GRAVITY_FALLS_TEST_REMOVE_DRIVE_POS         2

#define GRAVITY_FALLS_TEST_ELEMENT_SIZE 128 

/*!*******************************************************************
 * @var gravity_falls_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *        This array is used to test the expansion of a user raid group.
 *
 *********************************************************************/
fbe_test_rg_configuration_t gravity_falls_rg_config[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {4,       GRAVITY_FALLS_MAX_RG_SIZE,     FBE_RAID_GROUP_TYPE_RAID5,    FBE_CLASS_ID_STRIPER,    512,            1,          0,
     {{0,0,10}, {0,0,11}, {0,0,12}, {0,0,13}}},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,/* Terminator. */},
};

/*!*******************************************************************
 * @var gravity_falls_private_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *        This array is used to test the expansion of a user raid group.
 *
 *********************************************************************/
static fbe_test_rg_configuration_t gravity_falls_private_raid_group_config[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {4,       GRAVITY_FALLS_MAX_RG_SIZE,     FBE_RAID_GROUP_TYPE_RAID5,    FBE_CLASS_ID_PARITY,    520,            0,          0,
     {{0,0,0}, {0,0,1}, {0,0,2}, {0,0,3}}},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,/* Terminator. */},
};

/*!*******************************************************************
 * @def GRAVITY_FALLS_TEST_LUN_COUNT
 *********************************************************************
 * @brief  number of luns to be created
 *
 *********************************************************************/
#define GRAVITY_FALLS_TEST_LUN_COUNT        3


/*!*******************************************************************
 * @def GRAVITY_FALLS_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define GRAVITY_FALLS_TEST_NS_TIMEOUT        120000 /*wait maximum of 120 seconds*/

/************************************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 ***********************************************************************************/


/* The different test numbers.
 */
typedef enum gravity_falls_test_enclosure_num_e
{
    GRAVITY_FALLS_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    GRAVITY_FALLS_TEST_ENCL2_WITH_SAS_DRIVES,
    GRAVITY_FALLS_TEST_ENCL3_WITH_SAS_DRIVES,
    GRAVITY_FALLS_TEST_ENCL4_WITH_SAS_DRIVES
} gravity_falls_test_enclosure_num_t;

/* The different drive types.
 */
typedef enum gravity_falls_test_drive_type_e
{
    SAS_DRIVE,
} gravity_falls_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    gravity_falls_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    gravity_falls_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;

typedef  struct gravity_falls_dualsp_test_ndu_commit_context_s
{
    fbe_semaphore_t     sem;
    fbe_job_service_info_t    job_srv_info;
} gravity_falls_dualsp_test_ndu_commit_context_t;

#if 0 /* Unused and linux build does not like it */

/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        GRAVITY_FALLS_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        GRAVITY_FALLS_TEST_ENCL2_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        GRAVITY_FALLS_TEST_ENCL3_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        GRAVITY_FALLS_TEST_ENCL4_WITH_SAS_DRIVES,
        520
    },
};

/* Count of rows in the table.
 */
#define GRAVITY_FALLS_TEST_ENCL_MAX sizeof(encl_table)/sizeof(encl_table[0])

static fbe_sim_transport_connection_target_t gravity_falls_dualsp_pass_config[] =
{
    FBE_SIM_SP_A, FBE_SIM_SP_B,
};

#define GRAVITY_FALLS_MAX_PASS_COUNT (sizeof(gravity_falls_dualsp_pass_config)/sizeof(gravity_falls_dualsp_pass_config[0]))
#endif

/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/

static fbe_status_t gravity_falls_test_create_lun(fbe_lun_number_t lun_number, fbe_lba_t capacity);
static fbe_status_t gravity_falls_test_destroy_lun(fbe_lun_number_t lun_number);
static void gravity_falls_test_create_rg(void);
static void gravity_falls_test_load_physical_config(void);
static void gravity_falls_read_background_pattern(fbe_rdgen_pattern_t pattern,
                                                  fbe_object_id_t object_id);
static void gravity_falls_write_background_pattern(fbe_object_id_t object_id);
static fbe_status_t gravity_falls_wait_for_rg_degraded(fbe_object_id_t rg_object_id,
                                                       fbe_u32_t wait_seconds);
static void gravity_falls_start_io(fbe_object_id_t object_id,
                                   fbe_api_rdgen_context_t *context_p,
                                   fbe_bool_t b_dualsp_mode);
/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/
/*!****************************************************************************
 *          gravity_falls_pull_drive()
 ******************************************************************************
 *
 * @brief   Simply pull the drive specified.
 *
 * @param   rg_object_id - Raid group object id to degrade
 * @param   bus - bus of drive to remove
 * @param   enclosure - enclosure of drive to remove
 * @param   slot 
 * @param   b_wait_for_fail_state - FBE_TRUE wait for fail state
 *
 * @return  none
 *
 *****************************************************************************/
static void gravity_falls_pull_drive(fbe_object_id_t rg_object_id,
                                     fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot,
                                     fbe_bool_t b_wait_for_fail_state)
{
    fbe_status_t    status;
    fbe_object_id_t pvd_object_id;

    /* Pull a drive and wait for it to go away.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s degrade rg obj: 0x%x by pulling: %d_%d_%d",
               __FUNCTION__, rg_object_id, bus, enclosure, slot);
    if (fbe_test_sep_util_get_dualsp_test_mode()){
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_test_pp_util_pull_drive(bus, enclosure, slot,
                                             &gravity_falls_private_raid_group_config[0].drive_handle[0]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_pull_drive(bus, enclosure, slot,
                                         &gravity_falls_private_raid_group_config[0].drive_handle[0]);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location(bus, enclosure, slot, &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (b_wait_for_fail_state) {
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              pvd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                                              FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return;
}
/********************************** 
 * end gravity_falls_pull_drive()
 **********************************/

/*!****************************************************************************
 *          gravity_falls_reinsert_drive()
 ******************************************************************************
 *
 * @brief   Simply re-insert the drive specified.
 *
 * @param   rg_object_id - Raid group object id to make optimal
 * @param   bus - bus of drive to remove
 * @param   enclosure - enclosure of drive to remove
 * @param   slot 
 * @param   b_wait_for_ready_state - FBE_TRUE
 *
 * @return  none
 *
 *****************************************************************************/
static void gravity_falls_reinsert_drive(fbe_object_id_t rg_object_id,
                                         fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot,
                                         fbe_bool_t b_wait_for_ready_state)
{
    fbe_status_t    status;
    fbe_object_id_t pvd_object_id;

    /* Pull a drive and wait for it to go away.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s make rg obj: 0x%x optimal by inserting: %d_%d_%d",
               __FUNCTION__, rg_object_id, bus, enclosure, slot);
    if (fbe_test_sep_util_get_dualsp_test_mode()){
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_test_pp_util_reinsert_drive(bus, enclosure, slot,
                                                 gravity_falls_private_raid_group_config[0].drive_handle[0]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_reinsert_drive(bus, enclosure, slot,
                                             gravity_falls_private_raid_group_config[0].drive_handle[0]);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location(bus, enclosure, slot, &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (b_wait_for_ready_state) {
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              pvd_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                              FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return;
}
/********************************** 
 * end gravity_falls_reinsert_drive()
 **********************************/

/*!****************************************************************************
 *          gravity_falls_set_pvd_paged_pre_expansion()
 ******************************************************************************
 *
 * @brief   This routine set the pvd paged bits for the associated range to
 *          values that they would have been prior to installing Rockies MR1.
 *          (i.e. after the ICA to Rockies since they would not have been
 *           modified since the ICA because they are in the unconsumed space)
 *
 * @param   max_slot - The last system drive (from 0_0_0) to set.
 * @param   user_start_lba - Starting user space lba that associated pvd bits
 *              must be modified for.
 * @param   user_blocks_to_modify - How many user blocks to set pvd paged bits
 *              for.
 *
 * @return  none
 *
 * @author
 *  01/20/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void gravity_falls_set_pvd_paged_pre_expansion(fbe_u32_t max_slot,
                                                      fbe_lba_t user_start_lba,
                                                      fbe_block_count_t user_blocks_to_modify)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               slot;
    fbe_object_id_t                         pvd_object_id;
    fbe_u64_t                               metadata_offset;
    fbe_u64_t                               repeat_count;
    fbe_api_provision_drive_info_t          pvd_info;
    fbe_provision_drive_paged_metadata_t    paged_metadata_bits = { 0 };
	fbe_api_base_config_metadata_paged_change_bits_t paged_change_bits;

    /* Simply divide the request range by the pvd chunk size.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, max_slot, &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_info(pvd_object_id, &pvd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    metadata_offset = user_start_lba / pvd_info.chunk_size;
    repeat_count = (user_blocks_to_modify + (pvd_info.chunk_size - 1)) / pvd_info.chunk_size;

    /* Set the pvd area associated with the too be expanded area to the state that
     * ICA would have left it:
     *  o valid_bit:                ==> 1
     *  o need_zero_bit:            ==> 0
     *  o user_zero_bit:            ==> 0
     *  o consumed_user_data_bit:   ==> 0
     */
    paged_metadata_bits.valid_bit = 1;
    paged_metadata_bits.need_zero_bit = 0;
    paged_metadata_bits.user_zero_bit = 0;
    paged_metadata_bits.consumed_user_data_bit = 0;

    /* Trace some information.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Set %d_%d_%d thru %d_%d_%d start lba: 0x%llx blks: 0x%llx ==",
               __FUNCTION__, 0, 0, 0, 0, 0, max_slot,
               user_start_lba, user_blocks_to_modify);
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== set pvd chunk info index: 0x%llx thru 0x%llx",
               metadata_offset, (metadata_offset + repeat_count - 1));

    /* Configure the clear bits.  The metadata_offset is a byte offset so
     * multiply by the size.
     */
    paged_change_bits.metadata_offset = metadata_offset * sizeof(fbe_provision_drive_paged_metadata_t);
    paged_change_bits.metadata_record_data_size = sizeof(fbe_provision_drive_paged_metadata_t);
    paged_change_bits.metadata_repeat_count = repeat_count;
    paged_change_bits.metadata_repeat_offset = 0;

    /* Since this is a `clear bits' set the flag for each bit to clear: 
     *  o Clear valid_bit ?             ==> 0   No: Do not clear the valid bit  
     *  o Clear need_zero_bit ?         ==> 1   Yes: Clear the Needs Zero bit (if set)
     *  o Clear user_zero_bit ?         ==> 1   Yes: Clear the User Zero bit
     *  o Clear consumed_user_data_bit? ==> 1   Yes: Clear the consumed bit
     */
    paged_metadata_bits.valid_bit = 0;
    paged_metadata_bits.need_zero_bit = 1;
    paged_metadata_bits.user_zero_bit = 1;
    paged_metadata_bits.consumed_user_data_bit = 1;
    *((fbe_provision_drive_paged_metadata_t *)paged_change_bits.metadata_record_data) = paged_metadata_bits;

    /* For each disk set the bits.
     */
    for (slot = 0; slot <= max_slot; slot++) {
        status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, slot, &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Clear the NZ and CU bits */
        status = fbe_api_base_config_metadata_paged_clear_bits(pvd_object_id, &paged_change_bits);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    return;
}
/************************************************* 
 * end gravity_falls_set_pvd_paged_pre_expansion()
 *************************************************/

/*!****************************************************************************
 *          gravity_falls_reduce_private_raid_group_capacity()
 ******************************************************************************
 *
 * @brief   Reduce the capacity of the raid group 
 *
 * @param   rg_object_id - The private raid group object id to shrink.
 * @param   reduced_exported_capacity - The desired reduced exported capacity
 * @param   commit_start_ms - The time that the commit was started
 *
 * @return  None 
 *
 *****************************************************************************/
static void gravity_falls_reduce_private_raid_group_capacity(fbe_object_id_t rg_object_id,
                                                             fbe_lba_t reduced_exported_capacity_percentage,
                                                             fbe_time_t commit_start_ms) 
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_time_t                      elapsed_ms;
    fbe_u32_t                       wait_period_ms = ((3 * 2) * 1000);
    fbe_semaphore_t                 wait_for_reduce_sem;
    fbe_private_space_layout_region_t region;
    fbe_api_raid_group_get_info_t   rg_info;
    fbe_api_raid_group_get_paged_info_t original_summary;
    fbe_lba_t                       orig_paged_metadata_lba;
    fbe_lba_t                       reduced_exported_capacity;

    /* Get the region and raid group information prior to metadata shrink.
     */
    status = fbe_private_space_layout_get_region_by_object_id(rg_object_id, &region);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    orig_paged_metadata_lba = rg_info.paged_metadata_start_lba;

    /* Calculate the reduced capacity and align it (6144 is the chunk size * data drives)
    */
    mut_printf(MUT_LOG_TEST_STATUS, "== percentage: %llx ==\n", 
               reduced_exported_capacity_percentage);
    reduced_exported_capacity = ((rg_info.capacity * reduced_exported_capacity_percentage)/ (0x64 * 0x1800)) * 0x1800;

    /* Reduce capacity of raid group by some constant amount.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== reducing RG object: 0x%x from 0x%llx to 0x%llx ==\n", 
               rg_object_id,
               rg_info.capacity,
               reduced_exported_capacity);

    /* We must handle the case where the reduction fails.
     */
    status = FBE_STATUS_TIMEOUT;
    elapsed_ms = fbe_get_elapsed_milliseconds(commit_start_ms);
    fbe_semaphore_init(&wait_for_reduce_sem, 0, 1);

    /* Wait up to timeout or success of raid group expansion.
     */
    while ((status != FBE_STATUS_OK)               &&
           (elapsed_ms < FBE_TEST_WAIT_TIMEOUT_MS)    ) {
        /* Send the request.
         */
        status = fbe_api_raid_group_expand(rg_object_id, reduced_exported_capacity, reduced_exported_capacity);
        if (status != FBE_STATUS_OK) {
            /* Wait at least (2) monitor cycles then retry */
            mut_printf(MUT_LOG_TEST_STATUS,
                       "Retry shrink of %d. Old size: 0x%llx New Size: 0x%llx",
                       region.raid_info.raid_group_id, 
                       rg_info.capacity, reduced_exported_capacity);

            /* Wait for condition to clear
             */
            fbe_semaphore_wait_ms(&wait_for_reduce_sem, wait_period_ms);
            elapsed_ms = fbe_get_elapsed_milliseconds(commit_start_ms);
        }
    }

    /* Free the semaphore.
     */
    fbe_semaphore_destroy(&wait_for_reduce_sem);

    /* If we waited and the expand still failed return an error.
     */
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
    
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_api_raid_group_get_info(rg_object_id,
                                             &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (rg_info.capacity != reduced_exported_capacity)
        {
            MUT_FAIL_MSG("capacity not reduced as expected");
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "== reduced RG object: 0x%x to 0x%llx ==\n", 
               rg_object_id,
               reduced_exported_capacity);
        }
        if (rg_info.paged_metadata_start_lba >= orig_paged_metadata_lba)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "paged is at 0x%llx", 
                       rg_info.paged_metadata_start_lba);
            MUT_FAIL_MSG("paged is not at expected lba after expansion");
        }
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_raid_group_get_info(rg_object_id,
                                         &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (rg_info.capacity != reduced_exported_capacity)
    {
        MUT_FAIL_MSG("capacity not reduced as expected");
    }
    if (rg_info.paged_metadata_start_lba >= orig_paged_metadata_lba)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "paged is at 0x%llx", 
                   rg_info.paged_metadata_start_lba);
        MUT_FAIL_MSG("paged is not at expected lba after expansion");
    }

    /* Validate before expansion */
    mut_printf(MUT_LOG_TEST_STATUS, "validate paged bits before expansion");
    status = fbe_api_raid_group_get_paged_bits(rg_object_id, &original_summary, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (original_summary.num_error_vr_chunks != 0) {
        MUT_FAIL_MSG("error verify chunks present!!");
    }
    if (original_summary.needs_rebuild_bitmask != 0) {
        MUT_FAIL_MSG("nr is incorrect!!");
    }

    return;
}
/********************************************************
 * end gravity_falls_reduce_private_raid_group_capacity()
 ********************************************************/

/*!**************************************************************
 * gravity_falls_ndu_commit_callback_function()
 ****************************************************************
 * @brief
 *  callback function when ndu commit is finished.
 *
 * @param update_obj_msg - job commit/rollback sends this message.               
 * @param context - cassie_dualsp_test_ndu_commit_context we registered.        
 *
 * @return None.
 *
 ****************************************************************/
static void gravity_falls_ndu_commit_callback_function(update_object_msg_t *update_obj_msg,
                                                       void *context)
{
    gravity_falls_dualsp_test_ndu_commit_context_t *commit_context = (gravity_falls_dualsp_test_ndu_commit_context_t *)context;

    MUT_ASSERT_TRUE(NULL != update_obj_msg);
    MUT_ASSERT_TRUE(NULL != context);

    if (FBE_JOB_TYPE_SEP_NDU_COMMIT != update_obj_msg->notification_info.notification_data.job_service_error_info.job_type) {
        return;
    }

    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s - job type: 0x%x complete",
               __FUNCTION__, update_obj_msg->notification_info.notification_data.job_service_error_info.job_type);

    commit_context->job_srv_info = update_obj_msg->notification_info.notification_data.job_service_error_info;
    fbe_semaphore_release(&commit_context->sem, 0, 1, FALSE);
    return;
}
/********************************************************
 * end gravity_falls_ndu_commit_callback_function()
 ********************************************************/

/*!**************************************************************
 * gravity_falls_dualsp_get_and_verify_system_db_header()
 ****************************************************************
 * @brief
 *  get system db headers from both SPs
 *  verify they are equal to each other
 *
 * @param none.               
 *
 * @return  none
 *
 * @author
 *  01/21/2014  Ron Proulx  
 *
 ****************************************************************/
static void gravity_falls_dualsp_get_and_verify_system_db_header(void)
{
    fbe_status_t    status;
    database_system_db_header_t spa_db_header;
    database_system_db_header_t spb_db_header;
    fbe_sim_transport_connection_target_t original_target;

    original_target = fbe_api_sim_transport_get_target_server();

    /************read system db header from SPA************/    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
    status = fbe_api_database_get_system_db_header(&spa_db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~get SPA's system db header failed~~~");

    /************read system db header from SPB************/    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    
    status = fbe_api_database_get_system_db_header(&spb_db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~get SPB's system db header failed~~~");

    fbe_api_sim_transport_set_target_server(original_target);

    /************compare system db headers read from two sides************/    
    if(spa_db_header.magic_number != spb_db_header.magic_number
       || spa_db_header.persisted_sep_version != spb_db_header.persisted_sep_version
       || spa_db_header.version_header.size != spb_db_header.version_header.size) {
        MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Database versions on SPs don't match")
    }

    /*verify the system db header version is not the new one*/
    MUT_ASSERT_UINT64_NOT_EQUAL(GRAVITY_FALLS_DUALSP_TEST_NEW_SEP_VERSION, 
                                spa_db_header.persisted_sep_version); 
    return;
}
/************************************************************
 * end gravity_falls_dualsp_get_and_verify_system_db_header()
 ************************************************************/

/*!***************************************************************************
 *          gravity_falls_test_dualsp_validate_expansion_during_commit()
 *****************************************************************************
 *
 * @brief   This routine first shrinks all (3) of the private raid groups
 *          that have been expanded for MR1.  Then it commits the package.
 *          The commit will expand all three riad groups.
 *
 * @param none.               
 *
 * @return None.
 *
 * @author
 *  01/21/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void gravity_falls_test_dualsp_validate_expansion_during_commit(void)
{
    fbe_status_t                        status;
    gravity_falls_dualsp_test_ndu_commit_context_t commit_context;
    database_system_db_header_t         updated_system_db_header;
    fbe_notification_registration_id_t  reg_id;
    fbe_sim_transport_connection_target_t original_target;
    database_system_db_header_t         spa_db_header;
    fbe_time_t                          commit_start_ms;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Gravity Falls dualsp Expansion During Commit ===\n");

    /* Get the current header.*/
    original_target = fbe_api_sim_transport_get_target_server();
    status = fbe_api_database_get_system_db_header(&spa_db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~get SPA's system db header failed~~~");

    /* Get the commit start time in milliseconds
     */
    commit_start_ms = fbe_get_time();

    /* First shrink the (2) raid groups that will be expanded in MR1
     */
    gravity_falls_reduce_private_raid_group_capacity(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, 0x4b, commit_start_ms);
    gravity_falls_reduce_private_raid_group_capacity(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG, 0x45, commit_start_ms);

    /*set system db header to new version in SPA*/
    mut_printf(MUT_LOG_TEST_STATUS, "===second set system db headers in SPA to a new version===");
    updated_system_db_header = spa_db_header;
    updated_system_db_header.persisted_sep_version = GRAVITY_FALLS_DUALSP_TEST_NEW_SEP_VERSION;
    updated_system_db_header.version_header.size = spa_db_header.version_header.size;
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_set_system_db_header(&updated_system_db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~set system db header to new version failed~~~\n");

    /* Add a hook to validate retry is required
     */
    mut_printf(MUT_LOG_TEST_STATUS, "add a debug hook to validate expansion error");
    status = fbe_test_add_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                                            FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_QUIESCE_ERROR, 
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*register notification about ndu commit job*/
    fbe_semaphore_init(&commit_context.sem, 0, 1);
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED, 
        FBE_PACKAGE_NOTIFICATION_ID_SEP_0, FBE_TOPOLOGY_OBJECT_TYPE_ALL, 
        gravity_falls_ndu_commit_callback_function, &commit_context, &reg_id);    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Add a hook to force (1) retry (no need to wait).
     */
    mut_printf(MUT_LOG_TEST_STATUS, "add a debug hook to force quiesce error");
    status = fbe_test_add_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                                            FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_ERROR_QUIESCE, 
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_ONE_SHOT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*send ndu commit to database service*/
    status = fbe_api_database_commit();
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "~~~commit system db header failed~~~");

    /* Wait for the debug hook*/
    status = fbe_test_wait_for_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                                                 FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_QUIESCE_ERROR, 
                                                 SCHEDULER_CHECK_STATES, 
                                                 SCHEDULER_DEBUG_ACTION_LOG, 
                                                 0, 0); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Remove debug hooks and wait for job to complete*/
    status = fbe_test_del_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                                            FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_QUIESCE_ERROR, 
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_del_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                                            FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_ERROR_QUIESCE, 
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_ONE_SHOT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*wait notification from ndu commit job*/
    status = fbe_semaphore_wait_ms(&commit_context.sem, FBE_TEST_WAIT_TIMEOUT_MS);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,  "~~~wait ndu commit job failed~~~");
    status = fbe_api_notification_interface_unregister_notification(gravity_falls_ndu_commit_callback_function,
                                                                    reg_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~unregister notification failed~~~");
    fbe_semaphore_destroy(&commit_context.sem);

    gravity_falls_dualsp_get_and_verify_system_db_header();

    return;   
}
/*********************************************************** 
 * end gravity_falls_test_dualsp_validate_expansion_during_commit()
 ***********************************************************/

/*!****************************************************************************
 *          gravity_falls_emulate_ica_zero_unconsumed_area()
 ******************************************************************************
 *
 * @brief   Emulate the ICA process by zeroing the unconsumed space after the
 *          lun id specified to the end of the region specified.
 *
 *
 * @param   rg_object_id - The private raid group object id
 * @param   last_lun_in_rg_object_id - The object id of the last lun in the rg
 *
 * @return  None 
 *
 *****************************************************************************/
void gravity_falls_emulate_ica_zero_unconsumed_area(fbe_object_id_t rg_object_id,
                                                    fbe_object_id_t last_lun_in_rg_object_id)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_lun_get_lun_info_t      lun_info;
    fbe_private_space_layout_region_t region;
    fbe_api_raid_group_get_info_t   rg_info;
    fbe_u32_t                       slot;
    fbe_lba_t                       lba_to_clear;
    fbe_block_count_t               blocks_to_clear;
    fbe_object_id_t                 pvd_object_id;
    void fbe_test_sep_util_run_rdgen_for_pvd(fbe_object_id_t pvd_object_id, 
                                         fbe_rdgen_operation_t operation, 
                                         fbe_rdgen_pattern_t pattern,
                                         fbe_lba_t start_lba,
                                         fbe_block_count_t blocks);
    fbe_lba_t                       orig_paged_metadata_lba;
    fbe_lba_t                       last_lun_end_lba;
    fbe_u32_t                       rg_width;

    /* Get the region and raid group information prior to metadata shrink.
     */
    status = fbe_private_space_layout_get_region_by_object_id(rg_object_id, &region);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_raid_group_get_info(rg_object_id,
                                         &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    rg_width = rg_info.width;
    orig_paged_metadata_lba = rg_info.paged_metadata_start_lba;

    /* We MUST emulate the ICA process by writting zero to the `hole' from the 
     * last consumed block to the paged metadata.  Otherwise we will get get:
     * `terminator_simulated_disk_memory_populate_uninitialized_data' which
     * results in a multi-bit CRC error and the the drive being shot.
     */
    status = fbe_api_lun_get_lun_info(last_lun_in_rg_object_id, 
                                      &lun_info);
    last_lun_end_lba = lun_info.offset + lun_info.capacity;
    if (last_lun_end_lba < orig_paged_metadata_lba) {
        lba_to_clear = last_lun_end_lba / rg_info.num_data_disk;
        blocks_to_clear = (orig_paged_metadata_lba / rg_info.num_data_disk) - lba_to_clear + 1;
        lba_to_clear += region.starting_block_address;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "Simulate ICA process and zero `hole' from lun obj: 0x%x end pba: 0x%llx to paged pba: 0x%llx",
                   last_lun_in_rg_object_id, 
                   lba_to_clear, 
                   (lba_to_clear + blocks_to_clear - 1));
        for (slot = 0; slot < rg_width; slot++) {
            status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, slot, &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            fbe_test_sep_util_run_rdgen_for_pvd(pvd_object_id, FBE_RDGEN_OPERATION_WRITE_ONLY, FBE_RDGEN_PATTERN_ZEROS, 
                                                lba_to_clear, blocks_to_clear);
        }
    }

    return;
}
/********************************************************** 
 * end gravity_falls_emulate_ica_zero_unconsumed_area()
 **********************************************************/

/*!****************************************************************************
 *  gravity_falls_test_expand_4_drive_raid5
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the gravity_falls_simple_expand_test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void gravity_falls_test_expand_4_drive_raid5(void)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_region_t region;
    fbe_api_raid_group_get_info_t   rg_info;
    fbe_lba_t                       lba_to_clear;
    fbe_block_count_t               blocks_to_clear;
    fbe_api_raid_group_get_paged_info_t new_summary;
    fbe_lba_t                       orig_paged_metadata_lba;
    fbe_lba_t                       orig_paged_metadata_capacity;
    fbe_lba_t                       region_rg_capacity_aligned;
    fbe_api_rdgen_context_t         context;
    fbe_u32_t                       rg_width;
    fbe_time_t                      commit_start_ms;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Gravity Falls expand 4 drive R5 Test ===\n");

    /* Get the region and raid group information prior to metadata shrink.
     */
    status = fbe_private_space_layout_get_region_by_object_id(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG, &region);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG,
                                         &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    rg_width = rg_info.width;
    orig_paged_metadata_lba = rg_info.paged_metadata_start_lba;
    orig_paged_metadata_capacity = rg_info.paged_metadata_capacity;

    /* We MUST emulate the ICA process by writing zero to the `hole' from the 
     * last consumed block to the paged metadata.  Otherwise we will get get:
     * `terminator_simulated_disk_memory_populate_uninitialized_data' which
     * results in a multi-bit CRC error and the drive being shot.
     */
    /*! @note This is no longer required for (2) reasons:
     *          o When shrinking the 4D R5 there is no space remaining
     *          o We now re-zero the page metadata before re-constructing it.
     *            This the pre-reads for the reconstruction will not encounter
     *            CRC errors. 
     *        May use the routine in the future.
     *        (Writing of zeros is not the same as Zeroing so it resulted
     *         in 643MB of memory being consumed)
     */
    //gravity_falls_emulate_ica_zero_unconsumed_area(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG,
    //                                               FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_5);

    /* Set pattern on last LUN in RG, since this is the LUN closest to the end of the RG and 
     * most likely to be affected by any issued in expand. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Set and Check pattern on second small lun in RG, LUN object id: 0x%x",
               FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_WIL_B);
    gravity_falls_write_background_pattern(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_WIL_B);
    gravity_falls_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_WIL_B);

    /* Save the original alignment
     */
    region_rg_capacity_aligned = (region.raid_info.exported_capacity + (rg_info.lun_align_size - 1)) / rg_info.lun_align_size;
    region_rg_capacity_aligned *= rg_info.lun_align_size;

    /* Disable raid group paged metadata I/O until the shrink is complete.
     */
    status =  fbe_api_base_config_disable_background_operation(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG, 
                                                               FBE_RAID_GROUP_BACKGROUND_OP_ALL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Reduce capacity of 4D R5 to Rockies size.
     */
    commit_start_ms = fbe_get_time();
    gravity_falls_reduce_private_raid_group_capacity(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG, 0x51, commit_start_ms);

    /* Set the pvd area associated with the too be expanded area to the state that
     * ICA would have left it.
     */
    status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG,
                                         &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    blocks_to_clear = (orig_paged_metadata_lba + orig_paged_metadata_capacity) - (rg_info.paged_metadata_start_lba + rg_info.paged_metadata_capacity);
    blocks_to_clear = blocks_to_clear / rg_info.num_data_disk;
    lba_to_clear = (rg_info.paged_metadata_start_lba + rg_info.paged_metadata_capacity) / rg_info.num_data_disk;
    lba_to_clear += region.starting_block_address;
    gravity_falls_set_pvd_paged_pre_expansion((rg_width - 1), lba_to_clear, blocks_to_clear);

    /* Re-enabled background operation during expansion
     */
    status =  fbe_api_base_config_enable_background_operation(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG, 
                                                              FBE_RAID_GROUP_BACKGROUND_OP_ALL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Kick off I/O to one LUN in this RG during the expansion.
     * Our goal is to test that the I/O is properly quiesced and does not get errors. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "start I/O to object id: 0x%x", FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_WIL_A);
    gravity_falls_start_io(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_WIL_A, 
                           &context,
                           fbe_test_sep_util_get_dualsp_test_mode());

    mut_printf(MUT_LOG_TEST_STATUS, "expanding RG object: 0x%x to 0x%llx", 
               FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG,
               region.raid_info.exported_capacity);
    status = fbe_api_raid_group_expand(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG, 
                                       region.raid_info.exported_capacity, region.raid_info.exported_capacity);
    mut_printf(MUT_LOG_TEST_STATUS, "Expand status is: %d", status);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Stop the I/O we ran and check the status.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "stop I/O and check status");
    status = fbe_api_rdgen_stop_tests(&context, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context.start_io.statistics.error_count, 0);

    /* Make sure the paged looks correct.
     * The raid group is optimal so no verifies or rebuilds should be marked.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "validate paged bits after expansion");
    status = fbe_api_raid_group_get_paged_bits(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG, &new_summary, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (new_summary.num_error_vr_chunks != 0) {
        MUT_FAIL_MSG("error verify chunks present in new paged!!");
    }
    if (new_summary.needs_rebuild_bitmask != 0) {
        MUT_FAIL_MSG("nr is incorrect!!");
    }
    if (new_summary.num_nr_chunks != 0) {
        MUT_FAIL_MSG("nr is incorrect!!");
    }
    if (new_summary.pos_nr_chunks[0] != 0) {
        MUT_FAIL_MSG("nr is incorrect for pos 0!!");
    }
    if (new_summary.pos_nr_chunks[1] != 0) {
        MUT_FAIL_MSG("nr is incorrect for pos 1!!");
    }
    if (new_summary.pos_nr_chunks[2] != 0) {
        MUT_FAIL_MSG("nr is incorrect for pos 2!!");
    }
    if (new_summary.pos_nr_chunks[3] != 0) {
        MUT_FAIL_MSG("nr is incorrect for pos 3!!");
    }

    if (fbe_test_sep_util_get_dualsp_test_mode()){
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG,
                                             &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Make sure the paged is back to where it started.
         */
        if (rg_info.paged_metadata_start_lba != orig_paged_metadata_lba){
            mut_printf(MUT_LOG_TEST_STATUS, "paged is at 0x%llx not 0x%llx", 
                       rg_info.paged_metadata_start_lba, orig_paged_metadata_lba);
            MUT_FAIL_MSG("paged is not at expected lba after expansion");
        }
        if (rg_info.capacity != region_rg_capacity_aligned){
            mut_printf(MUT_LOG_TEST_STATUS, "capacity is 0x%llx not 0x%llx", 
                       rg_info.capacity, region_rg_capacity_aligned);
            MUT_FAIL_MSG("capacity is incorrect");
        }
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG,
                                         &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Make sure the paged is back to where it started.
     */
    if (rg_info.paged_metadata_start_lba != orig_paged_metadata_lba)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "paged is at 0x%llx not 0x%llx", 
                   rg_info.paged_metadata_start_lba, orig_paged_metadata_lba);
        MUT_FAIL_MSG("paged is not at expected lba after expansion");
    }
    if (rg_info.capacity != region_rg_capacity_aligned)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "capacity is 0x%llx not 0x%llx", 
                   rg_info.capacity, region_rg_capacity_aligned);
        MUT_FAIL_MSG("capacity is incorrect");
    }

    /* Make sure the final LUN in the RG is sane.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Check pattern on second small lun in RG.");
    gravity_falls_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_WIL_B);

    /* Kick off verify to mark the paged.
     */
    status = fbe_api_raid_group_initiate_verify(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG, FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_LUN_VERIFY_TYPE_USER_RW);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for incomplete write verify to complete.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for user read, write verify to complete rg: 0x%x==", 
               FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG);
    status = fbe_test_verify_wait_for_verify_completion(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG, 
                                               FBE_TEST_WAIT_TIMEOUT_MS, FBE_LUN_VERIFY_TYPE_USER_RW);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;

}/* End gravity_falls_test_expand_4_drive_raid5()

/*!****************************************************************************
 *  gravity_falls_test_expand_triple_mirror
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the gravity_falls_simple_expand_test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void gravity_falls_test_expand_triple_mirror(void)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_region_t region;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t slot;
    fbe_lba_t lba_to_clear;
    fbe_block_count_t blocks_to_clear;
    fbe_object_id_t pvd_object_id;
    fbe_u32_t monitor_substate;
    fbe_api_raid_group_get_paged_info_t original_summary;
    fbe_api_raid_group_get_paged_info_t new_summary;
    void fbe_test_sep_util_run_rdgen_for_pvd(fbe_object_id_t pvd_object_id, 
                                         fbe_rdgen_operation_t operation, 
                                         fbe_rdgen_pattern_t pattern,
                                         fbe_lba_t start_lba,
                                         fbe_block_count_t blocks);
    fbe_u32_t reduce_capacity_amount = 0x60000;
    fbe_lba_t reduced_capacity;
    fbe_lba_t orig_paged_metadata_lba;
    fbe_api_rdgen_context_t context;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Gravity Falls simple expand Test ===\n");

    /* Set pattern on last LUN in RG, since this is the LUN closest to the end of the RG and 
     * most likely to be affected by any issued in expand. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Set and Check pattern on last LUN in RG, LUN object id: 0x%x",
               FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX);
    gravity_falls_write_background_pattern(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX);
    gravity_falls_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX);

    status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                         &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    orig_paged_metadata_lba = rg_info.paged_metadata_start_lba;
    status = fbe_private_space_layout_get_region_by_object_id(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, &region);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    reduced_capacity = region.raid_info.exported_capacity - reduce_capacity_amount;

    /* Reduce capacity of tripple mirror by some constant amount.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "reducing RG object: 0x%x from 0x%llx to 0x%llx", 
               FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
               region.raid_info.exported_capacity,
               reduced_capacity);
	status = fbe_api_raid_group_expand(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, reduced_capacity, reduced_capacity);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
    
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                             &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (rg_info.capacity != reduced_capacity)
        {
            MUT_FAIL_MSG("capacity not reduced as expected");
        }
        if (rg_info.paged_metadata_start_lba >= orig_paged_metadata_lba)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "paged is at 0x%llx", 
                       rg_info.paged_metadata_start_lba);
            MUT_FAIL_MSG("paged is not at expected lba after expansion");
        }
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                         &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (rg_info.capacity != reduced_capacity)
    {
        MUT_FAIL_MSG("capacity not reduced as expected");
    }
    if (rg_info.paged_metadata_start_lba >= orig_paged_metadata_lba)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "paged is at 0x%llx", 
                   rg_info.paged_metadata_start_lba);
        MUT_FAIL_MSG("paged is not at expected lba after expansion");
    }

    /* Write to the Old paged region to clear it out.
     */
    blocks_to_clear = (region.size_in_blocks - rg_info.imported_blocks_per_disk) + 1;
    lba_to_clear = region.starting_block_address + rg_info.imported_blocks_per_disk;
    for (slot = 0; slot < 3; slot++) {
        status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, slot, &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_test_sep_util_run_rdgen_for_pvd(pvd_object_id, FBE_RDGEN_OPERATION_WRITE_ONLY, FBE_RDGEN_PATTERN_CLEAR, 
                                            lba_to_clear, blocks_to_clear);
    }

    /* Don't let verify proceed to the paged remains marked.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "mark for error verify on RG and use edge hook to prevent verify progress.");
    fbe_test_verify_get_verify_start_substate(FBE_LUN_VERIFY_TYPE_ERROR, &monitor_substate);
    status = fbe_test_add_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            monitor_substate,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Kick off verify to mark the paged.
     */
    status = fbe_api_raid_group_initiate_verify(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_LUN_VERIFY_TYPE_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure paged has the bits we set.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "validate paged bits before expansion");
    status = fbe_api_raid_group_get_paged_bits(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, &original_summary, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (original_summary.num_error_vr_chunks == 0) {
        MUT_FAIL_MSG("error verify chunks not present!!");
    }
    if (original_summary.needs_rebuild_bitmask != 0) {
        MUT_FAIL_MSG("nr is incorrect!!");
    }

    /* Pull a drive and wait for it to go away.
     */
    gravity_falls_pull_drive(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                             0, 0, 0,
                             FBE_TRUE /* Wait for drive to Fail on both SPs*/);
    status = gravity_falls_wait_for_rg_degraded(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, FBE_TEST_WAIT_TIMEOUT_MS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Kick off I/O to one LUN in this RG during the expansion.
     * Our goal is to test that the I/O is properly quiesced and does not get errors. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "start I/O to object id: 0x%x", FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_EFD_CACHE);
    gravity_falls_start_io(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_EFD_CACHE, 
                           &context,
                           fbe_test_sep_util_get_dualsp_test_mode());

    mut_printf(MUT_LOG_TEST_STATUS, "expanding RG object: 0x%x to 0x%llx", 
               FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
               region.size_in_blocks);
    status = fbe_api_raid_group_expand(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, 
                                       region.raid_info.exported_capacity, region.size_in_blocks);
    mut_printf(MUT_LOG_TEST_STATUS, "Expand status is: %d", status);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Stop the I/O we ran and check the status.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "stop I/O and check status");
    status = fbe_api_rdgen_stop_tests(&context, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context.start_io.statistics.error_count, 0);

    /* Make sure the paged looks correct.
     * It should have error verify marked and nr marked for position 0. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "validate paged bits after expansion");
    status = fbe_api_raid_group_get_paged_bits(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, &new_summary, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (new_summary.num_error_vr_chunks != new_summary.chunk_count) {
        MUT_FAIL_MSG("error verify chunks not present in new paged!!");
    }
    if (new_summary.needs_rebuild_bitmask != 1) {
        MUT_FAIL_MSG("nr is incorrect!!");
    }
    if (new_summary.num_nr_chunks != new_summary.chunk_count) {
        MUT_FAIL_MSG("nr is incorrect!!");
    }
    if (new_summary.pos_nr_chunks[0] != new_summary.chunk_count) {
        MUT_FAIL_MSG("nr is incorrect for pos 0!!");
    }
    if (new_summary.pos_nr_chunks[1] != 0) {
        MUT_FAIL_MSG("nr is incorrect for pos 1!!");
    }
    if (new_summary.pos_nr_chunks[2] != 0) {
        MUT_FAIL_MSG("nr is incorrect for pos 2!!");
    }

    if (fbe_test_sep_util_get_dualsp_test_mode()){
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                             &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Make sure the paged is back to where it started.
         */
        if (rg_info.paged_metadata_start_lba != orig_paged_metadata_lba){
            mut_printf(MUT_LOG_TEST_STATUS, "paged is at 0x%llx not 0x%llx", 
                       rg_info.paged_metadata_start_lba, orig_paged_metadata_lba);
            MUT_FAIL_MSG("paged is not at expected lba after expansion");
        }
        if (rg_info.capacity != region.raid_info.exported_capacity){
            mut_printf(MUT_LOG_TEST_STATUS, "capacity is 0x%llx not 0x%llx", 
                       rg_info.capacity, region.raid_info.exported_capacity);
            MUT_FAIL_MSG("capacity is incorrect");
        }
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                         &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Make sure the paged is back to where it started.
     */
    if (rg_info.paged_metadata_start_lba != orig_paged_metadata_lba)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "paged is at 0x%llx not 0x%llx", 
                   rg_info.paged_metadata_start_lba, orig_paged_metadata_lba);
        MUT_FAIL_MSG("paged is not at expected lba after expansion");
    }
    if (rg_info.capacity != region.raid_info.exported_capacity)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "capacity is 0x%llx not 0x%llx", 
                   rg_info.capacity, region.raid_info.exported_capacity);
        MUT_FAIL_MSG("capacity is incorrect");
    }

    /* Make sure the final LUN in the RG is sane.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Check pattern on last LUN in RG.");
    gravity_falls_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX);

    status = fbe_test_del_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            monitor_substate,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;

}/* gravity_falls_test_expand_triple_mirror() */
/*!****************************************************************************
 *  gravity_falls_test_expand_triple_mirror_active_error
 ******************************************************************************
 * @brief
 *  Test that an error on the active side during expansion can be handled.
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void gravity_falls_test_expand_triple_mirror_active_error(void)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_region_t region;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t slot;
    fbe_lba_t lba_to_clear;
    fbe_block_count_t blocks_to_clear;
    fbe_object_id_t pvd_object_id;
    fbe_u32_t monitor_substate;
    fbe_api_raid_group_get_paged_info_t original_summary;
    fbe_api_raid_group_get_paged_info_t new_summary;
    void fbe_test_sep_util_run_rdgen_for_pvd(fbe_object_id_t pvd_object_id, 
                                         fbe_rdgen_operation_t operation, 
                                         fbe_rdgen_pattern_t pattern,
                                         fbe_lba_t start_lba,
                                         fbe_block_count_t blocks);
    fbe_u32_t reduce_capacity_amount = 0x60000;
    fbe_lba_t reduced_capacity;
    fbe_lba_t reduced_paged_metadata_lba;
    fbe_lba_t orig_paged_metadata_lba;
    fbe_api_rdgen_context_t context;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Gravity Falls active error Test ===\n");

    /* Set pattern on last LUN in RG, since this is the LUN closest to the end of the RG and 
     * most likely to be affected by any issued in expand. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Set and Check pattern on last LUN in RG, LUN object id: 0x%x",
               FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX);
    gravity_falls_write_background_pattern(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX);
    gravity_falls_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX);

    status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                         &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    orig_paged_metadata_lba = rg_info.paged_metadata_start_lba;
    status = fbe_private_space_layout_get_region_by_object_id(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, &region);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    reduced_capacity = region.raid_info.exported_capacity - reduce_capacity_amount;

    /* Reduce capacity of tripple mirror by some constant amount.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "reducing RG object: 0x%x from 0x%llx to 0x%llx",
               FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
               region.raid_info.exported_capacity,
               reduced_capacity);
	status = fbe_api_raid_group_expand(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, reduced_capacity, reduced_capacity);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (fbe_test_sep_util_get_dualsp_test_mode()){
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                             &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (rg_info.capacity != reduced_capacity){
            MUT_FAIL_MSG("capacity not reduced as expected");
        }
        if (rg_info.paged_metadata_start_lba >= orig_paged_metadata_lba){
            mut_printf(MUT_LOG_TEST_STATUS, "paged is at 0x%llx", 
                       rg_info.paged_metadata_start_lba);
            MUT_FAIL_MSG("paged is not at expected lba after expansion");
        }
    }
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                         &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (rg_info.capacity != reduced_capacity)
    {
        MUT_FAIL_MSG("capacity not reduced as expected");
    }
    if (rg_info.paged_metadata_start_lba >= orig_paged_metadata_lba)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "paged is at 0x%llx", 
                   rg_info.paged_metadata_start_lba);
        MUT_FAIL_MSG("paged is not at expected lba after expansion");
    }
    reduced_paged_metadata_lba = rg_info.paged_metadata_start_lba;

    /* Write to the Old paged region to clear it out.
     */
    blocks_to_clear = (region.size_in_blocks - rg_info.imported_blocks_per_disk) + 1;
    lba_to_clear = region.starting_block_address + rg_info.imported_blocks_per_disk;
    /* The 0th drive is still removed from the first test.  Just write drives 2-3.
     */
    for (slot = 1; slot < 3; slot++) {
        status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, slot, &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_test_sep_util_run_rdgen_for_pvd(pvd_object_id, FBE_RDGEN_OPERATION_WRITE_ONLY, FBE_RDGEN_PATTERN_CLEAR, 
                                            lba_to_clear, blocks_to_clear);
    }

    /* Don't let verify proceed to the paged remains marked.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "mark for error verify on RG and use edge hook to prevent verify progress.");
    fbe_test_verify_get_verify_start_substate(FBE_LUN_VERIFY_TYPE_ERROR, &monitor_substate);
    status = fbe_test_add_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            monitor_substate,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Kick off verify to mark the paged.
     */
    status = fbe_api_raid_group_initiate_verify(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_LUN_VERIFY_TYPE_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure paged has the bits we set.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "validate paged bits before expansion");
    status = fbe_api_raid_group_get_paged_bits(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, &original_summary, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (original_summary.num_error_vr_chunks == 0) {
        MUT_FAIL_MSG("error verify chunks not present!!");
    }
    if (original_summary.needs_rebuild_bitmask != 1) {
        MUT_FAIL_MSG("nr is incorrect!!");
    }

    /* Add a hook that will cause the expand to fail on the active side.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "enable error for expansion. ");
    status = fbe_test_add_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                                            FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_ERROR_ACTIVE,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Kick off I/O to one LUN in this RG during the expansion.
     * Our goal is to test that the I/O is properly quiesced and does not get errors. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "start I/O to object id: 0x%x", FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_EFD_CACHE);
    gravity_falls_start_io(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_EFD_CACHE, 
                           &context,
                           fbe_test_sep_util_get_dualsp_test_mode());

    mut_printf(MUT_LOG_TEST_STATUS, "expanding RG object: 0x%x to 0x%llx.", 
               FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
               region.size_in_blocks);
    status = fbe_api_raid_group_expand(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, 
                                       region.raid_info.exported_capacity, region.size_in_blocks);
    mut_printf(MUT_LOG_TEST_STATUS, "Expand status is: %d", status);

    /* Stop the I/O we ran and check the status.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "stop I/O and check status");
    status = fbe_api_rdgen_stop_tests(&context, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context.start_io.statistics.error_count, 0);

    /* Make sure the paged looks correct.
     * It should have error verify marked and nr marked for position 0. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "validate paged bits after expansion");
    status = fbe_api_raid_group_get_paged_bits(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, &new_summary, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (new_summary.chunk_count != original_summary.chunk_count) {
        MUT_FAIL_MSG("chunk count unexpected!!");
    }
    if (new_summary.num_error_vr_chunks != original_summary.num_error_vr_chunks) {
        MUT_FAIL_MSG("error verify chunks not present in new paged!!");
    }
    if (new_summary.needs_rebuild_bitmask != 1) {
        MUT_FAIL_MSG("nr is incorrect!!");
    }
    if (new_summary.num_nr_chunks != original_summary.num_nr_chunks) {
        MUT_FAIL_MSG("nr is incorrect!!");
    }
    if (new_summary.pos_nr_chunks[0] != original_summary.pos_nr_chunks[0]) {
        MUT_FAIL_MSG("nr is incorrect for pos 0!!");
    }
    if (new_summary.pos_nr_chunks[1] != original_summary.pos_nr_chunks[1]) {
        MUT_FAIL_MSG("nr is incorrect for pos 1!!");
    }
    if (new_summary.pos_nr_chunks[2] != original_summary.pos_nr_chunks[2]) {
        MUT_FAIL_MSG("nr is incorrect for pos 2!!");
    }
    if (fbe_test_sep_util_get_dualsp_test_mode()){
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                             &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Make sure the paged is back to where it started.
         */
        if (rg_info.paged_metadata_start_lba != reduced_paged_metadata_lba){
            mut_printf(MUT_LOG_TEST_STATUS, "paged is at 0x%llx not 0x%llx", 
                       rg_info.paged_metadata_start_lba, reduced_paged_metadata_lba);
            MUT_FAIL_MSG("paged is not at expected lba after expansion");
        }
        if (rg_info.capacity != reduced_capacity){
            mut_printf(MUT_LOG_TEST_STATUS, "capacity is 0x%llx not 0x%llx", 
                       rg_info.capacity, reduced_capacity);
            MUT_FAIL_MSG("capacity is incorrect after expansion");
        }
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                         &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Make sure the paged is back to where it started.
     */
    if (rg_info.paged_metadata_start_lba != reduced_paged_metadata_lba){
        mut_printf(MUT_LOG_TEST_STATUS, "paged is at 0x%llx not 0x%llx", 
                   rg_info.paged_metadata_start_lba, reduced_paged_metadata_lba);
        MUT_FAIL_MSG("paged is not at expected lba after expansion");
    }
    if (rg_info.capacity != reduced_capacity){
        mut_printf(MUT_LOG_TEST_STATUS, "capacity is 0x%llx not 0x%llx", 
                   rg_info.capacity, region.raid_info.exported_capacity);
        MUT_FAIL_MSG("capacity is incorrect after expansion");
    }

    /* Make sure the final LUN in the RG is sane.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Check pattern on last LUN in RG.");
    gravity_falls_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX);

    status = fbe_test_del_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            monitor_substate,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_del_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                                            FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_ERROR_ACTIVE,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;

}/* gravity_falls_test_expand_triple_mirror_active_error() */
/*!****************************************************************************
 *  gravity_falls_test_expand_triple_mirror_passive_error
 ******************************************************************************
 * @brief
 *  Test that an error on the passive side during expansion can be handled.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void gravity_falls_test_expand_triple_mirror_passive_error(void)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_region_t region;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t monitor_substate;
    fbe_api_raid_group_get_paged_info_t original_summary;
    fbe_api_raid_group_get_paged_info_t new_summary;
    void fbe_test_sep_util_run_rdgen_for_pvd(fbe_object_id_t pvd_object_id, 
                                         fbe_rdgen_operation_t operation, 
                                         fbe_rdgen_pattern_t pattern,
                                         fbe_lba_t start_lba,
                                         fbe_block_count_t blocks);
    fbe_u32_t reduce_capacity_amount = 0x60000;
    fbe_lba_t reduced_capacity;
    fbe_lba_t reduced_paged_metadata_lba;
    fbe_lba_t orig_paged_metadata_lba;
    fbe_api_rdgen_context_t context;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Gravity Falls passive error Test ===\n");

    /* Set pattern on last LUN in RG, since this is the LUN closest to the end of the RG and 
     * most likely to be affected by any issued in expand. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Set and Check pattern on last LUN in RG, LUN object id: 0x%x",
               FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX);
    gravity_falls_write_background_pattern(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX);
    gravity_falls_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX);

    status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                         &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    orig_paged_metadata_lba = rg_info.paged_metadata_start_lba;
    status = fbe_private_space_layout_get_region_by_object_id(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, &region);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    reduced_capacity = region.raid_info.exported_capacity - reduce_capacity_amount;

    /* Validate the capacity is as expected.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Validate capacity of object: 0x%x is 0x%llx", 
               FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
               reduced_capacity);
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                         &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (rg_info.capacity != reduced_capacity){
        MUT_FAIL_MSG("capacity not reduced as expected");
    }
    reduced_paged_metadata_lba = rg_info.paged_metadata_start_lba;

    /* Don't let verify proceed to the paged remains marked.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "mark for error verify on RG and use edge hook to prevent verify progress.");
    fbe_test_verify_get_verify_start_substate(FBE_LUN_VERIFY_TYPE_ERROR, &monitor_substate);
    status = fbe_test_add_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            monitor_substate,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Kick off verify to mark the paged.
     */
    status = fbe_api_raid_group_initiate_verify(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_LUN_VERIFY_TYPE_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure paged has the bits we set.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "validate paged bits before expansion");
    status = fbe_api_raid_group_get_paged_bits(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, &original_summary, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (original_summary.num_error_vr_chunks == 0) {
        MUT_FAIL_MSG("error verify chunks not present!!");
    }
    if (original_summary.needs_rebuild_bitmask != 1) {
        MUT_FAIL_MSG("nr is incorrect!!");
    }

    /* Add a hook that will cause the expand to fail on the active side.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "enable error for expansion. ");
    status = fbe_test_add_debug_hook_passive(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                                            FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_ERROR_PASSIVE,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Kick off I/O to one LUN in this RG during the expansion.
     * Our goal is to test that the I/O is properly quiesced and does not get errors. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "start I/O to object id: 0x%x", FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_EFD_CACHE);
    gravity_falls_start_io(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_EFD_CACHE, 
                           &context,
                           fbe_test_sep_util_get_dualsp_test_mode());

    mut_printf(MUT_LOG_TEST_STATUS, "expanding RG object: 0x%x to 0x%llx.", 
               FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
               region.size_in_blocks);
    status = fbe_api_raid_group_expand(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, 
                                       region.raid_info.exported_capacity, region.size_in_blocks);
    mut_printf(MUT_LOG_TEST_STATUS, "Expand status is: %d", status);

    /* Stop the I/O we ran and check the status.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "stop I/O and check status");
    status = fbe_api_rdgen_stop_tests(&context, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context.start_io.statistics.error_count, 0);

    /* Make sure the paged looks correct.
     * It should have error verify marked and nr marked for position 0. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "validate paged bits after expansion");
    status = fbe_api_raid_group_get_paged_bits(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, &new_summary, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (new_summary.chunk_count != original_summary.chunk_count) {
        MUT_FAIL_MSG("chunk count unexpected!!");
    }
    if (new_summary.num_error_vr_chunks != original_summary.num_error_vr_chunks) {
        MUT_FAIL_MSG("error verify chunks not present in new paged!!");
    }
    if (new_summary.needs_rebuild_bitmask != 1) {
        MUT_FAIL_MSG("nr is incorrect!!");
    }
    if (new_summary.num_nr_chunks != original_summary.num_nr_chunks) {
        MUT_FAIL_MSG("nr is incorrect!!");
    }
    if (new_summary.pos_nr_chunks[0] != original_summary.pos_nr_chunks[0]) {
        MUT_FAIL_MSG("nr is incorrect for pos 0!!");
    }
    if (new_summary.pos_nr_chunks[1] != original_summary.pos_nr_chunks[1]) {
        MUT_FAIL_MSG("nr is incorrect for pos 1!!");
    }
    if (new_summary.pos_nr_chunks[2] != original_summary.pos_nr_chunks[2]) {
        MUT_FAIL_MSG("nr is incorrect for pos 2!!");
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                         &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Make sure the paged is back to where it started.
     */
    if (rg_info.paged_metadata_start_lba != reduced_paged_metadata_lba){
        mut_printf(MUT_LOG_TEST_STATUS, "paged is at 0x%llx not 0x%llx", 
                   rg_info.paged_metadata_start_lba, reduced_paged_metadata_lba);
        MUT_FAIL_MSG("paged is not at expected lba after expansion");
    }
    if (rg_info.capacity != reduced_capacity){
        mut_printf(MUT_LOG_TEST_STATUS, "capacity is 0x%llx not 0x%llx", 
                   rg_info.capacity, reduced_capacity);
        MUT_FAIL_MSG("capacity is incorrect after expansion");
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                         &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Make sure the paged is back to where it started.
     */
    if (rg_info.paged_metadata_start_lba != reduced_paged_metadata_lba)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "paged is at 0x%llx not 0x%llx", 
                   rg_info.paged_metadata_start_lba, reduced_paged_metadata_lba);
        MUT_FAIL_MSG("paged is not at expected lba after expansion");
    }
    if (rg_info.capacity != reduced_capacity)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "capacity is 0x%llx not 0x%llx", 
                   rg_info.capacity, region.raid_info.exported_capacity);
        MUT_FAIL_MSG("capacity is incorrect after expansion");
    }

    /* Make sure the final LUN in the RG is sane.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Check pattern on last LUN in RG.");
    gravity_falls_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX);

    status = fbe_test_del_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            monitor_substate,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_del_debug_hook_passive(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
                                            FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_ERROR_PASSIVE,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;

}/* End gravity_falls_test_expand_triple_mirror_passive_error() */

/*!****************************************************************************
 *  gravity_falls_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the gravity_falls_test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void gravity_falls_test(void)
{
#ifndef __SAFE__
    // no FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_WIL_B on KH
    gravity_falls_test_expand_4_drive_raid5();
#endif
    gravity_falls_test_expand_triple_mirror();
    gravity_falls_test_expand_triple_mirror_active_error();
    return;
}/* End gravity_falls_test() */

/*!****************************************************************************
 *  gravity_falls_dualsp_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the gravity_falls_dualsp_test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void gravity_falls_dualsp_test(void)
{
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    /*! @note gravity_falls_test_expand_4_drive_raid5 MUST run prior to
     *        gravity_falls_test_dualsp_validate_expansion_during_commit.
     */
#ifndef __SAFE__
    // no FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_WIL_B on KH
    gravity_falls_test_expand_4_drive_raid5();
#endif
    gravity_falls_test_dualsp_validate_expansion_during_commit();
    gravity_falls_test_expand_triple_mirror();
    gravity_falls_test_expand_triple_mirror_active_error();
    gravity_falls_test_expand_triple_mirror_passive_error();
    return;
}/* End gravity_falls_dualsp_test() */

/*!**************************************************************
 * gravity_falls_start_io()
 ****************************************************************
 * @brief
 *  start I/O.
 *
 * @param object_id
 * @param context_p
 * @param b_dualsp_mode     
 *
 * @return None.
 *
 * @author
 *  10/22/2013 - Created. Rob Foley
 *
 ****************************************************************/

static void gravity_falls_start_io(fbe_object_id_t object_id,
                                   fbe_api_rdgen_context_t *context_p,
                                   fbe_bool_t b_dualsp_mode)
{
    fbe_status_t status;

    /* Start write read compare I/O.
     */
    status = fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                                               object_id,
                                                               FBE_CLASS_ID_INVALID,
                                                               FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                                               FBE_LBA_INVALID,    /* use capacity */
                                                               0,    /* run forever*/ 
                                                               16,    /* threads */
                                                               FBE_RAID_MAX_BE_XFER_SIZE,
                                                               FBE_FALSE,    /* no aborts*/ 
                                                               b_dualsp_mode);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end gravity_falls_start_io()
 ******************************************/

/*!**************************************************************
 * gravity_falls_write_background_pattern()
 ****************************************************************
 * @brief
 *  Seed one object with a pattern.
 *
 * @param object_id - object id to start to.             
 *
 * @return None.
 *
 * @author
 *  10/22/2013 - Created. Rob Foley
 *
 ****************************************************************/

static void gravity_falls_write_background_pattern(fbe_object_id_t object_id)
{
    fbe_api_rdgen_context_t context;
    fbe_status_t status;

    /* Write a background pattern to the LUNs.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(&context,
                                            object_id,
                                            FBE_CLASS_ID_INVALID,
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            GRAVITY_FALLS_TEST_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(&context, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context.start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_NOT_EQUAL(context.start_io.statistics.io_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(&context,
                                            object_id,
                                            FBE_CLASS_ID_INVALID,
                                            FBE_RDGEN_OPERATION_READ_CHECK,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            GRAVITY_FALLS_TEST_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(&context, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context.start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end gravity_falls_write_background_pattern()
 ******************************************/
/*!**************************************************************
 * gravity_falls_read_background_pattern()
 ****************************************************************
 * @brief
 *  Check all luns for a pattern.
 *
 * @param pattern - the pattern to check for.  
 * @param object_id - The one object to check.
 *
 * @return None.
 *
 * @author
 *  10/22/2013 - Created. Rob Foley
 *
 ****************************************************************/

static void gravity_falls_read_background_pattern(fbe_rdgen_pattern_t pattern,
                                                  fbe_object_id_t object_id)
{
    fbe_api_rdgen_context_t context;
    fbe_status_t status;

    /* Read back the pattern and check it was written OK.
     */
    handy_manny_test_setup_fill_rdgen_test_context(&context,
                                                   object_id,
                                                   FBE_CLASS_ID_INVALID,
                                                   FBE_RDGEN_OPERATION_READ_CHECK,
                                                   FBE_LBA_INVALID, /* use max capacity */
                                                   GRAVITY_FALLS_TEST_ELEMENT_SIZE,
                                                   pattern,
                                                   1, /* passes */
                                                   0 /* I/O count not used */);
    status = fbe_api_rdgen_test_context_run_all_tests(&context, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context.start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end gravity_falls_read_background_pattern()
 ******************************************/
/*!**************************************************************
 * gravity_falls_wait_for_rg_degraded()
 ****************************************************************
 * @brief
 *  Wait for raid group to be degraded with certain flags set.
 *
 * @param rg_object_id - Object id to wait on flags for
 * @param wait_seconds - max seconds to wait for the flags to change.
 * @param flags - Flag value we are expecting.
 * @param b_cleared - FBE_TRUE if we want to wait for this flag
 *                    to get cleared.
 *                    FBE_FALSE to wait for this flag to get set.
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  10/22/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t gravity_falls_wait_for_rg_degraded(fbe_object_id_t rg_object_id,
                                                       fbe_u32_t wait_seconds)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_seconds * 1000;
    fbe_base_config_clustered_flags_t flags = FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCE_MASK;
    fbe_api_get_block_edge_info_t edge_info;
    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < target_wait_msec)
    {
        status = fbe_api_get_block_edge_info(rg_object_id, 0, &edge_info, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if (edge_info.path_state == FBE_PATH_STATE_BROKEN) {

            status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Check the flags and degraded bitmask.
             */
            if ( (rg_info.base_config_clustered_flags & flags) == 0 &&
                 (rg_info.b_rb_logging[0] == FBE_TRUE) &&
                 (rg_info.rebuild_checkpoint[0] == 0) ) {
                /* Flags are cleared, just break out and return.
                 */
                break;
            }
        }
        fbe_api_sleep(1000);
        elapsed_msec += 1000;
    }

    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= (wait_seconds * 1000)) {
        status = FBE_STATUS_TIMEOUT;
    }
    return status;
}
/******************************************
 * end gravity_falls_wait_for_rg_degraded()
 ******************************************/
/*!**************************************************************
 * gravity_falls_test_create_lun()
 ****************************************************************
 * @brief
 *  Lun Creation function for gravity_falls_test.
 *
 * @param fbe_lun_number_t lun_number
 *        fbe_lba_t        capacity
 *
 * @return fbe_status_t    status
 *
 ****************************************************************/
static fbe_status_t gravity_falls_test_create_lun(fbe_lun_number_t lun_number, fbe_lba_t capacity)
{
    fbe_status_t                    status;
    fbe_api_lun_create_t            fbe_lun_create_req;
    fbe_object_id_t                 lu_id; 
    fbe_job_service_error_type_t    job_error_type;
     
    // Create a LUN
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_lun_create_req.raid_type = gravity_falls_rg_config[0].raid_type;
    fbe_lun_create_req.raid_group_id = gravity_falls_rg_config[0].raid_group_id; /*sep_standard_logical_config_one_r5 is 5 so this is what we use*/
    fbe_lun_create_req.lun_number = lun_number;
    fbe_lun_create_req.capacity = capacity;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */

    status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, GRAVITY_FALLS_TEST_NS_TIMEOUT, &lu_id, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   

    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X created successfully! \n", __FUNCTION__, lu_id);

    return status;   
} /* End gravity_falls_test_create_lun() */



/*!**************************************************************
 * gravity_falls_test_destroy_lun()
 ****************************************************************
 * @brief
 *  Lun destroy function for gravity_falls_test.
 *
 * @param fbe_lun_number_t lun_number
 *
 * @return fbe_status_t    status
 *
 ****************************************************************/
static fbe_status_t gravity_falls_test_destroy_lun(fbe_lun_number_t lun_number)
{
    fbe_status_t                            status;
    fbe_api_lun_destroy_t                   fbe_lun_destroy_req;
    fbe_job_service_error_type_t            job_error_type;
    
    // Destroy a LUN
    fbe_lun_destroy_req.lun_number = lun_number;

    status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE, GRAVITY_FALLS_TEST_NS_TIMEOUT, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, " LUN destroyed successfully! \n");

    return status;
}/* End gravity_falls_test_destroy_lun() */


/*!**************************************************************
 * gravity_falls_test_create_rg()
 ****************************************************************
 * @brief
 *  Configure the gravity falls test logical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void gravity_falls_test_create_rg(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_status_t                        job_status = FBE_STATUS_OK;
    fbe_u32_t                           rg_index;

    // Create RGs using gravity_falls_rg_config
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create RAID Groups ===\n");

    for (rg_index = 0; rg_index < GRAVITY_FALLS_TEST_RAID_GROUP_COUNT; rg_index++)
    {
        status = fbe_test_sep_util_create_raid_group(&gravity_falls_rg_config[rg_index]);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

        // wait for notification from job service. 
        status = fbe_api_common_wait_for_job(gravity_falls_rg_config[rg_index].job_number,
                                             GRAVITY_FALLS_TEST_NS_TIMEOUT,
                                             &job_error_code,
                                             &job_status,
                                             NULL);

        MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
        MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

        // Verify the object id of the raid group 
        status = fbe_api_database_lookup_raid_group_by_number(gravity_falls_rg_config[rg_index].raid_group_id, &rg_object_id);

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

        // Verify the raid group get to ready state in reasonable time 
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                         FBE_LIFECYCLE_STATE_READY, GRAVITY_FALLS_TEST_NS_TIMEOUT,
                                                         FBE_PACKAGE_ID_SEP_0);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End gravity_falls_test_create_rg() */


/*
 * The following functions are utility functions used by this test suite
 */

/*!**************************************************************
 * gravity_falls_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the gravity_falls test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void gravity_falls_test_load_physical_config(void)
{

   
    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      encl0_0_handle;
    fbe_api_terminator_device_handle_t      encl0_1_handle;
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;


    /* Initialize local variables */
    status          = FBE_STATUS_GENERIC_FAILURE;
    object_handle   = 0;
    port_idx        = 0;
    enclosure_idx   = 0;
    drive_idx       = 0;
    total_objects   = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== configuring terminator ===\n");
    /* before loading the physical package we initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(   1, /* io port */
                                            2, /* portal */
                                            0, /* backend number */ 
                                            &port0_handle);

    /* Insert enclosures to port 0
     */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 1, port0_handle, &encl0_1_handle);

    /* Insert drives for enclosures.
     */
    for ( slot = 0; slot < 4; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, GRAVITY_FALLS_TEST_DRIVE_CAPACITY, &drive_handle);
    }
    for ( slot = 4; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, GRAVITY_FALLS_TEST_DRIVE_CAPACITY, &drive_handle);
    }
    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, GRAVITY_FALLS_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== set physical package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(GRAVITY_FALLS_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");

    /* Inserted a armada board so we check for it;
     * board is always assumed to be object id 0
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == GRAVITY_FALLS_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;
} /* End gravity_falls_test_load_physical_config() */


/*!****************************************************************************
 *  gravity_falls_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for kungfu_pand_test in dualsp mode. It is responsible
 *  for loading the physical configuration for this test suite on both SPs.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void gravity_falls_dualsp_setup(void)
{
    fbe_status_t status;
    fbe_sim_transport_connection_target_t   sp;

    if (fbe_test_util_is_simulation())
    { 
        mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Gravity Falls dualsp test ===\n");
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        /* Load the physical config on the target server */
        gravity_falls_test_load_physical_config();
    
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    
        /* Load the physical config on the target server */    
        gravity_falls_test_load_physical_config();

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
        fbe_test_wait_for_all_pvds_ready();
    }
    
    status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_TRUE, /* yes dual sp. */ 
                                                      FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;

} /* End gravity_falls_dualsp_setup() */

/*!****************************************************************************
 *  gravity_falls_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for gravity falls in single sp mode.
 *  It is responsible for loading the physical configuration for this test suite on both SPs.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void gravity_falls_setup(void)
{
    fbe_sim_transport_connection_target_t   sp;

    if (fbe_test_util_is_simulation())
    { 
        mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Gravity Falls test ===\n");
    
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    
        /* Load the physical config on the target server */    
        gravity_falls_test_load_physical_config();

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_load_sep_and_neit();
        fbe_test_wait_for_all_pvds_ready();
    }
    
    return;

} /* End gravity_falls_setup() */

/*!****************************************************************************
 *  gravity_falls_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the gravity_falls_dualsp_test.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void gravity_falls_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for gravity_falls_dualsp_test ===\n");
	
	fbe_test_sep_util_destroy_neit_sep_physical_both_sps();

    return;

} /* End gravity_falls_dualsp_test_cleanup() */


/*!****************************************************************************
 *  gravity_falls_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the gravity_falls_dualsp_test.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void gravity_falls_cleanup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for gravity_falls_dualsp_test ===\n");
	
	fbe_test_sep_util_destroy_neit_sep_physical();

    return;

} /* End gravity_falls_dualsp_test_cleanup() */
